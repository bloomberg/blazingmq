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


import copy
import functools
import itertools
import logging
from dataclasses import dataclass, field
from decimal import Decimal
from pathlib import Path
from typing import Dict, Iterator, List, Optional, Set, Tuple, Union
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from blazingmq.dev.configurator import Configurator

from blazingmq.dev.configurator.site import Site
from blazingmq.dev.paths import required_paths as paths
from blazingmq.schemas import mqbcfg, mqbconf

__all__ = [
    "AbstractCluster",
    "Broker",
    "Cluster",
    "ConfiguratorError",
    "Domain",
    "Proto",
    "VirtualCluster",
]


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
    proxy_clusters: Set[str] = field(default_factory=set)

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

    @property
    def listeners(self) -> List[mqbcfg.TcpInterfaceListener]:
        return self.config.app_config.network_interfaces.tcp_interface.listeners

    def add_proxy_definition(self, cluster: "AbstractCluster"):
        self.proxy_clusters.add(cluster.name)
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

            if cluster.name not in self.proxy_clusters:
                self.add_proxy_definition(cluster)

            if not reverse:
                continue

            if cluster.name in self.clusters.my_reverse_clusters:
                continue

            self.clusters.my_reverse_clusters.append(cluster.name)
            reverse_cluster = self.configurator.clusters[cluster.name]

            for node in reverse_cluster.nodes.values():
                if reverse_cluster.name not in node.proxy_clusters:
                    node.add_proxy_definition(reverse_cluster)

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

                if self.listeners:
                    for listener in self.listeners:
                        print(listener)
                        reversed_cluster_connections.connections.append(
                            mqbcfg.ClusterNodeConnection(
                                mqbcfg.TcpClusterNodeConnection(
                                    f"tcp://{self.host}:{listener.port}"
                                )
                            )
                        )
                else:
                    reversed_cluster_connections.connections.append(
                        mqbcfg.ClusterNodeConnection(
                            mqbcfg.TcpClusterNodeConnection(
                                f"tcp://{self.host}:{self.port}"
                            )
                        )
                    )

        return self


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
        parameters = self.configurator.broadcast_domain()
        parameters.name = name
        parameters.storage.config.in_memory = mqbconf.InMemoryStorage()
        parameters.storage.config.file_backed = None
        domain = mqbconf.DomainDefinition(self.name, parameters)

        return self._add_domain(Domain(self, domain))

    def fanout_domain(self, name: str, app_ids: List[str]) -> "Domain":
        parameters = self.configurator.fanout_domain()
        parameters.name = name
        parameters.mode.fanout.app_ids = app_ids.copy()
        domain = mqbconf.DomainDefinition(self.name, parameters)

        return self._add_domain(Domain(self, domain))

    def priority_domain(self, name: str) -> "Domain":
        parameters = self.configurator.priority_domain()
        parameters.name = name
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
            is_cslmode_enabled=True, is_fsmworkflow=True
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
            mode=mqbconf.QueueMode(
                broadcast=mqbconf.QueueModeBroadcast(),
                fanout=mqbconf.QueueModeFanout(),
                priority=mqbconf.QueueModePriority(),
            ),
            max_delivery_attempts=0,
            deduplication_time_ms=300000,
            consistency=mqbconf.Consistency(strong=mqbconf.QueueConsistencyStrong()),
            subscriptions=[],
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
                        client=10,
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
                archive_location="/dev/null",
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
