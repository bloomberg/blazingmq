// Copyright 2019-2023 Bloomberg Finance L.P.
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

// mqbc_incoreclusterstateledger.h                                    -*-C++-*-
#ifndef INCLUDED_MQBC_INCORECLUSTERSTATELEDGER
#define INCLUDED_MQBC_INCORECLUSTERSTATELEDGER

/// @file mqbc_incoreclusterstateledger.h
///
/// @brief Provide a concrete implementation of
///        @bbref{mqbc::ClusterStateLedger}.
///
/// The @bbref{mqbc::IncoreClusterStateLedger} class is a concrete
/// implementation of the @bbref{mqbc::ClusterStateLedger} base protocol to
/// maintain a replicated log of cluster's state.  The ledger is written to
/// only upon notification from the leader node, and the leader broadcasts
/// advisories which followers apply to their ledger and its in-memory
/// representation.  Leader will apply update to self's ledger, broadcast it
/// asynchronously to cluster nodes, and also advertise the update to cluster
/// state's observers when appropriate consistency level has been achieved.
/// Note that leader advisories are acknowledged individually by follower nodes
/// and committed individually by the leader (as opposed to in batches or
/// chunks).  The desired consistency level (eventual vs. strong) is configured
/// by the user.  This component is in-core because cluster state is persisted,
/// replicated and maintained by BlazingMQ cluster nodes themselves instead of
/// being offloaded to an external meta data server (e.g., ZooKeeper).
///
/// Thread Safety                       {#mqbc_incoreclusterstateledger_thread}
/// =============
///
/// The @bbref{mqbc::IncoreClusterStateLedger} object is not thread safe and
/// should always be manipulated from the associated cluster's dispatcher
/// thread, unless explicitly documented in a method's contract.

// MQB
#include <mqbc_clusterdata.h>
#include <mqbc_clusterstate.h>
#include <mqbc_clusterstateledger.h>
#include <mqbc_clusterstateledgerprotocol.h>
#include <mqbcfg_messages.h>
#include <mqbi_cluster.h>
#include <mqbi_dispatcher.h>
#include <mqbsi_ledger.h>

// BMQ
#include <bmqc_orderedhashmap.h>
#include <bmqp_ctrlmsg_messages.h>
#include <bmqu_blob.h>

// BDE
#include <ball_log.h>
#include <bdlbb_blob.h>
#include <bsl_string.h>
#include <bsl_unordered_map.h>
#include <bslma_allocator.h>
#include <bslma_default.h>
#include <bslma_managedptr.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_keyword.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace mqbnet {
class ClusterNode;
}

namespace mqbc {

/// Struct holding a cluster message and its associated state in the cluster
/// state ledger (replication timestamp, number of acknowledgements received,
/// etc.).
struct IncoreClusterStateLedger_ClusterMessageInfo {
    /// Cluster message, one of:
    ///   - `PartitionPrimaryAdvisory`,
    ///   - `QueueAssignmentAdvisory`,
    ///   - `LeaderAdvisory`,
    ///   - `QueueUnAssignmentAdvisory`.
    bmqp_ctrlmsg::ClusterMessage d_clusterMessage;

    /// Timestamp in nanoseconds at the moment when the replication of this
    /// `ClusterMessage` started.
    bsls::Types::Uint64 d_timestampNs;

    /// Number of ACKs received for this `ClusterMessage`.
    unsigned int d_ackCount;

    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(IncoreClusterStateLedger_ClusterMessageInfo,
                                   bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a new `ClusterMessageInfo` using the optionally specified
    /// `allocator`.
    explicit IncoreClusterStateLedger_ClusterMessageInfo(
        bslma::Allocator* allocator = 0);

    /// Create a new `ClusterMessageInfo` having the same value as the
    /// specified `other`, and using the optionally specified `allocator`.
    IncoreClusterStateLedger_ClusterMessageInfo(
        const IncoreClusterStateLedger_ClusterMessageInfo& other,
        bslma::Allocator*                                  allocator);
};

/// Provide a concrete in-core implementation of the base protocol
/// @bbref{mqbc::ClusterStateLedger} to maintain a replicated log of cluster's
/// state.  A concrete implementation can be configured with the desired
/// consistency level.  A leader node will apply update to self's ledger,
/// broadcast it asynchronously, and advertise when the configured consistency
/// level has been achieved.  This component is in-core because cluster state
/// is persisted, replicated, and maintained by BlazingMQ cluster nodes
/// themselves instead of being offloaded to an external meta data (e.g.,
/// ZooKeeper).
///
/// @todo Declare these methods once the parameter object types have been
/// defined in @bbref{mqbc::ClusterState}.
/// ```
/// virtual int apply(const ClusterStateQueueInfo& queueInfo) = 0;
/// virtual int apply(const UriToQueueInfoMap& queuesInfo) = 0;
/// virtual int apply(const ClusterStatePartitionInfo& partitionInfo) = 0;
/// virtual int apply(const PartitionsInfo& partitionsInfo) = 0;
/// ```
///
/// @todo Apply the specified message to self and replicate if self is leader.
///
/// @todo Notify via `commitCb` when consistency level has been achieved.
class IncoreClusterStateLedger BSLS_KEYWORD_FINAL : public ClusterStateLedger {
  public:
    // TYPES
    typedef bmqp::BlobPoolUtil::BlobSpPool BlobSpPool;

