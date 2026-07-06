// Copyright 2025-2026 Bloomberg Finance L.P.
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

// mqbraft_partitionraft.h -*-C++-*-
#ifndef INCLUDED_MQBRAFT_PARTITIONRAFT
#define INCLUDED_MQBRAFT_PARTITIONRAFT

//@PURPOSE: Provide glue between RaftNode and BlazingMQ partition
// infrastructure for journal+data replication.
//
//@CLASSES:
//  mqbraft::PartitionRaft: Manages RaftNode + PartitionRaftLog for one
//                          partition.
//
//@DESCRIPTION: This component wires 'RaftNode' into the partition dispatcher,
// translating between 'RaftNodeOutput' messages and the network
// ('e_RAFT_PARTITION' events and control messages), and notifying
// 'FileStore' of committed entries.
//
/// Threading
///----------
// This component is NOT thread-safe.  All methods except 'start()' and
// 'stop()' must be called from the partition's dispatcher thread.

// MQB
#include <mqbc_clusterdata.h>
#include <mqbraft_partitionraftlog.h>
#include <mqbraft_raftnode.h>
#include <mqbs_datastore.h>
#include <mqbs_filestoreprotocol.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_protocol.h>

// BDE
#include <ball_log.h>
#include <bdlbb_blob.h>
#include <bdlmt_eventscheduler.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_types.h>

namespace BloombergLP {

namespace mqbnet {
class ClusterNode;
}

namespace mqbs {
class FileStore;
}

namespace mqbraft {

// ====================
// class PartitionRaft
// ====================

class PartitionRaft : public mqbs::RecordStore {
  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBRAFT.PARTITIONRAFT");

    // DATA
    int                                         d_partitionId;
    bsl::shared_ptr<mqbs::FileStore>            d_fileStore_sp;
    mqbc::ClusterData*                          d_clusterData_p;
    mqbs::StoragesMonitor*                      d_storagesMonitor_p;
    bslma::ManagedPtr<PartitionRaftLog>         d_raftLog_mp;
    bslma::ManagedPtr<RaftNode>                 d_raftNode_mp;
    bdlmt::EventScheduler::RecurringEventHandle d_tickHandle;
    bsls::Types::Uint64                         d_writeIdCounter;

    /// Index of the most recently proposed `e_ROLLOVER` entry (0 if none).
    /// A new rollover must not be proposed while this one is still
    /// uncommitted (`RaftNode::commitIndex() < d_uncommittedRolloverIndex`),
    /// because the single-scalar `e_ROLLOVER` resend slot in
    /// `PartitionRaftLog` can hold only one at a time.  Self-clearing: once
    /// `commitIndex` reaches it, the gate opens.
    bsls::Types::Uint64 d_uncommittedRolloverIndex;

    bool                                        d_isStarted;
    bslma::Allocator*                           d_allocator_p;

    // Snapshot transfer state (receiver side)
    bool                d_receivingSnapshot;
    int                 d_snapshotJournalFd;
    int                 d_snapshotDataFd;
    int                 d_snapshotQlistFd;
    bsl::string         d_snapshotJournalPath;
    bsl::string         d_snapshotDataPath;
    bsl::string         d_snapshotQlistPath;
    bsls::Types::Uint64 d_snapshotLastIncludedIndex;
    bsls::Types::Uint64 d_snapshotLastIncludedTerm;

    // NOT IMPLEMENTED
    PartitionRaft(const PartitionRaft&);
    PartitionRaft& operator=(const PartitionRaft&);

    // PRIVATE MANIPULATORS

    /// Dispatch RaftNode output: send messages to peers and notify FileStore
    /// of committed entries.
    void dispatchOutput(RaftNodeOutput* output);

    /// Propose a sync-point log entry under the current term.  Called once
    /// upon becoming leader: per Raft \S5.4.2, a leader cannot advance
    /// 'commitIndex' over entries from prior terms by counting replicas
    /// alone; committing this current-term entry indirectly commits the
    /// inherited (prior-term) tail via the Log Matching Property.  This is
    /// also the Raft analog of legacy's "issue a sync point upon becoming
    /// active primary", and doubles as a legacy-recovery checkpoint.
    void proposeSyncPoint();

    /// Propose an `e_ROLLOVER` sync-point log entry under the current term and
    /// then drive the file rollover (`PartitionRaftLog::rollover`).  Called
    /// from the write path when the active file set has reached capacity.  The
    /// commit boundary is captured *before* the proposal so that `e_ROLLOVER`
    /// is treated as part of the uncommitted tail even in a single-node
    /// cluster (where `propose()` commits synchronously).
    void proposeRollover();

