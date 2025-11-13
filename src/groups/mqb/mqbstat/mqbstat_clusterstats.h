// Copyright 2018-2023 Bloomberg Finance L.P.
// SPDX-License-Identifier: Apache-2.0
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// mqbstat_clusterstats.h                                             -*-C++-*-
#ifndef INCLUDED_MQBSTAT_CLUSTERSTATS
#define INCLUDED_MQBSTAT_CLUSTERSTATS

//@PURPOSE: Provide mechanism to keep track of Cluster statistics.
//
//@CLASSES:
//  mqbstat::ClusterStats:     Mechanism to maintain stats of a cluster
//  mqbstat::ClusterNodeStats: Mechanism to maintain stats of a cluster node
//  mqbstat::ClusterStatsUtil: Utilities to initialize statistics
//
//@DESCRIPTION: 'mqbstat::ClusterStats' provides a mechanism to keep track of
// cluster level statistics.  'mqbstat::ClusterNodeStats' provides a mechanism
// to keep track of cluster node statistics.  'mqbstat::ClusterstatsUtil' is a
// utility namespace exposing methods to initialize the stat contexts.

// BMQ
#include <bmqst_statcontext.h>
#include <bmqt_uri.h>

// MQB

// BDE
#include <bsl_memory.h>
#include <bsl_ostream.h>
#include <bsl_string.h>
#include <bsl_vector.h>
#include <bsla_annotations.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_cpp11.h>
#include <bsls_types.h>

namespace BloombergLP {

namespace mqbstat {

class PartitionStats;

// ==================
// class ClusterStats
// ==================

/// Mechanism to keep track of individual overall statistics of a cluster
class ClusterStats {
  public:
    // TYPES

    /// Enum representing the various type of stats that can be obtained
    /// from this object.
    struct Stat {
        // TYPES
        enum Enum {
            // ClusterStats: those metrics make sense only from the 'cluster
            //               level' stat context
            e_CLUSTER_STATUS,
            e_ROLE,
            e_LEADER_STATUS,
            /// Time in nanoseconds it took for replication of a new entry in
            /// CSL file. Average observed during the report interval.
            e_CSL_REPLICATION_TIME_NS_AVG,
            /// Time in nanoseconds it took for replication of a new entry in
            /// CSL file. Maximum observed during the report interval.
            e_CSL_REPLICATION_TIME_NS_MAX,
            /// Last observed offset bytes in the newest log of the CSL.
            e_CSL_LOG_OFFSET_BYTES,
            /// Amount of bytes written to the csl file.
            e_CSL_WRITE_BYTES,
            /// Configured maximum size of the CSL file.
            e_CSL_CFG_BYTES,
            /// Configured maximum size of the data file.
            e_PARTITION_CFG_DATA_BYTES,
            /// Configured maximum size of the journal file.
            e_PARTITION_CFG_JOURNAL_BYTES,

            // PartitionStats: those metrics make sense only from the
            //                 'partition level' stat context

            /// Value from the 'PrimaryStatus::Enum' defining the primary
            /// status of this node for the partition.  Note that we report the
            /// highest observed value, so that if the node was primary at any
            /// time, even briefly, during the report interval, then it will be
            /// considered as primary for the whole interval.
            e_PARTITION_PRIMARY_STATUS,
            /// Time in nanoseconds it took for the rollover of the partition.
            /// Note that in case when more than one rollover operations
            /// happened during the report interval, then the maximum time is
            /// returned.
            e_PARTITION_ROLLOVER_TIME,
            /// Maximum observed outstanding bytes in the data file of the
            /// partition.
            e_PARTITION_DATA_CONTENT,
            /// Maximum observed outstanding bytes in the journal file of the
            /// partition.
            e_PARTITION_JOURNAL_CONTENT,
            /// Latest observed offset bytes in the data file of the partition.
            e_PARTITION_DATA_OFFSET,
            /// Latest observed offset bytes in the journal file of the
            /// partition.
            e_PARTITION_JOURNAL_OFFSET,
            /// Maximum observed utilization of the data file of the partition.
            e_PARTITION_DATA_UTILIZATION_MAX,
            /// Maximum observed utilization of the journal file of the
            /// partition.
            e_PARTITION_JOURNAL_UTILIZATION_MAX,
            /// Currently observed sequence number of the partition.
            e_PARTITION_SEQUENCE_NUMBER,
            /// Average observed time in nanoseconds it took to store a message
            /// record at primary and replicate it to a majority of nodes in
            /// the cluster.
            e_PARTITION_REPLICATION_TIME_NS_AVG,
            /// Maximum observed time in nanoseconds it took to store a message
            /// record at primary and replicate it to a majority of nodes in
            /// the cluster.
            e_PARTITION_REPLICATION_TIME_NS_MAX
        };
    };

