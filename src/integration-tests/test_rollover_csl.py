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
Testing rollover of CSL file.
"""
import blazingmq.dev.it.testconstants as tc
from blazingmq.dev.it.fixtures import (  # pylint: disable=unused-import
    Cluster,
    cluster,
    order,
    test_logger,
    tweak,
)
from blazingmq.dev.it.util import wait_until
import time

pytestmark = order(4)

default_app_ids = ["foo", "bar", "baz"]
timeout = 120


class TestRolloverCSL:
    @tweak.cluster.partition_config.max_cslfile_size(2000)
    @tweak.cluster.queue_operations.keepalive_duration_ms(1000)
    def test_rollover_queue_assignments(self, cluster: Cluster):
        leader = cluster.last_known_leader
        proxy = next(cluster.proxy_cycle())
        self.producer = proxy.create_client("producer")

        # Cause three QueueAssignmentAdvisories and QueueUnassignedAdvisories to be written to the CSL.  These records will be erased during rollover.
        for i in range(0, 3):
            self.producer.open(
                f"bmq://{tc.DOMAIN_PRIORITY_SC}/q{i}", flags=["write,ack"], succeed=True
            )
            self.producer.close(f"bmq://{tc.DOMAIN_PRIORITY_SC}/q{i}", succeed=True)
        for i in range(0, 3):
            assert leader.outputs_regex(r"QueueUnassignedAdvisory", timeout)

        self.producer.open(
            f"bmq://{tc.DOMAIN_FANOUT_SC}/q0", flags=["write,ack"], succeed=True
        )
        self.producer.post(
            f"bmq://{tc.DOMAIN_FANOUT_SC}/q0", ["msg1"], succeed=True, wait_ack=True
        )

        # Assigning these two queues will cause rollover
        self.producer.open(
            f"bmq://{tc.DOMAIN_PRIORITY_SC}/q_last", flags=["write,ack"], succeed=True
        )
        self.producer.open(
            f"bmq://{tc.DOMAIN_PRIORITY_SC}/q_last_2", flags=["write,ack"], succeed=True
        )
        assert leader.outputs_regex(r"Rolling over from log with logId", timeout)

        cluster.restart_nodes()
        # For a standard cluster, states have already been restored as part of
        # leader re-election.
        if cluster.is_single_node:
            self.producer.wait_state_restored()

        consumers = {}

        for app_id in default_app_ids:
            consumer = next(cluster.proxy_cycle()).create_client(app_id)
            consumers[app_id] = consumer
            consumer.open(
                f"bmq://{tc.DOMAIN_FANOUT_SC}/q0?id={app_id}",
                flags=["read"],
                succeed=True,
            )

        for app_id in default_app_ids:
            test_logger.info(f"Check if {app_id} still has 1 message")
            assert wait_until(
                lambda: len(
                    consumers[app_id].list(
                        f"bmq://{tc.DOMAIN_FANOUT_SC}/q0?id={app_id}", block=True
                    )
                )
                == 1,
                3,
            )
