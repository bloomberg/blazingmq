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
Integration tests for queue re-open scenarios.
"""

import time
import blazingmq.dev.it.testconstants as tc
from blazingmq.dev.it.fixtures import (  # pylint: disable=unused-import
    Cluster,
    multi_node,
)
from blazingmq.dev.it.process.client import Client


def test_client_timeout_open(multi_node: Cluster, domain_urls: tc.DomainUrls):
    """
    If opening a queue times out on the client, the client should kill the
    channel, dropping all queue connections, and mark the queue as closed.
    """
    uri_priority = domain_urls.uri_priority
    brokers = multi_node.nodes()
    leader = multi_node.last_known_leader

    # Ensure the producer is connected to a replica
    producer_broker = brokers[0] if brokers[0] is not leader else brokers[1]

    # Start a producer with a long timeout and open queue, to ensure the queue
    # is assigned.
    longTimeoutProducer = leader.create_client("producer_longtimeout")
    longTimeoutProducer.open(uri_priority, flags=["write,ack"], succeed=True)

    # Start a producer with a short timeout
    producer = producer_broker.create_client("producer", options=["--timeout=1"])

    # Suspend rest of the cluster to force future open to timeout
    for broker in brokers:
        if broker is not producer_broker:
            broker.suspend()

    # Open a queue while the cluster does not have quorum should timeout.
    assert (
        producer.open(uri_priority, flags=["write,ack"], succeed=False)
        == Client.e_TIMEOUT
    )

    # Release suspended brokers
    for broker in brokers:
        if broker is not producer_broker:
            broker.resume()
    producer_broker.capture("is back to healthy state")

    # Post after short timeout should result in failed Ack (open is not
    # retried).
    assert (
        producer.post(
            uri_priority, ["1"], wait_ack=False, succeed=False, no_except=True
        )
        == Client.e_UNKNOWN
    )

    # But we should be able to open the queue again and post
    producer.open(uri_priority, flags=["write,ack"], succeed=True)
    assert (
        producer.post(uri_priority, ["2"], wait_ack=True, succeed=True)
        == Client.e_SUCCESS
    )


def test_client_timeout_reopen(multi_node: Cluster, domain_urls: tc.DomainUrls):
    """
    If reopening a queue times out on the client, the client should kill the
    channel, dropping all queue connections, and automatically try to reopen
    them when the channel can be re-established.
    """
    uri_priority = domain_urls.uri_priority
    uri_priority2 = domain_urls.uri_priority_2
    brokers = multi_node.nodes()
    leader = multi_node.last_known_leader

    # Ensure the producer is connected to a replica
    producer_broker = brokers[0] if brokers[0] is not leader else brokers[1]

    # Start a producer with a long timeout and open queue, to ensure the queues
    # are assigned.
    longTimeoutProducer = leader.create_client("producer_longtimeout")
    longTimeoutProducer.open(uri_priority, flags=["write,ack"], succeed=True)
    longTimeoutProducer.open(uri_priority2, flags=["write,ack"], succeed=True)

    # Start a producer with a short timeout and open several queues
    producer = producer_broker.create_client("producer", options=["--timeout=1"])
    producer.open(uri_priority, flags=["write,ack"], succeed=True)
    producer.open(uri_priority2, flags=["write,ack"], succeed=True)

    # Suspend rest of the cluster to force future reopen to timeout
    for broker in brokers:
        if broker is not producer_broker:
            broker.suspend()

    # Crash connected broker and restart to force reopen
    producer_broker.stop()
    producer_broker.wait()
    producer_broker.start()

    # Wait past the short reopen timeout
    time.sleep(2)

    # Release suspended brokers
    for broker in brokers:
        if broker is not producer_broker:
            broker.resume()
    producer_broker.capture("is back to healthy state")

    # Post after short timeout should result in successful Ack
    assert (
        producer.post(uri_priority, ["1"], wait_ack=True, succeed=True)
        == Client.e_SUCCESS
    )
    assert (
        producer.post(uri_priority2, ["2"], wait_ack=True, succeed=True)
        == Client.e_SUCCESS
    )

    # Make sure the producer has not seen a QUEUE_REOPEN_RESULT failure.
    assert not producer.outputs_substr(
        '==> EVENT received: [ type = "SESSION" sessionEventType = QUEUE_REOPEN_RESULT statusCode = -',
        5,
    )


def test_client_timeout_close(multi_node: Cluster, domain_urls: tc.DomainUrls):
    """
    If closing a queue times out on the client, the client should kill the
    channel, dropping all queue connections, and mark the queue as closed.
    """
    uri_priority = domain_urls.uri_priority
    brokers = multi_node.nodes()
    leader = multi_node.last_known_leader

    # Ensure the producer is connected to a replica
    producer_broker = brokers[0] if brokers[0] is not leader else brokers[1]

    # Start a producer with a long timeout and open queue, to ensure the queues
    # are assigned.
    longTimeoutProducer = leader.create_client("producer_longtimeout")
    longTimeoutProducer.open(uri_priority, flags=["write,ack"], succeed=True)

    # Start a producer with a short timeout and open the queue
    producer = producer_broker.create_client("producer", options=["--timeout=1"])
    producer.open(uri_priority, flags=["write,ack"], succeed=True)

    # Suspend rest of the cluster to force future short timeout close to timeout
    for broker in brokers:
        if broker is not producer_broker:
            broker.suspend()

    # Close a queue with short timeout while the cluster does not have quorum
    # should correctly report failure, but should reset the connection.
    producer.close(uri_priority, succeed=False)

    # Wait past the short close timeout
    time.sleep(2)

    # Release suspended brokers
    for broker in brokers:
        if broker is not producer_broker:
            broker.resume()
    producer_broker.capture("is back to healthy state")

    # Post after short timeout should result in failed Ack
    assert (
        producer.post(
            uri_priority, ["1"], wait_ack=False, succeed=False, no_except=True
        )
        == Client.e_UNKNOWN
    )

    # But we should be able to open the queue again and post
    producer.open(uri_priority, flags=["write,ack"], succeed=True)
    assert (
        producer.post(uri_priority, ["2"], wait_ack=True, succeed=True)
        == Client.e_SUCCESS
    )