    /// Enum representing the leader status (leader or follower) of a node.
    struct LeaderStatus {
        // TYPES
        enum Enum { e_FOLLOWER, e_LEADER };
    };

    /// Enum representing the role (member or proxy) of a broker for a given
    /// cluster.
    struct Role {
        enum Enum { e_MEMBER = 1, e_PROXY };

        // CLASS METHODS

        /// Write the string representation of the specified enumeration
        /// `value` to the specified output `stream`, and return a reference
        /// to `stream`.  Optionally specify an initial indentation `level`,
        /// whose absolute value is incremented recursively for nested
        /// objects.  If `level` is specified, optionally specify
        /// `spacesPerLevel`, whose absolute value indicates the number of
        /// spaces per indentation level for this and all of its nested
        /// objects.  If `level` is negative, suppress indentation of the
        /// first line.  If `spacesPerLevel` is negative, format the entire
        /// output on one line, suppressing all but the initial indentation
        /// (as governed by `level`).  See `toAscii` for what constitutes
        /// the string representation of a `Role::Enum` value.
        static bsl::ostream& print(bsl::ostream& stream,
                                   Enum          value,
                                   int           level          = 0,
                                   int           spacesPerLevel = 4);

        /// Return the non-modifiable string representation corresponding to
        /// the specified enumeration `value`, if it exists, and a unique
        /// (error) string otherwise.  The string representation of `value`
        /// matches its corresponding enumerator name with the `e_` prefix
        /// elided.  Note that specifying a `value` that does not match any
        /// of the enumerators will result in a string representation that
        /// is distinct from any of those corresponding to the enumerators,
        /// but is otherwise unspecified.
        static const char* toAscii(Enum value);
    };

  private:
    // FRIENDS
    friend class PartitionStats;

    // PRIVATE TYPES

    /// Namespace for the constants of stat values that applies to the
    /// partition.
    struct ClusterStatsIndex {
        enum Enum {
            // Cluster stats

            /// Value: Health status of cluster, 1 implies healthy and 2
            ///        implies un-healthy.
            e_CLUSTER_STATUS = 0,
            /// Value: Leader status of cluster, non-zero (1) implies leader
            ///        and 0 implies follower.
            e_LEADER_STATUS,
            /// Value: Time in nanoseconds it took for replication of a new
            ///        entry in CSL file.
            e_CSL_REPLICATION_TIME_NS,
            /// Value: Last observed offset bytes in the newest log of the CSL.
            e_CSL_LOG_OFFSET_BYTES,
            /// Value: Bytes written to the CSL file.
            e_CSL_WRITE_BYTES,
            /// Value: Configured maximum size of the CSL file.
            e_CSL_CFG_BYTES,
            /// Value: Configured size of partitions' data file.
            e_PARTITION_CFG_DATA_BYTES,
            /// Value: Configured size of partitions' journal file.
            e_PARTITION_CFG_JOURNAL_BYTES,

            // Per partition stats

