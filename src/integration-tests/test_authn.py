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
import threading
import pytest

import blazingmq.dev.it.testconstants as tc
from blazingmq.dev.paths import paths
from blazingmq.dev.it.process.rawclient import RawClient
from blazingmq.dev.it.process.admin import AdminClient

from blazingmq.dev.it.fixtures import (  # pylint: disable=unused-import
    Cluster,
    order,
    single_node,
    multi_node,
    tweak,
    cluster,
)

PLUGIN_DIR = str(Path(__file__).parent.parent.parent / paths.build_dir / "src/plugins")


pytestmark = order(99)

libraries = tweak.broker.app_config.plugins.libraries([PLUGIN_DIR])

config_authentication = tweak.broker.app_config.authentication(
    {
        "plugins": [
            {"name": "FailAuthenticator", "configs": []},
            {"name": "PassAuthenticator", "configs": []},
            {"name": "BasicAuthenticator", "configs": []},
        ]
    }
)


@tweak.broker.app_config.plugins.enabled(
    ["PassAuthenticator"],
)
@libraries
@config_authentication
def test_authenticate_pass_basic(single_node: Cluster) -> None:
    """
    This test uses the PassAuthenticator plugin to simulate a successful authentication.
    """

    # Start the raw client
    client = RawClient()
    client.open_channel(*single_node.admin_endpoint)

    auth_resp = client.send_authentication_request("Basic", "username:password")
    assert auth_resp["authenticateResponse"]["status"]["code"] == 0

    nego_resp = client.send_negotiation_request()
    assert nego_resp["brokerResponse"]["result"]["code"] == 0

    client.stop()


@tweak.broker.app_config.plugins.enabled(
    ["FailAuthenticator"],
)
@libraries
@config_authentication
def test_authenticate_fail_basic(single_node: Cluster) -> None:
    """
    This test uses the FailAuthenticator plugin to simulate a failed authentication.
    """

    # Start the raw client
    client = RawClient()
    client.open_channel(*single_node.admin_endpoint)

    auth_resp = client.send_authentication_request("Basic", "username:password")
    assert auth_resp["authenticateResponse"]["status"]["code"] != 0

    client.stop()


@tweak.broker.app_config.plugins.enabled(
    ["PassAuthenticator"],
)
@libraries
@config_authentication
def test_authenticate_pass_concurrent(single_node: Cluster) -> None:
    """
    Test sending an authentication message using RawClient.
    This test uses the PassAuthenticator plugin to simulate successful concurrent authentications.
    """

    num_threads = 8
    results = [None] * num_threads
    threads = []

    def auth_worker(idx):
        client = RawClient()
        client.open_channel(*single_node.admin_endpoint)
        auth_resp = client.send_authentication_request("Basic", f"user{idx}:password")
        results[idx] = auth_resp["authenticateResponse"]["status"]["code"]
        client.stop()

    for i in range(num_threads):
        t = threading.Thread(target=auth_worker, args=(i,))
        threads.append(t)
        t.start()

    for t in threads:
        t.join()

    # Assert all authentications succeeded
    assert all(code == 0 for code in results), f"Some authentications failed: {results}"


@tweak.broker.app_config.plugins.enabled(
    ["BasicAuthenticator"],
)
@libraries
@config_authentication
def test_reauthenticate_basic_pass(single_node: Cluster) -> None:
    """
    This test checks the behavior of reauthentication with Basic mechanism.
    It simulates a scenario where the initial authentication is successful,
    and a subsequent reauthentication with a change of credential successes.
    """

    # Start the raw client
    client = RawClient()
    client.open_channel(*single_node.admin_endpoint)

    # Pass: Sending authentication request with Basic mechanism
    # and valid credentials
    auth_resp = client.send_authentication_request("Basic", "user1:password1")
    assert auth_resp["authenticateResponse"]["status"]["code"] == 0

    # Pass: Sending negotiation request after authentication
    nego_resp = client.send_negotiation_request()
    assert nego_resp["brokerResponse"]["result"]["code"] == 0

    # Pass: Sending reauthentication request with valid credentials
    auth_resp = client.send_authentication_request("Basic", "user1:password1")
    assert auth_resp["authenticateResponse"]["status"]["code"] == 0

    client.stop()


@tweak.broker.app_config.plugins.enabled(
    ["BasicAuthenticator"],
)
@libraries
@config_authentication
def test_reauthenticate_basic_fail(single_node: Cluster) -> None:
    """
    This test checks the behavior of reauthentication with Basic mechanism.
    It simulates a scenario where the initial authentication is successful,
    but a subsequent reauthentication fails due to invalid credentials.
    The test ensures that the client cannot proceed with negotiation after a failed reauthentication.
    """

    # Start the raw client
    client = RawClient()
    client.open_channel(*single_node.admin_endpoint)

    # Pass: Sending authentication request with Basic mechanism
    # and valid credentials
    auth_resp = client.send_authentication_request("Basic", "user1:password1")
    assert auth_resp["authenticateResponse"]["status"]["code"] == 0

    # Pass: Sending negotiation request after authentication
    nego_resp = client.send_negotiation_request()
    assert nego_resp["brokerResponse"]["result"]["code"] == 0

    # Fail: Sending reauthentication request with invalid credentials
    auth_resp = client.send_authentication_request("Basic", "user1:password2")
    assert auth_resp["authenticateResponse"]["status"]["code"] != 0

    # Fail: Sending negotiation request after failed reauthentication
    with pytest.raises(ConnectionError):
        nego_resp = client.send_negotiation_request()

    client.stop()


