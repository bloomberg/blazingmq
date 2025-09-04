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

import blazingmq.dev.it.testconstants as tc
from blazingmq.dev.it.fixtures import (  # pylint: disable=unused-import
    Cluster,
    test_logger,
    tweak,
)
from blazingmq.dev.it.util import wait_until

BACKLOG_MESSAGES = 400  # minimum to consume before killing / shutting down
NUM_MESSAGES = 2000
POST_INTERVAL_MS = 2


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
            replica) can result in lost CONFIRMs (among other messagetypes).  The
            primary (either new or existing) 'redelivers' unconfirmed messages
            (either because it is the new primary or new replica opens the queue).
            This results in the same message delivered twice to consumer.
         4. No lost messages, even in 'kill primary' cases because of strong
            consitency.
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
        Put = namedtuple("Put", ["message_index", "guid"])
        Ack = namedtuple("Ack", ["message_index", "guid", "status"])
        Push = namedtuple("Push", ["message_index", "guid", "index"])
        Confirm = namedtuple("Confirm", ["message_index", "guid"])

        puts = [None for i in range(NUM_MESSAGES)]  # all Put messages
        acks = [None for i in range(NUM_MESSAGES)]  # all Ack messages

        guids = {}  # {guid -> index}

        num_acks = 0
        num_nacks = 0
        num_puts = 0
        num_duplicates = 0
        num_errors = 0

        def warning(test_logger, prefix, p1, p2=None):
            test_logger.warning(f"{prefix}: {p1}{f' vs existing {p2}' if p2 else ''}")

        def error(test_logger, prefix, p1, p2=None):
            nonlocal num_errors
            num_errors += 1
            warning(test_logger, prefix, p1, p2)

        consumer_log = self.work_dir / "producer.log"
        with open(consumer_log, encoding="ascii") as f:
            re_put = re.compile(
                r"(?i)"  # case insensitive
                + r" PUT "  #  PUT
                + re.escape(self.uri)  # queue url
                + r"\|\[ autoValue = (\d+) \]"  # |autoValue
                + r"\|(.+)\|"  # |GUID|
                + r"msg\s*"  # msg
                + r"(\d+)"
            )  # %d

            re_ack = re.compile(
                r" ACK "  # ACK
                + re.escape(self.uri)  # queue url
                + r"\|\[ autoValue = (\d+) \]"  # |autoValue
                + r"\|(.+)\|"  # |GUID|
                + r"(-?\d+)\|"
            )  # status|

            for line in f:
                match = re_put.search(line)
                if match:
                    guid = match[2]
                    message_index = int(match[3])

                    put = Put(message_index, guid)

                    assert message_index < NUM_MESSAGES
                    # should be no duplicate PUTs
                    assert not puts[message_index]
                    # should be unique guid
                    assert guid not in guids
                    # should be monotonically increasing
                    assert message_index == num_puts

                    guids[guid] = message_index
                    puts[message_index] = put
                    num_puts += 1
                else:
                    match = re_ack.search(line)
                    if match:
                        guid = match[2]
                        status = int(match[3])
                        message_index = guids.get(guid)

                        ack = Ack(
                            message_index=message_index,
                            guid=guid,
                            status=status,
                        )

                        if message_index is None:
                            error(test_logger, "unexpected ACK guid", ack)
                        elif puts[message_index] is None:
                            error(test_logger, "unexpected ACK payload", ack)
                        elif acks[message_index]:
                            error(
                                test_logger, "duplicate ACK", ack, acks[message_index]
                            )
                        else:
                            acks[message_index] = ack

                            if status == 0:
                                num_acks += 1
                            else:
                                num_nacks += 1
                                warning(
                                    test_logger,
                                    f"unsuccessful ({status}) ACK",
                                    ack,
                                    puts[message_index],
                                )

        test_logger.info(f"{num_puts} PUTs, {num_acks} acks, {num_nacks} nacks")

        for uri in self.uris:
            re_push = re.compile(
                r"(?i)"  # case insensitive
                + r" PUSH "  # PUSH
                + re.escape(uri[0])  # queue url
                + r"\|(.+)\|"  # |GUID|
                + r"msg\s*"  # "msg"
                + r"(\d+)"
            )  # %d

            re_confirm = re.compile(
                "(?i)"  # case insensitive
                r" CONFIRM "  # CONFIRM
                r"{re.escape(uri[0])}"  # queue url
                r"\|(.+)\|"
            )  # |GUID|

            app = uri[1]  # client Id
            consumer_log = self.work_dir / f"{app}.log"
            consumed = 0
            pushes = [None] * NUM_MESSAGES  # Push
            num_confirms = 0

            with open(consumer_log, encoding="ascii") as f:
                for line in f:
                    match = re_push.search(line)
                    if match:
                        message_index = int(match[2])
                        guid = match[1]

                        push = Push(
                            message_index=message_index, guid=guid, index=consumed
                        )

                        if pushes[message_index]:  # duplicate PUSH
                            num_duplicates += 1
                            warning(
                                test_logger,
                                f"{app}: duplicate PUSH payload",
                                push,
                                pushes[message_index],
                            )
                        else:
                            pushes[message_index] = push
                            consumed += 1

                            if puts[message_index] is None:  # no corresponding PUT
                                error(test_logger, f"{app}: unexpected PUSH", push)
                            elif acks[message_index] is None:  # no corresponding ACK:
                                error(test_logger, f"{app}: unexpected PUSH", push)
                            elif acks[message_index].guid != guid:  # ACK GUID mismatch
                                error(
                                    test_logger,
                                    f"{app}: GUID mismatch",
                                    push,
                                    acks[message_index],
                                )
                    else:
                        match = re_confirm.search(line)
                        if match:
                            guid = match[1]
                            message_index = guids.get(guid)

                            confirm = Confirm(message_index=message_index, guid=guid)

                            if message_index is None:
                                error(
                                    test_logger,
                                    f"{app}: unexpected CONFIRM guid",
                                    confirm,
                                )
                            elif pushes[message_index] is None:
                                error(
                                    test_logger, f"{app}: unexpected CONFIRM", confirm
                                )
                            else:
                                num_confirms += 1

            test_logger.info(
                f"{app}: {num_puts} PUTs"
                f", {num_acks} acks"
                f", {num_nacks} nacks"
                f", {consumed} PUSHs"
                f", {num_confirms} CONFIRMs"
                f", {num_duplicates} duplicates"
            )

            num_lost = 0
            for message_index in range(0, NUM_MESSAGES):
                if pushes[message_index] is None:
                    # never received 'message_index'
                    if acks[message_index]:
                        error(
                            test_logger, f"{app}: missing message", acks[message_index]
                        )
                    num_lost += 1
                elif pushes[message_index].index != (message_index - num_lost):
                    error(
                        test_logger, f"{app}: out of order PUSH", pushes[message_index]
                    )

        assert num_puts == num_acks
        assert consumed == num_puts
        assert num_errors == 0
        assert num_duplicates == 0 or allow_duplicates
        assert num_lost == 0

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
