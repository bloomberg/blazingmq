# Copyright 2026 Bloomberg Finance L.P.
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""
Fuzz testing for BlazingMQ message properties in PUT messages.

Performs session setup once over a raw TCP connection, then sends many PUT
messages with fuzzed MessageProperties fields to verify the broker handles
malformed properties without crashing.

Public functions:
o 'fuzz_properties': launch a message-properties fuzzing session against a
                     BlazingMQ Broker at the given 'host':'port'.
"""

import logging
import socket
import struct
import time

import boofuzz
import crc32c
from boofuzz.mutation_context import MutationContext

from blazingmq.dev.fuzztest import (
    NumBytes,
    make_authentication_message,
    make_control_message,
)
from blazingmq.schemas import broker

logger = logging.getLogger(__name__)

# =============================================================================
#                               CONSTANTS
# =============================================================================

RECV_TIMEOUT = 0.1
CONNECT_RETRY_DELAY = 0.5
CONNECT_MAX_RETRIES = 10
PROPS_HEADER_SIZE_2X = 3  # 6 bytes / 2
MPH_SIZE_2X = 3  # 6 bytes / 2
MPH_SIZE = 6  # bytes
PROPS_HEADER_SIZE = 6  # bytes

PROP_TYPE_STRING = 6  # bmqt::PropertyType::e_STRING
PROP_TYPE_INT32 = 4  # bmqt::PropertyType::e_INT32
PROP_TYPE_BOOL = 1  # bmqt::PropertyType::e_BOOL
PROP_TYPE_INT64 = 5  # bmqt::PropertyType::e_INT64

# Pool of properties to draw from when generating multi-property messages.
# Each entry: (type_enum, name_bytes, value_bytes)
PROPERTY_POOL = [
    (PROP_TYPE_STRING, b"key", b"val"),
    (PROP_TYPE_INT32, b"id", struct.pack(">i", 42)),
    (PROP_TYPE_BOOL, b"flag", b"\x01"),
    (PROP_TYPE_INT64, b"ts", struct.pack(">q", 1234567890)),
    (PROP_TYPE_STRING, b"name", b"fuzz"),
    (PROP_TYPE_INT32, b"cnt", struct.pack(">i", 0)),
    (PROP_TYPE_STRING, b"tag", b"ab"),
    (PROP_TYPE_BOOL, b"ok", b"\x00"),
    (PROP_TYPE_INT64, b"big", struct.pack(">q", -1)),
    (PROP_TYPE_STRING, b"msg", b"hello"),
]

PROPERTY_COUNTS_TO_TEST = [1,2]


# =============================================================================
#                         MESSAGE PROPERTIES BUILDERS
# =============================================================================


def _make_mph_fields(prefix, prop_type, name, value):
    """Build the 3 boofuzz Words for a MessagePropertyHeader + name/value data."""
    value_len = len(value)
    value_len_upper = (value_len >> 16) & 0x3FF
    type_and_upper = (prop_type << 10) | value_len_upper

    return [
        # Bytes 0-1: [R(1) | PropType(5) | PropValueLenUpper(10)]
        boofuzz.Word(
            name=f"{prefix}_type_and_val_upper",
            default_value=type_and_upper,
            endian=">",
            fuzzable=False
        ),
        # Bytes 2-3: PropValueLenLower (16 bits)
        boofuzz.Word(
            name=f"{prefix}_val_lower",
            default_value=value_len & 0xFFFF,
            endian=">",
            fuzzable=False
        ),
        # Bytes 4-5: [R2(4) | PropNameLen(12)]
        boofuzz.Word(
            name=f"{prefix}_name_len",
            default_value=len(name) & 0x0FFF,
            endian=">",
            fuzzable=False
        ),
    ]


def _make_mph_fields_new_style(prefix, prop_type, name_len, cumulative_offset):
    """Build the 3 boofuzz Words for a MessagePropertyHeader using new-style
    (offset-based) encoding where propertyValueLength stores a cumulative
    byte offset instead of a direct value length."""
    offset_upper = (cumulative_offset >> 16) & 0x3FF
    type_and_upper = (prop_type << 10) | offset_upper

    return [
        # Bytes 0-1: [R(1) | PropType(5) | OffsetUpper(10)]
        boofuzz.Word(
            name=f"{prefix}_type_and_offset_upper",
            default_value=type_and_upper,
            endian=">",
            fuzzable=False,
        ),
        # Bytes 2-3: OffsetLower (16 bits)
        boofuzz.Word(
            name=f"{prefix}_offset_lower",
            default_value=cumulative_offset & 0xFFFF,
            endian=">",
            fuzzable=False,
        ),
        # Bytes 4-5: [R2(4) | PropNameLen(12)]
        boofuzz.Word(
            name=f"{prefix}_name_len",
            default_value=name_len & 0x0FFF,
            endian=">",
            fuzzable=False
        ),
    ]


def make_message_properties_area(num_properties: int = 2) -> boofuzz.Request:
    """
    Construct a boofuzz Request representing the MessageProperties wire format
    for the specified number of properties.  Properties are drawn from
    PROPERTY_POOL, using a mix of types (string, int32, bool, int64).

    Wire layout:
      [MessagePropertiesHeader        (6 bytes)]
      [MessagePropertyHeader #1       (6 bytes)]
      ...
      [MessagePropertyHeader #N       (6 bytes)]
      [name #1][value #1]...[name #N][value #N]
    """

    props = PROPERTY_POOL[:num_properties]

    # Calculate total unpadded size
    data_size = sum(len(name) + len(value) for _, name, value in props)
    total_unpadded = PROPS_HEADER_SIZE + num_properties * MPH_SIZE + data_size
    area_words = (total_unpadded + NumBytes.WORD - 1) // NumBytes.WORD

    # -- MessagePropertiesHeader (6 bytes) --
    packed_byte0 = (MPH_SIZE_2X << 3) | PROPS_HEADER_SIZE_2X
    children = [
        boofuzz.Byte(
            name="props_hdr_byte0",
            default_value=packed_byte0,
            full_range=True,
            fuzzable=False
        ),
        boofuzz.Byte(
            name="area_words_upper",
            default_value=(area_words >> 16) & 0xFF,
            full_range=True,
            fuzzable=False
        ),
        boofuzz.Word(
            name="area_words_lower",
            default_value=area_words & 0xFFFF,
            endian=">",
            fuzzable=False
        ),
        boofuzz.Byte(
            name="props_hdr_reserved",
            default_value=0,
            fuzzable=False,
        ),
        boofuzz.Byte(
            name="num_properties",
            default_value=num_properties,
            full_range=True,
            fuzzable=False
        ),
    ]

    # -- All MessagePropertyHeaders first --
    for i, (prop_type, name, value) in enumerate(props):
        children.extend(_make_mph_fields(f"mph{i}", prop_type, name, value))

    # -- Then all name/value data in order --
    for i, (_, name, value) in enumerate(props):
        children.append(boofuzz.Bytes(
            name=f"prop{i}_name",
            default_value=name,
            size=len(name),
            fuzzable=False
        ))
        children.append(boofuzz.Bytes(
            name=f"prop{i}_value",
            default_value=value,
            size=len(value),
            fuzzable=False
        ))

    return boofuzz.Request("message_properties", children=children)


def make_message_properties_area_new_style(num_properties: int = 2) -> boofuzz.Request:
    """
    Construct a boofuzz Request representing the MessageProperties wire format
    using new-style (offset-based) encoding.  In this encoding, each
    MessagePropertyHeader's propertyValueLength field stores a cumulative byte
    offset from the data area start to the property's name, rather than the
    direct value length.

    Wire layout is identical to old-style:
      [MessagePropertiesHeader        (6 bytes)]
      [MessagePropertyHeader #1       (6 bytes)]
      ...
      [MessagePropertyHeader #N       (6 bytes)]
      [name #1][value #1]...[name #N][value #N]
    """

    props = PROPERTY_POOL[:num_properties]

    data_size = sum(len(name) + len(value) for _, name, value in props)
    total_unpadded = PROPS_HEADER_SIZE + num_properties * MPH_SIZE + data_size
    area_words = (total_unpadded + NumBytes.WORD - 1) // NumBytes.WORD

    # -- MessagePropertiesHeader (6 bytes) --
    packed_byte0 = (MPH_SIZE_2X << 3) | PROPS_HEADER_SIZE_2X
    children = [
        boofuzz.Byte(
            name="props_hdr_byte0",
            default_value=packed_byte0,
            full_range=True,
            fuzzable=False
        ),
        boofuzz.Byte(
            name="area_words_upper",
            default_value=(area_words >> 16) & 0xFF,
            full_range=True,
            fuzzable=False
        ),
        boofuzz.Word(
            name="area_words_lower",
            default_value=area_words & 0xFFFF,
            endian=">",
            fuzzable=False
        ),
        boofuzz.Byte(
            name="props_hdr_reserved",
            default_value=0,
            fuzzable=False,
        ),
        boofuzz.Byte(
            name="num_properties",
            default_value=num_properties,
            full_range=True,
            fuzzable=False
        ),
    ]

    # -- Pre-compute cumulative offsets for new-style encoding --
    cumulative_offsets = []
    running_offset = 0
    for _, name, value in props:
        cumulative_offsets.append(running_offset)
        running_offset += len(name) + len(value)

    # -- All MessagePropertyHeaders with offset-based encoding --
    for i, (prop_type, name, _value) in enumerate(props):
        children.extend(_make_mph_fields_new_style(
            f"mph{i}", prop_type, len(name), cumulative_offsets[i],
        ))

    # -- Then all name/value data in order --
    for i, (_, name, value) in enumerate(props):
        children.append(boofuzz.Bytes(
            name=f"prop{i}_name",
            default_value=name,
            size=len(name),
            fuzzable=False
        ))
        children.append(boofuzz.Bytes(
            name=f"prop{i}_value",
            default_value=value,
            size=len(value),
            fuzzable=False
        ))

    return boofuzz.Request("message_properties_new_style", children=children)


def make_put_message_with_properties(properties_bytes: bytes, schema_id: int = 0) -> bytes:
    """
    Build a complete PUT event (EventHeader + PutHeader + app data) with the
    specified 'properties_bytes' as the message properties area in app data.

    The PUT header flags include both ACK_REQUESTED and MESSAGE_PROPERTIES.

    Returns the raw bytes of the entire event, ready to send on the wire.
    """

    flags = broker.PutHeaderFlags.ACK_REQUESTED | broker.PutHeaderFlags.MESSAGE_PROPERTIES
    flags_shifted = flags << 4

    guid = b"\x00\x00\x00\x00\x05\x78\x8d\xae\xd4\xb8\xca\x12\xae\xf3\x2d\xce"
    header_words = 9

    # Word-align properties area
    props_padded_len = len(properties_bytes)
    remainder = props_padded_len % NumBytes.WORD
    if remainder:
        padding_count = NumBytes.WORD - remainder
        properties_bytes += bytes([padding_count] * padding_count)
        props_padded_len += padding_count

    app_data = properties_bytes

    # PutHeader (36 bytes = 9 words) + app data
    message_words = header_words + len(app_data) // NumBytes.WORD
    options_words = 0

    put_header = b""
    # Word 0: flags(4) | messageWords(28)
    put_header += struct.pack(">I", (flags_shifted << 24) | message_words)
    # Word 1: optionsWords(24) | CAT(3) | headerWords(5)
    put_header += struct.pack(">I", (options_words << 8) | header_words)
    # Word 2: queueId
    put_header += struct.pack(">I", 0)
    # Words 3-6: GUID (16 bytes)
    put_header += guid
    # Word 7: CRC32-C over the application data
    crc = crc32c.crc32c(app_data)
    put_header += struct.pack(">I", crc)
    # Word 8: schemaId(16) | reserved(16)
    put_header += struct.pack(">HH", schema_id, 0)

    message = put_header + app_data
    total_size = NumBytes.WORD + NumBytes.WORD + len(message)

    # Pad entire event to word boundary
    remainder = total_size % NumBytes.WORD
    if remainder:
        padding_count = NumBytes.WORD - remainder
        message += bytes([padding_count] * padding_count)
        total_size += padding_count

    # EventHeader: size(4 bytes) + descriptor(4 bytes)
    event_descriptor = bytes([0x40 + broker.EventType.PUT, 0x02, broker.TypeSpecific.EMPTY, 0x00])
    event = struct.pack(">I", total_size) + event_descriptor + message

    return event


def make_put_request_with_cat_fuzz(properties_bytes: bytes, schema_id: int = 0) -> boofuzz.Request:
    """
    Build a complete PUT event as a boofuzz Request with the CAT
    (Compression Algorithm Type) field marked as fuzzable.  All other fields
    are static.

    The CAT field is 3 bits in byte 7 of the PutHeader (Word 1), packed as:
        optionsWords(24) | CAT(3) | headerWords(5)
    """

    flags = broker.PutHeaderFlags.ACK_REQUESTED | broker.PutHeaderFlags.MESSAGE_PROPERTIES
    flags_shifted = flags << 4

    guid = b"\x00\x00\x00\x00\x05\x78\x8d\xae\xd4\xb8\xca\x12\xae\xf3\x2d\xce"
    header_words = 9

    # Word-align properties area
    props_padded = bytearray(properties_bytes)
    remainder = len(props_padded) % NumBytes.WORD
    if remainder:
        padding_count = NumBytes.WORD - remainder
        props_padded += bytes([padding_count] * padding_count)

    app_data = bytes(props_padded)

    message_words = header_words + len(app_data) // NumBytes.WORD
    options_words = 0

    crc = crc32c.crc32c(app_data)

    total_size = NumBytes.WORD + NumBytes.WORD + header_words * NumBytes.WORD + len(app_data)
    event_padding = b""
    remainder = total_size % NumBytes.WORD
    if remainder:
        padding_count = NumBytes.WORD - remainder
        event_padding = bytes([padding_count] * padding_count)
        total_size += padding_count

    event_descriptor = bytes([0x40 + broker.EventType.PUT, 0x02, broker.TypeSpecific.EMPTY, 0x00])

    # Word 1 byte 3: (CAT << 5) | headerWords — CAT=0 by default
    word1_byte3_default = (0 << 5) | header_words

    children = [
        # EventHeader: size (4 bytes)
        boofuzz.DWord(
            name="event_size",
            default_value=total_size,
            endian=">",
            fuzzable=False,
        ),
        # EventHeader: descriptor (4 bytes)
        boofuzz.Bytes(
            name="event_descriptor",
            default_value=event_descriptor,
            size=4,
            fuzzable=False,
        ),
        # PutHeader Word 0: flags(4) | messageWords(28)
        boofuzz.DWord(
            name="put_word0",
            default_value=(flags_shifted << 24) | message_words,
            endian=">",
            fuzzable=False,
        ),
        # PutHeader Word 1 bytes 0-2: optionsWords upper bits
        boofuzz.Bytes(
            name="put_word1_upper",
            default_value=struct.pack(">I", options_words << 8)[:3],
            size=3,
            fuzzable=False,
        ),
        # PutHeader Word 1 byte 3: CAT(3) | headerWords(5) -- FUZZABLE
        boofuzz.Byte(
            name="cat_and_header_words",
            default_value=word1_byte3_default,
            fuzzable=True,
        ),
        # PutHeader Word 2: queueId
        boofuzz.DWord(
            name="queue_id",
            default_value=0,
            endian=">",
            fuzzable=False,
        ),
        # PutHeader Words 3-6: GUID (16 bytes)
        boofuzz.Bytes(
            name="guid",
            default_value=guid,
            size=16,
            fuzzable=False,
        ),
        # PutHeader Word 7: CRC32-C
        boofuzz.DWord(
            name="crc32c",
            default_value=crc,
            endian=">",
            fuzzable=False,
        ),
        # PutHeader Word 8: schemaId(16) | reserved(16)
        boofuzz.Word(
            name="schema_id",
            default_value=schema_id,
            endian=">",
            fuzzable=False,
        ),
        boofuzz.Word(
            name="reserved",
            default_value=0,
            endian=">",
            fuzzable=False,
        ),
        # App data (properties + event padding)
        boofuzz.Bytes(
            name="app_data",
            default_value=app_data + event_padding,
            size=len(app_data) + len(event_padding),
            fuzzable=False,
        ),
    ]

    return boofuzz.Request("put_with_cat_fuzz", children=children)


# =============================================================================
#                              SETUP HELPERS
# =============================================================================


def _render_setup_message(children):
    """Render a boofuzz sequence to its default (unfuzzed) bytes."""
    req = boofuzz.Request("_setup", children=children)
    return req.render()


def _connect(host: str, port: int) -> socket.socket:
    """Open a TCP connection to the broker, retrying on failure."""
    for attempt in range(CONNECT_MAX_RETRIES):
        try:
            sock = socket.create_connection((host, port), timeout=5)
            sock.settimeout(RECV_TIMEOUT)
            return sock
        except OSError:
            if attempt == CONNECT_MAX_RETRIES - 1:
                raise
            time.sleep(CONNECT_RETRY_DELAY)


def _send_and_recv(sock: socket.socket, data: bytes) -> None:
    """Send data and drain any response (ignoring timeouts)."""
    sock.sendall(data)
    try:
        sock.recv(4096)
    except socket.timeout:
        pass


def _send_setup_messages(sock: socket.socket) -> None:
    """
    Perform the full session handshake: authenticate, negotiate, open queue,
    and configure streams.
    """
    steps = [
        make_authentication_message(),
        make_control_message(broker.CLIENT_IDENTITY_SCHEMA),
        make_control_message(broker.OPEN_QUEUE_SCHEMA),
        make_control_message(broker.CONFIGURE_STREAM_SCHEMA),
        make_control_message(broker.CONFIGURE_QUEUE_STREAM_SCHEMA),
    ]

    for children in steps:
        data = _render_setup_message(children)
        _send_and_recv(sock, data)


def _is_broker_alive(sock: socket.socket) -> bool:
    """Check if the broker is still responsive by peeking at the socket."""
    try:
        data = sock.recv(1, socket.MSG_PEEK | socket.MSG_DONTWAIT)
        # Empty data means broker closed the connection.
        return len(data) > 0
    except (BlockingIOError, socket.timeout):
        # No data available — socket is open, broker is alive.
        return True
    except OSError:
        return False


# =============================================================================
#                              PUBLIC INTERFACE
# =============================================================================


def fuzz_properties(host: str, port: int) -> None:
    """
    Launch a CAT-field fuzzing session against a BlazingMQ Broker.

    Connects to the broker, performs the session handshake once, then sends
    PUT messages with fuzzed CAT (Compression Algorithm Type) field values
    in the PutHeader.  Raises AssertionError if the broker crashes.
    """

    sock = _connect(host, port)
    try:
        _send_setup_messages(sock)

        # Build static properties (new-style, 2 props) to embed in the PUT
        props_request = make_message_properties_area_new_style(num_properties=2)
        static_props = props_request.render()

        put_request = make_put_request_with_cat_fuzz(static_props, schema_id=2)
        mutations = put_request.num_mutations()
        logger.info("Fuzzing CAT field: %d mutations", mutations)

        total_mutation_count = 0

        for m_list in put_request.get_mutations():
            mc = MutationContext(
                mutations={m.qualified_name: m for m in m_list},
            )
            fuzzed_event = put_request.render(mutation_context=mc)

            try:
                sock.sendall(fuzzed_event)
            except OSError as e:
                raise AssertionError(
                    f"Broker connection lost at mutation "
                    f"{total_mutation_count} (CAT fuzz): {e}"
                ) from e

            try:
                sock.recv(4096)
            except socket.timeout:
                pass

            if not _is_broker_alive(sock):
                raise AssertionError(
                    f"Broker appears to have crashed at mutation "
                    f"{total_mutation_count} (CAT fuzz)"
                )

            total_mutation_count += 1

        logger.info(
            "Completed %d total mutations without broker crash",
            total_mutation_count,
        )
    finally:
        sock.close()
