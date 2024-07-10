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
from typing import Optional

import psutil


def stop_broker(
    path: Path, timeout: int = None, error_ok: bool = True
) -> Optional[int]:
    """
    Stop the broker (using the pid file), first with 'terminate', then, after the
    specified 'timeout', with 'kill'.  In case when timeout is not specified it
    is considered that the broker process should be killed instantly with 'kill'.
    If the specified 'error_ok' flag is True, all exceptions during
    stopping the broker will be suppressed, in case of False it will be re-raised.
    Return the broker's return code or None if not applicable.
    """

    try:
        with (path / "bmqbrkr.pid").open("r") as file:
            pid = int(file.read())

        broker_process = psutil.Process(pid)

        if timeout is not None:
            broker_process.terminate()

            try:
                return broker_process.wait(timeout=timeout)
            except psutil.TimeoutExpired:
                pass

        broker_process.kill()
        return broker_process.wait()

    except Exception:
        if error_ok:
            return None
        raise
