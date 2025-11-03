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
Testing primary-replica partition synchronization in FSM mode.
"""

import glob
from pathlib import Path
import subprocess


import blazingmq.dev.it.testconstants as tc
from blazingmq.dev.it.fixtures import (  # pylint: disable=unused-import
    Cluster,
    tweak,
    start_cluster,
    fsm_multi_cluster,
)

from blazingmq.dev import paths


# Set max journal file size to a small value to force rollover during the test
MAX_JOURNAL_FILE_SIZE = 884


def _run_storage_tool(journal_file: Path, mode: str) -> subprocess.CompletedProcess:
    """Run storage tool on `journal_file` in the specified `mode`."""

    return subprocess.run(
        [
            paths.required_paths.storagetool,
            "--journal-file",
            journal_file,
            f"--{mode}",
            "--print-mode=json-pretty",
        ],
        capture_output=True,
        check=True,
    )


def _compare_journal_files(
    leader_name: str, replica_name: str, cluster: Cluster
) -> None:
    """Compare leader and replica journal files content, and assert that they are equal."""

    leader_journal_files = glob.glob(
        str(cluster.work_dir.joinpath(leader_name, "storage")) + "/*journal*"
    )
    replica_journal_files = glob.glob(
        str(cluster.work_dir.joinpath(replica_name, "storage")) + "/*journal*"
    )

    # Check that number of journal files equal to partitions number
    num_partitions = cluster.config.definition.partition_config.num_partitions
    assert len(leader_journal_files) == num_partitions
    assert len(replica_journal_files) == num_partitions

    # Check that content of leader and replica journal files is equal
    for leader_file, replica_file in zip(
        sorted(leader_journal_files),
        sorted(replica_journal_files),
    ):
        # Run storage tool on leader journal file in "detail" mode to check record order and content
        leader_res = _run_storage_tool(leader_file, "details")
        assert leader_res.returncode == 0

        # Run storage tool on replica journal file in "detail" mode to check record order and content
        replica_res = _run_storage_tool(replica_file, "details")
        assert replica_res.returncode == 0

        # Check that content of leader and replica journal files is equal
        assert leader_res.stdout == replica_res.stdout

        # Run storage tool on leader journal file in "summary" mode to check journal file headers
        leader_res = _run_storage_tool(leader_file, "summary")
        assert leader_res.returncode == 0

        # Run storage tool on replica journal file in "summary" mode to check journal file headers
        replica_res = _run_storage_tool(replica_file, "summary")
        assert replica_res.returncode == 0

        # Check that content of leader and replica journal files is equal
        assert leader_res.stdout == replica_res.stdout


@tweak.cluster.partition_config.max_journal_file_size(MAX_JOURNAL_FILE_SIZE)
def test_sync_after_missed_rollover(
    fsm_multi_cluster: Cluster,
    domain_urls: tc.DomainUrls,
) -> None:
    """
    Test replica journal file synchronization with cluster after missed rollover.
    - start cluster
    - put 2 messages
    - stop replica
    - put more messages to initiate rollover
    - restart replica
    - check that replica is synchronized with primary (primary and replica journal files content is equal)
    """
    cluster: Cluster = fsm_multi_cluster
    uri_priority = domain_urls.uri_priority

    leader = cluster.last_known_leader
    proxy = next(cluster.proxy_cycle())

    # Create producer and consumer
    producer = proxy.create_client("producer")
    producer.open(uri_priority, flags=["write,ack"], succeed=True)

    consumer = proxy.create_client("consumer")
    consumer.open(uri_priority, flags=["read"], succeed=True)

    replicas = cluster.nodes(exclude=leader)
    replica = replicas[0]

    # Put 2 messages with confirms
    for i in range(1, 3):
        producer.post(uri_priority, [f"msg{i}"], succeed=True, wait_ack=True)

        consumer.wait_push_event()
        consumer.confirm(uri_priority, "*", succeed=True)

    # Stop replica
    replica.stop()
    cluster.make_sure_node_stopped(replica)
    replica.drain()

    # Put more messages w/o confirm to initiate the rollover
    i = 3
    while not leader.outputs_substr("Initiating rollover", 0.01):
        assert i < 8, "Rollover was not initiated"
        producer.post(uri_priority, [f"msg{i}"], succeed=True, wait_ack=True)
        i += 1

    # Wait until rollover completed
    assert leader.outputs_substr("ROLLOVER COMPLETE", 10)

    # Restart the stopped replica which missed rollover
    replica.start()
    replica.wait_until_started()

    # Wait until replica synchronizes with cluster
    assert replica.outputs_substr("Cluster (itCluster) is available", 10)

    assert leader == cluster.last_known_leader

    # Check that leader and replica journal files are equal
    _compare_journal_files(leader.name, replica.name, cluster)


@start_cluster(False)
@tweak.cluster.partition_config.max_journal_file_size(MAX_JOURNAL_FILE_SIZE)
def test_sync_after_missed_rollover_after_restart(
    fsm_multi_cluster: Cluster,
    domain_urls: tc.DomainUrls,
) -> None:
    """
    Test replica journal file synchronization with cluster after missed rollover and cluster restart.
    - start cluster
    - put 2 messages
    - stop replica
    - put more messages to initiate rollover
    - stop cluster nodes
    - start cluster nodes, leader is the last to start to initiate `ReplicaStateRequest` to all replicas
    - check that replica (which is missed rollover) is synchronized with primary (primary and replica journal files content is equal)
    """

    cluster = fsm_multi_cluster
    uri_priority = domain_urls.uri_priority

    # Start cluster with leader `east1`
    east1 = cluster.start_node("east1")
    east1.set_quorum(3)
    east2 = cluster.start_node("east2")
    east2.set_quorum(5)
    west1 = cluster.start_node("west1")
    west1.set_quorum(5)
    west2 = cluster.start_node("west2")
    west2.set_quorum(5)

    east1.wait_status(wait_leader=True, wait_ready=True)
    assert east1 == east1.last_known_leader

    # Create producer and consumer
    producer = east1.create_client("producer")
    producer.open(uri_priority, flags=["write,ack"], succeed=True)

    consumer = east1.create_client("consumer")
    consumer.open(uri_priority, flags=["read"], succeed=True)

    # Put 2 messages with confirms
    for i in range(1, 3):
        producer.post(uri_priority, [f"msg{i}"], succeed=True, wait_ack=True)

        consumer.wait_push_event()
        consumer.confirm(uri_priority, "*", succeed=True)

    # Stop replica `east2`
    east2.stop()
    cluster.make_sure_node_stopped(east2)
    east2.drain()

    # Put more messages w/o confirm to initiate the rollover
    i = 3
    while not east1.outputs_substr("Initiating rollover", 0.01):
        assert i < 8, "Rollover was not initiated"
        producer.post(uri_priority, [f"msg{i}"], succeed=True, wait_ack=True)
        i += 1

    # Wait until rollover completed on all running nodes
    assert east1.outputs_substr("ROLLOVER COMPLETE", 10)
    assert west1.outputs_substr("ROLLOVER COMPLETE", 10)
    assert west2.outputs_substr("ROLLOVER COMPLETE", 10)

    #  Stop all running nodes
    for node in (east1, west1, west2):
        node.stop()
        cluster.make_sure_node_stopped(node)
        node.drain()

    # Start all nodes, leader `east1` is the last to start
    # to initiate `ReplicaStateRequest` to all replicas
    for node in (east2, west1, west2):
        node.start()
        node.wait_until_started()
        node.set_quorum(5)

    east1.start()
    east1.wait_until_started()
    east1.set_quorum(3)

    # Wait until leader `east1` is ready
    east1.wait_status(wait_leader=True, wait_ready=True)
    assert east1 == east1.last_known_leader

    # Wait until replica `east2` synchronizes with leader `east1`
    assert east2.outputs_substr("Cluster (itCluster) is available", 10)

    # Check that leader `east1` and replica `east2` (which is missed rollover) journal files are equal
    _compare_journal_files(east1.name, east2.name, cluster)


def test_sync_after_missed_records(
    fsm_multi_cluster: Cluster,
    domain_urls: tc.DomainUrls,
) -> None:
    """
    Test replica journal file synchronization with cluster after missed records.
    - start cluster
    - put 2 messages
    - stop replica
    - put more messages
    - restart replica
    - check that replica is synchronized with primary (primary and replica journal files content is equal)
    """
    cluster: Cluster = fsm_multi_cluster
    uri_priority = domain_urls.uri_priority

    leader = cluster.last_known_leader
    proxy = next(cluster.proxy_cycle())

    # Create producer and consumer
    producer = proxy.create_client("producer")
    producer.open(uri_priority, flags=["write,ack"], succeed=True)

    consumer = proxy.create_client("consumer")
    consumer.open(uri_priority, flags=["read"], succeed=True)

    replica = cluster.nodes(exclude=leader)[0]

    # Put 2 messages
    for i in range(1, 3):
        producer.post(uri_priority, [f"msg{i}"], succeed=True, wait_ack=True)

    # Stop replica
    replica.stop()
    cluster.make_sure_node_stopped(replica)
    replica.drain()

    # Put two more messages
    for i in range(3, 5):
        producer.post(uri_priority, [f"msg{i}"], succeed=True, wait_ack=True)

    # Restart the stopped replica which missed messages
    replica.start()
    replica.wait_until_started()

    # Wait until replica synchronizes with cluster
    assert replica.outputs_substr("Cluster (itCluster) is available", 10)

    assert leader == cluster.last_known_leader

    # Check that leader and replica journal files are equal
    _compare_journal_files(leader.name, replica.name, cluster)


def test_sync_if_leader_missed_records(
    fsm_multi_cluster: Cluster,
    domain_urls: tc.DomainUrls,
) -> None:
    """
    Test leader journal file synchronization with cluster when it missed records.
    - start cluster, leader is east1
    - put 2 messages
    - kill replica east2, mark it as a `next_leader`
    - put 2 more messages
    - stop all running nodes
    - start all nodes, force `next_leader` (east2) to be a leader
    - check that leader (which is behind replicas) is synchronized with replicas (leader and replica journal files content is equal)
    """

    cluster = fsm_multi_cluster
    uri_priority = domain_urls.uri_priority

    # Start cluster with leader `east1`
    leader = cluster.last_known_leader

    # Create producer and consumer
    producer = leader.create_client("producer")
    producer.open(uri_priority, flags=["write,ack"], succeed=True)

    consumer = leader.create_client("consumer")
    consumer.open(uri_priority, flags=["read"], succeed=True)

    # Put 2 messages
    for i in range(1, 3):
        producer.post(uri_priority, [f"msg{i}"], succeed=True, wait_ack=True)

    # Mark `next_leader` node (east2)
    next_leader = cluster.nodes(exclude=leader)[0]

    # Kill `next_leader` node
    cluster.drain()
    next_leader.check_exit_code = False
    next_leader.kill()
    next_leader.wait()

    # Put 2 more messages
    for i in range(3, 5):
        producer.post(uri_priority, [f"msg{i}"], succeed=True, wait_ack=True)

    # Stop all running nodes
    for node in cluster.nodes(exclude=next_leader):
        node.check_exit_code = False
        node.stop()
        cluster.make_sure_node_stopped(node)

    cluster.drain()

    # Start all nodes, `next_leader` is the first, force it to be a leader
    sorted_nodes = sorted(
        cluster.nodes(), key=lambda node: 0 if node == next_leader else 1
    )
    for node in sorted_nodes:
        node.start()
        node.wait_until_started()
        quorum = 3 if node == next_leader else 5
        node.set_quorum(quorum)

    # Wait until cluster is ready
    next_leader.wait_status(wait_leader=True, wait_ready=True)
    assert next_leader.last_known_leader == next_leader

    # Select replica
    replica = cluster.nodes(exclude=next_leader)[0]

    # Check that `next_leader` and replica journal files are equal
    _compare_journal_files(next_leader.name, replica.name, cluster)
