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

import time
from typing import List

import blazingmq.dev.it.testconstants as tc
from blazingmq.dev.it.fixtures import (  # pylint: disable=unused-import
    Cluster,
    cluster,
    test_logger,
    order,
    multi_node,
    tweak,
)
from blazingmq.dev.it.process.client import Client
from blazingmq.dev.it.util import attempt, wait_until

pytestmark = order(3)

default_app_ids = ["foo", "bar", "baz"]
timeout = 60
max_msgs = 3

set_max_messages = tweak.domain.storage.queue_limits.messages(max_msgs)


def set_app_ids(cluster: Cluster, app_ids: List[str]):  # noqa: F811
    cluster.config.domains[
        tc.DOMAIN_FANOUT
    ].definition.parameters.mode.fanout.app_ids = app_ids  # type: ignore
    cluster.reconfigure_domain(tc.DOMAIN_FANOUT, succeed=True)


def test_open_alarm_authorize_post(cluster: Cluster):
    leader = cluster.last_known_leader
    proxies = cluster.proxy_cycle()

    producer = next(proxies).create_client("producer")
    producer.open(tc.URI_FANOUT, flags=["write,ack"], succeed=True)

    all_app_ids = default_app_ids + ["quux"]

    # ---------------------------------------------------------------------
    # Create a consumer for each authorized substream.

    consumers = {}

    for app_id in default_app_ids:
        consumer = next(proxies).create_client(app_id)
        consumers[app_id] = consumer
        consumer.open(f"{tc.URI_FANOUT}?id={app_id}", flags=["read"], succeed=True)

    # ---------------------------------------------------------------------
    # Create a consumer for the unauthorized substream. This should succeed
    # but with an ALARM.

    quux = next(proxies).create_client("quux")
    consumers["quux"] = quux
    assert (
        quux.open(f"{tc.URI_FANOUT}?id=quux", flags=["read"], block=True)
        == Client.e_SUCCESS
    )
    assert leader.alarms()

    # ---------------------------------------------------------------------
    # Check that authorized substreams are alive and 'quux' is unauthorized.

    leader.dump_queue_internals(tc.DOMAIN_FANOUT, tc.TEST_QUEUE)

    bar_status, baz_status, foo_status, quuxStatus = sorted(
        [
            leader.capture(r"(\w+).*: status=(\w+)(?:, StorageIter.atEnd=(\w+))?", 60)
            for i in all_app_ids
        ],
        key=lambda match: match[1],
    )
    assert bar_status[2] == "alive"
    assert baz_status[2] == "alive"
    assert foo_status[2] == "alive"
    assert quuxStatus.group(2, 3) == ("unauthorized", None)

    assert (
        quux.configure(
            f"{tc.URI_FANOUT}?id=quux", max_unconfirmed_messages=10, block=True
        )
        == Client.e_SUCCESS
    )

    # ---------------------------------------------------------------------
    # Post a message.
    producer.post(tc.URI_FANOUT, ["msg1"], succeed=True, wait_ack=True)

    # ---------------------------------------------------------------------
    # Check that 'quux' (unauthorized) client did not receive it.
    test_logger.info('Check that "quux" has not seen any messages')
    assert not quux.wait_push_event(timeout=2, quiet=True)
    assert len(quux.list(f"{tc.URI_FANOUT}?id=quux", block=True)) == 0

    # ---------------------------------------------------------------------
    # Authorize 'quux'.
    set_app_ids(cluster, default_app_ids + ["quux"])

    # ---------------------------------------------------------------------
    # Check that all substreams are alive.

    leader.dump_queue_internals(tc.DOMAIN_FANOUT, tc.TEST_QUEUE)

    for app_id in all_app_ids:
        leader.outputs_regex(r"(\w+).*: status=alive", timeout)

    # ---------------------------------------------------------------------
    # Post a second message.

    producer.post(tc.URI_FANOUT, ["msg2"])
    assert producer.outputs_regex(r"MESSAGE.*ACK", timeout)

    # ---------------------------------------------------------------------
    # Ensure that previously authorized substreams get 2 messages and the
    # newly authorized gets one.

    leader.dump_queue_internals(tc.DOMAIN_FANOUT, tc.TEST_QUEUE)
    # pylint: disable=cell-var-from-loop; passing lambda to 'wait_until' is safe
    for app_id in default_app_ids:
        test_logger.info(f"Check if {app_id} has seen 2 messages")
        assert wait_until(
            lambda: len(
                consumers[app_id].list(f"{tc.URI_FANOUT}?id={app_id}", block=True)
            )
            == 2,
            3,
        )

    test_logger.info("Check if quux has seen 1 message")
    assert wait_until(
        lambda: len(quux.list(f"{tc.URI_FANOUT}?id=quux", block=True)) == 1, 3
    )

    for app_id in all_app_ids:
        assert (
            consumers[app_id].close(f"{tc.URI_FANOUT}?id={app_id}", block=True)
            == Client.e_SUCCESS
        )

    # Start the 'quux' consumer and then ensure that no alarm is raised at
    # leader/primary when a consumer for a recently authorized appId is
    # stopped and started.

    quux.open(f"{tc.URI_FANOUT}?id=quux", flags=["read"], succeed=True)
    assert not leader.alarms()


