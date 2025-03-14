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
Testing runtime reconfiguration of domains.
"""
import time
from typing import Optional

import blazingmq.dev.it.testconstants as tc
from blazingmq.dev.it.fixtures import (  # pylint: disable=unused-import
    Cluster,
    order,
    multi_node,
    tweak,
)
from blazingmq.dev.it.process.admin import AdminClient
from blazingmq.dev.it.process.client import Client

pytestmark = order(6)

INITIAL_MSG_QUOTA = 10


class TestReconfigureDomains:
    def setup_cluster(self, cluster: Cluster, domain_urls: tc.DomainUrls):
        du = domain_urls
        proxy = next(cluster.proxy_cycle())

        uri_priority_1 = f"bmq://{du.domain_priority}/abcd-queue"
        uri_priority_2 = f"bmq://{du.domain_priority}/qrst-queue"

        self.writer = proxy.create_client("writers")
        self.writer.open(uri_priority_1, flags=["write,ack"], succeed=True)
        self.writer.open(uri_priority_2, flags=["write,ack"], succeed=True)

        self.reader = proxy.create_client("readers")
        self.reader.open(uri_priority_1, flags=["read"], succeed=True)
        self.reader.open(uri_priority_2, flags=["read"], succeed=True)

    # Instruct writer to POST `n` messages to the `uri`.  Return true if all of
    # them succeed, else a false-y value.
    def post_n_msgs(self, uri, n):
        results = (
            self.writer.post(uri, payload=[f"msg{i}"], wait_ack=True)
            for i in range(0, n)
        )
        return all(res == Client.e_SUCCESS for res in results)

    # Instruct the leader to reconfigure domain config in `cluster` to have a
    # limit of `max_num_msgs`.  If 'leader_only' is true, the 'DOMAINS
    # RECONFIGURE' command will only be issued to the leader.  Return true if
    # reconfigure succeeds, else false.
    def reconfigure_to_limit_n_msgs(
        self, cluster: Cluster, max_num_msgs: int, domain_urls: tc.DomainUrls
    ) -> bool:
        cluster.config.domains[
            domain_urls.domain_priority
        ].definition.parameters.storage.domain_limits.messages = max_num_msgs
        return cluster.reconfigure_domain(
            # TODO: Set `leader_only=True` when admin command routing is re-enabled
            domain_urls.domain_priority,
            leader_only=False,
            succeed=True,
        )

    # Verify that reconfiguring domain message limits works as expected.
    @tweak.domain.storage.domain_limits.messages(INITIAL_MSG_QUOTA)
    def test_reconfigure_domain_message_limits(
        self, multi_node: Cluster, domain_urls: tc.DomainUrls
    ):
        uri_priority_1 = f"bmq://{domain_urls.domain_priority}/abcd-queue"
        uri_priority_2 = f"bmq://{domain_urls.domain_priority}/qrst-queue"

        assert self.post_n_msgs(uri_priority_1, INITIAL_MSG_QUOTA)

        # Resource monitor allows exceeding message quota exactly once before
        # beginning to fail ACKs. So we expect 'INITIAL_MSG_QUOTA+1' messages
        # to succeed without error.
        #
        # We post the last message to a different queue to ensure the limit
        # applies throughout the domain, rather than only to the queue.
        assert self.post_n_msgs(uri_priority_2, 1)

        # Observe that posting once more fails, regardless of which queue.
        assert not self.post_n_msgs(uri_priority_1, 1)
        assert not self.post_n_msgs(uri_priority_2, 1)

        # Modify the domain configuration to hold 10 more messages.
        self.reconfigure_to_limit_n_msgs(
            multi_node, INITIAL_MSG_QUOTA + 10, domain_urls
        )

        # Observe that posting two more messages succeeds.
        assert self.post_n_msgs(uri_priority_1, 5)
        assert self.post_n_msgs(uri_priority_2, 5)

        # There are now (once again) INITIAL_MSG_QUOTA+1 messages queued.
        # Observe that no more messages may be posted.
        assert not self.post_n_msgs(uri_priority_1, 1)
        assert not self.post_n_msgs(uri_priority_2, 1)

        # Reconfigure limit back down to the initial value.
        self.reconfigure_to_limit_n_msgs(multi_node, INITIAL_MSG_QUOTA, domain_urls)

        # Observe that posting continues to fail.
        assert not self.post_n_msgs(uri_priority_1, 1)
        assert not self.post_n_msgs(uri_priority_2, 1)

        # Confirm 10 messages from the 'reader'.
        self.reader.confirm(uri_priority_1, "+10", succeed=True)

        # Observe that posting still fails, since we are still at capacity.
        assert not self.post_n_msgs(uri_priority_1, 1)
        assert not self.post_n_msgs(uri_priority_2, 1)

        # Confirm one more message, and observe that posting then succeeds.
        self.reader.confirm(uri_priority_1, "+1", succeed=True)
        assert self.post_n_msgs(uri_priority_1, 1)

        # Confirm that we are again at capacity, but that reading a message
        # from one queue unblocks posting on the other.
        assert not self.post_n_msgs(uri_priority_2, 1)
        self.reader.confirm(uri_priority_1, "+1", succeed=True)
        assert self.post_n_msgs(uri_priority_2, 1)

    # Verify that reconfiguring queue message limits works as expected.
    @tweak.domain.storage.queue_limits.messages(INITIAL_MSG_QUOTA)
    def test_reconfigure_queue_message_limits(
        self, multi_node: Cluster, domain_urls: tc.DomainUrls
    ):
        uri_priority_1 = f"bmq://{domain_urls.domain_priority}/abcd-queue"
        uri_priority_2 = f"bmq://{domain_urls.domain_priority}/qrst-queue"

        # Resource monitor allows exceeding message quota exactly once before
        # beginning to fail ACKs. So we expect 'INITIAL_MSG_QUOTA+1' messages
        # to succeed without error.
        assert self.post_n_msgs(uri_priority_1, INITIAL_MSG_QUOTA + 1)

        # Observe that posting once more fails.
        assert not self.post_n_msgs(uri_priority_1, 1)

        # Do the same for a different queue, and observe that posting succeeds.
        assert self.post_n_msgs(uri_priority_2, INITIAL_MSG_QUOTA + 1)

        # Again observe that posting once more fails.
        assert not self.post_n_msgs(uri_priority_2, 1)

        # Modify the domain configuration to hold 1 more message per queue.
        multi_node.config.domains[
            domain_urls.domain_priority
        ].definition.parameters.storage.queue_limits.messages = (INITIAL_MSG_QUOTA + 1)
        multi_node.reconfigure_domain(domain_urls.domain_priority, succeed=True)

        # Observe that posting one more message now succeeds for each queue.
        assert self.post_n_msgs(uri_priority_1, 1)
        assert self.post_n_msgs(uri_priority_2, 1)

        # Posting one more continues to fail.
        assert not self.post_n_msgs(uri_priority_1, 1)
        assert not self.post_n_msgs(uri_priority_2, 1)

    # Verify that domain reconfiguration persists after leader change.
    @tweak.domain.storage.domain_limits.messages(1)
    def test_reconfigure_with_leader_change(
        self, multi_node: Cluster, domain_urls: tc.DomainUrls
    ):
        uri_priority_1 = f"bmq://{domain_urls.domain_priority}/abcd-queue"

        leader = multi_node.last_known_leader

        # Exhaust the message capacity of the domain.
        assert self.post_n_msgs(uri_priority_1, 2)
        assert not self.post_n_msgs(uri_priority_1, 1)

        # Reconfigure every node to accept an additional message.
        self.reconfigure_to_limit_n_msgs(multi_node, 2, domain_urls)

        # Ensure the capacity increased as expected, then confirm one message.
        assert self.post_n_msgs(uri_priority_1, 1)
        assert not self.post_n_msgs(uri_priority_1, 1)
        self.reader.confirm(uri_priority_1, "+1", succeed=True)

        # Take the leader offline.
        leader.check_exit_code = False
        leader.kill()
        leader.wait()

        # Wait for a new leader to be elected.
        multi_node.wait_leader()
        assert leader != multi_node.last_known_leader
        leader = multi_node.last_known_leader

        # Verify that new leader accepts one more message and reaches capacity.
        assert self.post_n_msgs(uri_priority_1, 1)
        assert not self.post_n_msgs(uri_priority_1, 1)

    @tweak.domain.max_consumers(1)
    @tweak.domain.max_producers(1)
    def test_reconfigure_max_clients(
        self, multi_node: Cluster, domain_urls: tc.DomainUrls
    ):
        proxy = next(multi_node.proxy_cycle())

        domain_priority = domain_urls.domain_priority
        uri_priority_1 = f"bmq://{domain_priority}/abcd-queue"
        uri_priority_2 = f"bmq://{domain_priority}/qrst-queue"

        # Create another client.
        ad_client = proxy.create_client("another-client")

        # Confirm the client cannot open the queue for reading or writing.
        assert ad_client.open(uri_priority_1, flags=["write"], block=True) != 0
        assert ad_client.open(uri_priority_2, flags=["read"], block=True) != 0

        # Reconfigure the domain to allow for one more producer to connect.
        multi_node.config.domains[
            domain_priority
        ].definition.parameters.max_producers = 2
        multi_node.reconfigure_domain(domain_priority, leader_only=True, succeed=True)

        # Confirm that the queue can be opened for writing, but not reading.
        assert ad_client.open(uri_priority_1, flags=["write"], block=True) == 0
        assert ad_client.open(uri_priority_2, flags=["read"], block=True) != 0

        # Reconfigure the domain to allow for one more consumer to connect.
        multi_node.config.domains[
            domain_priority
        ].definition.parameters.max_consumers = 2
        multi_node.reconfigure_domain(domain_priority, leader_only=True, succeed=True)

        # Confirm that the queue can be opened for reading.
        assert ad_client.open(uri_priority_2, flags=["read"], block=True) == 0

        # Confirm that readers and writers are again at capacity.
        ad_client = proxy.create_client("third-client")
        assert ad_client.open(uri_priority_1, flags=["write"], block=True) != 0
        assert ad_client.open(uri_priority_2, flags=["read"], block=True) != 0

    @tweak.domain.max_queues(2)
    def test_reconfigure_max_queues(
        self, multi_node: Cluster, domain_urls: tc.DomainUrls
    ):
        uri_priority_1 = f"bmq://{domain_urls.domain_priority}/abcd-queue"

        ad_url_1 = f"{uri_priority_1}-third-queue"
        ad_url_2 = f"{uri_priority_1}-fourth-queue"

        # Confirm the client cannot open a third queue.
        assert self.reader.open(ad_url_1, flags=["read"], block=True) != 0

        # Reconfigure the domain to allow for one more producer to connect.
        multi_node.config.domains[
            domain_urls.domain_priority
        ].definition.parameters.max_queues = 3
        multi_node.reconfigure_domain(
            domain_urls.domain_priority, leader_only=True, succeed=True
        )

        # Confirm that one more queue can be opened.
        assert self.reader.open(ad_url_1, flags=["read"], block=True) == 0

        # No additional queues may be opened.
        assert self.reader.open(ad_url_2, flags=["read"], block=True) != 0

    @tweak.cluster.queue_operations.consumption_monitor_period_ms(500)
    @tweak.domain.max_idle_time(1)
    def test_reconfigure_max_idle_time(
        self, multi_node: Cluster, domain_urls: tc.DomainUrls
    ):
        uri_priority_1 = f"bmq://{domain_urls.domain_priority}/abcd-queue"
        leader = multi_node.last_known_leader

        # Configure reader to have at most one outstanding unconfirmed message.
        self.reader.configure(uri_priority_1, block=True, maxUnconfirmedMessages=1)

        # Write two messages to the queue (only one can be sent to reader).
        assert self.post_n_msgs(uri_priority_1, 2)

        # Sleep for long enough to trigger an alarm.
        time.sleep(1.5)
        assert leader.alarms("QUEUE_STUCK", 1)

        # Confirm all messages in the queue when they are received.
        self.reader.wait_push_event()
        self.reader.confirm(uri_priority_1, "+1", succeed=True)
        self.reader.wait_push_event()
        self.reader.confirm(uri_priority_1, "+1", succeed=True)

        # Reconfigure domain to tolerate as much as two seconds of idleness.
        multi_node.config.domains[
            domain_urls.domain_priority
        ].definition.parameters.max_idle_time = 2
        multi_node.reconfigure_domain(
            domain_urls.domain_priority, leader_only=True, succeed=True
        )

        # Write two further messages to the queue.
        assert self.post_n_msgs(uri_priority_1, 2)

        # Sleep for duration between old and new allowed idleness durations.
        time.sleep(1.5)

        # Confirm both messages when they are received.
        self.reader.wait_push_event()
        self.reader.confirm(uri_priority_1, "+1", succeed=True)
        self.reader.wait_push_event()
        self.reader.confirm(uri_priority_1, "+1", succeed=True)

        # Ensure that no alarm was issued.
        assert not leader.alarms("QUEUE_STUCK", 1)

    @tweak.domain.message_ttl(1)
    def test_reconfigure_message_ttl(
        self, multi_node: Cluster, domain_urls: tc.DomainUrls
    ):
        uri_priority_1 = f"bmq://{domain_urls.domain_priority}/abcd-queue"
        leader = multi_node.last_known_leader

        # Write two messages to the queue (only one can be sent to reader).
        assert self.post_n_msgs(uri_priority_1, 2)

        # Sleep for long enough to trigger message GC.
        time.sleep(2)

        # Observe that both messages were GC'd from the queue.
        assert leader.erases_messages(uri_priority_1, msgs=2, timeout=1)

        # Reconfigure the domain to wait 3 seconds before GC'ing messages.
        multi_node.config.domains[
            domain_urls.domain_priority
        ].definition.parameters.message_ttl = 10
        multi_node.reconfigure_domain(
            domain_urls.domain_priority, leader_only=True, succeed=True
        )

        # Write two further messages to the queue.
        assert self.post_n_msgs(uri_priority_1, 2)

        # Sleep for the same duration as before.
        time.sleep(2)

        # Observe that no messages were GC'd.
        assert not leader.erases_messages(uri_priority_1, timeout=1)

        # Verify that the reader can confirm the written messages.
        self.reader.confirm(uri_priority_1, "+2", succeed=True)

    @tweak.domain.max_delivery_attempts(0)
    def test_reconfigure_max_delivery_attempts(
        self, multi_node: Cluster, domain_urls: tc.DomainUrls
    ):
        URI = f"bmq://{domain_urls.domain_priority}/reconf-rda"
        proxy = next(multi_node.proxy_cycle())

        # Open the queue through the writer.
        self.writer.open(URI, flags=["write,ack"], succeed=True)

        def do_test(expect_success):
            # Write one message to 'URI'.
            self.post_n_msgs(URI, 1)

            # Open, read, and kill five consumers in sequence.
            for idx in range(0, 5):
                client = proxy.create_client(f"reader-unstable-{idx}")
                client.open(URI, flags=["read"], succeed=True)
                client.check_exit_code = False
                client.wait_push_event(timeout=5)
                client.kill()
                client.wait()

            # Open one more client, and ensure it succeeds or fails to read a
            # message according to 'expect_success'.
            client = proxy.create_client("reader-stable")
            client.open(URI, flags=["read"], succeed=True)
            if expect_success:
                client.confirm(URI, "+1", succeed=True)
            else:
                assert not client.wait_push_event(timeout=5)
            client.stop_session(block=True)

        # Expect that message will not expire after failed deliveries.
        do_test(True)

        # Reconfigure messages to expire after 5 delivery attempts.
        multi_node.config.domains[
            domain_urls.domain_priority
        ].definition.parameters.max_delivery_attempts = 5
        multi_node.reconfigure_domain(domain_urls.domain_priority, succeed=True)

        # Expect that message will expire after failed deliveries.
        do_test(False)

    @tweak.domain.max_delivery_attempts(1)
    def test_reconfigure_max_delivery_attempts_finite(
        self, multi_node: Cluster, domain_urls: tc.DomainUrls
    ):
        domain_priority = domain_urls.domain_priority
        URI = f"bmq://{domain_priority}/reconf-rda"
        proxy = next(multi_node.proxy_cycle())

        # Open the queue through the writer.
        self.writer.open(URI, flags=["write,ack"], succeed=True)

        def do_test(expect_success, delivery_attempts):
            # Write one message to 'URI'.
            self.post_n_msgs(URI, 1)

            # Open, read, and kill delivery_attempts consumers in sequence.
            for idx in range(0, delivery_attempts - 1):
                client = proxy.create_client(f"reader-unstable-{idx}")
                client.open(URI, flags=["read"], succeed=True)
                client.check_exit_code = False
                client.wait_push_event(timeout=5)
                client.kill()
                client.wait()

            # Open one more client, and ensure it succeeds or fails to read a
            # message according to 'expect_success'.
            client = proxy.create_client("reader-stable")
            client.open(URI, flags=["read"], succeed=True)
            if expect_success:
                client.confirm(URI, "+1", succeed=True)
            else:
                assert not client.wait_push_event(timeout=5)
            client.stop_session(block=True)

        # Expect that message will not expire after failed deliveries.
        do_test(True, 1)

        for max_delivery_attempts in range(2, 7):
            # Reconfigure messages to expire after max_delivery_attempts delivery attempts.
            multi_node.config.domains[
                domain_priority
            ].definition.parameters.max_delivery_attempts = max_delivery_attempts
            multi_node.reconfigure_domain(domain_priority, succeed=True)

            # Attempt to deliver message max_delivery_attempts times,
            # client confirms at the last attempt.
            do_test(True, max_delivery_attempts)

            # Attempt to deliver message max_delivery_attempts + 1 times,
            # client never confirms, and should timeout after broker maxing out the attempts.
            do_test(False, max_delivery_attempts + 1)

    @tweak.domain.max_delivery_attempts(0)
    def test_reconfigure_max_delivery_attempts_on_existing_messages(
        self, multi_node: Cluster, domain_urls: tc.DomainUrls
    ) -> None:
        cluster: Cluster = multi_node

        # Stage 1: data preparation
        # On this stage, we open a producer to a queue and post 5 messages
        # with serial payloads: ["msg0", "msg1", "msg2", "msg3", "msg4"].
        # We consider the very first message "msg0" poisonous.
        domain_priority = domain_urls.domain_priority
        URI = f"bmq://{domain_priority}/reconf-rda-on-existing-msgs"
        proxy = next(multi_node.proxy_cycle())

        self.writer.open(URI, flags=["write,ack"], succeed=True)

        # Post a sequence of messages with serial payloads:
        assert self.post_n_msgs(URI, 5)

        poisoned_message: str = "msg0"

        def try_consume(consumer: Client) -> Optional[int]:
            num_confirmed = 0
            while consumer.wait_push_event(timeout=1, quiet=True):
                msgs = consumer.list(URI, block=True)

                assert len(msgs) == 1

                if msgs[0].payload == poisoned_message:
                    # In this test, do not expect any messages confirmed before the poisoned one
                    assert num_confirmed == 0
                    return None

                consumer.confirm(URI, "+1", succeed=True)
                num_confirmed += 1
            return num_confirmed

        def run_consumers(max_attempts: int) -> int:
            for idx in range(0, max_attempts):
                consumer: Client = proxy.create_client(f"reader-unstable-{idx}")
                consumer.open(
                    URI, flags=["read"], succeed=True, max_unconfirmed_messages=1
                )

                num_confirmed = try_consume(consumer)

                # Each consumer either crashes on the very first poisoned message
                # Or it confirms all the remaining non-poisoned messages and exits
                if num_confirmed is None:
                    consumer.check_exit_code = False
                    consumer.kill()
                    consumer.wait()
                    continue
                return num_confirmed

            # We return earlier if any messages confirmed
            return 0

        # Stage 2: try to consume messages without poison pill detection enabled.
        # Expect all the attempts to process messages failed, since all the
        # consumers will crash on the poisoned message "msg0".
        #
        # The timeline:
        # Before: queue is ["msg0", "msg1", "msg2", "msg3", "msg4"]
        # consumer1: ["msg0"] -> crash
        # ... ... ...
        # consumer10: ["msg0"] -> crash
        # After: queue is ["msg0", "msg1", "msg2", "msg3", "msg4"]
        num_confirmed = run_consumers(max_attempts=10)
        assert 0 == num_confirmed

        # Stage 3: reconfigure maxDeliveryAttempts to enable poison pill detection.
        # Use an admin session to validate that the setting change reached the broker.
        admin = AdminClient()
        admin.connect(*cluster.admin_endpoint)

        res = admin.send_admin(f"DOMAINS DOMAIN {domain_priority} INFOS")
        assert '"maxDeliveryAttempts" : 0' in res

        cluster.config.domains[
            domain_priority
        ].definition.parameters.max_delivery_attempts = 5
        cluster.reconfigure_domain(domain_priority, succeed=True)

        res = admin.send_admin(f"DOMAINS DOMAIN {domain_priority} INFOS")
        assert '"maxDeliveryAttempts" : 5' in res

        admin.stop()

        # Stage 4: try to consume messages with poison pill detection enabled.
        # Expect first 5 consumers to controllably crash on the very first message "msg0",
        # and the 6th consumer will not receive message "msg0" anymore, since it was removed
        # from the queue as poisonous. As a result, 6th consumer will receive the rest of the
        # messages one by one and confirm them.
        #
        # The timeline:
        # Before: queue is ["msg0", "msg1", "msg2", "msg3", "msg4"]
        # consumer1: ["msg0"] -> crash
        # consumer2: ["msg0"] -> crash
        # consumer3: ["msg0"] -> crash
        # consumer4: ["msg0"] -> crash
        # consumer5: ["msg0"] -> crash -> "msg0" removed as poisonous after 5 attempts
        # consumer6: ["msg1", "msg2", "msg3", "msg4"] -> confirm
        # After: queue is []
        num_confirmed = run_consumers(max_attempts=6)
        assert 4 == num_confirmed
