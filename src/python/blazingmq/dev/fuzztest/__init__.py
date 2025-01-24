# Copyright 2024 Bloomberg Finance L.P.
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
Utilities for fuzz testing BlazingMQ Broker.

Public functions:
o 'fuzz': launch fuzzing session with the given 'host':'port' of a BlazingMQ
          Broker instance.  If the optionally specified 'request' string is
          provided, fuzz only the request with the given name.
"""
import logging
from enum import IntEnum
from typing import List, Optional

import boofuzz
from blazingmq.schemas import broker

# =============================================================================
#                                  CONSTANTS
# =============================================================================


BoofuzzSequence = List[boofuzz.Fuzzable]


class NumBytes(IntEnum):
    """
    This IntEnum represents a number of bytes for BlazingMQ Broker types.

    See also: bmqp::Protocol
    """

    BYTE = 1
    WORD = 4
    DWORD = 8


class NumBits(IntEnum):
    """
    This IntEnum is used to show conveniently where data size should be in bits
    instead of a bytes.
    """

    BYTE = 8


# =============================================================================
#                               BOOFUZZ UTILITIES
# =============================================================================


class PaddingBlock(boofuzz.Block):
    """
    This class is used to word-align child boofuzz.Fuzzable entities according
    to the BlazingMQ Broker padding conventions.
    """

    def encode(self, value, mutation_context) -> bytes:
        data: bytes = self.get_child_data(mutation_context=mutation_context)
        data_size = len(data)
        data_words = data_size // NumBytes.WORD

        padding_bytes_num = (data_words + 1) * NumBytes.WORD - data_size
        data += bytes([padding_bytes_num] * padding_bytes_num)

        return data


class FuzzLoggerLimited(boofuzz.fuzz_logger_text.FuzzLoggerText):
    """
    This class is used to replace default boofuzz console logger to limit the
    number of bytes printed in binary packets.
    """

    def _print_log_msg(self, msg_type, msg=None, data=None):
        super()._print_log_msg(
            msg_type, msg=msg, data=data if data is None else data[:256]
        )


def schema_to_boofuzz(schema: broker.SchemaDescription) -> BoofuzzSequence:
    """
    Convert dictionary schema representation to a boofuzz primitives
    representation.
    """

    # delimiters will be checked on json decoding level in the broker and
    # therefore it's a waste of time to check it during fuzz testing
    fuzz_delimiters = False

    res = [boofuzz.Static(default_value="{")]
    for key, value in schema.items():
        assert isinstance(key, str)

        res.append(boofuzz.Static(default_value=f'"{key}"'))
        res.append(boofuzz.Delim(default_value=":", fuzzable=fuzz_delimiters))

        if isinstance(value, dict):
            res += schema_to_boofuzz(value)
        elif isinstance(value, str):
            res.append(boofuzz.String(default_value=f'"{value}"'))
        elif isinstance(value, bool):
            # check for bool should be performed before the check for int
            # because bool is a subclass of int
            res.append(
                boofuzz.Group(
                    values=["true", "false"], default_value=str(value).lower()
                )
            )
        elif isinstance(value, int):
            res.append(boofuzz.DWord(default_value=value, output_format="ascii"))
        else:
            raise NotImplementedError(
                "Not implemented conversion of value of type " + str(type(value))
            )
        res.append(boofuzz.Delim(default_value=",", fuzzable=fuzz_delimiters))

    # last boofuzz.Delim "," should be removed if presented
    if len(res) > 0:
        res = res[:-1]
    res.append(boofuzz.Static(default_value="}"))
    return res


def disable_fuzzing(req: boofuzz.Request) -> None:
    """
    Disable fuzzing of the specified 'req' during fuzzing session.
    """

    for component in req.stack:
        # there is no any public function to enable/disable fuzzing on already
        # built request, so we have to set protected member '_fuzzable' as a
        # workaround
        component._fuzzable = False  # pylint: disable=W0212


# =============================================================================
#                       BLAZINGMQ BROKER BINARY PROTOCOL
# =============================================================================


def wrap_event(
    contents: BoofuzzSequence,
    event_type: int,
    type_specific: int,
    add_padding: bool = True,
) -> BoofuzzSequence:
    """
    Wraps boofuzz 'contents' with EventHeader and adds padding to the end if
    'add_padding' is True.

    See also: bmqp::EventHeader
    """

    event_size = boofuzz.Size(
        name="event_size",
        block_name="event_contents",
        endian=">",
        inclusive=True,
        length=NumBytes.WORD,
    )
    event_desc = boofuzz.Bytes(
        name="event_desc",
        default_value=bytes([0x40 + event_type, 0x02, type_specific, 0x00]),
        size=NumBytes.WORD,
    )

    build_block = PaddingBlock if add_padding else boofuzz.Block
    event_contents = build_block("event_contents", children=[event_desc] + contents)

    return [event_size, event_contents]


def wrap_event_wrong(
    contents: BoofuzzSequence,
    event_type: int,
    type_specific: int,
    add_padding: bool = True,  # pylint: disable=W0613
) -> BoofuzzSequence:
    """
    Wraps boofuzz 'contents' with EventHeader and adds malformed padding to the
    end.

    'add_padding' is not used but is presented to retain compatibility with a
    correct 'wrap_event'.

    See also: bmqp::EventHeader
    """

    event_size = boofuzz.Size(
        name="event_size",
        block_name="event_contents",
        endian=">",
        inclusive=True,
        length=NumBytes.WORD,
    )
    event_desc = boofuzz.Bytes(
        name="event_desc",
        default_value=bytes([0x40 + event_type, 0x02, type_specific, 0x00]),
        size=NumBytes.WORD,
    )

    wrong_bytes = boofuzz.Bytes(name="wrong", default_value=b"\x04")
    event_contents = boofuzz.Block(
        "event_contents", children=[event_desc] + contents + [wrong_bytes]
    )

    return [event_size, event_contents]


def make_put_message() -> BoofuzzSequence:
    """
    Constructs boofuzz structures representing PutMessage.
    """

    flags = broker.PutHeaderFlags.ACK_REQUESTED
    flags_offset = 4

    # actual flags bitmask is stored in the first 4 bits of the 'flags' byte,
    # the next 4 bits are used for message size high order bits and considered
    # to be 0 (for small message sizes)
    # [0, 1, 2, 3] [4, 5, 6, 7] - bits 0-3 are for flags, 4-7 are for put
    # message size (bits 4-7 are considered to be zero for simplicity)
    flags = boofuzz.BitField(
        name="flags",
        default_value=flags << flags_offset,
        width=NumBits.BYTE,
        full_range=True,
    )

    # 'offset' should contain both byte used for flags and 3 bytes used for
    # size - total 4 bytes (entire word)
    message_size = boofuzz.Size(
        name="size",
        block_name="message",
        offset=NumBytes.WORD,
        length=3 * NumBytes.BYTE,
        endian=">",
        inclusive=True,
        math=lambda x: x // NumBytes.WORD,
    )

    guid = b"\x00\x00\x00\x00\x05\x78\x8d\xae\xd4\xb8\xca\x12\xae\xf3\x2d\xce"

    message_components = [
        boofuzz.BitField(name="options_size", width=3 * NumBits.BYTE),
        boofuzz.BitField(
            name="header_words",
            default_value=9,
            width=NumBits.BYTE,
            endian=">",
            full_range=True,
        ),
        boofuzz.DWord(name="qId", default_value=0, endian=">", output_format="binary"),
        boofuzz.Bytes(
            name="correlation_id",
            default_value=guid,
            size=4 * NumBytes.WORD,
            padding=b"\x01",
        ),
        boofuzz.Checksum(
            name="checksum",
            block_name="app_data",
            algorithm="crc32c",
            length=NumBytes.WORD,
            endian=">",
        ),
        boofuzz.BitField(
            name="schema_id", default_value=0, width=2 * NumBits.BYTE, endian=">"
        ),
        boofuzz.BitField(
            name="reserved", default_value=0, width=2 * NumBits.BYTE, endian=">"
        ),
        # "options" section here is empty assuming that no options presented
        boofuzz.Bytes(
            name="app_data", default_value=bytes("AppData", encoding="ascii")
        ),
    ]

    message = boofuzz.Block(name="message", children=message_components)

    return wrap_event(
        [flags, message_size, message], broker.EventType.PUT, broker.TypeSpecific.EMPTY
    )


def make_confirm_message() -> BoofuzzSequence:
    """
    Constructs boofuzz structures representing ConfirmMessage.
    """

    guid = b"\x00\x00\x00\x00\x05\x78\x8d\xae\xd4\xb8\xca\x12\xae\xf3\x2d\xce"

    message_components = [
        boofuzz.BitField(
            name="confirm_header",
            default_value=16 + 6,
            width=NumBits.BYTE,
            full_range=True,
        ),
        boofuzz.Bytes(
            name="confirm_header_reserved",
            default_value=bytes(3 * [0]),
            size=3 * NumBytes.BYTE,
            padding=b"\x01",
        ),
        boofuzz.DWord(name="qId", default_value=0, endian=">", output_format="binary"),
        boofuzz.Bytes(
            name="correlation_id",
            default_value=guid,
            size=4 * NumBytes.WORD,
            padding=b"\x01",
        ),
        boofuzz.DWord(
            name="sub_qId", default_value=0, endian=">", output_format="binary"
        ),
    ]
    return wrap_event(
        message_components, broker.EventType.CONFIRM, broker.TypeSpecific.EMPTY, False
    )


def make_control_message(schema: broker.SchemaDescription) -> BoofuzzSequence:
    """
    Forms control event in boofuzz structures representation from the specified
    'schema' representation.
    """

    return wrap_event(
        schema_to_boofuzz(schema),
        broker.EventType.CONTROL,
        broker.TypeSpecific.ENCODING_JSON,
    )


# =============================================================================
#                                PUBLIC INTERFACE
# =============================================================================


def fuzz(host: str, port: int, request: Optional[str] = None) -> None:
    """
    Launch a fuzzing session with the specified 'host' and 'port' of the
    launched BlazingMQ Broker instance.
    """

    session = boofuzz.Session(
        target=boofuzz.Target(
            connection=boofuzz.TCPSocketConnection(host, port, recv_timeout=0.1)
        ),
        receive_data_after_each_request=True,
        receive_data_after_fuzz=True,
        web_port=None,
        fuzz_loggers=[FuzzLoggerLimited()],
        fuzz_db_keep_only_n_pass_cases=1,
    )

    identity = boofuzz.Request(
        "Identity", children=(make_control_message(broker.CLIENT_IDENTITY_SCHEMA))
    )

    open_queue = boofuzz.Request(
        "OpenQueue", children=(make_control_message(broker.OPEN_QUEUE_SCHEMA))
    )

    configure_queue_stream = boofuzz.Request(
        "ConfigureQueueStream",
        children=(make_control_message(broker.CONFIGURE_QUEUE_STREAM_SCHEMA)),
    )

    put = boofuzz.Request("Put", children=(make_put_message()))

    confirm = boofuzz.Request("Confirm", children=(make_confirm_message()))

    close_queue = boofuzz.Request(
        "CloseQueue", children=(make_control_message(broker.CLOSE_QUEUE_SCHEMA))
    )

    disconnect = boofuzz.Request(
        "Disconnect", children=(make_control_message(broker.DISCONNECT_SCHEMA))
    )

    sequence = [
        (identity, "identity"),
        (open_queue, "open_queue"),
        (configure_queue_stream, "configure_queue_stream"),
        (put, "put"),
        (confirm, "confirm"),
        (close_queue, "close_queue"),
        (disconnect, "disconnect"),
    ]

    if request is not None:
        disabled_count = 0
        for req, name in sequence:
            if name != request:
                disable_fuzzing(req)
                disabled_count += 1

    prev = None
    for req, name in sequence:
        if prev is None:
            session.connect(req)
        else:
            session.connect(prev, req)
        prev = req

    session.fuzz(max_depth=1)
