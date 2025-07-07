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

"""Run a cluster."""

import collections
import contextlib
import inspect
import itertools
import logging
import shutil
import signal
from typing import Dict, List, Optional, Union, Tuple
from pathlib import Path

from blazingmq.dev.configurator.localsite import LocalSite
import blazingmq.dev.it.process.proc
import blazingmq.dev.it.testconstants as tc
import blazingmq.dev.configurator.configurator as cfg
from blazingmq.dev.it.process.broker import Broker
from blazingmq.dev.it.process.client import Client
from blazingmq.dev.it.process.proc import Process
from blazingmq.dev.it.util import ListContextManager, Queue, internal_use

logger = logging.getLogger(__name__)

CORE_PATTERN_PATH = "/proc/sys/kernel/core_pattern"

QUORUM_DEFAULT = 0
QUORUM_TO_ENSURE_LEADER = 1
QUORUM_TO_ENSURE_NOT_LEADER = 100


def _match_broker(
    broker,
    *,
    datacenter: str = None,
    near: Broker = None,
    exclude: Union[Broker, List[Broker]] = None,
    alive=False,
    invert=False,
):
    if datacenter is not None and near is not None:
        raise RuntimeError("`near` and `datacenter` options cannot be both specified")

    result = True

    if exclude is not None and (broker is exclude or broker in exclude):
        result = False
    elif datacenter is not None:
        result = result and broker.datacenter == datacenter
    elif near is not None:
        result = result and broker.datacenter == near.datacenter

    if alive:
        result = result and broker.is_alive()

    if invert:
        result = not result

    return result


def _format_rc(rc):
    return f"{rc} ({signal.strsignal(-rc)})" if rc < 0 else str(rc)


