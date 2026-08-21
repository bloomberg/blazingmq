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

// mqbraft_clusterstateraft.h -*-C++-*-
#ifndef INCLUDED_MQBRAFT_CLUSTERSTATERAFT
#define INCLUDED_MQBRAFT_CLUSTERSTATERAFT

//@PURPOSE: Provide glue between RaftNode and BlazingMQ cluster infrastructure
// for CSL (cluster metadata) replication.
//
//@CLASSES:
//  mqbraft::ClusterStateRaft: Manages RaftNode + CslRaftLog for cluster
//                             metadata consensus.
//
//@DESCRIPTION: This component wires 'RaftNode' into the cluster dispatcher,
// translating between 'RaftNodeOutput' messages and the network
// ('ControlMessage.raftMessage'), and applying committed cluster state
// advisories to 'ClusterState'.
//
/// Threading
///----------
// This component is NOT thread-safe.  All methods except 'start()' and
// 'stop()' must be called from the cluster dispatcher thread.

// MQB
#include <mqbc_clusterdata.h>
#include <mqbc_clusterstate.h>
#include <mqbcfg_messages.h>
#include <mqbi_clusterstatemanager.h>
#include <mqbraft_cslraftlog.h>
#include <mqbraft_raftnode.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>
#include <bmqt_uri.h>

// BDE
#include <ball_log.h>
#include <bdlbb_blob.h>
#include <bdlmt_eventscheduler.h>
#include <bsl_functional.h>
#include <bsl_unordered_map.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>

namespace BloombergLP {

namespace mqbnet {
class ClusterNode;
}

namespace mqbraft {

// ======================
// class ClusterStateRaft
// ======================

class ClusterStateRaft : public mqbi::ClusterStateUpdater {
  public:
    // TYPES

    /// Callback invoked (on the cluster dispatcher thread) whenever the CSL
    /// Raft state advances -- leadership change or committed entries applied
    /// -- so the orchestrator can re-evaluate whether it may transition to
    /// AVAILABLE.
    typedef bsl::function<void()> AvailabilityCb;

    // PUBLIC CONSTANTS

    /// `transferLeadership` return codes.
    static const int k_TRANSFER_NOT_LEADER     = -1;
    static const int k_TRANSFER_UNKNOWN_TARGET = -2;

  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBRAFT.CLUSTERSTATERAFT");

    // DATA
    mqbcfg::PartitionConfig                     d_partitionConfig;
    bslma::ManagedPtr<CslRaftLog>               d_cslLog_mp;
    bslma::ManagedPtr<RaftNode>                 d_raftNode_mp;
    mqbc::ClusterData*                          d_clusterData_p;
    mqbc::ClusterState*                         d_clusterState_p;
    bdlmt::EventScheduler::RecurringEventHandle d_tickHandle;
    AvailabilityCb                              d_availabilityCb;
    bool                                        d_isStarted;
    bslma::Allocator*                           d_allocator_p;

    /// Nodes resolved by `peerNode`, so a linked-list walk is not repeated.
    bsl::unordered_map<int, mqbnet::ClusterNode*> d_peerNodes;

    // NOT IMPLEMENTED
    ClusterStateRaft(const ClusterStateRaft&);
    ClusterStateRaft& operator=(const ClusterStateRaft&);

    // PRIVATE MANIPULATORS

    /// Process RaftNode output: send messages to peers and apply committed
    /// entries to ClusterState.
    /// Return the node with the specified `nodeId`, or 0 if there is none.
    /// `mqbnet::Cluster::lookupNode` walks a linked list, so its result is
    /// resolved once per node and held: the sends of one round would otherwise
    /// walk it once per peer.  Membership is fixed after construction, so a
    /// held pointer stays valid; a node added later resolves on first use, but
    /// node *removal*, if it is ever implemented, has to clear this.
    mqbnet::ClusterNode* peerNode(int nodeId);

    void dispatchOutput(RaftNodeOutput* output);

    /// Send an AppendEntries message via binary e_RAFT_CLUSTER event.
    void sendAppendEntries(const RaftMessage& msg);

    /// Send an AppendEntries response via binary e_RAFT_CLUSTER event.
    void sendAppendEntriesResponse(const RaftMessage& msg);

    /// Send an election/control RaftMessage via ControlMessageTransmitter.
    void sendControlMessage(const RaftMessage& msg);

