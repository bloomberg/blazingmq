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
Integration tests for queue re-open scenarios.
"""

import re

import blazingmq.dev.it.testconstants as tc
from blazingmq.dev.it.fixtures import (
    Cluster,
)
from blazingmq.dev.it.process.client import Client


def test_reopen_empty_queue(multi_node: Cluster, domain_urls: tc.DomainUrls):
    """
    If queue has no handles by the time cluster state restores, it should
    still be notified in order to update its state.
    """
    uri_priority = domain_urls.uri_priority
    proxies = multi_node.proxy_cycle()
    # pick proxy in datacenter opposite to the primary's
    next(proxies)
    replica_proxy = next(proxies)

    # Start a producer and open a queue
    producer = replica_proxy.create_client("producer")
    producer.open(uri_priority, flags=["write,ack"], succeed=True)

    # If queue open has succeeded, then active_node is known
    active_node = multi_node.process(replica_proxy.get_active_node())

    # Close the queue.  The replica keeps (stale) RemoteQueue
    producer.exit_gracefully()

    # Prevent 'active_node' from becoming new primary
    active_node.set_quorum(4)

    # Shutdown the primary
    leader = multi_node.last_known_leader
    leader.stop()

    # Start a producer and open a queue
    producer = replica_proxy.create_client("producer")
    producer.open(uri_priority, flags=["write,ack"], succeed=True)

    # Post should result in successful Ack
    assert (
        producer.post(uri_priority, ["1"], wait_ack=True, succeed=True)
        == Client.e_SUCCESS
    )


def test_reopen_substream(multi_node: Cluster, domain_urls: tc.DomainUrls):
    """
    Ticket 169527537.  Make a primary's client reopen the same appId with a
    different subId.
    """
    du = domain_urls

    leader = multi_node.last_known_leader
    consumer1 = leader.create_client("consumer1")
    consumer1.open(du.uri_fanout_foo, flags=["read"], succeed=True)

    consumer2 = leader.create_client("consumer2")
    consumer2.open(du.uri_fanout_foo, flags=["read"], succeed=True)
    consumer2.open(du.uri_fanout_bar, flags=["read"], succeed=True)

    consumer2.close(du.uri_fanout_foo, succeed=True)
    consumer2.open(du.uri_fanout_foo, flags=["read"], succeed=True)


def test_open_queue_after_close_and_failover(
    multi_node: Cluster, domain_urls: tc.DomainUrls
):
    """
    If a queue has 0 handles (previous client closed) and a new client sends
    an open-queue request that gets E_CANCELED due to primary failover, the
    pending open must still complete after the new primary is elected.

    Reproduces a bug where restoreStateCluster skips reopening queues with 0
    handles, leaving SubQueueContext::d_state stuck at k_CLOSED, which blocks
    sendOpenQueueRequest from ever sending the upstream request.

    The bug only triggers when the node stays a REPLICA after failover (not
    when it becomes the new primary via convertToLocal).
    """
    uri_priority = domain_urls.uri_priority
    proxies = multi_node.proxy_cycle()
    # pick proxy in datacenter opposite to the primary's (connects to replica)
    next(proxies)
    replica_proxy = next(proxies)

    # Step 1: Open queue via proxy -> creates RemoteQueue on the replica
    producer1 = replica_proxy.create_client("producer1")
    producer1.open(uri_priority, flags=["write,ack"], succeed=True)

    # Identify the replica (where proxy connects) and the leader
    active_node = multi_node.process(replica_proxy.get_active_node())
    leader = multi_node.last_known_leader

    # Step 2: Close the queue -> handle count goes to 0, RemoteQueue persists
    producer1.close(uri_priority, succeed=True)

    # Pick a different node (not the replica, not the leader) to become new
    # primary.  The replica must stay a replica to trigger the bug.
    next_leader = None
    for node in multi_node.nodes():
        if node not in (leader, active_node):
            next_leader = node
            break
    assert next_leader is not None

    # Force next_leader to become the new primary; prevent everyone else
    next_leader.set_quorum(1)
    for node in multi_node.nodes():
        if node not in (next_leader, leader):
            node.set_quorum(99)

    # Step 3: Suspend the primary so it stops processing but stays "alive"
    leader.suspend()

    # Step 4: Send open-queue request (non-blocking) - replica forwards
    # upstream to the frozen primary, which will never respond
    producer2 = replica_proxy.create_client("producer2")
    producer2.open(uri_priority, flags=["write,ack"], block=False)

    # Step 5: Kill the primary - triggers E_CANCELED for the inflight request
    leader.check_exit_code = False
    leader.kill()
    leader.wait()

    # Step 6: Wait for new leader (a different node, NOT the replica)
    multi_node.wait_leader()
    assert multi_node.last_known_leader == next_leader

    # Step 7: The open-queue should complete (with the bug, it times out)
    assert producer2.capture(
        r"<--.*openQueue.*uri = " + re.escape(uri_priority) + r".*\(0\)",
        timeout=15,
    )

    # Step 8: Verify end-to-end: post should succeed
    assert (
        producer2.post(uri_priority, ["msg1"], wait_ack=True, succeed=True)
        == Client.e_SUCCESS
    )