class Cluster(contextlib.AbstractContextManager):
    """
    Run a set of brokers.
    """

    def __init__(
        self,
        config: cfg.Cluster,
        configurator: cfg.Configurator,
        work_dir: Path,
        tool_extra_args=None,
        copy_cores: Optional[Path] = None,
    ):
        """
        Initialize a Cluster object with the specified 'config'.
        """

        self.config = config
        self.configurator = configurator
        self.work_dir = work_dir
        self.copy_cores = copy_cores

        log_extra = {
            "bmqprocess": "pytest",
            "ball_overrides": {},
            "blp_log_from": inspect.getfile(type(self)),
        }
        self._internal_logger = logging.LoggerAdapter(
            logging.getLogger(__name__), extra=log_extra
        )
        self._logger = blazingmq.dev.it.process.proc.BallLoggerAdapter(
            logging.getLogger("blazingmq.test"), extra=log_extra
        )

        self.last_known_leader: Optional[Broker] = None
        self._processes: Dict[str, Union[Broker, Client]] = {}
        self._nodes: List[Broker] = []
        self._virtual_nodes: List[Broker] = []
        self._proxies: List[Broker] = []
        self._clients: List[Client] = []
        self._other_processes: List[Process] = []

        # out soon
        self._tool_extra_args = tool_extra_args

    def __exit__(self, *_):
        # self._esx.close()
        pass

    @property
    def name(self) -> str:
        """
        Return the name of the cluster.
        """

        return self.config.name

    @property
    def admin_endpoint(self) -> Union[Tuple[str, int], Tuple[None, None]]:
        """
        Return a tuple containing (host, port) of an admin endpoint of this cluster, if the
        admin endpoint is not decided, return (None, None) tuple
        """
        if not self.last_known_leader:
            return None, None

        return self.last_known_leader.config.host, int(
            self.last_known_leader.config.port
        )

    def start(
        self, wait_leader=True, wait_ready=False, leader_name: Union[str, None] = None
    ):
        """
        Start all the nodes and proxies in the cluster.

        If 'wait_leader' is 'True', wait for a leader to be elected.  If
        'wait_ready' is True, wait until the cluster is available in all nodes.
        Return a tuple consisting of an array of nodes and an array of proxies.
        See also: 'wait_status' for more information on the 'ready' flags.
        """

        need_preset_leader = leader_name is not None

        with internal_use(self):
            for broker in self.config.configurator.brokers.values():
                if len(broker.clusters.my_virtual_clusters) > 0:
                    self.start_virtual_node(broker)
                elif len(broker.clusters.my_clusters) == 0:
                    self.start_proxy(broker)
                else:
                    brkrproc = self.start_node(broker)

                    # Select leader based on the given leader_name
                    if need_preset_leader:
                        if broker.name == leader_name:
                            brkrproc.set_quorum(QUORUM_TO_ENSURE_LEADER)
                        else:
                            brkrproc.set_quorum(QUORUM_TO_ENSURE_NOT_LEADER)

            if self.is_single_node:
                self._proxies = self._nodes
                self.last_known_leader = self.nodes()[0]
            else:
                self.wait_status(wait_leader, wait_ready)
                self.drain()

            # Reset quorum to default
            if need_preset_leader:
                for brkrproc in self._nodes:
                    brkrproc.set_quorum(QUORUM_DEFAULT)

        return (self._nodes, self._proxies)

    @property
    def is_single_node(self) -> bool:
        """
        Return True if this is a single node cluster.
        """

        return len(self.config.nodes) == 1

    @property
    def all_processes(self):
        """
        Return all processes related to this cluster.

        This function returns *all* the processes, i.e. the nodes, the proxies
        and the clients.
        """

        return list(self._processes.values()) + self._clients

    def check_processes(self):
        """Check that all processes (nodes, proxies and clients) involved in this
        cluster are either running or have exited with a zero return code.
        """

        for process in self.all_processes:
            process.raise_if_exited_in_error()

    def stop(self):
        """Terminates all the nodes, proxies and clients."""

        processes = list(reversed(self.all_processes))

        for process in processes:
            with internal_use(process):
                try:
                    process.stop()
                except Exception as error:
                    self._logger.error(
                        "Exception raised while stopping process: %s", error
                    )
                if process.name in self._processes:
                    del self._processes[process.name]
                else:
                    self._clients.remove(process)

        self.last_known_leader = None
        bad_exit = False
        cores_dir = None if self.copy_cores is None else self._find_cores_dir()

        for process in processes:
            process.wait()
            if process.is_alive():
                self._logger.error("process %s refuses to exit", process.name)
            elif process.check_exit_code and process.returncode != 0:
                if cores_dir is not None:
                    core_found = False
                    for core in cores_dir.iterdir():
                        if core.is_file and str(process.pid) in str(core):
                            core_found = True
                            logger.info("copying core %s to %s", core, self.copy_cores)
                            shutil.copy(core, self.copy_cores)
                            break
                    if not core_found:
                        self._logger.warning("could not find core for %s", process.name)
                bad_exit = True
                self._logger.error(
                    "%s [%d] exited with rc = %s",
                    process.name,
                    process.pid,
                    _format_rc(process.returncode),
                )

        for process in processes:
            if process.is_alive():
                self._logger.error("killing recalcitrant process %s", process.name)
                process.force_stop()
                bad_exit = True

        for process in self._other_processes:
            self._logger.info("killing subordinate process %s", process.name)
            process.force_stop()

        if bad_exit:
            raise RuntimeError("cluster did not shut down cleanly")

    def start_nodes(self, wait_leader=True, wait_ready=False):
        """Start the nodes in the cluster.

        If 'wait_leader' is 'True', wait for a leader to be elected.  If
        'wait_ready' is True, wait until the cluster is available in all nodes.
        NOTE: this method does *not* start the proxies.  It is intended to be
        used to start a cluster that has been stopped via 'stop_nodes'.
        'start' is the preferred method to start the cluster as a whole.
        """

        self._logger.info("starting all nodes")

        for node in self.nodes():
            with internal_use(node):
                node.start()
                node.wait_until_started()

        self.wait_status(wait_leader, wait_ready)

    def stop_nodes(self, prevent_leader_bounce=False):
        """Stop the nodes in the cluster.

        If 'prevent_leader_bounce' is 'True', prevent leader bounce during
        shutdown by setting quorum of all non-leader nodes to 100.

        NOTE: this method does *not* stop the proxies.
        """

        self._logger.info("stopping all nodes")
        if prevent_leader_bounce:
            for node in self.nodes():
                if node is not self.last_known_leader:
                    node.set_quorum(100)

        self.last_known_leader = None
        for node in self.nodes():
            with internal_use(node):
                node.stop()

        for node in self.nodes():
            if node.is_alive():
                error = f"node {node.name} refused to stop"
                self._error(error)

    def restart_nodes(self, wait_leader=True, wait_ready=False):
        """Restart all the nodes.

        If 'wait_leader' is 'True', wait for a leader to be elected.  If
        'wait_ready' is True, wait until the cluster is available in all nodes.
        Return a tuple consisting of an array of nodes and an array of proxies.
        See also: 'wait_status' for more information on the 'ready' flags.
        """

        self._logger.info("restarting all nodes")
        for node in self.nodes():
            if node is not self.last_known_leader:
                node.set_quorum(QUORUM_TO_ENSURE_NOT_LEADER)

        self.last_known_leader = None
        with internal_use(self):
            self.stop_nodes()
            self.start_nodes(wait_leader, wait_ready)

    def drain(self):
        """Drain the logs of all the nodes."""

        for node in self.nodes():
            with internal_use(node):
                node.drain()

    def destroy(self):
        """Free the resources owned by this cluster."""
        # self._esx.close()

    # TODO: fold following three in start_broker
    def start_node(self, broker: Union[cfg.Broker, str]):
        """
        Start a process for 'broker'.
        """

        if isinstance(broker, str):
            broker = self.config.nodes[broker]

        self._logger.info("starting broker %s", broker)

        if len(broker.clusters.my_clusters) != 1:
            raise RuntimeError(f"Cannot use start_node to start {broker.name}")

        return self._start_broker(broker, self._nodes, "itCluster")

    def start_virtual_node(self, broker: Union[cfg.Broker, str]):
        """Start the node specified by 'name'."""

        if isinstance(broker, str):
            broker = self.config.nodes[broker]

        if len(broker.clusters.my_virtual_clusters) != 1:
            raise RuntimeError(f"Cannot use start_virtual_node to start {broker.name}")

        return self._start_broker(broker, self._virtual_nodes, "itVirtualCluster")

    def start_proxy(self, broker: Union[cfg.Broker, str]):
        """Start the proxy specified by 'name'."""

        if isinstance(broker, str):
            broker = self.config.configurator.brokers[broker]

        self._logger.info("starting proxy [%s]", broker.name)

        if (
            len(broker.clusters.my_clusters) != 0
            or len(broker.clusters.my_virtual_clusters) != 0
        ):
            raise RuntimeError(f"Cannot use start_proxy to start node {broker.name}")

        return self._start_broker(broker, self._proxies, None)

    def start_tproxy(
        self, broker: Union[cfg.Broker, str], port: str = None
    ) -> Tuple[str, Process]:
        """Start the tproxy for the specified 'broker'.

        If the 'port' is specified tproxy will try to start listening it.
        Otherwise, tproxy will allocate a port itself.
        """

        if isinstance(broker, str):
            broker = self.config.configurator.brokers[broker]

        self._logger.info("starting tproxy for [%s]", broker.name)

        command = [
            "tproxy",
            "-r",
            f"{broker.host}:{broker.port}",
        ]
        if port is not None:
            command.extend(["-p", port])

        tproxy = Process(
            f"tproxy_{broker.name}",
            command,
        )
        tproxy.start()
        tproxy_port = tproxy.capture(r"Listening on (.+):(\d+)", timeout=2).group(2)

        self._other_processes.append(tproxy)

        return tproxy_port, tproxy

    def process(self, name):
        """Return the process (node or proxy) specified by 'name'."""

        return self._processes[name]

    def nodes(
        self,
        *,
        datacenter: str = None,
        near: Broker = None,
        exclude: Union[Broker, List[Broker]] = None,
        alive=False,
        invert=False,
    ) -> List[Broker]:
        """Return the nodes matching the conditions specified by 'kw'.

        Conditions
        can be any combination of:
        - datacenter:<name> return the nodes in the specified data center
        - near:<node> return the nodes in the same data center as the specified
          node
        - alive:<True>: return the nodes that are alive
        - exclude: return nodes not exclude or not in exclude

        If several conditions are specified, they must all be satisfied.
        If keyword argument 'invert' is specified and its value is true, return
        the nodes that do *not* match the condition(s).
        """

        return [
            broker
            for broker in self._nodes
            if _match_broker(
                broker,
                datacenter=datacenter,
                near=near,
                exclude=exclude,
                alive=alive,
                invert=invert,
            )
        ]

    def virtual_nodes(
        self,
        *,
        datacenter: str = None,
        near: Broker = None,
        exclude: Union[Broker, List[Broker]] = None,
        alive=False,
        invert=False,
    ):
        """Return the virtual nodes matching the conditions specified by 'kw'.

        Conditions can be any combination of: - datacenter:<name> return the
        nodes in the specified data center - near:<node> return the nodes in
        the same data center as the specified node - alive:<True>: return the
        nodes that are alive If several conditions are specified, they must all
        be satisfied.  If keyword argument 'invert' is specified and its value
        is true, return the nodes that do *not* match the condition(s).
        """

        return [
            broker
            for broker in self._virtual_nodes
            if _match_broker(
                broker,
                datacenter=datacenter,
                near=near,
                exclude=exclude,
                alive=alive,
                invert=invert,
            )
        ]

    def proxies(
        self,
        *,
        datacenter: str = None,
        near: Broker = None,
        exclude: Union[Broker, List[Broker]] = None,
        alive=False,
        invert=False,
    ) -> List[Broker]:
        """Return the proxies matching the conditions specified by 'kw'.

        Conditions can be any combination of: - datacenter:<name> return the
        nodes in the specified data center - near:<node> return the nodes in
        the same data center as the specified node - alive:<True>: return the
        nodes that are alive If several conditions are specified, they must all
        be satisfied.  If keyword argument 'invert' is specified and its value
        is true, return the nodes that do *not* match the condition(s).
        """

        return [
            broker
            for broker in self._proxies
            if _match_broker(
                broker,
                datacenter=datacenter,
                near=near,
                exclude=exclude,
                alive=alive,
                invert=invert,
            )
        ]

    def proxy_cycle(self):
        """
        Return an iterator over a cyclic sequence of proxies.

        The sequence is ordered according to the following rules:
          o If the current leader is known, the first proxy in the list
            connects to the same data center as the leader.
          o Between two proxies connecting to the same data center, there is a
            proxy connecting to each of the other data centers.
          o Between two occurrences of the same proxy, all the other proxies
            connecting to the same data center occur.
        There are no other guarantees on the order of the sequence.

        Multiple calls to this method return independent iterators, but they
        return the proxies in the same order.  For example, if ORANGE has two
        proxies (o1 and o2) and RIDGE has three (r1, r2, r3), and the leader is
        in ORANGE, the sequence may consist of the endless repetition of the
        following sub-sequence: o1, r1, o2, r2, o1, r3, o2, r1, o1, r2, o2, r3
        """

        proxy_map: collections.defaultdict[str, List[Broker]] = collections.defaultdict(
            list
        )

        for proxy in self._proxies:
            proxy_map[proxy.datacenter].append(proxy)

        leader_proxies = (
            []
            if self.last_known_leader is None
            else [proxy_map.pop(self.last_known_leader.datacenter)]
        )

        cycles = itertools.cycle(
            [
                itertools.cycle(dcpx)
                for dcpx in leader_proxies + list(proxy_map.values())
            ]
        )

        while True:
            yield next(next(cycles))

    def create_client(
        self,
        prefix,
        broker: Broker,
        start=True,
        dump_messages=True,
        options=None,
        port=None,
    ) -> Client:
        """
        Create a client with the specified name.

        Either 'proxyhostname' or 'proxy' must be specified; the client
        connects to the specified proxy.  If 'options' is specified, its string
        value is tacked at the end of the 'bmqtool.tsk' argument list. If 'port' is not
        specified, use either the first listener if it exists or 'broker.port'.
        """

        if isinstance(options, str):
            options = options.split()

        name = f"{prefix}@{broker.name}"

        if port is None:
            if broker.config.listeners:
                port = broker.config.listeners[0]
            else:
                port = broker.config.port

        client = Client(
            name,
            ("localhost", port),
            tool_path="bin/bmqtool.tsk",
            cwd=(self.work_dir / broker.name),
            dump_messages=dump_messages,
            options=(self._tool_extra_args or []) + (options or []),
        )
        client.add_sync_log_hook(lambda _: self.check_processes())
        client.start()
        self._clients.append(client)
        self._logger.debug("%s pid = %s", client._process.pid)

        with internal_use(client):
            if start:
                client.start_session(succeed=True)

        return client

    def wait_status(self, wait_leader, wait_ready):
        """
        Wait until the cluster achieves the specified statuses.

        Wait until each node has an active leader if 'wait_leader' is True, and
        that this cluster is available in each node if 'wait_ready' is True.
        Additionally, if 'wait_leader' is True, store the Broker object
        corresponding to the leader in the 'last_known_leader' attribute.
        """

        self._logger.debug(
            "wait_status(wait_leader=%s, wait_ready=%s)", wait_leader, wait_ready
        )

        if not wait_leader and not wait_ready:
            return

        if wait_leader:
            self.last_known_leader = None

        for node in self.nodes():
            with internal_use(node):
                if node.is_alive():
                    node.wait_status(wait_leader, wait_ready, self.name)
                    if wait_leader:
                        self.last_known_leader = node.last_known_leader

    def wait_leader(self):
        """
        Wait until all the nodes have an active leader.

        Return a Broker object corresponding to it.
        """

        self.wait_status(wait_leader=True, wait_ready=False)

        return self.last_known_leader

    def open_priority_queues(
        self, count, start=0, port=None, uri_priority=tc.URI_PRIORITY, **kw
    ) -> ListContextManager[Queue]:
        """Open *distinct* priority queues with the options specified in 'kw'.

        While each queue uses a different URI, calling this method multiple
        times, on any client, proxy or cluster, with the same 'start' value
        will result in the same URIs to be used.

        Return a list of 'count' Queue objects.  The list is wrapped by a class
        that implement the ContextManager.  Using the value returned by this
        method in a 'with' statement will ensure that the queues are closed at
        the end of the block.
        """

        proxies = self.proxy_cycle()
        return ListContextManager(
            [
                Queue(
                    next(proxies).create_client(port=port),
                    f"{uri_priority}{i}",
                    **kw,
                )
                for i in range(start, start + count)
            ]
        )

    def open_broadcast_queues(self, count, start=0, **kw):
        """Open *distinct* broadcast queues with the options specified in 'kw'.

        While each queue uses a different URI, calling this method multiple
        times, on any client, proxy or cluster, with the same 'start' value
        will result in the same URIs to be used.

        Return a list of 'count' Queue objects.  The list is wrapped by a class
        that implement the ContextManager.  Using the value returned by this
        method in a 'with' statement will ensure that the queues are closed at
        the end of the block.
        """

        proxies = self.proxy_cycle()
        return ListContextManager(
            [
                Queue(next(proxies).create_client(), f"{tc.URI_BROADCAST}{i}", **kw)
                for i in range(start, start + count)
            ]
        )

    def open_fanout_queues(
        self, count, start=0, appids=None, uri_fanout=tc.URI_FANOUT, **kw
    ):
        """Open *distinct* fanout queues with the options specified in 'kw'.

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

        proxies = self.proxy_cycle()
        return ListContextManager(
            [
                Queue(next(proxies).create_client(), f"{uri_fanout}{i}", **kw)
                for i in range(start, start + count)
            ]
            if appids is None
            else [
                ListContextManager(
                    [
                        Queue(
                            next(proxies).create_client(),
                            f"{uri_fanout}{i}?id={id}",
                            **kw,
                        )
                        for id in appids
                    ]
                )
                for i in range(start, start + count)
            ]
        )

    def deploy_domains(self):
        for broker in self.configurator.brokers.values():
            self.configurator.deploy_domains(
                broker, LocalSite(self.work_dir / broker.name)
            )

    def reconfigure_domain(
        self,
        domain_name: str,
        leader_only: bool = False,
        write_only: bool = False,
        succeed: bool = False,
    ):
        """
        Overwrites the domains config files to use the key-value pairs in
        'kvs' for 'qualified_domain', and thereafter issues a 'DOMAINS
        RECONFIGURE' command, first to the leader, then to the other nodes,
        unless 'leader_only' is True. If 'succeed' is 'True', then the
        function will block until process output confirms that the reconfigure
        operation completed successfully. Returns 'True' if the command was
        successfully issued to all nodes. Keys in 'kvs' should be rendered as
        '.'-separated paths (e.g., "storage.domain.messages.quota").
        """

        assert self.last_known_leader is not None
        assert self.config is not None

        self.deploy_domains()

        if write_only:
            return True

        with internal_use(self):
            if not self.last_known_leader.reconfigure_domain(domain_name, succeed):
                return False

            if not leader_only:
                for node in self.nodes():
                    if node is not self.last_known_leader:
                        if not node.reconfigure_domain(domain_name, succeed):
                            return False

        return True

    ###########################################################################
    # Internals

    def _find_cores_dir(self) -> Optional[Path]:
        try:
            with open(CORE_PATTERN_PATH) as core_pattern_file:
                pattern = core_pattern_file.readline().strip()
                if "%p" in pattern:
                    cores_dir = Path(pattern).parent
                    if cores_dir.is_absolute():
                        return cores_dir

                    self._logger.warning(
                        "core pattern '%s' is not an absolute path, cores will not be saved",
                        pattern,
                    )
                else:
                    self._logger.warning(
                        "core pattern '%s' does not contain process id, cores will not be saved",
                        pattern,
                    )
        except FileNotFoundError:
            self._logger.warning(
                "%s does not exist, cores will not be saved", CORE_PATTERN_PATH
            )

        return None

    def _start_broker(
        self, broker: cfg.Broker, brokers: List[Broker], cluster_name: Union[str, None]
    ):
        if broker.name in self._processes:
            raise RuntimeError(
                f'node "{broker.name}" is already running in cluster "{cluster_name}"'
            )

        process = Broker(
            broker,
            cluster=self,
            cwd=self.work_dir / broker.name,
        )

        process.add_sync_log_hook(lambda _: self.check_processes())

        process.start()

        self._processes[broker.name] = process
        brokers.append(process)

        process.wait_until_started()

        return process

    def _error(self, error: str):
        self._logger.error(error)
        raise RuntimeError(error)
