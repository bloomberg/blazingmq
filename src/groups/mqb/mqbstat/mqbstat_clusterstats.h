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

// ==================
// class ClusterStats
// ==================

/// Mechanism to keep track of individual overall statistics of a cluster
class ClusterStats {
  public:
    // TYPES

    /// Enum representing the various types of events for which statistics
    /// are monitored.
    struct PartitionEventType {
        // TYPES
        enum Enum {
            /// Time in nanoseconds it took for the rollover operation.
            e_PARTITION_ROLLOVER
        };
    };

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
            e_PARTITION_CFG_DATA_BYTES,
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
            /// Maximum observed utilization of the data file of the partition.
            e_PARTITION_DATA_UTILIZATION_MAX,
            /// Maximum observed utilization of the journal file of the
            /// partition.
            e_PARTITION_JOURNAL_UTILIZATION_MAX
        };
    };

    /// Enum representing the leader status (leader or follower) of a node.
    struct LeaderStatus {
        // TYPES
        enum Enum { e_FOLLOWER, e_LEADER };
    };

    /// Enum representing the primary status of a partition.  The values
    /// should remain in that order, so that we favor primary by returning
    /// the maximum observed value over the report interval.
    struct PrimaryStatus {
        // TYPES
        enum Enum { e_UNKNOWN, e_REPLICA, e_PRIMARY };
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
    // DATA

    /// StatContext for the cluster
    bslma::ManagedPtr<bmqst::StatContext> d_statContext_mp;

    /// StatContext for each partition in
    /// the cluster, indexed by the
    /// partition id.  Those statContext
    /// are created as children of the
    /// above 'd_StatContext_mp'.
    bsl::vector<bsl::shared_ptr<bmqst::StatContext> > d_partitionsStatContexts;

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

    // CREATORS

    /// Create a new object in an uninitialized state.
    ClusterStats(bslma::Allocator* allocator);

    // MANIPULATORS

    /// Initialize this object for the cluster with the specified `name`
    /// having the specified `partitionsCount` partitions, and register it
    /// as a subcontext of the specified `clusterStatContext` and using the
    /// specified `allocator`.
    void initialize(const bsl::string&  name,
                    int                 partitionsCount,
                    bmqst::StatContext* clusterStatContext,
                    bslma::Allocator*   allocator);

    /// Update statistics for the event of the specified `type` for the
    /// specified `partitionId` and with the specified `value` (depending on
    /// the `type`, `value` can represent the number of bytes, a counter,
    /// some nanoseconds elapsed, ...).
    void onPartitionEvent(PartitionEventType::Enum type,
                          int                      partitionId,
                          bsls::Types::Int64       value);

    /// Update the `cluster_status` field of the StatContext being referred
    /// to by this object to be the specified `value`.
    ClusterStats& setHealthStatus(bool value);

    /// Update the `cluster_role` field of the StatContext being referred
    /// to by this object to be the specified `value`.
    ClusterStats& setIsMember(bool value);

    /// Update the `upstream` field of the StatContext being referred
    /// to by this object to be the specified `value`.
    ClusterStats& setUpstream(const bsl::string& value);

    /// Update the `cluster_leader` field of the StatContext being referred
    /// to by this object to be the specified `value`.
    ClusterStats& setIsLeader(LeaderStatus::Enum value);

    /// Update the `partition.cfg_data_bytes` field to the specified
    /// `dataBytes` and the `partition.cfg_journal_bytes` field to the
    /// specified `journalBytes` in the StatContext being referred to by
    /// this object.
    ClusterStats& setPartitionCfgBytes(bsls::Types::Int64 dataBytes,
                                       bsls::Types::Int64 journalBytes);

    /// Set the primary status of the specified `partitionId` to the
    /// specified `value`.
    ClusterStats& setNodeRoleForPartition(int                 partitionId,
                                          PrimaryStatus::Enum value);

    /// Set the partition outstanding bytes of the specified data and
    /// journal files for the specified `partitionId` to the corresponding
    /// specified `dataBytes` and `journalBytes` values.
    ClusterStats&
    setPartitionOutstandingBytes(int                partitionId,
                                 bsls::Types::Int64 dataBytes,
                                 bsls::Types::Int64 journalBytes);

    /// Return a pointer to the statcontext.
    bmqst::StatContext* statContext();
};


    /// StatContext
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

inline bmqst::StatContext* ClusterStats::statContext()
{
    return d_statContext_mp.get();
}

}  // close package namespace
}  // close enterprise namespace

#endif
