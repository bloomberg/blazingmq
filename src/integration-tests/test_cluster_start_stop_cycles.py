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
Integration test that repeatedly starts and stops the cluster, verifying that
it becomes healthy after each start.
"""

import blazingmq.dev.it.testconstants as tc
from blazingmq.dev.it.fixtures import (
    Cluster,
    order,
    start_cluster,
)
from blazingmq.dev.it.process.client import Client

pytestmark = order(6)

NUM_CYCLES = 100


@start_cluster(start=True, wait_leader=True, wait_ready=True)
def test_start_stop_cycles(multi_node: Cluster):
    """
    Test that the cluster can be repeatedly stopped and started, becoming
    healthy after each start.
    """

    cluster = multi_node

    for i in range(NUM_CYCLES):
        cluster.stop_nodes()
        cluster.start_nodes(wait_leader=True, wait_ready=True)

        leader = cluster.last_known_leader
        assert leader is not None, f"cycle {i}: no leader elected"
        leader.wait_healthy()


@start_cluster(start=True, wait_leader=True, wait_ready=True)
def test_start_stop_cycles_with_traffic(
    multi_node: Cluster, domain_urls: tc.DomainUrls
):
    """
    Test that the cluster can be repeatedly stopped and started while clients
    reconnect and successfully produce/consume messages after each restart.

    Each cycle:
    1. Stop and restart the cluster nodes.
    2. Wait for the previous cycle's clients to reconnect and restore state.
    3. Produce and consume one message through the previous clients.
    4. Close and tear down the previous clients.
    5. Open fresh clients for the next cycle.

    Using fresh clients each cycle avoids accumulating reconnection backoff in
    the SDK's ReconnectingChannelFactory across rapid stop/start cycles.
    """

    cluster = multi_node
    uri = domain_urls.uri_priority

    nodes = cluster.nodes(alive=True)
    proxies = cluster.proxies(alive=True)
    brokers = nodes + proxies

    consumers = []
    producers = []
    for idx, broker in enumerate(brokers):
        consumer = broker.create_client(f"consumer_0_{idx}")
        consumer.open(uri, flags=["read"], succeed=True)
        consumers.append(consumer)

        producer = broker.create_client(f"producer_0_{idx}")
        producer.open(uri, flags=["write", "ack"], succeed=True)
        producers.append(producer)

    for i in range(NUM_CYCLES):
        for proxy in proxies:
            proxy.drain()

        cluster.stop_nodes()
        cluster.start_nodes(wait_leader=True, wait_ready=True)

        leader = cluster.last_known_leader
        assert leader is not None, f"cycle {i}: no leader elected"

        # Wait for previous clients connected to cluster nodes to reconnect.
        for idx in range(len(nodes)):
            assert consumers[idx].wait_state_restored(), (
                f"cycle {i}: {consumers[idx].name} failed to restore state"
            )

        # Wait for proxies to restore state.
        for proxy in proxies:
            assert proxy.outputs_regex("state restored", timeout=30), (
                f"cycle {i}: {proxy.name} failed to restore state"
            )

        # Produce and consume through the reconnected clients.
        for idx, producer in enumerate(producers):
            assert (
                producer.post(
                    uri, payload=[f"cycle{i}_msg{idx}"], wait_ack=True, block=True
                )
                == Client.e_SUCCESS
            ), f"cycle {i}: producer {idx} failed to post"

        for idx, consumer in enumerate(consumers):
            consumer.wait_push_event()
            msgs = consumer.list(uri, block=True)
            assert len(msgs) == 1, (
                f"cycle {i}: consumer {idx} got {len(msgs)} messages, expected 1"
            )
            assert consumer.confirm(uri, "*", block=True) == Client.e_SUCCESS

        # Tear down old clients and open fresh ones for the next cycle.
        for consumer in consumers:
            consumer.close(uri, succeed=True)
            consumer.exit_gracefully()
        for producer in producers:
            producer.close(uri, succeed=True)
            producer.exit_gracefully()

        consumers = []
        producers = []
        for idx, broker in enumerate(brokers):
            consumer = broker.create_client(f"consumer_{i + 1}_{idx}")
            consumer.open(uri, flags=["read"], succeed=True)
            consumers.append(consumer)

            producer = broker.create_client(f"producer_{i + 1}_{idx}")
            producer.open(uri, flags=["write", "ack"], succeed=True)
            producers.append(producer)

    # Clean up the last batch of clients.
    for consumer in consumers:
        consumer.close(uri, succeed=True)
        consumer.exit_gracefully()
    for producer in producers:
        producer.close(uri, succeed=True)
        producer.exit_gracefully()


@start_cluster(start=True, wait_leader=True, wait_ready=True)
def test_leader_restart_cycles(multi_node: Cluster):
    """
    Test that the leader node can be repeatedly stopped and restarted, with a
    new leader elected each time.
    """

    cluster = multi_node

    for i in range(NUM_CYCLES):
        leader = cluster.last_known_leader
        assert leader is not None, f"cycle {i}: no leader before stop"

        leader.stop()
        cluster.make_sure_node_stopped(leader)
        leader.drain()

        cluster.wait_leader()

        leader.start()
        leader.wait_until_started()
        leader.wait_status(wait_leader=True, wait_ready=True)
