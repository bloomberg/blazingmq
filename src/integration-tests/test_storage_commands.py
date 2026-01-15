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
from blazingmq.dev.it.fixtures import Cluster, start_cluster


# We have to make sure that wait_ready is True (i.e. primary node is elected)
# Otherwise, the test will fail in multi-node clusters
# because rollover command has to be sent to primary node, but d_primary is still false for all nodes
@start_cluster(start=True, wait_leader=True, wait_ready=True)
def test_command_rollover_partitionid(
    cluster: Cluster,
    domain_urls: tc.DomainUrls,  # pylint: disable=unused-argument
):
    """
    Test that command STORAGE PARTITION ROLLOVER can accept partition IDs from -1 to NUM_PARTITIONS-1
    """

    num_partitions = cluster.config.definition.partition_config.num_partitions

    leader = cluster.last_known_leader

    invalid_partition_id = -2
    res = leader.trigger_rollover(invalid_partition_id, succeed=True)
    assert res is None, (
        f"Rollover for invalid partition {invalid_partition_id} should fail"
    )

    all_partition_id = -1
    res = leader.trigger_rollover(all_partition_id, succeed=True)
    assert not res is None, "Rollover for all partitions should succeed"

    for partition_id in range(num_partitions):
        res = leader.trigger_rollover(partition_id, succeed=True)
        assert not res is None, f"Rollover for partition {partition_id} should succeed"

    res = leader.trigger_rollover(num_partitions, succeed=True)
    assert res is None, f"Rollover for invalid partition {num_partitions} should fail"


def test_command_storage_partition_summary_partitionid(
    cluster: Cluster,
    domain_urls: tc.DomainUrls,  # pylint: disable=unused-argument
):
    """
    Test that command STORAGE PARTITION SUMMARY can accept partition IDs from 0 to NUM_PARTITIONS-1
    """

    num_partitions = cluster.config.definition.partition_config.num_partitions

    leader = cluster.last_known_leader

    invalid_partition_id = -1
    res = leader.get_storage_partition_summary(invalid_partition_id, succeed=True)
    assert res is None, "Summary for invalid partition -1 should fail"

    for partition_id in range(num_partitions):
        res = leader.get_storage_partition_summary(partition_id, succeed=True)
        assert not res is None, f"Summary for partition {partition_id} should succeed"

    res = leader.get_storage_partition_summary(num_partitions, succeed=True)
    assert res is None, f"Summary for invalid partition {num_partitions} should fail"
