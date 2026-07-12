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

// mqbraft_raftnode.cpp -*-C++-*-
#include <mqbraft_raftnode.h>

// BDE
#include <ball_log.h>
#include <bdlb_print.h>
#include <bsl_algorithm.h>
#include <bsl_cstdlib.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace mqbraft {


// ==============
// class RaftNode
// ==============

const int                 RaftNode::k_INVALID_NODE_ID;
const bsls::Types::Uint64 RaftNode::k_INVALID_TERM;

// ===============
// struct RaftState
// ===============

bsl::ostream& RaftState::print(bsl::ostream&   stream,
                               RaftState::Enum value,
                               int             level,
                               int             spacesPerLevel)
{
    bdlb::Print::indent(stream, level, spacesPerLevel);
    stream << toAscii(value);
    if (spacesPerLevel >= 0) {
        stream << '\n';
    }
    return stream;
}

const char* RaftState::toAscii(RaftState::Enum value)
{
#define CASE(X)                                                               \
    case e_##X: return #X;

    switch (value) {
        CASE(FOLLOWER)
        CASE(PRE_CANDIDATE)
        CASE(CANDIDATE)
        CASE(LEADER)
    default: return "(* UNKNOWN *)";
    }

#undef CASE
}

bsl::ostream& operator<<(bsl::ostream& stream, RaftState::Enum value)
{
    return RaftState::print(stream, value, 0, -1);
}

// =====================
// struct RaftMessageType
// =====================

bsl::ostream& RaftMessageType::print(bsl::ostream&         stream,
                                     RaftMessageType::Enum value,
                                     int                   level,
                                     int                   spacesPerLevel)
{
    bdlb::Print::indent(stream, level, spacesPerLevel);
    stream << toAscii(value);
    if (spacesPerLevel >= 0) {
        stream << '\n';
    }
    return stream;
}

const char* RaftMessageType::toAscii(RaftMessageType::Enum value)
{
#define CASE(X)                                                               \
    case e_##X: return #X;

    switch (value) {
        CASE(REQUEST_VOTE)
        CASE(REQUEST_VOTE_RESP)
        CASE(APPEND_ENTRIES)
        CASE(APPEND_ENTRIES_RESP)
        CASE(INSTALL_SNAPSHOT)
        CASE(INSTALL_SNAPSHOT_RESP)
        CASE(TIMEOUT_NOW)
    default: return "(* UNKNOWN *)";
    }

#undef CASE
}

bsl::ostream& operator<<(bsl::ostream& stream, RaftMessageType::Enum value)
{
    return RaftMessageType::print(stream, value, 0, -1);
}

// =============
// class RaftLog
// =============

RaftLog::~RaftLog()
{
}

// ==============
// class RaftNode
// ==============

// CREATORS
RaftNode::RaftNode(const RaftNodeConfig& config,
                   RaftLog*              log,
                   bslma::Allocator*     allocator)
: d_config(config, allocator)
, d_log_p(log)
, d_currentTerm(0)
, d_votedFor(k_INVALID_NODE_ID)
, d_state(RaftState::e_FOLLOWER)
, d_leaderId(k_INVALID_NODE_ID)
, d_commitIndex(0)
, d_lastApplied(0)
, d_votesReceived(allocator)
, d_electionTicks(0)
, d_electionTimeout(0)
, d_peerStates(allocator)
, d_heartbeatTicks(0)
, d_transferTargetId(k_INVALID_NODE_ID)
, d_allocator_p(bslma::Default::allocator(allocator))
{
    BSLS_ASSERT_SAFE(log);
    BSLS_ASSERT_SAFE(config.d_selfId != k_INVALID_NODE_ID);
    // Note: 'config.d_peerIds' may be empty -- a single-node cluster has no
    // peers and elects itself (see 'maybeCompleteElection').
    BSLS_ASSERT_SAFE(config.d_electionTimeoutMin > 0);
    BSLS_ASSERT_SAFE(config.d_electionTimeoutMax >=
                     config.d_electionTimeoutMin);
    BSLS_ASSERT_SAFE(config.d_heartbeatInterval > 0);

    resetElectionTimer();
}

// PRIVATE MANIPULATORS
void RaftNode::resetElectionTimer()
{
    int range = d_config.d_electionTimeoutMax - d_config.d_electionTimeoutMin +
                1;
    d_electionTimeout = d_config.d_electionTimeoutMin + (bsl::rand() % range);
    d_electionTicks   = 0;
}

