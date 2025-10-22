#Copyright 2024 Bloomberg Finance L.P.
#SPDX - License - Identifier : Apache - 2.0
#
#Licensed under the Apache License, Version 2.0(the "License");
#you may not use this file except in compliance with the License.
#You may obtain a copy of the                            License at
#
#http:  // www.apache.org/licenses/LICENSE-2.0
#
#Unless required by applicable law or agreed to in writing, software
#distributed under the License is distributed      on an "AS IS" BASIS,
#WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#See the License for the specific language governing permissions and
#limitations under the License.

"""
Integration test that tests some basic things while restarting *entire* cluster
in the middle of things.  Note that this test does not test the HA
functionality (i.e., no PUTs/CONFIRMs etc are retransmitted).
"""

import functools
import re
import time
from typing import List

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
#simulate_csl_rollover,
)

pytestmark = order(2)
timeout = 20

DEFAULT_APP_IDS = tc.TEST_APPIDS[:]


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


def restart_cluster_as_fsm_mode(
    cluster: Cluster,
    producer: Client,
    consumers: List[Client],  # pylint: disable=unused-argument
):
    """
    Restart the `cluster` as FSM mode.
    """

    cluster.stop_nodes(prevent_leader_bounce=True)

#Reconfigure the cluster from non - FSM to FSM mode
    configure_cluster(cluster, is_fsm=True)

    cluster.start_nodes(wait_leader=True, wait_ready=True)
#For a standard cluster, states have already been restored as part of
#leader re - election.
    if cluster.is_single_node:
        producer.wait_state_restored()


def restart_cluster_to_fsm_single_node_with_quorum_one(
    cluster: Cluster,
    producer: Client,
    consumers: List[Client],  # pylint: disable=unused-argument
):
    """
    Restart the `cluster` from non-FSM to FSM mode.
    """

    leader = cluster.last_known_leader

    cluster.stop_nodes(prevent_leader_bounce=True)

#Reconfigure the cluster from non - FSM to FSM mode
    configure_cluster(cluster, is_fsm=True)

#Start only the former leader and set its quorum to 1
#so the cluster can start up in single - node mode.
#print(f "Updating quorum of {leader.name} to 1")
#cluster.update_node_quorum_in_config(leader.name, 1)

    print(f"Starting only the former leader {leader.name}")
    leader.start()
    print(f"Waiting until {leader.name} is started")
    leader.wait_until_started()
    print(f"{leader.name} is started")
    leader.set_quorum(1)
    print(f"Set quorum of {leader.name} to 1")
    cluster.wait_status(wait_leader=True, wait_ready=False)
    print(f"{leader.name} is leader and ready")

#For a standard cluster, states have already been restored as part of
#leader re - election.
    if cluster.is_single_node:
        producer.wait_state_restored()


def restart_cluster_as_legacy_mode(
    cluster: Cluster,
    producer: Client,  # pylint: disable=unused-argument
    consumers: List[Client],
):
    """
    Restart the `cluster` as Legacy mode.
    """

#Non - FSM mode has poor healing mechanism, and can have flaky dirty
#shutdowns, so let's disable checking exit code here.
#
#To give an example, an in - sync node might attempt to syncrhonize with an
#out - of - sync node, and become out - of - sync too.FSM mode is determined to
#eliminate these kinds of defects.
    for node in cluster.nodes():
        node.check_exit_code = False
    cluster.stop_nodes(prevent_leader_bounce=True)

#Reconfigure the cluster from FSM to back to non - FSM mode
    configure_cluster(cluster, is_fsm=False)
    cluster.lower_leader_startup_wait()
    if cluster.is_single_node:
        cluster.start_nodes(wait_leader=True, wait_ready=True)
#For a standard cluster, states have already been restored as part of
#leader re - election.
        for consumer in consumers:
            consumer.wait_state_restored()
    else:
#Switching from FSM to non - FSM mode could introduce start - up failure,
#which auto - resolve upon a second                              restart.
        cluster.start_nodes(wait_leader=False, wait_ready=False)
        check_exited_nodes_and_restart(cluster)


def check_exited_nodes_and_restart(cluster: Cluster):
    """
    Wait and check if any nodes in the `cluster` have exited.  Attempt to
    start them again.
    """
#Wait for some seconds in case of a node start - up failure.
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


