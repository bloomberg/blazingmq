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

import struct
import boofuzz

from blazingmq.dev.fuzztest import (
    BoofuzzSequence,
    FuzzLoggerLimited,
    NumBits,
    NumBytes,
    PaddingBlock,
    make_authentication_message,
    make_control_message,
)
from blazingmq.dev.fuzztest.persistent_connection import PersistentConnection
from blazingmq.schemas import broker

PROPERTIES_HEADER_SIZE_2X = 3  # 6 bytes / 2
MPH_SIZE_2X = 3  # 6 bytes / 2
MPH_SIZE = 6  # bytes
PROPERTIES_HEADER_SIZE = 6  # bytes

PROP_TYPE_STRING = 6  # bmqt::PropertyType::e_STRING
PROP_TYPE_INT32 = 4  # bmqt::PropertyType::e_INT32


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
    """

    properties = [
        (PROP_TYPE_STRING, b"key", b"val"),
        (PROP_TYPE_INT32, b"id", struct.pack(">i", 42)),
    ]

    data_size = sum(len(name) + len(value) for _, name, value in properties)
    total_unpadded = PROPERTIES_HEADER_SIZE + num_properties * MPH_SIZE + data_size
    area_words = (total_unpadded + NumBytes.WORD - 1) // NumBytes.WORD

    packed_byte0 = (MPH_SIZE_2X << 3) | PROPERTIES_HEADER_SIZE_2X
    children = [
        boofuzz.Byte(
            name="properties_hdr_byte0",
            default_value=packed_byte0,
            full_range=True,
            fuzzable=fuzz_header,
        ),
        boofuzz.Byte(
            name="area_words_upper",
            default_value=(area_words >> 16) & 0xFF,
            fuzzable=False,
        ),
        boofuzz.Word(  # double check
            name="area_words_lower",
            default_value=area_words & 0xFFFF,
            endian=">",
            fuzzable=fuzz_header,
        ),
        boofuzz.Byte(
            name="properties_hdr_reserved",
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
    offsets = []
    if new_style:
        running = 0
        for _, name, value in properties:
            offsets.append(running)
            running += len(name) + len(value)

    for i, (prop_type, name, value) in enumerate(properties):
        middle = offsets[i] if new_style else len(value)
        children.extend(
            _make_mph_fields(
                f"mph{i}",
                prop_type,
                len(name),
                middle,
                fuzzable=(
                    fuzz_mph and i == 0
                ),  # only fuzz the first message property header to save time, might want to fuzz more
            )
        )

    for i, (_, name, value) in enumerate(properties):
        children.append(
            boofuzz.Bytes(
                name=f"prop{i}_name",
                default_value=name,
                size=len(name),
                fuzzable=False,
            )
        )
        children.append(
            boofuzz.Bytes(
                name=f"prop{i}_value",
                default_value=value,
                size=len(value),
                fuzzable=False,
            )
        )

    return boofuzz.Request(
        "message_properties_new_style" if new_style else "message_properties",
        children=children,
    )


def make_put_with_fuzzable_properties(
    num_properties: int = 2,
    new_style: bool = False,
) -> BoofuzzSequence:
    """
    Build a PUT event as a BoofuzzSequence with auto-computed CRC and the
    6 high-value properties fields marked fuzzable:
      - properties_hdr_byte0, area_words_lower, num_properties  (header)
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

    properties_request = make_message_properties_area(
        num_properties=num_properties,
        fuzz_header=True,
        fuzz_mph=True,
        new_style=new_style,
    )

    message_components = [
        boofuzz.BitField(
            name="options_size",
            width=3 * NumBits.BYTE,
            fuzzable=False,
        ),
        boofuzz.BitField(
            name="header_words",
            default_value=9,
            width=NumBits.BYTE,
            endian=">",
            fuzzable=False,
        ),
        boofuzz.DWord(
            name="qId",
            default_value=0,
            endian=">",
            output_format="binary",
            fuzzable=False,
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
            name="schema_id",
            default_value=0,
            width=2 * NumBits.BYTE,
            endian=">",
            fuzzable=False,
        ),
        boofuzz.BitField(
            name="reserved",
            default_value=0,
            width=2 * NumBits.BYTE,
            endian=">",
            fuzzable=False,
        ),
        # app_data is a Block so Checksum can reference it by name.
        # Its children are the fuzzable properties fields.
        boofuzz.Block(name="app_data", children=list(properties_request.stack)),
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
        default_value=bytes(
            [
                0x40 + broker.EventType.PUT,
                0x02,
                broker.TypeSpecific.EMPTY,
                0x00,
            ]
        ),
        size=NumBytes.WORD,
        fuzzable=False,
    )
    event_contents = PaddingBlock(
        "event_contents",
        children=[event_desc, flags_field, message_size, message],
    )

    return [event_size, event_contents]


def fuzz_properties(host: str, port: int, max_depth: int = 2) -> None:
    """
    Launch a long-running fuzzing session targeting message properties in PUT
    messages at depth 2 (all pairs of fuzzable fields).
    NOTE: This currently only fuzzes the first property's header fields to limit the combinatorial explosion, but can be extended to fuzz more.
    """

    setup_steps = [
        make_authentication_message(),
        make_control_message(broker.CLIENT_IDENTITY_SCHEMA),
        make_control_message(broker.OPEN_QUEUE_SCHEMA),
        make_control_message(broker.CONFIGURE_STREAM_WITH_CONSUMER_SUBSCRIPTION_SCHEMA),
    ]

    conn = PersistentConnection(
        host,
        port,
        setup_steps=setup_steps,
        recv_timeout=0.05,
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
        "Put",
        children=(make_put_with_fuzzable_properties()),
    )
    session.connect(put)

    try:
        session.fuzz(max_depth=max_depth)
    finally:
        conn.shutdown()
