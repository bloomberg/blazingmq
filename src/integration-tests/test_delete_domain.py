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
from blazingmq.dev.it.process.client import ITError
from blazingmq.dev.it.process.admin import AdminClient
import time


def test_domain_deletion_fail_confirm(cluster: Cluster):
    """
    1. Connect producer and consumer to BMQ on `domain1/qqq`
    2. Producer produces 5 messages, and the consumer consumes all 5
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

    for node in cluster.configurator.brokers.values():
        del node.domains[tc.DOMAIN_PRIORITY]
    cluster.deploy_domains()

    cluster._logger.info("================== BEFORE SHUTDOWN ==================")

    for node in cluster.nodes():
        node.force_stop()

    cluster._logger.info("================== BEFORE RESTART ==================")

    cluster.start_nodes(wait_ready=True)

    cluster._logger.info("================== AFTER ==================")

    # producer.open(tc.URI_PRIORITY_SC, flags=["write"], succeed=True)
    # consumer.open(tc.URI_PRIORITY_SC, flags=["read"], succeed=True)

    # cluster._logger.info("================== OPENED ==================")``

    # try:
    #     consumer.confirm(tc.URI_PRIORITY, "+1", succeed=True)
    # except ITError as e:
    #     print(e)

    # producer.post(tc.URI_PRIORITY, ["msg5"], succeed=True)


def test_domain_deletion_fail_confirm_change_config(cluster: Cluster):
    """
    1. Connect producer and consumer to BMQ on `domain1/qqq`
    2. Producer produces 5 messages, and the consumer consumes all 5
    3. Delete config of `domain1` on the disk
    4. Restart the broker, while producer and consumer are still alive
    5. Consumer couldn't connect with the broker, thus fails to confirm messages
    6. Producer can still put messages
    """
    proxies = cluster.proxy_cycle()

    producer = next(proxies).create_client("producer")
    producer.open(tc.URI_PRIORITY, flags=["write"], succeed=True)

    producer = next(proxies).create_client("producer2")
    producer.open(tc.URI_PRIORITY, flags=["write"], succeed=True)

    consumer = next(proxies).create_client("consumer")
    consumer.open(tc.URI_PRIORITY, flags=["read"], succeed=True)

    producer.post(
        tc.URI_PRIORITY, [f"msg{i}" for i in range(5)], succeed=True, wait_ack=True
    )

    assert consumer.wait_push_event()

    domain_config = cluster.config.domains[tc.DOMAIN_PRIORITY]
    domain_config.definition.parameters.max_producers = 1

    for node in cluster.configurator.brokers.values():
        node.domains[tc.DOMAIN_PRIORITY] = domain_config
    cluster.deploy_domains()

    cluster._logger.info("================== BEFORE SHUTDOWN ==================")

    for node in cluster.nodes():
        node.force_stop()

    cluster._logger.info("================== BEFORE RESTART ==================")

    cluster.start_nodes(wait_ready=True)

    cluster._logger.info("================== AFTER ==================")


def test_domain_deletion_fail_open_queue(cluster: Cluster):
    """
    1. Connect producer and consumer to BMQ on `domain1/qqq`
    2. Producer produces 5 messages, and the consumer consumes all 5
    3. Delete config of `domain1` on the disk
    4. Restart the broker, while producer and consumer are still alive
    5. Both producer and consumer would throw ITError when they try to open
       a queue on the deleted domain
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

    for node in cluster.configurator.brokers.values():
        del node.domains[tc.DOMAIN_PRIORITY]
    cluster.deploy_domains()

    for node in cluster.nodes():
        node.force_stop()
    cluster.start_nodes(wait_ready=True)

    try:
        producer.open(tc.URI_PRIORITY, flags=["write"], succeed=True)
    except ITError as e:
        print(e)

    try:
        consumer.open(tc.URI_PRIORITY, flags=["read"], succeed=True)
    except ITError as e:
        print(e)


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

    for node in cluster.configurator.brokers.values():
        del node.domains[tc.DOMAIN_PRIORITY]
    cluster.deploy_domains()

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
    1. Connect producer only to BMQ on `domain1/qqq`
    2. Producer produces 5 messages
    3. Producer closes queue
    4. Delete config of `domain1` on the disk
    5. Restart broker
    6. Add config of `domain1` back on the disk (with or without different config parameters)
    7. Invoke `DOMAIN RECONFIGURE` command to reload the config on the broker
    8. Producer opens the queue once again
    9. Producer produces 2 more messages
    10. Now, start a consumer to open the queue and try to confirm messages. Consumer sees 2 messages.
    11. Check to see `domain1/qqq` appears only in one partition
    12. Restart broker
    13. There's a key collision and caused failure during cluster recovery
    """
    proxies = cluster.proxy_cycle()

    producer = next(proxies).create_client("producer")
    producer.open(tc.URI_PRIORITY, flags=["write"], succeed=True)

    # producer.post(
    #     tc.URI_PRIORITY, [f"msg{i}" for i in range(5)], succeed=True, wait_ack=True
    # )

    producer.close(tc.URI_PRIORITY, succeed=True)

    domain_config = cluster.config.domains[tc.DOMAIN_PRIORITY]

    for node in cluster.configurator.brokers.values():
        del node.domains[tc.DOMAIN_PRIORITY]
    cluster.deploy_domains()

    cluster._logger.info("================ Before first restart ================")

    for node in cluster.nodes():
        node.force_stop()
    cluster.start_nodes(wait_ready=True)

    # leader = cluster.last_known_leader
    # leader.force_stop()
    # leader.start()
    # leader.wait_until_started()
    # leader.wait_status(wait_leader=True, wait_ready=True)

    cluster._logger.info("================ After first restart ================")

    cluster._logger.info("================ Before adding it back ================")

    for node in cluster.configurator.brokers.values():
        node.domains[tc.DOMAIN_PRIORITY] = domain_config
    cluster.deploy_domains()

    cluster._logger.info("================ After adding it back ================")

    cluster.reconfigure_domain(tc.DOMAIN_PRIORITY, succeed=True)

    cluster._logger.info("================ After reconfigure ================")

    # leader = cluster.last_known_leader
    # admin = AdminClient()
    # admin.connect(leader.config.host, int(leader.config.port))
    # res = admin.send_admin(f"CLUSTERS CLUSTER {cluster.name} STORAGE SUMMARY")
    # assert res.count("Number of assigned queue-storages: 1") == 10
    # admin.stop()

    producer.open(tc.URI_PRIORITY, flags=["write"], succeed=True)

    """
    The below part is not even necessary
    if search for "uri = "bmq://bmq.test.mmap.priority/qqq" key = [",
    we can see starting from here a different key is used for the same uri
    """

    # producer.post(
    #     tc.URI_PRIORITY, [f"msg{i}" for i in range(5, 7)], succeed=True, wait_ack=True
    # )

    # consumer = next(proxies).create_client("consumer")
    # consumer.open(tc.URI_PRIORITY, flags=["read"], block=True, succeed=True)

    # assert consumer.wait_push_event()
    # msgs = consumer.list(tc.URI_PRIORITY, block=True)
    # assert len(msgs) == 2

    # leader = cluster.last_known_leader
    # admin = AdminClient()
    # admin.connect(leader.config.host, int(leader.config.port))
    # res = admin.send_admin(f"CLUSTERS CLUSTER {cluster.name} STORAGE SUMMARY")
    # assert res.count("Number of assigned queue-storages: 1") == 1
    # admin.stop()

    # cluster._logger.info("================ Before second restart ================")

    for node in cluster.nodes():
        node.force_stop()
    cluster.start_nodes(wait_leader=True, wait_ready=True)

    # cluster._logger.info("================ After second restart ================")
