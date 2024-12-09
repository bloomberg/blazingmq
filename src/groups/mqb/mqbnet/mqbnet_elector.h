// Copyright 2015-2023 Bloomberg Finance L.P.
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

// mqbnet_elector.h                                                   -*-C++-*-
#ifndef INCLUDED_MQBNET_ELECTOR
#define INCLUDED_MQBNET_ELECTOR

//@PURPOSE: Provide a mechanism to elect leader from among a cluster of nodes.
//
//@CLASSES:
//  mqbnet::ElectorIOEventType:      Enum representing different I/O events
//  mqbnet::ElectorTimerEventType:   Enum representing different timer events
//  mqbnet::ElectorState:            Enum representing elector state
//  mqbnet::ElectorTransitionReason: Enum representing state transition reason
//  mqbnet::ElectorStateMachine:     VST representing the elector state machine
//  mqbnet::ElectorStateMachineOutput: VST representing state machine's output
//  mqbnet::Elector:                 Mechanism to elect leader within a cluster
//
//@DESCRIPTION: 'mqbnet::Elector' provides a mechanism to elect a leader from
// among a cluster of known number of nodes through distributed consensus.
// This implementation is based on the *Raft* algorithm
// ('https://ramcloud.stanford.edu/raft.pdf').  The number of participating
// nodes in the cluster and minimum quorum count required to become a leader
// need to be provided upfront.  An appropriate quorum value guarantees at most
// one leader, even in case of network splits.  A typical value for quorum is
// half the total number of participating nodes plus one.  A comprehensive
// overview of the algorithm can be found at 'doc/proposal/election.md'.
// 'mqbnet::ElectorState', and 'mqbnet::ElectorTransitionReason' are public
// classes.  'mqbnet::ElectorIOEventType', 'mqbnet::ElectorTimerEventType' and
// 'mqbnet::ElectorStateMachine' are private classes and should not be used
// outside of this component.
//
/// Threading
///---------
//
//
/// NOTE
///----
// The pseudo-random number generator should be seeded prior to usage of this
// component, by the following code (or similar):
//..
//  #include <bsl_cstdlib.h> // for bsl::srand()
//
//  // Initialize pseudo-random number generator with appropriate value.  Note
//  // that 'seedValue' used below should be different for different nodes
//  // using this component.  If same value is provided, it may cause
//  // signifcant delay for a leader to be elected.
//
//  bsl::srand(seedValue);
//..

// MQB

#include <mqbcfg_messages.h>
#include <mqbi_cluster.h>
#include <mqbi_dispatcher.h>
#include <mqbnet_cluster.h>
#include <mqbnet_session.h>

// BMQ
#include <bmqp_event.h>

// BDE
#include <ball_log.h>
#include <bdlbb_blob.h>
#include <bdlmt_eventscheduler.h>
#include <bsl_functional.h>
#include <bsl_map.h>
#include <bsl_ostream.h>
#include <bsl_string.h>
#include <bsl_vector.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bslmt_lockguard.h>
#include <bslmt_mutex.h>
#include <bslmt_threadutil.h>
#include <bsls_assert.h>
#include <bsls_types.h>

namespace BloombergLP {

// FORWARD DECLARATIONS
namespace mqbcmd {
class ElectorCommand;
}
namespace mqbcmd {
class ElectorResult;
}

namespace mqbnet {

// FORWARD DECLARATIONS
class Elector;

// =========================
// struct ElectorIOEventType
// =========================

/// This enum represents various I/O event types that are applied to and
/// emitted from mqbnet::ElectorStateMachine.
struct ElectorIOEventType {
    // TYPES
    enum Enum {
        e_NONE               = 0,
        e_ELECTION_PROPOSAL  = 1,
        e_ELECTION_RESPONSE  = 2,
        e_LEADER_HEARTBEAT   = 3,
        e_NODE_UNAVAILABLE   = 4,
        e_NODE_AVAILABLE     = 5,
        e_HEARTBEAT_RESPONSE = 6,
        e_SCOUTING_REQUEST   = 7,
        e_SCOUTING_RESPONSE  = 8,
        e_LEADERSHIP_CESSION = 9
    };

    // CLASS METHODS

    /// Write the string representation of the specified enumeration `value`
    /// to the specified output `stream`, and return a reference to
    /// `stream`.  Optionally specify an initial indentation `level`, whose
    /// absolute value is incremented recursively for nested objects.  If
    /// `level` is specified, optionally specify `spacesPerLevel`, whose
    /// absolute value indicates the number of spaces per indentation level
    /// for this and all of its nested objects.  If `level` is negative,
    /// suppress indentation of the first line.  If `spacesPerLevel` is
    /// negative, format the entire output on one line, suppressing all but
    /// the initial indentation (as governed by `level`).  See `toAscii` for
    /// what constitutes the string representation of a
    /// `ElectorIOEventType::Enum` value.
    static bsl::ostream& print(bsl::ostream&            stream,
                               ElectorIOEventType::Enum value,
                               int                      level          = 0,
                               int                      spacesPerLevel = 4);

    /// Return the non-modifiable string representation corresponding to the
    /// specified enumeration `value`, if it exists, and a unique (error)
    /// string otherwise.  The string representation of `value` matches its
    /// corresponding enumerator name with the `e_` prefix elided.  Note
    /// that specifying a `value` that does not match any of the enumerators
    /// will result in a string representation that is distinct from any of
    /// those corresponding to the enumerators, but is otherwise
    /// unspecified.
    static const char* toAscii(ElectorIOEventType::Enum value);

    /// Return true and fills the specified `out` with the enum value
    /// corresponding to the specified `str`, if valid, or return false and
    /// leave `out` untouched if `str` doesn't correspond to any value of
    /// the enum.
    static bool fromAscii(ElectorIOEventType::Enum* out,
                          const bslstl::StringRef&  str);
};

// FREE OPERATORS

/// Format the specified `value` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream& stream, ElectorIOEventType::Enum value);

// ============================
// struct ElectorTimerEventType
// ============================

/// This enum represents various timer event types that are applied to and
/// emitted from mqbnet::ElectorStateMachine
struct ElectorTimerEventType {
    // TYPES
    enum Enum {
        e_NONE                      = 0,
        e_INITIAL_WAIT_TIMER        = 1,
        e_RANDOM_WAIT_TIMER         = 2,
        e_ELECTION_RESULT_TIMER     = 3,
        e_HEARTBEAT_CHECK_TIMER     = 4,
        e_HEARTBEAT_BROADCAST_TIMER = 5,
        e_SCOUTING_RESULT_TIMER     = 6
    };

