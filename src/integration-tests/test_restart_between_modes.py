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
from typing import List, Dict, Optional

import pytest

import blazingmq.dev.it.testconstants as tc
from blazingmq.dev.it.fixtures import (
    Cluster,
    Mode,
    cluster_fixture,
    multi_node_cluster_config,
    single_node_cluster_config,
    test_logger,
    order,
    tweak,
)  # pylint: disable=unused-import
from blazingmq.dev.it.process.broker import Broker
from blazingmq.dev.it.process.client import Client
from blazingmq.dev.it.util import wait_until

from blazingmq.dev.it.cluster_util import (
    ensure_message_at_storage_layer,
    check_if_queue_has_n_messages,
    # simulate_csl_rollover,
)

pytestmark = order(2)
timeout = 20

DEFAULT_APP_IDS = tc.TEST_APPIDS[:]


def post_few_messages(producer: Client, queue: str, messages: List[str]):
    """
    Instruct the `producer` to post many `messages` on the `queue`.
    """

    producer.post(
        queue,
        messages,
        wait_ack=True,
        succeed=True,
    )


def confirm_only_one_message(
    consumerMap: Dict[str, Client], queue: str, app_id: Optional[str] = None
):
    """
    Find the corresponding `consumer` from the `consumerMap`, and
    instruct the `consumer` to confirm one message on the `queue` with the optional `app_id`
    """

    queue_with_appid = queue if app_id is None else queue + "?id=" + app_id

    assert queue_with_appid in consumerMap, (
        f"Queue with app_id '{queue_with_appid}' not found in consumerMap"
    )
    consumer = consumerMap[queue_with_appid]
    consumer.wait_push_event()
    consumer.confirm(queue_with_appid, "+1", succeed=True)


def confirm_one_message(consumer: Client, queue: str, app_id: Optional[str] = None):
    """
    Instruct the `consumer` to confirm one message on the `queue` with the optional `app_id`
    """

    queue_with_appid = queue if app_id is None else queue + "?id=" + app_id

    consumer.open(
        queue_with_appid,
        flags=["read"],
        succeed=True,
    )
    consumer.confirm(queue_with_appid, "+1", succeed=True)
    consumer.close(queue_with_appid, succeed=True)


def configure_cluster(cluster: Cluster, is_fsm: bool):
    """
    Configure the `cluster` to FSM or Legacy mode, based on the `is_fsm` flag.
    """
    for broker in cluster.configurator.brokers.values():
        my_clusters = broker.clusters.my_clusters
        if my_clusters:
            cluster_attr = my_clusters[0].cluster_attributes
            cluster_attr.is_cslmode_enabled = is_fsm
            cluster_attr.is_fsmworkflow = is_fsm
            cluster_attr.does_fsmwrite_qlist = True
    cluster.deploy_domains()


def restart_as_fsm_mode(
    cluster: Cluster,
    producer: Client,
):
    """
    Restart the `cluster` as FSM mode.
    """

    cluster.stop_nodes(prevent_leader_bounce=True)

    # Reconfigure the cluster to FSM mode
    configure_cluster(cluster, is_fsm=True)

    cluster.start_nodes(wait_leader=True, wait_ready=True)
    # For a standard cluster, states have already been restored as part of
    # leader re-election.

    if cluster.is_single_node:
        producer.wait_state_restored()


def restart_to_fsm_single_node_with_quorum_one(
    cluster: Cluster,
    producer: Client,  # pylint: disable=unused-argument
):
    """
    Restart the `cluster` to FSM mode.
    Start only the former leader and set its quorum to 1,
    so the cluster can start up in single-node mode.
    """

    leader = cluster.last_known_leader

    cluster.stop_nodes(prevent_leader_bounce=True)

    # Reconfigure the cluster  to FSM mode
    configure_cluster(cluster, is_fsm=True)

    # Start only the former leader and set its quorum to 1
    # so the cluster can start up in single-node mode.

    leader.start()
    leader.wait_until_started()
    leader.set_quorum(1)

    # Replication factor has to be set to 1
    # to run 4-node cluster in single-node mode
    # Otherwise the node gets stuck waiting for receipts from replicas
    leader.set_replication_factor(1)

    cluster.wait_status(wait_leader=True, wait_ready=True)


