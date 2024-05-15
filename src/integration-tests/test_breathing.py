"""
This test suite exercises basic routing functionality to in the presence of all
types of queues.
"""

from collections import namedtuple

import blazingmq.dev.it.testconstants as tc
import pytest
from blazingmq.dev.it.fixtures import (  # pylint: disable=unused-import
    Cluster,
    cartesian_product_cluster,
    cluster,
    order,
    multi_node,
    start_cluster,
    tweak,
)
from blazingmq.dev.it.process.client import Client
from blazingmq.dev.it.util import wait_until

pytestmark = order(1)

BmqClient = namedtuple("BmqClient", "handle, uri")


def _close_clients(clients, uris):
    for client, uri in zip(clients, uris):
        assert client.close(uri, block=True) == Client.e_SUCCESS


def _stop_clients(clients):
    for client in clients:
        assert client.stop_session(block=True) == Client.e_SUCCESS


def _verify_delivery(consumer, uri, messages, timeout=2):
    consumer.wait_push_event()
    assert wait_until(
        lambda: len(consumer.list(uri, block=True)) == len(messages), timeout
    )
    consumer.list(uri, block=True)


def _verify_delivery_and_confirm(consumer, uri, messages):
    _verify_delivery(consumer, uri, messages)
    assert consumer.confirm(uri, "*", block=True) == Client.e_SUCCESS


def _verify_delivery_and_confirm_balanced(consumer, uris, messages, timeout=3):
    consumer.wait_push_event()

    def wait_cond():
        return sum(map(lambda u: len(consumer.list(u, block=True)), uris)) == len(
            messages
        )

    assert wait_until(wait_cond, timeout)

    msgs = []
    for uri in uris:
        uri_msgs = consumer.list(uri, block=True)

        # Ensure each uri has received part of messages
        assert len(uri_msgs) > 0

        msgs.extend(uri_msgs)

    # We cannot rely on the order of incoming messages so we just sort both lists
    messages.sort()
    msgs.sort(key=lambda msg: msg.payload)

    for i, message in enumerate(messages):
        assert msgs[i].payload == message

    for uri in uris:
        assert consumer.confirm(uri, "*", block=True) == Client.e_SUCCESS


def _verify_max_messages_max_bytes_routing(producer, consumer, other_consumers):
    # Verify no messages when we start

    try:
        assert len(consumer.handle.list(consumer.uri, block=True)) == 0
    except RuntimeError:
        pass  # No messages, that's what we want

    # verify maxUnconfirmedBytes. Post message exceeding max
    assert (
        producer.handle.post(
            producer.uri,
            payload=["123"],
            block=True,
            wait_ack=True,
        )
        == Client.e_SUCCESS
    )

    _verify_delivery(consumer.handle, consumer.uri, ["123"])
    # not confirming

    for anotherConsumer in other_consumers:
        _verify_delivery_and_confirm(
            anotherConsumer.handle, anotherConsumer.uri, ["123"]
        )

    assert (
        producer.handle.post(producer.uri, payload=["1"], block=True, wait_ack=True)
        == Client.e_SUCCESS
    )

    for anotherConsumer in other_consumers:
        _verify_delivery_and_confirm(anotherConsumer.handle, anotherConsumer.uri, ["1"])

    # consumer is over maxUnconfirmedBytes (3)
    # assert no PUSH received within 1 second
    assert not consumer.handle.outputs_regex("MESSAGE.*PUSH", timeout=1)
    msgs = consumer.handle.list(consumer.uri, block=True)
    assert len(msgs) == 1
    assert msgs[0].payload == "123"

    assert consumer.handle.confirm(consumer.uri, "*", block=True) == Client.e_SUCCESS

    # onHandleUsable kicks in
    _verify_delivery(consumer.handle, consumer.uri, ["1"])
    # not confirming

    # verify maxUnconfirmedMessages
    assert (
        producer.handle.post(producer.uri, payload=["2"], block=True, wait_ack=True)
        == Client.e_SUCCESS
    )

    for anotherConsumer in other_consumers:
        _verify_delivery_and_confirm(anotherConsumer.handle, anotherConsumer.uri, ["2"])

    _verify_delivery(consumer.handle, consumer.uri, ["1", "2"])
    # not confirming

    assert (
        producer.handle.post(producer.uri, payload=["3"], block=True, wait_ack=True)
        == Client.e_SUCCESS
    )

    for anotherConsumer in other_consumers:
        _verify_delivery_and_confirm(anotherConsumer.handle, anotherConsumer.uri, ["3"])

    # consumer is over maxUnconfirmedMessages (2)
    # assert no PUSH received within 1 second
    assert not consumer.handle.outputs_regex("MESSAGE.*PUSH", timeout=1)
    msgs = consumer.handle.list(consumer.uri, block=True)
    assert len(msgs) == 2
    assert msgs[0].payload == "1"
    assert msgs[1].payload == "2"

    assert consumer.handle.confirm(consumer.uri, "*", block=True) == Client.e_SUCCESS

    # onHandleUsable kicks in
    _verify_delivery(consumer.handle, consumer.uri, ["3"])


