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

import blazingmq.dev.it.testconstants as tc
from blazingmq.dev.it.fixtures import (  # pylint: disable=unused-import
    Cluster,
    order,
    tweak,
)
from blazingmq.dev.it.process.client import Client

from blazingmq.dev.it.util import wait_until

pytestmark = order(4)


class TestMaxunconfirmed:
    def setup_cluster(self, multi_node, domain_urls: tc.DomainUrls):
        uri_priority = domain_urls.uri_priority
        proxies = multi_node.proxy_cycle()
        # pick proxy in datacenter opposite to the primary's
        next(proxies)
        self.proxy = next(proxies)

        self.producer = self.proxy.create_client("producer")
        self.producer.open(uri_priority, flags=["write,ack"], succeed=True)

        self.consumer = self.proxy.create_client("consumer")
        self.consumer.open(
            uri_priority, flags=["read"], max_unconfirmed_messages=1, succeed=True
        )

    # POST 'n' messages
    # Returns 'True' if all of them succeed, and 'False' otherwise
    def post_n_msgs(self, uri, n):
        results = (
            self.producer.post(uri, payload=["msg"], wait_ack=True) for _ in range(0, n)
        )
        return all(res == Client.e_SUCCESS for res in results)

    @tweak.cluster.queue_operations.shutdown_timeout_ms(1000)
    def test_maxunconfirmed(self, multi_node: Cluster, domain_urls: tc.DomainUrls):
        uri_priority = domain_urls.uri_priority

        # Post 100 messages
        assert self.post_n_msgs(uri_priority, 100)

        # Consumer gets just 1
        self.consumer.wait_push_event()
        assert len(self.consumer.list(uri_priority, block=True)) == 1

        # Confirm 1 message
        self.consumer.confirm(uri_priority, "*", succeed=True)

        # Consumer gets just 1
        self.consumer.wait_push_event()
        assert len(self.consumer.list(uri_priority, block=True)) == 1

        # Shutdown the primary
        leader = multi_node.last_known_leader
        active_node = multi_node.process(self.proxy.get_active_node())

        active_node.set_quorum(1)
        nodes = multi_node.nodes(exclude=[active_node, leader])
        for node in nodes:
            node.set_quorum(4)

        leader.stop()

        # Make sure the active node is new primary
        leader = active_node
        assert leader == multi_node.wait_leader()

        # Confirm 1 message
        self.consumer.confirm(uri_priority, "*", succeed=True)

        # Consumer gets just 1
        self.consumer.wait_push_event()
        assert len(self.consumer.list(uri_priority, block=True)) == 1

        # Confirm 1 message
        self.consumer.confirm(uri_priority, "*", succeed=True)

        # Consumer gets just 1
        self.consumer.wait_push_event()
        assert len(self.consumer.list(uri_priority, block=True)) == 1


def test_stuck_downstream(multi_node: Cluster, domain_urls: tc.DomainUrls):
    """
    Consider 10 consumers with 10 as maxUnconfirmed threshold; combined 100.
    The delivery to individual consumer starts when number of unconfirmed
    messages is less or equal to 8 (80% of 10);
    The delivery to Proxy starts when the number is less or equal to 80 (80% of
    100).
    Consider uneven data consumption by consumers. 8 consumers have 9 pending
    messages each, another one has 8 and the last one has 0.  The total is 80
    and that is enough for the Primary to send 20 messages.  Upon receipt, the
    first 8 consumers are already full, the other two get 2 and 10 messages
    respectively. And Proxy is left with 8 undelivered messages.
    Those messages should NOT go into the linear put-aside list.
    """
    uri = domain_urls.uri_priority

    proxy = next(multi_node.proxy_cycle())
    producer = proxy.create_client("producer")

    producer.open(uri, flags=["write,ack"], succeed=True)

    # open 10 consumers
    consumers = {}
    for i in range(0, 10):
        consumer = proxy.create_client(f"consumer_{i}")

        consumer.open(uri, flags=["read"], succeed=True, max_unconfirmed_messages=10)

        consumers[i] = consumer

    # post 100 messages
    for i in range(0, 100):
        assert (
            producer.post(
                uri, payload=["set#1"], wait_ack=True, block=True, succeed=True
            )
            == Client.e_SUCCESS
        )

    # all 10 consumers are full
    # pylint: disable=cell-var-from-loop
    for i in range(0, 10):
        assert wait_until(lambda: len(consumers[i].list(uri, block=True)) == 10, 3)

    # confirm 20 messages (8*1 + 2 + 10), leaving 80 messages
    for i in range(0, 8):
        consumers[i].confirm(uri, "+1", succeed=True)

    consumers[8].confirm(uri, "+2", succeed=True)
    consumers[9].confirm(uri, "+10", succeed=True)

    leader = multi_node.last_known_leader

    # 80 messages are unconfirmed
    leader.list_messages(domain_urls.domain_priority, tc.TEST_QUEUE, 0, 100)
    assert leader.outputs_substr("Printing 80 message(s)", 5)

    # post 20 messages which go straight to the proxy
    # pylint: disable=cell-var-from-loop
    for i in range(0, 20):
        assert (
            producer.post(
                uri, payload=["set#2"], wait_ack=True, block=True, succeed=True
            )
            == Client.e_SUCCESS
        )

    # but proxy cannot deliver 8 messages (to the first 8 consumers)
    early_exit_message = r"appId = \'(\w+)\' does not have any subscription capacity; early exits delivery"
    assert proxy.capture(early_exit_message, timeout=5)

    # the distrubution is this: 8 * 9 + 10 + 10 (92)
    assert wait_until(lambda: len(consumers[8].list(uri, block=True)) == 10, 3)
    assert wait_until(lambda: len(consumers[9].list(uri, block=True)) == 10, 3)

    # pylint: disable=cell-var-from-loop
    for i in range(0, 8):
        assert wait_until(lambda: len(consumers[i].list(uri, block=True)) == 9, 3)

    # confirm all
    for i in range(0, 10):
        consumers[i].confirm(uri, "*", succeed=True)

    # one more round of confirms for messages deleivered after the first round
    for i in range(0, 10):
        consumers[i].confirm(uri, "*", succeed=True)

    # The bmqtool does not actually wait and check that confirm command was
    # completed: 'success' is returned just when the command is enqueued
    # for send in bmqtool.
    # We need a strong guarantee that confirm was executed on the primary
    # node of the cluster. For that reason we open the same queue for write
    # and close it right away.

    producer.close(uri, succeed=True)

    leader.list_messages(domain_urls.domain_priority, tc.TEST_QUEUE, 0, 100)
    assert leader.outputs_substr("Printing 0 message(s)", 5)
