"""
This suite of test cases exercises admin session connection and some basic
commands.
"""
import dataclasses
import json
import re
from typing import Dict, Optional, Tuple

from blazingmq.dev.it.fixtures import Cluster, single_node, order  # pylint: disable=unused-import
from blazingmq.dev.it.process.admin import AdminClient
from blazingmq.dev.it.process.client import Client
import blazingmq.dev.it.testconstants as tc


def get_endpoint(cluster: Cluster) -> Tuple[str, int]:
    endpoint: str = cluster.config.definition.nodes[0].transport.tcp.endpoint  # type: ignore

    # Extract the (host, port) pair from the config
    m = re.match(r".+://(.+):(\d+)", endpoint)  # tcp://host:port
    assert m is not None

    return str(m.group(1)), int(m.group(2))


@dataclasses.dataclass
class PostRecord:
    domain: str
    queue_name: str
    num: int

    @property
    def uri(self) -> str:
        return f"bmq://{self.domain}/{self.queue_name}"

    def append(self, other: "PostRecord") -> None:
        assert self.domain == other.domain
        assert self.queue_name == other.queue_name
        self.num += other.num


def post_n_msgs(producer: Client, task: PostRecord, posted: Optional[Dict[str, PostRecord]] = None) -> None:
    """
    Execute the specified 'task' with the specified 'producer'.
    The summary of the executed post task is appended to the optionally
    specified 'posted'.
    """
    producer.open(task.uri, flags=["write,ack"], succeed=True)
    for _ in range(task.num):
        res = producer.post(task.uri, payload=["msg"], wait_ack=True)
        assert Client.e_SUCCESS == res
    producer.close(task.uri)

    if posted is not None:
        if task.uri in posted:
            posted[task.uri].append(task)
        else:
            posted[task.uri] = task


def test_breathing(single_node: Cluster):
    """
    Test: basic admin session usage.
    - Send 'HELP' admin command and check its expected output.
    - Send invalid admin command and check if it's rejected gracefully.
    - Send 'BROKERCONFIG DUMP' command and check the basic integrity
    of the returned data.

    Concerns:
    - The broker is able to accept admin commands via TCP interface.
    - Invalid admin commands are handled gracefully.
    """
    host, port = get_endpoint(single_node)

    # Start the admin client
    admin = AdminClient()
    admin.connect(host, port)

    # Check basic "help" command
    assert (
        "This process responds to the following CMD subcommands:"
        in admin.send_admin("help")
    )

    # Check non-existing "invalid cmd" command
    assert "Unable to decode command" in admin.send_admin("invalid cmd")

    # Check more complex "brokerconfig dump" command, expect a valid json
    # with the same "port" value as the one used for this connection
    broker_config_str = admin.send_admin("brokerconfig dump")
    broker_config = json.loads(broker_config_str)

    assert broker_config["networkInterfaces"]["tcpInterface"]["port"] == port

    # Stop the admin session
    admin.stop()


