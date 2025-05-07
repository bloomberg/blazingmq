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
Integration test that temporarily suspends the leader node, waits for followers
to notice the inactivity of the leader ("leader passive"), then resumes the
leader and verifies that all follower nodes successfully notice the leader
transitioning from PASSIVE to ACTIVE.
"""

import blazingmq.dev.it.testconstants as tc
from blazingmq.dev.it.fixtures import Cluster
from blazingmq.dev.it.fixtures import (  # pylint: disable=unused-import
    multi_node as cluster,
    order,
)

pytestmark = order(6)


def test_leader_node_delay(
    cluster: Cluster,
    domain_urls: tc.DomainUrls,  # pylint: disable=unused-argument
):
    leader = cluster.last_known_leader
    followers = [node for node in cluster.nodes() if node is not leader]

    # 1. Suspend leader node and wait until followers notice:

    for follower in followers:
        # We don't want the follower nodes to elect a leader among
        # themselves when we suspend the current leader
        follower.set_quorum(100, succeed=True)

        # make sure the folllower is available
        consumer = follower.create_client("consumer")
        consumer.open(tc.URI_BROADCAST, flags=["read"], succeed=True)
        consumer.close(tc.URI_BROADCAST, succeed=True)
        consumer.exit_gracefully()

    leader.suspend()

    for follower in followers:
        assert follower.outputs_substr("new code: LEADER_NO_HEARTBEAT", 120)

    # 2. Resume leader, then verify each follower node recognizes the
    #    transition of the leader from passive to active:
    leader.resume()

    for follower in followers:
        assert follower.outputs_regex(
            "#ELECTOR_INFO: leader.*transitioning status from PASSIVE to ACTIVE",
            timeout=60,
        )