    typedef IncoreClusterStateLedger_ClusterMessageInfo ClusterMessageInfo;

    /// Map from a `LeaderMessageSequence` to cluster message and its
    /// associated information.
    ///
    /// `sequenceNumber -> {clusterMessage, replicationTimestamp, ackCount}`
    typedef bmqc::OrderedHashMap<bmqp_ctrlmsg::LeaderMessageSequence,
                                 ClusterMessageInfo>
                                          AdvisoriesMap;
    typedef AdvisoriesMap::const_iterator AdvisoriesMapCIter;
    typedef AdvisoriesMap::iterator       AdvisoriesMapIter;

  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBC.INCORECLUSTERSTATELEDGER");

    // TYPES
    typedef ClusterStateLedgerCommitStatus CommitStatus;

  private:
    // DATA

    /// Allocator used to supply memory.
    bslma::Allocator* d_allocator_p;

    /// Flag to indicate open/close status of this object.
    bool d_isOpen;

    /// Pool of shared pointers to blobs.
    BlobSpPool* d_blobSpPool_p;

    // Brief description for logging.
    bsl::string d_description;

    /// Callback invoked when the status of a commit operation becomes
    /// available.
    CommitCb d_commitCb;

    /// Cluster's transient state.
    ClusterData* d_clusterData_p;

    /// Cluster's state.
    const ClusterState* d_clusterState_p;

    /// Ledger configuration.
    mqbsi::LedgerConfig d_ledgerConfig;

    /// Ledger owned by this object.
    bslma::ManagedPtr<mqbsi::Ledger> d_ledger_mp;

    /// Map of uncommitted (but not canceled) advisories and associated record
    /// id from leader message sequence number.
    AdvisoriesMap d_uncommittedAdvisories;

  private:
    // NOT IMPLEMENTED
    IncoreClusterStateLedger(const IncoreClusterStateLedger&)
        BSLS_KEYWORD_DELETED;
    IncoreClusterStateLedger&
    operator=(const IncoreClusterStateLedger&) BSLS_KEYWORD_DELETED;

  private:
    // PRIVATE MANIPULATORS

    /// Callback invoked to perform cleanup after closing (or failing to
    /// open/validate) the log located at the specified `logPath`.  Return 0
    /// on success and non-zero error value otherwise.  Note that this
    /// callback will be invoked by the ledger after it closes the old log
    /// during rollover to a new one.
    int cleanupLog(const bsl::string& logPath);

    /// Callback invoked upon internal rollover to a new log, providing the
    /// specified `oldLogId` (if any) and `newLogId`.  Return 0 on success and
    /// non-zero error value otherwise.
    int onLogRolloverCb(const mqbu::StorageKey& oldLogId,
                        const mqbu::StorageKey& newLogId);

    /// Callback invoked when the cluster's quorum changes to the specified new
    /// value `ackQuorum`.
    void onQuorumChangeCb(const unsigned int ackQuorum);

    /// Internal helper method to apply the advisory in the specified
    /// `clusterMessage`, of the specified `recordType` and identified by
    /// the specified `sequenceNumber`.  Notify via `commitCb` when consistency
    /// level has been achieved.  The behavior is undefined unless the
    /// `clusterMessage` is instantiated with the appropriate advisory.  Note
    /// that *only* a leader node may invoke this routine.
    int applyAdvisoryInternal(
        const bmqp_ctrlmsg::ClusterMessage&        clusterMessage,
        const bmqp_ctrlmsg::LeaderMessageSequence& sequenceNumber,
        ClusterStateRecordType::Enum               recordType);

