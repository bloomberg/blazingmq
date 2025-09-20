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
This test case verifies a fix for the broker crash when replica or proxy node
crashes while processing a configure or close queue response after a reopen
queue has failed. All nodes going down gracefully at cluster shutdown verifies
the fix.
"""

import blazingmq.dev.it.testconstants as tc
from blazingmq.dev.it.fixtures import (  # pylint: disable=unused-import
    Cluster,
    order,
)

pytestmark = order(6)


def test_reopen_queue_failure(cluster: Cluster, domain_urls: tc.DomainUrls):
    du = domain_urls
    proxies = cluster.proxy_cycle()

    # We want proxy connected to a replica
    next(proxies)

    proxy = next(proxies)
    consumer = proxy.create_client("consumer")

    consumer.open(du.uri_priority, flags=["read"], succeed=True)

    # Set the quorum of all non-leader nodes to 99 to prevent them from
    # becoming a new leader
    leader = cluster.last_known_leader
    next_leader = None
    for node in cluster.nodes():
        # NOTE: Following assumes 4-node cluster
        if node != leader:
            node.set_quorum(99)
            if node.datacenter == leader.datacenter:
                next_leader = node
    assert leader != next_leader

    # Kill the leader
    cluster.drain()
    leader.check_exit_code = False
    leader.kill()
    leader.wait()

    # Remove routing config on the next leader (to cause reopen
    # queue failure)
    cluster.work_dir.joinpath(
        next_leader.name, "etc", "domains", f"{du.domain_priority}.json"
    ).unlink()
    next_leader.command("DOMAINS RESOLVER CACHE_CLEAR ALL")

    # Make the quorum for selected node be 1 so it becomes new leader
    next_leader.set_quorum(1)

    # Wait for new leader
    cluster.wait_leader()
    assert cluster.last_known_leader == next_leader

    consumer.stop_session()
