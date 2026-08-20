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
#include <mqbs_storageutil.h>
#include <mqbsl_memorymappedondisklog.h>

#include <bmqt_uri.h>

#include <bmqtsk_alarmlog.h>
#include <bmqu_blobobjectproxy.h>

// BDE
#include <ball_log.h>
#include <bdlf_bind.h>
#include <bdls_filesystemutil.h>
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
    const AvailabilityCb&          availabilityCb,
    bslma::Allocator*              allocator)
: d_partitionConfig(partitionConfig, allocator)
, d_cslLog_mp()
, d_raftNode_mp()
, d_clusterData_p(clusterData)
, d_clusterState_p(clusterState)
, d_tickHandle()
, d_availabilityCb(bsl::allocator_arg, allocator, availabilityCb)
, d_isStarted(false)
, d_allocator_p(bslma::Default::allocator(allocator))
{
    BSLS_ASSERT_SAFE(clusterData);
    BSLS_ASSERT_SAFE(clusterState);
    BSLS_ASSERT_SAFE(availabilityCb);
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
        else if (msg.d_type == RaftMessageType::e_APPEND_ENTRIES_RESP) {
            sendAppendEntriesResponse(msg);
        }
        else {
            sendControlMessage(msg);

            if (msg.d_type == RaftMessageType::e_INSTALL_SNAPSHOT) {
                // The control message above is metadata only; the snapshot
                // itself follows on the same channel.
                sendSnapshotRecord(msg);
            }
        }
    }

    if (output->d_stateChanged || output->d_leaderChanged) {
        // Resets the leader status to 'e_PASSIVE' (or clears the leader
        // outright).  Promotion back to 'e_ACTIVE' is deliberately not done
        // here -- see 'promoteToActive' below.
        updateElectorInfo();

        // No separate become-leader no-op is proposed here.  The CSL leader's
        // first current-term entry is instead the artificial
        // 'partitionPrimaryAdvisory' issued (via the orchestrator) once every
        // partition has a leader; committing it both commits/applies the
        // recovered backlog (Raft 5.4.2, exactly as a no-op would) and
        // publishes the leaseIds.
        //
        // A leadership change also lets the orchestrator re-evaluate/re-issue
        // that advisory.  Other entry commits (queue assignments, app
        // updates, ...) do not re-invoke the callback here; see
        // 'promoteToActive' for the per-advisory trigger.
        if (d_availabilityCb) {
            d_availabilityCb();
        }
    }

    bool caughtUp = false;
    for (bsl::vector<LogEntry>::size_type i = 0;
         i < output->d_committed.size();
         ++i) {
        caughtUp |= applyCommittedEntry(output->d_committed[i]);
    }

    // Promote after the whole batch is applied: this fires observers that
    // 'propose', which in a single-node cluster re-enters here inline.
    if (caughtUp) {
        promoteToActive();
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

    // Reserve space for EventHeader + RaftHeader + RaftAppendEntriesHeader
    event.setLength(sizeof(bmqp::EventHeader) + sizeof(bmqp::RaftHeader) +
                    sizeof(bmqp::RaftAppendEntriesHeader));

    // Write RaftHeader
    bmqu::BlobObjectProxy<bmqp::RaftHeader> rh(
        &event,
        bmqu::BlobPosition(0, static_cast<int>(sizeof(bmqp::EventHeader))),
        true,   // read
        true);  // write
    (*rh)
        .setMsgType(bmqp::RaftHeader::k_MSG_TYPE_APPEND_ENTRIES)
        .setTerm(msg.d_term);
    rh.reset();

    // Write RaftAppendEntriesHeader
    bmqu::BlobObjectProxy<bmqp::RaftAppendEntriesHeader> aeh(
        &event,
        bmqu::BlobPosition(0,
                           static_cast<int>(sizeof(bmqp::EventHeader) +
                                            sizeof(bmqp::RaftHeader))),
        true,   // read
        true);  // write
    (*aeh)
        .setPrevLogIndex(msg.d_prevLogIndex)
        .setPrevLogTerm(msg.d_prevLogTerm)
        .setLeaderCommit(msg.d_leaderCommit)
        .setEntryCount(static_cast<unsigned int>(msg.d_entries.size()));
    aeh.reset();

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

void ClusterStateRaft::sendAppendEntriesResponse(const RaftMessage& msg)
{
    mqbnet::ClusterNode* destNode =
        d_clusterData_p->membership().netCluster()->lookupNode(
            msg.d_destinationNodeId);
    if (!destNode) {
        BALL_LOG_WARN
            << "Cannot send Raft AppendEntries response to unknown node "
            << msg.d_destinationNodeId;
        return;  // RETURN
    }

    bsl::shared_ptr<bdlbb::Blob> event_sp =
        d_clusterData_p->blobSpPool().getObject();
    bdlbb::Blob& event = *event_sp;

    event.setLength(sizeof(bmqp::EventHeader) + sizeof(bmqp::RaftHeader) +
                    sizeof(bmqp::RaftResponseHeader));

    bmqu::BlobObjectProxy<bmqp::RaftHeader> rh(
        &event,
        bmqu::BlobPosition(0, static_cast<int>(sizeof(bmqp::EventHeader))),
        true,   // read
        true);  // write
    (*rh)
        .setMsgType(bmqp::RaftHeader::k_MSG_TYPE_APPEND_ENTRIES_RESP)
        .setTerm(msg.d_term);
    rh.reset();

    bmqu::BlobObjectProxy<bmqp::RaftResponseHeader> resp(
        &event,
        bmqu::BlobPosition(0,
                           static_cast<int>(sizeof(bmqp::EventHeader) +
                                            sizeof(bmqp::RaftHeader))),
        true,   // read
        true);  // write
    (*resp)
        .setSuccess(msg.d_success)
        .setMatchIndex(msg.d_matchIndex)
        .setRejectedIndex(msg.d_rejectedIndex);
    resp.reset();

    bmqu::BlobObjectProxy<bmqp::EventHeader> eh(&event);
    (*eh) = bmqp::EventHeader(bmqp::EventType::e_RAFT_CLUSTER);
    (*eh).setLength(event.length());
    eh.reset();

    destNode->write(event_sp, bmqp::EventType::e_RAFT_CLUSTER);
}

void ClusterStateRaft::sendSnapshotRecord(const RaftMessage& msg)
{
    mqbnet::ClusterNode* destNode =
        d_clusterData_p->membership().netCluster()->lookupNode(
            msg.d_destinationNodeId);
    if (!destNode) {
        BALL_LOG_WARN << "Cannot send CSL snapshot to unknown node "
                      << msg.d_destinationNodeId;
        return;  // RETURN
    }

    bsl::shared_ptr<bdlbb::Blob> record;
    int                          rc = d_cslLog_mp->loadSnapshotRecord(&record);
    if (rc != 0) {
        BALL_LOG_ERROR << "Failed to load CSL snapshot record for node "
                       << msg.d_destinationNodeId << ", rc=" << rc;
        return;  // RETURN
    }

    BALL_LOG_INFO << "Sending CSL snapshot to node " << msg.d_destinationNodeId
                  << ", lastIncludedIndex=" << msg.d_lastLogIndex
                  << ", lastIncludedTerm=" << msg.d_lastLogTerm << ", "
                  << record->length() << " bytes";

    bsl::shared_ptr<bdlbb::Blob> event_sp =
        d_clusterData_p->blobSpPool().getObject();
    bdlbb::Blob& event = *event_sp;

    event.setLength(static_cast<int>(sizeof(bmqp::EventHeader) +
                                     sizeof(bmqp::SnapshotChunkHeader)));

    bmqu::BlobObjectProxy<bmqp::SnapshotChunkHeader> hdr(
        &event,
        bmqu::BlobPosition(0, static_cast<int>(sizeof(bmqp::EventHeader))),
        true,   // read
        true);  // write
    (*hdr)
        .setPartitionId(0)
        .setFileType(bmqp::SnapshotChunkHeader::k_FILE_TYPE_CSL)
        .setDone(true)
        .setLastIncludedIndex(msg.d_lastLogIndex)
        .setOffset(0)
        .setTotalSize(record->length())
        .setChunkLength(record->length());
    hdr.reset();

    bmqu::BlobUtil::appendToBlob(&event, *record, bmqu::BlobPosition());

    bmqu::BlobObjectProxy<bmqp::EventHeader> eh(&event);
    (*eh) = bmqp::EventHeader(bmqp::EventType::e_RAFT_SNAPSHOT);
    (*eh).setLength(event.length());
    eh.reset();

    destNode->write(event_sp, bmqp::EventType::e_RAFT_SNAPSHOT);
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

bool ClusterStateRaft::applyCommittedEntry(const LogEntry& entry)
{
    bool caughtUp = false;

    bmqp_ctrlmsg::ClusterMessage clusterMessage(d_allocator_p);

    int rc = mqbc::ClusterStateLedgerUtil::loadClusterMessage(&clusterMessage,
                                                              *entry.d_data);
    if (rc != 0) {
        BALL_LOG_ERROR << "Failed to decode committed CSL entry, rc=" << rc;
        return caughtUp;  // RETURN
    }

    if (clusterMessage.choice().selectionId() ==
        bmqp_ctrlmsg::ClusterMessageChoice::
            SELECTION_ID_PARTITION_PRIMARY_ADVISORY) {
        // Don't apply to ClusterState: under Raft the data-partition Raft, not
        // the CSL, owns partition primary/leaseId, so 'ClusterUtil::apply' ->
        // 'applyPartitionPrimary' is a no-op ('isRaftEnabled()' early-return)
        // and the Raft commit already guarantees persistence.
        //
        // The orchestrator declares a partition ready only once two leaseIds
        // agree: the one this CSL advisory just committed, and the one
        // observed locally from data-partition Raft leadership
        // ('primaryLeaseId').  The two can arrive in either order, so store
        // the advisory's leaseId in its own field, 'advisoryConfirmedLeaseId',
        // rather than overwriting 'primaryLeaseId'.  Update it monotonically
        // so a stale advisory replayed at recovery can't lower a leaseId a
        // later commit confirmed.
        // Only a current-term advisory describes the live epoch (Raft 5.4.2);
        // one replayed from a prior term confirms nothing.
        if (entry.d_term == d_raftNode_mp->currentTerm()) {
            const bmqp_ctrlmsg::PartitionPrimaryAdvisory& adv =
                clusterMessage.choice().partitionPrimaryAdvisory();
            for (bsl::vector<
                     bmqp_ctrlmsg::PartitionPrimaryInfo>::const_iterator it =
                     adv.partitions().cbegin();
                 it != adv.partitions().cend();
                 ++it) {
                d_clusterState_p->setPartitionAdvisoryConfirmedLeaseId(
                    it->partitionId(),
                    it->primaryLeaseId());
            }

            caughtUp = true;
        }
    }
    else {
        mqbc::ClusterUtil::apply(d_clusterState_p,
                                 clusterMessage,
                                 *d_clusterData_p);
    }

    BALL_LOG_INFO << "Applied committed CSL entry at term " << entry.d_term
                  << ": " << clusterMessage;

    return caughtUp;
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
    case RaftMessageType::e_APPEND_ENTRIES_RESP:
    default: {
        // Both go through the binary path ('sendAppendEntries',
        // 'sendAppendEntriesResponse').  Should not reach here.
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
    case Choice::SELECTION_ID_TIMEOUT_NOW: {
        out->d_type = RaftMessageType::e_TIMEOUT_NOW;
    } break;
    case Choice::SELECTION_ID_INSTALL_SNAPSHOT: {
        const bmqp_ctrlmsg::RaftInstallSnapshot& is =
            msg.choice().installSnapshot();
        out->d_type         = RaftMessageType::e_INSTALL_SNAPSHOT;
        out->d_lastLogIndex = is.lastIncludedIndex();
        out->d_lastLogTerm  = is.lastIncludedTerm();
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
        d_clusterData_p->electorInfo().setElectorInfo(
            mqbnet::ElectorState::e_DORMANT,
            d_raftNode_mp->currentTerm(),
            0,
            mqbc::ElectorInfoLeaderStatus::e_UNDEFINED);

        return;  // RETURN
    }

    mqbnet::ClusterNode* leaderNode =
        d_clusterData_p->membership().netCluster()->lookupNode(leaderId);

    if (!leaderNode) {
        BALL_LOG_WARN << "ClusterStateRaft::updateElectorInfo (node "
                      << d_clusterData_p->membership().selfNode()->nodeId()
                      << "): leaderId=" << leaderId
                      << " not found in netCluster; skipping elector update";
        return;  // RETURN
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

    BALL_LOG_INFO << "ClusterStateRaft::updateElectorInfo (node "
                  << d_clusterData_p->membership().selfNode()->nodeId()
                  << "): leader=" << leaderNode->nodeDescription()
                  << ", electorState=" << electorState
                  << ", term=" << d_raftNode_mp->currentTerm()
                  << " -> setElectorInfo(PASSIVE)";

    d_clusterData_p->electorInfo().setElectorInfo(
        electorState,
        d_raftNode_mp->currentTerm(),
        leaderNode,
        mqbc::ElectorInfoLeaderStatus::e_PASSIVE);
}

void ClusterStateRaft::promoteToActive()
{
    // 'e_ACTIVE' means the state machine holds every committed entry, not
    // just the log: election safety covers the log, but Raft 5.4.2 defers
    // applying an inherited backlog until a current-term entry commits.  The
    // caller invokes this at that point.  Legacy and FSM stay 'e_PASSIVE'
    // until healing likewise.
    d_clusterData_p->electorInfo().setLeaderStatus(
        mqbc::ElectorInfoLeaderStatus::e_ACTIVE);

    d_availabilityCb();
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

    d_cslLog_mp.load(
        new (*d_allocator_p)
            CslRaftLog(cslLog, &d_clusterData_p->blobSpPool(), d_allocator_p),
        d_allocator_p);

    rc = d_cslLog_mp->open();
    if (rc != 0) {
        errorDescription << "Failed to build CslRaftLog index, rc=" << rc;
        return rc;  // RETURN
    }

    // Note: a recovered base snapshot (rolled-over/migration-seeded file) is
    // NOT applied here.  'CslRaftLog::open' keeps it as the first committed
    // log entry (snapshot boundary = its index - 1), so it is applied by the
    // normal commit drain once this node re-commits its backlog -- by which
    // point the cluster's 'ClusterState' observer is registered, so both
    // 'ClusterState' and the queue helper's 'd_queues' are populated
    // consistently.  Applying it here (before observer registration) would
    // populate 'ClusterState' but not 'd_queues'.

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

void ClusterStateRaft::onRaftEvent(const bdlbb::Blob&   event,
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
        return;  // RETURN
    }

    bmqu::BlobObjectProxy<bmqp::RaftHeader> rh(&event,
                                               position,
                                               true,    // read
                                               false);  // write
    if (!rh.isSet()) {
        BALL_LOG_ERROR << "Failed to read RaftHeader from event";
        return;  // RETURN
    }

    const unsigned int        msgType = rh->msgType();
    const bsls::Types::Uint64 term    = rh->term();
    rh.reset();

    switch (msgType) {
    case bmqp::RaftHeader::k_MSG_TYPE_APPEND_ENTRIES: {
        appendEntries(event, source, term);
    } break;
    case bmqp::RaftHeader::k_MSG_TYPE_APPEND_ENTRIES_RESP: {
        onAppendEntriesResponse(event, source, term);
    } break;
    default: {
        BALL_LOG_ERROR << "Ignoring e_RAFT_CLUSTER event with unknown "
                       << "message type " << msgType;
    } break;
    }
}

void ClusterStateRaft::onAppendEntriesResponse(const bdlbb::Blob&   event,
                                               mqbnet::ClusterNode* source,
                                               bsls::Types::Uint64  term)
{
    BSLS_ASSERT_SAFE(source);

    bmqu::BlobPosition position;

    if (0 != bmqu::BlobUtil::findOffsetSafe(&position,
                                            event,
                                            sizeof(bmqp::EventHeader) +
                                                sizeof(bmqp::RaftHeader))) {
        BALL_LOG_ERROR << "Failed to locate RaftResponseHeader in "
                       << "e_RAFT_CLUSTER event";
        return;  // RETURN
    }

    bmqu::BlobObjectProxy<bmqp::RaftResponseHeader> resp(&event,
                                                         position,
                                                         true,    // read
                                                         false);  // write
    if (!resp.isSet()) {
        BALL_LOG_ERROR << "Failed to read RaftResponseHeader from event";
        return;  // RETURN
    }

    RaftMessage internalMsg(d_allocator_p);
    internalMsg.d_type          = RaftMessageType::e_APPEND_ENTRIES_RESP;
    internalMsg.d_term          = term;
    internalMsg.d_sourceNodeId  = source->nodeId();
    internalMsg.d_success       = resp->success();
    internalMsg.d_matchIndex    = resp->matchIndex();
    internalMsg.d_rejectedIndex = resp->rejectedIndex();
    resp.reset();

    RaftNodeOutput output(d_allocator_p);
    d_raftNode_mp->step(&output, internalMsg);
    dispatchOutput(&output);
}

void ClusterStateRaft::appendEntries(const bdlbb::Blob&   event,
                                     mqbnet::ClusterNode* source,
                                     bsls::Types::Uint64  term)
{
    BSLS_ASSERT_SAFE(source);

    bmqu::BlobPosition aehPosition;

    if (0 != bmqu::BlobUtil::findOffsetSafe(&aehPosition,
                                            event,
                                            sizeof(bmqp::EventHeader) +
                                                sizeof(bmqp::RaftHeader))) {
        BALL_LOG_ERROR << "Failed to locate RaftAppendEntriesHeader in "
                       << "e_RAFT_CLUSTER event";
        return;  // RETURN
    }

    bmqu::BlobObjectProxy<bmqp::RaftAppendEntriesHeader> aeh(&event,
                                                             aehPosition,
                                                             true,    // read
                                                             false);  // write
    if (!aeh.isSet()) {
        BALL_LOG_ERROR << "Failed to read RaftAppendEntriesHeader from event";
        return;  // RETURN
    }

    RaftMessage internalMsg(d_allocator_p);
    internalMsg.d_type         = RaftMessageType::e_APPEND_ENTRIES;
    internalMsg.d_term         = term;
    internalMsg.d_sourceNodeId = source->nodeId();
    internalMsg.d_prevLogIndex = aeh->prevLogIndex();
    internalMsg.d_prevLogTerm  = aeh->prevLogTerm();
    internalMsg.d_leaderCommit = aeh->leaderCommit();

    // Parse entry blobs after RaftAppendEntriesHeader.  'skip' hops from one
    // record to the next, starting past the headers: resolving each record by
    // its offset from the start of 'event' would rescan the buffer list every
    // time, which is quadratic in the entries one event carries.
    int skip = sizeof(bmqp::EventHeader) + sizeof(bmqp::RaftHeader) +
               sizeof(bmqp::RaftAppendEntriesHeader);
    int          remaining     = event.length() - skip;
    unsigned int entryCount    = aeh->entryCount();
    int          incomingBytes = 0;

    bmqu::BlobPosition recPos;  // start of 'event'

    for (unsigned int i = 0; i < entryCount && remaining > 0; ++i) {
        bmqu::BlobPosition nextPos;
        if (0 !=
            bmqu::BlobUtil::findOffsetSafe(&nextPos, event, recPos, skip)) {
            break;
        }
        recPos = nextPos;

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

        if (!d_cslLog_mp->canAppend(incomingBytes + recSize)) {
            // The next entry would overflow the current CSL file.  Flush the
            // entries accumulated so far (they fit) via 'step()', then roll
            // over so the fresh file has room for the rest.  After flushing,
            // advance the running prev-log marker to the last flushed entry:
            // 'handleAppendEntries' positions entries at 'prevLogIndex + 1 +
            // <position in this batch>', so the remaining entries must form a
            // correctly-anchored follow-on AppendEntries (new prevLogIndex =
            // last flushed index, new prevLogTerm = its term).
            if (!internalMsg.d_entries.empty()) {
                RaftNodeOutput output(d_allocator_p);
                d_raftNode_mp->step(&output, internalMsg);
                dispatchOutput(&output);

                internalMsg.d_prevLogIndex += internalMsg.d_entries.size();
                internalMsg.d_prevLogTerm =
                    internalMsg.d_entries.back().d_term;
                internalMsg.d_entries.clear();
                incomingBytes = 0;
            }

            // A follower must roll over its own CSL before applying replicated
            // entries that would overflow the log file.
            int rrc = rolloverCsl();
            if (rrc != 0) {
                BALL_LOG_ERROR
                    << "Follower CSL rollover failed before applying "
                    << "replicated entries, rc=" << rrc;
                return;  // RETURN
            }
        }

        // Extract the CSL record blob
        bsl::shared_ptr<bdlbb::Blob> entryBlob =
            d_clusterData_p->blobSpPool().getObject();
        bmqu::BlobUtil::appendToBlob(entryBlob.get(), event, recPos, recSize);

        // Index by position within the CURRENT sub-batch: after a mid-batch
        // flush 'd_prevLogIndex' is advanced and 'd_entries' cleared, so the
        // outer loop counter 'i' no longer aligns with the log index.  This
        // matches how 'handleAppendEntries' places entries.
        internalMsg.d_entries.push_back(LogEntry(
            recHeader->electorTerm(),
            internalMsg.d_prevLogIndex + 1 + internalMsg.d_entries.size(),
            entryBlob));

        skip = recSize;
        remaining -= recSize;
        incomingBytes += recSize;
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

    // If this record would overflow the current CSL file, roll over first --
    // snapshot the committed state into a fresh file and drop the compacted
    // prefix -- so the append below has room.  Rollover preserves 'lastIndex'
    // (only the committed prefix is compacted), so the record's
    // already-stamped LSN ('lastIndex + 1') remains valid.
    if (!d_cslLog_mp->canAppend(blob->length())) {
        rc = rolloverCsl();
        if (rc != 0) {
            return rc;  // RETURN
        }
    }

    RaftNodeOutput output(d_allocator_p);
    rc = d_raftNode_mp->propose(&output, blob);
    if (rc != 0) {
        return rc;
    }

    dispatchOutput(&output);
    return 0;
}

void ClusterStateRaft::setElectionMode(ElectionMode::Enum mode)
{
    // executed by the cluster *DISPATCHER* thread
    BSLS_ASSERT_SAFE(d_clusterData_p->cluster().inDispatcherThread());
    BSLS_ASSERT_SAFE(d_raftNode_mp);

    RaftNodeOutput output(d_allocator_p);
    d_raftNode_mp->setElectionMode(&output, mode);
    dispatchOutput(&output);
}

int ClusterStateRaft::transferLeadership(int targetNodeId)
{
    // executed by the cluster *DISPATCHER* thread
    BSLS_ASSERT_SAFE(d_clusterData_p->cluster().inDispatcherThread());
    BSLS_ASSERT_SAFE(d_raftNode_mp);

    if (targetNodeId == d_raftNode_mp->selfId()) {
        // Already in the requested state.
        return isLeader() ? 0 : k_TRANSFER_NOT_LEADER;  // RETURN
    }

    // -1 not the leader, -2 target is not a peer; both propagate as-is.
    RaftNodeOutput output(d_allocator_p);
    const int rc = d_raftNode_mp->transferLeadership(&output, targetNodeId);
    if (rc != 0) {
        return rc;  // RETURN
    }

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << ": initiating CSL leadership transfer to node "
                  << targetNodeId;

    dispatchOutput(&output);
    return 0;
}

int ClusterStateRaft::createNewCslLog(bsl::shared_ptr<mqbsi::Log>* logOut,
                                      mqbu::StorageKey*            logIdOut,
                                      bsls::Types::Uint64          minSize)
{
    BSLS_ASSERT_SAFE(logOut);
    BSLS_ASSERT_SAFE(logIdOut);

    // Build a fresh CSL file path matching 'k_CSL_FILE_PATTERN' so both the
    // Raft and legacy readers discover it:
    //   <location>/bmq_csl_YYYYMMDD_HHMMSS_<LOGID>.bmq_csl
    // 'generateStorageKey' is time-salted, so the id (and hence the filename)
    // differs from the current file even within the same second.
    bsl::string dir(d_partitionConfig.location(), d_allocator_p);
    if (!dir.empty() && dir[dir.length() - 1] != '/') {
        dir.append(1, '/');
    }

    bsl::string filePath(dir, d_allocator_p);
    filePath.append("bmq_csl_");
    mqbc::ClusterStateLedgerUtil::appendFormattedDatetime(&filePath);
    filePath.append("_");
    mqbs::StorageUtil::generateStorageKey(logIdOut, filePath);
    char logIdStr[mqbu::StorageKey::e_KEY_LENGTH_HEX + 1];
    logIdOut->loadHex(logIdStr);
    logIdStr[mqbu::StorageKey::e_KEY_LENGTH_HEX] = '\0';
    filePath.append(logIdStr);
    filePath.append(".bmq_csl");

    const bsls::Types::Uint64 maxSize =
        bsl::max(d_partitionConfig.maxCSLFileSize(), minSize);
    if (maxSize > d_partitionConfig.maxCSLFileSize()) {
        BALL_LOG_WARN << "Creating CSL file '" << filePath << "' at "
                      << maxSize << " bytes, above the configured maximum of "
                      << d_partitionConfig.maxCSLFileSize()
                      << ": the uncommitted tail it has to carry does not fit "
                      << "in the configured size.";
    }

    mqbsi::LogConfig logConfig(maxSize,
                               *logIdOut,
                               filePath,
                               d_partitionConfig.preallocate(),
                               d_partitionConfig.prefaultPages(),
                               d_allocator_p);

    bsl::shared_ptr<mqbsi::Log> log =
        bsl::allocate_shared<mqbsl::MemoryMappedOnDiskLog>(d_allocator_p,
                                                           logConfig,
                                                           d_allocator_p);

    int rc = log->open(mqbsi::Log::e_CREATE_IF_MISSING);
    if (rc != 0) {
        return rc;  // RETURN
    }

    *logOut = log;
    return 0;
}

int ClusterStateRaft::rolloverCsl()
{
    // Compact up to the last committed (and applied) index: only committed
    // state is folded into the snapshot; entries above it are the uncommitted
    // tail and are preserved by 'CslRaftLog::rollover'.
    const bsls::Types::Uint64 compactIndex = d_raftNode_mp->commitIndex();
    const bsls::Types::Uint64 compactTerm  = d_cslLog_mp->term(compactIndex);

    if (compactIndex == 0) {
        // Nothing committed, so there is nothing to compact and a rollover
        // cannot free anything.  The record would also be the entry at index
        // 0, which is not an index a log entry can occupy.
        BALL_LOG_ERROR << "Cannot roll over the CSL: nothing is committed.";
        return -1;  // RETURN
    }

    // Build the base snapshot: the full committed cluster state serialized as
    // a 'LeaderAdvisory'.  This mirrors legacy
    // 'IncoreClusterStateLedger::onLogRolloverCb', which each node writes
    // locally on its own rollover (never broadcast).
    bmqp_ctrlmsg::ClusterMessage  clusterMessage(d_allocator_p);
    bmqp_ctrlmsg::LeaderAdvisory& advisory =
        clusterMessage.choice().makeLeaderAdvisory();
    advisory.sequenceNumber().electorTerm()    = compactTerm;
    advisory.sequenceNumber().sequenceNumber() = compactIndex;
    mqbc::ClusterUtil::loadPartitionsInfo(&advisory.partitions(),
                                          *d_clusterState_p);
    mqbc::ClusterUtil::loadQueuesInfo(&advisory.queues(), *d_clusterState_p);

    bsl::shared_ptr<bdlbb::Blob> snapshotRecord =
        d_clusterData_p->blobSpPool().getObject();
    bmqp_ctrlmsg::LeaderMessageSequence lms;
    lms.electorTerm()    = compactTerm;
    lms.sequenceNumber() = compactIndex;

    int rc = mqbc::ClusterStateLedgerUtil::appendRecord(
        snapshotRecord.get(),
        clusterMessage,
        lms,
        0,  // timestamp
        mqbc::ClusterStateRecordType::e_SNAPSHOT,
        d_allocator_p);
    if (rc != 0) {
        BALL_LOG_ERROR << "Failed to build CSL rollover snapshot, rc=" << rc;
        return rc;  // RETURN
    }

    // The new file has to hold its own header, the base snapshot, and every
    // entry above 'compactIndex' -- only committed entries are compacted
    // away, so the rest are copied forward.
    const bsls::Types::Uint64 required = sizeof(mqbc::ClusterStateFileHeader) +
                                         snapshotRecord->length() +
                                         d_cslLog_mp->bytesAbove(compactIndex);

    // Create the new (empty) CSL file.
    bsl::shared_ptr<mqbsi::Log> newLog;
    mqbu::StorageKey            newLogId;
    rc = createNewCslLog(&newLog, &newLogId, required);
    if (rc != 0) {
        BALL_LOG_ERROR << "Failed to create new CSL log for rollover, rc="
                       << rc;
        return rc;  // RETURN
    }

    // Roll over: 'newLog' gets [file header | base snapshot | uncommitted
    // tail], we switch to it, and the old log is handed back for cleanup.
    bsl::shared_ptr<mqbsi::Log> oldLog;
    rc = d_cslLog_mp->rollover(&oldLog,
                               newLog,
                               newLogId,
                               snapshotRecord,
                               compactIndex,
                               compactTerm);
    if (rc != 0) {
        // The CSL keeps its old file, so it is still readable -- but it is
        // also still full, and nothing else shrinks it: the append that
        // needed this rollover keeps failing, so nothing commits, so the
        // uncommitted tail never gets compacted.  Alarm rather than log.
        BMQTSK_ALARMLOG_ALARM("CLUSTER_STATE")
            << "CSL rollover failed, rc=" << rc << ", compacting at index "
            << compactIndex << " with " << required
            << " bytes required.  The CSL cannot accept new records."
            << BMQTSK_ALARMLOG_END;

        const bsl::string newPath(newLog->logConfig().location(),
                                  d_allocator_p);
        newLog->close();
        bdls::FilesystemUtil::remove(newPath);
        return rc;  // RETURN
    }

    const bsl::string oldPath(oldLog->logConfig().location(), d_allocator_p);

    BALL_LOG_INFO << "Rolling over from log with logId; old CSL file '"
                  << oldPath << "' -> new CSL file '"
                  << newLog->logConfig().location() << "', compacted at index "
                  << compactIndex << " (term " << compactTerm << ")";

    // Close and remove the old file so only the new CSL file remains.
    oldLog->close();
    bdls::FilesystemUtil::remove(oldPath);

    return 0;
}

void ClusterStateRaft::applySnapshotChunk(const bdlbb::Blob&   event,
                                          mqbnet::ClusterNode* source)
{
    // executed by the cluster *DISPATCHER* thread
    BSLS_ASSERT_SAFE(source);

    const int recOffset = static_cast<int>(sizeof(bmqp::EventHeader) +
                                           sizeof(bmqp::SnapshotChunkHeader));

    bmqu::BlobPosition hdrPos;
    if (0 != bmqu::BlobUtil::findOffsetSafe(&hdrPos,
                                            event,
                                            sizeof(bmqp::EventHeader))) {
        BALL_LOG_ERROR << "Failed to locate SnapshotChunkHeader in "
                       << "e_RAFT_SNAPSHOT event";
        return;  // RETURN
    }

    bmqu::BlobObjectProxy<bmqp::SnapshotChunkHeader> hdr(&event,
                                                         hdrPos,
                                                         true,    // read
                                                         false);  // write
    if (!hdr.isSet()) {
        BALL_LOG_ERROR << "Failed to read SnapshotChunkHeader from "
                       << source->nodeDescription();
        return;  // RETURN
    }

    const bsls::Types::Uint64 advertisedIndex = hdr->lastIncludedIndex();
    hdr.reset();

    // The record carries its own term and index in the legacy CSL record
    // header, so the chunk is self-describing: it does not depend on the
    // metadata control message having arrived first.
    bmqu::BlobPosition recPos;
    if (0 != bmqu::BlobUtil::findOffsetSafe(&recPos, event, recOffset)) {
        BALL_LOG_ERROR << "Failed to locate CSL snapshot record in "
                       << "e_RAFT_SNAPSHOT event";
        return;  // RETURN
    }

    bmqu::BlobObjectProxy<mqbc::ClusterStateRecordHeader> recHeader(
        &event,
        recPos,
        true,    // read
        false);  // write
    if (!recHeader.isSet() ||
        recHeader->recordType() != mqbc::ClusterStateRecordType::e_SNAPSHOT) {
        BALL_LOG_ERROR << "Malformed CSL snapshot record from "
                       << source->nodeDescription();
        return;  // RETURN
    }

    const bsls::Types::Uint64 lastIncludedIndex = recHeader->sequenceNumber();
    const bsls::Types::Uint64 lastIncludedTerm  = recHeader->electorTerm();
    const int recSize = mqbc::ClusterStateLedgerUtil::recordSize(*recHeader);
    recHeader.reset();

    if (recSize <= 0 || recOffset + recSize > event.length()) {
        BALL_LOG_ERROR << "Truncated CSL snapshot record from "
                       << source->nodeDescription();
        return;  // RETURN
    }

    // A base snapshot is the entry at its own sequence number, so index 0 is
    // not one.  'open()' skips such a record for the same reason.
    if (lastIncludedIndex == 0) {
        BALL_LOG_ERROR << "Refusing CSL snapshot from "
                       << source->nodeDescription()
                       << ": record has sequence number 0";
        return;  // RETURN
    }

    // The sender advertises its own 'snapshotIndex()', which is one below the
    // record it ships (see 'CslRaftLog::rollover' and '::installSnapshot').
    // Installing a record the sender frames differently is what leaves the
    // two disagreeing about 'prevLogIndex' afterwards, so refuse it: no
    // install and no response, which surfaces as the leader retrying.
    if (advertisedIndex + 1 != lastIncludedIndex) {
        BALL_LOG_ERROR << "Refusing CSL snapshot from "
                       << source->nodeDescription() << ": record is the entry "
                       << "at index " << lastIncludedIndex
                       << ", but the sender advertised snapshotIndex "
                       << advertisedIndex << " (expected "
                       << (lastIncludedIndex - 1) << ")";
        return;  // RETURN
    }

    if (lastIncludedIndex <= d_cslLog_mp->snapshotIndex() + 1) {
        // Already installed: 'installSnapshot' leaves 'snapshotIndex()' one
        // below the record's index.  Answer anyway -- the leader resends on
        // every timeout until it hears back.
        BALL_LOG_INFO << "Ignoring CSL snapshot at index " << lastIncludedIndex
                      << " from " << source->nodeDescription()
                      << ": already at " << d_cslLog_mp->snapshotIndex();
        sendInstallSnapshotResponse(source, lastIncludedIndex);
        return;  // RETURN
    }

    bmqp_ctrlmsg::ClusterMessage clusterMessage(d_allocator_p);
    int rc = mqbc::ClusterStateLedgerUtil::loadClusterMessage(&clusterMessage,
                                                              event,
                                                              recOffset);
    if (rc != 0 || !clusterMessage.choice().isLeaderAdvisoryValue()) {
        BALL_LOG_ERROR << "Failed to decode CSL snapshot from "
                       << source->nodeDescription() << ", rc=" << rc;
        return;  // RETURN
    }

    BALL_LOG_INFO << "Installing CSL snapshot from "
                  << source->nodeDescription() << " at index "
                  << lastIncludedIndex << " (term " << lastIncludedTerm
                  << "): " << clusterMessage;

    // Copy the record out of the event: it seeds the new log file.
    bsl::shared_ptr<bdlbb::Blob> record =
        d_clusterData_p->blobSpPool().getObject();
    if (0 !=
        bmqu::BlobUtil::appendToBlob(record.get(), event, recPos, recSize)) {
        BALL_LOG_ERROR << "Failed to extract CSL snapshot record from "
                       << source->nodeDescription();
        return;  // RETURN
    }

    // Swap the CSL file before touching 'ClusterState': if the write fails
    // the node keeps its old (stale but coherent) state and the leader
    // retries, rather than holding state no log backs.
    bsl::shared_ptr<mqbsi::Log> newLog;
    mqbu::StorageKey            newLogId;
    rc = createNewCslLog(&newLog, &newLogId);
    if (rc != 0) {
        BALL_LOG_ERROR << "Failed to create new CSL log for snapshot, rc="
                       << rc;
        return;  // RETURN
    }

    bsl::shared_ptr<mqbsi::Log> oldLog;
    rc = d_cslLog_mp->installSnapshot(&oldLog,
                                      newLog,
                                      newLogId,
                                      record,
                                      lastIncludedIndex,
                                      lastIncludedTerm);
    if (rc != 0) {
        BALL_LOG_ERROR << "Failed to install CSL snapshot, rc=" << rc;
        newLog->close();
        return;  // RETURN
    }

    const bsl::string oldPath(oldLog->logConfig().location(), d_allocator_p);
    oldLog->close();
    bdls::FilesystemUtil::remove(oldPath);

    // A snapshot replaces the cluster state; 'ClusterUtil::apply' merges, so
    // without clearing first, queues unassigned before 'lastIncludedIndex'
    // would survive.
    d_clusterState_p->clearQueues();
    mqbc::ClusterUtil::apply(d_clusterState_p,
                             clusterMessage,
                             *d_clusterData_p);

    // The log can no longer serve indices at or below the new boundary, so
    // commitIndex/lastApplied must move up with it.
    d_raftNode_mp->initRecoveredState(lastIncludedTerm, lastIncludedIndex);

    sendInstallSnapshotResponse(source, lastIncludedIndex);

    BALL_LOG_INFO << "CSL snapshot installed at index " << lastIncludedIndex;
}

void ClusterStateRaft::sendInstallSnapshotResponse(
    mqbnet::ClusterNode* source,
    bsls::Types::Uint64  lastIncludedIndex)
{
    BSLS_ASSERT_SAFE(source);

    RaftMessage resp(d_allocator_p);
    resp.d_type         = RaftMessageType::e_INSTALL_SNAPSHOT_RESP;
    resp.d_term         = d_raftNode_mp->currentTerm();
    resp.d_sourceNodeId = d_clusterData_p->membership().selfNode()->nodeId();
    resp.d_destinationNodeId = source->nodeId();
    resp.d_lastLogIndex      = lastIncludedIndex;

    sendControlMessage(resp);
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
