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
import json

def test_primary_rerouting(multi_node: Cluster) -> None:
    """
    Test: commands intended only for primary node are automatically routed to
          primary node and response it sent back when executed from non-primary

    Stage 1: Find non-primary
    - Start admin client
    - Find node which is not known leader
    - Connect to that node
    - Open FANOUT domain

    Stage 3: Try all primary-only commands:
    - DOMAINS DOMAIN <domain> PURGE
    - CLUSTERS CLUSTER <name> FORCE_GC_QUEUES
    - CLUSTERS CLUSTER <name> STORAGE PARTITION <partitionId> ENABLE/DISABLE 
    """

    admin = AdminClient()
    
    # find the first node which is not a known leader
    for node in multi_node.nodes():
        if node != node.last_known_leader:
            member_node = node
            break

    admin.connect(member_node.config.host, int(member_node.config.port))
    
    # member_node should not be the primary, so lets try sending primary 
    # only commands to them
    
    # need to open a domain first
    proxies = multi_node.proxy_cycle()
    proxy = next(proxies)
    producer: Client = proxy.create_client("producer")

    producer.open(tc.URI_FANOUT, flags=["write,ack"], succeed=True)

    # Try DOMAINS DOMAIN <domain> PURGE
    res = admin.send_admin(f"DOMAINS DOMAIN {tc.DOMAIN_FANOUT} PURGE") 

    # response should say "Purged XX messages..."
    assert "Purged" in res

    # Try DOMAINS DOMAIN <name> QUEUE <queue_name> PURGE <appId>
    res = admin.send_admin(
        f"DOMAINS DOMAIN {tc.DOMAIN_FANOUT} QUEUE {tc.TEST_QUEUE} PURGE {tc.TEST_APPIDS[0]}")
    assert "Purged" in res

    # Try CLUSTERS CLUSTER <name> FORCE_GC_QUEUES
    res = admin.send_admin(f"CLUSTERS CLUSTER {multi_node.name} FORCE_GC_QUEUES")
    assert "SUCCESS" in res 

    # Try CLUSTERS CLUSTER <name> STORAGE PARTITION <partitionId> ENABLE/DISABLE
    res = admin.send_admin(f"CLUSTERS CLUSTER {multi_node.name} STORAGE PARTITION 0 ENABLE")
    assert "SUCCESS" in res

    res = admin.send_admin(f"CLUSTERS CLUSTER {multi_node.name} STORAGE PARTITION 0 DISABLE")
    assert "SUCCESS" in res

    admin.stop()

def test_cluster_rerouting(multi_node: Cluster) -> None:
    """
    Test: commands intended for cluster are routed to all nodes in the cluster
          regardless of the node the command is initially sent to

    Stage 1: Setup client
    - Start admin client
    - Connect to arbitrary node
    - Open PRIORITY domain

    Stage 2: Try all cluster commands:
    - DOMAINS RECONFIGURE <domain> 
    - CLUSTERS CLUSTER <name> STORAGE REPLICATION SET_ALL <param> <value>
    - CLUSTERS CLUSTER <name> STORAGE REPLICATION GET_ALL <param> <value> 
    - CLUSTERS CLUSTER <name> STATE ELECTOR SET_ALL <param> <value>
    - CLUSTERS CLUSTER <name> STATE ELECTOR GET_ALL <param> <value> 
    """

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

    # Try CLUSTERS CLUSTER <name> STORAGE REPLICATION SET_ALL <param> <value>
    res = admin.send_admin(f"CLUSTERS CLUSTER {multi_node.name} STORAGE REPLICATION SET_ALL quorum 2")
    success_count = res.split().count("Quorum")
    assert success_count == 4

    # GET_ALL
    res = admin.send_admin(f"CLUSTERS CLUSTER {multi_node.name} STORAGE REPLICATION SET_ALL quorum 2")
    success_count = res.split().count("Quorum")
    assert success_count == 4

    # Try CLUSTERS CLUSTER <name> STATE ELECTOR SET_ALL <param> <value>
    res = admin.send_admin(f"CLUSTERS CLUSTER {multi_node.name} STATE ELECTOR SET_ALL quorum 2")
    success_count = res.split().count("Quorum")
    assert success_count == 4

    # GET_ALL
    res = admin.send_admin(f"CLUSTERS CLUSTER {multi_node.name} STATE ELECTOR SET_ALL quorum 2")
    success_count = res.split().count("Quorum")
    assert success_count == 4

    admin.stop()

def test_multi_response_encoding(multi_node: Cluster):
    """
    Test: JSON encoding options work with multiple responses (when routing to
          multiple nodes)

    Stage 1: Setup client
    - Start admin client
    - Connect to arbitrary node
    - Open PRIORITY domain 

    Stage 2: Test compact json formatting

    Stage 3: Test pretty json formatting

    """

    def is_compact(json_str: str) -> bool:
        return "    " not in json_str

    admin = AdminClient()

    node = multi_node.nodes()[0]

    admin.connect(node.config.host, int(node.config.port))

    proxies = multi_node.proxy_cycle()
    proxy = next(proxies)
    producer: Client = proxy.create_client("producer")

    producer.open(tc.URI_PRIORITY, flags=["write,ack"], succeed=True)

    # Stage 2: Test Compact Encoding
    cmds = [json.dumps({"domains": {"reconfigure": {"domain": tc.DOMAIN_PRIORITY}}, "encoding": "JSON_COMPACT"}),
           f"ENCODING JSON_COMPACT DOMAINS RECONFIGURE {tc.DOMAIN_PRIORITY}"]
    for cmd in cmds:
        res = admin.send_admin(cmd)
        res_json = json.loads(res)
        assert "responses" in res_json
        assert is_compact(res)
        # we should have gotten 4 responses
        responses = res_json["responses"]
        assert len(responses) == 4

    # Stage 3: Test Pretty Encoding
    cmds = [json.dumps({"domains": {"reconfigure": {"domain": tc.DOMAIN_PRIORITY}}, "encoding": "JSON_PRETTY"}),
           f"ENCODING JSON_PRETTY DOMAINS RECONFIGURE {tc.DOMAIN_PRIORITY}"]
    for cmd in cmds:
        res = admin.send_admin(cmd)
        res_json = json.loads(res)
        assert "responses" in res_json
        assert not is_compact(res)
        # we should have gotten 4 responses
        responses = res_json["responses"]
        assert len(responses) == 4
    
    admin.stop()
        