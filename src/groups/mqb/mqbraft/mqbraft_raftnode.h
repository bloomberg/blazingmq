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
#include <bsl_string.h>
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

// =================
// struct ElectionMode
// =================

/// Per-node leadership-eligibility override.  Reproduces the legacy Elector's
/// per-node `quorum` knob (see `ClusterOrchestrator::processCommand`), which
/// tests use to pin or exclude a node as primary/leader:
/// * `e_NORMAL`: default Raft eligibility.
/// * `e_FORCE`:  campaign immediately (real election, skipping pre-vote) and
///   keep re-campaigning at the election-timeout cadence until leader.
/// * `e_NEVER`:  never become candidate; an incumbent leader keeps leading.
///   Still votes and replicates, so it counts toward another node's quorum.
struct ElectionMode {
    // TYPES
    enum Enum { e_NORMAL = 0, e_FORCE = 1, e_NEVER = 2 };

    // CLASS METHODS
    static const char* toAscii(ElectionMode::Enum value);
};

// FREE OPERATORS
bsl::ostream& operator<<(bsl::ostream& stream, ElectionMode::Enum value);

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

    LogEntry(bsls::Types::Uint64                 term,
             bsls::Types::Uint64                 index,
             const bsl::shared_ptr<bdlbb::Blob>& data);
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
    /// 'data'.  A null 'data' is the primary path: the log takes the entry
    /// from the 'PendingWrite' registered for it.
    virtual int append(bsls::Types::Uint64                 term,
                       const bsl::shared_ptr<bdlbb::Blob>& data) = 0;

    /// Drop the entry at the specified `index` and everything after it.
    /// Return 0 on success, non-zero if `index` is not in
    /// `(snapshotIndex(), lastIndex()]`.
    virtual int truncateFrom(bsls::Types::Uint64 index) = 0;

    // ACCESSORS

    /// Return the index of the last entry, or `snapshotIndex()` if the log
    /// holds none.
    virtual bsls::Types::Uint64 lastIndex() const = 0;

    /// Return the term of the last entry, or `snapshotTerm()` if the log holds
    /// none.
    virtual bsls::Types::Uint64 lastTerm() const = 0;

    /// Return the term of the entry at the specified `index`, `snapshotTerm()`
    /// if `index` is `snapshotIndex()`, and 0 if `index` is 0 or outside
    /// `[snapshotIndex(), lastIndex()]`.  A leader anchors an AppendEntries on
    /// this, so it must answer for the snapshot boundary as well as for the
    /// entries the log still holds.
    virtual bsls::Types::Uint64 term(bsls::Types::Uint64 index) const = 0;

    /// Append to the specified `out` the entries in `[lo, hi)`, stopping
    /// once they reach the specified `maxCount` or total the specified
    /// `maxBytes`, whichever comes first; 0 means unlimited.  At least one
    /// entry is always loaded, so a sender makes progress whatever the entry
    /// size.  The behavior is undefined unless `lo <= hi`,
    /// `lo > snapshotIndex()` and `hi <= lastIndex() + 1`.  Appending fewer
    /// than `hi - lo` entries means a cap was reached or an entry could not
    /// be read.  `out` is appended to, not cleared, so a caller can gather
    /// several ranges into one vector.  The specified `forApply` says the
    /// caller will hand each entry to the state machine rather than send it
    /// to a peer, which lets an implementation leave `d_data` null for the
    /// entries whose apply path does not read it; every entry is still
    /// appended, so the count is the same either way.
    virtual void entries(bsls::Types::Uint64    lo,
                         bsls::Types::Uint64    hi,
                         bsl::vector<LogEntry>* out,
                         bsls::Types::Uint64    maxCount,
                         bsls::Types::Uint64    maxBytes,
                         bool                   forApply) const = 0;

    /// Return the highest index the log no longer holds individually: entries
    /// at or below it have been compacted into a base snapshot.  0 if the log
    /// has never been compacted, in which case it holds everything from 1.
    virtual bsls::Types::Uint64 snapshotIndex() const = 0;

    /// Return the term of the entry at `snapshotIndex()`.
    virtual bsls::Types::Uint64 snapshotTerm() const = 0;
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

    /// Destinations past the first, which is `d_destinationNodeId`.  Only an
    /// `e_APPEND_ENTRIES` ever has any: a round collapses onto one message the
    /// peers at the same `prevLogIndex`, which are owed identical bytes, so
    /// the entries are read once and the event is built once.  Held apart from
    /// `d_destinationNodeId` so the one-destination messages -- every response
    /// and every vote -- still cost no allocation.  Use `destinationCount` and
    /// `destination`; a sender that reads only `d_destinationNodeId` silently
    /// drops peers.
    bsl::vector<int> d_otherDestinations;

    // AppendEntries
    bsls::Types::Uint64   d_prevLogIndex;
    bsls::Types::Uint64   d_prevLogTerm;
    bsls::Types::Uint64   d_leaderCommit;
    bsl::vector<LogEntry> d_entries;

    // Response
    bool                d_success;
    bsls::Types::Uint64 d_matchIndex;

    /// On a rejected AppendEntries response, the `d_prevLogIndex` of the
    /// request being rejected; 0 on success.  Lets the leader tell a response
    /// to its current request from one to a superseded request.
    bsls::Types::Uint64 d_rejectedIndex;

    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(RaftMessage, bslma::UsesBslmaAllocator)

    // CREATORS
    explicit RaftMessage(bslma::Allocator* allocator = 0);

    RaftMessage(const RaftMessage& other, bslma::Allocator* allocator = 0);

    // MANIPULATORS

    /// Also send this message to the peer with the specified `nodeId`.
    void addDestination(int nodeId);

    // ACCESSORS

    /// Return the number of peers this message goes to; at least one.
    size_t destinationCount() const;

    /// Return the destination at the specified `index`.  The behavior is
    /// undefined unless `index < destinationCount()`.
    int destination(size_t index) const;
};