def restart_to_fsm_single_node_with_quorum_one_and_start_others(
    cluster: Cluster,
    producer: Client,  # pylint: disable=unused-argument
):
    """
    Restart the `cluster` to FSM mode.
    Start only the former leader and set its quorum to 1,
    and then start all other nodes.
    """

    leader = cluster.last_known_leader

    cluster.stop_nodes(prevent_leader_bounce=True)

    # Reconfigure the cluster to FSM mode
    configure_cluster(cluster, is_fsm=True)

    # Start only the former leader and set its quorum to 1
    # so the cluster can start up in single-node mode.

    leader.start()
    leader.wait_until_started()
    leader.set_quorum(1)
    cluster.wait_status(wait_leader=True, wait_ready=True)

    time.sleep(5)  # wait a bit before starting other nodes
    for node in cluster.nodes():
        if node != leader:
            node.start()
            node.wait_until_started()


def restart_as_legacy_mode(
    cluster: Cluster,
    producer: Client,  # pylint: disable=unused-argument
):
    """
    Restart the `cluster` as Legacy mode.
    """

    # Legacy mode has poor healing mechanism, and can have flaky dirty
    # shutdowns, so let's disable checking exit code here.
    #
    # To give an example, an in-sync node might attempt to syncrhonize with an
    # out-of-sync node, and become out-of-sync too.  FSM mode is determined to
    # eliminate these kinds of defects.
    for node in cluster.nodes():
        node.check_exit_code = False
    cluster.stop_nodes(prevent_leader_bounce=True)

    # Reconfigure the cluster to Legacy mode
    configure_cluster(cluster, is_fsm=False)
    cluster.lower_leader_startup_wait()
    if cluster.is_single_node:
        cluster.start_nodes(wait_leader=True, wait_ready=True)
    else:
        # Switching to Legacy mode could introduce start-up failure,
        # which auto-resolve upon a second restart.
        cluster.start_nodes(wait_leader=False, wait_ready=False)
        check_exited_nodes_and_restart(cluster)


def check_exited_nodes_and_restart(cluster: Cluster):
    """
    Wait and check if any nodes in the `cluster` have exited.  Attempt to
    start them again.
    """
    # Wait for some seconds in case of a node start-up failure.
    NODE_START_UP_FAILURE_WAIT_TIME_SEC = 12.5
    time.sleep(NODE_START_UP_FAILURE_WAIT_TIME_SEC)
    test_logger.info("Checking if any nodes have exited")
    for node in cluster.nodes():
        if not node.is_alive():
            test_logger.info(
                "Node %s has exited with code: %d, attempting to start it again",
                node.name,
                node.returncode,
            )
            node.start()
            node.wait_until_started()
    cluster.wait_status(wait_leader=True, wait_ready=True)


def verify_messages(
    consumerMap: Dict[str, Client],
    queue: str,
    appId: Optional[str],
    expected_count: int,
):
    """
    Find the corresponding `consumer` from the `consumerMap`, and
    instruct the `consumer` to verify that the `queue` with the optional
    `appId` has `expected_count` unconfirmed messages.
    """
    queue_with_appid = queue if not appId else queue + f"?id={appId}"

    assert queue_with_appid in consumerMap, (
        f'Consumer for queue "{queue_with_appid}" not found'
    )
    consumer = consumerMap[queue_with_appid]

    actual_count = 0

    def check():
        nonlocal actual_count
        msgs = consumer.list(queue_with_appid, block=True)
        actual_count = len(msgs)
        return actual_count == expected_count

    consumer.wait_push_event()
    assert wait_until(check, timeout=3), (
        f"Queue {queue_with_appid} does not have expected {expected_count} messages, actual: {actual_count}"
    )


