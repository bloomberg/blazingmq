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
Integration test that tests the successful sending and receiving of compressed
payload in various scenarios as listed below:
    - cluster gets restarted after sending ack to producer.
"""

import blazingmq.dev.it.testconstants as tc
from blazingmq.dev.it.fixtures import (
    Cluster,
    order,
)  # pylint: disable=unused-import
from blazingmq.dev.it.util import random_string

pytestmark = order(10)


def test_compression_restart(cluster: Cluster, domain_urls: tc.DomainUrls):
    # Start a producer and post a message.
    uri_priority = domain_urls.uri_priority
    proxies = cluster.proxy_cycle()
    producer = next(proxies).create_client("producer")
    producer.open(uri_priority, flags=["write", "ack"], succeed=True)

    # Note for compression, we use a much larger payload of length greater
    # than 1024 characters. The reason being that internally BMQ SDK skips
    # compressing messages which are small sized specifically less than
    # 1024 bytes. In this test, we use a randomly generated 5000 character
    # string.
    payload = random_string(len=5000)
    producer.post(
        uri_priority,
        payload=[payload],
        wait_ack=True,
        succeed=True,
        compression_algorithm_type="ZLIB",
    )

    # Use strong consistency (SC) to ensure that majority nodes in the
    # cluster have received the message at the storage layer before
    # restarting the  cluster.

    cluster.restart_nodes()
    # For a standard cluster, states have already been restored as part of
    # leader re-election.
    if cluster.is_single_node:
        producer.wait_state_restored()

    consumer = next(proxies).create_client("consumer")
    consumer.open(uri_priority, flags=["read"], succeed=True)
    consumer.wait_push_event()
    msgs = consumer.list(uri_priority, block=True)

    # we truncate the message to 32 characters in the bmqtool api used by
    # the consumer.list function
    # TODO: Add support for listing complete messages in bmqtool
    assert len(msgs) == 1
    assert msgs[0].payload[:32] == payload[:32]
