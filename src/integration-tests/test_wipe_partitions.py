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
Testing node recovery after wiping partition files.
"""

import blazingmq.dev.it.testconstants as tc
import pytest
from blazingmq.dev.it.fixtures import (
    Cluster,
    order,
    test_logger,
    tweak,
)
from blazingmq.dev.it.cluster_util import (
    stop_cluster_and_compare_journal_files,
    wipe_files,
)
from blazingmq.dev.it.util import wait_until

pytestmark = order(4)


@tweak.cluster.queue_operations.shutdown_timeout_ms(100)
def test_wipe_single_partition_files(
    multi_node: Cluster,
    domain_urls: tc.DomainUrls,
) -> None:
    """
    Test that a replica can recover after one partition's files are wiped,
    and that a consumer connected to the healed replica can receive messages.
    - start cluster
    - post messages
    - stop a replica
    - wipe partition 0's journal, data, and qlist files on the replica
    - restart the replica
    - connect a consumer to the healed replica, verify messages are received
    - verify the replica synchronizes with the primary
    """
    cluster = multi_node
    uri_priority = domain_urls.uri_priority

    leader = cluster.last_known_leader
    proxy = next(cluster.proxy_cycle())

    producer = proxy.create_client("producer")
    producer.open(uri_priority, flags=["write,ack"], succeed=True)

    # Post messages
    for i in range(1, 5):
        producer.post(uri_priority, [f"msg{i}"], succeed=True, wait_ack=True)

    replica = cluster.nodes(exclude=leader)[0]

    # Stop the replica
    replica.stop()
    cluster.make_sure_node_stopped(replica)
    replica.drain()

    # Wipe partition 0's files (journal + data + qlist)
    storage_dir = str(cluster.work_dir.joinpath(replica.name, "storage"))
    wipe_files(
        ["*0*.bmq_journal", "*0*.bmq_data", "*0*.bmq_qlist"],
        storage_dir,
    )

    # Restart the replica
    replica.start()
    replica.wait_until_started()

    assert replica.outputs_substr("Cluster (itCluster) is available", 10), (
        f"Replica {replica} did not become available within 10s after "
        f"partition 0 files were wiped"
    )

    assert leader == cluster.last_known_leader, (
        f"Leader {leader} is not cluster.last_known_leader {cluster.last_known_leader}"
    )

    # Verify a consumer connected to the healed replica can receive messages
    replica_consumer = replica.create_client("replica_consumer")
    replica_consumer.open(uri_priority, flags=["read"], succeed=True)

    assert wait_until(
        lambda: len(replica_consumer.list(uri_priority, block=True)) == 4,
        3,
    ), "Consumer on healed replica did not receive 4 messages"

    replica_consumer.confirm(uri_priority, "*", succeed=True)
    replica_consumer.close(uri_priority, succeed=True)

    stop_cluster_and_compare_journal_files(leader.name, replica.name, cluster)


@tweak.cluster.queue_operations.shutdown_timeout_ms(100)
def test_wipe_all_partitions_and_csl(
    multi_node: Cluster,
    domain_urls: tc.DomainUrls,
) -> None:
    """
    Test that a replica can recover after ALL partition files and the CSL
    file are wiped, and that a consumer connected to the healed replica can
    receive messages.
    - start cluster
    - post messages
    - stop a replica
    - wipe all journal, data, qlist, and CSL files on the replica
    - restart the replica
    - connect a consumer to the healed replica, verify messages are received
    - verify the replica synchronizes with the primary
    """
    cluster = multi_node
    uri_priority = domain_urls.uri_priority

    leader = cluster.last_known_leader
    proxy = next(cluster.proxy_cycle())

    producer = proxy.create_client("producer")
    producer.open(uri_priority, flags=["write,ack"], succeed=True)

    # Post messages
    for i in range(1, 5):
        producer.post(uri_priority, [f"msg{i}"], succeed=True, wait_ack=True)

    replica = cluster.nodes(exclude=leader)[0]

    # Stop the replica
    replica.stop()
    cluster.make_sure_node_stopped(replica)
    replica.drain()

    # Wipe ALL partition files and CSL file
    storage_dir = str(cluster.work_dir.joinpath(replica.name, "storage"))
    wipe_files(
        ["*.bmq_journal", "*.bmq_data", "*.bmq_qlist", "*csl*"],
        storage_dir,
    )

    # Restart the replica
    replica.start()
    replica.wait_until_started()

    assert replica.outputs_substr("Cluster (itCluster) is available", 10), (
        f"Replica {replica} did not become available within 10s after "
        f"all partition files were wiped"
    )

    assert leader == cluster.last_known_leader, (
        f"Leader {leader} is not cluster.last_known_leader {cluster.last_known_leader}"
    )

    # Verify a consumer connected to the healed replica can receive messages
    replica_consumer = replica.create_client("replica_consumer")
    replica_consumer.open(uri_priority, flags=["read"], succeed=True)

    assert wait_until(
        lambda: len(replica_consumer.list(uri_priority, block=True)) == 4,
        3,
    ), "Consumer on healed replica did not receive 4 messages"

    replica_consumer.confirm(uri_priority, "*", succeed=True)
    replica_consumer.close(uri_priority, succeed=True)

    stop_cluster_and_compare_journal_files(leader.name, replica.name, cluster)


@tweak.cluster.queue_operations.shutdown_timeout_ms(100)
def test_wipe_csl_only(
    multi_node: Cluster,
    domain_urls: tc.DomainUrls,
) -> None:
    """
    Test that a replica can recover after only the CSL file is wiped, and that
    a consumer connected to the healed replica can receive messages.
    - start cluster
    - post messages
    - stop a replica
    - wipe only the CSL file on the replica
    - restart the replica
    - connect a consumer to the healed replica, verify messages are received
    - verify the replica synchronizes with the primary
    """
    cluster = multi_node
    uri_priority = domain_urls.uri_priority

    leader = cluster.last_known_leader
    proxy = next(cluster.proxy_cycle())

    producer = proxy.create_client("producer")
    producer.open(uri_priority, flags=["write,ack"], succeed=True)

    # Post messages
    for i in range(1, 5):
        producer.post(uri_priority, [f"msg{i}"], succeed=True, wait_ack=True)

    replica = cluster.nodes(exclude=leader)[0]

    # Stop the replica
    replica.stop()
    cluster.make_sure_node_stopped(replica)
    replica.drain()

    # Wipe only the CSL file
    storage_dir = str(cluster.work_dir.joinpath(replica.name, "storage"))
    wipe_files(["*csl*"], storage_dir)

    # Restart the replica
    replica.start()
    replica.wait_until_started()

    assert replica.outputs_substr("Cluster (itCluster) is available", 10), (
        f"Replica {replica} did not become available within 10s after "
        f"CSL file was wiped"
    )

    assert leader == cluster.last_known_leader, (
        f"Leader {leader} is not cluster.last_known_leader {cluster.last_known_leader}"
    )

    # Verify a consumer connected to the healed replica can receive messages
    replica_consumer = replica.create_client("replica_consumer")
    replica_consumer.open(uri_priority, flags=["read"], succeed=True)

    assert wait_until(
        lambda: len(replica_consumer.list(uri_priority, block=True)) == 4,
        3,
    ), "Consumer on healed replica did not receive 4 messages"

    replica_consumer.confirm(uri_priority, "*", succeed=True)
    replica_consumer.close(uri_priority, succeed=True)

    stop_cluster_and_compare_journal_files(leader.name, replica.name, cluster)


@tweak.cluster.queue_operations.shutdown_timeout_ms(100)
def test_wipe_storage_all_but_one_node(
    fsm_multi_cluster: Cluster,
    sc_domain_urls: tc.DomainUrls,
) -> None:
    """
    Test recovery when all partition files are wiped from every node except one.
    The surviving node should become primary for the partitions it still has
    data for, and the wiped nodes should heal from it.

    - start cluster
    - post messages
    - stop all nodes
    - wipe all partition files + CSL on every node except one
    - restart all nodes, but make sure the surviving node is part of the initial
      leader quorum
    - verify the cluster becomes available
    - connect a consumer, verify messages are received

    NOTE: This test is run in FSM mode only; in legacy mode, the cluster cannot
    heal when the majority of nodes have empty storage simultaneously.
    """
    cluster = fsm_multi_cluster
    uri_priority = sc_domain_urls.uri_priority

    leader = cluster.last_known_leader
    proxy = next(cluster.proxy_cycle())

    producer = proxy.create_client("producer")
    producer.open(uri_priority, flags=["write,ack"], succeed=True)

    # Post messages
    for i in range(1, 5):
        producer.post(uri_priority, [f"msg{i}"], succeed=True, wait_ack=True)

    # Stop all nodes
    cluster.stop_nodes()
    for node in cluster.nodes():
        node.drain()

    # Wipe all partition files + CSL on every node except the surviving one
    surviving_node = cluster.nodes(exclude=leader)[0]
    wiped_nodes = cluster.nodes(exclude=surviving_node)
    for node in wiped_nodes:
        storage_dir = str(cluster.work_dir.joinpath(node.name, "storage"))
        wipe_files(
            ["*.bmq_journal", "*.bmq_data", "*.bmq_qlist", "*csl*"],
            storage_dir,
        )

    # Restart all nodes, but make sure the surviving node is part of the initial
    # leader quorum
    first_batch = [surviving_node] + wiped_nodes[:2]
    last_node = wiped_nodes[2]
    for node in first_batch:
        node.start()
        node.wait_until_started()

    cluster.wait_status(wait_leader=True, wait_ready=False)
    assert surviving_node == cluster.last_known_leader, (
        f"Surviving node {surviving_node} is not cluster.last_known_leader "
        f"{cluster.last_known_leader}"
    )

    last_node.start()
    last_node.wait_until_started()
    last_node.wait_status(wait_leader=True, wait_ready=True)

    # Verify a consumer connected to a wiped replica can receive messages
    wiped_node = wiped_nodes[0]
    consumer = wiped_node.create_client("consumer")
    consumer.open(uri_priority, flags=["read"], succeed=True)

    assert wait_until(
        lambda: len(consumer.list(uri_priority, block=True)) == 4,
        3,
    ), "Consumer on wiped node did not receive 4 messages"

    consumer.confirm(uri_priority, "*", succeed=True)
    consumer.close(uri_priority, succeed=True)

    # Compare surviving node vs wiped node
    stop_cluster_and_compare_journal_files(
        surviving_node.name, wiped_node.name, cluster
    )


@tweak.cluster.queue_operations.shutdown_timeout_ms(100)
def test_wipe_qlist_all_nodes(
    multi_node: Cluster,
    domain_urls: tc.DomainUrls,
) -> None:
    """
    Test that the cluster survives when qlist files are wiped from ALL nodes.
    Without qlist, the broker cannot map queues to partition data files, so
    old messages are orphaned and lost.  The cluster should still come up
    cleanly and accept new messages.
    - start cluster
    - post messages
    - stop all nodes
    - wipe qlist files from every node
    - restart all nodes
    - verify the cluster becomes available
    - verify the queue is functional by posting and consuming new messages
    """
    cluster = multi_node
    uri_priority = domain_urls.uri_priority

    leader = cluster.last_known_leader
    proxy = next(cluster.proxy_cycle())

    producer = proxy.create_client("producer")
    producer.open(uri_priority, flags=["write,ack"], succeed=True)

    # Post messages
    for i in range(1, 5):
        producer.post(uri_priority, [f"msg{i}"], succeed=True, wait_ack=True)

    # Stop all nodes
    cluster.stop_nodes(prevent_leader_bounce=True)

    # Wipe qlist files from every node
    for node in cluster.nodes():
        storage_dir = str(cluster.work_dir.joinpath(node.name, "storage"))
        wipe_files(["*.bmq_qlist"], storage_dir)

    # Restart all nodes
    cluster.start_nodes(wait_leader=True, wait_ready=True)

    new_leader = cluster.last_known_leader

    # Old messages are lost (qlist maps queues to partition data).
    # Verify the queue is still functional by posting and consuming new ones.
    new_proxy = next(cluster.proxy_cycle())
    new_producer = new_proxy.create_client("new_producer")
    new_producer.open(uri_priority, flags=["write,ack"], succeed=True)

    for i in range(1, 4):
        new_producer.post(uri_priority, [f"new_msg{i}"], succeed=True, wait_ack=True)

    consumer_node = cluster.nodes(exclude=new_leader)[0]
    consumer = consumer_node.create_client("consumer")
    consumer.open(uri_priority, flags=["read"], succeed=True)

    assert wait_until(
        lambda: len(consumer.list(uri_priority, block=True)) == 3,
        3,
    ), "Consumer did not receive 3 new messages after qlist wipe"

    consumer.confirm(uri_priority, "*", succeed=True)
    consumer.close(uri_priority, succeed=True)