            /// Value: The primary status of the partition, from the
            ///        'PrimaryStatus::Enum' values.
            e_PRIMARY_STATUS,
            /// Value: Nanoseconds time it took for rolling over the partition.
            e_PARTITION_ROLLOVER_TIME,
            /// Value: Outstanding bytes in the data file of the partition.
            e_PARTITION_DATA_BYTES,
            /// Value: Outstanding bytes in the journal file of the partition.
            e_PARTITION_JOURNAL_BYTES,
            /// Value: Offset bytes in the data file of the partition.
            e_PARTITION_DATA_OFFSET_BYTES,
            /// Value: Offset bytes in the journal file of the partition.
            e_PARTITION_JOURNAL_OFFSET_BYTES,
            /// Value: The latest sequence number observed for the partition.
            e_PARTITION_SEQUENCE_NUMBER,
            /// Value: Time in nanoseconds it took for replication of a new
            /// entry in journal file.
            e_PARTITION_REPLICATION_TIME_NS
        };
    };

    /// Namespace for the constants of stat values that applies to the queues
    /// from the clients
    struct ClusterStatus {
        enum Enum {
            e_CLUSTER_STATUS_HEALTHY   = 1,
            e_CLUSTER_STATUS_UNHEALTHY = 2
        };
    };

    // DATA

    /// StatContext for the cluster
    bslma::ManagedPtr<bmqst::StatContext> d_statContext_mp;

    /// `PartitionStats` for each partition in the cluster, indexed by the
    /// partition id. Each instance contains a `StatContext` that is created as
    /// a child of the above `d_StatContext_mp`.
    bsl::vector<bsl::shared_ptr<PartitionStats> > d_partitionsStats;

  private:
    // NOT IMPLEMENTED
    ClusterStats(const ClusterStats&) BSLS_CPP11_DELETED;

    /// Copy constructor and assignment operator are not implemented.
    ClusterStats& operator=(const ClusterStats&) BSLS_CPP11_DELETED;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(ClusterStats, bslma::UsesBslmaAllocator)

    // CLASS METHODS

    /// Get the value of the specified `stat` reported to the cluster
    /// represented by its associated specified `context` as the difference
    /// between the latest snapshot-ed value (i.e., `snapshotId == 0`) and
    /// the value that was recorded at the specified `snapshotId` snapshots
    /// ago.
    ///
    /// THREAD: This method can only be invoked from the `snapshot` thread.
    static bsls::Types::Int64 getValue(const bmqst::StatContext& context,
                                       int                       snapshotId,
                                       const Stat::Enum&         stat);

    /// Get the `PartitionStats` of the specified `partitionId`.
    bsl::shared_ptr<PartitionStats> getPartitionStats(int partitionId) const;

    // CREATORS

    /// Create a new object in an uninitialized state.
    explicit ClusterStats(bslma::Allocator* allocator);

    // MANIPULATORS

    /// Initialize this object for the cluster with the specified `name`
    /// having the specified `partitionsCount` partitions, and register it
    /// as a subcontext of the specified `clusterStatContext` and using the
    /// specified `allocator`.
    void initialize(const bsl::string&  name,
                    int                 partitionsCount,
                    bmqst::StatContext* clusterStatContext,
                    bslma::Allocator*   allocator);

    /// Update the `cluster_status` field of the StatContext being referred
    /// to by this object to be the specified `value`.
    void setHealthStatus(bool value);

    /// Update the `cluster_role` field of the StatContext being referred
    /// to by this object to be the specified `value`.
    void setIsMember(bool value);

    /// Update the `upstream` field of the StatContext being referred
    /// to by this object to be the specified `value`.
    void setUpstream(const bsl::string& value);

    /// Update the `cluster_leader` field of the StatContext being referred
    /// to by this object to be the specified `value`.
    void setIsLeader(LeaderStatus::Enum value);

    /// Update the `partition.cfg_data_bytes` field to the specified
    /// `dataBytes`, the `partition.cfg_journal_bytes` field to the specified
    /// `journalBytes` and the `cluster_csl_cfg_bytes` field to the specified
    /// `cslBytes` in the StatContext being referred to by this object.
    void setPartitionCfgBytes(bsls::Types::Int64 dataBytes,
                              bsls::Types::Int64 journalBytes,
                              bsls::Types::Int64 cslBytes);

    /// Set the csl replication time of the StatContext being referred
    /// to by this object to be the specified `value`.
    void setCslReplicationTime(bsls::Types::Int64 value);