def test_create_authorize_open_post(cluster: Cluster):
    leader = cluster.last_known_leader
    proxies = cluster.proxy_cycle()

    producer = next(proxies).create_client("producer")
    producer.open(tc.URI_FANOUT, flags=["write"], succeed=True)

    # ---------------------------------------------------------------------
    # Authorize 'quux'.
    set_app_ids(cluster, default_app_ids + ["quux"])

    # ---------------------------------------------------------------------
    # Create a consumer for 'quux. This should succeed.

    quux = next(proxies).create_client("quux")
    assert (
        quux.open(f"{tc.URI_FANOUT}?id=quux", flags=["read"], block=True)
        == Client.e_SUCCESS
    )

    # ---------------------------------------------------------------------
    # Check that all substreams are alive.

    leader.dump_queue_internals(tc.DOMAIN_FANOUT, tc.TEST_QUEUE)
    leader.outputs_regex(r"quux.*: status=alive", timeout)


def test_load_domain_authorize_open_post(cluster: Cluster):
    leader = cluster.last_known_leader
    proxies = cluster.proxy_cycle()

    producer = next(proxies).create_client("producer")
    producer.open(tc.URI_FANOUT + "_another", flags=["write"], succeed=True)

    # ---------------------------------------------------------------------
    # Authorize 'quux'.
    set_app_ids(cluster, default_app_ids + ["quux"])

    # ---------------------------------------------------------------------
    # Create a consumer for 'quux. This should succeed.

    quux = next(proxies).create_client("quux")
    quux.open(f"{tc.URI_FANOUT}?id=quux", flags=["read"], succeed=True)

    # ---------------------------------------------------------------------
    # Check that all substreams are alive.

    leader.dump_queue_internals(tc.DOMAIN_FANOUT, tc.TEST_QUEUE)
    leader.outputs_regex(r"quux.*: status=alive", timeout)


# following test cannot run yet, because domain manager claims domain
# does not exist if no queue exists in it
def _test_authorize_before_domain_loaded(cluster):
    leader = cluster.last_known_leader
    proxies = cluster.proxy_cycle()

    # ---------------------------------------------------------------------
    # Authorize 'quux'.
    set_app_ids(cluster, default_app_ids + ["quux"])

    # ---------------------------------------------------------------------
    # Create the queue.

    producer = next(proxies).create_client("producer")
    producer.open(tc.URI_FANOUT, flags=["write"], succeed=True)

    # ---------------------------------------------------------------------
    # Create a consumer for quux. This should succeed.

    quux = next(proxies).create_client("quux")
    quux.open(f"{tc.URI_FANOUT}?id=quux", flags=["read"])
    assert quux.outputs_regex(r"openQueue.*\[SUCCESS\]", timeout)

    # ---------------------------------------------------------------------
    # Check that all substreams are alive.

    leader.dump_queue_internals(tc.DOMAIN_FANOUT, tc.TEST_QUEUE)
    leader.outputs_regex(r"quux.*: status=alive", timeout)


