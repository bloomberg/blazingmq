"""
Create a configurator for running a cluster.

BlazingMQ configurator generator

usage: python-m blazingmq.dev.configurator
            [-h] [--log-level LEVELS]
            --workdir WORKDIR [--port-base PORT]

optional arguments:
  -h, --help            show this help message and exit
  --log-level LEVELS, -l LEVELS
                        set log level(s) as specified by LEVELS
  --workdir WORKDIR, -w WORKDIR
                        write files in WORKDIR
  --port-base PORT, -b PORT
                        allocate ports starting at PORT (use ephemeral ports if not specified)
"""

import argparse
import itertools
import logging
import signal
import socket
from pathlib import Path

import blazingmq.util.logging as bul
from blazingmq.dev.configurator import Configurator, LocalSite, Session, logger

parser = argparse.ArgumentParser(
    prog="python -m blazingmq.dev.configurator",
    description="BMQ configurator generator",
    parents=[bul.make_parser()],
)

parser.add_argument(
    "root",
    type=Path,
    action="store",
    help="write files in ROOT directory",
)

parser.add_argument(
    "--port-base",
    "-b",
    type=int,
    action="store",
    required=False,
    metavar="PORT",
    help="allocate ports starting at PORT (use ephemeral ports if not specified)",
)

args = parser.parse_args()

for handler in logging.root.handlers:
    handler.setFormatter(logging.Formatter("%(message)s"))


def ephemeral_port_allocator():
    while True:
        sock = socket.socket()
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        sock.bind(("0.0.0.0", 0))
        yield sock.getsockname()[1]


configurator = Configurator()

if args.port_base is not None:
    port_allocator = itertools.count(args.port_base)
else:
    port_allocator = ephemeral_port_allocator()

cluster = configurator.cluster(
    name="c2x2",
    nodes=[
        configurator.broker(
            name="east/1",
            tcp_host="localhost",
            tcp_port=next(port_allocator),
            data_center="east",
        ),
        configurator.broker(
            name="east/2",
            tcp_host="localhost",
            tcp_port=next(port_allocator),
            data_center="east",
        ),
        configurator.broker(
            name="west/1",
            tcp_host="localhost",
            tcp_port=next(port_allocator),
            data_center="west",
        ),
        configurator.broker(
            name="west/2",
            tcp_host="localhost",
            tcp_port=next(port_allocator),
            data_center="west",
        ),
    ],
)


priority_domain = cluster.priority_domain("bmq.test.mmap.priority")
# TODO: move test domain definitions to this package, add them to the
# configurator.

for broker in configurator.brokers.values():
    broker.deploy(LocalSite(args.root / broker.name))


def on_signal(signum: int, frame) -> None:  # pylint: disable=W0613
    """
    Signal handler.

    Do nothing, 'main' just waits for C-C with signal.pause().
    """

    logger.info("received signal: %s. Exiting...", signum)


signal.signal(signal.SIGINT, on_signal)  # handle CTRL-C
signal.signal(signal.SIGTERM, on_signal)

with Session(configurator, args.root) as Session:
    for broker in configurator.brokers.values():
        print("{}: tcp://{}:{}".format(broker.name, broker.host, broker.port))

    print("C-c to exit...")
    Session.run()
    signal.pause()