def post_new_queues_and_verify(
    cluster: Cluster,
    producer: Client,
    consumerMap: Dict[str, Client],
    existing_queues_pair: List[List[str]],
    domain_urls: tc.DomainUrls,
):
    """
    On the `cluster`, create a new priority queue and a
    new fanout queue based on `domain_urls`, and append them to the
    `existing_queues_pair` of existing priority and fanout
    queues respectively.  Then, instruct the `producer` to post one message on
    the two new queues, and the `consumers` list of priority, foo, and bar consumers to open the queue.  Finally, instruct consumer_foo to confirm one message on appId 'foo'.
    """
    # Preconditions
    NUM_QUEUE_MODES = (
        2  # We only test on priority and fanout queues; broadcast queues are omitted.
    )
    assert len(existing_queues_pair) == NUM_QUEUE_MODES

    num_partitions = cluster.config.definition.partition_config.num_partitions
    existing_priority_queues, existing_fanout_queues = existing_queues_pair
    # Since we always append a queue to both lists, their length must be equal.
    assert len(existing_priority_queues) == len(existing_fanout_queues)

    proxies = cluster.proxy_cycle()
    proxy = next(proxies)

    # Post a message on new priority queue and new fanout queue
    n = len(existing_priority_queues)
    test_logger.info(
        f"There are currently {n} priority queues and {n} fanout queues, opening one more of each"
    )
    partition_id = (n * NUM_QUEUE_MODES) % num_partitions
    for domain, queues, consuming_app_ids in [
        (
            domain_urls.domain_priority,
            existing_priority_queues,
            [None],
        ),
        (
            domain_urls.domain_fanout,
            existing_fanout_queues,
            ["foo", "bar"],
        ),
    ]:
        new_queue = f"bmq://{domain}/qqq{n}"
        queues.append((new_queue, partition_id))

        producer.open(new_queue, flags=["write", "ack"], succeed=True)
        post_few_messages(producer, new_queue, ["msg0"])
        ensure_message_at_storage_layer(cluster, partition_id, new_queue, 1, alive=True)

        for app_id in consuming_app_ids:
            consumer_queue = new_queue if not app_id else new_queue + f"?id={app_id}"
            consumer = (
                consumerMap[consumer_queue]
                if consumer_queue in consumerMap
                else proxy.create_client(f"consumer_{app_id or 'priority'}")
            )
            consumer.open(
                consumer_queue,
                flags=["read"],
                succeed=True,
            )
            consumerMap[consumer_queue] = consumer

        # Per our queue assignment logic, the new queue will be assigned to the next partition in a round-robin fashion.
        partition_id = (partition_id + 1) % num_partitions

    # Save one confirm to the storage for new fanout queue
    QUEUE_ELEMENT_IDX = 0
    new_fanout_queue = existing_fanout_queues[-1][QUEUE_ELEMENT_IDX]

    verify_messages(consumerMap, new_fanout_queue, "foo", 1)

    confirm_only_one_message(consumerMap, new_fanout_queue, "foo")

    # Postconditions
    assert len(existing_priority_queues) == len(existing_fanout_queues)