# following test cannot run yet, because domain manager claims domain
# does not exist if no queue exists in it
def _test_command_errors(cluster):
    proxies = cluster.proxy_cycle()
    next(proxies).create_client("producer")

    set_app_ids(cluster, default_app_ids + ["quux"])

    set_app_ids(cluster, default_app_ids)


def test_unregister_in_presence_of_queues(cluster: Cluster):
    leader = cluster.last_known_leader
    proxies = cluster.proxy_cycle()

    producer = next(proxies).create_client("producer")
    producer.open(tc.URI_FANOUT, flags=["write,ack"], succeed=True)

    producer.post(tc.URI_FANOUT, ["before-unregister"], block=True)
    leader.dump_queue_internals(tc.DOMAIN_FANOUT, tc.TEST_QUEUE)

    foo = next(proxies).create_client("foo")
    foo.open(tc.URI_FANOUT_FOO, flags=["read"], succeed=True)
    bar = next(proxies).create_client("bar")
    bar.open(tc.URI_FANOUT_BAR, flags=["read"], succeed=True)
    baz = next(proxies).create_client("baz")
    baz.open(tc.URI_FANOUT_BAZ, flags=["read"], succeed=True)

    # In a moment we'll make sure no messages are sent to 'foo' after it
    # has been unregistered, so we need to eat the push event for the
    # message posted while 'foo' was still valid.
    foo.wait_push_event()

    set_app_ids(cluster, [a for a in default_app_ids if a not in ["foo"]])

    @attempt(3)
    def _():
        leader.dump_queue_internals(tc.DOMAIN_FANOUT, tc.TEST_QUEUE)
        assert leader.outputs_substr("Num virtual storages: 2")
        assert leader.outputs_substr("foo: status=unauthorized")

    test_logger.info("confirm msg 1 for bar, expecting 1 msg in storage")
    time.sleep(1)  # Let the message reach the proxy
    bar.confirm(tc.URI_FANOUT_BAR, "+1", succeed=True)

    @attempt(3)
    def _():
        leader.dump_queue_internals(tc.DOMAIN_FANOUT, tc.TEST_QUEUE)
        assert leader.outputs_regex("Storage.*: 1 messages")

    test_logger.info("confirm msg 1 for baz, expecting 0 msg in storage")
    time.sleep(1)  # Let the message reach the proxy
    baz.confirm(tc.URI_FANOUT_BAZ, "+1", succeed=True)

    @attempt(3)
    def _():
        leader.dump_queue_internals(tc.DOMAIN_FANOUT, tc.TEST_QUEUE)
        assert leader.outputs_regex("Storage.*: 0 messages")

    producer.post(tc.URI_FANOUT, ["after-unregister"], block=True)

    assert bar.wait_push_event()
    assert len(bar.list(tc.URI_FANOUT_BAR, block=True)) == 1
    assert baz.wait_push_event()
    assert len(baz.list(tc.URI_FANOUT_BAZ, block=True)) == 1

    assert not foo.wait_push_event(timeout=1)
    foo_msgs = foo.list(tc.URI_FANOUT_FOO, block=True)
    assert len(foo_msgs) == 1
    assert foo_msgs[0].payload == "before-unregister"

    assert Client.e_SUCCESS == foo.confirm(
        tc.URI_FANOUT_FOO, foo_msgs[0].guid, block=True
    )
    assert Client.e_SUCCESS == foo.close(tc.URI_FANOUT_FOO, block=True)

    # Re-authorize
    set_app_ids(cluster, default_app_ids)

    foo.open(tc.URI_FANOUT_FOO, flags=["read"], succeed=True)
    producer.post(tc.URI_FANOUT, ["after-reauthorize"], block=True)

    @attempt(3)
    def _():
        leader.dump_queue_internals(tc.DOMAIN_FANOUT, tc.TEST_QUEUE)
        leader.outputs_regex(r"foo.*: status=alive")

    assert foo.wait_push_event()
    foo_msgs = foo.list(tc.URI_FANOUT_FOO, block=True)
    assert len(foo_msgs) == 1
    assert foo_msgs[0].payload == "after-reauthorize"


