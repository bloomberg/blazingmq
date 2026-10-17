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
Fanout queues on a proxy: the proxy must keep the messages other Apps still
need when an App's last reader goes away, or when an App confirms a message
that was not pushed to it.

A proxy keeps a fanout message until every App it was pushed to has
confirmed it.  In each test, the proxy holds messages that were not pushed
to baz, and either baz's last reader goes away or baz confirms them.  The
test then checks that foo and bar still get every message.  If the proxy
treats baz as done with those messages, foo's CONFIRMs delete messages bar
still needs: the proxy either aborts, when bar's copy is in the PushStream,
or silently drops it, when bar's copy is in a redelivery list.
"""

import time

import blazingmq.dev.it.testconstants as tc
from blazingmq.dev.it.fixtures import (
    Cluster,
    order,
    tweak,
)
from blazingmq.dev.configurator.configurator import Configurator
from blazingmq.dev.it.util import wait_until

pytestmark = order(5)

THROTTLE_INTERVAL_MS = 8000


def long_message_throttling(configurator: Configurator):
    """Delay every redelivery of a rejected message by the throttle interval."""
    throttle_config = configurator.proto.cluster.message_throttle_config
    assert throttle_config is not None
    throttle_config.high_threshold = 5
    throttle_config.low_threshold = 5
    throttle_config.low_interval = THROTTLE_INTERVAL_MS
    throttle_config.high_interval = THROTTLE_INTERVAL_MS


def _payloads(client, uri):
    return [m.payload for m in client.list(uri, block=True)]


def _wait_count(client, uri, count, timeout=5):
    return wait_until(lambda: len(client.list(uri, block=True)) == count, timeout)


def _open_two_consumers_per_app(proxy, uri_foo, uri_bar):
    consumers = {}
    for name, app_uri in (
        ("foo1", uri_foo),
        ("foo2", uri_foo),
        ("bar1", uri_bar),
        ("bar2", uri_bar),
    ):
        consumer = proxy.create_client(name)
        consumer.open(app_uri, flags=["read"], max_unconfirmed_messages=3, succeed=True)
        consumers[name] = (consumer, app_uri)
    return consumers


def _consume_first_batch(producer, uri, baz, uri_baz, consumers):
    """
    Post M1..M12.  baz receives and confirms all of them, while each foo and
    bar consumer takes 3 and confirms them.  The proxy deletes M7..M12 once
    baz confirms them, so they are pushed again to foo and bar only.
    """
    for i in range(1, 13):
        producer.post(uri, payload=[f"M{i}"], succeed=True)

    assert _wait_count(baz, uri_baz, 12)
    baz.confirm(uri_baz, "*", succeed=True)
    for consumer, app_uri in consumers.values():
        assert _wait_count(consumer, app_uri, 3)
        consumer.confirm(app_uri, "*", succeed=True)
    for consumer, app_uri in consumers.values():
        assert _wait_count(consumer, app_uri, 3)


@tweak.domain.max_delivery_attempts(5)
@tweak(long_message_throttling)
def test_reader_close_with_pushstream_backlog(
    multi_node: Cluster, domain_urls: tc.DomainUrls
):
    """
    Minimal sequence; bar's copies are in the PushStream.

    1. foo (max 3), bar1 (max 2), bar2 (max 1), baz (max 1).  Post
       M1..M20: M1 is pushed to foo, bar and baz; M2 and M3 to foo and bar.
       baz is full, so no later message is pushed to it.
    2. Kill bar1.  Its two messages are rejected and wait, throttled, in
       bar's redelivery list.
    3. Raise bar2's limit.  The primary pushes M4..M20 to bar alone; bar's
       redelivery list is not empty, so they stay in bar's PushStream.
    4. Raise foo's limit.  M4..M20 are pushed to foo too.
    5. baz's consumer, its only reader, closes.
    6. foo confirms everything.
    7. When the throttle expires, bar redelivers and catches up: the proxy
       must stay up and bar2 must get every message.
    """
    du = domain_urls
    uri = du.uri_fanout
    uri_foo, uri_bar, uri_baz = du.uri_fanout_foo, du.uri_fanout_bar, du.uri_fanout_baz
    proxy = next(multi_node.proxy_cycle())
    num_messages = 20

    foo = proxy.create_client("foo")
    foo.open(uri_foo, flags=["read"], max_unconfirmed_messages=3, succeed=True)
    bar1 = proxy.create_client("bar1")
    bar1.open(uri_bar, flags=["read"], max_unconfirmed_messages=2, succeed=True)
    bar2 = proxy.create_client("bar2")
    bar2.open(uri_bar, flags=["read"], max_unconfirmed_messages=1, succeed=True)
    baz = proxy.create_client("baz")
    baz.open(uri_baz, flags=["read"], max_unconfirmed_messages=1, succeed=True)

    producer = proxy.create_client("producer")
    producer.open(uri, flags=["write", "ack"], succeed=True)
    payloads = [f"M{i}" for i in range(1, num_messages + 1)]
    for payload in payloads:
        producer.post(uri, payload=[payload], succeed=True)

    assert _wait_count(foo, uri_foo, 3)
    assert _wait_count(bar1, uri_bar, 2)
    assert _wait_count(bar2, uri_bar, 1)
    assert _wait_count(baz, uri_baz, 1)
    assert sorted(_payloads(bar1, uri_bar) + _payloads(bar2, uri_bar)) == [
        "M1",
        "M2",
        "M3",
    ]
    assert _payloads(baz, uri_baz) == ["M1"]

    bar1.check_exit_code = False
    bar1.kill()
    assert proxy.outputs_substr("rejecting 2 messages because of a client crash", 10)

    bar2.configure(uri_bar, max_unconfirmed_messages=num_messages, succeed=True)
    time.sleep(0.5)
    foo.configure(uri_foo, max_unconfirmed_messages=num_messages, succeed=True)
    assert _wait_count(foo, uri_foo, num_messages, 3)
    # bar2 has its first message and one redelivery; M4..M20 wait in the
    # PushStream.
    assert _wait_count(bar2, uri_bar, 2)

    baz.close(uri_baz, succeed=True)
    foo.confirm(uri_foo, "*", succeed=True)

    time.sleep(THROTTLE_INTERVAL_MS / 1000 + 2)

    assert proxy.is_alive()
    assert wait_until(lambda: sorted(_payloads(bar2, uri_bar)) == sorted(payloads), 5)


@tweak.domain.max_delivery_attempts(5)
@tweak(long_message_throttling)
def test_reader_crash_with_pushstream_backlog(
    multi_node: Cluster, domain_urls: tc.DomainUrls
):
    """
    foo and bar are slow, baz is fast; baz's client crashes while bar has
    copies both in a redelivery list and in the PushStream.

    1. foo and bar have two consumers each (max 3).  baz confirms M1..M12
       before M7..M12 are pushed again to foo and bar only (see
       `_consume_first_batch`).
    2. Pause baz.  Close foo2 and kill bar2, leaving a redelivery list of 3
       for each App.  bar2's messages are rejected, so bar's redeliveries
       are throttled.
    3. Raise foo1 and bar1's limits, then post N1..N6.  Each N is pushed to
       foo and bar together.  foo takes it; bar, whose redelivery list is
       not empty, falls behind in the PushStream.
    4. baz's client crashes holding no messages.
    5. foo confirms everything.
    6. bar redelivers and catches up: the proxy must stay up and bar1 must
       get every message.
    """
    du = domain_urls
    uri = du.uri_fanout
    uri_foo, uri_bar, uri_baz = du.uri_fanout_foo, du.uri_fanout_bar, du.uri_fanout_baz
    proxy = next(multi_node.proxy_cycle())

    consumers = _open_two_consumers_per_app(proxy, uri_foo, uri_bar)
    foo1, foo2 = consumers["foo1"][0], consumers["foo2"][0]
    bar1, bar2 = consumers["bar1"][0], consumers["bar2"][0]
    baz = proxy.create_client("baz")
    baz.open(uri_baz, flags=["read"], succeed=True)
    producer = proxy.create_client("producer")
    producer.open(uri, flags=["write", "ack"], succeed=True)

    _consume_first_batch(producer, uri, baz, uri_baz, consumers)
    second_batch = sorted(f"M{i}" for i in range(7, 13))
    assert sorted(_payloads(foo1, uri_foo) + _payloads(foo2, uri_foo)) == second_batch
    assert sorted(_payloads(bar1, uri_bar) + _payloads(bar2, uri_bar)) == second_batch

    baz.configure(uri_baz, max_unconfirmed_messages=0, succeed=True)

    foo2.close(uri_foo, succeed=True)
    bar2.check_exit_code = False
    bar2.kill()
    assert proxy.outputs_substr("rejecting 3 messages because of a client crash", 10)

    bar1.configure(uri_bar, max_unconfirmed_messages=12, succeed=True)
    foo1.configure(uri_foo, max_unconfirmed_messages=12, succeed=True)
    assert _wait_count(foo1, uri_foo, 6, 3)

    for i in range(1, 7):
        producer.post(uri, payload=[f"N{i}"], succeed=True)
    assert _wait_count(foo1, uri_foo, 12, 3)
    assert _wait_count(bar1, uri_bar, 4)

    baz.check_exit_code = False
    baz.kill()
    assert proxy.outputs_substr("rejecting 0 messages because of a client crash", 10)

    foo1.confirm(uri_foo, "*", succeed=True)

    time.sleep(THROTTLE_INTERVAL_MS / 1000 + 2)

    assert proxy.is_alive()
    # bar2's other two messages are redelivered one throttle interval apart
    # before bar catches up.
    expected = sorted(second_batch + [f"N{i}" for i in range(1, 7)])
    assert wait_until(
        lambda: sorted(_payloads(bar1, uri_bar)) == expected,
        THROTTLE_INTERVAL_MS / 1000 + 5,
    )


def test_reader_crash_with_redelivery_backlog(
    multi_node: Cluster, domain_urls: tc.DomainUrls
):
    """
    bar's copies are only in a redelivery list, so the bug loses them
    instead of aborting.

    1. foo and bar have two consumers each (max 3).  baz confirms M1..M12
       before M7..M12 are pushed again to foo and bar only (see
       `_consume_first_batch`).
    2. Close foo2 and bar2, leaving a redelivery list of 3 for each App.
    3. baz's client crashes holding no messages.
    4. foo1 confirms its messages and those redelivered from foo2.
    5. bar1 confirms its messages: it must then get the 3 that bar2 held.
    """
    du = domain_urls
    uri = du.uri_fanout
    uri_foo, uri_bar, uri_baz = du.uri_fanout_foo, du.uri_fanout_bar, du.uri_fanout_baz
    proxy = next(multi_node.proxy_cycle())

    consumers = _open_two_consumers_per_app(proxy, uri_foo, uri_bar)
    foo1, foo2 = consumers["foo1"][0], consumers["foo2"][0]
    bar1, bar2 = consumers["bar1"][0], consumers["bar2"][0]
    baz = proxy.create_client("baz")
    baz.open(uri_baz, flags=["read"], succeed=True)
    producer = proxy.create_client("producer")
    producer.open(uri, flags=["write", "ack"], succeed=True)

    _consume_first_batch(producer, uri, baz, uri_baz, consumers)
    foo2_payloads = sorted(_payloads(foo2, uri_foo))
    bar2_payloads = sorted(_payloads(bar2, uri_bar))

    foo2.close(uri_foo, succeed=True)
    bar2.close(uri_bar, succeed=True)

    baz.check_exit_code = False
    baz.kill()
    assert proxy.outputs_substr("rejecting 0 messages because of a client crash", 10)

    foo1.confirm(uri_foo, "*", succeed=True)
    assert wait_until(lambda: sorted(_payloads(foo1, uri_foo)) == foo2_payloads, 5)
    foo1.confirm(uri_foo, "*", succeed=True)

    bar1.confirm(uri_bar, "*", succeed=True)
    assert wait_until(lambda: sorted(_payloads(bar1, uri_bar)) == bar2_payloads, 5)
    assert proxy.is_alive()


def test_confirm_after_proxy_restart(multi_node: Cluster, domain_urls: tc.DomainUrls):
    """
    baz confirms messages that the restarted proxy never pushed to it; bar's
    copy is in a redelivery list.

    1. Only baz consumes.  It receives M1 and M2, then pauses (max 0).
    2. Restart the proxy.  baz reconnects, still paused and still holding M1
       and M2.
    3. foo (max 2), bar1 and bar2 (max 1 each) open.  The primary pushes M1
       and M2 to foo and bar only.
    4. Close bar2: its message waits in bar's redelivery list, as bar1 is
       full.
    5. baz confirms M1 and M2.
    6. foo confirms both.  bar1 confirms its message: it must then get the
       one bar2 held.
    """
    du = domain_urls
    uri = du.uri_fanout
    uri_foo, uri_bar, uri_baz = du.uri_fanout_foo, du.uri_fanout_bar, du.uri_fanout_baz
    proxy = next(multi_node.proxy_cycle())

    baz = proxy.create_client("baz")
    baz.open(uri_baz, flags=["read"], succeed=True)
    producer = proxy.create_client("producer")
    producer.open(uri, flags=["write", "ack"], succeed=True)
    producer.post(uri, payload=["M1"], succeed=True)
    producer.post(uri, payload=["M2"], succeed=True)
    assert _wait_count(baz, uri_baz, 2)
    baz.configure(uri_baz, max_unconfirmed_messages=0, succeed=True)

    proxy.stop()
    proxy.wait()
    proxy.start()
    proxy.wait_until_started()
    proxy.wait_healthy()
    baz.wait_state_restored()
    producer.wait_state_restored()

    foo = proxy.create_client("foo")
    foo.open(uri_foo, flags=["read"], max_unconfirmed_messages=2, succeed=True)
    bar1 = proxy.create_client("bar1")
    bar1.open(uri_bar, flags=["read"], max_unconfirmed_messages=1, succeed=True)
    bar2 = proxy.create_client("bar2")
    bar2.open(uri_bar, flags=["read"], max_unconfirmed_messages=1, succeed=True)
    assert _wait_count(foo, uri_foo, 2)
    assert _wait_count(bar1, uri_bar, 1)
    assert _wait_count(bar2, uri_bar, 1)
    bar2_payloads = _payloads(bar2, uri_bar)

    bar2.close(uri_bar, succeed=True)

    assert sorted(_payloads(baz, uri_baz)) == ["M1", "M2"]
    baz.confirm(uri_baz, "*", succeed=True)

    foo.confirm(uri_foo, "*", succeed=True)
    bar1.confirm(uri_bar, "*", succeed=True)
    assert wait_until(lambda: _payloads(bar1, uri_bar) == bar2_payloads, 5)
    assert proxy.is_alive()