def post_existing_queues_and_verify(
    cluster: Cluster,
    producer: Client,
    consumerMap: Dict[str, Client],
    existing_priority_queues: List[str],
    existing_fanout_queues: List[str],
):
    """
    On the `cluster`, instruct the `producer` to post one message to
    each of the `existing_priority_queues` and `existing_fanout_queues`.
    Verify that the messages are posted successfully and that the `consumers`
    list of priority, foo, and bar consumers have the expected number of
    messages in their respective queues.
    """
    # Preconditions
    assert len(existing_priority_queues) == len(existing_fanout_queues)

    QUEUE_ELEMENT_IDX, PARTITION_ELEMENT_IDX = 0, 1
    n = len(existing_priority_queues)
    test_logger.info(
        f"Posting one message to each of existing {n} priority queues and {n} fanout queues"
    )
    for i in range(n):
        # Every time we append a new queue, we post one message to it as well as
        # all existing queues.  When there are `n` queues, the last queue (index
        # `n-1`) will have 1 message, the second last queue (index `n-2`) will
        # have 2 messages, and so on.  Therefore, the number of messages in the
        # i-th queue will be `n - i`.
        NUM_MESSAGES_IN_QUEUE = n - i

        for queues in [existing_priority_queues, existing_fanout_queues]:
            post_few_messages(
                producer, queues[i][QUEUE_ELEMENT_IDX], [f"msg{NUM_MESSAGES_IN_QUEUE}"]
            )
            ensure_message_at_storage_layer(
                cluster,
                queues[i][PARTITION_ELEMENT_IDX],
                queues[i][QUEUE_ELEMENT_IDX],
                NUM_MESSAGES_IN_QUEUE + 1,
                alive=True,
            )

        NUM_MESSAGES_IN_QUEUE += 1  # Increment by 1 for the new message posted

        verify_messages(
            consumerMap,
            existing_priority_queues[i][QUEUE_ELEMENT_IDX],
            None,
            NUM_MESSAGES_IN_QUEUE,
        )
        verify_messages(
            consumerMap,
            existing_fanout_queues[i][QUEUE_ELEMENT_IDX],
            "foo",
            NUM_MESSAGES_IN_QUEUE - 1,  # Already confirmed one message on `foo`
        )
        verify_messages(
            consumerMap,
            existing_fanout_queues[i][QUEUE_ELEMENT_IDX],
            "bar",
            NUM_MESSAGES_IN_QUEUE,
        )


def close_and_unassign_queue(queueUri: str, client: Client, leader: Broker):
    """
    Close and unassign the `queueUri` connected to by the `client`.  Instruct
    the `leader` to force garbage collection of queues.
    """
    assert client.close(queueUri, succeed=True) == Client.e_SUCCESS

    leader.force_gc_queues(succeed=True)


def assignUnassignNewQueues(
    existing_queues: List[str],
    domain_urls: tc.DomainUrls,
    consumer: Client,
    leader: Broker,
):
    """
    Create two new priority queues based on `domain_urls`, and append them
    to `existing_queues`.  The first queue is assigned and then unassigned,
    while the second queue is only assigned.  The `consumer` is used to open
    and close the queues, while the `leader` is used to force garbage collection of queues.
    """
    n = len(existing_queues)
    new_queue_1 = f"bmq://{domain_urls.domain_priority}/qqq{n}"
    consumer.open(new_queue_1, flags=["read"], succeed=True)
    close_and_unassign_queue(new_queue_1, consumer, leader)
    existing_queues.append(new_queue_1)

    new_queue_2 = f"bmq://{domain_urls.domain_priority}/qqq{n + 1}"
    consumer.open(new_queue_2, flags=["read"], succeed=True)
    existing_queues.append(new_queue_2)


def assignUnassignExistingQueues(
    existing_queues: List[str], consumer: Client, leader: Broker
):
    """
    Enumerate through `existing_queues` and perform the following operations:
    - For even indexed queues, assign then unassign.
    - For odd indexed queues, unassign then assign.

    The `consumer` is used to open and close the queues, while the `leader`
    is used to force garbage collection of queues.
    """
    for i, queue in enumerate(existing_queues):
        if i % 2 == 0:
            # Even index: assign -> unassign
            consumer.open(queue, flags=["read"], succeed=True)
            close_and_unassign_queue(queue, consumer, leader)
        else:
            # Odd index: unassign -> assign
            close_and_unassign_queue(queue, consumer, leader)
            consumer.open(queue, flags=["read"], succeed=True)