// ===================
// struct RaftNodeInfo
// ===================

/// VST for one member of a Raft group: its node id, and the cluster name of
/// the node holding it.  The name is carried for log output: ids and names
/// share no numbering (`node0` is id 1), so a log printing ids would not line
/// up with the rest of the broker's output.
struct RaftNodeInfo {
    // DATA
    int         d_id;
    bsl::string d_name;

    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(RaftNodeInfo, bslma::UsesBslmaAllocator)

    // CREATORS
    explicit RaftNodeInfo(bslma::Allocator* allocator = 0);

    RaftNodeInfo(int                     id,
                 const bsl::string_view& name,
                 bslma::Allocator*       allocator = 0);

    RaftNodeInfo(const RaftNodeInfo& other, bslma::Allocator* allocator = 0);
};

// ====================
// struct RaftNodeConfig
// ====================

/// VST for RaftNode configuration parameters.
struct RaftNodeConfig {
    // PUBLIC CONSTANTS

    /// Value of `d_partitionId` identifying the cluster-state (CSL) Raft
    /// group, as opposed to a per-partition Raft group.
    static const int k_CSL_PARTITION_ID = -1;

    // DATA
    int d_selfId;

    /// Cluster name of this node, for log output.  See `RaftNodeInfo`.
    bsl::string d_nodeName;

    bsl::vector<RaftNodeInfo> d_peers;
    int                       d_electionTimeoutMin;
    int                       d_electionTimeoutMax;
    int                       d_heartbeatInterval;
    bool                      d_preVote;
    bool                      d_broadcastHeartbeatOnCommit;

    /// Entries this node will send one peer past what that peer has acked,
    /// before it stops sending to it.  See `k_MAX_UNACKED_ENTRIES`, its
    /// default.
    bsls::Types::Uint64 d_maxUnackedEntries;

    /// Ticks a leader waits for an `InstallSnapshotResp` before resending.
    /// Held apart from `d_electionTimeoutMin`, which is far too short: a peer
    /// must receive the whole file set, reopen it and re-index every entry
    /// before it can answer, and a retry costs another full transfer.  See
    /// `k_SNAPSHOT_TIMEOUT_TICKS`, its default.
    int d_snapshotTimeoutTicks;

