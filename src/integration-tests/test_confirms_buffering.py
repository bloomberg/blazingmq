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

    def setup_cluster(self, cluster: Cluster):
        proxies = cluster.proxy_cycle()
        # pick proxy in datacenter opposite to the primary's
        next(proxies)
        self.replica_proxy = next(proxies)

        self.producer = self.replica_proxy.create_client("producer")
        self.producer.open(tc.URI_FANOUT, flags=["write,ack"], succeed=True)

        self.consumer = self.replica_proxy.create_client("consumer")
        for uri in [tc.URI_FANOUT_FOO, tc.URI_FANOUT_BAR, tc.URI_FANOUT_BAZ]:
            self.consumer.open(uri, flags=["read"], succeed=True)

        self.producer.post(tc.URI_FANOUT, ["1"], wait_ack=True, succeed=True)

        assert wait_until(lambda: len(self.consumer.list(block=True)) == 3, 5)

        self.leader = cluster.last_known_leader
        self.active_node = cluster.process(self.replica_proxy.get_active_node())

        for node in cluster.nodes():
            node.set_quorum(4)
            if node not in (self.leader, self.active_node):
                self.candidate = node

    def test_kill_primary_confirm_puts_close_app_start_primary(
        self, multi_node: Cluster  # pylint: disable=unused-argument
    ):
        # add one more tc.URI_FANOUT_BAZ consumer
        baz = self.replica_proxy.create_client("baz")
        baz.open(tc.URI_FANOUT_BAZ, flags=["read"], succeed=True)

        self.active_node.drain()
        # Kill primary
        self.leader.force_stop()

        # confirm and then close keeping tc.URI_FANOUT_FOO
        # tc.URI_FANOUT_FOO - still open
        # tc.URI_FANOUT_BAR - all consumers closed
        # tc.URI_FANOUT_BAZ - one consumes has closed and one is till open
        #
        for uri in [tc.URI_FANOUT_BAR, tc.URI_FANOUT_BAZ]:
            self.consumer.confirm(uri, "*", succeed=True)
            self.consumer.close(uri, block=True)

        # post and then close producer
        self.producer.post(tc.URI_FANOUT, ["2"], wait_ack=True, succeed=True)
        self.producer.close(tc.URI_FANOUT, block=True)

        # get new primary
        self.candidate.set_quorum(3)

        # If shutting down primary, the replica needs to wait for new primary.
        self.active_node.wait_status(wait_leader=True, wait_ready=False)
        self.active_node.capture("is back to healthy state")
