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

from xsdata.formats.dataclass.context import XmlContext
from xsdata.formats.dataclass.serializers import JsonSerializer
from xsdata.formats.dataclass.serializers.config import SerializerConfig

from blazingmq.dev.configurator import *
from blazingmq.dev.configurator.site import Site
from blazingmq.dev.paths import required_paths as paths
from blazingmq.schemas import mqbcfg, mqbconf

logger = logging.getLogger(__name__)


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
        domain = copy.deepcopy(self.proto.domain)
        domain.mode.broadcast = None
        domain.mode.fanout = None
        domain.mode.priority = None

        return domain

    def broadcast_domain(self):
        domain = copy.deepcopy(self.proto.domain)
        domain.mode.fanout = None
        domain.mode.priority = None

        return domain

    def fanout_domain(self):
        domain = copy.deepcopy(self.proto.domain)
        domain.mode.broadcast = None
        domain.mode.priority = None

        return domain

    def priority_domain(self):
        domain = copy.deepcopy(self.proto.domain)
        domain.mode.broadcast = None
        domain.mode.fanout = None

        return domain

    def broker(
        self,
        tcp_host: str,
        tcp_port: int,
        name: str,
        instance: Optional[str] = None,
        data_center: str = "dc",
        listeners: List[Tuple[str, int]] = [],
        broker_version: Optional[int] = None,
    ) -> Broker:
        """Create a broker config and add it to the list of broker configs in
        the current configs.

        Args:
            tcp_host: config/app_config/network_interfaces/tcp_interface/name
            tcp_port: config/app_config/network_interfaces/tcp_interface/port
            name: The host name for this broker.
            instance: The BlazingMQ instance this broker belongs to.
            data_center: A name for the data center region this broker belongs
                to.
            listeners: A list of TCP interfaces the broker will listen on. The
                option overrides tcp_host and tcp_port if non-empty.
            broker_version: Optional broker version to specify binaries folder
                "BLAZINGMQ_BROKER{version}"

        Returns:
            A Broker object representing the fully configured broker
                configuration.
        """
        config = self.broker_configuration()
        assert config.app_config is not None
        assert config.app_config.network_interfaces is not None
        assert config.app_config.network_interfaces.tcp_interface is not None
        config.app_config.host_data_center = data_center
        config.app_config.host_name = name
        config.app_config.broker_instance_name = instance
        config.app_config.broker_version = broker_version
        config.app_config.network_interfaces.tcp_interface.name = tcp_host
        config.app_config.network_interfaces.tcp_interface.port = tcp_port
        config.app_config.network_interfaces.tcp_interface.listeners = [
            mqbcfg.TcpInterfaceListener(name=host, port=port)
            for (host, port) in listeners
        ]
        broker = Broker(self, next(self.host_id_allocator), config)
        self.brokers[name] = broker

        return broker

    def _prepare_cluster(
        self, name: str, nodes: List[Broker], definition: mqbcfg.ClusterDefinition
    ):
        if name in self.clusters:
            raise ConfiguratorError(f"cluster '{name}' already exists")

        def port(tcp_interface: mqbcfg.TcpInterfaceConfig) -> int:
            if tcp_interface.listeners:
                return next(
                    listener.port
                    for listener in tcp_interface.listeners
                    if listener.name == "BROKER"
                )
            else:
                return tcp_interface.port

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
                            port=port(tcp_interface),  # type: ignore
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
        """Create a cluster config and add it to the list of cluster configs in the
        current configs.

        If a broker specifies listeners in its TCP interface config, the inter-broker
        listener URI will be constructed as tcp://<broker.tcp_host>:<port> where <port>
        is specified by the special listener named "BROKER". Otherwise, the port is
        taken from the port field in the TCP interface config.

        Args:
            name: The name of the cluster.
            nodes: The broker members of the cluster.

        Returns:
            A cluster config definition.
        """
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

    def deploy(self, broker: Broker, site: Site) -> None:
        self.deploy_programs(broker, site)
        self.deploy_broker_config(broker, site)
        self.deploy_domains(broker, site)

    def deploy_programs(self, broker: Broker, site: Site) -> None:
        # print(f"deploy_programs {broker.config.app_config}")
        site.install(str(paths.getBrokerWithVersion(broker.version)), "bin")
        site.install(str(paths.tool), "bin")
        site.install(str(paths.plugins), ".")

        for script, cmd in (
            ("run", "exec"),
            ("debug", "gdb --args"),
            ("debug-lldb", "lldb --"),
        ):
            site.create_file(
                str(script),
                RUN_SCRIPT.format(cmd=cmd, host=broker.name),
                0o755,
            )

        for script, cmd in (
            ("run-client", "exec"),
            ("debug-client", "gdb ./bmqtool.tsk --args"),
        ):
            site.create_file(
                str(script),
                TOOL_SCRIPT.format(
                    cmd=cmd,
                    BMQ_PORT=broker.port,
                ),
                0o755,
            )

    def _create_json_file(self, obj, site: Site, path: str):
        def json_filter(kv_pairs: Tuple) -> Dict:
            return {
                k: (float(v) if "Ratio" in k else v)
                for k, v in kv_pairs
                if v is not None
            }

        config = SerializerConfig(indent=" " * 4)
        config.ignore_default_attributes = True
        serializer = JsonSerializer(
            context=XmlContext(), config=config, dict_factory=json_filter
        )
        site.create_file(str(path), serializer.render(obj), 0o644)

    def deploy_broker_config(self, broker: Broker, site: Site) -> None:
        self._create_json_file(broker.config, site, "etc/bmqbrkrcfg.json")

        log_dir = Path(broker.config.task_config.log_controller.file_name).parent  # type: ignore
        if log_dir != Path():
            site.mkdir(str(log_dir))

        stats_dir = Path(broker.config.app_config.stats.printer.file).parent  # type: ignore
        if stats_dir != Path():
            site.mkdir(str(stats_dir))

    def deploy_domains(self, broker: Broker, site: Site) -> None:
        self._create_json_file(broker.clusters, site, "etc/clusters.json")

        for cluster in broker.clusters.my_clusters:
            for storage_dir in (
                Path(cluster.partition_config.location),  # type: ignore
                Path(cluster.partition_config.archive_location),  # type: ignore
            ):
                if storage_dir != Path():
                    site.mkdir(str(storage_dir))

        site.rmdir(str("etc/domains"))

        for domain in broker.domains.values():
            self._create_json_file(
                mqbconf.DomainVariant(definition=domain.definition),
                site,
                f"etc/domains/{domain.name}.json",
            )
