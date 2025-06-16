# Copyright 2025 Bloomberg Finance L.P.
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

import blazingmq.dev.it.testconstants as tc
from blazingmq.dev.it.fixtures import (  # pylint: disable=unused-import
    Cluster,
    multi7_node,
    start_cluster,
)
from blazingmq.dev.it.util import wait_until

NEW_VERSION_SUFFIX = "NEW_VERSION"
DEFAULT_TIMEOUT = 2


def disable_exit_code_check(cluster: Cluster):
    """Disable exit code check for all nodes in the cluster."""

    # Non-FSM mode has poor healing mechanism, and can have flaky dirty
    # shutdowns, so let's disable checking exit code here.
    #
    # To give an example, an in-sync node might attempt to syncrhonize with an
    # out-of-sync node, and become out-of-sync too.  FSM mode is determined to
    # eliminate these kinds of defects.
    for node in cluster.nodes():
        node.check_exit_code = False


def update_and_redeploy(cluster: Cluster):
    """Update the cluster and redeploy it."""

    # Stop all nodes
    disable_exit_code_check(cluster)
    cluster.stop_nodes()

    # Update env var for all node, i.e. BLAZINGMQ_BROKER_{NAME}
    # to the value stored in BLAZINGMQ_BROKER_NEW_VERSION
    cluster.update_all_brokers_binary(NEW_VERSION_SUFFIX)

    # Restart all nodes to apply binary update
    cluster.start_nodes(wait_leader=True, wait_ready=True)


@start_cluster(start=True, wait_leader=True, wait_ready=True)
def test_redeploy_basic(multi7_node: Cluster, domain_urls: tc.DomainUrls):
    """Simple test start, stop, update broker version for all nodes and restart."""

    uri_priority = domain_urls.uri_priority

    # Start a producer and post a message.
    proxies = multi7_node.proxy_cycle()
    producer = next(proxies).create_client("producer")
    producer.open(uri_priority, flags=["write", "ack"], succeed=True)
    producer.post(uri_priority, payload=["msg1"], wait_ack=True, succeed=True)

    update_and_redeploy(multi7_node)

    producer.post(uri_priority, payload=["msg2"], wait_ack=True, succeed=True)

    consumer = next(proxies).create_client("consumer")
    consumer.open(uri_priority, flags=["read"], succeed=True)
    consumer.wait_push_event()

    assert wait_until(
        lambda: len(consumer.list(uri_priority, block=True)) == 2, DEFAULT_TIMEOUT
    )


@start_cluster(start=True, wait_leader=True, wait_ready=True)
def test_redeploy_one_by_one(multi7_node: Cluster, domain_urls: tc.DomainUrls):
    """
    Test to upgrade binaries of cluster nodes one by one.
    Every time a node is upgraded, all the nodes are restarted.
    """

    uri_priority = domain_urls.uri_priority

    # Start a producer and consumer
    proxies = multi7_node.proxy_cycle()
    producer = next(proxies).create_client("producer")
    consumer = next(proxies).create_client("consumer")

    # Open queue
    producer.open(uri_priority, flags=["write", "ack"], succeed=True)
    consumer.open(uri_priority, flags=["read"], succeed=True)

    number_posted = 0

    def post_message():
        nonlocal number_posted
        number_posted += 1
        producer.post(
            uri_priority, payload=[f"msg{number_posted}"], wait_ack=True, succeed=True
        )

    def assert_posted():
        nonlocal number_posted
        consumer.wait_push_event()
        assert wait_until(
            lambda: len(consumer.list(uri_priority, block=True)) == number_posted,
            DEFAULT_TIMEOUT,
        )

    # Post and receive initial message
    post_message()
    assert_posted()

    for broker in multi7_node.configurator.brokers.values():
        # Stop all nodes
        disable_exit_code_check(multi7_node)
        multi7_node.stop_nodes()

        # Update binary for the given broker
        multi7_node.update_broker_binary(broker, NEW_VERSION_SUFFIX)

        # Restart all nodes to apply binary update
        multi7_node.start_nodes(wait_leader=True, wait_ready=True)

        # Post and receive message after each redeploy
        post_message()
        assert_posted()

    # Post and receive final message after all nodes have been redeployed
    post_message()
    assert_posted()
