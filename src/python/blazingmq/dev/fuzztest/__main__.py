"""
Main function for running fuzz testing with a BlazingMQ Broker.
usage:
python3 -m blazingmq.dev.fuzztest
    [-h] [--host HOST] [--port PORT] [--broker-dir BROKER_DIR]
    [--broker-cmd BROKER_CMD] [--time-limit TIME_LIMIT]
optional arguments:
  -h, --help                show this help message and exit
  --host HOST               Broker instance HOST for client connection
  --port PORT               Broker instance PORT for client connection
  --broker-dir BROKER_DIR   Working directory containing the built bmqbrkr.tsk,
                            used to start a broker on fuzzing start
  --broker-cmd BROKER_CMD   Command for launching a broker on fuzzing start
  --time-limit TIME_LIMIT   Timeout to deduce if broker is stuck during fuzz test
"""

import argparse
import os
import subprocess
from pathlib import Path
from threading import Thread

import blazingmq.util.logging as bul
from blazingmq.dev import fuzztest
from blazingmq.dev.processtools import stop_broker

BROKER_TERMINATE_TIMEOUT = 10


def launch_broker(broker_cmd: str, broker_dir: str, time_limit: float):
    """
    Launch a BlazingMQ broker instance with the specified 'broker_cmd' from
    the specified 'broker_dir'.  Wait until broker is terminated gracefully
    on fuzzing end or stop fuzzing by the specified 'time_limit' or when the
    broker crashed.
    """

    try:
        subprocess.run(
            broker_cmd.split(), cwd=broker_dir, timeout=time_limit, check=True
        )
    except subprocess.CalledProcessError as ex:
        # Handle the case when terminate is called from the fuzzing script.
        # When the broker is launched via 'run' script, intermediate bash shell
        # is being opened.  So the process tree looks like:
        # fuzzscript -> bash -> bmqbrkr.tsk
        # Return codes are received from the bash process, not from the
        # 'bmqbrkr.tsk'.  There are conventional exit codes for linux utility
        # applications.  In this case:
        # 128+n - Fatal error signal "n"
        # With a SIGTERM = 15 signal, 128+15=143 should be expected.
        if ex.returncode == 143:
            return
    except subprocess.TimeoutExpired:
        stop_broker(Path(broker_dir), BROKER_TERMINATE_TIMEOUT)

    # boofuzz has an inner loop for handling exceptions.
    # Just calling 'exit()' or 'sys.exit()' will generate SystemExit exception
    # which will be handled - this will prevent the application from exit.
    # That is why 'os._exit()' is used instead.
    os._exit(-1)  # pylint: disable=W0212


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

    args = parser.parse_args()

    broker_thread = Thread(
        target=launch_broker,
        args=(
            args.broker_cmd,
            args.broker_dir,
            args.time_limit,
        ),
    )
    broker_thread.start()

    fuzztest.fuzz(args.host, args.port)

    stop_broker(Path(args.broker_dir), BROKER_TERMINATE_TIMEOUT)


if __name__ == "__main__":
    main()
