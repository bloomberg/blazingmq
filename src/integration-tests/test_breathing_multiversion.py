# Copyright 2025 Bloomberg Finance L.P.
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
This test suite exercises basic routing functionality to in the presence of all
types of queues.
"""

from collections import namedtuple

import blazingmq.dev.it.testconstants as tc
import pytest
from blazingmq.dev.it.fixtures import (  # pylint: disable=unused-import
    Cluster,
    order,
    multiversion_multi_node,
)
from blazingmq.dev.it.process.client import Client

pytestmark = order(1)

BmqClient = namedtuple("BmqClient", "handle, uri")


def _stop_clients(clients):
    for client in clients:
        assert client.stop_session(block=True) == Client.e_SUCCESS


def test_verify_partial_close(multiversion_multi_node: Cluster):
    """Drop one of two producers both having unacked message (primary is
    suspended.  Make sure the remaining producer does not get NACK but gets
    ACK when primary resumes.
    """
    proxies = multiversion_multi_node.proxy_cycle()

    proxy = next(proxies)
    proxy = next(proxies)

    producer1 = proxy.create_client("producer1")
    producer1.open(tc.URI_FANOUT, flags=["write", "ack"], succeed=True)

    producer2 = proxy.create_client("producer2")
    producer2.open(tc.URI_FANOUT, flags=["write", "ack"], succeed=True)

    leader = multiversion_multi_node.last_known_leader
    leader.suspend()

    producer1.post(tc.URI_FANOUT, payload=["1"], succeed=True, wait_ack=False)
    producer2.post(tc.URI_FANOUT, payload=["2"], succeed=True, wait_ack=False)

    producer2.stop_session(block=True)

    leader.resume()

    producer1.capture(r"ACK #0: \[ type = ACK status = SUCCESS", 2)

    _stop_clients([producer1, producer2])
