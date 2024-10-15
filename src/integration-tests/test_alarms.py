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
def test_no_alarms_for_a_slow_queue(cluster: Cluster):
    """
    Test no broker ALARMS for a slowly moving queue.
    """
    leader = cluster.last_known_leader
    proxy = next(cluster.proxy_cycle())

    producer = proxy.create_client("producer")
    producer.open(tc.URI_PRIORITY, flags=["write,ack"], succeed=True)

    consumer1 = proxy.create_client("consumer1")
    consumer2 = proxy.create_client("consumer2")
    consumer1.open(
        tc.URI_PRIORITY, flags=["read"], max_unconfirmed_messages=1, succeed=True
    )

    producer.post(tc.URI_PRIORITY, ["msg1"], succeed=True, wait_ack=True)

    consumer1.confirm(tc.URI_PRIORITY, "*", succeed=True)

    producer.post(tc.URI_PRIORITY, ["msg1"], succeed=True, wait_ack=True)
    producer.post(tc.URI_PRIORITY, ["msg1"], succeed=True, wait_ack=True)

    time.sleep(4)

    # First, test the alarm
    assert leader.alarms("QUEUE_STUCK", 1)
    leader.drain()

    # Then test no alarm while consumer1 slowly confirms
    time.sleep(1)
    consumer1.confirm(tc.URI_PRIORITY, "*", succeed=True)

    time.sleep(1)
    consumer1.confirm(tc.URI_PRIORITY, "*", succeed=True)
    producer.post(tc.URI_PRIORITY, ["msg1"], succeed=True, wait_ack=True)

    time.sleep(1)
    # Consumer2 picks the last message
    consumer2.open(
        tc.URI_PRIORITY, flags=["read"], max_unconfirmed_messages=1, succeed=True
    )

    time.sleep(1)
    assert not leader.alarms("QUEUE_STUCK", 1)


@tweak.cluster.queue_operations.consumption_monitor_period_ms(500)
@tweak.domain.max_idle_time(1)
def test_alarms_subscription_mismatch(cluster: Cluster):
    """
    Test broker ALARM log content for producer/consumer subscription expression mismatch (put aside list is not empty).
    """

    leader = cluster.last_known_leader
    proxy = next(cluster.proxy_cycle())

    producer = proxy.create_client("producer")
    producer.open(tc.URI_PRIORITY, flags=["write,ack"], succeed=True)

    consumer = proxy.create_client("consumer")
    consumer.open(
        tc.URI_PRIORITY,
        flags=["read"],
        succeed=True,
        subscriptions=[{"expression": "x == 1"}],
    )

    producer.post(
        tc.URI_PRIORITY,
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