def test_purge_breathing(single_node: Cluster):
    """
    Test: basic purge queue/purge domain commands usage.
    Preconditions:
    - Establish admin session with the cluster.

    Stage 1, repeat 5 times:
    - Post K messages to the PRIORITY queue.
    - Purge this queue once, expect exactly K messages purged.
    - Purge this queue again, expect exactly 0 messages purged.

    Stage 2, repeat 5 times:
    - Post K messages to queue1 in PRIORITY domain.
    - Post M messages to queue2 in the same PRIORITY domain.
    - Purge this domain once, expect exactly (K+M) messages purged.
    - Purge this domain again, expect exactly 0 messages purged.

    Stage 3, repeat 5 times:
    - Post K messages to FANOUT queue with app_id-s: "foo", "bar", "baz".
    - Purge this queue for app_id == "foo", expect exactly K messages purged.
    - Purge this queue for app_id == "bar", expect exactly K messages purged.
    - Purge this domain, expect exactly K messages purged.
    - Purge this queue for every app_id, expect 0 messages purged.
    - Purge this domain, expect exactly 0 messages purged.

    Concerns:
    - Purge queue command removes all messages stored in a queue entirely from the first call.
    - Purge domain command removes all messages stored in a domain entirely from the first call.
    - For fanout domains, purge commands remove messages precisely for their scope, be it for app_id or for entire domain.
    - Broker storage is not corrupted after purge commands and ready to store new messages.
    """

    cluster: Cluster = single_node

    proxies = cluster.proxy_cycle()
    proxy = next(proxies)
    producer: Client = proxy.create_client("producer")

    host, port = get_endpoint(cluster)

    # Start the admin client
    admin = AdminClient()
    admin.connect(host, port)

    # Stage 1: purge PRIORITY queue
    for i in range(1, 6):
        task = PostRecord(tc.DOMAIN_PRIORITY, "test_queue", num=i)
        post_n_msgs(producer, task)

        res = admin.send_admin(f"DOMAINS DOMAIN {task.domain} QUEUE {task.queue_name} PURGE *")
        assert f"Purged {task.num} message(s)" in res

        res = admin.send_admin(f"DOMAINS DOMAIN {task.domain} QUEUE {task.queue_name} PURGE *")
        assert f"Purged 0 message(s)" in res

    # Stage 2: purge PRIORITY domain
    for i in range(1, 6):
        q1_task = PostRecord(tc.DOMAIN_PRIORITY, "queue1", num=i)
        q2_task = PostRecord(tc.DOMAIN_PRIORITY, "queue2", num=i)

        post_n_msgs(producer, q1_task)
        post_n_msgs(producer, q2_task)

        res = admin.send_admin(f"DOMAINS DOMAIN {tc.DOMAIN_PRIORITY} PURGE")
        assert f"Purged {q1_task.num + q2_task.num} message(s)" in res

        res = admin.send_admin(f"DOMAINS DOMAIN {tc.DOMAIN_PRIORITY} PURGE")
        assert f"Purged 0 message(s)" in res

    # Stage 3: purge FANOUT queues and domain
    for i in range(1, 6):
        task = PostRecord(tc.DOMAIN_FANOUT, tc.TEST_QUEUE, num=i)
        post_n_msgs(producer, task)

        res = admin.send_admin(f"DOMAINS DOMAIN {task.domain} QUEUE {task.queue_name} PURGE {tc.TEST_APPIDS[0]}")
        assert f"Purged {task.num} message(s)" in res

        res = admin.send_admin(f"DOMAINS DOMAIN {task.domain} QUEUE {task.queue_name} PURGE {tc.TEST_APPIDS[1]}")
        assert f"Purged {task.num} message(s)" in res

        res = admin.send_admin(f"DOMAINS DOMAIN {task.domain} PURGE")
        assert f"Purged {task.num} message(s)" in res

        for app_id in tc.TEST_APPIDS:
            res = admin.send_admin(f"DOMAINS DOMAIN {task.domain} QUEUE {task.queue_name} PURGE {app_id}")
            assert f"Purged 0 message(s)" in res

        res = admin.send_admin(f"DOMAINS DOMAIN {task.domain} PURGE")
        assert f"Purged 0 message(s)" in res

    # Stop the admin session
    admin.stop()


