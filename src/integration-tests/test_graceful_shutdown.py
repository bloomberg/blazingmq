import functools
import re
from typing import Iterator

import blazingmq.dev.it.testconstants as tc
import pytest
from blazingmq.dev.it import fixtures
from blazingmq.dev.it.fixtures import (  # pylint: disable=unused-import
    Cluster,
    Mode,
    test_logger,
    order,
    multi_node,
    tweak,
    virtual_cluster_config,
)
from blazingmq.dev.it.process.client import Client
from blazingmq.dev.it.util import wait_until
from blazingmq.dev.configurator import Configurator

OTHER_DOMAIN = f"{tc.DOMAIN_PRIORITY}.other"


def multi_cluster_config(
    configurator: Configurator, port_allocator: Iterator[int], mode: Mode
):
    virtual_cluster_config(configurator, port_allocator, mode)
    other_cluster = configurator.cluster(
        "otherCluster",
        nodes=[
            configurator.broker(
                "localhost", next(port_allocator), f"{dc}{i}", data_center=dc
            )
            for dc in ("east", "west")
            for i in (3, 4)
        ],
    )
    other_cluster.priority_domain(OTHER_DOMAIN)

    for proxy in ("eastp", "westp"):
        configurator.brokers[proxy].proxy(other_cluster)


@pytest.fixture(
    params=[
        pytest.param(
            functools.partial(multi_cluster_config, mode=mode),
            id=f"multi_cluster{mode.suffix}",
            marks=[
                pytest.mark.integrationtest,
                pytest.mark.pr_integrationtest,
                pytest.mark.multi,
                *mode.marks,
            ],
        )
        for mode in Mode.__members__.values()
    ]
)
def multi_cluster(request):
    yield from fixtures.cluster_fixture(request, request.param)