void RaftNode::becomeFollower(bsls::Types::Uint64 term, int leaderId)
{
    if (term > d_currentTerm) {
        d_votedFor = k_INVALID_NODE_ID;
    }

    d_currentTerm = term;
    d_state       = RaftState::e_FOLLOWER;

    if (d_leaderId != leaderId) {
        d_leaderId = leaderId;
    }

    d_votesReceived.clear();
    d_peerStates.clear();
    d_transferTargetId = k_INVALID_NODE_ID;
    resetElectionTimer();

    BALL_LOG_INFO << "[partition " << d_config.d_partitionId << "] Node "
                  << d_config.d_selfId << " became FOLLOWER in term "
                  << d_currentTerm << ", leader=" << d_leaderId;
}

void RaftNode::becomeCandidate(RaftNodeOutput* output, bool preVote)
{
    BSLS_ASSERT_SAFE(output);

    if (!preVote) {
        d_currentTerm++;
        d_votedFor = d_config.d_selfId;
        d_state    = RaftState::e_CANDIDATE;
    }
    else {
        d_state = RaftState::e_PRE_CANDIDATE;
    }

    d_leaderId = k_INVALID_NODE_ID;
    d_votesReceived.clear();
    d_votesReceived.insert(d_config.d_selfId);
    resetElectionTimer();

    BALL_LOG_INFO << "[partition " << d_config.d_partitionId << "] Node "
                  << d_config.d_selfId << " became "
                  << (preVote ? "PRE_CANDIDATE" : "CANDIDATE") << " in term "
                  << (preVote ? d_currentTerm + 1 : d_currentTerm);

    bsls::Types::Uint64 requestTerm = preVote ? d_currentTerm + 1
                                              : d_currentTerm;

    for (bsl::vector<int>::const_iterator it = d_config.d_peerIds.begin();
         it != d_config.d_peerIds.end();
         ++it) {
        if (*it == d_config.d_selfId) {
            continue;
        }

        RaftMessage msg(d_allocator_p);
        msg.d_type              = RaftMessageType::e_REQUEST_VOTE;
        msg.d_term              = requestTerm;
        msg.d_sourceNodeId      = d_config.d_selfId;
        msg.d_destinationNodeId = *it;
        msg.d_lastLogIndex      = d_log_p->lastIndex();
        msg.d_lastLogTerm       = d_log_p->lastTerm();
        msg.d_preVote           = preVote;

        output->d_messages.push_back(msg);
    }
}

void RaftNode::becomeLeader(RaftNodeOutput* output)
{
    BSLS_ASSERT_SAFE(output);

    d_state            = RaftState::e_LEADER;
    d_leaderId         = d_config.d_selfId;
    d_transferTargetId = k_INVALID_NODE_ID;

    bsls::Types::Uint64 nextIdx = d_log_p->lastIndex() + 1;
    d_peerStates.clear();

    for (bsl::vector<int>::const_iterator it = d_config.d_peerIds.begin();
         it != d_config.d_peerIds.end();
         ++it) {
        if (*it == d_config.d_selfId) {
            continue;
        }
        PeerState ps;
        ps.d_nextIndex            = nextIdx;
        ps.d_matchIndex           = 0;
        ps.d_snapshotPending      = false;
        ps.d_snapshotPendingTicks = 0;
        ps.d_snapshotPendingIndex = 0;
        ps.d_snapshotPendingTerm  = 0;
        d_peerStates[*it]         = ps;
    }

    d_heartbeatTicks = 0;

    BALL_LOG_INFO << "[partition " << d_config.d_partitionId << "] Node "
                  << d_config.d_selfId << " became LEADER in term "
                  << d_currentTerm
                  << " with log lastIndex=" << d_log_p->lastIndex()
                  << ", lastTerm=" << d_log_p->lastTerm()
                  << ", commitIndex=" << d_commitIndex
                  << ", peers=" << d_peerStates.size()
                  << " (no become-leader no-op appended)";

    broadcastAppendEntries(output);
}