    /// Set the csl offset bytes of the StatContext being referred to by this
    /// object to be the specified `value`.
    void setCslOffsetBytes(bsls::Types::Int64 value);

    /// Adjust the csl offset bytes of the StatContext being referred to by
    /// this object by the specified `delta`.
    void addCslOffsetBytes(bsls::Types::Int64 delta);

    /// Return a pointer to the statcontext.
    bmqst::StatContext* statContext();
};

// ====================
// class PartitionStats
// ====================

/// Mechanism to keep track of individual overall statistics of a partition
class PartitionStats {
  public:
    // TYPES

    /// Enum representing the primary status of a partition.  The values
    /// should remain in that order, so that we favor primary by returning
    /// the maximum observed value over the report interval.
    struct PrimaryStatus {
        // TYPES
        enum Enum { e_UNKNOWN, e_REPLICA, e_PRIMARY };
    };

  private:
    // DATA

    /// StatContext for the partition
    bsl::shared_ptr<bmqst::StatContext> d_statContext_sp;

  private:
    // NOT IMPLEMENTED
    PartitionStats(const PartitionStats&) BSLS_CPP11_DELETED;

    /// Copy constructor and assignment operator are not implemented.
    PartitionStats& operator=(const PartitionStats&) BSLS_CPP11_DELETED;

  public:
    // CREATORS

    /// Create a new object in an uninitialized state.
    explicit PartitionStats(
        const bsl::shared_ptr<bmqst::StatContext>& statContext);

    /// Set the time in nanoseconds it took for the rollover operation to the
    /// specified `value`.
    void setRoloverTime(bsls::Types::Int64 value);

    /// Set the time in nanoseconds it took for the replication of a new entry
    /// in journal file to the specified `value`.
    void setReplicationTime(bsls::Types::Int64 value);

    /// Set the primary status of the partition to the specified `value`.
    void setNodeRole(PrimaryStatus::Enum value);

    /// Set the partition outstanding bytes of the partition data and journal
    /// files to the corresponding specified `outstandingDataBytes`,
    /// `outstandingJournalBytes`, `offsetDataBytes`, `offsetJournalBytes` and
    /// `sequenceNumber` values.
    void setPartitionBytes(bsls::Types::Uint64 outstandingDataBytes,
                           bsls::Types::Uint64 outstandingJournalBytes,
                           bsls::Types::Uint64 offsetDataBytes,
                           bsls::Types::Uint64 offsetJournalBytes,
                           bsls::Types::Uint64 sequenceNumber);

    /// Return a pointer to the statcontext.
    bmqst::StatContext* statContext();
};

// ======================
// class ClusterNodeStats
// ======================

/// Mechanism to keep track of individual overall statistics of a cluster
/// node.
class ClusterNodeStats {
  public:
    // TYPES

    /// Enum representing the various type of events for which statistics
    /// are monitored.
    struct EventType {
        // TYPES
        enum Enum { e_ACK, e_CONFIRM, e_PUT, e_PUSH };
    };

    /// Enum representing the various type of stats that can be obtained
    /// from this object.
    struct Stat {
        // TYPES
        enum Enum {
            e_PUT_MESSAGES_DELTA,
            e_PUT_BYTES_DELTA,
            e_PUT_MESSAGES_ABS,
            e_PUT_BYTES_ABS,
            e_PUSH_MESSAGES_DELTA,
            e_PUSH_BYTES_DELTA,
            e_PUSH_MESSAGES_ABS,
            e_PUSH_BYTES_ABS,
            e_ACK_DELTA,
            e_ACK_ABS,
            e_CONFIRM_DELTA,
            e_CONFIRM_ABS
        };
    };

  private:
    // DATA

    /// StatContext
    bslma::ManagedPtr<bmqst::StatContext> d_statContext_mp;

    // PRIVATE TYPES