    // CLASS METHODS

    /// Write the string representation of the specified enumeration `value`
    /// to the specified output `stream`, and return a reference to
    /// `stream`.  Optionally specify an initial indentation `level`, whose
    /// absolute value is incremented recursively for nested objects.  If
    /// `level` is specified, optionally specify `spacesPerLevel`, whose
    /// absolute value indicates the number of spaces per indentation level
    /// for this and all of its nested objects.  If `level` is negative,
    /// suppress indentation of the first line.  If `spacesPerLevel` is
    /// negative, format the entire output on one line, suppressing all but
    /// the initial indentation (as governed by `level`).  See `toAscii` for
    /// what constitutes the string representation of a
    /// `ElectorTimerEventType::Enum` value.
    static bsl::ostream& print(bsl::ostream&               stream,
                               ElectorTimerEventType::Enum value,
                               int                         level          = 0,
                               int                         spacesPerLevel = 4);

    /// Return the non-modifiable string representation corresponding to the
    /// specified enumeration `value`, if it exists, and a unique (error)
    /// string otherwise.  The string representation of `value` matches its
    /// corresponding enumerator name with the `e_` prefix elided.  Note
    /// that specifying a `value` that does not match any of the enumerators
    /// will result in a string representation that is distinct from any of
    /// those corresponding to the enumerators, but is otherwise
    /// unspecified.
    static const char* toAscii(ElectorTimerEventType::Enum value);

    /// Return true and fills the specified `out` with the enum value
    /// corresponding to the specified `str`, if valid, or return false and
    /// leave `out` untouched if `str` doesn't correspond to any value of
    /// the enum.
    static bool fromAscii(ElectorTimerEventType::Enum* out,
                          const bslstl::StringRef&     str);
};

// FREE OPERATORS

/// Format the specified `value` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream&               stream,
                         ElectorTimerEventType::Enum value);

// ===================
// struct ElectorState
// ===================

/// This enum represents various mutually exclusive states of a node
/// participating in an election.
struct ElectorState {
    // TYPES
    enum Enum { e_DORMANT = 0, e_FOLLOWER = 1, e_CANDIDATE = 2, e_LEADER = 3 };

    // CLASS METHODS

    /// Write the string representation of the specified enumeration `value`
    /// to the specified output `stream`, and return a reference to
    /// `stream`.  Optionally specify an initial indentation `level`, whose
    /// absolute value is incremented recursively for nested objects.  If
    /// `level` is specified, optionally specify `spacesPerLevel`, whose
    /// absolute value indicates the number of spaces per indentation level
    /// for this and all of its nested objects.  If `level` is negative,
    /// suppress indentation of the first line.  If `spacesPerLevel` is
    /// negative, format the entire output on one line, suppressing all but
    /// the initial indentation (as governed by `level`).  See `toAscii` for
    /// what constitutes the string representation of a `ElectorState::Enum`
    /// value.
    static bsl::ostream& print(bsl::ostream&      stream,
                               ElectorState::Enum value,
                               int                level          = 0,
                               int                spacesPerLevel = 4);

    /// Return the non-modifiable string representation corresponding to the
    /// specified enumeration `value`, if it exists, and a unique (error)
    /// string otherwise.  The string representation of `value` matches its
    /// corresponding enumerator name with the `e_` prefix elided.  Note
    /// that specifying a `value` that does not match any of the enumerators
    /// will result in a string representation that is distinct from any of
    /// those corresponding to the enumerators, but is otherwise
    /// unspecified.
    static const char* toAscii(ElectorState::Enum value);

    /// Return true and fills the specified `out` with the enum value
    /// corresponding to the specified `str`, if valid, or return false and
    /// leave `out` untouched if `str` doesn't correspond to any value of
    /// the enum.
    static bool fromAscii(ElectorState::Enum*      out,
                          const bslstl::StringRef& str);
};

// FREE OPERATORS

/// Format the specified `value` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream& stream, ElectorState::Enum value);

// ==============================
// struct ElectorTransitionReason
// ==============================

/// This enum represents various codes associated with an election state
/// change event.
struct ElectorTransitionReason {
    // TYPES
    enum Enum {
        // Valid with all 4 ElectorStates
        e_NONE = 0

        // Valid only with ElectorState::e_FOLLOWER
        ,
        e_STARTED             = 1,
        e_LEADER_NO_HEARTBEAT = 2,
        e_LEADER_UNAVAILABLE  = 3,
        e_LEADER_PREEMPTED    = 4,
        e_ELECTION_PREEMPTED  = 5,
        e_QUORUM_NOT_ACHIEVED = 6
    };

    // CLASS METHODS

    /// Write the string representation of the specified enumeration `value`
    /// to the specified output `stream`, and return a reference to
    /// `stream`.  Optionally specify an initial indentation `level`, whose
    /// absolute value is incremented recursively for nested objects.  If
    /// `level` is specified, optionally specify `spacesPerLevel`, whose
    /// absolute value indicates the number of spaces per indentation level
    /// for this and all of its nested objects.  If `level` is negative,
    /// suppress indentation of the first line.  If `spacesPerLevel` is
    /// negative, format the entire output on one line, suppressing all but
    /// the initial indentation (as governed by `level`).  See `toAscii` for
    /// what constitutes the string representation of a
    /// `ElectorTransitionReason::Enum` value.
    static bsl::ostream& print(bsl::ostream&                 stream,
                               ElectorTransitionReason::Enum value,
                               int                           level = 0,
                               int spacesPerLevel                  = 4);

    /// Return the non-modifiable string representation corresponding to the
    /// specified enumeration `value`, if it exists, and a unique (error)
    /// string otherwise.  The string representation of `value` matches its
    /// corresponding enumerator name with the `e_` prefix elided.  Note
    /// that specifying a `value` that does not match any of the enumerators
    /// will result in a string representation that is distinct from any of
    /// those corresponding to the enumerators, but is otherwise
    /// unspecified.
    static const char* toAscii(ElectorTransitionReason::Enum value);

