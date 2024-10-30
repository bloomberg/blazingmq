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

//@PURPOSE: Provide concrete impl. of 'mqbc::ClusterStateLedger' interface
//
//@CLASSES:
//  mqbc::IncoreClusterStateLedger: Replicated log of cluster's state
//
//@DESCRIPTION: The 'mqbc::IncoreClusterStateLedger' class is a concrete
// implementation of the 'mqbc::ClusterStateLedger' base protocol to maintain a
// replicated log of cluster's state.  The ledger is written to only upon
// notification from the leader node, and the leader broadcasts advisories
// which followers apply to their ledger and its in-memory representation.
// Leader will apply update to self's ledger, broadcast it asynchronously to
// cluster nodes, and also advertise the update to cluster state's observers
// when appropriate consistency level has been achieved.  Note that leader
// advisories are acknowledged individually by follower nodes and committed
// individually by the leader (as opposed to in batches or chunks).  The
// desired consistency level (eventual vs. strong) is configured by the user.
// This component is in-core because cluster state is persisted, replicated and
// maintained by BlazingMQ cluster nodes themselves instead of being offloaded
// to an external meta data server (e.g., ZooKeeper).
//
/// Thread Safety
///-------------
// The 'mqbc::IncoreClusterStateLedger' object is not thread safe and should
// always be manipulated from the associated cluster's dispatcher thread,
// unless explicitly documented in a method's contract.

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
#include <bmqp_ctrlmsg_messages.h>

#include <bmqc_orderedhashmap.h>
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
/// state ledger (record id, number of acknowledgements received, etc.).
struct IncoreClusterStateLedger_ClusterMessageInfo {
    bmqp_ctrlmsg::ClusterMessage d_clusterMessage;
    // Cluster message, one of:
    // o 'PartitionPrimaryAdvisory',
    // o 'QueueAssignmentAdvisory',
    // o 'LeaderAdvisory',
    // o 'QueueUnassignedAdvisory'

    mqbsi::LedgerRecordId d_recordId;
    // Record id stored in the ledger and
    // associated with the 'ClusterMessage'

    int d_ackCount;
    // Number of ACKs received for this
    // ClusterMessage

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
/// `mqbc::ClusterStateLedger` to maintain a replicated log of cluster's
/// state.  A concrete implementation can be configured with the desired
/// consistency level.  A leader node will apply update to self's ledger,
/// broadcast it asynchronously, and advertise when the configured
/// consistency level has been achieved.  This component is in-core because
/// cluster state is persisted, replicated, and maintained by BlazingMQ
/// cluster nodes themselves instead of being offloaded to an external meta
/// data (e.g., ZooKeeper).
class IncoreClusterStateLedger BSLS_KEYWORD_FINAL : public ClusterStateLedger {
  public:
    // TYPES
    typedef bmqp::BlobPoolUtil::BlobSpPool BlobSpPool;

  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBC.INCORECLUSTERSTATELEDGER");

    // TYPES
    typedef IncoreClusterStateLedger_ClusterMessageInfo ClusterMessageInfo;
    typedef ClusterStateLedgerCommitStatus              CommitStatus;

    /// Map from a `LeaderMessageSequence` to cluster message and its
    /// associated information.
    ///
    /// sequenceNumber -> {clusterMessage, recordId, ackCount}
    typedef bmqc::OrderedHashMap<bmqp_ctrlmsg::LeaderMessageSequence,
                                 ClusterMessageInfo>
                                    AdvisoriesMap;
    typedef AdvisoriesMap::iterator AdvisoriesMapIter;

  private:
    // DATA
    bslma::Allocator* d_allocator_p;
    // Allocator used to supply memory

    bool d_isFirstLeaderAdvisory;
    // Flag to indicate whether this is
    // first leader advisory.  *NOTE*: this
    // flag is a workaround to address the
    // existing cyclic dependency b/w
    // leader and primary at node startup
    // and will be removed once all CSL
    // phases are complete.

