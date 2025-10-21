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
blazingmq.dev.it.process.admin


PURPOSE: Provide a BMQ admin client.
"""

import json
from typing import Union, Dict, Any

from blazingmq.schemas import broker
from .rawclient import RawClient


class AdminClient(RawClient):
    @classmethod
    def _make_admin_command(cls, message: str) -> bytes:
        """
        Wraps the specified 'message' with admin command and returns it as raw
        bytes control message.
        """
        command = broker.ADMIN_COMMAND_SCHEMA
        command["adminCommand"]["command"] = message

        return cls._wrap_control_event(command)

    def send_admin(self, admin_command: str) -> Union[dict, str]:
        """
        Send the specified 'admin_command' to the admin session currently opened
        on the broker. Return the command execution results.
        """
        self._send_raw(self._make_admin_command(admin_command))
        response = self.decode_event_bytes(*self._receive_event())

        return response["adminCommandResponse"]["text"]

    def connect(self, host: str, port: int) -> None:
        """
        Connect to the broker using the specified 'host' / 'port' and open
        an admin session.
        """
        assert self._channel is None

        self.open_channel(host, port)

        admin_client_identity = broker.CLIENT_IDENTITY_SCHEMA
        admin_client_identity["clientIdentity"]["clientType"] = "E_TCPADMIN"

        self._send_raw(self._wrap_control_event(admin_client_identity))
        self._receive_event()

    # Admin APIs
    def get_domain_config(self, domain_name: str) -> Dict[Any, Any]:
        """
        Send the "DOMAINS DOMAIN <domain_name> INFOS" admin command, parse the received response and
        return the domain configuration as a structured dictionary.
        """

        admin_response: str = self.send_admin(f"ENCODING JSON_COMPACT DOMAINS DOMAIN {domain_name} INFOS")

        # Top-level domain information has the following structure:
        # {
        #     "domainInfo": {
        #         "name": str,
        #         "configJson": str,
        #         ...
        #     }
        # }
        # Need to look up to configJson and parse the string
        domain_stats = json.loads(admin_response)
        config_json = domain_stats["domainInfo"]["configJson"]
        return json.loads(config_json)
