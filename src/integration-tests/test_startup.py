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

import blazingmq.dev.it.testconstants as tc
from blazingmq.dev.it.fixtures import Cluster, order, start_cluster
from blazingmq.dev.it.process.client import Client

pytestmark = order(4)


# -------------------------------------------------------------------------------
@start_cluster(start=False)
def test_early_assign(multi_node: Cluster, domain_urls: tc.DomainUrls):
    """
    Early open a queue on a soon-to-be leader.  Legacy leader when it becomes
    ACTIVE, starts assigning queues _before_ it assigns partitions.  Then,
    the leader observes `onQueueAssigned` event _before_ it becomes ACTIVE
    primary.  If that event logic erroneously decides that the self is replica,
    the soon-to-be primary does not write QueueCreationRecord.  The primary
    still writes any posted message record though.  That leads to either assert
    in rollover or recovery failure on restart with "Encountered a MESSAGE
    record for queueKey [...], offset: ..., index: ..., for which a
    QueueOp.CREATION record was not seen..."
    """

    cluster = multi_node

    # leader-to-be
    east1 = cluster.start_node("east1")
    east1.set_quorum(3)
    west1 = cluster.start_node("west1")
    west1.set_quorum(5)

    producer = east1.create_client("east1")

    # do not wait got openQueue response
    producer.open(domain_urls.uri_priority, flags=["write", "ack"], block=False)

    east2 = cluster.start_node("east2")
    east2.set_quorum(5)
    west2 = cluster.start_node("west2")
    west2.set_quorum(5)

    east1.wait_status(wait_leader=True, wait_ready=True)

    assert east1 == east1.last_known_leader

    assert (
        producer.post(
            domain_urls.uri_priority, payload=["msg1"], block=True, wait_ack=True
        )
        == Client.e_SUCCESS
    )
    cluster.restart_nodes(wait_leader=True, wait_ready=True)


@start_cluster(start=False)
def test_replica_late_join(multi_node: Cluster, domain_urls: tc.DomainUrls):
    """
    In a steady-state cluster where only one replica node is down, with live
    messages flowing, the replica node rejoins and attempts to heal itself via
    the primary.  The concern is that the primary could send live data to the
    replica, and then re-send that data as recovery data chunks; we would like
    to make sure our deduplication logic is working correctly to handle this
    case.
    """

    cluster = multi_node

    # Starting all nodes except `east2`, which will join later
    east1 = cluster.start_node("east1")
    cluster.start_node("west1")
    cluster.start_node("west2")

    # The cluster will enter a healthy state with quorum of nodes
    east1.wait_status(wait_leader=True, wait_ready=True)

    producer = east1.create_client("producer")
    producer.open(domain_urls.uri_priority, flags=["write", "ack"], succeed=True)
    consumer = east1.create_client("consumer")
    consumer.open(domain_urls.uri_priority, flags=["read"], succeed=True)

    # Producer will post messages constantly for 30 seconds
    producer.batch_post(
        domain_urls.uri_priority,
        payload="msg",
        msg_size=1024,
        event_size=1,
        events_count=300000,
        post_interval=0.1,
        post_rate=1,
    )

    east2 = cluster.start_node("east2")
    east2.wait_status(wait_leader=True, wait_ready=True)
