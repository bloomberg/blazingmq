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
Support for creating a "configurator", i.e. a collection of directories and
scripts for running a cluster.
"""

# mypy: disable-error-code="union-attr"
# pylint: disable=missing-function-docstring, missing-class-docstring, consider-using-f-string
# pyright: reportOptionalMemberAccess=false


import abc
import contextlib
import copy
import functools
import itertools
import logging
import subprocess
import threading
from dataclasses import dataclass, field
from decimal import Decimal
from pathlib import Path
from shutil import rmtree
from typing import IO, Dict, Iterable, Iterator, List, Optional, Set, Tuple, Union

from termcolor import colored
from xsdata.formats.dataclass.context import XmlContext
from xsdata.formats.dataclass.serializers import JsonSerializer
from xsdata.formats.dataclass.serializers.config import SerializerConfig

from blazingmq.dev.paths import required_paths as paths
from blazingmq.schemas import mqbcfg, mqbconf

__all__ = ["Configurator"]

COLORS = {
    "green": 32,
    "yellow": 33,
    "magenta": 35,
    "cyan": 36,
    "blue": 34,
    "light_green": 92,
    "light_yellow": 93,
    "light_blue": 94,
    "light_magenta": 95,
    "light_cyan": 96,
}


logger = logging.getLogger(__name__)
broker_logger = logger.getChild("broker")


RUN_SCRIPT = """#! /usr/bin/env bash
host_dir=$(realpath $(dirname $0))
ws_dir=$(dirname $host_dir)
cd $host_dir
if [ -e bmqbrkr.pid ] && kill -s 0 $(cat bmqbrkr.pid); then
    echo "bmqbrkr is already running"
    exit 0
