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
    BSLS_ASSERT_SAFE(!config.d_peerIds.empty());
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

    BALL_LOG_INFO << "Node " << d_config.d_selfId
                  << " became FOLLOWER in term " << d_currentTerm
                  << ", leader=" << d_leaderId;
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

    BALL_LOG_INFO << "Node " << d_config.d_selfId << " became "
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
        ps.d_nextIndex    = nextIdx;
        ps.d_matchIndex   = 0;
        d_peerStates[*it] = ps;
    }

    d_heartbeatTicks = 0;

    BALL_LOG_INFO << "Node " << d_config.d_selfId << " became LEADER in term "
                  << d_currentTerm;

    for (bsl::unordered_map<int, PeerState>::const_iterator it =
             d_peerStates.begin();
         it != d_peerStates.end();
         ++it) {
        sendAppendEntries(output, it->first);
    }
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

    if (static_cast<int>(d_votesReceived.size()) >= quorum()) {
        if (msg.d_preVote) {
            becomeCandidate(output, false);
        }
        else {
            becomeLeader(output);
            output->d_stateChanged = true;
        }
    }
}

void RaftNode::handleAppendEntries(RaftNodeOutput*    output,
                                   const RaftMessage& msg)
{
    BSLS_ASSERT_SAFE(output);

    if (msg.d_term < d_currentTerm) {
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

    BALL_LOG_INFO << "Node " << d_config.d_selfId
                  << " received TimeoutNow, starting immediate election";

    becomeCandidate(output, false);
    output->d_stateChanged = true;
}

void RaftNode::handleInstallSnapshot(RaftNodeOutput*    output,
                                     const RaftMessage& msg)
{
    BSLS_ASSERT_SAFE(output);

    if (msg.d_term < d_currentTerm) {
        BALL_LOG_INFO << "Node " << d_config.d_selfId
                      << " rejecting stale InstallSnapshot from "
                      << msg.d_sourceNodeId << ", term " << msg.d_term
                      << " < " << d_currentTerm;
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

    BALL_LOG_INFO << "Node " << d_config.d_selfId
                  << " received InstallSnapshot from " << msg.d_sourceNodeId
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

    BALL_LOG_INFO << "Node " << d_config.d_selfId
                  << " received InstallSnapshot response from "
                  << msg.d_sourceNodeId
                  << ", lastIncludedIndex=" << msg.d_lastLogIndex;

    if (msg.d_lastLogIndex > it->second.d_matchIndex) {
        it->second.d_matchIndex = msg.d_lastLogIndex;
        it->second.d_nextIndex  = msg.d_lastLogIndex + 1;
    }

    advanceCommitIndex(output);
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
    int quorumIdx = static_cast<int>(matchIndices.size()) - quorum();
    bsls::Types::Uint64 newCommit = matchIndices[quorumIdx];

    if (newCommit > d_commitIndex &&
        d_log_p->term(newCommit) == d_currentTerm) {
        d_commitIndex = newCommit;

        bsl::vector<LogEntry> committed(d_allocator_p);
        if (d_lastApplied < d_commitIndex) {
            d_log_p->entries(d_lastApplied + 1, d_commitIndex + 1, &committed);
            d_lastApplied = d_commitIndex;
        }
        for (bsl::vector<LogEntry>::size_type i = 0; i < committed.size();
             ++i) {
            output->d_committed.push_back(committed[i]);
        }
    }
}

// MANIPULATORS
void RaftNode::tick(RaftNodeOutput* output)
{
    BSLS_ASSERT_SAFE(output);

    if (d_state == RaftState::e_LEADER) {
        d_heartbeatTicks++;
        if (d_heartbeatTicks >= d_config.d_heartbeatInterval) {
            d_heartbeatTicks = 0;
            for (bsl::unordered_map<int, PeerState>::const_iterator it =
                     d_peerStates.begin();
                 it != d_peerStates.end();
                 ++it) {
                sendAppendEntries(output, it->first);
            }
        }
    }
    else {
        d_electionTicks++;
        if (d_electionTicks >= d_electionTimeout) {
            if (d_config.d_preVote) {
                becomeCandidate(output, true);
            }
            else {
                becomeCandidate(output, false);
            }
            output->d_stateChanged = true;
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
        BALL_LOG_WARN << "Node " << d_config.d_selfId
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
        return -1;
    }

    d_log_p->append(d_currentTerm, data, id);

    for (bsl::unordered_map<int, PeerState>::const_iterator it =
             d_peerStates.begin();
         it != d_peerStates.end();
         ++it) {
        sendAppendEntries(output, it->first);
    }

    return 0;
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