def _verify_priority_routing(producers, consumers, lowPriorityConsumers):

    # Verify no messages when we start
    for consumer in consumers + lowPriorityConsumers:
        try:
            assert len(consumer.list(tc.URI_PRIORITY, block=True)) == 0
        except RuntimeError:
            pass  # No messages, that's what we want

    # Route messages and verify
    for producer in producers:
        assert (
            producer.post(tc.URI_PRIORITY, payload=["msg"], block=True, wait_ack=True)
            == Client.e_SUCCESS
        )

    for consumer in consumers:
        _verify_delivery_and_confirm(consumer, tc.URI_PRIORITY, ["msg"])

    for consumer in lowPriorityConsumers:
        # assert no PUSH received within 1 second
        assert not consumer.outputs_regex("MESSAGE.*PUSH", timeout=1)
        assert not consumer.list(tc.URI_PRIORITY, block=True)


def test_open_queue(cartesian_product_cluster: Cluster):
    cluster = cartesian_product_cluster
    [consumer] = cluster.open_priority_queues(1, flags=["read"])
    [producer] = cluster.open_priority_queues(1, flags=["write"])
    producer.post(payload=["foo"], succeed=True)
    consumer.client.wait_push_event()
    msgs = consumer.list(block=True)
    assert len(msgs) == 1
    assert msgs[0].payload == "foo"


def test_verify_priority(cluster: Cluster):
    proxies = cluster.proxy_cycle()

    # 1: Setup producers and consumers
    # Proxy in same datacenter as leader/primary
    proxy1 = next(proxies)

    producer1 = proxy1.create_client("producer1")
    assert (
        producer1.open(tc.URI_PRIORITY, flags=["write", "ack"], block=True)
        == Client.e_SUCCESS
    )

    consumer1 = proxy1.create_client("consumer1")
    assert (
        consumer1.open(tc.URI_PRIORITY, flags=["read"], consumer_priority=2, block=True)
        == Client.e_SUCCESS
    )

    # Replica proxy
    proxy2 = next(proxies)

    producer2 = proxy2.create_client("producer2")
    assert (
        producer2.open(tc.URI_PRIORITY, flags=["write", "ack"], block=True)
        == Client.e_SUCCESS
    )

    consumer2 = proxy2.create_client("consumer2")
    assert (
        consumer2.open(tc.URI_PRIORITY, flags=["read"], consumer_priority=2, block=True)
        == Client.e_SUCCESS
    )

    consumer3 = proxy1.create_client("consumer3")
    assert (
        consumer3.open(tc.URI_PRIORITY, flags=["read"], consumer_priority=1, block=True)
        == Client.e_SUCCESS
    )

    consumer4 = proxy2.create_client("consumer4")
    assert (
        consumer4.open(tc.URI_PRIORITY, flags=["read"], consumer_priority=1, block=True)
        == Client.e_SUCCESS
    )

    # 2: Route messages and verify
    _verify_priority_routing(
        [producer1, producer2], [consumer1, consumer2], [consumer3, consumer4]
    )

    # 3: Close everything
    _close_clients(
        [producer1, consumer1, producer2, consumer2, consumer3, consumer4],
        [tc.URI_PRIORITY],
    )
    _stop_clients([producer1, consumer1, producer2, consumer2, consumer3, consumer4])

    # 4: Repeat the test with reeverse order of opening clients (consumers
    #    first).
    consumer1 = proxy1.create_client("consumer1")
    assert (
        consumer1.open(tc.URI_PRIORITY, flags=["read"], consumer_priority=2, block=True)
        == Client.e_SUCCESS
    )

    consumer2 = proxy2.create_client("consumer2")
    assert (
        consumer2.open(tc.URI_PRIORITY, flags=["read"], consumer_priority=2, block=True)
        == Client.e_SUCCESS
    )

    consumer3 = proxy1.create_client("consumer3")
    assert (
        consumer3.open(tc.URI_PRIORITY, flags=["read"], consumer_priority=1, block=True)
        == Client.e_SUCCESS
    )

    consumer4 = proxy2.create_client("consumer4")
    assert (
        consumer4.open(tc.URI_PRIORITY, flags=["read"], consumer_priority=1, block=True)
        == Client.e_SUCCESS
    )

    producer2 = proxy2.create_client("producer2")
    assert (
        producer2.open(tc.URI_PRIORITY, flags=["write", "ack"], block=True)
        == Client.e_SUCCESS
    )

    producer1 = proxy1.create_client("producer1")
    assert (
        producer1.open(tc.URI_PRIORITY, flags=["write", "ack"], block=True)
        == Client.e_SUCCESS
    )

    # 5: Route messages and verify
    _verify_priority_routing(
        [producer1, producer2], [consumer1, consumer2], [consumer3, consumer4]
    )

    # 6: test maxUnconfirmedMessages, maxUnconfirmedBytes
    assert (
        consumer2.configure(
            tc.URI_PRIORITY,
            consumer_priority=3,
            max_unconfirmed_messages=2,
            max_unconfirmed_bytes=3,
            block=True,
        )
        == Client.e_SUCCESS
    )

    _verify_max_messages_max_bytes_routing(
        BmqClient(producer2, tc.URI_PRIORITY),
        BmqClient(consumer2, tc.URI_PRIORITY),
        [],
    )

    # 7: Close everything
    _close_clients(
        [producer1, consumer1, producer2, consumer2, consumer3, consumer4],
        [tc.URI_PRIORITY],
    )
    _stop_clients([producer1, consumer1, producer2, consumer2, consumer3, consumer4])


