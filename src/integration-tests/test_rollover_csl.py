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
from blazingmq.dev.it.fixtures import (
    Cluster,
    order,
    test_logger,
    tweak,
)
from blazingmq.dev.it.tests.tests_utils import (
    simulate_csl_rollover,
    check_if_queue_has_n_messages,
)
import glob

pytestmark = order(4)

default_app_ids = ["foo", "bar", "baz"]


class TestRolloverCSL:
    @tweak.cluster.partition_config.max_cslfile_size(2000)
    def test_csl_cleanup(self, cluster: Cluster, domain_urls: tc.DomainUrls):
        """
        Test that rolling over CSL cleans up the old file.
        """
        leader = cluster.last_known_leader
        proxy = next(cluster.proxy_cycle())
        domain_priority = domain_urls.domain_priority

        producer = proxy.create_client("producer")

        csl_files_before_rollover = glob.glob(
            str(cluster.work_dir.joinpath(leader.name, "storage")) + "/*csl*"
        )
        assert len(csl_files_before_rollover) == 1

        simulate_csl_rollover(domain_urls, leader, producer)

        csl_files_after_rollover = glob.glob(
            str(cluster.work_dir.joinpath(leader.name, "storage")) + "/*csl*"
        )
        assert len(csl_files_after_rollover) == 1
        assert csl_files_before_rollover[0] != csl_files_after_rollover[0]

    @tweak.cluster.partition_config.max_cslfile_size(3000)
    @tweak.cluster.queue_operations.keepalive_duration_ms(1000)
    def test_rollover_queues_and_apps(
        self, cluster: Cluster, domain_urls: tc.DomainUrls
    ):
        """
        Test that queue and appId information are preserved across rollover of
        CSL file, even after cluster restart.

        1. PROLOGUE:
            - both priority and fanout queues
            - post two messages
            - confirm one on priority and "foo"
            - add "quux" to the fanout
            - post to fanout
            - remove "bar"
        2. SWITCH:
            By opening and GC'ing queues, cause the CSL file to rollover.
            Then, restart the cluster.
        3. EPILOGUE:
            - priority consumer gets the second message
            - "foo" gets 2 messages
            - "bar" gets 0 messages
            - "baz" gets 3 messages
            - "quux" gets the third message
        """
        du = domain_urls
        leader = cluster.last_known_leader
        proxy = next(cluster.proxy_cycle())
        producer = proxy.create_client("producer")

        # PROLOGUE
        priority_queue = f"bmq://{du.domain_priority}/q_in_use"
        fanout_queue = f"bmq://{du.domain_fanout}/q_in_use"
        for queue in [priority_queue, fanout_queue]:
            producer.open(queue, flags=["write,ack"], succeed=True)
            producer.post(
                queue,
                ["msg1", "msg2"],
                succeed=True,
                wait_ack=True,
            )

        consumer = proxy.create_client("consumer")
        consumer.open(
            priority_queue,
            flags=["read"],
            succeed=True,
        )
        consumer.open(
            fanout_queue + "?id=foo",
            flags=["read"],
            succeed=True,
        )
        consumer.confirm(priority_queue, "+1", succeed=True)
        consumer.confirm(fanout_queue + "?id=foo", "+1", succeed=True)
        consumer.close(priority_queue, succeed=True)
        consumer.close(fanout_queue + "?id=foo", succeed=True)

        current_app_ids = default_app_ids + ["quux"]
        cluster.set_app_ids(current_app_ids, du)
        producer.post(
            fanout_queue,
            ["msg3"],
            succeed=True,
            wait_ack=True,
        )

        current_app_ids.remove("bar")
        cluster.set_app_ids(current_app_ids, du)

        # SWITCH
        simulate_csl_rollover(du, leader, producer)

        cluster.restart_nodes()
        # For a standard cluster, states have already been restored as part of
        # leader re-election.
        if cluster.is_single_node:
            producer.wait_state_restored()

        # EPILOGUE
        check_if_queue_has_n_messages(consumer, priority_queue, 1)
        check_if_queue_has_n_messages(consumer, fanout_queue + "?id=foo", 2)
        check_if_queue_has_n_messages(consumer, fanout_queue + "?id=bar", 0)
        check_if_queue_has_n_messages(consumer, fanout_queue + "?id=baz", 3)
        check_if_queue_has_n_messages(consumer, fanout_queue + "?id=quux", 1)