def test_dynamic_twice_alarm_once(cluster: Cluster):
    leader = cluster.last_known_leader
    proxies = cluster.proxy_cycle()

    producer = next(proxies).create_client("producer")
    producer.open(tc.URI_FANOUT, flags=["write,ack"], succeed=True)

    # ---------------------------------------------------------------------
    # Create a consumer for the unauthorized substream. This should succeed
    # but with an ALARM.

    consumer1 = next(proxies).create_client("consumer1")
    assert (
        consumer1.open(f"{tc.URI_FANOUT}?id=quux", flags=["read"], block=True)
        == Client.e_SUCCESS
    )
    assert leader.alarms()

    # ---------------------------------------------------------------------
    # Create a consumer for the same unauthorized substream. This should
    # succeed and no ALARM should be generated.

    leader.drain()
    consumer2 = next(proxies).create_client("consumer2")
    assert (
        consumer2.open(f"{tc.URI_FANOUT}?id=quux", flags=["read"], block=True)
        == Client.e_SUCCESS
    )
    assert not leader.alarms()

    # ---------------------------------------------------------------------
    # Close both unauthorized substreams and re-open new one.  It should
    # succeed and alarm again.

    consumer1.close(f"{tc.URI_FANOUT}?id=quux", succeed=True)
    consumer2.close(f"{tc.URI_FANOUT}?id=quux", succeed=True)

    assert (
        consumer2.open(f"{tc.URI_FANOUT}?id=quux", flags=["read"], block=True)
        == Client.e_SUCCESS
    )
    assert leader.alarms()


@set_max_messages
def test_unauthorized_appid_doesnt_hold_messages(cluster: Cluster):
    # Goal: check that dynamically allocated, but not yet authorized,
    # substreams do not hold messages in fanout queues.
    leader = cluster.last_known_leader
    proxies = cluster.proxy_cycle()

    producer = next(proxies).create_client("producer")
    producer.open(tc.URI_FANOUT, flags=["write,ack"], succeed=True)

    # ---------------------------------------------------------------------
    # fill queue to capacity

    for i in range(max_msgs):
        producer.post(tc.URI_FANOUT, [f"msg{i}"], block=True)
        if producer.outputs_regex("ERROR.*Failed ACK.*LIMIT_MESSAGES", timeout=0):
            break

    # ---------------------------------------------------------------------
    # dynamically create a substream
    unauthorized_consumer = next(proxies).create_client("unauthorized_consumer")
    unauthorized_consumer.open(f"{tc.URI_FANOUT}?id=unauthorized", flags=["read"])
    assert leader.alarms()

    # ---------------------------------------------------------------------
    # consume all the messages in all the authorized substreams

    # pylint: disable=cell-var-from-loop; passing lambda to 'wait_until' is safe
    for app_id in default_app_ids:
        appid_uri = f"{tc.URI_FANOUT}?id={app_id}"
        consumer = next(proxies).create_client(app_id)
        consumer.open(appid_uri, flags=["read"], succeed=True)
        assert consumer.wait_push_event()
        assert wait_until(
            lambda: len(consumer.list(appid_uri, block=True)) == max_msgs, 3
        )
        consumer.confirm(appid_uri, "*", succeed=True)

    # ---------------------------------------------------------------------
    # process a new message to confirm that 'unauthorized' substream did
    # not hold messages
    producer.post(tc.URI_FANOUT, ["newMsg"], block=True)
    assert consumer.wait_push_event()
    msgs = consumer.list(appid_uri, block=True)
    assert len(msgs) == 1