void RaftNode::handleRequestVote(RaftNodeOutput*    output,
                                 const RaftMessage& msg)
{
    BSLS_ASSERT_SAFE(output);

    bool grant = false;

    if (msg.d_preVote) {
        // Pre-vote: check if we WOULD vote, without changing state.
        // Reject if we have a leader (sticky leader).
        if (d_leaderId != k_INVALID_NODE_ID &&
            d_state == RaftState::e_FOLLOWER) {
            grant = false;
        }
        else if (msg.d_term <= d_currentTerm) {
            grant = false;
        }
        else {
            grant = isLogUpToDate(msg.d_lastLogTerm, msg.d_lastLogIndex);
        }
    }
    else {
        if (msg.d_term > d_currentTerm) {
            becomeFollower(msg.d_term, k_INVALID_NODE_ID);
        }

        if (msg.d_term < d_currentTerm) {
            grant = false;
        }
        else if (d_votedFor == k_INVALID_NODE_ID ||
                 d_votedFor == msg.d_sourceNodeId) {
            grant = isLogUpToDate(msg.d_lastLogTerm, msg.d_lastLogIndex);
            if (grant) {
                d_votedFor = msg.d_sourceNodeId;
                resetElectionTimer();
            }
        }
    }

    BALL_LOG_INFO << "[partition " << d_config.d_partitionId << "] Node "
                  << d_config.d_selfId << " [term " << d_currentTerm << "] "
                  << (grant ? "GRANTS" : "DENIES") << " "
                  << (msg.d_preVote ? "pre-vote" : "vote") << " to node "
                  << msg.d_sourceNodeId << " (candidateTerm=" << msg.d_term
                  << ", candidateLastLog=[" << msg.d_lastLogIndex << ","
                  << msg.d_lastLogTerm << "], myLastLog=["
                  << d_log_p->lastIndex() << "," << d_log_p->lastTerm()
                  << "], votedFor=" << d_votedFor << ")";

    RaftMessage resp(d_allocator_p);
    resp.d_type              = RaftMessageType::e_REQUEST_VOTE_RESP;
    resp.d_term              = msg.d_preVote ? msg.d_term : d_currentTerm;
    resp.d_sourceNodeId      = d_config.d_selfId;
    resp.d_destinationNodeId = msg.d_sourceNodeId;
    resp.d_success           = grant;
    resp.d_preVote           = msg.d_preVote;

    output->d_messages.push_back(resp);
}

void RaftNode::handleRequestVoteResp(RaftNodeOutput*    output,
                                     const RaftMessage& msg)
{
    BSLS_ASSERT_SAFE(output);

    if (msg.d_preVote) {
        if (d_state != RaftState::e_PRE_CANDIDATE) {
            return;
        }
        if (msg.d_term != d_currentTerm + 1) {
            return;
        }
    }
    else {
        if (d_state != RaftState::e_CANDIDATE) {
            return;
        }
        if (msg.d_term != d_currentTerm) {
            return;
        }
    }

    if (msg.d_success) {
        d_votesReceived.insert(msg.d_sourceNodeId);
    }

    maybeCompleteElection(output, msg.d_preVote);
}

void RaftNode::maybeCompleteElection(RaftNodeOutput* output, bool preVote)
{
    BSLS_ASSERT_SAFE(output);

    if (static_cast<int>(d_votesReceived.size()) < quorum()) {
        return;  // RETURN
    }

    if (preVote) {
        // Won the pre-vote round; begin the real election.  'becomeCandidate'
        // re-seeds 'd_votesReceived' with just the self vote, so re-evaluate:
        // in a single-node cluster the real round is also immediately decided,
        // while in a multi-node cluster this returns to await vote responses.
        becomeCandidate(output, false);
        maybeCompleteElection(output, false);
    }
    else {
        becomeLeader(output);
        output->d_stateChanged = true;
    }
}

