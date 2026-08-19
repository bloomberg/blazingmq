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
Regression coverage for ReplicaDataRequest partition and sender validation.

`E_PULL`, `E_PUSH` and `E_DROP` are only meaningful from the current primary
of a partition, but any authorized cluster peer can put one on the wire with
any partitionId.  All must be refused at runtime, leaving the broker up.

The tests establish a real authenticated `E_TCPBROKER` session over TCP.  A
configured node is taken down first so that its identity can be claimed.
"""

import os
import re
import time
from typing import Optional, Tuple

import pytest

from blazingmq.dev.it.fixtures import (
    Cluster,
    order,
    start_cluster,
)
from blazingmq.dev.it.process.admin import AdminClient
from blazingmq.dev.it.process.rawclient import RawClient
from blazingmq.dev.it.util import wait_until

pytestmark = order(6)

PARTITION_ID = 0
PRIMARY_FAILOVER_SECONDS = 10
NEGOTIATION_TIMEOUT_SECONDS = 10
NEGOTIATION_INTERVAL_SECONDS = 0.5

LOG_NAME = {"E_PULL": "Pull", "E_PUSH": "Push", "E_DROP": "Drop"}
DATA_TYPES = ["E_PULL", "E_PUSH", "E_DROP"]


def _partition_primary(
    admin: AdminClient, cluster_name: str, partition_id: int
) -> Optional[Tuple[str, int]]:
    """
    Return the (name, id) of the primary of 'partition_id', or None if the
    partition has no primary yet.
    """

    summary = admin.send_admin(f"CLUSTERS CLUSTER {cluster_name} STORAGE SUMMARY")

    # The summary lists partitions in order, so pick the 'partition_id'-th
    # primary it reports.
    primaries = re.findall(r"Primary Node\s*:\s*\[([^,]+),\s*(\d+)\]", summary)
    if len(primaries) <= partition_id:
        return None

    name, node_id = primaries[partition_id]
    return name, int(node_id)


def _wait_for_primary(
    admin: AdminClient,
    cluster_name: str,
    partition_id: int,
    exclude_node_id: Optional[int] = None,
) -> None:
    """
    Wait for 'partition_id' to have a primary other than the optionally
    specified 'exclude_node_id'.
    """

    def _has_usable_primary() -> bool:
        primary = _partition_primary(admin, cluster_name, partition_id)
        return primary is not None and primary[1] != exclude_node_id

    assert wait_until(_has_usable_primary, PRIMARY_FAILOVER_SECONDS), (
        f"partition {partition_id} never got a usable primary"
    )


def _prepare_impersonation(cluster: Cluster):
    """
    Take down a non-leader node so its identity can be claimed, and wait for
    PARTITION_ID to be led by someone else.  Return (victim, impostor, admin),
    where 'admin' is connected to the live victim and owned by the caller.
    """

    leader = cluster.last_known_leader
    assert leader is not None

    others = cluster.nodes(exclude=leader)
    impostor = others[0]
    victim = others[1]

    impostor.check_exit_code = False
    impostor.kill()
    impostor.wait()

    admin = AdminClient()
    admin.connect(victim.config.host, int(victim.config.port))

    try:
        _wait_for_primary(
            admin,
            cluster.config.name,
            PARTITION_ID,
            exclude_node_id=impostor.config.id,
        )
    except Exception:
        admin.stop()
        raise

    return victim, impostor, admin


def _connect_as_cluster_node(host, port, cluster_name, node_name, node_id):
    """
    Open an authenticated cluster-peer session to 'host:port' claiming the
    identity of the configured node 'node_name' / 'node_id'.  Return the
    connected `RawClient`.
    """

    last_result = None

    attempts = int(NEGOTIATION_TIMEOUT_SECONDS / NEGOTIATION_INTERVAL_SECONDS)

    for _ in range(attempts):
        client = RawClient(verbose=True)
        client.open_channel(host, port)

        response = client.negotiate(
            {
                "clientType": "E_TCPBROKER",
                "processName": "it-replica-data-request",
                "pid": os.getpid(),
                "hostName": node_name,
                "clusterName": cluster_name,
                "clusterNodeId": node_id,
            }
        )

        last_result = response.get("brokerResponse", {}).get("result", {})
        if last_result.get("code") == 0:
            return client

        # The victim has not dropped the impersonated node's channel yet.
        client.stop()
        time.sleep(NEGOTIATION_INTERVAL_SECONDS)

    raise AssertionError(f"cluster-peer negotiation never succeeded: {last_result}")


def _replica_data_request(request_id, partition_id, data_type):
    return {
        "rId": request_id,
        "clusterMessage": {
            "partitionMessage": {
                "replicaDataRequest": {
                    "partitionId": partition_id,
                    "primaryLeaseId": 1,
                    "replicaDataType": data_type,
                    "beginSequenceNumber": {
                        "primaryLeaseId": 1,
                        "sequenceNumber": 0,
                    },
                    "endSequenceNumber": {
                        "primaryLeaseId": 1,
                        "sequenceNumber": 0,
                    },
                }
            }
        },
    }


@pytest.mark.parametrize("data_type", DATA_TYPES)
@start_cluster(True, True, True)
def test_non_primary_replica_data_request_is_refused(
    fsm_multi_cluster: Cluster, data_type: str
):
    """
    An authorized cluster peer which is not the primary of a partition sends a
    ReplicaDataRequest for it.  The broker must refuse the request and stay
    alive.
    """

    cluster = fsm_multi_cluster
    cluster_name = cluster.config.name

    victim, impostor, admin = _prepare_impersonation(cluster)

    try:
        client = _connect_as_cluster_node(
            victim.config.host,
            int(victim.config.port),
            cluster_name,
            impostor.name,
            impostor.config.id,
        )

        try:
            client.send_control_message(
                _replica_data_request(4242, PARTITION_ID, data_type)
            )
        finally:
            client.stop()

        # The victim must have refused the request rather than aborted.
        assert victim.outputs_regex(
            rf"Received ReplicaDataRequest{LOG_NAME[data_type]}.*"
            r"but self's perceived primary is.*Sending failure response",
            timeout=30,
        )

        assert victim.is_alive()

        # And it must still serve admin traffic.
        assert admin.send_admin(f"CLUSTERS CLUSTER {cluster_name} STORAGE SUMMARY")
    finally:
        admin.stop()


@pytest.mark.parametrize("data_type", DATA_TYPES)
@start_cluster(True, True, True)
def test_replica_data_request_invalid_partition_is_refused(
    fsm_multi_cluster: Cluster, data_type: str
):
    """
    'partitionId' is peer-controlled.  An out-of-range value must be refused
    rather than used to index partition state.
    """

    cluster = fsm_multi_cluster
    cluster_name = cluster.config.name

    victim, impostor, admin = _prepare_impersonation(cluster)

    try:
        num_partitions = cluster.config.definition.partition_config.num_partitions

        client = _connect_as_cluster_node(
            victim.config.host,
            int(victim.config.port),
            cluster_name,
            impostor.name,
            impostor.config.id,
        )

        try:
            for invalid_partition_id in (-1, num_partitions):
                client.send_control_message(
                    _replica_data_request(4243, invalid_partition_id, data_type)
                )
        finally:
            client.stop()

        assert victim.outputs_regex(
            rf"Received ReplicaDataRequest{LOG_NAME[data_type]}.*invalid partitionId",
            timeout=30,
        )

        assert victim.is_alive()

        assert admin.send_admin(f"CLUSTERS CLUSTER {cluster_name} STORAGE SUMMARY")
    finally:
        admin.stop()