@set_max_messages
def test_deauthorized_appid_doesnt_hold_messages(cluster: Cluster):
    # Goal: check that dynamically de-authorized substreams do not hold
    # messages in fanout queues.
    leader = cluster.last_known_leader
    proxies = cluster.proxy_cycle()

    # ---------------------------------------------------------------------
    # force the leader to load the domain so we can unregister the appids
    producer = next(proxies).create_client("producer")
    producer.open(tc.URI_FANOUT, flags=["write,ack"], succeed=True)

    # ---------------------------------------------------------------------
    # and remove all the queues otherwise unregistration will fail
    producer.close(tc.URI_FANOUT, succeed=True)
    leader.force_gc_queues(succeed=True)

    # ---------------------------------------------------------------------
    # unauthorize 'bar' and 'baz'
    set_app_ids(cluster, [a for a in default_app_ids if a not in ["bar", "baz"]])

    # ---------------------------------------------------------------------
    # fill queue to capacity
    time.sleep(1)
    producer.open(tc.URI_FANOUT, flags=["write,ack"], succeed=True)
    num_msgs = 4

    for i in range(0, num_msgs):
        producer.post(tc.URI_FANOUT, [f"msg{i}"], succeed=True)

    # ---------------------------------------------------------------------
    # consume messages in the 'foo' substream
    appid_uri = f"{tc.URI_FANOUT}?id=foo"
    consumer = next(proxies).create_client("foo")
    consumer.open(appid_uri, flags=["read"], succeed=True)
    assert consumer.wait_push_event()
    assert wait_until(lambda: len(consumer.list(appid_uri, block=True)) == num_msgs, 3)
    msgs = consumer.list(appid_uri, block=True)
    for _ in msgs:
        consumer.confirm(appid_uri, "+1", succeed=True)

    # process a new message to confirm that 'bar' and 'baz' substreams did
    # not hold messages
    producer.post(tc.URI_FANOUT, ["newMsg"], block=True)
    assert consumer.wait_push_event()
    msgs = consumer.list(appid_uri, block=True)
    assert len(msgs) == 1


def test_unauthorization(cluster: Cluster):
    # Goal: check that dynamically unauthorizing apps with live consumers
    #       invalidates their virtual iterators
    proxies = cluster.proxy_cycle()

    # ---------------------------------------------------------------------
    # get producer and "foo" consumer
    producer = next(proxies).create_client("producer")
    producer.open(tc.URI_FANOUT, flags=["write,ack"], succeed=True)

    appid_uri = f"{tc.URI_FANOUT}?id=foo"
    consumer = next(proxies).create_client("foo")
    consumer.open(appid_uri, flags=["read"], succeed=True)

    producer.post(tc.URI_FANOUT, ["msg1"], succeed=True)

    # ---------------------------------------------------------------------
    # unauthorize everything
    set_app_ids(cluster, [])

    # ---------------------------------------------------------------------
    # if iterators are not invalidated, 'afterNewMessage' will crash
    producer.post(tc.URI_FANOUT, ["msg2"], succeed=True)

    # ---------------------------------------------------------------------
    # check if the leader is still there
    appid_uri = f"{tc.URI_FANOUT}?id=bar"
    consumer = next(proxies).create_client("bar")
    consumer.open(appid_uri, flags=["read"], succeed=True)


