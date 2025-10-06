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
Utility function for testing rollover
"""

import re
import time

import blazingmq.dev.it.testconstants as tc

from blazingmq.dev.it.fixtures import test_logger, Cluster

from blazingmq.dev.it.process.broker import Broker
from blazingmq.dev.it.process.client import Client
from blazingmq.dev.it.util import wait_until


def ensure_message_at_storage_layer(
    cluster: Cluster, partition_id: int, queue_uri: str, num_messages: int
):
    """
    Assert that in the `partitionId` of the `cluster`, there are exactly
    `numMessages` messages in the storage of the `queueUri`.
    """

    # Before restarting the cluster, ensure that all nodes in the cluster
    # have received the message at the storage layer.  This is necessary
    # in the absence of stronger consistency in storage replication in
    # BMQ.  Presence of message in the storage at each node is checked by
    # sending 'STORAGE SUMMARY' command and grepping its output.
    for node in cluster.nodes():
        node.command(f"CLUSTERS CLUSTER {node.cluster_name} STORAGE SUMMARY")

    time.sleep(2)
    for node in cluster.nodes():
        assert node.outputs_regex(
            r"\w{10}\s+%s\s+%s\s+\d+\s+B\s+" % (partition_id, num_messages)
            + re.escape(queue_uri),
            timeout=20,
        )
        # Above regex is to match line:
        # C1E2A44527    0      1      68  B      bmq://bmq.test.mmap.priority.~tst/qqq
        # where columns are: QueueKey, PartitionId, NumMsgs, NumBytes,
        # QueueUri respectively.


def simulate_rollover(du: tc.DomainUrls, leader: Broker, producer: Client):
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


def check_if_queue_has_n_messages(consumer: Client, queue: str, n: int):
    """
    Check if `queue` has exactly `n` messages.
    """

    test_logger.info(f"Check if queue {queue} still has {n} messages")
    consumer.open(
        queue,
        flags=["read"],
        succeed=True,
    )
    msgs = consumer.list(queue, block=True)
    assert wait_until(
        lambda: len(msgs) == n,
        3,
    )
    return msgs
