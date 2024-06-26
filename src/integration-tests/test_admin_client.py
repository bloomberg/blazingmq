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
This suite of test cases exercises admin session connection and some basic
commands.
"""
import dataclasses
import json
from typing import Dict, Optional

import blazingmq.dev.it.testconstants as tc
from blazingmq.dev.it.fixtures import (  # pylint: disable=unused-import
    Cluster,
    order,
    single_node,
    tweak,
)
from blazingmq.dev.it.process.admin import AdminClient
from blazingmq.dev.it.process.client import Client

pytestmark = order(1)


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


def post_n_msgs(
    producer: Client, task: PostRecord, posted: Optional[Dict[str, PostRecord]] = None
) -> None:
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


def extract_stats(admin_response: str) -> dict:
    """
    Extracts the dictionary containing stats from the specified 'admin_response'.
    Note that due to xsd schema limitations it's not possible to make a schema for a json
    containing random keys. Due to this, the stats encoding is not ideal:
    - The outer layer is a 'str' response from the admin session
    - Next layer is a 'dict' containing "stats" field with 'str' text
    - The last layer is another 'dict' containing the stats itself
    """
    d1 = json.loads(admin_response)
    d2 = json.loads(d1["stats"])
    return d2


class GreaterThan:
    def __init__(self, value):
        self.value = value


def expect_same_structure(obj, expected) -> None:
    """
    Check if the specified 'obj' dictionary has the same structure as the specified 'expected' dictionary.
    Assert on failure.
    """

    if isinstance(expected, dict):
        assert isinstance(obj, dict)
        assert expected.keys() == obj.keys()
        for key in expected:
            expect_same_structure(obj[key], expected[key])
    elif isinstance(expected, list):
        assert isinstance(obj, list)
        assert len(expected) == len(obj)
        for obj2, expected2 in zip(obj, expected):
            expect_same_structure(obj2, expected2)
    elif isinstance(expected, str):
        assert isinstance(obj, str)
        assert expected == obj
    else:
        assert isinstance(obj, int)
        if isinstance(expected, GreaterThan):
            assert obj > expected.value
        else:
            assert isinstance(expected, int)
            assert obj == expected


def test_breathing(single_node: Cluster) -> None:
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
    host, port = single_node.admin_endpoint

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


@tweak.broker.app_config.stats.app_id_tag_domains([tc.DOMAIN_FANOUT])
def test_stats(single_node: Cluster) -> None:
    """
    Test: queue metrics via admin command.
    Preconditions:
    - Establish admin session with the cluster.

    Stage 1: check stats after posting messages
    - Open a producer
    - Post messages to a fanout queue
    - Verify stats acquired via admin command with the expected stats

    Stage 2: check stats after confirming messages
    - Open a consumer for each appId
    - Confirm a portion of messages for each consumer
    - Verify stats acquired via admin command with the expected stats

    Stage 3: check too-often stats safeguard
    - Send several 'stat show' requests
    - Verify that the admin session complains about too often stat request

    Concerns:
    - The broker is able to report queue metrics for fanout queue.
    - Safeguarding mechanism prevents from getting stats too often.

    Note: when the metrics collection evolves, it might be necessary to update
          the expected stats in the test. To do so, it's possible to copy values
          observed in the debugger for this test and paste them as a dictionary,
          while replacing non-fixed parameters such as time intervals with a
          condition placeholders (see 'GreaterThan').
    """

    # Preconditions
    admin = AdminClient()
    admin.connect(*single_node.admin_endpoint)

    # Stage 1: check stats after posting messages
    cluster: Cluster = single_node
    proxies = cluster.proxy_cycle()
    proxy = next(proxies)
    producer: Client = proxy.create_client("producer")

    task = PostRecord(tc.DOMAIN_FANOUT, "test_stats", num=32)
    post_n_msgs(producer, task)

    stats = extract_stats(admin.send_admin("encoding json_pretty stat show"))
    queue_stats = stats["domainQueues"]["domains"][tc.DOMAIN_FANOUT][task.uri]

    expected_stats_after_post = {
        "appIds": {
            "bar": {
                "values": {
                    "queue_confirm_time_max": 0,
                    "queue_confirm_time_avg": 0,
                    "queue_nack_msgs": 0,
                    "queue_ack_time_max": 0,
                    "queue_ack_msgs": 0,
                    "queue_confirm_msgs": 0,
                    "queue_push_bytes": 0,
                    "queue_consumers_count": 0,
                    "queue_producers_count": 0,
                    "queue_push_msgs": 0,
                    "queue_ack_time_avg": 0,
                    "queue_put_bytes": 0,
                    "queue_put_msgs": 0,
                }
            },
            "baz": {
                "values": {
                    "queue_confirm_time_max": 0,
                    "queue_confirm_time_avg": 0,
                    "queue_nack_msgs": 0,
                    "queue_ack_time_max": 0,
                    "queue_ack_msgs": 0,
                    "queue_confirm_msgs": 0,
                    "queue_push_bytes": 0,
                    "queue_consumers_count": 0,
                    "queue_producers_count": 0,
                    "queue_push_msgs": 0,
                    "queue_ack_time_avg": 0,
                    "queue_put_bytes": 0,
                    "queue_put_msgs": 0,
                }
            },
            "foo": {
                "values": {
                    "queue_confirm_time_max": 0,
                    "queue_confirm_time_avg": 0,
                    "queue_nack_msgs": 0,
                    "queue_ack_time_max": 0,
                    "queue_ack_msgs": 0,
                    "queue_confirm_msgs": 0,
                    "queue_push_bytes": 0,
                    "queue_consumers_count": 0,
                    "queue_producers_count": 0,
                    "queue_push_msgs": 0,
                    "queue_ack_time_avg": 0,
                    "queue_put_bytes": 0,
                    "queue_put_msgs": 0,
                }
            },
        },
        "values": {
            "queue_confirm_time_max": 0,
            "queue_confirm_time_avg": 0,
            "queue_nack_msgs": 0,
            "queue_ack_time_max": GreaterThan(0),
            "queue_ack_msgs": 32,
            "queue_confirm_msgs": 0,
            "queue_push_bytes": 0,
            "queue_consumers_count": 0,
            "queue_producers_count": 0,
            "queue_push_msgs": 0,
            "queue_ack_time_avg": GreaterThan(0),
            "queue_put_bytes": 96,
            "queue_put_msgs": 32,
        },
    }
    expect_same_structure(queue_stats, expected_stats_after_post)

    # Stage 2: check stats after confirming messages
    consumer_foo: Client = proxy.create_client("consumer_foo")
    consumer_foo.open(f"{task.uri}?id=foo", flags=["read"], succeed=True)
    consumer_foo.confirm(f"{task.uri}?id=foo", "*", succeed=True)

    consumer_bar: Client = proxy.create_client("consumer_bar")
    consumer_bar.open(f"{task.uri}?id=bar", flags=["read"], succeed=True)
    consumer_bar.confirm(f"{task.uri}?id=bar", "+22", succeed=True)

    consumer_baz: Client = proxy.create_client("consumer_baz")
    consumer_baz.open(f"{task.uri}?id=baz", flags=["read"], succeed=True)
    consumer_baz.confirm(f"{task.uri}?id=baz", "+11", succeed=True)

    stats = extract_stats(admin.send_admin("encoding json_pretty stat show"))
    queue_stats = stats["domainQueues"]["domains"][tc.DOMAIN_FANOUT][task.uri]

    expected_stats_after_confirm = {
        "appIds": {
            "bar": {
                "values": {
                    "queue_confirm_time_max": GreaterThan(0),
                    "queue_confirm_time_avg": GreaterThan(0),
                    "queue_nack_msgs": 0,
                    "queue_ack_time_max": 0,
                    "queue_ack_msgs": 0,
                    "queue_confirm_msgs": 0,
                    "queue_push_bytes": 0,
                    "queue_consumers_count": 0,
                    "queue_producers_count": 0,
                    "queue_push_msgs": 0,
                    "queue_ack_time_avg": 0,
                    "queue_put_bytes": 0,
                    "queue_put_msgs": 0,
                }
            },
            "baz": {
                "values": {
                    "queue_confirm_time_max": GreaterThan(0),
                    "queue_confirm_time_avg": GreaterThan(0),
                    "queue_nack_msgs": 0,
                    "queue_ack_time_max": 0,
                    "queue_ack_msgs": 0,
                    "queue_confirm_msgs": 0,
                    "queue_push_bytes": 0,
                    "queue_consumers_count": 0,
                    "queue_producers_count": 0,
                    "queue_push_msgs": 0,
                    "queue_ack_time_avg": 0,
                    "queue_put_bytes": 0,
                    "queue_put_msgs": 0,
                }
            },
            "foo": {
                "values": {
                    "queue_confirm_time_max": GreaterThan(0),
                    "queue_confirm_time_avg": GreaterThan(0),
                    "queue_nack_msgs": 0,
                    "queue_ack_time_max": 0,
                    "queue_ack_msgs": 0,
                    "queue_confirm_msgs": 0,
                    "queue_push_bytes": 0,
                    "queue_consumers_count": 0,
                    "queue_producers_count": 0,
                    "queue_push_msgs": 0,
                    "queue_ack_time_avg": 0,
                    "queue_put_bytes": 0,
                    "queue_put_msgs": 0,
                }
            },
        },
        "values": {
            "queue_confirm_time_max": GreaterThan(0),
            "queue_confirm_time_avg": GreaterThan(0),
            "queue_nack_msgs": 0,
            "queue_ack_time_max": GreaterThan(0),
            "queue_ack_msgs": 32,
            "queue_confirm_msgs": 65,
            "queue_push_bytes": 288,
            "queue_consumers_count": 3,
            "queue_producers_count": 0,
            "queue_push_msgs": 96,
            "queue_ack_time_avg": GreaterThan(0),
            "queue_put_bytes": 96,
            "queue_put_msgs": 32,
        },
    }
    expect_same_structure(queue_stats, expected_stats_after_confirm)

    consumer_foo.close(f"{task.uri}?id=foo")
    consumer_bar.close(f"{task.uri}?id=bar")
    consumer_baz.close(f"{task.uri}?id=baz")

    # Stage 3: check too-often stats safeguard
    for i in range(5):
        admin.send_admin("encoding json_pretty stat show")
    res = admin.send_admin("encoding json_pretty stat show")
    obj = json.loads(res)

    too_often_snapshots_message = {
        "error": {
            "message": "Cannot save the recent snapshot, trying to make snapshots too often"
        }
    }
    expect_same_structure(obj, too_often_snapshots_message)

    admin.stop()


def test_admin_encoding(single_node: Cluster) -> None:
    """
    Test: admin commands output format.
    Preconditions:
    - Establish admin session with the cluster.

    Stage 1:
    - Send json and plaintext admin commands with 'TEXT' output encoding.
    - Expect the received responses in text format.

    Stage 2:
    - Send json and plaintext admin commands with 'JSON_COMPACT' output encoding.
    - Expect the received responses in compact json format.

    Stage 3:
    - Send json and plaintext admin commands with 'JSON_PRETTY' output encoding.
    - Expect the received responses in pretty json format.

    Stage 4:
    - Send json and plaintext admin commands with incorrect output encoding.
    - Expect the received responses with decode error message.

    Stage 5:
    - Send json and plaintext admin commands without output encoding option.
    - Expect the received responses in text format.

    Concerns:
    - It's possible to pass output encoding option to both json and plaintext encoded admin commands.
    - TEXT, JSON_COMPACT, JSON_PRETTY encoding formats are correctly supported.
    - Commands with incorrect encoding are handled with decode error.
    - Commands without encoding return text output for backward compatibility.
    """

    def is_compact(json_str: str) -> bool:
        return "    " not in json_str

    # Start the admin client
    admin = AdminClient()
    admin.connect(*single_node.admin_endpoint)

    # Stage 1: encode as TEXT
    cmds = [json.dumps({"help": {}, "encoding": "TEXT"}), "ENCODING TEXT HELP"]
    for cmd in cmds:
        res = admin.send_admin(cmd)
        assert "This process responds to the following CMD subcommands:" in res

    # Stage 2: encode as JSON_COMPACT
    cmds = [
        json.dumps({"help": {}, "encoding": "JSON_COMPACT"}),
        "ENCODING JSON_COMPACT HELP",
    ]
    for cmd in cmds:
        res = admin.send_admin(cmd)
        assert "help" in json.loads(res)
        assert is_compact(res)

    # Stage 3: encode as JSON_PRETTY
    cmds = [
        json.dumps({"help": {}, "encoding": "JSON_PRETTY"}),
        "ENCODING JSON_PRETTY HELP",
    ]
    for cmd in cmds:
        res = admin.send_admin(cmd)
        assert "help" in json.loads(res)
        assert not is_compact(res)

    # Stage 4: incorrect encoding
    cmds = [
        json.dumps({"help": {}, "encoding": "INCORRECT"}),
        "ENCODING INCORRECT HELP",
        "ENCODING HELP",
        "ENCODING TEXT",
        "ENCODING",
    ]
    for cmd in cmds:
        res = admin.send_admin(cmd)
        assert "Unable to decode command" in res

    # Stage 5: no encoding
    cmds = [json.dumps({"help": {}}), "HELP"]
    for cmd in cmds:
        res = admin.send_admin(cmd)
        assert "This process responds to the following CMD subcommands:" in res

    # Stop the admin session
    admin.stop()


def test_purge_breathing(single_node: Cluster) -> None:
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

    # Start the admin client
    admin = AdminClient()
    admin.connect(*cluster.admin_endpoint)

    # Stage 1: purge PRIORITY queue
    for i in range(1, 6):
        task = PostRecord(tc.DOMAIN_PRIORITY, "test_queue", num=i)
        post_n_msgs(producer, task)

        res = admin.send_admin(
            f"DOMAINS DOMAIN {task.domain} QUEUE {task.queue_name} PURGE *"
        )
        assert f"Purged {task.num} message(s)" in res

        res = admin.send_admin(
            f"DOMAINS DOMAIN {task.domain} QUEUE {task.queue_name} PURGE *"
        )
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

        res = admin.send_admin(
            f"DOMAINS DOMAIN {task.domain} QUEUE {task.queue_name} PURGE {tc.TEST_APPIDS[0]}"
        )
        assert f"Purged {task.num} message(s)" in res

        res = admin.send_admin(
            f"DOMAINS DOMAIN {task.domain} QUEUE {task.queue_name} PURGE {tc.TEST_APPIDS[1]}"
        )
        assert f"Purged {task.num} message(s)" in res

        res = admin.send_admin(f"DOMAINS DOMAIN {task.domain} PURGE")
        assert f"Purged {task.num} message(s)" in res

        for app_id in tc.TEST_APPIDS:
            res = admin.send_admin(
                f"DOMAINS DOMAIN {task.domain} QUEUE {task.queue_name} PURGE {app_id}"
            )
            assert f"Purged 0 message(s)" in res

        res = admin.send_admin(f"DOMAINS DOMAIN {task.domain} PURGE")
        assert f"Purged 0 message(s)" in res

    # Stop the admin session
    admin.stop()


def test_purge_inactive(single_node: Cluster) -> None:
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

    # Start the admin client.
    admin = AdminClient()
    admin.connect(*cluster.admin_endpoint)

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
        res = admin.send_admin(
            f"DOMAINS DOMAIN {record.domain} QUEUE {record.queue_name} PURGE *"
        )
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

        res = admin.send_admin(
            f"DOMAINS DOMAIN {record.domain} QUEUE {record.queue_name} PURGE *"
        )
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
            res = admin.send_admin(
                f"DOMAINS DOMAIN {record.domain} QUEUE {record.queue_name} PURGE {app_id}"
            )
            assert f"Purged {record.num} message(s)" in res

        record.num = 0

        res = admin.send_admin(
            f"DOMAINS DOMAIN {record.domain} QUEUE {record.queue_name} PURGE *"
        )
        assert f"Purged 0 message(s)" in res

    # Also check that purge domain for FANOUT could not purge more messages.
    res = admin.send_admin(f"DOMAINS DOMAIN {tc.DOMAIN_FANOUT} PURGE")
    assert f"Purged 0 message(s)" in res

    # Stop the admin session
    admin.stop()


def test_commands_on_non_existing_domain(single_node: Cluster) -> None:
    """
    Test: domain admin commands work even if domain was not loaded from the disk yet.
    This test works with an assumption that the cluster was just started before the test,
    so the broker doesn't have any internal domain objects constructed in memory.

    Stage 1: send commands to domains existing on disk but not yet loaded to the broker

    Stage 2: send commands to domains not existing on disk

    Concerns:
    - DOMAINS DOMAIN ... command works if domain exists on disk.
    - DOMAINS RECONFIGURE ... command works if domain exists on disk.
    """
    cluster: Cluster = single_node

    # Start the admin client
    admin = AdminClient()
    admin.connect(*cluster.admin_endpoint)

    # Stage 1: send commands to domains existing on disk but not yet loaded to the broker
    # Note that we use different domains for each test case, because we want to check each
    # command with clear state for a domain.
    res = admin.send_admin(f"domains domain {tc.DOMAIN_PRIORITY} infos")
    assert "ActiveQueues ..: 0" in res

    res = admin.send_admin(f"domains reconfigure {tc.DOMAIN_FANOUT}")
    assert "SUCCESS" in res

    # Stage 2: send commands to domains not existing on disk
    not_existing_domain = "this.domain.doesnt.exist"

    res = admin.send_admin(f"domains domain {not_existing_domain} infos")
    assert f"Domain '{not_existing_domain}' doesn't exist" in res

    res = admin.send_admin(f"domains reconfigure {not_existing_domain}")
    assert f"Domain '{not_existing_domain}' doesn't exist" in res

    # Stop the admin session
    admin.stop()
