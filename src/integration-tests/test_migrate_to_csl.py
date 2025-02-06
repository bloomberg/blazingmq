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
Testing migrating to CSL.
"""

import re
import time

import blazingmq.dev.it.testconstants as tc
from blazingmq.dev.it.fixtures import (  # pylint: disable=unused-import
    order,
    tweak,
    Cluster,
    cluster,
    multi_node,
)
from blazingmq.dev.it.util import wait_until

pytestmark = order(2)


def ensureMessageAtStorageLayer(
    cluster: Cluster, uri=tc.URI_PRIORITY, partition=0, num_msgs=1
):
    time.sleep(2)
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
            rf"\w{{10}}\s+{partition}\s+{num_msgs}\s+\d+\s+B\s+" + re.escape(uri),
            timeout=20,
        )
        # Above regex is to match line:
        # C1E2A44527    0      1      68  B      bmq://bmq.test.mmap.priority.~tst/qqq
        # where columns are: QueueKey, PartitionId, NumMsgs, NumBytes,
        # QueueUri respectively. Since we opened only 1 queue, we know that
        # it will be assigned to partitionId 0.


def test_assign_queue(multi_node: Cluster):
    """
    This test is to make sure when a queue assignment happens,
    it goes thought the CSL path, not the non-CSL.
    """
    proxies = multi_node.proxy_cycle()
    proxy = next(proxies)

    leader = multi_node.last_known_leader
    members = multi_node.nodes(exclude=leader)

    uri = tc.URI_PRIORITY_SC
    timeout = 1

    producer = proxy.create_client("producer")
    producer.open(uri, flags=["write"], succeed=True)

    for member in members:
        assert member.outputs_regex(
            "(Applying cluster message with type = UPDATE).*(queueAssignmentAdvisory)",
            timeout,
        )
        assert member.outputs_regex(
            "(Committed advisory).*queueAssignmentAdvisory", timeout
        )
        assert not member.outputs_regex(
            "Processing queueAssignmentAdvisory message", timeout
        )


@tweak.cluster.cluster_attributes.is_cslmode_enabled(False)
@tweak.cluster.cluster_attributes.is_fsmworkflow(False)
def test_reconfigure_from_non_CSL_to_CSL(cluster: Cluster):
    """
    This test does the following steps to validate our conversion from non-CSL to CSL:
    1. Run broker in non-CSL and open queues
    2. Reconfigure to CSL and make sure queues are operational
    3. Open queues while in CSL mode
    4. Reconfigure back to non-CSL and make sure queues are operational
    """
    uri_priority1 = f"bmq://{tc.DOMAIN_PRIORITY_SC}/q1"
    uri_fanout1 = f"bmq://{tc.DOMAIN_FANOUT_SC}/f1"
    uri_fanout1_foo = f"bmq://{tc.DOMAIN_FANOUT_SC}/f1?id=foo"
    uri_fanout1_bar = f"bmq://{tc.DOMAIN_FANOUT_SC}/f1?id=bar"
    uri_priority2 = f"bmq://{tc.DOMAIN_PRIORITY_SC}/q2"
    uri_fanout2 = f"bmq://{tc.DOMAIN_FANOUT_SC}/f2"
    uri_fanout2_foo = f"bmq://{tc.DOMAIN_FANOUT_SC}/f2?id=foo"
    uri_fanout2_bar = f"bmq://{tc.DOMAIN_FANOUT_SC}/f2?id=bar"

    # -----------------------------------------------
    # 1. Start a producer in non-CSL. Then, post a message on a priority queue and a fanout queue.
    proxies = cluster.proxy_cycle()
    producer = next(proxies).create_client("producer")
    producer.open(uri_priority1, flags=["write", "ack"], succeed=True)
    producer.post(uri_priority1, payload=["msg1"], wait_ack=True, succeed=True)
    producer.open(uri_fanout1, flags=["write", "ack"], succeed=True)
    producer.post(uri_fanout1, payload=["fanout_msg1"], wait_ack=True, succeed=True)

    ensureMessageAtStorageLayer(cluster, uri_priority1, partition=0, num_msgs=1)

    # Consumer for fanout queue
    consumer_foo = next(proxies).create_client("consumer_foo")
    consumer_foo.open(uri_fanout1_foo, flags=["read"], succeed=True)
    consumer_foo.wait_push_event()

    cluster._logger.info("send list command...")

    assert wait_until(
        lambda: len(consumer_foo.list(uri_fanout1_foo, block=True)) == 1, 2
    )

    # Save one confirm to the storage
    consumer_foo.confirm(uri_fanout1_foo, "+1", succeed=True)
    consumer_foo.close(uri_fanout1_foo, succeed=True)

    cluster.stop_nodes()

    cluster._logger.info("============ Before 2 ==========")

    # -----------------------------------------------
    # 2. Reconfigure the cluster from non-CSL to CSL mode
    for broker in cluster.configurator.brokers.values():
        my_clusters = broker.clusters.my_clusters
        if len(my_clusters) > 0:
            my_clusters[0].cluster_attributes.is_cslmode_enabled = True
            my_clusters[0].cluster_attributes.is_fsmworkflow = True
    cluster.deploy_domains()

    cluster.start_nodes(wait_leader=True, wait_ready=True)
    # For a standard cluster, states have already been restored as part of
    # leader re-election.
    if cluster.is_single_node:
        producer.wait_state_restored()

    producer.post(uri_priority1, payload=["msg2"], wait_ack=True, succeed=True)
    producer.post(uri_fanout1, payload=["fanout_msg2"], wait_ack=True, succeed=True)

    # Consumer for priority queue
    consumer = next(proxies).create_client("consumer")
    consumer.open(uri_priority1, flags=["read"], succeed=True)
    consumer.wait_push_event()
    assert wait_until(lambda: len(consumer.list(uri_priority1, block=True)) == 2, 2)

    # Consumers for fanout queue
    consumer_bar = next(proxies).create_client("consumer_bar")
    consumer_bar.open(uri_fanout1_bar, flags=["read"], succeed=True)
    consumer_bar.wait_push_event()
    assert wait_until(
        lambda: len(consumer_bar.list(uri_fanout1_bar, block=True)) == 2, 2
    )

    # make sure the previously saved confirm is not lost
    consumer_foo.open(uri_fanout1_foo, flags=["read"], succeed=True)
    consumer_foo.wait_push_event()
    assert wait_until(
        lambda: len(consumer_foo.list(uri_fanout1_foo, block=True)) == 1, 2
    )

    # confirm all the messages to make sure we can shutdown gracefully
    consumer.confirm(uri_priority1, "+2", succeed=True)
    consumer_foo.confirm(uri_fanout1_foo, "+1", succeed=True)
    consumer_bar.confirm(uri_fanout1_bar, "+2", succeed=True)

    cluster._logger.info("============ Before 3 ==========")

    # -----------------------------------------------
    # 3. Open queues in CSL mode
    producer = next(proxies).create_client("producer")
    producer.open(uri_priority2, flags=["write", "ack"], succeed=True)
    producer.post(uri_priority2, payload=["msg1"], wait_ack=True, succeed=True)
    producer.open(uri_fanout2, flags=["write", "ack"], succeed=True)
    producer.post(uri_fanout2, payload=["fanout_msg1"], wait_ack=True, succeed=True)

    ensureMessageAtStorageLayer(cluster, uri_fanout1, partition=1, num_msgs=2)
    ensureMessageAtStorageLayer(cluster, uri_priority2, partition=2, num_msgs=1)

    # Consumer for fanout queue
    consumer_foo.open(uri_fanout2_foo, flags=["read"], succeed=True)
    consumer_foo.wait_push_event()
    assert wait_until(
        lambda: len(consumer_foo.list(uri_fanout2_foo, block=True)) == 1, 2
    )

    # Save one confirm to the storage
    consumer_foo.confirm(uri_fanout2_foo, "+1", succeed=True)

    consumer_foo.close(uri_fanout2_foo, succeed=True)

    cluster._logger.info("before stop nodes...")
    cluster.stop_nodes()
    cluster._logger.info("after stop nodes...")

    cluster._logger.info("============ Before 4 ==========")

    # -----------------------------------------------
    # 4. Reconfigure the cluster back from CSL to non-CSL mode
    for broker in cluster.configurator.brokers.values():
        my_clusters = broker.clusters.my_clusters
        if len(my_clusters) > 0:
            my_clusters[0].cluster_attributes.is_cslmode_enabled = False
            my_clusters[0].cluster_attributes.is_fsmworkflow = False
    cluster.deploy_domains()

    cluster._logger.info("before starting nodes...")
    cluster.start_nodes(wait_leader=True, wait_ready=True)
    cluster._logger.info("after starting nodes...")
    # For a standard cluster, states have already been restored as part of
    # leader re-election.
    if cluster.is_single_node:
        producer.wait_state_restored()

    producer.post(uri_priority2, payload=["msg2"], wait_ack=True, succeed=True)
    producer.post(uri_fanout2, payload=["fanout_msg2"], wait_ack=True, succeed=True)

    # Consumer for priority queue
    consumer = next(proxies).create_client("consumer")
    consumer.open(uri_priority2, flags=["read"], succeed=True)
    consumer.wait_push_event()
    assert wait_until(lambda: len(consumer.list(uri_priority2, block=True)) == 2, 2)

    # Consumers for fanout queue
    consumer_bar = next(proxies).create_client("consumer_bar")
    consumer_bar.open(uri_fanout2_bar, flags=["read"], succeed=True)
    consumer_bar.wait_push_event()
    assert wait_until(
        lambda: len(consumer_bar.list(uri_fanout2_bar, block=True)) == 2, 2
    )

    # make sure the previously saved confirm is not lost
    consumer_foo.open(uri_fanout2_foo, flags=["read"], succeed=True)
    consumer_foo.wait_push_event()
    assert wait_until(
        lambda: len(consumer_foo.list(uri_fanout2_foo, block=True)) == 1, 2
    )

    ensureMessageAtStorageLayer(cluster, uri_fanout2, partition=3, num_msgs=2)
    ensureMessageAtStorageLayer(cluster, uri_priority2, partition=2, num_msgs=2)
