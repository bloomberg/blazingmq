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
import re
from time import sleep
from typing import List

import blazingmq.dev.it.testconstants as tc
from blazingmq.dev.it.fixtures import (  # pylint: disable=unused-import
    Cluster,
    order,
    multi_node,
)
from blazingmq.dev.it.process.admin import AdminClient
from blazingmq.dev.it.process.client import Client
from blazingmq.dev.it.util import wait_until

pytestmark = order(6)


class BMQITError(RuntimeError):
    """
    BMQ IT error.
    """


class TestClusterNodeShutdown:
    """
    This suite of test cases exercises node shutdown and subsequent failover
    to another cluster node in the presence of all types of queues.
    """

    @staticmethod
    def open_or_raise(client: Client, uri: str, flags: List[str]):
        rc = client.open(uri, flags=flags, succeed=True)
        if rc != Client.e_SUCCESS:
            raise BMQITError(
                f"Failed to open a queue: client = {client}, uri = {uri}, rc = {rc}"
            )

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
            raise BMQITError("State is not restored") from ex
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
            if len(msgs) != 1:
                raise BMQITError(
                    f"Expected 1 message, got: {len(msgs)} msgs.\nMessages history:\n{self.history}"
                )
            if msgs[0].payload not in expected:
                raise BMQITError(
                    f"Unexpected message payload: {msgs[0].payload} (expected: {expected})"
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

    def test_open_queue_while_cluster_blips_quorum(
        self, multi_node: Cluster, domain_urls: tc.DomainUrls
    ):
        """
        Test that if a cluster temporarily loses quorum, clients can still
        open queues and operate normally once quorum is restored.
        """
        du = domain_urls
        cluster = multi_node
        primary = cluster.last_known_leader
        active_replica = cluster.process(self.proxy2.get_active_node())

        # Suspend the other replicas to lose quorum
        other_replicas = [
            n for n in cluster.nodes() if n not in (primary, active_replica)
        ]

        for node in other_replicas:
            node.check_exit_code = False
            node.suspend()
        sleep(0.5)

        # Producer tries to open a new queue; will not succeed
        self.producer2.open(du.uri_priority_2, flags=["write", "ack"], block=False)

        for node in other_replicas:
            node.check_exit_code = False
            node.kill()

        assert primary.outputs_regex("LEADER lost quorum")

        # Restart the killed nodes to restore quorum
        for node in other_replicas:
            node.start()

        # Now the replica should receive an openQueueReponse
        assert active_replica.outputs_regex(
            f"OpenQueueResponse.*{du.uri_priority_2}", timeout=10
        ), (
            f"Replica did not receive OpenQueueResponse for {du.uri_priority_2} after quorum restored"
        )

        # Having a new client to open that queue a second time should succeed
        self.producer3 = self.proxy2.create_client("producer3")
        self.producer3.open(
            du.uri_priority_2, flags=["write", "ack"], block=True, timeout=30
        )

    def test_open_queue_while_cluster_blips_quorum_and_kill_all_non_leader(
        self, multi_node: Cluster, domain_urls: tc.DomainUrls
    ):
        """
        Test that if a cluster temporarily loses quorum, and subsequently all replicas and the client crash, clients can still open
        queues and operate normally once quorum is restored.
        """
        du = domain_urls
        cluster = multi_node
        primary = cluster.last_known_leader
        active_replica = cluster.process(self.proxy2.get_active_node())

        # Suspend the other replicas to lose quorum
        other_replicas = [
            n for n in cluster.nodes() if n not in (primary, active_replica)
        ]

        for node in other_replicas:
            node.check_exit_code = False
            node.suspend()
        sleep(0.5)

        # Producer tries to open a new queue; will not succeed
        self.producer2.open(du.uri_priority_2, flags=["write", "ack"], block=False)

        # Killing *all* replica nodes and the producer on the new queue.  Now,
        # no one except the leader should have any idea about this new queue.
        all_replicas = [active_replica] + other_replicas
        for task in all_replicas + [self.producer2]:
            task.check_exit_code = False
            task.kill()

        assert primary.outputs_regex("LEADER lost quorum")

        # Restart the killed nodes to restore quorum
        for node in all_replicas:
            node.start()
        for node in all_replicas:
            node.wait_status(wait_leader=True, wait_ready=True)

        # Having a new client to open that queue a second time should succeed
        self.producer3 = self.proxy2.create_client("producer3")
        self.producer3.open(du.uri_priority_2, flags=["write", "ack"], block=True)

    def test_open_queue_after_quorum_bump_up(
        self, multi_node: Cluster, domain_urls: tc.DomainUrls
    ):
        """
        Test that if we bump up elector quorum with admin command so that it becomes higher than the number of nodes,
        open queue request and the corresponding csl advisory, will get stuck. If we set quorum back, clients still
        can't open queues.
        """
        du = domain_urls
        cluster = multi_node
        primary = cluster.last_known_leader
        active_replica = cluster.process(self.proxy1.get_active_node())

        # Set quorum to unreachable number more than the number of nodes
        primary.set_quorum(5, cluster.config.name, True)

        res = self.producer1.open(
            du.uri_priority_2,
            flags=["write", "ack"],
            block=True,
            timeout=10,
            no_except=True,
        )
        # 10 seconds is enough. The request has got stuck
        assert res != Client.e_SUCCESS

        # Check that the cluster is still in healthy state
        admin = AdminClient()
        admin.connect(primary.config.host, int(primary.config.port))
        res = admin.send_admin(f"CLUSTERS CLUSTER {cluster.config.name} STATUS")
        healthy = False
        for line in res.splitlines():
            mm = re.search(r"Is Healthy.*\s+(\w+)", line)
            if mm:
                if mm.group(1) == "Yes":
                    healthy = True
                break
        assert healthy

        # Set quorum back to its default value (nodes_count/2 + 1)
        primary.set_quorum(3, cluster.config.name, True)

        # Now the replica should receive an openQueueReponse
        assert active_replica.outputs_regex(
            f"OpenQueueResponse.*{du.uri_priority_2}", timeout=10
        ), (
            f"Replica did not receive OpenQueueResponse for {du.uri_priority_2} after quorum restored"
        )

        # Trying to open that queue again should succeed
        res = self.producer2.open(
            du.uri_priority_2,
            flags=["write", "ack"],
            block=True,
            timeout=10,
            no_except=True,
        )
        assert res == Client.e_SUCCESS
