"""
This suite of test cases exercises functionality of prioritizing fanout
consumers .
"""


from blazingmq.dev.it.fixtures import Cluster, cluster, order  # pylint: disable=unused-import
from blazingmq.dev.it.process.client import Client
from blazingmq.dev.it.util import wait_until

pytestmark = order(4)

def test_fanout_priorities(cluster: Cluster):
    # create foo, bar, and baz clients on every node.

    # two of each with priorities 1 and 2

    producers = []
    apps = ["foo", "bar", "baz"]
    proxies = cluster.proxy_cycle()

    nodes = cluster.nodes()
    highPriorityQueues = []
    lowPriorityQueues = []

    nodes.append(next(proxies))
    nodes.append(next(proxies))

    for node in nodes:
        client = node.create_client("consumer2")
        [queues] = client.open_fanout_queues(
            1, flags=["read"], consumer_priority=2, block=True, appids=apps
        )
        highPriorityQueues += queues

        client = node.create_client("consumer1")
        [queues] = client.open_fanout_queues(
            1, flags=["read"], consumer_priority=1, block=True, appids=apps
        )
        lowPriorityQueues += queues

        [producer] = client.open_fanout_queues(1, flags=["write", "ack"], block=True)
        producers.append(producer)

    # Deliver to high priorities

    _verify_delivery(producers, "before", highPriorityQueues, lowPriorityQueues)

    # reverse priorities
    for queue in lowPriorityQueues:
        assert queue.configure(consumer_priority=2, block=True) == Client.e_SUCCESS

    for queue in highPriorityQueues:
        assert queue.configure(consumer_priority=1, block=True) == Client.e_SUCCESS

    # Deliver to new high priorities

    _verify_delivery(producers, "after", lowPriorityQueues, highPriorityQueues)

    # Close everything

    for queue in producers:
        assert queue.close(block=True) == Client.e_SUCCESS

    for queue in lowPriorityQueues:
        assert queue.close(block=True) == Client.e_SUCCESS

    for queue in highPriorityQueues:
        assert queue.close(block=True) == Client.e_SUCCESS


def _verify_delivery(producers, message, highPriorityQueues, lowPriorityQueues):

    for i, producer in enumerate(producers):
        # there is one producer on each node
        assert (
            producer.post([f"{message}{i}"], block=True, wait_ack=True)
            == Client.e_SUCCESS
        )
        # this results in one message per each item in highPriorityQueues

    # pylint: disable=cell-var-from-loop; passing lambda to 'wait_until' is safe
    for queue in highPriorityQueues:
        messages = []
        assert wait_until(lambda: len(queue.list(block=True)) == 1, 2, quiet=True)

        msgs = queue.list(block=True)
        assert msgs[0].payload not in messages
        messages.append(msgs[0].payload)

        assert queue.confirm("*", block=True) == Client.e_SUCCESS

    for queue in lowPriorityQueues:
        # assert no PUSH received within 0.1 second
        assert not wait_until(lambda: len(queue.list(block=True)) == 1, 0.1, quiet=True)
