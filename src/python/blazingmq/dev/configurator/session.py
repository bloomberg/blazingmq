"""
Support for creating a "configurator", i.e. a collection of directories and
scripts for running a cluster.
"""

# mypy: disable-error-code="union-attr"
# pylint: disable=missing-function-docstring, missing-class-docstring, consider-using-f-string
# pyright: reportOptionalMemberAccess=false


import contextlib
import itertools
import logging
import subprocess
import threading
from dataclasses import dataclass, field
from pathlib import Path
from typing import IO, Dict, Optional
import argparse
import itertools
import signal
import socket
from pathlib import Path

from termcolor import colored

from blazingmq.dev.configurator.configurator import Broker, Configurator
from blazingmq.dev.configurator.localsite import LocalSite
import blazingmq.util.logging as bul

COLORS = {
    "green": 32,
    "yellow": 33,
    "magenta": 35,
    "cyan": 36,
    "blue": 34,
    "light_green": 92,
    "light_yellow": 93,
    "light_blue": 94,
    "light_magenta": 95,
    "light_cyan": 96,
}


logger = logging.getLogger(__name__)
broker_logger = logger.getChild("broker")


class MonitoredProcess:
    process: Optional[subprocess.Popen] = None
    thread: Optional[threading.Thread] = None


def broker_monitor(out: IO[str], prefix: str, color: str):
    while not out.closed:
        line = out.readline()
        if line == "":
            break
        line = line.rstrip(" \n\r")
        if line:
            broker_logger.info(colored("%s | %s", color), prefix, line)


@dataclass
class Session(contextlib.AbstractContextManager):
    configurator: Configurator
    root: Path
    brokers: Dict[Broker, MonitoredProcess] = field(default_factory=dict)

    def __exit__(self, *args):
        for broker in reversed(self.brokers.values()):
            if broker.process is not None:
                broker.process.__exit__(*args)

        for broker in reversed(self.brokers.values()):
            if broker.thread is not None:
                broker.thread.join()

    def stop(self):
        for broker in self.brokers.values():
            if broker.process is not None:
                broker.process.terminate()
                broker.process.wait()

    def run(self):
        colors = itertools.cycle(COLORS)
        prefix_len = max(len(name) for name in self.configurator.brokers)

        for broker in self.configurator.brokers.values():
            monitored = MonitoredProcess()
            self.brokers[broker] = monitored

            monitored.process = subprocess.Popen(
                [self.root.joinpath(broker.name, "run")],
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                encoding="ASCII",
                bufsize=0,
            )

            assert monitored.process.stdout is not None
            monitored.thread = threading.Thread(
                target=broker_monitor,
                args=(
                    monitored.process.stdout,
                    broker.name.ljust(prefix_len),
                    next(colors),
                ),
            )
            monitored.thread.start()

if __name__ == "__main__":
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
        configurator.deploy(broker, LocalSite(args.root / broker.name))


    def on_signal(signum: int, frame) -> None:  # pylint: disable=W0613
        """
        Signal handler.

        Do nothing, 'main' just waits for C-C with signal.pause().
        """

        logger.info("received signal: %s. Exiting...", signum)


    signal.signal(signal.SIGINT, on_signal)  # handle CTRL-C
    signal.signal(signal.SIGTERM, on_signal)

    with Session(configurator, args.root) as session:
        for broker in configurator.brokers.values():
            print("{}: tcp://{}:{}".format(broker.name, broker.host, broker.port))

        print("C-c to exit...")
        session.run()
        signal.pause()
