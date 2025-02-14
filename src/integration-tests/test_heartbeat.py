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
Integration test that suspends leader/replica/proxy/tool to trigger missed
hearbeats followed by dead channel detection
"""

import blazingmq.dev.it.testconstants as tc
from blazingmq.dev.it.fixtures import Cluster
from blazingmq.dev.it.fixtures import (  # pylint: disable=unused-import
    multi_node as cluster,
    order,
    tweak,
)
from blazingmq.dev.it.util import wait_until

pytestmark = order(6)


def _verify_delivery_to_new_consumer(proxy, uri, messages, timeout=2):
    consumer = proxy.create_client("consumer")
    consumer.open(uri, flags=["read"], succeed=True)
    _verify_delivery(consumer, uri, ["msg1", "masg2"], timeout=2)
    consumer.exit_gracefully()


def _verify_delivery(consumer, uri, messages, timeout=2):
    consumer.wait_push_event()
    assert wait_until(
        lambda: len(consumer.list(uri, block=True)) == len(messages), timeout
    )
    consumer.list(uri, block=True)


def _verify_delivery_and_confirm(consumer, uri, messages):
    _verify_delivery(consumer, uri, messages)
    assert consumer.confirm(uri, "*", block=True) == Client.e_SUCCESS


"""
`cluster_peer`        cluster node -> cluster node
`upstream_broker`     proxy -> cluster node
`downstream_broker`   broker -> proxy 
`client`              broker -> SDK  _and_  SDK -> broker
"""


@tweak.broker.app_config.network_interfaces.heartbeats.cluster_peer(2)
@tweak.broker.app_config.network_interfaces.heartbeats.upstream_broker(2)
@tweak.broker.app_config.network_interfaces.tcp_interface.heartbeat_interval_ms(100)
def test_dead_leader(cluster: Cluster):
    leader = cluster.last_known_leader
    proxies = cluster.proxy_cycle()
    next(proxies)
    proxy = next(proxies)

    # Start a producer and post a message
    producer = proxy.create_client("producer")
    producer.open(tc.URI_FANOUT_SC, flags=["write", "ack"], succeed=True)
    producer.post(tc.URI_FANOUT_SC, ["msg1"], succeed=True, wait_ack=True)

    # Start a consumer
    consumer = proxy.create_client("consumer")
    consumer.open(tc.URI_FANOUT_SC_FOO, flags=["read"], succeed=True)

    _verify_delivery(consumer, tc.URI_FANOUT_SC_FOO, ["msg1"], timeout=2)

    replica = cluster.process(proxy.get_active_node())

    # replica.set_quorum(1, succeed=True)
    # leader.set_quorum(10, succeed=True)

    # imitate dead channel by suspending the leader process
    leader.suspend()

    # replica should detect dead channel
    replica.capture(r"#TCP_DEAD_CHANNEL", 5)

    # proxy should detect dead channel (it is connected to every node)
    proxy.capture(r"#TCP_DEAD_CHANNEL", 5)

    # wait for new leader
    replica.wait_status(wait_leader=True, wait_ready=False)

    # leader.resume()

    # cannot resume leader at the moment due to the "twin leaders" problem
    # For example, in the legacy mode:
    # "self node views self as active/available primary, but a different node
    # is proposed as primary in the partition/primary mapping: ... This
    # downgrade from primary to replica is currently not supported, and self
    # node will exit."
    # #EXIT Terminating BMQBRKR with error 'UNSUPPORTED_SCENARIO'

    assert not leader == replica.last_known_leader

    # post new message and verify delivery
    producer.post(tc.URI_FANOUT_SC, ["msg2"], succeed=True, wait_ack=True)
    _verify_delivery(consumer, tc.URI_FANOUT_SC_FOO, ["msg1", "masg2"], timeout=2)

    # since the leader is still suspended (see above), kill it to stop the
    # cluster gracefully
    leader.check_exit_code = False
    leader.kill()

    # verify delivery for a new consumer
    consumer.exit_gracefully()

    _verify_delivery_to_new_consumer(
        proxy, tc.URI_FANOUT_SC_FOO, ["msg1", "masg2"], timeout=2
    )

    producer.exit_gracefully()


@tweak.broker.app_config.network_interfaces.heartbeats.downstream_broker(3)
@tweak.broker.app_config.network_interfaces.heartbeats.client(3)
@tweak.broker.app_config.network_interfaces.tcp_interface.heartbeat_interval_ms(100)
def test_dead_proxy(cluster: Cluster):
    leader = cluster.last_known_leader
    proxies = cluster.proxy_cycle()
    next(proxies)
    proxy = next(proxies)

    # Start a producer and post a message
    producer = proxy.create_client("producer")
    producer.open(tc.URI_FANOUT_SC, flags=["write", "ack"], succeed=True)
    producer.post(tc.URI_FANOUT_SC, ["msg1"], succeed=True, wait_ack=True)

    # Start a consumer
    consumer = proxy.create_client("consumer")
    consumer.open(tc.URI_FANOUT_SC_FOO, flags=["read"], succeed=True)

    _verify_delivery(consumer, tc.URI_FANOUT_SC_FOO, ["msg1"], timeout=2)

    replica = cluster.process(proxy.get_active_node())

    # imitate dead channel by suspending the proxy to which SDK is connected
    proxy.suspend()

    # producer should detect dead channel
    producer.capture(r"#TCP_DEAD_CHANNEL", 5)
    # consumer should detect dead channel
    consumer.capture(r"#TCP_DEAD_CHANNEL", 5)
    # active node of the proxy should detect dead channel
    replica.capture(r"#TCP_DEAD_CHANNEL", 5)

    # post to SDK when it has no connection should still work
    producer.post(tc.URI_FANOUT_SC, ["msg2"], succeed=True, wait_ack=True)

    # SDK has no other endpoint to fail over.  It should keep reconnecting.
    proxy.resume()

    # capture reconnection events
    producer.capture(r"RECONNECTED", 5)
    consumer.capture(r"RECONNECTED", 5)

    _verify_delivery(consumer, tc.URI_FANOUT_SC_FOO, ["msg1", "masg2"], timeout=2)

    consumer.exit_gracefully()
    consumer.wait()

    # verify delivery for a new consumer
    _verify_delivery_to_new_consumer(
        proxy, tc.URI_FANOUT_SC_FOO, ["msg1", "masg2"], timeout=2
    )

    producer.exit_gracefully()


@tweak.broker.app_config.network_interfaces.heartbeats.cluster_peer(2)
@tweak.broker.app_config.network_interfaces.heartbeats.upstream_broker(2)
@tweak.broker.app_config.network_interfaces.heartbeats.client(2)
@tweak.broker.app_config.network_interfaces.tcp_interface.heartbeat_interval_ms(100)
def test_dead_replica(cluster: Cluster):
    leader = cluster.last_known_leader
    proxies = cluster.proxy_cycle()
    next(proxies)
    proxy = next(proxies)

    # Start a producer and post a message
    producer = proxy.create_client("producer")
    producer.open(tc.URI_FANOUT_SC, flags=["write", "ack"], succeed=True)
    producer.post(tc.URI_FANOUT_SC, ["msg1"], succeed=True, wait_ack=True)

    # Open consumers for all appIds and ensure that the one with 'foo' appId
    # does not receive the message, while other consumers do.
    consumer = proxy.create_client("consumer")
    consumer.open(tc.URI_FANOUT_SC_FOO, flags=["read"], succeed=True)

    _verify_delivery(consumer, tc.URI_FANOUT_SC_FOO, ["msg1"], timeout=2)

    replica = cluster.process(proxy.get_active_node())

    # imitate dead channel by suspending the active node of the proxy
    replica.suspend()

    # leader should detect dead channel
    leader.capture(r"#TCP_DEAD_CHANNEL", 5)

    # proxy should detect dead channel
    proxy.capture(r"#TCP_DEAD_CHANNEL", 5)

    # proxy should fail over to another active node
    failover = cluster.process(proxy.get_active_node())
    assert failover
    assert not failover == replica

    replica.resume()

    producer.post(tc.URI_FANOUT_SC, ["msg2"], succeed=True, wait_ack=True)

    _verify_delivery(consumer, tc.URI_FANOUT_SC_FOO, ["msg1", "masg2"], timeout=2)

    consumer.exit_gracefully()
    consumer.wait()

    # verify delivery for a new consumer
    _verify_delivery_to_new_consumer(
        proxy, tc.URI_FANOUT_SC_FOO, ["msg1", "masg2"], timeout=2
    )

    producer.exit_gracefully()