def test_purge_inactive(single_node: Cluster):
    """
    Test: queue purge and domain purge also work for inactive queues.

    Stage 1: data preparation
    - Post initial messages to PRIORITY and FANOUT queues.
    - Restart the cluster to make existing queues inactive.
    - Post another part of messages: the queues taking part in this stage
    will be active.
    - Open an admin session.

    Stage 2: PRIORITY purge
    - Verify that the observed number of active queues in PRIORITY domain is expected.
    - Purge a subset of PRIORITY queues one by one using queue purge command,
    some of these queues are inactive.
    - Purge the skipped queues in the PRIORITY domain using domain purge command.
    - Cycle through all PRIORITY queues again and verify that we purged everything.

    Stage 3: FANOUT purge
    - Verify that the observed number of active queues in FANOUT domain is expected.
    - Purge a subset of FANOUT queues one by one using queue purge command with app_id
    parameter specified, some of these queues are inactive.
    - Cycle through all FANOUT queues again and verify that we purged everything.

    Concerns:
    - Purge commands work on inactive queues.
    - Purge queue with app_id specified works for inactive queues.
    """
    cluster: Cluster = single_node

    # Stage 1: data preparation
    posted: Dict[str, PostRecord] = {}
    posted_fanout: Dict[str, PostRecord] = {}

    proxies = cluster.proxy_cycle()
    proxy = next(proxies)
    producer: Client = proxy.create_client("producer")
    # Post messages to the first PRIORITY domain
    for i in range(1, 6):
        task = PostRecord(tc.DOMAIN_PRIORITY, f"inactive{i}", num=i)
        post_n_msgs(producer, task, posted)
    for i in range(1, 6):
        task = PostRecord(tc.DOMAIN_PRIORITY, f"active{i}", num=i)
        post_n_msgs(producer, task, posted)
    for i in range(1, 6):
        task = PostRecord(tc.DOMAIN_PRIORITY, f"skip_inactive{i}", num=i)
        post_n_msgs(producer, task, posted)

    # Post messages to the second FANOUT domain
    for i in range(1, 6):
        task = PostRecord(tc.DOMAIN_FANOUT, f"irrelevant_inactive{i}", num=i)
        post_n_msgs(producer, task, posted_fanout)
    producer.stop()

    # Restart the cluster, to make brokers forget in-memory state for opened queues.
    cluster.stop()
    cluster.start(wait_leader=True, wait_ready=True)
    # Note that all the queues which were opened before "stop" are now inactive.

    producer: Client = proxy.create_client("producer")
    # Post messages to the first PRIORITY domain
    for i in range(1, 6):
        # These queues existed before the cluster was restarted, and here we make
        # them active again by actively posting new messages.
        task = PostRecord(tc.DOMAIN_PRIORITY, f"active{i}", num=i)
        post_n_msgs(producer, task, posted)
    for i in range(1, 6):
        task = PostRecord(tc.DOMAIN_PRIORITY, f"skip_active{i}", num=i)
        post_n_msgs(producer, task, posted)

    # Post messages to the second FANOUT domain
    for i in range(1, 6):
        task = PostRecord(tc.DOMAIN_FANOUT, f"irrelevant_active{i}", num=i)
        post_n_msgs(producer, task, posted_fanout)
    producer.stop()

    host, port = get_endpoint(cluster)

    # Start the admin client.
    admin = AdminClient()
    admin.connect(host, port)

    # Stage 2: PRIORITY purge

    # Verify the number of active queues for PRIORITY domain.
    # Note that active queues num is less than the total number of queues.
    expected_active_num = len([uri for uri in posted if "inactive" not in uri])
    assert expected_active_num < len(posted)
    res = admin.send_admin(f"DOMAINS DOMAIN {tc.DOMAIN_PRIORITY} INFOS")
    assert f"ActiveQueues ..: {expected_active_num}" in res

    # Finally, start purging queues one by one.
    # Note that we are skipping queues named with "skip" word in PRIORITY domain and
    # also should not interact with the second FANOUT domain.
    for uri in posted:
        if "skip" in uri:
            continue

        record = posted[uri]
        res = admin.send_admin(f"DOMAINS DOMAIN {record.domain} QUEUE {record.queue_name} PURGE *")
        assert f"Purged {record.num} message(s)" in res

        record.num = 0

    # The only queues with messages in the first PRIORITY domain are named with "skip" word.
    # Now we purge them all with purge domain command.
    remaining_num = sum(record.num for record in posted.values())
    res = admin.send_admin(f"DOMAINS DOMAIN {tc.DOMAIN_PRIORITY} PURGE")
    assert f"Purged {remaining_num} message(s)" in res

    for record in posted.values():
        record.num = 0

    # Go through all queues in PRIORITY domain and check that no more messages could be purged.
    for record in posted.values():
        assert record.num == 0

        res = admin.send_admin(f"DOMAINS DOMAIN {record.domain} QUEUE {record.queue_name} PURGE *")
        assert f"Purged 0 message(s)" in res

    # Also check that purge domain for PRIORITY could not purge more messages.
    res = admin.send_admin(f"DOMAINS DOMAIN {tc.DOMAIN_PRIORITY} PURGE")
    assert f"Purged 0 message(s)" in res

    # Stage 3: FANOUT purge

    # Verify the number of active queues for FANOUT domain.
    # Note that active queues num is less than the total number of queues.
    expected_active_num = len([uri for uri in posted_fanout if "inactive" not in uri])
    assert expected_active_num < len(posted_fanout)
    res = admin.send_admin(f"DOMAINS DOMAIN {tc.DOMAIN_FANOUT} INFOS")
    assert f"ActiveQueues ..: {expected_active_num}" in res

    for record in posted_fanout.values():
        for app_id in tc.TEST_APPIDS:
            res = admin.send_admin(f"DOMAINS DOMAIN {record.domain} QUEUE {record.queue_name} PURGE {app_id}")
            assert f"Purged {record.num} message(s)" in res

        record.num = 0

        res = admin.send_admin(f"DOMAINS DOMAIN {record.domain} QUEUE {record.queue_name} PURGE *")
        assert f"Purged 0 message(s)" in res

    # Also check that purge domain for FANOUT could not purge more messages.
    res = admin.send_admin(f"DOMAINS DOMAIN {tc.DOMAIN_FANOUT} PURGE")
    assert f"Purged 0 message(s)" in res

    # Stop the admin session
    admin.stop()
