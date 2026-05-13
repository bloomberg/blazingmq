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

import glob
from functools import partial

import blazingmq.dev.it.testconstants as tc
from blazingmq.dev.it.fixtures import (
    Cluster,
    order,
    tweak,
)
from blazingmq.dev.it.cluster_util import (
    rollover_queues_and_apps_test,
    simulate_csl_rollover,
)

pytestmark = order(4)


class TestRolloverCSL:
    @tweak.cluster.partition_config.max_cslfile_size(2000)
    def test_csl_cleanup(self, cluster: Cluster, domain_urls: tc.DomainUrls):
        """
        Test that rolling over CSL cleans up the old file.
        """
        leader = cluster.last_known_leader
        leader.drain()

        proxy = next(cluster.proxy_cycle())

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
        """

        rollover_queues_and_apps_test(
            cluster, domain_urls, partial(simulate_csl_rollover, domain_urls)
        )
