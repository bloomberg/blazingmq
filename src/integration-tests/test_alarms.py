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
Testing broker ALARMS.
"""

import time

import blazingmq.dev.it.testconstants as tc
from blazingmq.dev.it.fixtures import (
    Cluster,
    cluster,
    order,
    tweak,
)  # pylint: disable=unused-import


# -------------------------
# Consumption monitor tests
# -------------------------


@tweak.domain.max_idle_time(0)
def test_no_alarms_if_disabled(cluster: Cluster, domain_urls: tc.DomainUrls):
    """
    Test no broker ALARMS if alarm is disabled (max idle time is seto to zero).
    """
    uri_priority = domain_urls.uri_priority
    leader = cluster.last_known_leader
    proxy = next(cluster.proxy_cycle())

    producer = proxy.create_client("producer")
    producer.open(uri_priority, flags=["write,ack"], succeed=True)

    # Create consumer but not open the queue
    proxy.create_client("consumer")

    producer.post(uri_priority, ["msg1"], succeed=True, wait_ack=True)

    # Wait some time and check no alarm is triggered
    assert not leader.alarms("QUEUE_STUCK", 2)


@tweak.domain.max_idle_time(1)
def test_broadcast_no_alarms(cluster: Cluster, domain_urls: tc.DomainUrls):  # pylint: disable=unused-argument
    """
    Test no broker ALARMS in broadcast mode.
    """
    leader = cluster.last_known_leader
    proxy = next(cluster.proxy_cycle())

    # Create a producer and no consumers
    producer = proxy.create_client("producer")
    producer.open(tc.URI_BROADCAST, flags=["write,ack"], succeed=True)

    # Post a message
    producer.post(tc.URI_BROADCAST, ["msg1"], succeed=True, wait_ack=True)

    # Wait more than max idle time and check no alarm is triggered
    assert not leader.alarms("QUEUE_STUCK", 2)


# ---------------------------------------
# Consumption monitor priority mode tests
# ---------------------------------------


@tweak.domain.max_idle_time(3)
def test_priority_no_alarms_for_a_slow_queue(
    cluster: Cluster, domain_urls: tc.DomainUrls
):
    """
    Test no broker ALARMS in priority mode for a slowly moving queue.
    """
    uri_priority = domain_urls.uri_priority
    leader = cluster.last_known_leader
    proxy = next(cluster.proxy_cycle())

    producer = proxy.create_client("producer")
    producer.open(uri_priority, flags=["write,ack"], succeed=True)

    consumer = proxy.create_client("consumer")
    consumer.open(
        uri_priority, flags=["read"], max_unconfirmed_messages=1, succeed=True
    )

    # post 3 messages
    producer.post(uri_priority, ["msg1"], succeed=True, wait_ack=True)
    producer.post(uri_priority, ["msg2"], succeed=True, wait_ack=True)
    producer.post(uri_priority, ["msg3"], succeed=True, wait_ack=True)

    # Simulate a slow consumer by not confirming the message immediately
    time.sleep(1)
    consumer.confirm(uri_priority, "+1", succeed=True)
    consumer.wait_push_event()

    time.sleep(1)
    consumer.confirm(uri_priority, "+1", succeed=True)
    consumer.wait_push_event()

    # Test that no alarm is triggered
    assert not leader.alarms("QUEUE_STUCK", 1)


@tweak.domain.max_idle_time(1)
def test_priority_transition_active_alarm_active(
    cluster: Cluster, domain_urls: tc.DomainUrls
):
    """
    Test transition from active to alarm state and then back to active state in priority mode.
    """
    uri_priority = domain_urls.uri_priority
    leader = cluster.last_known_leader
    proxy = next(cluster.proxy_cycle())

    # Create producer and consumer
    producer = proxy.create_client("producer")
    producer.open(uri_priority, flags=["write,ack"], succeed=True)

    consumer = proxy.create_client("consumer")
    consumer.open(
        uri_priority, flags=["read"], max_unconfirmed_messages=1, succeed=True
    )

    # Post "msg1", it is delivered to consumer
    producer.post(uri_priority, ["msg1"], succeed=True, wait_ack=True)

    # Wait more than max idle time and check no alarm is triggered
    assert not leader.alarms("QUEUE_STUCK", 2)

    # Post "msg2", it is not delivered to consumer due to max_unconfirmed_messages=1
    producer.post(uri_priority, ["msg2"], succeed=True, wait_ack=True)

    # Wait more than max idle and check that alarm is triggered
    assert leader.alarms("QUEUE_STUCK", 2)
    assert leader.capture(r"For appId: __default")
    leader.drain()

    # Confirm messages, queue is empty
    consumer.confirm(uri_priority, "*", succeed=True)
    consumer.wait_push_event()

    # Test that queue is no longer in alarm state
    assert leader.outputs_regex(r"no longer appears to be stuck.", 1)


@tweak.domain.max_idle_time(1)
def test_priority_alarm_when_consumer_dropped(
    cluster: Cluster, domain_urls: tc.DomainUrls
):
    """
    Test that alarm is triggered when consumer droppped the connection in priority mode.
    """
    uri_priority = domain_urls.uri_priority
    leader = cluster.last_known_leader
    proxy = next(cluster.proxy_cycle())

    # Create producer and consumer
    producer = proxy.create_client("producer")
    producer.open(uri_priority, flags=["write,ack"], succeed=True)

    consumer = proxy.create_client("consumer")
    consumer.open(uri_priority, flags=["read"], succeed=True)

    # Post "msg1", it is delivered to consumer
    producer.post(uri_priority, ["msg1"], succeed=True, wait_ack=True)

    # Consumer dropping the connection, msg1 is in redelivery list
    consumer.stop_session(block=True)

    # Wait more than max idle and check that alarm is triggered
    assert leader.alarms("QUEUE_STUCK", 2)
    assert leader.capture(r"For appId: __default")


@tweak.domain.max_idle_time(1)
def test_priority_enable_disable_alarm(cluster: Cluster, domain_urls: tc.DomainUrls):
    """
    Test that alarm is triggered if consumption monitor is enabled (maxIdleTime > 0) in priority mode.
    Then reconfigure domain to disable consumption monitor (maxIdleTime = 0) and check that no alarm is triggered.
    """
    du = domain_urls
    leader = cluster.last_known_leader
    proxy = next(cluster.proxy_cycle())

    # Create producer and consumer, but consumer doesn't open a queue
    producer = proxy.create_client("producer")
    producer.open(du.uri_priority, flags=["write,ack"], succeed=True)

    consumer = proxy.create_client("consumer")

    # Post a message
    producer.post(du.uri_priority, ["msg1"], succeed=True, wait_ack=True)

    # Wait more than max idle and check that alarm is triggered
    assert leader.alarms("QUEUE_STUCK", 2)
    assert leader.capture(r"For appId: __default")
    leader.drain()

    # Open consumer queue
    consumer.open(du.uri_priority, flags=["read"], succeed=True)
    consumer.wait_push_event()

    # Test that queue is no longer in alarm state
    assert leader.outputs_regex(r"no longer appears to be stuck.", 1)

    # Disable consumption monitor (maxIdleTime = 0)
    cluster.config.domains[du.domain_priority].definition.parameters.max_idle_time = 0
    cluster.reconfigure_domain(du.domain_priority, succeed=True)

    # Close consumer queue and post one more message
    consumer.close(du.uri_priority, succeed=True)
    producer.post(du.uri_priority, ["msg2"], succeed=True, wait_ack=True)

    # Test that alarm is not triggered
    assert not leader.alarms("QUEUE_STUCK", 2)


@tweak.domain.max_idle_time(3)
def test_priority_reconfigure_max_idle_time(
    cluster: Cluster, domain_urls: tc.DomainUrls
):
    """
    Test that if max idle time is reconfigured, alarm is triggered on proper time in priority mode.
    """
    du = domain_urls
    leader = cluster.last_known_leader
    proxy = next(cluster.proxy_cycle())

    # Create producer and consumer
    producer = proxy.create_client("producer")
    producer.open(du.uri_priority, flags=["write,ack"], succeed=True)
    consumer = proxy.create_client("consumer")
    consumer.open(
        du.uri_priority, flags=["read"], max_unconfirmed_messages=1, succeed=True
    )

    # Post 2 messages, msg2 is not delivered to consumer due to max_unconfirmed_messages=1
    producer.post(du.uri_priority, ["msg1"], succeed=True, wait_ack=True)
    producer.post(du.uri_priority, ["msg2"], succeed=True, wait_ack=True)

    # Decrease max idle time from 3 to 1 second
    cluster.config.domains[du.domain_priority].definition.parameters.max_idle_time = 1
    cluster.reconfigure_domain(du.domain_priority, succeed=True)

    # Within 2 sec (more than new max idle time 1 sec but less than old one 3 sec) check that alarm is triggered
    assert leader.alarms("QUEUE_STUCK", 2)
    assert leader.capture(r"For appId: __default")
    leader.drain()

    # Confirm msg1 and wait for delivering of msg2
    consumer.confirm(du.uri_priority, "*", succeed=True)
    consumer.wait_push_event()

    # Check that queue is no longer in alarm state
    assert leader.outputs_regex(r"no longer appears to be stuck.", 1)

    # Post msg3 which is not delivered due to unconfirmed msg2
    producer.post(du.uri_priority, ["msg3"], succeed=True, wait_ack=True)

    # Increase max idle time from 1 to 3 seconds
    cluster.config.domains[du.domain_priority].definition.parameters.max_idle_time = 3
    cluster.reconfigure_domain(du.domain_priority, succeed=True)

    # Within 2 secs (more than old max idle time 1 sec) check no alarm is triggered
    assert not leader.alarms("QUEUE_STUCK", 2)

    # Wait 2 seconds more (more than new max idle time 3 sec) and check that alarm is triggered
    assert leader.alarms("QUEUE_STUCK", 2)
    assert leader.capture(r"For appId: __default")


@tweak.domain.max_idle_time(1)
def test_priority_alarms_subscription_mismatch(
    cluster: Cluster, domain_urls: tc.DomainUrls
):
    """
    Test broker ALARM log content for producer/consumer subscription expression mismatch (put aside list is not empty).
    """

    uri_priority = domain_urls.uri_priority
    leader = cluster.last_known_leader
    proxy = next(cluster.proxy_cycle())

    producer = proxy.create_client("producer")
    producer.open(uri_priority, flags=["write,ack"], succeed=True)

    consumer = proxy.create_client("consumer")
    consumer.open(
        uri_priority,
        flags=["read"],
        succeed=True,
        subscriptions=[{"expression": "x == 1"}],
    )

    producer.post(
        uri_priority,
        ["msg"],
        succeed=True,
        wait_ack=True,
        messageProperties=[{"name": "x", "value": "0", "type": "E_INT"}],
    )

    assert leader.alarms("QUEUE_STUCK", 2)
    assert leader.capture(r"For appId: __default")
    assert leader.capture(r"Put aside list size: 1")
    assert leader.capture(r"Redelivery list size: 0")
    assert leader.capture(r"Consumer subscription expressions:")
    assert leader.capture(r"x == 1")
    assert leader.capture(r"Oldest message in the 'Put aside' list:")
    assert leader.capture(r"Message Properties: \[ x \(INT32\) = 0 \]")


# -------------------------------------
# Consumption monitor fanout mode tests
# -------------------------------------


@tweak.domain.max_idle_time(3)
def test_fanout_no_alarms_for_a_slow_queue(
    cluster: Cluster, domain_urls: tc.DomainUrls
):
    """
    Test broker ALARMS in fanout mode for a slowly moving queue.
    """
    uri_fanout = domain_urls.uri_fanout
    leader = cluster.last_known_leader
    proxies = cluster.proxy_cycle()

    # Create producer
    producer = next(proxies).create_client("producer")
    producer.open(uri_fanout, flags=["write,ack"], succeed=True)

    # Create 3 consumers
    app_ids = tc.TEST_APPIDS[:]
    consumers = {}
    for app_id in app_ids:
        consumer = next(proxies).create_client(app_id)
        consumers[app_id] = consumer
        consumer.open(
            f"{uri_fanout}?id={app_id}",
            flags=["read"],
            max_unconfirmed_messages=1,
            succeed=True,
        )

    # Post 3 message
    producer.post(uri_fanout, ["msg1"], succeed=True, wait_ack=True)
    producer.post(uri_fanout, ["msg2"], succeed=True, wait_ack=True)
    producer.post(uri_fanout, ["msg3"], succeed=True, wait_ack=True)

    # Simulate a slow consumer by not confirming the message immediately
    time.sleep(1)
    for app_id in app_ids:
        consumers[app_id].confirm(f"{uri_fanout}?id={app_id}", "+1", succeed=True)
        consumers[app_id].wait_push_event()

    time.sleep(1)
    for app_id in app_ids:
        consumers[app_id].confirm(f"{uri_fanout}?id={app_id}", "+1", succeed=True)
        consumers[app_id].wait_push_event()

    # Test that no alarm is triggered
    assert not leader.alarms("QUEUE_STUCK", 2)


@tweak.domain.max_idle_time(1)
def test_fanout_transition_active_alarm_active(
    cluster: Cluster, domain_urls: tc.DomainUrls
):
    """
    Test transition from active to alarm state and then back to active state in fanout mode.
    """
    uri_fanout = domain_urls.uri_fanout
    leader = cluster.last_known_leader
    proxies = cluster.proxy_cycle()

    # Create producer
    producer = next(proxies).create_client("producer")
    producer.open(uri_fanout, flags=["write,ack"], succeed=True)

    # Create consumers
    app_ids = tc.TEST_APPIDS[:]
    consumers = {}
    for app_id in app_ids:
        consumer = next(proxies).create_client(app_id)
        consumers[app_id] = consumer
        consumer.open(
            f"{uri_fanout}?id={app_id}",
            flags=["read"],
            max_unconfirmed_messages=1,
            succeed=True,
        )

    # Post "msg1" message, it is delivered to all consumers
    producer.post(uri_fanout, ["msg1"], succeed=True, wait_ack=True)

    # Wait more than max idle time and check no alarm is triggered
    assert not leader.alarms("QUEUE_STUCK", 2)

    # Post "msg2" message, it is not delivered to consumers due to max_unconfirmed_messages=1
    producer.post(uri_fanout, ["msg2"], succeed=True, wait_ack=True)

    # Apps except 'foo' confirm all messages, msg2 for 'foo' remains in queue
    for app_id in app_ids:
        if app_id != "foo":
            consumers[app_id].confirm(f"{uri_fanout}?id={app_id}", "*", succeed=True)
            consumers[app_id].wait_push_event()

    # Wait more than max idle time and test that alarm is triggered for 'foo' consumer
    assert leader.alarms("QUEUE_STUCK", 2)
    assert leader.capture(r"For appId: foo")
    leader.drain()

    # Confirm all messages for 'foo', queue is empty
    consumers["foo"].confirm(f"{uri_fanout}?id=foo", "*", succeed=True)
    consumers["foo"].wait_push_event()

    # Test that queue for 'foo' consumer is no longer in alarm state
    assert leader.outputs_regex(r"id=foo' no longer appears to be stuck.", 1)


@tweak.domain.max_idle_time(1)
def test_fanout_alarms_subscription_mismatch(
    cluster: Cluster, domain_urls: tc.DomainUrls
):
    """
    Test broker ALARM log content in fanout mode for producer/consumer subscription
    expression mismatch (put aside list is not empty).
    """
    uri_fanout = domain_urls.uri_fanout
    leader = cluster.last_known_leader
    proxies = cluster.proxy_cycle()

    # Create producer
    producer = next(proxies).create_client("producer")
    producer.open(uri_fanout, flags=["write,ack"], succeed=True)

    # Create consumers, for 'foo' with wrong subscription expression
    app_ids = tc.TEST_APPIDS[:]
    consumers = {}
    for app_id in app_ids:
        subscr_expr = "x == 1" if app_id == "foo" else "x == 0"
        consumer = next(proxies).create_client(app_id)
        consumers[app_id] = consumer
        consumer.open(
            f"{uri_fanout}?id={app_id}",
            flags=["read"],
            succeed=True,
            subscriptions=[{"expression": subscr_expr}],
        )

    # Post message with subscription expression "x == 0"
    producer.post(
        uri_fanout,
        ["msg"],
        succeed=True,
        wait_ack=True,
        messageProperties=[{"name": "x", "value": "0", "type": "E_INT"}],
    )

    # Wait more than max idle time and check for alarms
    assert leader.alarms("QUEUE_STUCK", 2)
    assert leader.capture(r"For appId: foo", 1)
    assert leader.capture(r"Put aside list size: 1")
    assert leader.capture(r"Redelivery list size: 0")
    assert leader.capture(r"Consumer subscription expressions:")
    assert leader.capture(r"x == 1")
    assert leader.capture(r"Oldest message in the 'Put aside' list:")
    assert leader.capture(r"Message Properties: \[ x \(INT32\) = 0 \]")


# ----------------------------
# Capacity monitor alarm tests
# ----------------------------


@tweak.domain.storage.queue_limits.messages(2)
def test_capacity_alarm_subscription_mismatch(
    cluster: Cluster, domain_urls: tc.DomainUrls
):
    """
    Test broker capacity meter ALARM log content for producer/consumer subscription expression mismatch (put aside list is not empty).
    """

    uri_priority = domain_urls.uri_priority
    leader = cluster.last_known_leader
    proxy = next(cluster.proxy_cycle())

    producer = proxy.create_client("producer")
    producer.open(uri_priority, flags=["write"], succeed=True)

    consumer = proxy.create_client("consumer")
    consumer.open(
        uri_priority,
        flags=["read"],
        succeed=True,
        subscriptions=[{"expression": "x == 1"}],
    )

    producer.post(
        uri_priority,
        ["msg"],
        succeed=True,
        wait_ack=True,
        messageProperties=[{"name": "x", "value": "0", "type": "E_INT"}],
    )

    producer.post(
        uri_priority,
        ["msg"],
        succeed=True,
        wait_ack=True,
        messageProperties=[{"name": "y", "value": "0", "type": "E_INT"}],
    )

    assert leader.alarms("CAPACITY_STATE_FULL", 1)
    assert leader.capture(r"Put aside list size: 1")
    assert leader.capture(r"Redelivery list size: 0")
    assert leader.capture(r"Consumer subscription expressions:")
    assert leader.capture(r"x == 1")
    assert leader.capture(r"Oldest message in the 'Put aside' list:")
    assert leader.capture(r"Message Properties: \[ x \(INT32\) = 0 \]")