void RaftNode::handleAppendEntries(RaftNodeOutput*    output,
                                   const RaftMessage& msg)
{
    BSLS_ASSERT_SAFE(output);

    if (msg.d_term < d_currentTerm) {
        BALL_LOG_INFO << "[partition " << d_config.d_partitionId << "] Node "
                      << d_config.d_selfId << " [term " << d_currentTerm
                      << "] REJECT AppendEntries (stale term) from node "
                      << msg.d_sourceNodeId << ", msgTerm=" << msg.d_term;
        RaftMessage resp(d_allocator_p);
        resp.d_type              = RaftMessageType::e_APPEND_ENTRIES_RESP;
        resp.d_term              = d_currentTerm;
        resp.d_sourceNodeId      = d_config.d_selfId;
        resp.d_destinationNodeId = msg.d_sourceNodeId;
        resp.d_success           = false;
        resp.d_matchIndex        = 0;
        output->d_messages.push_back(resp);
        return;
    }

    if (msg.d_term > d_currentTerm || d_state != RaftState::e_FOLLOWER) {
        becomeFollower(msg.d_term, msg.d_sourceNodeId);
        output->d_stateChanged = true;
    }

    if (d_leaderId != msg.d_sourceNodeId) {
        d_leaderId              = msg.d_sourceNodeId;
        output->d_leaderChanged = true;
    }

    resetElectionTimer();

    // Log consistency check
    if (msg.d_prevLogIndex > 0) {
        if (msg.d_prevLogIndex > d_log_p->lastIndex()) {
            BALL_LOG_INFO << "[partition " << d_config.d_partitionId
                          << "] Node " << d_config.d_selfId << " [term "
                          << d_currentTerm
                          << "] REJECT AppendEntries (log gap) from node "
                          << msg.d_sourceNodeId
                          << ", prevLogIndex=" << msg.d_prevLogIndex
                          << " > myLastIndex=" << d_log_p->lastIndex();
            RaftMessage resp(d_allocator_p);
            resp.d_type              = RaftMessageType::e_APPEND_ENTRIES_RESP;
            resp.d_term              = d_currentTerm;
            resp.d_sourceNodeId      = d_config.d_selfId;
            resp.d_destinationNodeId = msg.d_sourceNodeId;
            resp.d_success           = false;
            resp.d_matchIndex        = d_log_p->lastIndex();
            output->d_messages.push_back(resp);
            return;
        }

        bsls::Types::Uint64 existingTerm = d_log_p->term(msg.d_prevLogIndex);
        if (existingTerm != msg.d_prevLogTerm) {
            BALL_LOG_INFO << "[partition " << d_config.d_partitionId
                          << "] Node " << d_config.d_selfId << " [term "
                          << d_currentTerm
                          << "] REJECT AppendEntries (prevLogTerm mismatch) "
                          << "from node " << msg.d_sourceNodeId
                          << ", prevLogIndex=" << msg.d_prevLogIndex
                          << ", myTerm=" << existingTerm
                          << " != msgPrevLogTerm=" << msg.d_prevLogTerm
                          << "; truncating from " << msg.d_prevLogIndex;
            // Truncate conflicting entries
            d_log_p->truncateFrom(msg.d_prevLogIndex);

            RaftMessage resp(d_allocator_p);
            resp.d_type              = RaftMessageType::e_APPEND_ENTRIES_RESP;
            resp.d_term              = d_currentTerm;
            resp.d_sourceNodeId      = d_config.d_selfId;
            resp.d_destinationNodeId = msg.d_sourceNodeId;
            resp.d_success           = false;
            resp.d_matchIndex        = d_log_p->lastIndex();
            output->d_messages.push_back(resp);
            return;
        }
    }

    if (!msg.d_entries.empty()) {
        BALL_LOG_INFO << "[partition " << d_config.d_partitionId << "] Node "
                      << d_config.d_selfId << " [term " << d_currentTerm
                      << "] APPEND " << msg.d_entries.size()
                      << " ent  from node " << msg.d_sourceNodeId
                      << ", prevLogIndex=" << msg.d_prevLogIndex
                      << ", myLastIndex(before)=" << d_log_p->lastIndex();
    }

    // Append new entries (skip entries already present)
    for (bsl::vector<LogEntry>::size_type i = 0; i < msg.d_entries.size();
         ++i) {
        bsls::Types::Uint64 entryIndex = msg.d_prevLogIndex + 1 + i;

        if (entryIndex <= d_log_p->lastIndex()) {
            bsls::Types::Uint64 existingTerm = d_log_p->term(entryIndex);
            if (existingTerm == msg.d_entries[i].d_term) {
                continue;
            }
            d_log_p->truncateFrom(entryIndex);
        }

        d_log_p->append(msg.d_entries[i].d_term,
                        msg.d_entries[i].d_data);
    }

    // Advance commit index
    if (msg.d_leaderCommit > d_commitIndex) {
        bsls::Types::Uint64 newCommit = bsl::min(msg.d_leaderCommit,
                                                 d_log_p->lastIndex());
        if (newCommit > d_commitIndex) {
            BALL_LOG_INFO << "[partition " << d_config.d_partitionId
                          << "] Node " << d_config.d_selfId << " [term "
                          << d_currentTerm << "] FOLLOWER commit advance "
                          << d_commitIndex << " -> " << newCommit
                          << " (leaderCommit=" << msg.d_leaderCommit << ")";
            d_commitIndex = newCommit;

            bsl::vector<LogEntry> committed(d_allocator_p);
            if (d_lastApplied < d_commitIndex) {
                d_log_p->entries(d_lastApplied + 1,
                                 d_commitIndex + 1,
                                 &committed);
                d_lastApplied = d_commitIndex;
            }
            for (bsl::vector<LogEntry>::size_type j = 0; j < committed.size();
                 ++j) {
                output->d_committed.push_back(committed[j]);
            }
        }
    }

    RaftMessage resp(d_allocator_p);
    resp.d_type              = RaftMessageType::e_APPEND_ENTRIES_RESP;
    resp.d_term              = d_currentTerm;
    resp.d_sourceNodeId      = d_config.d_selfId;
    resp.d_destinationNodeId = msg.d_sourceNodeId;
    resp.d_success           = true;
    resp.d_matchIndex        = d_log_p->lastIndex();
    output->d_messages.push_back(resp);
}

