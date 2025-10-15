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

from typing import Any, List, Optional

import blazingmq.dev.it.testconstants as tc

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
from blazingmq.dev.it.process.broker import Broker
from blazingmq.dev.it.process.client import Client


class TestAppSubscriptions:
    """
    This test verifies application subscription for one or more substreams
    (apps)
    """

    @staticmethod
    def _start_client(
        broker: Broker, uri: str, name: str, subscriptions: Optional[List[Any]] = None
    ) -> Client:
        consumer = broker.create_client(name)
        assert (
            consumer.open(
                uri,
                flags=["read"],
                consumer_priority=1,
                subscriptions=subscriptions,
                block=True,
            )
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
        [
            {"appId": "foo", "expression": {"version": "E_VERSION_1", "text": "x==1"}},
            {"appId": "bar", "expression": {"version": "E_VERSION_1", "text": "x==2"}},
        ]
    )
    def test_app_subscription_fanout(
        self, cluster: Cluster, domain_urls: tc.DomainUrls
    ):
        du = domain_urls
        proxies = cluster.proxy_cycle()

        """
        Out of the 3 apps, configure one to evaluate application subscription
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
            producer.open(du.uri_fanout, flags=["write", "ack"], block=True)
            == Client.e_SUCCESS
        )

        self.consumer = self._start_client(proxy, du.uri_fanout_foo, "consumerFoo")

        self.consumer_bar = self._start_client(proxy, du.uri_fanout_bar, "consumerBar")
        self.consumer_baz = self._start_client(proxy, du.uri_fanout_baz, "consumerBaz")

        assert (
            producer.post(
                du.uri_fanout,
                payload=["123"],
                block=True,
                wait_ack=True,
                messageProperties=[{"name": "x", "value": "2", "type": "E_INT"}],
            )
            == Client.e_SUCCESS
        )

        self.leader = cluster.last_known_leader

        self._verify(du.domain_fanout, 1)
        self._verify_fanout(
            du.domain_fanout, [self.consumer_bar, self.consumer_baz], ["foo"], 1
        )

        assert self.consumer.stop_session(block=True) == Client.e_SUCCESS
        assert self.consumer_bar.stop_session(block=True) == Client.e_SUCCESS
        assert self.consumer_baz.stop_session(block=True) == Client.e_SUCCESS

        self.consumer.exit_gracefully()
        self.consumer_bar.exit_gracefully()
        self.consumer_baz.exit_gracefully()

        cluster.restart_nodes()

        self.consumer = self._start_client(proxy, du.uri_fanout_foo, "consumerFoo")

        self.consumer_bar = self._start_client(proxy, du.uri_fanout_bar, "consumerBar")
        self.consumer_baz = self._start_client(proxy, du.uri_fanout_baz, "consumerBaz")

        self.leader = cluster.last_known_leader

        self._verify(du.domain_fanout, 1)
        self._verify_fanout(
            du.domain_fanout, [self.consumer_bar, self.consumer_baz], ["foo"], 1
        )

        self.consumer_bar.confirm(du.uri_fanout_bar, "*", succeed=True)
        self.consumer_baz.confirm(du.uri_fanout_baz, "*", succeed=True)

        self._verify(du.domain_fanout, 0)

        assert len(self.consumer_bar.list(block=True)) == 0
        assert len(self.consumer_baz.list(block=True)) == 0

    @tweak.domain.subscriptions(
        [{"appId": "", "expression": {"version": "E_VERSION_1", "text": "x==1"}}]
    )
    def test_app_subscription_priority(
        self, cluster: Cluster, domain_urls: tc.DomainUrls
    ):
        """
        Configure the priority queue to evaluate application subscription
        negatively.
        Make sure the queue does not get the message.
        Make sure the same is the case after restarts.
        """
        du = domain_urls

        proxies = cluster.proxy_cycle()

        # 1: Setup producers and consumers

        next(proxies)
        proxy = next(proxies)

        producer = proxy.create_client("producer")
        assert (
            producer.open(du.uri_priority, flags=["write", "ack"], block=True)
            == Client.e_SUCCESS
        )

        self.consumer = self._start_client(proxy, du.uri_priority, "consumer")

        assert (
            producer.post(
                du.uri_priority,
                payload=["123"],
                block=True,
                wait_ack=True,
                messageProperties=[{"name": "x", "value": "2", "type": "E_INT"}],
            )
            == Client.e_SUCCESS
        )

        self.leader = cluster.last_known_leader

        self._verify(du.domain_priority, 0)

        assert self.consumer.stop_session(block=True) == Client.e_SUCCESS

        self.consumer.exit_gracefully()

        cluster.restart_nodes()

        self.consumer = self._start_client(proxy, du.uri_priority, "consumer")

        self.leader = cluster.last_known_leader

        self._verify(du.domain_priority, 0)

    @tweak.domain.subscriptions(
        [
            {"appId": "foo", "expression": {"version": "E_VERSION_1", "text": "x==1"}},
            {"appId": "bar", "expression": {"version": "E_VERSION_1", "text": "x > 2"}},
        ]
    )
    def test_app_subscription_with_consumer_subscription(
        self, cluster: Cluster, domain_urls: tc.DomainUrls
    ):
        """
        Out of the 3 apps, configure two to evaluate application subscriptions.
        Configure consumsers with consumer subscriptions.
        Make sure outcome of message delivery is logical AND of both
        application and consumer subscriptions.
        """
        du = domain_urls

        proxies = cluster.proxy_cycle()

        # 1: Setup producers and consumers

        next(proxies)
        proxy = next(proxies)

        producer = proxy.create_client("producer")
        assert (
            producer.open(du.uri_fanout, flags=["write", "ack"], block=True)
            == Client.e_SUCCESS
        )

        self.consumer = self._start_client(
            proxy,
            du.uri_fanout_foo,
            "consumerFoo",
            subscriptions=[{"correlationId": 1, "expression": "x == 2"}],
        )

        self.consumer_bar = self._start_client(
            proxy,
            du.uri_fanout_bar,
            "consumerBar",
            subscriptions=[{"correlationId": 1, "expression": "x > 3"}],
        )
        self.consumer_baz = self._start_client(proxy, du.uri_fanout_baz, "consumerBaz")

        assert (
            producer.post(
                du.uri_fanout,
                payload=["123"],
                block=True,
                wait_ack=True,
                messageProperties=[{"name": "x", "value": "3", "type": "E_INT"}],
            )
            == Client.e_SUCCESS
        )

        self.leader = cluster.last_known_leader

        self._verify(du.domain_fanout, 1)

        self._verify_delivery(self.consumer_baz, 1)
        assert len(self.consumer_bar.list(block=True)) == 0

        assert (
            producer.post(
                du.uri_fanout,
                payload=["123"],
                block=True,
                wait_ack=True,
                messageProperties=[{"name": "x", "value": "4", "type": "E_INT"}],
            )
            == Client.e_SUCCESS
        )

        self._verify(du.domain_fanout, 2)
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
            du.uri_fanout_foo,
            "consumerFoo",
            subscriptions=[{"correlationId": 1, "expression": "x == 2"}],
        )

        self.consumer_bar = self._start_client(
            proxy,
            du.uri_fanout_bar,
            "consumerBar",
            subscriptions=[{"correlationId": 1, "expression": "x > 2"}],
        )
        self.consumer_baz = self._start_client(proxy, du.uri_fanout_baz, "consumerBaz")

        self.leader = cluster.last_known_leader

        self._verify(du.domain_fanout, 2)
        self._verify_delivery(self.consumer_bar, 2)
        self._verify_delivery(self.consumer_baz, 2)

        self.consumer_bar.confirm(du.uri_fanout_bar, "*", succeed=True)
        self.consumer_baz.confirm(du.uri_fanout_baz, "*", succeed=True)

        self._verify(du.domain_fanout, 0)

        assert len(self.consumer_bar.list(block=True)) == 0
        assert len(self.consumer_baz.list(block=True)) == 0

    @tweak.domain.subscriptions(
        [{"appId": "", "expression": {"version": "E_VERSION_1", "text": "x==1"}}]
    )
    def test_app_subscription_broadcast(
        self,
        cluster: Cluster,
        domain_urls: tc.DomainUrls,  # pylint: disable=unused-argument
    ):
        """
        Configure the boadcast queue to evaluate application subscription
        negatively.
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
                messageProperties=[{"name": "x", "value": "2", "type": "E_INT"}],
            )
            == Client.e_SUCCESS
        )

        self.leader = cluster.last_known_leader

        self._verify(tc.DOMAIN_BROADCAST, 0)

        assert self.consumer.stop_session(block=True) == Client.e_SUCCESS

    @tweak.domain.subscriptions(
        [
            {"appId": "foo", "expression": {"version": "E_VERSION_1", "text": "x==1"}},
            {"appId": "bar", "expression": {"version": "E_VERSION_1", "text": "x==2"}},
            {"appId": "baz", "expression": {"version": "E_VERSION_1", "text": "x==3"}},
        ]
    )
    def test_app_subscription_fanout_all_negative(
        self, cluster: Cluster, domain_urls: tc.DomainUrls
    ):
        """
        Configure all fanout Apps to evaluate application subscriptions
        negatively.
        Make sure none receives a message.
        """
        du = domain_urls
        proxies = cluster.proxy_cycle()

        # 1: Setup producers and consumers

        next(proxies)
        proxy = next(proxies)

        producer = proxy.create_client("producer")
        assert (
            producer.open(du.uri_fanout, flags=["write", "ack"], block=True)
            == Client.e_SUCCESS
        )

        self.consumer = self._start_client(proxy, du.uri_fanout_foo, "consumerFoo")

        self.consumer_bar = self._start_client(proxy, du.uri_fanout_bar, "consumerBar")
        self.consumer_baz = self._start_client(proxy, du.uri_fanout_baz, "consumerBaz")

        assert (
            producer.post(
                du.uri_fanout,
                payload=["123"],
                block=True,
                wait_ack=True,
                messageProperties=[{"name": "x", "value": "0", "type": "E_INT"}],
            )
            == Client.e_SUCCESS
        )

        self.leader = cluster.last_known_leader

        self._verify(du.domain_fanout, 0)
        self._verify_fanout(du.domain_fanout, [], ["foo", "bar", "baz"], 0)

        assert len(self.consumer.list(block=True)) == 0
        assert len(self.consumer_bar.list(block=True)) == 0
        assert len(self.consumer_baz.list(block=True)) == 0

    @tweak.domain.subscriptions(
        [
            {
                "appId": "",
                "expression": {"version": "E_VERSION_1", "text": "invalid expression"},
            }
        ]
    )
    def test_invalid_configuration(self, cluster: Cluster, domain_urls: tc.DomainUrls):
        """
        Configure priority domain with invalid application subscription.
        Make sure a queue fails to open.
        Reconfigure the domain with valid application subscription.
        Make sure a queue opens successfully.
        Reconfigure the domain with invalid application subscription.
        Make sure the reconfigure command fails.
        Make sure a queue opens successfully.
        """
        du = domain_urls

        proxies = cluster.proxy_cycle()

        # 1: Setup producers and consumers

        next(proxies)
        proxy = next(proxies)

        consumer = proxy.create_client("consumer")

        consumer.open(
            du.uri_priority,
            flags=["read"],
            consumer_priority=1,
            succeed=False,
        )

        cluster.config.domains[du.domain_priority].definition.parameters.subscriptions[
            0
        ]["expression"]["text"] = "x==1"

        cluster.reconfigure_domain(du.domain_priority, succeed=True)

        consumer.open(
            du.uri_priority,
            flags=["read"],
            consumer_priority=1,
            succeed=True,
        )

        consumer.close(
            du.uri_priority,
            succeed=True,
        )

        cluster.config.domains[du.domain_priority].definition.parameters.subscriptions[
            0
        ]["expression"]["text"] = "invalid expression"

        cluster.reconfigure_domain(du.domain_priority, succeed=None)
        assert cluster.last_known_leader.capture("Error processing command")

        # The validation fails and the domain is going to keep the old config
        consumer.open(
            du.uri_priority,
            flags=["read"],
            consumer_priority=1,
            succeed=True,
        )