def test_two_consumers_of_unauthorized_app(multi_node: Cluster):
    """Ticket 167201621: First client open authorized and unauthorized apps;
    second client opens unauthorized app.
    Then, primary shuts down causing replica to issue wildcard close
    requests to primary.
    """

    leader = multi_node.last_known_leader

    replica1 = multi_node.nodes()[0]
    if replica1 == leader:
        replica1 = multi_node.nodes()[1]

    # ---------------------------------------------------------------------
    # Two "foo" and "unauthorized" consumers
    consumer1 = replica1.create_client("consumer1")
    consumer1.open(tc.URI_FANOUT_FOO, flags=["read"], succeed=True)
    consumer1.open(f"{tc.URI_FANOUT}?id=unauthorized", flags=["read"], succeed=True)

    replica2 = multi_node.nodes()[2]
    if replica2 == leader:
        replica2 = multi_node.nodes()[3]

    consumer2 = replica2.create_client("consumer2")
    consumer2.open(f"{tc.URI_FANOUT}?id=unauthorized", flags=["read"], succeed=True)

    # ---------------------------------------------------------------------
    # shutdown and wait

    leader.stop()


@tweak.cluster.cluster_attributes.is_cslmode_enabled(False)
@tweak.cluster.cluster_attributes.is_fsmworkflow(False)
def test_open_authorize_restart_from_non_FSM_to_FSM(cluster: Cluster):
    leader = cluster.last_known_leader
    proxies = cluster.proxy_cycle()

    producer = next(proxies).create_client("producer")
    producer.open(tc.URI_FANOUT, flags=["write,ack"], succeed=True)

    all_app_ids = default_app_ids + ["quux"]

    # ---------------------------------------------------------------------
    # Create a consumer for each authorized substream.

    consumers = {}

    for app_id in all_app_ids:
        consumer = next(proxies).create_client(app_id)
        consumers[app_id] = consumer
        consumer.open(f"{tc.URI_FANOUT}?id={app_id}", flags=["read"], succeed=True)

    # ---------------------------------------------------------------------
    # Authorize 'quux'.
    set_app_ids(cluster, default_app_ids + ["quux"])

    # ---------------------------------------------------------------------
    # Post a message.
    producer.post(tc.URI_FANOUT, ["msg1"], succeed=True, wait_ack=True)

    # ---------------------------------------------------------------------
    # Post a second message.

    producer.post(tc.URI_FANOUT, ["msg2"])
    assert producer.outputs_regex(r"MESSAGE.*ACK", timeout)

    # ---------------------------------------------------------------------
    # Ensure that all substreams get 2 messages

    leader.dump_queue_internals(tc.DOMAIN_FANOUT, tc.TEST_QUEUE)
    # pylint: disable=cell-var-from-loop; passing lambda to 'wait_until' is safe
    for app_id in all_app_ids:
        test_logger.info(f"Check if {app_id} has seen 2 messages")
        assert wait_until(
            lambda: len(
                consumers[app_id].list(f"{tc.URI_FANOUT}?id={app_id}", block=True)
            )
            == 2,
            3,
        )

    # Save one confirm to the storage for 'quux' only
    consumers["quux"].confirm(f"{tc.URI_FANOUT}?id=quux", "+1", succeed=True)

    for app_id in all_app_ids:
        assert (
            consumers[app_id].close(f"{tc.URI_FANOUT}?id={app_id}", block=True)
            == Client.e_SUCCESS
        )

    cluster.stop_nodes()

    # Reconfigure the cluster from non-FSM to FSM mode
    for broker in cluster.configurator.brokers.values():
        my_clusters = broker.clusters.my_clusters
        if len(my_clusters) > 0:
            my_clusters[0].cluster_attributes.is_cslmode_enabled = True
            my_clusters[0].cluster_attributes.is_fsmworkflow = True
    cluster.deploy_domains()

    cluster.start_nodes(wait_leader=True, wait_ready=True)
    # For a standard cluster, states have already been restored as part of
    # leader re-election.
    if cluster.is_single_node:
        producer.wait_state_restored()

    for app_id in all_app_ids:
        consumer = next(proxies).create_client(app_id)
        consumers[app_id] = consumer
        consumer.open(f"{tc.URI_FANOUT}?id={app_id}", flags=["read"], succeed=True)

    # pylint: disable=cell-var-from-loop; passing lambda to 'wait_until' is safe
    for app_id in default_app_ids:
        test_logger.info(f"Check if {app_id} has seen 2 messages")
        assert wait_until(
            lambda: len(
                consumers[app_id].list(f"{tc.URI_FANOUT}?id={app_id}", block=True)
            )
            == 2,
            3,
        )

    assert wait_until(
        lambda: len(consumers["quux"].list(f"{tc.URI_FANOUT}?id=quux", block=True))
        == 1,
        3,
    )

    for app_id in all_app_ids:
        assert (
            consumers[app_id].close(f"{tc.URI_FANOUT}?id={app_id}", block=True)
            == Client.e_SUCCESS
        )


