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
    def setup_cluster(self, multi_node):
        proxies = multi_node.proxy_cycle()
        # pick proxy in datacenter opposite to the primary's
        next(proxies)
        self.proxy = next(proxies)

        self.producer = self.proxy.create_client("producer")
        self.producer.open(tc.URI_PRIORITY, flags=["write,ack"], succeed=True)

        self.consumer = self.proxy.create_client("consumer")
        self.consumer.open(
            tc.URI_PRIORITY, flags=["read"], max_unconfirmed_messages=1, succeed=True
        )

    # POST 'n' messages
    # Returns 'True' if all of them succeed, and 'False' otherwise
    def post_n_msgs(self, uri, n):
        results = (
            self.producer.post(uri, payload=["msg"], wait_ack=True) for _ in range(0, n)
        )
        return all(res == Client.e_SUCCESS for res in results)

    @tweak.cluster.queue_operations.stop_timeout_ms(1000)
    def test_maxunconfirmed(self, multi_node: Cluster):
        # Post 100 messages
        assert self.post_n_msgs(tc.URI_PRIORITY, 100)

        # Consumer gets just 1
        self.consumer.wait_push_event()
        assert len(self.consumer.list(tc.URI_PRIORITY, block=True)) == 1

        # Confirm 1 message
        self.consumer.confirm(tc.URI_PRIORITY, "*", succeed=True)

        # Consumer gets just 1
        self.consumer.wait_push_event()
        assert len(self.consumer.list(tc.URI_PRIORITY, block=True)) == 1

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
        self.consumer.confirm(tc.URI_PRIORITY, "*", succeed=True)

        # Consumer gets just 1
        self.consumer.wait_push_event()
        assert len(self.consumer.list(tc.URI_PRIORITY, block=True)) == 1

        # Confirm 1 message
        self.consumer.confirm(tc.URI_PRIORITY, "*", succeed=True)

        # Consumer gets just 1
        self.consumer.wait_push_event()
        assert len(self.consumer.list(tc.URI_PRIORITY, block=True)) == 1