    /// Identifier of the Raft group this node belongs to: a partition id for
    /// per-partition Raft, or `k_CSL_PARTITION_ID` for the cluster-state Raft.
    /// Used only to disambiguate log output.
    int d_partitionId;

    bslma::Allocator* d_allocator_p;

    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(RaftNodeConfig, bslma::UsesBslmaAllocator)

    // CREATORS
    explicit RaftNodeConfig(int partition, bslma::Allocator* allocator = 0);

    RaftNodeConfig(int               partition,
                   bool              broadcastHeartbeatOnCommit,
                   bslma::Allocator* allocator = 0);

    RaftNodeConfig(const RaftNodeConfig& other,
                   bslma::Allocator*     allocator = 0);

    // MANIPULATORS

    /// Set this node's id and cluster `name` to the specified `id` and
    /// `name`.  `addNode` still has to add it to the membership, which
    /// includes self.
    void setSelf(int id, const bsl::string_view& name);

    /// Add the node with the specified `id` and cluster `name` to the
    /// membership.
    void addNode(int id, const bsl::string_view& name);
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

    /// `true` when the apply gather stopped at its cap with entries still
    /// committed-but-unapplied.  The caller must come back for the rest --
    /// nothing else will, since `commitTo` only runs when the commit index
    /// advances.
    bool d_hasMoreToApply;

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

    /// Maximum number of entries sent to one peer past what it has acked.
    /// Sends are optimistic, so without a bound a peer that stops answering
    /// would be streamed the log as fast as it is produced -- the channel
    /// buffer is unbounded and neither blocks nor drops.  Derived from
    /// `nextIndex - matchIndex - 1` rather than counted, so it cannot drift
    /// out of step with the peer's real position.  Set to the ceiling the
    /// previous four-messages-in-flight window allowed, so it bounds a stalled
    /// peer without limiting a healthy one.
    static const bsls::Types::Uint64 k_MAX_UNACKED_ENTRIES = 4 * 4096;

    /// Default `RaftNodeConfig::d_snapshotTimeoutTicks`.  At the partition
    /// tick of 100ms this is 30 seconds, sized for a peer installing a full
    /// file set: a 1GB journal takes seconds to transfer and seconds more to
    /// re-index, and every premature retry re-sends the whole thing.
    static const int k_SNAPSHOT_TIMEOUT_TICKS = 300;

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

    /// Highest committed index handed to the caller to apply.  Trails
    /// `d_commitIndex` while a batch is outstanding.
    bsls::Types::Uint64 d_lastAppliedCommit;

    // Election state
    bsl::unordered_set<int> d_votesReceived;
    int                     d_electionTicks;
    int                     d_electionTimeout;

    // Leader state
    struct PeerState {
        /// Cluster name of this peer, for log output.  Set at construction
        /// from the membership and never reset: `nodeName` resolves ids
        /// through it whatever state this node is in.
        bsl::string d_name;

        bsls::Types::Uint64 d_nextIndex;
        bsls::Types::Uint64 d_matchIndex;

        /// True while an 'InstallSnapshot' is in flight to this peer with no
        /// response yet; 'sendAppendEntries' skips the peer while set.
        bool d_snapshotPending;

        /// Ticks since 'd_snapshotPending' was set.  Cleared on
        /// 'InstallSnapshotResp'; past 'd_electionTimeoutMin' the pending
        /// state is dropped and the peer retried.
        int d_snapshotPendingTicks;

        /// The snapshot index and term last sent to this peer.
        /// 'handleInstallSnapshotResp' advances 'd_matchIndex' and
        /// 'd_nextIndex' from these.
        bsls::Types::Uint64 d_snapshotPendingIndex;
        bsls::Types::Uint64 d_snapshotPendingTerm;

        /// True once this peer has rejected the optimistic boundary
        /// AppendEntries (see 'sendAppendEntries'), i.e. it lacks
        /// 'snapshotIndex' and needs an 'InstallSnapshot'.  Cleared once it
        /// advances past the boundary.
        bool d_boundaryProbeRejected;