def test_restart_between_Legacy_and_FSM(
    cluster: Cluster, domain_urls: tc.DomainUrls, switch_cluster_mode
):
    """
    This test verifies that we can safely switch clusters between Legacy and
    FSM modes.

    First, we start the cluster and post/confirm some messages.

    Then, we restart the cluster to another mode provided by switch fixture
    and post/confirm more messages.
    Applied one of the switches:
        - to Legacy
        - to FSM
        - to FSM single node with quorum 1
        - to FSM single node with quorum 1, then start other nodes

    Finally, we restart the cluster back to the corresponding backup mode and
    verify those messages.
    """
    du = domain_urls

    # Start a producer.
    proxies = cluster.proxy_cycle()
    proxy = next(proxies)
    producer = proxy.create_client("producer")

    existing_priority_queues = []
    existing_fanout_queues = []

    consumerMap = {}

    # Phase 1: From Legacy Mode to FSM Mode

    # PROLOGUE
    post_new_queues_and_verify(
        cluster,
        producer,
        consumerMap,
        [existing_priority_queues, existing_fanout_queues],
        du,
    )

    # SWITCH
    switch_cluster_mode[0](cluster, producer)

    # EPILOGUE
    post_existing_queues_and_verify(
        cluster,
        producer,
        consumerMap,
        existing_priority_queues,
        existing_fanout_queues,
    )

    # Phase 2: From FSM Mode to Legacy Mode

    # PROLOGUE
    post_new_queues_and_verify(
        cluster,
        producer,
        consumerMap,
        [existing_priority_queues, existing_fanout_queues],
        du,
    )

    # SWITCH
    switch_cluster_mode[1](cluster, producer)

    # EPILOGUE
    post_existing_queues_and_verify(
        cluster,
        producer,
        consumerMap,
        existing_priority_queues,
        existing_fanout_queues,
    )


@tweak.cluster.queue_operations.keepalive_duration_ms(1000)
def test_restart_between_Legacy_and_FSM_unassign_queue(
    cluster: Cluster, domain_urls: tc.DomainUrls
):
    """
    This test verifies that we can safely assign and unassign queues while
    switching the cluster between Legacy and FSM modes.  We start the cluster
    in Legacy mode, then conduct queue assignment/unassignment operations as
    follows:

    q0: assign -> unassign ---> assign -> unassign                         ---> assign -> unassign
    q1: assign             ---> unassign -> assign                         ---> unassign -> assign
    q2:                         None               ---> assign -> unassign ---> assign -> unassign
    q3:                         None               ---> assign             ---> unassign -> assign

    where  `--->` indicates a switch in cluster mode
    """
    cluster.lower_leader_startup_wait()
    du = domain_urls
    leader = cluster.last_known_leader

    proxies = cluster.proxy_cycle()
    consumer = next(proxies).create_client("consumer")

    existing_queues = []

    # PROLOGUE
    assignUnassignNewQueues(existing_queues, du, consumer, leader)

    # Wait one second to ensure that the primary has issued a sync point
    # containing the latest queue (un)assignment records.  This way, when the
    # cluster stops, we can ensure that every node already has a sync point
    # covering the latest queue (un)assignment records.
    SYNC_POINT_WAIT_TIME_SEC = 1.1
    time.sleep(SYNC_POINT_WAIT_TIME_SEC)

    cluster.disable_exit_code_check()
    cluster.stop_nodes(prevent_leader_bounce=True, exclude=[leader])
    configure_cluster(cluster, is_fsm=True)

    # TODO Test assign/unassign queues before stopping leader
    leader.stop()
    # TODO When you start, start the leader first
    cluster.lower_leader_startup_wait()
    if cluster.is_single_node:
        cluster.start_nodes(wait_leader=True, wait_ready=True)
        # For a standard cluster, states have already been restored as part of
        # leader re-election.
        consumer.wait_state_restored()
    else:
        # Switching from Legacy to FSM mode could introduce start-up failure,
        # which auto-resolve upon a second restart.
        cluster.start_nodes(wait_leader=False, wait_ready=False)
        check_exited_nodes_and_restart(cluster)

    # EPILOGUE
    leader = cluster.last_known_leader
    assignUnassignExistingQueues(existing_queues, consumer, leader)

    # PROLOGUE
    assignUnassignNewQueues(existing_queues, du, consumer, leader)

    # Wait one second to ensure that the primary has issued a sync point
    # containing the latest queue (un)assignment records.  This way, when the
    # cluster stops, we can ensure that every node already has a sync point
    # covering the latest queue (un)assignment records.
    time.sleep(SYNC_POINT_WAIT_TIME_SEC)

    cluster.stop_nodes(prevent_leader_bounce=True, exclude=[leader])

    # TODO Test assign/unassign queues before stopping leader
    leader.stop()
    # TODO When you start, start the leader first
    configure_cluster(cluster, is_fsm=False)
    cluster.lower_leader_startup_wait()
    if cluster.is_single_node:
        cluster.start_nodes(wait_leader=True, wait_ready=True)
        # For a standard cluster, states have already been restored as part of
        # leader re-election.
        consumer.wait_state_restored()
    else:
        # Switching from FSM to Legacy mode could introduce start-up failure,
        # which auto-resolve upon a second restart.
        cluster.start_nodes(wait_leader=False, wait_ready=False)
        check_exited_nodes_and_restart(cluster)

    # EPILOGUE
    leader = cluster.last_known_leader
    assignUnassignExistingQueues(existing_queues, consumer, leader)