def test_verify_fanout(cluster: Cluster):
    # 1: Setup producers and consumers
    proxies = cluster.proxy_cycle()

    # Proxy in same datacenter as leader/primary
    proxy1 = next(proxies)

    fooConsumerAndProducerOnPrimaryProxy = proxy1.create_client(
        "fooConsumerAndProducerOnPrimaryProxy"
    )

    # testing {client1 open "foo" for read, client2 open "bar" for read,
    #          client1 open for write} sequence (RDSIBMQ-1008).

    assert (
        fooConsumerAndProducerOnPrimaryProxy.open(
            tc.URI_FANOUT_FOO, flags=["read"], block=True
        )
        == Client.e_SUCCESS
    )

    barConsumerOnPrimaryProxy = proxy1.create_client("barConsumerOnPrimaryProxy")
    assert (
        barConsumerOnPrimaryProxy.open(tc.URI_FANOUT_BAR, flags=["read"], block=True)
        == Client.e_SUCCESS
    )

    assert (
        fooConsumerAndProducerOnPrimaryProxy.open(
            tc.URI_FANOUT, flags=["write", "ack"], block=True
        )
        == Client.e_SUCCESS
    )

    assert (
        barConsumerOnPrimaryProxy.close(tc.URI_FANOUT_BAR, block=True)
        == Client.e_SUCCESS
    )

    # Replica proxy
    proxy2 = next(proxies)

    producerOnReplicaProxy = proxy2.create_client("producerOnReplicaProxy")
    assert (
        producerOnReplicaProxy.open(tc.URI_FANOUT, flags=["write", "ack"], block=True)
        == Client.e_SUCCESS
    )

    barConsumerOnReplicaProxy = proxy2.create_client("barConsumerOnReplicaProxy")
    assert (
        barConsumerOnReplicaProxy.open(tc.URI_FANOUT_BAR, flags=["read"], block=True)
        == Client.e_SUCCESS
    )

    assert (
        len(fooConsumerAndProducerOnPrimaryProxy.list(tc.URI_FANOUT_FOO, block=True))
        == 0
    )
    assert len(barConsumerOnReplicaProxy.list(tc.URI_FANOUT_BAR, block=True)) == 0

    # 2: Route messages and verify
    assert (
        fooConsumerAndProducerOnPrimaryProxy.post(
            tc.URI_FANOUT, payload=["msg1"], block=True, wait_ack=True
        )
        == Client.e_SUCCESS
    )

    _verify_delivery_and_confirm(
        fooConsumerAndProducerOnPrimaryProxy, tc.URI_FANOUT_FOO, ["msg1"]
    )

    _verify_delivery_and_confirm(barConsumerOnReplicaProxy, tc.URI_FANOUT_BAR, ["msg1"])

    assert (
        producerOnReplicaProxy.post(
            tc.URI_FANOUT, payload=["msg2"], block=True, wait_ack=True
        )
        == Client.e_SUCCESS
    )

    _verify_delivery_and_confirm(
        fooConsumerAndProducerOnPrimaryProxy, tc.URI_FANOUT_FOO, ["msg2"]
    )

    _verify_delivery_and_confirm(barConsumerOnReplicaProxy, tc.URI_FANOUT_BAR, ["msg2"])

    # 3: test maxUnconfirmedMessages, maxUnconfirmedBytes
    assert (
        barConsumerOnReplicaProxy.configure(
            tc.URI_FANOUT_BAR,
            max_unconfirmed_messages=2,
            max_unconfirmed_bytes=3,
            block=True,
        )
        == Client.e_SUCCESS
    )

    _verify_max_messages_max_bytes_routing(
        BmqClient(handle=producerOnReplicaProxy, uri=tc.URI_FANOUT),
        BmqClient(handle=barConsumerOnReplicaProxy, uri=tc.URI_FANOUT_BAR),
        [BmqClient(handle=fooConsumerAndProducerOnPrimaryProxy, uri=tc.URI_FANOUT_FOO)],
    )

    # 4: Close everything
    _close_clients(
        [
            fooConsumerAndProducerOnPrimaryProxy,
            fooConsumerAndProducerOnPrimaryProxy,
            producerOnReplicaProxy,
            barConsumerOnReplicaProxy,
        ],
        [tc.URI_FANOUT, tc.URI_FANOUT_FOO, tc.URI_FANOUT, tc.URI_FANOUT_BAR],
    )

    _stop_clients(
        [
            fooConsumerAndProducerOnPrimaryProxy,
            producerOnReplicaProxy,
            barConsumerOnReplicaProxy,
        ]
    )


