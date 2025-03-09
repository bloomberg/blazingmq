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
    multi_node,
    tweak,
)
from blazingmq.dev.it.process.client import Client

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
