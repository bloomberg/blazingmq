import time

import bmq.dev.it.testconstants as tc
from bmq.dev.it.fixtures import Cluster, cluster, tweak  # pylint: disable=unused-import


class TestAlarms:
    """
    Testing broker ALARMS.
    """

    @tweak.cluster.queue_operations.consumption_monitor_period_ms(500)
    @tweak.domain.max_idle_time(3)
    def test_no_alarms_for_a_slow_queue(self, cluster: Cluster):
        """
        Test no broker ALARMS for a slowly moving queue.
        """
        leader = cluster.last_known_leader
        proxy = next(cluster.proxy_cycle())

        producer = proxy.create_client("producer")
        producer.open(tc.URI_PRIORITY, flags=["write,ack"], succeed=True)

        consumer1 = proxy.create_client("consumer1")
        consumer2 = proxy.create_client("consumer2")
        consumer1.open(
            tc.URI_PRIORITY, flags=["read"], max_unconfirmed_messages=1, succeed=True
        )

        producer.post(tc.URI_PRIORITY, ["msg1"], succeed=True, wait_ack=True)

        consumer1.confirm(tc.URI_PRIORITY, "*", succeed=True)

        producer.post(tc.URI_PRIORITY, ["msg1"], succeed=True, wait_ack=True)
        producer.post(tc.URI_PRIORITY, ["msg1"], succeed=True, wait_ack=True)

        time.sleep(4)

        # First, test the alarm
        assert leader.alarms("QUEUE_CONSUMER_MONITOR", 1)
        leader.drain()

        # Then test no alarm while consumer1 slowly confirms
        time.sleep(1)
        consumer1.confirm(tc.URI_PRIORITY, "*", succeed=True)

        time.sleep(1)
        consumer1.confirm(tc.URI_PRIORITY, "*", succeed=True)
        producer.post(tc.URI_PRIORITY, ["msg1"], succeed=True, wait_ack=True)

        time.sleep(1)
        # Consumer2 picks the last message
        consumer2.open(
            tc.URI_PRIORITY, flags=["read"], max_unconfirmed_messages=1, succeed=True
        )

        time.sleep(1)
        assert not leader.alarms("QUEUE_CONSUMER_MONITOR", 1)