    /// If this node is the leader and the active file set cannot accommodate a
    /// record needing the specified `dataBytes` in the DATA file and
    /// `qlistBytes` in the QLIST file (the JOURNAL reserve is always checked;
    /// pass 0 where DATA/QLIST are not written), trigger `proposeRollover()`
    /// before the record is written.  Called first thing by each write-path
    /// method.  Return 0 to proceed, or non-zero if the write cannot proceed
    /// because a rollover is required but a previous `e_ROLLOVER` is still
    /// uncommitted (at most one uncommitted rollover is allowed at a time).
    int rolloverIfNeeded(bsls::Types::Uint64 dataBytes,
                         bsls::Types::Uint64 qlistBytes);

    /// Send an AppendEntries message via binary e_RAFT_PARTITION event.
    void sendAppendEntries(const RaftMessage& msg);

    /// Send an election/control RaftMessage via ControlMessageTransmitter.
    void sendControlMessage(const RaftMessage& msg);

    /// Notify FileStore of a single committed entry.
    void applyCommittedEntry(const LogEntry& entry);

    /// Call rstorage methods for entries accepted during step (follower).
    void applyEntriesAsReplica(const RaftMessage&  msg,
                               bsls::Types::Uint64 prevLastIndex,
                               bsls::Types::Uint64 newLastIndex);

    /// Convert an internal RaftMessage to a bmqp_ctrlmsg::RaftMessage.
    void toCtrlMsg(bmqp_ctrlmsg::RaftMessage* out,
                   const RaftMessage&         msg) const;

    /// Convert a bmqp_ctrlmsg::RaftMessage to an internal RaftMessage.
    void fromCtrlMsg(RaftMessage*                     out,
                     const bmqp_ctrlmsg::RaftMessage& msg,
                     int                              sourceNodeId) const;

    /// Send snapshot file chunks to the peer identified by the specified
    /// 'destNodeId'. Called when 'e_INSTALL_SNAPSHOT' is emitted in
    /// dispatchOutput.
    void sendSnapshot(int                 destNodeId,
                      bsls::Types::Uint64 lastIncludedIndex,
                      bsls::Types::Uint64 lastIncludedTerm);

    /// Send one chunk of the specified 'fileType' from the file at the
    /// specified 'filePath' of the specified 'fileSize', starting at the
    /// specified 'offset', to the specified 'destNode'.  Set the done flag
    /// when 'offset + chunkSize >= fileSize'.  Return the number of bytes
    /// sent, or negative on error.
    int sendSnapshotChunk(mqbnet::ClusterNode* destNode,
                          unsigned int         fileType,
                          const bsl::string&   filePath,
                          bsls::Types::Uint64  fileSize,
                          bsls::Types::Uint64  offset,
                          bsls::Types::Uint64  lastIncludedIndex,
                          bool                 done);

    /// Begin receiving a snapshot: wipe current FileStore and open temp
    /// files for writing.
    void beginReceiveSnapshot(bsls::Types::Uint64 lastIncludedIndex,
                              bsls::Types::Uint64 lastIncludedTerm);

    /// Apply a received snapshot chunk to the appropriate temp file.
    void applySnapshotChunk(const bdlbb::Blob& event);

    /// Callback invoked by the scheduler. Dispatches to tickDispatched().
    void tickCb();

    /// Execute tick on the partition's dispatcher thread.
    void tickDispatched();

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(PartitionRaft, bslma::UsesBslmaAllocator)

    // CREATORS
    PartitionRaft(int                                     partitionId,
                  const bsl::shared_ptr<mqbs::FileStore>& fileStore,
                  mqbc::ClusterData*                      clusterData,
                  mqbs::StoragesMonitor*                  storagesMonitor,
                  bslma::Allocator*                       allocator = 0);

    ~PartitionRaft() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    /// Start: create PartitionRaftLog and RaftNode, schedule tick timer.
    /// Return 0 on success.
    int start(bsl::ostream& errorDescription);

    /// Stop: cancel tick timer.
    void stop();

