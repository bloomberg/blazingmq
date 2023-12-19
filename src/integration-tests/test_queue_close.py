"""
Integration test that tests closing a queue when the broker is down.
"""

import bmq.dev.it.testconstants as tc
from bmq.dev.it.fixtures import (  # pylint: disable=unused-import
    Cluster,
    local_cluster,
    standard_cluster,
    start_cluster,
    tweak,
)
from bmq.dev.it.process.client import Client


def test_close_queue(local_cluster: Cluster):
    assert local_cluster.is_local

    # Start a consumer and open a queue
    proxies = local_cluster.proxy_cycle()
    consumer = next(proxies).create_client("consumer")
    consumer.open(tc.URI_PRIORITY, flags=["read"], succeed=True)

    # Shutdown the broker
    leader = local_cluster.last_known_leader
    leader.stop()

    # Try to close the queue
    consumer.wait_connection_lost()

    # Pending queue can be closed successfully
    assert consumer.close(tc.URI_PRIORITY, block=True) == Client.e_SUCCESS


@tweak.domain.max_consumers(1)
@start_cluster(False)
def test_close_while_reopening(standard_cluster: Cluster):
    """
    DRQS 169125974.  Closing queue while reopen response is pending should
    not result in a dangling handle.
    """

    cluster = standard_cluster

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

    consumer1.open(tc.URI_PRIORITY, flags=["read"], succeed=True)

    assert west1 == cluster.process(westp.get_active_node())

    # Should fail
    consumer2.open(tc.URI_PRIORITY, flags=["read"], succeed=False)

    east1.set_quorum(3)
    east2.set_quorum(3)

    # Stop the primary.  The proxy will pick new active node and re-issue
    # Open request but the new active node (either r1 or r2) will not
    # respond because there is no quorum (3) for new primary

    west1.exit_gracefully()
    # Wait for the subprocess to terminate
    west1.wait()

    # Now send Close request which the proxy should park
    consumer1.close(tc.URI_PRIORITY, block=False)

    # Restore the quorum.  Proxy should send the parked Close _after_
    # receiving Reopen response (and after sending Configure request)
    west2 = cluster.start_node("west2")
    west2.wait_status(wait_leader=True, wait_ready=True)

    # Should succeed now!
    consumer2.open(tc.URI_PRIORITY, flags=["read"], succeed=True)

    consumer3 = westp.create_client("consumer3")
    # Should fail
    consumer3.open(tc.URI_PRIORITY, flags=["read"], succeed=False)


def test_close_open(standard_cluster: Cluster):
    """
    DRQS 169326671.  Close, followed by Open with a different subId.
    """
    proxies = standard_cluster.proxy_cycle()
    # pick proxy in datacenter opposite to the primary's
    next(proxies)
    proxy = next(proxies)
    consumer1 = proxy.create_client("consumer1")
    consumer1.open(tc.URI_FANOUT_FOO, flags=["read"], succeed=True)

    consumer2 = proxy.create_client("consumer2")
    consumer2.open(tc.URI_FANOUT_BAR, flags=["read"], succeed=True)

    leader = standard_cluster.last_known_leader
    consumer3 = leader.create_client("consumer3")
    consumer3.open(tc.URI_FANOUT_FOO, flags=["read"], succeed=True)

    consumer1.close(tc.URI_FANOUT_FOO, succeed=True)
    consumer1.open(tc.URI_FANOUT_FOO, flags=["read"], succeed=True)


@tweak.domain.max_consumers(1)
@tweak.cluster.queue_operations.reopen_retry_interval_ms(1234)
def test_close_while_retrying_reopen(standard_cluster: Cluster):
    """
    DRQS 170043950.  Trigger reopen failure causing proxy to retry on
    timeout. While waiting, close the queue and make sure, the retry
    accounts for that close.
    """

    proxies = standard_cluster.proxy_cycle()
    # pick proxy in datacenter opposite to the primary's
    next(proxies)
    proxy1 = next(proxies)
    proxy2 = next(proxies)

    producer = proxy1.create_client("producer")
    consumer1 = proxy1.create_client("consumer1")
    consumer2 = proxy2.create_client("consumer2")

    producer.open(tc.URI_PRIORITY, flags=["write,ack"], succeed=True)
    consumer1.open(tc.URI_PRIORITY, flags=["read"], succeed=True)

    active_node = standard_cluster.process(proxy1.get_active_node())
    proxy1.suspend()

    # this is to trigger reopen when proxy1 resumes
    active_node.force_stop()

    # this is to make the reopen fail
    consumer2.open(tc.URI_PRIORITY, flags=["read"], succeed=True)

    # trigger reopen
    proxy1.resume()

    # reopen should fail because of consumer2
    assert proxy1.capture(
        r"queue reopen-request failed. .*, error response: \[ rId = \d+ choice = \[ status = \[ category = E_UNKNOWN code = -1 message = \"Client would exceed the limit of 1 consumer\(s\)\" \] \] \]. Attempt number was: 1. Attempting again after 1234 milliseconds",
        timeout=10,
    )

    # this should stop reopening consumer
    consumer1.close(tc.URI_PRIORITY, succeed=True)

    # this is to make (re)open to succeed
    consumer2.close(tc.URI_PRIORITY, succeed=True)

    # next reopen should not have readCount
    assert proxy1.capture(
        r"Sending request to .* \[request: \[ rId = \d+ choice = \[ openQueue = \[ handleParameters = \[ .* flags = 4 readCount = 0 writeCount = 1 adminCount = 0 \] \] \] \]",
        timeout=10,
    )

    # verify new open
    consumer1.open(tc.URI_PRIORITY, flags=["read"], succeed=True)
