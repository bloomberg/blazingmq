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

import blazingmq.dev.it.testconstants as tc
from blazingmq.dev.it.fixtures import (  # pylint: disable=unused-import
    Cluster,
    cluster,
    order,
    multi_node,
    start_cluster,
    tweak,
)
from blazingmq.dev.it.process.client import Client

pytestmark = order(4)

timeout = 60

# -------------------------------------------------------------------------------


def check_num_assigned_queues(expected, *brokers, du: tc.DomainUrls):
    for broker in brokers:
        broker.queue_helper()
        m = broker.capture(
            f"{du.domain_priority}.*numAssignedQueues=(\\d+)",
            timeout,
        )
        assert m is not None, f"broker={broker.name}"
        assert int(m[1]) == expected, f"broker={broker.name}"


@tweak.domain.max_queues(5)
class TestMaxQueues:
    def test_max_queue(self, cluster: Cluster, domain_urls: tc.DomainUrls):
        du = domain_urls
        [q1, q2, q3, q4, q5, q6, q7, q8, q9] = [
            f"bmq://{du.domain_priority}/q{i}" for i in range(9)
        ]

        # ==================
        # beginning of setup
        leader = cluster.last_known_leader
        proxies = cluster.proxy_cycle()
        leader_proxy = next(proxies)
        replica_proxy = next(proxies)

        leader_client = leader_proxy.create_client("LC")
        leader_client.open(q1, flags=["read"], succeed=True)
        leader_client.open(q2, flags=["read"], succeed=True)
        leader_client.open(q3, flags=["read"], succeed=True)
        # end of setup
        # ============

        # hit high watermark
        leader_client.open(q4, flags=["read"], succeed=True)
        assert leader.alarms("DOMAIN_QUEUE_LIMIT_HIGH_WATERMARK.*current: 4, limit: 5")
        check_num_assigned_queues(4, leader, du=du)

        # hit maximum
        assert leader_client.open(q5, flags=["read"], block=True) == Client.e_SUCCESS
        check_num_assigned_queues(5, leader, du=du)

        # cannot open new queues anymore now
        assert leader_client.open(q6, flags=["read"], block=True) == Client.e_REFUSED
        assert leader.panics("DOMAIN_QUEUE_LIMIT_FULL.*limit: 5")
        check_num_assigned_queues(5, leader, du=du)

        # but opening existing queues is OK
        replica_client = replica_proxy.create_client("SC")
        assert replica_client.open(q1, flags=["read"], block=True) == Client.e_SUCCESS

        active_replica = (
            leader
            if cluster.is_single_node
            else cluster.process(replica_proxy.get_active_node())
        )
        check_num_assigned_queues(5, leader, active_replica, du=du)

        # check that opening an existing queue doesn't mess up things
        # still cannot open new queues anymore
        assert leader_client.open(q6, flags=["read"], block=True) == Client.e_REFUSED
        check_num_assigned_queues(5, leader, active_replica, du=du)

        # ---------------------------------------------------------------------
        # check that removing queues make space for new queues

        leader_client.close(q4, succeed=True)
        leader_client.close(q5, succeed=True)

        # they're not GCed yet, thus opening a new queue still fails
        assert replica_client.open(q6, flags=["read"], block=True) == Client.e_REFUSED

        # GC; now we can open a new queue
        leader.force_gc_queues(succeed=True)
        check_num_assigned_queues(3, leader, active_replica, du=du)

        assert replica_client.open(q6, flags=["read"], block=True) == Client.e_SUCCESS
        assert leader.alarms("DOMAIN_QUEUE_LIMIT_HIGH_WATERMARK.*current: 4, limit: 5")
        check_num_assigned_queues(4, leader, active_replica, du=du)

        assert leader_client.open(q7, flags=["read"], block=True) == Client.e_SUCCESS

        assert replica_client.open(q8, flags=["read"], block=True) == Client.e_REFUSED
        assert leader.panics("DOMAIN_QUEUE_LIMIT_FULL.*limit: 5")
        check_num_assigned_queues(5, leader, active_replica, du=du)

        # ---------------------------------------------------------------------
        # force a change of leadership

        if not cluster.is_single_node:
            for node in cluster.nodes():
                # avoid picking up old leader
                node.drain()
            leader.stop()
            leader = cluster.wait_leader()

            # Wait for new leader to become active primary by opening a queue
            # a *different* domain from a new client for synchronization.
            dummy_producer = replica_proxy.create_client("dummy_producer")
            dummy_producer.open(
                f"bmq://{tc.DOMAIN_BROADCAST}/qqq", flags=["write", "ack"], succeed=True
            )

            proxies = cluster.proxy_cycle()
            leader_proxy = next(proxies)
            replica_proxy = next(proxies)
            replica_client = replica_proxy.create_client("SC2")
            assert (
                replica_client.open(q9, flags=["read"], block=True) == Client.e_REFUSED
            )

    # =========================================================================
    # open queues before there is a leader (restore, in flight contexts)

    @start_cluster(False)
    def test_max_queue_restore(self, multi_node: Cluster, domain_urls: tc.DomainUrls):
        du = domain_urls
        [q1, q2, q3, q4, q5, q6, q7, q8, q9] = [
            f"bmq://{du.domain_priority}/q{i}" for i in range(9)
        ]

        cluster = multi_node

        cluster.start_node("west1")
        cluster.start_node("east1")

        # proxies
        op = cluster.start_proxy("westp")
        rp = cluster.start_proxy("eastp")

        oc = op.create_client("oc")
        rc = rp.create_client("rc")

        oc.open(q1, flags=["read"])
        rc.open(q2, flags=["read"])
        oc.open(q3, flags=["read"])
        rc.open(q4, flags=["read"])
        oc.open(q5, flags=["read"])
        rc.open(q6, flags=["read"])
        oc.open(q7, flags=["read"])

        # add missing node to reach quorum
        o2 = cluster.start_node("west2")
        o2.wait_status(wait_leader=True, wait_ready=True)
        leader = o2.last_known_leader

        oa = cluster.process(op.get_active_node())
        ra = cluster.process(rp.get_active_node())

        results = []

        for client in [oc, oc, oc, oc, rc, rc, rc]:
            m = client.capture(r"<-- session.openQueue.*\((-?\d+)\)", timeout)
            if m:
                results.append(int(m[1]))

        assert sorted(results) == [-6, -6, 0, 0, 0, 0, 0]

        assert leader.alarms("DOMAIN_QUEUE_LIMIT_HIGH_WATERMARK.*current: 4, limit: 5")

        assert leader.panics("DOMAIN_QUEUE_LIMIT_FULL.*limit: 5")

        check_num_assigned_queues(5, leader, oa, ra, du=du)
