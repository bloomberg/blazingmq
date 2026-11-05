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
TCP proxy that forwards client connections to a broker and records the bytes
sent by each client.
"""

import socket
import threading
from typing import List, Optional


def _pump(
    source: socket.socket, destination: socket.socket, recorded: Optional[bytearray]
) -> None:
    """
    Forward bytes from 'source' to 'destination' until 'source' is closed,
    appending them to 'recorded' if it is not None.  Then stop sending to
    'destination'.
    """

    while True:
        try:
            data = source.recv(65536)
        except OSError:
            data = b""
        if not data:
            break
        if recorded is not None:
            recorded += data
        try:
            destination.sendall(data)
        except OSError:
            break

    try:
        destination.shutdown(socket.SHUT_WR)
    except OSError:
        pass


class RecordingProxy:
    """
    Forward each accepted connection to the broker on 'broker_port', and keep
    the bytes sent by the client once the connection is closed.
    """

    def __init__(self, broker_port: int):
        self._broker_port = broker_port
        self._server = socket.create_server(("localhost", 0))
        self.port: int = self._server.getsockname()[1]
        self._condition = threading.Condition()
        self._num_active = 0
        self._recordings: List[bytes] = []
        threading.Thread(target=self._accept, daemon=True).start()

    def take_recordings(self, timeout: float) -> List[bytes]:
        """
        Wait until all connections are closed, and return the bytes sent by
        each client since the last call.  Raise 'RuntimeError' if connections
        are still open after 'timeout' seconds.
        """

        with self._condition:
            if not self._condition.wait_for(lambda: self._num_active == 0, timeout):
                raise RuntimeError("proxied connections were not closed")
            recordings, self._recordings = self._recordings, []
        return recordings

    def close(self) -> None:
        """
        Stop accepting connections.
        """

        self._server.close()

    def _accept(self) -> None:
        while True:
            try:
                client, _ = self._server.accept()
            except OSError:
                return
            upstream = socket.create_connection(("localhost", self._broker_port))
            with self._condition:
                self._num_active += 1
            threading.Thread(
                target=self._serve, args=(client, upstream), daemon=True
            ).start()

    def _serve(self, client: socket.socket, upstream: socket.socket) -> None:
        recorded = bytearray()
        downstream = threading.Thread(
            target=_pump, args=(upstream, client, None), daemon=True
        )
        downstream.start()
        _pump(client, upstream, recorded)
        downstream.join()
        client.close()
        upstream.close()

        with self._condition:
            self._num_active -= 1
            self._recordings.append(bytes(recorded))
            self._condition.notify_all()