        /// True while this peer's match index is unknown: right after
        /// becoming leader, and after a rejection.  A probing peer is sent one
        /// entry-carrying AppendEntries at a time, so a log that has forked is
        /// not streamed entries it will discard while its rejection is still
        /// in flight.
        bool d_probing;

        /// True once an entry-carrying AppendEntries has gone to this peer
        /// while `d_probing`, so the round skips it until the answer arrives.
        bool d_probeSent;

        /// Ticks since `d_matchIndex` last advanced while sends were
        /// outstanding.  Past `d_electionTimeoutMin` the peer is presumed to
        /// have dropped them and is reset to probing.
        int d_stalledTicks;

        /// Highest `leaderCommit` sent to this peer.  A commit advance needs
        /// an extra message only for peers this leaves behind; the rest learn
        /// it from a message they are already getting.
        bsls::Types::Uint64 d_sentCommit;

        /// True while this peer has a channel.  Not reset by
        /// `becomeLeader`/`becomeFollower`.
        bool d_isAvailable;

        // TRAITS
        BSLMF_NESTED_TRAIT_DECLARATION(PeerState, bslma::UsesBslmaAllocator)

        // CREATORS
        explicit PeerState(bslma::Allocator* allocator = 0);

        PeerState(const bsl::string_view& name,
                  bslma::Allocator*       allocator = 0);

        PeerState(const PeerState& other, bslma::Allocator* allocator = 0);

        // MANIPULATORS

        /// Set every replication field to its start-of-leadership value, with
        /// `d_nextIndex` the specified `nextIndex`.  Leaves `d_isAvailable`.
        void reset(bsls::Types::Uint64 nextIndex);
    };

    /// Entries appended since the last round, past which `propose` runs a
    /// round rather than waiting for the caller to flush.  A dispatcher batch
    /// is not bounded in time -- it runs until its queue drains -- and the
    /// tick is far too coarse (100ms) to bound it.  Counted rather than sized
    /// because the partition primary passes its payload through the pending
    /// write, not through `propose`.
    static const bsls::Types::Uint64 k_SEND_TRIGGER_ENTRIES = 64;

    /// Caps on the entries one AppendEntries carries.  A peer catching up on
    /// a long log is served over several messages rather than one: the count
    /// bounds how long applying a single message holds the partition's
    /// dispatcher, the byte total bounds what a message costs in memory when
    /// the entries carry payloads.
    /// Entries applied to the state machine in one pass.  A restarted node
    /// has everything since the last rollover committed-but-unapplied, so an
    /// uncapped gather would build a vector of millions of entries and hold
    /// the partition's dispatcher for the whole replay.
    static const bsls::Types::Uint64 k_MAX_APPLY_PER_BATCH = 4096;

    static const bsls::Types::Uint64 k_MAX_ENTRIES_PER_MESSAGE     = 4096;
    static const bsls::Types::Uint64 k_MAX_ENTRY_BYTES_PER_MESSAGE = 1024 *
                                                                     1024;

    bsl::unordered_map<int, PeerState> d_peerStates;
    int                                d_heartbeatTicks;

    /// Set by `tick` when the heartbeat interval elapses, consumed by
    /// `flushSends`, which then sends to every peer rather than only those
    /// owed entries or a commit advance.
    bool d_heartbeatDue;

    /// Entries appended since the last `flushSends`, against
    /// `k_SEND_TRIGGER_ENTRIES`.
    bsls::Types::Uint64 d_appendsSinceFlush;

    /// Scratch, a member so that the per-response path does not allocate.
    bsl::vector<bsls::Types::Uint64> d_matchIndices;

    // Leadership transfer
    int d_transferTargetId;

    // Leadership-eligibility override (see 'ElectionMode')
    ElectionMode::Enum d_electionMode;

    bslma::Allocator* d_allocator_p;

    // NOT IMPLEMENTED
    RaftNode(const RaftNode&);
    RaftNode& operator=(const RaftNode&);

    // PRIVATE MANIPULATORS
    void becomeFollower(bsls::Types::Uint64 term, int leaderId);

    void becomeCandidate(RaftNodeOutput* output, bool preVote);

    void becomeLeader(RaftNodeOutput* output);