    /// Return true and fills the specified `out` with the enum value
    /// corresponding to the specified `str`, if valid, or return false and
    /// leave `out` untouched if `str` doesn't correspond to any value of
    /// the enum.
    static bool fromAscii(ElectorTransitionReason::Enum* out,
                          const bslstl::StringRef&       str);
};

// FREE OPERATORS

/// Format the specified `value` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream&                 stream,
                         ElectorTransitionReason::Enum value);

// ===============================
// class ElectorStateMachineOutput
// ===============================

/// PRIVATE CLASS. This components provides a VST representing the output
/// of state machine of `mqbnet::Elector`.  This component is not
/// thread-safe.
class ElectorStateMachineOutput {
  private:
    // DATA
    bool                d_stateChangedFlag;
    bsls::Types::Uint64 d_term;  // Outgoing term can be different
                                 // from state machine's current term.
    ElectorIOEventType::Enum    d_io;
    ElectorTimerEventType::Enum d_timer;
    int                         d_destinationNodeId;
    bool                        d_cancelTimerEventsFlag;
    bool                        d_scoutingResponseFlag;

  public:
    // CREATORS

    /// Create an instance with invalid/undefined values for all fields.
    ElectorStateMachineOutput();

    /// Create an instance with the specified `term, `io', `timer` and
    /// `destinationNodeId` values.
    ElectorStateMachineOutput(bsls::Types::Uint64         term,
                              ElectorIOEventType::Enum    io,
                              ElectorTimerEventType::Enum timer,
                              int                         destinationNodeId);

    // MANIPULATORS
    ElectorStateMachineOutput& setStateChangedFlag(bool value);
    ElectorStateMachineOutput& setTerm(bsls::Types::Uint64 value);
    ElectorStateMachineOutput& setIo(ElectorIOEventType::Enum value);
    ElectorStateMachineOutput& setTimer(ElectorTimerEventType::Enum value);
    ElectorStateMachineOutput& setDestination(int value);
    ElectorStateMachineOutput& setCancelTimerEventsFlag(bool value);

    /// Set the corresponding field to the specified `value`.
    ElectorStateMachineOutput& setScoutingResponseFlag(bool value);

    /// Reset all fields of the instance to invalid/undefined values.
    void reset();

    // ACCESSORS
    bool stateChangedFlag() const;

    bsls::Types::Uint64 term() const;

    ElectorIOEventType::Enum io() const;

    ElectorTimerEventType::Enum timer() const;

    int destination() const;

    bool cancelTimerEventsFlag() const;

    bool scoutingResponseFlag() const;
};

// =====================================
// class ElectorStateMachineScoutingInfo
// =====================================

/// PRIVATE CLASS. This components captures the scouting information for a
/// node which proposes a pre-election scouting request.
class ElectorStateMachineScoutingInfo {
  private:
    typedef bsl::map<int, bool> NodeIdResponseMap;

    bsls::Types::Uint64 d_term;

    NodeIdResponseMap d_responses;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(ElectorStateMachineScoutingInfo,
                                   bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create an instance using the specified `allocator` for memory
    /// allocation.
    ElectorStateMachineScoutingInfo(bslma::Allocator* allocator);

    /// Create an instance initialized with the specified `other` instance
    /// and using the specified `allocator` for memory allocation.
    ElectorStateMachineScoutingInfo(
        const ElectorStateMachineScoutingInfo& other,
        bslma::Allocator*                      allocator);

    // MANIPULATORS
    void setTerm(bsls::Types::Uint64 term);

    void addNodeResponse(int nodeId, bool willVote);

    void removeNodeResponse(int nodeId);

    void reset();

    // ACCESSORS
    bsls::Types::Uint64 term() const;

    size_t numResponses() const;

    size_t numSupportingNodes() const;
};

// =========================
// class ElectorStateMachine
// =========================

/// PRIVATE CLASS. This components provides a VST representing the state
/// machine of `mqbnet::Elector`.  This component is not thread-safe.
class ElectorStateMachine {
  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBNET.ELECTORSTATEMACHINE");

    typedef ElectorStateMachineScoutingInfo ScoutingInfo;

  public:
    // PUBLIC CLASS DATA
    static const bsls::Types::Uint64 k_INVALID_TERM = 0;
    static const int k_INVALID_NODE_ID = Cluster::k_INVALID_NODE_ID;
    static const int k_ALL_NODES_ID;

  private:
    // DATA
    ElectorState::Enum d_state;

    ElectorTransitionReason::Enum d_reason;

    bsls::Types::Uint64 d_term;

    int d_quorum;

    int d_numTotalPeers;

    int d_selfId;

    int d_leaderNodeId;
    // k_INVALID_NODE_ID => no leader

    int d_tentativeLeaderNodeId;
    // k_INVALID_NODE_ID => no tentative
    // leader

    bsl::vector<int> d_supporters;
    // NodeIds which voted for this elector
    // instance.  Empty unless this
    // instance is a candidate or a leader

    ScoutingInfo d_scoutingInfo;
    // Scouting info maintained by a node
    // when it sends pre-election scouting
    // request to peer nodes.

    bsls::Types::Int64 d_lastLeaderHeartbeatTime;
    // Time stamp in nano seconds when last
    // heartbeat from the current leader
    // was seen.  Non zero value only if
    // currently, this node is the leader,
    // and has received at least one
    // heartbeat from the leader.

    bsls::Types::Int64 d_leaderInactivityInterval;
    // Maximum interval in nano seconds
    // after which an inactive leader is
    // assumed to be not a leader.

    bsls::Types::Uint64 d_age;
    // Age of the state machine.  Age is
    // bumped up everytime a state
    // transition occurs.

  private:
    // PRIVATE MANIPULATORS
    void applyLeadershipCessionEventToFollower(ElectorStateMachineOutput* out,
                                               bsls::Types::Uint64        term,
                                               int sourceNodeId);
    void applyLeadershipCessionEventToCandidate(ElectorStateMachineOutput* out,
                                                bsls::Types::Uint64 term,
                                                int sourceNodeId);

