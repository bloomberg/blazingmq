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
import time

import blazingmq.dev.it.testconstants as tc
from blazingmq.dev.it.fixtures import (
    Cluster,
    multi_node,
    multi3_node,
    order,
    tweak,
)  # pylint: disable=unused-import
from blazingmq.dev.it.process.client import Client
from blazingmq.dev.it.util import attempt, wait_until

new_version_suffix = "NEW_VERSION"

def update_and_redeploy(cluster: Cluster):
    """Update the cluster and redeploy it."""

    # Stop all nodes
    cluster.stop_nodes()

    # Update env var for all node, i.e. BLAZINGMQ_BROKER_{NAME}
    # to the value stored in BLAZINGMQ_BROKER_NEW_VERSION
    cluster.update_all_brokers_binary(new_version_suffix)
    
    # Restart all nodes to apply binary update
    cluster.start_nodes(wait_leader=True, wait_ready=True)


def test_redeploy_basic(multi3_node: Cluster, domain_urls: tc.DomainUrls):
    """Simple test start, stop, update broker version for all nodes and restart."""

    cluster = multi3_node

    uri_priority = domain_urls.uri_priority

    # Start a producer and post a message.
    proxies = cluster.proxy_cycle()
    producer = next(proxies).create_client("producer")
    producer.open(uri_priority, flags=["write", "ack"], succeed=True)
    producer.post(uri_priority, payload=["msg1"], wait_ack=True, succeed=True)

    update_and_redeploy(cluster)

    producer.post(uri_priority, payload=["msg2"], wait_ack=True, succeed=True)

    consumer = next(proxies).create_client("consumer")
    consumer.open(uri_priority, flags=["read"], succeed=True)
    consumer.wait_push_event()
    assert wait_until(lambda: len(consumer.list(uri_priority, block=True)) == 2, 2)


def test_redeploy_one_by_one(multi3_node: Cluster, domain_urls: tc.DomainUrls):
    """
        Test to upgrade binaries of cluster nodes one by one.
        Every time a node is upgraded, all the nodes are restarted.
    """

    cluster = multi3_node

    uri_priority = domain_urls.uri_priority

    # Start a producer and post a message.
    proxies = cluster.proxy_cycle()
    producer = next(proxies).create_client("producer")
    producer.open(uri_priority, flags=["write", "ack"], succeed=True)
    producer.post(uri_priority, payload=["msg1"], wait_ack=True, succeed=True)

    for broker in cluster.configurator.brokers.values():
        # Stop the node
        cluster.stop_nodes(broker.name)

        # Update binary for the given broker
        cluster.update_broker_binary(broker, new_version_suffix)

        # Restart all nodes to apply binary update
        cluster.start_nodes(wait_leader=True, wait_ready=True)

    producer.post(uri_priority, payload=["msg2"], wait_ack=True, succeed=True)

    consumer = next(proxies).create_client("consumer")
    consumer.open(uri_priority, flags=["read"], succeed=True)
    consumer.wait_push_event()
    assert wait_until(lambda: len(consumer.list(uri_priority, block=True)) == 2, 2)

