from itertools import islice

import bmq.dev.it.testconstants as tc
from bmq.dev.it.fixtures import Cluster, cluster  # pylint: disable=unused-import
from bmq.dev.it.process.client import Client


class TestBroadcast:
    def test_breathing(self, cluster: Cluster):
        """
        Verify that broadcast mode works properly for a single producer and a
        single consumer.
        """

        proxy1, proxy2 = islice(cluster.proxy_cycle(), 2)

        producer = proxy1.create_client("producer")
        consumer = proxy2.create_client("consumer")

        producer.open(tc.URI_BROADCAST, flags=["write", "ack"], succeed=True)
        consumer.open(tc.URI_BROADCAST, flags=["read"], succeed=True)

        assert (
            producer.post(tc.URI_BROADCAST, payload=["msg1"], block=True, wait_ack=True)
            == Client.e_SUCCESS
        )

        assert consumer.wait_push_event()
        msgs = consumer.list(tc.URI_BROADCAST, block=True)
        assert len(msgs) == 1
        assert msgs[0].payload == "msg1"

    def test_multi_consumers(self, cluster: Cluster):
        """
        Verify that broadcast mode works properly for multiple consumers.
        """

        proxies = cluster.proxy_cycle()
        producer = next(proxies).create_client("producer")
        consumers = [next(proxies).create_client(f"client{i}") for i in range(3)]

        producer.open(tc.URI_BROADCAST, flags=["write", "ack"], succeed=True)
        for consumer in consumers:
            consumer.open(tc.URI_BROADCAST, flags=["read"], succeed=True)

        producer.post(tc.URI_BROADCAST, payload=["msg1"], succeed=True, wait_ack=True)
        for consumer in consumers:
            assert consumer.wait_push_event()
            msgs = consumer.list(tc.URI_BROADCAST, block=True)
            assert len(msgs) == 1
            assert msgs[0].payload == "msg1"

    def test_multi_producers_consumers(self, cluster: Cluster):
        """
        Verify that broadcast mode works properly for multiple producers and
        consumers.
        """

        proxy1, proxy2 = islice(cluster.proxy_cycle(), 2)
        po1 = proxy1.create_client("po1")
        po2 = proxy1.create_client("po2")
        pr1 = proxy2.create_client("pr1")
        producers = [po1, po2, pr1]
        co1 = proxy1.create_client("CO1")
        cr1 = proxy2.create_client("CR1")
        cr2 = proxy2.create_client("CR2")
        consumers = [co1, cr1, cr2]

        for producer in producers:
            producer.open(tc.URI_BROADCAST, flags=["write", "ack"], succeed=True)
        for consumer in consumers:
            consumer.open(tc.URI_BROADCAST, flags=["read"], succeed=True)

        for i, producer in enumerate(producers, 1):
            producer.post(
                tc.URI_BROADCAST, payload=[f"msg{i}"], block=True, wait_ack=True
            )
            for consumer in consumers:
                assert consumer.wait_push_event()
                msgs = consumer.list(tc.URI_BROADCAST, block=True)
                assert len(msgs) == i
                assert msgs[i - 1].payload == f"msg{i}"

    def test_resubscribe(self, cluster: Cluster):
        """
        Verify that when a consumer undergoes a re-subscription, messages
        posted during non-subscription will not be received.
        """

        proxy = next(cluster.proxy_cycle())
        producer = proxy.create_client("producer")
        consumer = proxy.create_client("consumer")

        # Consumer subscribes
        producer.open(tc.URI_BROADCAST, flags=["write", "ack"], succeed=True)
        consumer.open(tc.URI_BROADCAST, flags=["read"], succeed=True)

        producer.post(tc.URI_BROADCAST, payload=["msg1"], succeed=True, wait_ack=True)
        assert consumer.wait_push_event()
        msgs = consumer.list(tc.URI_BROADCAST, block=True)
        assert len(msgs) == 1
        assert msgs[0].payload == "msg1"
        consumer.confirm(tc.URI_BROADCAST, "+1", succeed=True)

        # Consumer unsubscribes
        assert consumer.close(tc.URI_BROADCAST, block=True) == Client.e_SUCCESS

        producer.post(tc.URI_BROADCAST, payload=["msg2"], succeed=True, wait_ack=True)

        # Consumer resubscribes
        consumer.open(tc.URI_BROADCAST, flags=["read"], succeed=True)
        assert not consumer.list(tc.URI_BROADCAST, block=True)

        producer.post(tc.URI_BROADCAST, payload=["msg3"], succeed=True, wait_ack=True)
        assert consumer.wait_push_event()
        msgs = consumer.list(tc.URI_BROADCAST, block=True)
        assert len(msgs) == 1
        assert msgs[0].payload == "msg3"

    def test_add_consumers(self, cluster: Cluster):
        """
        Verify that only active consumers receive messages as new consumers are
        being added.
        """

        proxy1, proxy2 = islice(cluster.proxy_cycle(), 2)
        producer = proxy1.create_client("producer")
        co1 = proxy1.create_client("CO1")
        cr1 = proxy2.create_client("CR1")

        producer.open(tc.URI_BROADCAST, flags=["write", "ack"], succeed=True)

        # This message should not be received by any consumer
        producer.post(
            tc.URI_BROADCAST, payload=["null_msg"], succeed=True, wait_ack=True
        )

        co1.open(tc.URI_BROADCAST, flags=["read"], succeed=True)
        assert not co1.list(tc.URI_BROADCAST, block=True)

        # This message should only be received by CO1
        producer.post(tc.URI_BROADCAST, payload=["msg1"], succeed=True, wait_ack=True)

        assert co1.wait_push_event()
        msgs = co1.list(tc.URI_BROADCAST, block=True)
        assert len(msgs) == 1
        assert msgs[0].payload == "msg1"
        co1.confirm(tc.URI_BROADCAST, "+1", succeed=True)

        cr1.open(tc.URI_BROADCAST, flags=["read"], succeed=True)
        assert not cr1.list(tc.URI_BROADCAST, block=True)

        # This messages should be received by all consumers
        producer.post(tc.URI_BROADCAST, payload=["msg2"], succeed=True, wait_ack=True)

        for consumer in [co1, cr1]:
            assert consumer.wait_push_event()
            msgs = consumer.list(tc.URI_BROADCAST, block=True)
            assert len(msgs) == 1
            assert msgs[0].payload == "msg2"

    def test_dynamic_priorities(self, cluster: Cluster):
        """
        Verify that only the highest priority consumers receive messages when
        the priorities are dynamically changing
        """

        proxy1, proxy2 = islice(cluster.proxy_cycle(), 2)
        producer = proxy1.create_client("producer")
        co1 = proxy1.create_client("CO1")
        cr1 = proxy2.create_client("CR1")
        cr2 = proxy2.create_client("CR2")

        producer.open(tc.URI_BROADCAST, flags=["write", "ack"], succeed=True)
        for consumer in [co1, cr1, cr2]:
            consumer.open(
                tc.URI_BROADCAST, flags=["read"], succeed=True, consumer_priority=2
            )

        producer.post(tc.URI_BROADCAST, payload=["msg1"], succeed=True, wait_ack=True)
        for consumer in [co1, cr1, cr2]:
            assert consumer.wait_push_event()
            msgs = consumer.list(tc.URI_BROADCAST, block=True)
            assert len(msgs) == 1
            assert msgs[0].payload == "msg1"
            consumer.confirm(tc.URI_BROADCAST, "+1", succeed=True)

        # CR1's priority is lowered. It should not receive messages anymore
        assert (
            cr1.configure(tc.URI_BROADCAST, block=True, consumer_priority=1)
            == Client.e_SUCCESS
        )

        producer.post(tc.URI_BROADCAST, payload=["msg2"], succeed=True, wait_ack=True)

        for consumer in [co1, cr2]:
            assert consumer.wait_push_event()
            msgs = consumer.list(tc.URI_BROADCAST, block=True)
            assert len(msgs) == 1
            assert msgs[0].payload == "msg2"
            consumer.confirm(tc.URI_BROADCAST, "+1", succeed=True)

        assert not cr1.list(tc.URI_BROADCAST, block=True)

        # CO1 becomes the single highest priority consumer. Only it should
        # receive any message
        assert (
            co1.configure(tc.URI_BROADCAST, block=True, consumer_priority=99)
            == Client.e_SUCCESS
        )

        producer.post(tc.URI_BROADCAST, payload=["msg3"], succeed=True, wait_ack=True)

        assert co1.wait_push_event()
        msgs = co1.list(tc.URI_BROADCAST, block=True)
        assert len(msgs) == 1
        assert msgs[0].payload == "msg3"
        co1.confirm(tc.URI_BROADCAST, "+1", succeed=True)

        for consumer in [cr1, cr2]:
            assert not consumer.list(tc.URI_BROADCAST, block=True)

        # Increase CR1's priority to be the same as CO1
        assert (
            cr1.configure(tc.URI_BROADCAST, block=True, consumer_priority=99)
            == Client.e_SUCCESS
        )

        producer.post(tc.URI_BROADCAST, payload=["msg4"], succeed=True, wait_ack=True)

        for consumer in [co1, cr1]:
            assert consumer.wait_push_event()
            msgs = consumer.list(tc.URI_BROADCAST, block=True)
            assert len(msgs) == 1
            assert msgs[0].payload == "msg4"

        assert not cr2.list(tc.URI_BROADCAST, block=True)

    def test_priority_failover(self, cluster: Cluster):
        """
        Verify that when highest priority consumers unsubscribe gradually, only
        the new highest priority consumers might receive messages.
        """

        proxy1, proxy2 = islice(cluster.proxy_cycle(), 2)
        producer = proxy1.create_client("producer")
        co1 = proxy1.create_client("CO1")
        cr1 = proxy2.create_client("CR1")
        cr2 = proxy2.create_client("CR2")

        producer.open(tc.URI_BROADCAST, flags=["write", "ack"], succeed=True)
        co1.open(tc.URI_BROADCAST, flags=["read"], succeed=True, consumer_priority=1)
        cr1.open(tc.URI_BROADCAST, flags=["read"], succeed=True, consumer_priority=2)
        cr2.open(tc.URI_BROADCAST, flags=["read"], succeed=True, consumer_priority=3)

        # CR2 is highest priority; only it should receive messages
        producer.post(tc.URI_BROADCAST, payload=["msg1"], succeed=True, wait_ack=True)

        assert cr2.wait_push_event()
        msgs = cr2.list(tc.URI_BROADCAST, block=True)
        assert len(msgs) == 1
        assert msgs[0].payload == "msg1"
        cr2.confirm(tc.URI_BROADCAST, "+1", succeed=True)

        for consumer in [co1, cr1]:
            assert not consumer.list(tc.URI_BROADCAST, block=True)

        # CR2 unsubscribes. Only CR1 should receive messages now
        cr2.close(tc.URI_BROADCAST, succeed=True)

        producer.post(tc.URI_BROADCAST, payload=["msg2"], succeed=True, wait_ack=True)

        assert cr1.wait_push_event()
        msgs = cr1.list(tc.URI_BROADCAST, block=True)
        assert len(msgs) == 1
        assert msgs[0].payload == "msg2"
        cr1.confirm(tc.URI_BROADCAST, "+1", succeed=True)

        assert not co1.list(tc.URI_BROADCAST, block=True)

        # CR1 unsubscribes. Only CO1 should receive messages now
        cr1.close(tc.URI_BROADCAST, succeed=True)

        producer.post(tc.URI_BROADCAST, payload=["msg3"], succeed=True, wait_ack=True)

        assert co1.wait_push_event()
        msgs = co1.list(tc.URI_BROADCAST, block=True)
        assert len(msgs) == 1
        assert msgs[0].payload == "msg3"

    def test_add_variable_priority_consumers(self, cluster: Cluster):
        """
        Verify that only the highest priority consumers receive messages as new
        consumers with variable priority are being added.
        """

        proxy1, proxy2 = islice(cluster.proxy_cycle(), 2)
        producer = proxy1.create_client("producer")
        co1 = proxy1.create_client("CO1")
        co2 = proxy1.create_client("CO2")
        co3 = proxy1.create_client("CO3")
        cr1 = proxy2.create_client("CR1")
        cr2 = proxy2.create_client("CR2")
        cr3 = proxy2.create_client("CR3")

        producer.open(tc.URI_BROADCAST, flags=["write", "ack"], succeed=True)

        # Add consumer with priority 2
        co1.open(tc.URI_BROADCAST, flags=["read"], succeed=True, consumer_priority=2)

        producer.post(tc.URI_BROADCAST, payload=["msg1"], succeed=True, wait_ack=True)

        assert co1.wait_push_event()
        msgs = co1.list(tc.URI_BROADCAST, block=True)
        assert len(msgs) == 1
        assert msgs[0].payload == "msg1"
        co1.confirm(tc.URI_BROADCAST, "+1", succeed=True)

        # Add consumer with priority 1
        co2.open(tc.URI_BROADCAST, flags=["read"], succeed=True, consumer_priority=1)

        producer.post(tc.URI_BROADCAST, payload=["msg2"], succeed=True, wait_ack=True)

        assert co1.wait_push_event()
        msgs = co1.list(tc.URI_BROADCAST, block=True)
        assert len(msgs) == 1
        assert msgs[0].payload == "msg2"
        co1.confirm(tc.URI_BROADCAST, "+1", succeed=True)

        assert not co2.list(tc.URI_BROADCAST, block=True)

        # Add consumer with priority 2
        co3.open(tc.URI_BROADCAST, flags=["read"], succeed=True, consumer_priority=2)

        producer.post(tc.URI_BROADCAST, payload=["msg3"], succeed=True, wait_ack=True)

        for consumer in [co1, co3]:
            assert consumer.wait_push_event()
            msgs = consumer.list(tc.URI_BROADCAST, block=True)
            assert len(msgs) == 1
            assert msgs[0].payload == "msg3"
            consumer.confirm(tc.URI_BROADCAST, "+1", succeed=True)

        assert not co2.list(tc.URI_BROADCAST, block=True)

        # Add consumer with priority 3
        cr1.open(tc.URI_BROADCAST, flags=["read"], succeed=True, consumer_priority=3)

        producer.post(tc.URI_BROADCAST, payload=["msg4"], succeed=True, wait_ack=True)

        assert cr1.wait_push_event()
        msgs = cr1.list(tc.URI_BROADCAST, block=True)
        assert len(msgs) == 1
        assert msgs[0].payload == "msg4"
        cr1.confirm(tc.URI_BROADCAST, "+1", succeed=True)

        for consumer in [co1, co2, co3]:
            assert not consumer.list(tc.URI_BROADCAST, block=True)

        # Add consumer with priority 5
        cr2.open(tc.URI_BROADCAST, flags=["read"], succeed=True, consumer_priority=5)

        producer.post(tc.URI_BROADCAST, payload=["msg5"], succeed=True, wait_ack=True)

        assert cr2.wait_push_event()
        msgs = cr2.list(tc.URI_BROADCAST, block=True)
        assert len(msgs) == 1
        assert msgs[0].payload == "msg5"
        cr2.confirm(tc.URI_BROADCAST, "+1", succeed=True)

        for consumer in [co1, co2, co3, cr1]:
            assert not consumer.list(tc.URI_BROADCAST, block=True)

        # Add consumer with priority 5
        cr3.open(tc.URI_BROADCAST, flags=["read"], succeed=True, consumer_priority=5)

        producer.post(tc.URI_BROADCAST, payload=["msg6"], succeed=True, wait_ack=True)

        for consumer in [cr2, cr3]:
            assert consumer.wait_push_event()
            msgs = consumer.list(tc.URI_BROADCAST, block=True)
            assert len(msgs) == 1
            assert msgs[0].payload == "msg6"

        for consumer in [co1, co2, co3, cr1]:
            assert not consumer.list(tc.URI_BROADCAST, block=True)
