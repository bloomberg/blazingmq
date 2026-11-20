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
This suite of test cases exercises connection losses.
"""

import re
from time import sleep
from typing import Dict

import blazingmq.dev.it.testconstants as tc
from blazingmq.dev.it.fixtures import (
    Cluster,
    order,
    test_logger,
    tweak,
    start_cluster,
)
from blazingmq.dev.it.process.admin import AdminClient
from blazingmq.dev.it.process.client import Client
from blazingmq.dev.it.process.proc import Process

pytestmark = order(2)

# NOTE: We run these tests only in strong consistency mode because consistency
# doesn't matter for the tested functionality. We don't even open queues.
# Hence, we can save time skipping eventual consistency tests.


@tweak.broker.app_config.network_interfaces.heartbeats.client(2)
@tweak.broker.app_config.network_interfaces.tcp_interface.heartbeat_interval_ms(100)
def test_broker_client(
    cluster: Cluster,
    sc_domain_urls: tc.DomainUrls,  # pylint: disable=unused-argument
) -> None:
    """
    Test: connection loss between a broker and a client.
    - Start a broker and save the port it is listening.
    - Start a tproxy redirecting to the broker started on the previous step.
    - Start a client and connect it to the tproxy started on the previous step.
    - Kill the tproxy to break the connection between the client and the broker.

    Concerns:
    - The client is able to detect the connection loss.
    - The connection is restored after tproxy restart.
    """

    broker = next(cluster.proxy_cycle())

    # Start tproxy between broker's and client's ports
    tproxy_port, tproxy = cluster.start_tproxy(broker.config)

    # Start a client
    client: Client = broker.create_client(
        f"client@{broker.name}", port=tproxy_port, start=False
    )
    client.start_session(block=False)
    # There is a race between "session.start" log line and "CONNECTED" log line.
    # Due to this, we do not check for "session.start" and only check for "CONNECTED" event.
    assert client.capture(r"CONNECTED", 5)

    # Kill tproxy to break the connection between broker and client
    tproxy.kill()
    assert client.wait_connection_lost(5)

    # Start tproxy to restore the connection between broker and client
    cluster.start_tproxy(broker.config, port=tproxy_port)
    assert client.capture(r"RECONNECTED", 5)

    client.exit_gracefully()
    client.wait(5)


def _partition_lease_ids(node, cluster_name) -> "list[int]":
    """
    Return the primary leaseId this 'node' locally believes each partition has.

    Each node answers from its own view, which is the point: one whose Partition
    FSM never learned of a new lease keeps reporting the stale one.
    """

    admin = AdminClient()
    admin.connect(node.config.host, node.config.port)
    try:
        res = admin.send_admin(f"CLUSTERS CLUSTER {cluster_name} STATUS")
    finally:
        admin.stop()

    return [int(lease) for lease in re.findall(r"Primary LeaseId:\s*(\d+)", res)]


@start_cluster(False)
@tweak.cluster.elector.quorum(4)
@tweak.broker.app_config.network_interfaces.heartbeats.cluster_peer(3)
@tweak.broker.app_config.network_interfaces.tcp_interface.heartbeat_interval_ms(100)
def test_force_leader_primary_divergence(
    multi_node: Cluster,
    sc_domain_urls: tc.DomainUrls,  # pylint: disable=unused-argument
) -> None:
    """
    Test: connection loss between cluster nodes.
    - Run three instances of tproxy redirecting to endpoints of nodes "east2", "west1" and "west2".
      Capture new ports from tproxies' output:
        # tproxy -r localhost:old_port_1
        Listening on localhost:new_port_1...
        Where "localhost:old_port_1" is the endpoint of "east2" extracted from the config.
        And "localhost:new_port_1" is the new endpoint of tproxy.
    - Before starting the cluster modify cluster.json configs for all nodes:
      in cluster config of "east1", change for:
        - modify endpoint of east2 from "tcp://localhost:old_port_1" to "tcp://localhost:new_port_1"
        - modify endpoint of west1 from "tcp://localhost:old_port_2" to "tcp://localhost:new_port_2"
        - modify endpoint of west2 from "tcp://localhost:old_port_3" to "tcp://localhost:new_port_3"
      in cluster config of "east2", "west1", "west2":
        - modify elector.quorum from 0 to 4
    - Start "east1", "west1", and "west2"
    - Wait until "east1" becomes leader. It is the only possible leader because only it has
      quorum = 3
    - Start "east2"
    - Kill "tproxy_1" and "tproxy_2". It disconnects "east2" and "west1" from "east1"
    - Leader must become "west2"; it is the only node connected to all other nodes
    - "east1" is not a leader anymore but still primary for the partitions. It detects
      leader/primary divergence and exits gracefully. So we wait for "east1" to terminate, check
      the exit code, and restart.
    - After that cluster is expected to heal. Give it some time and check that primary for all
      partitions is the same as the leader - "west2".

    Concerns:
    - The connection loss leads to leader/primary divergence: Leader is "west2", but primary for all
     partitions is "east1". The node that loses leadership ("east1") terminates itself gracefully.
    """

    cluster = multi_node
    tproxies: Dict[str, Process] = {}

    # Modify cluster config for node "east1"
    broker = cluster.configurator.brokers["east1"]
    cluster_def = broker.clusters.my_clusters[0]
    cluster_def.elector.quorum = 0
    for node in cluster_def.nodes:
        if node.name != "east1":
            # For all the nodes except "east1" start a tproxy connected to the
            # node's endpoint. Change the endpoint in the config to the port of
            # just started tproxy. So "east1" will connect to tproxies instead of
            # the nodes.
            broker_config = cluster.config.nodes[node.name]
            tproxy_port, tproxy = cluster.start_tproxy(broker_config)
            tproxies[tproxy.name] = tproxy
            node.transport.tcp.endpoint = node.transport.tcp.endpoint.replace(
                str(broker_config.port), tproxy_port
            )
    cluster.configurator.deploy_clusters(broker, cluster.get_broker_local_site(broker))

    # Start east1, west1, and west2
    cluster.start_node("east1")
    cluster.start_node("west1")
    cluster.start_node("west2")

    # Wait until "east1" becomes leader. It is the only possible leader because only it has
    # quorum = 3
    old_leader = cluster.wait_leader()
    assert old_leader.name == "east1"

    # Start "east2" and kill two tproxies disconnecting "east2" and "west1" from "east1".
    east2 = cluster.start_node("east2")
    east2.wait_status(wait_leader=True, wait_ready=True)
    tproxies["tproxy_east2"].kill()
    tproxies["tproxy_west1"].kill()

    # Leader must become "west2" as it is the only node connected to all other nodes
    new_leader = cluster.wait_leader()
    assert new_leader.name == "west2"

    # Now "east1" detects the leader / primary divergence. It is not a leader anymore but
    # still primary. Hence, it is expected to shutdown itself gracefully.
    rc = old_leader.wait()
    assert rc == 0

    # Restart "east1"
    old_leader.start()
    old_leader.wait_until_started()

    # Request partitions summary with admin command. Check that primary for all partitions is
    # "west2" - the same as the leader
    admin = AdminClient()
    admin.connect(new_leader.config.host, new_leader.config.port)
    # Assigning primaries can take time, so we give the cluster 15 seconds for this
    test_logger.info("Try to detect new primaries...")
    attempts = 15
    while attempts > 0:
        res = admin.send_admin(
            f"CLUSTERS CLUSTER {cluster.config.name} STORAGE SUMMARY"
        )
        primaries: [str] = []
        try:
            for line in res.splitlines():
                mm = re.search(r"Primary Node.*\[(.+), \d+\]", line)
                if mm:
                    if mm.group(1) != new_leader.name:
                        raise RuntimeError(
                            f'Primary node "{mm.group(1)}" for partition does not match leader name "{new_leader.name}"'
                        )
                    primaries.append(mm.group(1))
            if (
                len(primaries)
                != cluster.config.definition.partition_config.num_partitions
            ):
                raise RuntimeError(
                    f'Primaries count "{len(primaries)}" does not match partitions number from config "{cluster.config.definition.partition_config.num_partitions}"'
                )
            test_logger.info("Success!")
            break
        except RuntimeError as e:
            attempts -= 1
            if attempts == 0:
                test_logger.info(res)
                raise e
            test_logger.info("Wait primaries for 1 more second...")
            sleep(1)


@start_cluster(False)
@tweak.cluster.elector.quorum(4)
@tweak.broker.app_config.network_interfaces.heartbeats.cluster_peer(3)
@tweak.broker.app_config.network_interfaces.tcp_interface.heartbeat_interval_ms(100)
def test_unobserved_leader_primary_divergence(
    fsm_multi_cluster: Cluster,
    sc_domain_urls: tc.DomainUrls,  # pylint: disable=unused-argument
) -> None:
    """
    Test: a primary that misses an election entirely and is then re-elected.

    Unobserved counterpart of 'test_force_leader_primary_divergence': the old
    primary is cut off from its replacement, so it never sees the takeover and
    is simply re-elected.

    1. "east1" leads, so is primary for all partitions (E_LEADER_IS_MASTER_ALL).
    2. Raise "west1" and "east2" to an unreachable quorum so neither can ever
       win, then cut "east1" from "west2" and "east2"; it drops below quorum
       and cedes.  "west1" stays up but cannot become a rival leader.
    3. "west2" takes over at a new lease.
    4. Suspend "west2" and heal.
    5. "east1" wins the next election.
    6. Resume "west2"; it aborts with UNSUPPORTED_SCENARIO.  "east1" must not:
       if it does, it saw the takeover and this is the sibling test.

    Concerns:
    - After step 5 every node agrees on the primary leaseId.
    """

    cluster = fsm_multi_cluster
    tproxies: Dict[str, Process] = {}
    tproxy_ports: Dict[str, str] = {}

    # "east1" gets the default majority quorum (3), everyone else 4.
    cut_peers = ("west2", "east2")

    broker = cluster.configurator.brokers["east1"]
    cluster_def = broker.clusters.my_clusters[0]
    cluster_def.elector.quorum = 0
    for node in cluster_def.nodes:
        if node.name in cut_peers:
            broker_config = cluster.config.nodes[node.name]
            tproxy_port, tproxy = cluster.start_tproxy(broker_config)
            tproxies[node.name] = tproxy
            tproxy_ports[node.name] = tproxy_port
            node.transport.tcp.endpoint = node.transport.tcp.endpoint.replace(
                str(broker_config.port), tproxy_port
            )
    cluster.configurator.deploy_clusters(broker, cluster.get_broker_local_site(broker))

    # 1. Start three nodes so nobody can reach quorum 4, letting "east1" win.
    east1 = cluster.start_node("east1")
    west1 = cluster.start_node("west1")
    west2 = cluster.start_node("west2")
    east1.wait_status(wait_leader=True, wait_ready=True)
    assert east1.last_known_leader == east1
    east2 = cluster.start_node("east2")
    east2.wait_status(wait_leader=True, wait_ready=True)

    initial_leases = _partition_lease_ids(east1, cluster.config.name)
    test_logger.info(f"east1 is leader and primary; leases {initial_leases}")
    assert initial_leases, "no partitions reported"

    # 2. "west1" and "east2" must never win: 5 is unreachable in a 4-node
    #    cluster, regardless of who they can still talk to.  Then steer the
    #    election to "west2" and cut "east1" below quorum.
    west1.set_quorum(5, succeed=True)
    east2.set_quorum(5, succeed=True)
    west2.set_quorum(3, succeed=True)
    for name in cut_peers:
        tproxies[name].kill()

    # 3. "west2" takes over and bumps the lease.
    for node in (west2, west1, east2):
        node.wait_status(wait_leader=True, wait_ready=False)
        assert node.last_known_leader == west2

    assert east1.outputs_regex("LEADER lost quorum", timeout=30)
    assert east1.is_alive(), "east1 must not have observed the takeover"

    new_leases = _partition_lease_ids(west2, cluster.config.name)
    test_logger.info(f"west2 is leader and primary; leases {new_leases}")
    assert new_leases > initial_leases, "the lease should have advanced"

    # 4. Freeze "west2", then heal every link.
    west2.suspend()
    for name in cut_peers:
        cluster.start_tproxy(cluster.config.nodes[name], port=tproxy_ports[name])

    # 5. Steer the election back to "east1".
    east1.set_quorum(1, succeed=True)
    east1.wait_status(wait_leader=True, wait_ready=False)
    assert east1.last_known_leader == east1
    assert east1.is_alive()
    assert not east1.outputs_regex("UNSUPPORTED_SCENARIO", timeout=5)

    # 6. Wake "west2"; it finds "east1" leading and aborts.
    west2.check_exit_code = False
    west2.resume()
    assert west2.outputs_regex("UNSUPPORTED_SCENARIO", timeout=60)
    west2.wait()

    final_leases = _partition_lease_ids(east1, cluster.config.name)
    peer_leases = _partition_lease_ids(west1, cluster.config.name)
    test_logger.info(f"east1 reports {final_leases}, west1 reports {peer_leases}")

    assert peer_leases > initial_leases, "the cluster's lease should have advanced"
    assert final_leases == peer_leases, (
        f"east1 reports stale primary leaseId {final_leases} while the rest of "
        f"the cluster is at {peer_leases}"
    )
