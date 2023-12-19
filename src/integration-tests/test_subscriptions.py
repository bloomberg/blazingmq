"""
Tests for subscriptions.

This module consists of these components:
- Producer: high level wrapper for Client that is opened in 'write' mode.
- Consumer: high level wrapper for Client that is opened in 'read' mode.
- TestSubscriptions: the main test class.

Note: Producer and Consumer implementations hide common boilerplate code.  It
makes tests code simpler and more extendable.  That is the main reason why these
classes exist.

Existing tests:
- test_second_configure
- test_open
- test_non_blocking
- test_configure_subscription
- test_reconfigure_subscription
- test_non_overlapping
- test_priorities
- test_fanout
- test_round_robin
- test_redelivery
- test_max_unconfirmed
- test_max_unconfirmed_low_priority_spillover
- test_many_subscriptions
- test_complex_expressions
- test_incorrect_expressions
- test_non_bool_expressions
- test_numeric_limits
- test_empty_subscription
- test_empty_expression
- test_non_existent_property
- test_date_time
- test_consumer_multiple_priorities
- test_no_capacity_all_optimization
- test_no_capacity_all_fanout
- test_primary_node_crash
- test_redelivery_on_primary_node_crash
- test_reconfigure_on_primary_node_crash
- test_poison

Not implemented:
- test_allowed_operands
    Expression validation is on evaluator side for now.
- test_restricted_operands:
    Expression validation is on evaluator side for now.
"""
import re
import time
from datetime import datetime
from typing import Any, Dict, List, Optional, Tuple, Union

import bmq.dev.it.testconstants as tc
import pytest
from bmq.dev.it.fixtures import (  # pylint: disable=unused-import
    Cluster,
    cluster,
    logger,
    standard_cluster,
    tweak,
)
from bmq.dev.it.process.broker import Broker
from bmq.dev.it.process.client import Client, ITError, Message
from bmq.dev.it.util import wait_until
from bmq.schemas import mqbcfg

EMPTY_SUBSCRIPTION = []
EPS = 1e-6

Subscription = Union[str, Tuple[str, int]]
SubscriptionList = List[Subscription]


class DeliveryOptimizationMonitor:
    """DeliveryOptimizationMonitor is a class which scans the primary node's
    logs and collects log records related to the delivery optimization.  This
    class exists because log records for different app_ids could be mixed in
    different order, so we want to memorize it and ensure that no records
    lost."""

    def __init__(self, cluster: Cluster):
        """
        Construct a DeliveryOptimizationMonitor with the specified 'cluster'.
        """
        self._cluster: Cluster = cluster
        self._app_id_to_messages: Dict[str, List[re.Match]] = {}

    def _try_find_message_in_log(
        self, app_id: Optional[str] = None
    ) -> Optional[re.Match]:
        """
        Scan primary node's log until delivery optimization message with the
        optionally specified 'app_id' is found.  'None' value of 'app_id' means
        that any delivery optimization message is good.  Return the found
        're.Match' object if found or 'None' value.
        """
        leader = self._cluster.last_known_leader
        assert leader is not None

        early_exit_message = r"appId = \'(\w+)\' does not have any subscription capacity; early exits delivery"
        capture_timeout = 2

        while True:
            match = leader.capture(early_exit_message, timeout=capture_timeout)
            if match is None:
                return None

            msg_app_id = match.group(1)
            if app_id is None or msg_app_id == app_id:
                return match

            self._app_id_to_messages.setdefault(msg_app_id, []).append(match)

    def has_new_message(self, app_id: Optional[str] = None) -> bool:
        """
        Try to find next delivery optimization message for the optionally
        specified 'app_id' in primary node's logs or within the cached messages.
        'None' value of 'app_id' means that any delivery optimization message is
        good.  Return 'True' if message is found, and 'False' otherwise.
        """

        if (
            app_id not in self._app_id_to_messages
            or len(self._app_id_to_messages[app_id]) == 0
        ):
            return self._try_find_message_in_log(app_id) is not None

        del self._app_id_to_messages[app_id][0]
        return True


class Producer:
    """Producer is a high-level wrapper over bmq client opened in write mode."""

    _next_id: int = 1

    def __init__(self, cluster: Cluster, uri: str, make_auto_properties: bool = True):
        """
        Construct the producer in the specified 'cluster' and open 'uri' for
        write.  The specified 'make_auto_properties' flag enables automatic
        message properties extension with extra properties for each posted
        message.
        """

        self._name = f"producer_{Producer._next_id}"
        Producer._next_id += 1

        proxy: Broker = next(cluster.proxy_cycle())
        self._client: Client = proxy.create_client(self._name)

        self._posted_count = 0

        self._uri = uri
        self._make_auto_properties = make_auto_properties
        self._client.open(uri, flags=["write,ack"], succeed=True)

    @classmethod
    def _convert_properties(cls, **kwargs) -> List[Dict[str, str]]:
        """
        Convert the specified 'kwargs' key/values to properties list and return
        it.

        Example:
        __build_properties(x = -10, y = "sample")
        -> [{"name": "x", "value": "-10",    "type": "E_INT"},
            {"name": "y", "value": "sample", "type": "E_STRING"}]
        """

        res = []

        type_mapping = {str: "E_STRING", int: "E_INT"}
        for name, value in kwargs.items():
            assert isinstance(name, str)
            assert type(value) in type_mapping
            res.append(
                {
                    "name": name,
                    "value": str(value),
                    "type": type_mapping[type(value)],
                }
            )

        return res

    @property
    def _auto_properties(self) -> Dict[str, Union[int, str]]:
        """
        Return automatically generated meta properties for the next message.
        These properties are valuable, because we add some extra information to
        posted messages, so these are not too trivial.
        """
        return {
            "producer": self._name,
            "posted_count": self._posted_count,
            "sample_int": 678,
            "sample_str": "foo",
        }

    def post(self, **kwargs) -> str:
        """
        Post the message with properties built from the specified 'kwargs'.

        Return the payload of the posted message.

        Note: payload is formed from the provided 'kwargs', which is convenient
        for sanity checks and message delivery verification on consumer side.

        Example:
            producer.post(x=10, y="test") -> "x: 10, y: test"
        """

        # It is very difficult to check exact payloads in various test cases if
        # we include automatic properties to payload.  So instead we just
        # generate payload using only the explicitly specified arguments.
        payload = ", ".join(f"{key}: {value}" for key, value in kwargs.items())

        # Automatic properties make messages less trivial.
        properties = kwargs
        if self._make_auto_properties:
            properties.update(self._auto_properties)

        assert (
            self._client.post(
                self._uri,
                payload=[payload],
                wait_ack=True,
                block=True,
                succeed=True,
                messageProperties=self._convert_properties(**properties),
            )
            == Client.e_SUCCESS
        )
        self._posted_count += 1
        return payload

    def post_same(self, num: int, value: int) -> List[str]:
        """
        Post the specified 'num' of messages with the specified 'value' as
        property "x".  Return the list of expected payloads of posted messages.

        Note: it is convenient to post messages with identical payloads and
        properties for some tests because we don't know the exact order of
        round-robin for consumers.  If messages are seemingly equal the order
        doesn't matter.

        Example:
            post_same(num=3, value=1) -> ["x: 1", "x: 1", "x: 1"]
        """
        return [self.post(x=value) for _ in range(num)]

    def post_diff(self, num: int, offset: int = 0) -> List[str]:
        """
        Post the specified 'num' of messages with property "x" from range
        [offset, offset + num) using the specified 'offset'.
        Return the list of expected payloads of posted messages.

        Note: 'offset' is useful when we post several different groups of
        messages, so it is easy to distinguish these after delivery.

        Example:
            post_diff(num=3, offset=100) -> ["x: 100", "x: 101", "x: 102"]
        """
        return [self.post(x=offset + i) for i in range(num)]


