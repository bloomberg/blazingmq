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
Testing migrating to CSL.
"""
import blazingmq.dev.it.testconstants as tc
from blazingmq.dev.it.fixtures import (  # pylint: disable=unused-import
    Cluster,
    multi_node,
)


def test_assign_queue(multi_node: Cluster):
    """
    This test is to make sure when a queue assignment happens,
    it goes thought the CSL path, not the non-CSL.
    """
    proxies = multi_node.proxy_cycle()
    proxy = next(proxies)

    leader = multi_node.last_known_leader
    members = multi_node.nodes(exclude=leader)

    uri = tc.URI_PRIORITY_SC

    producer = proxy.create_client("producer")
    producer.open(uri, flags=["write"], succeed=True)

    timeout = 1

    for member in members:
        assert member.outputs_regex(
            "(Applying cluster message with type = UPDATE).*(queueAssignmentAdvisory)",
            timeout,
        )
        assert member.outputs_regex(
            "(Committed advisory).*queueAssignmentAdvisory", timeout
        )
        assert not member.outputs_regex(
            "'QueueUnAssignmentAdvisory' will be applied to", timeout
        )
