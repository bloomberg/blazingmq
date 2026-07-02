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

// mqbraft_raftnode.h -*-C++-*-
#ifndef INCLUDED_MQBRAFT_RAFTNODE
#define INCLUDED_MQBRAFT_RAFTNODE

//@PURPOSE: Provide a pure Raft consensus state machine.
//
//@CLASSES:
//  mqbraft::RaftState:       Enum for Raft node states
//  mqbraft::RaftMessageType: Enum for Raft RPC message types
//  mqbraft::LogEntry:        VST for a single log entry (term + data)
//  mqbraft::RaftLog:         Protocol for log storage
//  mqbraft::MemoryRaftLog:   In-memory log implementation
//  mqbraft::RaftMessage:     VST for a Raft RPC message
//  mqbraft::RaftNodeConfig:  VST for RaftNode configuration
//  mqbraft::RaftNodeOutput:  VST for output produced by RaftNode
//  mqbraft::RaftNode:        Core Raft state machine
//
//@DESCRIPTION: This component implements the Raft consensus algorithm as a
// pure state machine with no I/O, no threads, and no timers.  The caller
// drives it with 'tick()' (logical clock) and 'step()' (incoming messages).
// The same 'RaftNode' class is used for both cluster metadata (CSL) and
// partition data (journal) Raft groups.
//
// The algorithm follows the Raft paper (Ongaro & Ousterhout, 2014) Figure 2,
// with the addition of pre-vote (Section 9.6) and leadership transfer.
//
/// Threading
///----------
// This component is NOT thread-safe.  All methods must be called from the
// same thread (the cluster dispatcher thread in production).

// BDE
#include <ball_log.h>
#include <bdlbb_blob.h>
#include <bsl_iosfwd.h>
#include <bsl_unordered_map.h>
#include <bsl_unordered_set.h>
#include <bsl_vector.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_assert.h>
#include <bsls_types.h>

namespace BloombergLP {
namespace mqbraft {

// ===============
// struct RaftState
// ===============

struct RaftState {
    // TYPES
    enum Enum {
        e_FOLLOWER      = 0,
        e_PRE_CANDIDATE = 1,
        e_CANDIDATE     = 2,
        e_LEADER        = 3
    };

    // CLASS METHODS
    static bsl::ostream& print(bsl::ostream&   stream,
                               RaftState::Enum value,
                               int             level          = 0,
                               int             spacesPerLevel = 4);

    static const char* toAscii(RaftState::Enum value);
};

// FREE OPERATORS
bsl::ostream& operator<<(bsl::ostream& stream, RaftState::Enum value);

// =====================
// struct RaftMessageType
// =====================

struct RaftMessageType {
    // TYPES
    enum Enum {
        e_REQUEST_VOTE          = 0,
        e_REQUEST_VOTE_RESP     = 1,
        e_APPEND_ENTRIES        = 2,
        e_APPEND_ENTRIES_RESP   = 3,
        e_INSTALL_SNAPSHOT      = 4,
        e_INSTALL_SNAPSHOT_RESP = 5,
        e_TIMEOUT_NOW           = 6
    };

    // CLASS METHODS
    static bsl::ostream& print(bsl::ostream&         stream,
                               RaftMessageType::Enum value,
                               int                   level          = 0,
                               int                   spacesPerLevel = 4);

    static const char* toAscii(RaftMessageType::Enum value);
};

// FREE OPERATORS
bsl::ostream& operator<<(bsl::ostream& stream, RaftMessageType::Enum value);

// ==============
// struct LogEntry
// ==============

/// VST representing a single entry in the Raft log.  'd_data' holds the
/// primary record blob (journal record for partitions, CSL record for
/// cluster).  'd_auxiliary' holds the optional supplementary payload
/// (data-file or qlist-file content for partition MESSAGE/QUEUE_OP
/// entries); null for all other record types.
struct LogEntry {
    // DATA
    bsls::Types::Uint64          d_term;
    bsls::Types::Uint64          d_index;
    bsl::shared_ptr<bdlbb::Blob> d_data;

    // CREATORS
    LogEntry();

    LogEntry(bsls::Types::Uint64                  term,
             bsls::Types::Uint64                  index,
             const bsl::shared_ptr<bdlbb::Blob>&  data);
};

// =============
// class RaftLog
// =============

/// Protocol for Raft log storage.  Implementations must provide indexed
/// access to log entries by position (1-based).  Position 0 is reserved for
/// the virtual entry before the log (term 0).
class RaftLog {
  public:
    // CREATORS
    virtual ~RaftLog();

    // MANIPULATORS