void RaftNode::handleAppendEntriesResp(RaftNodeOutput*    output,
                                       const RaftMessage& msg)
{
    BSLS_ASSERT_SAFE(output);

    if (d_state != RaftState::e_LEADER) {
        return;
    }

    bsl::unordered_map<int, PeerState>::iterator it = d_peerStates.find(
        msg.d_sourceNodeId);
    if (it == d_peerStates.end()) {
        return;
    }

    if (msg.d_success) {
        if (msg.d_matchIndex > it->second.d_matchIndex) {
            BALL_LOG_INFO << "[partition " << d_config.d_partitionId
                          << "] Node " << d_config.d_selfId << " [term "
                          << d_currentTerm << "] LEADER peer "
                          << msg.d_sourceNodeId << " matchIndex "
                          << it->second.d_matchIndex << " -> "
                          << msg.d_matchIndex;
            it->second.d_matchIndex = msg.d_matchIndex;
            it->second.d_nextIndex  = msg.d_matchIndex + 1;
        }

        advanceCommitIndex(output);

        // Leadership transfer: if target is caught up, send TimeoutNow
        if (d_transferTargetId == msg.d_sourceNodeId &&
            it->second.d_matchIndex >= d_log_p->lastIndex()) {
            RaftMessage tn(d_allocator_p);
            tn.d_type              = RaftMessageType::e_TIMEOUT_NOW;
            tn.d_term              = d_currentTerm;
            tn.d_sourceNodeId      = d_config.d_selfId;
            tn.d_destinationNodeId = d_transferTargetId;
            output->d_messages.push_back(tn);
            d_transferTargetId = k_INVALID_NODE_ID;
        }
    }
    else {
        // Decrement nextIndex and retry
        BALL_LOG_INFO << "[partition " << d_config.d_partitionId << "] Node "
                      << d_config.d_selfId << " [term " << d_currentTerm
                      << "] LEADER got REJECT from peer " << msg.d_sourceNodeId
                      << " (peerMatchIndex=" << msg.d_matchIndex
                      << ", nextIndex(before)=" << it->second.d_nextIndex
                      << "); backing off and retrying";
        if (msg.d_matchIndex > 0 &&
            msg.d_matchIndex < it->second.d_nextIndex) {
            it->second.d_nextIndex = msg.d_matchIndex + 1;
        }
        else if (it->second.d_nextIndex > 1) {
            it->second.d_nextIndex--;
        }
        sendAppendEntries(output, msg.d_sourceNodeId);
    }
}

void RaftNode::handleTimeoutNow(RaftNodeOutput* output, const RaftMessage& msg)
{
    BSLS_ASSERT_SAFE(output);
    (void)msg;

    if (d_state != RaftState::e_FOLLOWER) {
        return;
    }

    BALL_LOG_INFO << "[partition " << d_config.d_partitionId << "] Node "
                  << d_config.d_selfId
                  << " received TimeoutNow, starting immediate election";

    becomeCandidate(output, false);
    output->d_stateChanged = true;
    // Single-node clusters have no peers to respond; self-elect immediately.
    maybeCompleteElection(output, false);
}

void RaftNode::handleInstallSnapshot(RaftNodeOutput*    output,
                                     const RaftMessage& msg)
{
    BSLS_ASSERT_SAFE(output);

    if (msg.d_term < d_currentTerm) {
        BALL_LOG_INFO << "[partition " << d_config.d_partitionId << "] Node "
                      << d_config.d_selfId
                      << " rejecting stale InstallSnapshot from "
                      << msg.d_sourceNodeId << ", term " << msg.d_term << " < "
                      << d_currentTerm;
        return;
    }

    if (msg.d_term > d_currentTerm) {
        d_currentTerm = msg.d_term;
        d_votedFor    = k_INVALID_NODE_ID;
    }

    if (d_state != RaftState::e_FOLLOWER) {
        d_state                = RaftState::e_FOLLOWER;
        output->d_stateChanged = true;
    }

    d_leaderId = msg.d_sourceNodeId;
    resetElectionTimer();

    BALL_LOG_INFO << "[partition " << d_config.d_partitionId << "] Node "
                  << d_config.d_selfId << " received InstallSnapshot from "
                  << msg.d_sourceNodeId
                  << ", lastIncludedIndex=" << msg.d_lastLogIndex
                  << ", lastIncludedTerm=" << msg.d_lastLogTerm;

    output->d_hasInstallSnapshot = true;
    output->d_installSnapshot    = msg;
}