def test_remove_appid_reconfigure(cluster: Cluster):
    proxies = cluster.proxy_cycle()

    producer = next(proxies).create_client("producer")
    producer.open(tc.URI_FANOUT, flags=["write,ack"], succeed=True)

    # ---------------------------------------------------------------------
    # Create a consumer for each authorized substream.

    consumers = {}

    app_ids = default_app_ids
    victim_app_id = app_ids[0]

    for app_id in app_ids:
        consumer = next(proxies).create_client(app_id)
        consumers[app_id] = consumer
        consumer.open(f"{tc.URI_FANOUT}?id={app_id}", flags=["read"], succeed=True)

    # ---------------------------------------------------------------------
    # Post 5 messages.

    producer.post(
        tc.URI_FANOUT,
        ["msg1", "msg2", "msg3", "msg4", "msg5"],
        succeed=True,
        wait_ack=True,
    )

    # ---------------------------------------------------------------------
    # Reconfigure domain to remove app_id foo.

    app_ids.remove(victim_app_id)
    set_app_ids(cluster, app_ids)

    # ---------------------------------------------------------------------
    # Consumer connected to foo receives the message and tries to confirm.

    consumers[victim_app_id].wait_push_event(timeout=5)
    assert (
        consumers[victim_app_id].confirm(
            tc.URI_FANOUT, "+1", succeed=True, no_except=True
        )
        == Client.e_UNKNOWN
    )


def test_remove_appid_cluster_restart(cluster: Cluster):
    leader = cluster.last_known_leader
    proxies = cluster.proxy_cycle()

    producer = next(proxies).create_client("producer")
    producer.open(tc.URI_FANOUT, flags=["write,ack"], succeed=True)

    # ---------------------------------------------------------------------
    # Create a consumer for each authorized substream.

    consumers = {}

    app_ids = default_app_ids
    victim_app_id = app_ids[0]

    for app_id in app_ids:
        consumer = next(proxies).create_client(app_id)
        consumers[app_id] = consumer
        consumer.open(f"{tc.URI_FANOUT}?id={app_id}", flags=["read"], succeed=True)

    # ---------------------------------------------------------------------
    # Post 5 messages.

    producer.post(
        tc.URI_FANOUT,
        ["msg1", "msg2", "msg3", "msg4", "msg5"],
        succeed=True,
        wait_ack=True,
    )

    # ---------------------------------------------------------------------
    # Remove app_id foo and restart broker

    victim_app_id = app_ids[0]
    app_ids.remove(victim_app_id)

    cluster.restart_nodes()

    # ---------------------------------------------------------------------
    # Broker can still see the messages

    assert leader.capture_n("msg*", 5, timeout=1)

    # ---------------------------------------------------------------------
    # Consumer connected to foo receives the message and tries to confirm.

    consumers[victim_app_id].wait_push_event(timeout=5)
    assert (
        consumers[victim_app_id].confirm(
            tc.URI_FANOUT, "+1", succeed=True, no_except=True
        )
        == Client.e_UNKNOWN
    )