    /// Namespace for the constants of stat values that applies to the
    /// cluster node
    struct ClusterNodeStatsIndex {
        enum Enum {
            /// Value:      Number of ack messages delivered to the client
            e_STAT_ACK,
            /// Value:      Number of confirm messages delivered to the client
            e_STAT_CONFIRM,
            /// Value:      Accumulated bytes of all messages ever pushed to
            ///             the client
            /// Increments: Number of messages ever pushed to the client
            e_STAT_PUSH,
            /// Value:      Accumulated bytes of all messages ever received
            ///             from the client
            /// Increments: Number of messages ever received from the client
            e_STAT_PUT
        };
    };

  private:
    // NOT IMPLEMENTED
    ClusterNodeStats(const ClusterNodeStats&) BSLS_CPP11_DELETED;

    /// Copy constructor and assignment operator are not implemented.
    ClusterNodeStats& operator=(const ClusterNodeStats&) BSLS_CPP11_DELETED;

  public:
    // CLASS METHODS

    /// Get the value of the specified `stat` reported to the cluster node
    /// represented by its associated specified `context` as the difference
    /// between the latest snapshot-ed value (i.e., `snapshotId == 0`) and
    /// the value that was recorded at the specified `snapshotId` snapshots
    /// ago.
    ///
    /// THREAD: This method can only be invoked from the `snapshot` thread.
    static bsls::Types::Int64 getValue(const bmqst::StatContext& context,
                                       int                       snapshotId,
                                       const Stat::Enum&         stat);

    // CREATORS

    /// Create a new object in an uninitialized state.
    ClusterNodeStats();

    // MANIPULATORS

    /// Initialize this object for the queue with the specified `uri`, and
    /// register it as a subcontext of the specified
    /// `clusterNodesStatContext`, using the specified `allocator`.
    void initialize(const bmqt::Uri&    uri,
                    bmqst::StatContext* clusterNodesStatContext,
                    bslma::Allocator*   allocator);

    /// Update statistics for the event of the specified `type` and with the
    /// specified `value` (depending on the `type`, `value` can represent
    /// the number of bytes, a counter, ...
    template <EventType::Enum type>
    void onEvent(bsls::Types::Int64 value);

    /// Return a pointer to the statcontext.
    bmqst::StatContext* statContext();
};

// =======================
// struct ClusterStatsUtil
// =======================

/// Utility namespace of methods to initialize cluster stats.
struct ClusterStatsUtil {
    // CLASS METHODS

    /// Initialize the statistics for the cluster stat context, keeping the
    /// specified `historySize` of history.  Return the created top level
    /// stat context to use for all cluster level statistics.  Use the
    /// specified `allocator` for all stat context and stat values.
    static bsl::shared_ptr<bmqst::StatContext>
    initializeStatContextCluster(int historySize, bslma::Allocator* allocator);

