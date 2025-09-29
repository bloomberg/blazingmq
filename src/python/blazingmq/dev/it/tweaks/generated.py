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
GENERATED CODE - DO NOT EDIT
"""

# pylint: disable=missing-class-docstring, missing-function-docstring
# pyright: reportOptionalMemberAccess=false
# mypy: disable-error-code="union-attr"

import decimal
import typing
from typing import Callable

import blazingmq.schemas.mqbcfg
import blazingmq.schemas.mqbconf

from . import TweakMetaclass, decorator

NoneType = typing.Type[None]


class TweakFactory:
    def __call__(self, tweak: Callable) -> Callable:
        return decorator(tweak)

    class Broker:
        class TaskConfig(metaclass=TweakMetaclass):
            class AllocatorType(metaclass=TweakMetaclass):
                def __call__(
                    self,
                    value: typing.Union[
                        blazingmq.schemas.mqbcfg.AllocatorType, NoneType
                    ],
                ) -> Callable: ...

            allocator_type = AllocatorType()

            class AllocationLimit(metaclass=TweakMetaclass):
                def __call__(self, value: typing.Union[int, NoneType]) -> Callable: ...

            allocation_limit = AllocationLimit()

            class LogController(metaclass=TweakMetaclass):
                class FileName(metaclass=TweakMetaclass):
                    def __call__(
                        self, value: typing.Union[str, NoneType]
                    ) -> Callable: ...

                file_name = FileName()

                class FileMaxAgeDays(metaclass=TweakMetaclass):
                    def __call__(
                        self, value: typing.Union[int, NoneType]
                    ) -> Callable: ...

                file_max_age_days = FileMaxAgeDays()

                class RotationBytes(metaclass=TweakMetaclass):
                    def __call__(
                        self, value: typing.Union[int, NoneType]
                    ) -> Callable: ...

                rotation_bytes = RotationBytes()

                class LogfileFormat(metaclass=TweakMetaclass):
                    def __call__(
                        self, value: typing.Union[str, NoneType]
                    ) -> Callable: ...

                logfile_format = LogfileFormat()

                class ConsoleFormat(metaclass=TweakMetaclass):
                    def __call__(
                        self, value: typing.Union[str, NoneType]
                    ) -> Callable: ...

                console_format = ConsoleFormat()

                class LoggingVerbosity(metaclass=TweakMetaclass):
                    def __call__(
                        self, value: typing.Union[str, NoneType]
                    ) -> Callable: ...

                logging_verbosity = LoggingVerbosity()

                class BslsLogSeverityThreshold(metaclass=TweakMetaclass):
                    def __call__(self, value: str) -> Callable: ...

                bsls_log_severity_threshold = BslsLogSeverityThreshold()

                class ConsoleSeverityThreshold(metaclass=TweakMetaclass):
                    def __call__(
                        self, value: typing.Union[str, NoneType]
                    ) -> Callable: ...

                console_severity_threshold = ConsoleSeverityThreshold()

                class Categories(metaclass=TweakMetaclass):
                    def __call__(self, value: None) -> Callable: ...

                categories = Categories()

                class Syslog(metaclass=TweakMetaclass):
                    class Enabled(metaclass=TweakMetaclass):
                        def __call__(self, value: bool) -> Callable: ...

                    enabled = Enabled()

                    class AppName(metaclass=TweakMetaclass):
                        def __call__(
                            self, value: typing.Union[str, NoneType]
                        ) -> Callable: ...

                    app_name = AppName()

                    class LogFormat(metaclass=TweakMetaclass):
                        def __call__(
                            self, value: typing.Union[str, NoneType]
                        ) -> Callable: ...

                    log_format = LogFormat()

                    class Verbosity(metaclass=TweakMetaclass):
                        def __call__(
                            self, value: typing.Union[str, NoneType]
                        ) -> Callable: ...

                    verbosity = Verbosity()

                    def __call__(
                        self,
                        value: typing.Union[
                            blazingmq.schemas.mqbcfg.SyslogConfig, NoneType
                        ],
                    ) -> Callable: ...

                syslog = Syslog()

                class LogDump(metaclass=TweakMetaclass):
                    class RecordBufferSize(metaclass=TweakMetaclass):
                        def __call__(self, value: int) -> Callable: ...

                    record_buffer_size = RecordBufferSize()

                    class RecordingLevel(metaclass=TweakMetaclass):
                        def __call__(self, value: str) -> Callable: ...

                    recording_level = RecordingLevel()

                    class TriggerLevel(metaclass=TweakMetaclass):
                        def __call__(self, value: str) -> Callable: ...

                    trigger_level = TriggerLevel()

                    def __call__(
                        self,
                        value: typing.Union[
                            blazingmq.schemas.mqbcfg.LogDumpConfig, NoneType
                        ],
                    ) -> Callable: ...

                log_dump = LogDump()

                def __call__(
                    self,
                    value: typing.Union[
                        blazingmq.schemas.mqbcfg.LogController, NoneType
                    ],
                ) -> Callable: ...

            log_controller = LogController()

            def __call__(
                self, value: typing.Union[blazingmq.schemas.mqbcfg.TaskConfig, NoneType]
            ) -> Callable: ...

        task_config = TaskConfig()

        class AppConfig(metaclass=TweakMetaclass):
            class BrokerInstanceName(metaclass=TweakMetaclass):
                def __call__(self, value: typing.Union[str, NoneType]) -> Callable: ...

            broker_instance_name = BrokerInstanceName()

            class BrokerVersion(metaclass=TweakMetaclass):
                def __call__(self, value: typing.Union[int, NoneType]) -> Callable: ...

            broker_version = BrokerVersion()

            class ConfigVersion(metaclass=TweakMetaclass):
                def __call__(self, value: typing.Union[int, NoneType]) -> Callable: ...

            config_version = ConfigVersion()

            class EtcDir(metaclass=TweakMetaclass):
                def __call__(self, value: typing.Union[str, NoneType]) -> Callable: ...

            etc_dir = EtcDir()

            class HostName(metaclass=TweakMetaclass):
                def __call__(self, value: typing.Union[str, NoneType]) -> Callable: ...

            host_name = HostName()

            class HostTags(metaclass=TweakMetaclass):
                def __call__(self, value: typing.Union[str, NoneType]) -> Callable: ...

            host_tags = HostTags()

            class HostDataCenter(metaclass=TweakMetaclass):
                def __call__(self, value: typing.Union[str, NoneType]) -> Callable: ...

            host_data_center = HostDataCenter()

            class LogsObserverMaxSize(metaclass=TweakMetaclass):
                def __call__(self, value: typing.Union[int, NoneType]) -> Callable: ...

            logs_observer_max_size = LogsObserverMaxSize()

            class LatencyMonitorDomain(metaclass=TweakMetaclass):
                def __call__(self, value: str) -> Callable: ...

            latency_monitor_domain = LatencyMonitorDomain()

            class DispatcherConfig(metaclass=TweakMetaclass):
                class Sessions(metaclass=TweakMetaclass):
                    class NumProcessors(metaclass=TweakMetaclass):
                        def __call__(
                            self, value: typing.Union[int, NoneType]
                        ) -> Callable: ...

                    num_processors = NumProcessors()

                    class ProcessorConfig(metaclass=TweakMetaclass):
                        class QueueSize(metaclass=TweakMetaclass):
                            def __call__(
                                self, value: typing.Union[int, NoneType]
                            ) -> Callable: ...

                        queue_size = QueueSize()

                        class QueueSizeLowWatermark(metaclass=TweakMetaclass):
                            def __call__(
                                self, value: typing.Union[int, NoneType]
                            ) -> Callable: ...

                        queue_size_low_watermark = QueueSizeLowWatermark()

                        class QueueSizeHighWatermark(metaclass=TweakMetaclass):
                            def __call__(
                                self, value: typing.Union[int, NoneType]
                            ) -> Callable: ...

                        queue_size_high_watermark = QueueSizeHighWatermark()

                        def __call__(
                            self,
                            value: typing.Union[
                                blazingmq.schemas.mqbcfg.DispatcherProcessorParameters,
                                NoneType,
                            ],
                        ) -> Callable: ...

                    processor_config = ProcessorConfig()

                    def __call__(
                        self,
                        value: typing.Union[
                            blazingmq.schemas.mqbcfg.DispatcherProcessorConfig, NoneType
                        ],
                    ) -> Callable: ...

                sessions = Sessions()

                class Queues(metaclass=TweakMetaclass):
                    class NumProcessors(metaclass=TweakMetaclass):
                        def __call__(
                            self, value: typing.Union[int, NoneType]
                        ) -> Callable: ...

                    num_processors = NumProcessors()

                    class ProcessorConfig(metaclass=TweakMetaclass):
                        class QueueSize(metaclass=TweakMetaclass):
                            def __call__(
                                self, value: typing.Union[int, NoneType]
                            ) -> Callable: ...

                        queue_size = QueueSize()

                        class QueueSizeLowWatermark(metaclass=TweakMetaclass):
                            def __call__(
                                self, value: typing.Union[int, NoneType]
                            ) -> Callable: ...

                        queue_size_low_watermark = QueueSizeLowWatermark()

                        class QueueSizeHighWatermark(metaclass=TweakMetaclass):
                            def __call__(
                                self, value: typing.Union[int, NoneType]
                            ) -> Callable: ...

                        queue_size_high_watermark = QueueSizeHighWatermark()

                        def __call__(
                            self,
                            value: typing.Union[
                                blazingmq.schemas.mqbcfg.DispatcherProcessorParameters,
                                NoneType,
                            ],
                        ) -> Callable: ...

                    processor_config = ProcessorConfig()

                    def __call__(
                        self,
                        value: typing.Union[
                            blazingmq.schemas.mqbcfg.DispatcherProcessorConfig, NoneType
                        ],
                    ) -> Callable: ...

                queues = Queues()

                class Clusters(metaclass=TweakMetaclass):
                    class NumProcessors(metaclass=TweakMetaclass):
                        def __call__(
                            self, value: typing.Union[int, NoneType]
                        ) -> Callable: ...

                    num_processors = NumProcessors()

                    class ProcessorConfig(metaclass=TweakMetaclass):
                        class QueueSize(metaclass=TweakMetaclass):
                            def __call__(
                                self, value: typing.Union[int, NoneType]
                            ) -> Callable: ...

                        queue_size = QueueSize()

                        class QueueSizeLowWatermark(metaclass=TweakMetaclass):
                            def __call__(
                                self, value: typing.Union[int, NoneType]
                            ) -> Callable: ...

                        queue_size_low_watermark = QueueSizeLowWatermark()

                        class QueueSizeHighWatermark(metaclass=TweakMetaclass):
                            def __call__(
                                self, value: typing.Union[int, NoneType]
                            ) -> Callable: ...

                        queue_size_high_watermark = QueueSizeHighWatermark()

                        def __call__(
                            self,
                            value: typing.Union[
                                blazingmq.schemas.mqbcfg.DispatcherProcessorParameters,
                                NoneType,
                            ],
                        ) -> Callable: ...

                    processor_config = ProcessorConfig()

                    def __call__(
                        self,
                        value: typing.Union[
                            blazingmq.schemas.mqbcfg.DispatcherProcessorConfig, NoneType
                        ],
                    ) -> Callable: ...

                clusters = Clusters()

                def __call__(
                    self,
                    value: typing.Union[
                        blazingmq.schemas.mqbcfg.DispatcherConfig, NoneType
                    ],
                ) -> Callable: ...

            dispatcher_config = DispatcherConfig()

            class Stats(metaclass=TweakMetaclass):
                class SnapshotInterval(metaclass=TweakMetaclass):
                    def __call__(self, value: int) -> Callable: ...

                snapshot_interval = SnapshotInterval()

                class Plugins(metaclass=TweakMetaclass):
                    class Name(metaclass=TweakMetaclass):
                        def __call__(self, value: str) -> Callable: ...

                    name = Name()

                    class QueueSize(metaclass=TweakMetaclass):
                        def __call__(self, value: int) -> Callable: ...

                    queue_size = QueueSize()

                    class QueueHighWatermark(metaclass=TweakMetaclass):
                        def __call__(self, value: int) -> Callable: ...

                    queue_high_watermark = QueueHighWatermark()

                    class QueueLowWatermark(metaclass=TweakMetaclass):
                        def __call__(self, value: int) -> Callable: ...

                    queue_low_watermark = QueueLowWatermark()

                    class PublishInterval(metaclass=TweakMetaclass):
                        def __call__(self, value: int) -> Callable: ...

                    publish_interval = PublishInterval()

                    class NamespacePrefix(metaclass=TweakMetaclass):
                        def __call__(self, value: str) -> Callable: ...

                    namespace_prefix = NamespacePrefix()

                    class Hosts(metaclass=TweakMetaclass):
                        def __call__(self, value: None) -> Callable: ...

                    hosts = Hosts()

                    class InstanceId(metaclass=TweakMetaclass):
                        def __call__(self, value: str) -> Callable: ...

                    instance_id = InstanceId()

                    class PrometheusSpecific(metaclass=TweakMetaclass):
                        class Mode(metaclass=TweakMetaclass):
                            def __call__(
                                self, value: blazingmq.schemas.mqbcfg.ExportMode
                            ) -> Callable: ...

                        mode = Mode()

                        class Host(metaclass=TweakMetaclass):
                            def __call__(self, value: str) -> Callable: ...

                        host = Host()

                        class Port(metaclass=TweakMetaclass):
                            def __call__(self, value: int) -> Callable: ...

                        port = Port()

                        def __call__(
                            self,
                            value: typing.Union[
                                blazingmq.schemas.mqbcfg.StatPluginConfigPrometheus,
                                NoneType,
                            ],
                        ) -> Callable: ...

                    prometheus_specific = PrometheusSpecific()

                    def __call__(self, value: None) -> Callable: ...

                plugins = Plugins()

                class Printer(metaclass=TweakMetaclass):
                    class PrintInterval(metaclass=TweakMetaclass):
                        def __call__(self, value: int) -> Callable: ...

                    print_interval = PrintInterval()

                    class File(metaclass=TweakMetaclass):
                        def __call__(
                            self, value: typing.Union[str, NoneType]
                        ) -> Callable: ...

                    file = File()

                    class MaxAgeDays(metaclass=TweakMetaclass):
                        def __call__(
                            self, value: typing.Union[int, NoneType]
                        ) -> Callable: ...

                    max_age_days = MaxAgeDays()

                    class RotateBytes(metaclass=TweakMetaclass):
                        def __call__(self, value: int) -> Callable: ...

                    rotate_bytes = RotateBytes()

                    class RotateDays(metaclass=TweakMetaclass):
                        def __call__(self, value: int) -> Callable: ...

                    rotate_days = RotateDays()

                    def __call__(
                        self,
                        value: typing.Union[
                            blazingmq.schemas.mqbcfg.StatsPrinterConfig, NoneType
                        ],
                    ) -> Callable: ...

                printer = Printer()

                def __call__(
                    self,
                    value: typing.Union[blazingmq.schemas.mqbcfg.StatsConfig, NoneType],
                ) -> Callable: ...

            stats = Stats()

            class NetworkInterfaces(metaclass=TweakMetaclass):
                class Heartbeats(metaclass=TweakMetaclass):
                    class Client(metaclass=TweakMetaclass):
                        def __call__(self, value: int) -> Callable: ...

                    client = Client()

                    class DownstreamBroker(metaclass=TweakMetaclass):
                        def __call__(self, value: int) -> Callable: ...

                    downstream_broker = DownstreamBroker()

                    class UpstreamBroker(metaclass=TweakMetaclass):
                        def __call__(self, value: int) -> Callable: ...

                    upstream_broker = UpstreamBroker()

                    class ClusterPeer(metaclass=TweakMetaclass):
                        def __call__(self, value: int) -> Callable: ...

                    cluster_peer = ClusterPeer()

                    def __call__(
                        self,
                        value: typing.Union[
                            blazingmq.schemas.mqbcfg.Heartbeat, NoneType
                        ],
                    ) -> Callable: ...

                heartbeats = Heartbeats()

                class TcpInterface(metaclass=TweakMetaclass):
                    class Name(metaclass=TweakMetaclass):
                        def __call__(
                            self, value: typing.Union[str, NoneType]
                        ) -> Callable: ...

                    name = Name()

                    class Port(metaclass=TweakMetaclass):
                        def __call__(
                            self, value: typing.Union[int, NoneType]
                        ) -> Callable: ...

                    port = Port()

                    class IoThreads(metaclass=TweakMetaclass):
                        def __call__(
                            self, value: typing.Union[int, NoneType]
                        ) -> Callable: ...

                    io_threads = IoThreads()

                    class MaxConnections(metaclass=TweakMetaclass):
                        def __call__(self, value: int) -> Callable: ...

                    max_connections = MaxConnections()

                    class LowWatermark(metaclass=TweakMetaclass):
                        def __call__(
                            self, value: typing.Union[int, NoneType]
                        ) -> Callable: ...

                    low_watermark = LowWatermark()

                    class HighWatermark(metaclass=TweakMetaclass):
                        def __call__(
                            self, value: typing.Union[int, NoneType]
                        ) -> Callable: ...

                    high_watermark = HighWatermark()

                    class NodeLowWatermark(metaclass=TweakMetaclass):
                        def __call__(self, value: int) -> Callable: ...

                    node_low_watermark = NodeLowWatermark()

                    class NodeHighWatermark(metaclass=TweakMetaclass):
                        def __call__(self, value: int) -> Callable: ...

                    node_high_watermark = NodeHighWatermark()

                    class HeartbeatIntervalMs(metaclass=TweakMetaclass):
                        def __call__(self, value: int) -> Callable: ...

                    heartbeat_interval_ms = HeartbeatIntervalMs()

                    class Listeners(metaclass=TweakMetaclass):
                        class Name(metaclass=TweakMetaclass):
                            def __call__(
                                self, value: typing.Union[str, NoneType]
                            ) -> Callable: ...

                        name = Name()

                        class Port(metaclass=TweakMetaclass):
                            def __call__(
                                self, value: typing.Union[int, NoneType]
                            ) -> Callable: ...

                        port = Port()

                        def __call__(self, value: None) -> Callable: ...

                    listeners = Listeners()

                    def __call__(
                        self,
                        value: typing.Union[
                            blazingmq.schemas.mqbcfg.TcpInterfaceConfig, NoneType
                        ],
                    ) -> Callable: ...

                tcp_interface = TcpInterface()

                def __call__(
                    self,
                    value: typing.Union[
                        blazingmq.schemas.mqbcfg.NetworkInterfaces, NoneType
                    ],
                ) -> Callable: ...

            network_interfaces = NetworkInterfaces()

            class BmqconfConfig(metaclass=TweakMetaclass):
                class CacheTtlseconds(metaclass=TweakMetaclass):
                    def __call__(
                        self, value: typing.Union[int, NoneType]
                    ) -> Callable: ...

                cache_ttlseconds = CacheTtlseconds()

                def __call__(
                    self,
                    value: typing.Union[
                        blazingmq.schemas.mqbcfg.BmqconfConfig, NoneType
                    ],
                ) -> Callable: ...

            bmqconf_config = BmqconfConfig()

            class Plugins(metaclass=TweakMetaclass):
                class Libraries(metaclass=TweakMetaclass):
                    def __call__(self, value: None) -> Callable: ...

                libraries = Libraries()

                class Enabled(metaclass=TweakMetaclass):
                    def __call__(self, value: None) -> Callable: ...

                enabled = Enabled()

                def __call__(
                    self,
                    value: typing.Union[blazingmq.schemas.mqbcfg.Plugins, NoneType],
                ) -> Callable: ...

            plugins = Plugins()

            class MessagePropertiesV2(metaclass=TweakMetaclass):
                class AdvertiseV2Support(metaclass=TweakMetaclass):
                    def __call__(self, value: bool) -> Callable: ...

                advertise_v2_support = AdvertiseV2Support()

                class MinCppSdkVersion(metaclass=TweakMetaclass):
                    def __call__(self, value: int) -> Callable: ...

                min_cpp_sdk_version = MinCppSdkVersion()

                class MinJavaSdkVersion(metaclass=TweakMetaclass):
                    def __call__(self, value: int) -> Callable: ...

                min_java_sdk_version = MinJavaSdkVersion()

                def __call__(
                    self,
                    value: typing.Union[
                        blazingmq.schemas.mqbcfg.MessagePropertiesV2, NoneType
                    ],
                ) -> Callable: ...

            message_properties_v2 = MessagePropertiesV2()

            class ConfigureStream(metaclass=TweakMetaclass):
                def __call__(self, value: bool) -> Callable: ...

            configure_stream = ConfigureStream()

            class AdvertiseSubscriptions(metaclass=TweakMetaclass):
                def __call__(self, value: bool) -> Callable: ...

            advertise_subscriptions = AdvertiseSubscriptions()

            class RouteCommandTimeoutMs(metaclass=TweakMetaclass):
                def __call__(self, value: int) -> Callable: ...

            route_command_timeout_ms = RouteCommandTimeoutMs()

            def __call__(
                self, value: typing.Union[blazingmq.schemas.mqbcfg.AppConfig, NoneType]
            ) -> Callable: ...

        app_config = AppConfig()

    class Domain:
        class Name(metaclass=TweakMetaclass):
            def __call__(self, value: typing.Union[str, NoneType]) -> Callable: ...

        name = Name()

        class Mode(metaclass=TweakMetaclass):
            class Fanout(metaclass=TweakMetaclass):
                class AppIds(metaclass=TweakMetaclass):
                    def __call__(self, value: None) -> Callable: ...

                app_ids = AppIds()

                class PublishAppIdMetrics(metaclass=TweakMetaclass):
                    def __call__(self, value: bool) -> Callable: ...

                publish_app_id_metrics = PublishAppIdMetrics()

                def __call__(
                    self,
                    value: typing.Union[
                        blazingmq.schemas.mqbconf.QueueModeFanout, NoneType
                    ],
                ) -> Callable: ...

            fanout = Fanout()

            class Priority(metaclass=TweakMetaclass):
                def __call__(
                    self,
                    value: typing.Union[
                        blazingmq.schemas.mqbconf.QueueModePriority, NoneType
                    ],
                ) -> Callable: ...

            priority = Priority()

            class Broadcast(metaclass=TweakMetaclass):
                def __call__(
                    self,
                    value: typing.Union[
                        blazingmq.schemas.mqbconf.QueueModeBroadcast, NoneType
                    ],
                ) -> Callable: ...

            broadcast = Broadcast()

            def __call__(
                self, value: typing.Union[blazingmq.schemas.mqbconf.QueueMode, NoneType]
            ) -> Callable: ...

        mode = Mode()

        class Storage(metaclass=TweakMetaclass):
            class DomainLimits(metaclass=TweakMetaclass):
                class Messages(metaclass=TweakMetaclass):
                    def __call__(
                        self, value: typing.Union[int, NoneType]
                    ) -> Callable: ...

                messages = Messages()

                class MessagesWatermarkRatio(metaclass=TweakMetaclass):
                    def __call__(self, value: decimal.Decimal) -> Callable: ...

                messages_watermark_ratio = MessagesWatermarkRatio()

                class Bytes(metaclass=TweakMetaclass):
                    def __call__(
                        self, value: typing.Union[int, NoneType]
                    ) -> Callable: ...

                bytes = Bytes()

                class BytesWatermarkRatio(metaclass=TweakMetaclass):
                    def __call__(self, value: decimal.Decimal) -> Callable: ...

                bytes_watermark_ratio = BytesWatermarkRatio()

                def __call__(
                    self,
                    value: typing.Union[blazingmq.schemas.mqbconf.Limits, NoneType],
                ) -> Callable: ...

            domain_limits = DomainLimits()

            class QueueLimits(metaclass=TweakMetaclass):
                class Messages(metaclass=TweakMetaclass):
                    def __call__(
                        self, value: typing.Union[int, NoneType]
                    ) -> Callable: ...

                messages = Messages()

                class MessagesWatermarkRatio(metaclass=TweakMetaclass):
                    def __call__(self, value: decimal.Decimal) -> Callable: ...

                messages_watermark_ratio = MessagesWatermarkRatio()

                class Bytes(metaclass=TweakMetaclass):
                    def __call__(
                        self, value: typing.Union[int, NoneType]
                    ) -> Callable: ...

                bytes = Bytes()

                class BytesWatermarkRatio(metaclass=TweakMetaclass):
                    def __call__(self, value: decimal.Decimal) -> Callable: ...

                bytes_watermark_ratio = BytesWatermarkRatio()

                def __call__(
                    self,
                    value: typing.Union[blazingmq.schemas.mqbconf.Limits, NoneType],
                ) -> Callable: ...

            queue_limits = QueueLimits()

            class Config(metaclass=TweakMetaclass):
                class InMemory(metaclass=TweakMetaclass):
                    def __call__(
                        self,
                        value: typing.Union[
                            blazingmq.schemas.mqbconf.InMemoryStorage, NoneType
                        ],
                    ) -> Callable: ...

                in_memory = InMemory()

                class FileBacked(metaclass=TweakMetaclass):
                    def __call__(
                        self,
                        value: typing.Union[
                            blazingmq.schemas.mqbconf.FileBackedStorage, NoneType
                        ],
                    ) -> Callable: ...

                file_backed = FileBacked()

                def __call__(
                    self,
                    value: typing.Union[blazingmq.schemas.mqbconf.Storage, NoneType],
                ) -> Callable: ...

            config = Config()

            def __call__(
                self,
                value: typing.Union[
                    blazingmq.schemas.mqbconf.StorageDefinition, NoneType
                ],
            ) -> Callable: ...

        storage = Storage()

        class MaxConsumers(metaclass=TweakMetaclass):
            def __call__(self, value: int) -> Callable: ...

        max_consumers = MaxConsumers()

        class MaxProducers(metaclass=TweakMetaclass):
            def __call__(self, value: int) -> Callable: ...

        max_producers = MaxProducers()

        class MaxQueues(metaclass=TweakMetaclass):
            def __call__(self, value: int) -> Callable: ...

        max_queues = MaxQueues()

        class MsgGroupIdConfig(metaclass=TweakMetaclass):
            class Rebalance(metaclass=TweakMetaclass):
                def __call__(self, value: bool) -> Callable: ...

            rebalance = Rebalance()

            class MaxGroups(metaclass=TweakMetaclass):
                def __call__(self, value: int) -> Callable: ...

            max_groups = MaxGroups()

            class TtlSeconds(metaclass=TweakMetaclass):
                def __call__(self, value: int) -> Callable: ...

            ttl_seconds = TtlSeconds()

            def __call__(
                self,
                value: typing.Union[
                    blazingmq.schemas.mqbconf.MsgGroupIdConfig, NoneType
                ],
            ) -> Callable: ...

        msg_group_id_config = MsgGroupIdConfig()

        class MaxIdleTime(metaclass=TweakMetaclass):
            def __call__(self, value: int) -> Callable: ...

        max_idle_time = MaxIdleTime()

        class MessageTtl(metaclass=TweakMetaclass):
            def __call__(self, value: typing.Union[int, NoneType]) -> Callable: ...

        message_ttl = MessageTtl()

        class MaxDeliveryAttempts(metaclass=TweakMetaclass):
            def __call__(self, value: int) -> Callable: ...

        max_delivery_attempts = MaxDeliveryAttempts()

        class DeduplicationTimeMs(metaclass=TweakMetaclass):
            def __call__(self, value: int) -> Callable: ...

        deduplication_time_ms = DeduplicationTimeMs()

        class Consistency(metaclass=TweakMetaclass):
            class Eventual(metaclass=TweakMetaclass):
                def __call__(
                    self,
                    value: typing.Union[
                        blazingmq.schemas.mqbconf.QueueConsistencyEventual, NoneType
                    ],
                ) -> Callable: ...

            eventual = Eventual()

            class Strong(metaclass=TweakMetaclass):
                def __call__(
                    self,
                    value: typing.Union[
                        blazingmq.schemas.mqbconf.QueueConsistencyStrong, NoneType
                    ],
                ) -> Callable: ...

            strong = Strong()

            def __call__(
                self,
                value: typing.Union[blazingmq.schemas.mqbconf.Consistency, NoneType],
            ) -> Callable: ...

        consistency = Consistency()

        class Subscriptions(metaclass=TweakMetaclass):
            class AppId(metaclass=TweakMetaclass):
                def __call__(self, value: typing.Union[str, NoneType]) -> Callable: ...

            app_id = AppId()

            class Expression(metaclass=TweakMetaclass):
                class Version(metaclass=TweakMetaclass):
                    def __call__(
                        self, value: blazingmq.schemas.mqbconf.ExpressionVersion
                    ) -> Callable: ...

                version = Version()

                class Text(metaclass=TweakMetaclass):
                    def __call__(
                        self, value: typing.Union[str, NoneType]
                    ) -> Callable: ...

                text = Text()

                def __call__(
                    self,
                    value: typing.Union[blazingmq.schemas.mqbconf.Expression, NoneType],
                ) -> Callable: ...

            expression = Expression()

            def __call__(self, value: None) -> Callable: ...

        subscriptions = Subscriptions()

    class Cluster:
        class Name(metaclass=TweakMetaclass):
            def __call__(self, value: typing.Union[str, NoneType]) -> Callable: ...

        name = Name()

        class Nodes(metaclass=TweakMetaclass):
            class Id(metaclass=TweakMetaclass):
                def __call__(self, value: typing.Union[int, NoneType]) -> Callable: ...

            id = Id()

            class Name(metaclass=TweakMetaclass):
                def __call__(self, value: typing.Union[str, NoneType]) -> Callable: ...

            name = Name()

            class DataCenter(metaclass=TweakMetaclass):
                def __call__(self, value: typing.Union[str, NoneType]) -> Callable: ...

            data_center = DataCenter()

            class Transport(metaclass=TweakMetaclass):
                class Tcp(metaclass=TweakMetaclass):
                    class Endpoint(metaclass=TweakMetaclass):
                        def __call__(
                            self, value: typing.Union[str, NoneType]
                        ) -> Callable: ...

                    endpoint = Endpoint()

                    def __call__(
                        self,
                        value: typing.Union[
                            blazingmq.schemas.mqbcfg.TcpClusterNodeConnection, NoneType
                        ],
                    ) -> Callable: ...

                tcp = Tcp()

                def __call__(
                    self,
                    value: typing.Union[
                        blazingmq.schemas.mqbcfg.ClusterNodeConnection, NoneType
                    ],
                ) -> Callable: ...

            transport = Transport()

            def __call__(self, value: None) -> Callable: ...

        nodes = Nodes()

        class PartitionConfig(metaclass=TweakMetaclass):
            class NumPartitions(metaclass=TweakMetaclass):
                def __call__(self, value: typing.Union[int, NoneType]) -> Callable: ...

            num_partitions = NumPartitions()

            class Location(metaclass=TweakMetaclass):
                def __call__(self, value: typing.Union[str, NoneType]) -> Callable: ...

            location = Location()

            class ArchiveLocation(metaclass=TweakMetaclass):
                def __call__(self, value: typing.Union[str, NoneType]) -> Callable: ...

            archive_location = ArchiveLocation()

            class MaxDataFileSize(metaclass=TweakMetaclass):
                def __call__(self, value: typing.Union[int, NoneType]) -> Callable: ...

            max_data_file_size = MaxDataFileSize()

            class MaxJournalFileSize(metaclass=TweakMetaclass):
                def __call__(self, value: typing.Union[int, NoneType]) -> Callable: ...

            max_journal_file_size = MaxJournalFileSize()

            class MaxQlistFileSize(metaclass=TweakMetaclass):
                def __call__(self, value: typing.Union[int, NoneType]) -> Callable: ...

            max_qlist_file_size = MaxQlistFileSize()

            class MaxCslfileSize(metaclass=TweakMetaclass):
                def __call__(self, value: int) -> Callable: ...

            max_cslfile_size = MaxCslfileSize()

            class Preallocate(metaclass=TweakMetaclass):
                def __call__(self, value: bool) -> Callable: ...

            preallocate = Preallocate()

            class MaxArchivedFileSets(metaclass=TweakMetaclass):
                def __call__(self, value: typing.Union[int, NoneType]) -> Callable: ...

            max_archived_file_sets = MaxArchivedFileSets()

            class PrefaultPages(metaclass=TweakMetaclass):
                def __call__(self, value: bool) -> Callable: ...

            prefault_pages = PrefaultPages()

            class FlushAtShutdown(metaclass=TweakMetaclass):
                def __call__(self, value: bool) -> Callable: ...

            flush_at_shutdown = FlushAtShutdown()

            class SyncConfig(metaclass=TweakMetaclass):
                class StartupRecoveryMaxDurationMs(metaclass=TweakMetaclass):
                    def __call__(self, value: int) -> Callable: ...

                startup_recovery_max_duration_ms = StartupRecoveryMaxDurationMs()

                class MaxAttemptsStorageSync(metaclass=TweakMetaclass):
                    def __call__(self, value: int) -> Callable: ...

                max_attempts_storage_sync = MaxAttemptsStorageSync()

                class StorageSyncReqTimeoutMs(metaclass=TweakMetaclass):
                    def __call__(self, value: int) -> Callable: ...

                storage_sync_req_timeout_ms = StorageSyncReqTimeoutMs()

                class MasterSyncMaxDurationMs(metaclass=TweakMetaclass):
                    def __call__(self, value: int) -> Callable: ...

                master_sync_max_duration_ms = MasterSyncMaxDurationMs()

                class PartitionSyncStateReqTimeoutMs(metaclass=TweakMetaclass):
                    def __call__(self, value: int) -> Callable: ...

                partition_sync_state_req_timeout_ms = PartitionSyncStateReqTimeoutMs()

                class PartitionSyncDataReqTimeoutMs(metaclass=TweakMetaclass):
                    def __call__(self, value: int) -> Callable: ...

                partition_sync_data_req_timeout_ms = PartitionSyncDataReqTimeoutMs()

                class StartupWaitDurationMs(metaclass=TweakMetaclass):
                    def __call__(self, value: int) -> Callable: ...

                startup_wait_duration_ms = StartupWaitDurationMs()

                class FileChunkSize(metaclass=TweakMetaclass):
                    def __call__(self, value: int) -> Callable: ...

                file_chunk_size = FileChunkSize()

                class PartitionSyncEventSize(metaclass=TweakMetaclass):
                    def __call__(self, value: int) -> Callable: ...

                partition_sync_event_size = PartitionSyncEventSize()

                def __call__(
                    self,
                    value: typing.Union[
                        blazingmq.schemas.mqbcfg.StorageSyncConfig, NoneType
                    ],
                ) -> Callable: ...

            sync_config = SyncConfig()

            def __call__(
                self,
                value: typing.Union[blazingmq.schemas.mqbcfg.PartitionConfig, NoneType],
            ) -> Callable: ...

        partition_config = PartitionConfig()

        class MasterAssignment(metaclass=TweakMetaclass):
            def __call__(
                self,
                value: typing.Union[
                    blazingmq.schemas.mqbcfg.MasterAssignmentAlgorithm, NoneType
                ],
            ) -> Callable: ...

        master_assignment = MasterAssignment()

        class Elector(metaclass=TweakMetaclass):
            class InitialWaitTimeoutMs(metaclass=TweakMetaclass):
                def __call__(self, value: int) -> Callable: ...

            initial_wait_timeout_ms = InitialWaitTimeoutMs()

            class MaxRandomWaitTimeoutMs(metaclass=TweakMetaclass):
                def __call__(self, value: int) -> Callable: ...

            max_random_wait_timeout_ms = MaxRandomWaitTimeoutMs()

            class ScoutingResultTimeoutMs(metaclass=TweakMetaclass):
                def __call__(self, value: int) -> Callable: ...

            scouting_result_timeout_ms = ScoutingResultTimeoutMs()

            class ElectionResultTimeoutMs(metaclass=TweakMetaclass):
                def __call__(self, value: int) -> Callable: ...

            election_result_timeout_ms = ElectionResultTimeoutMs()

            class HeartbeatBroadcastPeriodMs(metaclass=TweakMetaclass):
                def __call__(self, value: int) -> Callable: ...

            heartbeat_broadcast_period_ms = HeartbeatBroadcastPeriodMs()

            class HeartbeatCheckPeriodMs(metaclass=TweakMetaclass):
                def __call__(self, value: int) -> Callable: ...

            heartbeat_check_period_ms = HeartbeatCheckPeriodMs()

            class HeartbeatMissCount(metaclass=TweakMetaclass):
                def __call__(self, value: int) -> Callable: ...

            heartbeat_miss_count = HeartbeatMissCount()

            class Quorum(metaclass=TweakMetaclass):
                def __call__(self, value: int) -> Callable: ...

            quorum = Quorum()

            class LeaderSyncDelayMs(metaclass=TweakMetaclass):
                def __call__(self, value: int) -> Callable: ...

            leader_sync_delay_ms = LeaderSyncDelayMs()

            def __call__(
                self,
                value: typing.Union[blazingmq.schemas.mqbcfg.ElectorConfig, NoneType],
            ) -> Callable: ...

        elector = Elector()

        class QueueOperations(metaclass=TweakMetaclass):
            class OpenTimeoutMs(metaclass=TweakMetaclass):
                def __call__(self, value: int) -> Callable: ...

            open_timeout_ms = OpenTimeoutMs()

            class ConfigureTimeoutMs(metaclass=TweakMetaclass):
                def __call__(self, value: int) -> Callable: ...

            configure_timeout_ms = ConfigureTimeoutMs()

            class CloseTimeoutMs(metaclass=TweakMetaclass):
                def __call__(self, value: int) -> Callable: ...

            close_timeout_ms = CloseTimeoutMs()

            class ReopenTimeoutMs(metaclass=TweakMetaclass):
                def __call__(self, value: int) -> Callable: ...

            reopen_timeout_ms = ReopenTimeoutMs()

            class ReopenRetryIntervalMs(metaclass=TweakMetaclass):
                def __call__(self, value: int) -> Callable: ...

            reopen_retry_interval_ms = ReopenRetryIntervalMs()

            class ReopenMaxAttempts(metaclass=TweakMetaclass):
                def __call__(self, value: int) -> Callable: ...

            reopen_max_attempts = ReopenMaxAttempts()

            class AssignmentTimeoutMs(metaclass=TweakMetaclass):
                def __call__(self, value: int) -> Callable: ...

            assignment_timeout_ms = AssignmentTimeoutMs()

            class KeepaliveDurationMs(metaclass=TweakMetaclass):
                def __call__(self, value: int) -> Callable: ...

            keepalive_duration_ms = KeepaliveDurationMs()

            class ConsumptionMonitorPeriodMs(metaclass=TweakMetaclass):
                def __call__(self, value: int) -> Callable: ...

            consumption_monitor_period_ms = ConsumptionMonitorPeriodMs()

            class StopTimeoutMs(metaclass=TweakMetaclass):
                def __call__(self, value: int) -> Callable: ...

            stop_timeout_ms = StopTimeoutMs()

            class ShutdownTimeoutMs(metaclass=TweakMetaclass):
                def __call__(self, value: int) -> Callable: ...

            shutdown_timeout_ms = ShutdownTimeoutMs()

            class AckWindowSize(metaclass=TweakMetaclass):
                def __call__(self, value: int) -> Callable: ...

            ack_window_size = AckWindowSize()

            def __call__(
                self,
                value: typing.Union[
                    blazingmq.schemas.mqbcfg.QueueOperationsConfig, NoneType
                ],
            ) -> Callable: ...

        queue_operations = QueueOperations()

        class ClusterAttributes(metaclass=TweakMetaclass):
            class IsCslmodeEnabled(metaclass=TweakMetaclass):
                def __call__(self, value: bool) -> Callable: ...

            is_cslmode_enabled = IsCslmodeEnabled()

            class IsFsmworkflow(metaclass=TweakMetaclass):
                def __call__(self, value: bool) -> Callable: ...

            is_fsmworkflow = IsFsmworkflow()

            class DoesFsmwriteQlist(metaclass=TweakMetaclass):
                def __call__(self, value: bool) -> Callable: ...

            does_fsmwrite_qlist = DoesFsmwriteQlist()

            def __call__(
                self,
                value: typing.Union[
                    blazingmq.schemas.mqbcfg.ClusterAttributes, NoneType
                ],
            ) -> Callable: ...

        cluster_attributes = ClusterAttributes()

        class ClusterMonitorConfig(metaclass=TweakMetaclass):
            class MaxTimeLeader(metaclass=TweakMetaclass):
                def __call__(self, value: int) -> Callable: ...

            max_time_leader = MaxTimeLeader()

            class MaxTimeMaster(metaclass=TweakMetaclass):
                def __call__(self, value: int) -> Callable: ...

            max_time_master = MaxTimeMaster()

            class MaxTimeNode(metaclass=TweakMetaclass):
                def __call__(self, value: int) -> Callable: ...

            max_time_node = MaxTimeNode()

            class MaxTimeFailover(metaclass=TweakMetaclass):
                def __call__(self, value: int) -> Callable: ...

            max_time_failover = MaxTimeFailover()

            class ThresholdLeader(metaclass=TweakMetaclass):
                def __call__(self, value: int) -> Callable: ...

            threshold_leader = ThresholdLeader()

            class ThresholdMaster(metaclass=TweakMetaclass):
                def __call__(self, value: int) -> Callable: ...

            threshold_master = ThresholdMaster()

            class ThresholdNode(metaclass=TweakMetaclass):
                def __call__(self, value: int) -> Callable: ...

            threshold_node = ThresholdNode()

            class ThresholdFailover(metaclass=TweakMetaclass):
                def __call__(self, value: int) -> Callable: ...

            threshold_failover = ThresholdFailover()

            def __call__(
                self,
                value: typing.Union[
                    blazingmq.schemas.mqbcfg.ClusterMonitorConfig, NoneType
                ],
            ) -> Callable: ...

        cluster_monitor_config = ClusterMonitorConfig()

        class MessageThrottleConfig(metaclass=TweakMetaclass):
            class LowThreshold(metaclass=TweakMetaclass):
                def __call__(self, value: int) -> Callable: ...

            low_threshold = LowThreshold()

            class HighThreshold(metaclass=TweakMetaclass):
                def __call__(self, value: int) -> Callable: ...

            high_threshold = HighThreshold()

            class LowInterval(metaclass=TweakMetaclass):
                def __call__(self, value: int) -> Callable: ...

            low_interval = LowInterval()

            class HighInterval(metaclass=TweakMetaclass):
                def __call__(self, value: int) -> Callable: ...

            high_interval = HighInterval()

            def __call__(
                self,
                value: typing.Union[
                    blazingmq.schemas.mqbcfg.MessageThrottleConfig, NoneType
                ],
            ) -> Callable: ...

        message_throttle_config = MessageThrottleConfig()

    broker = Broker()
    domain = Domain()
    cluster = Cluster()