    /// Propose the write described by the specified `pw` for replication.
    /// This is the single entry point for every Raft partition write: it
    /// computes the rollover footprint from `pw`, runs `rolloverIfNeeded`
    /// (returning non-zero if the write must be deferred because a previous
    /// `e_ROLLOVER` is still uncommitted), assigns the write id, hands `pw`
    /// to the log, and drives the Raft propose sequence.  The record's
    /// sequence number (log index) is stamped in `PartitionRaftLog::append`
    /// at append time (not baked in here).  Return 0 on success, non-zero
    /// otherwise.  On success the caller may retrieve the
    /// DataStoreRecordHandle from `d_raftLog_mp->cachedHandle()`.
    int propose(mqbs::FileStore::PendingWrite& pw);

    /// Handle an incoming binary AppendEntries event (e_RAFT_PARTITION)
    /// from the specified 'source' node.
    void appendEntries(const bdlbb::Blob& event, mqbnet::ClusterNode* source);

    /// Handle an incoming Raft control message from the specified
    /// 'source' node.
    void onRaftControlMessage(const bmqp_ctrlmsg::RaftMessage& message,
                              mqbnet::ClusterNode*             source);

    /// Handle an incoming snapshot chunk event (e_RAFT_SNAPSHOT) from
    /// the specified 'source' node.
    void appendSnapshotChunk(const bdlbb::Blob&   event,
                             mqbnet::ClusterNode* source);

    // RecordStore OVERRIDES

    /// Write the specified `appData` and `options` belonging to specified
    /// `queueKey` and having specified `guid` and `attributes` to the data
    /// store, and update the specified `handle` with an identifier which
    /// can be used to retrieve the message.  Return zero on success,
    /// non-zero value otherwise.
    int
    writeMessageRecord(mqbi::StorageMessageAttributes*     attributes,
                       mqbs::DataStoreRecordHandle*        handle,
                       const bmqt::MessageGUID&            guid,
                       const bsl::shared_ptr<bdlbb::Blob>& appData,
                       const bsl::shared_ptr<bdlbb::Blob>& options,
                       const mqbu::StorageKey& queueKey) BSLS_KEYWORD_OVERRIDE;

    /// Write a CONFIRM record to the data store with the specified
    /// `queueKey`, optional `appKey`, `guid`, `timestamp` and `reason`.
    /// Return zero on success, non-zero value otherwise.
    int
    writeConfirmRecord(mqbs::DataStoreRecordHandle* handle,
                       const bmqt::MessageGUID&     guid,
                       const mqbu::StorageKey&      queueKey,
                       const mqbu::StorageKey&      appKey,
                       bsls::Types::Uint64          timestamp,
                       mqbs::ConfirmReason::Enum reason) BSLS_KEYWORD_OVERRIDE;

    /// Write a DELETION record to the data store with the specified
    /// `queueKey`, `flag`, `guid` and `timestamp`.  Return zero on success,
    /// non-zero value otherwise.
    int
    writeDeletionRecord(const bmqt::MessageGUID&       guid,
                        const mqbu::StorageKey&        queueKey,
                        mqbs::DeletionRecordFlag::Enum deletionFlag,
                        bsls::Types::Uint64 timestamp) BSLS_KEYWORD_OVERRIDE;

    int writeQueuePurgeRecord(mqbs::DataStoreRecordHandle*       handle,
                              const mqbu::StorageKey&            queueKey,
                              const mqbu::StorageKey&            appKey,
                              bsls::Types::Uint64                timestamp,
                              const mqbs::DataStoreRecordHandle& start)
        BSLS_KEYWORD_OVERRIDE;

    int writeQueueDeletionRecord(mqbs::DataStoreRecordHandle* handle,
                                 const mqbu::StorageKey&      queueKey,
                                 const mqbu::StorageKey&      appKey,
                                 bsls::Types::Uint64          timestamp)
        BSLS_KEYWORD_OVERRIDE;

    int writeQueueCreationRecord(mqbs::DataStoreRecordHandle* handle,
                                 const bmqt::Uri&             queueUri,
                                 const mqbu::StorageKey&      queueKey,
                                 const AppInfos&              appIdKeyPairs,
                                 bsls::Types::Uint64          timestamp,
                                 bool isNewQueue) BSLS_KEYWORD_OVERRIDE;

    void
    registerStorage(mqbs::ReplicatedStorage* storage) BSLS_KEYWORD_OVERRIDE;

    void unregisterStorage(const mqbs::ReplicatedStorage* storage)
        BSLS_KEYWORD_OVERRIDE;

