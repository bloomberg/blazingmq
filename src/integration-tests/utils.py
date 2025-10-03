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
Utility function for testing rollover
"""

import blazingmq.dev.it.testconstants as tc

from blazingmq.dev.it.fixtures import (
    test_logger,
)

from blazingmq.dev.it.process.broker import Broker
from blazingmq.dev.it.process.client import Client


def simulate_rollover(du: tc.DomainUrls, leader: Broker, producer: Client):
    i = 0
    # Open queues until rollover detected
    while not leader.outputs_regex(r"Rolling over from log with logId", 0.01):
        producer.open(
            f"bmq://{du.domain_priority}/q_dummy_{i}",
            flags=["write,ack"],
            succeed=True,
        )
        producer.close(f"bmq://{du.domain_priority}/q_dummy_{i}", succeed=True)
        i += 1

        if i % 5 == 0:
            # do not wait for success, otherwise the following capture will fail
            leader.force_gc_queues(block=False)

        assert i < 10000, (
            "Failed to detect rollover after opening a reasonable number of queues"
        )
    test_logger.info(f"Rollover detected after opening {i} queues")

    # Rollover and queueUnAssignmentAdvisory interleave
    assert leader.outputs_regex(r"queueUnAssignmentAdvisory", timeout=5)