    /// Apply `e_LEADERSHIP_CESSION` event from the specified `sourceNodeId`
    /// with the specified `term` to this elector instance in the
    /// corresponding state, and update the specified `out` buffer with the
    /// state machine's output.  The behavior is undefined unless state
    /// machine is the corresponding state.  The behavior is also undefined
    /// unless `out` buffer is non-null.
    void applyLeadershipCessionEventToLeader(ElectorStateMachineOutput* out,
                                             bsls::Types::Uint64        term,
                                             int sourceNodeId);

    void applyLeaderHeartbeatEventToFollower(ElectorStateMachineOutput* out,
                                             bsls::Types::Uint64        term,
                                             int sourceNodeId);
    void applyLeaderHeartbeatEventToCandidate(ElectorStateMachineOutput* out,
                                              bsls::Types::Uint64        term,
                                              int sourceNodeId);

    /// Apply `e_LEADER_HEARTBEAT` event from the specified `sourceNodeId`
    /// with the specified `term` to this elector instance in the
    /// corresponding state, and update the specified `out` buffer with the
    /// state machine's output.  The behavior is undefined unless state
    /// machine is the corresponding state.  The behavior is also undefined
    /// unless `out` buffer is non-null.
    void applyLeaderHeartbeatEventToLeader(ElectorStateMachineOutput* out,
                                           bsls::Types::Uint64        term,
                                           int sourceNodeId);

    void applyElectionProposalEventToFollower(ElectorStateMachineOutput* out,
                                              bsls::Types::Uint64        term,
                                              int sourceNodeId);
    void applyElectionProposalEventToCandidate(ElectorStateMachineOutput* out,
                                               bsls::Types::Uint64        term,
                                               int sourceNodeId);

    /// Apply `e_ELECTION_PROPOSAL` event from the specified `sourceNodeId`
    /// with the specified `term` to this elector instance in the
    /// corresponding state, and update the specified `out` buffer with the
    /// output of state machine.  The behavior is undefined unless state
    /// machine is the corresponding state.  The behavior is also undefined
    /// unless `out` buffer is non-null.
    void applyElectionProposalEventToLeader(ElectorStateMachineOutput* out,
                                            bsls::Types::Uint64        term,
                                            int sourceNodeId);

    void applyElectionResponseEventToFollower(ElectorStateMachineOutput* out,
                                              bsls::Types::Uint64        term,
                                              int sourceNodeId);
    void applyElectionResponseEventToCandidate(ElectorStateMachineOutput* out,
                                               bsls::Types::Uint64        term,
                                               int sourceNodeId);

    /// Apply `e_ELECTION_RESPONSE` event from the specified `sourceNodeId`
    /// with the specified `term` to this elector instance in the
    /// corresponding state, and update the specified `out` buffer with the
    /// output of state machine.  The behavior is undefined unless state
    /// machine is the corresponding state.  The behavior is also undefined
    /// unless `out` buffer is non-null.
    void applyElectionResponseEventToLeader(ElectorStateMachineOutput* out,
                                            bsls::Types::Uint64        term,
                                            int sourceNodeId);

    /// Apply `e_HEARTBEAT_RESPONSE` event from the specified `sourceNodeId`
    /// with the specified `term` to this elector instance in the leader
    /// state, and update the specified `out` buffer with the output of
    /// state machine.  The behavior is undefined unless state machine is
    /// the leader state.  The behavior is also undefined unless `out`
    /// buffer is non-null.
    void applyHeartbeatResponseEventToLeader(ElectorStateMachineOutput* out,
                                             bsls::Types::Uint64        term,
                                             int sourceNodeId);

    void applyNodeStatusEventToFollower(ElectorStateMachineOutput* out,
                                        ElectorIOEventType::Enum   event,
                                        int sourceNodeId);
    void applyNodeStatusEventToCandidate(ElectorStateMachineOutput* out,
                                         ElectorIOEventType::Enum   event,
                                         int sourceNodeId);

    /// Apply the specified elector IO `event` from the specified
    /// `sourceNodeId` to this elector instance in the corresponding
    /// follower/candidate/leader state, and update the specified `out`
    /// buffer with the output of state machine.  The behavior is undefined
    /// unless state machine is the corresponding state.  The behavior is
    /// also undefined unless `out` buffer is non-null and `event` is either
    /// `e_NODE_AVAILABLE` or `e_NODE_UNAVAILABLE`.
    void applyNodeStatusEventToLeader(ElectorStateMachineOutput* out,
                                      ElectorIOEventType::Enum   event,
                                      int                        sourceNodeId);

    void applyLeaderHeartbeatEvent(ElectorStateMachineOutput* out,
                                   bsls::Types::Uint64        term,
                                   int                        sourceNodeId);
    void applyLeadershipCessionEvent(ElectorStateMachineOutput* out,
                                     bsls::Types::Uint64        term,
                                     int                        sourceNodeId);
    void applyElectionProposalEvent(ElectorStateMachineOutput* out,
                                    bsls::Types::Uint64        term,
                                    int                        sourceNodeId);
    void applyElectionResponseEvent(ElectorStateMachineOutput* out,
                                    bsls::Types::Uint64        term,
                                    int                        sourceNodeId);
    void applyHeartbeatResponseEvent(ElectorStateMachineOutput* out,
                                     bsls::Types::Uint64        term,
                                     int                        sourceNodeId);
    void applyScoutingRequestEvent(ElectorStateMachineOutput* out,
                                   bsls::Types::Uint64        term,
                                   int                        sourceNodeId);

    /// Apply the specified I/O `event` from the specified `sourceNodeId`
    /// optionally having the specified `term` on the elector state machine,
    /// and update the specified `out` buffer with the output of state
    /// machine.  The behavior is undefined unless `out` buffer is non null.
    void applyScoutingResponseEvent(ElectorStateMachineOutput* out,
                                    bool                       willVote,
                                    bsls::Types::Uint64        term,
                                    int                        sourceNodeId);

    void applyElectionResultTimerEvent(ElectorStateMachineOutput* out);
    void applyHeartbeatCheckTimerEvent(ElectorStateMachineOutput* out);
    void applyRandomWaitTimerEvent(ElectorStateMachineOutput* out);
    void applyHeartbeatBroadcastTimerEvent(ElectorStateMachineOutput* out);

    /// Apply the corresponding timer event on the elector state machine,
    /// and update the specified `out` buffer with the output of state
    /// machine.  The behavior is undefined unless `out` buffer is
    /// non-null.
    void applyScoutingResultTimerEvent(ElectorStateMachineOutput* out);