def test_default_anony_credential(single_node: Cluster) -> None:
    """
    This test sends a negotiation request without prior authentication.
    It succees without enabling any authentication plugin since we use
    didn't provide an anonymous credential so the default is used.
    """

    # Start the raw client
    client = RawClient()
    client.open_channel(*single_node.admin_endpoint)

    # Pass: Sending negotiation request without prior authentication
    nego_resp = client.send_negotiation_request()
    assert nego_resp["brokerResponse"]["result"]["code"] == 0

    client.stop()


def test_default_anony_credential_multi(
    multi_node: Cluster,
    sc_domain_urls: tc.DomainUrls,  # pylint: disable=unused-argument
) -> None:
    """
    This test sends a negotiation request without prior authentication
    for a multi-node cluster.
    It succees without enabling any authentication plugin since we use
    didn't provide an anonymous credential so the default is used.
    """

    # Start the raw client
    client = RawClient()
    client.open_channel(*multi_node.admin_endpoint)

    # Pass: Sending negotiation request without prior authentication
    nego_resp = client.send_negotiation_request()
    assert nego_resp["brokerResponse"]["result"]["code"] == 0

    client.stop()


@tweak.broker.app_config.authentication({"anonymousCredential": {"disallow": {}}})
def test_anony_disallow_without_authentication(single_node: Cluster) -> None:
    """
    This test sends a negotiation request without prior authentication
    and configures the broker to disallow anonymous credentials.
    It should fail since the broker does not allow anonymous credential.
    """

    # Start the raw client
    client = RawClient()
    client.open_channel(*single_node.admin_endpoint)

    # Fail: Sending negotiation request without prior authentication
    with pytest.raises(ConnectionError):
        client.send_negotiation_request()

    client.stop()


@libraries
@tweak.broker.app_config.plugins.enabled(
    ["BasicAuthenticator"],
)
@tweak.broker.app_config.authentication(
    {
        "plugins": [
            {"name": "BasicAuthenticator", "configs": []},
        ],
        "anonymousCredential": {
            "credential": {"mechanism": "Basic", "identity": "user1:password1"}
        },
    }
)
def test_anony_credential_specified_correct(single_node: Cluster) -> None:
    """
    This test sends a negotiation request without prior authentication.
    It succees since we provide a correct anonymous credential to use.
    """

    # Start the raw client
    client = RawClient()
    client.open_channel(*single_node.admin_endpoint)

    # Pass: Sending negotiation request without prior authentication
    nego_resp = client.send_negotiation_request()
    assert nego_resp["brokerResponse"]["result"]["code"] == 0

    client.stop()


@libraries
@tweak.broker.app_config.plugins.enabled(
    ["BasicAuthenticator"],
)
@tweak.broker.app_config.authentication(
    {
        "plugins": [
            {"name": "BasicAuthenticator", "configs": []},
        ],
        "anonymousCredential": {
            "credential": {"mechanism": "Basic", "identity": "user1:password2"}
        },
    }
)
def test_anony_credential_specified_wrong(single_node: Cluster) -> None:
    """
    This test sends a negotiation request without prior authentication.
    It fails since we provide a wrong anonymous credential to use.
    """

    # Start the raw client
    client = RawClient()
    client.open_channel(*single_node.admin_endpoint)

    # Fail: Sending negotiation request without prior authentication
    with pytest.raises(ConnectionError):
        client.send_negotiation_request()

    client.stop()


@libraries
@tweak.broker.app_config.plugins.enabled(
    ["BasicAuthenticator"],
)
@tweak.broker.app_config.authentication(
    {
        "plugins": [
            {"name": "FailAuthenticator", "configs": []},
            {"name": "PassAuthenticator", "configs": []},
            {"name": "BasicAuthenticator", "configs": []},
        ],
        "anonymousCredential": {
            "credential": {"mechanism": "NotExisted", "identity": ""}
        },
    }
)
def test_anony_credential_specified_without_plugin_support(
    single_node: Cluster,
) -> None:
    """
    This test sends a negotiation request without prior authentication.
    It fails since we provide an anonymous credential without the corresponding plugin.
    """

    # Start the raw client
    client = RawClient()
    client.open_channel(*single_node.admin_endpoint)

    # Fail: Sending negotiation request without prior authentication
    with pytest.raises(ConnectionError):
        client.send_negotiation_request()

    client.stop()


def test_anony_credential_with_admin(single_node: Cluster) -> None:
    """
    This test sends an admin command without prior authentication.
    It succeeds since we use the default anonymous credential.
    """

    # Start the admin client
    admin = AdminClient()
    admin.connect(*single_node.admin_endpoint)

    # Check basic "help" command
    assert (
        "This process responds to the following CMD subcommands:"
        in admin.send_admin("help")
    )

    # Stop the admin session
    admin.stop()
