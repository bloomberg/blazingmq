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
Integration test that tests some basic things while restarting *entire* cluster
in the middle of things.  Note that this test does not test the HA
functionality (i.e., no PUTs/CONFIRMs etc are retransmitted).
"""

import functools
import re
import time

import pytest

import blazingmq.dev.it.testconstants as tc
from blazingmq.dev.it.fixtures import (
    Cluster,
    Mode,
    cluster,
    cluster_fixture,
    multi_node_cluster_config,
    single_node_cluster_config,
    order,
    tweak,
)  # pylint: disable=unused-import
from blazingmq.dev.it.process.client import Client
from blazingmq.dev.it.util import attempt, wait_until

pytestmark = order(2)


def configure_cluster(cluster: Cluster, is_fsm: bool):
    """
    Configure the `cluster` to FSM or non-FSM mode, based on the `is_fsm` flag.
    """
    for broker in cluster.configurator.brokers.values():
        my_clusters = broker.clusters.my_clusters
        if my_clusters:
            cluster_attr = my_clusters[0].cluster_attributes
            cluster_attr.is_cslmode_enabled = is_fsm
            cluster_attr.is_fsmworkflow = is_fsm
            cluster_attr.does_fsmwrite_qlist = True
    cluster.deploy_domains()


def ensureMessageAtStorageLayer(
    cluster: Cluster, partitionId: int, queueUri: str, numMessages: int
):
    """
    Assert that in the `partitionId` of the `cluster`, there are exactly
    `numMessages` messages in the storage of the `queueUri`.
    """

    # Before restarting the cluster, ensure that all nodes in the cluster
    # have received the message at the storage layer.  This is necessary
    # in the absence of stronger consistency in storage replication in
    # BMQ.  Presence of message in the storage at each node is checked by
    # sending 'STORAGE SUMMARY' command and grepping its output.
    for node in cluster.nodes():
        node.command(f"CLUSTERS CLUSTER {node.cluster_name} STORAGE SUMMARY")

    time.sleep(2)
    for node in cluster.nodes():
        assert node.outputs_regex(
            r"\w{10}\s+%s\s+%s\s+\d+\s+B\s+" % (partitionId, numMessages)
            + re.escape(queueUri),
            timeout=20,
        )
        # Above regex is to match line:
        # C1E2A44527    0      1      68  B      bmq://bmq.test.mmap.priority.~tst/qqq
        # where columns are: QueueKey, PartitionId, NumMsgs, NumBytes,
        # QueueUri respectively.


def test_basic(cluster: Cluster, domain_urls: tc.DomainUrls):
    uri_priority = domain_urls.uri_priority

    # Start a producer and post a message.
    proxies = cluster.proxy_cycle()
    producer = next(proxies).create_client("producer")
    producer.open(uri_priority, flags=["write", "ack"], succeed=True)
    producer.post(uri_priority, payload=["msg1"], wait_ack=True, succeed=True)

    ensureMessageAtStorageLayer(cluster, 0, uri_priority, 1)

    cluster.restart_nodes()
    # For a standard cluster, states have already been restored as part of
    # leader re-election.
    if cluster.is_single_node:
        producer.wait_state_restored()

    producer.post(uri_priority, payload=["msg2"], wait_ack=True, succeed=True)

    consumer = next(proxies).create_client("consumer")
    consumer.open(uri_priority, flags=["read"], succeed=True)
    consumer.wait_push_event()
    assert wait_until(lambda: len(consumer.list(uri_priority, block=True)) == 2, 2)


def test_wrong_domain(cluster: Cluster, domain_urls: tc.DomainUrls):
    proxies = cluster.proxy_cycle()
    producer = next(proxies).create_client("producer")

    assert Client.e_SUCCESS is producer.open(
        domain_urls.uri_fanout, flags=["write"], block=True
    )
    assert Client.e_SUCCESS is not producer.open(
        "bmq://domain.does.not.exist/qqq",
        flags=["write"],
        block=True,
        no_except=True,
    )


def test_migrate_domain_to_another_cluster(
    cluster: Cluster, domain_urls: tc.DomainUrls
):
    uri_fanout = domain_urls.uri_fanout
    proxies = cluster.proxy_cycle()
    producer = next(proxies).create_client("producer")

    assert Client.e_SUCCESS == producer.open(uri_fanout, flags=["write"], block=True)

    # Before changing domain config of each node in the cluster, ensure that
    # all nodes in the cluster have observed the previous open-queue event.
    # If we don't do this, replicas may receive a queue creation event at
    # the storage layer, pick up the domain, and, as a result, fail to
    # successfully apply the queue creation event.
    @attempt(3, 5)
    def wait_replication():
        for node in cluster.nodes():
            node.command(f"CLUSTERS CLUSTER {node.cluster_name} STORAGE SUMMARY")

        for node in cluster.nodes():
            assert node.outputs_regex(
                r"\w{10}\s+0\s+0\s+\d+\s+B\s+" + re.escape(uri_fanout),
                timeout=1,
            )
            # Above regex is to match line:
            # C1E2A44527    0      0      68  B      bmq://bmq.test.mmap.fanout/qqq
            # where columns are: QueueKey, PartitionId, NumMsgs, NumBytes,
            # QueueUri respectively. Since we opened only 1 queue, we know that
            # it will be assigned to partitionId 0.

    cluster.config.domains.clear()
    cluster.deploy_domains()
    cluster.restart_nodes()

    assert Client.e_SUCCESS != producer.open(uri_fanout, flags=["write"], block=True)


@pytest.fixture(
    params=[
        pytest.param(
            functools.partial(single_node_cluster_config, mode=Mode.LEGACY),
            id=f"single_node_switch_fsm",
            marks=[
                pytest.mark.integrationtest,
                pytest.mark.quick_integrationtest,
                pytest.mark.pr_integrationtest,
                pytest.mark.single,
                *Mode.FSM.marks,
            ],
        )
    ]
    + [
        pytest.param(
            functools.partial(
                multi_node_cluster_config,
                mode=Mode.LEGACY,
                reverse_proxy=rp_suffix != "",
            ),
            id=f"multi_node_switch_fsm{rp_suffix}",
            marks=[
                pytest.mark.integrationtest,
                pytest.mark.quick_integrationtest,
                pytest.mark.pr_integrationtest,
                pytest.mark.multi,
                *Mode.FSM.marks,
            ],
        )
        for rp_suffix in ("", "_rp")
    ]
)
def switch_fsm_cluster(request):
    yield from cluster_fixture(request, request.param)


def test_restart_between_non_FSM_and_FSM(
    switch_fsm_cluster: Cluster, domain_urls: tc.DomainUrls
):
    """
    This test verifies that we can safely switch clusters between non-FSM and
    FSM modes.  First, we start the cluster in non-FSM mode and post/confirm
    some messages.  Then, we restart the cluster in FSM mode and post/confirm
    more messages.  Finally, we restart the cluster back in non-FSM mode and
    verify those messages.
    """
    cluster = switch_fsm_cluster
    du = domain_urls

    # Start a producer. Then, post a message on priority queue #1 and fanout
    # queue #1
    proxies = cluster.proxy_cycle()
    producer = next(proxies).create_client("producer")
    producer.open(du.uri_priority, flags=["write", "ack"], succeed=True)
    producer.post(du.uri_priority, payload=["msg1"], wait_ack=True, succeed=True)
    producer.open(du.uri_fanout, flags=["write", "ack"], succeed=True)
    producer.post(du.uri_fanout, payload=["fanout_msg1"], wait_ack=True, succeed=True)

    ensureMessageAtStorageLayer(cluster, 0, du.uri_priority, 1)
    ensureMessageAtStorageLayer(cluster, 1, du.uri_fanout, 1)

    # Consumer for fanout queue #1
    consumer_foo = next(proxies).create_client("consumer_foo")
    consumer_foo.open(du.uri_fanout_foo, flags=["read"], succeed=True)
    consumer_foo.wait_push_event()
    assert wait_until(
        lambda: len(consumer_foo.list(du.uri_fanout_foo, block=True)) == 1, 2
    )

    # Save one confirm to the storage for fanout queue #1
    consumer_foo.confirm(du.uri_fanout_foo, "+1", succeed=True)
    consumer_foo.close(du.uri_fanout_foo, succeed=True)

    cluster.stop_nodes(prevent_leader_bounce=True)

    # Reconfigure the cluster from non-FSM to FSM mode
    configure_cluster(cluster, is_fsm=True)

    cluster.start_nodes(wait_leader=True, wait_ready=True)
    # For a standard cluster, states have already been restored as part of
    # leader re-election.
    if cluster.is_single_node:
        producer.wait_state_restored()

    # The producers posts one more message on priority queue #1 and fanout
    # queue #1
    producer.post(du.uri_priority, payload=["msg2"], wait_ack=True, succeed=True)
    producer.post(du.uri_fanout, payload=["fanout_msg2"], wait_ack=True, succeed=True)

    # Consumer for priority queue #1
    consumer = next(proxies).create_client("consumer")
    consumer.open(du.uri_priority, flags=["read"], succeed=True)
    consumer.wait_push_event()
    assert wait_until(lambda: len(consumer.list(du.uri_priority, block=True)) == 2, 2)
    consumer.close(du.uri_priority, succeed=True)

    # Consumers for fanout queue #1
    consumer_bar = next(proxies).create_client("consumer_bar")
    consumer_bar.open(du.uri_fanout_bar, flags=["read"], succeed=True)
    consumer_bar.wait_push_event()
    assert wait_until(
        lambda: len(consumer_bar.list(du.uri_fanout_bar, block=True)) == 2, 2
    )
    consumer_bar.close(du.uri_fanout_bar, succeed=True)

    # make sure the previously saved confirm is not lost
    consumer_foo.open(du.uri_fanout_foo, flags=["read"], succeed=True)
    consumer_foo.wait_push_event()
    assert wait_until(
        lambda: len(consumer_foo.list(du.uri_fanout_foo, block=True)) == 1, 2
    )
    consumer_foo.close(du.uri_fanout_foo, succeed=True)

    # The producer now posts on priority queue #2 and fanout queue #2
    producer.open(du.uri_priority_2, flags=["write", "ack"], succeed=True)
    producer.post(du.uri_priority_2, payload=["new_msg1"], wait_ack=True, succeed=True)
    producer.open(du.uri_fanout_2, flags=["write", "ack"], succeed=True)
    producer.post(
        du.uri_fanout_2, payload=["new_fanout_msg1"], wait_ack=True, succeed=True
    )

    ensureMessageAtStorageLayer(cluster, 0, du.uri_priority, 2)
    ensureMessageAtStorageLayer(cluster, 1, du.uri_fanout, 2)
    ensureMessageAtStorageLayer(cluster, 2, du.uri_priority_2, 1)
    ensureMessageAtStorageLayer(cluster, 3, du.uri_fanout_2, 1)

    # Consumer for fanout queue #2
    consumer_foo.open(du.uri_fanout_2_foo, flags=["read"], succeed=True)
    consumer_foo.wait_push_event()
    assert wait_until(
        lambda: len(consumer_foo.list(du.uri_fanout_2_foo, block=True)) == 1, 2
    )

    # Save one confirm to the storage for fanout queue #2
    consumer_foo.confirm(du.uri_fanout_2_foo, "+1", succeed=True)
    consumer_foo.close(du.uri_fanout_2_foo, succeed=True)

    # Non-FSM mode has poor healing mechanism, and can have flaky dirty
    # shutdowns, so let's disable checking exit code here.
    #
    # To give an example, an in-sync node might attempt to syncrhonize with an
    # out-of-sync node, and become out-of-sync too.  FSM mode is determined to
    # eliminate these kinds of defects.
    for node in cluster.nodes():
        node.check_exit_code = False
    cluster.stop_nodes(prevent_leader_bounce=True)

    # Reconfigure the cluster from FSM to back to non-FSM mode
    configure_cluster(cluster, is_fsm=False)

    cluster.start_nodes(wait_leader=True, wait_ready=True)
    # For a standard cluster, states have already been restored as part of
    # leader re-election.
    if cluster.is_single_node:
        producer.wait_state_restored()

    # Non-FSM mode has poor healing mechanism, but restarting once more will
    # fix the flaky dirty shutdowns
    cluster.restart_nodes()
    if cluster.is_single_node:
        producer.wait_state_restored()

    # The producers posts one more message on every queue
    producer.post(du.uri_priority, payload=["msg3"], wait_ack=True, succeed=True)
    producer.post(du.uri_fanout, payload=["fanout_msg3"], wait_ack=True, succeed=True)
    producer.post(du.uri_priority_2, payload=["new_msg2"], wait_ack=True, succeed=True)
    producer.post(
        du.uri_fanout_2, payload=["new_fanout_msg2"], wait_ack=True, succeed=True
    )

    ensureMessageAtStorageLayer(cluster, 0, du.uri_priority, 3)
    ensureMessageAtStorageLayer(cluster, 1, du.uri_fanout, 3)
    ensureMessageAtStorageLayer(cluster, 2, du.uri_priority_2, 2)
    ensureMessageAtStorageLayer(cluster, 3, du.uri_fanout_2, 2)

    # Consumer for both priority queues
    consumer = next(proxies).create_client("consumer")
    consumer.open(du.uri_priority, flags=["read"], succeed=True)
    consumer.open(du.uri_priority_2, flags=["read"], succeed=True)
    # for _ in range(4):
    consumer.wait_push_event()
    assert wait_until(lambda: len(consumer.list(du.uri_priority, block=True)) == 3, 2)
    assert wait_until(lambda: len(consumer.list(du.uri_priority_2, block=True)) == 2, 2)

    # Consumers for both fanout queues
    consumer_foo.open(du.uri_fanout_foo, flags=["read"], succeed=True)
    consumer_foo.open(du.uri_fanout_2_foo, flags=["read"], succeed=True)
    consumer.wait_push_event()
    assert wait_until(
        lambda: len(consumer_foo.list(du.uri_fanout_foo, block=True)) == 2, 2
    )
    assert wait_until(
        lambda: len(consumer_foo.list(du.uri_fanout_2_foo, block=True)) == 1, 2
    )

    consumer_bar.open(du.uri_fanout_bar, flags=["read"], succeed=True)
    consumer_bar.open(du.uri_fanout_2_bar, flags=["read"], succeed=True)
    consumer_bar.wait_push_event()
    assert wait_until(
        lambda: len(consumer_bar.list(du.uri_fanout_bar, block=True)) == 3, 2
    )
    assert wait_until(
        lambda: len(consumer_bar.list(du.uri_fanout_2_bar, block=True)) == 2, 2
    )