    // PRIVATE ACCESSORS

    /// Return true if the specified `sourceNodeId` is a valid ID for a
    /// source node and false otherwise.
    bool isValidSourceNode(int sourceNodeId) const;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(ElectorStateMachine,
                                   bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create an instance in disabled state where `isEnabled` will return
    /// false, using the specified `allocator` for memory allocation.  Note
    /// that the state is `DORMANT` and term is zero.
    explicit ElectorStateMachine(bslma::Allocator* allocator);

    /// Create an instance initialized with the specified `other` instance
    /// and using the specified `allocator` for memory allocation.
    ElectorStateMachine(const ElectorStateMachine& other,
                        bslma::Allocator*          allocator);

    // MANIPULATORS

    /// Enable this state machine having the specified `selfId`, requiring
    /// the specified `quorum` in a cluster of `numTotalPeers` for winning
    /// an election and waiting for the specified
    /// `leaderInactivityIntervalMs` before marking the current leader as
    /// inactive.  This method has no effect if state machine is already
    /// enabled.  Behavior is undefined unless `selfId` !=
    /// k_INVALID_NODE_ID, `quorum` > 0 and `leaderInactivityIntervalMs` >
    /// 0 or `numTotalPeers < quorum`.
    void enable(int selfId,
                int quorum,
                int numTotalPeers,
                int leaderInactivityIntervalMs);

    /// Disable this state machine by moving it to `DORMANT` state, such
    /// that `isEnabled` returns false.  This method has no effect if the
    /// instance is already disabled.
    void disable();

    /// Apply the I/O event of the specified `type` having the specified
    /// `term` from the specified `sourceNodeId`, and update the specified
    /// `out` buffer with the output of state machine.  The behavior is
    /// undefined unless `out` buffer is non-null.
    void applyIOEvent(ElectorStateMachineOutput* out,
                      ElectorIOEventType::Enum   type,
                      bsls::Types::Uint64        term,
                      int                        sourceNodeId);

    /// Apply the availability I/O event of the specified `type` from the
    /// specified `sourceNodeId`, and update the specified `out` buffer with
    /// the output of state machine.  The behavior is undefined unless `out`
    /// buffer is non-null and `type` is either `e_NODE_AVAILABLE` or
    /// `e_NODE_UNAVAILABLE`.
    void applyAvailability(ElectorStateMachineOutput* out,
                           ElectorIOEventType::Enum   type,
                           int                        sourceNodeId);

    /// Apply the scouting response with the specified `willVote` flag and
    /// `term` from the specified `sourceNodeId`, and update the specified
    /// `out` buffer with the output of state machine.  The behavior is
    /// undefined unless `out` buffer is non-null.
    void applyScout(ElectorStateMachineOutput* out,
                    bool                       willVote,
                    bsls::Types::Uint64        term,
                    int                        sourceNodeId);

    /// Apply the timer event of the specified `type` to the state machine
    /// and update the specified `out` buffer with the output of state
    /// machine.  The behavior is undefined unless `out` buffer is non-null.
    void applyTimer(ElectorStateMachineOutput*  out,
                    ElectorTimerEventType::Enum type);

    /// Set quorum of this state machine to the specified `quorum`.
    void setQuorum(int quorum);

    /// Set term of this state machine to the specified `term`.
    void setTerm(bsls::Types::Uint64 term);

    // ACCESSORS

    /// Return true is this instance is enabled (i.e., in `DORMANT` state),
    /// false otherwise.
    bool isEnabled() const;

    /// Return the current state of the instance.
    ElectorState::Enum state() const;

    /// Return the reason associated with the last state change.
    ElectorTransitionReason::Enum reason() const;

    /// Return the current term of the instance.
    bsls::Types::Uint64 term() const;

    /// Return the node ID of the current leader. A return value of
    /// `k_INVALID_NODE_ID` indicates that there is no leader.
    int leaderNodeId() const;

    /// Return the node ID of the current tentative leader. A return value
    /// of `k_INVALID_NODE_ID` indicates that there is no tentative leader.
    int tentativeLeaderNodeId() const;

    /// Return the current age of the state machine.  Note that state
    /// machine's age is bumped up every time a state transition occurs.
    /// Also note that this age can be used to figure out relative order of
    /// state transitions, which can help with dropping stale state
    /// transition notifications in an asynchronous execution environment.
    bsls::Types::Uint64 age() const;
};

// =============
// class Elector
// =============

/// This component provides a mechanism to elect a leader from among a
/// cluster of nodes.
class Elector : public SessionEventProcessor {
  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBNET.ELECTOR");

  public:
    // TYPES
    typedef bmqp::BlobPoolUtil::BlobSpPool BlobSpPool;

    // PUBLIC CLASS DATA
    static const bsls::Types::Uint64 k_INVALID_TERM =
        ElectorStateMachine::k_INVALID_TERM;

    static const int k_INVALID_NODE_ID = Cluster::k_INVALID_NODE_ID;

    static const int k_ALL_NODES_ID;

    // TYPES

    /// Signature of the callback method for notifying any change in the
    /// `state` of election with an appropriate reason `code`,
    /// `leaderNodeId` and `term`.
    typedef bsl::function<void(ElectorState::Enum            state,
                               ElectorTransitionReason::Enum code,
                               int                           leaderNodeId,
                               bsls::Types::Uint64           term)>
        ElectorStateCallback;

  private:
    // PRIVATE TYPES
    typedef bsl::map<int, ClusterNode*> NodesMap;

    typedef NodesMap::iterator NodesMapIter;

    typedef bdlmt::EventScheduler::EventHandle EventHandle;

    typedef bdlmt::EventScheduler::RecurringEventHandle RecurringEventHandle;

    /// Signature of the internal callback method which wraps the user
    /// specified callback, and uses the specified `age` to prevent
    /// dispatching stale state-change events to the user.
    typedef bsl::function<void(ElectorState::Enum            state,
                               ElectorTransitionReason::Enum code,
                               int                           leaderNodeId,
                               bsls::Types::Uint64           term,
                               bsls::Types::Uint64           age)>
        ElectorStateWrapperCallback;

