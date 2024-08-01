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
This suite of test cases verifies that admin commands routed to a member node
undergo the proper routing to the relevant primary/cluster if that command
requires it.

This test suite does not concern itself with verifying the functional details
of commands; only that they get routed to the proper nodes.
"""

import blazingmq.dev.it.testconstants as tc
from blazingmq.dev.it.fixtures import (
    Cluster,
    multi_node,
)
from blazingmq.dev.it.process.admin import AdminClient
from blazingmq.dev.it.process.client import Client
import json
import multiprocessing.pool


def test_primary_rerouting(multi_node: Cluster) -> None:
    """
    Test: commands intended only for primary node are automatically routed to
          primary node and response it sent back when executed from non-primary

    Stage 1: Find non-primary
    - Start admin client
    - Find node which is not known leader
    - Connect to that node
    - Open FANOUT domain

    Stage 3: Try all primary-only commands:
    - DOMAINS DOMAIN <domain> PURGE
    - CLUSTERS CLUSTER <name> FORCE_GC_QUEUES
    - CLUSTERS CLUSTER <name> STORAGE PARTITION <partitionId> ENABLE/DISABLE
    """

    admin = AdminClient()

    # find the first node which is not a known leader
    for node in multi_node.nodes():
        if node != node.last_known_leader:
            member_node = node
            break

    admin.connect(member_node.config.host, int(member_node.config.port))

    # member_node should not be the primary, so lets try sending primary
    # only commands to them

    # need to open a domain first
    proxies = multi_node.proxy_cycle()
    proxy = next(proxies)
    producer: Client = proxy.create_client("producer")

    producer.open(tc.URI_FANOUT, flags=["write,ack"], succeed=True)

    # Try DOMAINS DOMAIN <domain> PURGE
    res = admin.send_admin(f"DOMAINS DOMAIN {tc.DOMAIN_FANOUT} PURGE")

    # response should say "Purged XX messages..."
    assert "Purged" in res

    # Try DOMAINS DOMAIN <name> QUEUE <queue_name> PURGE <appId>
    res = admin.send_admin(
        f"DOMAINS DOMAIN {tc.DOMAIN_FANOUT} QUEUE {tc.TEST_QUEUE} PURGE {tc.TEST_APPIDS[0]}"
    )
    assert "Purged" in res

    # Try CLUSTERS CLUSTER <name> FORCE_GC_QUEUES
    res = admin.send_admin(f"CLUSTERS CLUSTER {multi_node.name} FORCE_GC_QUEUES")
    assert "SUCCESS" in res

    # Try CLUSTERS CLUSTER <name> STORAGE PARTITION <partitionId> ENABLE/DISABLE
    res = admin.send_admin(
        f"CLUSTERS CLUSTER {multi_node.name} STORAGE PARTITION 0 ENABLE"
    )
    assert "SUCCESS" in res

    res = admin.send_admin(
        f"CLUSTERS CLUSTER {multi_node.name} STORAGE PARTITION 0 DISABLE"
    )
    assert "SUCCESS" in res

    admin.stop()


def test_cluster_rerouting(multi_node: Cluster) -> None:
    """
    Test: commands intended for cluster are routed to all nodes in the cluster
          regardless of the node the command is initially sent to

    Stage 1: Setup client
    - Start admin client
    - Connect to arbitrary node
    - Open PRIORITY domain

    Stage 2: Try all cluster commands:
    - DOMAINS RECONFIGURE <domain>
    - CLUSTERS CLUSTER <name> STORAGE REPLICATION SET_ALL <param> <value>
    - CLUSTERS CLUSTER <name> STORAGE REPLICATION GET_ALL <param> <value>
    - CLUSTERS CLUSTER <name> STATE ELECTOR SET_ALL <param> <value>
    - CLUSTERS CLUSTER <name> STATE ELECTOR GET_ALL <param> <value>
    """

    admin = AdminClient()

    node = multi_node.nodes()[0]

    num_nodes = len(multi_node.nodes())

    admin.connect(node.config.host, int(node.config.port))

    proxies = multi_node.proxy_cycle()
    proxy = next(proxies)
    producer: Client = proxy.create_client("producer")

    producer.open(tc.URI_PRIORITY, flags=["write,ack"], succeed=True)

    # Try DOMAINS RECONFIGURE <domain>
    res = admin.send_admin(f"DOMAINS RECONFIGURE {tc.DOMAIN_PRIORITY}")

    # Expect num_nodes "SUCCESS" responses
    success_count = res.split().count("SUCCESS")
    assert success_count == num_nodes

    # Try CLUSTERS CLUSTER <name> STORAGE REPLICATION SET_ALL <param> <value>
    res = admin.send_admin(
        f"CLUSTERS CLUSTER {multi_node.name} STORAGE REPLICATION SET_ALL quorum 2"
    )
    success_count = res.split().count("Quorum")
    assert success_count == num_nodes

    # GET_ALL
    res = admin.send_admin(
        f"CLUSTERS CLUSTER {multi_node.name} STORAGE REPLICATION SET_ALL quorum 2"
    )
    success_count = res.split().count("Quorum")
    assert success_count == num_nodes

    # Try CLUSTERS CLUSTER <name> STATE ELECTOR SET_ALL <param> <value>
    res = admin.send_admin(
        f"CLUSTERS CLUSTER {multi_node.name} STATE ELECTOR SET_ALL quorum 2"
    )
    success_count = res.split().count("Quorum")
    assert success_count == num_nodes

    # GET_ALL
    res = admin.send_admin(
        f"CLUSTERS CLUSTER {multi_node.name} STATE ELECTOR SET_ALL quorum 2"
    )
    success_count = res.split().count("Quorum")
    assert success_count == num_nodes

    admin.stop()


def test_multi_response_encoding(multi_node: Cluster):
    """
    Test: JSON encoding options work with multiple responses (when routing to
          multiple nodes)

    Stage 1: Setup client
    - Start admin client
    - Connect to arbitrary node
    - Open PRIORITY domain

    Stage 2: Test compact json formatting

    Stage 3: Test pretty json formatting

    """

    def is_compact(json_str: str) -> bool:
        return "    " not in json_str

    admin = AdminClient()

    node = multi_node.nodes()[0]
    num_nodes = len(multi_node.nodes())

    admin.connect(node.config.host, int(node.config.port))

    proxies = multi_node.proxy_cycle()
    proxy = next(proxies)
    producer: Client = proxy.create_client("producer")

    producer.open(tc.URI_PRIORITY, flags=["write,ack"], succeed=True)

    # Stage 2: Test Compact Encoding
    cmds = [
        json.dumps(
            {
                "domains": {"reconfigure": {"domain": tc.DOMAIN_PRIORITY}},
                "encoding": "JSON_COMPACT",
            }
        ),
        f"ENCODING JSON_COMPACT DOMAINS RECONFIGURE {tc.DOMAIN_PRIORITY}",
    ]
    for cmd in cmds:
        res = admin.send_admin(cmd)
        res_json = json.loads(res)
        assert "responses" in res_json
        assert is_compact(res)
        # we should have gotten num_nodes responses
        responses = res_json["responses"]
        assert len(responses) == num_nodes

    # Stage 3: Test Pretty Encoding
    cmds = [
        json.dumps(
            {
                "domains": {"reconfigure": {"domain": tc.DOMAIN_PRIORITY}},
                "encoding": "JSON_PRETTY",
            }
        ),
        f"ENCODING JSON_PRETTY DOMAINS RECONFIGURE {tc.DOMAIN_PRIORITY}",
    ]
    for cmd in cmds:
        res = admin.send_admin(cmd)
        res_json = json.loads(res)
        assert "responses" in res_json
        assert not is_compact(res)
        # we should have gotten num_nodes responses
        responses = res_json["responses"]
        assert len(responses) == num_nodes

    admin.stop()


def test_concurrently_routed_commands(multi_node: Cluster):
    """
    Test: Ensure issuing all-cluster commands to each node in parallel does
          not cause the system to fail.

    Stage 1: Connect clients
    Stage 2: Issue command in parallel
    Stage 3: Expect
    """
    # Connect a client to each node in the cluster
    clients = []
    for node in multi_node.nodes():
        client = AdminClient()
        client.connect(node.config.host, int(node.config.port))
        clients.append(client)
    num_nodes = len(multi_node.nodes())

    def exec_command(client, cmd):
        response = client.send_admin(cmd)
        return response

    pool = multiprocessing.pool.ThreadPool(len(multi_node.nodes()))
    cmd = f"DOMAINS RECONFIGURE {tc.DOMAIN_PRIORITY}"
    result_list = pool.starmap(exec_command, ((client, cmd) for client in clients))

    # Ensure we got 4 responses back each with 4 successes
    for result in result_list:
        assert result.split().count("SUCCESS") == num_nodes

    # Cleanly shut down
    for client in clients:
        client.stop()
