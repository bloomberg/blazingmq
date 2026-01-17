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

# pylint: disable=protected-access; TODO: fix

from pathlib import Path
import re
from collections import namedtuple
from typing import Dict, List, Optional

import blazingmq.dev.it.testconstants as tc
from blazingmq.dev.it.fixtures import (  # pylint: disable=unused-import
    Cluster,
    cartesian_product_cluster,
    multi_node,
    order,
    test_logger,
    tweak,
)
from blazingmq.dev.it.util import wait_until

BACKLOG_MESSAGES = 400  # minimum to consume before killing / shutting down
NUM_MESSAGES = 2000
MAX_PRINT_ERRORS = 100
POST_INTERVAL_MS = 2

Put = namedtuple("Put", ["message_index", "guid"])
Ack = namedtuple("Ack", ["message_index", "guid", "status"])
Push = namedtuple("Push", ["message_index", "guid", "index"])
Confirm = namedtuple("Confirm", ["message_index", "guid"])


class DeliveryLog:
    """
    This class is used for verifying integrity of message flow from producer to consumers.
    """

    def __init__(self, logger, allow_duplicates: bool):
        self._logger = logger
        self._allow_duplicates = allow_duplicates

        self._num_acks = 0
        self._num_nacks = 0
        self._num_puts = 0
        self._num_duplicates = 0
        self._num_errors = 0

        self._errors: List[str] = []

        # Should be initialized once on `feed_producer_log` call:
        self._puts: List[Optional[Put]] = [None for _ in range(NUM_MESSAGES)]
        self._acks: List[Optional[Ack]] = [None for _ in range(NUM_MESSAGES)]

        self._guids: Dict[str, int] = {}  # {guid -> index}

        # Should be reinitialized on each `feed_consumer_log` call for each app_id:
        self._pushes: List[Optional[Push]] = []
        self._push_order: List[int] = []
        self._consumed = 0
        self._num_confirms = 0

    def _print_summary(self, app_id: Optional[str] = None) -> None:
        if app_id:
            self._logger.info(
                f"Summary ({app_id}): {self._num_puts} PUTs"
                f", {self._num_acks} ACKs"
                f", {self._num_nacks} NACKs"
                f", {self._consumed} PUSHs"
                f", {self._num_confirms} CONFIRMs"
                f", {self._num_duplicates} duplicates"
            )
        else:
            self._logger.info(
                f"Summary: {self._num_puts} PUTs"
                f", {self._num_acks} ACKs"
                f", {self._num_nacks} NACKs"
            )

    @staticmethod
    def _format_message(prefix: str, msg1, msg2=None) -> str:
        return f"{prefix}: {msg1}" + (f" vs existing {msg2}" if msg2 else "")

    def _warning(self, prefix: str, msg1, msg2=None):
        self._logger.warning(self._format_message(prefix, msg1, msg2))

    def _error(self, prefix: str, msg1, msg2=None):
        self._num_errors += 1

        error_log = self._format_message(prefix, msg1, msg2)
        if len(self._errors) < MAX_PRINT_ERRORS:
            self._errors.append(error_log)
        self._logger.error(error_log)

    def _format_error_status(self) -> str:
        return (
            f"Found {self._num_errors} errors, printing {min(MAX_PRINT_ERRORS, len(self._errors))}:\n"
            + "\n".join(self._errors)
        )

    def _on_put(self, put: Put) -> None:
        # should be monotonically increasing and not surpass the expected number of messages:
        assert put.message_index == self._num_puts
        assert put.message_index < NUM_MESSAGES

        # should be no duplicate PUTs:
        assert not self._puts[put.message_index]

        # should be unique guid:
        assert put.guid not in self._guids

        self._guids[put.guid] = put.message_index
        self._puts[put.message_index] = put
        self._num_puts += 1

    def _on_ack(self, ack: Ack) -> None:
        if ack.message_index is None:
            self._error("unexpected ACK guid", ack)
        elif self._puts[ack.message_index] is None:
            self._error("unexpected ACK payload", ack)
        elif self._acks[ack.message_index]:
            self._error("duplicate ACK", ack, self._acks[ack.message_index])
        else:
            self._acks[ack.message_index] = ack

            if ack.status == 0:
                self._num_acks += 1
            else:
                self._num_nacks += 1
                self._warning(
                    f"unsuccessful ({ack.status}) ACK",
                    ack,
                    self._puts[ack.message_index],
                )

    def _on_push(self, push: Push, app_id: str) -> None:
        message_index = push.message_index

        if self._pushes[message_index]:  # duplicate PUSH
            self._num_duplicates += 1
            self._warning(
                f"{app_id}: duplicate PUSH payload",
                push,
                self._pushes[message_index],
            )
        else:
            self._pushes[push.message_index] = push
            self._push_order.append(push.message_index)
            self._consumed += 1

            if self._puts[message_index] is None:  # no corresponding PUT
                self._error(f"{app_id}: unexpected PUSH (no corresponding PUT)", push)
            elif self._acks[message_index] is None:  # no corresponding ACK:
                self._error(f"{app_id}: unexpected PUSH (no corresponding ACK)", push)
            elif self._acks[message_index].guid != push.guid:  # ACK GUID mismatch
                self._error(
                    f"{app_id}: GUID mismatch",
                    push,
                    self._acks[message_index],
                )

    def _on_confirm(self, confirm: Confirm, app_id: str) -> None:
        if confirm.message_index is None:
            self._error(
                f"{app_id}: unexpected CONFIRM guid",
                confirm,
            )
        elif self._pushes[confirm.message_index] is None:
            self._error(f"{app_id}: unexpected CONFIRM", confirm)
        else:
            self._num_confirms += 1

    def feed_producer_log(self, path: Path, uri: str) -> None:
        re_put = re.compile(
            r"(?i)"  # case insensitive
            + r" PUT "  # PUT
            + re.escape(uri)  # queue url
            + r"\|\[ autoValue = (\d+) \]"  # |autoValue
            + r"\|(.+)\|"  # |GUID|
            + r"msg\s*"  # msg
            + r"(\d+)"  # %d
        )

        re_ack = re.compile(
            r" ACK "  # ACK
            + re.escape(uri)  # queue url
            + r"\|\[ autoValue = (\d+) \]"  # |autoValue
            + r"\|(.+)\|"  # |GUID|
            + r"(-?\d+)\|"  # status|
        )

        with open(path, encoding="ascii") as f:
            for line in f:
                match = re_put.search(line)
                if match:
                    guid = match[2]
                    message_index = int(match[3])
                    put = Put(message_index, guid)
                    self._on_put(put)
                    continue

                match = re_ack.search(line)
                if match:
                    guid = match[2]
                    status = int(match[3])
                    message_index = self._guids.get(guid)
                    ack = Ack(
                        message_index=message_index,
                        guid=guid,
                        status=status,
                    )
                    self._on_ack(ack)
                    continue

        self._print_summary()
        assert self._num_errors == 0, self._format_error_status()
        assert self._num_puts == self._num_acks

    def feed_consumer_log(self, path: Path, uri: str, app_id: str) -> None:
        re_push = re.compile(
            r"(?i)"  # case insensitive
            + r" PUSH "  # PUSH
            + re.escape(uri)  # queue url
            + r"\|(.+)\|"  # |GUID|
            + r"msg\s*"  # "msg"
            + r"(\d+)"  # %d
        )

        re_confirm = re.compile(
            r"(?i)"  # case insensitive
            + " CONFIRM "  # CONFIRM
            + re.escape(uri)  # queue url
            + r"\|(.+)\|"  # |GUID|
        )

        # Reinitialize for the current `app_id`:
        self._pushes: List[Optional[Push]] = [None for _ in range(NUM_MESSAGES)]
        self._push_order: List[int] = []
        self._consumed = 0
        self._num_confirms = 0

        with open(path, encoding="ascii") as f:
            for line in f:
                match = re_push.search(line)
                if match:
                    message_index = int(match[2])
                    guid = match[1]
                    push = Push(
                        message_index=message_index, guid=guid, index=self._consumed
                    )
                    self._on_push(push, app_id)
                    continue

                match = re_confirm.search(line)
                if match:
                    guid = match[1]
                    message_index = self._guids.get(guid)
                    confirm = Confirm(message_index=message_index, guid=guid)
                    self._on_confirm(confirm, app_id)
                    continue

        self._print_summary(app_id)

        # First, make sure that all ACKed messages were PUSHed
        num_lost = 0
        for message_index in range(0, NUM_MESSAGES):
            if self._pushes[message_index] is None:
                num_lost += 1
                if self._acks[message_index]:
                    self._error(f"{app_id}: missing message", self._acks[message_index])

        assert self._num_errors == 0, self._format_error_status()

        # Secondly, make sure that PUSHes are in correct order
        previous_index = -1
        for message_index in self._push_order:
            if message_index < previous_index:
                self._error(
                    f"{app_id}: out of order PUSH",
                    self._pushes[message_index],
                    self._pushes[previous_index],
                )
            previous_index = message_index

        assert self._num_errors == 0, self._format_error_status()
        assert self._consumed == self._num_puts
        assert self._num_duplicates == 0 or self._allow_duplicates
        assert num_lost == 0


