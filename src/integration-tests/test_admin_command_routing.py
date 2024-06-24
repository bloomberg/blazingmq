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

# @dataclasses.dataclass
# class PostRecord:
#     domain: str
#     queue_name: str
#     num: int

#     @property
#     def uri(self) -> str:
#         return f"bmq://{self.domain}/{self.queue_name}"
    
#     def append(self, other: "PostRecord") -> None:
#         assert self.domain == other.domain
#         assert self.queue_name == other.queue_name
#         self.num += other.num

# def post_n_msgs(
#     producer: Client, task: PostRecord, posted: Optional[Dict[str, PostRecord]]=None
# ) -> None:

def test_primary_rerouting(multi_node: Cluster) -> None:
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

    # now try to purge that domain with a member node
    res = admin.send_admin(f"DOMAINS DOMAIN {tc.DOMAIN_FANOUT} PURGE")

    print(res)

    # we can tell that the command failed if it mentioned errors
    assert "Errors encountered" not in res
    # otherwise, we will assume it was routed properly and succeeded

    # also try "DOMAINS DOMAIN <name> QUEUE <queue_name> PURGE <appId>"
    # res = admin.send_admin(
    #     f"DOMAINS DOMAIN {tc.DOMAIN_PRIORITY} QUEUE {tc.TEST_QUEUE} PURGE {tc.TEST_APPIDS[0]}")

    # print(res)

    # assert "Errors encountered" not in res

    admin.stop()