    /// If the votes gathered so far meet `quorum()`, advance the election: on
    /// a pre-vote win start the real election (and re-evaluate), on a
    /// real-vote win become leader.  The specified `preVote` indicates which
    /// round just gathered a vote.  This is what lets a single-node cluster
    /// (no peers, so no vote responses ever arrive) elect itself from its own
    /// vote.
    void maybeCompleteElection(RaftNodeOutput* output, bool preVote);

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

    /// Append to the specified `output` what the peer with the specified
    /// `peerId` is owed, or nothing if it is owed nothing or is barred from
    /// receiving right now.  The specified `roundBegin` is where this round's
    /// messages start in `output->d_messages`: a peer whose anchor matches one
    /// of those is added to it as a further destination instead of drawing a
    /// second identical message.
    void sendAppendEntries(RaftNodeOutput*                     output,
                           int                                 peerId,
                           PeerState*                          peer,
                           bsl::vector<RaftMessage>::size_type roundBegin);

    /// Move the commit index to the specified `newCommit` and append the
    /// entries it newly commits to `output->d_committed`.  The behavior is
    /// undefined unless `newCommit` is above the current commit index and at
    /// or below `lastIndex()`.
    void commitTo(RaftNodeOutput* output, bsls::Types::Uint64 newCommit);

    /// Commit whatever a quorum of peers has acked, if that is more than is
    /// committed now.  Only meaningful on a leader with peers: a lone node
    /// commits at `propose` time instead.
    void advanceCommitIndex(RaftNodeOutput* output);

    void resetElectionTimer();

    // PRIVATE ACCESSORS
    bool isLogUpToDate(bsls::Types::Uint64 lastLogTerm,
                       bsls::Types::Uint64 lastLogIndex) const;

    /// Return the cluster name of the node with the specified `id`, for log
    /// output: node ids and node names share no numbering (`node0` is id 1),
    /// so a log printing ids would not line up with the rest of the broker's
    /// output.  `k_INVALID_NODE_ID` reads "none".  The behavior is undefined
    /// unless `id` is `k_INVALID_NODE_ID` or a member of this Raft group.
    const char* nodeName(int id) const;

    /// Return the cluster name of this node.  See `nodeName`.
    const char* selfName() const;

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

    /// Append to `output->d_committed` the next batch of committed entries
    /// that have not been applied, up to `k_MAX_APPLY_PER_BATCH`, and set
    /// `output->d_hasMoreToApply` if any remain after it.  A no-op when
    /// everything committed has been applied.
    void loadCommittedBatch(RaftNodeOutput* output);

    /// Append to the specified `output` the AppendEntries this leader owes its
    /// peers, and nothing if not the leader.  This is where every
    /// AppendEntries is produced: `propose`, `step` and `tick` only move peer
    /// state, so a caller decides when a round runs -- `PartitionRaft` at the
    /// dispatcher's flush, `ClusterStateRaft` on every event.  Peers at the
    /// same anchor share one message, so their entries are read from the log
    /// once and their event is built once.
    void flushSends(RaftNodeOutput* output);

    /// Propose the specified 'data' as a new log entry; a null 'data' is the
    /// primary path, see 'RaftLog::append'.  Return 0 on success, non-zero if
    /// not the leader.
    int propose(RaftNodeOutput*                     output,
                const bsl::shared_ptr<bdlbb::Blob>& data);

    /// Initiate leadership transfer to the specified 'targetNodeId'.
    /// Return 0 on success, non-zero if this node is not the leader.
    int transferLeadership(RaftNodeOutput* output, int targetNodeId);

    /// Set the leadership-eligibility override to the specified 'mode' (see
    /// 'ElectionMode').  'e_FORCE' triggers an immediate real election if not
    /// already leader; 'e_NEVER' takes effect on future elections only.  Any
    /// resulting messages/committed entries are emitted via 'output'.
    void setElectionMode(RaftNodeOutput* output, ElectionMode::Enum mode);

