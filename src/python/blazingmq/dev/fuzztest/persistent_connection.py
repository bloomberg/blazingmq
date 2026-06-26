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
Persistent TCP connection for long-running fuzz tests.

Provides a boofuzz TCPSocketConnection subclass that stays open across test
cases and auto-reconnects with handshake replay if the broker drops the
connection.
"""

import logging

import boofuzz

logger = logging.getLogger(__name__)


class PersistentConnection(boofuzz.TCPSocketConnection):
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
        except OSError:
            logger.info("Connection lost — reconnecting and replaying handshake")
            self._connected = False
            try:
                super().close()
            except OSError:
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
            except OSError:
                pass

    def shutdown(self):
        self._connected = False
        super().close()