    /// Internal helper method to apply the specified raw `record` at the
    /// specified `recordOffset` and/or `recordPosition`, containing the
    /// specified `clusterMessage` and having the specified `sequenceNumber`
    /// and `recordType`.  Notify via `commitCb` when consistency level has
    /// been achieved.  Note that the `record` can either be generated at self
    /// node or received from a peer node.
    int applyRecordInternalImpl(
        const bdlbb::Blob&                         record,
        int                                        recordOffset,
        const bmqu::BlobPosition&                  recordPosition,
        const bmqp_ctrlmsg::ClusterMessage&        clusterMessage,
        const bmqp_ctrlmsg::LeaderMessageSequence& sequenceNumber,
        ClusterStateRecordType::Enum               recordType);
    int applyRecordInternal(
        const bdlbb::Blob&                         record,
        int                                        recordOffset,
        const bmqp_ctrlmsg::ClusterMessage&        clusterMessage,
        const bmqp_ctrlmsg::LeaderMessageSequence& sequenceNumber,
        ClusterStateRecordType::Enum               recordType);
    int applyRecordInternal(
        const bdlbb::Blob&                         record,
        const bmqu::BlobPosition&                  recordPosition,
        const bmqp_ctrlmsg::ClusterMessage&        clusterMessage,
        const bmqp_ctrlmsg::LeaderMessageSequence& sequenceNumber,
        ClusterStateRecordType::Enum               recordType);

    /// Internal helper method to apply commit for the advisory with the
    /// specified `sequenceNumber` as the specified `ackQuorum` is reached.
    int applyCommit(const bmqp_ctrlmsg::LeaderMessageSequence& sequenceNumber,
                    const unsigned int                         ackQuorum);

    /// Cancel all uncommitted advisories.  Called upon new leader or term,
    /// or when this ledger is closed.
    ///
    /// THREAD: This method can be invoked only in the associated cluster's
    ///         dispatcher thread.
    void cancelUncommittedAdvisories();

    /// Review all uncommitted advisories and commit those which have more acks
    /// then the new `ackQuorum`. Called upon quorum change.
    ///
    /// THREAD: This method can be invoked only in the associated cluster's
    ///         dispatcher thread.
    void reviewUncommittedAdvisories(const unsigned int ackQuorum);

    /// Apply the specified raw cluster state record `event` received from
    /// the specified `source` node.  Note that while a replica node may
    /// receive any type of records from the leader, the leader may *only*
    /// receive ack records from a replica.
    ///
    /// THREAD: This method can be invoked only in the associated cluster's
    ///         dispatcher thread.
    int applyImpl(const bdlbb::Blob& event, mqbnet::ClusterNode* source);

    // PRIVATE ACCESSORS

    /// Return true if self node is the leader, false otherwise.
    ///
    /// THREAD: This method can be invoked only in the associated cluster's
    ///         dispatcher thread.
    bool isSelfLeader() const;

