import blazingmq.dev.it.testconstants as tc
import pytest

from blazingmq.dev.it.fixtures import (  # pylint: disable=unused-import
    Cluster,
    cluster,
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

class TestAutoSubscriptions:
    """
    This test verifies auto subscription for one or more substreams (apps)
    """
    def _start_client(self, broker, uri, name, subscriptions=[]):

        consumer = broker.create_client(name)
        assert (
            consumer.open(
                uri,
                flags=["read"],
                consumer_priority=1,
                subscriptions=subscriptions,
                block=True)
            == Client.e_SUCCESS
        )

        return consumer

    def _verify(self, domain, num):

        assert len(self.consumer.list(block=True)) == 0

        self.leader.list_messages(domain, tc.TEST_QUEUE, 0, 2)
        assert self.leader.outputs_substr(f"Printing {num} message(s)", 1)

    def _verify_fanout(self, domain, positiveApps, negativeAppIds, num):

        for app in positiveApps:
            self._verify_delivery(app, num)

        for appId in negativeAppIds:
            self.leader.list_messages(domain, tc.TEST_QUEUE, 0, 2, appId)
            assert self.leader.outputs_substr(f"Printing 0 message(s)", 1)

    def _verify_delivery(self, consumer, num):
        consumer.wait_push_event()
        msgs = consumer.list(block=True)
        assert len(msgs) == num
        assert msgs[0].payload == "123"

    @tweak.domain.subscriptions(
        [{"appId": "foo", "expression": {"version" : "E_VERSION_1", "text": "x==1"}},
         {"appId": "bar", "expression": {"version" : "E_VERSION_1", "text": "x==2"}}])
    def test_auto_subscription_fanout(self, cluster: Cluster):
        proxies = cluster.proxy_cycle()

        """
        Out of the 3 apps, configure one to evaluate auto subscription
        negatively, another to evaluate positively, and another do not
        configure.
        Make sure the first does not get the message, and the rest do.
        Make sure the same is the case after restarts.
        Make sure two CONFIRMs delete the message.
        """

        next(proxies)
        proxy = next(proxies)

        producer = proxy.create_client("producer")
        assert (
            producer.open(tc.URI_FANOUT_SC, flags=["write", "ack"], block=True)
            == Client.e_SUCCESS
        )

        self.consumer = self._start_client(proxy, tc.URI_FANOUT_SC_FOO, "consumerFoo")

        self.consumer_bar = self._start_client(proxy, tc.URI_FANOUT_SC_BAR, "consumerBar")
        self.consumer_baz = self._start_client(proxy, tc.URI_FANOUT_SC_BAZ, "consumerBaz")

        assert (
            producer.post(
                tc.URI_FANOUT_SC,
                payload=["123"],
                block=True,
                wait_ack=True,
                messageProperties=[{"name": "x", "value": "2", "type": "E_INT"}]
            )
            == Client.e_SUCCESS
        )

        self.leader = cluster.last_known_leader

        self._verify(tc.DOMAIN_FANOUT_SC, 1)
        self._verify_fanout(
            tc.DOMAIN_FANOUT_SC,
            [self.consumer_bar, self.consumer_baz],
            ["foo"],
            1)

        assert self.consumer.stop_session(block=True) == Client.e_SUCCESS
        assert self.consumer_bar.stop_session(block=True) == Client.e_SUCCESS
        assert self.consumer_baz.stop_session(block=True) == Client.e_SUCCESS

        self.consumer.exit_gracefully()
        self.consumer_bar.exit_gracefully()
        self.consumer_baz.exit_gracefully()

        cluster.restart_nodes()

        self.consumer = self._start_client(proxy, tc.URI_FANOUT_SC_FOO, "consumerFoo")

        self.consumer_bar = self._start_client(proxy, tc.URI_FANOUT_SC_BAR, "consumerBar")
        self.consumer_baz = self._start_client(proxy, tc.URI_FANOUT_SC_BAZ, "consumerBaz")

        self.leader = cluster.last_known_leader

        self._verify(tc.DOMAIN_FANOUT_SC, 1)
        self._verify_fanout(
            tc.DOMAIN_FANOUT_SC,
            [self.consumer_bar, self.consumer_baz],
            ["foo"],
            1)

        self.consumer_bar.confirm(tc.URI_FANOUT_SC_BAR, "*", succeed=True)
        self.consumer_baz.confirm(tc.URI_FANOUT_SC_BAZ, "*", succeed=True)

        self._verify(tc.DOMAIN_FANOUT_SC, 0)

        assert len(self.consumer_bar.list(block=True)) == 0
        assert len(self.consumer_baz.list(block=True)) == 0

    @tweak.domain.subscriptions([{"appId": "", "expression": {"version" : "E_VERSION_1", "text": "x==1"}}])
    def test_auto_subscription_priority(self, cluster: Cluster):
        """
        Configure the priority queue to evaluate auto subscription negatively.
        Make sure the queue does not get the message.
        Make sure the same is the case after restarts.
        """

        proxies = cluster.proxy_cycle()

        # 1: Setup producers and consumers

        next(proxies)
        proxy = next(proxies)

        producer = proxy.create_client("producer")
        assert (
            producer.open(tc.URI_PRIORITY_SC, flags=["write", "ack"], block=True)
            == Client.e_SUCCESS
        )

        self.consumer = self._start_client(proxy, tc.URI_PRIORITY_SC, "consumer")

        assert (
            producer.post(
                tc.URI_PRIORITY_SC,
                payload=["123"],
                block=True,
                wait_ack=True,
                messageProperties=[{"name": "x", "value": "2", "type": "E_INT"}]
            )
            == Client.e_SUCCESS
        )

        self.leader = cluster.last_known_leader

        self._verify(tc.DOMAIN_PRIORITY_SC, 0)

        assert self.consumer.stop_session(block=True) == Client.e_SUCCESS

        self.consumer.exit_gracefully()

        cluster.restart_nodes()

        self.consumer = self._start_client(proxy, tc.URI_PRIORITY_SC, "consumer")

        self.leader = cluster.last_known_leader

        self._verify(tc.DOMAIN_PRIORITY_SC, 0)

    @tweak.domain.subscriptions(
        [{"appId": "foo", "expression": {"version" : "E_VERSION_1", "text": "x==1"}},
         {"appId": "bar", "expression": {"version" : "E_VERSION_1", "text": "x > 2"}}])
    def test_auto_subscription_with_consumer_subscription(self, cluster: Cluster):
        """
        Out of the 3 apps, configure two to evaluate auto subscriptions.
        Configure consumsers with consumer subscriptions.
        Make sure outcome of message delivery is logical AND of both auto and
        consumer subscriptions.
        """

        proxies = cluster.proxy_cycle()

        # 1: Setup producers and consumers

        next(proxies)
        proxy = next(proxies)

        producer = proxy.create_client("producer")
        assert (
            producer.open(tc.URI_FANOUT_SC, flags=["write", "ack"], block=True)
            == Client.e_SUCCESS
        )

        self.consumer = self._start_client(
            proxy,
            tc.URI_FANOUT_SC_FOO,
            "consumerFoo",
            subscriptions=[{"correlationId": 1, "expression": "x == 2"}])

        self.consumer_bar = self._start_client(
            proxy,
            tc.URI_FANOUT_SC_BAR,
            "consumerBar",
            subscriptions=[{"correlationId": 1, "expression": "x > 3"}])
        self.consumer_baz = self._start_client(proxy, tc.URI_FANOUT_SC_BAZ, "consumerBaz")

        assert (
            producer.post(
                tc.URI_FANOUT_SC,
                payload=["123"],
                block=True,
                wait_ack=True,
                messageProperties=[{"name": "x", "value": "3", "type": "E_INT"}]
            )
            == Client.e_SUCCESS
        )

        self.leader = cluster.last_known_leader

        self._verify(tc.DOMAIN_FANOUT_SC, 1)

        self._verify_delivery(self.consumer_baz, 1)
        assert len(self.consumer_bar.list(block=True)) == 0

        assert (
            producer.post(
                tc.URI_FANOUT_SC,
                payload=["123"],
                block=True,
                wait_ack=True,
                messageProperties=[{"name": "x", "value": "4", "type": "E_INT"}]
            )
            == Client.e_SUCCESS
        )

        self._verify(tc.DOMAIN_FANOUT_SC, 2)
        self._verify_delivery(self.consumer_bar, 1)
        self._verify_delivery(self.consumer_baz, 2)

        assert self.consumer.stop_session(block=True) == Client.e_SUCCESS
        assert self.consumer_bar.stop_session(block=True) == Client.e_SUCCESS
        assert self.consumer_baz.stop_session(block=True) == Client.e_SUCCESS

        self.consumer.exit_gracefully()
        self.consumer_bar.exit_gracefully()
        self.consumer_baz.exit_gracefully()

        cluster.restart_nodes()

        self.consumer = self._start_client(
            proxy,
            tc.URI_FANOUT_SC_FOO,
            "consumerFoo",
            subscriptions=[{"correlationId": 1, "expression": "x == 2"}])

        self.consumer_bar = self._start_client(
            proxy,
            tc.URI_FANOUT_SC_BAR,
            "consumerBar",
            subscriptions=[{"correlationId": 1, "expression": "x > 2"}])
        self.consumer_baz = self._start_client(proxy, tc.URI_FANOUT_SC_BAZ, "consumerBaz")

        self.leader = cluster.last_known_leader

        self._verify(tc.DOMAIN_FANOUT_SC, 2)
        self._verify_delivery(self.consumer_bar, 2)
        self._verify_delivery(self.consumer_baz, 2)

        self.consumer_bar.confirm(tc.URI_FANOUT_SC_BAR, "*", succeed=True)
        self.consumer_baz.confirm(tc.URI_FANOUT_SC_BAZ, "*", succeed=True)

        self._verify(tc.DOMAIN_FANOUT_SC, 0)

        assert len(self.consumer_bar.list(block=True)) == 0
        assert len(self.consumer_baz.list(block=True)) == 0

    @tweak.domain.subscriptions([{"appId": "", "expression": {"version" : "E_VERSION_1", "text": "x==1"}}])
    def test_auto_subscription_broadcast(self, cluster: Cluster):
        """
        Configure the boadcast queue to evaluate auto subscription negatively.
        Make sure the queue does not get the message.
        """

        proxies = cluster.proxy_cycle()

        # 1: Setup producers and consumers

        next(proxies)
        proxy = next(proxies)

        producer = proxy.create_client("producer")
        assert (
            producer.open(tc.URI_BROADCAST, flags=["write", "ack"], block=True)
            == Client.e_SUCCESS
        )

        self.consumer = self._start_client(proxy, tc.URI_BROADCAST, "consumer")

        assert (
            producer.post(
                tc.URI_BROADCAST,
                payload=["123"],
                block=True,
                wait_ack=True,
                messageProperties=[{"name": "x", "value": "2", "type": "E_INT"}]
            )
            == Client.e_SUCCESS
        )

        self.leader = cluster.last_known_leader

        self._verify(tc.DOMAIN_BROADCAST, 0)

        assert self.consumer.stop_session(block=True) == Client.e_SUCCESS

        self.consumer.exit_gracefully()

        cluster.restart_nodes()

        self.consumer = self._start_client(proxy, tc.URI_BROADCAST, "consumer")

        self.leader = cluster.last_known_leader

        self._verify(tc.DOMAIN_BROADCAST, 0)

    @tweak.domain.subscriptions(
        [{"appId": "foo", "expression": {"version" : "E_VERSION_1", "text": "x==1"}},
         {"appId": "bar", "expression": {"version" : "E_VERSION_1", "text": "x==2"}},
         {"appId": "baz", "expression": {"version" : "E_VERSION_1", "text": "x==3"}}])
    def test_auto_subscription_fanout_all_negative(self, cluster: Cluster):
        """
        Configure all fanout Apps to evaluate auto subscriptions negatively.
        Make sure none receives a message.
        """
        proxies = cluster.proxy_cycle()

        # 1: Setup producers and consumers

        next(proxies)
        proxy = next(proxies)

        producer = proxy.create_client("producer")
        assert (
            producer.open(tc.URI_FANOUT_SC, flags=["write", "ack"], block=True)
            == Client.e_SUCCESS
        )

        self.consumer = self._start_client(proxy, tc.URI_FANOUT_SC_FOO, "consumerFoo")

        self.consumer_bar = self._start_client(proxy, tc.URI_FANOUT_SC_BAR, "consumerBar")
        self.consumer_baz = self._start_client(proxy, tc.URI_FANOUT_SC_BAZ, "consumerBaz")

        assert (
            producer.post(
                tc.URI_FANOUT_SC,
                payload=["123"],
                block=True,
                wait_ack=True,
                messageProperties=[{"name": "x", "value": "0", "type": "E_INT"}]
            )
            == Client.e_SUCCESS
        )

        self.leader = cluster.last_known_leader

        self._verify(tc.DOMAIN_FANOUT_SC, 0)
        self._verify_fanout(
            tc.DOMAIN_FANOUT_SC,
            [],
            ["foo", "bar", "baz"],
            0)

        assert len(self.consumer.list(block=True)) == 0
        assert len(self.consumer_bar.list(block=True)) == 0
        assert len(self.consumer_baz.list(block=True)) == 0

