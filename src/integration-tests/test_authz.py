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
Authorization test suite using ONLY built-in authorizer.

This test suite validates authentication logic without any external plugins.
All tests use the built-in authorizer:
  - DefaultAuthorizer: Authorizes principals using policy documents defined in mqbpoly

This approach tests authorization scenarios without needing external plugins.
"""

import json
from pathlib import Path

import blazingmq.dev.it.testconstants as tc
import pytest
from blazingmq.dev.configurator.localsite import LocalSite
from blazingmq.dev.it.fixtures import (
    Cluster,
    order,
    start_cluster,
    tweak,
)
from blazingmq.dev.it.process.rawclient import RawClient
from blazingmq.schemas.mqbcfg import (
    AuthorizerConfig,
    AuthorizerPluginConfig,
    PluginSettingKeyValue,
    PluginSettingValue,
)

pytestmark = order(99)


# ==============================================================================
# Basic Authentication Tests
# ==============================================================================

POLICY_DEFS = {
    "simple": {
        "roles": [
            {
                "id": {"name": "anonymous"},
                "permissions": [
                    {"action": "connectClient"},
                    {"action": "queueRead", "resources": [{"id": "*"}]},
                    {"action": "queueWrite", "resources": [{"id": "*"}]},
                ],
            }
        ]
    }
}


@pytest.fixture()
def policies(tmp_path: Path):
    d = tmp_path / "policies"
    d.mkdir()
    policy_paths = {}
    for fname, policy in POLICY_DEFS.items():
        policy_file = d / f"{fname}.json"
        policy_file.write_text(json.dumps(policy))
        policy_paths[fname] = policy_file
    return policy_paths


def configure_cluster_policy(cluster: Cluster, policy: Path) -> None:
    config = cluster.configurator
    for broker in config.brokers.values():
        assert broker.config.app_config is not None
        broker.config.app_config.authorization = AuthorizerConfig(
            authorizer=AuthorizerPluginConfig(
                name="DefaultAuthorizer",
                settings=[
                    PluginSettingKeyValue(
                        key="policyPath",
                        value=PluginSettingValue(string_val=str(policy)),
                    )
                ],
            )
        )
        broker_config_site = LocalSite(cluster.work_dir / broker.name)
        config.deploy_broker_config(broker, broker_config_site)


@start_cluster(start=False)
def test_authorize_basic_success(
    policies,
    single_node: Cluster,
) -> None:
    """Test successful authentication with built-in DefaultAuthorizer."""
    configure_cluster_policy(single_node, policies["simple"])
    single_node.start(wait_ready=True)

    client = RawClient()
    admin_host, admin_port = single_node.admin_endpoint
    assert admin_host != None
    assert admin_port != None
    client.open_channel(admin_host, admin_port)

    nego_resp = client.negotiate()
    assert nego_resp["brokerResponse"]["result"]["code"] == 0

    client.stop()


@tweak.broker.app_config.authentication(
    {
        "authenticators": [
            {
                "name": "BasicAuthenticator",
                "settings": [
                    {"key": "user1", "value": {"stringVal": "password1"}},
                ],
            }
        ]
    }
)
@start_cluster(start=False)
def test_authorize_deny_unknown_user(
    policies,
    single_node: Cluster,
) -> None:
    """Test successful authentication with built-in DefaultAuthorizer."""
    configure_cluster_policy(single_node, policies["simple"])
    single_node.start(wait_ready=True)

    client = RawClient()
    admin_host, admin_port = single_node.admin_endpoint
    assert admin_host is not None
    assert admin_port is not None
    client.open_channel(admin_host, admin_port)

    auth_resp = client.authenticate("Basic", "user1:password1")
    assert auth_resp["authenticationResponse"]["status"]["code"] == 0

    nego_resp = client.negotiate()
    assert nego_resp["brokerResponse"]["result"]["code"] != 0

    client.stop()