    /// Append a new log entry with the specified 'term' and record blob
    /// 'data'.  The optionally specified 'id' is an opaque token set by
    /// 'PartitionRaft' on the primary path to route to a pre-registered
    /// 'PendingWrite'; zero on the replica path and for CSL.
    virtual int append(bsls::Types::Uint64                  term,
                       const bsl::shared_ptr<bdlbb::Blob>&  data,
                       bsls::Types::Uint64                  id = 0) = 0;

    virtual int truncateFrom(bsls::Types::Uint64 index) = 0;

    // ACCESSORS
    virtual bsls::Types::Uint64 lastIndex() const = 0;

    virtual bsls::Types::Uint64 lastTerm() const = 0;

    virtual bsls::Types::Uint64 term(bsls::Types::Uint64 index) const = 0;

    virtual int entries(bsls::Types::Uint64    lo,
                        bsls::Types::Uint64    hi,
                        bsl::vector<LogEntry>* out) const = 0;

    virtual bsls::Types::Uint64 snapshotIndex() const = 0;

    virtual bsls::Types::Uint64 snapshotTerm() const = 0;

    /// Reset the log to the specified snapshot state.  Set
    /// `d_snapshotIndex` to `lastIncludedIndex` and `d_snapshotTerm` to
    /// `lastIncludedTerm`, clear all in-memory log entries, and invalidate
    /// any cached state.  Called after the application has applied a
    /// received snapshot.
    virtual void applySnapshot(bsls::Types::Uint64 lastIncludedIndex,
                                bsls::Types::Uint64 lastIncludedTerm) = 0;
};

// =================
// struct RaftMessage
// =================

/// VST representing a Raft RPC message.  All fields are present; unused
/// fields for a given message type are set to default values.
struct RaftMessage {
    // DATA
    RaftMessageType::Enum d_type;
    bsls::Types::Uint64   d_term;
    int                   d_sourceNodeId;
    int                   d_destinationNodeId;

    // RequestVote
    bsls::Types::Uint64 d_lastLogIndex;
    bsls::Types::Uint64 d_lastLogTerm;
    bool                d_preVote;

    // AppendEntries
    bsls::Types::Uint64   d_prevLogIndex;
    bsls::Types::Uint64   d_prevLogTerm;
    bsls::Types::Uint64   d_leaderCommit;
    bsl::vector<LogEntry> d_entries;

    // Response
    bool                d_success;
    bsls::Types::Uint64 d_matchIndex;

    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(RaftMessage, bslma::UsesBslmaAllocator)

    // CREATORS
    explicit RaftMessage(bslma::Allocator* allocator = 0);

    RaftMessage(const RaftMessage& other, bslma::Allocator* allocator = 0);
};

// ====================
// struct RaftNodeConfig
// ====================

/// VST for RaftNode configuration parameters.
struct RaftNodeConfig {
    // DATA
    int              d_selfId;
    bsl::vector<int> d_peerIds;
    int              d_electionTimeoutMin;
    int              d_electionTimeoutMax;
    int              d_heartbeatInterval;
    bool             d_preVote;

    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(RaftNodeConfig, bslma::UsesBslmaAllocator)

    // CREATORS
    explicit RaftNodeConfig(bslma::Allocator* allocator = 0);

    RaftNodeConfig(const RaftNodeConfig& other,
                   bslma::Allocator*     allocator = 0);
};

// ====================
// struct RaftNodeOutput
// ====================

/// VST for output produced by a single RaftNode operation.  The caller is
/// responsible for processing 'd_messages' (send to peers) and
/// 'd_committed' (apply to state machine).
struct RaftNodeOutput {
    // DATA
    bsl::vector<RaftMessage> d_messages;
    bsl::vector<LogEntry>    d_committed;
    bool                     d_stateChanged;
    bool                     d_leaderChanged;
    bool                     d_hasInstallSnapshot;
    RaftMessage              d_installSnapshot;

    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(RaftNodeOutput, bslma::UsesBslmaAllocator)

    // CREATORS
    explicit RaftNodeOutput(bslma::Allocator* allocator = 0);

    RaftNodeOutput(const RaftNodeOutput& other,
                   bslma::Allocator*     allocator = 0);

    // MANIPULATORS
    void reset();
};

// ==============
// class RaftNode
// ==============

/// Pure Raft consensus state machine.  Driven externally via 'tick()' and
/// 'step()'.  Produces output in 'RaftNodeOutput' for the caller to
/// process (send messages, apply committed entries).
class RaftNode {
  public:
    // PUBLIC CLASS DATA
    static const int                 k_INVALID_NODE_ID = -1;
    static const bsls::Types::Uint64 k_INVALID_TERM    = 0;

  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBRAFT.RAFTNODE");