  private:
    // DATA
    bslma::Allocator* d_allocator_p;

    BlobSpPool* d_blobSpPool_p;

    mqbi::Cluster* d_cluster_p;

    Cluster* d_netCluster_p;

    mqbcfg::ElectorConfig& d_config;

    bdlmt::EventScheduler d_scheduler;

    bslmt::ThreadUtil::Handle d_schedDispThreadId;
    // Handle (identifier) for the thread
    // created by the event scheduler for
    // dispatching events.  This variable
    // is used only for assertions and
    // *must* *not* be used for any
    // business logic.

    mutable bslmt::Mutex d_lock;
    // Lock to serialize access to data
    // members b/w cluster-dispatcher
    // thread and scheduler thread.

    EventHandle d_initialWaitTimeoutHandle;
    // Scheduler handle for initial wait
    // timer event.

    EventHandle d_randomWaitTimeoutHandle;
    // Scheduler handle for random wait
    // timer event.

    EventHandle d_electionResultTimeoutHandle;
    // Scheduler handle for election result
    // timeout event.

    RecurringEventHandle d_heartbeatSenderRecurringHandle;
    // Scheduler handle for the recurring
    // heart beat sender event.

    RecurringEventHandle d_heartbeatCheckerRecurringHandle;
    // Scheduler handle for the recurring
    // heart beat checker handle.

    EventHandle d_scoutingResultTimeoutHandle;
    // Scheduler handle for scouting result
    // timeout event.

    ElectorStateCallback d_callback;

    ElectorStateMachine d_state;

    NodesMap d_nodes;

    bsls::Types::Uint64 d_previousEventAge;
    // Age of the previous elector state
    // change event of which the client was
    // notified.  This age can be used to
    // drop stale events.  Note that this
    // variable is manipulated *only* from
    // the cluster-dispatcher thread.

  private:
    // NOT IMPLEMENTED
    Elector(const Elector&);             // = delete;
    Elector& operator=(const Elector&);  // = delete

  private:
    // PRIVATE MANIPULATORS

    /// Callback invoked when elector state is changed to the specified
    /// `state`, `code`, `leaderNodeId`, and `term`, which the specified
    /// `age` representing the age of this event, which is used to decide if
    /// this events needs to be propagated to the user.  Note that this
    /// method is always invoked in the cluster dispatcher thread.
    void electorStateInternalCb(ElectorState::Enum            state,
                                ElectorTransitionReason::Enum code,
                                int                           leaderNodeId,
                                bsls::Types::Uint64           term,
                                bsls::Types::Uint64           age);

    /// Return a pointer to the dispatcher associated with the cluster of
    /// this elector.
    mqbi::Dispatcher* dispatcher();

    /// Process the specified `output` of the elector state machine, and if
    /// elector state change callback needs to be invoked, invoke it inline
    /// if specified `invokeStateChangeCbInline` flag is true, otherwise
    /// schedule the callback to be invoked in cluster dispatcher thread.
    /// Behavior is undefined unless `d_lock` is already acquired.
    void processStateMachineOutput(const ElectorStateMachineOutput& output,
                                   bool invokeStateChangeCbInline);

    /// Dispatch the installed state change callback in the calling thread
    /// immediately with the current state of the state machine.  Behavior
    /// is undefined unless this method is invoked in the associated
    /// cluster's dispatcher thread.
    void dispatchElectorCallback();

    /// Populate the `d_schedulerThreadHandle` variable with the ID of the
    /// scheduler's dispatcher thread.  Behavior is undefined unless this
    /// method is executed from scheduler's dispatcher thread.
    void getSchedulerDispatcherThreadHandle();

    /// Schedule the installed state change callback to be invoked in the
    /// associated cluster's dispatcher thread.  Behavior is undefined
    /// unless this method is invoked from the scheduler's dispatcher
    /// thread.  Behavior is also undefined unless `d_lock` is held.
    void scheduleElectorCallback();

    /// Cancel all the *internal* events currently scheduled in the
    /// scheduler.  Note that as specified, only internal events are
    /// cancelled, state change callback events are not.  Also note that
    /// this method does not wait for the events to get cancelled because
    /// this method may be called in the scheduler's dispatcher thread.
    /// This method can be invoked from any thread.
    void cancelSchedulerEvents();

    /// Schedule the timer event of specified `type` after a time interval
    /// corresponding to the type of the event.  This method can be invoked
    /// from any thread.
    void scheduleTimer(ElectorTimerEventType::Enum type);

    /// Emit the I/O event to the node listed in the specified
    /// ElectorStateMachine `output`.  This method can be invoked from any
    /// thread.
    void emitIOEvent(const ElectorStateMachineOutput& output);

    /// Callback invoked when elector has waited for configured time and no
    /// leader has been found.  Behavior is undefined unless this method is
    /// invoked from associated scheduler's dispatcher thread.
    void initialWaitTimeoutCb();

    /// Callback invoked when elector has waited for configured time after
    /// proposing election and quorum has not been reached.  Behavior is
    /// undefined unless this method is invoked from associated scheduler's
    /// dispatcher thread.
    void electionResultTimeoutCb();

    /// Callback invoked when elector has waited a random time (upto a
    /// maximum configured value) before proposing the election.  Behavior
    /// is undefined unless this method is invoked from associated
    /// scheduler's dispatcher thread.
    void randomWaitTimerCb();

    /// Callback invoked when leader has to send heartbeat.  Behavior is
    /// undefined unless this method is invoked from associated scheduler's
    /// dispatcher thread.
    void heartbeatSenderRecurringTimerCb();

    /// Callback invoked when to check if heart beat has been received from
    /// the leader.  Behavior is undefined unless this method is invoked
    /// from associated scheduler's dispatcher thread.
    void heartbeatCheckerRecurringTimerCb();

    /// Callback invoked when elector has waited for configured time after
    /// proposing scouting request and majority of the nodes haven't yet
    /// replied.  Behavior is undefined unless this method is invoked from
    /// associated scheduler's dispatcher thread.
    void scoutingResultTimeoutCb();

    // PRIVATE ACCESSORS

