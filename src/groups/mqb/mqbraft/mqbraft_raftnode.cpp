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

// =================
// struct ElectionMode
// =================

const char* ElectionMode::toAscii(ElectionMode::Enum value)
{
#define CASE(X)                                                               \
    case e_##X: return #X;

    switch (value) {
        CASE(NORMAL)
        CASE(FORCE)
        CASE(NEVER)
    default: return "(* UNKNOWN *)";
    }

#undef CASE
}

bsl::ostream& operator<<(bsl::ostream& stream, ElectionMode::Enum value)
{
    return stream << ElectionMode::toAscii(value);
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

// =========================
// struct RaftNode::PeerState
// =========================

RaftNode::PeerState::PeerState()
: d_isAvailable(true)
{
    reset(0);
}

void RaftNode::PeerState::reset(bsls::Types::Uint64 nextIndex)
{
    d_nextIndex             = nextIndex;
    d_matchIndex            = 0;
    d_snapshotPending       = false;
    d_snapshotPendingTicks  = 0;
    d_snapshotPendingIndex  = 0;
    d_snapshotPendingTerm   = 0;
    d_boundaryProbeRejected = false;
    // The peer's match index is unknown until it accepts something.
    d_probing      = true;
    d_probeSent    = false;
    d_stalledTicks = 0;
    d_sentCommit   = 0;
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
, d_lastAppliedCommit(0)
, d_votesReceived(allocator)
, d_electionTicks(0)
, d_electionTimeout(0)
, d_peerStates(allocator)
, d_heartbeatTicks(0)
, d_heartbeatDue(false)
, d_appendsSinceFlush(0)
, d_matchIndices(allocator)
, d_transferTargetId(k_INVALID_NODE_ID)
, d_electionMode(ElectionMode::e_NORMAL)
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

    for (bsl::vector<int>::const_iterator it = d_config.d_peerIds.begin();
         it != d_config.d_peerIds.end();
         ++it) {
        if (*it != d_config.d_selfId) {
            d_peerStates.insert(bsl::make_pair(*it, PeerState()));
        }
    }

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
        // A new election's term must exceed every term in the log.
        // 'initRecoveredState' cannot always supply it: at start the log's
        // backing files are not mapped yet, so 'lastTerm()' reads 0 and this
        // node would otherwise re-elect itself in a term a previous
        // incarnation already used.
        const bsls::Types::Uint64 lastLogTerm = d_log_p->lastTerm();
        if (lastLogTerm > d_currentTerm) {
            d_currentTerm = lastLogTerm;
        }
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

    for (bsl::unordered_map<int, PeerState>::iterator it =
             d_peerStates.begin();
         it != d_peerStates.end();
         ++it) {
        it->second.reset(nextIdx);
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

    // Assert leadership at the next round even though no peer is owed
    // anything yet; the round itself runs when the caller flushes.
    d_heartbeatDue = true;
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
        resp.d_rejectedIndex     = msg.d_prevLogIndex;
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
            resp.d_rejectedIndex     = msg.d_prevLogIndex;
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
            resp.d_rejectedIndex     = msg.d_prevLogIndex;
            output->d_messages.push_back(resp);
            return;
        }
    }

    // Append new entries (skip entries already present).  The log's own last
    // index is tracked here rather than asked for once per entry: entries
    // arrive in order, so each append leaves it at that entry's index.
    bsls::Types::Uint64 lastIndex = d_log_p->lastIndex();

    for (bsl::vector<LogEntry>::size_type i = 0; i < msg.d_entries.size();
         ++i) {
        bsls::Types::Uint64 entryIndex = msg.d_prevLogIndex + 1 + i;

        if (entryIndex <= lastIndex) {
            bsls::Types::Uint64 existingTerm = d_log_p->term(entryIndex);
            if (existingTerm == msg.d_entries[i].d_term) {
                continue;
            }
            d_log_p->truncateFrom(entryIndex);
        }

        int rc = d_log_p->append(msg.d_entries[i].d_term,
                                 msg.d_entries[i].d_data);
        if (rc != 0) {
            // The log refused it, so this node does not have it and must not
            // claim it below.  The leader keeps its optimistically advanced
            // 'nextIndex', so the retry comes from the tick stall-detector.
            BALL_LOG_ERROR << "[partition " << d_config.d_partitionId
                           << "] Node " << d_config.d_selfId
                           << " failed to append entry at index " << entryIndex
                           << ", rc=" << rc << "; truncating this batch";
            break;  // BREAK
        }
        lastIndex = entryIndex;
    }

    // Advance commit index
    if (msg.d_leaderCommit > d_commitIndex) {
        bsls::Types::Uint64 newCommit = bsl::min(msg.d_leaderCommit,
                                                 lastIndex);
        if (newCommit > d_commitIndex) {
            commitTo(output, newCommit);
        }
    }

    RaftMessage resp(d_allocator_p);
    resp.d_type              = RaftMessageType::e_APPEND_ENTRIES_RESP;
    resp.d_term              = d_currentTerm;
    resp.d_sourceNodeId      = d_config.d_selfId;
    resp.d_destinationNodeId = msg.d_sourceNodeId;
    resp.d_success           = true;
    resp.d_matchIndex        = lastIndex;
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
            it->second.d_stalledTicks = 0;

            // Only ever move 'nextIndex' forward here: sends advance it
            // optimistically, so a response to an earlier send reports a
            // 'matchIndex' behind what has already gone out.
            if (msg.d_matchIndex + 1 > it->second.d_nextIndex) {
                it->second.d_nextIndex = msg.d_matchIndex + 1;
            }
        }

        // The peer accepted (an optimistic boundary probe, or normal
        // replication): its match index is now known, so replication can
        // pipeline, and it is at/past the snapshot boundary.
        it->second.d_probing               = false;
        it->second.d_probeSent             = false;
        it->second.d_boundaryProbeRejected = false;

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
        // Every rejection is acted on, including the several that one
        // divergence draws when more than one send was outstanding: the
        // backoff below reads the peer's own last index, so they all land on
        // the same 'nextIndex'.
        BALL_LOG_INFO << "[partition " << d_config.d_partitionId << "] Node "
                      << d_config.d_selfId << " [term " << d_currentTerm
                      << "] LEADER got REJECT from peer " << msg.d_sourceNodeId
                      << " (peerMatchIndex=" << msg.d_matchIndex
                      << ", rejectedIndex=" << msg.d_rejectedIndex
                      << ", nextIndex(before)=" << it->second.d_nextIndex
                      << "); backing off and retrying";

        // Everything optimistically sent past the divergence is void.  Go
        // back to probing so the retry is the only message in flight and its
        // response is unambiguous.
        it->second.d_probing      = true;
        it->second.d_probeSent    = false;
        it->second.d_stalledTicks = 0;

        // If the peer's reported lastIndex is below the snapshot boundary, it
        // genuinely lacks 'snapshotIndex' -- an optimistic boundary probe
        // cannot succeed, so remember to go straight to InstallSnapshot on the
        // retry below (prevents re-probing when the reject leaves 'nextIndex'
        // back at 'snapshotIndex').
        if (msg.d_matchIndex < d_log_p->snapshotIndex()) {
            it->second.d_boundaryProbeRejected = true;
        }

        // The peer reports what it actually holds, which a wipe or truncation
        // can put below what it once acked.  Lowering it keeps
        // 'advanceCommitIndex' from counting entries this peer no longer has
        // toward a quorum.
        if (msg.d_matchIndex < it->second.d_matchIndex) {
            it->second.d_matchIndex = msg.d_matchIndex;
        }

        // Resume from the peer's own last index rather than stepping down one
        // at a time.  A peer whose files were wiped reports 0 and is served
        // its whole log on the next send, and repeating this for a second
        // rejection of the same divergence computes the same 'nextIndex'.
        if (msg.d_matchIndex < it->second.d_nextIndex) {
            it->second.d_nextIndex = msg.d_matchIndex + 1;
        }
        else if (it->second.d_nextIndex > 1) {
            it->second.d_nextIndex--;
        }
    }
}

