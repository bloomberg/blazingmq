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

import blazingmq.dev.it.testconstants as tc
from blazingmq.dev.it.fixtures import (  # pylint: disable=unused-import
    Cluster,
    order,
    multi_node,
)
from blazingmq.dev.it.process.client import Client


def test_reopen_empty_queue(multi_node: Cluster):
    """
    If queue has no handles by the time cluster state restores, it should
    still be notified in order to update its state.
    """
    proxies = multi_node.proxy_cycle()
    # pick proxy in datacenter opposite to the primary's
    next(proxies)
    replica_proxy = next(proxies)

    # Start a producer and open a queue
    producer = replica_proxy.create_client("producer")
    producer.open(tc.URI_PRIORITY, flags=["write,ack"], succeed=True)

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
    producer.open(tc.URI_PRIORITY, flags=["write,ack"], succeed=True)

    # Post should result in successful Ack
    assert (
        producer.post(tc.URI_PRIORITY, ["1"], wait_ack=True, succeed=True)
        == Client.e_SUCCESS
    )


def test_reopen_substream(multi_node: Cluster):
    """
    Ticket 169527537.  Make a primary's client reopen the same appId with a
    different subId.
    """

    leader = multi_node.last_known_leader
    consumer1 = leader.create_client("consumer1")
    consumer1.open(tc.URI_FANOUT_FOO, flags=["read"], succeed=True)

    consumer2 = leader.create_client("consumer2")
    consumer2.open(tc.URI_FANOUT_FOO, flags=["read"], succeed=True)
    consumer2.open(tc.URI_FANOUT_BAR, flags=["read"], succeed=True)

    consumer2.close(tc.URI_FANOUT_FOO, succeed=True)
    consumer2.open(tc.URI_FANOUT_FOO, flags=["read"], succeed=True)