    /// Acknowledge to the specified `source` a CSL snapshot whose record is
    /// the entry at the specified `lastIncludedIndex`.
    void sendInstallSnapshotResponse(mqbnet::ClusterNode* source,
                                     bsls::Types::Uint64  lastIncludedIndex);

    /// Process an incoming binary AppendEntries event (e_RAFT_CLUSTER) from
    /// the specified 'source' node, carrying the specified 'term' read from
    /// the event's `RaftHeader`.
    void appendEntries(const bdlbb::Blob&   event,
                       mqbnet::ClusterNode* source,
                       bsls::Types::Uint64  term);

    /// Process an incoming binary AppendEntries response event
    /// (e_RAFT_CLUSTER) from the specified 'source' node, carrying the
    /// specified 'term' read from the event's `RaftHeader`.
    void onAppendEntriesResponse(const bdlbb::Blob&   event,
                                 mqbnet::ClusterNode* source,
                                 bsls::Types::Uint64  term);

    /// Apply a single committed CSL entry to ClusterState and write
    /// an e_COMMIT record for rollback compatibility.  Return true if the
    /// entry is the first of the current term (the artificial
    /// `partitionPrimaryAdvisory`), whose commit is what applies the backlog
    /// inherited from prior terms.
    bool applyCommittedEntry(const LogEntry& entry);

    /// Move the elector's leader status to `e_ACTIVE` and notify the
    /// orchestrator.  Call only once the first current-term entry has been
    /// applied.
    void promoteToActive();

    /// Roll over the CSL log: snapshot the committed cluster state into a
    /// fresh file (preserving the uncommitted tail) and remove the old file.
    /// Invoked when the current log cannot fit the next record.  Mirrors the
    /// per-node local rollover of `IncoreClusterStateLedger::onLogRolloverCb`.
    /// Return 0 on success and a non-zero value otherwise.
    int rolloverCsl();

    /// Create and open a fresh, empty CSL log file in the configured
    /// location, loading it into the specified `logOut` and its generated id
    /// into `logIdOut`.  Size it at the configured `maxCSLFileSize`, or at
    /// the specified `minSize` when that is larger: a rollover has to copy
    /// the uncommitted tail forward, and the configured maximum is a
    /// compaction target, not a bound on data that must be kept.  Return 0
    /// on success and a non-zero value otherwise.
    int createNewCslLog(bsl::shared_ptr<mqbsi::Log>* logOut,
                        mqbu::StorageKey*            logIdOut,
                        bsls::Types::Uint64          minSize = 0);

    /// Convert an internal RaftMessage to a bmqp_ctrlmsg::RaftMessage.
    void toCtrlMsg(bmqp_ctrlmsg::RaftMessage* out,
                   const RaftMessage&         msg) const;

    /// Convert a bmqp_ctrlmsg::RaftMessage to an internal RaftMessage.
    void fromCtrlMsg(RaftMessage*                     out,
                     const bmqp_ctrlmsg::RaftMessage& msg,
                     int                              sourceNodeId) const;

    /// Callback invoked by the scheduler. Dispatches to tickDispatched().
    void tickCb();

    /// Execute tick on the cluster dispatcher thread.
    void tickDispatched();

    /// Update ElectorInfo from RaftNode state after a state change.
    void updateElectorInfo();

    /// Send the base `e_SNAPSHOT` record of the current CSL log to the
    /// destination of the specified `msg`, as a single `e_RAFT_SNAPSHOT`
    /// chunk.  The record's own sequence number is the log's snapshot index,
    /// which is what `msg` announces as `lastIncludedIndex`.
    void sendSnapshotRecord(const RaftMessage& msg);

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(ClusterStateRaft, bslma::UsesBslmaAllocator)

    // CREATORS
    ClusterStateRaft(mqbc::ClusterData*             clusterData,
                     mqbc::ClusterState*            clusterState,
                     const mqbcfg::PartitionConfig& partitionConfig,
                     const AvailabilityCb&          availabilityCb,
                     bslma::Allocator*              allocator = 0);

    ~ClusterStateRaft() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    /// Start the Raft node: open the CSL log, schedule tick timer.
    /// Return 0 on success.
    int start(bsl::ostream& errorDescription);

    /// Stop the Raft node: cancel tick timer, close CSL log.
    void stop();

