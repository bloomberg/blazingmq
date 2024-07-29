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

import contextlib

import blazingmq.dev.it.testconstants as tc
from blazingmq.dev.it.fixtures import (  # pylint: disable=unused-import
    Cluster,
    single_node,
    order,
    multi_node,
    tweak,
)
from blazingmq.dev.it.util import wait_until
from blazingmq.schemas import mqbconf

pytestmark = order(5)


class Suspender:
    def __init__(self, node):
        self.node = node

    def __enter__(self):
        self.node.suspend()

    def __exit__(self, exc_type, exc_value, traceback):
        self.node.resume()


class Killer:
    def __init__(self, node):
        self.node = node

    def __enter__(self):
        self.node.check_exit_code = False
        self.node.kill()

    def __exit__(self, exc_type, exc_value, traceback):
        self.node.start()
        self.node.check_exit_code = True


class TestStrongConsistency:
    """
    This test verifies strong consistency.
    The consumer connects to replica Proxy and opens strong and weak consistency
    queues.
    The producer connects to replica Proxy and post messages to both queues while
    non-active replica node are paused (strong consistency queue first).
    Weak consistency queue should get deliver message(s) to the consumer while
    strong consistency queue should not.
    """

    def setup_cluster(self, cluster):
        proxies = cluster.proxy_cycle()
        # pick proxy in datacenter opposite to the primary's
        next(proxies)
        self.replica_proxy = next(proxies)
        self.producer = self.replica_proxy.create_client("producer")
        self.producer.open(tc.URI_FANOUT, flags=["write,ack"], succeed=True)
        self.producer.open(tc.URI_FANOUT_SC, flags=["write,ack"], succeed=True)

        # start consumer
        self.consumer = self.replica_proxy.create_client("consumer")

        for uri in [tc.URI_FANOUT_FOO, tc.URI_FANOUT_BAR, tc.URI_FANOUT_BAZ]:
            self.consumer.open(uri, flags=["read"], succeed=True)

        for uri in [tc.URI_FANOUT_SC_FOO, tc.URI_FANOUT_SC_BAR, tc.URI_FANOUT_SC_BAZ]:
            self.consumer.open(uri, flags=["read"], succeed=True)

    def _break_post_unbreak(self, multi_node, breaker, has_timeout):
        cluster = multi_node
        leader = cluster.last_known_leader
        active_node = cluster.process(self.replica_proxy.get_active_node())

        # pylint: disable=cell-var-from-loop; passing lambda to 'wait_until' is safe
        with contextlib.ExitStack() as stack:
            # break non-active replica nodes
            for node in cluster.nodes(exclude=[leader, active_node]):
                stack.enter_context(breaker(node))

            # post SC
            self.producer.post(tc.URI_FANOUT_SC, payload=["msg"], succeed=True)
            # post WC
            self.producer.post(tc.URI_FANOUT, payload=["msg"], succeed=True)

            # receive WC messages
            self.consumer.wait_push_event()
            for uri in [tc.URI_FANOUT_FOO, tc.URI_FANOUT_BAR, tc.URI_FANOUT_BAZ]:
                assert wait_until(
                    lambda: len(self.consumer.list(uri, block=True)) == 1, 2
                )
            # the WC queue was last sending data and the SC one was first which
            # means it is sufficent to wait on WC before checking SC

            if has_timeout:
                # expect NACK
                assert self.producer.capture(
                    r"ACK #0: \[ type = ACK status = UNKNOWN", 2
                )
            else:
                # make sure there are neither SC ACK(s) nor message(s)
                assert not self.producer.outputs_regex(
                    f"MESSAGE.*ACK.*{tc.URI_FANOUT_SC}", 1
                )

                for uri in [
                    tc.URI_FANOUT_SC_FOO,
                    tc.URI_FANOUT_SC_BAR,
                    tc.URI_FANOUT_SC_BAZ,
                ]:
                    assert len(self.consumer.list(uri, block=True)) == 0

        if not has_timeout:
            self.consumer.wait_push_event(timeout=120)
            # make sure there are SC ACK(s) and message(s)
            assert self.producer.outputs_regex(f"MESSAGE.*ACK.*{tc.URI_FANOUT_SC}", 25)
            for uri in [
                tc.URI_FANOUT_SC_FOO,
                tc.URI_FANOUT_SC_BAR,
                tc.URI_FANOUT_SC_BAZ,
            ]:
                assert wait_until(
                    lambda: len(self.consumer.list(uri, block=True)) == 1, 2
                )

    def test_suspend_post_resume(self, multi_node: Cluster):
        self._break_post_unbreak(multi_node, Suspender, False)

    def test_kill_post_start(self, multi_node: Cluster):
        self._break_post_unbreak(multi_node, Killer, False)

    def test_strong_consistency_local(
        self, single_node  # pylint: disable=unused-argument
    ):
        # post SC
        self.producer.post(
            tc.URI_FANOUT_SC, payload=["msg"], succeed=True, wait_ack=True
        )

        # receive SC messages
        self.consumer.wait_push_event()
        for uri in [tc.URI_FANOUT_SC_FOO, tc.URI_FANOUT_SC_BAR, tc.URI_FANOUT_SC_BAZ]:
            # pylint: disable=cell-var-from-loop; passing lambda to 'wait_until' is safe
            assert wait_until(lambda: len(self.consumer.list(uri, block=True)) == 1, 2)

    @tweak.domain.deduplication_time_ms(1000)
    def test_timeout_receipt(self, multi_node: Cluster):
        self._break_post_unbreak(multi_node, Suspender, True)

    def test_dynamic_replication_factor(self, multi_node: Cluster):
        leader = multi_node.last_known_leader

        # Exercise the LIST_TUNABLES command to ensure it executes safely.
        leader.command("CLUSTERS CLUSTER itCluster STORAGE REPLICATION LIST_TUNABLES")

        with contextlib.ExitStack() as stack:
            # Suspend all nodes that are not the leader or active.
            active_node = multi_node.process(self.replica_proxy.get_active_node())
            for node in multi_node.nodes(exclude=[leader, active_node]):
                stack.enter_context(Suspender(node))

            # Default replication-factor is 3. Since only two nodes are active, no
            # PUSH event should be issued.
            self.producer.post(
                tc.URI_FANOUT_SC, payload=["msg"], succeed=True, wait_ack=True
            )
            assert not self.consumer.wait_push_event(timeout=5, quiet=False)

            # Setting the replication-factor to 2 should now mark the unreceipted
            # message as having met replication quorum.
            leader.set_replication_factor(2)
            assert self.consumer.wait_push_event()

            # Post another message; make sure that it's receipted without issue.
            self.producer.post(
                tc.URI_FANOUT_SC, payload=["msg"], succeed=True, wait_ack=True
            )
            assert self.consumer.wait_push_event()

            # Each fanout queue should now have two messages.
            for uri in [
                tc.URI_FANOUT_SC_FOO,
                tc.URI_FANOUT_SC_BAR,
                tc.URI_FANOUT_SC_BAZ,
            ]:
                # pylint: disable=cell-var-from-loop; passing lambda to 'wait_until' is safe
                assert wait_until(
                    lambda: len(self.consumer.list(uri, block=True)) == 2, 2
                )

    def test_change_consistency(self, multi_node: Cluster):
        leader = multi_node.last_known_leader
        assert leader

        # Open priority queue.
        self.consumer.open(tc.URI_PRIORITY, flags=["read"], succeed=True)
        self.producer.open(tc.URI_PRIORITY, flags=["write,ack"], succeed=True)

        # Build list of nodes to be suspended.
        suspended_nodes = multi_node.nodes(
            exclude=[
                leader,
                multi_node.process(self.replica_proxy.get_active_node()),
            ]
        )
        with contextlib.ExitStack() as stack:
            with Suspender(suspended_nodes[0]):
                # Suspend all nodes set to be suspended.
                for node in suspended_nodes[1:]:
                    stack.enter_context(Suspender(node))

                # Verify that a message can be sent to non-SC queue.
                self.producer.post(
                    tc.URI_PRIORITY,
                    payload=["msg-without-receipt"],
                    succeed=True,
                    wait_ack=True,
                )
                assert self.consumer.wait_push_event()

                multi_node.config.domains[
                    tc.DOMAIN_PRIORITY
                ].definition.parameters.consistency = mqbconf.Consistency(  # type: ignore
                    strong=mqbconf.QueueConsistencyStrong()
                )
                multi_node.reconfigure_domain(tc.DOMAIN_PRIORITY, write_only=True)

                # Reconfigure domain to be strongly consistent.
                multi_node.nodes(exclude=suspended_nodes)[0].reconfigure_domain(
                    tc.DOMAIN_PRIORITY, succeed=True
                )

                # Require messages to be written to three machines before push.
                leader.set_replication_factor(3, succeed=True)

                # Since only two nodes are active, no PUSH event is issued.
                self.producer.post(
                    tc.URI_PRIORITY,
                    payload=["msg-with-receipt"],
                    succeed=True,
                    wait_ack=False,
                )
                # Cannot wait for too long because resumed nodes may disconnect
                # because of mised hearbeats and fail to send the receipt.
                assert not self.consumer.wait_push_event(timeout=3)

            # Verify that message is now delivered.
            assert self.consumer.wait_push_event()
