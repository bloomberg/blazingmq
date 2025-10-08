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
Integration test that tests some basic things while restarting *entire* cluster
in the middle of things.  Note that this test does not test the HA
functionality (i.e., no PUTs/CONFIRMs etc are retransmitted).
"""

import re

import blazingmq.dev.it.testconstants as tc
from blazingmq.dev.it.fixtures import (
    Cluster,
    order,
)  # pylint: disable=unused-import
from blazingmq.dev.it.process.client import Client
from blazingmq.dev.it.util import attempt, wait_until

from blazingmq.dev.it.cluster_util import (
    ensure_message_at_storage_layer,
)

pytestmark = order(2)


def test_basic(cluster: Cluster, domain_urls: tc.DomainUrls):
    """
    Test basic functionality while restarting the entire cluster in the
    middle of things.
    """

    uri_priority = domain_urls.uri_priority

    # Start a producer and post a message.
    proxies = cluster.proxy_cycle()
    producer = next(proxies).create_client("producer")
    producer.open(uri_priority, flags=["write", "ack"], succeed=True)
    producer.post(uri_priority, payload=["msg1"], wait_ack=True, succeed=True)

    ensure_message_at_storage_layer(cluster, 0, uri_priority, 1)

    cluster.restart_nodes()
    # For a standard cluster, states have already been restored as part of
    # leader re-election.
    if cluster.is_single_node:
        producer.wait_state_restored()

    producer.post(uri_priority, payload=["msg2"], wait_ack=True, succeed=True)

    consumer = next(proxies).create_client("consumer")
    consumer.open(uri_priority, flags=["read"], succeed=True)
    consumer.wait_push_event()
    assert wait_until(
        lambda: len(consumer.list(uri_priority, block=True)) == 2, timeout=2
    )


def test_wrong_domain(cluster: Cluster, domain_urls: tc.DomainUrls):
    """
    Test that opening a queue in a non-existent domain fails, while opening
    a queue in an existing domain succeeds.
    """

    proxies = cluster.proxy_cycle()
    producer = next(proxies).create_client("producer")

    assert Client.e_SUCCESS is producer.open(
        domain_urls.uri_fanout, flags=["write"], block=True
    )
    assert Client.e_SUCCESS is not producer.open(
        "bmq://domain.does.not.exist/qqq",
        flags=["write"],
        block=True,
        no_except=True,
    )


def test_migrate_domain_to_another_cluster(
    cluster: Cluster, domain_urls: tc.DomainUrls
):
    """
    Test that migrating a domain to another cluster works as expected.
    Steps:
    1. Start a cluster with 2 domains: priority and fanout.
    2. Open a queue in the fanout domain and ensure that it is present
         at the storage layer.
    3. Change the cluster configuration to remove the fanout domain.
    4. Restart the cluster.
    5. Ensure that opening a queue in the fanout domain fails.
    """

    uri_fanout = domain_urls.uri_fanout
    proxies = cluster.proxy_cycle()
    producer = next(proxies).create_client("producer")

    assert Client.e_SUCCESS == producer.open(uri_fanout, flags=["write"], block=True)

    # Before changing domain config of each node in the cluster, ensure that
    # all nodes in the cluster have observed the previous open-queue event.
    # If we don't do this, replicas may receive a queue creation event at
    # the storage layer, pick up the domain, and, as a result, fail to
    # successfully apply the queue creation event.
    @attempt(3, 5)
    def wait_replication():
        for node in cluster.nodes():
            node.command(f"CLUSTERS CLUSTER {node.cluster_name} STORAGE SUMMARY")

        for node in cluster.nodes():
            assert node.outputs_regex(
                r"\w{10}\s+0\s+0\s+\d+\s+B\s+" + re.escape(uri_fanout),
                timeout=1,
            )
            # Above regex is to match line:
            # C1E2A44527    0      0      68  B      bmq://bmq.test.mmap.fanout/qqq
            # where columns are: QueueKey, PartitionId, NumMsgs, NumBytes,
            # QueueUri respectively. Since we opened only 1 queue, we know that
            # it will be assigned to partitionId 0.

    cluster.config.domains.clear()
    cluster.deploy_domains()
    cluster.restart_nodes()

    assert Client.e_SUCCESS != producer.open(uri_fanout, flags=["write"], block=True)
