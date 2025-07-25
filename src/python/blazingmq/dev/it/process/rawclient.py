# Copyright 2025 Bloomberg Finance L.P.
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
blazingmq.dev.it.process.rawclient


PURPOSE: Provide a BMQ raw client.
"""

import socket
import json
from typing import Optional, Union

from blazingmq.schemas import broker


class RawClient:
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

    @staticmethod
    def _wrap_heartbeat_res_event() -> bytes:
        """
        Wraps the specified 'payload' with EventHeader and adds padding to the
        end. Returns the raw bytes heartbeat response event. Notice that heartbeat
        response only has header and no body.

        See also: bmqp::EventHeader
        """

        event_type = broker.EventType.HEARTBEAT_RSP
        type_specific = broker.TypeSpecific.EMPTY

        header_bytes = 8
        event_size = header_bytes.to_bytes(4, "big")
        event_desc = bytes([0x40 + event_type, 0x02, type_specific, 0x00])

        return event_size + event_desc

    def _send_raw(self, message: bytes) -> None:
        """
        Send the specified raw "message" over the channel to the broker.
        Return the received byte response.
        """
        assert self._channel is not None

        try:
            self._channel.send(message)
        except Exception as e:
            raise ConnectionError(f"Failed to send message: {e}") from e

    def _receive_event(self) -> tuple[bytes, bytes]:
        """
        Read the channel until the next event is received.

        Return the header and received event contents excluding event header and padding
        bytes at the end of the message.
        """

        # Process the event header

        header_bytes = 8

        while True:
            try:
                # The situation when the event header is not fully received
                # with one 'recv' call is highly improbable.
                header = self._channel.recv(header_bytes)
            except socket.timeout as exc:
                raise ConnectionError("Timeout while waiting for event header") from exc
            except Exception as exc:
                raise ConnectionError(f"Failed to receive event header: {exc}") from exc

            if len(header) != header_bytes:
                raise ConnectionError(
                    f"Failed to receive event header from the broker, "
                    f"expected {header_bytes} bytes, got {len(header)} bytes"
                )

            event_type = header[4] & 0b00111111
            if not any(event_type == et.value for et in broker.EventType):
                raise ValueError(
                    f"Unknown event type: {event_type}, "
                    "expected one of the EventType values"
                )
            if event_type == broker.EventType.HEARTBEAT_REQ:
                print("Received heartbeat request.")
                self._send_raw(self._wrap_heartbeat_res_event())
            else:
                print("Received event with type: ", broker.EventType(event_type).name)
                break

        # Process the event body

        message = bytearray()

        # The first 4 bytes of the message contain message full size in bytes.
        # See also: bmqp_protocol.h / EventHeader
        remaining = int.from_bytes(header[:4], "big") - header_bytes
        while remaining > 0:
            part = self._channel.recv(remaining)
            if not part:
                raise ConnectionError(
                    "Connection closed by broker while receiving event body."
                )
            message += part
            remaining -= len(part)

        if len(message) < 1:
            raise ConnectionError("Received empty message from the broker")

        try:
            padding_bytes = message[-1]
        except IndexError as exc:
            raise ConnectionError(
                "Received message too short to contain padding byte"
            ) from exc

        # expect correct padding byte value
        if not 1 <= padding_bytes <= 4:
            raise ValueError(
                f"Invalid padding bytes value: {padding_bytes}, "
                "expected value in range [1, 4]"
            )

        body = message[:-padding_bytes]

        return header, body

    def open_channel(self, host: str, port: int) -> None:
        """
        Open a new channel to the broker using the specified 'host' / 'port'.
        This method is used to establish a connection for sending negotiation requests.
        """
        assert self._channel is None

        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        sock.connect((host, port))
        sock.settimeout(10.0)

        self._channel = sock

    def decode_event_bytes(self, response_header: bytes, response_body: bytes) -> dict:
        """
        Decode the received event header into a dictionary.
        This is used to parse the response from the broker.
        """
        # The response is expected to be in JSON format
        type_specific = response_header[6]  # 7th byte is typeSpecific

        if type_specific == broker.TypeSpecific.ENCODING_JSON:
            return json.loads(response_body.decode("utf-8"))
        if type_specific == broker.TypeSpecific.ENCODING_BER:
            print("BER encoding is not supported in open source Python.")
            raise ValueError("Not supported encoding in response")

        print(f"Unknown encoding type: {type_specific}")
        raise ValueError("Unknown encoding in response")

    def send_negotiation_request(self) -> dict:
        """
        Send a negotiation request to the broker.
        """
        assert self._channel is not None

        raw_client_identity = broker.CLIENT_IDENTITY_SCHEMA
        raw_client_identity["clientIdentity"]["clientType"] = "E_TCPCLIENT"

        self._send_raw(self._wrap_control_event(raw_client_identity))
        _, response_body = self._receive_event()

        return json.loads(response_body)

    def stop(self) -> None:
        """
        Disconnect from the broker.
        """
        if self._channel is not None:
            self._channel.close()
            self._channel = None