    bool d_isOpen;
    // Flag to indicate open/close status
    // of this object

    bdlbb::BlobBufferFactory* d_bufferFactory_p;
    // Buffer factory for the headers and
    // payloads of the messages to be
    // written

    /// Pool of shared pointers to blobs
    BlobSpPool* d_blobSpPool_p;

    bsl::string d_description;
    // Brief description for logging

    CommitCb d_commitCb;
    // Callback invoked when the status of
    // a commit operation becomes
    // available.

    ClusterData* d_clusterData_p;
    // Cluster's transient state

    const ClusterState* d_clusterState_p;
    // Cluster's state

    ClusterStateLedgerConsistency::Enum d_consistencyLevel;
    // desired consistency level (eventual
    // vs. strong), configured by the user.

    int d_ackQuorum;
    // Number of nodes required to achieve
    // consistency level.

    mqbsi::LedgerConfig d_ledgerConfig;
    // Ledger configuration

    bslma::ManagedPtr<mqbsi::Ledger> d_ledger_mp;
    // Ledger owned by this object

    AdvisoriesMap d_uncommittedAdvisories;
    // Map of uncommitted (but not
    // canceled) advisories and associated
    // record id from leader message
    // sequence number.

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
    /// specified `oldLogId` (if any) and `newLogId`.  Return 0 on success
    /// and non-zero error value otherwise.
    int onLogRolloverCb(const mqbu::StorageKey& oldLogId,
                        const mqbu::StorageKey& newLogId);

    /// Internal helper method to apply the advisory in the specified
    /// `clusterMessage`, of the specified `recordType` and identified by
    /// the specified `sequenceNumber`.  The behavior is undefined unless
    /// the `clusterMessage` is instantiated with the appropriate advisory.
    /// Note that *only* a leader node may invoke this routine.
    int applyAdvisoryInternal(
        const bmqp_ctrlmsg::ClusterMessage&        clusterMessage,
        const bmqp_ctrlmsg::LeaderMessageSequence& sequenceNumber,
        ClusterStateRecordType::Enum               recordType);