    /// Initialize recovered state at startup: raise 'd_currentTerm' to at
    /// least the specified 'term' (the recovered log's last term), and both
    /// 'd_commitIndex' and 'd_lastAppliedCommit' to at least the specified
    /// 'commitIndex'.  Per the Raft persistent-state contract (Figure 2),
    /// 'currentTerm' must never regress across a restart; without seeding it
    /// here, the constructor's 'd_currentTerm(0)' would let a restarted node
    /// re-propose a term already present in the recovered log.  Without
    /// seeding 'd_commitIndex', a node that ever rolled over/snapshot would
    /// stall because 'entries()' cannot serve indices at or below the
    /// snapshot floor.
    ///
    /// The applied watermark takes the same value: recovery restores storage
    /// to the compaction boundary and no further, so entries above it are
    /// applied by the commit path.
    void initRecoveredState(bsls::Types::Uint64 term,
                            bsls::Types::Uint64 commitIndex);

    /// Record that the peer with the specified `peerNodeId` is reachable or
    /// not, per the specified `isAvailable`.  Becoming reachable resets that
    /// peer back to probing.
    void setPeerAvailability(int peerNodeId, bool isAvailable);

    // ACCESSORS
    RaftState::Enum     state() const;
    int                 leaderId() const;
    int                 selfId() const;
    bsls::Types::Uint64 currentTerm() const;
    bsls::Types::Uint64 commitIndex() const;

    /// Return the term of the log entry at the current commit index (0 if
    /// nothing is committed).  Used to check whether a current-term entry
    /// has committed, which under Raft 5.4.2 implies all prior committed
    /// entries are also committed (and, once applied, present in state).
    bsls::Types::Uint64 commitTerm() const;

    bsls::Types::Uint64   lastAppliedCommit() const;
    const RaftNodeConfig& config() const;
    int                   quorum() const;
    ElectionMode::Enum    electionMode() const;

