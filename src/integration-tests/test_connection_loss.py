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
from typing import Dict

from blazingmq.dev.it.fixtures import (  # pylint: disable=unused-import
    Cluster,
    order,
    cluster,
    multi_node,
    tweak,
    start_cluster,
)
from blazingmq.dev.it.process.admin import AdminClient
from blazingmq.dev.it.process.client import Client
from blazingmq.dev.it.process.proc import Process

pytestmark = order(1)


def test_broker_client(
    cluster: Cluster,  # pylint: disable=redefined-outer-name
) -> None:
    """
    Test: connection loss between a broker and a client.
    - Start a broker and save the port it is listening.
    - Start a tproxy redirecting to the broker started on the previous step.
    - Start a client and connect it to the tproxy started on the previous step.
    - Kill the tproxy to brake the connection between the client and the broker.

    Concerns:
    - The client is able to detect the connection loss.
    - The connection is restored after tproxy restart.
    """

    proxies = cluster.proxy_cycle()
    proxy = next(proxies)
    broker_host = proxy.config.host
    broker_port = proxy.config.port

    # Start tproxy between broker's and client's ports
    tproxy = Process("tproxy", ["tproxy", "-r", f"{broker_host}:{broker_port}"])
    try:
        tproxy.start()
        tproxy_port = tproxy.capture(r"Listening on (.+):(\d+)", timeout=2).group(2)

        # Start a client
        client: Client = proxy.create_client(f"client@{proxy.name}", port=tproxy_port)
        client.start_session()
        assert client.capture(r"CONNECTED", 5)

        # Kill tproxy to brake the connection between broker and client
        tproxy.kill()
        assert client.wait_connection_lost(5)

        # Start tproxy to restore the connection between broker and client
        tproxy = Process(
            "tproxy",
            ["tproxy", "-r", f"{broker_host}:{broker_port}", "-p", f"{tproxy_port}"],
        )
        tproxy.start()
        assert client.capture(r"RECONNECTED", 5)

        client.exit_gracefully()
        client.wait(5)
    finally:
        tproxy.kill()


@start_cluster(False)
@tweak.cluster.cluster_attributes.is_cslmode_enabled(False)
@tweak.cluster.cluster_attributes.is_fsmworkflow(False)
def test_force_leader_primary_divergence(  # pylint: disable=too-many-locals
    multi_node: Cluster,  # pylint: disable=redefined-outer-name
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
    - Check that primary for all partitions is "east1"

    Concerns:
    - The connection loss leads to leader/primary divergence: Leader is "west2", but primary for all
     partitions is "east1"
    """

    cluster = multi_node  # pylint: disable=redefined-outer-name
    tproxies: Dict[str, Process] = {}

    try:
        for name in cluster.config.nodes:
            with open(
                cluster.work_dir.joinpath(
                    cluster.config.nodes[name].config_dir, "clusters.json"
                ),
                "r+",
                encoding="utf-8",
            ) as f:
                data = json.load(f)
                if name == "east1":
                    # Modify cluster config for node "east1"
                    for node in data["myClusters"][0]["nodes"]:
                        if node["name"] != name:
                            # For all the nodes except "east1" start a tproxy connected to the
                            # node's endpoint. Change the endpoint in the config to the port of
                            # just started tproxy. So "east1" will connect to tproxies instead of
                            # the nodes.
                            broker_config = cluster.config.nodes[node["name"]]
                            tproxy = Process(
                                "tproxy",
                                [
                                    "tproxy",
                                    "-r",
                                    f"{broker_config.host}:{broker_config.port}",
                                ],
                            )
                            tproxy.start()
                            tproxy_port = tproxy.capture(
                                r"Listening on (.+):(\d+)", timeout=2
                            ).group(2)
                            tproxies[node["name"]] = tproxy
                            node["transport"]["tcp"]["endpoint"] = node["transport"][
                                "tcp"
                            ]["endpoint"].replace(str(broker_config.port), tproxy_port)
                else:
                    data["myClusters"][0]["elector"]["quorum"] = 4
                f.seek(0)
                json.dump(data, f, indent=4)
                f.truncate()

        # Start east1, west1, and west2
        cluster.start_node("east1")
        cluster.start_node("west1")
        cluster.start_node("west2")

        # Wait until "east1" becomes leader. It is the only possible leader because only it has
        # quorum = 3
        leader = cluster.wait_leader()
        assert leader.name == "east1"

        # Start "east2" and kill two tproxies disconnecting "east2" and "west1" from "east1".
        cluster.start_node("east2")
        tproxies["east2"].kill()
        tproxies["west1"].kill()

        # Leader must become "west2" as it is the only node connected to all other nodes
        leader = cluster.wait_leader()
        assert leader.name == "west2"

        # Request partitions summary with admin command. Check that primary for all partitions is
        # "east1"
        admin = AdminClient()
        admin.connect(leader.config.host, leader.config.port)
        res = admin.send_admin(
            f"CLUSTERS CLUSTER {cluster.config.name} STORAGE SUMMARY"
        )
        primaries: [str] = []
        for line in res.splitlines():
            m = re.search(r"Primary Node.*\[(.+), \d+\]", line)
            if m:
                assert m.group(1) == "east1"
                primaries.append(m.group(1))
        assert len(primaries) == 4
    finally:
        # Kill the tproxies
        for tp in tproxies.values():
            tp.kill()