@pytest.fixture(
    params=[
        (restart_as_fsm_mode, restart_as_legacy_mode),
        (restart_as_legacy_mode, restart_as_fsm_mode),
        (restart_to_fsm_single_node_with_quorum_one, restart_as_legacy_mode),
        (
            restart_to_fsm_single_node_with_quorum_one_and_start_others,
            restart_as_legacy_mode,
        ),
    ],
    ids=[
        "to_fsm/from_legacy",
        "to_legacy/from_fsm",
        "to_fsm_quorum_1/from_legacy",
        "to_fsm_quorum_1_then_start_all/from_legacy",
    ],
)
def switch_cluster_mode(request):
    """
    Fixture to switch between cluster modes
    """

    return request.param


def without_rollover(
    du: tc.DomainUrls,  # pylint: disable=unused-argument
    leader: Broker,  # pylint: disable=unused-argument
    producer: Client,  # pylint: disable=unused-argument
):
    """
    Simulate fixture scenario without rollover. Just do nothing.
    """


@pytest.fixture(
    params=[
        without_rollover,
        # simulate_journal_rollover
    ]
)
def optional_rollover(request):
    """
    Fixture to optionally simulate rollover of CSL file.
    """

    return request.param


def test_restart_between_legacy_and_fsm_add_remove_app(
    cluster: Cluster,
    domain_urls: tc.DomainUrls,
    switch_cluster_mode,
    optional_rollover,
):
    """
    This test verifies that we can safely switch clusters between Legacy and
    FSM modes and add/remove appIds.

    Cluster fixture starts as:
    - Legacy mode
    - FSM mode

    Switch_cluster_mode fixture switches to:
    - Legacy mode
    - FSM mode
    - FSM mode with single node and quorum 1
    - FSM mode with single node and quorum 1, then start other nodes

    1. PROLOGUE:
        - both priority and fanout queues
        - post two messages
        - confirm one on priority and "foo"
        - add "quux" to the fanout
        - post to fanout
        - remove "bar"
    2. SWITCH:
        Apply one of the switches:
        - to Legacy
        - to FSM
        - to FSM single node with quorum 1
        - to FSM single node with quorum 1, then start other nodes
    3. VERIFY:
        - priority consumer gets the second message
        - "foo" gets 2 messages
        - "bar" gets 0 messages
        - "baz" gets 3 messages
        - "quux" gets the third message

    4. Add/Remove more appIds
        Add "corge" appId
        Post to fanout queue
        Remove "foo" appId
        Post more messages to priority and fanout queues
    5. SWITCH BACK
        Switch back to the corresponding backup mode
    6. VERIFY AGAIN
    """

    du = domain_urls
    proxy = next(cluster.proxy_cycle())
    producer = proxy.create_client("producer")

    test_queue = tc.TEST_QUEUE

    # 1. PROLOGUE
    priority_queue = f"bmq://{du.domain_priority}/{test_queue}"
    fanout_queue = f"bmq://{du.domain_fanout}/{test_queue}"

    # post two messages
    for queue in [priority_queue, fanout_queue]:
        producer.open(queue, flags=["write,ack"], succeed=True)

    for queue in [priority_queue, fanout_queue]:
        post_few_messages(producer, queue, ["msg1", "msg2"])

    consumer = proxy.create_client("consumer")
    confirm_one_message(consumer, priority_queue)
    confirm_one_message(consumer, fanout_queue, "foo")

    current_app_ids = DEFAULT_APP_IDS + ["quux"]
    cluster.set_app_ids(current_app_ids, du)

    post_few_messages(producer, fanout_queue, ["msg3"])

    current_app_ids.remove("bar")
    cluster.set_app_ids(current_app_ids, du)

    # 2. SWITCH
    # 2.1 Optional rollover
    optional_rollover(du, cluster.last_known_leader, producer)
    # 2.2 Switch cluster mode
    switch_cluster_mode[0](cluster, producer)

    # 3. VERIFY
    check_if_queue_has_n_messages(consumer, priority_queue, 1)
    check_if_queue_has_n_messages(consumer, fanout_queue + "?id=foo", 2)
    check_if_queue_has_n_messages(consumer, fanout_queue + "?id=bar", 0)
    check_if_queue_has_n_messages(consumer, fanout_queue + "?id=baz", 3)
    quux_messages = check_if_queue_has_n_messages(
        consumer, fanout_queue + "?id=quux", 1
    )
    assert re.match(r"msg3", quux_messages[0].payload)

    # 4. POST MORE MESSAGES
    current_app_ids.append("corge")
    cluster.set_app_ids(current_app_ids, du)
    post_few_messages(producer, fanout_queue, ["msg4"])

    current_app_ids.remove("foo")
    cluster.set_app_ids(current_app_ids, du)

    for queue in [priority_queue, fanout_queue]:
        post_few_messages(producer, queue, ["msg5"])

    # 5. SWITCH BACK
    switch_cluster_mode[1](cluster, producer)

    # 6. VERIFY AGAIN
    check_if_queue_has_n_messages(consumer, priority_queue, 1 + 1)
    check_if_queue_has_n_messages(consumer, fanout_queue + "?id=foo", 0)
    check_if_queue_has_n_messages(consumer, fanout_queue + "?id=bar", 0)
    check_if_queue_has_n_messages(consumer, fanout_queue + "?id=baz", 3 + 2)
    check_if_queue_has_n_messages(consumer, fanout_queue + "?id=quux", 1 + 2)
    check_if_queue_has_n_messages(consumer, fanout_queue + "?id=corge", 0 + 2)


