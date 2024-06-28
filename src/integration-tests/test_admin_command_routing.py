"""
This suite of test cases verifies that admin commands routed to a member node
undergo the proper routing to the relevant primary/cluster if that command
requires it.

This test suite does not concern itself with verifying the functional details
of commands; only that they get routed to the proper nodes.
"""

import blazingmq.dev.it.testconstants as tc
from blazingmq.dev.it.fixtures import (
    Cluster,
    multi_node,
)
from blazingmq.dev.it.process.admin import AdminClient
from blazingmq.dev.it.process.client import Client

def test_primary_rerouting(multi_node: Cluster) -> None:
    print(multi_node.name)

    admin = AdminClient()
    
    # find the first node which is not a known leader
    for node in multi_node.nodes():
        if node != node.last_known_leader:
            member_node = node
            break

    print("DEBUG: using node",member_node)

    admin.connect(member_node.config.host, int(member_node.config.port))
    
    # member_node should not be the primary, so lets try sending a primary 
    # only command to them.

    # -- First test sending DOMAIN PURGE command --
    
    # need to open a domain first
    proxies = multi_node.proxy_cycle()
    proxy = next(proxies)
    producer: Client = proxy.create_client("producer")

    producer.open(tc.URI_FANOUT, flags=["write,ack"], succeed=True)

    # Try DOMAINS DOMAIN <domain> PURGE
    res = admin.send_admin(f"DOMAINS DOMAIN {tc.DOMAIN_FANOUT} PURGE")

    print(res)

    # response should say "Purged XX messages..."
    assert "Purged" in res

    # Try DOMAINS DOMAIN <name> QUEUE <queue_name> PURGE <appId>
    res = admin.send_admin(
        f"DOMAINS DOMAIN {tc.DOMAIN_FANOUT} QUEUE {tc.TEST_QUEUE} PURGE {tc.TEST_APPIDS[0]}")

    print(res)

    assert "Purged" in res

    # Try CLUSTERS CLUSTER <name> FORCE_GC_QUEUES
    # Difficult to test since currently this command returns "SUCCESS" no matter what
    #       (i.e. even if it fails due to not being ran on the primary)
    # res = admin.send_admin(f"CLUSTERS CLUSTER {multi_node.name} FORCE_GC_QUEUES")


    admin.stop()

def test_cluster_rerouting(multi_node: Cluster) -> None:
    admin = AdminClient()

    node = multi_node.nodes()[0]

    admin.connect(node.config.host, int(node.config.port))

    proxies = multi_node.proxy_cycle()
    proxy = next(proxies)
    producer: Client = proxy.create_client("producer")

    producer.open(tc.URI_PRIORITY, flags=["write,ack"], succeed=True)

    # Try DOMAINS RECONFIGURE <domain> 
    res = admin.send_admin(f"DOMAINS RECONFIGURE {tc.DOMAIN_PRIORITY}")

    # Expect 4 "SUCCESS" responses
    success_count = res.split().count("SUCCESS")

    assert success_count == 4


