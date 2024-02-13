"""
blazingmq.dev.it.process.client


PURPOSE: Provide a BMQ client.

Provide a subclass of 'ito.proc.Process' that wraps 'bmqtool.tsk'.
"""

from contextlib import ExitStack

from collections import namedtuple
import json
import re
import subprocess
from typing import Any, Callable, Dict, Optional, List

import blazingmq.dev.it.process.bmqproc
from blazingmq.dev.it.testconstants import *
from blazingmq.dev.it.util import internal_use, ListContextManager, Queue
import blazingmq.dev.configurator as cfg

Message = namedtuple("Message", "guid, uri, correlationId, payload")
CommandResult = namedtuple("CommandResult", "error_code, matches")

blocktimeout = 15


class ITError(Exception):
    """Base class for exceptions specific to integration tests."""

    pass


def _build_command(
    seed: str, opts: Dict[str, Optional[Callable]], kw: Dict[str, Any]
) -> str:
    """
    Build and return the command string.

    The specified 'seed' is a fixed command prefix.  The specified 'opts' is
    a dictionary of optionally specified transformation functions for
    command arguments that are passed with the specified 'kw' dictionary.
    """

    cmd = [seed]
    for name, value in kw.items():
        if name[-1] == "_":
            bmq_name = name[:-1]
        else:
            first, *rest = name.split("_")
            rest = [word[0].upper() + word[1:] for word in rest]
            bmq_name = "".join([first, *rest])
        transform = opts[bmq_name]
        if transform is not None:
            value = transform(value)
        cmd.append(f"{bmq_name}={value}")
    return " ".join(cmd)


def _bool_lower(b):
    return str(b).lower()


def _quote(value):
    return f'"{value}"'


