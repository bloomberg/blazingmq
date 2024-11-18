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
Testing deletion of domains.
"""

import blazingmq.dev.it.testconstants as tc
from blazingmq.dev.it.fixtures import Cluster, cluster
from blazingmq.dev.it.process.client import Client, ITError


def test_domain_deletion_fail_confirm(cluster: Cluster):
    """
    1. Connect producer and consumer to BMQ on `domain1/qqq`
    2. Producer produces 5 messages, and the consumer consumes all 5.
    3. Delete config of `domain1` on the disk
    4. Restart the broker, while producer and consumer are still alive
    5. Consumer couldn't connect with the broker, thus fails to confirm messages
    6. Producer can still put messages
    """
    proxies = cluster.proxy_cycle()

    producer = next(proxies).create_client("producer")
    producer.open(tc.URI_PRIORITY, flags=["write"], succeed=True)

    consumer = next(proxies).create_client("consumer")
    consumer.open(tc.URI_PRIORITY, flags=["read"], succeed=True)

    producer.post(
        tc.URI_PRIORITY, [f"msg{i}" for i in range(5)], succeed=True, wait_ack=True
    )

    assert consumer.wait_push_event()

    del cluster.config.domains[tc.DOMAIN_PRIORITY]
    cluster.reconfigure_domain(tc.DOMAIN_PRIORITY, write_only=True, succeed=True)

    for node in cluster.nodes():
        node.force_stop()
    cluster.start_nodes(wait_ready=True)

    try:
        consumer.confirm(tc.URI_PRIORITY, "+1", succeed=True)
    except ITError as e:
        print(e)

    assert producer.post(tc.URI_PRIORITY, ["msg5"], succeed=True) == Client.e_SUCCESS


def test_domain_deletion_produce_more_after_delete_domain(cluster: Cluster):
    """
    1. Connect producer and consumer to BMQ on `domain1/qqq`
    2. Producer produces 5 messages
    3. Delete config of `domain1` on the disk
    4. Producer produces 2 more messages
    5. Restart the broker
    6. Consumer couldn't connect with the broker, thus fails to confirm messages
    """
    proxies = cluster.proxy_cycle()

    producer = next(proxies).create_client("producer")
    producer.open(tc.URI_PRIORITY, flags=["write"], succeed=True)

    consumer = next(proxies).create_client("consumer")
    consumer.open(tc.URI_PRIORITY, flags=["read"], succeed=True)

    producer.post(
        tc.URI_PRIORITY, [f"msg{i}" for i in range(5)], succeed=True, wait_ack=True
    )

    del cluster.config.domains[tc.DOMAIN_PRIORITY]
    cluster.reconfigure_domain(tc.DOMAIN_PRIORITY, write_only=True, succeed=True)

    producer.post(
        tc.URI_PRIORITY,
        [f"msg{i}" for i in range(5, 7)],
        succeed=True,
        wait_ack=True,
    )

    for node in cluster.nodes():
        node.force_stop()
    cluster.start_nodes(wait_ready=True)

    try:
        consumer.confirm(tc.URI_PRIORITY, "+1", succeed=True)
    except ITError as e:
        print(e)


def test_domain_deletion_add_back(cluster: Cluster):
    """
    1. Connect producer only to BMQ on `domain1/q1`
    2. Producer produces 5 messages
    3. Producer closes queue
    4. Delete config of `domain1` on the disk
    5. Restart broker
    6. Add config of `domain1` back on the disk (can try same and different config parameters)
    7. Invoke `DOMAIN RECONFIGURE` command to reload the config on the broker
    8. Producer opens the queue once again
    9. Producer produces 2 more messages
    10. Now, start a consumer to open the queue and try to confirm messages. How many messages will the consumer see?
    11. Check if `domain1/q1` is now assigned to two different partitions, which is the bug we saw in {DRQS 176641839 30 <GO>}
    """
    proxies = cluster.proxy_cycle()

    producer = next(proxies).create_client("producer")
    producer.open(tc.URI_PRIORITY, flags=["write"], succeed=True)

    producer.post(
        tc.URI_PRIORITY, [f"msg{i}" for i in range(5)], succeed=True, wait_ack=True
    )

    producer.close(tc.URI_PRIORITY, succeed=True)

    domain_config = cluster.config.domains[tc.DOMAIN_PRIORITY]

    del cluster.config.domains[tc.DOMAIN_PRIORITY]
    cluster.reconfigure_domain(tc.DOMAIN_PRIORITY, write_only=True, succeed=True)

    for node in cluster.nodes():
        node.force_stop()
    cluster.start_nodes(wait_ready=True)

    # TODO: set domain to have different config
    cluster.config.domains[tc.DOMAIN_PRIORITY] = domain_config

    cluster.reconfigure_domain(tc.DOMAIN_PRIORITY, succeed=True)

    producer.open(tc.URI_PRIORITY, flags=["write"], succeed=True)

    producer.post(
        tc.URI_PRIORITY, [f"msg{i}" for i in range(5, 7)], succeed=True, wait_ack=True
    )

    consumer = next(proxies).create_client("consumer")
    consumer.open(tc.URI_PRIORITY, flags=["read"], succeed=True)

    assert consumer.wait_push_event()
    msgs = consumer.list(tc.URI_PRIORITY, block=True)
    assert len(msgs) == 7

    # TODO: how to check partition
