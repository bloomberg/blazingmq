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

import blazingmq.dev.it.testconstants as tc
from blazingmq.dev.it.fixtures import (
    Cluster,
    order,
)
from blazingmq.dev.it.util import wait_until

TIMEOUT = 30

pytestmark = order(3)


def expected_header(start, count, total, size):
    return (
        f"Printing {count} message(s) [{start}-{start + count - 1} "
        f"/ {total}] (total: {size}  B)"
    )


def test_list_messages_fanout(cluster: Cluster, domain_urls: tc.DomainUrls):
    du = domain_urls
    proxies = cluster.proxy_cycle()

    producer = next(proxies).create_client("producer")
    producer.open(du.uri_fanout, flags=["write,ack"], succeed=True)

    # In Raft mode the CSL leader need not own the queue's partition; the
    # 'LIST' admin command must be issued on that partition's primary.  Resolve
    # it after the queue is assigned (i.e. after the producer opened it).
    primary = cluster.last_known_leader.wait_queue_primary(du.uri_fanout)

    for i in range(1, 4):
        producer.post(
            du.uri_fanout,
            ["x" * 10 * i],
            wait_ack=True,
            succeed=True,
        )

    consumer = next(proxies).create_client("consumer")

    for i, uri in enumerate([du.uri_fanout_foo, du.uri_fanout_bar, du.uri_fanout_baz]):
        consumer.open(uri, flags=["read"], succeed=True)

    assert wait_until(lambda: len(consumer.list(block=True)) == 9, TIMEOUT)
    guids = [msg.guid for msg in consumer.list(du.uri_fanout_foo, block=True)]

    for i, uri in enumerate([du.uri_fanout_foo, du.uri_fanout_bar, du.uri_fanout_baz]):
        consumer.confirm(uri, guids[i], succeed=True)

    primary.list_messages(du.domain_fanout, tc.TEST_QUEUE, 0, 3)
    assert primary.outputs_substr(expected_header(0, 3, 3, 60), TIMEOUT)
    assert primary.outputs_substr("10", TIMEOUT)
    assert primary.outputs_substr("20", TIMEOUT)
    assert primary.outputs_substr("30", TIMEOUT)

    primary.list_messages(du.domain_fanout, tc.TEST_QUEUE, 0, "UNLIMITED")
    assert primary.outputs_substr(expected_header(0, 3, 3, 60), TIMEOUT)
    assert primary.outputs_substr("10", TIMEOUT)
    assert primary.outputs_substr("20", TIMEOUT)
    assert primary.outputs_substr("30", TIMEOUT)

    primary.list_messages(du.domain_fanout, tc.TEST_QUEUE, 1, 1)
    assert primary.outputs_substr(expected_header(1, 1, 3, 20), TIMEOUT)
    assert primary.outputs_substr("20", TIMEOUT)

    primary.list_messages(du.domain_fanout, tc.TEST_QUEUE, -1, 1)
    assert primary.outputs_substr(expected_header(2, 1, 3, 30), TIMEOUT)
    assert primary.outputs_substr("30", TIMEOUT)

    primary.list_messages(du.domain_fanout, tc.TEST_QUEUE, 1, -1)
    assert primary.outputs_substr(expected_header(0, 1, 3, 10), TIMEOUT)
    assert primary.outputs_substr("10", TIMEOUT)

    primary.list_messages(du.domain_fanout, tc.TEST_QUEUE, -1, -1)
    assert primary.outputs_substr(expected_header(1, 1, 3, 20), TIMEOUT)
    assert primary.outputs_substr("20", TIMEOUT)

    primary.list_messages(du.domain_fanout, tc.TEST_QUEUE, 0, "UNLIMITED garbage")
    assert primary.outputs_substr("Error processing command", TIMEOUT)

    time.sleep(2)  # Allow previous confirmations to complete
    for i, appid in enumerate(["foo", "bar", "baz"]):
        primary.list_messages(
            du.domain_fanout, tc.TEST_QUEUE, 0, "UNLIMITED", appid=appid
        )
        assert primary.outputs_substr(
            expected_header(0, 2, 2, 60 - (i + 1) * 10), TIMEOUT
        )
        for j in range(0, 3):
            if i != j:
                assert primary.outputs_regex(f"{guids[j]}.*{(j + 1) * 10}", TIMEOUT)

    primary.list_messages(
        du.domain_fanout, tc.TEST_QUEUE, 0, "UNLIMITED", appid="pikachu"
    )
    assert primary.outputs_substr("Invalid 'LIST' command: invalid APPID", TIMEOUT)