class TestGracefulShutdown:
    def post_kill_confirm(self, node, peer):
        test_logger.info("posting...")

        # post 3 PUTs
        for i in range(1, 4):
            self.producer.post(tc.URI_FANOUT, payload=[f"msg{i}"], succeed=True)

        uris = [tc.URI_FANOUT_FOO, tc.URI_FANOUT_BAR, tc.URI_FANOUT_BAZ]
        # start consumer
        consumer = self.replica_proxy.create_client("consumer")

        for uri in uris:
            consumer.open(uri, flags=["read"], succeed=True)

        # pylint: disable=cell-var-from-loop; passing lambda to 'wait_until' is safe
        # receive messages
        consumer.wait_push_event()
        for uri in uris:
            assert wait_until(lambda: len(consumer.list(uri, block=True)) == 3, 2)

        # wait for previous PUTs to replicate
        def num_broker_messages():
            num_messages = 0
            peer.drain()
            peer.list_messages(tc.DOMAIN_FANOUT, tc.TEST_QUEUE, 0, 3)

            capture = peer.capture(r"Printing (-?\d+) message\(s\)", timeout=2)
            assert capture

            num_messages = int(capture[1])
            return num_messages

        assert wait_until(lambda: num_broker_messages() == 3, 3)

        # DRQS 168471730.   Downstream should update its opened subIds upon
        # closing and not attempt to deconfigure it upon StopRequest
        self.producer.close(tc.URI_FANOUT, block=True)

        # start graceful shutdown
        peer.drain()
        node.exit_gracefully()

        # confirm 2 messages leaving 1 unconfirmed
        for uri in uris:
            guids = [msg.guid for msg in consumer.list(uri, block=True)]
            assert consumer.confirm(uri, guids[0], block=True) == Client.e_SUCCESS
            assert consumer.confirm(uri, guids[1], block=True) == Client.e_SUCCESS

        assert wait_until(lambda: num_broker_messages() == 1, 10)

        consumer.drain()

        # This Open Queue is async and will not succeed until new primary is
        # available.  Switching the primary depends on the presence of
        # unconfirmed messages (1)
        self.producer.open(
            tc.URI_PRIORITY, flags=["write,ack"], block=False, async_=True
        )

        consumer.open(tc.URI_PRIORITY, flags=["read"], block=False, async_=True)

        for uri in uris:
            assert consumer.confirm(uri, "*", block=True) == Client.e_SUCCESS

        assert wait_until(lambda: num_broker_messages() == 0, 5)

        # now check if that queue is open and functional
        assert consumer.capture(
            f'<--.*openQueue.*uri = {re.escape(tc.URI_PRIORITY)}.*result = "SUCCESS ',
            timeout=15,
        )
        assert self.producer.capture(
            f'<--.*openQueue.*uri = {re.escape(tc.URI_PRIORITY)}.*result = "SUCCESS ',
            timeout=15,
        )

        self.producer.post(tc.URI_PRIORITY, payload=["check"], succeed=True)
        consumer.wait_push_event()

        msgs = consumer.list(tc.URI_PRIORITY, block=True)
        assert len(msgs) == 1
        assert msgs[0].payload == "check"

        assert consumer.confirm(tc.URI_PRIORITY, "*", block=True) == Client.e_SUCCESS

        node.wait()
        # assert node.return_code == 0

    def kill_wait_unconfirmed(self, peer):
        test_logger.info("posting...")

        uriWrite = tc.URI_FANOUT
        uriRead = tc.URI_FANOUT_FOO

        # post 2 PUTs
        self.producer.post(uriWrite, payload=["msg1"], succeed=True)
        self.producer.post(uriWrite, payload=["msg2"], succeed=True)

        # start consumer
        consumer = peer.create_client("consumer")

        consumer.open(uriRead, flags=["read"], succeed=True)

        # receive messages
        consumer.wait_push_event()
        assert wait_until(lambda: len(consumer.list(uriRead, block=True)) == 2, 2)

        # start graceful shutdown
        peer.exit_gracefully()

        capture = peer.capture(r"Waiting for (-?\d+) unconfirmed messages", timeout=2)
        assert capture

        num_messages = int(capture[1])
        assert num_messages == 2

        # confirm 1 message leaving 1 unconfirmed
        guids = [msg.guid for msg in consumer.list(uriRead, block=True)]
        assert consumer.confirm(uriRead, guids[0], block=True) == Client.e_SUCCESS

        consumer.drain()

        capture = peer.capture(r"Waiting for (-?\d+) unconfirmed messages", timeout=2)
        assert capture

        num_messages = int(capture[1])
        assert num_messages == 1

        # confirm the last unconfirmed message
        assert consumer.confirm(uriRead, guids[1], block=True) == Client.e_SUCCESS

        consumer.drain()

        capture = peer.capture(
            r"finish shutdown sequence having (-?\d+) unconfirmed messages", timeout=2
        )
        assert capture

        num_messages = int(capture[1])
        assert num_messages == 0

        peer.wait()

    def setup_cluster(self, cluster):
        self.cluster = cluster
        proxies = cluster.proxy_cycle()
        # pick proxy in datacenter opposite to the primary's
        next(proxies)
        self.replica_proxy = next(proxies)
        self.producer = self.replica_proxy.create_client("producer")
        self.producer.open(tc.URI_FANOUT, flags=["write,ack"], succeed=True)

    @tweak.cluster.queue_operations.stop_timeout_ms(1000)
    def test_shutting_down_primary(self, multi_node: Cluster):
        cluster = multi_node
        leader = cluster.last_known_leader
        active_node = cluster.process(self.replica_proxy.get_active_node())
        self.post_kill_confirm(leader, active_node)

    @tweak.cluster.queue_operations.stop_timeout_ms(1000)
    def test_shutting_down_replica(self, multi_node: Cluster):
        cluster = multi_node
        leader = cluster.last_known_leader
        active_node = cluster.process(self.replica_proxy.get_active_node())
        self.post_kill_confirm(active_node, leader)

    @tweak.cluster.queue_operations.stop_timeout_ms(1000)
    @tweak.cluster.queue_operations.shutdown_timeout_ms(5000)
    def test_wait_unconfirmed_proxy(
        self, multi_node  # pylint: disable=unused-argument
    ):
        proxy = self.replica_proxy
        self.kill_wait_unconfirmed(proxy)

    @tweak.cluster.queue_operations.stop_timeout_ms(1000)
    @tweak.cluster.queue_operations.shutdown_timeout_ms(5000)
    def test_wait_unconfirmed_replica(
        self, multi_node  # pylint: disable=unused-argument
    ):
        cluster = multi_node
        replica = cluster.process(self.replica_proxy.get_active_node())
        self.kill_wait_unconfirmed(replica)

    @tweak.cluster.queue_operations.stop_timeout_ms(3000)
    @tweak.cluster.queue_operations.shutdown_timeout_ms(2000)
    def test_cancel_unconfirmed_timer(
        self, multi_node  # pylint: disable=unused-argument
    ):
        uriWrite = tc.URI_FANOUT
        uriRead = tc.URI_FANOUT_FOO

        leader = multi_node.last_known_leader

        # post 2 PUTs
        self.producer.post(uriWrite, payload=["msg1"], succeed=True)
        self.producer.post(uriWrite, payload=["msg2"], succeed=True)

        # start consumer
        consumer = self.replica_proxy.create_client("consumer")

        consumer.open(uriRead, flags=["read"], succeed=True)

        replica = multi_node.process(self.replica_proxy.get_active_node())

        # receive messages
        consumer.wait_push_event()
        assert wait_until(lambda: len(consumer.list(uriRead, block=True)) == 2, 2)

        # start graceful shutdown
        leader.exit_gracefully()

        capture = replica.capture(r"waiting for 2 unconfirmed message", timeout=2)
        assert capture

        leader.force_stop()

        replica.drain()

        # wait for the queue to recover
        self.producer.post(uriWrite, payload=["msg3"], succeed=True)
        consumer.wait_push_event()

        # the timer should be cancelled
        capture = replica.capture(r"giving up on 2 unconfirmed message", timeout=2)
        assert not capture

    @tweak.cluster.queue_operations.stop_timeout_ms(3000)
    @tweak.cluster.queue_operations.shutdown_timeout_ms(2000)
    def test_multiple_stop_requests(self, multi_cluster: Cluster):
        cluster = multi_cluster

        uriWrite = tc.URI_FANOUT
        uriRead = tc.URI_FANOUT_FOO

        # post 2 PUTs
        self.producer.post(uriWrite, payload=["msg1"], succeed=True)
        self.producer.post(uriWrite, payload=["msg2"], succeed=True)

        # start consumer
        consumer = self.replica_proxy.create_client("consumer")

        consumer.open(uriRead, flags=["read"], succeed=True)

        consumer2 = self.replica_proxy.create_client("consumer2")
        consumer2.open(f"bmq://{OTHER_DOMAIN}/q", flags=["read"], succeed=True)

        # receive messages
        consumer.wait_push_event()
        assert wait_until(lambda: len(consumer.list(uriRead, block=True)) == 2, 2)

        # start graceful shutdown
        for node in cluster.virtual_nodes():
            node.exit_gracefully()

        capture = self.replica_proxy.capture(
            r"waiting for 2 unconfirmed message", timeout=2
        )
        assert capture

    @tweak.cluster.queue_operations.stop_timeout_ms(999999)
    @tweak.cluster.queue_operations.shutdown_timeout_ms(999999)
    def test_active_node_down_stop_requests(self, multi_cluster: Cluster):
        """
        DRQS 169782591
        We have: Consumer -> Proxy -> active_node -> upstream_node.
        Start shutting down active_node (one of cluster.virtual_nodes())
        Because there are unconfirmed, Proxy lingers with StopResponse.
        Kill upstream_node.  That event should not cancel StopRequest!
        """
        cluster = multi_cluster

        uriWrite = tc.URI_FANOUT
        uriRead = tc.URI_FANOUT_FOO

        active_node = cluster.process(self.replica_proxy.get_active_node())
        assert active_node in cluster.virtual_nodes()

        upstream_node = cluster.process(active_node.get_active_node())

        # post 2 PUTs
        self.producer.post(uriWrite, payload=["msg1"], succeed=True)
        self.producer.post(uriWrite, payload=["msg2"], succeed=True)

        # start consumer
        consumer = self.replica_proxy.create_client("consumer")

        consumer.open(uriRead, flags=["read"], succeed=True)

        # receive messages
        consumer.wait_push_event()
        assert wait_until(lambda: len(consumer.list(uriRead, block=True)) == 2, 2)

        # start graceful shutdown
        active_node.exit_gracefully()

        capture = self.replica_proxy.capture(
            r"waiting for 2 unconfirmed message", timeout=2
        )
        assert capture

        upstream_node.force_stop()

        consumer.confirm(uriRead, "*", succeed=True)

        capture = active_node.capture(
            r"Received control message: \[ rId = (-?\d+) choice = \[ clusterMessage = \[ choice = \[ stopResponse"
        )
        assert capture

    @tweak.cluster.queue_operations.stop_timeout_ms(999999)
    @tweak.cluster.queue_operations.shutdown_timeout_ms(5)
    def test_active_node_shutdown_timeout(self, multi_cluster: Cluster):
        """
        Test upstream timeout waiting for StopResponse
        We have: Consumer -> Proxy -> active_node -> upstream_node.
        Start shutting down active_node (one of cluster.virtual_nodes())
        Because there are unconfirmed, Proxy lingers with StopResponse.
        active_node times out and disconnects.  Proxy should reopen queue.
        """

        cluster = multi_cluster

        uriWrite = tc.URI_FANOUT
        uriRead = tc.URI_FANOUT_FOO

        active_node = cluster.process(self.replica_proxy.get_active_node())
        assert active_node in cluster.virtual_nodes()

        cluster.process(active_node.get_active_node())

        # post 2 PUTs
        self.producer.post(uriWrite, payload=["msg1"], succeed=True)
        self.producer.post(uriWrite, payload=["msg2"], succeed=True)

        # start consumer
        consumer = self.replica_proxy.create_client("consumer")

        consumer.open(uriRead, flags=["read"], succeed=True)

        # receive messages
        consumer.wait_push_event()
        assert wait_until(lambda: len(consumer.list(uriRead, block=True)) == 2, 2)

        # start graceful shutdown
        active_node.exit_gracefully()

        capture = self.replica_proxy.capture(
            r"Attempting to re-issue open-queue request", timeout=10
        )
        assert capture

        # post one more PUT
        self.producer.post(uriWrite, payload=["msg3"], succeed=True)

        # make sure the queue successfully reopens
        consumer.wait_push_event()
        assert wait_until(lambda: len(consumer.list(uriRead, block=True)) == 3, 10)