def test_verify_broadcast(cluster: Cluster):
    # 1: Setup producers and consumers
    proxies = cluster.proxy_cycle()

    # Proxy in same datacenter as leader/primary
    proxy1 = next(proxies)

    producer1 = proxy1.create_client("producer1")
    assert (
        producer1.open(tc.URI_BROADCAST, flags=["write", "ack"], block=True)
        == Client.e_SUCCESS
    )

    consumer1 = proxy1.create_client("consumer1")
    assert (
        consumer1.open(tc.URI_BROADCAST, flags=["read"], block=True) == Client.e_SUCCESS
    )

    # Replica proxy
    proxy2 = next(proxies)

    producer2 = proxy2.create_client("producer2")
    assert (
        producer2.open(tc.URI_BROADCAST, flags=["write", "ack"], block=True)
        == Client.e_SUCCESS
    )

    consumer2 = proxy2.create_client("consumer2")
    assert (
        consumer2.open(tc.URI_BROADCAST, flags=["read"], block=True) == Client.e_SUCCESS
    )

    assert len(consumer1.list(tc.URI_BROADCAST, block=True)) == 0
    assert len(consumer2.list(tc.URI_BROADCAST, block=True)) == 0

    # 2: Route messages and verify
    assert (
        producer1.post(tc.URI_BROADCAST, payload=["msg1"], block=True, wait_ack=True)
        == Client.e_SUCCESS
    )

    _verify_delivery_and_confirm(consumer1, tc.URI_BROADCAST, ["msg1"])

    _verify_delivery_and_confirm(consumer2, tc.URI_BROADCAST, ["msg1"])

    assert (
        producer2.post(tc.URI_BROADCAST, payload=["msg2"], block=True, wait_ack=True)
        == Client.e_SUCCESS
    )

    _verify_delivery_and_confirm(consumer1, tc.URI_BROADCAST, ["msg2"])

    _verify_delivery_and_confirm(consumer2, tc.URI_BROADCAST, ["msg2"])

    # 4: Close everything
    _close_clients([producer1, consumer1, producer2, consumer2], [tc.URI_BROADCAST])

    _stop_clients([producer1, consumer1, producer2, consumer2])


