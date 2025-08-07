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
    tweak,
)
from blazingmq.dev.it.process.client import Client
from blazingmq.dev.it.util import wait_until


NEW_VERSION_SUFFIX = "NEW_VERSION"
CONSUMER_WAIT_TIMEOUT_SEC = 60


class MessagesCounter:
    """Simple counter class to keep track of posted messages."""

    def __init__(self):
        self.number_posted = 0

    def post_message(self, producer: Client, uri: str):
        """Post a message and increment the counter."""

        self.number_posted += 1
        producer.post(
            uri, payload=[f"msg {self.number_posted}"], wait_ack=True, succeed=True
        )

    def assert_posted(self, consumer: Client, uri: str):
        consumer.wait_push_event()
        assert wait_until(
            lambda: len(consumer.list(uri, block=True)) == 1,
            CONSUMER_WAIT_TIMEOUT_SEC,
        ), f"Consumer did not receive message {self.number_posted}"

        assert consumer.confirm(uri, "*", block=True) == Client.e_SUCCESS, (
            f"Consumer did not confirm message {self.number_posted}"
        )

        assert wait_until(
            lambda: len(consumer.list(uri, block=True)) == 0,
            CONSUMER_WAIT_TIMEOUT_SEC,
        ), f"Consumer did not receive message {self.number_posted} after confirm"


@start_cluster(False)
@tweak.cluster.partition_config.sync_config.startup_wait_duration_ms(60000)
def test_redeploy_basic(multi7_node: Cluster, domain_urls: tc.DomainUrls):
    """Simple test start, stop, update broker version for all nodes and restart."""

    multi7_node.lower_leader_startup_wait()
    multi7_node.start(wait_leader=True, wait_ready=True)

    uri_priority = domain_urls.uri_priority

    # Start a producer and post a message.
    proxies = multi7_node.proxy_cycle()
    producer = next(proxies).create_client("producer")
    consumer = next(proxies).create_client("consumer")

    # Open queue
    producer.open(uri_priority, flags=["write", "ack"], succeed=True)
    consumer.open(uri_priority, flags=["read"], succeed=True)

    # Post and receive initial message
    messagesCounter = MessagesCounter()
    messagesCounter.post_message(producer, uri_priority)
    messagesCounter.assert_posted(consumer, uri_priority)

    # Stop all nodes
    multi7_node.disable_exit_code_check()
    multi7_node.stop_nodes(prevent_leader_bounce=True)

    # Update env var for all node, i.e. BLAZINGMQ_BROKER_{NAME}
    # to the value stored in BLAZINGMQ_BROKER_NEW_VERSION
    multi7_node.update_all_brokers_binary(NEW_VERSION_SUFFIX)

    # Restart all nodes to apply binary update
    multi7_node.start_nodes(wait_leader=True, wait_ready=False)

    # Post and receive message after redeploy
    messagesCounter.post_message(producer, uri_priority)
    messagesCounter.assert_posted(consumer, uri_priority)


@start_cluster(False)
@tweak.cluster.partition_config.sync_config.startup_wait_duration_ms(60000)
def test_redeploy_whole_cluster_restart(
    multi7_node: Cluster, domain_urls: tc.DomainUrls
):
    """
    Test to upgrade binaries of cluster nodes one by one.
    Every time a node is upgraded, all the nodes are restarted.
    """

    multi7_node.lower_leader_startup_wait()
    multi7_node.start(wait_leader=True, wait_ready=True)

    uri_priority = domain_urls.uri_priority

    # Start a producer and consumer
    proxies = multi7_node.proxy_cycle()
    producer = next(proxies).create_client("producer")
    consumer = next(proxies).create_client("consumer")

    # Open queue
    producer.open(uri_priority, flags=["write", "ack"], succeed=True)
    consumer.open(uri_priority, flags=["read"], succeed=True)

    # Post and receive initial message
    messagesCounter = MessagesCounter()
    messagesCounter.post_message(producer, uri_priority)
    messagesCounter.assert_posted(consumer, uri_priority)

    multi7_node.disable_exit_code_check()

    for broker in multi7_node.nodes():
        # Stop all nodes
        multi7_node.stop_nodes(prevent_leader_bounce=True)

        # Update binary for the given broker
        multi7_node.update_broker_binary(broker.config, NEW_VERSION_SUFFIX)

        # Restart all nodes to apply binary update
        multi7_node.start_nodes(wait_leader=True, wait_ready=False)

        # Post and receive message after each redeploy
        messagesCounter.post_message(producer, uri_priority)
        messagesCounter.assert_posted(consumer, uri_priority)

    # Post and receive final message after all nodes have been redeployed
    messagesCounter.post_message(producer, uri_priority)
    messagesCounter.assert_posted(consumer, uri_priority)


@start_cluster(start=True, wait_leader=True, wait_ready=True)
def test_redeploy_one_by_one(multi7_node: Cluster, domain_urls: tc.DomainUrls):
    """
    Test to upgrade binaries of cluster nodes one by one.
    Every time a node is upgraded, only this node is restarted.
    """

    uri_priority = domain_urls.uri_priority

    # Start a producer and consumer
    proxies = multi7_node.proxy_cycle()
    producer = next(proxies).create_client("producer")
    consumer = next(proxies).create_client("consumer")

    # Open queue
    producer.open(uri_priority, flags=["write", "ack"], succeed=True)
    consumer.open(uri_priority, flags=["read"], succeed=True)

    # Post and receive initial message
    messagesCounter = MessagesCounter()
    messagesCounter.post_message(producer, uri_priority)
    messagesCounter.assert_posted(consumer, uri_priority)

    multi7_node.disable_exit_code_check()

    for broker in multi7_node.nodes():
        # Stop one node
        broker.stop()
        multi7_node.make_sure_node_stopped(broker)

        # Update binary for the given broker
        multi7_node.update_broker_binary(broker.config, NEW_VERSION_SUFFIX)

        # Restart the stopped node
        broker.start()
        broker.wait_until_started()

        # Here we do not have to wait for leader election and cluster ready state,
        # because we are restarting only one node
        # so cluster is likely to stay in ready state
        # and leader don't lose quorum.

        # Post and receive message after each redeploy
        messagesCounter.post_message(producer, uri_priority)
        messagesCounter.assert_posted(consumer, uri_priority)

    # Post and receive final message after all nodes have been redeployed
    messagesCounter.post_message(producer, uri_priority)
    messagesCounter.assert_posted(consumer, uri_priority)
