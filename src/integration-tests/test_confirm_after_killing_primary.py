import bmq.dev.it.testconstants as tc
from bmq.dev.it.fixtures import (  # pylint: disable=unused-import
    Cluster,
    standard_cluster as cluster,
)
from bmq.dev.it.process.client import Client
from bmq.dev.it.util import wait_until


class TestConfirmAfterKillingPrimary:
    """
    This test case verifies fix for the broker crash when virtual iterator goes
    out of sync while processing CONFIRM after converting priority queue to
    local.  DRQS 144256387.
    """

    def test_confirm_after_killing_primary(self, cluster: Cluster):
        proxies = cluster.proxy_cycle()

        # we want proxy connected to a replica
        next(proxies)

        proxy = next(proxies)
        consumer = proxy.create_client("consumer")
        producer = proxy.create_client("producer")

        producer.open(tc.URI_PRIORITY, flags=["write", "ack"], succeed=True)
        consumer.open(tc.URI_PRIORITY, flags=["read"], succeed=True)

        producer.post(tc.URI_PRIORITY, payload=["msg1"], wait_ack=True, succeed=True)

        consumer.wait_push_event()
        assert wait_until(
            lambda: len(consumer.list(tc.URI_PRIORITY, block=True)) == 1, 2
        )
        msgs = consumer.list(tc.URI_PRIORITY, block=True)
        assert msgs[0].payload == "msg1"

        # make the quorum for replica to be 1 so it becomes new primary
        replica = cluster.process(proxy.get_active_node())
        for node in cluster.nodes():
            if node == replica:
                node.set_quorum(1)
            else:
                node.set_quorum(99)

        # kill the primary
        replica.drain()
        cluster.drain()
        leader = cluster.last_known_leader
        leader.check_exit_code = False
        leader.kill()
        leader.wait()

        # wait for new leader
        cluster.wait_leader()
        assert cluster.last_known_leader == replica

        # need to wait for remote queue converted to local
        # otherwise CONFIRM/PUT can get rejected if happen in between the
        # conversion
        assert replica.outputs_substr(
            f"Rebuilt internal state of queue engine for queue [{tc.URI_PRIORITY}]",
            timeout=5,
        )

        # confirm
        assert consumer.confirm(tc.URI_PRIORITY, "*", block=True) == Client.e_SUCCESS
        # post
        producer.post(tc.URI_PRIORITY, payload=["msg2"], wait_ack=True, succeed=True)

        # verify that replica did not crash
        consumer.wait_push_event()
        assert wait_until(
            lambda: len(consumer.list(tc.URI_PRIORITY, block=True)) == 1, 2
        )
        msgs = consumer.list(tc.URI_PRIORITY, block=True)
        assert msgs[0].payload == "msg2"
