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
    multiversion_multi_node,
    multi_interface,
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


def test_verify_partial_close(multiversion_multi_node: Cluster):
    """Drop one of two producers both having unacked message (primary is
    suspended.  Make sure the remaining producer does not get NACK but gets
    ACK when primary resumes.
    """
    proxies = multiversion_multi_node.proxy_cycle()

    proxy = next(proxies)
    proxy = next(proxies)

    producer1 = proxy.create_client("producer1")
    producer1.open(tc.URI_FANOUT, flags=["write", "ack"], succeed=True)

    producer2 = proxy.create_client("producer2")
    producer2.open(tc.URI_FANOUT, flags=["write", "ack"], succeed=True)

    leader = multiversion_multi_node.last_known_leader
    leader.suspend()

    producer1.post(tc.URI_FANOUT, payload=["1"], succeed=True, wait_ack=False)
    producer2.post(tc.URI_FANOUT, payload=["2"], succeed=True, wait_ack=False)

    producer2.stop_session(block=True)

    leader.resume()

    producer1.capture(r"ACK #0: \[ type = ACK status = SUCCESS", 2)

    _stop_clients([producer1, producer2])


# @start_cluster(True, True, True)
# @tweak.cluster.queue_operations.open_timeout_ms(2)
# def test_command_timeout(multiversion_multi_node: Cluster):
#     """Simple test to execute onOpenQueueResponse timeout."""

#     # make sure the cluster is healthy and the queue is assigned
#     # Cannot use proxies as they do not read cluster config

#     leader = multiversion_multi_node.last_known_leader
#     host = multiversion_multi_node.nodes()[0]
#     if host == leader:
#         host = multiversion_multi_node.nodes()[1]

#     client = host.create_client("client")
#     # this may fail due to the short timeout; we just need queue assigned
#     client.open(tc.URI_FANOUT, flags=["write", "ack"], block=True)

#     leader.suspend()

#     result = client.open(tc.URI_FANOUT_FOO, flags=["read"], block=True)
#     leader.resume()

#     assert result == Client.e_TIMEOUT


# def test_queue_purge_command(multiversion_multi_node: Cluster):
#     """Ensure that 'queue purge' command is working as expected.  Post a
#     message to the queue, then purge the queue, then bring up a consumer.
#     Ensure that consumer does not receive any message.
#     """
#     proxy = next(multiversion_multi_node.proxy_cycle())

#     # Start a producer and post a message
#     producer = proxy.create_client("producer")
#     producer.open(tc.URI_FANOUT, flags=["write", "ack"], succeed=True)
#     producer.post(tc.URI_FANOUT, ["msg1"], succeed=True, wait_ack=True)

#     leader = multiversion_multi_node.last_known_leader

#     # Purge queue, but *only* for 'foo' appId
#     leader.command(f"DOMAINS DOMAIN {tc.DOMAIN_FANOUT} QUEUE {tc.TEST_QUEUE} PURGE foo")

#     # Open consumers for all appIds and ensure that the one with 'foo' appId
#     # does not receive the message, while other consumers do.
#     consumer1 = proxy.create_client("consumer1")
#     consumer1.open(tc.URI_FANOUT_FOO, flags=["read"], succeed=True)

#     consumer2 = proxy.create_client("consumer2")
#     consumer2.open(tc.URI_FANOUT_BAR, flags=["read"], succeed=True)

#     consumer3 = proxy.create_client("consumer3")
#     consumer3.open(tc.URI_FANOUT_BAZ, flags=["read"], succeed=True)

#     assert consumer2.wait_push_event()
#     msgs = consumer2.list(block=True)
#     assert len(msgs) == 1
#     assert msgs[0].payload == "msg1"

#     assert consumer3.wait_push_event()
#     msgs = consumer3.list(block=True)
#     assert len(msgs) == 1
#     assert msgs[0].payload == "msg1"

#     assert not consumer1.wait_push_event(timeout=5, quiet=True)
#     msgs = consumer1.list(block=True)
#     assert len(msgs) == 0

#     consumer2.confirm(tc.URI_FANOUT_BAR, "*", succeed=True)
#     consumer3.confirm(tc.URI_FANOUT_BAZ, "*", succeed=True)

#     # Stop all consumers, post another message, then purge entire queue
#     # (i.e., all appIds), then restart all consumers and ensure that none
#     # of them got any messages.
#     consumer1.close(tc.URI_FANOUT_FOO, succeed=True)
#     consumer2.close(tc.URI_FANOUT_BAR, succeed=True)
#     consumer3.close(tc.URI_FANOUT_BAZ, succeed=True)

#     producer.post(tc.URI_FANOUT, ["msg2"], succeed=True, wait_ack=True)

#     leader.command(f"DOMAINS DOMAIN {tc.DOMAIN_FANOUT} QUEUE {tc.TEST_QUEUE} PURGE *")

#     consumer1 = proxy.create_client("consumer1")
#     consumer1.open(tc.URI_FANOUT_FOO, flags=["read"], succeed=True)
#     consumer2 = proxy.create_client("consumer2")
#     consumer2.open(tc.URI_FANOUT_BAR, flags=["read"], succeed=True)
#     consumer3 = proxy.create_client("consumer3")
#     consumer3.open(tc.URI_FANOUT_BAZ, flags=["read"], succeed=True)

#     consumers = [consumer1, consumer2, consumer3]

#     for consumer in consumers:
#         assert not consumer.wait_push_event(timeout=2, quiet=True)
#         msgs = consumer.list(block=True)
#         assert len(msgs) == 0