    /// Internal helper method to apply the specified raw `record` at the
    /// specified `recordOffset` and/or `recordPosition`, containing the
    /// specified `clusterMessage` and having the specified `sequenceNumber`
    /// and `recordType`.  Note that the `record` can either be generated at
    /// self node or received from a peer node.
    int applyRecordInternal(
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

    /// Cancel all uncommitted advisories.  Called upon new leader or term,
    /// or when this ledger is closed.
    void cancelUncommittedAdvisories();

    /// Apply the specified raw cluster state record `event` received from
    /// the specified `source` node according to the specified `delayed`
    /// flag indicating if the event was previously buffered.  Note that
    /// while a replica node may receive any type of records from the
    /// leader, the leader may *only* receive ack records from a replica.
    ///
    /// THREAD: This method can be invoked only in the associated cluster's
    ///         dispatcher thread.
    int applyImpl(const bdlbb::Blob&   event,
                  mqbnet::ClusterNode* source,
                  bool                 delayed);

    // PRIVATE ACCESSORS

    /// Return true if self node is the leader, false otherwise.
    ///
    /// THREAD: This method can be invoked only in the associated cluster's
    ///         dispatcher thread.
    bool isSelfLeader() const;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(IncoreClusterStateLedger,
                                   bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a new `mqbc::IncoreClusterStateLedger` with the specified
    /// `clusterDefinition`, `consistencyLevel`, `clusterData` and
    /// `clusterState`, and using the specified `bufferFactory` and
    /// `allocator` to supply memory.
    IncoreClusterStateLedger(
        const mqbcfg::ClusterDefinition&    clusterDefinition,
        ClusterStateLedgerConsistency::Enum consistencyLevel,
        ClusterData*                        clusterData,
        ClusterState*                       clusterState,
        bdlbb::BlobBufferFactory*           bufferFactory,
        BlobSpPool*                         blobSpPool_p,
        bslma::Allocator*                   allocator);

    /// Destructor.
    ~IncoreClusterStateLedger() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS
    //   (virtual mqbc::ElectorInfoObserver)

    /// Callback invoked when the cluster's leader changes to the specified
    /// `node` with specified `status`.  Note that null is a valid value for
    /// the `node`, and it implies that the cluster has transitioned to a
    /// state of no leader, and in this case, `status` will be `UNDEFINED`.
    ///
    /// THREAD: This method can be invoked only in the associated cluster's
    ///         dispatcher thread.
    void onClusterLeader(mqbnet::ClusterNode*          node,
                         ElectorInfoLeaderStatus::Enum status)
        BSLS_KEYWORD_OVERRIDE;

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

    // TODO: Declare these methods once the parameter object types have been
    //       defined in 'mqbc::ClusterState'.
    // virtual int apply(const ClusterStateQueueInfo& queueInfo) = 0;
    // virtual int apply(const UriToQueueInfoMap& queuesInfo) = 0;
    // virtual int apply(const ClusterStatePartitionInfo& partitionInfo) = 0;
    // virtual int apply(const PartitionsInfo& partitionsInfo) = 0;
    // Apply the specified message to self and replicate if self is leader.
    // Notify via 'commitCb' when consistency level has been achieved.

    int apply(const bmqp_ctrlmsg::PartitionPrimaryAdvisory& advisory)
        BSLS_KEYWORD_OVERRIDE;
    int apply(const bmqp_ctrlmsg::QueueAssignmentAdvisory& advisory)
        BSLS_KEYWORD_OVERRIDE;
    int apply(const bmqp_ctrlmsg::QueueUnassignedAdvisory& advisory)
        BSLS_KEYWORD_OVERRIDE;
    int apply(const bmqp_ctrlmsg::QueueUpdateAdvisory& advisory)
        BSLS_KEYWORD_OVERRIDE;

    /// Apply the specified `advisory` to self and replicate if self is
    /// leader.  Notify via `commitCb` when consistency level has been
    /// achieved.  Note that *only* a leader node may invoke this routine.
    ///
    /// THREAD: This method can be invoked only in the associated cluster's
    ///         dispatcher thread.
    int
    apply(const bmqp_ctrlmsg::LeaderAdvisory& advisory) BSLS_KEYWORD_OVERRIDE;

    /// Apply the advisory contained in the specified `clusterMessage` to
    /// self and replicate to followers.  Notify via `commitCb` when
    /// consistency level has been achieved.  Note that *only* a leader node
    /// may invoke this routine.  Behavior is undefined unless the contained
    /// advisory is one of `PartitionPrimaryAdvisory`,
    /// `QueueAssignmentAdvisory`, `QueueUnassignedAdvisory`.
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

    void
    setIsFirstLeaderAdvisory(bool isFirstLeaderAdvisory) BSLS_KEYWORD_OVERRIDE;

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
, d_recordId()
, d_ackCount(0)
{
    // NOTHING
}

inline IncoreClusterStateLedger_ClusterMessageInfo ::
    IncoreClusterStateLedger_ClusterMessageInfo(
        const IncoreClusterStateLedger_ClusterMessageInfo& other,
        bslma::Allocator*                                  allocator)
: d_clusterMessage(other.d_clusterMessage, allocator)
, d_recordId(other.d_recordId)
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
    BSLS_ASSERT_SAFE(
        d_clusterData_p->cluster().dispatcher()->inDispatcherThread(
            &d_clusterData_p->cluster()));

    return d_clusterData_p->electorInfo().isSelfLeader();
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
    BSLS_ASSERT_SAFE(
        d_clusterData_p->cluster().dispatcher()->inDispatcherThread(
            &d_clusterData_p->cluster()));

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
    BSLS_ASSERT_SAFE(
        d_clusterData_p->cluster().dispatcher()->inDispatcherThread(
            &d_clusterData_p->cluster()));

    return d_ledger_mp.get();
}

}  // close package namespace
}  // close enterprise namespace

#endif