def test_list_messages_priority(cluster: Cluster, domain_urls: tc.DomainUrls):
    du = domain_urls
    proxies = cluster.proxy_cycle()

    producer = next(proxies).create_client("producer")
    producer.open(du.uri_priority, flags=["write,ack"], succeed=True)

    # In Raft mode the CSL leader need not own the queue's partition; the
    # 'LIST' admin command must be issued on that partition's primary.  Resolve
    # it after the queue is assigned (i.e. after the producer opened it).
    primary = cluster.last_known_leader.wait_queue_primary(du.uri_priority)

    for i in range(1, 4):
        producer.post(
            du.uri_priority,
            ["x" * 10 * i],
            wait_ack=True,
            succeed=True,
        )

    client = next(proxies).create_client("consumer")
    client.open(du.uri_priority, flags=["read"], succeed=True)

    assert wait_until(lambda: len(client.list(block=True)) == 3, TIMEOUT)
    _ = [msg.guid for msg in client.list(uri=du.uri_priority, block=True)]

    primary.list_messages(du.domain_priority, tc.TEST_QUEUE, 0, 3)
    assert primary.outputs_substr(expected_header(0, 3, 3, 60), TIMEOUT)
    assert primary.outputs_substr("10", TIMEOUT)
    assert primary.outputs_substr("20", TIMEOUT)
    assert primary.outputs_substr("30", TIMEOUT)

    primary.list_messages(du.domain_priority, tc.TEST_QUEUE, 0, "UNLIMITED")
    assert primary.outputs_substr(expected_header(0, 3, 3, 60), TIMEOUT)
    assert primary.outputs_substr("10", TIMEOUT)
    assert primary.outputs_substr("20", TIMEOUT)
    assert primary.outputs_substr("30", TIMEOUT)

    primary.list_messages(du.domain_priority, tc.TEST_QUEUE, 1, 1)
    assert primary.outputs_substr(expected_header(1, 1, 3, 20), TIMEOUT)
    assert primary.outputs_substr("20", TIMEOUT)

    primary.list_messages(du.domain_priority, tc.TEST_QUEUE, -1, 1)
    assert primary.outputs_substr(expected_header(2, 1, 3, 30), TIMEOUT)
    assert primary.outputs_substr("30", TIMEOUT)

    primary.list_messages(du.domain_priority, tc.TEST_QUEUE, 1, -1)
    assert primary.outputs_substr(expected_header(0, 1, 3, 10), TIMEOUT)
    assert primary.outputs_substr("10", TIMEOUT)

    primary.list_messages(du.domain_priority, tc.TEST_QUEUE, -1, -1)
    assert primary.outputs_substr(expected_header(1, 1, 3, 20), TIMEOUT)
    assert primary.outputs_substr("20", TIMEOUT)

    primary.list_messages(du.domain_priority, tc.TEST_QUEUE, 0, "UNLIMITED garbage")
    assert primary.outputs_substr("Error processing command", TIMEOUT)

    primary.list_messages(
        du.domain_priority, tc.TEST_QUEUE, 0, "UNLIMITED", appid="pikachu"
    )
    assert primary.outputs_substr("Invalid 'LIST' command: invalid APPID", TIMEOUT)