    /// Initialize the statistics for the cluster nodes keeping the
    /// specified `historySize` of history: return the created top level
    /// stat context to use as parent of all domains statistics.  Use the
    /// specified `allocator` for all stat context and stat values.
    static bsl::shared_ptr<bmqst::StatContext>
    initializeStatContextClusterNodes(int               historySize,
                                      bslma::Allocator* allocator);
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ------------------
// class ClusterStats
// ------------------

inline void ClusterStats::setHealthStatus(bool value)
{
    d_statContext_mp->setValue(
        ClusterStatsIndex::e_CLUSTER_STATUS,
        value ? ClusterStatus::e_CLUSTER_STATUS_HEALTHY
              : ClusterStatus::e_CLUSTER_STATUS_UNHEALTHY);
}

inline void ClusterStats::setIsLeader(LeaderStatus::Enum value)
{
    d_statContext_mp->setValue(ClusterStatsIndex::e_LEADER_STATUS, value);
}

inline void ClusterStats::setCslReplicationTime(bsls::Types::Int64 value)
{
    d_statContext_mp->reportValue(ClusterStatsIndex::e_CSL_REPLICATION_TIME_NS,
                                  value);
}

inline void ClusterStats::setCslOffsetBytes(bsls::Types::Int64 value)
{
    d_statContext_mp->setValue(ClusterStatsIndex::e_CSL_LOG_OFFSET_BYTES,
                               value);
}

inline void ClusterStats::addCslOffsetBytes(bsls::Types::Int64 delta)
{
    d_statContext_mp->adjustValue(ClusterStatsIndex::e_CSL_WRITE_BYTES, delta);
    d_statContext_mp->adjustValue(ClusterStatsIndex::e_CSL_LOG_OFFSET_BYTES,
                                  delta);
}

inline bmqst::StatContext* ClusterStats::statContext()
{
    return d_statContext_mp.get();
}

// --------------------
// class PartitionStats
// --------------------

inline void PartitionStats::setRoloverTime(bsls::Types::Int64 value)
{
    d_statContext_sp->reportValue(
        ClusterStats::ClusterStatsIndex::e_PARTITION_ROLLOVER_TIME,
        value);
}

inline void PartitionStats::setReplicationTime(bsls::Types::Int64 value)
{
    d_statContext_sp->reportValue(
        ClusterStats::ClusterStatsIndex::e_PARTITION_REPLICATION_TIME_NS,
        value);
}

inline void PartitionStats::setNodeRole(PrimaryStatus::Enum value)
{
    d_statContext_sp->setValue(
        ClusterStats::ClusterStatsIndex::e_PRIMARY_STATUS,
        value);
}

inline void
PartitionStats::setPartitionBytes(bsls::Types::Uint64 outstandingDataBytes,
                                  bsls::Types::Uint64 outstandingJournalBytes,
                                  bsls::Types::Uint64 offsetDataBytes,
                                  bsls::Types::Uint64 offsetJournalBytes,
                                  bsls::Types::Uint64 sequenceNumber)
{
    d_statContext_sp->reportValue(
        ClusterStats::ClusterStatsIndex::e_PARTITION_DATA_BYTES,
        static_cast<bsls::Types::Int64>(outstandingDataBytes));
    d_statContext_sp->reportValue(
        ClusterStats::ClusterStatsIndex::e_PARTITION_JOURNAL_BYTES,
        static_cast<bsls::Types::Int64>(outstandingJournalBytes));
    d_statContext_sp->setValue(
        ClusterStats::ClusterStatsIndex::e_PARTITION_DATA_OFFSET_BYTES,
        static_cast<bsls::Types::Int64>(offsetDataBytes));
    d_statContext_sp->setValue(
        ClusterStats::ClusterStatsIndex::e_PARTITION_JOURNAL_OFFSET_BYTES,
        static_cast<bsls::Types::Int64>(offsetJournalBytes));
    d_statContext_sp->setValue(
        ClusterStats::ClusterStatsIndex::e_PARTITION_SEQUENCE_NUMBER,
        static_cast<bsls::Types::Int64>(sequenceNumber));
}

inline bmqst::StatContext* PartitionStats::statContext()
{
    return d_statContext_sp.get();
}

// ----------------------
// class ClusterNodeStats
// ----------------------

inline bmqst::StatContext* ClusterNodeStats::statContext()
{
    return d_statContext_mp.get();
}

template <>
inline void ClusterNodeStats::onEvent<ClusterNodeStats::EventType::e_ACK>(
    BSLA_UNUSED bsls::Types::Int64 value)
{
    d_statContext_mp->adjustValue(ClusterNodeStatsIndex::e_STAT_ACK, 1);
}

template <>
inline void ClusterNodeStats::onEvent<ClusterNodeStats::EventType::e_CONFIRM>(
    BSLA_UNUSED bsls::Types::Int64 value)
{
    d_statContext_mp->adjustValue(ClusterNodeStatsIndex::e_STAT_CONFIRM, 1);
}

template <>
inline void ClusterNodeStats::onEvent<ClusterNodeStats::EventType::e_PUSH>(
    bsls::Types::Int64 value)
{
    d_statContext_mp->adjustValue(ClusterNodeStatsIndex::e_STAT_PUSH, value);
}

template <>
inline void ClusterNodeStats::onEvent<ClusterNodeStats::EventType::e_PUT>(
    bsls::Types::Int64 value)
{
    d_statContext_mp->adjustValue(ClusterNodeStatsIndex::e_STAT_PUT, value);
}

}  // close package namespace
}  // close enterprise namespace

#endif