    // DATA
    RaftNodeConfig d_config;
    RaftLog*       d_log_p;

    // Persistent state (must be saved to stable storage)
    bsls::Types::Uint64 d_currentTerm;
    int                 d_votedFor;

    // Volatile state
    RaftState::Enum     d_state;
    int                 d_leaderId;
    bsls::Types::Uint64 d_commitIndex;
    bsls::Types::Uint64 d_lastApplied;

    // Election state
    bsl::unordered_set<int> d_votesReceived;
    int                     d_electionTicks;
    int                     d_electionTimeout;

    // Leader state
    struct PeerState {
        bsls::Types::Uint64 d_nextIndex;
        bsls::Types::Uint64 d_matchIndex;
    };

    bsl::unordered_map<int, PeerState> d_peerStates;
    int                                d_heartbeatTicks;

    // Leadership transfer
    int d_transferTargetId;

    bslma::Allocator* d_allocator_p;

    // NOT IMPLEMENTED
    RaftNode(const RaftNode&);
    RaftNode& operator=(const RaftNode&);

    // PRIVATE MANIPULATORS
    void becomeFollower(bsls::Types::Uint64 term, int leaderId);

    void becomeCandidate(RaftNodeOutput* output, bool preVote);

    void becomeLeader(RaftNodeOutput* output);

    void handleRequestVote(RaftNodeOutput* output, const RaftMessage& msg);

    void handleRequestVoteResp(RaftNodeOutput* output, const RaftMessage& msg);

    void handleAppendEntries(RaftNodeOutput* output, const RaftMessage& msg);

    void handleAppendEntriesResp(RaftNodeOutput*    output,
                                 const RaftMessage& msg);

    void handleTimeoutNow(RaftNodeOutput* output, const RaftMessage& msg);

    /// Handle an InstallSnapshot request on a follower.
    void handleInstallSnapshot(RaftNodeOutput* output, const RaftMessage& msg);

    /// Handle an InstallSnapshot response on a leader.
    void handleInstallSnapshotResp(RaftNodeOutput*    output,
                                   const RaftMessage& msg);

    void sendAppendEntries(RaftNodeOutput* output, int peerId);

    void advanceCommitIndex(RaftNodeOutput* output);

    void resetElectionTimer();

    int quorum() const;

    bool isLogUpToDate(bsls::Types::Uint64 lastLogTerm,
                       bsls::Types::Uint64 lastLogIndex) const;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(RaftNode, bslma::UsesBslmaAllocator)

    // CREATORS
    RaftNode(const RaftNodeConfig& config,
             RaftLog*              log,
             bslma::Allocator*     allocator = 0);

    // MANIPULATORS

    /// Advance the logical clock by one tick.  Drives election timeouts
    /// (follower/candidate) and heartbeat emission (leader).
    void tick(RaftNodeOutput* output);

    /// Process the specified incoming 'message' from a peer.
    void step(RaftNodeOutput* output, const RaftMessage& message);

    /// Propose the specified 'data' as a new log entry.  The optionally
    /// specified 'id' is passed through to the log's 'append()' for primary
    /// path routing.  Return 0 on success, non-zero if not the leader.
    int propose(RaftNodeOutput*                      output,
                const bsl::shared_ptr<bdlbb::Blob>&  data,
                bsls::Types::Uint64                  id = 0);

    /// Initiate leadership transfer to the specified 'targetNodeId'.
    /// Return 0 on success, non-zero if this node is not the leader.
    int transferLeadership(RaftNodeOutput* output, int targetNodeId);

