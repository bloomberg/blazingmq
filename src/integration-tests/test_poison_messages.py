"""
Testing poison message detection and handling.
"""

import blazingmq.dev.it.testconstants as tc
from blazingmq.dev.it.fixtures import (  # pylint: disable=unused-import
    Cluster,
    order,
    multi_node,
    start_cluster,
    tweak,
)
from blazingmq.dev.configurator.configurator import Configurator

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
        leader = multi_node.last_known_leader

        self._list_messages(proxy, domain, ["1"])
        self._list_messages(replica, domain, ["1"])
        self._list_messages(leader, domain, ["1"])

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
                self._list_messages(leader, domain, ["1"])

        # post a new message to the apps and verify the old message is gone
        producer.post(uri, payload=["2"], succeed=True)
        for consumer in new_consumers:
            consumer.wait_push_event()
            msgs = consumer.list(block=True)
            assert len(msgs) == 1
            assert msgs[0].payload == "2"

        self._list_messages(proxy, domain, ["2"])
        self._list_messages(replica, domain, ["2"])
        self._list_messages(leader, domain, ["2"])

        # change the leader and check if the original message ('1') is still
        # gone
        replica.set_quorum(1)
        nodes = multi_node.nodes(exclude=[replica, leader])
        for node in nodes:
            node.set_quorum(4)
        leader.stop()
        leader = replica
        assert leader == multi_node.wait_leader()

        # Wait for new leader to become active primary by opening a queue from
        # a new producer for synchronization.
        producer2 = proxy.create_client("producer2")
        producer2.open(uri, flags=["write", "ack"], succeed=True)

        self._list_messages(proxy, domain, ["2"])
        self._list_messages(leader, domain, ["2"])

        producer.exit_gracefully()
        producer2.exit_gracefully()

        for count, consumer in enumerate(new_consumers):
            consumer.confirm(f"{uri}{suffixes[count]}", "*", True)

    def _crash_consumer_restart_leader(
        self, multi_node, proxy, domain, make_active_node_leader
    ):
        # We want to make sure the rda counter resets to the value in the
        # configuration after losing a leader. Since the rda counter is set to
        # two, if we:
        # 1. open a producer
        # 2. send a message to the consumer
        # 3. open a consumer
        # 4. kill a consumer
        # 5. force a leader change
        # 6. open a consumer
        # 7. kill a consumer a again
        # 8. open a consumer
        # The message should still exist and be delivered to the consumer (the
        # counter would have been reset when the leader changed).

        uri = f"bmq://{domain}/{tc.TEST_QUEUE}"
        producer = proxy.create_client("producer")
        producer.open(uri, flags=["write", "ack"], succeed=True)
        producer.post(uri, payload=["1"], succeed=True)

        consumer = proxy.create_client("consumer_0")
        consumer.open(f"{uri}", flags=["read"], succeed=True)

        replica = multi_node.process(proxy.get_active_node())
        leader = multi_node.last_known_leader

        self._list_messages(proxy, domain, ["1"])
        self._list_messages(replica, domain, ["1"])
        self._list_messages(leader, domain, ["1"])

        consumer.check_exit_code = False
        consumer.kill()

        # start new consumer to synchronize with proxy and replica
        consumer = proxy.create_client("consumer_1")
        consumer.open(f"{uri}", flags=["read"], succeed=True)
        consumer.wait_push_event()

        # make sure the message is still present
        self._list_messages(proxy, domain, ["1"])
        self._list_messages(replica, domain, ["1"])
        self._list_messages(leader, domain, ["1"])

        if make_active_node_leader:
            # make the active replica the new leader
            replica.set_quorum(1)
            nodes = multi_node.nodes(exclude=replica)
            for node in nodes:
                node.set_quorum(4)
            leader.stop()
            leader = replica
        else:
            # make a replica that isn't the active node the new leader
            nodes = multi_node.nodes(exclude=[replica, leader])
            assert nodes
            leader_candidate = nodes.pop()
            leader_candidate.set_quorum(1)
            replica.set_quorum(4)
            for node in nodes:
                node.set_quorum(4)
            leader.stop()
            leader = leader_candidate
        assert leader == multi_node.wait_leader()

        consumer.check_exit_code = False
        consumer.kill()

        # start new consumer to synchronize with proxy and replica
        consumer = proxy.create_client("consumer_2")
        consumer.open(f"{uri}", flags=["read"], succeed=True)
        consumer.wait_push_event()

        self._list_messages(replica, domain, ["1"])
        self._list_messages(leader, domain, ["1"])

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
        # everywhere
        leader = multi_node.last_known_leader
        self._list_messages(proxy, domain, ["2"])
        self._list_messages(leader, domain, ["2"])

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

        leader = multi_node.last_known_leader

        self._list_messages(proxy, domain, ["1"])
        self._list_messages(leader, domain, ["1"])
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
        leader = multi_node.last_known_leader
        potential_replicas = multi_node.nodes(exclude=leader)

        assert potential_replicas

        replica = potential_replicas[0]
        producer = proxy.create_client("producer")
        producer.open(uri, flags=["write", "ack"], succeed=True)
        producer.post(uri, payload=["1"], succeed=True)

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

        leader = multi_node.last_known_leader

        msgs = consumer_0.list(block=True)
        assert len(msgs) == 1
        assert msgs[0].payload == "1"
        self._list_messages(proxy, domain, ["1"])
        self._list_messages(leader, domain, ["1"])

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
        self._list_messages(leader, domain, ["1"])

        consumer_0.confirm(f"{uri}", "*", True)
        consumer_1.confirm(f"{uri}", "*", True)

    @max_delivery_attempts(1)
    @message_throttling(high=0, low=0)
    def test_poison_proxy_and_replica_priority(self, multi_node: Cluster):
        proxies = multi_node.proxy_cycle()
        # pick proxy in datacenter opposite to the primary's
        next(proxies)
        proxy = next(proxies)
        self._post_crash_consumers(multi_node, proxy, tc.DOMAIN_PRIORITY, [""])

    @max_delivery_attempts(1)
    @message_throttling(high=0, low=0)
    def test_poison_proxy_and_replica_fanout(self, multi_node: Cluster):
        proxies = multi_node.proxy_cycle()
        # pick proxy in datacenter opposite to the primary's
        next(proxies)
        proxy = next(proxies)
        self._post_crash_consumers(
            multi_node, proxy, tc.DOMAIN_FANOUT, ["?id=foo", "?id=bar", "?id=baz"]
        )

    @max_delivery_attempts(2)
    def test_poison_rda_reset_priority_active(self, multi_node: Cluster):
        proxies = multi_node.proxy_cycle()
        # pick proxy in datacenter opposite to the primary's
        next(proxies)
        proxy = next(proxies)
        self._crash_consumer_restart_leader(
            multi_node, proxy, tc.DOMAIN_PRIORITY, True
        )  # when set to true, make the
        # active node of the proxy
        # the new leader

    @max_delivery_attempts(2)
    @message_throttling(high=1, low=0)
    @start_cluster(True, True, True)
    def test_poison_rda_reset_priority_non_active(self, multi_node: Cluster):
        proxies = multi_node.proxy_cycle()
        # pick proxy in datacenter opposite to the primary's
        next(proxies)
        proxy = next(proxies)
        self._crash_consumer_restart_leader(
            multi_node, proxy, tc.DOMAIN_PRIORITY, False
        )

    @max_delivery_attempts(2)
    @message_throttling(high=1, low=0)
    def test_poison_fanout_crash_one_app(self, multi_node: Cluster):
        proxies = multi_node.proxy_cycle()
        # pick proxy in datacenter opposite to the primary's
        next(proxies)
        proxy = next(proxies)
        self._crash_one_consumer(
            multi_node, proxy, tc.DOMAIN_FANOUT, ["?id=foo", "?id=bar", "?id=baz"]
        )

    @max_delivery_attempts(1)
    @message_throttling(high=0, low=0)
    def test_poison_consumer_graceful_shutdown(self, multi_node: Cluster):
        proxies = multi_node.proxy_cycle()
        # pick proxy in datacenter opposite to the primary's
        next(proxies)
        proxy = next(proxies)
        self._stop_consumer_gracefully(multi_node, proxy, tc.DOMAIN_PRIORITY)

    @max_delivery_attempts(2)
    @message_throttling(high=1, low=0)
    def test_poison_replica_receives_updated_rda(self, multi_node: Cluster):
        proxies = multi_node.proxy_cycle()
        next(proxies)
        proxy = next(proxies)
        self._crash_consumer_connected_to_replica(
            multi_node, proxy, tc.DOMAIN_PRIORITY
        )

    @max_delivery_attempts(1)
    @message_throttling(high=0, low=0)
    def test_poison_no_reject_broker_crash(self, multi_node: Cluster):
        proxies = multi_node.proxy_cycle()
        next(proxies)
        proxy = next(proxies)
        self._stop_proxy(multi_node, proxy, tc.DOMAIN_PRIORITY, True)

    @max_delivery_attempts(1)
    @message_throttling(high=0, low=0)
    def test_poison_no_reject_broker_graceful_shutdown(self, multi_node: Cluster):
        proxies = multi_node.proxy_cycle()
        next(proxies)
        proxy = next(proxies)
        self._stop_proxy(multi_node, proxy, tc.DOMAIN_PRIORITY, False)
