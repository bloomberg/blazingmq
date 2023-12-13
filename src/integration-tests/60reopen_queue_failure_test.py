import bmq.dev.it.testconstants as tc
from bmq.dev.it.fixtures import (  # pylint: disable=unused-import
    Cluster,
    standard_cluster as cluster,
)


class TestReopenQueueFailure:
    """
    This test case verifies a fix for the broker crash when replica or proxy
    node crashes while processing a configure or close queue response after a
    reopen queue has failed. All nodes going down gracefully at cluster
    shutdown verifies the fix.
    """

    def test_reopen_queue_failure(self, cluster: Cluster):
        proxies = cluster.proxy_cycle()

        # We want proxy connected to a replica
        next(proxies)

        proxy = next(proxies)
        consumer = proxy.create_client("consumer")

        consumer.open(tc.URI_PRIORITY, flags=["read"], succeed=True)

        # Set the quorum of all non-leader nodes to 99 to prevent them from
        # becoming a new leader
        leader = cluster.last_known_leader
        next_leader = None
        for node in cluster.nodes():
            # NOTE: Following assumes 4-node cluster
            if node != leader:
                node.set_quorum(99)
                if node.datacenter == leader.datacenter:
                    next_leader = node
        assert leader != next_leader

        # Kill the leader
        cluster.drain()
        leader.check_exit_code = False
        leader.kill()
        leader.wait()

        # Remove routing config on the next leader (to cause reopen
        # queue failure)
        cluster.work_dir.joinpath(
            next_leader.name, "etc", "domains", f"{tc.DOMAIN_PRIORITY}.json"
        ).unlink()
        next_leader.command("DOMAINS RESOLVER CACHE_CLEAR ALL")

        # Make the quorum for selected node be 1 so it becomes new leader
        next_leader.set_quorum(1)

        # Wait for new leader
        cluster.wait_leader()
        assert cluster.last_known_leader == next_leader

        consumer.stop_session()