void RaftNode::handleInstallSnapshotResp(RaftNodeOutput*    output,
                                         const RaftMessage& msg)
{
    BSLS_ASSERT_SAFE(output);

    if (d_state != RaftState::e_LEADER) {
        return;
    }

    bsl::unordered_map<int, PeerState>::iterator it = d_peerStates.find(
        msg.d_sourceNodeId);
    if (it == d_peerStates.end()) {
        return;
    }

    // 'RaftInstallSnapshotResponse' carries no payload on the wire (see
    // 'PeerState::d_snapshotPendingIndex'), so 'msg.d_lastLogIndex' here is
    // always 0 and cannot be used -- advance from what we remember sending
    // instead. Guarded on 'd_snapshotPending' so a stray/duplicate response
    // with nothing actually pending is a no-op rather than replaying a
    // stale index.
    if (it->second.d_snapshotPending) {
        BALL_LOG_INFO << "[partition " << d_config.d_partitionId << "] Node "
                      << d_config.d_selfId
                      << " received InstallSnapshot response from "
                      << msg.d_sourceNodeId << ", lastIncludedIndex="
                      << it->second.d_snapshotPendingIndex;

        if (it->second.d_snapshotPendingIndex > it->second.d_matchIndex) {
            it->second.d_matchIndex = it->second.d_snapshotPendingIndex;
            it->second.d_nextIndex  = it->second.d_snapshotPendingIndex + 1;
        }
    }

    // The peer has responded, so the snapshot is no longer in flight --
    // un-pause it and immediately resume replication instead of waiting for
    // the next heartbeat.
    it->second.d_snapshotPending      = false;
    it->second.d_snapshotPendingTicks = 0;
    sendAppendEntries(output, msg.d_sourceNodeId);

    advanceCommitIndex(output);
}

void RaftNode::broadcastAppendEntries(RaftNodeOutput* output)
{
    BSLS_ASSERT_SAFE(output);
    BSLS_ASSERT_SAFE(d_state == RaftState::e_LEADER);

    for (bsl::unordered_map<int, PeerState>::const_iterator it =
             d_peerStates.begin();
         it != d_peerStates.end();
         ++it) {
        sendAppendEntries(output, it->first);
    }
}

void RaftNode::sendAppendEntries(RaftNodeOutput* output, int peerId)
{
    BSLS_ASSERT_SAFE(output);
    BSLS_ASSERT_SAFE(d_state == RaftState::e_LEADER);

    bsl::unordered_map<int, PeerState>::iterator it = d_peerStates.find(
        peerId);
    if (it == d_peerStates.end()) {
        return;
    }

    if (it->second.d_snapshotPending) {
        // An 'InstallSnapshot' is already in flight to this peer with no
        // response yet (etcd/raft's 'ProgressStateSnapshot::IsPaused()' is
        // always true): skip it entirely rather than re-sending a snapshot
        // or, worse, an AppendEntries built from a 'nextIndex' we know is
        // stale, since we don't yet know whether the peer applied it.
        // 'tick()' clears this if it times out with no response.
        return;
    }

    bsls::Types::Uint64 nextIdx = it->second.d_nextIndex;

    if (nextIdx <= d_log_p->snapshotIndex()) {
        RaftMessage snap(d_allocator_p);
        snap.d_type              = RaftMessageType::e_INSTALL_SNAPSHOT;
        snap.d_term              = d_currentTerm;
        snap.d_sourceNodeId      = d_config.d_selfId;
        snap.d_destinationNodeId = peerId;
        snap.d_lastLogIndex      = d_log_p->snapshotIndex();
        snap.d_lastLogTerm       = d_log_p->snapshotTerm();
        output->d_messages.push_back(snap);

        it->second.d_snapshotPending      = true;
        it->second.d_snapshotPendingTicks = 0;
        it->second.d_snapshotPendingIndex = snap.d_lastLogIndex;
        it->second.d_snapshotPendingTerm  = snap.d_lastLogTerm;
        return;
    }

    bsls::Types::Uint64 prevLogIndex = nextIdx - 1;
    bsls::Types::Uint64 prevLogTerm  = d_log_p->term(prevLogIndex);

    RaftMessage msg(d_allocator_p);
    msg.d_type              = RaftMessageType::e_APPEND_ENTRIES;
    msg.d_term              = d_currentTerm;
    msg.d_sourceNodeId      = d_config.d_selfId;
    msg.d_destinationNodeId = peerId;
    msg.d_prevLogIndex      = prevLogIndex;
    msg.d_prevLogTerm       = prevLogTerm;
    msg.d_leaderCommit      = d_commitIndex;

    if (nextIdx <= d_log_p->lastIndex()) {
        d_log_p->entries(nextIdx, d_log_p->lastIndex() + 1, &msg.d_entries);
    }

    output->d_messages.push_back(msg);
}

