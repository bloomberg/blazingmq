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
Testing poison message detection and handling.
"""

import blazingmq.dev.it.testconstants as tc
from blazingmq.dev.it.fixtures import (
    Cluster,
    order,
    start_cluster,
    tweak,
)
from blazingmq.dev.configurator.configurator import Configurator
from blazingmq.dev.it.util import wait_until

pytestmark = order(5)


def message_throttling(high: int, low: int):
    def tweaker(configurator: Configurator):
        throttle_config = configurator.proto.cluster.message_throttle_config
        assert throttle_config is not None
        throttle_config.high_threshold = high
        throttle_config.low_threshold = low

    return tweak(tweaker)


def max_delivery_attempts(num: int):
    return tweak.domain.max_delivery_attempts(num)


@tweak.cluster.queue_operations.shutdown_timeout_ms(2000)
@tweak.cluster.queue_operations.stop_timeout_ms(2000)
class TestPoisonMessages:
    def _list_messages(self, broker, uri, messages):
        broker.list_messages(uri, tc.TEST_QUEUE, 0, len(messages))
        assert broker.outputs_substr(f"Printing {len(messages)} message(s)", 10)

    def _post_crash_consumers(self, multi_node, proxy, domain, suffixes):
        # We want to make sure a messages aren't redelivered when the rda
        # count reaches zero after a consumer crash. In the case of fanout,
        # the 'suffixes' list will be populated with an app id for each
        # consumer. In the case of priority, 'suffixes' will contain one empty
        # string. We also want to ensure the message is still gone in the event
        # we change leaders. In this method, we will
        # 1. open a producer
        # 2. send a message to the consumer(s)
        # 3. open the consumer(s)
        # 4. kill the consumer(s) and open new consumer(s) with the same app id
        #    to synchronize the check which we do to ensure the message is
        #    still there while other substreams are open.
        # 5. send a different message to the new consumer(s) as a way to
        #    synchronize so we can test whether the first message was
        #    redelivered.
        # 6. check to make sure the second message is present at the
        #    consumer(s) but not the first one
        # 7. force a leader change
        # 8. ensure the first message still isn't present but the second
        #    message is.

        uri = f"bmq://{domain}/{tc.TEST_QUEUE}"
        producer = proxy.create_client("producer")
        producer.open(uri, flags=["write", "ack"], succeed=True)
        producer.post(uri, payload=["1"], succeed=True)

        consumers = []

        for count, suffix in enumerate(suffixes):
            consumer = proxy.create_client(f"consumer_{count}")
            consumer.open(f"{uri}{suffix}", flags=["read"], succeed=True)
            consumers.append(consumer)

        replica = multi_node.process(proxy.get_active_node())
        # The queue's partition primary in Raft mode need not be the CSL leader,
        # and its placement is not datacenter-pinned: it may even coincide with
        # the proxy's active node ('replica').  Discover it explicitly.
        primary = replica.wait_queue_primary(uri)

        # 'LIST' on the proxy only reflects the message once the round-trip PUSH
        # has come back down to it, so synchronize on delivery before listing;
        # otherwise the check races the (slower, in strong-consistency)
        # replication round-trip and finds an empty proxy.
        for consumer in consumers:
            consumer.wait_push_event()

        self._list_messages(proxy, domain, ["1"])
        self._list_messages(replica, domain, ["1"])
        self._list_messages(primary, domain, ["1"])

        new_consumers = []

        for count, consumer in enumerate(consumers):
            consumer.check_exit_code = False
            consumer.kill()

            # start another consumer here with the same app id of the one we've
            # just killed. we will use this consumer to synchronize the
            # previous consumer crash and to help ensure message is removed
            # from the proxy.
            new_consumer = proxy.create_client(f"consumer_{count + len(consumers)}")
            new_consumer.open(f"{uri}{suffixes[count]}", flags=["read"], succeed=True)
            new_consumers.append(new_consumer)

            if count < len(consumers) - 1:
                # should not remove the message if there are other subStreams
                self._list_messages(proxy, domain, ["1"])
                self._list_messages(replica, domain, ["1"])
                self._list_messages(primary, domain, ["1"])

        # post a new message to the apps and verify the old message is gone
        producer.post(uri, payload=["2"], succeed=True)
        for consumer in new_consumers:
            consumer.wait_push_event()
            msgs = consumer.list(block=True)
            assert len(msgs) == 1
            assert msgs[0].payload == "2"

        self._list_messages(proxy, domain, ["2"])
        self._list_messages(replica, domain, ["2"])
        self._list_messages(primary, domain, ["2"])

        # Change the primary and check that the original message ('1') is still
        # gone.  Which node takes over is immaterial to poison handling, so
        # fail over the queue's current primary and discover its replacement.
        primary.stop()

        # Wait for the new primary to become active by opening a queue from a
        # new producer for synchronization.
        producer2 = proxy.create_client("producer2")
        producer2.open(uri, flags=["write", "ack"], succeed=True)

        # Discover the replacement primary from a node that is guaranteed to be
        # alive: 'replica' cannot be reused here because in Raft mode it may be
        # the very node we just stopped (primary co-located with the proxy's
        # active node).
        survivor = multi_node.nodes(alive=True, exclude=primary)[0]
        primary = survivor.wait_queue_primary(uri)
        self._list_messages(proxy, domain, ["2"])
        self._list_messages(primary, domain, ["2"])

        producer.exit_gracefully()
        producer2.exit_gracefully()

        for count, consumer in enumerate(new_consumers):
            consumer.confirm(f"{uri}{suffixes[count]}", "*", True)

    def _wait_live_queue_primary(self, multi_node, uri, timeout=60):
        """
        Return the queue's partition primary once it is a LIVE node.  In Raft
        the primary is per-partition and need not be the CSL leader, so shutting
        down the primary does not change the CSL leader and the partition's
        primary is reassigned asynchronously; the old (dead) primary may still
        be reported briefly.  Poll (querying a live node) until the reported
        primary is up.
        """
        primary = [None]

        def check():
            live = multi_node.nodes(alive=True)[0]
            try:
                # During failover the cluster is briefly unstable, so
                # 'open_admin_client' can transiently refuse; retry rather than
                # propagate.
                candidate = live.wait_queue_primary(uri, timeout=5)
            except Exception:  # pylint: disable=broad-except
                return False
            if candidate is not None and candidate.is_alive():
                primary[0] = candidate
                return True
            return False

        assert wait_until(check, timeout)
        return primary[0]

    def _crash_consumer_restart_primary(
        self, multi_node, domain, make_active_node_primary
    ):
        # We want to make sure the rda counter resets to the value in the
        # configuration after a failover. Since the rda counter is set to
        # two, if we:
        # 1. open a producer
        # 2. send a message to the consumer
        # 3. open a consumer
        # 4. kill a consumer
        # 5. open a consumer
        # 6. force a failover by stopping the queue's partition primary
        # 7. kill a consumer a again
        # 8. open a consumer
        # The message should still exist and be delivered to the consumer (the
        # counter would have been reset when the primary changed).

        uri = f"bmq://{domain}/{tc.TEST_QUEUE}"

        # Resolve the queue's partition primary, then pick the proxy in the data
        # center opposite it so the chain is Client -> proxy -> replica ->
        # primary (the proxy's active node must be a REPLICA, not the primary,
        # otherwise stopping the primary would take the proxy's upstream down).
        # In Raft the primary need not share the CSL leader's data center, so it
        # is resolved dynamically rather than assumed.
        probe = multi_node.last_known_leader.create_client("primary-probe")
        probe.open(uri, flags=["write", "ack"], succeed=True)  # assign the queue
        primary = multi_node.last_known_leader.wait_queue_primary(uri)
        probe.stop_session(block=True)

        proxy = multi_node.proxies(near=primary, invert=True)[0]

        producer = proxy.create_client("producer")
        producer.open(uri, flags=["write", "ack"], succeed=True)
        producer.post(uri, payload=["1"], succeed=True)

        consumer = proxy.create_client("consumer_0")
        consumer.open(f"{uri}", flags=["read"], succeed=True)
        # Synchronize on delivery before the first proxy 'LIST', which otherwise
        # races the round-trip PUSH back down to the proxy.
        consumer.wait_push_event()

        replica = multi_node.process(proxy.get_active_node())

        # 'LIST' must be issued on the queue's partition primary, which in Raft
        # mode need not be the CSL leader (a node that is neither the primary nor
        # has the queue open reports "Queue not found").
        self._list_messages(proxy, domain, ["1"])
        self._list_messages(replica, domain, ["1"])
        self._list_messages(primary, domain, ["1"])

        consumer.check_exit_code = False
        consumer.kill()

        # start new consumer to synchronize with proxy and replica
        consumer = proxy.create_client("consumer_1")
        consumer.open(f"{uri}", flags=["read"], succeed=True)
        consumer.wait_push_event()

        # make sure the message is still present
        self._list_messages(proxy, domain, ["1"])
        self._list_messages(replica, domain, ["1"])
        self._list_messages(primary, domain, ["1"])

        # Force a failover so the RDA counter resets.  Stopping the queue's
        # partition primary makes a replacement take over and re-push the
        # outstanding message downstream with a fresh RDA (the counter is
        # in-memory delivery state, not persisted).
        #
        # 'make_active_node_primary' selects whether the proxy's active node
        # ('replica') becomes the replacement primary.  In legacy this is
        # steerable via the partition-election quorum, and it matters: the RDA
        # only resets when the proxy re-syncs from a *different* primary, so the
        # non-active case must keep the active node from taking over.  In
        # FSM/Raft the quorum has no effect (any primary change rebuilds delivery
        # state and resets the RDA regardless), so the calls are harmless there.
        if make_active_node_primary:
            # make the active node the new primary
            for node in multi_node.nodes(exclude=replica):
                node.set_quorum(4)
        else:
            # prevent the active node from becoming the new primary
            replica.set_quorum(4)

        primary.stop()
        primary = self._wait_live_queue_primary(multi_node, uri)

        consumer.check_exit_code = False
        consumer.kill()

        # Start a new consumer; opening it (with 'succeed') and awaiting its
        # PUSH synchronizes on the replacement primary being active.  The
        # message must still be delivered because the failover reset its RDA
        # counter.
        consumer = proxy.create_client("consumer_2")
        consumer.open(f"{uri}", flags=["read"], succeed=True)
        consumer.wait_push_event()

        self._list_messages(proxy, domain, ["1"])
        self._list_messages(primary, domain, ["1"])

        producer.exit_gracefully()
        consumer.confirm(f"{uri}", "*", True)

    def _crash_one_consumer(self, multi_node, proxy, domain, suffixes):
        # We want to make sure when the rda counter reaches 0 for app #1 while
        # the other apps (#2 and #3) haven't been confirmed, the message
        # doesn't get redelivered for app #1. In this method, we will:
        # 1. open a producer
        # 2. send a message to the consumers
        # 3. open three consumers all with different app ids
        # 4. choose a consumer to kill twice
        # 5. bring back the consumer which was killed
        # 6. confirm the messages on the other consumers
        # 7. send a different message to the consumers as a way to synchronize
        #    so we can test that the first message won't be redelivered to our
        #    originally crashed app.
        # 7. make sure the first message wasn't delivered to our crashed app
        # 8. make sure the first message is gone from everywhere

        uri = f"bmq://{domain}/{tc.TEST_QUEUE}"
        producer = proxy.create_client("producer")
        producer.open(uri, flags=["write", "ack"], succeed=True)
        producer.post(uri, payload=["1"], succeed=True)

        consumers = []

        for count, suffix in enumerate(suffixes):
            consumer = proxy.create_client(f"consumer_{count}")
            consumer.open(f"{uri}{suffix}", flags=["read"], succeed=True)
            consumers.append(consumer)

        # kill one of the consumers twice
        consumers[0].check_exit_code = False
        consumers[0].kill()

        consumers[0] = proxy.create_client(f"consumer_{0}")
        consumers[0].open(f"{uri}{suffixes[0]}", flags=["read"], succeed=True)
        consumers[0].wait_push_event()

        # ensure the message still got delivered
        assert len(consumers[0].list(f"{uri}{suffixes[0]}", True)) == 1

        consumers[0].check_exit_code = False
        consumers[0].kill()

        # confirm the message for the other consumers and bring back the
        # crashed consumer
        for count, consumer in enumerate(consumers):
            if count == 0:
                consumers[count] = proxy.create_client(f"consumer_{count}")
                consumers[count].open(
                    f"{uri}{suffixes[count]}", flags=["read"], succeed=True
                )
            else:
                consumer.confirm(f"{uri}{suffixes[count]}", "*", True)

        # Send another message, have any one of the consumers
        producer.post(uri, payload=["2"], succeed=True)

        # make sure the consumer that crashed twice doesn't get a redelivery
        # for the first message
        consumers[0].wait_push_event()
        assert len(consumers[0].list(f"{uri}{suffixes[0]}", True)) == 1

        # make sure after the confirms, the first message is gone from
        # everywhere.  'LIST' must target the queue's partition primary, which
        # in Raft need not be the CSL leader.
        primary = multi_node.last_known_leader.wait_queue_primary(uri)
        self._list_messages(proxy, domain, ["2"])
        self._list_messages(primary, domain, ["2"])

        producer.exit_gracefully()
        for count, consumer in enumerate(consumers):
            consumer.confirm(f"{uri}{suffixes[count]}", "*", True)

    def _stop_consumer_gracefully(self, multi_node, proxy, domain):
        # We want to make sure the rda counter isn't decremented when a
        # consumer is shut down gracefully. To test this, we set the rda
        # counter to 1 and:
        # 1. open a producer
        # 2. send a message to the consumer
        # 3. open a consumer, wait for the message, then stop it gracefully
        # 4. open a new consumer, wait for the message to be resent
        # 5. make sure the message is present everywhere

        uri = f"bmq://{domain}/{tc.TEST_QUEUE}"
        producer = proxy.create_client("producer")
        producer.open(uri, flags=["write", "ack"], succeed=True)
        producer.post(uri, payload=["1"], succeed=True)

        consumer = proxy.create_client("consumer_0")
        consumer.open(f"{uri}", flags=["read"], succeed=True)
        consumer.wait_push_event()
        consumer.exit_gracefully()

        consumer = proxy.create_client("consumer_1")
        consumer.open(f"{uri}", flags=["read"], succeed=True)
        consumer.wait_push_event()

        # 'LIST' must target the queue's partition primary, which in Raft need
        # not be the CSL leader (a node that is neither the primary nor has the
        # queue open reports "Queue not found").
        primary = multi_node.last_known_leader.wait_queue_primary(uri)

        self._list_messages(proxy, domain, ["1"])
        self._list_messages(primary, domain, ["1"])
        consumer.confirm(f"{uri}", "*", True)

    def _crash_consumer_connected_to_replica(self, multi_node, proxy, domain):
        # We want to make sure when a consumer on a replica node crashes and the
        # reject message propagates to the primary, when a new consumer appears
        # on the same replica, the updated rda bubbles down from the primary to
        # the replica (we need to check for this since the virtual storage is
        # deleted on the replica once the last consumer is gone).
        # 1. open a producer
        # 2. find a replica and open a consumer on it
        # 3. send a message to the consumer
        # 4. verify the message was received by the consumer
        # 5. kill the consumer
        # 6. open two consumers on that same replica
        # 7. verify the message was received by one of the consumers
        # 8. kill the consumer that received the redelivery
        # 9. send a second message to the remaining consumer as a way to
        #    synchronize so we can test that the first message won't be
        #    redelivered.
        uri = f"bmq://{domain}/{tc.TEST_QUEUE}"
        producer = proxy.create_client("producer")
        producer.open(uri, flags=["write", "ack"], succeed=True)  # assign the queue
        producer.post(uri, payload=["1"], succeed=True)

        # Host the consumer on a REPLICA of the queue's partition, i.e. a node
        # that is not its primary.  In Raft the primary need not be the CSL
        # leader, so exclude the resolved primary rather than the leader.
        primary = multi_node.last_known_leader.wait_queue_primary(uri)
        potential_replicas = multi_node.nodes(exclude=primary)

        assert potential_replicas

        replica = potential_replicas[0]

        consumer_0 = replica.create_client("consumer_0")
        consumer_0.open(f"{uri}", flags=["read"], succeed=True)
        consumer_0.wait_push_event()
        consumer_0.check_exit_code = False
        consumer_0.kill()

        consumer_0 = replica.create_client("consumer_0")
        consumer_0.open(f"{uri}", flags=["read"], succeed=True)
        consumer_0.wait_push_event()
        consumer_1 = replica.create_client("consumer_1")
        consumer_1.open(f"{uri}", flags=["read"], succeed=True)
        consumer_0.check_exit_code = False
        consumer_0.kill()
        producer.post(uri, payload=["2"], succeed=True)
        consumer_1.wait_push_event()

        msgs = consumer_1.list(block=True)
        assert len(msgs) == 1
        assert msgs[0].payload == "2"
        consumer_1.confirm(f"{uri}", "*", True)

    def _stop_proxy(self, multi_node, proxy, domain, should_kill):
        # We want to make sure when a broker either crashes or exits
        # gracefully, outstanding messages from that broker's downstream aren't
        # rejected. To test this, we will set the rda to 1 and:
        # 1. open a producer and consumer connected to a broker (shouldn't be a
        #    leader)
        # 2. send a message to the consumer
        # 3. verify the message was received by the consumer and both brokers
        # 4. either kill or stop the non-leader broker gracefully
        # 5. open a new consumer on the leader broker
        # 6. verify the message was received by the new consumer
        # 7. verify the message is still in the leader broker
        uri = f"bmq://{domain}/{tc.TEST_QUEUE}"
        producer = proxy.create_client("producer")
        producer.open(uri, flags=["write", "ack"], succeed=True)
        producer.post(uri, payload=["1"], succeed=True)

        consumer_0 = proxy.create_client("consumer_0")
        consumer_0.open(f"{uri}", flags=["read"], succeed=True)
        consumer_0.wait_push_event()

        leader = multi_node.last_known_leader
        # 'LIST' must target the queue's partition primary, which in Raft need
        # not be the CSL leader.
        primary = leader.wait_queue_primary(uri)

        msgs = consumer_0.list(block=True)
        assert len(msgs) == 1
        assert msgs[0].payload == "1"
        self._list_messages(proxy, domain, ["1"])
        self._list_messages(primary, domain, ["1"])

        if should_kill:
            proxy.check_exit_code = False
            proxy.kill()
        else:
            proxy.stop()

        consumer_1 = leader.create_client("consumer_1")
        consumer_1.open(f"{uri}", flags=["read"], succeed=True)
        consumer_1.wait_push_event()
        msgs = consumer_1.list(block=True)
        assert len(msgs) == 1
        assert msgs[0].payload == "1"
        self._list_messages(primary, domain, ["1"])

        consumer_0.confirm(f"{uri}", "*", True)
        consumer_1.confirm(f"{uri}", "*", True)

    @max_delivery_attempts(1)
    @message_throttling(high=0, low=0)
    def test_poison_proxy_and_replica_priority(
        self, multi_node: Cluster, domain_urls: tc.DomainUrls
    ):
        proxies = multi_node.proxy_cycle()
        # pick proxy in datacenter opposite to the primary's
        next(proxies)
        proxy = next(proxies)
        self._post_crash_consumers(multi_node, proxy, domain_urls.domain_priority, [""])

    @max_delivery_attempts(1)
    @message_throttling(high=0, low=0)
    def test_poison_proxy_and_replica_fanout(
        self, multi_node: Cluster, domain_urls: tc.DomainUrls
    ):
        proxies = multi_node.proxy_cycle()
        # pick proxy in datacenter opposite to the primary's
        next(proxies)
        proxy = next(proxies)
        self._post_crash_consumers(
            multi_node,
            proxy,
            domain_urls.domain_fanout,
            ["?id=foo", "?id=bar", "?id=baz"],
        )

    @max_delivery_attempts(3)
    def test_poison_rda_reset_priority_active(
        self, multi_node: Cluster, domain_urls: tc.DomainUrls
    ):
        # when set to true, make the proxy's active node the new primary
        self._crash_consumer_restart_primary(
            multi_node, domain_urls.domain_priority, True
        )

    @max_delivery_attempts(2)
    @message_throttling(high=1, low=0)
    @start_cluster(True, True, True)
    def test_poison_rda_reset_priority_non_active(
        self, multi_node: Cluster, domain_urls: tc.DomainUrls
    ):
        self._crash_consumer_restart_primary(
            multi_node, domain_urls.domain_priority, False
        )

    @max_delivery_attempts(2)
    @message_throttling(high=1, low=0)
    def test_poison_fanout_crash_one_app(
        self, multi_node: Cluster, domain_urls: tc.DomainUrls
    ):
        proxies = multi_node.proxy_cycle()
        # pick proxy in datacenter opposite to the primary's
        next(proxies)
        proxy = next(proxies)
        self._crash_one_consumer(
            multi_node,
            proxy,
            domain_urls.domain_fanout,
            ["?id=foo", "?id=bar", "?id=baz"],
        )

    @max_delivery_attempts(1)
    @message_throttling(high=0, low=0)
    def test_poison_consumer_graceful_shutdown(
        self, multi_node: Cluster, domain_urls: tc.DomainUrls
    ):
        proxies = multi_node.proxy_cycle()
        # pick proxy in datacenter opposite to the primary's
        next(proxies)
        proxy = next(proxies)
        self._stop_consumer_gracefully(multi_node, proxy, domain_urls.domain_priority)

    @max_delivery_attempts(2)
    @message_throttling(high=1, low=0)
    def test_poison_replica_receives_updated_rda(
        self, multi_node: Cluster, domain_urls: tc.DomainUrls
    ):
        proxies = multi_node.proxy_cycle()
        next(proxies)
        proxy = next(proxies)
        self._crash_consumer_connected_to_replica(
            multi_node, proxy, domain_urls.domain_priority
        )

    @max_delivery_attempts(1)
    @message_throttling(high=0, low=0)
    def test_poison_no_reject_broker_crash(
        self, multi_node: Cluster, domain_urls: tc.DomainUrls
    ):
        proxies = multi_node.proxy_cycle()
        next(proxies)
        proxy = next(proxies)
        self._stop_proxy(multi_node, proxy, domain_urls.domain_priority, True)

    @max_delivery_attempts(1)
    @message_throttling(high=0, low=0)
    def test_poison_no_reject_broker_graceful_shutdown(
        self, multi_node: Cluster, domain_urls: tc.DomainUrls
    ):
        proxies = multi_node.proxy_cycle()
        next(proxies)
        proxy = next(proxies)
        self._stop_proxy(multi_node, proxy, domain_urls.domain_priority, False)
