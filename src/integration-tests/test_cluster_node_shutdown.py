"""
Integration test that shuts down a cluster node and confirms that the system
recovers and keeps working as expected. For detailed documentation and diagrams
go to the relevant section in the README.md, in this directory.
"""

from threading import Semaphore
from time import sleep

import blazingmq.dev.it.testconstants as tc
from blazingmq.dev.it.fixtures import (  # pylint: disable=unused-import
    Cluster,
    order,
    multi_node,
)
from blazingmq.dev.it.process.client import Client

pytestmark = order(6)


class TestClusterNodeShutdown:
    """
    This suite of test cases exercises node shutdown and subsequent failover
    to another cluster node in the presence of all types of queues.
    """

    def setup_cluster(self, cluster: Cluster):
        proxies = cluster.proxy_cycle()

        # 1: Proxy in same datacenter as leader/primary
        self.proxy1 = next(proxies)

        self.producer1 = self.proxy1.create_client("producer1")
        assert (
            self.producer1.open(
                tc.URI_PRIORITY_SC, flags=["write", "ack"], succeed=True
            )
            == Client.e_SUCCESS
        )
        assert (
            self.producer1.open(tc.URI_FANOUT_SC, flags=["write", "ack"], succeed=True)
            == Client.e_SUCCESS
        )
        assert (
            self.producer1.open(tc.URI_BROADCAST, flags=["write", "ack"], succeed=True)
            == Client.e_SUCCESS
        )

        self.consumer1 = self.proxy1.create_client("consumer1")
        assert (
            self.consumer1.open(tc.URI_PRIORITY_SC, flags=["read"], succeed=True)
            == Client.e_SUCCESS
        )
        assert (
            self.consumer1.open(tc.URI_FANOUT_SC_FOO, flags=["read"], succeed=True)
            == Client.e_SUCCESS
        )
        assert (
            self.consumer1.open(tc.URI_FANOUT_SC_BAR, flags=["read"], succeed=True)
            == Client.e_SUCCESS
        )
        assert (
            self.consumer1.open(tc.URI_BROADCAST, flags=["read"], succeed=True)
            == Client.e_SUCCESS
        )

        # 2: Replica proxy
        self.proxy2 = next(proxies)

        self.producer2 = self.proxy2.create_client("producer2")
        assert (
            self.producer2.open(
                tc.URI_PRIORITY_SC, flags=["write", "ack"], succeed=True
            )
            == Client.e_SUCCESS
        )
        assert (
            self.producer2.open(tc.URI_FANOUT_SC, flags=["write", "ack"], succeed=True)
            == Client.e_SUCCESS
        )
        assert (
            self.producer2.open(tc.URI_BROADCAST, flags=["write", "ack"], succeed=True)
            == Client.e_SUCCESS
        )

        self.consumer2 = self.proxy2.create_client("consumer2")
        assert (
            self.consumer2.open(tc.URI_PRIORITY_SC, flags=["read"], succeed=True)
            == Client.e_SUCCESS
        )
        assert (
            self.consumer2.open(tc.URI_FANOUT_SC_FOO, flags=["read"], succeed=True)
            == Client.e_SUCCESS
        )
        assert (
            self.consumer2.open(tc.URI_FANOUT_SC_BAR, flags=["read"], succeed=True)
            == Client.e_SUCCESS
        )
        assert (
            self.consumer2.open(tc.URI_BROADCAST, flags=["read"], succeed=True)
            == Client.e_SUCCESS
        )

    def test_primary_shutdown_with_proxy(self, multi_node: Cluster):
        cluster = multi_node
        primary = cluster.last_known_leader

        self._post_kill_recover_post(cluster, primary)

    def test_replica_shutdown_with_proxy(self, multi_node: Cluster):
        cluster = multi_node
        replica = cluster.process(self.proxy2.get_active_node())

        self._post_kill_recover_post(cluster, replica)

    def _post_kill_recover_post(self, cluster, active_node):
        self._verify_all_queues_operational()  # Prior to kill

        recovery = Semaphore(0)

        def release_recovery_if_state_restored(line):
            if ": state restored" in line:
                recovery.release()

        for node in cluster.nodes() + cluster.proxies():
            node.add_async_log_hook(release_recovery_if_state_restored)

        # Kill active node and wait for state restoration of exactly one node
        # - the node which was connected to the active and through which queues
        #   were opened
        active_node.check_exit_code = False
        active_node.kill()

        assert recovery.acquire(timeout=120)  # pylint: disable=consider-using-with
        sleep(5)

        self._verify_all_queues_operational()  # After recovery

    def _verify_all_queues_operational(self):
        self._verify(tc.URI_PRIORITY_SC, tc.URI_PRIORITY_SC, None, True)
        self._verify(tc.URI_FANOUT_SC, tc.URI_FANOUT_SC_FOO, tc.URI_FANOUT_SC_BAR, True)
        self._verify(tc.URI_BROADCAST, tc.URI_BROADCAST, None, False)

    def _verify(self, uri_post, uri_group1, uri_group2, priority):
        def check_received_one_of(consumer, uri_group, *expected):
            msgs = consumer.list(uri_group, block=True)
            assert len(msgs) == 1
            assert msgs[0].payload in expected
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
            assert len(self.consumer1.list(uri_group2, block=True)) == 0

        # Main part of the test starts here

        self.producer1.post(uri_post, payload=["msg1"], succeed=True, wait_ack=True)

        # One has received it but we don't know which one. We will check after
        # we post another one
        if not priority:
            check_both_received_one_of("msg1")

        self.producer2.post(uri_post, payload=["msg2"], succeed=True, wait_ack=True)

        if priority:
            # By this point both should have received one but we don't know
            # who got what
            check_both_received_one_of("msg1", "msg2")
        else:
            check_both_received_one_of("msg2")