class Consumer:
    """Consumer is a high-level wrapper over bmq client opened in read mode."""

    _next_id: int = 1
    _correlation_id: int = 1

    def __init__(
        self,
        cluster: Cluster,
        uri: str,
        subscriptions: Optional[SubscriptionList] = None,
        max_unconfirmed_messages: int = 4096,
        consumer_priority: int = 0,
        timeout: Optional[int] = None,
    ):
        """
        Construct the consumer in the specified 'cluster' and open 'uri' for
        read.

        The optionally specified 'subscriptions' list represents subscription
        parameters for open command.  The specified 'max_unconfirmed_messages'
        and 'consumer_priority' represent corresponding parameters for open command.

        The optionally specified 'timeout' is a maximum waiting time (in seconds)
        for open command to succeed, if it's not specified, the default timeout
        is used.
        """
        self._proxy: Broker = next(cluster.proxy_cycle())
        self._client: Client = self._proxy.create_client(
            f"consumer_{Consumer._next_id}"
        )
        Consumer._next_id += 1

        self._uri = uri
        self._subscriptions = self._build_subscriptions(subscriptions or [])
        self._max_unconfirmed_messages = max_unconfirmed_messages
        self._consumer_priority = consumer_priority
        self._client.open(
            uri,
            flags=["read"],
            succeed=True,
            max_unconfirmed_messages=max_unconfirmed_messages,
            consumer_priority=consumer_priority,
            subscriptions=self._subscriptions,
            timeout=timeout,
        )

    def configure(
        self,
        subscriptions: Optional[SubscriptionList] = None,
        max_unconfirmed_messages: int = 4096,
        consumer_priority: int = 0,
        timeout: Optional[int] = None,
    ) -> None:
        """
        Configure the consumer using the optionally specified 'subscriptions'
        list represents subscription parameters for configure command.
        The specified 'max_unconfirmed_messages' and 'consumer_priority'
        represent corresponding parameters for configure command.

        The optionally specified 'timeout' is a maximum waiting time (in seconds)
        for command to succeed, if it's not specified, the default timeout is used.
        """
        self._subscriptions = self._build_subscriptions(subscriptions or [])
        self._max_unconfirmed_messages = max_unconfirmed_messages
        self._consumer_priority = consumer_priority
        self._client.configure(
            self._uri,
            succeed=True,
            subscriptions=self._subscriptions,
            max_unconfirmed_messages=max_unconfirmed_messages,
            consumer_priority=consumer_priority,
            timeout=timeout,
        )

    def close(self) -> None:
        """
        Close the opened queue.
        """
        self._client.close(self._uri, succeed=True)

    def kill(self) -> None:
        """
        Unconditionally kill the consumer client.
        """
        self._client.check_exit_code = False
        self._client.kill()

    def wait_for_messages(
        self, num_messages: int, timeout: Optional[float] = 2
    ) -> List[Message]:
        """
        Wait for at least the specified 'num_messages' with the specified
        'timeout' in seconds.  Non-positive or None 'timestamp' means that no
        wait is needed, so the messages must be received as-is.
        """
        if timeout is not None and timeout > EPS:
            start_time = time.time()
            self._client.wait_push_event(timeout=timeout)
            remaining_time = start_time + timeout - time.time()
            wait_until(
                lambda: len(self._client.list(self._uri, block=True)) >= num_messages,
                remaining_time,
            )
        return self._client.list(self._uri, block=True)

    def expect_messages(
        self, expected: List[str], confirm: bool, timeout: Optional[float] = 2
    ) -> None:
        """
        Wait for the specified 'expected' message payloads and ensure fail if
        observed message payloads are not the same as expected.  Confirm all
        unconfirmed messages after check if the specified 'confirm' flag is True.
        """
        msgs = self.wait_for_messages(len(expected), timeout)
        payloads = [message.payload for message in msgs]
        assert payloads == expected

        if confirm:
            self.confirm_all()

    def expect_empty(self, timeout: float = 0.1) -> None:
        """
        Wait for the specified 'timeout' in seconds and ensure fail if non-zero
        number of messages observed.
        """
        self.expect_messages([], confirm=False, timeout=timeout)

    def confirm_all(self):
        """
        Confirm all unconfirmed messages.
        """
        self._client.confirm(self._uri, "*", succeed=True)

        # The bmqtool does not actually wait and check that confirm command was
        # completed: 'success' is returned just when the command is enqueued
        # for send in bmqtool.
        # We need a strong guarantee that confirm was executed on the primary
        # node of the cluster. For that reason we open the same queue for write
        # and close it right away.

        # remove 'id' if present:
        uri = (self._uri.split("?"))[0]
        producer: Client = self._proxy.create_client("producer_confirm")
        producer.open(uri, flags=["write,ack"], succeed=True)
        producer.close(uri, succeed=True)

    @classmethod
    def _build_subscriptions(
        cls, subscriptions: SubscriptionList
    ) -> List[Dict[str, Any]]:
        """
        Convert the specified 'subscriptions' to the bmq expected format.
        """
        if len(subscriptions) == 0 or subscriptions == [{}]:
            # Contract: empty {} in subscription json means empty subscription
            # for backward compatibility
            cls._correlation_id += 1
            return [{}]

        res = []
        for subscription in subscriptions:
            if isinstance(subscription, str):
                desc = {
                    "correlationId": cls._correlation_id,
                    "expression": subscription,
                }
            else:
                assert isinstance(subscription, tuple)
                desc = {
                    "correlationId": cls._correlation_id,
                    "expression": subscription[0],
                    "consumerPriority": subscription[1],
                }

            res.append(desc)
            cls._correlation_id += 1
        return res


