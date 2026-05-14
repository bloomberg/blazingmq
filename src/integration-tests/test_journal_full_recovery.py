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
Testing recovery from journal-full (PARTITION_READONLY) state via PURGE.
"""

import blazingmq.dev.it.testconstants as tc
from blazingmq.dev.it.process.client import Client
from blazingmq.dev.it.fixtures import (
    Cluster,
    tweak,
)


# Journal file size chosen so that outstanding bytes exceed 80% of the file
# when it fills up, causing rollover to fail with PARTITION_READONLY.
#
# Without confirms each message = 1 journal record (60 bytes) that is
# outstanding.  For rollover to fail: outstanding > 0.8 * fileSize.
# With fileSize = 8192: ~110 message records fit, outstanding ~6600 bytes =
# ~81% of 8192.
MAX_JOURNAL_FILE_SIZE = 8192


@tweak.cluster.partition_config.num_partitions(1)
@tweak.cluster.partition_config.max_journal_file_size(MAX_JOURNAL_FILE_SIZE)
def test_journal_full_purge_recovery(
    cluster: Cluster,
    domain_urls: tc.DomainUrls,
) -> None:
    """
    Test that a partition which becomes READONLY due to a full journal can
    recover after a full queue purge, but not after a per-appId purge.

    A priority queue fills ~50% of the journal, then a fanout queue fills the
    rest.  Only the fanout queue is purged — the priority queue's messages
    remain outstanding.  The partition should still recover because
    outstanding bytes drop below the 80% rollover threshold.

    Then:
    1. Fill ~50% of journal with priority queue messages.
    2. Fill the rest with fanout queue messages until PARTITION_READONLY.
    3. Verify posts fail.
    4. Purge appId "baz" only — should NOT recover (per-appId purge is
       rejected on unavailable journal).
    5. Purge the entire fanout queue — should recover the journal.
    6. Verify posts succeed again on both queues.
    7. Restart leader and verify the partition remains healthy.
    """
    du = domain_urls

    leader = cluster.last_known_leader
    proxy = next(cluster.proxy_cycle())

    # Open priority queue producer and consumer.  Consumer never confirms,
    # keeping all messages outstanding.
    pri_producer = proxy.create_client("pri_producer")
    pri_producer.open(du.uri_priority, flags=["write,ack"], succeed=True)

    pri_consumer = proxy.create_client("pri_consumer")
    pri_consumer.open(du.uri_priority, flags=["read"], succeed=True)

    # 1. Post ~50 messages to the priority queue to fill ~50% of the journal.
    num_priority_msgs = 50
    for i in range(num_priority_msgs):
        rc = pri_producer.post(
            du.uri_priority, [f"pri{i}"], wait_ack=True, no_except=True
        )
        assert rc == Client.e_SUCCESS, (
            f"Priority post {i} failed unexpectedly (journal should not be full yet)"
        )

    # Open fanout producer.
    producer = proxy.create_client("producer")
    producer.open(du.uri_fanout, flags=["write,ack"], succeed=True)

    # Open consumers for all appIds so messages are routed, but never confirm
    # from any of them.  This keeps every message outstanding in storage.
    consumer_foo = proxy.create_client("consumer_foo")
    consumer_foo.open(du.uri_fanout_foo, flags=["read"], succeed=True)

    consumer_bar = proxy.create_client("consumer_bar")
    consumer_bar.open(du.uri_fanout_bar, flags=["read"], succeed=True)

    consumer_baz = proxy.create_client("consumer_baz")
    consumer_baz.open(du.uri_fanout_baz, flags=["read"], succeed=True)

    # 2. Post to fanout without confirming until journal is full.
    max_iterations = 500
    num_posted = 0
    for i in range(max_iterations):
        rc = producer.post(du.uri_fanout, [f"msg{i}"], wait_ack=True, no_except=True)
        if rc != Client.e_SUCCESS:
            break
        num_posted += 1
    else:
        assert False, f"Journal did not fill up after {max_iterations} posts"

    assert num_posted > 0, "Should have posted at least one message before full"
    assert leader.outputs_substr("PANIC [PARTITION_READONLY]", timeout=5), (
        "Expected PARTITION_READONLY panic"
    )

    # 3. Verify further posts fail on both queues.
    rc = producer.post(du.uri_fanout, ["should_fail"], wait_ack=True, no_except=True)
    assert rc != Client.e_SUCCESS, "Fanout post should fail when journal is full"

    rc = pri_producer.post(
        du.uri_priority, ["should_fail"], wait_ack=True, no_except=True
    )
    assert rc != Client.e_SUCCESS, "Priority post should fail when journal is full"

    # 4. Purge appId "baz" only — per-appId purge should NOT recover.
    leader.purge(du.domain_fanout, tc.TEST_QUEUE, appid="baz")

    rc = producer.post(
        du.uri_fanout, ["after_baz_purge"], wait_ack=True, no_except=True
    )
    assert rc != Client.e_SUCCESS, (
        "Post should still fail after per-appId purge (journal still unavailable)"
    )

    # 5. Purge the entire fanout queue — this should recover.
    #    Priority queue is NOT purged; its messages remain outstanding.
    leader.purge(du.domain_fanout, tc.TEST_QUEUE)

    assert leader.outputs_substr("Writing PURGE record", timeout=5), (
        "Expected leader to log PURGE record write on unavailable journal"
    )

    # 6. Verify posts succeed after full purge on both queues.
    rc = producer.post(du.uri_fanout, ["after_purge"], wait_ack=True, no_except=True)
    assert rc == Client.e_SUCCESS, (
        "Fanout post should succeed after full purge recovered the journal"
    )

    rc = pri_producer.post(
        du.uri_priority, ["pri_after_purge"], wait_ack=True, no_except=True
    )
    assert rc == Client.e_SUCCESS, "Priority post should succeed after journal recovery"

    # 7. Verify end-to-end: consume the post-recovery fanout message.
    consumer_foo.wait_push_event()
    consumer_foo.confirm(du.uri_fanout_foo, "*", succeed=True)

    # 8. Verify end-to-end: consume the post-recovery priority message.
    pri_consumer.wait_push_event()
    pri_consumer.confirm(du.uri_priority, "*", succeed=True)

    # 9. Restart leader and verify the partition recovers cleanly.
    leader.stop()
    cluster.make_sure_node_stopped(leader)
    leader.drain()
    leader.start()
    leader.wait_until_started()

    producer2 = proxy.create_client("producer2")
    producer2.open(du.uri_fanout, flags=["write,ack"], succeed=True)
    rc = producer2.post(du.uri_fanout, ["after_restart"], wait_ack=True, no_except=True)
    assert rc == Client.e_SUCCESS, "Post should succeed after leader restart"