def test_verify_redelivery(cluster: Cluster):
    """Drop one consumer having unconfirmed message while there is another
    consumer unable to take the message (due to max_unconfirmed_messages
    limit).  Then start new consumer and make sure it does not crash (DRQS
    156808957) and receives that unconfirmed message.
    """
    proxies = cluster.proxy_cycle()

    # Proxy in same datacenter as leader/primary
    proxy = next(proxies)

    producer = proxy.create_client("producer1")
    producer.open(tc.URI_FANOUT, flags=["write", "ack"], succeed=True)

    consumer1 = proxy.create_client("consumer1")
    consumer1.open(
        tc.URI_FANOUT_FOO,
        flags=["read"],
        consumer_priority=1,
        max_unconfirmed_messages=1,
        succeed=True,
    )

    consumer2 = proxy.create_client("consumer2")
    consumer2.open(
        tc.URI_FANOUT_FOO,
        flags=["read"],
        consumer_priority=1,
        max_unconfirmed_messages=1,
        succeed=True,
    )

    producer.post(tc.URI_FANOUT, payload=["1"], succeed=True, wait_ack=True)
    producer.post(tc.URI_FANOUT, payload=["2"], succeed=True, wait_ack=True)

    consumer1.wait_push_event()
    before = consumer1.list(tc.URI_FANOUT_FOO, block=True)

    consumer2.wait_push_event()

    consumer1.stop_session(block=True)

    consumer1 = proxy.create_client("consumer1")
    consumer1.open(
        tc.URI_FANOUT_FOO,
        flags=["read"],
        consumer_priority=1,
        max_unconfirmed_messages=1,
        succeed=True,
    )

    consumer1.wait_push_event()
    after = consumer1.list(tc.URI_FANOUT_FOO, block=True)

    assert before[0].payload == after[0].payload

    _stop_clients([producer, consumer1, consumer2])


def test_verify_priority_queue_redelivery(cluster: Cluster):
    """Restart consumer having unconfirmed messages while a producer is
    still present (queue context is not erased).  Make sure the consumer
    receives the unconfirmed messages.
    """
    proxies = cluster.proxy_cycle()

    # Proxy in same datacenter as leader/primary
    proxy = next(proxies)

    producer = proxy.create_client("producer")
    producer.open(tc.URI_PRIORITY, flags=["write", "ack"], succeed=True)

    consumer = proxy.create_client("consumer")
    consumer.open(
        tc.URI_PRIORITY,
        flags=["read"],
        consumer_priority=1,
        max_unconfirmed_messages=1,
        succeed=True,
    )

    producer.post(tc.URI_PRIORITY, payload=["1"], succeed=True, wait_ack=True)
    producer.post(tc.URI_PRIORITY, payload=["2"], succeed=True, wait_ack=True)

    consumer.wait_push_event()
    before = consumer.list(tc.URI_PRIORITY, block=True)

    consumer.stop_session(block=True)

    consumer = proxy.create_client("consumer")
    consumer.open(
        tc.URI_PRIORITY,
        flags=["read"],
        consumer_priority=1,
        max_unconfirmed_messages=1,
        succeed=True,
    )

    consumer.wait_push_event()
    after = consumer.list(tc.URI_PRIORITY, block=True)

    assert before == after

    _stop_clients([producer, consumer])


def test_verify_partial_close(multi_node: Cluster):
    """Drop one of two producers both having unacked message (primary is
    suspended.  Make sure the remaining producer does not get NACK but gets
    ACK when primary resumes.
    """
    proxies = multi_node.proxy_cycle()

    proxy = next(proxies)
    proxy = next(proxies)

    producer1 = proxy.create_client("producer1")
    producer1.open(tc.URI_FANOUT, flags=["write", "ack"], succeed=True)

    producer2 = proxy.create_client("producer2")
    producer2.open(tc.URI_FANOUT, flags=["write", "ack"], succeed=True)

    leader = multi_node.last_known_leader
    leader.suspend()

    producer1.post(tc.URI_FANOUT, payload=["1"], succeed=True, wait_ack=False)
    producer2.post(tc.URI_FANOUT, payload=["2"], succeed=True, wait_ack=False)

    producer2.stop_session(block=True)

    leader.resume()

    producer1.capture(r"ACK #0: \[ type = ACK status = SUCCESS", 2)

    _stop_clients([producer1, producer2])