    /// Return a pointer to the dispatcher associated with the cluster of
    /// this elector.
    const mqbi::Dispatcher* dispatcher() const;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(Elector, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create an elector instance with the specified `config`, invoking the
    /// specified `callback` whenever elector state changes, using the
    /// specified `cluster` for emitting I/O events, using the specified
    /// `initalTerm` and using the specified `blobSpPool_p` for blobs
    /// allocation and `allocator` for memory allocation.
    Elector(mqbcfg::ElectorConfig&      config,
            mqbi::Cluster*              cluster,
            const ElectorStateCallback& callback,
            bsls::Types::Uint64         initialTerm,
            BlobSpPool*                 blobSpPool_p,
            bslma::Allocator*           allocator);

    /// Destroy this instance.  Behavior is undefined unless this instance
    /// is not stopped.
    ~Elector() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    /// Start the participation in election.  Return zero on success,
    /// non-zero value otherwise.  Note that the state callback can be
    /// invoked before this method has returned.  Also note that elector can
    /// be started again once it has been stopped.  Behavior is undefined
    /// unless this method is invoked in the associated cluster's dispatcher
    /// thread.
    int start();

    /// Stop the participation in election.  Note that state callback can be
    /// invoked before this method has returned.  Also note that no events
    /// will be dispatched once this method has returned.  Behavior is
    /// undefined if this method is called from the state callback.
    /// Behavior is undefined unless this method is invoked in the
    /// associated cluster's dispatcher thread.
    void stop();

    /// Apply the specified I/O `event` coming from the optionally specified
    /// `source`.  The behavior is undefined unless `event` is of type
    /// `bmqp::EventType::e_ELECTOR`.  Note that state change callback can
    /// be invoked before this method returns.  Behavior is undefined unless
    /// this method is invoked in the associated cluster's dispatcher
    /// thread.
    void processEvent(const bmqp::Event& event,
                      ClusterNode*       source = 0) BSLS_KEYWORD_OVERRIDE;

    /// Process the change in status of the specified `node` as indicated by
    /// the specified `isAvailable` flag.  Behavior is undefined unless this
    /// method is invoked in the associated cluster's dispatcher thread.
    virtual void processNodeStatus(ClusterNode* node, bool isAvailable);

    /// Process the specified `command`, and write the result to the
    /// specified electorResult.  Return zero on success or a nonzero value
    /// otherwise.
    int processCommand(mqbcmd::ElectorResult*        electorResult,
                       const mqbcmd::ElectorCommand& command);

    // ACCESSORS

    /// Return the cluster associated with this instance.
    mqbi::Cluster* cluster() const;

    /// Return the configuration of this instance.
    const mqbcfg::ElectorConfig& config() const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -------------------------------
// class ElectorStateMachineOutput
// -------------------------------

// CREATORS
inline ElectorStateMachineOutput::ElectorStateMachineOutput()
{
    reset();
}

inline ElectorStateMachineOutput::ElectorStateMachineOutput(
    bsls::Types::Uint64         term,
    ElectorIOEventType::Enum    io,
    ElectorTimerEventType::Enum timer,
    int                         destinationNodeId)
{
    d_stateChangedFlag      = false;
    d_term                  = term;
    d_io                    = io;
    d_timer                 = timer;
    d_destinationNodeId     = destinationNodeId;
    d_cancelTimerEventsFlag = false;
    d_scoutingResponseFlag  = false;
}

// MANIPULATORS
inline void ElectorStateMachineOutput::reset()
{
    d_stateChangedFlag      = false;
    d_term                  = ElectorStateMachine::k_INVALID_TERM;
    d_io                    = ElectorIOEventType::e_NONE;
    d_timer                 = ElectorTimerEventType::e_NONE;
    d_destinationNodeId     = ElectorStateMachine::k_INVALID_NODE_ID;
    d_cancelTimerEventsFlag = false;
    d_scoutingResponseFlag  = false;
}

inline ElectorStateMachineOutput&
ElectorStateMachineOutput::setStateChangedFlag(bool value)
{
    d_stateChangedFlag = value;
    return *this;
}

inline ElectorStateMachineOutput&
ElectorStateMachineOutput::setTerm(bsls::Types::Uint64 term)
{
    d_term = term;
    return *this;
}

inline ElectorStateMachineOutput&
ElectorStateMachineOutput::setIo(ElectorIOEventType::Enum value)
{
    d_io = value;
    return *this;
}

inline ElectorStateMachineOutput&
ElectorStateMachineOutput::setTimer(ElectorTimerEventType::Enum value)
{
    d_timer = value;
    return *this;
}

inline ElectorStateMachineOutput&
ElectorStateMachineOutput::setDestination(int value)
{
    d_destinationNodeId = value;
    return *this;
}

inline ElectorStateMachineOutput&
ElectorStateMachineOutput::setCancelTimerEventsFlag(bool value)
{
    d_cancelTimerEventsFlag = value;
    return *this;
}

inline ElectorStateMachineOutput&
ElectorStateMachineOutput::setScoutingResponseFlag(bool value)
{
    d_scoutingResponseFlag = value;
    return *this;
}

// ACCESSORS
inline bool ElectorStateMachineOutput::stateChangedFlag() const
{
    return d_stateChangedFlag;
}

inline bsls::Types::Uint64 ElectorStateMachineOutput::term() const
{
    return d_term;
}

inline ElectorIOEventType::Enum ElectorStateMachineOutput::io() const
{
    return d_io;
}

inline ElectorTimerEventType::Enum ElectorStateMachineOutput::timer() const
{
    return d_timer;
}

inline int ElectorStateMachineOutput::destination() const
{
    return d_destinationNodeId;
}

inline bool ElectorStateMachineOutput::cancelTimerEventsFlag() const
{
    return d_cancelTimerEventsFlag;
}

inline bool ElectorStateMachineOutput::scoutingResponseFlag() const
{
    return d_scoutingResponseFlag;
}

// -------------------------------------
// class ElectorStateMachineScoutingInfo
// -------------------------------------

// CREATORS
inline ElectorStateMachineScoutingInfo::ElectorStateMachineScoutingInfo(
    bslma::Allocator* allocator)
: d_term(ElectorStateMachine::k_INVALID_TERM)
, d_responses(allocator)
{
}

inline ElectorStateMachineScoutingInfo::ElectorStateMachineScoutingInfo(
    const ElectorStateMachineScoutingInfo& other,
    bslma::Allocator*                      allocator)
: d_term(other.d_term)
, d_responses(other.d_responses, allocator)
{
}

// MANIPULATORS
inline void ElectorStateMachineScoutingInfo::setTerm(bsls::Types::Uint64 term)
{
    d_term = term;
}

inline void ElectorStateMachineScoutingInfo::addNodeResponse(int  nodeId,
                                                             bool willVote)
{
    // A node is not expected to respond twice to the same scouting request so
    // we simply record the response.  Even if it occurs, it is ok because
    // scouting round is just a guideline, and peers are under no obligation to
    // vote positively in an election.
    d_responses[nodeId] = willVote;
}

inline void ElectorStateMachineScoutingInfo::removeNodeResponse(int nodeId)
{
    d_responses.erase(nodeId);
}

inline void ElectorStateMachineScoutingInfo::reset()
{
    d_term = ElectorStateMachine::k_INVALID_TERM;
    d_responses.clear();
}

// ACCESSORS
inline bsls::Types::Uint64 ElectorStateMachineScoutingInfo::term() const
{
    return d_term;
}

inline size_t ElectorStateMachineScoutingInfo::numResponses() const
{
    return d_responses.size();
}

inline size_t ElectorStateMachineScoutingInfo::numSupportingNodes() const
{
    size_t result = 0;

    for (NodeIdResponseMap::const_iterator cit = d_responses.begin();
         cit != d_responses.end();
         ++cit) {
        if (cit->second == true) {
            ++result;
        }
    }

    return result;
}

// -------------------------
// class ElectorStateMachine
// -------------------------

// PRIVATE ACCESSORS
inline bool ElectorStateMachine::isValidSourceNode(int sourceNodeId) const
{
    if (d_selfId == sourceNodeId || sourceNodeId == k_INVALID_NODE_ID) {
        return false;  // RETURN
    }
    return true;
}

// CREATORS
inline ElectorStateMachine::ElectorStateMachine(bslma::Allocator* allocator)
: d_state(ElectorState::e_DORMANT)
, d_reason(ElectorTransitionReason::e_NONE)
, d_term(ElectorStateMachine::k_INVALID_TERM)
, d_quorum(0)
, d_numTotalPeers(0)
, d_selfId(k_INVALID_NODE_ID)
, d_leaderNodeId(ElectorStateMachine::k_INVALID_NODE_ID)
, d_tentativeLeaderNodeId(ElectorStateMachine::k_INVALID_NODE_ID)
, d_supporters(allocator)
, d_scoutingInfo(allocator)
, d_lastLeaderHeartbeatTime(0)
, d_leaderInactivityInterval(0)
, d_age(0)
{
}

inline ElectorStateMachine::ElectorStateMachine(
    const ElectorStateMachine& other,
    bslma::Allocator*          allocator)
: d_state(other.d_state)
, d_reason(other.d_reason)
, d_term(other.d_term)
, d_quorum(other.d_quorum)
, d_selfId(other.d_selfId)
, d_leaderNodeId(other.d_leaderNodeId)
, d_tentativeLeaderNodeId(other.d_tentativeLeaderNodeId)
, d_supporters(other.d_supporters, allocator)
, d_scoutingInfo(other.d_scoutingInfo, allocator)
, d_lastLeaderHeartbeatTime(other.d_lastLeaderHeartbeatTime)
, d_leaderInactivityInterval(other.d_leaderInactivityInterval)
, d_age(other.d_age)
{
}

// MANIPULATORS
inline void ElectorStateMachine::setQuorum(int quorum)
{
    d_quorum = quorum;
}

inline void ElectorStateMachine::setTerm(bsls::Types::Uint64 term)
{
    d_term = term;
}

// ACCESSORS
inline bool ElectorStateMachine::isEnabled() const
{
    return ElectorState::e_DORMANT != d_state;
}

inline ElectorState::Enum ElectorStateMachine::state() const
{
    return d_state;
}

inline ElectorTransitionReason::Enum ElectorStateMachine::reason() const
{
    return d_reason;
}

inline bsls::Types::Uint64 ElectorStateMachine::term() const
{
    return d_term;
}

inline int ElectorStateMachine::leaderNodeId() const
{
    return d_leaderNodeId;
}

inline int ElectorStateMachine::tentativeLeaderNodeId() const
{
    return d_tentativeLeaderNodeId;
}

inline bsls::Types::Uint64 ElectorStateMachine::age() const
{
    return d_age;
}

// -------------
// class Elector
// -------------

// PRIVATE MANIPULATORS
inline mqbi::Dispatcher* Elector::dispatcher()
{
    return d_cluster_p->dispatcher();
}

// PRIVATE ACCESSORS
inline const mqbi::Dispatcher* Elector::dispatcher() const
{
    return d_cluster_p->dispatcher();
}

// ACCESSORS
inline mqbi::Cluster* Elector::cluster() const
{
    return d_cluster_p;
}

inline const mqbcfg::ElectorConfig& Elector::config() const
{
    return d_config;
}

}  // close package namespace

// -------------------------
// struct ElectorIOEventType
// -------------------------

// FREE OPERATORS
inline bsl::ostream& mqbnet::operator<<(bsl::ostream&            stream,
                                        ElectorIOEventType::Enum value)
{
    return mqbnet::ElectorIOEventType::print(stream, value, 0, -1);
}

// ----------------------------
// struct ElectorTimerEventType
// ----------------------------

// FREE OPERATORS
inline bsl::ostream& mqbnet::operator<<(bsl::ostream&               stream,
                                        ElectorTimerEventType::Enum value)
{
    return mqbnet::ElectorTimerEventType::print(stream, value, 0, -1);
}

// -------------------
// struct ElectorState
// -------------------

// FREE OPERATORS
inline bsl::ostream& mqbnet::operator<<(bsl::ostream&              stream,
                                        mqbnet::ElectorState::Enum value)
{
    return mqbnet::ElectorState::print(stream, value, 0, -1);
}

// ------------------------------
// struct ElectorTransitionReason
// ------------------------------

// FREE OPERATORS
inline bsl::ostream&
mqbnet::operator<<(bsl::ostream&                         stream,
                   mqbnet::ElectorTransitionReason::Enum value)
{
    return mqbnet::ElectorTransitionReason::print(stream, value, 0, -1);
}

}  // close enterprise namespace

#endif