void RaftNode::advanceCommitIndex(RaftNodeOutput* output)
{
    BSLS_ASSERT_SAFE(output);
    BSLS_ASSERT_SAFE(d_state == RaftState::e_LEADER);

    // Find the highest N such that a majority of matchIndex[i] >= N
    // and log[N].term == currentTerm.
    bsl::vector<bsls::Types::Uint64> matchIndices(d_allocator_p);
    matchIndices.push_back(d_log_p->lastIndex());  // leader's own match

    for (bsl::unordered_map<int, PeerState>::const_iterator it =
             d_peerStates.begin();
         it != d_peerStates.end();
         ++it) {
        matchIndices.push_back(it->second.d_matchIndex);
    }

    bsl::sort(matchIndices.begin(), matchIndices.end());

    // The median (index at quorum-1 from the end) is the highest N
    // replicated on a majority.
    unsigned int        quorumIdx = matchIndices.size() - quorum();
    bsls::Types::Uint64 newCommit = matchIndices[quorumIdx];

    if (newCommit > d_commitIndex) {
        const bsls::Types::Uint64 commitTerm = d_log_p->term(newCommit);
        if (commitTerm == d_currentTerm) {
            BALL_LOG_INFO << "[partition " << d_config.d_partitionId
                          << "] Node " << d_config.d_selfId << " [term "
                          << d_currentTerm << "] LEADER commit advance "
                          << d_commitIndex << " -> " << newCommit;
            d_commitIndex = newCommit;

            bsl::vector<LogEntry> committed(d_allocator_p);
            if (d_lastApplied < d_commitIndex) {
                d_log_p->entries(d_lastApplied + 1,
                                 d_commitIndex + 1,
                                 &committed);
                d_lastApplied = d_commitIndex;
            }
            for (bsl::vector<LogEntry>::size_type i = 0; i < committed.size();
                 ++i) {
                output->d_committed.push_back(committed[i]);
            }

            // Immediately broadcast AppendEntries to followers to notify them
            // of the new commitIndex, minimizing the window where they're
            // unaware of committed entries.
            if (d_config.d_broadcastHeartbeatOnCommit) {
                broadcastAppendEntries(output);
            }
        }
        else {
            // Raft §5.4.2: a leader only commits an entry from its own term.
            BALL_LOG_INFO << "[partition " << d_config.d_partitionId
                          << "] Node " << d_config.d_selfId << " [term "
                          << d_currentTerm
                          << "] LEADER commit BLOCKED: majority at index "
                          << newCommit << " but term(" << newCommit
                          << ")=" << commitTerm
                          << " != currentTerm=" << d_currentTerm;
        }
    }
}

// MANIPULATORS
void RaftNode::tick(RaftNodeOutput* output)
{
    BSLS_ASSERT_SAFE(output);

    if (d_state == RaftState::e_LEADER) {
        // Substitute for etcd/raft's transport-level snapshot-status report
        // (this implementation's snapshot send is synchronous/in-band, so
        // there is no independent transfer to report failure): if a peer's
        // 'InstallSnapshotResp' never arrives (e.g. the response was lost),
        // un-pause it after a timeout so it is not stuck forever.
        for (bsl::unordered_map<int, PeerState>::iterator it =
                 d_peerStates.begin();
             it != d_peerStates.end();
             ++it) {
            if (!it->second.d_snapshotPending) {
                continue;
            }
            if (++it->second.d_snapshotPendingTicks >=
                d_config.d_electionTimeoutMin) {
                BALL_LOG_INFO << "[partition " << d_config.d_partitionId
                              << "] Node " << d_config.d_selfId
                              << " timed out waiting for InstallSnapshotResp "
                              << "from " << it->first << "; retrying";
                it->second.d_snapshotPending      = false;
                it->second.d_snapshotPendingTicks = 0;
            }
        }

        d_heartbeatTicks++;
        if (d_heartbeatTicks >= d_config.d_heartbeatInterval) {
            d_heartbeatTicks = 0;
            broadcastAppendEntries(output);
        }
    }
    else {
        d_electionTicks++;
        if (d_electionTicks >= d_electionTimeout) {
            const bool preVote = d_config.d_preVote;
            becomeCandidate(output, preVote);
            output->d_stateChanged = true;
            // A single-node cluster has no peers to respond, so complete the
            // election immediately from the self vote.
            maybeCompleteElection(output, preVote);
        }
    }
}