@start_cluster(True, True, True)
@tweak.cluster.queue_operations.open_timeout_ms(2)
def test_command_timeout(multi_node: Cluster):
    """Simple test to execute onOpenQueueResponse timeout."""

    # make sure the cluster is healthy and the queue is assigned
    # Cannot use proxies as they do not read cluster config

    leader = multi_node.last_known_leader
    host = multi_node.nodes()[0]
    if host == leader:
        host = multi_node.nodes()[1]

    client = host.create_client("client")
    # this may fail due to the short timeout; we just need queue assigned
    client.open(tc.URI_FANOUT, flags=["write", "ack"], block=True)

    leader.suspend()

    result = client.open(tc.URI_FANOUT_FOO, flags=["read"], block=True)
    leader.resume()

    assert result == Client.e_TIMEOUT


def test_queue_purge_command(multi_node: Cluster):
    """Ensure that 'queue purge' command is working as expected.  Post a
    message to the queue, then purge the queue, then bring up a consumer.
    Ensure that consumer does not receive any message.
    """
    proxy = next(multi_node.proxy_cycle())

    # Start a producer and post a message
    producer = proxy.create_client("producer")
    producer.open(tc.URI_FANOUT, flags=["write", "ack"], succeed=True)
    producer.post(tc.URI_FANOUT, ["msg1"], succeed=True, wait_ack=True)

    leader = multi_node.last_known_leader

    # Purge queue, but *only* for 'foo' appId
    leader.command(f"DOMAINS DOMAIN {tc.DOMAIN_FANOUT} QUEUE {tc.TEST_QUEUE} PURGE foo")

    # Open consumers for all appIds and ensure that the one with 'foo' appId
    # does not receive the message, while other consumers do.
    consumer1 = proxy.create_client("consumer1")
    consumer1.open(tc.URI_FANOUT_FOO, flags=["read"], succeed=True)

    consumer2 = proxy.create_client("consumer2")
    consumer2.open(tc.URI_FANOUT_BAR, flags=["read"], succeed=True)

    consumer3 = proxy.create_client("consumer3")
    consumer3.open(tc.URI_FANOUT_BAZ, flags=["read"], succeed=True)

    assert consumer2.wait_push_event()
    msgs = consumer2.list(block=True)
    assert len(msgs) == 1
    assert msgs[0].payload == "msg1"

    assert consumer3.wait_push_event()
    msgs = consumer3.list(block=True)
    assert len(msgs) == 1
    assert msgs[0].payload == "msg1"

    assert not consumer1.wait_push_event(timeout=5, quiet=True)
    msgs = consumer1.list(block=True)
    assert len(msgs) == 0

    consumer2.confirm(tc.URI_FANOUT_BAR, "*", succeed=True)
    consumer3.confirm(tc.URI_FANOUT_BAZ, "*", succeed=True)

    # Stop all consumers, post another message, then purge entire queue
    # (i.e., all appIds), then restart all consumers and ensure that none
    # of them got any messages.
    consumer1.close(tc.URI_FANOUT_FOO, succeed=True)
    consumer2.close(tc.URI_FANOUT_BAR, succeed=True)
    consumer3.close(tc.URI_FANOUT_BAZ, succeed=True)

    producer.post(tc.URI_FANOUT, ["msg2"], succeed=True, wait_ack=True)

    leader.command(f"DOMAINS DOMAIN {tc.DOMAIN_FANOUT} QUEUE {tc.TEST_QUEUE} PURGE *")

    consumer1 = proxy.create_client("consumer1")
    consumer1.open(tc.URI_FANOUT_FOO, flags=["read"], succeed=True)
    consumer2 = proxy.create_client("consumer2")
    consumer2.open(tc.URI_FANOUT_BAR, flags=["read"], succeed=True)
    consumer3 = proxy.create_client("consumer3")
    consumer3.open(tc.URI_FANOUT_BAZ, flags=["read"], succeed=True)

    consumers = [consumer1, consumer2, consumer3]

    for consumer in consumers:
        assert not consumer.wait_push_event(timeout=2, quiet=True)
        msgs = consumer.list(block=True)
        assert len(msgs) == 0


