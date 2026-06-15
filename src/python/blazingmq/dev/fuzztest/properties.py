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
import struct

import boofuzz
import crc32c

from blazingmq.dev.fuzztest import (
    BoofuzzSequence,
    FuzzLoggerLimited,
    NumBits,
    NumBytes,
    PaddingBlock,
    make_authentication_message,
    make_control_message,
)
from blazingmq.schemas import broker

logger = logging.getLogger(__name__)

# =============================================================================
#                               CONSTANTS
# =============================================================================

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


def _make_mph_fields(prefix, prop_type, name_len, middle_value, fuzzable=False):
    """Build the 3 boofuzz Words for a MessagePropertyHeader.

    The middle field (word 1) holds either the property value length (old-style)
    or a cumulative byte offset (new-style) — same bit layout either way.
    """
    upper = (middle_value >> 16) & 0x3FF
    type_and_upper = (prop_type << 10) | upper

    return [
        boofuzz.Word(
            name=f"{prefix}_type_and_upper",
            default_value=type_and_upper,
            endian=">",
            fuzzable=fuzzable,
        ),
        boofuzz.Word(
            name=f"{prefix}_lower",
            default_value=middle_value & 0xFFFF,
            endian=">",
            fuzzable=fuzzable,
        ),
        boofuzz.Word(
            name=f"{prefix}_name_len",
            default_value=name_len & 0x0FFF,
            endian=">",
            fuzzable=fuzzable,
        ),
    ]


