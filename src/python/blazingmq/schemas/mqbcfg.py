# Copyright 2026 Bloomberg Finance L.P.
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

from __future__ import annotations

from dataclasses import dataclass, field
from enum import Enum

__NAMESPACE__ = "http://bloomberg.com/schemas/mqbcfg"


class AllocatorType(Enum):
    NEWDELETE = "NEWDELETE"
    COUNTING = "COUNTING"
    STACKTRACETEST = "STACKTRACETEST"


@dataclass(kw_only=True)
class BmqconfConfig:
    cache_ttlseconds: int = field(
        metadata={
            "name": "cacheTTLSeconds",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )


@dataclass(kw_only=True)
class ClusterAttributes:
    """
    Type representing the attributes specific to a cluster.
    isCSLModeEnabled...............: indicates if CSL is enabled for this
    cluster isFSMWorkflow..................: indicates if CSL FSM workflow
    is enabled for this cluster.

    This flag *must* be false if 'isCSLModeEnabled' is false.
    doesFSMwriteQLIST..............: indicates whether the broker still
    writes to the to-be-deprecated QLIST file when FSM workflow is enabled.
    If above 'isFSMWorkflow' flag is false, this flag is ignored.
    clusterFsmWatchdogTimeoutSec...: timeout duration in seconds for
    Cluster FSM watchdog. Only applies when 'isFSMWorkflow' is true.
    clusterFsmWatchdogNumRetries...: number of retries for Cluster FSM
    watchdog before we give up and terminate the broker. Only applies when
    'isFSMWorkflow' is true. partitionFsmWatchdogTimeoutSec.: timeout
    duration in seconds for Partition FSM watchdog. Only applies when
    'isFSMWorkflow' is true. partitionFsmWatchdogNumRetries.: number of
    retries for Partition FSM watchdog before we give up and terminate the
    broker. Only applies when 'isFSMWorkflow' is true.
    """

    is_cslmode_enabled: bool = field(
        default=False,
        metadata={
            "name": "isCSLModeEnabled",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    is_fsmworkflow: bool = field(
        default=False,
        metadata={
            "name": "isFSMWorkflow",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    does_fsmwrite_qlist: bool = field(
        default=True,
        metadata={
            "name": "doesFSMwriteQLIST",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    cluster_fsm_watchdog_timeout_sec: int = field(
        default=300,
        metadata={
            "name": "clusterFsmWatchdogTimeoutSec",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    cluster_fsm_watchdog_num_retries: int = field(
        default=1,
        metadata={
            "name": "clusterFsmWatchdogNumRetries",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    partition_fsm_watchdog_timeout_sec: int = field(
        default=300,
        metadata={
            "name": "partitionFsmWatchdogTimeoutSec",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    partition_fsm_watchdog_num_retries: int = field(
        default=1,
        metadata={
            "name": "partitionFsmWatchdogNumRetries",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )


@dataclass(kw_only=True)
class ClusterMonitorConfig:
    """
    Type representing the configuration for cluster state monitor.
    maxTimeLeader......: Time (in seconds) before alarming that the
    cluster's leader is not 'active' maxTimeMaster......: Time (in seconds)
    before alarming that a partition's master is not 'active'
    maxTimeNode........: Time (in seconds) before alarming that a node is
    not 'available' maxTimeFailover..: Time (in seconds) before alarming
    that failover hasn't completed thresholdLeader....: Time (in seconds)
    before first notifying observers that cluster's leader is not 'active'.

    This time interval is smaller than 'maxTimeLeader' because observing
    components may attempt to heal the cluster state before an alarm is
    raised. thresholdMaster....: Time (in seconds) before notifying
    observers that a partition's master is not 'active'. This time interval
    is smaller than 'maxTimeMaster' because observing components may
    attempt to heal the cluster state before an alarm is raised.
    thresholdNode......: Time (in seconds) before notifying observers that
    a node is not 'available'. This time interval is smaller than
    'maxTimeNode' because observing components may attempt to heal the
    cluster state before an alarm is raised. thresholdFailover..: Time (in
    seconds) before notifying observers that failover has not completed.
    This time interval is smaller than 'maxTimeFailover' because observing
    components may attempt to fix the issue before an alarm is raised.
    """

    max_time_leader: int = field(
        default=60,
        metadata={
            "name": "maxTimeLeader",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    max_time_master: int = field(
        default=120,
        metadata={
            "name": "maxTimeMaster",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    max_time_node: int = field(
        default=120,
        metadata={
            "name": "maxTimeNode",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    max_time_failover: int = field(
        default=600,
        metadata={
            "name": "maxTimeFailover",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    threshold_leader: int = field(
        default=30,
        metadata={
            "name": "thresholdLeader",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    threshold_master: int = field(
        default=60,
        metadata={
            "name": "thresholdMaster",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    threshold_node: int = field(
        default=60,
        metadata={
            "name": "thresholdNode",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    threshold_failover: int = field(
        default=300,
        metadata={
            "name": "thresholdFailover",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )


@dataclass(kw_only=True)
class Credential:
    """
    Type representing a credential used for authentication.

    This type is used to represent a credential that can be used for
    authentication. It contains an authentication mechanism and an
    identity.
    """

    mechanism: str = field(
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )
    identity: str = field(
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )


@dataclass(kw_only=True)
class Disallow:
    """
    Type representing the disallow anonymous credential configuration.

    This type is used to indicate that anonymous authentication is not
    allowed on the broker. If this is set, the broker will not use the
    anonymous authenticator plugin. Authentication is required and clients
    which cannot or do not authenticate will be rejected.
    """


@dataclass(kw_only=True)
class DispatcherProcessorParameters:
    queue_size: int = field(
        metadata={
            "name": "queueSize",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )
    queue_size_low_watermark: int = field(
        metadata={
            "name": "queueSizeLowWatermark",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )
    queue_size_high_watermark: int = field(
        metadata={
            "name": "queueSizeHighWatermark",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )


@dataclass(kw_only=True)
class ElectorConfig:
    """
    Type representing the configuration for leader election amongst a
    cluster of nodes. initialWaitTimeoutMs.......: initial wait timeout, in
    milliseconds, of a follower for leader heartbeat before initiating
    election, as per the *Raft* Algorithm.

    Note that `initialWaitTimeoutMs` should be larger than
    `maxRandomWaitTimeoutMs` maxRandomWaitTimeoutMs.....: maximum random
    wait timeout, in milliseconds, of a follower for leader heartbeat
    before initiating election, as per the *Raft* Algorithm
    scoutingResultTimeoutMs....: timeout, in milliseconds, of a follower
    for awaiting scouting responses from all nodes after sending scouting
    request. electionResultTimeoutMs....: timeout, in milliseconds, of a
    candidate for awaiting quorum to be reached after proposing election,
    as per the *Raft* Algorithm heartbeatBroadcastPeriodMs.: frequency, in
    milliseconds, in which the leader broadcasts a heartbeat signal, as per
    the *Raft* Algorithm, heartbeatCheckPeriodMs.....: frequency, in
    milliseconds, in which a follower checks for heartbeat signals from the
    leader, as per the *Raft* Algorithm heartbeatMissCount.........: the
    number of missed heartbeat signals required before a follower marks the
    current leader as inactive, as per the *Raft* Algorithm
    quorum.....................: the minimum number of votes required for a
    candidate to transition to the leader. If zero, dynamically set to half
    the number of nodes plus one leaderSyncDelayMs..........: delay, in
    milliseconds, after a leader has been elected before initiating leader
    sync, in order to give a chance to all nodes to come up and declare
    themselves AVAILABLE. Note that this should be done only in case of
    cluster of size &gt; 1.
    """

    initial_wait_timeout_ms: int = field(
        default=8000,
        metadata={
            "name": "initialWaitTimeoutMs",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    max_random_wait_timeout_ms: int = field(
        default=3000,
        metadata={
            "name": "maxRandomWaitTimeoutMs",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    scouting_result_timeout_ms: int = field(
        default=4000,
        metadata={
            "name": "scoutingResultTimeoutMs",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    election_result_timeout_ms: int = field(
        default=4000,
        metadata={
            "name": "electionResultTimeoutMs",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    heartbeat_broadcast_period_ms: int = field(
        default=2000,
        metadata={
            "name": "heartbeatBroadcastPeriodMs",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    heartbeat_check_period_ms: int = field(
        default=1000,
        metadata={
            "name": "heartbeatCheckPeriodMs",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    heartbeat_miss_count: int = field(
        default=10,
        metadata={
            "name": "heartbeatMissCount",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    quorum: int = field(
        default=0,
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    leader_sync_delay_ms: int = field(
        default=80000,
        metadata={
            "name": "leaderSyncDelayMs",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )


class ExportMode(Enum):
    E_PUSH = "E_PUSH"
    E_PULL = "E_PULL"


@dataclass(kw_only=True)
class Heartbeat:
    """
    The following parameters define, for the various connection types,
    after how many missed heartbeats the connection should be proactively
    resetted.

    Note that a value of 0 means that smart-heartbeat is entirely disabled
    for this kind of connection (i.e., it will not periodically emit
    heatbeats in case no traffic is received, and will therefore not
    quickly detect stale remote peer). Each value is in multiple of the
    'NetworkInterfaces/TCPInterfaceConfig/heartIntervalMs'.
    client............: The channel represents a client connected to the
    broker downstreamBroker..: The channel represents a downstream broker
    connected to this broker, i.e. a proxy. upstreamBroker....: The channel
    represents an upstream broker connection from this broker, i.e. a
    cluster proxy connection. clusterPeer.......: The channel represents a
    connection with a peer node in the cluster this broker is part of.
    """

    client: int = field(
        default=0,
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    downstream_broker: int = field(
        default=0,
        metadata={
            "name": "downstreamBroker",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    upstream_broker: int = field(
        default=0,
        metadata={
            "name": "upstreamBroker",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    cluster_peer: int = field(
        default=0,
        metadata={
            "name": "clusterPeer",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )


@dataclass(kw_only=True)
class LogDumpConfig:
    record_buffer_size: int = field(
        default=32768,
        metadata={
            "name": "recordBufferSize",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    recording_level: str = field(
        default="OFF",
        metadata={
            "name": "recordingLevel",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    trigger_level: str = field(
        default="OFF",
        metadata={
            "name": "triggerLevel",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )


class MasterAssignmentAlgorithm(Enum):
    """
    Enumeration of the various algorithm's used for assigning a master to a
    partition: - E_LEADER_IS_MASTER_ALL: the leader is master for all
    partitions - E_LEAST_ASSIGNED: the active node with the least number of
    partitions assigned is used.
    """

    E_LEADER_IS_MASTER_ALL = "E_LEADER_IS_MASTER_ALL"
    E_LEAST_ASSIGNED = "E_LEAST_ASSIGNED"


@dataclass(kw_only=True)
class MessagePropertiesV2:
    """
    This complex type captures information which can be used to tell a
    broker if it should advertise support for message properties v2 format
    (also knownn as extended or 'EX' message properties at some places).

    Additionally, broker can be configured to advertise this feature only
    to those C++ and Java clients which match a certain minimum SDK
    version.
    """

    advertise_v2_support: bool = field(
        default=True,
        metadata={
            "name": "advertiseV2Support",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    min_cpp_sdk_version: int = field(
        default=11207,
        metadata={
            "name": "minCppSdkVersion",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    min_java_sdk_version: int = field(
        default=10,
        metadata={
            "name": "minJavaSdkVersion",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )


@dataclass(kw_only=True)
class MessageThrottleConfig:
    """
    Configuration values for message throttling intervals and thresholds.
    lowInterval...: time in milliseconds. highInterval..: time in
    milliseconds. lowThreshold..: indicates the rda counter value at which
    we start throttlling for time equal to 'lowInterval'. highThreshold.:
    indicates the rda counter value at which we start throttlling for time
    equal to 'highInterval'.

    Note: lowInterval should be less than/equal to highInterval,
    lowThreshold should be less than highThreshold.
    """

    low_threshold: int = field(
        default=2,
        metadata={
            "name": "lowThreshold",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    high_threshold: int = field(
        default=4,
        metadata={
            "name": "highThreshold",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    low_interval: int = field(
        default=1000,
        metadata={
            "name": "lowInterval",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    high_interval: int = field(
        default=3000,
        metadata={
            "name": "highInterval",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )


@dataclass(kw_only=True)
class PluginSettingValue:
    bool_val: None | bool = field(
        default=None,
        metadata={
            "name": "boolVal",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    int_val: None | int = field(
        default=None,
        metadata={
            "name": "intVal",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    long_val: None | int = field(
        default=None,
        metadata={
            "name": "longVal",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    double_val: None | float = field(
        default=None,
        metadata={
            "name": "doubleVal",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    string_val: None | str = field(
        default=None,
        metadata={
            "name": "stringVal",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )


@dataclass(kw_only=True)
class Plugins:
    libraries: list[str] = field(
        default_factory=list,
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "min_occurs": 1,
        },
    )
    enabled: list[str] = field(
        default_factory=list,
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "min_occurs": 1,
        },
    )


@dataclass(kw_only=True)
class QueueOperationsConfig:
    """
    Type representing the configuration for queue operations on a cluster.
    openTimeoutMs..............: timeout, in milliseconds, to use when
    opening a queue.

    An open request requires some co-ordination among the nodes (queue
    assignment request/response, followed by queue open request/response,
    etc) configureTimeoutMs.........: timeout, in milliseconds, to use for
    a configure queue request. Note that `configureTimeoutMs` must be less
    than or equal to `closeTimeoutMs` to prevent out-of-order processing of
    closeQueue (e.g. closeQueue sent after configureQueue but timeout
    response processed first for the closeQueue)
    closeTimeoutMs.............: timeout, in milliseconds, to use for a
    close queue request reopenTimeoutMs............: timeout, in
    milliseconds, to use when sending a reopen-queue request. Ideally, we
    should use same value as `openTimeoutMs`, but we are using a very large
    value as a workaround: during network outages, a proxy or a replica may
    failover to a new upstream node, which itself may be out of sync or not
    yet ready. Eventually, the reopen-queue requests sent during failover
    may timeout, and will never be retried, leading to a 'permanent'
    failure (client consumer app stops receiving messages; PUT messages
    from client producer app starts getting NAK'd or buffered). Using such
    a large timeout value helps in a situation when network outages or its
    after-effects are fixed after a few hours).
    reopenRetryIntervalMs......: duration, in milliseconds, after which a
    retry attempt should be made to reopen the queue
    reopenMaxAttempts..........: maximum number of attempts to reopen a
    queue when restoring the state in a proxy upon getting a new active
    node notification assignmentTimeoutMs........: timeout, in
    milliseconds, to use for a queue assignment request
    keepaliveDurationMs........: duration, in milliseconds, to keep a queue
    alive after it has met the criteria for deletion
    consumptionMonitorPeriodMs.: frequency, in milliseconds, in which the
    consumption monitor checks queue consumption statistics on the cluster
    stopTimeoutMs..............: timeout, in milliseconds, to use in
    StopRequest between deconfiguring and closing each affected queue. This
    is primarily to give a chance for pending PUSH mesages to be CONFIRMed.
    shutdownTimeoutMs..........: timeout, in milliseconds, to use when node
    stops for shutdown or maintenance mode. This timeout should be greater
    than the 'stopTimeoutMs'. This is to handle misbehaving downstream
    which may not reply to stopRequest (otherwise, this timeout is not
    expected to be reached). ackWindowSize..............: number of PUTs
    without ACK requested after which we request an ACK. This is to remove
    pending broadcast PUTs.
    """

    open_timeout_ms: int = field(
        default=300000,
        metadata={
            "name": "openTimeoutMs",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    configure_timeout_ms: int = field(
        default=300000,
        metadata={
            "name": "configureTimeoutMs",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    close_timeout_ms: int = field(
        default=300000,
        metadata={
            "name": "closeTimeoutMs",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    reopen_timeout_ms: int = field(
        default=43200000,
        metadata={
            "name": "reopenTimeoutMs",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    reopen_retry_interval_ms: int = field(
        default=5000,
        metadata={
            "name": "reopenRetryIntervalMs",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    reopen_max_attempts: int = field(
        default=10,
        metadata={
            "name": "reopenMaxAttempts",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    assignment_timeout_ms: int = field(
        default=15000,
        metadata={
            "name": "assignmentTimeoutMs",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    keepalive_duration_ms: int = field(
        default=1800000,
        metadata={
            "name": "keepaliveDurationMs",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    consumption_monitor_period_ms: int = field(
        default=30000,
        metadata={
            "name": "consumptionMonitorPeriodMs",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    stop_timeout_ms: int = field(
        default=10000,
        metadata={
            "name": "stopTimeoutMs",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    shutdown_timeout_ms: int = field(
        default=20000,
        metadata={
            "name": "shutdownTimeoutMs",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    ack_window_size: int = field(
        default=500,
        metadata={
            "name": "ackWindowSize",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )


@dataclass(kw_only=True)
class ResolvedDomain:
    """
    Top level type representing the information retrieved when resolving a
    domain. resolvedName.: Resolved name of the domain clusterName..: Name
    of the cluster where this domain exists.
    """

    resolved_name: str = field(
        metadata={
            "name": "resolvedName",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )
    cluster_name: str = field(
        metadata={
            "name": "clusterName",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )


@dataclass(kw_only=True)
class StatsPrinterConfig:
    print_interval: int = field(
        default=60,
        metadata={
            "name": "printInterval",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    file: str = field(
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )
    max_age_days: int = field(
        metadata={
            "name": "maxAgeDays",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )
    rotate_bytes: int = field(
        default=268435456,
        metadata={
            "name": "rotateBytes",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    rotate_days: int = field(
        default=1,
        metadata={
            "name": "rotateDays",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )


@dataclass(kw_only=True)
class StorageSyncConfig:
    """
    Type representing the configuration for storage synchronization and
    recovery of a cluster. startupRecoveryMaxDurationMs...: maximum amount
    of time, in milliseconds, in which recovery for a partition at node
    startup must complete.

    This interval captures the time taken to receive storage-sync response,
    retry attempts for failed storage-sync request, as well as the time
    taken by the peer node to send partition files to the starting (i.e.
    requester) node maxAttemptsStorageSync.........: maximum number of
    attempts that a node makes for storage-sync requests when it comes up
    (this value includes the 1st attempt) storageSyncReqTimeoutMs........:
    timeout, in milliseconds, for the storage-sync request. A bigger value
    is recommended because peer node could be busy serving storage-sync
    request for other partition(s) assigned to the same
    partition-dispatcher thread, etc. This timeout does *not* capture the
    time taken by the peer to send partition files
    masterSyncMaxDurationMs........: maximum amount of time, in
    milliseconds, in which master sync for a partition must complete. This
    interval includes the time taken by replica node (the one with advanced
    view of the partition) to send the file chunks, as well as time taken
    to receive partition-sync state and data responses
    partitionSyncStateReqTimeoutMs.: timeout, in milliseconds, for
    partition-sync-state-query request. This request is sent by a new
    master node to all replica nodes to query their view of the partition
    partitionSyncDataReqTimeoutMs..: timeout, in milliseconds, for
    partition-sync-data-query request. This request is sent by a new master
    node to a replica node (which has an advanced view of the partition) to
    initiate partition sync. This duration does *not* capture the amount of
    time which replica might take to send the partition file
    startupWaitDurationMs..........: duration, in milliseconds, for which
    recovery manager waits for a sync point for a partition. If no sync
    point is received from a peer for a partition during this time, it is
    assumed that there is no master for that partition, and recovery
    manager randomly picks up a node from all the available ones with send
    a sync request. If no peers are available at this time, it is assumed
    that entire cluster is coming up together, and proceeds with local
    recovery for that partition. Note that this value should be less than
    the duration for which a node waits if its elected a leader and there
    are no AVAILABLE nodes. This is important so that if all nodes in the
    cluster are starting, they have a chance to wait for
    'startupWaitDurationMs' milliseconds, find out that none of the
    partitions have any master, go ahead with local recovery and declare
    themselves as AVAILABLE. This will give the new leader node a chance to
    make each node a master for a given partition. Moreover, this value
    should be greater than the duration for which a peer waits before
    attempting to reconnect to the node in the cluster, so that peer has a
    chance to connect to this node, get notified (via ClusterObserver), and
    send sync point if its a master for any partition
    fileChunkSize..................: chunk size, in bytes, to send in one
    go to the peer when serving a storage sync request from it
    partitionSyncEventSize.........: maximum size, in bytes, of
    bmqp::EventType::PARTITION_SYNC before we send it to the peer.
    """

    startup_recovery_max_duration_ms: int = field(
        default=1200000,
        metadata={
            "name": "startupRecoveryMaxDurationMs",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    max_attempts_storage_sync: int = field(
        default=3,
        metadata={
            "name": "maxAttemptsStorageSync",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    storage_sync_req_timeout_ms: int = field(
        default=300000,
        metadata={
            "name": "storageSyncReqTimeoutMs",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    master_sync_max_duration_ms: int = field(
        default=600000,
        metadata={
            "name": "masterSyncMaxDurationMs",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    partition_sync_state_req_timeout_ms: int = field(
        default=120000,
        metadata={
            "name": "partitionSyncStateReqTimeoutMs",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    partition_sync_data_req_timeout_ms: int = field(
        default=120000,
        metadata={
            "name": "partitionSyncDataReqTimeoutMs",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    startup_wait_duration_ms: int = field(
        default=60000,
        metadata={
            "name": "startupWaitDurationMs",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    file_chunk_size: int = field(
        default=4194304,
        metadata={
            "name": "fileChunkSize",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    partition_sync_event_size: int = field(
        default=4194304,
        metadata={
            "name": "partitionSyncEventSize",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )


@dataclass(kw_only=True)
class SyslogConfig:
    enabled: bool = field(
        default=False,
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    app_name: str = field(
        metadata={
            "name": "appName",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )
    log_format: str = field(
        metadata={
            "name": "logFormat",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )
    verbosity: str = field(
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )


@dataclass(kw_only=True)
class TcpClusterNodeConnection:
    """
    Configuration of a TCP based cluster node connectivity. endpoint.:
    endpoint URI of the node address.
    """

    endpoint: str = field(
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )


@dataclass(kw_only=True)
class TcpInterfaceListener:
    """
    This type describes the information needed for the broker to open a TCP
    listener. name.................: A name to associate this listener to.
    address..............: The IPv4 address this listener will accept
    connections on. port.................: The port this listener will
    accept connections on. tls..................: Use TLS on this
    interface.
    """

    name: str = field(
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )
    address: str = field(
        default="0.0.0.0",
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    port: int = field(
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )
    tls: bool = field(
        default=False,
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )


@dataclass(kw_only=True)
class TlsConfig:
    """
    certificateAuthority.: A path to the FILE, containing concatenation of
    known certificates the server can use to reference as its certificate
    store. certificate..........: A path to the FILE, containing the
    certificate the broker will use to identify itself to other clients.
    key..................: A path to the FILE, containing the private key
    that the broker uses to read the certificate. versions.............: A
    string with a comma-separated list of supported protocol versions.
    """

    certificate_authority: str = field(
        metadata={
            "name": "certificateAuthority",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )
    certificate: str = field(
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )
    key: str = field(
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )
    versions: str = field(
        default="TLSv1.3",
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )


@dataclass(kw_only=True)
class VirtualClusterInformation:
    """
    Type representing the information about the current node with regards
    to virtual cluster. name.............: name of the cluster
    selfNodeId.......: id of the current node in that virtual cluster.
    """

    name: str = field(
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )
    self_node_id: int = field(
        metadata={
            "name": "selfNodeId",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )


@dataclass(kw_only=True)
class AnonymousCredential:
    """
    Type representing the anonymous credential configuration. disallow...:
    If set, the anonymous credential is not allowed.

    Authentication is required and clients which cannot or do not
    authenticate will be rejected. credential.: If set, the credential is
    used for anonymous authentication in case the client does not support
    authentication or has not been configured to authenticate.
    """

    disallow: None | Disallow = field(
        default=None,
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    credential: None | Credential = field(
        default=None,
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )


@dataclass(kw_only=True)
class ClusterNodeConnection:
    """
    Choice of all the various transport mode available to establish
    connectivity with a node. tcp.: TCP connectivity.
    """

    tcp: None | TcpClusterNodeConnection = field(
        default=None,
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )


@dataclass(kw_only=True)
class DispatcherProcessorConfig:
    num_processors: int = field(
        metadata={
            "name": "numProcessors",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )
    processor_config: DispatcherProcessorParameters = field(
        metadata={
            "name": "processorConfig",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )


@dataclass(kw_only=True)
class LogController:
    file_name: str = field(
        metadata={
            "name": "fileName",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )
    file_max_age_days: int = field(
        metadata={
            "name": "fileMaxAgeDays",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )
    rotation_bytes: int = field(
        metadata={
            "name": "rotationBytes",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )
    logfile_format: str = field(
        metadata={
            "name": "logfileFormat",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )
    console_format: str = field(
        metadata={
            "name": "consoleFormat",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )
    logging_verbosity: str = field(
        metadata={
            "name": "loggingVerbosity",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )
    bsls_log_severity_threshold: str = field(
        default="ERROR",
        metadata={
            "name": "bslsLogSeverityThreshold",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    console_severity_threshold: str = field(
        metadata={
            "name": "consoleSeverityThreshold",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )
    categories: list[str] = field(
        default_factory=list,
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "min_occurs": 1,
        },
    )
    syslog: SyslogConfig = field(
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )
    log_dump: LogDumpConfig = field(
        metadata={
            "name": "logDump",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )


@dataclass(kw_only=True)
class PartitionConfig:
    """
    Type representing the configuration for the storage layer of a cluster.
    numPartitions........: number of partitions at each node in the cluster
    location.............: location of active files for a partition
    archiveLocation......: location of archive files for a partition
    maxDataFileSize......: maximum size of partitions' data file
    maxJournalFileSize...: maximum size of partitions' journal file
    maxQlistFileSize.....: maximum size of partitions' qlist file
    maxCSLFileSize.......: maximum size of partitions' CSL file
    preallocate..........: flag to indicate whether files should be
    preallocated on disk maxArchivedFileSets..: maximum number of archived
    file sets per partition to keep prefaultPages........: flag to indicate
    whether to populate (prefault) page tables for a mapping.
    flushAtShutdown......: flag to indicate whether broker should flush
    storage files to disk at shutdown syncConfig...........: configuration
    for storage synchronization and recovery.
    """

    num_partitions: int = field(
        metadata={
            "name": "numPartitions",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )
    location: str = field(
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )
    archive_location: str = field(
        metadata={
            "name": "archiveLocation",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )
    max_data_file_size: int = field(
        metadata={
            "name": "maxDataFileSize",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )
    max_journal_file_size: int = field(
        metadata={
            "name": "maxJournalFileSize",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )
    max_qlist_file_size: int = field(
        metadata={
            "name": "maxQlistFileSize",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )
    max_cslfile_size: int = field(
        default=67108864,
        metadata={
            "name": "maxCSLFileSize",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    preallocate: bool = field(
        default=False,
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    max_archived_file_sets: int = field(
        metadata={
            "name": "maxArchivedFileSets",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )
    prefault_pages: bool = field(
        default=False,
        metadata={
            "name": "prefaultPages",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    flush_at_shutdown: bool = field(
        default=True,
        metadata={
            "name": "flushAtShutdown",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    sync_config: StorageSyncConfig = field(
        metadata={
            "name": "syncConfig",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )


@dataclass(kw_only=True)
class PluginSettingKeyValue:
    """
    The key-value pair used for plugin settings. key...: setting key/name
    value.: setting value.
    """

    key: str = field(
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )
    value: PluginSettingValue = field(
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )


@dataclass(kw_only=True)
class StatPluginConfigPrometheus:
    mode: ExportMode = field(
        default=ExportMode.E_PULL,
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    host: str = field(
        default="localhost",
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    port: int = field(
        default=8080,
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )


@dataclass(kw_only=True)
class TcpInterfaceConfig:
    """
    name.................: The name of the TCP session manager.
    port.................: (Deprecated) The port to receive connections.
    lowWatermark.........: highWatermark........: Watermarks used for
    channels with a client or proxy. nodeLowWatermark.....:
    nodeHighWatermark....: Reduced watermarks for communication between
    cluster nodes where BlazingMQ maintains its own cache.
    heartbeatIntervalMs..: How often (in milliseconds) to check if the
    channel received data, and emit heartbeat. 0 to globally disable.
    listeners: A list of listener interfaces to receive TCP connections
    from.

    When non-empty this option overrides the listener specified by port.
    """

    name: str = field(
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )
    port: int = field(
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )
    io_threads: int = field(
        metadata={
            "name": "ioThreads",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )
    max_connections: int = field(
        default=10000,
        metadata={
            "name": "maxConnections",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    low_watermark: int = field(
        metadata={
            "name": "lowWatermark",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )
    high_watermark: int = field(
        metadata={
            "name": "highWatermark",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )
    node_low_watermark: int = field(
        default=1024,
        metadata={
            "name": "nodeLowWatermark",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    node_high_watermark: int = field(
        default=2048,
        metadata={
            "name": "nodeHighWatermark",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    heartbeat_interval_ms: int = field(
        default=3000,
        metadata={
            "name": "heartbeatIntervalMs",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    listeners: list[TcpInterfaceListener] = field(
        default_factory=list,
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )


@dataclass(kw_only=True)
class AuthenticatorPluginConfig:
    """
    The configuration for an authenticator plugin. name.....: The name of
    the authenticator plugin. settings.: Plugin-specific settings.
    """

    name: str = field(
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )
    settings: list[PluginSettingKeyValue] = field(
        default_factory=list,
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )


@dataclass(kw_only=True)
class ClusterNode:
    """
    Type representing the configuration of a node in a cluster.
    id.........: the unique ID of that node in the cluster; must be a &gt;
    0 value name.......: name of this node datacenter.: the datacenter of
    that node transport..: the transport configuration for establishing
    connectivity with the node.
    """

    id: int = field(
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )
    name: str = field(
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )
    data_center: str = field(
        metadata={
            "name": "dataCenter",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )
    transport: ClusterNodeConnection = field(
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )


@dataclass(kw_only=True)
class DispatcherConfig:
    sessions: DispatcherProcessorConfig = field(
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )
    queues: DispatcherProcessorConfig = field(
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )
    clusters: DispatcherProcessorConfig = field(
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )
    alarm_timeout_ms: int = field(
        default=180000,
        metadata={
            "name": "alarmTimeoutMs",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    warning_timeout_ms: int = field(
        default=10000,
        metadata={
            "name": "warningTimeoutMs",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )


@dataclass(kw_only=True)
class NetworkInterfaces:
    heartbeats: Heartbeat = field(
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )
    tcp_interface: None | TcpInterfaceConfig = field(
        default=None,
        metadata={
            "name": "tcpInterface",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )


@dataclass(kw_only=True)
class StatPluginConfig:
    name: str = field(
        default="",
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    queue_size: int = field(
        default=10000,
        metadata={
            "name": "queueSize",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    queue_high_watermark: int = field(
        default=5000,
        metadata={
            "name": "queueHighWatermark",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    queue_low_watermark: int = field(
        default=1000,
        metadata={
            "name": "queueLowWatermark",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    publish_interval: int = field(
        default=30,
        metadata={
            "name": "publishInterval",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    namespace_prefix: str = field(
        default="",
        metadata={
            "name": "namespacePrefix",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    hosts: list[str] = field(
        default_factory=list,
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "min_occurs": 1,
        },
    )
    instance_id: str = field(
        default="",
        metadata={
            "name": "instanceId",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    prometheus_specific: None | StatPluginConfigPrometheus = field(
        default=None,
        metadata={
            "name": "prometheusSpecific",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )


@dataclass(kw_only=True)
class TaskConfig:
    allocator_type: AllocatorType = field(
        metadata={
            "name": "allocatorType",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )
    allocation_limit: int = field(
        metadata={
            "name": "allocationLimit",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )
    log_controller: LogController = field(
        metadata={
            "name": "logController",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )


@dataclass(kw_only=True)
class AuthenticatorConfig:
    """
    Top level type for the broker's authentication configurations.
    authenticators...........: Configuration entries for authenticator
    plugins (built-in or external).

    Each entry defines settings for a specific plugin. All plugins must
    have unique authentication mechanisms. anonymousCredential.: Controls
    anonymous authentication behavior. When specified, the broker uses the
    provided credential with a matching plugin from `authenticators`. When
    omitted, the broker defaults to AnonAuthenticator and always passes for
    anonymous authentication. minThreads..............: Minimum number of
    threads in the authentication thread pool. maxThreads..............:
    Maximum number of threads in the authentication thread pool.
    """

    authenticators: list[AuthenticatorPluginConfig] = field(
        default_factory=list,
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    anonymous_credential: None | AnonymousCredential = field(
        default=None,
        metadata={
            "name": "anonymousCredential",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    min_threads: int = field(
        default=1,
        metadata={
            "name": "minThreads",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    max_threads: int = field(
        default=8,
        metadata={
            "name": "maxThreads",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )


@dataclass(kw_only=True)
class ClusterDefinition:
    """
    Type representing the configuration for a cluster.
    name..................: name of the cluster nodes.................:
    list of nodes in the cluster partitionConfig.......: configuration for
    the storage masterAssignment......: algorithm to use for partition's
    master assignment elector...............: configuration for leader
    election amongst the nodes queueOperations.......: configuration for
    queue operations on the cluster clusterAttributes.....: attributes
    specific to this cluster clusterMonitorConfig..: configuration for
    cluster state monitor messageThrottleConfig.: configuration for message
    throttling intervals and thresholds.
    """

    name: str = field(
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )
    nodes: list[ClusterNode] = field(
        default_factory=list,
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "min_occurs": 1,
        },
    )
    partition_config: PartitionConfig = field(
        metadata={
            "name": "partitionConfig",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )
    master_assignment: MasterAssignmentAlgorithm = field(
        metadata={
            "name": "masterAssignment",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )
    elector: ElectorConfig = field(
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )
    queue_operations: QueueOperationsConfig = field(
        metadata={
            "name": "queueOperations",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )
    cluster_attributes: ClusterAttributes = field(
        metadata={
            "name": "clusterAttributes",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )
    cluster_monitor_config: ClusterMonitorConfig = field(
        metadata={
            "name": "clusterMonitorConfig",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )
    message_throttle_config: MessageThrottleConfig = field(
        metadata={
            "name": "messageThrottleConfig",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )


@dataclass(kw_only=True)
class ClusterProxyDefinition:
    """
    Type representing the configuration for a cluster proxy.
    name..................: name of the cluster nodes.................:
    list of nodes in the cluster queueOperations.......: configuration for
    queue operations with the cluster clusterMonitorConfig..: configuration
    for cluster state monitor messageThrottleConfig.: configuration for
    message throttling intervals and thresholds.
    """

    name: str = field(
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )
    nodes: list[ClusterNode] = field(
        default_factory=list,
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "min_occurs": 1,
        },
    )
    queue_operations: QueueOperationsConfig = field(
        metadata={
            "name": "queueOperations",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )
    cluster_monitor_config: ClusterMonitorConfig = field(
        metadata={
            "name": "clusterMonitorConfig",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )
    message_throttle_config: MessageThrottleConfig = field(
        metadata={
            "name": "messageThrottleConfig",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )


@dataclass(kw_only=True)
class StatsConfig:
    snapshot_interval: int = field(
        default=1,
        metadata={
            "name": "snapshotInterval",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    plugins: list[StatPluginConfig] = field(
        default_factory=list,
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "min_occurs": 1,
        },
    )
    printer: StatsPrinterConfig = field(
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )


@dataclass(kw_only=True)
class AppConfig:
    """
    Top level type for the broker's configuration. brokerInstanceName...:
    name of the broker instance brokerVersion........: version of the
    broker configVersion........: version of the bmqbrkr.cfg config
    etcDir...............: directory containing the json config files
    hostName.............: name of the current host hostTags.............:
    tags of the current host hostDataCenter.......: datacenter the current
    host resides in logsObserverMaxSize..: maximum number of log records to
    keep latencyMonitorDomain.: common prefix of all latemon domains
    dispatcherConfig.....: configuration for the dispatcher
    stats................: configuration for the stats
    networkInterfaces....: configuration for the network interfaces
    bmqconfConfig........: configuration for bmqconf plugins..............:
    configuration for the plugins msgPropertiesSupport.: information about
    if/how to advertise support for v2 message properties
    configureStream......: send new ConfigureStream instead of old
    ConfigureQueue advertiseSubscriptions.: temporarily control use of
    ConfigureStream in SDK routeCommandTimeoutMs: maximum amount of time to
    wait for a routed command's response authentication.......:
    configuration for authentication tlsConfig............: optional
    configuration for TLS.
    """

    broker_instance_name: str = field(
        metadata={
            "name": "brokerInstanceName",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )
    broker_version: int = field(
        metadata={
            "name": "brokerVersion",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )
    config_version: int = field(
        metadata={
            "name": "configVersion",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )
    etc_dir: str = field(
        metadata={
            "name": "etcDir",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )
    host_name: str = field(
        metadata={
            "name": "hostName",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )
    host_tags: str = field(
        metadata={
            "name": "hostTags",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )
    host_data_center: str = field(
        metadata={
            "name": "hostDataCenter",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )
    logs_observer_max_size: int = field(
        metadata={
            "name": "logsObserverMaxSize",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )
    latency_monitor_domain: str = field(
        default="bmq.sys.latemon.latency",
        metadata={
            "name": "latencyMonitorDomain",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    dispatcher_config: DispatcherConfig = field(
        metadata={
            "name": "dispatcherConfig",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )
    stats: StatsConfig = field(
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )
    network_interfaces: NetworkInterfaces = field(
        metadata={
            "name": "networkInterfaces",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )
    bmqconf_config: BmqconfConfig = field(
        metadata={
            "name": "bmqconfConfig",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )
    plugins: Plugins = field(
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )
    message_properties_v2: MessagePropertiesV2 = field(
        metadata={
            "name": "messagePropertiesV2",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )
    configure_stream: bool = field(
        default=False,
        metadata={
            "name": "configureStream",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    advertise_subscriptions: bool = field(
        default=False,
        metadata={
            "name": "advertiseSubscriptions",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    route_command_timeout_ms: int = field(
        default=3000,
        metadata={
            "name": "routeCommandTimeoutMs",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )
    authentication: AuthenticatorConfig = field(
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )
    tls_config: None | TlsConfig = field(
        default=None,
        metadata={
            "name": "tlsConfig",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )


@dataclass(kw_only=True)
class ClustersDefinition:
    """
    Top level type representing the configuration for all clusters.
    myClusters.................: definition of the clusters the current
    machine is part of (if any); empty means this broker does not belong to
    any cluster myVirtualClusters..........: information about all the
    virtual clusters the current machine is considered to belong to (if
    any) proxyClusters..............: array of cluster proxy definition.
    """

    my_clusters: list[ClusterDefinition] = field(
        default_factory=list,
        metadata={
            "name": "myClusters",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "min_occurs": 1,
        },
    )
    my_virtual_clusters: list[VirtualClusterInformation] = field(
        default_factory=list,
        metadata={
            "name": "myVirtualClusters",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "min_occurs": 1,
        },
    )
    proxy_clusters: list[ClusterProxyDefinition] = field(
        default_factory=list,
        metadata={
            "name": "proxyClusters",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "min_occurs": 1,
        },
    )


@dataclass(kw_only=True)
class Configuration:
    task_config: TaskConfig = field(
        metadata={
            "name": "taskConfig",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )
    app_config: AppConfig = field(
        metadata={
            "name": "appConfig",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        }
    )
