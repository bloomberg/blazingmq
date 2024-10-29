# Copyright 2024 Bloomberg Finance L.P.
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
This suite of test cases verifies that only the node that the admin
connects to send the response back to the admin, and the corresponding
response is logged.
"""

import blazingmq.dev.it.testconstants as tc
from blazingmq.dev.it.fixtures import (
    multi_node,
    Cluster,
)
from blazingmq.dev.it.process.admin import AdminClient
from blazingmq.dev.it.process.client import Client

TIMEOUT = 1


def test_adminsession_res_log_stat(multi_node: Cluster):
    """
    Test: STAT SHOW command: response message only generate on the node itself
    """

    admin = AdminClient()

    # find the first two nodes which are not the known leader
    leader = multi_node.nodes()[0].last_known_leader
    replicas = multi_node.nodes(exclude=leader)
    member1 = replicas[0]
    member2 = replicas[1]

    # connect and send request to primary
    admin.connect(leader.config.host, int(leader.config.port))
    res = admin.send_admin(f"STAT SHOW")

    assert ":::::::::: :::::::::: DOMAINQUEUES >>" in res

    assert leader.capture("Send response message", TIMEOUT)
    assert not member1.capture("Send response message", TIMEOUT)
    assert not member2.capture("Send response message", TIMEOUT)

    admin.stop()

    # connect and send request to member1
    admin.connect(member1.config.host, int(member1.config.port))
    res = admin.send_admin(f"STAT SHOW")

    assert ":::::::::: :::::::::: DOMAINQUEUES >>" in res

    assert member1.capture("Send response message", TIMEOUT)
    assert not leader.capture("Send response message", TIMEOUT)
    assert not member2.capture("Send response message", TIMEOUT)

    admin.stop()


def test_adminsession_res_log_purge(multi_node: Cluster):
    """
    Test: PURGE command: response message only generate on the node itself
    """

    admin = AdminClient()

    # find the first two nodes which are not the known leader
    leader = multi_node.nodes()[0].last_known_leader
    replicas = multi_node.nodes(exclude=leader)
    member1 = replicas[0]
    member2 = replicas[1]

    # open a domain
    proxies = multi_node.proxy_cycle()
    proxy = next(proxies)
    producer: Client = proxy.create_client("producer")
    producer.open(tc.URI_FANOUT, flags=["write,ack"], succeed=True)

    # connect and send request to primary
    admin.connect(leader.config.host, int(leader.config.port))
    res = admin.send_admin(f"DOMAINS DOMAIN {tc.DOMAIN_FANOUT} PURGE")

    assert "Purged" in res

    assert leader.capture("Send response message", TIMEOUT)
    assert not member1.capture("Send response message", TIMEOUT)
    assert not member2.capture("Send response message", TIMEOUT)

    admin.stop()

    # connect and send request to member1
    admin.connect(member1.config.host, int(member1.config.port))
    res = admin.send_admin(f"DOMAINS DOMAIN {tc.DOMAIN_FANOUT} PURGE")

    assert "Purged" in res

    assert member1.capture("Send response message", TIMEOUT)
    assert not leader.capture("Send response message", TIMEOUT)
    assert not member2.capture("Send response message", TIMEOUT)

    admin.stop()


def test_adminsession_res_log_reconfigure(multi_node: Cluster):
    """
    Test: RECONFIGURE command: response message only generate on the node itself
    """

    admin = AdminClient()

    # find the first two nodes which are not the known leader
    leader = multi_node.nodes()[0].last_known_leader
    replicas = multi_node.nodes(exclude=leader)
    member1 = replicas[0]
    member2 = replicas[1]

    num_nodes = len(multi_node.nodes())

    # connect and send request to primary
    admin.connect(leader.config.host, int(leader.config.port))
    res = admin.send_admin(f"DOMAINS RECONFIGURE {tc.DOMAIN_FANOUT}")

    success_count = res.split().count("SUCCESS")
    assert success_count == num_nodes

    assert leader.capture("Send response message", TIMEOUT)
    assert not member1.capture("Send response message", TIMEOUT)
    assert not member2.capture("Send response message", TIMEOUT)

    admin.stop()

    # connect and send request to member1
    admin.connect(member1.config.host, int(member1.config.port))
    res = admin.send_admin(f"DOMAINS RECONFIGURE {tc.DOMAIN_FANOUT}")

    success_count = res.split().count("SUCCESS")
    assert success_count == num_nodes

    assert member1.capture("Send response message", TIMEOUT)
    assert not leader.capture("Send response message", TIMEOUT)
    assert not member2.capture("Send response message", TIMEOUT)

    admin.stop()
