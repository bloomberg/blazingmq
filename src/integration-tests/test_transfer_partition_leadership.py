# Copyright 2026 Bloomberg Finance L.P.
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
Hand a partition's primaryship to another node while a producer and a
consumer are attached to the current primary, and verify the queue keeps
working across the demotion.

The node that gives up primaryship must convert its local queue to a remote
one and buffer, rather than fail, whatever arrives before the new primary is
reachable.  Its handles survive the change, so everything derived from them
has to survive too: the aggregated handle parameters the reopen asks the new
primary for, and the routing -- consumer priorities and subscriptions -- that
the replaced queue engine rebuilds from those handles.  Legacy cannot move
primaryship at all -- the command is rejected outright -- so these run on
'fsm_multi_cluster' only.
"""

import blazingmq.dev.it.testconstants as tc
from blazingmq.dev.it.fixtures import (
    Cluster,
    tweak,
)
from blazingmq.dev.it.process.client import Client
from blazingmq.dev.it.util import wait_until


def _int_property(name, value):
    return [{"name": name, "value": str(value), "type": "E_INT"}]


def _place_queue(cluster: Cluster, uri):
    """
    Create the queue and return the node that is primary for its partition,
    along with that partition's id.  The queue has to exist before its
    partition has a primary to move.
    """
    leader = cluster.last_known_leader
    probe = leader.create_client("probe")
    probe.open(uri, flags=["write,ack"], succeed=True)
    primary = leader.wait_queue_primary(uri)
    partition_id = leader.queue_partition_id(uri)
    probe.stop_session(block=True)

    return primary, partition_id


def _move_primary(
    cluster: Cluster, uri, primary, partition_id, target=None, expect=()
):
    """
    Hand primaryship of `partition_id` to the specified `target`, or to some
    other node if none is named, and return the new primary.

    The optionally specified `expect` names substrings the ex-primary must log,
    given **in the order it logs them**: the capture reads its output forward,
    so asking for an earlier line after a later one has been consumed can never
    match.
    """
    if target is None:
        target = next(node for node in cluster.nodes() if node != primary)
    assert target != primary

    # Sent without waiting for the reply: waiting reads the ex-primary's
    # output forward to the command's completion line, past everything checked
    # below.  The completion is checked last instead, where it falls in the
    # stream.
    primary.transfer_leadership(target, partition_id=partition_id)

    # Primaryship really moved, and to the node we named.
    leader = cluster.last_known_leader
    assert wait_until(lambda: leader.wait_queue_primary(uri) == target, 20)
    assert leader.wait_queue_primary(uri) == target

    for substr in expect:
        assert primary.outputs_substr(substr, timeout=10), substr

    # The ex-primary keeps the queue and its storage, and turns it into a
    # remote one addressed at the new primary.
    assert primary.outputs_substr("converting to remote", timeout=10)

    # The broker logs this once it is done with the command, after everything
    # above, so it is still ahead in the stream.  A rejected transfer logs
    # 'Error processing command' instead and this times out.
    assert primary.outputs_regex(
        r"TRANSFER_LEADERSHIP.*processed successfully", timeout=10
    )

    return target


def test_transfer_partition_leadership(
    fsm_multi_cluster: Cluster, domain_urls: tc.DomainUrls
):
    uri = domain_urls.uri_priority
    primary, partition_id = _place_queue(fsm_multi_cluster, uri)

    # Attach both clients to the primary itself: their traffic then goes
    # through the very queue that converts, which is what this exercises.
    producer = primary.create_client("producer")
    producer.open(uri, flags=["write", "ack"], succeed=True)

    consumer = primary.create_client("consumer")
    consumer.open(uri, flags=["read"], succeed=True)

    producer.post(uri, payload=["before"], wait_ack=True, succeed=True)
    consumer.wait_push_event()
    assert wait_until(lambda: len(consumer.list(uri, block=True)) == 1, 5)

    _move_primary(fsm_multi_cluster, uri, primary, partition_id)

    # The point of the exercise: a PUT the ex-primary accepts after the
    # demotion is buffered and relayed upstream, not NACKed, and comes back
    # as a PUSH to a consumer still attached to that same node.
    producer.post(uri, payload=["after"], wait_ack=True, succeed=True)

    assert consumer.wait_push_event()
    assert wait_until(lambda: len(consumer.list(uri, block=True)) == 2, 10)

    payloads = [msg.payload for msg in consumer.list(uri, block=True)]
    assert payloads == ["before", "after"]

    # Both are confirmable through the converted queue.
    assert consumer.confirm(uri, "*", block=True) == Client.e_SUCCESS
    assert wait_until(lambda: len(consumer.list(uri, block=True)) == 0, 10)


def test_transfer_preserves_consumer_priority(
    fsm_multi_cluster: Cluster, domain_urls: tc.DomainUrls
):
    """
    Two consumers of different priority on the node that gets demoted.  The
    replacement engine builds each App's routing from the handles, so the
    priority selection has to come out the same: the high-priority consumer
    keeps getting everything and the low-priority one keeps getting nothing.
    An engine left with empty routing would fail this by delivering to
    neither.
    """
    uri = domain_urls.uri_priority
    primary, partition_id = _place_queue(fsm_multi_cluster, uri)

    producer = primary.create_client("producer")
    producer.open(uri, flags=["write", "ack"], succeed=True)

    high = primary.create_client("high")
    high.open(uri, flags=["read"], consumer_priority=2, succeed=True)

    low = primary.create_client("low")
    low.open(uri, flags=["read"], consumer_priority=1, succeed=True)

    producer.post(uri, payload=["before"], wait_ack=True, succeed=True)
    assert high.wait_push_event()
    assert wait_until(lambda: len(high.list(uri, block=True)) == 1, 5)
    assert len(low.list(uri, block=True)) == 0

    _move_primary(fsm_multi_cluster, uri, primary, partition_id)

    producer.post(uri, payload=["after"], wait_ack=True, succeed=True)
    assert high.wait_push_event()
    assert wait_until(lambda: len(high.list(uri, block=True)) == 2, 10)

    assert [msg.payload for msg in high.list(uri, block=True)] == [
        "before",
        "after",
    ]
    assert len(low.list(uri, block=True)) == 0


def _relayed_transfer(cluster: Cluster, uri, pick_target):
    """
    Run a leadership transfer with the clients attached to a replica rather
    than to the primary, so the primary's only handles are the cluster-member
    ones that replica opened on it.  `pick_target` chooses the new primary
    given (primary, relay, nodes).
    """
    primary, partition_id = _place_queue(cluster, uri)

    relay = next(node for node in cluster.nodes() if node != primary)

    producer = relay.create_client("producer")
    producer.open(uri, flags=["write", "ack"], succeed=True)

    consumer = relay.create_client("consumer")
    consumer.open(uri, flags=["read"], succeed=True)

    producer.post(uri, payload=["before"], wait_ack=True, succeed=True)
    consumer.wait_push_event()
    assert wait_until(lambda: len(consumer.list(uri, block=True)) == 1, 5)

    target = pick_target(primary, relay, cluster.nodes())

    # The ex-primary lets go of the handles the replica opened on it -- that
    # replica reopens against the new primary itself -- and it does so before
    # converting, so the conversion finds no handles left to rebuild from.
    _move_primary(
        cluster,
        uri,
        primary,
        partition_id,
        target,
        expect=["dropping the handle it opened on"],
    )

    # And therefore it does not reopen the queue upstream on the replica's
    # behalf.  A reopen here would make it a subscriber at the new primary
    # with nothing downstream of it, and double-count the replica's reader.
    assert not primary.outputs_substr("Sent ReopenQueue request", timeout=5)

    producer.post(uri, payload=["after"], wait_ack=True, succeed=True)

    assert consumer.wait_push_event()
    assert wait_until(lambda: len(consumer.list(uri, block=True)) == 2, 10)

    # Exactly two, in order: no message re-appended by way of the ex-primary
    # relaying what the replica also retransmitted.
    payloads = [msg.payload for msg in consumer.list(uri, block=True)]
    assert payloads == ["before", "after"]

    assert consumer.confirm(uri, "*", block=True) == Client.e_SUCCESS
    assert wait_until(lambda: len(consumer.list(uri, block=True)) == 0, 10)


def test_transfer_relayed_traffic_to_third_node(
    fsm_multi_cluster: Cluster, domain_urls: tc.DomainUrls
):
    """
    Clients on a replica, primaryship moved to a node that is neither.  The
    ex-primary leaves the data route entirely and should keep nothing.
    """
    _relayed_transfer(
        fsm_multi_cluster,
        domain_urls.uri_priority,
        lambda primary, relay, nodes: next(
            node for node in nodes if node != primary and node != relay
        ),
    )


def test_transfer_relayed_traffic_to_the_relay(
    fsm_multi_cluster: Cluster, domain_urls: tc.DomainUrls
):
    """
    Clients on a replica, primaryship moved to that same replica.  The
    ex-primary's upstream becomes one of its own downstreams, so anything it
    kept for that downstream would be relayed straight back to it.
    """
    _relayed_transfer(
        fsm_multi_cluster,
        domain_urls.uri_priority,
        lambda primary, relay, nodes: relay,
    )


@tweak.broker.app_config.configure_stream(True)
@tweak.broker.app_config.advertise_subscriptions(True)
def test_transfer_preserves_subscriptions(
    fsm_multi_cluster: Cluster, domain_urls: tc.DomainUrls
):
    """
    A consumer with an expression subscription on the node that gets demoted.
    The replacement engine rebuilds the subscription from the handle and
    advertises it to the new primary, so the same filtering has to hold after
    the move: matching messages are delivered, non-matching ones are not.
    """
    uri = domain_urls.uri_priority
    primary, partition_id = _place_queue(fsm_multi_cluster, uri)

    producer = primary.create_client("producer")
    producer.open(uri, flags=["write", "ack"], succeed=True)

    consumer = primary.create_client("consumer")
    consumer.open(
        uri,
        flags=["read"],
        subscriptions=[{"correlationId": 1, "expression": "x >= 100"}],
        succeed=True,
    )

    # Posted first and filtered out, so its arrival would show up as an extra
    # message ahead of the one that does match.
    producer.post(
        uri,
        payload=["no-before"],
        messageProperties=_int_property("x", 1),
        wait_ack=True,
        succeed=True,
    )
    producer.post(
        uri,
        payload=["yes-before"],
        messageProperties=_int_property("x", 200),
        wait_ack=True,
        succeed=True,
    )

    assert consumer.wait_push_event()
    assert wait_until(lambda: len(consumer.list(uri, block=True)) == 1, 5)
    assert [msg.payload for msg in consumer.list(uri, block=True)] == [
        "yes-before"
    ]

    _move_primary(fsm_multi_cluster, uri, primary, partition_id)

    producer.post(
        uri,
        payload=["no-after"],
        messageProperties=_int_property("x", 2),
        wait_ack=True,
        succeed=True,
    )
    producer.post(
        uri,
        payload=["yes-after"],
        messageProperties=_int_property("x", 300),
        wait_ack=True,
        succeed=True,
    )

    assert consumer.wait_push_event()
    assert wait_until(lambda: len(consumer.list(uri, block=True)) == 2, 10)
    assert [msg.payload for msg in consumer.list(uri, block=True)] == [
        "yes-before",
        "yes-after",
    ]