    void createStorage(bsl::shared_ptr<mqbs::ReplicatedStorage>* storageSp,
                       const bmqt::Uri&                          queueUri,
                       const mqbu::StorageKey&                   queueKey,
                       mqbi::Domain* domain) BSLS_KEYWORD_OVERRIDE;

    mqbs::StoragesMonitor* storagesMonitor() BSLS_KEYWORD_OVERRIDE;

    /// Remove the record identified by the specified `handle`.
    void removeRecordRaw(const mqbs::DataStoreRecordHandle& handle)
        BSLS_KEYWORD_OVERRIDE;

    /// Execute the specified `functor` on this partition's dispatcher thread
    /// by delegating to the owned `FileStore`.
    void execute(const mqbi::Dispatcher::VoidFunction& functor)
        BSLS_KEYWORD_OVERRIDE;

    /// Drive a Raft rollover (admin `rollover` command): if this node is the
    /// leader and no previous `e_ROLLOVER` is still uncommitted, propose
    /// `e_ROLLOVER` and orchestrate the rollover.  Return zero on success,
    /// non-zero if rejected (not leader, or a rollover is already in flight).
    int rollover() BSLS_KEYWORD_OVERRIDE;

    /// Enable or disable writing to this partition per the specified
    /// `enable`, by delegating to the owned `FileStore`.
    void setAvailabilityStatus(bool enable) BSLS_KEYWORD_OVERRIDE;

    /// Set the strong-consistency replication factor to the specified
    /// `factor`, by delegating to the owned `FileStore`.
    void setReplicationFactor(int factor) BSLS_KEYWORD_OVERRIDE;

    /// Attempt to rollover the journal if needed after a purge has cleared
    /// outstanding records.
    void onPurgeComplete() BSLS_KEYWORD_OVERRIDE;

    /// Flush any buffered replication messages to the peers.
    void flushStorage() BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS (RecordStore)

    /// Load a summary of this partition into the specified `summary` by
    /// delegating to the owned `FileStore`.
    void loadSummary(mqbcmd::FileStore* summary) const BSLS_KEYWORD_OVERRIDE;

    /// Load into the specified `storages` the list of storages matching every
    /// predicate in `filters`, by delegating to the owned `FileStore`.
    void getStorages(mqbs::RecordStore::StorageList*          storages,
                     const mqbs::RecordStore::StorageFilters& filters) const
        BSLS_KEYWORD_OVERRIDE;

    void loadMessageRaw(bsl::shared_ptr<bdlbb::Blob>*      appData,
                        bsl::shared_ptr<bdlbb::Blob>*      options,
                        mqbi::StorageMessageAttributes*    attributes,
                        const mqbs::DataStoreRecordHandle& handle) const
        BSLS_KEYWORD_OVERRIDE;

    void loadMessageAttributesRaw(
        mqbi::StorageMessageAttributes*    buffer,
        const mqbs::DataStoreRecordHandle& handle) const BSLS_KEYWORD_OVERRIDE;

    void loadQueueOpRecordRaw(mqbs::QueueOpRecord*               buffer,
                              const mqbs::DataStoreRecordHandle& handle) const
        BSLS_KEYWORD_OVERRIDE;

    unsigned int getMessageLenRaw(
        const mqbs::DataStoreRecordHandle& handle) const BSLS_KEYWORD_OVERRIDE;

    /// Return the current primary leaseId for this partition.
    unsigned int primaryLeaseId() const BSLS_KEYWORD_OVERRIDE;

    /// Return `true` if there was Replication Receipt for the specified
    /// `handle`.
    bool hasReceipt(const mqbs::DataStoreRecordHandle& handle) const
        BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS
    int                 partitionId() const BSLS_KEYWORD_OVERRIDE;
    bool                isLeader() const BSLS_KEYWORD_OVERRIDE;
    int                 leaderId() const;
    bsls::Types::Uint64 currentTerm() const;

    /// Return a printable description of the client (e.g., for logging).
    bsl::string_view description() const BSLS_KEYWORD_OVERRIDE;

    /// Return the PartitionRaftLog.
    PartitionRaftLog* raftLog();

    /// Return the FileStore.
    mqbs::FileStore* fileStore();
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

inline int PartitionRaft::partitionId() const
{
    return d_partitionId;
}

inline PartitionRaftLog* PartitionRaft::raftLog()
{
    return d_raftLog_mp.get();
}

inline mqbs::FileStore* PartitionRaft::fileStore()
{
    return d_fileStore_sp.get();
}

}  // close package namespace
}  // close enterprise namespace

#endif
