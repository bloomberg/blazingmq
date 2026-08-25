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
Integration test that tests closing a queue when the broker is down.
"""

import time

import blazingmq.dev.it.testconstants as tc
from blazingmq.dev.it.fixtures import (
    Cluster,
    start_cluster,
    tweak,
)
from blazingmq.dev.it.process.client import Client


def test_close_queue(single_node: Cluster, domain_urls: tc.DomainUrls):
    assert single_node.is_single_node

    du = domain_urls

    # Start a consumer and open a queue
    proxies = single_node.proxy_cycle()
    consumer = next(proxies).create_client("consumer")
    consumer.open(du.uri_priority, flags=["read"], succeed=True)

    # Shutdown the broker
    leader = single_node.last_known_leader
    leader.stop()

    # Try to close the queue
    consumer.wait_connection_lost()

    # Pending queue can be closed successfully
    assert consumer.close(du.uri_priority, block=True) == Client.e_SUCCESS


@tweak.domain.max_consumers(1)
@start_cluster(False)
def test_close_while_reopening(multi_node: Cluster, domain_urls: tc.DomainUrls):
    """
    Ticket 169125974.  Closing queue while reopen response is pending should
    not result in a dangling handle.
    """
    du = domain_urls

    cluster = multi_node

    west1 = cluster.start_node("west1")
    # make it primary
    west1.set_quorum(1)

    # Two replicas for a total of 3 nodes
    east1 = cluster.start_node("east1")
    east1.set_quorum(5)

    east2 = cluster.start_node("east2")
    east2.set_quorum(5)

    east1.wait_status(wait_leader=True, wait_ready=True)

    # west1 is the primary
    assert west1 == east1.last_known_leader

    # One proxy connected to the primary
    westp = cluster.start_proxy("westp")

    consumer1 = westp.create_client("consumer1")
    consumer2 = westp.create_client("consumer2")

    consumer1.open(du.uri_priority, flags=["read"], succeed=True)

    assert west1 == cluster.process(westp.get_active_node())

    # Should fail
    consumer2.open(du.uri_priority, flags=["read"], succeed=False)

    east1.set_quorum(3)
    east2.set_quorum(3)

    # Stop the primary.  The proxy will pick new active node and re-issue
    # Open request but the new active node (either r1 or r2) will not
    # respond because there is no quorum (3) for new primary

    west1.exit_gracefully()
    # Wait for the subprocess to terminate
    west1.wait()

    # Now send Close request which the proxy should park
    consumer1.close(du.uri_priority, block=False)

    # Restore the quorum.  Proxy should send the parked Close _after_
    # receiving Reopen response (and after sending Configure request)
    west2 = cluster.start_node("west2")
    west2.wait_status(wait_leader=True, wait_ready=True)

    # Should succeed now!
    consumer2.open(du.uri_priority, flags=["read"], succeed=True)

    consumer3 = westp.create_client("consumer3")
    # Should fail
    consumer3.open(du.uri_priority, flags=["read"], succeed=False)


def test_close_open(multi_node: Cluster, domain_urls: tc.DomainUrls):
    """
    Ticket 169326671.  Close, followed by Open with a different subId.
    """
    du = domain_urls
    proxies = multi_node.proxy_cycle()
    # pick proxy in datacenter opposite to the primary's
    next(proxies)
    proxy = next(proxies)
    consumer1 = proxy.create_client("consumer1")
    consumer1.open(du.uri_fanout_foo, flags=["read"], succeed=True)

    consumer2 = proxy.create_client("consumer2")
    consumer2.open(du.uri_fanout_bar, flags=["read"], succeed=True)

    leader = multi_node.last_known_leader
    consumer3 = leader.create_client("consumer3")
    consumer3.open(du.uri_fanout_foo, flags=["read"], succeed=True)

    consumer1.close(du.uri_fanout_foo, succeed=True)
    consumer1.open(du.uri_fanout_foo, flags=["read"], succeed=True)


@tweak.domain.max_consumers(1)
@tweak.cluster.queue_operations.reopen_retry_interval_ms(1234)
def test_close_while_retrying_reopen(multi_node: Cluster, domain_urls: tc.DomainUrls):
    """
    Ticket 170043950.  Trigger reopen failure causing proxy to retry on
    timeout. While waiting, close the queue and make sure, the retry
    accounts for that close.
    """

    uri_priority = domain_urls.uri_priority
    proxies = multi_node.proxy_cycle()
    # pick proxy in datacenter opposite to the primary's
    next(proxies)
    proxy1 = next(proxies)
    proxy2 = next(proxies)

    producer = proxy1.create_client("producer")
    consumer1 = proxy1.create_client("consumer1")
    consumer2 = proxy2.create_client("consumer2")

    producer.open(uri_priority, flags=["write,ack"], succeed=True)
    consumer1.open(uri_priority, flags=["read"], succeed=True)

    active_node = multi_node.process(proxy1.get_active_node())
    proxy1.suspend()

    # this is to trigger reopen when proxy1 resumes
    active_node.force_stop()

    # this is to make the reopen fail
    consumer2.open(uri_priority, flags=["read"], succeed=True)

    # trigger reopen
    proxy1.resume()

    # reopen should fail because of consumer2
    assert proxy1.capture(
        r"queue reopen-request failed. .*, error response: \[ rId = \d+ choice = \[ status = \[ category = E_UNKNOWN code = -1 message = \"Client would exceed the limit of 1 consumer\(s\)\" \] \] \]. Attempt number was: 1. Attempting again after 1234 milliseconds",
        timeout=10,
    )

    # this should stop reopening consumer
    consumer1.close(uri_priority, succeed=True)

    # this is to make (re)open to succeed
    consumer2.close(uri_priority, succeed=True)

    # next reopen should not have readCount
    assert proxy1.capture(
        r"Sending request to .* \[request: \[ rId = \d+ choice = \[ openQueue = \[ handleParameters = \[ .* flags = 4 readCount = 0 writeCount = 1 adminCount = 0 \] \] \] \]",
        timeout=10,
    )

    # verify new open
    consumer1.open(uri_priority, flags=["read"], succeed=True)


@tweak.cluster.queue_operations.configure_timeout_ms(500)
@tweak.cluster.queue_operations.close_timeout_ms(500)
@tweak.cluster.queue_operations.keepalive_duration_ms(100)
def test_upstream_replies_racing_client_teardown(
    multi_node: Cluster, domain_urls: tc.DomainUrls
):
    """
    A proxy answers its clients only once its upstream has answered it, so an
    open-queue reply can arrive after the client that asked for it is already
    gone.  Such a handle is rolled back as soon as it is created, which can
    retire it -- and the queue holding it -- while the de-configure the proxy
    issued for that same handle is still outstanding upstream.  The reply to
    that de-configure then has nothing left to apply, and it arrives on the
    thread that received it rather than on the one that owns the queue.
    Applying it, or discarding it, is the queue dispatcher thread's job either
    way.

    Each round parks a batch of opens on an upstream that cannot answer, then
    releases the batch at the same moment the clients that issued it are
    destroyed, so the replies land while the proxy is dismantling their
    handles.  Every broker must survive every round.
    """
    # The two things that have to overlap are a reply arriving and a client
    # being torn down, and neither side can be pinned down exactly.  Rounds are
    # cheap and each one throws a whole batch of replies into the window.
    rounds = 6

    # Clients destroyed together, so that later teardowns overlap the replies
    # still owed to earlier ones.
    clients_per_round = 3

    # Opens each client leaves outstanding when it is destroyed.  The batch has
    # to be long enough to still be arriving once teardown has started.
    queues_per_client = 15

    cluster = multi_node
    du = domain_urls

    proxies = cluster.proxy_cycle()
    # pick the proxy in the data center opposite to the primary's
    next(proxies)
    proxy = next(proxies)

    for rnd in range(rounds):
        # Each round uses queues of its own, so a round's queues are retired
        # while that round's replies may still be travelling.
        def held(index, rnd=rnd):
            return f"{du.uri_priority}r{rnd}h{index}"

        def parked(client_index, index, rnd=rnd):
            return f"{du.uri_priority}r{rnd}c{client_index}q{index}"

        clients = []
        for i in range(clients_per_round):
            client = proxy.create_client(f"consumer{rnd}n{i}")
            # An established handle, so that losing this client also leaves a
            # de-configure outstanding upstream.
            client.open(held(i), flags=["read"], succeed=True)
            clients.append(client)

        # The proxy names a session after the pid of its client, which is how
        # this round tells its own teardowns from any other round's.
        pids = [client.pid for client in clients]

        active_node = cluster.process(proxy.get_active_node())

        # Nothing the proxy forwards from here on can be answered.
        active_node.suspend()

        for i, client in enumerate(clients):
            for q in range(queues_per_client):
                client.open(parked(i, q), flags=["read"], block=False)

        # Release the batch and lose the clients at the same time.  Which of
        # the two lands first decides whether a reply meets a live handle, a
        # retired one, or a queue that is already gone, so alternate the order.
        if rnd % 2:
            active_node.resume()
            for client in clients:
                client.force_stop()
        else:
            for client in clients:
                client.force_stop()
            active_node.resume()

        # Every client of this round reached teardown.  Each session that goes
        # away logs this line, and scanning consumes the output, so claim one
        # line per client: a line left behind here would be matched by a later
        # round, which would then be looking at a teardown that is not its own.
        assert all(
            proxy.capture_n(
                [rf":{pid}\b.*Dropped \d+ queue handles" for pid in pids],
                timeout=10,
            )
        )

        if rnd == 0:
            # The batch really did outlive its requesters: replies are being
            # applied to handles nobody is waiting for any more.
            assert proxy.capture(
                r"OpenQueueConfirmationCookie released without", timeout=10
            )

        # Let the queues nobody holds any more be collected while the rest of
        # the batch is still being digested.
        time.sleep(1)

        # The proxy digested the whole batch and still serves clients.
        witness = proxy.create_client(f"witness{rnd}")
        witness.open(held(0), flags=["write,ack"], succeed=True)
        witness.stop()

        assert proxy.is_alive()
        for node in cluster.nodes():
            assert node.is_alive()
