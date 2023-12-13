"""
Unit and integration tests for blazingmq.dev.workspace.
"""

import itertools

from blazingmq.dev.workspace import Workspace, HostLocation


def test_cluster_proxies(tmpdir):
    workspace = Workspace()
    instance = itertools.count(start=1)
    port_allocator = itertools.count(start=40000)
    cluster = workspace.cluster(
        name="c2x2",
        nodes=[
            workspace.broker(
                name="east1",
                instance=str(next(instance)),
                tcp_host="localhost",
                tcp_port=next(port_allocator),
                data_center="east",
            ),
            workspace.broker(
                name="east2",
                instance=str(next(instance)),
                tcp_host="localhost",
                tcp_port=next(port_allocator),
                data_center="east",
            ),
            workspace.broker(
                name="west1",
                instance=str(next(instance)),
                tcp_host="localhost",
                tcp_port=next(port_allocator),
                data_center="west",
            ),
            workspace.broker(
                name="west2",
                instance=str(next(instance)),
                tcp_host="localhost",
                tcp_port=next(port_allocator),
                data_center="west",
            ),
        ],
    )
    cluster.priority_domain("bmq.test.mmap.priority")
    workspace.broker(
        name="eastp",
        instance=str(next(instance)),
        tcp_host="localhost",
        tcp_port=next(port_allocator),
        data_center="east",
    ).proxy(cluster)
