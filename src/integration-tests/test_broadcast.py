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

import time
from itertools import islice

import blazingmq.dev.it.testconstants as tc
from blazingmq.dev.it.fixtures import (
    Cluster,
    order,
    tweak,
)
from blazingmq.dev.it.process.client import Client

pytestmark = order(3)


def test_breathing(
    cluster: Cluster,
    domain_urls: tc.DomainUrls,  # pylint: disable=unused-argument
):
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


def test_multi_consumers(
    cluster: Cluster,
    domain_urls: tc.DomainUrls,  # pylint: disable=unused-argument
):
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


def test_multi_producers_consumers(
    cluster: Cluster,
    domain_urls: tc.DomainUrls,  # pylint: disable=unused-argument
):
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
        producer.post(tc.URI_BROADCAST, payload=[f"msg{i}"], block=True, wait_ack=True)
        for consumer in consumers:
            assert consumer.wait_push_event()
            msgs = consumer.list(tc.URI_BROADCAST, block=True)
            assert len(msgs) == i
            assert msgs[i - 1].payload == f"msg{i}"


def test_resubscribe(
    cluster: Cluster,
    domain_urls: tc.DomainUrls,  # pylint: disable=unused-argument
):
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


def test_add_consumers(
    cluster: Cluster,
    domain_urls: tc.DomainUrls,  # pylint: disable=unused-argument
):
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
    producer.post(tc.URI_BROADCAST, payload=["null_msg"], succeed=True, wait_ack=True)

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


def test_dynamic_priorities(
    cluster: Cluster,
    domain_urls: tc.DomainUrls,  # pylint: disable=unused-argument
):
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


def test_priority_failover(
    cluster: Cluster,
    domain_urls: tc.DomainUrls,  # pylint: disable=unused-argument
):
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


def test_add_variable_priority_consumers(
    cluster: Cluster,
    domain_urls: tc.DomainUrls,  # pylint: disable=unused-argument
):
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


@tweak.domain.message_ttl(0)
def test_ttl_zero_delivers_to_subscribed_consumer(
    cluster: Cluster,
    domain_urls: tc.DomainUrls,  # pylint: disable=unused-argument
):
    """
    Verify that a broadcast domain configured with a message TTL of 0 seconds
    delivers every posted message to a subscribed consumer.

    A broadcast queue delivers a message on a best-effort basis to the
    consumers subscribed at delivery time, then removes it from its in-memory
    storage.  A broadcast queue removes messages only after delivery, never via
    TTL garbage collection, regardless of the configured TTL.

    Messages are posted continuously for several seconds, spanning many
    one-second boundaries, and the following hold throughout:
      * no node reports TTL garbage collection for the broadcast queue while a
        consumer is subscribed; and
      * the consumer receives every posted message.
    """

    # Duration of the posting phase.  Posting spans many one-second boundaries
    # to exercise TTL handling under continuous load.
    load_duration_seconds = 30

    # Number of messages posted back-to-back before the consumer is drained.
    post_burst = 50

    uri = tc.URI_BROADCAST

    proxy = next(cluster.proxy_cycle())
    producer = proxy.create_client("producer")
    consumer = proxy.create_client("consumer")

    producer.open(uri, flags=["write", "ack"], succeed=True)
    consumer.open(uri, flags=["read"], succeed=True)

    # Confirm the delivery path before the continuous-posting phase.
    producer.post(uri, payload=["warmup"], succeed=True, wait_ack=True)
    assert consumer.wait_push_event()
    consumer.list(uri, block=True)
    consumer.confirm(uri, "*", succeed=True)

    posted = []
    received = set()

    def drain(*, timeout):
        # Collect everything the consumer has received so far, confirming to
        # keep the unconfirmed set bounded.  Return as soon as every posted
        # message has been accounted for, or once the consumer stops pushing
        # for the specified 'timeout'.
        while len(received) < len(posted):
            if not consumer.wait_push_event(timeout=timeout, quiet=True):
                break
            msgs = consumer.list(uri, block=True)
            if not msgs:
                # The push was announced before the message became listable;
                # pick it up on the next round.
                continue
            for msg in msgs:
                received.add(msg.payload)
            # Confirm exactly the listed messages.  Both 'list' and '+N' walk
            # the unconfirmed messages oldest-first, so this cannot discard a
            # message that arrived after the listing was taken.
            consumer.confirm(uri, f"+{len(msgs)}", succeed=True)

    # Post messages continuously so that the broker is kept busy across many
    # one-second boundaries.
    deadline = time.time() + load_duration_seconds
    seq = 0
    while time.time() < deadline:
        for _ in range(post_burst):
            payload = f"m-{seq}"
            producer.post(uri, payload=[payload])
            posted.append(payload)
            seq += 1
        # The producer keeps the consumer busy, so a short silence means the
        # backlog is drained.
        drain(timeout=0.2)

    # Collect the remainder, tolerating a longer silence while in-flight
    # pushes settle.  In a good scenario it returns almost instantly due to all
    # messages being received.
    drain(timeout=2)

    # A broadcast queue removes messages only after delivery, so no node
    # reports TTL garbage collection for the queue while a consumer is
    # subscribed.
    for node in cluster.nodes():
        assert not node.erases_messages(uri, timeout=1), (
            f"node '{node.name}' garbage-collected broadcast messages due to "
            "TTL expiration while a consumer was subscribed"
        )

    missing = [payload for payload in posted if payload not in received]
    assert not missing, (
        f"{len(missing)} of {len(posted)} broadcast messages were never "
        f"delivered to a subscribed consumer (e.g. {missing[:5]})"
    )
