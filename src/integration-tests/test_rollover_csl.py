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
from blazingmq.dev.it.util import wait_until
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

        i = 0
        # Open queues until rollover detected
        while not leader.outputs_regex(r"Rolling over from log with logId", 0.01):
            producer.open(
                f"bmq://{domain_priority}/q{i}", flags=["write,ack"], succeed=True
            )
            producer.close(f"bmq://{domain_priority}/q{i}", succeed=True)
            i += 1
            assert i < 10000, (
                "Failed to detect rollover after opening a reasonable number of queues"
            )
        test_logger.info(f"Rollover detected after opening {i} queues")

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

        cluster.restart_nodes()
        # For a standard cluster, states have already been restored as part of
        # leader re-election.
        if cluster.is_single_node:
            producer.wait_state_restored()

        # EPILOGUE
        test_logger.info("Check if priority queue still has 1 message")
        consumer.open(
            priority_queue,
            flags=["read"],
            succeed=True,
        )
        assert wait_until(
            lambda: len(consumer.list(priority_queue, block=True)) == 1,
            3,
        )

        test_logger.info("Check if app 'foo' still has 2 messages")
        consumer.open(
            fanout_queue + "?id=foo",
            flags=["read"],
            succeed=True,
        )
        assert wait_until(
            lambda: len(consumer.list(fanout_queue + "?id=foo", block=True)) == 2,
            3,
        )

        test_logger.info("Verify that removed app 'bar' cannot receive any message")
        consumer.open(
            fanout_queue + "?id=bar",
            flags=["read"],
            succeed=True,
        )
        assert wait_until(
            lambda: len(consumer.list(fanout_queue + "?id=bar", block=True)) == 0,
            3,
        )

        test_logger.info("Check if app 'baz' still has 3 messages")
        consumer.open(
            fanout_queue + "?id=baz",
            flags=["read"],
            succeed=True,
        )
        assert wait_until(
            lambda: len(consumer.list(fanout_queue + "?id=baz", block=True)) == 3,
            3,
        )

        test_logger.info("Check if app 'quux' still has 1 message")
        consumer.open(
            fanout_queue + "?id=quux",
            flags=["read"],
            succeed=True,
        )
        assert wait_until(
            lambda: len(consumer.list(fanout_queue + "?id=quux", block=True)) == 1,
            3,
        )
