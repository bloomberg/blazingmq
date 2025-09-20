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
This test case verifies fix for the broker crash when virtual iterator goes
out of sync while processing CONFIRM after converting priority queue to
local.
"""

import blazingmq.dev.it.testconstants as tc
from blazingmq.dev.it.fixtures import (
    Cluster,  # pylint: disable=unused-import
)
from blazingmq.dev.it.process.client import Client
from blazingmq.dev.it.util import wait_until


def test_confirm_after_killing_primary(cluster: Cluster, domain_urls: tc.DomainUrls):
    uri_priority = domain_urls.uri_priority
    proxies = cluster.proxy_cycle()

    # we want proxy connected to a replica
    next(proxies)

    proxy = next(proxies)
    consumer = proxy.create_client("consumer")
    producer = proxy.create_client("producer")

    producer.open(uri_priority, flags=["write", "ack"], succeed=True)
    consumer.open(uri_priority, flags=["read"], succeed=True)

    producer.post(uri_priority, payload=["msg1"], wait_ack=True, succeed=True)

    consumer.wait_push_event()
    assert wait_until(lambda: len(consumer.list(uri_priority, block=True)) == 1, 2)
    msgs = consumer.list(uri_priority, block=True)
    assert msgs[0].payload == "msg1"

    # make the quorum for replica to be 1 so it becomes new primary
    replica = cluster.process(proxy.get_active_node())
    for node in cluster.nodes():
        if node == replica:
            node.set_quorum(1)
        else:
            node.set_quorum(99)

    # kill the primary
    replica.drain()
    cluster.drain()
    leader = cluster.last_known_leader
    leader.check_exit_code = False
    leader.kill()
    leader.wait()

    # wait for new leader
    cluster.wait_leader()
    assert cluster.last_known_leader == replica

    # need to wait for remote queue converted to local
    # otherwise CONFIRM/PUT can get rejected if happen in between the
    # conversion
    assert replica.outputs_substr(
        f"Rebuilt internal state of queue engine for queue [{uri_priority}]",
        timeout=5,
    )

    # confirm
    assert consumer.confirm(uri_priority, "*", block=True) == Client.e_SUCCESS
    # post
    producer.post(uri_priority, payload=["msg2"], wait_ack=True, succeed=True)

    # verify that replica did not crash
    consumer.wait_push_event()
    assert wait_until(lambda: len(consumer.list(uri_priority, block=True)) == 1, 2)
    msgs = consumer.list(uri_priority, block=True)
    assert msgs[0].payload == "msg2"
