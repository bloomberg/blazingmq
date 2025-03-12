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


@tweak.cluster.queue_operations.consumption_monitor_period_ms(500)
@tweak.domain.max_idle_time(3)
def test_no_alarms_for_a_slow_queue(cluster: Cluster, domain_urls: tc.DomainUrls):
    """
    Test no broker ALARMS for a slowly moving queue.
    """
    uri_priority = domain_urls.uri_priority
    leader = cluster.last_known_leader
    proxy = next(cluster.proxy_cycle())

    producer = proxy.create_client("producer")
    producer.open(uri_priority, flags=["write,ack"], succeed=True)

    consumer1 = proxy.create_client("consumer1")
    consumer2 = proxy.create_client("consumer2")
    consumer1.open(
        uri_priority, flags=["read"], max_unconfirmed_messages=1, succeed=True
    )

    producer.post(uri_priority, ["msg1"], succeed=True, wait_ack=True)

    consumer1.confirm(uri_priority, "*", succeed=True)

    producer.post(uri_priority, ["msg1"], succeed=True, wait_ack=True)
    producer.post(uri_priority, ["msg1"], succeed=True, wait_ack=True)

    time.sleep(4)

    # First, test the alarm
    assert leader.alarms("QUEUE_STUCK", 1)
    leader.drain()

    # Then test no alarm while consumer1 slowly confirms
    time.sleep(1)
    consumer1.confirm(uri_priority, "*", succeed=True)

    time.sleep(1)
    consumer1.confirm(uri_priority, "*", succeed=True)
    producer.post(uri_priority, ["msg1"], succeed=True, wait_ack=True)

    time.sleep(1)
    # Consumer2 picks the last message
    consumer2.open(
        uri_priority, flags=["read"], max_unconfirmed_messages=1, succeed=True
    )

    time.sleep(1)
    assert not leader.alarms("QUEUE_STUCK", 1)


@tweak.cluster.queue_operations.consumption_monitor_period_ms(500)
@tweak.domain.max_idle_time(1)
def test_alarms_subscription_mismatch(cluster: Cluster, domain_urls: tc.DomainUrls):
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

    time.sleep(2)

    assert leader.alarms("QUEUE_STUCK", 1)
    assert leader.capture(r"Put aside list size: 1")
    assert leader.capture(r"Redelivery list size: 0")
    assert leader.capture(r"Consumer subscription expressions:")
    assert leader.capture(r"x == 1")
    assert leader.capture(r"Oldest message in the 'Put aside' list:")
    assert leader.capture(r"Message Properties: \[ x \(INT32\) = 0 \]")


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