def verifyMessages(consumer: Client, queue: str, appId: str, expected_count: int):
    """
    Instruct the `consumer` to verify that the `queue` with the optional
    `appId` has `expected_count` unconfirmed messages.
    """
    queue_with_appid = queue if not appId else queue + f"?id={appId}"
    consumer.wait_push_event()
    assert wait_until(
        lambda: len(consumer.list(queue_with_appid, block=True)) == expected_count,
        timeout=2,
    )


def post_new_queues_and_verify(
    cluster: Cluster,
    producer: Client,
    consumers: List[Client],
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
#Preconditions
    NUM_QUEUE_MODES = (
        2  # We only test on priority and fanout queues; broadcast queues are omitted.
    )
    assert len(existing_queues_pair) == NUM_QUEUE_MODES
    NUM_CONSUMER_TYPES = 3  # priority, foo, bar
    assert len(consumers) == NUM_CONSUMER_TYPES

    consumer_priority, consumer_foo, consumer_bar = consumers

    num_partitions = cluster.config.definition.partition_config.num_partitions
    existing_priority_queues, existing_fanout_queues = existing_queues_pair
#Since we always append a queue to both lists, their length must be equal.
    assert len(existing_priority_queues) == len(existing_fanout_queues)

#Post a message on new priority queue and new fanout queue
    n = len(existing_priority_queues)
    test_logger.info(
        f"There are currently {n} priority queues and {n} fanout queues, opening one more of each"
    )
    partition_id = (n * NUM_QUEUE_MODES) % num_partitions
    for domain, queues, domain_consumers, consuming_app_ids in [
        (
            domain_urls.domain_priority,
            existing_priority_queues,
            [consumer_priority],
            [None],
        ),
        (
            domain_urls.domain_fanout,
            existing_fanout_queues,
            [consumer_foo, consumer_bar],
            ["foo", "bar"],
        ),
    ]:
        new_queue = f"bmq://{domain}/qqq{n}"
        queues.append((new_queue, partition_id))

        producer.open(new_queue, flags=["write", "ack"], succeed=True)
        producer.post(new_queue, payload=["msg0"], wait_ack=True, succeed=True)
        ensure_message_at_storage_layer(cluster, partition_id, new_queue, 1)

        for consumer, app_id in zip(domain_consumers, consuming_app_ids):
            consumer.open(
                new_queue if not app_id else new_queue + f"?id={app_id}",
                flags=["read"],
                succeed=True,
            )

#Per our queue assignment                                     logic,          \
    the new queue will be assigned to the next partition in a round -         \
        robin                                                 fashion.
        partition_id = (partition_id + 1) % num_partitions

#Save one confirm to the storage for new fanout queue
    consumer_foo.wait_push_event()
    QUEUE_ELEMENT_IDX = 0
    new_fanout_queue = existing_fanout_queues[-1][QUEUE_ELEMENT_IDX]
    assert wait_until(
        lambda: len(consumer_foo.list(new_fanout_queue + "?id=foo", block=True)) == 1,
        timeout=2,
    )

    consumer_foo.confirm(new_fanout_queue + "?id=foo", "+1", succeed=True)

#Postconditions
    assert len(existing_priority_queues) == len(existing_fanout_queues)


def post_existing_queues_and_verify(
    cluster: Cluster,
    producer: Client,
    consumers: List[Client],
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
#Preconditions
    assert len(existing_priority_queues) == len(existing_fanout_queues)
    NUM_CONSUMER_TYPES = 3  # priority, foo, bar
    assert len(consumers) == NUM_CONSUMER_TYPES

    consumer_priority, consumer_foo, consumer_bar = consumers

    QUEUE_ELEMENT_IDX, PARTITION_ELEMENT_IDX = 0, 1
    n = len(existing_priority_queues)
    test_logger.info(
        f"Posting one message to each of existing {n} priority queues and {n} fanout queues"
    )
    for i in range(n):
#Every time we append a new queue, we post one message to it as well as
#all existing queues.When there are `n` queues, the last queue(index
# `n - 1`) will have 1 message, the second last queue(index `n - 2`) will
#have 2 messages, and so on.Therefore, the number of messages in the
#i - th queue will be `n - i`.
        NUM_MESSAGES_IN_QUEUE = n - i

        for queues in [existing_priority_queues, existing_fanout_queues]:
            producer.post(
                queues[i][QUEUE_ELEMENT_IDX],
                payload=[f"msg{NUM_MESSAGES_IN_QUEUE}"],
                wait_ack=True,
                succeed=True,
            )
            ensure_message_at_storage_layer(
                cluster,
                queues[i][PARTITION_ELEMENT_IDX],
                queues[i][QUEUE_ELEMENT_IDX],
                NUM_MESSAGES_IN_QUEUE + 1,
            )
        NUM_MESSAGES_IN_QUEUE += 1  # Increment by 1 for the new message posted

        verifyMessages(
            consumer_priority,
            existing_priority_queues[i][QUEUE_ELEMENT_IDX],
            None,
            NUM_MESSAGES_IN_QUEUE,
        )
        verifyMessages(
            consumer_foo,
            existing_fanout_queues[i][QUEUE_ELEMENT_IDX],
            "foo",
            NUM_MESSAGES_IN_QUEUE - 1,  # Already confirmed one message on `foo`
        )
        verifyMessages(
            consumer_bar,
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
#Even index : assign->unassign
            consumer.open(queue, flags=["read"], succeed=True)
            close_and_unassign_queue(queue, consumer, leader)
        else:
#Odd index : unassign->assign
            close_and_unassign_queue(queue, consumer, leader)
            consumer.open(queue, flags=["read"], succeed=True)


@pytest.fixture(
    params=[
        pytest.param(
            functools.partial(single_node_cluster_config, mode=Mode.LEGACY),
            id="single_node_switch_fsm",
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
            ),
            id="multi_node_switch_fsm",
            marks=[
                pytest.mark.integrationtest,
                pytest.mark.quick_integrationtest,
                pytest.mark.pr_integrationtest,
                pytest.mark.multi,
                *Mode.FSM.marks,
            ],
        )
    ]
)
def switch_fsm_cluster(request: pytest.FixtureRequest):
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

#Start a producer.
    proxies = cluster.proxy_cycle()
    producer = next(proxies).create_client("producer")

    existing_priority_queues = []
    existing_fanout_queues = []

    consumer_priority = next(proxies).create_client("consumer")
    consumer_foo = next(proxies).create_client("consumer_foo")
    consumer_bar = next(proxies).create_client("consumer_bar")
    consumers = [consumer_priority, consumer_foo, consumer_bar]

#Phase 1 : From Legacy Mode to FSM Mode

#PROLOGUE
    post_new_queues_and_verify(
        cluster,
        producer,
        consumers,
        [existing_priority_queues, existing_fanout_queues],
        du,
    )

#SWITCH
    restart_cluster_as_fsm_mode(cluster, producer, consumers)

#EPILOGUE
    post_existing_queues_and_verify(
        cluster,
        producer,
        consumers,
        existing_priority_queues,
        existing_fanout_queues,
    )

#Phase 2 : From FSM Mode to Legacy Mode

#PROLOGUE
    post_new_queues_and_verify(
        cluster,
        producer,
        consumers,
        [existing_priority_queues, existing_fanout_queues],
        du,
    )

#SWITCH
    restart_cluster_as_legacy_mode(cluster, producer, consumers)

#EPILOGUE
    post_existing_queues_and_verify(
        cluster,
        producer,
        consumers,
        existing_priority_queues,
        existing_fanout_queues,
    )


@tweak.cluster.queue_operations.keepalive_duration_ms(1000)
def test_restart_between_non_FSM_and_FSM_unassign_queue(
    switch_fsm_cluster: Cluster, domain_urls: tc.DomainUrls
):
    """
    This test verifies that we can safely assign and unassign queues while
    switching the cluster between non-FSM and FSM modes.  We start the cluster
    in non-FSM mode, then conduct queue assignment/unassignment operations as
    follows:

    q0: assign -> unassign ---> assign -> unassign                         ---> assign -> unassign
    q1: assign             ---> unassign -> assign                         ---> unassign -> assign
    q2:                         None               ---> assign -> unassign ---> assign -> unassign
    q3:                         None               ---> assign             ---> unassign -> assign

    where  `--->` indicates a switch in cluster mode
    """
    cluster = switch_fsm_cluster
    cluster.lower_leader_startup_wait()
    du = domain_urls
    leader = cluster.last_known_leader

    proxies = cluster.proxy_cycle()
    consumer = next(proxies).create_client("consumer")

    existing_queues = []

#PROLOGUE
    assignUnassignNewQueues(existing_queues, du, consumer, leader)

#Wait one second to ensure that the primary has issued a sync point
#containing the latest queue(un) assignment records.This way, when the
#cluster stops, we can ensure that every node already has a sync point
#covering the latest queue(un) assignment records.
    SYNC_POINT_WAIT_TIME_SEC = 1.1
    time.sleep(SYNC_POINT_WAIT_TIME_SEC)

    cluster.disable_exit_code_check()
    cluster.stop_nodes(prevent_leader_bounce=True, exclude=[leader])
    configure_cluster(cluster, is_fsm=True)

#TODO Test assign / unassign queues before stopping leader
    leader.stop()
#TODO When you start, start the leader first
    cluster.lower_leader_startup_wait()
    if cluster.is_single_node:
        cluster.start_nodes(wait_leader=True, wait_ready=True)
#For a standard cluster, states have already been restored as part of
#leader re - election.
        consumer.wait_state_restored()
    else:
#Switching from non - FSM to FSM mode could introduce start - up failure,
#which auto - resolve upon a second                              restart.
        cluster.start_nodes(wait_leader=False, wait_ready=False)
        check_exited_nodes_and_restart(cluster)

#EPILOGUE
    leader = cluster.last_known_leader
    assignUnassignExistingQueues(existing_queues, consumer, leader)

#PROLOGUE
    assignUnassignNewQueues(existing_queues, du, consumer, leader)

#Wait one second to ensure that the primary has issued a sync point
#containing the latest queue(un) assignment records.This way, when the
#cluster stops, we can ensure that every node already has a sync point
#covering the latest queue(un) assignment records.
    time.sleep(SYNC_POINT_WAIT_TIME_SEC)

    cluster.stop_nodes(prevent_leader_bounce=True, exclude=[leader])

#TODO Test assign / unassign queues before stopping leader
    leader.stop()
#TODO When you start, start the leader first
    configure_cluster(cluster, is_fsm=False)
    cluster.lower_leader_startup_wait()
    if cluster.is_single_node:
        cluster.start_nodes(wait_leader=True, wait_ready=True)
#For a standard cluster, states have already been restored as part of
#leader re - election.
        consumer.wait_state_restored()
    else:
#Switching from FSM to non - FSM mode could introduce start - up failure,
#which auto - resolve upon a second                              restart.
        cluster.start_nodes(wait_leader=False, wait_ready=False)
        check_exited_nodes_and_restart(cluster)

#EPILOGUE
    leader = cluster.last_known_leader
    assignUnassignExistingQueues(existing_queues, consumer, leader)


@pytest.fixture(
    params=[
#restart_cluster_as_fsm_mode,
#restart_cluster_as_legacy_mode,
        restart_cluster_to_fsm_single_node_with_quorum_one,
    ]
)
def switch_cluster_mode(request):
    """
    Fixture to switch cluster mode between non-FSM and FSM.
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
#simulate_csl_rollover
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

    switch_cluster_mode fixture switches to:
    - Legacy mode
    - FSM mode

    Resulting in four combinations of switches:
    1. Legacy -> FSM
    2. FSM -> Legacy
    3. Legacy -> Legacy
    4. FSM -> FSM

    1. PROLOGUE:
        - both priority and fanout queues
        - post two messages
        - confirm one on priority and "foo"
        - add "quux" to the fanout
        - post to fanout
        - remove "bar"
    2. SWITCH:
        Apply one of the switches:
        - non-FSM to FSM
        - FSM to non-FSM
    3. EPILOGUE:
        - priority consumer gets the second message
        - "foo" gets 2 messages
        - "bar" gets 0 messages
        - "baz" gets 3 messages
        - "quux" gets the third message
    """

    du = domain_urls
    proxy = next(cluster.proxy_cycle())
    producer = proxy.create_client("producer")

    test_queue = tc.TEST_QUEUE

# 1. PROLOGUE
    priority_queue = f"bmq://{du.domain_priority}/{test_queue}"
    fanout_queue = f"bmq://{du.domain_fanout}/{test_queue}"

#post two messages
    for queue in [priority_queue, fanout_queue]:
        producer.open(queue, flags=["write,ack"], succeed=True)
        producer.post(
            queue,
            ["msg1", "msg2"],
            succeed=True,
            wait_ack=True,
        )

    consumer = proxy.create_client("consumer")
    consumer.open(
        priority_queue,
        flags=["read"],
        succeed=True,
    )
    consumer.open(
        fanout_queue + "?id=foo",
        flags=["read"],
        succeed=True,
    )
    consumer.confirm(priority_queue, "+1", succeed=True)
    consumer.confirm(fanout_queue + "?id=foo", "+1", succeed=True)
    consumer.close(priority_queue, succeed=True)
    consumer.close(fanout_queue + "?id=foo", succeed=True)
    consumers = [consumer]

    current_app_ids = DEFAULT_APP_IDS + ["quux"]
    cluster.set_app_ids(current_app_ids, du)
    producer.post(
        fanout_queue,
        ["msg3"],
        succeed=True,
        wait_ack=True,
    )

    current_app_ids.remove("bar")
    cluster.set_app_ids(current_app_ids, du)

# 2. SWITCH
# 2.1 Optional rollover
    optional_rollover(du, cluster.last_known_leader, producer)
# 2.2 Switch cluster mode
    switch_cluster_mode(cluster, producer, consumers)

# 3. EPILOGUE
    check_if_queue_has_n_messages(consumer, priority_queue, 1)
    check_if_queue_has_n_messages(consumer, fanout_queue + "?id=foo", 2)
    check_if_queue_has_n_messages(consumer, fanout_queue + "?id=bar", 0)
    check_if_queue_has_n_messages(consumer, fanout_queue + "?id=baz", 3)
    quux_messages = check_if_queue_has_n_messages(
        consumer, fanout_queue + "?id=quux", 1
    )
    assert re.match(r"msg3", quux_messages[0].payload)


def xtest_restart_between_legacy_and_fsm_purge_queue_app(
    cluster: Cluster,
    domain_urls: tc.DomainUrls,
    switch_cluster_mode,
    optional_rollover,
):
    """
    This test verifies that we can safely switch clusters between non-FSM and
    FSM modes and add/remove appIds.

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
        - non-FSM to FSM
        - FSM to non-FSM
    3. EPILOGUE:
        - priority consumer gets the third message
        - "foo" gets 3 messages
        - "bar" gets 0 messages
        - "baz" gets 3 messages
        - "quux" gets the fourth message
    """

    du = domain_urls
    proxy = next(cluster.proxy_cycle())
    producer = proxy.create_client("producer")

    test_queue = tc.TEST_QUEUE

# 1. PROLOGUE
    priority_queue = f"bmq://{du.domain_priority}/{test_queue}"
    fanout_queue = f"bmq://{du.domain_fanout}/{test_queue}"

#Post one message
    for queue in [priority_queue, fanout_queue]:
        producer.open(queue, flags=["write,ack"], succeed=True)

    for queue in [priority_queue, fanout_queue]:
        producer.post(
            queue,
            ["msg0"],
            succeed=True,
            wait_ack=True,
        )

#Purge priority queue
    cluster.last_known_leader.purge(du.domain_priority, test_queue, succeed=True)
#Purge fanout queue app "baz"
    cluster.last_known_leader.purge(du.domain_fanout, test_queue, "baz", succeed=True)

#Post two messages
    for queue in [priority_queue, fanout_queue]:
        producer.post(
            queue,
            ["msg1", "msg2"],
            succeed=True,
            wait_ack=True,
        )

    consumer = proxy.create_client("consumer")
    consumer.open(
        priority_queue,
        flags=["read"],
        succeed=True,
    )
    consumer.open(
        fanout_queue + "?id=foo",
        flags=["read"],
        succeed=True,
    )
    consumer.confirm(priority_queue, "+1", succeed=True)
    consumer.confirm(fanout_queue + "?id=foo", "+1", succeed=True)
    consumer.close(priority_queue, succeed=True)
    consumer.close(fanout_queue + "?id=foo", succeed=True)
    consumers = [consumer]

    current_app_ids = DEFAULT_APP_IDS + ["quux"]
    cluster.set_app_ids(current_app_ids, du)
    producer.post(
        fanout_queue,
        ["msg3"],
        succeed=True,
        wait_ack=True,
    )

    current_app_ids.remove("bar")
    cluster.set_app_ids(current_app_ids, du)

# 2. SWITCH
# 2.1 Optional rollover
    optional_rollover(du, cluster.last_known_leader, producer)
# 2.2 Switch cluster mode
    switch_cluster_mode(cluster, producer, consumers)

# 3. EPILOGUE
    check_if_queue_has_n_messages(consumer, priority_queue, 1)
    check_if_queue_has_n_messages(consumer, fanout_queue + "?id=foo", 3)
    check_if_queue_has_n_messages(consumer, fanout_queue + "?id=bar", 0)
    check_if_queue_has_n_messages(consumer, fanout_queue + "?id=baz", 3)
    quux_messages = check_if_queue_has_n_messages(
        consumer, fanout_queue + "?id=quux", 1
    )
    assert re.match(r"msg3", quux_messages[0].payload)
