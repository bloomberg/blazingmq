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
    Open a connection for the specified 'producer' to the specified 'uri'
    and post the specified 'n' messages.
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
    - For fanout domains, purge commands remove messages precisely for their scope, be it for app_id or entire domain.
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
    for i in range(5):
        task = PostRecord(tc.DOMAIN_PRIORITY, "test_queue", i + 1)
        post_n_msgs(producer, task)

        res = admin.send_admin(f"DOMAINS DOMAIN {task.domain} QUEUE {task.queue_name} PURGE *")
        assert f"Purged {task.num} message(s)" in res

        res = admin.send_admin(f"DOMAINS DOMAIN {task.domain} QUEUE {task.queue_name} PURGE *")
        assert f"Purged 0 message(s)" in res

    # Stage 2: purge PRIORITY domain
    for i in range(5):
        q1_task = PostRecord(tc.DOMAIN_PRIORITY, "queue1", i + 1)
        q2_task = PostRecord(tc.DOMAIN_PRIORITY, "queue2", i + 1)

        post_n_msgs(producer, q1_task)
        post_n_msgs(producer, q2_task)

        res = admin.send_admin(f"DOMAINS DOMAIN {tc.DOMAIN_PRIORITY} PURGE")
        assert f"Purged {q1_task.num + q2_task.num} message(s)" in res

        res = admin.send_admin(f"DOMAINS DOMAIN {tc.DOMAIN_PRIORITY} PURGE")
        assert f"Purged 0 message(s)" in res

    # Stage 3: purge FANOUT queues and domain
    for i in range(5):
        task = PostRecord(tc.DOMAIN_FANOUT, tc.TEST_QUEUE, i + 1)
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

def test_purge_priority(single_node: Cluster):
    cluster: Cluster = single_node

    posted: Dict[str, PostRecord] = {}

    proxies = cluster.proxy_cycle()
    proxy = next(proxies)
    producer: Client = proxy.create_client("producer")
    # Post messages to the first PRIORITY domain
    for i in range(5):
        task = PostRecord(tc.DOMAIN_PRIORITY, f"inactive{i + 1}", i + 1)
        post_n_msgs(producer, task, posted)
    for i in range(5):
        task = PostRecord(tc.DOMAIN_PRIORITY, f"inactive_to_active{i + 1}", i + 1)
        post_n_msgs(producer, task, posted)
    for i in range(5):
        task = PostRecord(tc.DOMAIN_PRIORITY, f"skip_inactive{i + 1}", i + 1)
        post_n_msgs(producer, task, posted)

    # Post messages to the second FANOUT domain
    for i in range(5):
        task = PostRecord(tc.DOMAIN_FANOUT, f"irrelevant_inactive{i + 1}", i + 1)
        post_n_msgs(producer, task)
    producer.stop()

    # Now, restart the cluster.
    cluster.stop()
    cluster.start(wait_leader=True, wait_ready=True)
    # Note that all the queues which were opened before "stop" are now inactive.

    producer: Client = proxy.create_client("producer")
    # Post messages to the first PRIORITY domain
    for i in range(5):
        # These queues existed before the cluster was restarted, and here we make
        # them active again by actively posting new messages.
        task = PostRecord(tc.DOMAIN_PRIORITY, f"inactive_to_active{i + 1}", i + 1)
        post_n_msgs(producer, task, posted)
    for i in range(5):
        task = PostRecord(tc.DOMAIN_PRIORITY, f"active{i + 1}", i + 1)
        post_n_msgs(producer, task, posted)
    for i in range(5):
        task = PostRecord(tc.DOMAIN_PRIORITY, f"skip_active{i + 1}", i + 1)
        post_n_msgs(producer, task, posted)

    # Post messages to the second FANOUT domain
    for i in range(5):
        task = PostRecord(tc.DOMAIN_FANOUT, f"irrelevant_active{i + 1}", i + 1)
        post_n_msgs(producer, task)
    producer.stop()

    host, port = get_endpoint(cluster)

    # Start the admin client.
    admin = AdminClient()
    admin.connect(host, port)

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

    # Stop the admin session
    admin.stop()