def make_message_properties_area(
    num_properties: int = 2,
    fuzz_header: bool = False,
    fuzz_mph: bool = False,
    new_style: bool = False,
) -> boofuzz.Request:
    """
    Construct a boofuzz Request representing the MessageProperties wire format.

    In old-style encoding, each MPH stores the property value length directly.
    In new-style (offset-based) encoding, it stores a cumulative byte offset.

    Wire layout (same for both styles):
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

    packed_byte0 = (MPH_SIZE_2X << 3) | PROPS_HEADER_SIZE_2X
    children = [
        boofuzz.Byte(
            name="props_hdr_byte0",
            default_value=packed_byte0,
            full_range=True,
            fuzzable=fuzz_header,
        ),
        boofuzz.Byte(
            name="area_words_upper",
            default_value=(area_words >> 16) & 0xFF,
            fuzzable=False,
        ),
        boofuzz.Word(
            name="area_words_lower",
            default_value=area_words & 0xFFFF,
            endian=">",
            fuzzable=fuzz_header,
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
            fuzzable=fuzz_header,
        ),
    ]

    # Pre-compute cumulative offsets for new-style encoding
    if new_style:
        offsets = []
        running = 0
        for _, name, value in props:
            offsets.append(running)
            running += len(name) + len(value)

    for i, (prop_type, name, value) in enumerate(props):
        middle = offsets[i] if new_style else len(value)
        children.extend(_make_mph_fields(
            f"mph{i}", prop_type, len(name), middle,
            fuzzable=(fuzz_mph and i == 0),
        ))

    for i, (_, name, value) in enumerate(props):
        children.append(boofuzz.Bytes(
            name=f"prop{i}_name",
            default_value=name,
            size=len(name),
            fuzzable=False,
        ))
        children.append(boofuzz.Bytes(
            name=f"prop{i}_value",
            default_value=value,
            size=len(value),
            fuzzable=False,
        ))

    return boofuzz.Request(
        "message_properties_new_style" if new_style else "message_properties",
        children=children,
    )


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


def make_control_request_with_encoding_fuzz(schema) -> boofuzz.Request:
    """
    Build a CONTROL event as a boofuzz Request with the EventHeader's
    TypeSpecific byte (encoding type, 3 bits at bits 5-7) marked as fuzzable.
    All other fields are static.

    The payload is a valid JSON-encoded control message from the given schema,
    so the only variable is how the broker interprets the encoding type field.
    Valid encoding values: BER=0, JSON=1 (3-bit field allows 0-7).
    """

    from blazingmq.dev.fuzztest import schema_to_boofuzz, PaddingBlock

    # Render the JSON payload statically
    payload_children = schema_to_boofuzz(schema)
    payload_req = boofuzz.Request("_payload", children=payload_children)
    payload_bytes = payload_req.render()

    # Word-align the payload
    padded_payload = bytearray(payload_bytes)
    remainder = len(padded_payload) % NumBytes.WORD
    if remainder:
        padding_count = NumBytes.WORD - remainder
        padded_payload += bytes([padding_count] * padding_count)

    # Total event size: EventHeader (8 bytes) + padded payload
    total_size = 2 * NumBytes.WORD + len(padded_payload)

    children = [
        # EventHeader: length (4 bytes) — includes fragment bit (0) in MSB
        boofuzz.DWord(
            name="event_length",
            default_value=total_size,
            endian=">",
            fuzzable=False,
        ),
        # EventHeader byte 4: [PV(2) | Type(6)]
        boofuzz.Byte(
            name="pv_and_type",
            default_value=0x40 + broker.EventType.CONTROL,
            fuzzable=False,
        ),
        # EventHeader byte 5: headerWords
        boofuzz.Byte(
            name="header_words",
            default_value=0x02,
            fuzzable=False,
        ),
        # EventHeader byte 6: TypeSpecific (encoding type) -- FUZZABLE
        boofuzz.Byte(
            name="type_specific_encoding",
            default_value=broker.TypeSpecific.ENCODING_JSON,
            fuzzable=True,
        ),
        # EventHeader byte 7: reserved
        boofuzz.Byte(
            name="event_reserved",
            default_value=0x00,
            fuzzable=False,
        ),
        # Payload (static JSON control message)
        boofuzz.Bytes(
            name="control_payload",
            default_value=bytes(padded_payload),
            size=len(padded_payload),
            fuzzable=False,
        ),
    ]

    return boofuzz.Request("control_with_encoding_fuzz", children=children)


# =============================================================================
#                              PUBLIC INTERFACE
# =============================================================================


def make_put_with_fuzzable_properties(
    num_properties: int = 2,
    new_style: bool = False,
) -> BoofuzzSequence:
    """
    Build a PUT event as a BoofuzzSequence with auto-computed CRC and the
    6 high-value properties fields marked fuzzable:
      - props_hdr_byte0, area_words_lower, num_properties  (header)
      - mph0 type/value_len/name_len                       (first MPH)

    Uses boofuzz.Checksum to recompute CRC32-C over the app_data block on
    every render, so mutated properties always have a valid CRC and the
    broker actually parses them instead of rejecting at the event level.

    Follows the same pattern as make_put_message() in __init__.py.
    """

    flags = (
        broker.PutHeaderFlags.ACK_REQUESTED | broker.PutHeaderFlags.MESSAGE_PROPERTIES
    )
    flags_offset = 4

    flags_field = boofuzz.BitField(
        name="flags",
        default_value=flags << flags_offset,
        width=NumBits.BYTE,
        fuzzable=False,
    )

    message_size = boofuzz.Size(
        name="size",
        block_name="message",
        offset=NumBytes.WORD,
        length=3 * NumBytes.BYTE,
        endian=">",
        inclusive=True,
        fuzzable=False,
        math=lambda x: x // NumBytes.WORD,
    )

    guid = b"\x00\x00\x00\x00\x05\x78\x8d\xae\xd4\xb8\xca\x12\xae\xf3\x2d\xce"

    props_request = make_message_properties_area(
        num_properties=num_properties,
        fuzz_header=True,
        fuzz_mph=True,
        new_style=new_style,
    )

    message_components = [
        boofuzz.BitField(
            name="options_size", width=3 * NumBits.BYTE, fuzzable=False,
        ),
        boofuzz.BitField(
            name="header_words",
            default_value=9,
            width=NumBits.BYTE,
            endian=">",
            fuzzable=False,
        ),
        boofuzz.DWord(
            name="qId", default_value=0, endian=">",
            output_format="binary", fuzzable=False,
        ),
        boofuzz.Bytes(
            name="correlation_id",
            default_value=guid,
            size=4 * NumBytes.WORD,
            padding=b"\x01",
            fuzzable=False,
        ),
        # CRC32-C auto-recomputed over app_data on every render.
        # Not fuzzable — we want valid CRCs so the broker parses properties.
        boofuzz.Checksum(
            name="checksum",
            block_name="app_data",
            algorithm="crc32c",
            length=NumBytes.WORD,
            endian=">",
            fuzzable=False,
        ),
        boofuzz.BitField(
            name="schema_id", default_value=0, width=2 * NumBits.BYTE,
            endian=">", fuzzable=False,
        ),
        boofuzz.BitField(
            name="reserved", default_value=0, width=2 * NumBits.BYTE,
            endian=">", fuzzable=False,
        ),
        # app_data is a Block so Checksum can reference it by name.
        # Its children are the fuzzable properties fields.
        boofuzz.Block(name="app_data", children=list(props_request.stack)),
    ]

    message = boofuzz.Block(name="message", children=message_components)

    event_size = boofuzz.Size(
        name="event_size",
        block_name="event_contents",
        endian=">",
        inclusive=True,
        length=NumBytes.WORD,
        fuzzable=False,
    )
    event_desc = boofuzz.Bytes(
        name="event_desc",
        default_value=bytes([
            0x40 + broker.EventType.PUT, 0x02, broker.TypeSpecific.EMPTY, 0x00,
        ]),
        size=NumBytes.WORD,
        fuzzable=False,
    )
    event_contents = PaddingBlock(
        "event_contents",
        children=[event_desc, flags_field, message_size, message],
    )

    return [event_size, event_contents]


class _PersistentConnection(boofuzz.TCPSocketConnection):
    """TCPSocketConnection that stays open across test cases and auto-reconnects.

    Boofuzz's Session calls open()/close() around each mutation.  close() is
    a no-op so the connection persists.  If the broker RSTs the connection
    (e.g. after a heartbeat timeout), send() catches the error, reconnects,
    replays the handshake, and retries the send.
    """

    def __init__(self, host, port, setup_steps=None, **kwargs):
        super().__init__(host, port, **kwargs)
        self._setup_steps = setup_steps or []
        self._connected = False

    def open(self):
        if not self._connected:
            super().open()
            self._send_setup()
            self._connected = True

    def close(self):
        pass

    def send(self, data):
        try:
            super().send(data)
        except Exception:
            logger.info("Connection lost — reconnecting and replaying handshake")
            self._connected = False
            try:
                super().close()
            except Exception:
                pass
            super().open()
            self._send_setup()
            self._connected = True
            super().send(data)

    def _send_setup(self):
        """Replay the full handshake (auth, negotiate, open queue, configure)."""
        for children in self._setup_steps:
            req = boofuzz.Request("_setup", children=children)
            super().send(req.render())
            try:
                self.recv(4096)
            except Exception:
                pass

    def shutdown(self):
        self._connected = False
        super().close()


def fuzz_properties(host: str, port: int) -> None:
    """
    Launch a long-running fuzzing session targeting message properties in PUT
    messages at depth 2 (all pairs of fuzzable fields).

    How it works:
      1. _PersistentConnection opens a TCP connection and sends the handshake
         (auth, negotiate, open queue, configure stream with 'x > 0').
      2. The connection stays open across mutations.  If the broker resets it
         (heartbeat timeout), _PersistentConnection auto-reconnects and
         replays the handshake.
      3. The PUT Request uses boofuzz.Checksum to recompute a valid CRC for
         every mutation, so the broker accepts the event and actually parses
         the properties (instead of rejecting at the CRC check).
      4. session.fuzz(max_depth=2) iterates all single-field mutations AND
         all pairwise field combinations.
    """

    setup_steps = [
        make_authentication_message(),
        make_control_message(broker.CLIENT_IDENTITY_SCHEMA),
        make_control_message(broker.OPEN_QUEUE_SCHEMA),
        make_control_message(broker.CONFIGURE_STREAM_PROPERTY_EXPRESSION_SCHEMA),
    ]

    conn = _PersistentConnection(
        host, port, setup_steps=setup_steps, recv_timeout=0.05,
    )

    session = boofuzz.Session(
        target=boofuzz.Target(connection=conn),
        receive_data_after_each_request=True,
        receive_data_after_fuzz=True,
        web_port=None,
        fuzz_loggers=[FuzzLoggerLimited()],
        fuzz_db_keep_only_n_pass_cases=1,
    )

    put = boofuzz.Request(
        "Put", children=(make_put_with_fuzzable_properties()),
    )
    session.connect(put)

    try:
        session.fuzz(max_depth=2)
    finally:
        conn.shutdown()
