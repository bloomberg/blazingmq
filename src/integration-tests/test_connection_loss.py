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

import json
import re
from time import sleep
from typing import Dict

import blazingmq.dev.it.testconstants as tc
from blazingmq.dev.it.fixtures import (  # pylint: disable=unused-import
    Cluster,
    order,
    cluster,
    test_logger,
    multi_node,
    tweak,
    start_cluster,
)
from blazingmq.dev.it.process.admin import AdminClient
from blazingmq.dev.it.process.broker import Broker
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
    client: Client = broker.create_client(f"client@{broker.name}", port=tproxy_port)
    client.start_session()
    assert client.capture(r"CONNECTED", 5)

    # Kill tproxy to break the connection between broker and client
    tproxy.kill()
    assert client.wait_connection_lost(5)

    # Start tproxy to restore the connection between broker and client
    cluster.start_tproxy(broker.config, port=tproxy_port)
    assert client.capture(r"RECONNECTED", 5)

    client.exit_gracefully()
    client.wait(5)


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
    with open(
        cluster.work_dir.joinpath(
            cluster.config.nodes["east1"].config_dir, "clusters.json"
        ),
        "r+",
        encoding="utf-8",
    ) as f:
        data = json.load(f)
        data["myClusters"][0]["elector"]["quorum"] = 0
        for node in data["myClusters"][0]["nodes"]:
            if node["name"] != "east1":
                # For all the nodes except "east1" start a tproxy connected to the
                # node's endpoint. Change the endpoint in the config to the port of
                # just started tproxy. So "east1" will connect to tproxies instead of
                # the nodes.
                broker_config = cluster.config.nodes[node["name"]]
                tproxy_port, tproxy = cluster.start_tproxy(broker_config)
                tproxies[tproxy.name] = tproxy
                node["transport"]["tcp"]["endpoint"] = node["transport"]["tcp"][
                    "endpoint"
                ].replace(str(broker_config.port), tproxy_port)
        f.seek(0)
        json.dump(data, f, indent=4)
        f.truncate()

    # Start east1, west1, and west2
    cluster.start_node("east1")
    cluster.start_node("west1")
    cluster.start_node("west2")

    # Wait until "east1" becomes leader. It is the only possible leader because only it has
    # quorum = 3
    old_leader = cluster.wait_leader()
    assert old_leader.name == "east1"

    # Start "east2" and kill two tproxies disconnecting "east2" and "west1" from "east1".
    cluster.start_node("east2")
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