def test_restart_between_legacy_and_fsm_purge_queue_app(
    cluster: Cluster,
    domain_urls: tc.DomainUrls,
    switch_cluster_mode,
    optional_rollover,
):
    """
    This test verifies that we can safely switch clusters between Legacy and
    FSM modes, add/remove appIds and purge queues/appIds.

    Cluster fixture starts as:
    - Legacy mode
    - FSM mode

    Switch_cluster_mode fixture switches to:
    - Legacy mode
    - FSM mode
    - FSM mode with single node and quorum 1
    - FSM mode with single node and quorum 1, then start other nodes

    1. PROLOGUE:
        - both priority and fanout queues
        - post one message
        - purge priority queue
        - purge app "baz"
        - post two messages
        - confirm one on priority and "foo"
        - add "quux" to the fanout
        - post to fanout
        - remove "bar"
    2. SWITCH:
        Apply one of the switches:
        - to Legacy
        - to FSM
        - to FSM single node with quorum 1
        - to FSM single node with quorum 1, then start other nodes
    3. VERIFY:
        - priority consumer gets the third message
        - "foo" gets 3 messages
        - "bar" gets 0 messages
        - "baz" gets 3 messages
        - "quux" gets the fourth message

    4. POST MORE MESSAGES
    5. SWITCH BACK
        Switch back to the corresponding backup mode
    6. VERIFY AGAIN
    """

    du = domain_urls
    proxy = next(cluster.proxy_cycle())
    producer = proxy.create_client("producer")

    test_queue = tc.TEST_QUEUE

    # 1. PROLOGUE
    priority_queue = f"bmq://{du.domain_priority}/{test_queue}"
    fanout_queue = f"bmq://{du.domain_fanout}/{test_queue}"

    # Post one message
    for queue in [priority_queue, fanout_queue]:
        producer.open(queue, flags=["write,ack"], succeed=True)

    for queue in [priority_queue, fanout_queue]:
        post_few_messages(producer, queue, ["msg0"])

    # Purge priority queue
    cluster.last_known_leader.purge(du.domain_priority, test_queue, succeed=True)
    # Purge fanout queue app "baz"
    cluster.last_known_leader.purge(du.domain_fanout, test_queue, "baz", succeed=True)

    # Post two messages
    for queue in [priority_queue, fanout_queue]:
        post_few_messages(producer, queue, ["msg1", "msg2"])

    consumer = proxy.create_client("consumer")
    confirm_one_message(consumer, priority_queue)
    confirm_one_message(consumer, fanout_queue, "foo")

    current_app_ids = DEFAULT_APP_IDS + ["quux"]
    cluster.set_app_ids(current_app_ids, du)

    post_few_messages(producer, fanout_queue, ["msg3"])

    current_app_ids.remove("bar")
    cluster.set_app_ids(current_app_ids, du)

    # 2. SWITCH
    # 2.1 Optional rollover
    optional_rollover(du, cluster.last_known_leader, producer)
    # 2.2 Switch cluster mode
    switch_cluster_mode[0](cluster, producer)

    # 3. VERIFY
    check_if_queue_has_n_messages(consumer, priority_queue, 1)
    check_if_queue_has_n_messages(consumer, fanout_queue + "?id=foo", 3)
    check_if_queue_has_n_messages(consumer, fanout_queue + "?id=bar", 0)
    check_if_queue_has_n_messages(consumer, fanout_queue + "?id=baz", 3)
    quux_messages = check_if_queue_has_n_messages(
        consumer, fanout_queue + "?id=quux", 1
    )
    assert re.match(r"msg3", quux_messages[0].payload)

    # 4. POST MORE MESSAGES
    for queue in [priority_queue, fanout_queue]:
        post_few_messages(producer, queue, ["msg4"])

    # 5. SWITCH BACK
    switch_cluster_mode[1](cluster, producer)

    # 6. VERIFY AGAIN
    check_if_queue_has_n_messages(consumer, priority_queue, 1 + 1)
    check_if_queue_has_n_messages(consumer, fanout_queue + "?id=foo", 3 + 1)
    check_if_queue_has_n_messages(consumer, fanout_queue + "?id=bar", 0)
    check_if_queue_has_n_messages(consumer, fanout_queue + "?id=baz", 3 + 1)
    check_if_queue_has_n_messages(consumer, fanout_queue + "?id=quux", 1 + 1)