class Client(blazingmq.dev.it.process.bmqproc.BMQProcess):
    e_SUCCESS = 0
    e_UNKNOWN = -1
    e_TIMEOUT = -2
    e_NOT_CONNECTED = -3
    e_CANCELED = -4
    e_NOT_SUPPORTED = -5
    e_REFUSED = -6
    e_INVALID_ARGUMENT = -7
    e_NOT_READY = -8

    def __init__(self, name, broker: cfg.Broker, options=None, dump_messages=True, **kwargs):
        if options is None:
            options = []
        elif type(options) is str:
            options = options.split("")

        if dump_messages:
            options.append("-d")

        super().__init__(
            name,
                [
                    "bin/bmqtool.tsk",
                    "-b",
                    f"tcp://localhost:{broker.config.port}",
                    f'--logFormat="{blazingmq.dev.it.process.bmqproc.PROC_LOG_FORMAT}"',
                ]
                + options,
            stdin=subprocess.PIPE,
            process_log_category="bmqtool",
            **kwargs,
        )

    ###########################################################################
    # Public API

    def send(self, command):
        """
        Send a command to the client and return immediately.  This function
        returns nothing.
        """
        self._logger.info(f"send: command = {command}")
        self.write_stdin(command + "\n").flush_stdin()

    def start_session(self, block=None, succeed=None, no_except=None, **kw):
        """
        Start the client.  If 'block' is specified and is True, wait for the
        command to complete.  If 'succeed' is specified, wait for the command
        to complete and raise a 'RuntimeError' if the session could not be
        started.  Additional optional argument 'async_' may be specified, which
        will be translated to BMQ option 'async'; see the BMQ documentation for
        more information on this option.
        In blocking mode, the function returns True if the client was
        successfully started and False otherwise.  In non-blocking mode, the
        function always returns None.
        """
        command = _build_command(
            "start",
            {
                "async": _bool_lower,
            },
            kw,
        )
        res = self._command_helper(
            command,
            block,
            r"session.start.*" r"\((-?\d+)\)",
            succeed,
            no_except,
        )
        return res.error_code

    def open(
        self, uri, flags, block=None, succeed=None, no_except=None, timeout=None, **kw
    ):
        """
        Open the queue with the specified 'uri' and the specified 'flags'.  If
        'block' is specified and is True, wait for the command to complete.  If
        'succeed' is specified, wait for the command to complete and raise a
        'RuntimeError' if the queue could not be opened.  The optionally specified
        'timeout' is a maximum waiting time (in seconds) for block/succeed modes,
        if it's not specified, the default timeout is used.

        Additional optional arguments 'async_' (which will be translated to 'async'),
        'max_unconfirmed_messages', 'max_unconfirmed_bytes', and
        'consumer_priority' may be specified; see the BMQ documentation for
        more information on these options.  In blocking mode, the function
        returns True if the queue was successfully opened and False otherwise.
        In non-blocking mode, the function always returns None.
        """
        command = _build_command(
            'open uri="{}" flags="{}"'.format(uri, ",".join(flags)),
            {
                "async": _bool_lower,
                "maxUnconfirmedMessages": None,
                "maxUnconfirmedBytes": None,
                "consumerPriority": None,
                "subscriptions": json.dumps,
            },
            kw,
        )
        res = self._command_helper(
            command,
            block,
            f"<--.*openQueue.*uri = {re.escape(uri)}.*" r"\((-?\d+)\)",
            succeed,
            no_except,
            timeout=timeout,
        )
        return res.error_code

    def configure(
        self, uri, block=None, succeed=None, no_except=None, timeout=None, **kw
    ):
        """
        Configure the queue with the specified 'uri'.  If 'block' is specified
        and is True, wait for the command to complete.  If 'succeed' is
        specified, wait for the command to complete and raise an 'ITError'
        if the queue could not be configured.  The optionally specified 'timeout'
        is a maximum waiting time (in seconds) for block/succeed modes, if it's
        not specified, the default timeout is used.

        Additional optional arguments 'async_' (which will be translated to 'async'),
        'max_unconfirmed_messages', 'max_unconfirmed_bytes', and
        'consumer_priority' may be specified; see the BMQ documentation for
        more information on these options.  In blocking mode, the function
        returns True if the queue was successfully configured and False
        otherwise.  In non-blocking mode, the function always returns None.
        """
        command = _build_command(
            f'configure uri="{uri}"',
            {
                "async": _bool_lower,
                "maxUnconfirmedMessages": None,
                "maxUnconfirmedBytes": None,
                "consumerPriority": None,
                "subscriptions": json.dumps,
            },
            kw,
        )
        res = self._command_helper(
            command,
            block,
            f"<--.*configureQueue.*uri = {re.escape(uri)}.*" r"\((-?\d+)\)",
            succeed,
            no_except,
            timeout,
        )
        return res.error_code

    def post(
        self,
        uri,
        payload,
        block=None,
        succeed=None,
        no_except=None,
        wait_ack=None,
        **kw,
    ):
        """
        Post a message to the queue with the specified 'uri' with the specified
        'payload'.  If 'block' is specified and is True, wait for the command
        to complete.  If 'succeed' is specified, wait for the command to
        complete and raise a 'RuntimeError' if the queue could not be posted.
        If 'wait_ack' is specified, also block until an ACK is received for
        the queue.  Additional optional arguments 'async_' (which will be
        translated to 'async') and 'properties' may be specified; see the BMQ
        documentation for more information on these options.
        In blocking mode, the function returns True if the queue was
        successfully posted and False otherwise.  In non-blocking mode, the
        function always returns None.
        """
        succeed = succeed or wait_ack
        extra_patterns = ["ACK #.* type = ACK.*"] if wait_ack else None
        command = _build_command(
            f'post uri="{uri}" payload={json.dumps(payload)}',
            {
                "async": _bool_lower,
                "compressionAlgorithmType": _quote,
                "messageProperties": json.dumps,
            },
            kw,
        )
        res = self._command_helper(
            command,
            block,
            r"<--.*post.*\((-?\d+)\)",
            succeed,
            no_except,
            extra_patterns,
        )
        error_code = res.error_code
        if wait_ack:
            ack = res.matches[0]
            ack_success = ack and "status = SUCCESS" in ack.group(0)
            error_code = Client.e_SUCCESS if ack_success else Client.e_UNKNOWN
        return error_code

    def list(self, uri=None, block=None):
        """
        Send the 'list' command with the 'uri' argument, if specified.
        If 'block' is specified and is True, wait for the command
        to complete and return the messages as an array of tuples.
        """

        with internal_use(self):
            self.send("list" + f' uri="{uri}"' if uri else "list")

            if not block:
                return

            m = self.capture(
                r"Unconfirmed message listing: (-?\d+) messages", timeout=blocktimeout
            )
            if m is None:
                self._error(f"list did not complete within {blocktimeout}s")
                return []

            msgs = []
            for i in range(0, int(m[1])):
                m = self.capture(
                    "\\[([0-9a-fA-F]+)\\] Queue: '\\[ uri = (\\S+) correlationId = \\[ autoValue = (-?\\d+) \\] \\]' = '([^']*)'",
                    timeout=blocktimeout,
                )
                if m is None:
                    self._error(f"list timed out while waiting for message #{i + 1}")
                    return []

                msgs.append(Message(m[1], m[2], m[3], m[4]))

            self._logger.info(f"list -> {len(msgs)} message(s)")

            return msgs

    def confirm(self, uri, guid, block=None, succeed=None, no_except=None):
        command = _build_command(f'confirm uri="{uri}" guid="{guid}"', {}, {})
        with internal_use(self):
            res = self._command_helper(
                command,
                block,
                r"<--.*confirmMessage.*\((-?\d+)\)",
                succeed,
                no_except,
            )
            return res.error_code

    def close(self, uri, block=None, succeed=None, no_except=None, **kw):
        command = _build_command(
            f'close uri="{uri}"',
            {
                "async": _bool_lower,
            },
            kw,
        )
        with internal_use(self):
            res = self._command_helper(
                command,
                block,
                f"<--.*closeQueue.*uri = {re.escape(uri)}.*" r"\((-?\d+)\)",
                succeed,
                no_except,
            )
        return res.error_code

    def stop_session(self, block=None, **kw):
        """
        Stop the client.  If 'block' is specified and is True, wait for the
        command to complete.  Additional optional argument 'async_' (which will
        be translated to 'async') may be specified; see the BMQ documentation
        for more information on this option. This functions returns nothing.
        """
        command = _build_command(
            "stop",
            {
                "async": _bool_lower,
            },
            kw,
        )
        res = self._command_helper(
            command,
            block,
            r"<--.*stop\(\)",
            None,
            None,
        )
        return res.error_code

    def wait_push_event(self, timeout=blocktimeout, quiet=False):
        """
        Wait until the client receives a 'PUSH' events.  Return 'True' if the
        event was seen within the specified 'timeout'.
        """
        action = "waiting for a PUSH event"
        self._logger.info(action)
        with internal_use(self):
            if self.outputs_regex("PUSH #", timeout):
                return True
        if not quiet:
            self._logger.warning(f"TIMEOUT: timed out after {timeout}s while {action}")
        return False

    def wait_state_restored(self, timeout=blocktimeout):
        action = "waiting for STATE_RESTORED event"
        self._logger.info(action)
        with internal_use(self):
            if self.outputs_regex("EVENT received:.*STATE_RESTORED", timeout=timeout):
                return True
        self._logger.info(f"TIMEOUT: timed out after {timeout}s while {action}")
        return False

    def wait_connection_lost(self, timeout=blocktimeout):
        action = "waiting for CONNECTION_LOST event"
        self._logger.info(action)
        with internal_use(self):
            if self.outputs_regex("EVENT received:.*CONNECTION_LOST", timeout=timeout):
                return True
        self._logger.info(f"TIMEOUT: timed out after {timeout}s while {action}")
        return False

    def open_priority_queues(self, count, start=0, **kw):
        """
        Open *distinct* priority queues with the options specified in 'kw'.
        While each queue uses a different URI, calling this method multiple
        times, on any client, proxy or cluster, with the same 'start' value
        will result in the same URIs to be used.

        Return a list of 'count' Queue objects.  The list is wrapped by a class
        that implement the ContextManager.  Using the value returned by this
        method in a 'with' statement will ensure that the queues are closed at
        the end of the block.
        """
        return ListContextManager(
            [
                Queue(self, f"{URI_PRIORITY}{i}", **kw)
                for i in range(start, start + count)
            ]
        )

    def open_broadcast_queues(self, count, start=0, **kw):
        """
        Open *distinct* broadcast queues with the options specified in 'kw'.
        While each queue uses a different URI, calling this method multiple
        times, on any client, proxy or cluster, with the same 'start' value
        will result in the same URIs to be used.

        Return a list of 'count' Queue objects.  The list is wrapped by a class
        that implement the ContextManager.  Using the value returned by this
        method in a 'with' statement will ensure that the queues are closed at
        the end of the block.
        """
        return ListContextManager(
            [
                Queue(self, f"{URI_BROADCAST}{i}", **kw)
                for i in range(start, start + count)
            ]
        )

    def open_fanout_queues(self, count, start=0, appids=None, **kw):
        """
        Open *distinct* fanout queues with the options specified in 'kw'.
        While each queue uses a different URI, calling this method multiple
        times, on any client, proxy or cluster, with the same 'start' value
        will result in the same URIs to be used.

        If 'appids' is None, return a list of 'count' Queue objects.
        Otherwise, 'appids' is assumed to be a list; return a list of 'count'
        lists, each consisting of the one Queue object per app id.

        Whether a list of Queue or a list of lists of Queues is returned, all
        lists are wrapped by a class that implement the ContextManager
        protocol.  Using the value returned by this method in a 'with'
        statement will ensure that the queues are closed at the end of the
        block.
        """
        return ListContextManager(
            [Queue(self, f"{URI_FANOUT}{i}", **kw) for i in range(start, start + count)]
            if appids is None
            else [
                ListContextManager(
                    [Queue(self, f"{URI_FANOUT}{i}?id={id}", **kw) for id in appids]
                )
                for i in range(start, start + count)
            ]
        )

    def exit_gracefully(self):
        with internal_use(self):
            self.send("quit")

    ###########################################################################
    # Internals

    def _command_helper(
        self,
        command: str,
        block: Optional[bool],
        pattern: str,
        succeed: Optional[bool],
        no_except: Optional[bool],
        extra_patterns: Optional[List[str]] = None,
        timeout: Optional[int] = None,
    ) -> CommandResult:
        """
        Send the specified 'command' and wait for result if needed.
        Return the CommandResult named tuple containing valid error_code in
        blocking mode or None error_code if blocking mode is disabled.

        The specified 'block' flag ensures waiting until the command is
        executed on a remote client.  The specified 'pattern' is a regular
        expression that is captured on client logs to check if command was
        executed successfully.  The optionally specified 'timeout' is a maximum
        waiting time (in seconds) for pattern search.

        If 'succeed' is specified, check that the command result is correct and
        raise an 'ITError' otherwise.  The specified 'no_except' flag
        suppresses the exception raising.

        The optionally specified 'extra_patterns' is a list of regular
        expressions that must be captured together with the main 'pattern'.

        Note: there are several situations when we send the command in a
        blocking mode:
        - When 'block' is True (this doesn't ensure success check)
        - When 'succeed' is not None (need to block to capture success pattern)
        - When 'extra_patterns' list is present and at least one extra pattern
        is passed (need to block to capture extra patterns)

        Note: the reason why 'extra_patterns' exist is that sometimes we need
        to capture several different regular expressions together.  We might
        not know the exact order of these lines in a log so serial one-by-one
        search for expressions might fail.  This problem could be solved by
        storing previous match line number for each exact expression or by
        expression subscriptions, but it requires additional effort.
        """

        # The 0-indexed expression is reserved for success pattern.
        all_patterns = [pattern]
        if isinstance(extra_patterns, list):
            all_patterns += extra_patterns

        block = block or (succeed is not None) or (len(all_patterns) > 1)

        self.send(command)

        if not block:
            return CommandResult(None, None)

        with internal_use(self):
            # The optionally specified 'timeout' takes priority over the default one.
            matches = self.capture_n(all_patterns, timeout=timeout or blocktimeout)
            result = matches[0]
            extra_matches = matches[1:]

            error_code = self._parse_command_result(
                command, result, succeed, no_except, timeout
            )
            self._logger.info(f"{command} -> {error_code}")

        return CommandResult(error_code, extra_matches)

    def _parse_command_result(
        self,
        command: str,
        result: Optional[re.Match],
        succeed: Optional[bool],
        no_except: Optional[bool],
        timeout: int,
    ) -> int:
        """
        Parse the specified 'result' of the specified 'command' execution.

        If 'succeed' is specified, check that the command result is correct and
        raise a 'RuntimeError' otherwise.  The specified 'no_except' flag
        suppresses the exception raising.

        Return the error code.
        """

        def raise_if_enabled(message):
            if not no_except:
                self._error(message, exception=ITError)

        if not result:
            raise_if_enabled(f"{command} did not complete within {timeout}s")
            return Client.e_UNKNOWN

        error_code = Client.e_SUCCESS
        if len(result.groups()) > 0:
            error_code = int(result[1])

        if succeed is None:
            return error_code

        if succeed:
            if error_code != Client.e_SUCCESS:
                # No need to set 'error_code' to some other value because it is
                # already contains the fail reason
                raise_if_enabled(f"{command} did not succeed")
        else:
            if error_code == Client.e_SUCCESS:
                # Something is not working as intended, the problem is unknown
                error_code = Client.e_UNKNOWN
                raise_if_enabled(f"{command} succeeded but should have failed")
        return error_code
