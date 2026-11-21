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
Client workloads recorded as fuzz corpus.  Each scenario drives one bmqtool
with a started session, and uses its own queues.
"""

import time
from typing import Callable, Dict, List

from blazingmq.dev.it.process.client import Client, Message

# Domain known by the broker of 's_bmqfuzz_mqba_application_client'.
DOMAIN = "bmq.test.mem.priority"

RECEIVE_TIMEOUT = 15


def _uri(name: str) -> str:
    return f"bmq://{DOMAIN}/{name}"


def _receive(client: Client, uri: str, count: int) -> List[Message]:
    """
    Wait until 'client' has at least 'count' unconfirmed messages on 'uri',
    and return them.
    """

    deadline = time.monotonic() + RECEIVE_TIMEOUT
    while time.monotonic() < deadline:
        messages = client.list(uri, block=True)
        if len(messages) >= count:
            return messages
        time.sleep(0.1)
    raise RuntimeError(f"{client.name} did not receive {count} messages on {uri}")


def producer(client: Client) -> None:
    uri = _uri("producer")
    client.open(uri, ["write", "ack"], succeed=True)
    for i in range(3):
        client.post(uri, [f"message-{i}"], wait_ack=True)
    client.post(
        uri,
        ["with-properties"],
        wait_ack=True,
        messageProperties=[
            {"name": "x", "value": "1", "type": "E_INT"},
            {"name": "s", "value": "text", "type": "E_STRING"},
        ],
    )
    client.close(uri, succeed=True)


def compressed_producer(client: Client) -> None:
    uri = _uri("compressed")
    client.open(uri, ["write", "ack"], succeed=True)
    client.post(uri, ["x" * 4096], wait_ack=True, compression_algorithm_type="ZLIB")
    client.close(uri, succeed=True)


def producer_consumer(client: Client) -> None:
    uri = _uri("producer_consumer")
    client.open(uri, ["read", "write", "ack"], succeed=True)
    for i in range(3):
        client.post(uri, [f"message-{i}"], wait_ack=True)
    for message in _receive(client, uri, 3):
        client.confirm(uri, message.guid, succeed=True)
    client.close(uri, succeed=True)


def subscriptions(client: Client) -> None:
    uri = _uri("subscriptions")
    client.open(
        uri,
        ["read", "write", "ack"],
        succeed=True,
        subscriptions=[{"expression": "x == 1"}],
    )
    for value in ("0", "1"):
        client.post(
            uri,
            [f"x-{value}"],
            wait_ack=True,
            messageProperties=[{"name": "x", "value": value, "type": "E_INT"}],
        )
    for message in _receive(client, uri, 1):
        client.confirm(uri, message.guid, succeed=True)
    client.configure(uri, succeed=True, subscriptions=[{"expression": "x >= 0"}])
    for message in _receive(client, uri, 1):
        client.confirm(uri, message.guid, succeed=True)
    client.close(uri, succeed=True)


def many_queues(client: Client) -> None:
    uris = [_uri(f"many_queues_{i}") for i in range(3)]
    for uri in uris:
        client.open(uri, ["read", "write"], succeed=True)
    for uri in uris:
        client.close(uri, succeed=True)


def unknown_domain(client: Client) -> None:
    client.open("bmq://bmq.unknown.domain/queue", ["write"], succeed=False)


SCENARIOS: Dict[str, Callable[[Client], None]] = {
    "producer": producer,
    "compressed_producer": compressed_producer,
    "producer_consumer": producer_consumer,
    "subscriptions": subscriptions,
    "many_queues": many_queues,
    "unknown_domain": unknown_domain,
}
