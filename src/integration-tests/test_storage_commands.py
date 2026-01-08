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
)


class TestStorageCommands:
    def test_command_rollover_partitionid(
        self,
        cluster: Cluster,
        domain_urls: tc.DomainUrls,  # pylint: disable=unused-argument
    ):
        """
        Test that command STORAGE PARTITION ROLLOVER can accept partition IDs from -1 to NUM_PARTITIONS-1
        """

        num_partitions = cluster.config.definition.partition_config.num_partitions

        leader = cluster.last_known_leader

        all_partition_id = -1
        res = leader.trigger_rollover(all_partition_id, succeed=True, timeout=11)
        assert not res is None

        for partition_id in range(num_partitions):
            res = leader.trigger_rollover(partition_id, succeed=True, timeout=11)
            assert not res is None

        res = leader.trigger_rollover(num_partitions, succeed=True, timeout=11)
        assert res is None

    def test_command_storage_partition_summary_partitionid(
        self,
        cluster: Cluster,
        domain_urls: tc.DomainUrls,  # pylint: disable=unused-argument
    ):
        """
        Test that command STORAGE PARTITION SUMMARY can accept partition IDs from 0 to NUM_PARTITIONS-1
        """

        num_partitions = cluster.config.definition.partition_config.num_partitions

        leader = cluster.last_known_leader

        invalid_partition_id = -1
        res = leader.get_storage_partition_summary(
            invalid_partition_id, succeed=True, timeout=11
        )
        assert res is None

        for partition_id in range(num_partitions):
            res = leader.get_storage_partition_summary(
                partition_id, succeed=True, timeout=11
            )
            assert not res is None

        res = leader.get_storage_partition_summary(
            num_partitions, succeed=True, timeout=11
        )
        assert res is None
