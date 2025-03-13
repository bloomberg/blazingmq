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
)
from blazingmq.dev.it.util import wait_until


class TestConfirmsBuffering:
    """
    Test the sequence of buffering CONFIRMs (due to upstream failure) followed
    by closing the queue, followed by queue reopen.  If the subQueueId is gone,
    CONFIRMs cannot proceed (there is no upstream subQueueId anymore).
    """

    def setup_cluster(self, cluster: Cluster, domain_urls: tc.DomainUrls):
        du = domain_urls
        proxies = cluster.proxy_cycle()
        # pick proxy in datacenter opposite to the primary's
        next(proxies)
        self.replica_proxy = next(proxies)

        self.producer = self.replica_proxy.create_client("producer")
        self.producer.open(du.uri_fanout, flags=["write,ack"], succeed=True)

        self.consumer = self.replica_proxy.create_client("consumer")
        for uri in [du.uri_fanout_foo, du.uri_fanout_bar, du.uri_fanout_baz]:
            self.consumer.open(uri, flags=["read"], succeed=True)

        self.producer.post(du.uri_fanout, ["1"], wait_ack=True, succeed=True)

        assert wait_until(lambda: len(self.consumer.list(block=True)) == 3, 5)

        self.leader = cluster.last_known_leader
        self.active_node = cluster.process(self.replica_proxy.get_active_node())

        for node in cluster.nodes():
            node.set_quorum(4)
            if node not in (self.leader, self.active_node):
                self.candidate = node

    def test_kill_primary_confirm_puts_close_app_start_primary(
        self,
        multi_node: Cluster,
        domain_urls: tc.DomainUrls,  # pylint: disable=unused-argument
    ):
        du = domain_urls
        # add one more du.uri_fanout_baz consumer
        baz = self.replica_proxy.create_client("baz")
        baz.open(du.uri_fanout_baz, flags=["read"], succeed=True)

        self.active_node.drain()
        # Kill primary
        self.leader.force_stop()

        # confirm and then close keeping du.uri_fanout_foo
        # du.uri_fanout_foo - still open
        # du.uri_fanout_bar - all consumers closed
        # du.uri_fanout_baz - one consumes has closed and one is till open
        #
        for uri in [du.uri_fanout_bar, du.uri_fanout_baz]:
            self.consumer.confirm(uri, "*", succeed=True)
            self.consumer.close(uri, block=True)

        # post and then close producer
        self.producer.post(du.uri_fanout, ["2"], wait_ack=True, succeed=True)
        self.producer.close(du.uri_fanout, block=True)

        # get new primary
        self.candidate.set_quorum(3)

        # If shutting down primary, the replica needs to wait for new primary.
        self.active_node.wait_status(wait_leader=True, wait_ready=False)
        self.active_node.capture("is back to healthy state")