    /// Process an incoming Raft control message (election, response) from
    /// the specified 'source' node.
    void onRaftControlMessage(const bmqp_ctrlmsg::RaftMessage& message,
                              mqbnet::ClusterNode*             source);

    /// Process an incoming binary Raft event (e_RAFT_CLUSTER) from the
    /// specified 'source' node, routing it to the handler for its message
    /// type.
    void onRaftEvent(const bdlbb::Blob& event, mqbnet::ClusterNode* source);

    /// Install the CSL snapshot carried by the specified `e_RAFT_SNAPSHOT`
    /// `event` from the specified `source` node: replace `ClusterState` and
    /// the CSL log with the base `e_SNAPSHOT` record it carries, then
    /// acknowledge.  The snapshot arrives in a single chunk.
    void applySnapshotChunk(const bdlbb::Blob&   event,
                            mqbnet::ClusterNode* source);

    /// Propose the specified 'advisory' for replication via Raft.
    /// Return 0 on success, non-zero if not the leader.
    int propose(const bmqp_ctrlmsg::ClusterMessage& advisory);

    /// Set the CSL Raft group's leadership-eligibility override to the
    /// specified `mode` (see `RaftNode::setElectionMode`) and dispatch any
    /// resulting Raft messages.  Used to reproduce the legacy per-node
    /// `set_quorum` leader-pinning knob.
    ///
    /// THREAD: This method is invoked in the associated cluster's dispatcher
    ///         thread.
    void setElectionMode(ElectionMode::Enum mode);

    /// Hand CSL Raft leadership to the node with the specified `targetNodeId`:
    /// catch the target up and send it a `TimeoutNow`.  Return 0 if the
    /// transfer was initiated (it completes asynchronously, once the target
    /// wins the election it starts), `k_TRANSFER_NOT_LEADER` if this node is
    /// not the leader, or `k_TRANSFER_UNKNOWN_TARGET` if `targetNodeId` is not
    /// a peer of this Raft group.
    ///
    /// THREAD: This method is invoked in the associated cluster's dispatcher
    ///         thread.
    int transferLeadership(int targetNodeId);

    /// If self is the CSL Raft leader and every partition's (primaryNodeId,
    /// leaseId) is known (per `ClusterState::partitions()`), propose a
    /// combined `partitionPrimaryAdvisory` capturing every partition's
    /// (primaryNodeId, leaseId==Raft term) to the CSL Raft.  This is the
    /// "artificial" advisory that keeps the CSL's recorded leaseId in step
    /// with the journal (== term) for legacy-broker interoperability, and
    /// whose commit fires the availability callback with true.  Idempotent:
    /// re-proposes only when the set of leaseIds has changed since the last
    /// successful proposal.  A no-op on non-leaders or before the
    /// preconditions hold.  Called by the orchestrator only after it has
    /// verified every partition has a locally-known leader.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    void maybeIssuePartitionPrimaryAdvisory();

    /// Process the queue-assignment `request` received from the specified
    /// `requester` node.  Reply with a failure status if self is not the
    /// active leader or is stopping; otherwise assign the queue (which
    /// proposes the assignment advisory to the CSL Raft, applying the same
    /// domain/limit/duplicate checks as the legacy path) and reply with the
    /// resulting status.  This is the Raft-mode counterpart of
    /// `mqbc::ClusterUtil::processQueueAssignmentRequest`.
    void
    processQueueAssignmentRequest(const bmqp_ctrlmsg::ControlMessage& request,
                                  mqbnet::ClusterNode* requester);

    // ClusterStateUpdater interface

    bool assignQueue(const bmqt::Uri&      uri,
                     bmqp_ctrlmsg::Status* status) BSLS_KEYWORD_OVERRIDE;

    void unassignQueue(const bmqp_ctrlmsg::QueueUnAssignmentAdvisory& advisory)
        BSLS_KEYWORD_OVERRIDE;

    mqbi::ClusterErrorCode::Enum
    updateAppIds(const bsl::vector<bsl::string>& added,
                 const bsl::vector<bsl::string>& removed,
                 const bsl::string&              domainName,
                 const bsl::string&              uri) BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS
    bool                isLeader() const;
    int                 leaderId() const;
    bsls::Types::Uint64 currentTerm() const;
    int                 quorum() const;
};

}  // close package namespace
}  // close enterprise namespace

#endif
