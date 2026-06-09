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

"""Broker-to-broker authentication integration tests using built-in auth."""

import blazingmq.dev.it.testconstants as tc

from blazingmq.dev.it.fixtures import (
    Cluster,
    order,
    tweak,
    start_cluster,
)

pytestmark = order(99)


@start_cluster(False)
@tweak.broker.app_config.authentication(
    {
        "authenticators": [
            {
                "name": "BasicAuthenticator",
                "settings": [
                    {"key": "broker", "value": {"stringVal": "secret"}},
                ],
            }
        ],
        "credentialProvider": {
            "name": "BasicCredentialProvider",
            "settings": [
                {"key": "username", "value": {"stringVal": "broker"}},
                {"key": "password", "value": {"stringVal": "secret"}},
            ],
        },
    }
)
def test_broker_to_broker_basic_auth(
    multi_node: Cluster,
    sc_domain_urls: tc.DomainUrls,  # pylint: disable=unused-argument
) -> None:
    """
    Test broker-to-broker authentication with matching credentials.

    Start two nodes so they connect and authenticate to each other. Verify
    that at least one node sent an outbound AuthenticationRequest and that no
    authentication failures occurred.
    """
    east1 = multi_node.start_node("east1")
    east2 = multi_node.start_node("east2")

    authn_sent = east1.outputs_regex(
        r"Sending AuthenticationRequest", timeout=10
    ) or east2.outputs_regex(r"Sending AuthenticationRequest", timeout=10)
    assert authn_sent, "No node sent an outbound AuthenticationRequest"

    assert not east1.outputs_regex(r"#AUTHENTICATION_FAILED", timeout=0)
    assert not east2.outputs_regex(r"#AUTHENTICATION_FAILED", timeout=0)


@start_cluster(False)
@tweak.broker.app_config.authentication(
    {
        "authenticators": [
            {
                "name": "BasicAuthenticator",
                "settings": [
                    {"key": "broker", "value": {"stringVal": "secret"}},
                ],
            }
        ],
        "credentialProvider": {
            "name": "BasicCredentialProvider",
            "settings": [
                {"key": "username", "value": {"stringVal": "broker"}},
                {"key": "password", "value": {"stringVal": "wrong"}},
            ],
        },
    }
)
def test_broker_to_broker_wrong_credentials(
    multi_node: Cluster,
    sc_domain_urls: tc.DomainUrls,  # pylint: disable=unused-argument
) -> None:
    """
    Test that broker-to-broker authentication fails with wrong credentials.

    Start two nodes. The BasicCredentialProvider sends 'broker:wrong' but the
    BasicAuthenticator expects 'broker:secret'. Verify that at least one node
    logs an authentication failure.
    """
    east1 = multi_node.start_node("east1")
    east2 = multi_node.start_node("east2")

    authn_failed = east1.outputs_regex(
        r"Authentication failed", timeout=10
    ) or east2.outputs_regex(r"Authentication failed", timeout=10)
    assert authn_failed, "Expected authentication failure not logged"


@start_cluster(False)
@tweak.broker.app_config.authentication(
    {
        "authenticators": [
            {
                "name": "BasicAuthenticator",
                "settings": [
                    {"key": "broker", "value": {"stringVal": "secret"}},
                ],
            }
        ],
    }
)
def test_broker_to_broker_no_credential_provider(
    multi_node: Cluster,
    sc_domain_urls: tc.DomainUrls,  # pylint: disable=unused-argument
) -> None:
    """
    Test that outbound broker-to-broker connections do not initiate an
    AuthenticationRequest when no credentialProvider is configured.

    Start two nodes with a BasicAuthenticator but no credentialProvider.
    Verify that no outbound AuthenticationRequest is sent.
    """
    east1 = multi_node.start_node("east1")
    east2 = multi_node.start_node("east2")

    # Wait for nodes to connect to each other.
    assert east1.outputs_regex(r"BMQbrkr started successfully", timeout=10)
    assert east2.outputs_regex(r"BMQbrkr started successfully", timeout=10)

    assert not east1.outputs_regex(r"Sending AuthenticationRequest", timeout=1)
    assert not east2.outputs_regex(r"Sending AuthenticationRequest", timeout=1)
