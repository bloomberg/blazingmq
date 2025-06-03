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
This suite of test cases is for authentication.
"""

from pathlib import Path
import os

from blazingmq.dev.it.process.rawclient import RawClient

from blazingmq.dev.it.fixtures import (  # pylint: disable=unused-import
    Cluster,
    order,
    single_node,
    tweak,
    cluster,
)

# PLUGIN_DIR = str(Path(os.environ.get("BLAZINGMQ_BUILD_DIR")) / "src/plugins/")
PLUGIN_DIR = str(Path(__file__).parent.parent.parent / "build/blazingmq/src/plugins")


pytestmark = order(99)

libraries = tweak.broker.app_config.plugins.libraries([PLUGIN_DIR])

config_authentication = tweak.broker.app_config.authentication(
    {
        "plugins": [
            {"name": "FailAuthenticator", "configs": []},
            {"name": "PassAuthenticator", "configs": []},
        ],
        "fallbackPrincipal": "defaultFallbackPrincipal",
    }
)


@tweak.broker.app_config.plugins.enabled(
    ["PassAuthenticator"],
)
@libraries
@config_authentication
def test_authenticate_pass(single_node: Cluster) -> None:
    """
    Test sending an authentication message using RawClient.
    """

    # Start the raw client
    client = RawClient()
    client.open_channel(*single_node.admin_endpoint)

    auth_resp = client.send_authentication_request("Basic", "username:password")
    print(f"Authentication Response: {auth_resp}")
    assert auth_resp["authenticateResponse"]["status"]["code"] == 0

    nego_resp = client.send_negotiation_request()
    assert nego_resp["brokerResponse"]["result"]["code"] == 0

    client.stop()


def test_authenticate_fail(single_node: Cluster) -> None:
    """
    Test sending an authentication message using RawClient.
    """

    # Start the raw client
    client = RawClient()
    client.open_channel(*single_node.admin_endpoint)

    auth_resp = client.send_authentication_request("Basic", "username:password")
    print(f"Authentication Response: {auth_resp}")
    assert auth_resp["authenticateResponse"]["status"]["code"] != 0

    client.stop()
