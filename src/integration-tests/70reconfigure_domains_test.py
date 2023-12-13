"""
Testing runtime reconfiguration of domains.
"""
import time

import bmq.dev.it.testconstants as tc
from bmq.dev.it.fixtures import (  # pylint: disable=unused-import
    Cluster,
    standard_cluster,
    tweak,
)
from bmq.dev.it.process.client import Client

INITIAL_MSG_QUOTA = 10

URI_PRIORITY_1 = f"bmq://{tc.DOMAIN_PRIORITY}/abcd-queue"
URI_PRIORITY_2 = f"bmq://{tc.DOMAIN_PRIORITY}/qrst-queue"


class TestReconfigureDomains:
    def setup_cluster(self, cluster):
        proxy = next(cluster.proxy_cycle())

        self.writer = proxy.create_client("writers")
        self.writer.open(URI_PRIORITY_1, flags=["write,ack"], succeed=True)
        self.writer.open(URI_PRIORITY_2, flags=["write,ack"], succeed=True)

        self.reader = proxy.create_client("readers")
        self.reader.open(URI_PRIORITY_1, flags=["read"], succeed=True)
        self.reader.open(URI_PRIORITY_2, flags=["read"], succeed=True)

    # Instruct 'writer to 'POST 'n' messages to the domain.
    # Returns 'True' if all of them succeed, and a false-y value otherwise.
    def post_n_msgs(self, uri, n):
        results = (
            self.writer.post(uri, payload=["msg"], wait_ack=True) for _ in range(0, n)
        )
        return all(res == Client.e_SUCCESS for res in results)

    # Helper method which tells 'leader' to reload the domain config.
    def reconfigure_to_n_msgs(self, cluster, num_msgs, leader_only=True):
        return cluster.reconfigure_domain_values(
            tc.DOMAIN_PRIORITY,
            {"storage.domain_limits.messages": num_msgs},
            leader_only=leader_only,
            succeed=True,
        )

    @tweak.domain.storage.domain_limits.messages(INITIAL_MSG_QUOTA)
    def test_reconfigure_domain_message_limits(self, standard_cluster: Cluster):
        assert self.post_n_msgs(URI_PRIORITY_1, INITIAL_MSG_QUOTA)

        # Resource monitor allows exceeding message quota exactly once before
        # beginning to fail ACKs. So we expect 'INITIAL_MSG_QUOTA+1' messages
        # to succeed without error.
        #
        # We post the last message to a different queue to ensure the limit
        # applies throughout the domain, rather than only to the queue.
        assert self.post_n_msgs(URI_PRIORITY_2, 1)

        # Observe that posting once more fails, regardless of which queue.
        assert not self.post_n_msgs(URI_PRIORITY_1, 1)
        assert not self.post_n_msgs(URI_PRIORITY_2, 1)

        # Modify the domain configuration to hold 2 more messages.
        self.reconfigure_to_n_msgs(standard_cluster, INITIAL_MSG_QUOTA + 10)

        # Observe that posting two more messages succeeds.
        assert self.post_n_msgs(URI_PRIORITY_1, 5)
        assert self.post_n_msgs(URI_PRIORITY_2, 5)

        # There are now (once again) INITIAL_MSG_QUOTA+1 messages queued.
        # Observe that no more messages may be posted.
        assert not self.post_n_msgs(URI_PRIORITY_1, 1)
        assert not self.post_n_msgs(URI_PRIORITY_2, 1)

        # Reconfigure limit back down to the initial value.
        self.reconfigure_to_n_msgs(standard_cluster, INITIAL_MSG_QUOTA)

        # Observe that posting continues to fail.
        assert not self.post_n_msgs(URI_PRIORITY_1, 1)
        assert not self.post_n_msgs(URI_PRIORITY_2, 1)

        # Confirm 5 messages from the 'reader'.
        self.reader.confirm(URI_PRIORITY_1, "+10", succeed=True)

        # Observe that posting still fails, since we are still at capacity.
        assert not self.post_n_msgs(URI_PRIORITY_1, 1)
        assert not self.post_n_msgs(URI_PRIORITY_2, 1)

        # Confirm one more message, and observe that posting then succeeds.
        self.reader.confirm(URI_PRIORITY_1, "+1", succeed=True)
        assert self.post_n_msgs(URI_PRIORITY_1, 1)

        # Confirm that we are again at capacity, but that reading a message
        # from one queue unblocks posting on the other.
        assert not self.post_n_msgs(URI_PRIORITY_2, 1)
        self.reader.confirm(URI_PRIORITY_1, "+1", succeed=True)
        assert self.post_n_msgs(URI_PRIORITY_2, 1)

    @tweak.domain.storage.queue_limits.messages(INITIAL_MSG_QUOTA)
    def test_reconfigure_queue_message_limits(self, standard_cluster: Cluster):
        # Resource monitor allows exceeding message quota exactly once before
        # beginning to fail ACKs. So we expect 'INITIAL_MSG_QUOTA+1' messages
        # to succeed without error.
        assert self.post_n_msgs(URI_PRIORITY_1, INITIAL_MSG_QUOTA + 1)

        # Observe that posting once more fails.
        assert not self.post_n_msgs(URI_PRIORITY_1, 1)

        # Do the same for a different queue, and observe that posting succeeds.
        assert self.post_n_msgs(URI_PRIORITY_2, INITIAL_MSG_QUOTA + 1)

        # Again observe that posting once more fails.
        assert not self.post_n_msgs(URI_PRIORITY_2, 1)

        # Modify the domain configuration to hold 2 more messages per queue.
        standard_cluster.reconfigure_domain_values(
            tc.DOMAIN_PRIORITY,
            {"storage.queue_limits.messages": INITIAL_MSG_QUOTA + 1},
            succeed=True,
        )

        # Observe that posting one more message now succeeds for each queue.
        assert self.post_n_msgs(URI_PRIORITY_1, 1)
        assert self.post_n_msgs(URI_PRIORITY_2, 1)

        # Posting one more continues to fail.
        assert not self.post_n_msgs(URI_PRIORITY_1, 1)
        assert not self.post_n_msgs(URI_PRIORITY_2, 1)

    @tweak.domain.storage.domain_limits.messages(1)
    def test_reconfigure_with_leader_change(self, standard_cluster: Cluster):
        leader = standard_cluster.last_known_leader

        # Exhaust the message capacity of the domain.
        assert self.post_n_msgs(URI_PRIORITY_1, 2)
        assert not self.post_n_msgs(URI_PRIORITY_1, 1)

        # Reconfigure every node to accept an additional message.
        self.reconfigure_to_n_msgs(standard_cluster, 3, leader_only=False)

        # Ensure the capacity increased as expected, then confirm one message.
        assert self.post_n_msgs(URI_PRIORITY_1, 2)
        assert not self.post_n_msgs(URI_PRIORITY_1, 1)
        self.reader.confirm(URI_PRIORITY_1, "+1", succeed=True)

        # Take the leader offline.
        leader.check_exit_code = False
        leader.kill()
        leader.wait()

        # Wait for a new leader to be elected.
        standard_cluster.wait_leader()
        assert leader != standard_cluster.last_known_leader
        leader = standard_cluster.last_known_leader

        # Verify that new leader accepts one more message and reaches capacity.
        assert self.post_n_msgs(URI_PRIORITY_1, 1)
        assert not self.post_n_msgs(URI_PRIORITY_1, 1)

    @tweak.domain.max_consumers(1)
    @tweak.domain.max_producers(1)
    def test_reconfigure_max_clients(self, standard_cluster: Cluster):
        proxy = next(standard_cluster.proxy_cycle())

        # Create another client.
        ad_client = proxy.create_client("another-client")

        # Confirm the client cannot open the queue for reading or writing.
        assert ad_client.open(URI_PRIORITY_1, flags=["write"], block=True) != 0
        assert ad_client.open(URI_PRIORITY_2, flags=["read"], block=True) != 0

        # Reconfigure the domain to allow for one more producer to connect.
        standard_cluster.reconfigure_domain_values(
            tc.DOMAIN_PRIORITY,
            {"max_producers": 2},
            leader_only=True,
            succeed=True,
        )

        # Confirm that the queue can be opened for writing, but not reading.
        assert ad_client.open(URI_PRIORITY_1, flags=["write"], block=True) == 0
        assert ad_client.open(URI_PRIORITY_2, flags=["read"], block=True) != 0

        # Reconfigure the domain to allow for one more consumer to connect.
        standard_cluster.reconfigure_domain_values(
            tc.DOMAIN_PRIORITY,
            {"max_consumers": 2},
            leader_only=True,
            succeed=True,
        )

        # Confirm that the queue can be opened for reading.
        assert ad_client.open(URI_PRIORITY_2, flags=["read"], block=True) == 0

        # Confirm that readers and writers are again at capacity.
        ad_client = proxy.create_client("third-client")
        assert ad_client.open(URI_PRIORITY_1, flags=["write"], block=True) != 0
        assert ad_client.open(URI_PRIORITY_2, flags=["read"], block=True) != 0

    @tweak.domain.max_queues(2)
    def test_reconfigure_max_queues(self, standard_cluster: Cluster):
        ad_url_1 = f"{URI_PRIORITY_1}-third-queue"
        ad_url_2 = f"{URI_PRIORITY_1}-fourth-queue"

        # Confirm the client cannot open a third queue.
        assert self.reader.open(ad_url_1, flags=["read"], block=True) != 0

        # Reconfigure the domain to allow for one more producer to connect.
        standard_cluster.reconfigure_domain_values(
            tc.DOMAIN_PRIORITY,
            {"max_queues": 3},
            leader_only=True,
            succeed=True,
        )

        # Confirm that one more queue can be opened.
        assert self.reader.open(ad_url_1, flags=["read"], block=True) == 0

        # No additional queues may be opened.
        assert self.reader.open(ad_url_2, flags=["read"], block=True) != 0

    @tweak.cluster.queue_operations.consumption_monitor_period_ms(500)
    @tweak.domain.max_idle_time(1)
    def test_reconfigure_max_idle_time(self, standard_cluster: Cluster):
        leader = standard_cluster.last_known_leader

        # Configure reader to have at most one outstanding unconfirmed message.
        self.reader.configure(URI_PRIORITY_1, block=True, maxUnconfirmedMessages=1)

        # Write two messages to the queue (only one can be sent to reader).
        assert self.post_n_msgs(URI_PRIORITY_1, 2)

        # Sleep for long enough to trigger an alarm.
        time.sleep(1.5)
        assert leader.alarms("QUEUE_CONSUMER_MONITOR", 1)

        # Confirm all messages in the queue.
        self.reader.confirm(URI_PRIORITY_1, "+2", succeed=True)

        # Reconfigure domain to tolerate as much as two seconds of idleness.
        standard_cluster.reconfigure_domain_values(
            tc.DOMAIN_PRIORITY,
            {"max_idle_time": 2},
            leader_only=True,
            succeed=True,
        )

        # Write two further messages to the queue.
        assert self.post_n_msgs(URI_PRIORITY_1, 2)

        # Sleep for duration between old and new allowed idleness durations.
        time.sleep(1.5)

        # Confirm both messages.
        self.reader.confirm(URI_PRIORITY_1, "+2", succeed=True)

        # Ensure that no alarm was issued.
        assert not leader.alarms("QUEUE_CONSUMER_MONITOR", 1)

    @tweak.domain.message_ttl(1)
    def test_reconfigure_message_ttl(self, standard_cluster: Cluster):
        leader = standard_cluster.last_known_leader

        # Write two messages to the queue (only one can be sent to reader).
        assert self.post_n_msgs(URI_PRIORITY_1, 2)

        # Sleep for long enough to trigger message GC.
        time.sleep(2)

        # Observe that both messages were GC'd from the queue.
        assert leader.erases_messages(URI_PRIORITY_1, msgs=2, timeout=1)

        # Reconfigure the domain to wait 3 seconds before GC'ing messages.
        standard_cluster.reconfigure_domain_values(
            tc.DOMAIN_PRIORITY,
            {"message_ttl": 10},
            leader_only=True,
            succeed=True,
        )

        # Write two further messages to the queue.
        assert self.post_n_msgs(URI_PRIORITY_1, 2)

        # Sleep for the same duration as before.
        time.sleep(2)

        # Observe that no messages were GC'd.
        assert not leader.erases_messages(URI_PRIORITY_1, timeout=1)

        # Verify that the reader can confirm the written messages.
        self.reader.confirm(URI_PRIORITY_1, "+2", succeed=True)

    @tweak.domain.max_delivery_attempts(0)
    def test_reconfigure_max_delivery_attempts(self, standard_cluster: Cluster):
        URI = f"bmq://{tc.DOMAIN_PRIORITY}/my-queue"
        proxy = next(standard_cluster.proxy_cycle())

        # Open the queue through the writer.
        self.writer.open(URI, flags=["write,ack"], succeed=True)

        def do_test(expect_success):
            # Write one message to 'URI'.
            self.post_n_msgs(URI, 1)

            # Open, read, and kill three consumers in sequence.
            for idx in range(0, 5):
                client = proxy.create_client(f"reader-unstable-{idx}")
                client.open(URI, flags=["read"], succeed=True)
                client.check_exit_code = False
                client.wait_push_event()
                client.kill()
                client.wait()

            # Open one more client, and ensure it succeeds or fails to read a
            # message according to 'expect_success'.
            client = proxy.create_client("reader-stable")
            client.open(URI, flags=["read"], succeed=True)
            if expect_success:
                client.confirm(URI, "+1", succeed=True)
            else:
                assert not client.wait_push_event()
            client.stop_session(block=True)

        # Expect that message will not expire after failed deliveries.
        do_test(True)

        # Reconfigure messages to expire after 5 delivery attempts.
        standard_cluster.reconfigure_domain_values(
            tc.DOMAIN_PRIORITY, {"max_delivery_attempts": 5}, succeed=True
        )

        # Expect that message will expire after failed deliveries.
        do_test(False)
