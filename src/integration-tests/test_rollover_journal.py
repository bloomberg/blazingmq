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
Testing rollover of journal file.
"""

import glob

import blazingmq.dev.it.testconstants as tc
from blazingmq.dev.it.cluster_util import (
    check_if_queue_has_n_messages,
    rollover_queues_and_apps_test,
)
from blazingmq.dev.it.fixtures import (
    Cluster,
    order,
)
from blazingmq.dev.it.util import wait_until

pytestmark = order(4)


class TestRolloverJournal:
    def test_journal_cleanup(self, cluster: Cluster, domain_urls: tc.DomainUrls):
        """
        Test that rolling over journal file via admin command cleans up the old
        file.
        """
        NUM_PARTITIONS = cluster.config.definition.partition_config.num_partitions
        leader = cluster.last_known_leader

        proxy = next(cluster.proxy_cycle())
        producer = proxy.create_client("producer")

        producer.open(
            f"bmq://{domain_urls.domain_priority}/q0",
            flags=["write,ack"],
            succeed=True,
        )
        producer.close(f"bmq://{domain_urls.domain_priority}/q0", succeed=True)

        journal_files_before_rollover = glob.glob(
            str(cluster.work_dir.joinpath(leader.name, "storage")) + "/*journal*"
        )
        journal_files_before_rollover.sort()
        assert len(journal_files_before_rollover) == NUM_PARTITIONS, (
            f"Expected {NUM_PARTITIONS} journal files before rollover, "
            f"got {len(journal_files_before_rollover)}"
        )

        all_partitions = -1
        leader.trigger_rollover(partitionId=all_partitions)
        leader.wait_rollover_complete()

        # Old files are archived asynchronously by a worker thread after
        # "ROLLOVER COMPLETE" is logged, so poll until the old files are moved.
        storage_pattern = (
            str(cluster.work_dir.joinpath(leader.name, "storage")) + "/*journal*"
        )

        def _journal_files_cleaned_up():
            return len(glob.glob(storage_pattern)) == NUM_PARTITIONS

        assert wait_until(_journal_files_cleaned_up, timeout=10), (
            f"Expected {NUM_PARTITIONS} journal files after rollover, "
            f"got: {glob.glob(storage_pattern)}"
        )

        journal_files_after_rollover = sorted(glob.glob(storage_pattern))
        assert journal_files_before_rollover[0] != journal_files_after_rollover[0], (
            f"Journal file for partition 0 did not change after rollover: "
            f"{journal_files_before_rollover[0]}"
        )

    def test_rollover_queues_and_apps(
        self, cluster: Cluster, domain_urls: tc.DomainUrls
    ):
        """
        Test that queue and appId information are preserved across rollover of
        journal file via admin command, even after cluster restart.
        """

        def trigger_rollover(leader, producer):  # pylint: disable=unused-argument
            leader.trigger_rollover(partitionId=-1)
            leader.wait_rollover_complete()

        rollover_queues_and_apps_test(cluster, domain_urls, trigger_rollover)

    def test_rollover_replica_rejected(
        self, multi_node: Cluster, domain_urls: tc.DomainUrls
    ):
        """
        Test that the ROLLOVER admin command is rejected on a replica, and that
        the cluster remains healthy afterwards (replica restart, leader restart,
        queue still works).
        """
        leader = multi_node.last_known_leader
        replicas = multi_node.nodes(exclude=leader)
        replica = replicas[0]

        proxy = next(multi_node.proxy_cycle())
        producer = proxy.create_client("producer")
        consumer = proxy.create_client("consumer")

        queue = f"bmq://{domain_urls.domain_priority}/rollover_test"
        producer.open(queue, flags=["write,ack"], succeed=True)
        producer.post(queue, ["msg1", "msg2"], succeed=True, wait_ack=True)

        # Attempt rollover on replica — should be rejected
        replica.trigger_rollover(partitionId=0)
        assert replica.outputs_substr(
            "Rollover rejected: node is not primary for this partition",
            timeout=5,
        ), "Replica should have rejected rollover"

        # Restart the replica, then the leader — cluster should remain healthy
        replica.stop()
        replica.start()
        replica.wait_until_started()

        leader.stop()
        leader.start()
        leader.wait_until_started()

        # Verify the queue still works after both restarts
        check_if_queue_has_n_messages(consumer, queue, 2)
