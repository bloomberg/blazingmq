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
Provide utilities to facilitate launching and manipulating processes.
"""

from pathlib import Path
import signal
from typing import Optional

import psutil


def stop_broker(broker_dir: Path, timeout: int = None) -> Optional[int]:
    """
    Stop the broker (using bmqbrkr.pid file from the `broker_dir` directory):
    - First with `SIGTERM`
    - Then, after the specified `timeout`, with `SIGQUIT`
    - Then, after the specified `timeout`, once more with `SIGKILL`
    In case when timeout is not specified, it is considered that the broker
    process should be killed instantly with 'SIGKILL'.
    Return the broker's return code or None if not applicable.
    """

    with (broker_dir / "bmqbrkr.pid").open("r") as file:
        pid = int(file.read())

    broker_process = psutil.Process(pid)

    if timeout is not None:
        print("Sending SIGTERM to the broker...", flush=True)
        broker_process.terminate()
        try:
            return broker_process.wait(timeout=timeout)
        except psutil.TimeoutExpired:
            print("Broker hasn't exited after the timeout", flush=True)

        print("Sending SIGQUIT to the broker...", flush=True)
        broker_process.send_signal(signal.SIGQUIT)
        try:
            return broker_process.wait(timeout=timeout)
        except psutil.TimeoutExpired:
            print("Broker hasn't exited after the timeout", flush=True)

    print("Sending SIGKILL to the broker...", flush=True)
    broker_process.kill()
    return broker_process.wait()