@tweak.domain.storage.domain_limits.messages(NUM_MESSAGES)
@tweak.domain.storage.queue_limits.messages(NUM_MESSAGES)
class TestPutsRetransmission:
    """
    This test verifies PUTs retransmission by downstream to new upstream.
    The producer connects to replica Proxy and streams NUM_MESSAGES every
    POST_INTERVAL ms.  The consumer connects to replica Proxy and expects to
    receive all messages.  Meanwhile, either primary or _the_ replica either
    gracefully shuts down or gets killed:
        1. Kill the replica
        2. Shutdown the replica
        3. Kill the primary
            a. The replica becomes new primary
            b. The replica does not become new primary
        4. Shutdown the primary
            a. The replica becomes new primary
            b. The replica does not become new primary

    The test expects:
        1. Success ACKs for all PUTs
        2. The same order of PUSH messages as the order of PUT messages
            (monotonically increasing "msg%10d|")
        3. No duplicates except for 'kill' cases.  Killing either primary or
            replica) can result in lost CONFIRMs (among other message types).
            The primary (either new or existing) 'redelivers' unconfirmed messages
            (either because it is the new primary or new replica opens the queue).
            This results in the same message delivered twice to consumer.
         4. No lost messages, even in 'kill primary' cases because of strong
            consistency.
    """

    work_dir: Path

    def inspect_results(self, allow_duplicates=False):
        if self.active_node in self.cluster.virtual_nodes():
            self.active_node.wait_status(wait_leader=True, wait_ready=False)

        leader = self.active_node.last_known_leader
        assert leader

        assert wait_until(
            lambda: self.capture_number_of_consumed_messages(2 * NUM_MESSAGES), 40
        )

        for uri, consumer in zip(self.uris, self.consumers):
            test_logger.info(f"{uri[0]}: received {consumer[1]} messages")
            # assert consumer[1] == NUM_MESSAGES

        leader.command(f"DOMAINS DOMAIN {self.domain} QUEUE {tc.TEST_QUEUE} INTERNALS")

        leader.list_messages(self.domain, tc.TEST_QUEUE, 0, NUM_MESSAGES)
        assert leader.outputs_substr("Printing 0 message(s)", 10)

        self.producer.force_stop()

        for consumer in self.consumers:
            consumer[0].force_stop()

        self.parse_message_logs(allow_duplicates=allow_duplicates)

    def parse_message_logs(self, allow_duplicates=False):
        delivery_log = DeliveryLog(test_logger, allow_duplicates)
        delivery_log.feed_producer_log(self.work_dir / "producer.log", self.uri)
        for uri, app_id in self.uris:
            delivery_log.feed_consumer_log(self.work_dir / f"{app_id}.log", uri, app_id)

    def capture_number_of_consumed_messages(self, at_least, timeout=20):
        """
        This method sums up total number of PUSH messages for each consumer and
        stores the result in 'self.consumers[i].[1]'.  If all consumers have
        received at least 'at_least' messages, return 'True'.  Else, continue
        until a consumer does not report any progress ('self.consumers[i].[2]')
        for second consecutive check.  In which case, return 'False'.
        """
        for consumer in self.consumers:
            if consumer[1] < at_least:
                capture = consumer[0].capture(
                    r"consumed \|.*\| \s*(\d+) \|", timeout=timeout
                )
                assert capture

                if int(capture[1]) > 0:
                    consumer[2] = int(capture[1])
                    consumer[1] += consumer[2]
                    if consumer[1] < at_least:
                        return False
                elif consumer[2] > 0:
                    consumer[2] = 0
                    return False
                # else not making progress for two consecutive runs, give up

        return True

    def has_consumed_all(self, timeout=20):
        for consumer in self.consumers:
            capture = consumer[0].capture(
                r"consumed \|.*\| \s*(\d+) \|", timeout=timeout
            )
            assert capture

            if int(capture[1]) > 0:
                return False

        return True

    def setup_cluster_fanout(self, cluster, du: tc.DomainUrls):
        self.uri = du.uri_fanout
        self.domain = du.domain_fanout
        # uri and client Id pairs
        self.uris = [
            (du.uri_fanout_foo, "foo"),
            (du.uri_fanout_bar, "bar"),
            (du.uri_fanout_baz, "baz"),
        ]

        self.set_proxy(cluster)

        self.mps = '[{"name": "p1", "value": "s1", "type": "E_STRING"}]'

        self.start_producer(0)
        self.start_consumers(cluster, BACKLOG_MESSAGES)

    def setup_cluster_broadcast(self, cluster):
        self.uri = tc.URI_BROADCAST
        self.domain = tc.DOMAIN_BROADCAST
        # uri and client Id pairs
        self.uris = [
            (tc.URI_BROADCAST, 1),
            (tc.URI_BROADCAST, 2),
            (tc.URI_BROADCAST, 3),
        ]

        self.set_proxy(cluster)
        self.mps = '[{"name": "p1", "value": "s1", "type": "E_STRING"}]'

        self.start_consumers(cluster, 0)
        self.start_producer(BACKLOG_MESSAGES)

    def set_proxy(self, cluster):
        self.cluster = cluster
        self.work_dir = cluster.work_dir
        proxies = cluster.proxy_cycle()
        # pick proxy in datacenter opposite to the primary's
        next(proxies)
        self.replica_proxy = next(proxies)

    def capture_number_of_produced_messages(self, producer, timeout=20):
        capture = producer.capture(r"produced \|.*\| \s*(\d+) \|", timeout=timeout)
        assert capture
        produced = int(capture[1])
        return produced

    def start_producer(self, after=BACKLOG_MESSAGES):
        producer_log = self.work_dir / "producer.log"
        producer_log.unlink(missing_ok=True)

        self.producer = self.replica_proxy.create_client(
            "producer",
            start=False,
            dump_messages=False,
            options=[
                "--mode",
                "auto",
                "--queueflags",
                "write,ack",
                "--eventscount",
                NUM_MESSAGES,
                "--postinterval",
                POST_INTERVAL_MS,
                "--queueuri",
                self.uri,
                "--messagepattern",
                "msg",
                f"--messageProperties={self.mps}",
                "--log",
                producer_log,
            ],
        )
        # generate BACKLOG_MESSAGES
        if after > 0:
            assert wait_until(
                lambda: self.capture_number_of_produced_messages(self.producer)
                >= after,
                40,
            )

    def start_consumers(self, cluster, after=BACKLOG_MESSAGES):
        # generate BACKLOG_MESSAGES
        if after > 0:
            assert wait_until(
                lambda: self.capture_number_of_produced_messages(self.producer)
                >= after,
                20,
            )

        def create_consumer(proxy, uri, work_dir: Path):
            app = uri[1]  # uri[1] - client id
            assert app
            consumer_log = work_dir / f"{app}.log"
            consumer_log.unlink(missing_ok=True)

            return [
                proxy.create_client(
                    f"consumer{app}",
                    start=False,
                    dump_messages=False,
                    options=[
                        "--mode",
                        "auto",
                        "--queueflags",
                        "read",
                        "--eventscount",
                        NUM_MESSAGES,
                        "--queueuri",
                        uri[0],
                        "-c",
                        "--log",
                        consumer_log,
                        f"--messageProperties={self.mps}",
                        "--maxunconfirmed",
                        f"{NUM_MESSAGES}:{NUM_MESSAGES * 1024}",
                    ],
                ),
                0,  # total received
                0,  # last received (for tracking progress)
            ]

        # create consumers
        self.consumers = [
            create_consumer(self.replica_proxy, uri, self.work_dir) for uri in self.uris
        ]

        # make sure consumers are running and receiving messages
        assert wait_until(lambda: self.capture_number_of_consumed_messages(1), 40)

        self.leader = cluster.last_known_leader
        self.active_node = cluster.process(self.replica_proxy.get_active_node())

    def test_shutdown_primary_convert_replica(
        self, multi_node: Cluster, domain_urls: tc.DomainUrls
    ):
        self.setup_cluster_fanout(multi_node, domain_urls)

        # make the 'active_node' new primary
        for node in multi_node.nodes(exclude=self.active_node):
            node.set_quorum(4)

        self.active_node.drain()
        # Start graceful shutdown
        self.leader.exit_gracefully()
        # Wait for the subprocess to terminate
        self.leader.wait()

        # If shutting down primary, the replica needs to wait for new primary.
        self.active_node.wait_status(wait_leader=True, wait_ready=False)

        self.inspect_results(allow_duplicates=False)

    def test_shutdown_primary_keep_replica(
        self, multi_node: Cluster, domain_urls: tc.DomainUrls
    ):
        self.setup_cluster_fanout(multi_node, domain_urls)

        # prevent 'active_node' from becoming new primary
        self.active_node.set_quorum(4)

        self.active_node.drain()
        # Start graceful shutdown
        self.leader.exit_gracefully()
        # Wait for the subprocess to terminate
        self.leader.wait()

        # If shutting down primary, the replica needs to wait for new primary.
        self.active_node.wait_status(wait_leader=True, wait_ready=False)

        # Do allow duplicates for the scenario when a CONFIRM had passed Proxy
        # but did not reach the replication.  New Primary then redelivers and
        # the Proxy cannot detect the duplicate because it had removed the GUID
        # upon the first CONFIRM

        self.inspect_results(allow_duplicates=True)

    def test_shutdown_replica(self, multi_node: Cluster, domain_urls: tc.DomainUrls):
        self.setup_cluster_fanout(multi_node, domain_urls)

        # Start graceful shutdown
        self.active_node.exit_gracefully()
        # Wait for the subprocess to terminate
        self.active_node.wait()

        # Because the quorum is 3, cluster is still healthy after shutting down
        # replica.

        # Do allow duplicates for the scenario when a CONFIRM had passed Proxy
        # but did not reach the replication.  New Primary then redelivers and
        # the Proxy cannot detect the duplicate because it had removed the GUID
        # upon the first CONFIRM

        self.inspect_results(allow_duplicates=True)

    def test_kill_primary_convert_replica(
        self, multi_node: Cluster, domain_urls: tc.DomainUrls
    ):
        self.setup_cluster_fanout(multi_node, domain_urls)

        # make the 'active_node' new primary
        nodes = multi_node.nodes(exclude=self.active_node)
        for node in nodes:
            node.set_quorum(4)

        self.active_node.drain()
        # Kill leader.
        self.leader.force_stop()

        # If shutting down primary, the replica needs to wait for new primary.
        self.active_node.wait_status(wait_leader=True, wait_ready=False)

        self.inspect_results(allow_duplicates=True)

    def test_kill_primary_keep_replica(
        self, multi_node: Cluster, sc_domain_urls: tc.DomainUrls
    ):
        self.setup_cluster_fanout(multi_node, sc_domain_urls)

        # prevent 'active_node' from becoming new primary
        self.active_node.set_quorum(4)

        self.active_node.drain()
        # Kill leader.
        self.leader.force_stop()

        # If shutting down primary, the replica needs to wait for new primary.
        self.active_node.wait_status(wait_leader=True, wait_ready=False)

        self.inspect_results(allow_duplicates=True)

    def test_kill_replica(self, multi_node: Cluster, domain_urls: tc.DomainUrls):
        self.setup_cluster_fanout(multi_node, domain_urls)

        # Start graceful shutdown
        self.active_node.force_stop()

        # Because the quorum is 3, cluster is still healthy after shutting down
        # replica.
        self.inspect_results(allow_duplicates=True)

    @tweak.broker.app_config.network_interfaces.tcp_interface.low_watermark(512)
    @tweak.broker.app_config.network_interfaces.tcp_interface.high_watermark(1024)
    def test_watermarks(self, multi_node: Cluster, domain_urls: tc.DomainUrls):
        uri_priority = domain_urls.uri_priority

        self.setup_cluster_fanout(multi_node, domain_urls)

        producer1 = self.replica_proxy.create_client("producer1")
        producer1.open(uri_priority, flags=["write", "ack"], succeed=True)

        consumer1 = self.replica_proxy.create_client("consumer1")
        consumer1.open(uri_priority, flags=["read"], succeed=True)

        producer1.post(uri_priority, payload=["msg"], succeed=True, wait_ack=True)
        consumer1.wait_push_event()
        assert wait_until(
            lambda: len(consumer1.list(uri_priority, block=True)) == 1, 10
        )
        msgs = consumer1.list(uri_priority, block=True)
        assert msgs[0].payload == "msg"

        assert wait_until(self.has_consumed_all, 120)
        self.inspect_results(allow_duplicates=False)

    def test_kill_proxy(self, multi_node: Cluster, domain_urls: tc.DomainUrls):
        self.setup_cluster_fanout(multi_node, domain_urls)

        self.replica_proxy.force_stop()

        # Wait for the subprocess to terminate

        self.replica_proxy.start()
        self.replica_proxy.wait_until_started()

        self.inspect_results(allow_duplicates=True)

    def test_shutdown_upstream_proxy(
        self,
        cartesian_product_cluster: Cluster,
        domain_urls: tc.DomainUrls,  # pylint: disable=unused-argument
    ):
        if not cartesian_product_cluster.virtual_nodes():
            # Skip cluster without virtual nodes
            return

        self.setup_cluster_broadcast(cartesian_product_cluster)

        # Shutdown upstream proxy (VR)
        self.active_node.exit_gracefully()

        self.inspect_results(allow_duplicates=True)
