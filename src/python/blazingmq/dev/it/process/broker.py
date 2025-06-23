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
blazingmq.dev.it.process.broker


PURPOSE: Provide a means for interacting with a broker.

Provide a subclass of 'ito.proc.Process' with methods for common interactions
with a broker: sending commands, waiting until a leader is elected, etc.
"""

import itertools
import os
from pathlib import Path
import signal

from typing import Optional, TypeVar
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from blazingmq.dev.it.cluster import Cluster

from blazingmq.dev.it.process import proc
import blazingmq.dev.it.process.bmqproc
import blazingmq.dev.it.testconstants as tc
import blazingmq.dev.configurator.configurator as cfg

from blazingmq.dev.it.util import internal_use, ListContextManager, Queue

BLOCK_TIMEOUT = 60
START_TIMEOUT = 120


def open_non_blocking(path, flags):
    """Open the file at the specified 'path' with the specified 'flags' in a
    non-blocking mode. Return a file descriptor to the opened file.
    """
    return os.open(path, flags | os.O_NONBLOCK)


class Broker(blazingmq.dev.it.process.bmqproc.BMQProcess):
    Self = TypeVar("Self", bound="Broker")

    config: cfg.Broker
    cluster: "Cluster"
    cluster_name: str
    _pid: Optional[int]
    _auto_id: itertools.count
    last_known_leader: Optional[Self]
    last_known_active: Optional[str]

    def __init__(self, config: cfg.Broker, cluster: "Cluster", **kwargs):
        cwd: Path = kwargs["cwd"]
        (cwd / "bmqbrkr.ctl").unlink(missing_ok=True)
        super().__init__(
            config.name,
            ["bin/bmqbrkr.tsk", "etc"],
            process_log_category="bmqbrkr",
            **kwargs,
        )
        self.config = config
        self.cluster = cluster
        self.cluster_name = cluster.name
        self._pid = None
        self._auto_id = itertools.count(1)

        if len(cluster.config.nodes) == 1:
            self.last_known_leader = self
            self.last_known_active = config.name
        else:
            self.last_known_leader = None
            self.last_known_active = None

    def __str__(self):
        return f"Broker({self.name})"

    def __repr__(self):
        return f"Broker({self.name})"

    @property
    def datacenter(self) -> str:
        return self.config.config.app_config.host_data_center

    @property
    def pid(self):
        """
        Return the id of the *broker* process, as read from the 'bmqbrkr.pid'
        file.
        IMPLEMENTATION NOTE: The broker is started via a script that sets up
        the environment and creates directories for storage and logs, then runs
        the broker proper via the 'exec' shell command.  Depending on the shell
        in use, a new process is created, or not.  Thus 'pid' is overridden to
        make methods like 'kill' and 'stack_trace' use the correct process id.
        """
        return self._pid

    def wait_until_started(self):
        """
        Wait until the broker has started.
        """
        with internal_use(self):
            if not self.outputs_substr(
                "BMQbrkr started successfully", timeout=START_TIMEOUT
            ):
                raise RuntimeError(f"Failed to start broker on {self.name}: timeout")

        with (self._cwd / "bmqbrkr.pid").open("r") as file:
            self._pid = int(file.read())

    def create_client(self, prefix=None, dump_messages=True, **kwargs):
        return self.cluster.create_client(
            prefix or f"CLI{next(self._auto_id)}",
            self,
            dump_messages=dump_messages,
            **kwargs,
        )

    def send(self, string):
        """
        Send the specified 'string' to the broker.
        """
        self._logger.info(f"send {string}")
        with open(str(self._cwd / "bmqbrkr.ctl"), "w", opener=open_non_blocking) as fhw:
            fhw.write(string + "\n")

    def command(self, command, succeed=None, timeout=BLOCK_TIMEOUT):
        """
        Send the specified 'cmd' command to the broker, prefixing it with 'CMD '.
        """

        cmd = "CMD " + command
        self.send(cmd)

        return (
            None
            if succeed is None
            else self.capture(f"'{cmd}' processed successfully", timeout=timeout)
        )

    def get_active_node(self):
        """
        Wait until an active node becomes available, and return its host
        name.  Should only be called on proxies.
        """
        if self.cluster.is_single_node:
            return self.last_known_active

        self._logger.info("Waiting for active node...")

        with internal_use(self):
            m = self.capture(
                r"new leader\: \[(\w+), \d+\], leader status: ACTIVE", timeout=3
            )
            if m:
                self.last_known_active = m[1]
            if self.last_known_active is None:
                self._error("Could not determine active node")
            self._logger.info(f"Active node is {self.last_known_active}")
            return self.last_known_active

    def wait_status(
        self, wait_leader: bool, wait_ready: bool, cluster: str = "itCluster"
    ):
        """
        Wait until this node has an active leader if 'wait_leader' is True, and
        that the cluster is available if 'wait_ready' is True.  Additionally,
        if 'wait_leader' is True, store the Broker object corresponding to the
        leader in the 'last_known_leader' attribute.
        """

        if cluster is None:
            cluster = self.cluster_name

        if wait_leader:
            if self.cluster.is_single_node:
                self.last_known_leader = self
                wait_leader = None
            else:
                self.last_known_leader = None

        regexes = []

        if wait_leader:
            self._logger.info("Waiting for active leader...")
            regexes.append(r"new leader\: \[([^,]+), \d+\], leader status: ACTIVE")

        if wait_ready:
            self._logger.log(self._log_level, "Waiting for cluster to be ready...")
            regexes.append(f"Cluster \\({cluster}\\) is available")

        if len(regexes) == 0:
            return

        with internal_use(self):
            matches = self.capture_n(regexes)

        if wait_leader:
            leader_name = matches.pop(0)
            if leader_name is None:
                error = f"[broker {self.name}]: no active leader"
                self._logger.error(error)
                raise RuntimeError(error)
            self.last_known_leader = self.cluster.process(leader_name[1])
            self._logger.log(self._log_level, "leader is %s", self.last_known_leader)

        if wait_ready and matches.pop(0) is None:
            error = f"[broker {self.name}]: cluster not ready"
            self._logger.error(error)
            raise RuntimeError(error)

    def dump_queue_internals(self, domain, queue):
        """
        Dump state of the specified 'queue' in the specified 'domain'.
        """
        self.command(f"DOMAINS DOMAIN {domain} QUEUE {queue} INTERNALS")

    def queue_helper(self, cluster=None):
        """
        Dump state of the specified 'cluster'.
        """

        if cluster is None:
            cluster = self.cluster_name

        self.command(f"CLUSTERS CLUSTER {cluster} QUEUEHELPER")

    def set_quorum(self, quorum, cluster=None, succeed=None):
        """
        Set the quorum of the specified 'cluster' to the specified 'quorum'.
        """

        if cluster is None:
            cluster = self.cluster_name

        return self.command(
            f"CLUSTERS CLUSTER {cluster} STATE ELECTOR SET quorum {quorum}",
            succeed=succeed,
        )

    def set_replication_factor(self, quorum, cluster=None, succeed=None):
        """
        Set the replication factor of the specified 'cluster' to the specified
        'quorum'.
        """

        if cluster is None:
            cluster = self.cluster_name

        return self.command(
            f"CLUSTERS CLUSTER {cluster} STORAGE REPLICATION SET quorum {quorum}",
            succeed,
        )

    def force_gc_queues(self, block=None, succeed=None):
        """
        Force garbage collection for the specified 'cluster'.  If 'block' is
        specified and set to 'True', wait until the command completes.  If
        'succeed' is specified and set to 'True', raise an exception if the
        command does not succeed.
        """

        return self._command_helper(
            f"CMD CLUSTERS CLUSTER {self.cluster_name} FORCE_GC_QUEUES", block, succeed
        )

    def reconfigure_domain(self, domain: str, succeed: bool = False):
        """
        Issue a 'DOMAINS RECONFIGURE'
        command for this broker to reload the configuration from disk. Returns
        'True' if the command was successfully issued. If 'succeed' is 'True',
        then the function will block until process output confirms that the
        reconfigure operation completed successfully.

        Keys in 'kvs' should be rendered as '.'-separated paths (e.g.,
        "storage.domain.messages.quota").
        """

        return self.command(
            f"DOMAINS RECONFIGURE {domain}", succeed=succeed, timeout=BLOCK_TIMEOUT
        )

    def list_messages(self, qdomain, queue, offset, count, appid=None):
        """
        List the specified 'count' of messages for the specified 'queue',
        starting at the specified 'offset', restricted to the 'appid'
        substream, if specified.  If 'offset' is negative, it is relative to
        the last message.  If 'count' is zero, list all the messages starting
        at 'offset'.  If 'count' is strictly negative, list '-count' messages
        preceding 'offset'.
        """
        ss = "" if appid is None else f" {appid}"
        self.command(
            f"DOMAINS DOMAIN {qdomain} QUEUE {queue} LIST{ss} {offset} {count}"
        )

    def alarms(self, pattern=None, timeout=5):
        with internal_use(self):
            m = self.capture(f"ALARM.*{pattern or ''}", timeout)
            self._logger.info(f"alarms?: {m}")
            return m

    def erases_messages(self, queue, msgs=None, timeout=5):
        with internal_use(self):
            msgs = msgs or ".*"
            m = self.capture(
                f"queue \\[{queue}\\].*garbage-collected \\[{msgs}\\] messages", timeout
            )
            self._logger.info(f"gcs?: {m}")
            return m

    def panics(self, pattern=None, timeout=5):
        with internal_use(self):
            m = self.capture(f"PANIC.*{pattern or ''}", timeout)
            self._logger.info(f"panics?: {m}")
            return m

    def open_priority_queues(self, count, start=0, uri_priority=tc.URI_PRIORITY, **kw):
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
                Queue(self.create_client(), f"{uri_priority}{i}", **kw)
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
                Queue(self.create_client(), f"{tc.URI_BROADCAST}{i}", **kw)
                for i in range(start, start + count)
            ]
        )

    def open_fanout_queues(
        self, count, start=0, appids=None, uri_fanout=tc.URI_FANOUT, **kw
    ):
        """
        Open *distinct* fanout queues with the options specified in 'kw', each
        in a distinct, newly created client.  While each queue uses a different
        URI, calling this method multiple times, on any client, proxy or
        cluster, with the same 'start' value will result in the same URIs to be
        used.

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
            [
                Queue(self.create_client(), f"{uri_fanout}{i}", **kw)
                for i in range(start, start + count)
            ]
            if appids is None
            else [
                ListContextManager(
                    [
                        Queue(self.create_client(), f"{uri_fanout}{i}?id={id}", **kw)
                        for id in appids
                    ]
                )
                for i in range(start, start + count)
            ]
        )

    def exit_gracefully(self):
        self._logger.info(f"sending SIGINT to {self.pid}")
        self._process.send_signal(signal.SIGINT)

    def raise_if_exited_in_error(self):
        if not self.check_exit_code:
            # we don't care
            return

        if rc := self._process.poll():
            # either rc is None, in that case process is still running, either
            # it's zero, then it's OK
            if -rc != signal.SIGTERM:
                raise proc.ProcessExitError(self.name, self.returncode)

    ###########################################################################
    # Internals

    def _command_helper(self, command, block, succeed):
        """
        Execute the specified 'command'.  If 'block' is true, wait until the
        command completes and return the status.  If 'succeed' is true, wait
        until the command completes and raise an exception if if fails.  If
        either 'block' or 'succeed' are specified, drain the log before
        executing the command.
        """
        block = block or succeed

        if block:
            self.drain()

        self.send(command)

        if block:
            with internal_use(self):
                m = self.capture(
                    f"(?:Command '{command}' processed successfully)|(?:Error processing command.*rc: (?P<rc>-?\\d+))",
                    timeout=BLOCK_TIMEOUT,
                )
                if not m:
                    self._error(f"{command} did not complete within {BLOCK_TIMEOUT}s")
                rc = int(m.group("rc")) if m.group("rc") else 0
                if not (succeed is None):
                    if succeed:
                        if rc != 0:
                            self._error(f"{command} did not succeed")
                    else:
                        if rc == 0:
                            self._error(f"{command} succeed but should have failed")
                self._logger.info(f"{command} -> {rc}")
                return rc
