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
    bool                                        d_isStarted;
    bslma::Allocator*                           d_allocator_p;

    // NOT IMPLEMENTED
    ClusterStateRaft(const ClusterStateRaft&);
    ClusterStateRaft& operator=(const ClusterStateRaft&);

    // PRIVATE MANIPULATORS

    /// Process RaftNode output: send messages to peers and apply committed
    /// entries to ClusterState.
    void dispatchOutput(RaftNodeOutput* output);

    /// Send an AppendEntries message via binary e_RAFT_CLUSTER event.
    void sendAppendEntries(const RaftMessage& msg);

    /// Send an election/control RaftMessage via ControlMessageTransmitter.
    void sendControlMessage(const RaftMessage& msg);

    /// Apply a single committed CSL entry to ClusterState and write
    /// an e_COMMIT record for rollback compatibility.
    void applyCommittedEntry(const LogEntry& entry);

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

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(ClusterStateRaft, bslma::UsesBslmaAllocator)

    // CREATORS
    ClusterStateRaft(mqbc::ClusterData*             clusterData,
                     mqbc::ClusterState*            clusterState,
                     const mqbcfg::PartitionConfig& partitionConfig,
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

    /// Process an incoming binary AppendEntries event (e_RAFT_CLUSTER)
    /// from the specified 'source' node.
    void appendEntries(const bdlbb::Blob&   event,
                                   mqbnet::ClusterNode* source);

    /// Propose the specified 'advisory' for replication via Raft.
    /// Return 0 on success, non-zero if not the leader.
    int propose(const bmqp_ctrlmsg::ClusterMessage& advisory);

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
};

}  // close package namespace
}  // close enterprise namespace

#endif
