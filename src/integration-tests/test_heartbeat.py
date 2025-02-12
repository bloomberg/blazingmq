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

pytestmark = order(6)

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

    # Open consumers for all appIds and ensure that the one with 'foo' appId
    # does not receive the message, while other consumers do.
    consumer = proxy.create_client("consumer1")
    consumer.open(tc.URI_FANOUT_SC_FOO, flags=["read"], succeed=True)
    
    replica = cluster.process(proxy.get_active_node())
    
    # replica.set_quorum(1, succeed=True)
    # leader.set_quorum(10, succeed=True)
    
    leader.suspend()

    proxy.capture(r"#TCP_DEAD_CHANNEL", 5)
    replica.capture(r"#TCP_DEAD_CHANNEL", 5)
    replica.wait_status(wait_leader=True, wait_ready=False)

    # leader.resume()
    
    # cannot resume leader at them moment due to the "twin leaders" problem
    # For example, in the legacy mode: 
    # "self node views self as active/available primary, but a different node is proposed as primary in the partition/primary mapping: ... This downgrade from primary to replica is currently not supported, and self node will exit."
    # #EXIT Terminating BMQBRKR with error 'UNSUPPORTED_SCENARIO'
    
    assert not leader == replica.last_known_leader
    
    # since the leader is still suspended (see above), kill it to stop the
    # cluster gracefully
    
    leader.check_exit_code = False
    leader.kill()
    
    consumer.exit_gracefully()
    producer.exit_gracefully()