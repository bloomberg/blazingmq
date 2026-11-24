# Copyright 2026 Bloomberg Finance L.P.
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""
Record fuzz corpus for 's_bmqfuzz_mqba_application_client': run client
scenarios against a local broker through a recording proxy, and write the
bytes sent on each client connection to a file.
"""

import argparse
import contextlib
import logging
import os
import socket
import subprocess
import tempfile
import threading
from pathlib import Path
from typing import Iterator

from blazingmq.dev.configurator.configurator import Configurator
from blazingmq.dev.configurator.localsite import LocalSite
from blazingmq.dev.fuzzcorpus.proxy import RecordingProxy
from blazingmq.dev.fuzzcorpus.scenarios import DOMAIN, SCENARIOS
from blazingmq.dev.it.process.client import Client
from blazingmq.dev.paths import required_paths
from blazingmq.schemas import mqbconf
import blazingmq.util.logging as bul

logger = logging.getLogger("blazingmq.dev.fuzzcorpus")

# Cluster name used by the broker of 's_bmqfuzz_mqba_application_client'.
CLUSTER = "local"

START_TIMEOUT = 120
STOP_TIMEOUT = 30
RECORD_TIMEOUT = 15


def _free_port() -> int:
    with socket.socket() as sock:
        sock.bind(("localhost", 0))
        return sock.getsockname()[1]


@contextlib.contextmanager
def _broker(work_dir: Path) -> Iterator[int]:
    """
    Deploy and start a single-node broker in 'work_dir', and yield its port.
    """

    port = _free_port()
    configurator = Configurator()
    broker = configurator.broker(tcp_host="localhost", tcp_port=port, name="fuzz")
    cluster = configurator.cluster(name=CLUSTER, nodes=[broker])

    domain = configurator.priority_domain()
    domain.name = DOMAIN
    domain.consistency = mqbconf.Consistency(
        eventual=mqbconf.QueueConsistencyEventual()
    )
    domain.storage.config = mqbconf.Storage(in_memory=mqbconf.InMemoryStorage())
    cluster.domain(domain)

    broker_dir = work_dir / broker.name
    configurator.deploy(broker, LocalSite(broker_dir))

    started = threading.Event()
    with (broker_dir / "console.log").open("w") as console:
        process = subprocess.Popen(
            ["./run"],
            cwd=broker_dir,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            errors="replace",
        )

        def monitor() -> None:
            assert process.stdout is not None
            for line in process.stdout:
                console.write(line)
                if "BMQbrkr started successfully" in line:
                    started.set()

        monitor_thread = threading.Thread(target=monitor, daemon=True)
        monitor_thread.start()

        try:
            if not started.wait(START_TIMEOUT):
                raise RuntimeError(f"broker did not start, see {console.name}")
            logger.info("broker started on port %s", port)
            yield port
        finally:
            process.terminate()
            try:
                process.wait(STOP_TIMEOUT)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()
            monitor_thread.join()


def _wait_until_ready(port: int, work_dir: Path) -> None:
    """
    Open a queue directly on the broker, to make sure its cluster is ready.
    """

    with Client(
        "ready",
        ("localhost", port),
        tool_path=required_paths.tool,
        cwd=work_dir,
        dump_messages=False,
    ) as client:
        client.start()
        client.start_session(succeed=True)
        uri = f"bmq://{DOMAIN}/ready"
        client.open(uri, ["write"], succeed=True, timeout=START_TIMEOUT)
        client.close(uri, succeed=True)
        client.stop_session(block=True)


def _record(corpus_dir: Path, work_dir: Path) -> None:
    """
    Run all scenarios through the recording proxy, and write the bytes sent on
    each client connection to 'corpus_dir'.
    """

    corpus_dir.mkdir(parents=True, exist_ok=True)

    with _broker(work_dir) as port:
        _wait_until_ready(port, work_dir)

        proxy = RecordingProxy(port)
        try:
            for name, scenario in SCENARIOS.items():
                with Client(
                    name,
                    ("localhost", proxy.port),
                    tool_path=required_paths.tool,
                    cwd=work_dir,
                ) as client:
                    client.start()
                    client.start_session(succeed=True)
                    scenario(client)
                    client.stop_session(block=True)

                for i, data in enumerate(proxy.take_recordings(RECORD_TIMEOUT)):
                    path = corpus_dir / f"{name}_{i}.bin"
                    path.write_bytes(data)
                    logger.info("recorded %s (%s bytes)", path, len(data))
        finally:
            proxy.close()


def main() -> None:
    parser = argparse.ArgumentParser(
        prog="python -m blazingmq.dev.fuzzcorpus",
        description=__doc__,
        parents=[bul.make_parser()],
    )
    parser.add_argument(
        "corpus_dir", type=Path, help="directory to write the corpus files to"
    )
    parser.add_argument(
        "--build-dir",
        type=Path,
        help="BlazingMQ build directory (default: BLAZINGMQ_BUILD_DIR, or "
        "build/blazingmq in the repository)",
    )
    parser.add_argument(
        "--work-dir",
        type=Path,
        help="directory for the broker files (default: a temporary directory)",
    )
    args = parser.parse_args()

    logging.basicConfig(level=logging.WARNING)
    logger.setLevel(logging.INFO)

    if args.build_dir is not None:
        os.environ["BLAZINGMQ_BUILD_DIR"] = str(args.build_dir.resolve())

    if args.work_dir is not None:
        args.work_dir.mkdir(parents=True, exist_ok=True)
        _record(args.corpus_dir, args.work_dir.resolve())
    else:
        with tempfile.TemporaryDirectory(prefix="bmqfuzzcorpus_") as work_dir:
            _record(args.corpus_dir, Path(work_dir))


if __name__ == "__main__":
    main()