    // ACCESSORS
    RaftState::Enum       state() const;
    int                   leaderId() const;
    int                   selfId() const;
    bsls::Types::Uint64   currentTerm() const;
    bsls::Types::Uint64   commitIndex() const;
    bsls::Types::Uint64   lastApplied() const;
    const RaftNodeConfig& config() const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// --------------
// struct LogEntry
// --------------

// CREATORS
inline LogEntry::LogEntry()
: d_term(0)
, d_index(0)
, d_data()
{
}

inline LogEntry::LogEntry(bsls::Types::Uint64                  term,
                          bsls::Types::Uint64                  index,
                          const bsl::shared_ptr<bdlbb::Blob>&  data)
: d_term(term)
, d_index(index)
, d_data(data)
{
}

// -----------------
// struct RaftMessage
// -----------------

inline RaftMessage::RaftMessage(bslma::Allocator* allocator)
: d_type(RaftMessageType::e_REQUEST_VOTE)
, d_term(0)
, d_sourceNodeId(RaftNode::k_INVALID_NODE_ID)
, d_destinationNodeId(RaftNode::k_INVALID_NODE_ID)
, d_lastLogIndex(0)
, d_lastLogTerm(0)
, d_preVote(false)
, d_prevLogIndex(0)
, d_prevLogTerm(0)
, d_leaderCommit(0)
, d_entries(allocator)
, d_success(false)
, d_matchIndex(0)
{
}

inline RaftMessage::RaftMessage(const RaftMessage& other,
                                bslma::Allocator*  allocator)
: d_type(other.d_type)
, d_term(other.d_term)
, d_sourceNodeId(other.d_sourceNodeId)
, d_destinationNodeId(other.d_destinationNodeId)
, d_lastLogIndex(other.d_lastLogIndex)
, d_lastLogTerm(other.d_lastLogTerm)
, d_preVote(other.d_preVote)
, d_prevLogIndex(other.d_prevLogIndex)
, d_prevLogTerm(other.d_prevLogTerm)
, d_leaderCommit(other.d_leaderCommit)
, d_entries(other.d_entries, allocator)
, d_success(other.d_success)
, d_matchIndex(other.d_matchIndex)
{
}

// --------------------
// struct RaftNodeConfig
// --------------------

inline RaftNodeConfig::RaftNodeConfig(bslma::Allocator* allocator)
: d_selfId(RaftNode::k_INVALID_NODE_ID)
, d_peerIds(allocator)
, d_electionTimeoutMin(10)
, d_electionTimeoutMax(20)
, d_heartbeatInterval(3)
, d_preVote(true)
{
}

inline RaftNodeConfig::RaftNodeConfig(const RaftNodeConfig& other,
                                      bslma::Allocator*     allocator)
: d_selfId(other.d_selfId)
, d_peerIds(other.d_peerIds, allocator)
, d_electionTimeoutMin(other.d_electionTimeoutMin)
, d_electionTimeoutMax(other.d_electionTimeoutMax)
, d_heartbeatInterval(other.d_heartbeatInterval)
, d_preVote(other.d_preVote)
{
}

// --------------------
// struct RaftNodeOutput
// --------------------

inline RaftNodeOutput::RaftNodeOutput(bslma::Allocator* allocator)
: d_messages(allocator)
, d_committed(allocator)
, d_stateChanged(false)
, d_leaderChanged(false)
, d_hasInstallSnapshot(false)
, d_installSnapshot(allocator)
{
}

inline RaftNodeOutput::RaftNodeOutput(const RaftNodeOutput& other,
                                      bslma::Allocator*     allocator)
: d_messages(other.d_messages, allocator)
, d_committed(other.d_committed, allocator)
, d_stateChanged(other.d_stateChanged)
, d_leaderChanged(other.d_leaderChanged)
, d_hasInstallSnapshot(other.d_hasInstallSnapshot)
, d_installSnapshot(other.d_installSnapshot, allocator)
{
}

inline void RaftNodeOutput::reset()
{
    d_messages.clear();
    d_committed.clear();
    d_stateChanged        = false;
    d_leaderChanged       = false;
    d_hasInstallSnapshot  = false;
}

// --------------
// class RaftNode
// --------------

// ACCESSORS
inline RaftState::Enum RaftNode::state() const
{
    return d_state;
}

inline int RaftNode::leaderId() const
{
    return d_leaderId;
}

inline int RaftNode::selfId() const
{
    return d_config.d_selfId;
}

inline bsls::Types::Uint64 RaftNode::currentTerm() const
{
    return d_currentTerm;
}

inline bsls::Types::Uint64 RaftNode::commitIndex() const
{
    return d_commitIndex;
}

inline bsls::Types::Uint64 RaftNode::lastApplied() const
{
    return d_lastApplied;
}

inline const RaftNodeConfig& RaftNode::config() const
{
    return d_config;
}

inline int RaftNode::quorum() const
{
    return static_cast<int>(d_config.d_peerIds.size()) / 2 + 1;
}

inline bool RaftNode::isLogUpToDate(bsls::Types::Uint64 lastLogTerm,
                                    bsls::Types::Uint64 lastLogIndex) const
{
    bsls::Types::Uint64 myLastTerm  = d_log_p->lastTerm();
    bsls::Types::Uint64 myLastIndex = d_log_p->lastIndex();

    if (lastLogTerm != myLastTerm) {
        return lastLogTerm > myLastTerm;
    }
    return lastLogIndex >= myLastIndex;
}

}  // close package namespace
}  // close enterprise namespace

#endif
