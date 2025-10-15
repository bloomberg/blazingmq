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
Integration test that shuts down a cluster node and confirms that the system
recovers and keeps working as expected. For detailed documentation and diagrams
go to the relevant section in the README.md, in this directory.
"""

import queue
from time import sleep
from typing import List

import blazingmq.dev.it.testconstants as tc
from blazingmq.dev.it.fixtures import (  # pylint: disable=unused-import
    Cluster,
    order,
    multi_node,
)
from blazingmq.dev.it.common import BMQTestError, BMQTST_ASSERT
from blazingmq.dev.it.process.client import Client
from blazingmq.dev.it.util import wait_until

pytestmark = order(6)


class TestClusterNodeShutdown:
    """
    This suite of test cases exercises node shutdown and subsequent failover
    to another cluster node in the presence of all types of queues.
    """

    @staticmethod
    def open_or_raise(client: Client, uri: str, flags: List[str]):
        rc = client.open(uri, flags=flags, succeed=True)
        BMQTST_ASSERT(rc == Client.e_SUCCESS,
                      "Failed to open a queue",
                      client=client,
                      uri=uri,
                      flags=flags,
                      rc=rc)

    def setup_cluster(self, cluster: Cluster, domain_urls: tc.DomainUrls):
        du = domain_urls
        proxies = cluster.proxy_cycle()

        # 1: Proxy in same datacenter as leader/primary
        self.proxy1 = next(proxies)

        self.producer1 = self.proxy1.create_client("producer1")
        self.open_or_raise(self.producer1, du.uri_priority, ["write", "ack"])
        self.open_or_raise(self.producer1, du.uri_fanout, ["write", "ack"])
        self.open_or_raise(self.producer1, tc.URI_BROADCAST, ["write", "ack"])

        self.consumer1 = self.proxy1.create_client("consumer1")
        self.open_or_raise(self.consumer1, du.uri_priority, ["read"])
        self.open_or_raise(self.consumer1, du.uri_fanout_foo, ["read"])
        self.open_or_raise(self.consumer1, du.uri_fanout_bar, ["read"])
        self.open_or_raise(self.consumer1, tc.URI_BROADCAST, ["read"])

        # 2: Replica proxy
        self.proxy2 = next(proxies)

        self.producer2 = self.proxy2.create_client("producer2")
        self.open_or_raise(self.producer2, du.uri_priority, ["write", "ack"])
        self.open_or_raise(self.producer2, du.uri_fanout, ["write", "ack"])
        self.open_or_raise(self.producer2, tc.URI_BROADCAST, ["write", "ack"])

        self.consumer2 = self.proxy2.create_client("consumer2")
        self.open_or_raise(self.consumer2, du.uri_priority, ["read"])
        self.open_or_raise(self.consumer2, du.uri_fanout_foo, ["read"])
        self.open_or_raise(self.consumer2, du.uri_fanout_bar, ["read"])
        self.open_or_raise(self.consumer2, tc.URI_BROADCAST, ["read"])

        # 3: Setup message history
        self.history = []

    def test_primary_shutdown_with_proxy(
        self, multi_node: Cluster, domain_urls: tc.DomainUrls
    ):
        cluster = multi_node
        primary = cluster.last_known_leader

        self._post_kill_recover_post(cluster, primary, domain_urls)

    def test_replica_shutdown_with_proxy(
        self, multi_node: Cluster, domain_urls: tc.DomainUrls
    ):
        cluster = multi_node
        replica = cluster.process(self.proxy2.get_active_node())

        self._post_kill_recover_post(cluster, replica, domain_urls)

    def _post_kill_recover_post(self, cluster, active_node, domain_urls: tc.DomainUrls):
        self._verify_all_queues_operational(domain_urls)  # Prior to kill

        recovery = queue.Queue()

        def release_recovery_if_state_restored(line: str) -> None:
            if ": state restored" in line:
                recovery.put(line)

        for node in cluster.nodes() + cluster.proxies():
            node.add_async_log_hook(release_recovery_if_state_restored)

        # Kill active node and wait for state restoration of exactly one node
        # - the node which was connected to the active and through which queues
        #   were opened
        active_node.check_exit_code = False
        active_node.kill()

        try:
            _ = recovery.get(timeout=120)
        except queue.Empty as ex:
            # No recovery log observed
            raise BMQTestError("State is not restored") from ex
        sleep(5)

        self._verify_all_queues_operational(domain_urls)  # After recovery

    def _verify_all_queues_operational(self, domain_urls: tc.DomainUrls):
        self.history = []
        du = domain_urls
        self._verify(du.uri_priority, du.uri_priority, None, True)
        self._verify(du.uri_fanout, du.uri_fanout_foo, du.uri_fanout_bar, True)
        self._verify(tc.URI_BROADCAST, tc.URI_BROADCAST, None, False)

    def _verify(self, uri_post, uri_group1, uri_group2, priority):
        def check_received_one_of(consumer: Client, uri_group: str, *expected):
            # wait_push_event() might be triggered by either uri_group1 or uri_group2,
            # we might still need to wait to get messages
            wait_until(
                lambda: len(consumer.list(uri_group, block=True)) >= 1, timeout=5
            )
            msgs = consumer.list(uri_group, block=True)
            self.history.append((str(consumer.name), uri_group, msgs))
            BMQTST_ASSERT(len(msgs) == 1,
                          "Expected exactly 1 message",
                          client=consumer,
                          uri=uri_group,
                          observed_messages=msgs,
                          history=self.history)
            BMQTST_ASSERT(msgs[0].payload in expected,
                          "Unexpected message payload",
                          client=consumer,
                          uri=uri_group,
                          observed_messages=msgs,
                          history=self.history,
                          expected_any_of=expected
                          )
            consumer.confirm(uri_group, "*", succeed=True)

        def check_both_received_one_of(*expected):
            self.consumer1.wait_push_event()
            check_received_one_of(self.consumer1, uri_group1, *expected)

            self.consumer2.wait_push_event()
            check_received_one_of(self.consumer2, uri_group1, *expected)

            if uri_group2 is not None:
                check_received_one_of(self.consumer1, uri_group2, *expected)
                check_received_one_of(self.consumer2, uri_group2, *expected)

        # Both have nothing at the beginning
        assert len(self.consumer1.list(uri_group1, block=True)) == 0
        assert len(self.consumer2.list(uri_group1, block=True)) == 0

        if uri_group2 is not None:
            assert len(self.consumer1.list(uri_group2, block=True)) == 0
            assert len(self.consumer2.list(uri_group2, block=True)) == 0

        # Main part of the test starts here

        self.producer1.post(uri_post, payload=["msg1"], succeed=True, wait_ack=True)

        # One has received it, but we don't know which one. We will check after
        # we post another one
        if not priority:
            check_both_received_one_of("msg1")

        self.producer2.post(uri_post, payload=["msg2"], succeed=True, wait_ack=True)

        if priority:
            # By this point both should have received one,
            # but we don't know who got what
            check_both_received_one_of("msg1", "msg2")
        else:
            check_both_received_one_of("msg2")
