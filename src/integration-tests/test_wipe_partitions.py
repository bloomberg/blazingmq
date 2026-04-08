# Copyright 2026 Bloomberg Finance L.P.
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
Testing node recovery after wiping partition files.
"""

import blazingmq.dev.it.testconstants as tc
from blazingmq.dev.it.fixtures import (  # pylint: disable=unused-import
    Cluster,
    multi_node,
    order,
    tweak,
)
from blazingmq.dev.it.cluster_util import (
    stop_cluster_and_compare_journal_files,
    wipe_files,
)

pytestmark = order(4)


@tweak.cluster.queue_operations.shutdown_timeout_ms(100)
def test_wipe_single_partition_files(
    multi_node: Cluster,
    domain_urls: tc.DomainUrls,
) -> None:
    """
    Test that a replica can recover after one partition's files are wiped.
    - start cluster
    - post messages
    - stop a replica
    - wipe partition 0's journal, data, and qlist files on the replica
    - restart the replica
    - verify the replica synchronizes with the primary
    """
    cluster = multi_node
    uri_priority = domain_urls.uri_priority

    leader = cluster.last_known_leader
    proxy = next(cluster.proxy_cycle())

    producer = proxy.create_client("producer")
    producer.open(uri_priority, flags=["write,ack"], succeed=True)

    consumer = proxy.create_client("consumer")
    consumer.open(uri_priority, flags=["read"], succeed=True)

    # Post messages
    for i in range(1, 5):
        producer.post(uri_priority, [f"msg{i}"], succeed=True, wait_ack=True)

    replica = cluster.nodes(exclude=leader)[0]

    # Stop the replica
    replica.stop()
    cluster.make_sure_node_stopped(replica)
    replica.drain()

    # Wipe partition 0's files (journal + data + qlist)
    storage_dir = str(cluster.work_dir.joinpath(replica.name, "storage"))
    wipe_files(
        ["*0*.bmq_journal", "*0*.bmq_data", "*0*.bmq_qlist"],
        storage_dir,
    )

    # Restart the replica
    replica.start()
    replica.wait_until_started()

    assert replica.outputs_substr("Cluster (itCluster) is available", 10), (
        f"Replica {replica} did not become available within 10s after "
        f"partition 0 files were wiped"
    )

    assert leader == cluster.last_known_leader, (
        f"Leader {leader} is not cluster.last_known_leader {cluster.last_known_leader}"
    )

    stop_cluster_and_compare_journal_files(leader.name, replica.name, cluster)


@tweak.cluster.queue_operations.shutdown_timeout_ms(100)
def test_wipe_all_partitions_and_csl(
    multi_node: Cluster,
    domain_urls: tc.DomainUrls,
) -> None:
    """
    Test that a replica can recover after ALL partition files and the CSL
    file are wiped.
    - start cluster
    - post messages
    - stop a replica
    - wipe all journal, data, qlist, and CSL files on the replica
    - restart the replica
    - verify the replica synchronizes with the primary
    """
    cluster = multi_node
    uri_priority = domain_urls.uri_priority

    leader = cluster.last_known_leader
    proxy = next(cluster.proxy_cycle())

    producer = proxy.create_client("producer")
    producer.open(uri_priority, flags=["write,ack"], succeed=True)

    consumer = proxy.create_client("consumer")
    consumer.open(uri_priority, flags=["read"], succeed=True)

    # Post messages
    for i in range(1, 5):
        producer.post(uri_priority, [f"msg{i}"], succeed=True, wait_ack=True)

    replica = cluster.nodes(exclude=leader)[0]

    # Stop the replica
    replica.stop()
    cluster.make_sure_node_stopped(replica)
    replica.drain()

    # Wipe ALL partition files and CSL file
    storage_dir = str(cluster.work_dir.joinpath(replica.name, "storage"))
    wipe_files(
        ["*.bmq_journal", "*.bmq_data", "*.bmq_qlist", "*csl*"],
        storage_dir,
    )

    # Restart the replica
    replica.start()
    replica.wait_until_started()

    assert replica.outputs_substr("Cluster (itCluster) is available", 10), (
        f"Replica {replica} did not become available within 10s after "
        f"all partition files were wiped"
    )

    assert leader == cluster.last_known_leader, (
        f"Leader {leader} is not cluster.last_known_leader {cluster.last_known_leader}"
    )

    stop_cluster_and_compare_journal_files(leader.name, replica.name, cluster)
