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
