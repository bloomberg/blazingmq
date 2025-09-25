# Copyright 2024 Bloomberg Finance L.P.
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
Main function for running fuzz testing with a BlazingMQ Broker.
usage:
python3 -m blazingmq.dev.fuzztest
    [-h] [--host HOST] [--port PORT] [--broker-dir BROKER_DIR]
    [--broker-cmd BROKER_CMD] [--time-limit TIME_LIMIT] [--request REQUEST]
optional arguments:
  -h, --help                show this help message and exit
  --host HOST               Broker instance HOST for client connection
  --port PORT               Broker instance PORT for client connection
  --broker-dir BROKER_DIR   Working directory containing the built bmqbrkr.tsk,
                            used to start a broker on fuzzing start
  --broker-cmd BROKER_CMD   Command for launching a broker on fuzzing start
  --time-limit TIME_LIMIT   Timeout to deduce if broker is stuck during fuzz test
  --request REQUEST         If specified, the name of the request to fuzz, fuzz all
                            the known requests otherwise
"""

import argparse
import queue
import subprocess
import sys
from pathlib import Path
from threading import Thread

import blazingmq.util.logging as bul
from blazingmq.dev import fuzztest
from blazingmq.dev.processtools import stop_broker

BROKER_TERMINATE_TIMEOUT = 10


class FuzzError(RuntimeError):
    pass


def start_broker(
    result_queue: queue.Queue, broker_dir: Path, broker_cmd: str, time_limit: float
):
    """
    Launch a BlazingMQ broker instance with the specified 'broker_cmd' from
    the specified 'broker_dir'.  Wait until broker is terminated gracefully
    on fuzzing end or stop fuzzing by the specified 'time_limit' or when the
    broker crashed.  Use the provided `result_queue` to return the error code
    to the caller thread.
    """

    try:
        subprocess.run(
            broker_cmd.split(), cwd=broker_dir, timeout=time_limit, check=True
        )
        print("Broker exited gracefully")  # e.g. due to a stop_broker() call
        result_queue.put(0)
    except subprocess.CalledProcessError as ex:
        # Broker exited with non-zero exit code.
        # This could be due the broker calling abort, crashing, etc. The broker
        # should not exit with a non-zero exit code during graceful shutdown.
        print(f"Broker command exited with non-zero exit code: {ex.returncode}")
        result_queue.put(ex.returncode)
    except subprocess.TimeoutExpired:
        print("Fuzz test run timeout expired, trying to stop the broker...", flush=True)
        stop_broker(broker_dir, BROKER_TERMINATE_TIMEOUT)
        result_queue.put(-1)


def main():
    """
    Main program.
    Parse CMD args and launch BlazingMQ Broker for Fuzz Test.
    """

    parser = argparse.ArgumentParser(
        prog=f"python3 -m {fuzztest.__name__}",
        description="Fuzzing test script",
        parents=[bul.make_parser()],
    )

    parser.add_argument(
        "--host",
        default="localhost",
        type=str,
        action="store",
        metavar="HOST",
        help="Broker instance HOST for client connection",
    )

    parser.add_argument(
        "--port",
        default=30114,
        type=int,
        action="store",
        metavar="PORT",
        help="Broker instance PORT for client connection",
    )

    parser.add_argument(
        "--broker-dir",
        default="../../cmake.bld/Linux/src/applications/bmqbrkr",
        type=str,
        action="store",
        metavar="BROKER_DIR",
        help="Working directory containing the built bmqbrkr.tsk used to start a broker on fuzzing start",
    )

    parser.add_argument(
        "--broker-cmd",
        default="./run info",
        type=str,
        action="store",
        metavar="BROKER_CMD",
        help="Command for launching a broker on fuzzing start",
    )

    parser.add_argument(
        "--time-limit",
        default=60 * 60,
        type=float,
        action="store",
        metavar="TIME_LIMIT",
        help="Timeout to deduce if broker is stuck during fuzz test",
    )

    parser.add_argument(
        "--request",
        default=None,
        type=str,
        action="store",
        metavar="REQUEST",
        help="If specified, the name of the request to fuzz, fuzz all the known requests otherwise",
    )

    args = parser.parse_args()

    broker_dir = Path(args.broker_dir)
    if not broker_dir.exists():
        raise FuzzError(
            f"The provided broker dir '{broker_dir}' doesn't exist, please specify "
            "a directory containing bmqbrkr.tsk with --broker-dir"
        )

    if not (broker_dir / "bmqbrkr.tsk").exists():
        raise FuzzError(
            f"The provided broker dir '{broker_dir}' doesn't contain a built broker "
            "bmqbrkr.tsk, please specify a directory containing bmqbrkr.tsk with --broker-dir"
        )

    result_queue = queue.Queue()

    broker_thread = Thread(
        target=start_broker,
        args=(
            result_queue,
            broker_dir,
            args.broker_cmd,
            args.time_limit,
        ),
    )
    broker_thread.start()

    fuzztest.fuzz(args.host, args.port, args.request)

    stop_broker(broker_dir, BROKER_TERMINATE_TIMEOUT)

    broker_thread.join(timeout=BROKER_TERMINATE_TIMEOUT)

    rc = result_queue.get()
    sys.exit(rc)


if __name__ == "__main__":
    main()
