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

//@PURPOSE: Connect 'RaftNode' to a BlazingMQ partition for journal and data
// replication.
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
// This component is NOT thread-safe.  Most methods must be called on the
// partition's dispatcher thread.

// MQB
#include <mqbc_clusterdata.h>
#include <mqbnet_cluster.h>
#include <mqbraft_partitionraftlog.h>
#include <mqbraft_raftnode.h>
#include <mqbs_datastore.h>
#include <mqbs_filestoreprotocol.h>
#include <mqbs_filestoreset.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_protocol.h>

// BDE
#include <ball_log.h>
#include <bdlbb_blob.h>
#include <bdlcc_objectpool.h>
#include <bdlcc_sharedobjectpool.h>
#include <bdlmt_eventscheduler.h>
#include <bsl_deque.h>
#include <bsl_functional.h>
#include <bsl_unordered_map.h>
#include <bsl_vector.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_atomic.h>
#include <bsls_timeinterval.h>
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

class PartitionRaft : public mqbs::RecordStore,
                      public mqbnet::ClusterObserver {
  public:
    // TYPES

    /// Callback for this partition's Raft leadership changes and, with
    /// `haveCommit=true`, for this node applying the first sync point
    /// committed under `term`.  The latter fires on the leader and on the
    /// replicas.
    typedef bsl::function<void(int                 partitionId,
                               int                 leaderNodeId,
                               bsls::Types::Uint64 term,
                               bool                haveCommit)>
        PartitionLeadershipCb;

    /// Holds a reference to the cluster's stop context.  The StopResponse to
    /// the stopping peer goes out once every copy is destroyed.
    typedef bsl::function<void()> StopContextHolder;

  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBRAFT.PARTITIONRAFT");

    // TYPES

    /// Pool of `Proposal` objects, passed along the write path by shared_ptr.
    typedef bdlcc::SharedObjectPool<
        mqbs::Proposal,
        bdlcc::ObjectPoolFunctors::DefaultCreator,
        bdlcc::ObjectPoolFunctors::Reset<mqbs::Proposal> >
        ProposalSpPool;

    /// A stopping peer whose StopResponse waits for the sync point at
    /// `d_index` to reach it.
    struct StopResponseWait {
        int                 d_peerId;
        bsls::Types::Uint64 d_index;
        bsls::TimeInterval  d_whenToStop;
        StopContextHolder   d_stopContext;
    };

    // DATA
    unsigned int                                d_partitionId;
    bsl::shared_ptr<mqbs::FileStore>            d_fileStore_sp;
    mqbc::ClusterData*                          d_clusterData_p;
    mqbs::StorageMonitor*                       d_storageMonitor_p;
    bslma::ManagedPtr<PartitionRaftLog>         d_raftLog_mp;
    bslma::ManagedPtr<RaftNode>                 d_raftNode_mp;
    bdlmt::EventScheduler::RecurringEventHandle d_tickHandle;

    ProposalSpPool d_proposalPool;

    bool              d_isStarted;
    bslma::Allocator* d_allocator_p;

    /// Nodes found by `peerNode`.
    bsl::unordered_map<int, mqbnet::ClusterNode*> d_peerNodes;

    // Snapshot transfer state (receiver side)
    bool                d_receivingSnapshot;
    int                 d_snapshotJournalFd;
    int                 d_snapshotDataFd;
    int                 d_snapshotQlistFd;
    mqbs::FileStoreSet  d_snapshotFiles;
    bsls::Types::Uint64 d_snapshotLastIncludedIndex;
    bsls::Types::Uint64 d_snapshotLastIncludedTerm;

    /// `true` while `dispatchOutput` is walking its committed entries.  A
    /// propose issued from there would re-enter `dispatchOutput` on a frame
    /// that has not finished that walk.
    bool d_isDispatchingOutput;

    /// DELETION records written while `d_isDispatchingOutput` is `true`.
    /// `postDispatch` proposes them.
    bsl::vector<bsl::shared_ptr<mqbs::Proposal> > d_deferred;

    /// `true` if an `e_ROLLOVER` has been proposed but not yet committed.
    bool d_isRolloverPending;

    /// Term of the last sync point reported through `d_leadershipCb`.  Only
    /// the first sync point committed under each term is reported.
    bsls::Types::Uint64 d_reportedSyncPointTerm;

    /// `true` from becoming leader until this term's become-leader sync
    /// point is proposed.
    bool d_needsBecomeLeaderSyncPoint;

    /// Signals the cluster (see `PartitionLeadershipCb`).
    PartitionLeadershipCb d_leadershipCb;

    /// Result of the last `checkIfCanShutdown`, read by the cluster
    /// dispatcher thread.  Initially `false`.
    bsls::AtomicBool d_canShutdown;

    /// Stopping peers whose StopResponse waits on this partition.
    bsl::vector<StopResponseWait> d_stopResponseWaits;

    /// File holding this partition's `PersistentState`.
    bsl::string d_persistentStatePath;

    /// `true` while `currentTerm` or the vote has changed and is not saved.
    bool d_isPersistentStateDirty;

    // NOT IMPLEMENTED
    PartitionRaft(const PartitionRaft&);
    PartitionRaft& operator=(const PartitionRaft&);

    // PRIVATE ACCESSORS

    // PRIVATE MANIPULATORS

    /// Recompute `d_canShutdown`: whether every peer that has a channel has
    /// acknowledged this node's whole log.
    ///
    /// THREAD: Executed by this partition's dispatcher thread.
    void checkIfCanShutdownDispatched();

    /// Release the stop context of each entry in `d_stopResponseWaits` whose
    /// peer has the sync point, has no channel, or has waited past
    /// `d_whenToStop`.  Release all of them if this node is not the leader.
    ///
    /// THREAD: Executed by this partition's dispatcher thread.
    void checkIfCanSendStopResponse();

    /// Return the node with the specified `nodeId`, or 0 if there is none.
    /// Look in `d_peerNodes` first, and add a node found by
    /// `mqbnet::Cluster::lookupNode`.
    mqbnet::ClusterNode* peerNode(int nodeId);

    /// Pass the specified `isAvailable` for the peer with the specified
    /// `nodeId` to this partition's `RaftNode`.
    ///
    /// THREAD: Executed by this partition's dispatcher thread.
    void setPeerAvailabilityDispatched(int nodeId, bool isAvailable);

    /// Save the `RaftNode`'s `PersistentState` if the specified `changed` is
    /// `true` or an earlier save failed.  Return `true` if it is on disk.
    bool savePersistentState(bool changed);

    /// Send every message in the specified `output`, once
    /// `savePersistentState` succeeds; send none if it fails.
    void dispatchMessages(RaftNodeOutput* output);

    /// Dispatch RaftNode output: send messages to peers and notify FileStore
    /// of committed entries.
    void dispatchOutput(RaftNodeOutput* output);

    /// Run the part of `dispatchOutput` that must follow its apply loop, for
    /// the specified `output`, `hadRollover` telling whether that loop rolled
    /// the file set over.  Anything that proposes belongs here: a propose from
    /// inside the loop re-enters `dispatchOutput` on a frame still walking
    /// `d_committed`.
    void postDispatch(RaftNodeOutput* output, bool hadRollover);

    /// Take the held proposals from the log (`takeHeldProposals`).  Hold
    /// again a PUT whose queue is still local.  For the rest, undo what each
    /// one reserved (`mqbs::FileStore::undoPropose`), re-post each PUT whose
    /// producer is still attached to its queue, now a remote one, and drop the
    /// others.  Call after the conversion, so the handles cluster peers opened
    /// are already gone.
    void repostHeldProposals();

    /// Take the held proposals from the log (`takeHeldProposals`) and propose
    /// each one into this node's log.  Call when the leadership transfer that
    /// held them is abandoned.  The behavior is undefined unless this node is
    /// the leader and has no transfer under way.
    void reproposeHeldProposals();

    /// Pass each of the specified `proposals` to `propose`, in order.  Each
    /// was set aside by an earlier `propose`.  On failure, undo what the
    /// proposal reserved (`mqbs::FileStore::undoPropose`), and NACK it if it
    /// is a PUT.  The behavior is undefined unless this node is the leader.
    void rePropose(const PartitionRaftLog::Proposals& proposals);

  public:
    /// Convert this partition's queues to remote, self no longer leading it,
    /// and re-post the PUTs held since the stepdown (`repostHeldProposals`).
    /// Called by the cluster once it has released the handles the peers
    /// opened here.
    void convertQueuesToRemote();

  private:
    /// Apply the next batch of committed-but-unapplied entries.  Re-posted to
    /// this partition's dispatcher between batches.  This is to avoid holding
    /// the thread for a whole replay (a restarted node applies everything
    /// since the last rollover).
    void applyCommittedBatchDispatched();

    /// Run a Raft send round, then `checkIfCanSendStopResponse`.  Installed on
    /// the `FileStore` as its flush callback, so the dispatcher calls this
    /// once it has drained this partition's queue.  One round then serves a
    /// batch of proposals and responses.
    void flush();

    /// Propose a sync-point log entry under the current term.  Called by
    /// `proposeDeferredSyncPoint`, `proposeShutdownSyncPoint`, and
    /// `onPeerNodeStopping`.  The become-leader sync point is a current-term
    /// entry, so its commit also commits the entries inherited from prior
    /// terms (Raft \S5.4.2).  Legacy recovery uses the last sync point in the
    /// journal as a checkpoint.
    void proposeSyncPoint();

    /// Propose an `e_ROLLOVER` sync-point log entry under the current term.
    /// Called from the write path (`rolloverIfNeeded`) and by the admin
    /// `rollover`.  Every node rolls the file set over when the committed
    /// `e_ROLLOVER` reaches `dispatchOutput` (`PartitionRaftLog::rollover`).
    /// Until then, `propose` buffers writes in the log
    /// (`PartitionRaftLog::bufferProposal`), and `drainProposals` replays them
    /// into the new file set.  Return 0 on success, or the non-zero rc of the
    /// failed `propose`; no `e_ROLLOVER` entry then exists.
    int proposeRollover();

    /// If this node is the leader and the active file set cannot accommodate a
    /// record needing the specified `dataBytes` in the DATA file and
    /// `qlistBytes` in the QLIST file, call `proposeRollover`, unless a
    /// rollover is already pending.  The JOURNAL reserve is always checked.  A
    /// record that writes nothing to DATA or QLIST passes 0 for that file.
    /// Return 0, or a non-zero value if a rollover cannot free enough space
    /// (the partition is then read-only).  While the rollover is pending,
    /// `propose` buffers the triggering write and the writes after it.
    int rolloverIfNeeded(bsls::Types::Uint64 dataBytes,
                         bsls::Types::Uint64 qlistBytes);

    /// Take the writes the log buffered during the rollover
    /// (`takeBufferedProposals`) and propose them into the new file set, in
    /// order (`rePropose`).  Called from `postDispatch` after the committed
    /// `e_ROLLOVER` rolled the file set over.
    void drainProposals();

    /// Send an AppendEntries message via binary e_RAFT_PARTITION event.
    void sendAppendEntries(const RaftMessage& msg);

    /// Send an AppendEntries response via binary e_RAFT_PARTITION event.
    void sendAppendEntriesResponse(const RaftMessage& msg);

    /// Send an election/control RaftMessage via ControlMessageTransmitter.
    void sendControlMessage(const RaftMessage& msg);

    /// Apply the specified committed `entry`.  The specified
    /// `commitTimepoint` is when the caller applied this batch of commits.
    void applyCommittedEntry(const LogEntry&    entry,
                             bsls::Types::Int64 commitTimepoint);

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
    void sendSnapshot(int destNodeId, bsls::Types::Uint64 lastIncludedIndex);

    /// Begin receiving a snapshot: empty the storages, close the `FileStore`
    /// (archiving its files), and open the current file set's paths for
    /// writing.
    void beginReceiveSnapshot(bsls::Types::Uint64 lastIncludedIndex,
                              bsls::Types::Uint64 lastIncludedTerm);

    /// Apply one snapshot chunk from the specified `event`, received from the
    /// specified `source`, and on the terminal chunk reopen the file set and
    /// answer `source`.
    void applySnapshotChunk(const bdlbb::Blob&   event,
                            mqbnet::ClusterNode* source);

    /// Callback invoked by the scheduler. Dispatches to tickDispatched().
    void tickCb();

    /// Execute tick on the partition's dispatcher thread.
    void tickDispatched();

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(PartitionRaft, bslma::UsesBslmaAllocator)

    // CREATORS
    PartitionRaft(unsigned int                            partitionId,
                  const bsl::shared_ptr<mqbs::FileStore>& fileStore,
                  mqbc::ClusterData*                      clusterData,
                  mqbs::StorageMonitor*                   storageMonitor,
                  const PartitionLeadershipCb&            leadershipCb,
                  bslma::Allocator*                       allocator = 0);

    ~PartitionRaft() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    /// Open and recover the FileStore-backed Raft log, seed the applied state,
    /// and schedule the tick timer.  A FileStore open or recovery failure is
    /// unrecoverable, so this ALARMs and terminates the process.
    ///
    /// THREAD: Executed by this partition's dispatcher thread.
    void start();

    /// Remove the `FileStore` flush callback, unregister from the cluster's
    /// node state changes, and cancel the tick timer.
    void stop();

    // mqbnet::ClusterObserver OVERRIDES

    /// Notification that the specified `node` is available or not, per the
    /// specified `isAvailable`.
    ///
    /// THREAD: Executed by the *IO* thread.
    void onNodeStateChange(mqbnet::ClusterNode* node,
                           bool isAvailable) BSLS_KEYWORD_OVERRIDE;

    /// Propose this term's become-leader sync point, if this node is the
    /// leader and has not proposed it yet (`d_needsBecomeLeaderSyncPoint`).
    /// Called by the cluster once the CSL has committed this partition's
    /// leaseId.
    ///
    /// THREAD: Executed by this partition's dispatcher thread.
    void proposeDeferredSyncPoint();

    /// Propose a sync point, if this node is the leader and has proposed this
    /// term's become-leader sync point.  Called when self is shutting down
    /// (`PartitionRaftManager::processShutdownEvent`).  This is to avoid
    /// legacy recovery truncating the journal back to an older sync point on
    /// a restart into legacy mode.
    ///
    /// THREAD: Executed by this partition's dispatcher thread.
    void proposeShutdownSyncPoint();

    /// Propose a sync point, if this node is the leader and has proposed this
    /// term's become-leader sync point, for the peer with the specified
    /// `peerId`, which announced it is stopping.  Hold the specified
    /// `stopContext` until that peer's `matchIndex` reaches the sync point, or
    /// until `checkIfCanSendStopResponse` gives up.  The StopResponse goes out
    /// when the last copy of `stopContext` is destroyed.
    ///
    /// THREAD: Executed by this partition's dispatcher thread.
    void onPeerNodeStopping(int peerId, const StopContextHolder& stopContext);

    /// Return true if every peer that has a channel has acknowledged this
    /// node's whole log, as computed by the previous call.  If not, request a
    /// new computation on this partition's dispatcher thread.
    ///
    /// THREAD: Called from the cluster dispatcher thread.
    bool checkIfCanShutdown();

    /// Set this partition Raft group's leadership-eligibility override to the
    /// specified `mode` (see `RaftNode::setElectionMode`) and dispatch any
    /// resulting Raft messages.  Used for the QUORUM tunable
    /// (`ClusterOrchestrator::processCommand`).
    ///
    /// THREAD: Executed by this partition's dispatcher thread.
    void setElectionMode(ElectionMode::Enum mode);

    /// Propose the write described by the specified `pw` for replication.
    /// This is the single entry point for every Raft partition write.  Return
    /// 0 on success, non-zero otherwise.  In order:
    /// 1. During a leadership transfer, hold a PUT.
    /// 2. If no rollover is pending: when the journal is read-only, reject any
    ///    write other than a full-queue PURGE; otherwise, run
    ///    `rolloverIfNeeded` with the footprint of `pw`.
    /// 3. While a rollover is pending, buffer the write.
    /// 4. If this node is not the leader, hold the write.
    /// 5. Otherwise, append `pw` to the log through the `RaftNode`.
    ///    `pw->d_handle` then becomes valid, and the record's sequence number
    ///    is its log index.
    /// A held or buffered write returns 0 without a valid `pw->d_handle`.
    int propose(const bsl::shared_ptr<mqbs::Proposal>& pw);

    /// Handle an incoming binary AppendEntries event (e_RAFT_PARTITION)
    /// from the specified 'source' node.
    void appendEntries(const bsl::shared_ptr<const bdlbb::Blob>& event,
                       mqbnet::ClusterNode*                      source);

    /// Handle an incoming binary AppendEntries response event
    /// (e_RAFT_PARTITION) from the specified 'source' node.
    void
    onAppendEntriesResponse(const bsl::shared_ptr<const bdlbb::Blob>& event,
                            mqbnet::ClusterNode*                      source);

    /// Handle an incoming Raft control message from the specified
    /// 'source' node.
    void onRaftControlMessage(const bmqp_ctrlmsg::RaftMessage& message,
                              mqbnet::ClusterNode*             source);

    /// Handle an incoming snapshot chunk event (e_RAFT_SNAPSHOT) from
    /// the specified 'source' node.
    void appendSnapshotChunk(const bsl::shared_ptr<const bdlbb::Blob>& event,
                             mqbnet::ClusterNode*                      source);

    /// Execute the specified `functor` on this partition's dispatcher thread
    /// by delegating to the owned `FileStore`.
    void execute(const mqbi::Dispatcher::VoidFunction& functor);

    // RecordStore OVERRIDES

    /// Write the specified `appData` and `options` belonging to specified
    /// `queueKey` and having specified `guid` and `attributes` to the data
    /// store.  Return zero on success, non-zero value otherwise.  Raft callers
    /// do not read the specified `handle`: the record enters the storage when
    /// it commits (`FileStore::onRecordCommittedPrimary`).
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

    /// Every partition served through this object is Raft-replicated.
    bool isRaft() const BSLS_KEYWORD_OVERRIDE { return true; }

    void unregisterStorage(const mqbs::ReplicatedStorage* storage)
        BSLS_KEYWORD_OVERRIDE;

    void createStorage(bsl::shared_ptr<mqbs::ReplicatedStorage>* storageSp,
                       const bmqt::Uri&                          queueUri,
                       const mqbu::StorageKey&                   queueKey,
                       mqbi::Domain* domain) BSLS_KEYWORD_OVERRIDE;

    mqbs::StorageMonitor* storageMonitor() BSLS_KEYWORD_OVERRIDE;

    const mqbs::DataStoreConfig::Records&
    records() const BSLS_KEYWORD_OVERRIDE;

    bsls::Types::Uint64 numRecords() const BSLS_KEYWORD_OVERRIDE;

    void
    loadMessageRecord(mqbs::MessageRecord* buffer,
                      const mqbs::DataStoreConfig::Records::const_iterator& it)
        const BSLS_KEYWORD_OVERRIDE;

    void
    loadConfirmRecord(mqbs::ConfirmRecord* buffer,
                      const mqbs::DataStoreConfig::Records::const_iterator& it)
        const BSLS_KEYWORD_OVERRIDE;

    void
    loadQueueOpRecord(mqbs::QueueOpRecord* buffer,
                      const mqbs::DataStoreConfig::Records::const_iterator& it)
        const BSLS_KEYWORD_OVERRIDE;

    void recordIteratorToHandle(
        mqbs::DataStoreRecordHandle*                          handle,
        const mqbs::DataStoreConfig::Records::const_iterator& it) const
        BSLS_KEYWORD_OVERRIDE;

    /// Remove the record identified by the specified `handle`.
    void removeRecordRaw(const mqbs::DataStoreRecordHandle& handle)
        BSLS_KEYWORD_OVERRIDE;

    /// Close this record store.  If the optional `flush` flag is true, flush
    /// to the backup storage (e.g., disk) if applicable.  If the optional
    /// `archive` flag is true, archive the files.  Return zero on success,
    /// non-zero value otherwise.  Before closing the `FileStore`, drop the
    /// proposals still in the log and clear the cached entry blobs (both
    /// alias the file set), and release `d_stopResponseWaits`.  The behavior
    /// is undefined unless called on this record store's dispatcher thread
    /// (see `execute`).
    int close(bool flush = false, bool archive = false) BSLS_KEYWORD_OVERRIDE;

    /// Admin `rollover` command: if this node is the leader and no
    /// `e_ROLLOVER` is pending, call `proposeRollover`.  Return zero on
    /// success, non-zero if this node is not the leader, a rollover is
    /// pending, or the proposal fails.
    int rollover() BSLS_KEYWORD_OVERRIDE;

    /// Drive a Raft leadership transfer to the node whose host name is the
    /// specified `targetHostName` (admin `transferLeadership` command): if
    /// this node is the leader, catch the target up and send it a
    /// `TimeoutNow`.  Return zero if the transfer was initiated (it completes
    /// asynchronously, once the target wins the election it starts) or the
    /// target is self.  Return non-zero if this node is not the leader, the
    /// target is unknown, a rollover is pending, or `RaftNode` rejects the
    /// transfer.
    int transferLeadership(const bsl::string& targetHostName)
        BSLS_KEYWORD_OVERRIDE;

    /// Enable or disable writing to this partition per the specified
    /// `enable`, by delegating to the owned `FileStore`.
    void setAvailabilityStatus(bool enable) BSLS_KEYWORD_OVERRIDE;

    /// Set the strong-consistency replication factor to the specified
    /// `factor`, by delegating to the owned `FileStore`.
    void setReplicationFactor(int factor) BSLS_KEYWORD_OVERRIDE;

    /// Called after a purge cleared outstanding records.  If this node is the
    /// leader and the journal is read-only, make the journal writable again.
    /// Then call `rolloverIfNeeded`.
    void onPurgeComplete() BSLS_KEYWORD_OVERRIDE;

    /// Call `FileStore::flushStorage`, then run a send round (`flush`).
    void flushStorage() BSLS_KEYWORD_OVERRIDE;

    /// Set the last strong consistency point. No-op for Raft partitions.
    void setLastStrongConsistency(unsigned int        primaryLeaseId,
                                  bsls::Types::Uint64 sequenceNum)
        BSLS_KEYWORD_OVERRIDE;

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

    /// Return the write-head leaseId for this partition: the current Raft
    /// term.
    virtual unsigned int writeHeadLeaseId() const BSLS_KEYWORD_OVERRIDE;

    /// Return the log's `lastIndex()` plus the number of writes buffered for a
    /// pending rollover.  The drain appends those writes at the next indices.
    ///
    /// THREAD: Executed by this partition's dispatcher thread.
    bsls::Types::Uint64 writeHeadSeqNum() const BSLS_KEYWORD_OVERRIDE;

    /// Return true if the entry at the specified `sequenceNumber` (a log
    /// index) has been applied.
    ///
    /// THREAD: Executed by this partition's dispatcher thread.
    bool
    hasApplied(bsls::Types::Uint64 sequenceNumber) const BSLS_KEYWORD_OVERRIDE;

    /// Return `true` if there was Replication Receipt for the specified
    /// `handle`.
    bool hasReceipt(const mqbs::DataStoreRecordHandle& handle) const
        BSLS_KEYWORD_OVERRIDE;

    bool isFileSetAvailable() const BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS
    int  partitionId() const BSLS_KEYWORD_OVERRIDE;
    bool isLeader() const BSLS_KEYWORD_OVERRIDE;

    /// Return `true` if this node leads this partition and a leadership
    /// transfer is under way; see `mqbs::RecordStore`.
    bool                isTransferringLeadership() const BSLS_KEYWORD_OVERRIDE;
    int                 leaderId() const;
    bsls::Types::Uint64 currentTerm() const;

    /// Return `true` if the specified `probe` indicates that the corresponding
    /// data can still arrive, and update `probe` if necessary.  Return `false`
    /// on the leader, which holds every committed record.  `isAbsent` is
    /// unused: on Raft a record's presence is its receipt.
    bool isPendingReplication(mqbi::Storage::DeliveryProbe* probe,
                              bool isAbsent) const BSLS_KEYWORD_OVERRIDE;

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