fi
rm -f bmqbrkr.ctl
mkdir -p storage/archive
{cmd} $host_dir/bin/bmqbrkr.tsk $host_dir/etc
"""

TOOL_SCRIPT = """#! /usr/bin/env bash
cd $(dirname $0)
{cmd} bin/bmqtool.tsk -b tcp://localhost:{BMQ_PORT} "$@"
"""


class ConfiguratorError(RuntimeError):
    pass


@dataclass
class Domain:
    cluster: "AbstractCluster"
    definition: mqbconf.DomainDefinition

    @property
    def name(self) -> str:
        return self.definition.parameters.name  # type: ignore


@dataclass(frozen=True)
class Broker:
    configurator: "Configurator"
    id: int
    config: mqbcfg.Configuration
    clusters: mqbcfg.ClustersDefinition = field(
        default_factory=lambda: mqbcfg.ClustersDefinition([], [], [], [], [])
    )
    domains: Dict[str, "Domain"] = field(default_factory=dict)
    _proxy_clusters: Set[str] = field(default_factory=set)

    def __str__(self) -> str:
        return self.name

    def __repr__(self) -> str:
        return f"Broker({self.name})"

    def __hash__(self) -> int:
        return self.name.__hash__()

    @property
    def name(self) -> str:
        return self.config.app_config.host_name  # type: ignore

    @property
    def data_center(self) -> str:
        return self.config.app_config.host_data_center  # type: ignore

    @property
    def instance(self) -> str:
        return self.config.app_config.broker_instance_name  # type: ignore

    @property
    def host(self) -> str:
        return self.config.app_config.network_interfaces.tcp_interface.name  # type: ignore

    @property
    def port(self) -> str:
        return self.config.app_config.network_interfaces.tcp_interface.port  # type: ignore

    @property
    def config_dir(self) -> Path:
        return Path(self.name) / "etc"

    def _add_proxy_definition(self, cluster: "AbstractCluster"):
        self._proxy_clusters.add(cluster.name)
        self.clusters.proxy_clusters.append(
            mqbcfg.ClusterProxyDefinition(
                name=cluster.name,
                nodes=[copy.deepcopy(node) for node in cluster.definition.nodes],
                queue_operations=copy.deepcopy(
                    self.configurator.proto.cluster.queue_operations
                ),
                cluster_monitor_config=copy.deepcopy(
                    self.configurator.proto.cluster.cluster_monitor_config
                ),
                message_throttle_config=copy.deepcopy(
                    self.configurator.proto.cluster.message_throttle_config
                ),
            )
        )

    def proxy(self, target: Union[Domain, "AbstractCluster"], reverse: bool = False):
        domains = [target] if isinstance(target, Domain) else target.domains.values()
        for domain in domains:
            cluster = domain.cluster
            domain_definition = copy.deepcopy(domain.definition)
            domain_definition.location = cluster.name
            self.domains[domain.name] = Domain(cluster, domain_definition)

            if cluster.name not in self._proxy_clusters:
                self._add_proxy_definition(cluster)

            if not reverse:
                continue

            if cluster.name in self.clusters.my_reverse_clusters:
                continue

            self.clusters.my_reverse_clusters.append(cluster.name)
            reverse_cluster = self.configurator.clusters[cluster.name]

            for node in reverse_cluster.nodes.values():
                if reverse_cluster.name not in node._proxy_clusters:
                    node._add_proxy_definition(reverse_cluster)

                reversed_cluster_connections_found = False
                if node.clusters.reversed_cluster_connections is None:
                    node.clusters.reversed_cluster_connections = []
                else:
                    for (
                        reversed_cluster_connections
                    ) in node.clusters.reversed_cluster_connections:
                        if reversed_cluster_connections.name == reverse_cluster.name:
                            reversed_cluster_connections_found = True
                            break

                if not reversed_cluster_connections_found:
                    reversed_cluster_connections = mqbcfg.ReversedClusterConnection(
                        reverse_cluster.name, []
                    )
                    node.clusters.reversed_cluster_connections.append(
                        reversed_cluster_connections
                    )

                reversed_cluster_connections.connections.append(
                    mqbcfg.ClusterNodeConnection(
                        mqbcfg.TcpClusterNodeConnection(
                            f"tcp://{self.host}:{self.port}"
                        )
                    )
                )

        return self

    def deploy(self, site: "Site") -> None:
        self.deploy_programs(site)
        self.deploy_broker_config(site)
        self.deploy_domains(site)

    def deploy_programs(self, site: "Site") -> None:
        site.install(paths.broker, "bin")
        site.install(paths.tool, "bin")
        site.install(paths.plugins, ".")

        for script, cmd in ("run", "exec"), ("debug", "gdb --args"):
            site.create_file(
                script,
                RUN_SCRIPT.format(cmd=cmd, host=self.name),
                mode=0o755,
            )

        for script, cmd in (
            ("run-client", "exec"),
            ("debug-client", "gdb ./bmqtool.tsk --args"),
        ):
            site.create_file(
                script,
                TOOL_SCRIPT.format(
                    cmd=cmd,
                    BMQ_PORT=self.port,
                ),
                mode=0o755,
            )

    def deploy_broker_config(self, site: "Site") -> None:
        site.create_json_file("etc/bmqbrkrcfg.json", self.config)

        log_dir = Path(self.config.task_config.log_controller.file_name).parent  # type: ignore
        if log_dir != Path():
            site.mkdir(log_dir)

        stats_dir = Path(self.config.app_config.stats.printer.file).parent  # type: ignore
        if stats_dir != Path():
            site.mkdir(stats_dir)

    def deploy_domains(self, site: "Site") -> None:
        site.create_json_file("etc/clusters.json", self.clusters)

        for cluster in self.clusters.my_clusters:
            for storage_dir in (
                Path(cluster.partition_config.location),  # type: ignore
                Path(cluster.partition_config.archive_location),  # type: ignore
            ):
                if storage_dir != Path():
                    site.mkdir(storage_dir)

        site.rmdir("etc/domains")

        for domain in self.domains.values():
            site.create_json_file(
                f"etc/domains/{domain.name}.json",
                mqbconf.DomainVariant(definition=domain.definition),
            )


@dataclass(frozen=True, repr=False)
class AbstractCluster:
    configurator: "Configurator"
    definition: Union[mqbcfg.ClusterDefinition, mqbcfg.VirtualClusterInformation]
    nodes: Dict[str, Broker]
    domains: Dict[str, "Domain"] = field(default_factory=dict)

    @property
    def name(self) -> str:
        assert self.definition.name is not None
        return self.definition.name

    def __str__(self) -> str:
        return f"{type(self).__name__}({self.name})"

    def __repr__(self) -> str:
        return str(self)

    def _add_domain(self, domain: "Domain") -> "Domain":
        if domain.name in self.domains:
            raise ConfiguratorError(
                f"domain '{domain.name}' already exists in {self.name}"
            )

        self.domains[domain.name] = domain

        for node in self.nodes.values():
            node.domains[domain.name] = domain

        return domain


class Cluster(AbstractCluster):
    def domain(self, parameters: mqbconf.Domain) -> "Domain":
        domain = mqbconf.DomainDefinition(self.name, parameters)

        return self._add_domain(Domain(self, domain))

    def broadcast_domain(self, name: str) -> "Domain":
        parameters = self.configurator.domain_definition()
        parameters.name = name
        parameters.mode = mqbconf.QueueMode(broadcast=mqbconf.QueueModeBroadcast())
        parameters.storage.config.in_memory = mqbconf.InMemoryStorage()
        parameters.storage.config.file_backed = None
        domain = mqbconf.DomainDefinition(self.name, parameters)

        return self._add_domain(Domain(self, domain))

    def fanout_domain(self, name: str, app_ids: List[str]) -> "Domain":
        parameters = self.configurator.domain_definition()
        parameters.name = name
        parameters.mode = mqbconf.QueueMode(fanout=mqbconf.QueueModeFanout([*app_ids]))
        domain = mqbconf.DomainDefinition(self.name, parameters)

        return self._add_domain(Domain(self, domain))

    def priority_domain(self, name: str) -> "Domain":
        parameters = self.configurator.domain_definition()
        parameters.name = name
        parameters.mode = mqbconf.QueueMode(priority=mqbconf.QueueModePriority())
        domain = mqbconf.DomainDefinition(self.name, parameters)

        return self._add_domain(Domain(self, domain))


class VirtualCluster(AbstractCluster):
    def proxy(self, target: Union[Domain, AbstractCluster]):
        domains = [target] if isinstance(target, Domain) else target.domains.values()
        for domain in domains:
            for broker in self.nodes.values():
                broker.proxy(domain)

            definition = copy.deepcopy(domain.definition)
            definition.location = self.name
            self.domains[domain.name] = Domain(self, definition)


class Site(abc.ABC):
    configurator: "Configurator"

    @abc.abstractmethod
    def __str__(self) -> str:
        ...

    @abc.abstractmethod
    def install(self, from_path: Union[str, Path], to_path: Union[str, Path]) -> None:
        ...

    @abc.abstractmethod
    def create_file(self, path: Union[str, Path], content: str, mode=None) -> None:
        ...

    @abc.abstractmethod
    def mkdir(self, path: Union[str, Path]) -> None:
        ...

    @abc.abstractmethod
    def rmdir(self, path: Union[str, Path]) -> None:
        ...

    @abc.abstractmethod
    def create_json_file(
        self,
        path: Union[str, Path],
        content,
    ) -> None:
        ...


def _cluster_definition_partial_prototype(partition_config: mqbcfg.PartitionConfig):
    return mqbcfg.ClusterDefinition(
        name="",  # overwritten
        nodes=[],
        partition_config=partition_config,
        master_assignment=mqbcfg.MasterAssignmentAlgorithm(
            mqbcfg.MasterAssignmentAlgorithm.E_LEADER_IS_MASTER_ALL
        ),
        elector=mqbcfg.ElectorConfig(
            initial_wait_timeout_ms=4000,
            max_random_wait_timeout_ms=3000,
            scouting_result_timeout_ms=4000,
            election_result_timeout_ms=3000,
            heartbeat_broadcast_period_ms=2000,
            heartbeat_check_period_ms=1000,
            heartbeat_miss_count=10,
            quorum=0,
            leader_sync_delay_ms=15000,
        ),
        queue_operations=mqbcfg.QueueOperationsConfig(
            open_timeout_ms=300000,
            configure_timeout_ms=300000,
            close_timeout_ms=300000,
            reopen_timeout_ms=43200000,
            reopen_retry_interval_ms=5000,
            reopen_max_attempts=10,
            assignment_timeout_ms=15000,
            keepalive_duration_ms=1800000,
            consumption_monitor_period_ms=30000,
            stop_timeout_ms=10000,
            shutdown_timeout_ms=20000,
            ack_window_size=500,
        ),
        cluster_attributes=mqbcfg.ClusterAttributes(
            is_cslmode_enabled=True, is_fsmworkflow=False
        ),
        cluster_monitor_config=mqbcfg.ClusterMonitorConfig(
            max_time_leader=15,
            max_time_master=30,
            max_time_node=30,
            max_time_failover=60,
            threshold_leader=8,
            threshold_master=15,
            threshold_node=15,
            threshold_failover=30,
        ),
        message_throttle_config=mqbcfg.MessageThrottleConfig(
            low_threshold=2,
            high_threshold=4,
            low_interval=1000,
            high_interval=3000,
        ),
    )


@dataclass(frozen=True)
class Proto:
    domain: mqbconf.Domain = field(
        default_factory=functools.partial(
            mqbconf.Domain,
            max_delivery_attempts=0,
            deduplication_time_ms=300000,
            consistency=mqbconf.Consistency(strong=mqbconf.QueueConsistencyStrong()),
            storage=mqbconf.StorageDefinition(
                config=mqbconf.Storage(file_backed=mqbconf.FileBackedStorage()),
                domain_limits=mqbconf.Limits(
                    bytes=2097152,
                    messages=2000,
                    bytes_watermark_ratio=Decimal("0.8"),
                    messages_watermark_ratio=Decimal("0.8"),
                ),
                queue_limits=mqbconf.Limits(
                    bytes=1048576,
                    messages=1000,
                    bytes_watermark_ratio=Decimal("0.8"),
                    messages_watermark_ratio=Decimal("0.8"),
                ),
            ),
            message_ttl=300,
            max_producers=0,
            max_consumers=0,
            max_queues=0,
            max_idle_time=0,
            mode=None,  # overwritten
        )
    )

    broker: mqbcfg.Configuration = field(
        default_factory=functools.partial(
            mqbcfg.Configuration,
            task_config=mqbcfg.TaskConfig(
                allocator_type=mqbcfg.AllocatorType.COUNTING,
                allocation_limit=34359738368,
                log_controller=mqbcfg.LogController(
                    file_name="logs/logs.%T.%p",
                    file_max_age_days=10,
                    rotation_bytes=268435456,
                    logfile_format="%d (%t) %s %F:%l %m\n\n",
                    console_format="%d (%t) %s %F:%l %m\n",
                    logging_verbosity="INFO",
                    console_severity_threshold="INFO",
                    categories=[
                        "BMQBRKR:INFO:green",
                        "BMQ*:INFO:green",
                        "MQB*:INFO:green",
                        "SIM*:INFO:gray",
                        "DMC*:INFO:yellow",
                        "BAEA.PERFORMANCEMONITOR:INFO:white",
                    ],
                    syslog=mqbcfg.SyslogConfig(
                        enabled=False,
                        app_name="BMQ",
                        log_format="%d (%t) %s %F:%l %m\n\n",
                        verbosity="",
                    ),
                ),
            ),
            app_config=mqbcfg.AppConfig(
                broker_instance_name="default",
                broker_version=999999,
                config_version=999999,
                host_name="",  # overwritten
                host_data_center="",  # overwritten
                is_running_on_dev=False,
                logs_observer_max_size=1000,
                dispatcher_config=mqbcfg.DispatcherConfig(
                    sessions=mqbcfg.DispatcherProcessorConfig(
                        num_processors=4,
                        processor_config=mqbcfg.DispatcherProcessorParameters(
                            queue_size=500000,
                            queue_size_low_watermark=100,
                            queue_size_high_watermark=200000,
                        ),
                    ),
                    queues=mqbcfg.DispatcherProcessorConfig(
                        num_processors=8,
                        processor_config=mqbcfg.DispatcherProcessorParameters(
                            queue_size=500000,
                            queue_size_low_watermark=100,
                            queue_size_high_watermark=200000,
                        ),
                    ),
                    clusters=mqbcfg.DispatcherProcessorConfig(
                        num_processors=4,
                        processor_config=mqbcfg.DispatcherProcessorParameters(
                            queue_size=500000,
                            queue_size_low_watermark=100,
                            queue_size_high_watermark=200000,
                        ),
                    ),
                ),
                stats=mqbcfg.StatsConfig(
                    plugins=[],
                    snapshot_interval=1,
                    printer=mqbcfg.StatsPrinterConfig(
                        print_interval=60,
                        file="logs/stat.%T.%p",
                        max_age_days=3,
                        rotate_bytes=268435456,
                        rotate_days=1,
                    ),
                ),
                network_interfaces=mqbcfg.NetworkInterfaces(
                    heartbeats=mqbcfg.Heartbeat(
                        client=0,
                        downstream_broker=10,
                        upstream_broker=10,
                        cluster_peer=10,
                    ),
                    tcp_interface=mqbcfg.TcpInterfaceConfig(
                        name="TCPInterface",
                        port=0,  # overwritten
                        io_threads=4,
                        max_connections=10000,
                        low_watermark=4194304,
                        high_watermark=1073741824,
                        node_low_watermark=5242880,
                        node_high_watermark=1073741824,
                        heartbeat_interval_ms=3000,
                        use_ntf=False,
                    ),
                ),
                bmqconf_config=mqbcfg.BmqconfConfig(cache_ttlseconds=30),
                plugins=mqbcfg.Plugins(
                    enabled=[],
                    libraries=[],
                ),
                message_properties_v2=mqbcfg.MessagePropertiesV2(
                    advertise_v2_support=True,
                    min_cpp_sdk_version=11207,
                    min_java_sdk_version=10,
                ),
            ),
        )
    )

    cluster: mqbcfg.ClusterDefinition = field(
        default_factory=functools.partial(
            _cluster_definition_partial_prototype,
            partition_config=mqbcfg.PartitionConfig(
                num_partitions=4,
                location="storage",
                archive_location="storage/archive",
                max_data_file_size=268435456,
                max_journal_file_size=67108864,
                max_qlist_file_size=33554432,
                preallocate=False,
                max_archived_file_sets=0,
                prefault_pages=False,
                flush_at_shutdown=True,
                sync_config=mqbcfg.StorageSyncConfig(
                    startup_recovery_max_duration_ms=20000,
                    max_attempts_storage_sync=2,
                    storage_sync_req_timeout_ms=10000,
                    master_sync_max_duration_ms=10000,
                    partition_sync_state_req_timeout_ms=10000,
                    partition_sync_data_req_timeout_ms=10000,
                    startup_wait_duration_ms=5000,
                    file_chunk_size=4194304,
                    partition_sync_event_size=4194304,
                ),
            ),
        )
    )

    virtual_cluster: mqbcfg.ClusterDefinition = field(
        default_factory=functools.partial(
            _cluster_definition_partial_prototype,
            partition_config=mqbcfg.PartitionConfig(
                num_partitions=0,
                location="/dev/null",
                max_data_file_size=0,
                max_journal_file_size=0,
                max_qlist_file_size=0,
                preallocate=False,
                max_archived_file_sets=0,
                prefault_pages=False,
                flush_at_shutdown=True,
                sync_config=mqbcfg.StorageSyncConfig(
                    startup_recovery_max_duration_ms=0,
                    max_attempts_storage_sync=0,
                    storage_sync_req_timeout_ms=0,
                    master_sync_max_duration_ms=0,
                    partition_sync_state_req_timeout_ms=0,
                    partition_sync_data_req_timeout_ms=0,
                    startup_wait_duration_ms=0,
                    file_chunk_size=0,
                    partition_sync_event_size=0,
                ),
            ),
        )
    )


@dataclass(frozen=True)
class Configurator:
    """
    Configurator builder.

    This mechanism has two purposes:

    * Incrementally build configurations for a set of brokers, clusters and
      domains.
    * Write the configuration for each broker in its own directory, and create
      various scripts to start the clusters as a whole, or the brokers
      individually.
    """

    proto: Proto = field(default_factory=lambda: copy.deepcopy(Proto()))
    brokers: Dict[str, Broker] = field(default_factory=dict)
    clusters: Dict[str, AbstractCluster] = field(default_factory=dict)
    host_id_allocator: Iterator[int] = field(
        default_factory=functools.partial(itertools.count, 1)
    )

    def broker_configuration(self):
        return copy.deepcopy(self.proto.broker)

    def cluster_definition(self):
        return copy.deepcopy(self.proto.cluster)

    def virtual_cluster_definition(self):
        return copy.deepcopy(self.proto.virtual_cluster)

    def domain_definition(self):
        return copy.deepcopy(self.proto.domain)

    def broker(
        self,
        tcp_host: str,
        tcp_port: int,
        name: str,
        instance: Optional[str] = None,
        data_center: str = "dc",
    ) -> Broker:
        config = self.broker_configuration()
        assert config.app_config is not None
        assert config.app_config.network_interfaces is not None
        assert config.app_config.network_interfaces.tcp_interface is not None
        config.app_config.host_data_center = data_center
        config.app_config.host_name = name
        config.app_config.broker_instance_name = instance
        config.app_config.network_interfaces.tcp_interface.name = tcp_host
        config.app_config.network_interfaces.tcp_interface.port = tcp_port
        broker = Broker(self, next(self.host_id_allocator), config)
        self.brokers[name] = broker

        return broker

    def _prepare_cluster(
        self, name: str, nodes: List[Broker], definition: mqbcfg.ClusterDefinition
    ):
        if name in self.clusters:
            raise ConfiguratorError(f"cluster '{name}' already exists")

        definition.name = name
        definition.nodes = [
            mqbcfg.ClusterNode(
                id=broker.id,
                name=broker.name,
                data_center=broker.data_center,
                transport=mqbcfg.ClusterNodeConnection(
                    mqbcfg.TcpClusterNodeConnection(
                        "tcp://{host}:{port}".format(
                            host=tcp_interface.name,  # type: ignore
                            port=tcp_interface.port,  # type: ignore
                        )
                    )
                ),
            )
            for broker in nodes
            for tcp_interface in (
                broker.config.app_config.network_interfaces.tcp_interface,
            )
        ]

    def cluster(self, name: str, nodes: List[Broker]) -> Cluster:
        definition = self.cluster_definition()
        self._prepare_cluster(name, nodes, definition)

        for node in nodes:
            node.clusters.my_clusters.append(copy.deepcopy(definition))
            # Do not share 'definition' between Broker instances. This makes it
            # possible to create faulty configs for tests.

        cluster = Cluster(self, definition, {node.name: node for node in nodes}, {})
        self.clusters[name] = cluster

        return cluster

    def virtual_cluster(self, name: str, nodes: List[Broker]) -> VirtualCluster:
        definition = self.virtual_cluster_definition()
        self._prepare_cluster(name, nodes, definition)

        for ws_node, cfg_node in zip(nodes, definition.nodes):
            ws_node.clusters.my_virtual_clusters.append(
                mqbcfg.VirtualClusterInformation(name, self_node_id=cfg_node.id)
            )

        cluster = VirtualCluster(
            self, definition, {node.name: node for node in nodes}, {}
        )
        self.clusters[name] = cluster

        return cluster

    @property
    def domains(self) -> List[Domain]:
        return [
            domain
            for cluster in self.clusters.values()
            for domain in cluster.domains.values()
        ]


class MonitoredProcess:
    process: Optional[subprocess.Popen] = None
    thread: Optional[threading.Thread] = None


def broker_monitor(out: IO[str], prefix: str, color: str):
    while not out.closed:
        line = out.readline()
        if line == "":
            break
        line = line.rstrip(" \n\r")
        if line:
            broker_logger.info(colored("%s | %s", color), prefix, line)


def _json_filter(kv_pairs: Tuple) -> Dict:
    return {k: (float(v) if "Ratio" in k else v) for k, v in kv_pairs if v is not None}


class LocalSite(Site):
    root_dir: Path

    def __init__(self, root_dir: Union[Path, str]):
        self.root_dir = Path(root_dir)

    def __str__(self) -> str:
        return str(self.root_dir)

    def mkdir(self, path: Union[str, Path]) -> None:
        target = self.root_dir / path
        target.mkdir(0o755, exist_ok=True, parents=True)

    def rmdir(self, path: Union[str, Path]) -> None:
        rmtree(self.root_dir / path, ignore_errors=True)

    def install(self, from_path: Union[str, Path], to_path: Union[str, Path]) -> None:
        from_path = Path(from_path).resolve()
        to_path = self.root_dir / to_path
        to_path.mkdir(0o755, exist_ok=True, parents=True)
        target = Path(to_path) / from_path.name
        if target.is_symlink():
            target.unlink(missing_ok=True)
        target.symlink_to(from_path.resolve(), target_is_directory=from_path.is_dir())

    def create_file(self, path: Union[str, Path], content: str, mode=None) -> None:
        path = self.root_dir / path
        path.parent.mkdir(0o755, exist_ok=True, parents=True)
        with open(path, "w", encoding="ascii") as out:
            out.write(content)
        path.chmod(mode or 0o644)

    def create_json_file(self, path: Union[str, Path], content) -> None:
        config = SerializerConfig(pretty_print=True)
        config.ignore_default_attributes = True
        serializer = JsonSerializer(
            context=XmlContext(), config=config, dict_factory=_json_filter
        )
        path = self.root_dir / path
        path.parent.mkdir(0o755, exist_ok=True, parents=True)

        with open(path, "w", encoding="ascii") as out:
            serializer.write(out, content)
        path.chmod(0o644)


@dataclass
class Session(contextlib.AbstractContextManager):
    configurator: Configurator
    root: Path
    brokers: Dict[Broker, MonitoredProcess] = field(default_factory=dict)

    def __exit__(self, *args):
        for broker in reversed(self.brokers.values()):
            if broker.process is not None:
                broker.process.__exit__(*args)

        for broker in reversed(self.brokers.values()):
            if broker.thread is not None:
                broker.thread.join()

    def stop(self):
        for broker in self.brokers.values():
            if broker.process is not None:
                broker.process.terminate()
                broker.process.wait()

    def run(self):
        colors = itertools.cycle(COLORS)
        prefix_len = max(len(name) for name in self.configurator.brokers)

        for broker in self.configurator.brokers.values():
            monitored = MonitoredProcess()
            self.brokers[broker] = monitored

            monitored.process = subprocess.Popen(
                [self.root.joinpath(broker.name, "run")],
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                encoding="ASCII",
                bufsize=0,
            )

            assert monitored.process.stdout is not None
            monitored.thread = threading.Thread(
                target=broker_monitor,
                args=(
                    monitored.process.stdout,
                    broker.name.ljust(prefix_len),
                    next(colors),
                ),
            )
            monitored.thread.start()
