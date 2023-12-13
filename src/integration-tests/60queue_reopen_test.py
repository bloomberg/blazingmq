"""
Integration tests for queue reopen scenarios.
"""

import bmq.dev.it.testconstants as tc
from bmq.dev.it.fixtures import (  # pylint: disable=unused-import
    Cluster,
    standard_cluster,
)
from bmq.dev.it.process.client import Client


class TestReopenQueue:
    def test_reopen_empty_queue(self, standard_cluster: Cluster):
        """
        DRQS 158382093.  If queue has no handles by the time cluster state
        restores, it should still be notified in order to update its state.
        """
        proxies = standard_cluster.proxy_cycle()
        # pick proxy in datacenter opposite to the primary's
        next(proxies)
        replica_proxy = next(proxies)

        # Start a producer and open a queue
        producer = replica_proxy.create_client("producer")
        producer.open(tc.URI_PRIORITY, flags=["write,ack"], succeed=True)

        # If queue open has succeeded, then active_node is known
        active_node = standard_cluster.process(replica_proxy.get_active_node())

        # Close the queue.  The replica keeps (stale) RemoteQueue
        producer.exit_gracefully()

        # Prevent 'active_node' from becoming new primary
        active_node.set_quorum(4)

        # Shutdown the primary
        leader = standard_cluster.last_known_leader
        leader.stop()

        # Start a producer and open a queue
        producer = replica_proxy.create_client("producer")
        producer.open(tc.URI_PRIORITY, flags=["write,ack"], succeed=True)

        # Post should result in successful Ack
        assert (
            producer.post(tc.URI_PRIORITY, ["1"], wait_ack=True, succeed=True)
            == Client.e_SUCCESS
        )

    def test_reopen_substream(self, standard_cluster: Cluster):
        """
        DRQS 169527537.  Make a primary's client reopen the same appId with a
        different subId.
        """

        leader = standard_cluster.last_known_leader
        consumer1 = leader.create_client("consumer1")
        consumer1.open(tc.URI_FANOUT_FOO, flags=["read"], succeed=True)

        consumer2 = leader.create_client("consumer2")
        consumer2.open(tc.URI_FANOUT_FOO, flags=["read"], succeed=True)
        consumer2.open(tc.URI_FANOUT_BAR, flags=["read"], succeed=True)

        consumer2.close(tc.URI_FANOUT_FOO, succeed=True)
        consumer2.open(tc.URI_FANOUT_FOO, flags=["read"], succeed=True)
