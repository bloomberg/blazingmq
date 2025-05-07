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
blazingmq.dev.it.process.admin


PURPOSE: Provide a BMQ admin client.
"""

import socket
import json
from typing import Optional, Union

from blazingmq.schemas import broker


class AdminClient:
    def __init__(self):
        self._channel: Optional[socket.socket] = None

    @staticmethod
    def _wrap_control_event(payload: Union[str, dict]) -> bytes:
        """
        Wraps the specified 'payload' with EventHeader and adds padding to the
        end. Returns the raw bytes control message.

        See also: bmqp::EventHeader
        """

        if isinstance(payload, str):
            payload_str = payload
        else:
            payload_str = json.dumps(payload)

        padding_len = 4 - len(payload_str) % 4
        padding = bytes([padding_len] * padding_len)

        event_type = broker.EventType.CONTROL
        type_specific = broker.TypeSpecific.ENCODING_JSON

        control_header_bytes = 8
        control_event_size = (
            control_header_bytes + len(payload_str) + padding_len
        ).to_bytes(4, "big")
        control_event_desc = bytes([0x40 + event_type, 0x02, type_specific, 0x00])

        return (
            control_event_size
            + control_event_desc
            + payload_str.encode("ascii")
            + padding
        )

    @classmethod
    def _make_admin_command(cls, message: str) -> bytes:
        """
        Wraps the specified 'message' with admin command and returns it as raw
        bytes control message.
        """

        command = broker.ADMIN_COMMAND_SCHEMA
        command["adminCommand"]["command"] = message

        return cls._wrap_control_event(command)

    def _send_raw(self, message: bytes) -> bytes:
        """
        Send the specified raw "message" over the channel to the broker.
        Return the received byte response.
        """

        assert self._channel is not None

        self._channel.send(message)
        return self._receive_event()

    def _receive_event(self) -> bytes:
        """
        Read the channel until the next event is received.

        Return the received event contents excluding event header and padding
        bytes at the end of the message.
        """

        header_bytes = 8
        header = self._channel.recv(header_bytes)
        # The situation when the event header is not fully received with one
        # 'recv' call is highly improbable.
        assert len(header) == header_bytes

        message = b""

        # The first 4 bytes of the message contain message full size in bytes.
        # See also: bmqp_protocol.h / EventHeader
        remaining = int.from_bytes(header[:4], "big") - header_bytes
        while remaining > 0:
            part = self._channel.recv(remaining)
            remaining -= len(part)
            message += part

        padding_bytes = message[-1]
        assert 1 <= padding_bytes <= 4  # expect correct padding byte value
        message = message[:-padding_bytes]

        return message

    def send_admin(self, admin_command: str) -> Union[dict, str]:
        """
        Send the specified 'admin_command' to the admin session currently opened
        on the broker. Return the command execution results.
        """

        command = broker.ADMIN_COMMAND_SCHEMA
        command["adminCommand"]["command"] = admin_command

        response_bytes = self._send_raw(self._wrap_control_event(command))
        response_dict = json.loads(response_bytes)
        return response_dict["adminCommandResponse"]["text"]

    def connect(self, host: str, port: int) -> None:
        """
        Connect to the broker using the specified 'host' / 'port' and open
        an admin session.
        """
        assert self._channel is None

        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        sock.connect((host, port))
        sock.settimeout(10.0)

        self._channel = sock

        admin_client_identity = broker.CLIENT_IDENTITY_SCHEMA
        admin_client_identity["clientIdentity"]["clientType"] = "E_TCPADMIN"

        self._send_raw(self._wrap_control_event(admin_client_identity))

    def stop(self) -> None:
        """
        Disconnect from the broker.
        """
        if self._channel is not None:
            self._channel.close()
            self._channel = None
