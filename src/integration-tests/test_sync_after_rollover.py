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
Testing primary-replica synchronization after missed rollover.
"""

import glob
from pathlib import Path
import subprocess


import blazingmq.dev.it.testconstants as tc
from blazingmq.dev.it.fixtures import (
    Cluster,
    tweak,
)

from blazingmq.dev import paths


@tweak.cluster.partition_config.max_journal_file_size(884)
def test_synch_after_missed_rollover(
    multi_node: Cluster,
    domain_urls: tc.DomainUrls,
) -> None:
    """
    Test replica journal file syncronization with cluster after missed rollover.
    - start cluster
    - put 2 messages
    - stop replica
    - put one message to initiate rollover
    - restart replica
    - check that replica is synchronized with primary (primary and replica journal files content is equal)
    """
    cluster: Cluster = multi_node
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

    # Put more messages w/o confirm to initiate rollover
    for i in range(3, 8):
        producer.post(uri_priority, [f"msg{i}"], succeed=True, wait_ack=True)

    # Wait until rollover completed
    assert leader.outputs_substr("ROLLOVER COMPLETE", 10)

    # Restart the stopped replica which missed rollover
    replica.start()
    replica.wait_until_started()

    # Wait until replica synchronizes with cluster
    assert replica.outputs_substr("Cluster (itCluster) is available", 10)

    # Check that leader and replica journal files are equal
    leader = cluster.last_known_leader

    leader_journal_files_after_rollover = glob.glob(
        str(cluster.work_dir.joinpath(leader.name, "storage")) + "/*journal*"
    )
    replica_journal_files_after_rollover = glob.glob(
        str(cluster.work_dir.joinpath(replica.name, "storage")) + "/*journal*"
    )

    # Check that number of journal files equal to partitions number
    num_partitions = cluster.config.definition.partition_config.num_partitions
    assert len(leader_journal_files_after_rollover) == num_partitions
    assert len(replica_journal_files_after_rollover) == num_partitions

    # Check that content of leader and replica journal files is equal
    for leader_file, replica_file in zip(
        sorted(leader_journal_files_after_rollover),
        sorted(replica_journal_files_after_rollover),
    ):

        def run_storage_tool(
            journal_file: Path, mode: str
        ) -> subprocess.CompletedProcess:
            """Run storage tool on journal file in specified mode."""
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

        # Run storage tool on leader journal file in "detail" mode to check record order and content
        leader_res = run_storage_tool(leader_file, "details")
        assert leader_res.returncode == 0

        # Run storage tool on replica journal file in "detail" mode to check record order and content
        replica_res = run_storage_tool(replica_file, "details")
        assert replica_res.returncode == 0

        # Check that content of leader and replica journal files is equal
        assert leader_res.stdout == replica_res.stdout

        # Run storage tool on leader journal file in "summary" mode to check journal file headers
        leader_res = run_storage_tool(leader_file, "summary")
        assert leader_res.returncode == 0

        # Run storage tool on replica journal file in "summary" mode to check journal file headers
        replica_res = run_storage_tool(replica_file, "summary")
        assert replica_res.returncode == 0

        # Check that content of leader and replica journal files is equal
        assert leader_res.stdout == replica_res.stdout
