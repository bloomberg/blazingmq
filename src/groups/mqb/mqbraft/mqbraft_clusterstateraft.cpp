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
    RaftNodeConfig config(allocator);

    mqbnet::Cluster* netCluster = clusterData.membership().netCluster();

    config.d_selfId = netCluster->selfNodeId();

    const mqbnet::Cluster::NodesList& nodes = netCluster->nodes();
    for (mqbnet::Cluster::NodesList::const_iterator it = nodes.begin();
         it != nodes.end();
         ++it) {
        config.d_peerIds.push_back((*it)->nodeId());
    }
    config.d_peerIds.push_back(config.d_selfId);

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
void ClusterStateRaft::processOutput(RaftNodeOutput* output)
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

    if (output->d_stateChanged) {
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
    bmqu::BlobObjectProxy<bmqp::RaftHeader> rh(&event,
                                               true,
                                               sizeof(bmqp::EventHeader));
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
                                     msg.d_entries[i].d_data,
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
                                                              entry.d_data);
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

    out->term() = msg.d_term;

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
    processOutput(&output);
}

void ClusterStateRaft::updateElectorInfo()
{
    mqbnet::ClusterNode* leaderNode = 0;
    int                  leadId     = d_raftNode_mp->leaderId();

    if (leadId != RaftNode::k_INVALID_NODE_ID) {
        leaderNode = d_clusterData_p->membership().netCluster()->lookupNode(
            leadId);
    }

    mqbc::ElectorInfoLeaderStatus::Enum leaderStatus =
        mqbc::ElectorInfoLeaderStatus::e_UNDEFINED;

    if (d_raftNode_mp->state() == RaftState::e_LEADER) {
        leaderStatus = mqbc::ElectorInfoLeaderStatus::e_ACTIVE;
    }
    else if (leaderNode) {
        leaderStatus = mqbc::ElectorInfoLeaderStatus::e_ACTIVE;
    }

    mqbnet::ElectorState::Enum electorState;
    switch (d_raftNode_mp->state()) {
    case RaftState::e_LEADER:
        electorState = mqbnet::ElectorState::e_LEADER;
        break;
    case RaftState::e_CANDIDATE:
    case RaftState::e_PRE_CANDIDATE:
        electorState = mqbnet::ElectorState::e_CANDIDATE;
        break;
    case RaftState::e_FOLLOWER:
    default: electorState = mqbnet::ElectorState::e_FOLLOWER; break;
    }

    d_clusterData_p->electorInfo().setElectorInfo(electorState,
                                                  d_raftNode_mp->currentTerm(),
                                                  leaderNode,
                                                  leaderStatus);
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
        cslLog->write(&fh,
                      0,
                      static_cast<int>(sizeof(mqbc::ClusterStateFileHeader)));
    }

    d_cslLog_mp.load(new (*d_allocator_p)
                         CslRaftLog(cslLog,
                                    &d_clusterData_p->bufferFactory(),
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

void ClusterStateRaft::processRaftMessage(
    const bmqp_ctrlmsg::RaftMessage& message,
    mqbnet::ClusterNode*             source)
{
    BSLS_ASSERT_SAFE(source);

    RaftMessage internalMsg(d_allocator_p);
    fromCtrlMsg(&internalMsg, message, source->nodeId());

    RaftNodeOutput output(d_allocator_p);
    d_raftNode_mp->step(&output, internalMsg);
    processOutput(&output);
}

void ClusterStateRaft::processAppendEntriesEvent(const bdlbb::Blob&   event,
                                                 mqbnet::ClusterNode* source)
{
    BSLS_ASSERT_SAFE(source);

    // Parse RaftHeader after EventHeader
    bmqu::BlobObjectProxy<bmqp::RaftHeader> rh(&event,
                                               false,
                                               sizeof(bmqp::EventHeader));
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
        // Each entry is a CSL record blob; read its header to get size
        bmqu::BlobObjectProxy<mqbc::ClusterStateRecordHeader> recHeader(
            &event,
            false,
            offset);
        if (!recHeader.isSet()) {
            break;
        }

        int recSize = static_cast<int>(
            mqbc::ClusterStateLedgerUtil::recordSize(*recHeader));
        if (recSize <= 0 || recSize > remaining) {
            break;
        }

        // Extract the CSL record blob
        bdlbb::Blob        entryBlob(&d_clusterData_p->bufferFactory(),
                              d_allocator_p);
        bmqu::BlobPosition startPos;
        bmqu::BlobUtil::findOffsetSafe(&startPos, event, offset);
        bmqu::BlobUtil::appendToBlob(&entryBlob, event, startPos, recSize);

        internalMsg.d_entries.push_back(
            LogEntry(recHeader->electorTerm(), entryBlob, d_allocator_p));

        offset += recSize;
        remaining -= recSize;
    }

    RaftNodeOutput output(d_allocator_p);
    d_raftNode_mp->step(&output, internalMsg);
    processOutput(&output);
}

int ClusterStateRaft::propose(const bmqp_ctrlmsg::ClusterMessage& advisory)
{
    bdlbb::Blob blob(&d_clusterData_p->bufferFactory(), d_allocator_p);

    bmqp_ctrlmsg::LeaderMessageSequence lms;
    lms.electorTerm()    = d_raftNode_mp->currentTerm();
    lms.sequenceNumber() = d_cslLog_mp->lastIndex() + 1;

    int rc = mqbc::ClusterStateLedgerUtil::appendRecord(
        &blob,
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

    processOutput(&output);
    return 0;
}

// ClusterStateUpdater interface
void ClusterStateRaft::setAfterPartitionPrimaryAssignmentCb(
    const AfterPartitionPrimaryAssignmentCb& value)
{
    d_afterPartitionPrimaryAssignmentCb = value;
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

}  // close package namespace
}  // close enterprise namespace