    /// Load into the specified `result` the highest log index known to be
    /// replicated on the peer having the specified `peerId`, and return
    /// true.  Return false, leaving `result` untouched, if `peerId` is not
    /// a peer of this Raft group: that covers self, and every node while
    /// this node is not leader, since peer state is built by
    /// `becomeLeader`.
    bool matchIndex(bsls::Types::Uint64* result, int peerId) const;
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

inline LogEntry::LogEntry(bsls::Types::Uint64                 term,
                          bsls::Types::Uint64                 index,
                          const bsl::shared_ptr<bdlbb::Blob>& data)
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
, d_otherDestinations(allocator)
, d_prevLogIndex(0)
, d_prevLogTerm(0)
, d_leaderCommit(0)
, d_entries(allocator)
, d_success(false)
, d_matchIndex(0)
, d_rejectedIndex(0)
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
, d_otherDestinations(other.d_otherDestinations, allocator)
, d_prevLogIndex(other.d_prevLogIndex)
, d_prevLogTerm(other.d_prevLogTerm)
, d_leaderCommit(other.d_leaderCommit)
, d_entries(other.d_entries, allocator)
, d_success(other.d_success)
, d_matchIndex(other.d_matchIndex)
, d_rejectedIndex(other.d_rejectedIndex)
{
}

inline void RaftMessage::addDestination(int nodeId)
{
    d_otherDestinations.push_back(nodeId);
}

inline size_t RaftMessage::destinationCount() const
{
    return 1 + d_otherDestinations.size();
}

inline int RaftMessage::destination(size_t index) const
{
    return index == 0 ? d_destinationNodeId : d_otherDestinations[index - 1];
}

// -------------------
// struct RaftNodeInfo
// -------------------

inline RaftNodeInfo::RaftNodeInfo(bslma::Allocator* allocator)
: d_id(RaftNode::k_INVALID_NODE_ID)
, d_name(allocator)
{
}

inline RaftNodeInfo::RaftNodeInfo(int                     id,
                                  const bsl::string_view& name,
                                  bslma::Allocator*       allocator)
: d_id(id)
, d_name(name, allocator)
{
}

inline RaftNodeInfo::RaftNodeInfo(const RaftNodeInfo& other,
                                  bslma::Allocator*   allocator)
: d_id(other.d_id)
, d_name(other.d_name, allocator)
{
}

// --------------------
// struct RaftNodeConfig
// --------------------

inline RaftNodeConfig::RaftNodeConfig(int               partition,
                                      bslma::Allocator* allocator)
: d_selfId(RaftNode::k_INVALID_NODE_ID)
, d_nodeName(allocator)
, d_peers(allocator)
, d_electionTimeoutMin(10)
, d_electionTimeoutMax(20)
, d_heartbeatInterval(3)
, d_preVote(true)
, d_broadcastHeartbeatOnCommit(false)
, d_maxUnackedEntries(RaftNode::k_MAX_UNACKED_ENTRIES)
, d_snapshotTimeoutTicks(RaftNode::k_SNAPSHOT_TIMEOUT_TICKS)
, d_partitionId(partition)
, d_allocator_p(allocator)
{
}

inline RaftNodeConfig::RaftNodeConfig(int  partition,
                                      bool broadcastHeartbeatOnCommit,
                                      bslma::Allocator* allocator)
: d_selfId(RaftNode::k_INVALID_NODE_ID)
, d_nodeName(allocator)
, d_peers(allocator)
, d_electionTimeoutMin(10)
, d_electionTimeoutMax(20)
, d_heartbeatInterval(3)
, d_preVote(true)
, d_broadcastHeartbeatOnCommit(broadcastHeartbeatOnCommit)
, d_maxUnackedEntries(RaftNode::k_MAX_UNACKED_ENTRIES)
, d_snapshotTimeoutTicks(RaftNode::k_SNAPSHOT_TIMEOUT_TICKS)
, d_partitionId(partition)
, d_allocator_p(allocator)
{
}

inline RaftNodeConfig::RaftNodeConfig(const RaftNodeConfig& other,
                                      bslma::Allocator*     allocator)
: d_selfId(other.d_selfId)
, d_nodeName(other.d_nodeName, allocator)
, d_peers(other.d_peers, allocator)
, d_electionTimeoutMin(other.d_electionTimeoutMin)
, d_electionTimeoutMax(other.d_electionTimeoutMax)
, d_heartbeatInterval(other.d_heartbeatInterval)
, d_preVote(other.d_preVote)
, d_broadcastHeartbeatOnCommit(other.d_broadcastHeartbeatOnCommit)
, d_maxUnackedEntries(other.d_maxUnackedEntries)
, d_snapshotTimeoutTicks(other.d_snapshotTimeoutTicks)
, d_partitionId(other.d_partitionId)
, d_allocator_p(allocator)
{
}

inline void RaftNodeConfig::setSelf(int id, const bsl::string_view& name)
{
    d_selfId   = id;
    d_nodeName = name;
}

inline void RaftNodeConfig::addNode(int id, const bsl::string_view& name)
{
    d_peers.push_back(RaftNodeInfo(id, name, d_allocator_p));
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
, d_hasMoreToApply(false)
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
, d_hasMoreToApply(other.d_hasMoreToApply)
, d_installSnapshot(other.d_installSnapshot, allocator)
{
}

inline void RaftNodeOutput::reset()
{
    d_messages.clear();
    d_committed.clear();
    d_stateChanged       = false;
    d_leaderChanged      = false;
    d_hasInstallSnapshot = false;
    d_hasMoreToApply     = false;
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

inline bsls::Types::Uint64 RaftNode::commitTerm() const
{
    return d_log_p->term(d_commitIndex);
}

inline bsls::Types::Uint64 RaftNode::lastAppliedCommit() const
{
    return d_lastAppliedCommit;
}

inline const RaftNodeConfig& RaftNode::config() const
{
    return d_config;
}

inline int RaftNode::quorum() const
{
    return static_cast<int>(d_config.d_peers.size()) / 2 + 1;
}

inline ElectionMode::Enum RaftNode::electionMode() const
{
    return d_electionMode;
}

inline bool RaftNode::matchIndex(bsls::Types::Uint64* result, int peerId) const
{
    bsl::unordered_map<int, PeerState>::const_iterator it = d_peerStates.find(
        peerId);
    if (it == d_peerStates.end()) {
        return false;  // RETURN
    }

    *result = it->second.d_matchIndex;
    return true;
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
