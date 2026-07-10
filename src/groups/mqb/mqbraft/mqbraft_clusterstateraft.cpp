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

// mqbraft_clusterstateraft.cpp -*-C++-*-
#include <mqbraft_clusterstateraft.h>

// MQB
#include <mqbc_clusterstateledgerprotocol.h>
#include <mqbc_clusterstateledgerutil.h>
#include <mqbc_clusterutil.h>
#include <mqbnet_cluster.h>
#include <mqbnet_controlmessagetransmitter.h>
#include <mqbsl_memorymappedondisklog.h>

#include <bmqt_uri.h>

#include <bmqu_blobobjectproxy.h>

// BDE
#include <ball_log.h>
#include <bdlf_bind.h>
#include <bdlt_datetime.h>
#include <bsls_assert.h>
#include <bsls_timeinterval.h>

namespace BloombergLP {
namespace mqbraft {

namespace {

const int k_TICK_INTERVAL_MS = 100;

RaftNodeConfig makeRaftConfig(const mqbc::ClusterData& clusterData,
                              bslma::Allocator*        allocator)
{
    RaftNodeConfig config(RaftNodeConfig::k_CSL_PARTITION_ID,
                          true,  // broadcastHeartbeatOnCommit
                          allocator);

    mqbnet::Cluster* netCluster = clusterData.membership().netCluster();

    config.d_selfId = netCluster->selfNodeId();

    // 'd_peerIds' holds the *full* membership including self: 'quorum()' is
    // 'peerIds.size()/2 + 1' (== majority of the whole cluster for both odd
    // and even sizes), and 'becomeCandidate'/'becomeLeader' skip self while
    // iterating.  'netCluster->nodes()' already includes self, so add each
    // node exactly once (the previous code additionally pushed self a second
    // time, which made a single-node cluster size 2 -> quorum 2 -> never
    // elects).
    const mqbnet::Cluster::NodesList& nodes = netCluster->nodes();
    for (mqbnet::Cluster::NodesList::const_iterator it = nodes.begin();
         it != nodes.end();
         ++it) {
        config.d_peerIds.push_back((*it)->nodeId());
    }

    config.d_electionTimeoutMin = 10;
    config.d_electionTimeoutMax = 20;
    config.d_heartbeatInterval  = 3;
    config.d_preVote            = true;

    return config;
}

}  // close unnamed namespace

// ======================
// class ClusterStateRaft
// ======================

// CREATORS
ClusterStateRaft::ClusterStateRaft(
    mqbc::ClusterData*             clusterData,
    mqbc::ClusterState*            clusterState,
    const mqbcfg::PartitionConfig& partitionConfig,
    bslma::Allocator*              allocator)
: d_partitionConfig(partitionConfig, allocator)
, d_cslLog_mp()
, d_raftNode_mp()
, d_clusterData_p(clusterData)
, d_clusterState_p(clusterState)
, d_tickHandle()
, d_isStarted(false)
, d_allocator_p(bslma::Default::allocator(allocator))
{
    BSLS_ASSERT_SAFE(clusterData);
    BSLS_ASSERT_SAFE(clusterState);
}

ClusterStateRaft::~ClusterStateRaft()
{
    BSLS_ASSERT_SAFE(!d_isStarted);
}

// PRIVATE MANIPULATORS
void ClusterStateRaft::dispatchOutput(RaftNodeOutput* output)
{
    BSLS_ASSERT_SAFE(output);

    // TODO: optimize for the normal case when multiple peers need the same
    // AppendEntries content (all caught up) — build one event blob and
    // send to all instead of per-peer.
    for (bsl::vector<RaftMessage>::size_type i = 0;
         i < output->d_messages.size();
         ++i) {
        const RaftMessage& msg = output->d_messages[i];
        if (msg.d_type == RaftMessageType::e_APPEND_ENTRIES) {
            sendAppendEntries(msg);
        }
        else {
            sendControlMessage(msg);
        }
    }

    for (bsl::vector<LogEntry>::size_type i = 0;
         i < output->d_committed.size();
         ++i) {
        applyCommittedEntry(output->d_committed[i]);
    }

    if (output->d_stateChanged || output->d_leaderChanged) {
        updateElectorInfo();
    }
}

void ClusterStateRaft::sendAppendEntries(const RaftMessage& msg)
{
    mqbnet::ClusterNode* destNode =
        d_clusterData_p->membership().netCluster()->lookupNode(
            msg.d_destinationNodeId);
    if (!destNode) {
        BALL_LOG_WARN << "Cannot send Raft AppendEntries to unknown node "
                      << msg.d_destinationNodeId;
        return;
    }

    bsl::shared_ptr<bdlbb::Blob> event_sp =
        d_clusterData_p->blobSpPool().getObject();
    bdlbb::Blob& event = *event_sp;

    // Reserve space for EventHeader + RaftHeader
    event.setLength(sizeof(bmqp::EventHeader) + sizeof(bmqp::RaftHeader));

    // Write RaftHeader
    bmqu::BlobObjectProxy<bmqp::RaftHeader> rh(
        &event,
        bmqu::BlobPosition(0, static_cast<int>(sizeof(bmqp::EventHeader))),
        true,   // read
        true);  // write
    (*rh)
        .setTerm(msg.d_term)
        .setPrevLogIndex(msg.d_prevLogIndex)
        .setPrevLogTerm(msg.d_prevLogTerm)
        .setLeaderCommit(msg.d_leaderCommit)
        .setEntryCount(static_cast<unsigned int>(msg.d_entries.size()));
    rh.reset();

    // Append entry blobs (CSL record blobs — same as on disk)
    for (bsl::vector<LogEntry>::size_type i = 0; i < msg.d_entries.size();
         ++i) {
        bmqu::BlobUtil::appendToBlob(&event,
                                     *msg.d_entries[i].d_data,
                                     bmqu::BlobPosition());
    }

    // Fill EventHeader
    bmqu::BlobObjectProxy<bmqp::EventHeader> eh(&event);
    (*eh) = bmqp::EventHeader(bmqp::EventType::e_RAFT_CLUSTER);
    (*eh).setLength(event.length());
    eh.reset();

    destNode->write(event_sp, bmqp::EventType::e_RAFT_CLUSTER);
}

void ClusterStateRaft::sendControlMessage(const RaftMessage& msg)
{
    mqbnet::ClusterNode* destNode =
        d_clusterData_p->membership().netCluster()->lookupNode(
            msg.d_destinationNodeId);
    if (!destNode) {
        BALL_LOG_WARN << "Cannot send Raft control message to unknown node "
                      << msg.d_destinationNodeId;
        return;
    }

    bmqp_ctrlmsg::ControlMessage controlMsg;
    bmqp_ctrlmsg::RaftMessage& raftMsg = controlMsg.choice().makeRaftMessage();
    toCtrlMsg(&raftMsg, msg);

    d_clusterData_p->messageTransmitter().sendMessage(controlMsg, destNode);
}

void ClusterStateRaft::applyCommittedEntry(const LogEntry& entry)
{
    bmqp_ctrlmsg::ClusterMessage clusterMessage(d_allocator_p);

    int rc = mqbc::ClusterStateLedgerUtil::loadClusterMessage(&clusterMessage,
                                                              *entry.d_data);
    if (rc != 0) {
        BALL_LOG_ERROR << "Failed to decode committed CSL entry, rc=" << rc;
        return;
    }

    mqbc::ClusterUtil::apply(d_clusterState_p,
                             clusterMessage,
                             *d_clusterData_p);

    BALL_LOG_INFO << "Applied committed CSL entry at term " << entry.d_term;
}

void ClusterStateRaft::toCtrlMsg(bmqp_ctrlmsg::RaftMessage* out,
                                 const RaftMessage&         msg) const
{
    BSLS_ASSERT_SAFE(out);

    out->term()        = msg.d_term;
    out->partitionId() = 0;  // CSL Raft group

    switch (msg.d_type) {
    case RaftMessageType::e_REQUEST_VOTE: {
        bmqp_ctrlmsg::RaftRequestVote& rv = out->choice().makeRequestVote();
        rv.lastLogIndex()                 = msg.d_lastLogIndex;
        rv.lastLogTerm()                  = msg.d_lastLogTerm;
        rv.preVote()                      = msg.d_preVote;
    } break;
    case RaftMessageType::e_REQUEST_VOTE_RESP: {
        bmqp_ctrlmsg::RaftRequestVoteResponse& rvr =
            out->choice().makeRequestVoteResponse();
        rvr.voteGranted() = msg.d_success;
        rvr.preVote()     = msg.d_preVote;
    } break;
    case RaftMessageType::e_APPEND_ENTRIES_RESP: {
        bmqp_ctrlmsg::RaftAppendEntriesResponse& aer =
            out->choice().makeAppendEntriesResponse();
        aer.success()    = msg.d_success;
        aer.matchIndex() = msg.d_matchIndex;
    } break;
    case RaftMessageType::e_TIMEOUT_NOW: {
        out->choice().makeTimeoutNow();
    } break;
    case RaftMessageType::e_INSTALL_SNAPSHOT: {
        bmqp_ctrlmsg::RaftInstallSnapshot& is =
            out->choice().makeInstallSnapshot();
        is.lastIncludedIndex() = msg.d_lastLogIndex;
        is.lastIncludedTerm()  = msg.d_lastLogTerm;
        is.offset()            = 0;
        is.done()              = true;
    } break;
    case RaftMessageType::e_INSTALL_SNAPSHOT_RESP: {
        out->choice().makeInstallSnapshotResponse();
    } break;
    case RaftMessageType::e_APPEND_ENTRIES:
    default: {
        // e_APPEND_ENTRIES goes through the binary path (sendAppendEntries).
        // Should not reach here.
        BSLS_ASSERT_SAFE(false);
    } break;
    }
}

void ClusterStateRaft::fromCtrlMsg(RaftMessage*                     out,
                                   const bmqp_ctrlmsg::RaftMessage& msg,
                                   int sourceNodeId) const
{
    BSLS_ASSERT_SAFE(out);

    out->d_term         = msg.term();
    out->d_sourceNodeId = sourceNodeId;

    typedef bmqp_ctrlmsg::RaftMessageChoice Choice;

    switch (msg.choice().selectionId()) {
    case Choice::SELECTION_ID_REQUEST_VOTE: {
        const bmqp_ctrlmsg::RaftRequestVote& rv = msg.choice().requestVote();
        out->d_type         = RaftMessageType::e_REQUEST_VOTE;
        out->d_lastLogIndex = rv.lastLogIndex();
        out->d_lastLogTerm  = rv.lastLogTerm();
        out->d_preVote      = rv.preVote();
    } break;
    case Choice::SELECTION_ID_REQUEST_VOTE_RESPONSE: {
        const bmqp_ctrlmsg::RaftRequestVoteResponse& rvr =
            msg.choice().requestVoteResponse();
        out->d_type    = RaftMessageType::e_REQUEST_VOTE_RESP;
        out->d_success = rvr.voteGranted();
        out->d_preVote = rvr.preVote();
    } break;
    case Choice::SELECTION_ID_APPEND_ENTRIES_RESPONSE: {
        const bmqp_ctrlmsg::RaftAppendEntriesResponse& aer =
            msg.choice().appendEntriesResponse();
        out->d_type       = RaftMessageType::e_APPEND_ENTRIES_RESP;
        out->d_success    = aer.success();
        out->d_matchIndex = aer.matchIndex();
    } break;
    case Choice::SELECTION_ID_TIMEOUT_NOW: {
        out->d_type = RaftMessageType::e_TIMEOUT_NOW;
    } break;
    case Choice::SELECTION_ID_INSTALL_SNAPSHOT: {
        out->d_type = RaftMessageType::e_INSTALL_SNAPSHOT;
    } break;
    case Choice::SELECTION_ID_INSTALL_SNAPSHOT_RESPONSE: {
        out->d_type = RaftMessageType::e_INSTALL_SNAPSHOT_RESP;
    } break;
    default: break;
    }
}

void ClusterStateRaft::tickCb()
{
    d_clusterData_p->cluster().dispatcher()->execute(
        bdlf::BindUtil::bind(&ClusterStateRaft::tickDispatched, this),
        &d_clusterData_p->cluster());
}

void ClusterStateRaft::tickDispatched()
{
    RaftNodeOutput output(d_allocator_p);
    d_raftNode_mp->tick(&output);
    dispatchOutput(&output);
}

void ClusterStateRaft::updateElectorInfo()
{
    int leaderId = d_raftNode_mp->leaderId();

    BALL_LOG_INFO << "ClusterStateRaft::updateElectorInfo (node "
                  << d_clusterData_p->membership().selfNode()->nodeId()
                  << "): raftState=" << d_raftNode_mp->state()
                  << ", leaderId=" << leaderId
                  << ", currentTerm=" << d_raftNode_mp->currentTerm();

    if (leaderId == RaftNode::k_INVALID_NODE_ID) {
        BALL_LOG_INFO << "ClusterStateRaft::updateElectorInfo (node "
                      << d_clusterData_p->membership().selfNode()->nodeId()
                      << "): no leader -> DORMANT/UNDEFINED";
        d_clusterData_p->electorInfo().setElectorInfo(mqbnet::ElectorState::e_DORMANT,
                                                      d_raftNode_mp->currentTerm(),
                                                      0,
                                                      mqbc::ElectorInfoLeaderStatus::e_UNDEFINED);

        return; // RETURN
    }


    mqbnet::ClusterNode* leaderNode = d_clusterData_p->membership().netCluster()->lookupNode(
        leaderId);

    if (!leaderNode) {
        BALL_LOG_WARN << "ClusterStateRaft::updateElectorInfo (node "
                      << d_clusterData_p->membership().selfNode()->nodeId()
                      << "): leaderId=" << leaderId
                      << " not found in netCluster; skipping elector update";
        return;  // RETURN
    }

    bool                        isActive;
    mqbnet::ElectorState::Enum  electorState;

    switch (d_raftNode_mp->state()) {
    case RaftState::e_LEADER:
        electorState = mqbnet::ElectorState::e_LEADER;
        isActive = true;
        break;
    case RaftState::e_CANDIDATE:
    case RaftState::e_PRE_CANDIDATE:
        electorState = mqbnet::ElectorState::e_CANDIDATE;
        isActive = false;
        break;
    case RaftState::e_FOLLOWER:
    default:
        electorState = mqbnet::ElectorState::e_FOLLOWER;
        isActive = true;
        break;
    }

    BALL_LOG_INFO << "ClusterStateRaft::updateElectorInfo (node "
                  << d_clusterData_p->membership().selfNode()->nodeId()
                  << "): leader=" << leaderNode->nodeDescription()
                  << ", electorState=" << electorState
                  << ", term=" << d_raftNode_mp->currentTerm()
                  << " -> setElectorInfo(PASSIVE)"
                  << (isActive ? ", then setLeaderStatus(ACTIVE)"
                               : ", staying PASSIVE (not active)");

    d_clusterData_p->electorInfo().setElectorInfo(electorState,
                                                  d_raftNode_mp->currentTerm(),
                                                  leaderNode,
                                                  mqbc::ElectorInfoLeaderStatus::e_PASSIVE);

    if (isActive) {
        // Raft doesn't need a healing phase: election safety guarantees
        // the leader has all committed entries. Both leader and followers
        // can immediately proceed with cluster operations.

        d_clusterData_p->electorInfo().setLeaderStatus(mqbc::ElectorInfoLeaderStatus::e_ACTIVE);
    }
}

// MANIPULATORS
int ClusterStateRaft::start(bsl::ostream& errorDescription)
{
    // Discover or create the CSL log file path.
    bsl::string      filePath(d_allocator_p);
    mqbu::StorageKey logId;

    int rc = mqbc::ClusterStateLedgerUtil::generateCslFilePath(
        &filePath,
        &logId,
        d_partitionConfig.location(),
        errorDescription,
        d_allocator_p);
    if (rc != 0) {
        return rc;  // RETURN
    }

    mqbsi::LogConfig logConfig(d_partitionConfig.maxCSLFileSize(),
                               logId,
                               filePath,
                               d_partitionConfig.preallocate(),
                               d_partitionConfig.prefaultPages(),
                               d_allocator_p);

    bsl::shared_ptr<mqbsi::Log> cslLog =
        bsl::allocate_shared<mqbsl::MemoryMappedOnDiskLog>(d_allocator_p,
                                                           logConfig,
                                                           d_allocator_p);

    rc = cslLog->open(mqbsi::Log::e_CREATE_IF_MISSING);
    if (rc != 0) {
        errorDescription << "Failed to open CSL log at '" << filePath
                         << "', rc=" << rc;
        return rc;  // RETURN
    }

    if (cslLog->outstandingNumBytes() == 0) {
        mqbc::ClusterStateFileHeader fh;
        fh.setProtocolVersion(mqbc::ClusterStateLedgerProtocol::k_VERSION)
            .setHeaderWords(mqbc::ClusterStateFileHeader::k_HEADER_NUM_WORDS)
            .setFileKey(logId);
        cslLog->write(&fh,
                      0,
                      static_cast<int>(sizeof(mqbc::ClusterStateFileHeader)));
    }

    d_cslLog_mp.load(new (*d_allocator_p)
                         CslRaftLog(cslLog,
                                    &d_clusterData_p->blobSpPool(),
                                    d_allocator_p),
                     d_allocator_p);

    rc = d_cslLog_mp->open();
    if (rc != 0) {
        errorDescription << "Failed to build CslRaftLog index, rc=" << rc;
        return rc;  // RETURN
    }

    d_raftNode_mp.load(new (*d_allocator_p) RaftNode(
                           makeRaftConfig(*d_clusterData_p, d_allocator_p),
                           d_cslLog_mp.get(),
                           d_allocator_p),
                       d_allocator_p);

    // Seed the recovered term and applied state; see
    // 'PartitionRaft::start()' for why this is needed.
    d_raftNode_mp->initRecoveredState(d_cslLog_mp->lastTerm(),
                                      d_cslLog_mp->snapshotIndex());

    bsls::TimeInterval tickInterval;
    tickInterval.setTotalMilliseconds(k_TICK_INTERVAL_MS);

    d_clusterData_p->scheduler().scheduleRecurringEvent(
        &d_tickHandle,
        tickInterval,
        bdlf::BindUtil::bind(&ClusterStateRaft::tickCb, this));

    d_isStarted = true;

    BALL_LOG_INFO << "ClusterStateRaft started for node "
                  << d_raftNode_mp->selfId();

    return 0;
}

void ClusterStateRaft::stop()
{
    if (!d_isStarted) {
        return;
    }

    d_clusterData_p->scheduler().cancelEventAndWait(&d_tickHandle);
    if (d_cslLog_mp) {
        d_cslLog_mp->close();
    }
    d_isStarted = false;

    BALL_LOG_INFO << "ClusterStateRaft stopped for node "
                  << d_raftNode_mp->selfId();
}

void ClusterStateRaft::onRaftControlMessage(
    const bmqp_ctrlmsg::RaftMessage& message,
    mqbnet::ClusterNode*             source)
{
    BSLS_ASSERT_SAFE(source);

    RaftMessage internalMsg(d_allocator_p);
    fromCtrlMsg(&internalMsg, message, source->nodeId());

    RaftNodeOutput output(d_allocator_p);
    d_raftNode_mp->step(&output, internalMsg);
    dispatchOutput(&output);
}

void ClusterStateRaft::appendEntries(const bdlbb::Blob&   event,
                                                 mqbnet::ClusterNode* source)
{
    BSLS_ASSERT_SAFE(source);

    // Parse RaftHeader after EventHeader
    bmqu::BlobPosition position;

    if (0 != bmqu::BlobUtil::findOffsetSafe(&position,
                                            event,
                                            sizeof(bmqp::EventHeader))) {
        BALL_LOG_ERROR
            << "Failed to locate RaftHeader in e_RAFT_CLUSTER event";
        return;
    }

    bmqu::BlobObjectProxy<bmqp::RaftHeader> rh(&event,
                                               position,
                                               true,    // read
                                               false);  // write
    if (!rh.isSet()) {
        BALL_LOG_ERROR << "Failed to read RaftHeader from event";
        return;
    }

    RaftMessage internalMsg(d_allocator_p);
    internalMsg.d_type         = RaftMessageType::e_APPEND_ENTRIES;
    internalMsg.d_term         = rh->term();
    internalMsg.d_sourceNodeId = source->nodeId();
    internalMsg.d_prevLogIndex = rh->prevLogIndex();
    internalMsg.d_prevLogTerm  = rh->prevLogTerm();
    internalMsg.d_leaderCommit = rh->leaderCommit();

    // Parse entry blobs after RaftHeader
    int          offset = sizeof(bmqp::EventHeader) + sizeof(bmqp::RaftHeader);
    int          remaining  = event.length() - offset;
    unsigned int entryCount = rh->entryCount();

    for (unsigned int i = 0; i < entryCount && remaining > 0; ++i) {
        // Resolve the absolute byte offset of this entry across blob buffers.
        bmqu::BlobPosition recPos;
        if (0 != bmqu::BlobUtil::findOffsetSafe(&recPos, event, offset)) {
            break;
        }

        // Each entry is a CSL record blob; read its header to get size
        bmqu::BlobObjectProxy<mqbc::ClusterStateRecordHeader> recHeader(
            &event,
            recPos,
            true,    // read
            false);  // write
        if (!recHeader.isSet()) {
            break;
        }

        int recSize = static_cast<int>(
            mqbc::ClusterStateLedgerUtil::recordSize(*recHeader));
        if (recSize <= 0 || recSize > remaining) {
            break;
        }

        // Extract the CSL record blob
        bsl::shared_ptr<bdlbb::Blob> entryBlob =
            d_clusterData_p->blobSpPool().getObject();
        bmqu::BlobUtil::appendToBlob(entryBlob.get(), event, recPos, recSize);

        internalMsg.d_entries.push_back(
            LogEntry(recHeader->electorTerm(),
                     internalMsg.d_prevLogIndex + 1 + i,
                     entryBlob));

        offset += recSize;
        remaining -= recSize;
    }

    RaftNodeOutput output(d_allocator_p);
    d_raftNode_mp->step(&output, internalMsg);
    dispatchOutput(&output);
}

int ClusterStateRaft::propose(const bmqp_ctrlmsg::ClusterMessage& advisory)
{
    bsl::shared_ptr<bdlbb::Blob> blob =
        d_clusterData_p->blobSpPool().getObject();

    bmqp_ctrlmsg::LeaderMessageSequence lms;
    lms.electorTerm()    = d_raftNode_mp->currentTerm();
    lms.sequenceNumber() = d_cslLog_mp->lastIndex() + 1;

    int rc = mqbc::ClusterStateLedgerUtil::appendRecord(
        blob.get(),
        advisory,
        lms,
        0,
        mqbc::ClusterStateRecordType::e_UPDATE,
        d_allocator_p);
    if (rc != 0) {
        return rc;
    }

    RaftNodeOutput output(d_allocator_p);
    rc = d_raftNode_mp->propose(&output, blob);
    if (rc != 0) {
        return rc;
    }

    dispatchOutput(&output);
    return 0;
}



bool ClusterStateRaft::assignQueue(const bmqt::Uri&      uri,
                                   bmqp_ctrlmsg::Status* status)
{
    bmqp_ctrlmsg::QueueAssignmentAdvisory queueAdvisory(d_allocator_p);

    bool result = mqbc::ClusterUtil::startQueueAssignment(
        &queueAdvisory,
        d_clusterState_p,
        d_clusterData_p,
        &d_clusterData_p->cluster(),
        uri,
        status,
        d_allocator_p);

    if (status->category() == bmqp_ctrlmsg::StatusCategory::E_SUCCESS) {
        BSLS_ASSERT_SAFE(result);

        bmqp_ctrlmsg::ClusterMessage clusterMessage(d_allocator_p);
        clusterMessage.choice().makeQueueAssignmentAdvisory() = queueAdvisory;

        int rc = propose(clusterMessage);
        if (rc != 0) {
            status->category() = bmqp_ctrlmsg::StatusCategory::E_REFUSED;
            status->code()     = -1;
            status->message()  = "Raft propose failed";
            result             = false;
        }
    }

    return result;
}

void ClusterStateRaft::processQueueAssignmentRequest(
    const bmqp_ctrlmsg::ControlMessage& request,
    mqbnet::ClusterNode*                requester)
{
    // executed by the cluster *DISPATCHER* thread

    BSLS_ASSERT_SAFE(requester);
    BSLS_ASSERT_SAFE(request.choice().isClusterMessageValue());
    BSLS_ASSERT_SAFE(request.choice()
                         .clusterMessage()
                         .choice()
                         .isQueueAssignmentRequestValue());

    BALL_LOG_INFO << "ClusterStateRaft: processing queueAssignment request "
                  << "from '" << requester->nodeDescription()
                  << "': " << request;

    bmqp_ctrlmsg::ControlMessage response(d_allocator_p);
    response.rId() = request.rId();

    // Only the active CSL leader can assign queues.  'startQueueAssignment'
    // (reached via 'assignQueue') asserts 'isSelfActiveLeader()', so these
    // guards must run first; a failure here tells the requester to retry or
    // wait for a new leader.
    if (!d_clusterData_p->electorInfo().isSelfLeader() ||
        !d_clusterData_p->electorInfo().isSelfActiveLeader()) {
        bmqp_ctrlmsg::Status& failure = response.choice().makeStatus();
        failure.category() = bmqp_ctrlmsg::StatusCategory::E_REFUSED;
        failure.code()     = mqbi::ClusterErrorCode::e_NOT_LEADER;
        failure.message()  = "Not an active leader";

        d_clusterData_p->messageTransmitter().sendMessage(response, requester);
        return;  // RETURN
    }

    if (bmqp_ctrlmsg::NodeStatus::E_STOPPING ==
        d_clusterData_p->membership().selfNodeStatus()) {
        bmqp_ctrlmsg::Status& failure = response.choice().makeStatus();
        failure.category() = bmqp_ctrlmsg::StatusCategory::E_REFUSED;
        failure.code()     = mqbi::ClusterErrorCode::e_STOPPING;
        failure.message()  = "Leader is stopping";

        d_clusterData_p->messageTransmitter().sendMessage(response, requester);
        return;  // RETURN
    }

    const bmqp_ctrlmsg::QueueAssignmentRequest& assignment =
        request.choice().clusterMessage().choice().queueAssignmentRequest();
    bmqt::Uri uri(assignment.queueUri(), d_allocator_p);

    // Domain/limit/duplicate checks and the CSL Raft propose all happen inside
    // 'assignQueue' (via 'ClusterUtil::startQueueAssignment'); it populates
    // 'status' accordingly.
    bmqp_ctrlmsg::Status& status = response.choice().makeStatus();
    status.category()            = bmqp_ctrlmsg::StatusCategory::E_SUCCESS;
    status.code()                = 0;
    status.message()             = "";

    assignQueue(uri, &status);

    d_clusterData_p->messageTransmitter().sendMessage(response, requester);
}

void ClusterStateRaft::unassignQueue(
    const bmqp_ctrlmsg::QueueUnAssignmentAdvisory& advisory)
{
    bmqp_ctrlmsg::ClusterMessage msg(d_allocator_p);
    msg.choice().makeQueueUnAssignmentAdvisory() = advisory;
    propose(msg);
}

mqbi::ClusterErrorCode::Enum
ClusterStateRaft::updateAppIds(const bsl::vector<bsl::string>& added,
                               const bsl::vector<bsl::string>& removed,
                               const bsl::string&              domainName,
                               const bsl::string&              uri)
{
    bmqp_ctrlmsg::QueueUpdateAdvisory queueAdvisory(d_allocator_p);

    mqbi::ClusterErrorCode::Enum rc = mqbc::ClusterUtil::startQueueUpdate(
        &queueAdvisory,
        d_clusterData_p,
        *d_clusterState_p,
        added,
        removed,
        domainName,
        uri,
        d_allocator_p);
    if (rc != mqbi::ClusterErrorCode::e_OK) {
        return rc;
    }

    bmqp_ctrlmsg::ClusterMessage clusterMessage(d_allocator_p);
    clusterMessage.choice().makeQueueUpdateAdvisory() = queueAdvisory;

    int proposeRc = propose(clusterMessage);
    if (proposeRc != 0) {
        return mqbi::ClusterErrorCode::e_UNKNOWN;
    }

    return mqbi::ClusterErrorCode::e_OK;
}

// ACCESSORS
bool ClusterStateRaft::isLeader() const
{
    return d_raftNode_mp->state() == RaftState::e_LEADER;
}

int ClusterStateRaft::leaderId() const
{
    return d_raftNode_mp->leaderId();
}

bsls::Types::Uint64 ClusterStateRaft::currentTerm() const
{
    return d_raftNode_mp->currentTerm();
}

int ClusterStateRaft::quorum() const
{
    return d_raftNode_mp->quorum();
}

}  // close package namespace
}  // close enterprise namespace
