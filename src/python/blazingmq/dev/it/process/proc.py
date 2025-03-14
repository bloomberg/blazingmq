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
blazingmq.dev.it.process.proc


PURPOSE: Provide a means for executing and monitoring processes.

TYPES:
    Process: start a process and check for patterns on standard output
    ProcessExitError: exception raised if process exited with non-zero code

Provide means to start the process and read the standard output. Standard
output can be read line-by-line via an iterator or, alternatively, examined
for specific patterns only.

ACKNOWLEDGEMENTS:
    This component was originally written by the BAS team.

Usage
-----

This section illustrates intended use of this module.

Example 1: Unix 'ls' Command
- - - - - - - - - - - - - -

This example illustrates the creation of a 'Process' to execute and monitor
the Unix 'ls' (list directory contents) command for output. The function
takes a path and a list of file names, and returns 'True' if the specified
path contains the specified list of file names, and 'False' otherwise.

```
import pathlib
import typing

from blazingmq.dev.it.process.proc import Process, Command

def contains(path: pathlib.Path, filenames: typing.List[str]) -> bool:
    # Note that 'ls' output is in sorted order
    with Process('ls', ['ls', '-1', '-a', str(path)]) as proc:
            return all([proc.outputs_substr(filename) for filename in sorted(filenames)])
```
"""
import abc
from contextlib import ExitStack, suppress
import inspect
import logging
import queue
import os
import re
import signal
import subprocess
import threading
import time
from typing import Any, IO, Iterator, List, Optional, Union

from blazingmq.dev.it.logging import BallLoggerAdapter


# Same as used in Popen.__init__
_FILE = Union[None, int, IO[Any]]
_DEFAULT_LONG_TIMEOUT = 120


class _StdoutSentinel:
    pass


def _format_rc(rc):
    return f"{rc} ({signal.strsignal(-rc)})" if rc < 0 else str(rc)


class ProcessExitError(Exception):
    """This class implements a custom exception.
    The 'ProcessExitError' exception is raised whenever process exits with
    a non-zero status code.
    """

    def __init__(self, name, return_code):
        """Create an instance of 'ProcessExitError' having the specified
        'name', corresponding to the specified 'pid'. Optionally specify
        a 'return_code', corresponding to the exit status of the named process.
        """
        self.name = name
        self.return_code = return_code

    def __str__(self):
        return f"{self.name}: process exited with return code {_format_rc(self.return_code)}"


def launder_log_line(line):
    try:
        return line.decode("utf-8")
    except UnicodeDecodeError as error:
        return (
            line[: error.start].decode("utf-8")
            + "*" * (error.end - error.start)
            + launder_log_line(line[error.end :])
        )


class Process:
    """This class provides an interface for executing, monitoring,
    and examining the output of a process.

    A 'Process' is any executable program, specified by a 'command' according
    to the Python 'subprocess' documentation.

    The 'outputs_regex()' method can be used to check whether a particular
    message has been written to standard output within a specified period of
    time, by providing a regular expression. Similarly, the 'outputs_substr()'
    method can be used to check if a particular substring appears in the output
    without requiring the caller to manually escape special regex characters.

    'Process' objects are intended to be utilized as context managers, in which
    case managed resources are automatically released. Alternatively, 'Process'
    objects may be executed in a try/finally block using 'wait()' or
    'terminate()' to release managed resources.
    """

    def __init__(
        self,
        name,
        command,
        stdin=None,
        read_timeout=5.0,
        wait_timeout=15.0,
        check_exit_code=True,
        cwd=None,
        env=None,
        shell=None,
    ):
        self.name = name
        self._command = list(map(str, command))
        self._stdin = stdin
        self._read_timeout = read_timeout
        self._wait_timeout = wait_timeout
        self.check_exit_code = check_exit_code
        self._cwd = cwd
        self._env = env
        self._shell = shell
        self._async_log_hooks = set()
        self._sync_log_hooks = set()
        self._log_level = logging.INFO
        # Give a chance to subclass to override the logger.  For example, we
        # prefer `Client` to log using its own logger.
        self._internal_logger = logging.LoggerAdapter(
            logging.getLogger(self.__class__.__module__), {"bmqprocess": name}
        )
        self._logger = BallLoggerAdapter(
            logging.getLogger("blazingmq.test"),
            extra={
                "bmqprocess": self.name,
                "ball_overrides": {"processName": self.name},
                "blp_log_from": inspect.getfile(type(self)),
            },
        )

    @property
    def pid(self):
        """
        Return the id of the process involved in the test.  This defaults to
        the id of the process started by the constructor, but may be
        overridden.
        """
        return self._process.pid

    def start(self):
        """
        Start a process which executes the specified 'command' identified, for
        logging purposes, by the specified 'name'. The specified
        'read_timeout' and 'wait_timeout' define default timeouts for stdout
        reads and 'wait()' call. If the specified 'check_exit_code' is True,
        on call to wait() ProcessExitError will be raised if sub-process exits
        with code other than zero. Optionally specify 'cwd' which sets the
        current directory for the process. Optionally specify 'env' dictionary
        used as a custom environment for the 'command'.
        """

        self._internal_logger.debug(
            f"Starting process: command = {' '.join(self._command)}"
        )

        # Start the subprocess and a thread to read and buffer its output.
        self._process = subprocess.Popen(
            self._command,
            stdin=self._stdin,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            shell=self._shell,
            env=self._env,
            cwd=self._cwd,
            bufsize=0,
            universal_newlines=False,
        )

        self._internal_logger.info(f"Current pid = {self._process.pid}")

        self._queue = queue.Queue()

        self._stdout_thread = threading.Thread(
            target=self.__stdout_reader, name=f"{self.name}-stdout"
        )
        self._stdout_thread.start()

        self._stderr_thread = threading.Thread(
            target=self.__stderr_reader, name=f"{self.name}-stderr"
        )
        self._stderr_thread.start()

    def __stdout_reader(self):
        """Read from the pipe '_process.stdout' until pipe is closed.  Launder
        each line, pass it to 'log_stdout' and the registered hooks, and post
        to '_queue'.
        """

        with ExitStack() as on_exit:

            def exit_scope():
                if self.check_exit_code and self.returncode:
                    self._logger.error(
                        f"exited with return code {_format_rc(self.returncode)}"
                    )

                self._queue.put(_StdoutSentinel())
                self._logger.debug("stop reading stdout")

            on_exit.callback(exit_scope)

            while True:
                line = self._process.stdout.readline()

                if not line:
                    return

                line = launder_log_line(line).rstrip("\n")

                for hook in self._async_log_hooks:
                    hook(line)

                self.log_stdout(line)
                self._queue.put(line)

    def __stderr_reader(self):
        """Read from the pipe '_process.stderr' until pipe is closed.  Pass
        each line to 'log_stderr'.
        """

        with ExitStack() as on_exit:
            on_exit.callback(lambda: self._logger.debug("stop reading stderr"))

            while True:
                line = self._process.stderr.readline().decode("utf-8")

                if not line:
                    return

                self.log_stderr(line)

    def log_stdout(self, line):
        pass

    def log_stderr(self, line):
        pass

    def __enter__(self):
        return self

    def __exit__(self, *args):
        self.stop()

    def write_stdin(self, line):
        self._process.stdin.write((line + "\n").encode("utf-8"))
        return self

    def flush_stdin(self):
        self._process.stdin.flush()
        return self

    def capture(self, pattern, timeout=_DEFAULT_LONG_TIMEOUT):
        """Return a corresponding regular expression match if the specified
        regular expression 'pattern' is observed in the output.

        The optional 'timeout' specifies the approximate time after which
        'False' is returned if the 'pattern' was not observed.
        """
        return self.capture_n([pattern], count=1, timeout=timeout)[0]

    def capture_n(
        self, patterns, count=None, timeout=_DEFAULT_LONG_TIMEOUT, warn_on_timeout=True
    ) -> List[Optional[re.Match]]:
        """Scan the output of this process until the specified 'count' of patterns has
        been matched or the specified 'timeout' (in seconds) has elapsed.
        Return a list consisting of the match result for each corresponding
        pattern, or 'None' if no match occurred.
        If not specified, 'count' defaults to the length of 'patterns'.
        If not specified, timeout defaults to 120 seconds.
        """
        if count is None:
            count = len(patterns)

        self._logger.log(
            self._log_level, f"capture: {patterns}, count={count}, timeout={timeout}..."
        )

        assert count > 0
        assert count <= len(patterns)

        captured = 0

        regexes = [re.compile(pattern) for pattern in patterns]
        results = [None] * len(patterns)
        time_remaining = timeout or self._read_timeout

        def scan_line(line, where):
            nonlocal captured
            for i, regex in enumerate(regexes):
                if results[i] is not None:
                    continue
                match = regex.search(line)
                if match:
                    self._logger.log(
                        self._log_level, f"captured{where}: {match.group(0)}"
                    )
                    # This code overwrites previously found matches, but also
                    # increments the counter
                    results[i] = match
                    captured += 1
                    if captured == count:
                        return True
            return False

        for line in self.get_output(0):
            if scan_line(line, " [BACKLOG]"):
                return results

        start_time = time.time()

        for line in self.get_output(time_remaining):
            if scan_line(line, ""):
                return results
            if timeout is not None:
                time_remaining = timeout - (time.time() - start_time)
                if time_remaining < 0:
                    if warn_on_timeout:
                        self._logger.warning("Wait timed out.")
                    return results

        return results

    def outputs_regex(self, pattern, timeout=_DEFAULT_LONG_TIMEOUT):
        """Return 'True' if the specified regular expression 'pattern' is
        observed in the output.

        The optional 'timeout' specifies the approximate time after which
        'False' is returned if the 'pattern' was not observed. The actual
        timeout may be between the value of 'timeout' and ('read_timeout' +
        'timeout'). The optionally specified 'read_timeout' limits the
        maximum wait for stdout pipe read. If the optional 'timeout' is
        'None' or not specified, the parameter 'read_timeout' supplied to the
        constructor will be used.
        """
        return self.capture(pattern, timeout) is not None

    def outputs_substr(self, string, timeout=_DEFAULT_LONG_TIMEOUT):
        """Return 'True' if the specified string 'string' is a substring of
        the output.

        The optional 'timeout' specifies the approximate time after which
        'False' is returned if the 'string' was not observed. The actual
        timeout may be between the value of 'timeout' and ('read_timeout' +
        'timeout'). The optionally specified 'read_timeout' limits the
        maximum wait for stdout pipe read. If the optional 'timeout' is
        'None' or not specified, the parameter 'read_timeout' supplied to the
        constructor will be used.
        """
        return self.outputs_regex(re.escape(string), timeout)

    def suspend(self):
        """
        Send the STOP signal to the broker.
        """
        self._logger.info(f"sending SIGSTOP to {self.pid}")
        os.kill(self.pid, signal.SIGSTOP)

    def resume(self):
        """
        Send the CONT signal to the broker.
        """
        self._logger.info("sending SIGCONT")
        os.kill(self.pid, signal.SIGCONT)

    def kill(self):
        """
        Send the KILL signal to the broker.
        """

        self._logger.info("sending SIGKILL")
        os.kill(self.pid, signal.SIGKILL)

    def __iter__(self):
        return self.get_output()

    def get_output(self, timeout: Optional[float] = None) -> Iterator[str]:
        """Return iterator over lines of stdout as strings.

        The optionally specified 'timeout' limits the maximum wait for stdout
        pipe read. If the optional 'timeout' is 'None' or not specified,
        the parameter 'read_timeout' supplied to the constructor will be used.
        """

        read_timeout = timeout if timeout is not None else self._read_timeout

        while True:
            try:
                value = self._queue.get(timeout=read_timeout)
            except queue.Empty:
                return

            self.raise_if_exited_in_error()

            if isinstance(value, _StdoutSentinel):
                return

            for hook in self._sync_log_hooks:
                hook(value)

            yield value

    def wait(self, timeout: Optional[float] = None) -> int:
        """Wait for the subprocess to terminate for the duration
        of the specified 'timeout'.

        The optionally specified 'timeout' limits the maximum wait for stdout
        pipe read. If the optional 'timeout' is 'None' or not specified,
        the parameter 'read_timeout' supplied to the constructor will be used.
        """

        if self.returncode is None:
            wait_timeout = timeout if timeout is not None else self._wait_timeout

            try:
                self._process.wait(timeout=wait_timeout)
            except subprocess.TimeoutExpired:
                return 1

            self._internal_logger.debug(
                "waiting for stdout consuming thread to exit..."
            )
            self._stdout_thread.join()
            self._stderr_thread.join()

        if self.check_exit_code:
            if not self.is_alive() and self.returncode != 0:
                self._logger.debug(
                    f"Process exited with non-zero code {_format_rc(self.returncode)}."
                )
                return self.returncode

        return self.returncode

    def stop(self):
        self._logger.log(self._log_level, "Stopping...")
        with suppress():
            self.exit_gracefully()

        self.wait()
        self.raise_if_exited_in_error()

    @abc.abstractmethod
    def exit_gracefully(self):
        raise NotImplementedError("subclass responsibility")

    def add_async_log_hook(self, hook):
        self._async_log_hooks.add(hook)

    def remove_async_log_hook(self, hook):
        self._async_log_hooks.remove(hook)

    def add_sync_log_hook(self, hook):
        self._sync_log_hooks.add(hook)

    def remove_sync_log_hook(self, hook):
        self._sync_log_hooks.remove(hook)

    def is_alive(self) -> bool:
        """Return True if subprocess is alive."""
        return self._process.poll() is None

    def raise_if_exited_in_error(self):
        if self.check_exit_code and self._process.poll():  # None or 0: OK
            raise ProcessExitError(self.name, self.returncode)

    def drain(self):
        """
        Read and discard the log, until no log entry is produced for the
        specified 'timeout'.
        """
        drained, self._queue = self._queue, queue.Queue()
        if drained.qsize() > 0:
            self._logger.log(self._log_level, f"drained {drained.qsize()} lines")

    def stack_trace(self, limit=None):
        """
        Print a stack trace to the log.  If specified, print only the 'limit'
        first stack entries for each thread.
        """
        ps = subprocess.Popen(
            ["/bin/pstack", str(self.pid)], stdout=subprocess.PIPE, encoding="utf-8"
        )
        while True:
            line = ps.stdout.readline()
            if line == "":
                break
            if line.startswith("Thread"):
                n = 0
            if limit is None or n <= limit:
                self._logger.log(self._log_level, line[:-2])
                n += 1

    @property
    def returncode(self) -> Optional[int]:
        """Return exit code of the process if process exited, None otherwise."""
        return self._process.returncode

    def force_stop(self) -> None:
        """Terminate the process by force."""

        self._logger.warning("Killing process.")
        self.check_exit_code = False

        try:
            self._process.kill()
            self.wait()
        except ProcessLookupError:
            pass

    def _error(self, message, exception=RuntimeError):
        self._logger.error(f"Raising error {message}...")
        raise exception(message)