void RaftNode::handleTimeoutNow(RaftNodeOutput* output, const RaftMessage& msg)
{
    BSLS_ASSERT_SAFE(output);
    (void)msg;

    if (d_state != RaftState::e_FOLLOWER) {
        return;
    }

    if (d_electionMode == ElectionMode::e_NEVER) {
        // This node is excluded from leadership; ignore a TimeoutNow (e.g. a
        // leadership transfer targeting it) rather than campaigning.
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

    // Report a term advance as a state change even when already a follower:
    // observers key their per-term state (elector status, and with it the
    // "CSL backlog applied for this term" signal) off these flags, and a term
    // advance invalidates it.  Mirrors 'handleAppendEntries'.
    if (msg.d_term > d_currentTerm) {
        d_currentTerm          = msg.d_term;
        d_votedFor             = k_INVALID_NODE_ID;
        output->d_stateChanged = true;
    }

    if (d_state != RaftState::e_FOLLOWER) {
        d_state                = RaftState::e_FOLLOWER;
        output->d_stateChanged = true;
    }

    if (d_leaderId != msg.d_sourceNodeId) {
        d_leaderId              = msg.d_sourceNodeId;
        output->d_leaderChanged = true;
    }

    resetElectionTimer();

    BALL_LOG_INFO << "[partition " << d_config.d_partitionId << "] Node "
                  << d_config.d_selfId << " received InstallSnapshot from "
                  << msg.d_sourceNodeId
                  << ", lastIncludedIndex=" << msg.d_lastLogIndex
                  << ", lastIncludedTerm=" << msg.d_lastLogTerm;

    // This node already holds the snapshot's last included entry, so
    // installing it would replace a log that is at or ahead of it (Raft 7).
    // Acknowledge instead: the leader clears its pending state and resumes
    // AppendEntries, which reconciles any divergence above this point through
    // the usual prevLogTerm check.  Both conditions are needed -- the index
    // alone can be held by an entry of a different term.
    if (msg.d_lastLogIndex <= d_log_p->lastIndex() &&
        d_log_p->term(msg.d_lastLogIndex) == msg.d_lastLogTerm) {
        BALL_LOG_INFO << "[partition " << d_config.d_partitionId << "] Node "
                      << d_config.d_selfId << " already holds index "
                      << msg.d_lastLogIndex << " at term " << msg.d_lastLogTerm
                      << " (lastIndex=" << d_log_p->lastIndex()
                      << "); acknowledging without installing";

        RaftMessage resp(d_allocator_p);
        resp.d_type              = RaftMessageType::e_INSTALL_SNAPSHOT_RESP;
        resp.d_term              = d_currentTerm;
        resp.d_sourceNodeId      = d_config.d_selfId;
        resp.d_destinationNodeId = msg.d_sourceNodeId;
        resp.d_lastLogIndex      = msg.d_lastLogIndex;
        output->d_messages.push_back(resp);
        return;  // RETURN
    }

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

    // 'msg.d_lastLogIndex' is always 0 on this response; advance from
    // 'd_snapshotPendingIndex' instead.
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

    it->second.d_snapshotPending      = false;
    it->second.d_snapshotPendingTicks = 0;

    advanceCommitIndex(output);
}

void RaftNode::flushSends(RaftNodeOutput* output)
{
    BSLS_ASSERT_SAFE(output);

    d_appendsSinceFlush = 0;

    if (d_state != RaftState::e_LEADER) {
        d_heartbeatDue = false;
        return;  // RETURN
    }

    // Where this round starts: 'output' may already carry a response or a vote
    // from the event that led here, and only messages built below may be
    // joined by a later peer of this round.
    const bsl::vector<RaftMessage>::size_type roundBegin =
        output->d_messages.size();

    // Growing this vector copies the messages already in it, entries and all.
    output->d_messages.reserve(roundBegin + d_peerStates.size());

    for (bsl::unordered_map<int, PeerState>::iterator it =
             d_peerStates.begin();
         it != d_peerStates.end();
         ++it) {
        sendAppendEntries(output, it->first, &it->second, roundBegin);
    }

    d_heartbeatDue = false;
}

void RaftNode::sendAppendEntries(
    RaftNodeOutput*                     output,
    int                                 peerId,
    PeerState*                          peer,
    bsl::vector<RaftMessage>::size_type roundBegin)
{
    BSLS_ASSERT_SAFE(output);
    BSLS_ASSERT_SAFE(peer);
    BSLS_ASSERT_SAFE(d_state == RaftState::e_LEADER);

    if (!peer->d_isAvailable) {
        return;  // RETURN
    }

    // A snapshot in flight supersedes replication entirely; 'tick()' clears it
    // on timeout.
    if (peer->d_snapshotPending) {
        return;  // RETURN
    }

    // The peer's log has forked and its rejection is still in flight: one
    // entry-carrying message at a time, so it is not streamed entries it is
    // going to discard.
    if (peer->d_probing && peer->d_probeSent) {
        return;  // RETURN
    }

    const bsls::Types::Uint64 nextIdx       = peer->d_nextIndex;
    const bsls::Types::Uint64 snapshotIndex = d_log_p->snapshotIndex();
    const bsls::Types::Uint64 lastIndex     = d_log_p->lastIndex();

    // Sends are optimistic, so 'nextIndex' runs ahead of what the peer has
    // acked.  Stop once too much is outstanding: a peer that has stopped
    // answering would otherwise be queued the log as fast as it is produced,
    // and the channel buffer neither blocks nor drops.
    // Added, not subtracted: a rejection can leave 'nextIndex' below
    // 'matchIndex', and the difference would wrap to a huge unsigned value
    // that suppresses this peer for good.
    if (nextIdx > peer->d_matchIndex + 1 + d_config.d_maxUnackedEntries) {
        return;  // RETURN
    }

    // Anchor first, so that the join below is one test for both the normal
    // path and the boundary probe.
    bsls::Types::Uint64 prevLogIndex = 0;
    bsls::Types::Uint64 prevLogTerm  = 0;

    if (nextIdx <= snapshotIndex) {
        // 'nextIdx - 1' is at or below the compacted snapshot boundary, so no
        // AppendEntries can be built from the log.  Try one optimistic
        // AppendEntries anchored at the boundary before falling back to
        // 'InstallSnapshot': a peer already at 'snapshotIndex' accepts it, one
        // genuinely behind rejects it and gets the snapshot on retry.
        if (nextIdx != snapshotIndex || peer->d_boundaryProbeRejected) {
            RaftMessage snap(d_allocator_p);
            snap.d_type              = RaftMessageType::e_INSTALL_SNAPSHOT;
            snap.d_term              = d_currentTerm;
            snap.d_sourceNodeId      = d_config.d_selfId;
            snap.d_destinationNodeId = peerId;
            snap.d_lastLogIndex      = snapshotIndex;
            snap.d_lastLogTerm       = d_log_p->snapshotTerm();
            output->d_messages.push_back(snap);

            peer->d_snapshotPending       = true;
            peer->d_snapshotPendingTicks  = 0;
            peer->d_snapshotPendingIndex  = snap.d_lastLogIndex;
            peer->d_snapshotPendingTerm   = snap.d_lastLogTerm;
            peer->d_boundaryProbeRejected = false;
            return;  // RETURN
        }

        prevLogIndex = snapshotIndex;
        prevLogTerm  = d_log_p->snapshotTerm();
    }
    else {
        peer->d_boundaryProbeRejected = false;

        prevLogIndex = nextIdx - 1;
        prevLogTerm  = d_log_p->term(prevLogIndex);
    }

    // A peer not yet told that the compaction at 'snapshotIndex' committed
    // must apply it before receiving anything past it: a replica appends
    // entries before it applies commits, so entries sent now would land in the
    // file set its rollover is about to replace.  Send the commit alone.
    const bool holdEntries = peer->d_sentCommit < snapshotIndex &&
                             snapshotIndex <= d_commitIndex;

    // What this message carries.  'holdEntries' also forces the commit through
    // regardless of configuration, since that is what releases the hold.
    const bool sendEntries = prevLogIndex < lastIndex && !holdEntries;
    const bool sendCommit  = peer->d_sentCommit < d_commitIndex &&
                            (d_config.d_broadcastHeartbeatOnCommit ||
                             holdEntries);

    if (!sendEntries && !sendCommit && !d_heartbeatDue) {
        return;  // RETURN
    }

    // A peer at the same anchor carrying the same entries is owed the same
    // bytes, so join that message rather than build a second one saying the
    // same thing.  A held peer must not join one carrying entries, nor a
    // peer owed entries join a held peer's empty one.
    RaftMessage* msg_p = 0;
    for (bsl::vector<RaftMessage>::size_type i = roundBegin;
         i < output->d_messages.size();
         ++i) {
        RaftMessage& m = output->d_messages[i];
        if (m.d_type == RaftMessageType::e_APPEND_ENTRIES &&
            m.d_prevLogIndex == prevLogIndex &&
            m.d_entries.empty() != sendEntries) {
            m.addDestination(peerId);
            msg_p = &m;
            break;
        }
    }

    if (!msg_p) {
        // Built in place: 'RaftMessage' holds the entries by value, so filling
        // a local and pushing it would copy every one of them.
        output->d_messages.emplace_back();
        msg_p = &output->d_messages.back();

        msg_p->d_type              = RaftMessageType::e_APPEND_ENTRIES;
        msg_p->d_term              = d_currentTerm;
        msg_p->d_sourceNodeId      = d_config.d_selfId;
        msg_p->d_destinationNodeId = peerId;
        msg_p->d_prevLogIndex      = prevLogIndex;
        msg_p->d_prevLogTerm       = prevLogTerm;
        msg_p->d_leaderCommit      = d_commitIndex;

        if (sendEntries) {
            d_log_p->entries(prevLogIndex + 1,
                             lastIndex + 1,
                             &msg_p->d_entries,
                             k_MAX_ENTRIES_PER_MESSAGE,
                             k_MAX_ENTRY_BYTES_PER_MESSAGE,
                             false);  // forApply
        }
    }

    peer->d_sentCommit = msg_p->d_leaderCommit;

    // An empty (heartbeat) message moves nothing, so it neither draws the
    // probe guard nor advances 'nextIndex'.
    if (!msg_p->d_entries.empty()) {
        peer->d_probeSent = peer->d_probing;

        // Advance past what was just sent, so the next round builds the next
        // non-overlapping message instead of resending these entries.  A
        // rejection recomputes 'nextIndex' from the peer's own 'matchIndex'.
        peer->d_nextIndex = prevLogIndex + msg_p->d_entries.size() + 1;
    }
}

void RaftNode::commitTo(RaftNodeOutput* output, bsls::Types::Uint64 newCommit)
{
    BSLS_ASSERT_SAFE(output);
    BSLS_ASSERT_SAFE(newCommit > d_commitIndex);
    BSLS_ASSERT_SAFE(newCommit <= d_log_p->lastIndex());

    d_commitIndex = newCommit;

    loadCommittedBatch(output);
}

void RaftNode::loadCommittedBatch(RaftNodeOutput* output)
{
    BSLS_ASSERT_SAFE(output);

    if (d_lastAppliedCommit >= d_commitIndex) {
        return;  // RETURN
    }

    const bsls::Types::Uint64 cap     = k_MAX_APPLY_PER_BATCH;
    const bsls::Types::Uint64 pending = d_commitIndex - d_lastAppliedCommit;
    const bsls::Types::Uint64 count   = bsl::min(pending, cap);

    const bsl::vector<LogEntry>::size_type before = output->d_committed.size();
    output->d_committed.reserve(before + count);

    d_log_p->entries(d_lastAppliedCommit + 1,
                     d_lastAppliedCommit + 1 + count,
                     &output->d_committed,
                     count,  // maxCount
                     0,      // maxBytes
                     true);  // forApply

    // An unreadable entry cuts the range short; the ones past it stay
    // unapplied and are retried on the next pass.
    d_lastAppliedCommit += output->d_committed.size() - before;

    if (d_lastAppliedCommit < d_commitIndex) {
        output->d_hasMoreToApply = true;
    }
}

void RaftNode::advanceCommitIndex(RaftNodeOutput* output)
{
    BSLS_ASSERT_SAFE(output);
    BSLS_ASSERT_SAFE(d_state == RaftState::e_LEADER);

    // Find the highest N such that a majority of matchIndex[i] >= N
    // and log[N].term == currentTerm.
    d_matchIndices.clear();
    d_matchIndices.push_back(d_log_p->lastIndex());  // leader's own match

    for (bsl::unordered_map<int, PeerState>::const_iterator it =
             d_peerStates.begin();
         it != d_peerStates.end();
         ++it) {
        d_matchIndices.push_back(it->second.d_matchIndex);
    }

    bsl::sort(d_matchIndices.begin(), d_matchIndices.end());

    // The median (index at quorum-1 from the end) is the highest N
    // replicated on a majority.
    unsigned int        quorumIdx = d_matchIndices.size() - quorum();
    bsls::Types::Uint64 newCommit = d_matchIndices[quorumIdx];

    if (newCommit > d_commitIndex) {
        const bsls::Types::Uint64 commitTerm = d_log_p->term(newCommit);

        // Raft §5.4.2: a leader only commits an entry from its own term.
        if (commitTerm == d_currentTerm) {
            commitTo(output, newCommit);

            // The peers this advance leaves behind learn the new commit index
            // from the next round, which the caller runs as soon as it is done
            // with the event that led here.  A replica may not deliver a PUSH
            // until it has applied the entry carrying its payload, so this
            // must not wait for the next heartbeat.
        }
    }
}

// MANIPULATORS
void RaftNode::tick(RaftNodeOutput* output)
{
    BSLS_ASSERT_SAFE(output);

    if (d_state == RaftState::e_LEADER) {
        // Clear the in-flight guard of any peer whose response never arrived,
        // so it becomes sendable again.
        for (bsl::unordered_map<int, PeerState>::iterator it =
                 d_peerStates.begin();
             it != d_peerStates.end();
             ++it) {
            if (it->second.d_snapshotPending) {
                if (++it->second.d_snapshotPendingTicks >=
                    d_config.d_snapshotTimeoutTicks) {
                    BALL_LOG_INFO
                        << "[partition " << d_config.d_partitionId << "] Node "
                        << d_config.d_selfId
                        << " timed out waiting for InstallSnapshotResp "
                        << "from " << it->first << "; retrying";
                    it->second.d_snapshotPending      = false;
                    it->second.d_snapshotPendingTicks = 0;
                }
            }

            // Sends are outstanding but 'matchIndex' has not moved: presume
            // they were dropped.  Rewind to what the peer last acked and go
            // back to probing, so the retry is the only message in flight.
            if (it->second.d_nextIndex > it->second.d_matchIndex + 1) {
                if (++it->second.d_stalledTicks >=
                    d_config.d_electionTimeoutMin) {
                    BALL_LOG_INFO
                        << "[partition " << d_config.d_partitionId << "] Node "
                        << d_config.d_selfId
                        << " timed out waiting for AppendEntriesResp from "
                        << it->first << "; retrying";
                    it->second.d_probing      = true;
                    it->second.d_probeSent    = false;
                    it->second.d_stalledTicks = 0;
                    it->second.d_nextIndex    = it->second.d_matchIndex + 1;
                }
            }
        }

        d_heartbeatTicks++;
        if (d_heartbeatTicks >= d_config.d_heartbeatInterval) {
            d_heartbeatTicks = 0;
            d_heartbeatDue   = true;
        }
    }
    else {
        d_electionTicks++;
        if (d_electionTicks >= d_electionTimeout) {
            if (d_electionMode == ElectionMode::e_NEVER) {
                // Excluded from leadership: never campaign.  Also drop any
                // sticky leader now that it has stopped heartbeating (this
                // timeout fired), so this node grants pre-votes to an eligible
                // candidate instead of denying them in 'handleRequestVote' --
                // otherwise a forced takeover ('e_FORCE' elsewhere) would
                // stall on the excluded nodes' sticky-leader denial.  It still
                // does not become a candidate itself.
                d_leaderId = k_INVALID_NODE_ID;
                // Publish the loss, else the cluster state keeps reporting the
                // silent leader as active.
                output->d_leaderChanged = true;
                resetElectionTimer();
                return;  // RETURN
            }
            // 'e_NORMAL' and 'e_FORCE' both campaign with pre-vote.  Pre-vote
            // is what prevents term inflation when a quorum is unreachable
            // (e.g. the other partition members are down or suspended):
            // without it, a forced node would bump the term every timeout and
            // never win, and on a suspended peer's resume that inflated term
            // causes churn. The excluded voters above drop their sticky leader
            // on timeout, so they still grant this pre-vote once the old
            // leader is gone.
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

int RaftNode::propose(RaftNodeOutput*                     output,
                      const bsl::shared_ptr<bdlbb::Blob>& data)
{
    BSLS_ASSERT_SAFE(output);

    if (d_state != RaftState::e_LEADER) {
        BALL_LOG_INFO << "[partition " << d_config.d_partitionId << "] Node "
                      << d_config.d_selfId << " [term " << d_currentTerm
                      << "] PROPOSE rejected: not leader (state=" << d_state
                      << ")";
        return -1;
    }

    int rc = d_log_p->append(d_currentTerm, data);
    if (rc != 0) {
        // The log refused the entry (e.g. 'format*Record' failed because the
        // active file set is out of space or unavailable).  Do NOT proceed to
        // replicate/commit a non-existent entry -- surface the failure so the
        // caller can react instead of silently stalling (an unappended entry
        // never commits, so any state keyed on it would deadlock).
        BALL_LOG_ERROR << "[partition " << d_config.d_partitionId << "] Node "
                       << d_config.d_selfId << " [term " << d_currentTerm
                       << "] PROPOSE FAILED to append to log, rc=" << rc
                       << " (lastIndex=" << d_log_p->lastIndex() << ")";
        return rc;  // RETURN
    }

    // A lone node is its own quorum, so the entry just appended is committed
    // here.  With peers there is nothing to compute: the leader's index is the
    // largest in the match set and the quorum order statistic never sits at
    // the largest, so an append alone cannot move the commit index.
    if (d_peerStates.empty()) {
        commitTo(output, d_log_p->lastIndex());
    }

    // Peers are served by the round the caller runs once it is done with the
    // batch this proposal belongs to -- unless enough has piled up that
    // waiting would hold entries back too long.  The caller's flush is not
    // bounded in time: a dispatcher batch runs until its queue drains.
    if (++d_appendsSinceFlush >= k_SEND_TRIGGER_ENTRIES) {
        flushSends(output);
    }

    return 0;
}

void RaftNode::initRecoveredState(bsls::Types::Uint64 term,
                                  bsls::Types::Uint64 commitIndex)
{
    if (term > d_currentTerm) {
        d_currentTerm = term;
    }
    if (commitIndex > d_commitIndex) {
        d_commitIndex = commitIndex;
    }
    if (commitIndex > d_lastAppliedCommit) {
        d_lastAppliedCommit = commitIndex;
    }
}

void RaftNode::setPeerAvailability(int peerNodeId, bool isAvailable)
{
    bsl::unordered_map<int, PeerState>::iterator it = d_peerStates.find(
        peerNodeId);
    if (it == d_peerStates.end()) {
        return;  // RETURN
    }

    if (it->second.d_isAvailable == isAvailable) {
        return;  // RETURN
    }

    it->second.d_isAvailable = isAvailable;

    BALL_LOG_INFO << "[partition " << d_config.d_partitionId << "] Node "
                  << d_config.d_selfId << " peer " << peerNodeId << " is now "
                  << (isAvailable ? "reachable" : "unreachable");

    // Either direction invalidates what this leader believed about the peer:
    // nothing survives the channel, and on the way back it may have restarted.
    // In particular a snapshot left marked in flight would suppress
    // replication to it until that times out.
    it->second.reset(d_log_p->lastIndex() + 1);
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
        // Catch the target up first; the round that does so runs when the
        // caller flushes, and 'handleAppendEntriesResp' sends the TimeoutNow
        // once the target reports it is current.
        flushSends(output);
    }

    return 0;
}

void RaftNode::setElectionMode(RaftNodeOutput* output, ElectionMode::Enum mode)
{
    BSLS_ASSERT_SAFE(output);

    if (d_electionMode == mode) {
        return;  // RETURN
    }

    BALL_LOG_INFO << "[partition " << d_config.d_partitionId << "] Node "
                  << d_config.d_selfId << " [term " << d_currentTerm
                  << "] election mode " << d_electionMode << " -> " << mode;

    d_electionMode = mode;

    if (mode == ElectionMode::e_FORCE) {
        // Force this node to become leader, but do NOT disrupt a healthy
        // existing leader: campaign immediately only when the seat is open
        // (leaderless).  If another node is currently leading, defer --
        // 'tick()' campaigns once that leader stops heartbeating (election
        // timeout), which is the only situation a takeover is wanted (the
        // prior leader died, or was set 'e_NEVER' and stepped down).  This
        // keeps 'set_quorum(1)' on the CSL leader (see
        // 'ClusterOrchestrator::processCommand') from stealing a partition
        // primary held by another node -- the CSL leader and a partition's
        // primary are frequently different nodes.
        //
        // Campaign with pre-vote (not a bare real election): pre-vote avoids
        // term inflation if a quorum is momentarily unreachable, and 'e_NEVER'
        // voters drop their sticky leader on timeout so they grant it.  The
        // real-vote path still enforces 'isLogUpToDate' (Raft safety).
        if (d_state != RaftState::e_LEADER &&
            d_leaderId == k_INVALID_NODE_ID) {
            const bool preVote = d_config.d_preVote;
            becomeCandidate(output, preVote);
            output->d_stateChanged = true;
            maybeCompleteElection(output, preVote);
        }
    }
    // 'e_NEVER' applies to future elections only (see 'tick' and
    // 'handleTimeoutNow'); an incumbent leader keeps leading.
}

}  // close package namespace
}  // close enterprise namespace
