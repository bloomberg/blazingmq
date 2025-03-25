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

from dataclasses import dataclass, field
from enum import Enum
from typing import List, Optional

__NAMESPACE__ = "http://bloomberg.com/schemas/mqbcfg"


class AllocatorType(Enum):
    NEWDELETE = "NEWDELETE"
    COUNTING = "COUNTING"
    STACKTRACETEST = "STACKTRACETEST"


@dataclass
class BmqconfConfig:
    cache_ttlseconds: Optional[int] = field(
        default=None,
        metadata={
            "name": "cacheTTLSeconds",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )


@dataclass
class ClusterAttributes:
    """Type representing the attributes specific to a cluster.

    isCSLModeEnabled.: indicates if CSL is enabled for this cluster
    isFSMWorkflow....: indicates if CSL FSM workflow is enabled for this
    cluster.  This flag *must* be false if
    'isCSLModeEnabled' is false.
    """

    is_cslmode_enabled: bool = field(
        default=False,
        metadata={
            "name": "isCSLModeEnabled",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    is_fsmworkflow: bool = field(
        default=False,
        metadata={
            "name": "isFSMWorkflow",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )


@dataclass
class ClusterMonitorConfig:
    """Type representing the configuration for cluster state monitor.

    maxTimeLeader......:
    Time (in seconds) before alarming that the cluster's leader is not
    'active'
    maxTimeMaster......:
    Time (in seconds) before alarming that a partition's master is not
    'active'
    maxTimeNode........:
    Time (in seconds) before alarming that a node is not 'available'
    maxTimeFailover..:
    Time (in seconds) before alarming that failover hasn't completed
    thresholdLeader....:
    Time (in seconds) before first notifying observers that cluster's
    leader is not 'active'.  This time interval is smaller than
    'maxTimeLeader' because observing components may attempt to heal
    the cluster state before an alarm is raised.
    thresholdMaster....:
    Time (in seconds) before notifying observers that a partition's
    master is not 'active'.  This time interval is smaller than
    'maxTimeMaster' because observing components may attempt to heal
    the cluster state before an alarm is raised.
    thresholdNode......:
    Time (in seconds) before notifying observers that a node is not
    'available'.  This time interval is smaller than 'maxTimeNode'
    because observing components may attempt to heal the cluster state
    before an alarm is raised.
    thresholdFailover..:
    Time (in seconds) before notifying observers that failover has not
    completed.  This time interval is smaller than 'maxTimeFailover'
    because observing components may attempt to fix the issue before an
    alarm is raised.
    """

    max_time_leader: int = field(
        default=60,
        metadata={
            "name": "maxTimeLeader",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    max_time_master: int = field(
        default=120,
        metadata={
            "name": "maxTimeMaster",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    max_time_node: int = field(
        default=120,
        metadata={
            "name": "maxTimeNode",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    max_time_failover: int = field(
        default=600,
        metadata={
            "name": "maxTimeFailover",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    threshold_leader: int = field(
        default=30,
        metadata={
            "name": "thresholdLeader",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    threshold_master: int = field(
        default=60,
        metadata={
            "name": "thresholdMaster",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    threshold_node: int = field(
        default=60,
        metadata={
            "name": "thresholdNode",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    threshold_failover: int = field(
        default=300,
        metadata={
            "name": "thresholdFailover",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )


@dataclass
class DispatcherProcessorParameters:
    queue_size: Optional[int] = field(
        default=None,
        metadata={
            "name": "queueSize",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    queue_size_low_watermark: Optional[int] = field(
        default=None,
        metadata={
            "name": "queueSizeLowWatermark",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    queue_size_high_watermark: Optional[int] = field(
        default=None,
        metadata={
            "name": "queueSizeHighWatermark",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )


@dataclass
class ElectorConfig:
    """Type representing the configuration for leader election amongst a cluster of
    nodes.

    initialWaitTimeoutMs.......:
    initial wait timeout, in milliseconds, of a follower for leader
    heartbeat before initiating election, as per the *Raft* Algorithm.
    Note that `initialWaitTimeoutMs` should be larger than
    `maxRandomWaitTimeoutMs`
    maxRandomWaitTimeoutMs.....:
    maximum random wait timeout, in milliseconds, of a follower for
    leader heartbeat before initiating election, as per the *Raft*
    Algorithm
    scoutingResultTimeoutMs....:
    timeout, in milliseconds, of a follower for awaiting scouting
    responses from all nodes after sending scouting request.
    electionResultTimeoutMs....:
    timeout, in milliseconds, of a candidate for awaiting quorum to be
    reached after proposing election, as per the *Raft* Algorithm
    heartbeatBroadcastPeriodMs.:
    frequency, in milliseconds, in which the leader broadcasts a
    heartbeat signal, as per the *Raft* Algorithm,
    heartbeatCheckPeriodMs.....:
    frequency, in milliseconds, in which a follower checks for
    heartbeat signals from the leader, as per the *Raft* Algorithm
    heartbeatMissCount.........:
    the number of missed heartbeat signals required before a follower
    marks the current leader as inactive, as per the *Raft* Algorithm
    quorum.....................:
    the minimum number of votes required for a candidate to transition
    to the leader. If zero, dynamically set to half the number of nodes
    plus one
    leaderSyncDelayMs..........:
    delay, in milliseconds, after a leader has been elected before
    initiating leader sync, in order to give a chance to all nodes to
    come up and declare themselves AVAILABLE.  Note that this should be
    done only in case of cluster of size &gt; 1
    """

    initial_wait_timeout_ms: int = field(
        default=8000,
        metadata={
            "name": "initialWaitTimeoutMs",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    max_random_wait_timeout_ms: int = field(
        default=3000,
        metadata={
            "name": "maxRandomWaitTimeoutMs",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    scouting_result_timeout_ms: int = field(
        default=4000,
        metadata={
            "name": "scoutingResultTimeoutMs",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    election_result_timeout_ms: int = field(
        default=4000,
        metadata={
            "name": "electionResultTimeoutMs",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    heartbeat_broadcast_period_ms: int = field(
        default=2000,
        metadata={
            "name": "heartbeatBroadcastPeriodMs",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    heartbeat_check_period_ms: int = field(
        default=1000,
        metadata={
            "name": "heartbeatCheckPeriodMs",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    heartbeat_miss_count: int = field(
        default=10,
        metadata={
            "name": "heartbeatMissCount",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    quorum: int = field(
        default=0,
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    leader_sync_delay_ms: int = field(
        default=80000,
        metadata={
            "name": "leaderSyncDelayMs",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )


class ExportMode(Enum):
    E_PUSH = "E_PUSH"
    E_PULL = "E_PULL"


@dataclass
class Heartbeat:
    """The following parameters define, for the various connection types, after how
    many missed heartbeats the connection should be proactively resetted.

    Note that a value of 0 means that smart-heartbeat is
    entirely disabled for this kind of connection (i.e., it will not
    periodically emit heatbeats in case no traffic is received, and will
    therefore not quickly detect stale remote peer).  Each value is in
    multiple of the 'NetworkInterfaces/TCPInterfaceConfig/heartIntervalMs'.
    client............:
    The channel represents a client connected to the broker
    downstreamBroker..:
    The channel represents a downstream broker connected to this
    broker, i.e. a proxy.
    upstreamBroker....:
    The channel represents an upstream broker connection from this
    broker, i.e. a cluster proxy connection.
    clusterPeer.......:
    The channel represents a connection with a peer node in the cluster
    this broker is part of.
    """

    client: int = field(
        default=0,
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    downstream_broker: int = field(
        default=0,
        metadata={
            "name": "downstreamBroker",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    upstream_broker: int = field(
        default=0,
        metadata={
            "name": "upstreamBroker",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    cluster_peer: int = field(
        default=0,
        metadata={
            "name": "clusterPeer",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )


@dataclass
class LogDumpConfig:
    record_buffer_size: int = field(
        default=32768,
        metadata={
            "name": "recordBufferSize",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    recording_level: str = field(
        default="OFF",
        metadata={
            "name": "recordingLevel",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    trigger_level: str = field(
        default="OFF",
        metadata={
            "name": "triggerLevel",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )


class MasterAssignmentAlgorithm(Enum):
    """Enumeration of the various algorithm's used for assigning a master to.

    a partition:
    - E_LEADER_IS_MASTER_ALL: the leader is master for all partitions
    - E_LEAST_ASSIGNED:       the active node with the least number of
    partitions assigned is used
    """

    E_LEADER_IS_MASTER_ALL = "E_LEADER_IS_MASTER_ALL"
    E_LEAST_ASSIGNED = "E_LEAST_ASSIGNED"


@dataclass
class MessagePropertiesV2:
    """This complex type captures information which can be used to tell a broker if
    it should advertise support for message properties v2 format (also knownn as
    extended or 'EX' message properties at some places).

    Additionally, broker can be configured to advertise this feature
    only to those C++ and Java clients which match a certain minimum SDK
    version.
    """

    advertise_v2_support: bool = field(
        default=True,
        metadata={
            "name": "advertiseV2Support",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    min_cpp_sdk_version: int = field(
        default=11207,
        metadata={
            "name": "minCppSdkVersion",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    min_java_sdk_version: int = field(
        default=10,
        metadata={
            "name": "minJavaSdkVersion",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )


@dataclass
class MessageThrottleConfig:
    """Configuration values for message throttling intervals and thresholds.

    lowInterval...: time in milliseconds.
    highInterval..: time in milliseconds.
    lowThreshold..: indicates the rda counter value at which we start
    throttlling for time equal to 'lowInterval'.
    highThreshold.: indicates the rda counter value at which we start
    throttlling for time equal to 'highInterval'.
    Note: lowInterval should be less than/equal to highInterval,
    lowThreshold should be less than highThreshold.
    """

    low_threshold: int = field(
        default=2,
        metadata={
            "name": "lowThreshold",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    high_threshold: int = field(
        default=4,
        metadata={
            "name": "highThreshold",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    low_interval: int = field(
        default=1000,
        metadata={
            "name": "lowInterval",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    high_interval: int = field(
        default=3000,
        metadata={
            "name": "highInterval",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )


@dataclass
class Plugins:
    libraries: List[str] = field(
        default_factory=list,
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "min_occurs": 1,
        },
    )
    enabled: List[str] = field(
        default_factory=list,
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "min_occurs": 1,
        },
    )


@dataclass
class QueueOperationsConfig:
    """Type representing the configuration for queue operations on a cluster.

    openTimeoutMs..............:
    timeout, in milliseconds, to use when opening a queue.  An open
    request requires some co-ordination among the nodes (queue
    assignment request/response, followed by queue open
    request/response, etc)
    configureTimeoutMs.........:
    timeout, in milliseconds, to use for a configure queue request.
    Note that `configureTimeoutMs` must be less than or equal to
    `closeTimeoutMs` to prevent out-of-order processing of closeQueue
    (e.g.  closeQueue sent after configureQueue but timeout response
    processed first for the closeQueue)
    closeTimeoutMs.............:
    timeout, in milliseconds, to use for a close queue request
    reopenTimeoutMs............:
    timeout, in milliseconds, to use when sending a reopen-queue
    request.  Ideally, we should use same value as `openTimeoutMs`, but
    we are using a very large value as a workaround: during network
    outages, a proxy or a replica may failover to a new upstream node,
    which itself may be out of sync or not yet ready.  Eventually, the
    reopen-queue requests sent during failover may timeout, and will
    never be retried, leading to a 'permanent' failure (client consumer
    app stops receiving messages; PUT messages from client producer app
    starts getting NAK'd or buffered).  Using such a large timeout
    value helps in a situation when network outages or its
    after-effects are fixed after a few hours).
    reopenRetryIntervalMs......:
    duration, in milliseconds, after which a retry attempt should be
    made to reopen the queue
    reopenMaxAttempts..........:
    maximum number of attempts to reopen a queue when restoring the
    state in a proxy upon getting a new active node notification
    assignmentTimeoutMs........:
    timeout, in milliseconds, to use for a queue assignment request
    keepaliveDurationMs........:
    duration, in milliseconds, to keep a queue alive after it has met
    the criteria for deletion
    consumptionMonitorPeriodMs.:
    frequency, in milliseconds, in which the consumption monitor checks
    queue consumption statistics on the cluster
    stopTimeoutMs..............:
    timeout, in milliseconds, to use in StopRequest between
    deconfiguring and closing each affected queue.  This is primarily
    to give a chance for pending PUSH mesages to be CONFIRMed.
    shutdownTimeoutMs..........:
    timeout, in milliseconds, to use when node stops for shutdown or
    maintenance mode.  This timeout should be greater than the
    'stopTimeoutMs'. This is to handle misbehaving downstream which may
    not reply to stopRequest (otherwise, this timeout is not expected
    to be reached).
    ackWindowSize..............:
    number of PUTs without ACK requested after which we request an ACK.
    This is to remove pending broadcast PUTs.
    """

    open_timeout_ms: int = field(
        default=300000,
        metadata={
            "name": "openTimeoutMs",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    configure_timeout_ms: int = field(
        default=300000,
        metadata={
            "name": "configureTimeoutMs",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    close_timeout_ms: int = field(
        default=300000,
        metadata={
            "name": "closeTimeoutMs",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    reopen_timeout_ms: int = field(
        default=43200000,
        metadata={
            "name": "reopenTimeoutMs",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    reopen_retry_interval_ms: int = field(
        default=5000,
        metadata={
            "name": "reopenRetryIntervalMs",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    reopen_max_attempts: int = field(
        default=10,
        metadata={
            "name": "reopenMaxAttempts",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    assignment_timeout_ms: int = field(
        default=15000,
        metadata={
            "name": "assignmentTimeoutMs",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    keepalive_duration_ms: int = field(
        default=1800000,
        metadata={
            "name": "keepaliveDurationMs",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    consumption_monitor_period_ms: int = field(
        default=30000,
        metadata={
            "name": "consumptionMonitorPeriodMs",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    stop_timeout_ms: int = field(
        default=10000,
        metadata={
            "name": "stopTimeoutMs",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    shutdown_timeout_ms: int = field(
        default=20000,
        metadata={
            "name": "shutdownTimeoutMs",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    ack_window_size: int = field(
        default=500,
        metadata={
            "name": "ackWindowSize",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )


@dataclass
class ResolvedDomain:
    """Top level type representing the information retrieved when resolving a
    domain.

    resolvedName.: Resolved name of the domain clusterName..: Name of
    the cluster where this domain exists
    """

    resolved_name: Optional[str] = field(
        default=None,
        metadata={
            "name": "resolvedName",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    cluster_name: Optional[str] = field(
        default=None,
        metadata={
            "name": "clusterName",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )


@dataclass
class StatsPrinterConfig:
    print_interval: int = field(
        default=60,
        metadata={
            "name": "printInterval",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    file: Optional[str] = field(
        default=None,
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    max_age_days: Optional[int] = field(
        default=None,
        metadata={
            "name": "maxAgeDays",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    rotate_bytes: int = field(
        default=268435456,
        metadata={
            "name": "rotateBytes",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    rotate_days: int = field(
        default=1,
        metadata={
            "name": "rotateDays",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )


@dataclass
class StorageSyncConfig:
    """Type representing the configuration for storage synchronization and recovery
    of a cluster.

    startupRecoveryMaxDurationMs...:
    maximum amount of time, in milliseconds, in which recovery for a
    partition at node startup must complete.  This interval captures
    the time taken to receive storage-sync response, retry attempts for
    failed storage-sync request, as well as the time taken by the peer
    node to send partition files to the starting (i.e.  requester) node
    maxAttemptsStorageSync.........:
    maximum number of attempts that a node makes for storage-sync
    requests when it comes up (this value includes the 1st attempt)
    storageSyncReqTimeoutMs........:
    timeout, in milliseconds, for the storage-sync request.  A bigger
    value is recommended because peer node could be busy serving
    storage-sync request for other partition(s) assigned to the same
    partition-dispatcher thread, etc.  This timeout does *not* capture
    the time taken by the peer to send partition files
    masterSyncMaxDurationMs........:
    maximum amount of time, in milliseconds, in which master sync for a
    partition must complete.  This interval includes the time taken by
    replica node (the one with advanced view of the partition) to send
    the file chunks, as well as time taken to receive partition-sync
    state and data responses
    partitionSyncStateReqTimeoutMs.:
    timeout, in milliseconds, for partition-sync-state-query request.
    This request is sent by a new master node to all replica nodes to
    query their view of the partition
    partitionSyncDataReqTimeoutMs..:
    timeout, in milliseconds, for partition-sync-data-query request.
    This request is sent by a new master node to a replica node (which
    has an advanced view of the partition) to initiate partition sync.
    This duration does *not* capture the amount of time which replica
    might take to send the partition file
    startupWaitDurationMs..........:
    duration, in milliseconds, for which recovery manager waits for a
    sync point for a partition.  If no sync point is received from a
    peer for a partition during this time, it is assumed that there is
    no master for that partition, and recovery manager randomly picks
    up a node from all the available ones with send a sync request.  If
    no peers are available at this time, it is assumed that entire
    cluster is coming up together, and proceeds with local recovery for
    that partition.  Note that this value should be less than the
    duration for which a node waits if its elected a leader and there
    are no AVAILABLE nodes.  This is important so that if all nodes in
    the cluster are starting, they have a chance to wait for
    'startupWaitDurationMs' milliseconds, find out that none of the
    partitions have any master, go ahead with local recovery and
    declare themselves as AVAILABLE.  This will give the new leader
    node a chance to make each node a master for a given partition.
    Moreover, this value should be greater than the duration for which
    a peer waits before attempting to reconnect to the node in the
    cluster, so that peer has a chance to connect to this node, get
    notified (via ClusterObserver), and send sync point if its a master
    for any partition
    fileChunkSize..................:
    chunk size, in bytes, to send in one go to the peer when serving a
    storage sync request from it
    partitionSyncEventSize.........:
    maximum size, in bytes, of bmqp::EventType::PARTITION_SYNC before
    we send it to the peer
    """

    startup_recovery_max_duration_ms: int = field(
        default=1200000,
        metadata={
            "name": "startupRecoveryMaxDurationMs",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    max_attempts_storage_sync: int = field(
        default=3,
        metadata={
            "name": "maxAttemptsStorageSync",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    storage_sync_req_timeout_ms: int = field(
        default=300000,
        metadata={
            "name": "storageSyncReqTimeoutMs",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    master_sync_max_duration_ms: int = field(
        default=600000,
        metadata={
            "name": "masterSyncMaxDurationMs",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    partition_sync_state_req_timeout_ms: int = field(
        default=120000,
        metadata={
            "name": "partitionSyncStateReqTimeoutMs",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    partition_sync_data_req_timeout_ms: int = field(
        default=120000,
        metadata={
            "name": "partitionSyncDataReqTimeoutMs",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    startup_wait_duration_ms: int = field(
        default=60000,
        metadata={
            "name": "startupWaitDurationMs",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    file_chunk_size: int = field(
        default=4194304,
        metadata={
            "name": "fileChunkSize",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    partition_sync_event_size: int = field(
        default=4194304,
        metadata={
            "name": "partitionSyncEventSize",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )


@dataclass
class SyslogConfig:
    enabled: bool = field(
        default=False,
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    app_name: Optional[str] = field(
        default=None,
        metadata={
            "name": "appName",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    log_format: Optional[str] = field(
        default=None,
        metadata={
            "name": "logFormat",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    verbosity: Optional[str] = field(
        default=None,
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )


@dataclass
class TcpClusterNodeConnection:
    """Configuration of a TCP based cluster node connectivity.

    endpoint.: endpoint URI of the node address
    """

    endpoint: Optional[str] = field(
        default=None,
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )


@dataclass
class TcpInterfaceListener:
    """This type describes the information needed for the broker to open a TCP
    listener.

    name.................:
    A name to associate this listener to.
    port.................:
    The port this listener will accept connections on.
    """

    name: Optional[str] = field(
        default=None,
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    port: Optional[int] = field(
        default=None,
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )


@dataclass
class VirtualClusterInformation:
    """Type representing the information about the current node with regards to
    virtual cluster.

    name.............: name of the cluster
    selfNodeId.......: id of the current node in that virtual cluster
    """

    name: Optional[str] = field(
        default=None,
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    self_node_id: Optional[int] = field(
        default=None,
        metadata={
            "name": "selfNodeId",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )


@dataclass
class ClusterNodeConnection:
    """Choice of all the various transport mode available to establish connectivity
    with a node.

    tcp.: TCP connectivity
    """

    tcp: Optional[TcpClusterNodeConnection] = field(
        default=None,
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )


@dataclass
class DispatcherProcessorConfig:
    num_processors: Optional[int] = field(
        default=None,
        metadata={
            "name": "numProcessors",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    processor_config: Optional[DispatcherProcessorParameters] = field(
        default=None,
        metadata={
            "name": "processorConfig",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )


@dataclass
class LogController:
    file_name: Optional[str] = field(
        default=None,
        metadata={
            "name": "fileName",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    file_max_age_days: Optional[int] = field(
        default=None,
        metadata={
            "name": "fileMaxAgeDays",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    rotation_bytes: Optional[int] = field(
        default=None,
        metadata={
            "name": "rotationBytes",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    logfile_format: Optional[str] = field(
        default=None,
        metadata={
            "name": "logfileFormat",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    console_format: Optional[str] = field(
        default=None,
        metadata={
            "name": "consoleFormat",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    logging_verbosity: Optional[str] = field(
        default=None,
        metadata={
            "name": "loggingVerbosity",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    bsls_log_severity_threshold: str = field(
        default="ERROR",
        metadata={
            "name": "bslsLogSeverityThreshold",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    console_severity_threshold: Optional[str] = field(
        default=None,
        metadata={
            "name": "consoleSeverityThreshold",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    categories: List[str] = field(
        default_factory=list,
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "min_occurs": 1,
        },
    )
    syslog: Optional[SyslogConfig] = field(
        default=None,
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    log_dump: Optional[LogDumpConfig] = field(
        default=None,
        metadata={
            "name": "logDump",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )


@dataclass
class PartitionConfig:
    """Type representing the configuration for the storage layer of a cluster.

    numPartitions........: number of partitions at each node in the cluster
    location.............: location of active files for a partition
    archiveLocation......: location of archive files for a partition
    maxDataFileSize......: maximum size of partitions' data file
    maxJournalFileSize...: maximum size of partitions' journal file
    maxQlistFileSize.....: maximum size of partitions' qlist file
    maxCSLFileSize.......: maximum size of partitions' CSL file
    preallocate..........: flag to indicate whether files should be
    preallocated on disk
    maxArchivedFileSets..: maximum number of archived file sets per
    partition to keep
    prefaultPages........: flag to indicate whether to populate (prefault)
    page tables for a mapping.
    flushAtShutdown......: flag to indicate whether broker should flush
    storage files to disk at shutdown
    syncConfig...........: configuration for storage synchronization and
    recovery
    """

    num_partitions: Optional[int] = field(
        default=None,
        metadata={
            "name": "numPartitions",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    location: Optional[str] = field(
        default=None,
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    archive_location: Optional[str] = field(
        default=None,
        metadata={
            "name": "archiveLocation",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    max_data_file_size: Optional[int] = field(
        default=None,
        metadata={
            "name": "maxDataFileSize",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    max_journal_file_size: Optional[int] = field(
        default=None,
        metadata={
            "name": "maxJournalFileSize",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    max_qlist_file_size: Optional[int] = field(
        default=None,
        metadata={
            "name": "maxQlistFileSize",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    max_cslfile_size: int = field(
        default=67108864,
        metadata={
            "name": "maxCSLFileSize",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    preallocate: bool = field(
        default=False,
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    max_archived_file_sets: Optional[int] = field(
        default=None,
        metadata={
            "name": "maxArchivedFileSets",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    prefault_pages: bool = field(
        default=False,
        metadata={
            "name": "prefaultPages",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    flush_at_shutdown: bool = field(
        default=True,
        metadata={
            "name": "flushAtShutdown",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    sync_config: Optional[StorageSyncConfig] = field(
        default=None,
        metadata={
            "name": "syncConfig",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )


@dataclass
class StatPluginConfigPrometheus:
    mode: ExportMode = field(
        default=ExportMode.E_PULL,
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    host: str = field(
        default="localhost",
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    port: int = field(
        default=8080,
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )


@dataclass
class TcpInterfaceConfig:
    """name.................:

    The name of the TCP session manager.
    port.................:
    (Deprecated) The port to receive connections.
    lowWatermark.........:
    highWatermark........:
    Watermarks used for channels with a client or proxy.
    nodeLowWatermark.....:
    nodeHighWatermark....:
    Reduced watermarks for communication between cluster nodes where
    BlazingMQ maintains its own cache.
    heartbeatIntervalMs..:
    How often (in milliseconds) to check if the channel received data,
    and emit heartbeat.  0 to globally disable.
    listeners:
    A list of listener interfaces to receive TCP connections from. When non-empty
    this option overrides the listener specified by port.
    """

    name: Optional[str] = field(
        default=None,
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    port: Optional[int] = field(
        default=None,
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    io_threads: Optional[int] = field(
        default=None,
        metadata={
            "name": "ioThreads",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    max_connections: int = field(
        default=10000,
        metadata={
            "name": "maxConnections",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    low_watermark: Optional[int] = field(
        default=None,
        metadata={
            "name": "lowWatermark",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    high_watermark: Optional[int] = field(
        default=None,
        metadata={
            "name": "highWatermark",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    node_low_watermark: int = field(
        default=1024,
        metadata={
            "name": "nodeLowWatermark",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    node_high_watermark: int = field(
        default=2048,
        metadata={
            "name": "nodeHighWatermark",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    heartbeat_interval_ms: int = field(
        default=3000,
        metadata={
            "name": "heartbeatIntervalMs",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    listeners: List[TcpInterfaceListener] = field(
        default_factory=list,
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )


@dataclass
class ClusterNode:
    """Type representing the configuration of a node in a cluster.

    id.........: the unique ID of that node in the cluster; must be a &gt; 0
    value
    name.......: name of this node
    datacenter.: the datacenter of that node
    transport..: the transport configuration for establishing connectivity
    with the node
    """

    id: Optional[int] = field(
        default=None,
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    name: Optional[str] = field(
        default=None,
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    data_center: Optional[str] = field(
        default=None,
        metadata={
            "name": "dataCenter",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    transport: Optional[ClusterNodeConnection] = field(
        default=None,
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )


@dataclass
class DispatcherConfig:
    sessions: Optional[DispatcherProcessorConfig] = field(
        default=None,
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    queues: Optional[DispatcherProcessorConfig] = field(
        default=None,
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    clusters: Optional[DispatcherProcessorConfig] = field(
        default=None,
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )


@dataclass
class NetworkInterfaces:
    heartbeats: Optional[Heartbeat] = field(
        default=None,
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    tcp_interface: Optional[TcpInterfaceConfig] = field(
        default=None,
        metadata={
            "name": "tcpInterface",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )


@dataclass
class ReversedClusterConnection:
    """Type representing the configuration for remote cluster connections..

    name.............: name of the cluster
    connections......: list of connections to establish
    """

    name: Optional[str] = field(
        default=None,
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    connections: List[ClusterNodeConnection] = field(
        default_factory=list,
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "min_occurs": 1,
        },
    )


@dataclass
class StatPluginConfig:
    name: str = field(
        default="",
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    queue_size: int = field(
        default=10000,
        metadata={
            "name": "queueSize",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    queue_high_watermark: int = field(
        default=5000,
        metadata={
            "name": "queueHighWatermark",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    queue_low_watermark: int = field(
        default=1000,
        metadata={
            "name": "queueLowWatermark",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    publish_interval: int = field(
        default=30,
        metadata={
            "name": "publishInterval",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    namespace_prefix: str = field(
        default="",
        metadata={
            "name": "namespacePrefix",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    hosts: List[str] = field(
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
            "required": True,
        },
    )
    prometheus_specific: Optional[StatPluginConfigPrometheus] = field(
        default=None,
        metadata={
            "name": "prometheusSpecific",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
        },
    )


@dataclass
class TaskConfig:
    allocator_type: Optional[AllocatorType] = field(
        default=None,
        metadata={
            "name": "allocatorType",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    allocation_limit: Optional[int] = field(
        default=None,
        metadata={
            "name": "allocationLimit",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    log_controller: Optional[LogController] = field(
        default=None,
        metadata={
            "name": "logController",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )


@dataclass
class ClusterDefinition:
    """Type representing the configuration for a cluster.

    name..................: name of the cluster
    nodes.................: list of nodes in the cluster
    partitionConfig.......: configuration for the storage
    masterAssignment......: algorithm to use for partition's master assignment
    elector...............: configuration for leader election amongst the nodes
    queueOperations.......: configuration for queue operations on the cluster
    clusterAttributes.....: attributes specific to this cluster
    clusterMonitorConfig..: configuration for cluster state monitor
    messageThrottleConfig.: configuration for message throttling intervals and
    thresholds.
    """

    name: Optional[str] = field(
        default=None,
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    nodes: List[ClusterNode] = field(
        default_factory=list,
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "min_occurs": 1,
        },
    )
    partition_config: Optional[PartitionConfig] = field(
        default=None,
        metadata={
            "name": "partitionConfig",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    master_assignment: Optional[MasterAssignmentAlgorithm] = field(
        default=None,
        metadata={
            "name": "masterAssignment",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    elector: Optional[ElectorConfig] = field(
        default=None,
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    queue_operations: Optional[QueueOperationsConfig] = field(
        default=None,
        metadata={
            "name": "queueOperations",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    cluster_attributes: Optional[ClusterAttributes] = field(
        default=None,
        metadata={
            "name": "clusterAttributes",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    cluster_monitor_config: Optional[ClusterMonitorConfig] = field(
        default=None,
        metadata={
            "name": "clusterMonitorConfig",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    message_throttle_config: Optional[MessageThrottleConfig] = field(
        default=None,
        metadata={
            "name": "messageThrottleConfig",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )


@dataclass
class ClusterProxyDefinition:
    """Type representing the configuration for a cluster proxy.

    name..................: name of the cluster
    nodes.................: list of nodes in the cluster
    queueOperations.......: configuration for queue operations with the cluster
    clusterMonitorConfig..: configuration for cluster state monitor
    messageThrottleConfig.: configuration for message throttling intervals and
    thresholds.
    """

    name: Optional[str] = field(
        default=None,
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    nodes: List[ClusterNode] = field(
        default_factory=list,
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "min_occurs": 1,
        },
    )
    queue_operations: Optional[QueueOperationsConfig] = field(
        default=None,
        metadata={
            "name": "queueOperations",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    cluster_monitor_config: Optional[ClusterMonitorConfig] = field(
        default=None,
        metadata={
            "name": "clusterMonitorConfig",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    message_throttle_config: Optional[MessageThrottleConfig] = field(
        default=None,
        metadata={
            "name": "messageThrottleConfig",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )


@dataclass
class StatsConfig:
    snapshot_interval: int = field(
        default=1,
        metadata={
            "name": "snapshotInterval",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    plugins: List[StatPluginConfig] = field(
        default_factory=list,
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "min_occurs": 1,
        },
    )
    printer: Optional[StatsPrinterConfig] = field(
        default=None,
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )


@dataclass
class AppConfig:
    """Top level type for the broker's configuration.

    brokerInstanceName...: name of the broker instance
    brokerVersion........: version of the broker
    configVersion........: version of the bmqbrkr.cfg config
    etcDir...............: directory containing the json config files
    hostName.............: name of the current host
    hostTags.............: tags of the current host
    hostDataCenter.......: datacenter the current host resides in
    isRunningOnDev.......: true if running on dev
    logsObserverMaxSize..: maximum number of log records to keep
    latencyMonitorDomain.: common part of all latemon domains
    dispatcherConfig.....: configuration for the dispatcher
    stats................: configuration for the stats
    networkInterfaces....: configuration for the network interfaces
    bmqconfConfig........: configuration for bmqconf
    plugins..............: configuration for the plugins
    msgPropertiesSupport.: information about if/how to advertise support for v2 message properties
    configureStream......: send new ConfigureStream instead of old ConfigureQueue
    advertiseSubscriptions.: temporarily control use of ConfigureStream in SDK
    routeCommandTimeoutMs: maximum amount of time to wait for a routed command's response
    """

    broker_instance_name: Optional[str] = field(
        default=None,
        metadata={
            "name": "brokerInstanceName",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    broker_version: Optional[int] = field(
        default=None,
        metadata={
            "name": "brokerVersion",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    config_version: Optional[int] = field(
        default=None,
        metadata={
            "name": "configVersion",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    etc_dir: Optional[str] = field(
        default=None,
        metadata={
            "name": "etcDir",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    host_name: Optional[str] = field(
        default=None,
        metadata={
            "name": "hostName",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    host_tags: Optional[str] = field(
        default=None,
        metadata={
            "name": "hostTags",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    host_data_center: Optional[str] = field(
        default=None,
        metadata={
            "name": "hostDataCenter",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    is_running_on_dev: Optional[bool] = field(
        default=None,
        metadata={
            "name": "isRunningOnDev",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    logs_observer_max_size: Optional[int] = field(
        default=None,
        metadata={
            "name": "logsObserverMaxSize",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    latency_monitor_domain: str = field(
        default="bmq.sys.latemon.latency",
        metadata={
            "name": "latencyMonitorDomain",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    dispatcher_config: Optional[DispatcherConfig] = field(
        default=None,
        metadata={
            "name": "dispatcherConfig",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    stats: Optional[StatsConfig] = field(
        default=None,
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    network_interfaces: Optional[NetworkInterfaces] = field(
        default=None,
        metadata={
            "name": "networkInterfaces",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    bmqconf_config: Optional[BmqconfConfig] = field(
        default=None,
        metadata={
            "name": "bmqconfConfig",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    plugins: Optional[Plugins] = field(
        default=None,
        metadata={
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    message_properties_v2: Optional[MessagePropertiesV2] = field(
        default=None,
        metadata={
            "name": "messagePropertiesV2",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    configure_stream: bool = field(
        default=False,
        metadata={
            "name": "configureStream",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    advertise_subscriptions: bool = field(
        default=False,
        metadata={
            "name": "advertiseSubscriptions",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    route_command_timeout_ms: int = field(
        default=3000,
        metadata={
            "name": "routeCommandTimeoutMs",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )


@dataclass
class ClustersDefinition:
    """Top level type representing the configuration for all clusters.

    myClusters.................:
    definition of the clusters the current machine is part of (if any);
    empty means this broker does not belong to any cluster
    myReverseClusters..........:
    name of the clusters (if any) the current machine is expected to
    receive inbound connections about and therefore should pro-actively
    create a proxy cluster at startup
    myVirtualClusters..........:
    information about all the virtual clusters the current machine is
    considered to belong to (if any)
    clusters...................: array of cluster definition
    reversedClusterConnections.:
    cluster and associated remote connections that should be
    established
    """

    my_clusters: List[ClusterDefinition] = field(
        default_factory=list,
        metadata={
            "name": "myClusters",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "min_occurs": 1,
        },
    )
    my_reverse_clusters: List[str] = field(
        default_factory=list,
        metadata={
            "name": "myReverseClusters",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "min_occurs": 1,
        },
    )
    my_virtual_clusters: List[VirtualClusterInformation] = field(
        default_factory=list,
        metadata={
            "name": "myVirtualClusters",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "min_occurs": 1,
        },
    )
    proxy_clusters: List[ClusterProxyDefinition] = field(
        default_factory=list,
        metadata={
            "name": "proxyClusters",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "min_occurs": 1,
        },
    )
    reversed_cluster_connections: List[ReversedClusterConnection] = field(
        default_factory=list,
        metadata={
            "name": "reversedClusterConnections",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "min_occurs": 1,
        },
    )


@dataclass
class Configuration:
    task_config: Optional[TaskConfig] = field(
        default=None,
        metadata={
            "name": "taskConfig",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
    app_config: Optional[AppConfig] = field(
        default=None,
        metadata={
            "name": "appConfig",
            "type": "Element",
            "namespace": "http://bloomberg.com/schemas/mqbcfg",
            "required": True,
        },
    )