class TestSubscriptions:
    def test_second_configure(self, cluster: Cluster):
        """
        Test: a simple scenario where a crash might occur.
        - Create 1 producer / 1 consumer.
        - Open 1 priority queue.
        - Configure the only consumer.
        - Produce 1 simple message to the queue.
        - Receive the message and confirm it by consumer.
        - Configure the only consumer again.
        All these steps do not include any specified subscription or message
        properties.

        Concerns:
        - This scenario failed on the early development stage.
        """
        proxy = next(cluster.proxy_cycle())
        producer = proxy.create_client("producer")
        consumer = proxy.create_client("consumer")

        uri = tc.URI_PRIORITY
        producer.open(uri, flags=["write,ack"], succeed=True)
        consumer.open(uri, flags=["read"], succeed=True)

        consumer.configure(uri, block=True)

        assert (
            producer.post(uri, payload=["payload"], wait_ack=True) == Client.e_SUCCESS
        )
        consumer.wait_push_event()
        assert consumer.confirm(uri, "*", block=True) == Client.e_SUCCESS

        consumer.configure(uri, block=True)

    def test_open(self, cluster: Cluster):
        """
        Test: open queue with the specified subscription.
        - Create 1 producer / 1 consumer.
        - Open 1 priority queue, with subscription parameters for the consumer.
        - Producer: produce messages M1-M3 with properties.
        - Consumer: expect and confirm only M1-M2 in correct order.

        Concerns:
        - Subscription expression must be passed with 'open' correctly.
        - Only the messages expected by subscription must be received.
        """
        uri = tc.URI_PRIORITY
        producer = Producer(cluster, uri)
        consumer = Consumer(cluster, uri, ["x <= 1"])

        producer.post(x=0)
        producer.post(x=1)
        producer.post(x=2)

        consumer.expect_messages(["x: 0", "x: 1"], confirm=True)

    def test_non_blocking(self, cluster: Cluster):
        """
        Test: messages that must not be received by the given consumer do not
        block it to receive the following messages.
        - Create 1 producer / 1 consumer: C1.
        - Open 1 priority queue, with subscription parameters for the consumer C1.
        - Producer: produce 3 messages M1-M3 with properties.
        - Consumer C1: expect and confirm only messages M2-M3 (M1 is not
        suitable for subscription).
        - Create another consumer C2 and open the same priority queue, with
        subscription expression that is always true.
        - Consumer C2: expect and confirm the only remaining message M1.

        Producer    -> M1, M2, M3
        Consumer C1 <-     M2, M3
          M1 is not routed to C1 because it is not suitable for its subscription
        Consumer C2 <- M1
          the remaining M1 is routed to C2

        Concerns:
        - The first message M1 must not block consumer C1 to receive messages
        M2, M3.
        - Consumer C2 must receive the skipped message M1.
        - Consumer C2 must not receive messages M2, M3 that were confirmed.
        """
        uri = tc.URI_PRIORITY
        producer = Producer(cluster, uri)
        consumer1 = Consumer(cluster, uri, ["x >= 1"])

        producer.post(x=0)
        producer.post(x=1)
        producer.post(x=2)

        consumer1.expect_messages(["x: 1", "x: 2"], confirm=True)

        consumer2 = Consumer(
            cluster,
            uri,
            ["x >= 0 || x < 0"],
        )
        consumer2.expect_messages(["x: 0"], confirm=True)

    def test_configure_subscription(self, cluster: Cluster):
        """
        Test: configure queue with the specified subscription.
        - Create 1 producer / 1 consumer.
        - Open 1 priority queue, without subscription parameters.
        - Configure queue for consumer with subscription parameters.
        - Producer: produce messages M1-M3 with properties.
        - Consumer: expect and confirm only M1-M2 in correct order.

        Concerns:
        - Subscription expression must be passed with 'configure' correctly.
        - Only the messages expected by subscription must be received.
        """
        uri = tc.URI_PRIORITY
        producer = Producer(cluster, uri)
        consumer = Consumer(cluster, uri)

        consumer.configure(["x <= 1"])

        producer.post(x=0)
        producer.post(x=1)
        producer.post(x=2)

        consumer.expect_messages(["x: 0", "x: 1"], confirm=True)

    def test_reconfigure_subscription(self, cluster: Cluster):
        """
        Test: reconfigure queue with the specified subscription.
        - Create 1 producer / 1 consumer.
        - Open 1 priority queue, with subscription parameters for the consumer.
        - Produce 6 messages with properties to the queue: M1-M6.
        - Consumer: expect and confirm only the last 2 messages M5-M6.

        - Consumer: configure queue with another subscription parameters.
        - Consumer: expect and confirm the current last 2 messages M3-M4.

        - Consumer: configure queue with another subscription parameters.
        - Consumer: expect and confirm the remaining last 2 messages M1-M2.

        Producer  -> [M1, M2, M3, M4, M5, M6]
        Consumer
             open <-                 [M5, M6]
        configure <-         [M3, M4]
        configure <- [M1, M2]

        Concerns:
        - Subscription expressions are replaced correctly each time with
        'configure'.
        - Only the messages expected by each subscription must be received.
        - Confirmed messages are not being received again.
        - Valid substreams: the order of messages is the same as it was during
        the 'post' for each subscription.
        """
        uri = tc.URI_PRIORITY
        producer = Producer(cluster, uri)
        consumer = Consumer(cluster, uri, ["x >= 4"])

        producer.post(x=0)
        producer.post(x=1)

        producer.post(x=2)
        m3 = producer.post(x=3)  # pylint: disable=unused-variable

        producer.post(x=4)
        producer.post(x=5)

        consumer.expect_messages(["x: 4", "x: 5"], confirm=True)

        consumer.configure(["x >= 2"])
        consumer.expect_messages(["x: 2", "x: 3"], confirm=True)

        consumer.configure(["x >= 0"])
        consumer.expect_messages(["x: 0", "x: 1"], confirm=True)

    def test_non_overlapping(self, cluster: Cluster):
        """
        Test: several non-overlapping subscriptions to the same queue.
        - Create 1 producer / 3 consumers.
        - Open 1 priority queue, each consumer has its own subscription
        expression, these expressions are strictly non-overlapping.
        - Produce N messages with properties to the queue.
        - Expect and confirm only the messages that are good for the given
        subscription expression for each consumer.

        Concerns:
        - Routing of messages works correctly for multiple consumers with
        subscriptions.
        """
        uri = tc.URI_PRIORITY
        producer = Producer(cluster, uri)
        consumer1 = Consumer(cluster, uri, ["x > 0"])
        consumer2 = Consumer(cluster, uri, ["x < 0"])
        consumer3 = Consumer(cluster, uri, ["x == 0"])

        values = [-1, 9, 0, 5, 678, 54, 17, -9, 0, -4, 3, -2, 10, 2, 11, -5]
        expected = [producer.post(x=num) for num in values]

        expected1 = [payload for num, payload in zip(values, expected) if num > 0]
        expected2 = [payload for num, payload in zip(values, expected) if num < 0]
        expected3 = [payload for num, payload in zip(values, expected) if num == 0]

        consumer1.expect_messages(expected1, confirm=True)
        consumer2.expect_messages(expected2, confirm=True)
        consumer3.expect_messages(expected3, confirm=True)

    def test_priorities(self, cluster: Cluster):
        """
        Test: several subscriptions to the same queue with different priorities.
        - Create 1 producer / 2 consumers: C1, C2.
        - Open 1 priority queue, the subscription expression is the same for
        each consumer but priorities are different.
        - Produce N messages with properties to the queue.

        - Expect and confirm only the messages that are suitable for the given
        subscription expression for each consumer.

        Concerns:
        - Routing of messages works correctly for multiple consumers with
        subscriptions.
        """
        uri = tc.URI_PRIORITY
        producer = Producer(cluster, uri)
        consumer1 = Consumer(cluster, uri)
        consumer2 = Consumer(cluster, uri)

        consumer1.configure(
            [("x >= 0", 10)],
        )
        consumer2.configure(
            [("x >= 0", 5)],
        )

        expected = producer.post_diff(num=5, offset=0)
        consumer1.expect_messages(expected, confirm=True)

        consumer1.configure(
            [("x >= 0", 1)],
        )

        expected = producer.post_diff(num=5, offset=100)
        consumer2.expect_messages(expected, confirm=True)

    def test_fanout(self, cluster: Cluster):
        """
        Test: subscriptions work in fanout mode.
        - Create 1 producer / 3 consumers.
        - Open 1 fanout queue, the subscription expression is the same for
        each consumer but app uris are different.
        - Produce N messages with properties to the queue.
        - Expect all N messages for each consumer and confirm these.

        Concerns:
        - Fanout works for subscriptions: every message delivered for each
        consumer.
        """
        producer_uri = tc.URI_FANOUT
        app_uris = [tc.URI_FANOUT_FOO, tc.URI_FANOUT_BAR, tc.URI_FANOUT_BAZ]

        producer = Producer(cluster, producer_uri)
        consumers = [
            Consumer(
                cluster,
                uri,
                ["x >= 0"],
            )
            for uri in app_uris
        ]

        expected = producer.post_diff(num=5)
        for consumer in consumers:
            consumer.expect_messages(expected, confirm=True)

    def test_round_robin(self, cluster: Cluster):
        """
        Test: round-robin routing of messages with subscriptions.
        - Create 1 producer / K consumers.
        - Open 1 priority queue, the subscription expression is the same for
        each consumer.
        - Produce K*N messages with properties to the queue.
        - Expect and confirm strictly N messages for each consumer.

        Concerns:
        - Round-robin works for subscriptions: messages are evenly routed
        between consumers.
        """
        uri = tc.URI_PRIORITY
        num_consumers = 10
        messages_per_consumer = 15

        producer = Producer(cluster, uri)
        consumers = [
            Consumer(
                cluster,
                uri,
                ["x >= 0"],
            )
            for _ in range(num_consumers)
        ]

        # Note: we don't know the exact order of consumers for round-robin,
        # this could change from launch to launch.  Because of that it's not
        # trivial to expect concrete messages for each consumer if all messages
        # are different.  To deal with this we generate messages in groups of
        # size K, messages in one group have the same properties and payloads.
        # So it doesn't matter in what order it will be routed with round-robin.
        # producer  > 0 0 0 1 1 1 2 2 2 3 3 3
        # consumer1 < 0 1 2 3
        # consumer2 < 0 1 2 3
        # consumer3 < 0 1 2 3
        expected = []
        for i in range(messages_per_consumer):
            posted_payloads = producer.post_same(num=num_consumers, value=i)
            # Posted payloads on specific step are equal, need to memorize just
            # one sample from each group.
            expected.append(posted_payloads[0])

        for consumer in consumers:
            consumer.expect_messages(expected, confirm=True)

    def test_redelivery(self, cluster: Cluster):
        """
        Test: message redelivery when the consumer exits without confirmation.
        - Create 1 producer / 1 consumer: C1.
        - Open 1 priority queue, with subscription parameters.
        - Producer: produce N messages with properties.
        - Consumer C1: expect but not confirm all N messages.
        - Consumer C1: close.
        - Open another consumer C2 with the same parameters.
        - Consumer C2: expect and confirm all N messages.

        Concerns:
        - Unconfirmed messages must be redelivered correctly.
        """
        uri = tc.URI_PRIORITY
        producer = Producer(cluster, uri)
        consumer = Consumer(
            cluster,
            uri,
            ["x >= 0"],
        )

        expected = producer.post_diff(num=5)
        consumer.expect_messages(expected, confirm=False)
        # No confirm

        consumer.close()

        consumer = Consumer(
            cluster,
            uri,
            ["x >= 0"],
        )
        consumer.expect_messages(expected, confirm=True)

    def test_max_unconfirmed(self, cluster: Cluster):
        """
        Test: routing of messages between several subscriptions with the same
        expression and priority when max_unconfirmed reached.
        - Create 1 producer / 2 consumers: C1, C2.
        - Open 1 priority queue, the subscription expression and priority are
        the same for each consumer.

        Stage 1:
        - Producer: produce 10 identical messages (group A).
        - Expect 5 messages for each consumer (round-robin routed)
        Note: here max_unconfirmed just reached for consumer C1 but it doesn't
        affect routing yet.
        Producer -> [A * 10]        - 10 identical posted
        C1       <- [A * 5]         - 5 listed, no more unconfirmed allowed
        C2       <- [A * 5][_ * 10] - 5 listed, 10 unconfirmed allowed

        Stage 2:
        - Produce another 10 messages (group B), non identical.
        - Expect 5 messages for consumer C1.
        - Expect 15 messages for consumer C2.
        Note: max_unconfirmed was reached for C1 so this block of messages goes
        entirely to the consumer C2.  In the end max_unconfirmed reached for C2.
        Producer -> [B * 10]        - 10 posted
        C1       <- [A * 5]         - 5 listed, no more unconfirmed allowed
        C2       <- [A * 5][B * 10] - 15 listed, no more unconfirmed allowed

        Stage 3:
        - Produce another 1 message (group C), non identical.
        - Expect 5 messages for consumer C1.
        - Expect 15 messages for consumer C2.
        Note: max_unconfirmed was reached for C1 and C2, so this block of
        messages is not routed to either consumer.
        Producer -> [C * 1]         - 1 posted, not routed to consumers
        C1       <- [A * 5]         - 5 received, no more unconfirmed allowed
        C2       <- [A * 5][B * 10] - 15 received, no more unconfirmed allowed

        Stage 4:
        - Confirm all 5 messages for consumer C1.
        - Expect another 1 message for consumer C1 from group [C].
        - Expect 15 messages for consumer C2.
        Note: max_unconfirmed was reached for C2, but C1 confirms all received
        messages.  It is possible to route messages from Stage 3 to consumer C1.
        In the end max_unconfirmed reached for C1 again.
        Producer -> no more posted
        C1       <- [C * 1][_ * 4]  - 1 listed, 4 more unconfirmed allowed
        C2       <- [A * 5][B * 10] - 15 listed, no more unconfirmed allowed
        Confirmed:  [A * 5]

        Stage 5:
        - Confirm all messages for consumers C1 and C2.
        - Expect empty C1 and C2.

        Note: the group C contains only 1 message because we encounter
        implementation-specific delay of unconfirmed messages delivery if we
        try to post more messages here.

        Concerns:
        - Round-robin works for subscriptions until max_unconfirmed limit reached.
        - When max_unconfirmed reached for one consumer all messages go to the
        other consumer if possible.
        - When max_unconfirmed reached for all consumers extra messages are not
        routed to these.
        - Extra messages are delivered correctly when one of the consumers is
        ready to receive again.
        """
        uri = tc.URI_PRIORITY
        producer = Producer(cluster, uri)
        consumer1 = Consumer(
            cluster,
            uri,
            ["x >= 0"],
            max_unconfirmed_messages=5,
        )
        consumer2 = Consumer(
            cluster,
            uri,
            ["x >= 0"],
            max_unconfirmed_messages=15,
        )

        msgs_A = producer.post_same(num=10, value=0)
        half_A = msgs_A[:5]
        consumer1.expect_messages(half_A, confirm=False)
        consumer2.expect_messages(half_A, confirm=False)

        msgs_B = producer.post_diff(num=10, offset=100)

        consumer1.expect_messages(half_A, confirm=False, timeout=0)
        consumer2.expect_messages(half_A + msgs_B, confirm=False)

        msgs_C = producer.post_diff(num=1, offset=200)

        consumer1.expect_messages(half_A, confirm=False, timeout=0)
        consumer2.expect_messages(half_A + msgs_B, confirm=False, timeout=0)

        consumer1.confirm_all()

        consumer1.expect_messages(msgs_C, confirm=False)
        consumer2.expect_messages(half_A + msgs_B, confirm=False, timeout=0)

        consumer1.confirm_all()
        consumer2.confirm_all()

        consumer1.expect_empty()
        consumer2.expect_empty()

    def test_max_unconfirmed_low_priority_spillover(self, cluster: Cluster):
        """
        Test: routing of messages between several subscriptions with the same
        expression but with different priorities when max_unconfirmed reached.
        - Create 1 producer / 2 consumers: C1, C2.
        - Open 1 priority queue, the subscription expressions are the same but
        priorities are different for each consumer.

        Stage 1:
        - Producer: produce 15 sequential different messages M1-M15.
        - Consumer C1: expect but not confirm 5 messages M1-M5.
        - Consumer C2: expect empty (low priority).

        Stage 2:
        - Consumer C1: confirm all messages.
        - Consumer C1: expect but not confirm 5 next messages M6-M10.
        - Consumer C2: expect empty (low priority).

        Stage 3:
        - Consumer C2: reconfigure - set the same expression with a new highest
        priority.
        - Consumer C1: confirm all messages.
        - Consumer C1: expect empty (low priority).
        - Consumer C2: expect and confirm 5 last messages M11-M15.

        Concerns:
        - Messages are not routed to low priority consumers even when
        max_unconfirmed reached.
        - Reconfiguration does not break this behavior.
        """
        uri = tc.URI_PRIORITY
        producer = Producer(cluster, uri)
        consumer1 = Consumer(
            cluster,
            uri,
            subscriptions=[("x >= 0", 20)],
            max_unconfirmed_messages=5,
        )
        consumer2 = Consumer(
            cluster,
            uri,
            subscriptions=[("x >= 0", 1)],
            max_unconfirmed_messages=15,
        )

        expected = producer.post_diff(num=15, offset=0)

        consumer1.expect_messages(expected[:5], confirm=False)
        consumer2.expect_empty()

        consumer1.confirm_all()

        consumer1.expect_messages(expected[5:10], confirm=False)
        consumer2.expect_empty()

        consumer2.configure(subscriptions=[("x >= 0", 400)])

        consumer1.confirm_all()

        consumer1.expect_empty()
        consumer2.expect_messages(expected[10:15], confirm=False)

        consumer2.confirm_all()
        consumer2.expect_empty()

    def test_many_subscriptions(self, cluster: Cluster):
        """
        Test: open multiple subscriptions at once.
        - Create 1 producer / 1 consumer.
        - Open 1 priority queue, with K different subscriptions for consumer.
        - Produce K different messages suitable for each subscription 1-by-1.
        - Expect and confirm all K messages.

        Concerns:
        - All messages must be received in correct order.
        """
        uri = tc.URI_PRIORITY
        num_subscriptions = 256

        subscriptions = [f"x == {i}" for i in range(num_subscriptions)]

        producer = Producer(cluster, uri)
        consumer = Consumer(
            cluster,
            uri,
            subscriptions,
        )

        expected = producer.post_diff(num=num_subscriptions)

        consumer.expect_messages(expected, confirm=True)

    def test_complex_expressions(self, cluster: Cluster):
        """
        Test: complex subscription expressions with several properties.
        - Create 1 producer / 1 consumer.
        - Open 1 priority queue, with complex subscription expression for consumer.
        - Produce K different messages suitable for subscription.
        - Produce different messages not suitable for subscription.
        - Expect and confirm only K expected messages.

        Concerns:
        - Complex expressions work.
        - All expected messages must be received in correct order.
        """
        uri = tc.URI_PRIORITY

        producer = Producer(cluster, uri)
        consumer = Consumer(
            cluster,
            uri,
            ["x >= 0 && x <= 100 && y <= 100"],
        )

        expected = [producer.post(x=x, y=10 + x) for x in range(10)]
        producer.post(x=-10, y=100)
        producer.post(x=-100, y=-10)
        producer.post(x=-1, y=-1)

        consumer.expect_messages(expected, confirm=True)

    def test_incorrect_expressions(self, cluster: Cluster):
        """
        Test: incorrect expressions.
        - Create 1 producer / 1 consumer.
        - Open 1 priority queue, with always false subscription.

        For each pre-defined incorrect expression EXPR:
        - Try to configure consumer with expression EXPR.
        - Expect fail (exception) during configuration.
        - Post K different messages.
        - Expect empty consumer.
        - Reconfigure consumer with correct and always true expression.
        - Expect consumer to receive previously posted K messages.
        - Cleanup: reconfigure consumer with always false expression.

        Concerns:
        - Broker and tool (which uses C++ SDK) do not crash when incorrect
        expression specified.
        - Incorrect expressions are rejected by the SDK.
        - Broker and tool continue to work correctly.
        """
        expressions = [
            " ",
            "()",
            "((true)",
            "x > ",
            "x x",
            "&&",
            " && 1 > 0",
            "() && 1 > 0",
            "true && (>>)",
        ]

        uri = tc.URI_PRIORITY

        producer = Producer(cluster, uri)
        consumer = Consumer(cluster, uri, ["x < 0"])

        for i, expression in enumerate(expressions):
            try:
                consumer.configure([expression], timeout=5)
                assert False  # must not get here, expect exception
            except ITError:
                # expected
                pass

            expected = producer.post_diff(num=10, offset=i * 100)
            consumer.expect_empty()

            consumer.configure(["x >= 0"])
            consumer.expect_messages(expected, confirm=True)

            consumer.configure(["x < 0"])

    def test_non_bool_expressions(self, cluster: Cluster):
        """
        Test: expressions that are not evaluated as booleans.
        - Create 1 producer / 1 consumer.
        - Open 1 priority queue, with always true subscription.

        For each pre-defined non-boolean expression EXPR:
        - Configure consumer with expression EXPR.
        - Produce K different messages.
        - Consumer: expect empty.
        Cleanup:
        - Consumer: reconfigure with always true expression.
        - Consumer: expect and confirm all K messages.

        Concerns:
        - Broker and tool do not crash when non-bool expression specified.
        - Non-bool expressions considered to be always false.
        """
        expressions = ["x", "x + 33"]

        uri = tc.URI_PRIORITY

        producer = Producer(cluster, uri)
        consumer = Consumer(cluster, uri, ["x >= 0"])

        for i, expression in enumerate(expressions):
            consumer.configure([expression])

            expected = producer.post_diff(num=10, offset=i * 100)
            consumer.expect_empty()

            consumer.configure(["x >= 0"])
            consumer.expect_messages(expected, confirm=True)

    def test_numeric_limits(self, cluster: Cluster, logger):
        """
        Test: pass huge values in subscription expressions.
        - Create 1 producer / 1 consumer.
        - Open 1 priority queue.

        For each pre-defined huge value VAL:
        - Starting state: configure consumer with always false expression.
        - Try to re-configure consumer with expression including huge value VAL.
        - Expect successful configuration for supported values.
        - Expect exception during configuration for unsupported values.
        - Produce K different messages which are suitable for subscription in
        ideal mathematical means.
        - Expect empty consumer for unsupported VAL.
        - Expect and confirm all K messages for supported VAL.
        - Reconfigure consumer if needed and confirm all messages for clean-up.

        Concerns:
        - Expressions with out-of-bounds values are rejected by the C++ SDK.
        - Broker and tool (which uses C++ SDK) do not crash when out-of-bounds
        expression specified.
        - Seemingly adequate range of integer values in expressions is still
        supported.
        """
        uri = tc.URI_PRIORITY

        supported_ints = [
            -(2**15),
            2**15 - 1,
            -(2**31),
            2**31 - 1,
            -(2**63) + 1,
            2**63 - 1,
            -(2**63),
        ]
        unsupported_ints = [
            2**63,
            -(2**127),
            2**127 - 1,
            -(2**255),
            2**255 - 1,
        ]

        producer = Producer(cluster, uri)
        consumer = Consumer(
            cluster,
            uri,
        )

        for i, value in enumerate(supported_ints + unsupported_ints):
            consumer.configure(["x < 0"])

            try:
                consumer.configure([f"x != {value}"], timeout=5)
                assert value in supported_ints
            except ITError:
                # exception is expected for values from 'unsupported_ints'
                assert value in unsupported_ints

            expected = producer.post_diff(num=10, offset=i * 100)

            if value in supported_ints:
                logger.info("supported_ints[%s]: %s ", i, value)
                consumer.expect_messages(expected, confirm=True)
            else:
                logger.info("unsupported_ints[%s]: %s", i - len(supported_ints), value)
                consumer.expect_empty()
                # Reconfigure for clean-up
                consumer.configure(subscriptions=["x >= 0"])
                consumer.expect_messages(expected, confirm=True)

    def test_empty_subscription(self, cluster: Cluster):
        """
        Test: empty subscription works for backward compatibility.
        - Create 1 producer / 1 consumer.
        - Open 1 priority queue, with empty subscription for consumer.

        - Produce [M1] messages with properties to the queue.
        - Expect and confirm all [M1] messages in correct order by consumer.

        - Reconfigure consumer with always false expression.
        - Produce [M2] messages with properties to the queue.
        - Expect empty consumer.

        - Reconfigure consumer with empty subscription.
        - Expect and confirm all [M2] messages in correct order by consumer.

        Concerns:
        - Consumer with empty subscription must receive all messages.
        - Reconfiguration does not break this behaviour.
        """
        uri = tc.URI_PRIORITY

        producer = Producer(cluster, uri)
        consumer = Consumer(cluster, uri, EMPTY_SUBSCRIPTION)

        expected = producer.post_diff(num=10, offset=0)
        consumer.expect_messages(expected, confirm=True)

        consumer.configure(["x < 0 && x > 0"])  # always false

        expected = producer.post_diff(num=10, offset=100)
        consumer.expect_empty()

        consumer.configure(EMPTY_SUBSCRIPTION)
        consumer.expect_messages(expected, confirm=True)

        expected = producer.post_diff(num=10, offset=200)
        consumer.expect_messages(expected, confirm=True)

    def test_empty_expression(self, cluster: Cluster):
        """
        Test: subscription with specific empty string "" expression rejected.
        - Create 1 producer / 1 consumer.
        - Open 1 priority queue for producer.
        - Produce N messages with properties to the queue.
        - Expect fail when try to open consumer with empty "" subscription
        expression.
        - Open consumer with always false expression.
        - Expect empty consumer.
        - Expect fail when try to configure consumer with empty "" subscription
        expression, mixed together with always true expressions.
        - Expect empty consumer.
        - Reconfigure consumer with always true expression.
        - Expect and confirm all N messages in correct order by consumer.

        Concerns:
        - Empty "" subscription expression is rejected by both open/configure.
        - These rejections do not break the broker.
        - These rejections do not allow consumer to receive not routed yet
        messages unintentionally.
        - If valid expressions passed together with empty "" expression the
        entire configure is rejected, so these valid subscriptions do not work.
        """
        uri = tc.URI_PRIORITY

        producer = Producer(cluster, uri)
        expected = producer.post_diff(num=10, offset=0)
        always_true = "x >= 0"
        always_false = "x < -100"

        try:
            # Expect exception raise in constructor, so do not want to use this
            # object later.  Let garbage collector handle it.
            _ = Consumer(cluster, uri, [""], timeout=5)
            assert False, "Expected fail on empty subscription expression"
        except ITError as ex:
            exception_message = str(ex)
            assert "did not complete" in exception_message

        consumer = Consumer(cluster, uri, [always_false])
        consumer.expect_empty()

        try:
            # Expect exception raise in 'configure'.
            consumer.configure([always_true, "", always_true], timeout=5)
            assert False, "Expected fail on empty subscription expression"
        except ITError as ex:
            exception_message = str(ex)
            assert "did not complete" in exception_message

        consumer.expect_empty()

        consumer.configure([always_true])
        consumer.expect_messages(expected, confirm=True)

    def test_non_existent_property(self, cluster: Cluster):
        """
        Test: expressions are evaluated even if some properties are missing.
        - Create 1 producer / 2 consumers: C1, C2.
        - Open 1 priority queue, provide different complex subscription
        expressions for each consumer:
        consumer C1 -> property "x" first OR "y" second.
        consumer C2 -> property "y" first OR "x" second.
        - Produce [M1] messages with property "x" only to the queue.
        - Produce [M2] messages with property "y" only to the queue.
        - Expect consumer C1 to receive and confirm only messages [M1].
        - Expect consumer C2 to receive and confirm only messages [M2].

        Concerns:
        - Expression strings evaluated even if some properties are missing, if
        some non-existent property encountered the expression result is False.
        - Expressions are evaluated from left to right, and it is possible to
        return result early if logical expression allows it.
        """
        uri = tc.URI_PRIORITY

        producer = Producer(cluster, uri)
        consumer1 = Consumer(cluster, uri, ["x >= 0 || (y >= 0)"])
        consumer2 = Consumer(cluster, uri, ["y >= 0 || (x >= 0)"])

        expected1 = [producer.post(x=10) for _ in range(10)]
        expected2 = [producer.post(y=10) for _ in range(10)]

        consumer1.expect_messages(expected1, confirm=True)
        consumer2.expect_messages(expected2, confirm=True)

    def test_date_time(self, cluster: Cluster):
        """
        Test: expressions with date/time passed as string work.
        - Create 1 producer / 1 consumers.
        - Open 1 priority queue.
        - Specify a list of sorted dates DATES.
        - Consider START and END from DATES, where START < 1+ elements < END.
        - For each such pair START/END do the following:

        - Configure consumer to receive only messages with date from (START; END).
        - Produce N messages with all sorted and specified dates from DATES.
        - Expect and confirm only messages with date from (START; END).
        - Reconfigure consumer to receive messages from the reversed date range:
        (-inf; START] union [END; +inf).
        - Expect and confirm only messages with date from (-inf; START] union [END; +inf).

        Concerns:
        - String properties and expressions work.
        - Dates in ISO string format are compared in obvious and expected way.
        """
        uri = tc.URI_PRIORITY
        producer = Producer(cluster, uri)
        consumer = Consumer(cluster, uri)

        # Contract: 'dates' elements are sorted.
        dates = [
            datetime(1970, 1, 1, 0, 0, 0, 0),
            datetime(1970, 1, 1, 12, 0, 0, 0),
            datetime(1999, 10, 30, 4, 0, 0, 0),
            datetime(2007, 4, 3, 11, 15, 25, 50),
            datetime(2007, 8, 6, 23, 30, 50),
            datetime(2007, 8, 6, 23, 30, 50, 100),
            datetime(2022, 12, 12, 10, 56, 34, 678),
        ]

        for id_end in range(len(dates)):  # pylint: disable=consider-using-enumerate
            for id_start in range(id_end - 1):
                start = dates[id_start].isoformat()
                end = dates[id_end].isoformat()

                expression = f'x > "{start}" && x < "{end}"'
                reverse = f'x <= "{start}" || x >= "{end}"'

                consumer.configure([expression])

                posted = [producer.post(x=date.isoformat()) for date in dates]

                # sample:
                # [] [s] [] [] [] [e] []
                #        [++++++]
                expected = posted[id_start + 1 : id_end]

                # sample:
                # [] [s] [] [] [] [e] []
                # [++++]          [++++]
                remaining = posted[: id_start + 1] + posted[id_end:]

                consumer.expect_messages(expected, confirm=True)

                consumer.configure([reverse])
                consumer.expect_messages(remaining, confirm=True)

    def test_consumer_multiple_priorities(self, cluster: Cluster):
        """
        Test: messages are routed correctly with overlapping subscriptions with
        different priorities.
        - Create 1 producer / 2 consumers: C1, C2.
        - Open 1 priority queue, provide different overlapping subscriptions
        with different priorities.
        Consumer C1: high priority, suitable "x" range     [1000; +inf)
        Consumer C2: low priority, suitable "x" range [0 .......; +inf)
        - Producer: produce groups of messages with property "x" from [0; 1000)
        range and [1000; +inf) range.
        - Consumer C1: expect and confirm all messages with "x" in range [1000; +inf).
        - Consumer C2: expect and confirm all messages with "x" in range [0; 1000).

        Concerns:
        - High priority consumer receives all suitable messages according to
        subscription.
        - Low priority consumer will receive all suitable messages according to
        subscription if there are no conflicting consumers with higher priority.
        """
        uri = tc.URI_PRIORITY

        producer = Producer(cluster, uri)
        consumer1 = Consumer(cluster, uri, [("x >= 1000", 5)])
        consumer2 = Consumer(cluster, uri, [("x >= 0", 1)])

        expected = producer.post_diff(num=10, offset=1000)
        consumer1.expect_messages(expected, confirm=True)

        expected = producer.post_diff(num=10, offset=0)
        consumer2.expect_messages(expected, confirm=True)

        expected = producer.post_diff(num=10, offset=2000)
        consumer1.expect_messages(expected, confirm=True)

        expected = producer.post_diff(num=10, offset=100)
        consumer2.expect_messages(expected, confirm=True)

    def test_no_capacity_all_optimization(self, cluster: Cluster):
        """
        Test: delivery optimization works during routing when all subscriptions
        have no capacity.

        DRQS  171509204

        - Create 1 producer / 3 consumers: C1, C2, C3.
        - Consumers: max_unconfirmed_messages = 1.
        - Post several messages to cause NO_CAPACITY_ALL condition.
        - Expect delivery optimization log messages when necessary.

        TODO: compare GUIDs of the posted messages and messages on which NO_CAPACITY_ALL
        encountered.

        Concerns:
        - Delivery optimization works when NO_CAPACITY_ALL condition observed.
        """
        uri = tc.URI_PRIORITY

        producer = Producer(cluster, uri)
        consumer1 = Consumer(cluster, uri, ["x > 0"], max_unconfirmed_messages=1)

        optimization_monitor = DeliveryOptimizationMonitor(cluster)

        # consumer1: capacity 0/1
        # pending messages: []

        m1 = producer.post(x=1)

        consumer1.expect_messages([m1], confirm=False)
        # consumer1: capacity 1/1 [m1]
        # pending messages: []

        # Do not expect a new delivery optimization message, because
        # 'consumer1' just reached the full capacity with the last message 'm1',
        # but this is not enough to enable the delivery optimization: need at least
        # one message after the added one to see that all consumers are full.
        assert not optimization_monitor.has_new_message()

        m2 = producer.post(x=2)
        # consumer1: capacity 1/1 [m1]
        # pending messages: [m2]

        # Extra message 'm2' is good for 'consumer1' but cannot be delivered:
        # no capacity all condition observed.
        assert optimization_monitor.has_new_message()

        m3 = producer.post(x=3)  # pylint: disable=unused-variable
        # consumer1: capacity 1/1 [m1]
        # pending messages: [m2, m3]

        # Extra message 'm3' is good for 'consumer1' but cannot be delivered:
        # no capacity all condition observed.
        assert optimization_monitor.has_new_message()

        consumer1.confirm_all()
        consumer1.expect_messages([m2], confirm=False)
        # consumer1: capacity 1/1 [m2]
        # pending messages: [m3]

        # Message 'm1' was confirmed, so routing is triggered.
        # 'consumer1' receives the pending message 'm2' and becomes full again.
        # The message 'm3' still exists and is good for 'consumer1', but cannot
        # be delivered: no capacity all condition observed.
        assert optimization_monitor.has_new_message()

        m4 = producer.post(y=1)
        # consumer1: capacity 1/1 [m2]
        # pending messages: [m3, m4]

        # Posting the message 'm4' triggers routing, but the only consumer is full:
        # no capacity all condition observed.
        assert optimization_monitor.has_new_message()

        consumer2 = Consumer(cluster, uri, ["y < 0"], max_unconfirmed_messages=1)

        m5 = producer.post(y=-1)
        consumer2.expect_messages([m5], confirm=False)
        # consumer1: capacity 1/1 [m2]
        # consumer2: capacity 1/1 [m5]
        # pending messages: [m3, m4]

        # Do not expect a new delivery optimization message, because
        # 'consumer2' just reached the full capacity with the last message 'm5',
        # but this is not enough to enable the delivery optimization: need at least
        # one message after the added one to see that all consumers are full.
        assert not optimization_monitor.has_new_message()

        consumer3 = Consumer(cluster, uri, ["y >= 0"], max_unconfirmed_messages=1)
        consumer3.expect_messages([m4], confirm=False)
        # consumer1: capacity 1/1 [m2]
        # consumer2: capacity 1/1 [m5]
        # consumer3: capacity 1/1 [m4]
        # pending messages: [m3]

        # Do not expect a new delivery optimization message, because
        # 'consumer3' just reached the full capacity with the last message 'm4',
        # but this is not enough to enable the delivery optimization: need at least
        # one message after the added one to see that all consumers are full.
        assert not optimization_monitor.has_new_message()

    def test_no_capacity_all_fanout(self, cluster: Cluster):
        """
        Test: delivery optimization encountered with one app does not affect
        other apps.

        DRQS  171509204

        - Create 1 producer / 2 consumers: C_foo, C_bar.
        - C_foo: max_unconfirmed_messages = 128.
        - C_bar: max_unconfirmed_messages = 1.
        - Post several messages to cause NO_CAPACITY_ALL condition for C_bar.
        - Expect delivery log messages when necessary.

        Concerns:
        - Delivery optimization condition encountered with one app does not
        affect the other app.
        """
        producer_uri = tc.URI_FANOUT
        uri_foo = tc.URI_FANOUT_FOO
        uri_bar = tc.URI_FANOUT_BAR

        producer = Producer(cluster, producer_uri)
        consumer_foo = Consumer(
            cluster, uri_foo, ["x > 0"], max_unconfirmed_messages=128
        )
        consumer_bar = Consumer(cluster, uri_bar, ["x > 0"], max_unconfirmed_messages=1)

        optimization_monitor = DeliveryOptimizationMonitor(cluster)

        # consumer_foo: capacity 0/128
        # consumer_bar: capacity 0/1
        # pending messages (bar): []

        m1 = producer.post(x=1)

        consumer_foo.expect_messages([m1], confirm=False)
        consumer_bar.expect_messages([m1], confirm=False)
        # consumer_foo: capacity 1/128 [m1]
        # consumer_bar: capacity 1/1 [m1]
        # pending messages (bar): []

        # Do not expect a new delivery optimization message, because
        # 'consumer_bar' just reached the full capacity with the last message 'm1',
        # but this is not enough to enable the delivery optimization: need at least
        # one message after the added one to see that all consumers are full.
        assert not optimization_monitor.has_new_message("bar")

        m2 = producer.post(x=2)
        # consumer_foo: capacity 2/128 [m1, m2]
        # consumer_bar: capacity 1/1 [m1]
        # pending messages (bar): [m2]

        # Extra message 'm2' is good for 'consumer_bar' but cannot be delivered:
        # no capacity all condition observed.
        assert optimization_monitor.has_new_message("bar")

        m3 = producer.post(x=3)  # pylint: disable=unused-variable
        # consumer_foo: capacity 3/128 [m1, m2, m3]
        # consumer_bar: capacity 1/1 [m1]
        # pending messages (bar): [m2, m3]

        # Extra message 'm3' is good for 'consumer_bar' but cannot be delivered:
        # no capacity all condition observed.
        assert optimization_monitor.has_new_message("bar")

        consumer_bar.confirm_all()
        consumer_bar.expect_messages([m2], confirm=False)
        # consumer_foo: capacity 3/128 [m1, m2, m3]
        # consumer_bar: capacity 1/1 [m2]
        # pending messages (bar): [m3]

        # Message 'm1' was confirmed, so routing is triggered.
        # 'consumer_bar' receives the pending message 'm2' and becomes full again.
        # The message 'm3' still exists and is good for 'consumer_bar', but cannot
        # be delivered: no capacity all condition observed.
        assert optimization_monitor.has_new_message("bar")

        # App 'foo' has not reached NO_CAPACITY_ALL condition, so there must
        # not be any delivery optimization messages.
        assert not optimization_monitor.has_new_message("foo")

        # App 'baz' does not have any consumers.  It will have the NO_CAPACITY_ALL
        # condition, because it's impossible to have any progress.
        assert optimization_monitor.has_new_message("baz")

    def test_primary_node_crash(self, standard_cluster: Cluster):
        """
        Test: configured subscriptions work after primary node crash.
        - Create 1 producer / N consumers.
        - Open 1 priority queue, provide non-overlapping subscriptions.
        - Produce messages [A] suitable for each consumer.
        - Expect but not confirm suitable messages from [A] for each consumer.

        - Kill primary node.
        - Wait for a new leader.

        - Expect and confirm messages from [A] for each consumer.
        - Produce messages [B] suitable for each consumer.
        - Expect and confirm messages from [B] for each consumer.

        Concerns:
        - Message listing works after primary node crash and change.
        - Already configured subscriptions work after primary node crash and
        change.
        """
        uri = tc.URI_PRIORITY

        producer = Producer(standard_cluster, uri)
        consumer1 = Consumer(standard_cluster, uri, ["x < 0"])
        consumer2 = Consumer(standard_cluster, uri, ["x > 0"])
        consumer3 = Consumer(standard_cluster, uri, ["x == 0"])

        expected1 = producer.post_diff(num=10, offset=-100)
        expected2 = producer.post_diff(num=10, offset=100)
        expected3 = producer.post_same(num=10, value=0)

        consumer1.expect_messages(expected1, confirm=False)
        consumer2.expect_messages(expected2, confirm=False)
        consumer3.expect_messages(expected3, confirm=False)

        standard_cluster.drain()
        leader = standard_cluster.last_known_leader
        leader.check_exit_code = False
        leader.kill()
        leader.wait()

        leader = standard_cluster.wait_leader()
        assert leader is not None

        # Consumer with always false expression in this context.
        # Used for synchronization.
        Consumer(standard_cluster, uri, ["x > 100000"])

        consumer1.expect_messages(expected1, confirm=True, timeout=0)
        consumer2.expect_messages(expected2, confirm=True, timeout=0)
        consumer3.expect_messages(expected3, confirm=True, timeout=0)

        expected1 = producer.post_diff(num=10, offset=-200)
        expected2 = producer.post_diff(num=10, offset=200)
        expected3 = producer.post_same(num=10, value=0)

        consumer1.expect_messages(expected1, confirm=True)
        consumer2.expect_messages(expected2, confirm=True)
        consumer3.expect_messages(expected3, confirm=True)

    def test_redelivery_on_primary_node_crash(self, standard_cluster: Cluster):
        """
        Test: configured subscriptions work after primary node crash.
        - Create 1 producer / 3 consumers: C_high, C_low1, C_low2.
        - Open 1 priority queue.
        Consumer C_high: high priority, expression (-inf  ;  +inf)
        Consumer C_low1: low priority, expression  (-inf; 0]
        Consumer C_low2: low priority, expression        (0; +inf)
        - Produce messages [A1] suitable both for C_high and C_low1.
        - Produce messages [B1] suitable both for C_high and C_low2.
        - Expect but not confirm messages [A1, B1] for consumer C_high.

        - Kill primary node.
        - Close consumer C_high.
        - Wait for a new leader.

        - Expect and confirm messages [A1] for consumer C_low1.
        - Expect and confirm messages [B1] for consumer C_low2.
        - Produce messages [A2] suitable both for C_low1.
        - Produce messages [B2] suitable both for C_low2.
        - Expect and confirm messages [A2] for consumer C_low1.
        - Expect and confirm messages [B2] for consumer C_low2.

        Concerns:
        - Message re-routing works after primary node crash and change.
        - Already configured subscriptions work after primary node crash and
        change.
        """
        uri = tc.URI_PRIORITY

        producer = Producer(standard_cluster, uri)
        consumer_high = Consumer(standard_cluster, uri, [("x <= 0 || x > 0", 2)])
        consumer_low1 = Consumer(standard_cluster, uri, [("x <= 0", 1)])
        consumer_low2 = Consumer(standard_cluster, uri, [("x > 0", 1)])

        expected1 = producer.post_diff(num=10, offset=-100)
        expected2 = producer.post_diff(num=10, offset=100)

        consumer_high.expect_messages(expected1 + expected2, confirm=False)

        standard_cluster.drain()
        leader = standard_cluster.last_known_leader
        leader.check_exit_code = False
        leader.kill()
        leader.wait()

        consumer_high.close()

        leader = standard_cluster.wait_leader()
        assert leader is not None

        # Consumer with always false expression in this context.
        # Used for synchronization.
        Consumer(standard_cluster, uri, ["x > 100000"])

        consumer_low1.expect_messages(expected1, confirm=True)
        consumer_low2.expect_messages(expected2, confirm=True)

        expected1 = producer.post_diff(num=10, offset=-200)
        expected2 = producer.post_diff(num=10, offset=200)

        consumer_low1.expect_messages(expected1, confirm=True)
        consumer_low2.expect_messages(expected2, confirm=True)

    def test_reconfigure_on_primary_node_crash(self, standard_cluster: Cluster):
        """
        Test: configured subscriptions work after primary node crash.
        - Create 1 producer / 3 consumers: C_high, C_low1, C_low2.
        - Open 1 priority queue.
        Consumer C_high: high priority, expression (-inf  ;  +inf)
        Consumer C_low1: low priority, expression        (0; +inf)
        Consumer C_low2: low priority, expression  (-inf; 0]
        - Produce messages [A1] suitable both for C_high and C_low1.
        - Produce messages [B1] suitable both for C_high and C_low2.
        - Expect but not confirm messages [A1, B1] for consumer C_high.

        - Kill primary node.
        - Close consumer C_high.
        - Reconfigure consumers C_low1, C_low2: reverse expressions.
        Consumer C_low1: low priority, expression  (-inf; 0]
        Consumer C_low2: low priority, expression        (0; +inf)
        - Wait for a new leader.

        - Expect and confirm messages [B1] for consumer C_low1.
        - Expect and confirm messages [A1] for consumer C_low2.
        - Produce messages [B2] suitable both for C_low1.
        - Produce messages [A2] suitable both for C_low2.
        - Expect and confirm messages [B2] for consumer C_low1.
        - Expect and confirm messages [A2] for consumer C_low2.

        Concerns:
        - Configure works after primary node crash and change.
        """
        uri = tc.URI_PRIORITY

        producer = Producer(standard_cluster, uri)
        consumer_high = Consumer(standard_cluster, uri, [("x <= 0 || x > 0", 2)])
        consumer_low1 = Consumer(standard_cluster, uri, [("x > 0", 1)])
        consumer_low2 = Consumer(standard_cluster, uri, [("x <= 0", 1)])

        expected1 = producer.post_diff(num=10, offset=-100)
        expected2 = producer.post_diff(num=10, offset=100)

        consumer_high.expect_messages(expected1 + expected2, confirm=False)

        standard_cluster.drain()
        leader = standard_cluster.last_known_leader
        leader.check_exit_code = False
        leader.kill()
        leader.wait()

        # Expressions are reversed here intentionally
        consumer_low1.configure(subscriptions=["x <= 0"])
        consumer_low2.configure(subscriptions=["x > 0"])
        consumer_high.close()

        leader = standard_cluster.wait_leader()
        assert leader is not None

        # Consumer with always false expression in this context.
        # Used for synchronization.
        Consumer(standard_cluster, uri, ["x > 100000"])

        consumer_low1.expect_messages(expected1, confirm=True)
        consumer_low2.expect_messages(expected2, confirm=True)

        expected1 = producer.post_diff(num=10, offset=-200)
        expected2 = producer.post_diff(num=10, offset=200)

        consumer_low1.expect_messages(expected1, confirm=True)
        consumer_low2.expect_messages(expected2, confirm=True)

    @pytest.mark.skip("TODO: fix this test")
    @tweak.domain.max_delivery_attempts(2)
    @tweak.cluster.message_throttle_config(
        mqbcfg.MessageThrottleConfig(
            high_threshold=1,
            low_threshold=1,
            high_interval=5000000,
            low_interval=5000000,
        )
    )
    def test_poison(self, cluster: Cluster):
        """
        Test: poison messages detection works for subscriptions.
        - Configure max delivery attempts = 1 and adjust message throttling
        parameters for long delay if poisoned message found.
        - Create 1 producer / 2 consumers: C1, C2.
        - Open 1 priority queue, consumers have non-overlapping subscriptions.
        - Produce 2 messages: M1, M2.
        - Expect M1 for consumer C1, M2 for consumer C2, but not confirm.
        - Kill consumer C1.
        - Produce 1 message M3 suitable for consumer C2.
        - Consumer C2: expect and confirm only message M2 (M3 is not delivered
        because of the delay).

        Producer    -> M1, M2
        Consumer C1 <- M1
        Consumer C2 <-     M2
          kill C1
          message M1 is marked as poisoned, the queue is suspended for delay
        Producer    -> M3
        Consumer C2 <- M2
          delay, M3 is not delivered to C2

        Concerns:
        - Delay condition works for the entire queue, not just for concrete
        subscriptions.
        """
        uri = tc.URI_PRIORITY

        producer = Producer(cluster, uri)

        consumer1 = Consumer(cluster, uri, ["x <= 0"])
        consumer2 = Consumer(cluster, uri, ["x > 0"])
        expected1 = [producer.post(x=0)]
        expected2 = [producer.post(x=1)]

        consumer1.expect_messages(expected1, confirm=False)
        consumer2.expect_messages(expected2, confirm=False)

        consumer1.kill()

        # The queue must be suspended for some time, so 'delayed' should not
        # be delivered fast.
        producer.post(x=2)
        delayed = [producer.post(x=2)]  # pylint: disable=unused-variable

        consumer2.expect_messages(expected2, confirm=True)