    /// Return the acknowledgment quorum required for this ledger.
    unsigned int getAckQuorum() const;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(IncoreClusterStateLedger,
                                   bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a new @bbref{mqbc::IncoreClusterStateLedger} with the specified
    /// `clusterDefinition`, `clusterData` and
    /// `clusterState`, and using the specified `bufferFactory` and `allocator`
    /// to supply memory.
    IncoreClusterStateLedger(
        const mqbcfg::ClusterDefinition& clusterDefinition,
        ClusterData*                     clusterData,
        ClusterState*                    clusterState,
        BlobSpPool*                      blobSpPool_p,
        bslma::Allocator*                allocator);

    /// Destructor.
    ~IncoreClusterStateLedger() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS
    //   (virtual mqbc::ClusterStateLedger)

    /// Initialize the ledger, and return 0 on success or a non-zero value
    /// otherwise.  This method must be called first before any other
    /// method, or it can cause undefined behavior.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    int open() BSLS_KEYWORD_OVERRIDE;

    /// Close the ledger, and return 0 on success or a non-zero value
    /// otherwise.
    ///
    /// THREAD: This method can be invoked only in the associated cluster's
    ///         dispatcher thread.
    int close() BSLS_KEYWORD_OVERRIDE;

    /// @{
    /// Apply the specified `advisory` to self and replicate to followers.
    /// Notify via `commitCb` when consistency level has been achieved.  Note
    /// that *only* a leader node may invoke this routine.
    ///
    /// THREAD: This method can be invoked only in the associated cluster's
    ///         dispatcher thread.
    int apply(const bmqp_ctrlmsg::PartitionPrimaryAdvisory& advisory)
        BSLS_KEYWORD_OVERRIDE;
    int apply(const bmqp_ctrlmsg::QueueAssignmentAdvisory& advisory)
        BSLS_KEYWORD_OVERRIDE;
    int apply(const bmqp_ctrlmsg::QueueUnAssignmentAdvisory& advisory)
        BSLS_KEYWORD_OVERRIDE;
    int apply(const bmqp_ctrlmsg::QueueUpdateAdvisory& advisory)
        BSLS_KEYWORD_OVERRIDE;
    int
    apply(const bmqp_ctrlmsg::LeaderAdvisory& advisory) BSLS_KEYWORD_OVERRIDE;
    /// @}

    /// Apply the advisory contained in the specified `clusterMessage` to
    /// self and replicate to followers.  Notify via `commitCb` when
    /// consistency level has been achieved.  Note that *only* a leader node
    /// may invoke this routine.  Behavior is undefined unless the contained
    /// advisory is one of `PartitionPrimaryAdvisory`,
    /// `QueueAssignmentAdvisory`, `QueueUnAssignmentAdvisory`.
    /// `QueueUpdateAdvisory` or `LeaderAdvisory`.
    ///
    /// THREAD: This method can be invoked only in the associated cluster's
    ///         dispatcher thread.
    int apply(const bmqp_ctrlmsg::ClusterMessage& clusterMessage)
        BSLS_KEYWORD_OVERRIDE;

    /// Apply the specified raw cluster state record `event` received from
    /// the specified `source` node.  Note that while a replica node may
    /// receive any type of records from the leader, the leader may *only*
    /// receive ack records from a replica.
    ///
    /// THREAD: This method can be invoked only in the associated cluster's
    ///         dispatcher thread.
    int apply(const bdlbb::Blob&   event,
              mqbnet::ClusterNode* source) BSLS_KEYWORD_OVERRIDE;

    /// Set the commit callback to the specified `value`.
    void setCommitCb(const CommitCb& value) BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS
    //   (virtual mqbc::ClusterStateLedger)

    /// Return true if this ledger is opened, false otherwise.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    bool isOpen() const BSLS_KEYWORD_OVERRIDE;

    /// Return an iterator to this ledger.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    bslma::ManagedPtr<ClusterStateLedgerIterator>
    getIterator() const BSLS_KEYWORD_OVERRIDE;

    /// Load into `out` the list of uncommitted advisories as const references.
    ///
    /// THREAD: This method can be invoked only in the associated cluster's
    ///         dispatcher thread.
    void uncommittedAdvisories(ClusterMessageCRefList* out) const
        BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS

    /// Return a brief description of the ClusterStateLedger for logging
    /// purposes.
    const bsl::string& description() const;

    /// Return the underlying ledger.
    ///
    /// THREAD: This method can be invoked only in the associated cluster's
    ///         dispatcher thread.
    const mqbsi::Ledger* ledger() const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// --------------------------------------------------
// struct IncoreClusterStateLedger_ClusterMessageInfo
// --------------------------------------------------

// CREATORS
inline IncoreClusterStateLedger_ClusterMessageInfo ::
    IncoreClusterStateLedger_ClusterMessageInfo(bslma::Allocator* allocator)
: d_clusterMessage(bslma::Default::allocator(allocator))
, d_timestampNs(0)
, d_ackCount(0)
{
    // NOTHING
}

inline IncoreClusterStateLedger_ClusterMessageInfo ::
    IncoreClusterStateLedger_ClusterMessageInfo(
        const IncoreClusterStateLedger_ClusterMessageInfo& other,
        bslma::Allocator*                                  allocator)
: d_clusterMessage(other.d_clusterMessage, allocator)
, d_timestampNs(other.d_timestampNs)
, d_ackCount(other.d_ackCount)
{
    // NOTHING
}

// ------------------------------
// class IncoreClusterStateLedger
// ------------------------------

// PRIVATE ACCESSORS
inline bool IncoreClusterStateLedger::isSelfLeader() const
{
    // executed by the *CLUSTER DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_clusterData_p->cluster().inDispatcherThread());

    return d_clusterData_p->electorInfo().isSelfLeader();
}

inline unsigned int IncoreClusterStateLedger::getAckQuorum() const
{
    return d_clusterData_p->quorumManager().quorum();
}

// MANIPULATORS
//   (virtual mqbc::ClusterStateLedger)
inline void IncoreClusterStateLedger::setCommitCb(const CommitCb& value)
{
    d_commitCb = value;
}

// ACCESSORS
//   (virtual mqbc::ClusterStateLedger)
inline bool IncoreClusterStateLedger::isOpen() const
{
    // executed by the *CLUSTER DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_clusterData_p->cluster().inDispatcherThread());

    return d_isOpen;
}

// ACCESSORS
inline const bsl::string& IncoreClusterStateLedger::description() const
{
    return d_description;
}

inline const mqbsi::Ledger* IncoreClusterStateLedger::ledger() const
{
    // executed by the *CLUSTER DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_clusterData_p->cluster().inDispatcherThread());

    return d_ledger_mp.get();
}

}  // close package namespace
}  // close enterprise namespace

#endif