void RaftNode::step(RaftNodeOutput* output, const RaftMessage& message)
{
    BSLS_ASSERT_SAFE(output);

    // All messages: if term > currentTerm, step down
    if (!message.d_preVote && message.d_term > d_currentTerm) {
        if (message.d_type == RaftMessageType::e_REQUEST_VOTE) {
            // Will be handled in handleRequestVote
        }
        else if (message.d_type == RaftMessageType::e_APPEND_ENTRIES) {
            // Will be handled in handleAppendEntries
        }
        else {
            becomeFollower(message.d_term, k_INVALID_NODE_ID);
            output->d_stateChanged = true;
        }
    }

    switch (message.d_type) {
    case RaftMessageType::e_REQUEST_VOTE: {
        handleRequestVote(output, message);
    } break;
    case RaftMessageType::e_REQUEST_VOTE_RESP: {
        handleRequestVoteResp(output, message);
    } break;
    case RaftMessageType::e_APPEND_ENTRIES: {
        handleAppendEntries(output, message);
    } break;
    case RaftMessageType::e_APPEND_ENTRIES_RESP: {
        handleAppendEntriesResp(output, message);
    } break;
    case RaftMessageType::e_TIMEOUT_NOW: {
        handleTimeoutNow(output, message);
    } break;
    case RaftMessageType::e_INSTALL_SNAPSHOT: {
        handleInstallSnapshot(output, message);
    } break;
    case RaftMessageType::e_INSTALL_SNAPSHOT_RESP: {
        handleInstallSnapshotResp(output, message);
    } break;
    default: {
        BALL_LOG_WARN << "[partition " << d_config.d_partitionId << "] Node "
                      << d_config.d_selfId
                      << " received unknown message type: " << message.d_type;
    } break;
    }
}

int RaftNode::propose(RaftNodeOutput*                      output,
                      const bsl::shared_ptr<bdlbb::Blob>&  data,
                      bsls::Types::Uint64                  id)
{
    BSLS_ASSERT_SAFE(output);

    if (d_state != RaftState::e_LEADER) {
        BALL_LOG_INFO << "[partition " << d_config.d_partitionId << "] Node "
                      << d_config.d_selfId << " [term " << d_currentTerm
                      << "] PROPOSE rejected: not leader (state=" << d_state
                      << "), id=" << id;
        return -1;
    }

    int rc = d_log_p->append(d_currentTerm, data, id);
    if (rc != 0) {
        // The log refused the entry (e.g. 'format*Record' failed because the
        // active file set is out of space or unavailable).  Do NOT proceed to
        // replicate/commit a non-existent entry -- surface the failure so the
        // caller can react instead of silently stalling (an unappended entry
        // never commits, so any state keyed on it would deadlock).
        BALL_LOG_ERROR << "[partition " << d_config.d_partitionId << "] Node "
                       << d_config.d_selfId << " [term " << d_currentTerm
                       << "] PROPOSE id=" << id
                       << " FAILED to append to log, rc=" << rc
                       << " (lastIndex=" << d_log_p->lastIndex() << ")";
        return rc;  // RETURN
    }

    BALL_LOG_INFO << "[partition " << d_config.d_partitionId << "] Node "
                  << d_config.d_selfId << " [term " << d_currentTerm
                  << "] PROPOSE id=" << id
                  << " -> newLastIndex=" << d_log_p->lastIndex()
                  << ", replicating to " << d_peerStates.size() << " peer(s)";

    broadcastAppendEntries(output);

    // Advance the commit index (leader-guarded above).  In a single-node
    // cluster this commits the just-appended entry synchronously via
    // self-quorum; in a multi-node cluster it is a no-op here because peers'
    // matchIndex has not yet advanced.
    advanceCommitIndex(output);

    return 0;
}

void RaftNode::initRecoveredState(bsls::Types::Uint64 term,
                                  bsls::Types::Uint64 index)
{
    if (term > d_currentTerm) {
        d_currentTerm = term;
    }
    if (index > d_commitIndex) {
        d_commitIndex = index;
    }
    if (index > d_lastApplied) {
        d_lastApplied = index;
    }
}

int RaftNode::transferLeadership(RaftNodeOutput* output, int targetNodeId)
{
    BSLS_ASSERT_SAFE(output);

    if (d_state != RaftState::e_LEADER) {
        return -1;
    }

    if (d_peerStates.find(targetNodeId) == d_peerStates.end()) {
        return -2;
    }

    d_transferTargetId = targetNodeId;

    PeerState& ps = d_peerStates[targetNodeId];
    if (ps.d_matchIndex >= d_log_p->lastIndex()) {
        RaftMessage tn(d_allocator_p);
        tn.d_type              = RaftMessageType::e_TIMEOUT_NOW;
        tn.d_term              = d_currentTerm;
        tn.d_sourceNodeId      = d_config.d_selfId;
        tn.d_destinationNodeId = targetNodeId;
        output->d_messages.push_back(tn);
        d_transferTargetId = k_INVALID_NODE_ID;
    }
    else {
        sendAppendEntries(output, targetNodeId);
    }

    return 0;
}

}  // close package namespace
}  // close enterprise namespace
