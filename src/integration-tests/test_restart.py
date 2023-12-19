"""
Integration test that tests some basic things while restarting *entire* cluster
in the middle of things.  Note that this test does not test the HA
functionality (i.e., no PUTs/CONFIRMs etc are retransmitted).
"""

import re
import time

import bmq.dev.it.testconstants as tc
from bmq.dev.it.fixtures import Cluster, cluster  # pylint: disable=unused-import
from bmq.dev.it.process.client import Client
from bmq.dev.it.util import attempt, wait_until


def test_basic(cluster: Cluster):
    # Start a producer and post a message.
    proxies = cluster.proxy_cycle()
    producer = next(proxies).create_client("producer")
    producer.open(tc.URI_PRIORITY, flags=["write", "ack"], succeed=True)
    producer.post(tc.URI_PRIORITY, payload=["msg1"], wait_ack=True, succeed=True)

    time.sleep(2)
    # Before restarting the cluster, ensure that all nodes in the cluster
    # have received the message at the storage layer.  This is necessary
    # in the absence of stronger consistency in storage replication in
    # BMQ.  Presence of message in the storage at each node is checked by
    # sending 'STORAGE SUMMARY' command and grepping its output.
    for node in cluster.nodes():
        node.command(f"CLUSTERS CLUSTER {node.cluster_name} STORAGE SUMMARY")

    time.sleep(2)
    for node in cluster.nodes():
        assert node.outputs_regex(
            r"\w{10}\s+0\s+1\s+\d+\s+B\s+" + re.escape(tc.URI_PRIORITY),
            timeout=20,
        )
        # Above regex is to match line:
        # C1E2A44527    0      1      68  B      bmq://bmq.test.mmap.priority.~tst/qqq
        # where columns are: QueueKey, PartitionId, NumMsgs, NumBytes,
        # QueueUri respectively. Since we opened only 1 queue, we know that
        # it will be assigned to partitionId 0.

    cluster.restart_nodes()
    # For a standard cluster, states have already been restored as part of
    # leader re-election.
    if cluster.is_local:
        producer.wait_state_restored()

    producer.post(tc.URI_PRIORITY, payload=["msg2"], wait_ack=True, succeed=True)

    consumer = next(proxies).create_client("consumer")
    consumer.open(tc.URI_PRIORITY, flags=["read"], succeed=True)
    consumer.wait_push_event()
    assert wait_until(lambda: len(consumer.list(tc.URI_PRIORITY, block=True)) == 2, 2)


def test_wrong_domain(cluster: Cluster):
    proxies = cluster.proxy_cycle()
    producer = next(proxies).create_client("producer")

    assert Client.e_SUCCESS is producer.open(tc.URI_FANOUT, flags=["write"], block=True)
    assert Client.e_SUCCESS is not producer.open(
        "bmq://domain.does.not.exist/qqq",
        flags=["write"],
        block=True,
        no_except=True,
    )


def test_migrate_domain_to_another_cluster(cluster: Cluster):
    proxies = cluster.proxy_cycle()
    producer = next(proxies).create_client("producer")

    assert Client.e_SUCCESS == producer.open(tc.URI_FANOUT, flags=["write"], block=True)

    # Before changing domain config of each node in the cluster, ensure that
    # all nodes in the cluster have observed the previous open-queue event.
    # If we don't do this, replicas may receive a queue creation event at
    # the storage layer, pick up the domain, and, as a result, fail to
    # successfully apply the queue creation event.
    @attempt(3, 5)
    def wait_replication():
        for node in cluster.nodes():
            node.command(f"CLUSTERS CLUSTER {node.cluster_name} STORAGE SUMMARY")

        for node in cluster.nodes():
            assert node.outputs_regex(
                r"\w{10}\s+0\s+0\s+\d+\s+B\s+" + re.escape(tc.URI_FANOUT),
                timeout=1,
            )
            # Above regex is to match line:
            # C1E2A44527    0      0      68  B      bmq://bmq.test.mmap.fanout/qqq
            # where columns are: QueueKey, PartitionId, NumMsgs, NumBytes,
            # QueueUri respectively. Since we opened only 1 queue, we know that
            # it will be assigned to partitionId 0.

    cluster.config.domains.clear()
    cluster.deploy_domains()
    cluster.restart_nodes()

    assert Client.e_SUCCESS != producer.open(tc.URI_FANOUT, flags=["write"], block=True)
