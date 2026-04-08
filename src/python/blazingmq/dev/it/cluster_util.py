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

import glob
import json
import re
import subprocess
from pathlib import Path

import blazingmq.dev.it.testconstants as tc
from blazingmq.dev import paths

from blazingmq.dev.it.fixtures import test_logger, Cluster

from blazingmq.dev.it.process.broker import Broker
from blazingmq.dev.it.process.client import Client
from blazingmq.dev.it.util import wait_until


def ensure_message_at_storage_layer(
    cluster: Cluster,
    partition_id: int,
    queue_uri: str,
    expected_count: int,
    alive=False,
):
    """
    Assert that in the `partitionId` of the `cluster`, there are exactly
    `expected_count` messages in the storage of the `queueUri`.
    Param 'alive': if True, check only alive nodes in the cluster.
    """

    # Before restarting the cluster, ensure that all nodes in the cluster
    # have received the message at the storage layer.  This is necessary
    # in the absence of stronger consistency in storage replication in
    # BMQ.  Presence of message in the storage at each node is checked by
    # sending 'STORAGE SUMMARY' command and grepping its output.

    nodes = cluster.nodes(alive=alive)

    for node in nodes:
        node.command(f"CLUSTERS CLUSTER {node.cluster_name} STORAGE SUMMARY")

    def check():
        return node.outputs_regex(
            r"\w{10}\s+%s\s+%s\s+\d+\s+B\s+" % (partition_id, expected_count)
            + re.escape(queue_uri)
        )

    for node in nodes:
        assert wait_until(
            check,
            timeout=3,
        ), (
            f"Node {node.name} does not have {expected_count} messages for queue {queue_uri} in partition {partition_id} at storage layer"
        )

        # Above regex is to match line:
        # C1E2A44527    0      1      68  B      bmq://bmq.test.mmap.priority.~tst/qqq
        # where columns are: QueueKey, PartitionId, NumMsgs, NumBytes,
        # QueueUri respectively.


def simulate_csl_rollover(du: tc.DomainUrls, leader: Broker, producer: Client):
    """
    Simulate rollover of CSL file by opening and closing many queues.
    """

    i = 0
    # Open queues until rollover detected
    while not leader.outputs_regex(r"Rolling over from log with logId", 0.01):
        producer.open(
            f"bmq://{du.domain_priority}/q_dummy_{i}",
            flags=["write,ack"],
            succeed=True,
        )
        producer.close(f"bmq://{du.domain_priority}/q_dummy_{i}", succeed=True)
        i += 1

        if i % 5 == 0:
            # do not wait for success, otherwise the following capture will fail
            leader.force_gc_queues(block=False)

        assert i < 10000, (
            "Failed to detect rollover after opening a reasonable number of queues"
        )
    test_logger.info(f"Rollover detected after opening {i} queues")

    # Rollover and queueUnAssignmentAdvisory interleave
    assert leader.outputs_regex(r"queueUnAssignmentAdvisory", timeout=5)


def check_if_queue_has_n_messages(consumer: Client, queue: str, expected_count: int):
    """
    Use the `consumer` to check if `queue` has exactly `n` messages.
    """

    test_logger.info(f"Check if queue {queue} still has {expected_count} messages")
    consumer.open(
        queue,
        flags=["read"],
        block=True,
        succeed=True,
    )
    msgs = []

    def check():
        nonlocal msgs
        msgs = consumer.list(queue, block=True)
        return len(msgs) == expected_count

    assert wait_until(
        check,
        timeout=3,
    ), f"Queue {queue} does not have {expected_count} messages, has {len(msgs)} instead"
    consumer.close(queue, succeed=True)
    return msgs


def run_storage_tool(journal_file: Path, mode: str) -> subprocess.CompletedProcess:
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


def clean_storage_output(output_str: str) -> str:
    """
    Clean the output by removing any non-deterministic parts, such as
    timestamps/epochs.
    """
    RECORDS_KEYS_TO_REMOVE = [
        "Timestamp",
        "Epoch",
    ]
    SUMMARY_KEYS_TO_REMOVE = [
        "First SyncPointRecord timestamp",
        "First SyncPointRecord epoch",
        "Record Timestamp",
        "Record Epoch",
        "SyncPoint Timestamp",
        "SyncPoint Epoch",
    ]
    data = json.loads(output_str)
    if "Records" in data:
        for record in data["Records"]:
            for key in RECORDS_KEYS_TO_REMOVE:
                if key in record:
                    del record[key]
    elif "JournalFileDetails" in data:
        journal_details = data["JournalFileDetails"]
        journal_hdr = journal_details["Journal File Header"]
        for key in SUMMARY_KEYS_TO_REMOVE:
            if key in journal_hdr:
                del journal_hdr[key]
        if "Journal SyncPoint" in journal_details:
            journal_sync = journal_details["Journal SyncPoint"]
            for key in SUMMARY_KEYS_TO_REMOVE:
                if key in journal_sync:
                    del journal_sync[key]

    return json.dumps(data, indent=2)


def stop_cluster_and_compare_journal_files(
    leader_name: str, replica_name: str, cluster: Cluster
) -> None:
    """
    Stop cluster after bumping quorum on all replicas to prevent primary
    switch.  Then, compare leader and replica journal files content, and
    assert that they are equal.

    NOTE: Stopping all nodes ensures that all journal files are closed and
    flushed to disk, and that there are no discrepancies due to in-flight
    sync points.
    """

    for node in cluster.nodes():
        if node.is_alive():
            node.set_quorum(5)
    if cluster.last_known_leader:
        cluster.last_known_leader.stop()
        cluster.make_sure_node_stopped(cluster.last_known_leader)
    cluster.stop_nodes()

    leader_journal_files = glob.glob(
        str(cluster.work_dir.joinpath(leader_name, "storage")) + "/*journal*"
    )
    replica_journal_files = glob.glob(
        str(cluster.work_dir.joinpath(replica_name, "storage")) + "/*journal*"
    )

    num_partitions = cluster.config.definition.partition_config.num_partitions
    assert len(leader_journal_files) == num_partitions, (
        f"Expected {num_partitions} leader journal files, got {len(leader_journal_files)}"
    )
    assert len(replica_journal_files) == num_partitions, (
        f"Expected {num_partitions} replica journal files, got {len(replica_journal_files)}"
    )

    for leader_file, replica_file in zip(
        sorted(leader_journal_files),
        sorted(replica_journal_files),
    ):
        leader_res = run_storage_tool(leader_file, "details")
        assert leader_res.returncode == 0, (
            f"Leader storage tool failed on {leader_file} with rc {leader_res.returncode}"
        )

        replica_res = run_storage_tool(replica_file, "details")
        assert replica_res.returncode == 0, (
            f"Replica storage tool failed on {replica_file} with rc {replica_res.returncode}"
        )

        assert clean_storage_output(leader_res.stdout) == clean_storage_output(
            replica_res.stdout
        ), (
            f"Leader and replica journal file contents differ for "
            f"{leader_file} and {replica_file}"
        )

        leader_res = run_storage_tool(leader_file, "summary")
        assert leader_res.returncode == 0, (
            f"Leader storage tool (summary) failed on {leader_file} with rc {leader_res.returncode}"
        )

        replica_res = run_storage_tool(replica_file, "summary")
        assert replica_res.returncode == 0, (
            f"Replica storage tool (summary) failed on {replica_file} with rc {replica_res.returncode}"
        )

        assert clean_storage_output(leader_res.stdout) == clean_storage_output(
            replica_res.stdout
        ), (
            f"Leader and replica journal file summary differ for "
            f"{leader_file} and {replica_file}"
        )
