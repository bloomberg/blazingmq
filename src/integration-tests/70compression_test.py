"""
Integration test that tests the successful sending and receiving of compressed
payload in various scenarios as listed below:
    - cluster gets restarted after sending ack to producer.
"""

import bmq.dev.it.testconstants as tc
from bmq.dev.it.fixtures import Cluster, cluster  # pylint: disable=unused-import
from bmq.dev.it.util import random_string


class TestCompression:
    def test_compression_restart(self, cluster: Cluster):

        # Start a producer and post a message.
        proxies = cluster.proxy_cycle()
        producer = next(proxies).create_client("producer")
        producer.open(tc.URI_PRIORITY_SC, flags=["write", "ack"], succeed=True)

        # Note for compression, we use a much larger payload of length greater
        # than 1024 characters. The reason being that internally BMQ SDK skips
        # compressing messages which are small sized specifically less than
        # 1024 bytes. In this test, we use a randomly generated 5000 character
        # string.
        payload = random_string(len=5000)
        producer.post(
            tc.URI_PRIORITY_SC,
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
        if cluster.is_local:
            producer.wait_state_restored()

        consumer = next(proxies).create_client("consumer")
        consumer.open(tc.URI_PRIORITY_SC, flags=["read"], succeed=True)
        consumer.wait_push_event()
        msgs = consumer.list(tc.URI_PRIORITY_SC, block=True)

        # we truncate the message to 32 characters in the bmqtool api used by
        # the consumer.list function
        # TODO: Add support for listing complete messages in bmqtool
        assert len(msgs) == 1
        assert msgs[0].payload[:32] == payload[:32]
