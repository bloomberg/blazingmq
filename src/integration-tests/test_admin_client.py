"""
This suite of test cases exercises admin session connection and some basic
commands.
"""

import json
import re

from blazingmq.dev.it.fixtures import Cluster, single_node, order  # pylint: disable=unused-import
from blazingmq.dev.it.process.admin import AdminClient


def test_admin(single_node: Cluster):
    cluster: Cluster = single_node
    endpoint: str = cluster.config.definition.nodes[0].transport.tcp.endpoint  # type: ignore

    # Extract the (host, port) pair from the config
    m = re.match(r".+://(.+):(\d+)", endpoint)  # tcp://host:port
    assert m is not None
    host, port = str(m.group(1)), int(m.group(2))

    # Start the admin client
    admin = AdminClient()
    admin.connect(host, port)

    # Check basic "help" command
    assert (
        "This process responds to the following CMD subcommands:"
        in admin.send_admin("help")
    )

    # Check non-existing "invalid cmd" command
    assert "Unable to decode command" in admin.send_admin("invalid cmd")

    # Check more complex "brokerconfig dump" command, expect valid json
    # with the same "port" value as the one used for this connection
    broker_config_str = admin.send_admin("brokerconfig dump")
    broker_config = json.loads(broker_config_str)

    assert broker_config["networkInterfaces"]["tcpInterface"]["port"] == port

    # Stop the admin session
    admin.stop()
