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
Pytest fixtures for fuzz testing the BlazingMQ broker.
"""

import queue
import subprocess
from dataclasses import dataclass
from threading import Thread

import pytest

from blazingmq.dev.paths import paths
from blazingmq.dev.processtools import stop_broker

BROKER_DEFAULT_PORT = 30114
BROKER_TERMINATE_TIMEOUT = 10
BROKER_TIME_LIMIT = 60 * 60  # 1 hour


@dataclass
class BrokerInfo:
    host: str
    port: int


def _run_broker(result_queue, broker_dir, broker_cmd, time_limit):
    """Run the broker and put the exit code on the result queue."""
    try:
        subprocess.run(
            broker_cmd.split(), cwd=broker_dir, timeout=time_limit, check=True
        )
        result_queue.put(0)
    except subprocess.CalledProcessError as ex:
        result_queue.put(ex.returncode)
    except subprocess.TimeoutExpired:
        stop_broker(broker_dir, BROKER_TERMINATE_TIMEOUT)
        result_queue.put(-1)


@pytest.fixture
def broker():
    """
    Start a BlazingMQ broker, yield connection info, then stop it.

    The broker directory is resolved from BLAZINGMQ_BUILD_DIR (set by cmake
    or the user) via blazingmq.dev.paths, falling back to build/blazingmq
    relative to the repository root.
    """
    broker_dir = paths.broker.parent

    if not broker_dir.exists():
        pytest.fail(
            f"Broker directory '{broker_dir}' does not exist. "
            "Set BLAZINGMQ_BUILD_DIR or build with: cmake --build --preset default"
        )

    if not (broker_dir / "bmqbrkr.tsk").exists():
        pytest.fail(
            f"'{broker_dir}' does not contain bmqbrkr.tsk. "
            "Build with: cmake --build --preset default"
        )

    result_queue = queue.Queue()
    broker_thread = Thread(
        target=_run_broker,
        args=(result_queue, broker_dir, "./run", BROKER_TIME_LIMIT),
    )
    broker_thread.start()

    yield BrokerInfo(host="localhost", port=BROKER_DEFAULT_PORT)

    stop_broker(broker_dir, BROKER_TERMINATE_TIMEOUT)
    broker_thread.join(timeout=BROKER_TERMINATE_TIMEOUT)

    rc = result_queue.get()
    assert rc == 0, f"Broker exited with non-zero return code: {rc}"