def test_message_properties(cluster: Cluster):
    """Ensure that posting different sequences of MessageProperties works."""
    proxies = cluster.proxy_cycle()

    # 1: Setup producers and consumers
    # Proxy in same datacenter as leader/primary
    proxy1 = next(proxies)

    producer1 = proxy1.create_client("producer1")
    assert (
        producer1.open(tc.URI_PRIORITY, flags=["write", "ack"], block=True)
        == Client.e_SUCCESS
    )

    consumer = proxy1.create_client("consumer")
    assert (
        consumer.open(tc.URI_PRIORITY, flags=["read"], block=True) == Client.e_SUCCESS
    )

    # Replica proxy
    proxy2 = next(proxies)

    producer2 = proxy2.create_client("producer2")
    assert (
        producer2.open(tc.URI_PRIORITY, flags=["write", "ack"], block=True)
        == Client.e_SUCCESS
    )

    # 2: Route messages and verify

    assert (
        producer1.post(
            tc.URI_PRIORITY,
            payload=["msg"],
            block=True,
            wait_ack=True,
            messageProperties=[],
        )
        == Client.e_SUCCESS
    )

    _verify_delivery_and_confirm(consumer, tc.URI_PRIORITY, ["msg"])

    assert (
        producer1.post(
            tc.URI_PRIORITY,
            payload=["msg"],
            block=True,
            wait_ack=True,
            messageProperties=[
                {"name": "pairs_", "value": "3", "type": "E_INT32"},
                {"name": "p1", "value": "1", "type": "E_INT32"},
                {"name": "p1_value", "value": "1", "type": "E_INT32"},
                {"name": "p3", "value": "1", "type": "E_INT32"},
                {"name": "p3_value", "value": "1", "type": "E_INT32"},
                {"name": "p4", "value": "1", "type": "E_STRING"},
                {"name": "p4_value", "value": "1", "type": "E_STRING"},
            ],
        )
        == Client.e_SUCCESS
    )

    _verify_delivery_and_confirm(consumer, tc.URI_PRIORITY, ["msg"])

    assert (
        producer1.post(
            tc.URI_PRIORITY,
            payload=["msg"],
            block=True,
            wait_ack=True,
            messageProperties=[
                {"name": "pairs_", "value": "4", "type": "E_INT32"},
                {"name": "p1", "value": "1", "type": "E_INT32"},
                {"name": "p1_value", "value": "1", "type": "E_INT32"},
                {"name": "p2", "value": "1", "type": "E_STRING"},
                {"name": "p2_value", "value": "1", "type": "E_STRING"},
                {"name": "p3", "value": "1", "type": "E_INT32"},
                {"name": "p3_value", "value": "1", "type": "E_INT32"},
                {"name": "p4", "value": "1", "type": "E_STRING"},
                {"name": "p4_value", "value": "1", "type": "E_STRING"},
            ],
        )
        == Client.e_SUCCESS
    )

    _verify_delivery_and_confirm(consumer, tc.URI_PRIORITY, ["msg"])

    assert (
        producer1.post(
            tc.URI_PRIORITY,
            payload=["msg"],
            block=True,
            wait_ack=True,
            messageProperties=[
                {"name": "pairs_", "value": "3", "type": "E_INT32"},
                {"name": "p1", "value": "1", "type": "E_INT32"},
                {"name": "p1_value", "value": "1", "type": "E_INT32"},
                {"name": "p3", "value": "1", "type": "E_INT32"},
                {"name": "p3_value", "value": "1", "type": "E_INT32"},
                {"name": "p4", "value": "1", "type": "E_STRING"},
                {"name": "p4_value", "value": "1", "type": "E_STRING"},
            ],
        )
        == Client.e_SUCCESS
    )

    _verify_delivery_and_confirm(consumer, tc.URI_PRIORITY, ["msg"])

    assert (
        producer1.post(
            tc.URI_PRIORITY,
            payload=["msg"],
            block=True,
            wait_ack=True,
            messageProperties=[
                {"name": "pairs_", "value": "4", "type": "E_INT32"},
                {"name": "p1", "value": "1", "type": "E_INT32"},
                {"name": "p1_value", "value": "1", "type": "E_INT32"},
                {"name": "p2", "value": "1", "type": "E_STRING"},
                {"name": "p2_value", "value": "1", "type": "E_STRING"},
                {"name": "p3", "value": "1", "type": "E_INT32"},
                {"name": "p3_value", "value": "1", "type": "E_INT32"},
                {"name": "p4", "value": "1", "type": "E_STRING"},
                {"name": "p4_value", "value": "1", "type": "E_STRING"},
            ],
        )
        == Client.e_SUCCESS
    )

    _verify_delivery_and_confirm(consumer, tc.URI_PRIORITY, ["msg"])

    # 3: Close everything
    _close_clients(
        [producer1, consumer, producer2],
        [tc.URI_PRIORITY],
    )
    _stop_clients([producer1, consumer, producer2])
