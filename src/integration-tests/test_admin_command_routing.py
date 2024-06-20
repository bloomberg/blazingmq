"""
This suite of test cases verifies that admin commands routed to a member node
undergo the proper routing to the relevant primary/cluster if that command
requires it.
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
    # connect to the second node (shouldn't be the primary)
    member_node = multi_node.nodes()[1]
    
    print(member_node)

    admin.connect(member_node.config.host, int(member_node.config.port))

    res = admin.send_admin("HELP")

    print(res)

    assert len(res) > 0

    admin.stop()


