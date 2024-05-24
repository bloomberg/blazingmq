import time

import blazingmq.dev.it.testconstants as tc
from blazingmq.dev.it.fixtures import (
    Cluster,
    cluster,
    order,
)  # pylint: disable=unused-import
from blazingmq.dev.it.util import wait_until

TIMEOUT = 30

pytestmark = order(3)


def expected_header(start, count, total, size):
    return (
        f"Printing {count} message(s) [{start}-{start + count - 1} "
        f"/ {total}] (total: {size}  B)"
    )


def test_list_messages_fanout(cluster: Cluster):
    leader = cluster.last_known_leader
    proxies = cluster.proxy_cycle()

    producer = next(proxies).create_client("producer")
    producer.open(tc.URI_FANOUT, flags=["write,ack"], succeed=True)

    for i in range(1, 4):
        producer.post(
            tc.URI_FANOUT,
            ["x" * 10 * i],
            wait_ack=True,
            succeed=True,
        )

    consumer = next(proxies).create_client("consumer")

    for i, uri in enumerate([tc.URI_FANOUT_FOO, tc.URI_FANOUT_BAR, tc.URI_FANOUT_BAZ]):
        consumer.open(uri, flags=["read"], succeed=True)

    assert wait_until(lambda: len(consumer.list(block=True)) == 9, TIMEOUT)
    guids = [msg.guid for msg in consumer.list(tc.URI_FANOUT_FOO, block=True)]

    for i, uri in enumerate([tc.URI_FANOUT_FOO, tc.URI_FANOUT_BAR, tc.URI_FANOUT_BAZ]):
        consumer.confirm(uri, guids[i], succeed=True)

    leader.list_messages(tc.DOMAIN_FANOUT, tc.TEST_QUEUE, 0, 3)
    assert leader.outputs_substr(expected_header(0, 3, 3, 60), TIMEOUT)
    assert leader.outputs_substr("10", TIMEOUT)
    assert leader.outputs_substr("20", TIMEOUT)
    assert leader.outputs_substr("30", TIMEOUT)

    leader.list_messages(tc.DOMAIN_FANOUT, tc.TEST_QUEUE, 0, "UNLIMITED")
    assert leader.outputs_substr(expected_header(0, 3, 3, 60), TIMEOUT)
    assert leader.outputs_substr("10", TIMEOUT)
    assert leader.outputs_substr("20", TIMEOUT)
    assert leader.outputs_substr("30", TIMEOUT)

    leader.list_messages(tc.DOMAIN_FANOUT, tc.TEST_QUEUE, 1, 1)
    assert leader.outputs_substr(expected_header(1, 1, 3, 20), TIMEOUT)
    assert leader.outputs_substr("20", TIMEOUT)

    leader.list_messages(tc.DOMAIN_FANOUT, tc.TEST_QUEUE, -1, 1)
    assert leader.outputs_substr(expected_header(2, 1, 3, 30), TIMEOUT)
    assert leader.outputs_substr("30", TIMEOUT)

    leader.list_messages(tc.DOMAIN_FANOUT, tc.TEST_QUEUE, 1, -1)
    assert leader.outputs_substr(expected_header(0, 1, 3, 10), TIMEOUT)
    assert leader.outputs_substr("10", TIMEOUT)

    leader.list_messages(tc.DOMAIN_FANOUT, tc.TEST_QUEUE, -1, -1)
    assert leader.outputs_substr(expected_header(1, 1, 3, 20), TIMEOUT)
    assert leader.outputs_substr("20", TIMEOUT)

    leader.list_messages(tc.DOMAIN_FANOUT, tc.TEST_QUEUE, 0, "UNLIMITED garbage")
    assert leader.outputs_substr("Error processing command", TIMEOUT)

    time.sleep(2)  # Allow previous confirmations to complete
    for i, appid in enumerate(["foo", "bar", "baz"]):
        leader.list_messages(
            tc.DOMAIN_FANOUT, tc.TEST_QUEUE, 0, "UNLIMITED", appid=appid
        )
        assert leader.outputs_substr(
            expected_header(0, 2, 2, 60 - (i + 1) * 10), TIMEOUT
        )
        for j in range(0, 3):
            if i != j:
                assert leader.outputs_regex(f"{guids[j]}.*{(j + 1) * 10}", TIMEOUT)

    leader.list_messages(
        tc.DOMAIN_FANOUT, tc.TEST_QUEUE, 0, "UNLIMITED", appid="pikachu"
    )
    assert leader.outputs_substr("Invalid 'LIST' command: invalid APPID", TIMEOUT)


def test_list_messages_priority(cluster: Cluster):
    leader = cluster.last_known_leader
    proxies = cluster.proxy_cycle()

    producer = next(proxies).create_client("producer")
    producer.open(tc.URI_PRIORITY, flags=["write,ack"], succeed=True)

    for i in range(1, 4):
        producer.post(
            tc.URI_PRIORITY,
            ["x" * 10 * i],
            wait_ack=True,
            succeed=True,
        )

    client = next(proxies).create_client("consumer")
    client.open(tc.URI_PRIORITY, flags=["read"], succeed=True)

    assert wait_until(lambda: len(client.list(block=True)) == 3, TIMEOUT)
    _ = [msg.guid for msg in client.list(uri=tc.URI_PRIORITY, block=True)]

    leader.list_messages(tc.DOMAIN_PRIORITY, tc.TEST_QUEUE, 0, 3)
    assert leader.outputs_substr(expected_header(0, 3, 3, 60), TIMEOUT)
    assert leader.outputs_substr("10", TIMEOUT)
    assert leader.outputs_substr("20", TIMEOUT)
    assert leader.outputs_substr("30", TIMEOUT)

    leader.list_messages(tc.DOMAIN_PRIORITY, tc.TEST_QUEUE, 0, "UNLIMITED")
    assert leader.outputs_substr(expected_header(0, 3, 3, 60), TIMEOUT)
    assert leader.outputs_substr("10", TIMEOUT)
    assert leader.outputs_substr("20", TIMEOUT)
    assert leader.outputs_substr("30", TIMEOUT)

    leader.list_messages(tc.DOMAIN_PRIORITY, tc.TEST_QUEUE, 1, 1)
    assert leader.outputs_substr(expected_header(1, 1, 3, 20), TIMEOUT)
    assert leader.outputs_substr("20", TIMEOUT)

    leader.list_messages(tc.DOMAIN_PRIORITY, tc.TEST_QUEUE, -1, 1)
    assert leader.outputs_substr(expected_header(2, 1, 3, 30), TIMEOUT)
    assert leader.outputs_substr("30", TIMEOUT)

    leader.list_messages(tc.DOMAIN_PRIORITY, tc.TEST_QUEUE, 1, -1)
    assert leader.outputs_substr(expected_header(0, 1, 3, 10), TIMEOUT)
    assert leader.outputs_substr("10", TIMEOUT)

    leader.list_messages(tc.DOMAIN_PRIORITY, tc.TEST_QUEUE, -1, -1)
    assert leader.outputs_substr(expected_header(1, 1, 3, 20), TIMEOUT)
    assert leader.outputs_substr("20", TIMEOUT)

    leader.list_messages(tc.DOMAIN_PRIORITY, tc.TEST_QUEUE, 0, "UNLIMITED garbage")
    assert leader.outputs_substr("Error processing command", TIMEOUT)

    leader.list_messages(
        tc.DOMAIN_PRIORITY, tc.TEST_QUEUE, 0, "UNLIMITED", appid="pikachu"
    )
    assert leader.outputs_substr("Invalid 'LIST' command: invalid APPID", TIMEOUT)
