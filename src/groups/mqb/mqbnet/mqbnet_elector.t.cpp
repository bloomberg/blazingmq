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

// mqbnet_elector.t.cpp                                               -*-C++-*-
#include <mqbnet_elector.h>

// MQB
#include <mqbnet_cluster.h>

#include <bmqsys_mocktime.h>
#include <bmqsys_time.h>

// BDE
#include <bdlb_nullablevalue.h>
#include <bdlf_bind.h>
#include <bdlt_timeunitratio.h>
#include <bsl_iostream.h>
#include <bslim_printer.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

using namespace BloombergLP;
using namespace bsl;
using namespace BloombergLP::mqbnet;

// IMPLEMENTATION NOTE
// ===================
// Some test cases document the state transitions that are being tested, like
// so: "Dormant -D1-> Follower -F2-> Candidate -C4-> Leader".

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {

/// Elector clock with static storage used in various test cases
static bmqsys::MockTime* s_electorClock;

// ====================
// struct ExpectedState
// ====================

struct ExpectedState {
    ElectorState::Enum            d_state;
    ElectorTransitionReason::Enum d_reason;
    int                           d_leaderNodeId;
    int                           d_tentativeLeaderNodeId;
    bsls::Types::Uint64           d_term;
    bsls::Types::Uint64           d_age;

    ExpectedState()
    {
        // TBD: Temporary to silence compiler warning of
        //      unused-member-functions until used in test cases
        this->setLeader(0).setTentativeLeader(0).setTerm(0);
        reset();
    }

    void validate(const ElectorStateMachine& sm, int line) const
    {
        ASSERT_EQ_D(line, d_state, sm.state());
        ASSERT_EQ_D(line, d_reason, sm.reason());
        ASSERT_EQ_D(line, d_leaderNodeId, sm.leaderNodeId());
        ASSERT_EQ_D(line, d_tentativeLeaderNodeId, sm.tentativeLeaderNodeId());
        ASSERT_EQ_D(line, d_term, sm.term());
        ASSERT_EQ_D(line, d_age, sm.age());
    }

    ExpectedState& bumpAge()
    {
        ++d_age;
        return *this;
    }

    ExpectedState& setElectorState(ElectorState::Enum state)
    {
        d_state = state;
        return *this;
    }

    ExpectedState& setReason(ElectorTransitionReason::Enum reason)
    {
        d_reason = reason;
        return *this;
    }

    ExpectedState& setLeader(int leaderNodeId)
    {
        d_leaderNodeId = leaderNodeId;
        return *this;
    }

    ExpectedState& setTentativeLeader(int value)
    {
        d_tentativeLeaderNodeId = value;
        return *this;
    }

    ExpectedState& setTerm(bsls::Types::Uint64 term)
    {
        d_term = term;
        return *this;
    }

    ExpectedState& reset()
    {
        resetElectorState();
        resetReason();
        resetLeader();
        resetTentativeLeader();
        resetTerm();
        resetAge();
        return *this;
    }

    ExpectedState& resetElectorState()
    {
        d_state = ElectorState::e_DORMANT;
        return *this;
    }

    ExpectedState& resetReason()
    {
        d_reason = ElectorTransitionReason::e_NONE;
        return *this;
    }

    ExpectedState& resetLeader()
    {
        d_leaderNodeId = ElectorStateMachine::k_INVALID_NODE_ID;
        return *this;
    }

    ExpectedState& resetTentativeLeader()
    {
        d_tentativeLeaderNodeId = ElectorStateMachine::k_INVALID_NODE_ID;
        return *this;
    }

    ExpectedState& resetTerm()
    {
        d_term = 0ULL;
        return *this;
    }

    ExpectedState& resetAge()
    {
        d_age = 0ULL;
        return *this;
    }
};

#define ELECTOR_VALIDATE(A, B) A.validate(B, __LINE__);

}  // close unnamed namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------
static void test1_breathingTest()
// --------------------------------------------------------------------
// BREATHING TEST
//
// Testing:
//   Dormant -> Follower -> Candidate -> Leader
// --------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("BREATHING TEST");

    {
        PV("Ensure constants values");
        ASSERT_EQ(Elector::k_INVALID_NODE_ID, Elector::k_INVALID_NODE_ID);
        ASSERT_EQ(Elector::k_ALL_NODES_ID, Elector::k_ALL_NODES_ID);
        ASSERT_NE(Elector::k_INVALID_NODE_ID, Elector::k_ALL_NODES_ID);
    }

    // ElectorStateMachine: breathing test
    const int k_SELFID             = 0;
    const int k_QUORUM             = 3;
    const int k_TOTAL_NODES        = 4;
    const int k_INACTIVITY_INTV_MS = 6 * bdlt::TimeUnitRatio::k_MS_PER_S;

    ExpectedState exState;

    // Default construct the state machine
    ElectorStateMachine sm(s_allocator_p);
    ASSERT_EQ(false, sm.isEnabled());
    ELECTOR_VALIDATE(exState, sm);

    // Enable the state machine
    sm.enable(k_SELFID, k_QUORUM, k_TOTAL_NODES, k_INACTIVITY_INTV_MS);
    ASSERT_EQ(true, sm.isEnabled());

    exState.bumpAge()
        .setElectorState(ElectorState::e_FOLLOWER)
        .setReason(ElectorTransitionReason::e_STARTED);
    ELECTOR_VALIDATE(exState, sm);

    // Re-enable.. should be no-op.
    sm.enable(k_SELFID, k_QUORUM, k_TOTAL_NODES, k_INACTIVITY_INTV_MS);
    ASSERT_EQ(true, sm.isEnabled());
    ELECTOR_VALIDATE(exState, sm);

    // Disable
    sm.disable();
    ASSERT_EQ(false, sm.isEnabled());

    exState.bumpAge()
        .setElectorState(ElectorState::e_DORMANT)
        .setReason(ElectorTransitionReason::e_NONE);
    ELECTOR_VALIDATE(exState, sm);

    // Disable again
    sm.disable();
    ASSERT_EQ(false, sm.isEnabled());
    ELECTOR_VALIDATE(exState, sm);

    // Enable
    sm.enable(k_SELFID, k_QUORUM, k_TOTAL_NODES, k_INACTIVITY_INTV_MS);
    ASSERT_EQ(true, sm.isEnabled());

    exState.bumpAge()
        .setElectorState(ElectorState::e_FOLLOWER)
        .setReason(ElectorTransitionReason::e_STARTED);
    ELECTOR_VALIDATE(exState, sm);
}

static void test2()
// ------------------------------------------------------------------------
// Testing:
//   * Dormant -D1-> Follower -F2-> Candidate -C4-> Leader -L1-> Dormant
//   * Quorum == 3
//   * This test simulates the scenario where elector is started, becomes a
//     follower, then proposes an election after waiting for the initial
//     wait time, then becomes leader.
// ------------------------------------------------------------------------
{
    s_ignoreCheckDefAlloc = true;
    // Logging infrastructure allocates using the default allocator, and
    // that logging is beyond the control of this function.

    bmqtst::TestHelper::printTestName("TEST 2");

    const int k_SELFID             = 0;
    const int k_QUORUM             = 3;
    const int k_TOTAL_NODES        = 4;
    const int k_INVALID_NODE       = ElectorStateMachine::k_INVALID_NODE_ID;
    const int k_ALL_NODES          = ElectorStateMachine::k_ALL_NODES_ID;
    const int k_INACTIVITY_INTV_MS = 6 * bdlt::TimeUnitRatio::k_MS_PER_S;

    bsls::Types::Uint64 age = 0;
    ElectorStateMachine sm(s_allocator_p);

    ASSERT_EQ(ElectorState::e_DORMANT, sm.state());

    // Dormant -D1-> Follower
    sm.enable(k_SELFID, k_QUORUM, k_TOTAL_NODES, k_INACTIVITY_INTV_MS);
    ASSERT_EQ(ElectorState::e_FOLLOWER, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_STARTED, sm.reason());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(0ULL, sm.term());
    ASSERT_EQ(++age, sm.age());

    ElectorStateMachineOutput output;

    // Apply INITIAL_WAIT_TIMER to follower.  It should emit scouting request.
    sm.applyTimer(&output, ElectorTimerEventType::e_INITIAL_WAIT_TIMER);
    ASSERT_EQ(false, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_FOLLOWER, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_NONE, sm.reason());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(0ULL, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_SCOUTING_REQUEST, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_SCOUTING_RESULT_TIMER, output.timer());
    ASSERT_EQ(ElectorStateMachine::k_ALL_NODES_ID, output.destination());
    ASSERT_EQ(false, output.cancelTimerEventsFlag());
    ASSERT_EQ(age, sm.age());

    // Apply 2 scouting responses.

    // Apply 1st scouting response.
    sm.applyScout(&output,
                  true,           // Will vote
                  sm.term() + 1,  // Scouting happens with a higher term
                  k_SELFID + 1);  // Peer node Id
    ASSERT_EQ(false, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_FOLLOWER, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_NONE, sm.reason());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(0ULL, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_NONE, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_NONE, output.timer());
    ASSERT_EQ(k_INVALID_NODE, output.destination());
    ASSERT_EQ(false, output.cancelTimerEventsFlag());
    ASSERT_EQ(age, sm.age());

    // Apply 2nd scouting response.  Quorum will be achieved, and elector will
    // transition to candidate.
    // Follower -F2-> Candidate
    sm.applyScout(&output,
                  true,           // Will vote
                  sm.term() + 1,  // Scouting happens with a higher term
                  k_SELFID + 2);  // Peer node Id
    ASSERT_EQ(true, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_CANDIDATE, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_NONE, sm.reason());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(1ULL, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_ELECTION_PROPOSAL, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_ELECTION_RESULT_TIMER, output.timer());
    ASSERT_EQ(ElectorStateMachine::k_ALL_NODES_ID, output.destination());
    ASSERT_EQ(true, output.cancelTimerEventsFlag());
    ASSERT_EQ(++age, sm.age());

    // Apply 2 election responses

    // 1st election response
    sm.applyIOEvent(&output,
                    ElectorIOEventType::e_ELECTION_RESPONSE,
                    sm.term(),      // term
                    k_SELFID + 1);  // Node

    ASSERT_EQ(false, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_CANDIDATE, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_NONE, sm.reason());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(1ULL, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_NONE, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_NONE, output.timer());
    ASSERT_EQ(k_INVALID_NODE, output.destination());
    ASSERT_EQ(false, output.cancelTimerEventsFlag());
    ASSERT_EQ(age, sm.age());

    // 2nd election response
    // Candidate -C4-> Leader
    sm.applyIOEvent(&output,
                    ElectorIOEventType::e_ELECTION_RESPONSE,
                    sm.term(),      // term
                    k_SELFID + 2);  // nodeId

    ASSERT_EQ(true, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_LEADER, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_NONE, sm.reason());
    ASSERT_EQ(k_SELFID, sm.leaderNodeId());
    ASSERT_EQ(1ULL, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_LEADER_HEARTBEAT, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_HEARTBEAT_BROADCAST_TIMER,
              output.timer());
    ASSERT_EQ(k_ALL_NODES, output.destination());
    ASSERT_EQ(true, output.cancelTimerEventsFlag());
    ASSERT_EQ(++age, sm.age());

    // Apply a node_available event to the leader.  Leader should emit a
    // heart beat event only for the new nodeId, with no state change.
    sm.applyAvailability(&output,
                         ElectorIOEventType::e_NODE_AVAILABLE,
                         k_SELFID + 3);  // New nodeId

    ASSERT_EQ(false, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_LEADER, sm.state());
    ASSERT_EQ(k_SELFID, sm.leaderNodeId());
    ASSERT_EQ(1ULL, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_LEADER_HEARTBEAT, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_NONE, output.timer());
    ASSERT_EQ(3, output.destination());
    ASSERT_EQ(false, output.cancelTimerEventsFlag());
    ASSERT_EQ(age, sm.age());

    // Leader -L1-> Dormant
    sm.disable();
    ASSERT_EQ(false, sm.isEnabled());
    ASSERT_EQ(ElectorState::e_DORMANT, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_NONE, sm.reason());
    ASSERT_EQ(0ULL, sm.term());
    ASSERT_EQ(ElectorStateMachine::k_INVALID_NODE_ID, sm.leaderNodeId());
    ASSERT_EQ(ElectorStateMachine::k_INVALID_NODE_ID,
              sm.tentativeLeaderNodeId());
    ASSERT_EQ(++age, sm.age());
}

static void test3()
// ------------------------------------------------------------------------
// Testing:
//   * Dormant -D1-> Follower -F3-> Candidate -C4-> Leader -L1-> Dormant
//   * Quorum == 3
//   * This test simulates the scenario where elector is started, becomes a
//     follower, then proposes an election after waiting for the random
//     wait time, then becomes leader.
// ------------------------------------------------------------------------
{
    s_ignoreCheckDefAlloc = true;
    // Logging infrastructure allocates using the default allocator, and
    // that logging is beyond the control of this function.

    bmqtst::TestHelper::printTestName("TEST 3");

    const int k_SELFID             = 0;
    const int k_QUORUM             = 3;
    const int k_TOTAL_NODES        = 4;
    const int k_INVALID_NODE       = ElectorStateMachine::k_INVALID_NODE_ID;
    const int k_ALL_NODES          = ElectorStateMachine::k_ALL_NODES_ID;
    const int k_INACTIVITY_INTV_MS = 6 * bdlt::TimeUnitRatio::k_MS_PER_S;

    bsls::Types::Uint64 age = 0;
    ElectorStateMachine sm(s_allocator_p);

    ASSERT_EQ(ElectorState::e_DORMANT, sm.state());

    // Dormant -D1-> Follower
    sm.enable(k_SELFID, k_QUORUM, k_TOTAL_NODES, k_INACTIVITY_INTV_MS);
    ASSERT_EQ(ElectorState::e_FOLLOWER, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_STARTED, sm.reason());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(0ULL, sm.term());
    ASSERT_EQ(++age, sm.age());

    ElectorStateMachineOutput output;

    // Apply RANDOM_WAIT_TIMER to follower.  It should emit scouting request.
    sm.applyTimer(&output, ElectorTimerEventType::e_RANDOM_WAIT_TIMER);
    ASSERT_EQ(false, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_FOLLOWER, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_NONE, sm.reason());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(0ULL, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_SCOUTING_REQUEST, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_SCOUTING_RESULT_TIMER, output.timer());
    ASSERT_EQ(ElectorStateMachine::k_ALL_NODES_ID, output.destination());
    ASSERT_EQ(false, output.cancelTimerEventsFlag());
    ASSERT_EQ(age, sm.age());

    // Apply 2 scouting responses.

    // Apply 1st scouting response.
    sm.applyScout(&output,
                  true,           // Will vote
                  sm.term() + 1,  // Scouting happens with a higher term
                  k_SELFID + 1);  // Peer node Id
    ASSERT_EQ(false, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_FOLLOWER, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_NONE, sm.reason());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(0ULL, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_NONE, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_NONE, output.timer());
    ASSERT_EQ(k_INVALID_NODE, output.destination());
    ASSERT_EQ(false, output.cancelTimerEventsFlag());
    ASSERT_EQ(age, sm.age());

    // Apply 2nd scouting response.  Quorum will be achieved, and elector will
    // transition to candidate.
    // Follower -F3-> Candidate
    sm.applyScout(&output,
                  true,           // Will vote
                  sm.term() + 1,  // Scouting happens with a higher term
                  k_SELFID + 2);  // Peer node Id
    ASSERT_EQ(true, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_CANDIDATE, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_NONE, sm.reason());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(1ULL, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_ELECTION_PROPOSAL, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_ELECTION_RESULT_TIMER, output.timer());
    ASSERT_EQ(ElectorStateMachine::k_ALL_NODES_ID, output.destination());
    ASSERT_EQ(true, output.cancelTimerEventsFlag());
    ASSERT_EQ(++age, sm.age());

    // Apply 2 election responses

    // 1st election response
    sm.applyIOEvent(&output,
                    ElectorIOEventType::e_ELECTION_RESPONSE,
                    sm.term(),      // term
                    k_SELFID + 1);  // nodeId

    ASSERT_EQ(false, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_CANDIDATE, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_NONE, sm.reason());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(1ULL, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_NONE, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_NONE, output.timer());
    ASSERT_EQ(k_INVALID_NODE, output.destination());
    ASSERT_EQ(false, output.cancelTimerEventsFlag());
    ASSERT_EQ(age, sm.age());

    // 2nd election response
    // Candidate -C4-> Leader
    sm.applyIOEvent(&output,
                    ElectorIOEventType::e_ELECTION_RESPONSE,
                    sm.term(),      // term
                    k_SELFID + 2);  // nodeId

    ASSERT_EQ(true, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_LEADER, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_NONE, sm.reason());
    ASSERT_EQ(k_SELFID, sm.leaderNodeId());
    ASSERT_EQ(1ULL, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_LEADER_HEARTBEAT, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_HEARTBEAT_BROADCAST_TIMER,
              output.timer());
    ASSERT_EQ(k_ALL_NODES, output.destination());
    ASSERT_EQ(true, output.cancelTimerEventsFlag());
    ASSERT_EQ(++age, sm.age());

    // Leader -L1-> Dormant
    sm.disable();
    ASSERT_EQ(false, sm.isEnabled());
    ASSERT_EQ(ElectorState::e_DORMANT, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_NONE, sm.reason());
    ASSERT_EQ(0ULL, sm.term());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(k_INVALID_NODE, sm.tentativeLeaderNodeId());
    ASSERT_EQ(++age, sm.age());
}

static void test4()
// ------------------------------------------------------------------------
// Testing:
//   * Dormant -D1-> Follower -F2-> Candidate -C2-> Follower -F3->
//     Candidate -C3-> Follower -F3-> Candidate -C6-> Follower -F3->
//     Candidate -C1-> Dormant
//   * Quorum == 3
//   * This test checks for the scenario when an elector is started,
//     transitions to follower, then candidate, but receives a leader
//     hearbeat and starts following it, then leader inactivity timer
//     fires.
//     and goes back to being a follower (this occurs 2-3 times).
// ------------------------------------------------------------------------
{
    s_ignoreCheckDefAlloc = true;
    // Logging infrastructure allocates using the default allocator, and
    // that logging is beyond the control of this function.

    bmqtst::TestHelper::printTestName("TEST 4");

    const int k_SELFID             = 0;
    const int k_QUORUM             = 3;
    const int k_TOTAL_NODES        = 4;
    const int k_INVALID_NODE       = ElectorStateMachine::k_INVALID_NODE_ID;
    const int k_ALL_NODES          = ElectorStateMachine::k_ALL_NODES_ID;
    const int k_INACTIVITY_INTV_MS = 6 * bdlt::TimeUnitRatio::k_MS_PER_S;

    // Reset the test clock as it will be used in this test case
    s_electorClock->reset();

    bsls::Types::Uint64 age = 0;
    ElectorStateMachine sm(s_allocator_p);

    ASSERT_EQ(ElectorState::e_DORMANT, sm.state());

    // Dormant -D1-> Follower
    sm.enable(k_SELFID, k_QUORUM, k_TOTAL_NODES, k_INACTIVITY_INTV_MS);
    ASSERT_EQ(ElectorState::e_FOLLOWER, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_STARTED, sm.reason());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(k_INVALID_NODE, sm.tentativeLeaderNodeId());
    ASSERT_EQ(0ULL, sm.term());
    ASSERT_EQ(++age, sm.age());

    ElectorStateMachineOutput output;
    bsls::Types::Uint64       term = 1;

    // Apply INITIAL_WAIT_TIMER to follower.  It should emit scouting request.
    sm.applyTimer(&output, ElectorTimerEventType::e_INITIAL_WAIT_TIMER);
    ASSERT_EQ(false, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_FOLLOWER, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_NONE, sm.reason());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(0ULL, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_SCOUTING_REQUEST, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_SCOUTING_RESULT_TIMER, output.timer());
    ASSERT_EQ(ElectorStateMachine::k_ALL_NODES_ID, output.destination());
    ASSERT_EQ(false, output.cancelTimerEventsFlag());
    ASSERT_EQ(age, sm.age());

    // Apply 2 scouting responses.

    // Apply 1st scouting response.
    sm.applyScout(&output,
                  true,           // Will vote
                  sm.term() + 1,  // Scouting happens with a higher term
                  k_SELFID + 1);  // Peer node Id
    ASSERT_EQ(false, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_FOLLOWER, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_NONE, sm.reason());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(0ULL, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_NONE, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_NONE, output.timer());
    ASSERT_EQ(k_INVALID_NODE, output.destination());
    ASSERT_EQ(false, output.cancelTimerEventsFlag());
    ASSERT_EQ(age, sm.age());

    // Apply 2nd scouting response.  Quorum will be achieved, and elector will
    // transition to candidate.
    // Follower -F2-> Candidate
    sm.applyScout(&output,
                  true,           // Will vote
                  sm.term() + 1,  // Scouting happens with a higher term
                  k_SELFID + 2);  // Peer node Id
    ASSERT_EQ(true, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_CANDIDATE, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_NONE, sm.reason());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(1ULL, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_ELECTION_PROPOSAL, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_ELECTION_RESULT_TIMER, output.timer());
    ASSERT_EQ(ElectorStateMachine::k_ALL_NODES_ID, output.destination());
    ASSERT_EQ(true, output.cancelTimerEventsFlag());
    ASSERT_EQ(++age, sm.age());

    // Apply leader heartbeat with same term as candidate.  Candidate should
    // transition to a follower.
    // Candidate -C2-> Follower
    sm.applyIOEvent(&output,
                    ElectorIOEventType::e_LEADER_HEARTBEAT,
                    sm.term(),      // leader's term == existing 'd_term'
                    k_SELFID + 3);  // leader node id
    ASSERT_EQ(true, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_FOLLOWER, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_NONE, sm.reason());
    ASSERT_EQ(k_SELFID + 3, sm.leaderNodeId());
    ASSERT_EQ(k_INVALID_NODE, sm.tentativeLeaderNodeId());
    ASSERT_EQ(term, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_ELECTION_RESPONSE, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_HEARTBEAT_CHECK_TIMER, output.timer());
    ASSERT_EQ(k_SELFID + 3, output.destination());
    ASSERT_EQ(true, output.cancelTimerEventsFlag());
    ASSERT_EQ(++age, sm.age());

    // Move time forward by an interval > k_INACTIVITY_INTV_MS.  Note that
    // state machine uses high resolution timer, which has nano second
    // resolution.
    s_electorClock->advanceHighResTimer(
        (k_INACTIVITY_INTV_MS + 2 * bdlt::TimeUnitRatio::k_MS_PER_S) *
        bdlt::TimeUnitRatio::k_NS_PER_MS);

    // Follower -> HeartbeatCheckTimer (No leader due to inactivity)
    sm.applyTimer(&output, ElectorTimerEventType::e_HEARTBEAT_CHECK_TIMER);
    ASSERT_EQ(true, output.stateChangedFlag());  // no leader
    ASSERT_EQ(ElectorState::e_FOLLOWER, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_LEADER_NO_HEARTBEAT, sm.reason());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(k_INVALID_NODE, sm.tentativeLeaderNodeId());
    ASSERT_EQ(term, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_NONE, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_RANDOM_WAIT_TIMER, output.timer());
    ASSERT_EQ(k_INVALID_NODE, output.destination());
    ASSERT_EQ(true, output.cancelTimerEventsFlag());
    ASSERT_EQ(++age, sm.age());

    // Apply random wait timer.  Follower should emit scouting request.
    sm.applyTimer(&output, ElectorTimerEventType::e_RANDOM_WAIT_TIMER);
    ASSERT_EQ(false, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_FOLLOWER, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_NONE, sm.reason());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(term, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_SCOUTING_REQUEST, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_SCOUTING_RESULT_TIMER, output.timer());
    ASSERT_EQ(ElectorStateMachine::k_ALL_NODES_ID, output.destination());
    ASSERT_EQ(false, output.cancelTimerEventsFlag());
    ASSERT_EQ(age, sm.age());

    // Apply 2 scouting responses.

    // Apply 1st scouting response.
    sm.applyScout(&output,
                  true,           // Will vote
                  sm.term() + 1,  // Scouting happens with a higher term
                  k_SELFID + 1);  // Peer node Id
    ASSERT_EQ(false, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_FOLLOWER, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_NONE, sm.reason());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(term, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_NONE, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_NONE, output.timer());
    ASSERT_EQ(k_INVALID_NODE, output.destination());
    ASSERT_EQ(false, output.cancelTimerEventsFlag());
    ASSERT_EQ(age, sm.age());

    // Apply 2nd scouting response.  Quorum will be achieved, and elector will
    // transition to candidate.
    // Follower -F3-> Candidate
    term += 1;
    sm.applyScout(&output,
                  true,           // Will vote
                  sm.term() + 1,  // Scouting happens with a higher term
                  k_SELFID + 2);  // Peer node Id
    ASSERT_EQ(true, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_CANDIDATE, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_NONE, sm.reason());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(k_INVALID_NODE, sm.tentativeLeaderNodeId());
    ASSERT_EQ(term, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_ELECTION_PROPOSAL, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_ELECTION_RESULT_TIMER, output.timer());
    ASSERT_EQ(k_ALL_NODES, output.destination());
    ASSERT_EQ(true, output.cancelTimerEventsFlag());
    ASSERT_EQ(++age, sm.age());

    term += 1;

    // Candidate -C3-> Follower
    sm.applyIOEvent(&output,
                    ElectorIOEventType::e_ELECTION_PROPOSAL,
                    term,           // new term > existing 'd_term'
                    k_SELFID + 3);  // leader node id
    ASSERT_EQ(true, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_FOLLOWER, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_ELECTION_PREEMPTED, sm.reason());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(k_SELFID + 3, sm.tentativeLeaderNodeId());
    ASSERT_EQ(term, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_ELECTION_RESPONSE, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_HEARTBEAT_CHECK_TIMER, output.timer());
    ASSERT_EQ(k_SELFID + 3, output.destination());
    ASSERT_EQ(true, output.cancelTimerEventsFlag());
    ASSERT_EQ(++age, sm.age());

    // No heartbeat received from tentative leader within stipulated time.
    // Move time forward by an interval > k_INACTIVITY_INTV_MS
    s_electorClock->advanceHighResTimer(
        (k_INACTIVITY_INTV_MS + bdlt::TimeUnitRatio::k_MS_PER_S) *
        bdlt::TimeUnitRatio::k_NS_PER_MS);

    // Follower -> HeartbeatCheckTimer -> No leader
    sm.applyTimer(&output, ElectorTimerEventType::e_HEARTBEAT_CHECK_TIMER);
    ASSERT_EQ(false, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_FOLLOWER, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_NONE, sm.reason());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(k_INVALID_NODE, sm.tentativeLeaderNodeId());
    ASSERT_EQ(term, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_NONE, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_RANDOM_WAIT_TIMER, output.timer());
    ASSERT_EQ(k_INVALID_NODE, output.destination());
    ASSERT_EQ(true, output.cancelTimerEventsFlag());
    ASSERT_EQ(age, sm.age());

    // Apply random wait timer.  Follower should emit scouting request.
    sm.applyTimer(&output, ElectorTimerEventType::e_RANDOM_WAIT_TIMER);
    ASSERT_EQ(false, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_FOLLOWER, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_NONE, sm.reason());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(term, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_SCOUTING_REQUEST, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_SCOUTING_RESULT_TIMER, output.timer());
    ASSERT_EQ(ElectorStateMachine::k_ALL_NODES_ID, output.destination());
    ASSERT_EQ(false, output.cancelTimerEventsFlag());
    ASSERT_EQ(age, sm.age());

    // Apply 2 scouting responses.

    // Apply 1st scouting response.
    sm.applyScout(&output,
                  true,           // Will vote
                  sm.term() + 1,  // Scouting happens with a higher term
                  k_SELFID + 1);  // Peer node Id
    ASSERT_EQ(false, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_FOLLOWER, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_NONE, sm.reason());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(term, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_NONE, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_NONE, output.timer());
    ASSERT_EQ(k_INVALID_NODE, output.destination());
    ASSERT_EQ(false, output.cancelTimerEventsFlag());
    ASSERT_EQ(age, sm.age());

    // Apply 2nd scouting response.  Quorum will be achieved, and elector will
    // transition to candidate.
    // Follower -F3-> Candidate
    term += 1;
    sm.applyScout(&output,
                  true,           // Will vote
                  sm.term() + 1,  // Scouting happens with a higher term
                  k_SELFID + 2);  // Peer node Id
    ASSERT_EQ(true, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_CANDIDATE, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_NONE, sm.reason());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(k_INVALID_NODE, sm.tentativeLeaderNodeId());
    ASSERT_EQ(term, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_ELECTION_PROPOSAL, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_ELECTION_RESULT_TIMER, output.timer());
    ASSERT_EQ(k_ALL_NODES, output.destination());
    ASSERT_EQ(true, output.cancelTimerEventsFlag());
    ASSERT_EQ(++age, sm.age());

    // xxx

    // Not election responses are received.  Apply election result timer.
    // Candidate -C6-> Follower
    sm.applyTimer(&output, ElectorTimerEventType::e_ELECTION_RESULT_TIMER);
    ASSERT_EQ(true, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_FOLLOWER, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_QUORUM_NOT_ACHIEVED, sm.reason());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(k_INVALID_NODE, sm.tentativeLeaderNodeId());
    ASSERT_EQ(term, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_NONE, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_RANDOM_WAIT_TIMER, output.timer());
    ASSERT_EQ(k_INVALID_NODE, output.destination());
    ASSERT_EQ(true, output.cancelTimerEventsFlag());
    ASSERT_EQ(++age, sm.age());

    // Follower -F1-> Dormant
    sm.disable();
    ASSERT_EQ(false, sm.isEnabled());
    ASSERT_EQ(ElectorState::e_DORMANT, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_NONE, sm.reason());
    ASSERT_EQ(0ULL, sm.term());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(k_INVALID_NODE, sm.tentativeLeaderNodeId());
    ASSERT_EQ(++age, sm.age());
}

static void test5()
// ------------------------------------------------------------------------
// Testing:
//   * Dormant -D1-> Follower -F2-> Candidate -C4-> Leader -L2-> Follower
//   * Quorum == 3
//   * This test ensures that when an elector is started, it transitions to
//     follower, then sends scouting request after initial wait timer, then
//     transitions to candidate upon receiving sufficient scouting
//     responses, then proposes election and transitions to candidate, and
//     upon receiving sufficient election response, transitions to leader.
//     Finally, it transitions to a follower after receiving leader heart
//     beat with higher term and sends leadership cession event.
// -------------------------------------------------------------------------
{
    s_ignoreCheckDefAlloc = true;
    // Logging infrastructure allocates using the default allocator, and
    // that logging is beyond the control of this function.

    bmqtst::TestHelper::printTestName("TEST 5");

    const int k_SELFID             = 0;
    const int k_QUORUM             = 3;
    const int k_TOTAL_NODES        = 4;
    const int k_INVALID_NODE       = ElectorStateMachine::k_INVALID_NODE_ID;
    const int k_ALL_NODES          = ElectorStateMachine::k_ALL_NODES_ID;
    const int k_INACTIVITY_INTV_MS = 6 * bdlt::TimeUnitRatio::k_MS_PER_S;

    bsls::Types::Uint64 age = 0;
    ElectorStateMachine sm(s_allocator_p);

    ASSERT_EQ(ElectorState::e_DORMANT, sm.state());

    // Dormant -D1-> Follower
    sm.enable(k_SELFID, k_QUORUM, k_TOTAL_NODES, k_INACTIVITY_INTV_MS);
    ASSERT_EQ(ElectorState::e_FOLLOWER, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_STARTED, sm.reason());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(k_INVALID_NODE, sm.tentativeLeaderNodeId());
    ASSERT_EQ(0ULL, sm.term());
    ASSERT_EQ(++age, sm.age());

    ElectorStateMachineOutput output;
    bsls::Types::Uint64       term = 1;

    // Apply INITIAL_WAIT_TIMER to follower.  It should emit scouting request.
    sm.applyTimer(&output, ElectorTimerEventType::e_INITIAL_WAIT_TIMER);
    ASSERT_EQ(false, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_FOLLOWER, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_NONE, sm.reason());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(0ULL, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_SCOUTING_REQUEST, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_SCOUTING_RESULT_TIMER, output.timer());
    ASSERT_EQ(ElectorStateMachine::k_ALL_NODES_ID, output.destination());
    ASSERT_EQ(false, output.cancelTimerEventsFlag());
    ASSERT_EQ(age, sm.age());

    // Apply 2 scouting responses.

    // Apply 1st scouting response.
    sm.applyScout(&output,
                  true,           // Will vote
                  sm.term() + 1,  // Scouting happens with a higher term
                  k_SELFID + 1);  // Peer node Id
    ASSERT_EQ(false, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_FOLLOWER, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_NONE, sm.reason());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(0ULL, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_NONE, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_NONE, output.timer());
    ASSERT_EQ(k_INVALID_NODE, output.destination());
    ASSERT_EQ(false, output.cancelTimerEventsFlag());
    ASSERT_EQ(age, sm.age());

    // Apply 2nd scouting response.  Quorum will be achieved, and elector will
    // transition to candidate.
    // Follower -F2-> Candidate
    sm.applyScout(&output,
                  true,           // Will vote
                  sm.term() + 1,  // Scouting happens with a higher term
                  k_SELFID + 2);  // Peer node Id
    ASSERT_EQ(true, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_CANDIDATE, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_NONE, sm.reason());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(term, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_ELECTION_PROPOSAL, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_ELECTION_RESULT_TIMER, output.timer());
    ASSERT_EQ(ElectorStateMachine::k_ALL_NODES_ID, output.destination());
    ASSERT_EQ(true, output.cancelTimerEventsFlag());
    ASSERT_EQ(++age, sm.age());

    // Apply 2 election responses

    // 1st election response
    sm.applyIOEvent(&output,
                    ElectorIOEventType::e_ELECTION_RESPONSE,
                    term,
                    k_SELFID + 1);  // nodeId
    ASSERT_EQ(false, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_CANDIDATE, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_NONE, sm.reason());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(term, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_NONE, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_NONE, output.timer());
    ASSERT_EQ(k_INVALID_NODE, output.destination());
    ASSERT_EQ(false, output.cancelTimerEventsFlag());
    ASSERT_EQ(age, sm.age());

    // 2nd election response
    // Candidate -C4-> Leader
    sm.applyIOEvent(&output,
                    ElectorIOEventType::e_ELECTION_RESPONSE,
                    term,           // term
                    k_SELFID + 2);  // nodeId
    ASSERT_EQ(true, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_LEADER, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_NONE, sm.reason());
    ASSERT_EQ(k_SELFID, sm.leaderNodeId());
    ASSERT_EQ(term, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_LEADER_HEARTBEAT, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_HEARTBEAT_BROADCAST_TIMER,
              output.timer());
    ASSERT_EQ(k_ALL_NODES, output.destination());
    ASSERT_EQ(true, output.cancelTimerEventsFlag());
    ASSERT_EQ(++age, sm.age());

    term += 1;

    // Leader -L2-> Follower
    sm.applyIOEvent(&output,
                    ElectorIOEventType::e_LEADER_HEARTBEAT,
                    term,           // term > leader's term
                    k_SELFID + 2);  // new leader nodeId

    ASSERT_EQ(true, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_FOLLOWER, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_NONE, sm.reason());
    ASSERT_EQ(k_SELFID + 2, sm.leaderNodeId());  // new
                                                 // leader
                                                 // nodeId
    ASSERT_EQ(k_INVALID_NODE, sm.tentativeLeaderNodeId());
    ASSERT_EQ(term, sm.term());

    ASSERT_EQ(ElectorIOEventType::e_LEADERSHIP_CESSION, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_HEARTBEAT_CHECK_TIMER, output.timer());

    ASSERT_EQ(k_ALL_NODES, output.destination());
    ASSERT_EQ(true, output.cancelTimerEventsFlag());
    ASSERT_EQ(++age, sm.age());
}

static void test6()
// ------------------------------------------------------------------------
// Testing:
//   * Dormant -D1-> Follower -F2-> Candidate -C4-> Leader -> Receive
//     election proposal -> Continue to be the leader
//   * This case tests the scenario where a leader receives election
//     proposals with smaller, same and higher terms, but ignores them in
//     all 3 cases.
// ------------------------------------------------------------------------
{
    s_ignoreCheckDefAlloc = true;
    // Logging infrastructure allocates using the default allocator, and
    // that logging is beyond the control of this function.

    bmqtst::TestHelper::printTestName("TEST 6");

    const int k_SELFID             = 0;
    const int k_QUORUM             = 3;
    const int k_TOTAL_NODES        = 4;
    const int k_INVALID_NODE       = ElectorStateMachine::k_INVALID_NODE_ID;
    const int k_ALL_NODES          = ElectorStateMachine::k_ALL_NODES_ID;
    const int k_INACTIVITY_INTV_MS = 6 * bdlt::TimeUnitRatio::k_MS_PER_S;

    bsls::Types::Uint64 age = 0;
    ElectorStateMachine sm(s_allocator_p);

    ASSERT_EQ(ElectorState::e_DORMANT, sm.state());

    // Dormant -D1-> Follower
    sm.enable(k_SELFID, k_QUORUM, k_TOTAL_NODES, k_INACTIVITY_INTV_MS);
    ASSERT_EQ(ElectorState::e_FOLLOWER, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_STARTED, sm.reason());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(k_INVALID_NODE, sm.tentativeLeaderNodeId());
    ASSERT_EQ(0ULL, sm.term());
    ASSERT_EQ(++age, sm.age());

    ElectorStateMachineOutput output;
    bsls::Types::Uint64       term = 1;

    // Apply INITIAL_WAIT_TIMER to follower.  It should emit scouting request.
    sm.applyTimer(&output, ElectorTimerEventType::e_INITIAL_WAIT_TIMER);
    ASSERT_EQ(false, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_FOLLOWER, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_NONE, sm.reason());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(0ULL, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_SCOUTING_REQUEST, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_SCOUTING_RESULT_TIMER, output.timer());
    ASSERT_EQ(ElectorStateMachine::k_ALL_NODES_ID, output.destination());
    ASSERT_EQ(false, output.cancelTimerEventsFlag());
    ASSERT_EQ(age, sm.age());

    // Apply 2 scouting responses.

    // Apply 1st scouting response.
    sm.applyScout(&output,
                  true,           // Will vote
                  sm.term() + 1,  // Scouting happens with a higher term
                  k_SELFID + 1);  // Peer node Id
    ASSERT_EQ(false, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_FOLLOWER, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_NONE, sm.reason());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(0ULL, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_NONE, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_NONE, output.timer());
    ASSERT_EQ(k_INVALID_NODE, output.destination());
    ASSERT_EQ(false, output.cancelTimerEventsFlag());
    ASSERT_EQ(age, sm.age());

    // Apply 2nd scouting response.  Quorum will be achieved, and elector will
    // transition to candidate.
    // Follower -F2-> Candidate
    sm.applyScout(&output,
                  true,           // Will vote
                  sm.term() + 1,  // Scouting happens with a higher term
                  k_SELFID + 2);  // Peer node Id
    ASSERT_EQ(true, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_CANDIDATE, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_NONE, sm.reason());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(term, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_ELECTION_PROPOSAL, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_ELECTION_RESULT_TIMER, output.timer());
    ASSERT_EQ(ElectorStateMachine::k_ALL_NODES_ID, output.destination());
    ASSERT_EQ(true, output.cancelTimerEventsFlag());
    ASSERT_EQ(++age, sm.age());

    // Apply 2 election responses

    // 1st election response
    sm.applyIOEvent(&output,
                    ElectorIOEventType::e_ELECTION_RESPONSE,
                    term,
                    k_SELFID + 1);  // nodeId

    ASSERT_EQ(false, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_CANDIDATE, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_NONE, sm.reason());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(term, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_NONE, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_NONE, output.timer());
    ASSERT_EQ(k_INVALID_NODE, output.destination());
    ASSERT_EQ(false, output.cancelTimerEventsFlag());
    ASSERT_EQ(age, sm.age());

    // 2nd election response
    // Candidate -C4-> Leader
    sm.applyIOEvent(&output,
                    ElectorIOEventType::e_ELECTION_RESPONSE,
                    term,           // term
                    k_SELFID + 2);  // nodeId

    ASSERT_EQ(true, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_LEADER, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_NONE, sm.reason());
    ASSERT_EQ(k_SELFID, sm.leaderNodeId());
    ASSERT_EQ(term, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_LEADER_HEARTBEAT, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_HEARTBEAT_BROADCAST_TIMER,
              output.timer());
    ASSERT_EQ(k_ALL_NODES, output.destination());
    ASSERT_EQ(true, output.cancelTimerEventsFlag());
    ASSERT_EQ(++age, sm.age());

    // Apply election proposal with smaller term from another node.  Leader
    // should ignore it.
    sm.applyIOEvent(&output,
                    ElectorIOEventType::e_ELECTION_PROPOSAL,
                    sm.term() - 1,  // Smaller term
                    3);             // Different nodeId

    ASSERT_EQ(false, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_LEADER, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_NONE, sm.reason());
    ASSERT_EQ(k_SELFID, sm.leaderNodeId());
    ASSERT_EQ(k_INVALID_NODE, sm.tentativeLeaderNodeId());
    ASSERT_EQ(term, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_NONE, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_NONE, output.timer());
    ASSERT_EQ(k_INVALID_NODE, output.destination());
    ASSERT_EQ(false, output.cancelTimerEventsFlag());
    ASSERT_EQ(age, sm.age());

    // Apply election proposal with same term from another node.  Leader
    // should ignore it.
    sm.applyIOEvent(&output,
                    ElectorIOEventType::e_ELECTION_PROPOSAL,
                    sm.term(),  // Same term
                    3);         // Different nodeId

    ASSERT_EQ(false, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_LEADER, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_NONE, sm.reason());
    ASSERT_EQ(k_SELFID, sm.leaderNodeId());
    ASSERT_EQ(k_INVALID_NODE, sm.tentativeLeaderNodeId());
    ASSERT_EQ(term, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_NONE, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_NONE, output.timer());
    ASSERT_EQ(k_INVALID_NODE, output.destination());
    ASSERT_EQ(false, output.cancelTimerEventsFlag());
    ASSERT_EQ(age, sm.age());

    // Apply election proposal with greater term from another node.  Leader
    // should ignore it.  This scenario tests the notion of sticky leader.
    sm.applyIOEvent(&output,
                    ElectorIOEventType::e_ELECTION_PROPOSAL,
                    sm.term() + 5,  // Greater term
                    3);             // Different nodeId

    ASSERT_EQ(false, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_LEADER, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_NONE, sm.reason());
    ASSERT_EQ(k_SELFID, sm.leaderNodeId());
    ASSERT_EQ(k_INVALID_NODE, sm.tentativeLeaderNodeId());
    ASSERT_EQ(term, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_NONE, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_NONE, output.timer());
    ASSERT_EQ(k_INVALID_NODE, output.destination());
    ASSERT_EQ(false, output.cancelTimerEventsFlag());
    ASSERT_EQ(age, sm.age());
}

static void test7()
// ------------------------------------------------------------------------
// Testing:
//   * Dormant -D1-> Follower -F2-> Candidate -C4-> Leader -L3-> Follower
//   * Quorum == 3
//   * This test ensures that when an elector is started, it transitions to
//     a follower, then eventually to a candidate and then leader upon
//     receiving support from majority of the nodes.  Then goes back to
//     being a follower when loses majority support (one of the supporters
//     becomes unavailable) and emits a leadership cession event.
// ------------------------------------------------------------------------
{
    s_ignoreCheckDefAlloc = true;
    // Logging infrastructure allocates using the default allocator, and
    // that logging is beyond the control of this function.

    bmqtst::TestHelper::printTestName("TEST 7");

    const int k_SELFID             = 0;
    const int k_QUORUM             = 3;
    const int k_TOTAL_NODES        = 4;
    const int k_INVALID_NODE       = ElectorStateMachine::k_INVALID_NODE_ID;
    const int k_ALL_NODES          = ElectorStateMachine::k_ALL_NODES_ID;
    const int k_INACTIVITY_INTV_MS = 6 * bdlt::TimeUnitRatio::k_MS_PER_S;

    bsls::Types::Uint64 age = 0;
    ElectorStateMachine sm(s_allocator_p);

    ASSERT_EQ(ElectorState::e_DORMANT, sm.state());

    // Dormant -D1-> Follower
    sm.enable(k_SELFID, k_QUORUM, k_TOTAL_NODES, k_INACTIVITY_INTV_MS);
    ASSERT_EQ(ElectorState::e_FOLLOWER, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_STARTED, sm.reason());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(k_INVALID_NODE, sm.tentativeLeaderNodeId());
    ASSERT_EQ(0ULL, sm.term());
    ASSERT_EQ(++age, sm.age());

    ElectorStateMachineOutput output;
    bsls::Types::Uint64       term = 1;

    // Apply INITIAL_WAIT_TIMER to follower.  It should emit scouting request.
    sm.applyTimer(&output, ElectorTimerEventType::e_INITIAL_WAIT_TIMER);
    ASSERT_EQ(false, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_FOLLOWER, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_NONE, sm.reason());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(0ULL, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_SCOUTING_REQUEST, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_SCOUTING_RESULT_TIMER, output.timer());
    ASSERT_EQ(ElectorStateMachine::k_ALL_NODES_ID, output.destination());
    ASSERT_EQ(false, output.cancelTimerEventsFlag());
    ASSERT_EQ(age, sm.age());

    // Apply 2 scouting responses.

    // Apply 1st scouting response.
    sm.applyScout(&output,
                  true,           // Will vote
                  sm.term() + 1,  // Scouting happens with a higher term
                  k_SELFID + 1);  // Peer node Id
    ASSERT_EQ(false, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_FOLLOWER, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_NONE, sm.reason());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(0ULL, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_NONE, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_NONE, output.timer());
    ASSERT_EQ(k_INVALID_NODE, output.destination());
    ASSERT_EQ(false, output.cancelTimerEventsFlag());
    ASSERT_EQ(age, sm.age());

    // Apply 2nd scouting response.  Quorum will be achieved, and elector will
    // transition to candidate.
    // Follower -F2-> Candidate
    sm.applyScout(&output,
                  true,           // Will vote
                  sm.term() + 1,  // Scouting happens with a higher term
                  k_SELFID + 2);  // Peer node Id
    ASSERT_EQ(true, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_CANDIDATE, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_NONE, sm.reason());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(term, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_ELECTION_PROPOSAL, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_ELECTION_RESULT_TIMER, output.timer());
    ASSERT_EQ(ElectorStateMachine::k_ALL_NODES_ID, output.destination());
    ASSERT_EQ(true, output.cancelTimerEventsFlag());
    ASSERT_EQ(++age, sm.age());

    // Apply 2 election responses

    // 1st election response
    sm.applyIOEvent(&output,
                    ElectorIOEventType::e_ELECTION_RESPONSE,
                    term,
                    k_SELFID + 1);  // nodeId

    ASSERT_EQ(false, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_CANDIDATE, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_NONE, sm.reason());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(term, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_NONE, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_NONE, output.timer());
    ASSERT_EQ(k_INVALID_NODE, output.destination());
    ASSERT_EQ(false, output.cancelTimerEventsFlag());
    ASSERT_EQ(age, sm.age());

    // 2nd election response
    // Candidate -C4-> Leader
    sm.applyIOEvent(&output,
                    ElectorIOEventType::e_ELECTION_RESPONSE,
                    term,           // term
                    k_SELFID + 2);  // nodeId

    ASSERT_EQ(true, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_LEADER, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_NONE, sm.reason());
    ASSERT_EQ(k_SELFID, sm.leaderNodeId());
    ASSERT_EQ(term, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_LEADER_HEARTBEAT, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_HEARTBEAT_BROADCAST_TIMER,
              output.timer());
    ASSERT_EQ(k_ALL_NODES, output.destination());
    ASSERT_EQ(true, output.cancelTimerEventsFlag());
    ASSERT_EQ(++age, sm.age());

    // One of the supporting node becomes unavailable.  Leader loses majority
    // and goes back to being a follower.
    // Leader -L3-> Follower
    sm.applyAvailability(&output,
                         ElectorIOEventType::e_NODE_UNAVAILABLE,
                         k_SELFID + 2);  // A supporting nodeId

    ASSERT_EQ(true, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_FOLLOWER, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_QUORUM_NOT_ACHIEVED, sm.reason());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(k_INVALID_NODE, sm.tentativeLeaderNodeId());
    ASSERT_EQ(term, sm.term());

    ASSERT_EQ(ElectorIOEventType::e_LEADERSHIP_CESSION, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_RANDOM_WAIT_TIMER, output.timer());

    ASSERT_EQ(k_ALL_NODES, output.destination());
    ASSERT_EQ(true, output.cancelTimerEventsFlag());
    ASSERT_EQ(++age, sm.age());
}

static void test8()
// ------------------------------------------------------------------------
// Testing:
//   * Dormant -D1-> Follower -F2-> Candidate -C4-> Leader -L4->
//     Follower upon receiving stale heartbeat response.
//   * Quorum == 3
//   * This test ensures that when an elector is started, it transitions to
//     a follower, then eventually to a candidate and then leader upon
//     receiving support from majority of the nodes.  Then goes back to
//     being a follower when leader receives a stale heartbeat response
//     from a follower and emits a leadership cession event.
// ------------------------------------------------------------------------
{
    s_ignoreCheckDefAlloc = true;
    // Logging infrastructure allocates using the default allocator, and
    // that logging is beyond the control of this function.

    bmqtst::TestHelper::printTestName("TEST 8");

    const int k_SELFID             = 0;
    const int k_QUORUM             = 3;
    const int k_TOTAL_NODES        = 4;
    const int k_INVALID_NODE       = ElectorStateMachine::k_INVALID_NODE_ID;
    const int k_ALL_NODES          = ElectorStateMachine::k_ALL_NODES_ID;
    const int k_INACTIVITY_INTV_MS = 6 * bdlt::TimeUnitRatio::k_MS_PER_S;

    bsls::Types::Uint64 age = 0;
    ElectorStateMachine sm(s_allocator_p);

    ASSERT_EQ(ElectorState::e_DORMANT, sm.state());

    // Dormant -D1-> Follower
    sm.enable(k_SELFID, k_QUORUM, k_TOTAL_NODES, k_INACTIVITY_INTV_MS);
    ASSERT_EQ(ElectorState::e_FOLLOWER, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_STARTED, sm.reason());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(k_INVALID_NODE, sm.tentativeLeaderNodeId());
    ASSERT_EQ(0ULL, sm.term());
    ASSERT_EQ(++age, sm.age());

    ElectorStateMachineOutput output;
    bsls::Types::Uint64       term = 1;

    // Apply INITIAL_WAIT_TIMER to follower.  It should emit scouting request.
    sm.applyTimer(&output, ElectorTimerEventType::e_INITIAL_WAIT_TIMER);
    ASSERT_EQ(false, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_FOLLOWER, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_NONE, sm.reason());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(0ULL, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_SCOUTING_REQUEST, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_SCOUTING_RESULT_TIMER, output.timer());
    ASSERT_EQ(ElectorStateMachine::k_ALL_NODES_ID, output.destination());
    ASSERT_EQ(false, output.cancelTimerEventsFlag());
    ASSERT_EQ(age, sm.age());

    // Apply 2 scouting responses.

    // Apply 1st scouting response.
    sm.applyScout(&output,
                  true,           // Will vote
                  sm.term() + 1,  // Scouting happens with a higher term
                  k_SELFID + 1);  // Peer node Id
    ASSERT_EQ(false, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_FOLLOWER, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_NONE, sm.reason());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(0ULL, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_NONE, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_NONE, output.timer());
    ASSERT_EQ(k_INVALID_NODE, output.destination());
    ASSERT_EQ(false, output.cancelTimerEventsFlag());
    ASSERT_EQ(age, sm.age());

    // Apply 2nd scouting response.  Quorum will be achieved, and elector will
    // transition to candidate.
    // Follower -F2-> Candidate
    sm.applyScout(&output,
                  true,           // Will vote
                  sm.term() + 1,  // Scouting happens with a higher term
                  k_SELFID + 2);  // Peer node Id
    ASSERT_EQ(true, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_CANDIDATE, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_NONE, sm.reason());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(term, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_ELECTION_PROPOSAL, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_ELECTION_RESULT_TIMER, output.timer());
    ASSERT_EQ(ElectorStateMachine::k_ALL_NODES_ID, output.destination());
    ASSERT_EQ(true, output.cancelTimerEventsFlag());
    ASSERT_EQ(++age, sm.age());

    // Apply 2 election responses

    // 1st election response
    sm.applyIOEvent(&output,
                    ElectorIOEventType::e_ELECTION_RESPONSE,
                    term,
                    k_SELFID + 1);  // nodeId

    ASSERT_EQ(false, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_CANDIDATE, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_NONE, sm.reason());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(term, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_NONE, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_NONE, output.timer());
    ASSERT_EQ(k_INVALID_NODE, output.destination());
    ASSERT_EQ(false, output.cancelTimerEventsFlag());
    ASSERT_EQ(age, sm.age());

    // 2nd election response
    // Candidate -C4-> Leader
    sm.applyIOEvent(&output,
                    ElectorIOEventType::e_ELECTION_RESPONSE,
                    term,           // term
                    k_SELFID + 2);  // nodeId

    ASSERT_EQ(true, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_LEADER, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_NONE, sm.reason());
    ASSERT_EQ(k_SELFID, sm.leaderNodeId());
    ASSERT_EQ(term, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_LEADER_HEARTBEAT, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_HEARTBEAT_BROADCAST_TIMER,
              output.timer());
    ASSERT_EQ(k_ALL_NODES, output.destination());
    ASSERT_EQ(true, output.cancelTimerEventsFlag());
    ASSERT_EQ(++age, sm.age());

    // Leader receives a stale heartbeat response from a follower, and reverts
    // back to being a follower.
    // Leader -L4-> Follower
    sm.applyIOEvent(&output,
                    ElectorIOEventType::e_HEARTBEAT_RESPONSE,
                    ++term,  // Stale heartbeat response having higher term
                    k_SELFID + 2);  // NodeId

    ASSERT_EQ(true, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_FOLLOWER, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_LEADER_PREEMPTED, sm.reason());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(k_INVALID_NODE, sm.tentativeLeaderNodeId());
    ASSERT_EQ(term, sm.term());

    ASSERT_EQ(ElectorIOEventType::e_LEADERSHIP_CESSION, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_RANDOM_WAIT_TIMER, output.timer());

    ASSERT_EQ(k_ALL_NODES, output.destination());
    ASSERT_EQ(true, output.cancelTimerEventsFlag());
    ASSERT_EQ(++age, sm.age());
}

static void test9()
// ------------------------------------------------------------------------
// Testing:
//   * Quorum == 1 (simulating one node in the cluster)
//   * Dormant -D1-> Follower -F2-> Candidate -C5-> Leader
//   * This test ensures that with a quorum of 1, when an elector is
//     started, it transitions to follower, then candidate, and then a
//     leader.
// ------------------------------------------------------------------------
{
    s_ignoreCheckDefAlloc = true;
    // Logging infrastructure allocates using the default allocator, and
    // that logging is beyond the control of this function.

    bmqtst::TestHelper::printTestName("TEST 9");

    const int k_SELFID             = 0;
    const int k_QUORUM             = 1;
    const int k_TOTAL_NODES        = 4;
    const int k_INVALID_NODE       = ElectorStateMachine::k_INVALID_NODE_ID;
    const int k_ALL_NODES          = ElectorStateMachine::k_ALL_NODES_ID;
    const int k_INACTIVITY_INTV_MS = 6 * bdlt::TimeUnitRatio::k_MS_PER_S;

    bsls::Types::Uint64 age = 0;
    ElectorStateMachine sm(s_allocator_p);

    ASSERT_EQ(ElectorState::e_DORMANT, sm.state());

    // Dormant -D1-> Follower
    sm.enable(k_SELFID, k_QUORUM, k_TOTAL_NODES, k_INACTIVITY_INTV_MS);
    ASSERT_EQ(ElectorState::e_FOLLOWER, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_STARTED, sm.reason());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(k_INVALID_NODE, sm.tentativeLeaderNodeId());
    ASSERT_EQ(0ULL, sm.term());
    ASSERT_EQ(++age, sm.age());

    ElectorStateMachineOutput output;
    bsls::Types::Uint64       term = 1;

    // Apply INITIAL_WAIT_TIMER to follower.  Note that since quorum is 1,
    // instead of emitting a scouting request, elector will emit election
    // proposal and transition to candidate.
    // Follower -F2-> Candidate
    sm.applyTimer(&output, ElectorTimerEventType::e_INITIAL_WAIT_TIMER);
    ASSERT_EQ(true, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_CANDIDATE, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_NONE, sm.reason());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(k_INVALID_NODE, sm.tentativeLeaderNodeId());
    ASSERT_EQ(term, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_ELECTION_PROPOSAL, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_ELECTION_RESULT_TIMER, output.timer());
    ASSERT_EQ(k_ALL_NODES, output.destination());
    ASSERT_EQ(true, output.cancelTimerEventsFlag());
    ASSERT_EQ(++age, sm.age());

    // After election result timer fires, elector will see that it has quorum
    // and will transition to leader.
    // Candidate -C5-> Leader
    sm.applyTimer(&output, ElectorTimerEventType::e_ELECTION_RESULT_TIMER);

    ASSERT_EQ(true, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_LEADER, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_NONE, sm.reason());
    ASSERT_EQ(k_SELFID, sm.leaderNodeId());
    ASSERT_EQ(term, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_LEADER_HEARTBEAT, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_HEARTBEAT_BROADCAST_TIMER,
              output.timer());
    ASSERT_EQ(k_ALL_NODES, output.destination());
    ASSERT_EQ(true, output.cancelTimerEventsFlag());
    ASSERT_EQ(++age, sm.age());
}

static void test10()
// ------------------------------------------------------------------------
// Testing:
//   Dormant -D1-> Follower -F2-> Candidate -C5-> Leader
//   with quorum == 1 (simulating one node in the cluster), with the
//   additional logic of another node sending election response.  This test
//   case recreates the scenario when we set the quorum of a node in a
//   cluster to 1 in order to force it to elect itself.  This node still
//   ends up getting election response from its peers.
// ------------------------------------------------------------------------
{
    s_ignoreCheckDefAlloc = true;
    // Logging infrastructure allocates using the default allocator, and
    // that logging is beyond the control of this function.

    bmqtst::TestHelper::printTestName("TEST 10");

    const int k_SELFID             = 0;
    const int k_QUORUM             = 1;
    const int k_TOTAL_NODES        = 4;
    const int k_INVALID_NODE       = ElectorStateMachine::k_INVALID_NODE_ID;
    const int k_ALL_NODES          = ElectorStateMachine::k_ALL_NODES_ID;
    const int k_INACTIVITY_INTV_MS = 6 * bdlt::TimeUnitRatio::k_MS_PER_S;

    bsls::Types::Uint64 age = 0;
    ElectorStateMachine sm(s_allocator_p);

    ASSERT_EQ(ElectorState::e_DORMANT, sm.state());

    // Dormant -D1-> Follower
    sm.enable(k_SELFID, k_QUORUM, k_TOTAL_NODES, k_INACTIVITY_INTV_MS);
    ASSERT_EQ(ElectorState::e_FOLLOWER, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_STARTED, sm.reason());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(k_INVALID_NODE, sm.tentativeLeaderNodeId());
    ASSERT_EQ(0ULL, sm.term());
    ASSERT_EQ(++age, sm.age());

    ElectorStateMachineOutput output;
    bsls::Types::Uint64       term = 1;

    // Apply INITIAL_WAIT_TIMER to follower.  Note that since quorum is 1,
    // instead of emitting a scouting request, elector will emit election
    // proposal and transition to candidate.
    // Follower -F2-> Candidate
    sm.applyTimer(&output, ElectorTimerEventType::e_INITIAL_WAIT_TIMER);
    ASSERT_EQ(true, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_CANDIDATE, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_NONE, sm.reason());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(k_INVALID_NODE, sm.tentativeLeaderNodeId());
    ASSERT_EQ(term, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_ELECTION_PROPOSAL, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_ELECTION_RESULT_TIMER, output.timer());
    ASSERT_EQ(k_ALL_NODES, output.destination());
    ASSERT_EQ(true, output.cancelTimerEventsFlag());
    ASSERT_EQ(++age, sm.age());

    // 1st election response
    sm.applyIOEvent(&output,
                    ElectorIOEventType::e_ELECTION_RESPONSE,
                    term,
                    1);  // nodeId
    ASSERT_EQ(false, output.stateChangedFlag());

    // 2nd election response
    sm.applyIOEvent(&output,
                    ElectorIOEventType::e_ELECTION_RESPONSE,
                    term,
                    2);  // nodeId
    ASSERT_EQ(false, output.stateChangedFlag());

    // Candidate -C5-> Leader
    sm.applyTimer(&output, ElectorTimerEventType::e_ELECTION_RESULT_TIMER);

    ASSERT_EQ(true, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_LEADER, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_NONE, sm.reason());
    ASSERT_EQ(k_SELFID, sm.leaderNodeId());
    ASSERT_EQ(term, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_LEADER_HEARTBEAT, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_HEARTBEAT_BROADCAST_TIMER,
              output.timer());
    ASSERT_EQ(k_ALL_NODES, output.destination());
    ASSERT_EQ(true, output.cancelTimerEventsFlag());
    ASSERT_EQ(++age, sm.age());
}

static void test11()
// ------------------------------------------------------------------------
// Testing:
//   * Dormant -> Follower -> Follower with leader -> Leader inactivity ->
//     Follower without leader
//   * Quorum == 3
// ------------------------------------------------------------------------
{
    s_ignoreCheckDefAlloc = true;
    // Logging infrastructure allocates using the default allocator, and
    // that logging is beyond the control of this function.

    bmqtst::TestHelper::printTestName("TEST 11");

    const int k_SELFID             = 0;
    const int k_QUORUM             = 3;
    const int k_TOTAL_NODES        = 4;
    const int k_INACTIVITY_INTV_MS = 20 * bdlt::TimeUnitRatio::k_MS_PER_S;
    const int k_INVALID_NODE       = ElectorStateMachine::k_INVALID_NODE_ID;

    const bsls::Types::Int64 k_NS_PER_S = bdlt::TimeUnitRatio::k_NS_PER_S;

    // Reset the clock
    s_electorClock->reset();

    bsls::Types::Uint64 age = 0;
    ElectorStateMachine sm(s_allocator_p);

    ASSERT_EQ(ElectorState::e_DORMANT, sm.state());

    // Dormant -D1-> Follower
    sm.enable(k_SELFID, k_QUORUM, k_TOTAL_NODES, k_INACTIVITY_INTV_MS);
    ASSERT_EQ(ElectorState::e_FOLLOWER, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_STARTED, sm.reason());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(k_INVALID_NODE, sm.tentativeLeaderNodeId());
    ASSERT_EQ(0ULL, sm.term());
    ASSERT_EQ(++age, sm.age());

    ElectorStateMachineOutput output;
    bsls::Types::Uint64       term = 1;

    // Follower gets election proposal
    sm.applyIOEvent(&output,
                    ElectorIOEventType::e_ELECTION_PROPOSAL,
                    term,  // new term > existing 'd_term'
                    3);    // leader node id

    ASSERT_EQ(false, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_FOLLOWER, sm.state());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(3, sm.tentativeLeaderNodeId());  // new tentative leader
    ASSERT_EQ(term, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_ELECTION_RESPONSE, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_HEARTBEAT_CHECK_TIMER, output.timer());
    ASSERT_EQ(3, output.destination());
    ASSERT_EQ(true, output.cancelTimerEventsFlag());
    ASSERT_EQ(age, sm.age());

    s_electorClock->advanceHighResTimer(10 * k_NS_PER_S);

    // Follower gets first heartbeat from leader
    sm.applyIOEvent(&output,
                    ElectorIOEventType::e_LEADER_HEARTBEAT,
                    term,  // new term > existing 'd_term'
                    3);    // leader node id

    ASSERT_EQ(true, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_FOLLOWER, sm.state());
    ASSERT_EQ(3, sm.leaderNodeId());  // new leader
    ASSERT_EQ(k_INVALID_NODE, sm.tentativeLeaderNodeId());
    ASSERT_EQ(term, sm.term());
    ASSERT_EQ(ElectorTransitionReason::e_NONE, sm.reason());
    ASSERT_EQ(ElectorIOEventType::e_NONE, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_HEARTBEAT_CHECK_TIMER, output.timer());
    ASSERT_EQ(k_INVALID_NODE, output.destination());
    ASSERT_EQ(true, output.cancelTimerEventsFlag());
    ASSERT_EQ(++age, sm.age());

    // 1 second elapses
    s_electorClock->advanceHighResTimer(1 * k_NS_PER_S);

    // Heart beat check time fires
    sm.applyTimer(&output, ElectorTimerEventType::e_HEARTBEAT_CHECK_TIMER);
    ASSERT_EQ(false, output.stateChangedFlag());  // No state change
    ASSERT_EQ(3, sm.leaderNodeId());
    ASSERT_EQ(k_INVALID_NODE, sm.tentativeLeaderNodeId());
    ASSERT_EQ(term, sm.term());
    ASSERT_EQ(ElectorTransitionReason::e_NONE, sm.reason());
    ASSERT_EQ(ElectorIOEventType::e_NONE, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_NONE, output.timer());
    ASSERT_EQ(k_INVALID_NODE, output.destination());
    ASSERT_EQ(false, output.cancelTimerEventsFlag());
    ASSERT_EQ(age, sm.age());

    // 2 seconds elapse
    s_electorClock->advanceHighResTimer(2 * k_NS_PER_S);

    // Heart beat check time fires
    sm.applyTimer(&output, ElectorTimerEventType::e_HEARTBEAT_CHECK_TIMER);
    ASSERT_EQ(false, output.stateChangedFlag());  // No state change
    ASSERT_EQ(3, sm.leaderNodeId());
    ASSERT_EQ(k_INVALID_NODE, sm.tentativeLeaderNodeId());
    ASSERT_EQ(term, sm.term());
    ASSERT_EQ(ElectorTransitionReason::e_NONE, sm.reason());
    ASSERT_EQ(ElectorIOEventType::e_NONE, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_NONE, output.timer());
    ASSERT_EQ(k_INVALID_NODE, output.destination());
    ASSERT_EQ(false, output.cancelTimerEventsFlag());
    ASSERT_EQ(age, sm.age());

    // 2.5 seconds elapse
    s_electorClock->advanceHighResTimer(2 * k_NS_PER_S + (k_NS_PER_S / 2));

    // Heart beat check time fires
    sm.applyTimer(&output, ElectorTimerEventType::e_HEARTBEAT_CHECK_TIMER);
    ASSERT_EQ(false, output.stateChangedFlag());  // No state change
    ASSERT_EQ(3, sm.leaderNodeId());
    ASSERT_EQ(k_INVALID_NODE, sm.tentativeLeaderNodeId());
    ASSERT_EQ(term, sm.term());
    ASSERT_EQ(ElectorTransitionReason::e_NONE, sm.reason());
    ASSERT_EQ(ElectorIOEventType::e_NONE, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_NONE, output.timer());
    ASSERT_EQ(k_INVALID_NODE, output.destination());
    ASSERT_EQ(false, output.cancelTimerEventsFlag());
    ASSERT_EQ(age, sm.age());

    // 0.1 seconds elapse
    s_electorClock->advanceHighResTimer(k_NS_PER_S / 10);

    // Leader heartbeat arrives
    sm.applyIOEvent(&output,
                    ElectorIOEventType::e_LEADER_HEARTBEAT,
                    term,
                    3);                           // leader node Id
    ASSERT_EQ(false, output.stateChangedFlag());  // No state change
    ASSERT_EQ(3, sm.leaderNodeId());
    ASSERT_EQ(k_INVALID_NODE, sm.tentativeLeaderNodeId());
    ASSERT_EQ(term, sm.term());
    ASSERT_EQ(ElectorTransitionReason::e_NONE, sm.reason());
    ASSERT_EQ(ElectorIOEventType::e_NONE, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_NONE, output.timer());
    ASSERT_EQ(k_INVALID_NODE, output.destination());
    ASSERT_EQ(false, output.cancelTimerEventsFlag());
    ASSERT_EQ(age, sm.age());

    // Time elapses by more than leader inactivity period
    s_electorClock->advanceHighResTimer(
        (k_INACTIVITY_INTV_MS + bdlt::TimeUnitRatio::k_MS_PER_S) *
        bdlt::TimeUnitRatio::k_NS_PER_MS);

    // Hearbeat timer fires
    sm.applyTimer(&output, ElectorTimerEventType::e_HEARTBEAT_CHECK_TIMER);
    ASSERT_EQ(true, output.stateChangedFlag());  // state change
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(k_INVALID_NODE, sm.tentativeLeaderNodeId());
    ASSERT_EQ(term, sm.term());
    ASSERT_EQ(ElectorTransitionReason::e_LEADER_NO_HEARTBEAT, sm.reason());
    ASSERT_EQ(ElectorIOEventType::e_NONE, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_RANDOM_WAIT_TIMER, output.timer());
    ASSERT_EQ(k_INVALID_NODE, output.destination());
    ASSERT_EQ(true, output.cancelTimerEventsFlag());
    ASSERT_EQ(++age, sm.age());
}

static void test12()
// ------------------------------------------------------------------------
// Testing:
//   Dormant -> Follower -> Follower with leader (after receiving
//   heartbeat from the leader)
//
// This case tests the scenario when a node comes up, starts the
// elector to become a follower, and then receives heartbeat from a
// leader.  The node must send out voting support event to the leader.
// ------------------------------------------------------------------------
{
    s_ignoreCheckDefAlloc = true;
    // Logging infrastructure allocates using the default allocator, and
    // that logging is beyond the control of this function.

    bmqtst::TestHelper::printTestName("TEST 12");

    const int k_SELFID             = 0;
    const int k_QUORUM             = 4;
    const int k_TOTAL_NODES        = 4;
    const int k_INACTIVITY_INTV_MS = 6 * bdlt::TimeUnitRatio::k_MS_PER_S;
    const int k_INVALID_NODE       = ElectorStateMachine::k_INVALID_NODE_ID;

    bsls::Types::Uint64 age = 0;
    ElectorStateMachine sm(s_allocator_p);

    ASSERT_EQ(ElectorState::e_DORMANT, sm.state());

    // Dormant -D1-> Follower
    sm.enable(k_SELFID, k_QUORUM, k_TOTAL_NODES, k_INACTIVITY_INTV_MS);
    ASSERT_EQ(ElectorState::e_FOLLOWER, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_STARTED, sm.reason());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(k_INVALID_NODE, sm.tentativeLeaderNodeId());
    ASSERT_EQ(0ULL, sm.term());
    ASSERT_EQ(++age, sm.age());

    ElectorStateMachineOutput output;
    bsls::Types::Uint64       leaderTerm   = 10;
    int                       leaderNodeId = 123;

    // Leader's heartbeat arrives
    sm.applyIOEvent(&output,
                    ElectorIOEventType::e_LEADER_HEARTBEAT,
                    leaderTerm,
                    leaderNodeId);

    ASSERT_EQ(true, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_FOLLOWER, sm.state());
    ASSERT_EQ(leaderNodeId, sm.leaderNodeId());
    ASSERT_EQ(k_INVALID_NODE, sm.tentativeLeaderNodeId());
    ASSERT_EQ(leaderTerm, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_ELECTION_RESPONSE, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_HEARTBEAT_CHECK_TIMER, output.timer());
    ASSERT_EQ(leaderNodeId, output.destination());
    ASSERT_EQ(true, output.cancelTimerEventsFlag());
    ASSERT_EQ(++age, sm.age());
}

static void test13()
// ------------------------------------------------------------------------
// Testing:
//   Dormant -> Follower -> Follower with leader (after receiving
//   heartbeat from the leader) -> Emit stale heartbeat response upon
//   receiving stale leader hearbeat.
//
// ------------------------------------------------------------------------
{
    s_ignoreCheckDefAlloc = true;
    // Logging infrastructure allocates using the default allocator, and
    // that logging is beyond the control of this function.

    bmqtst::TestHelper::printTestName("TEST 13");

    const int k_SELFID             = 0;
    const int k_QUORUM             = 3;
    const int k_TOTAL_NODES        = 4;
    const int k_INACTIVITY_INTV_MS = 6 * bdlt::TimeUnitRatio::k_MS_PER_S;
    const int k_INVALID_NODE       = ElectorStateMachine::k_INVALID_NODE_ID;

    bsls::Types::Uint64 age = 0;
    ElectorStateMachine sm(s_allocator_p);

    ASSERT_EQ(ElectorState::e_DORMANT, sm.state());

    // Dormant -D1-> Follower
    sm.enable(k_SELFID, k_QUORUM, k_TOTAL_NODES, k_INACTIVITY_INTV_MS);
    ASSERT_EQ(ElectorState::e_FOLLOWER, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_STARTED, sm.reason());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(k_INVALID_NODE, sm.tentativeLeaderNodeId());
    ASSERT_EQ(0ULL, sm.term());
    ASSERT_EQ(++age, sm.age());

    ElectorStateMachineOutput output;
    bsls::Types::Uint64       leaderTerm   = 10;
    int                       leaderNodeId = 123;

    sm.applyIOEvent(&output,
                    ElectorIOEventType::e_LEADER_HEARTBEAT,
                    leaderTerm,
                    leaderNodeId);

    ASSERT_EQ(true, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_FOLLOWER, sm.state());
    ASSERT_EQ(leaderNodeId, sm.leaderNodeId());
    ASSERT_EQ(k_INVALID_NODE, sm.tentativeLeaderNodeId());
    ASSERT_EQ(leaderTerm, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_ELECTION_RESPONSE, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_HEARTBEAT_CHECK_TIMER, output.timer());
    ASSERT_EQ(leaderNodeId, output.destination());
    ASSERT_EQ(true, output.cancelTimerEventsFlag());
    ASSERT_EQ(++age, sm.age());

    // Stale leader's heartbeat arrives.
    sm.applyIOEvent(&output,
                    ElectorIOEventType::e_LEADER_HEARTBEAT,
                    leaderTerm - 1,  // Stale term
                    100);            // Stale leader's nodeId

    ASSERT_EQ(false, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_FOLLOWER, sm.state());
    ASSERT_EQ(leaderNodeId, sm.leaderNodeId());
    ASSERT_EQ(k_INVALID_NODE, sm.tentativeLeaderNodeId());
    ASSERT_EQ(leaderTerm, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_HEARTBEAT_RESPONSE, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_NONE, output.timer());
    ASSERT_EQ(100, output.destination());
    ASSERT_EQ(false, output.cancelTimerEventsFlag());
    ASSERT_EQ(age, sm.age());
}

static void test14()
// ------------------------------------------------------------------------
// Testing:
//   Dormant -> Follower -> Follower with leader (after receiving
//   heartbeat from the leader) -> Receive election proposals with lower,
//   same, and higher terms -> Ensure it's a no-op in all 3 cases.
//
//   Note that applying election proposal with higher term and ensuring
//   that follower continues to follow the existing leader tests the notion
//   of sticky leader.
//
// ------------------------------------------------------------------------
{
    s_ignoreCheckDefAlloc = true;
    // Logging infrastructure allocates using the default allocator, and
    // that logging is beyond the control of this function.

    bmqtst::TestHelper::printTestName("TEST 14");

    const int k_SELFID             = 0;
    const int k_QUORUM             = 3;
    const int k_TOTAL_NODES        = 4;
    const int k_INACTIVITY_INTV_MS = 6 * bdlt::TimeUnitRatio::k_MS_PER_S;
    const int k_INVALID_NODE       = ElectorStateMachine::k_INVALID_NODE_ID;

    bsls::Types::Uint64 age = 0;
    ElectorStateMachine sm(s_allocator_p);

    ASSERT_EQ(ElectorState::e_DORMANT, sm.state());

    // Dormant -D1-> Follower
    sm.enable(k_SELFID, k_QUORUM, k_TOTAL_NODES, k_INACTIVITY_INTV_MS);
    ASSERT_EQ(ElectorState::e_FOLLOWER, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_STARTED, sm.reason());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(k_INVALID_NODE, sm.tentativeLeaderNodeId());
    ASSERT_EQ(0ULL, sm.term());
    ASSERT_EQ(++age, sm.age());

    // Apply leaader heartbeat event with a higher term.  Follower should start
    // following the leader node.
    ElectorStateMachineOutput output;
    bsls::Types::Uint64       leaderTerm   = 10;
    int                       leaderNodeId = 123;

    sm.applyIOEvent(&output,
                    ElectorIOEventType::e_LEADER_HEARTBEAT,
                    leaderTerm,
                    leaderNodeId);

    ASSERT_EQ(true, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_FOLLOWER, sm.state());
    ASSERT_EQ(leaderNodeId, sm.leaderNodeId());
    ASSERT_EQ(k_INVALID_NODE, sm.tentativeLeaderNodeId());
    ASSERT_EQ(leaderTerm, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_ELECTION_RESPONSE, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_HEARTBEAT_CHECK_TIMER, output.timer());
    ASSERT_EQ(leaderNodeId, output.destination());
    ASSERT_EQ(true, output.cancelTimerEventsFlag());
    ASSERT_EQ(++age, sm.age());

    // Apply an election proposal from different node, with lower term.
    // Follower should ignore this.
    sm.applyIOEvent(&output,
                    ElectorIOEventType::e_ELECTION_PROPOSAL,
                    sm.term() - 1,      // Lower term
                    leaderNodeId + 3);  // Different from current leader

    ASSERT_EQ(false, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_FOLLOWER, sm.state());
    ASSERT_EQ(leaderNodeId, sm.leaderNodeId());
    ASSERT_EQ(k_INVALID_NODE, sm.tentativeLeaderNodeId());
    ASSERT_EQ(leaderTerm, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_NONE, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_NONE, output.timer());
    ASSERT_EQ(k_INVALID_NODE, output.destination());
    ASSERT_EQ(false, output.cancelTimerEventsFlag());
    ASSERT_EQ(age, sm.age());

    // Apply an election proposal from different node, with same term.
    // Follower should ignore this.
    sm.applyIOEvent(&output,
                    ElectorIOEventType::e_ELECTION_PROPOSAL,
                    sm.term(),          // Same term
                    leaderNodeId + 3);  // Different from current leader

    ASSERT_EQ(false, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_FOLLOWER, sm.state());
    ASSERT_EQ(leaderNodeId, sm.leaderNodeId());
    ASSERT_EQ(k_INVALID_NODE, sm.tentativeLeaderNodeId());
    ASSERT_EQ(leaderTerm, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_NONE, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_NONE, output.timer());
    ASSERT_EQ(k_INVALID_NODE, output.destination());
    ASSERT_EQ(false, output.cancelTimerEventsFlag());
    ASSERT_EQ(age, sm.age());

    // Apply an election proposal from different node, with higher term.
    // Follower should ignore this.
    sm.applyIOEvent(&output,
                    ElectorIOEventType::e_ELECTION_PROPOSAL,
                    sm.term() + 2,      // Higher term
                    leaderNodeId + 3);  // Different from current leader

    ASSERT_EQ(false, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_FOLLOWER, sm.state());
    ASSERT_EQ(leaderNodeId, sm.leaderNodeId());
    ASSERT_EQ(k_INVALID_NODE, sm.tentativeLeaderNodeId());
    ASSERT_EQ(leaderTerm, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_NONE, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_NONE, output.timer());
    ASSERT_EQ(k_INVALID_NODE, output.destination());
    ASSERT_EQ(false, output.cancelTimerEventsFlag());
    ASSERT_EQ(age, sm.age());
}

static void test15()
// ------------------------------------------------------------------------
// Testing:
//   Dormant -> Follower -> Follower sends scouting request -> Receives
//   scouting response from only 1 node before scouting result timer fires
//   -> Follower emits random wait timer etc.
//
// ------------------------------------------------------------------------
{
    s_ignoreCheckDefAlloc = true;
    // Logging infrastructure allocates using the default allocator, and
    // that logging is beyond the control of this function.

    bmqtst::TestHelper::printTestName("TEST 15");

    const int k_SELFID             = 0;
    const int k_QUORUM             = 3;
    const int k_TOTAL_NODES        = 4;
    const int k_INACTIVITY_INTV_MS = 6 * bdlt::TimeUnitRatio::k_MS_PER_S;
    const int k_INVALID_NODE       = ElectorStateMachine::k_INVALID_NODE_ID;

    bsls::Types::Uint64       age = 0;
    ElectorStateMachine       sm(s_allocator_p);
    ElectorStateMachineOutput output;
    bsls::Types::Uint64       term = 0;

    ASSERT_EQ(ElectorState::e_DORMANT, sm.state());

    // Dormant -D1-> Follower
    sm.enable(k_SELFID, k_QUORUM, k_TOTAL_NODES, k_INACTIVITY_INTV_MS);
    ASSERT_EQ(ElectorState::e_FOLLOWER, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_STARTED, sm.reason());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(k_INVALID_NODE, sm.tentativeLeaderNodeId());
    ASSERT_EQ(term, sm.term());
    ASSERT_EQ(++age, sm.age());

    // Apply INITIAL_WAIT_TIMER to follower.  It should emit scouting request.
    sm.applyTimer(&output, ElectorTimerEventType::e_INITIAL_WAIT_TIMER);
    ASSERT_EQ(false, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_FOLLOWER, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_NONE, sm.reason());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(term, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_SCOUTING_REQUEST, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_SCOUTING_RESULT_TIMER, output.timer());
    ASSERT_EQ(ElectorStateMachine::k_ALL_NODES_ID, output.destination());
    ASSERT_EQ(false, output.cancelTimerEventsFlag());
    ASSERT_EQ(age, sm.age());

    // Apply 1 scouting response.
    sm.applyScout(&output,
                  true,           // Will vote
                  sm.term() + 1,  // Scouting happens with a higher term
                  k_SELFID + 1);  // Peer node Id
    ASSERT_EQ(false, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_FOLLOWER, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_NONE, sm.reason());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(term, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_NONE, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_NONE, output.timer());
    ASSERT_EQ(k_INVALID_NODE, output.destination());
    ASSERT_EQ(false, output.cancelTimerEventsFlag());
    ASSERT_EQ(age, sm.age());

    // Apply scouting result timer while elector hasn't received scouting
    // responses from majority of the nodes.  Elector should emit a random
    // wait timer and not change state.
    sm.applyTimer(&output, ElectorTimerEventType::e_SCOUTING_RESULT_TIMER);
    ASSERT_EQ(false, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_FOLLOWER, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_NONE, sm.reason());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(term, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_NONE, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_RANDOM_WAIT_TIMER, output.timer());
    ASSERT_EQ(k_INVALID_NODE, output.destination());
    ASSERT_EQ(false, output.cancelTimerEventsFlag());
    ASSERT_EQ(age, sm.age());
}

static void test16()
// ------------------------------------------------------------------------
// Testing:
//   Dormant -> Follower -> Follower sends scouting request -> Receives
//   scouting response from 1 node -> Receives election proposal from a
//   node with higher term -> Supports the election proposal -> Then
//   receives scouting response from another node -> Ignores the scouting
//   response.
//
//   This test ensures that scouting state is abandoned upon receiving an
//   election proposal with higher term.
//
// ------------------------------------------------------------------------
{
    s_ignoreCheckDefAlloc = true;
    // Logging infrastructure allocates using the default allocator, and
    // that logging is beyond the control of this function.

    bmqtst::TestHelper::printTestName("TEST 16");

    const int k_SELFID             = 0;
    const int k_QUORUM             = 3;
    const int k_TOTAL_NODES        = 4;
    const int k_INACTIVITY_INTV_MS = 6 * bdlt::TimeUnitRatio::k_MS_PER_S;
    const int k_INVALID_NODE       = ElectorStateMachine::k_INVALID_NODE_ID;

    bsls::Types::Uint64       age = 0;
    ElectorStateMachine       sm(s_allocator_p);
    ElectorStateMachineOutput output;
    bsls::Types::Uint64       term = 0;

    ASSERT_EQ(ElectorState::e_DORMANT, sm.state());

    // Dormant -D1-> Follower
    sm.enable(k_SELFID, k_QUORUM, k_TOTAL_NODES, k_INACTIVITY_INTV_MS);
    ASSERT_EQ(ElectorState::e_FOLLOWER, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_STARTED, sm.reason());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(k_INVALID_NODE, sm.tentativeLeaderNodeId());
    ASSERT_EQ(term, sm.term());
    ASSERT_EQ(++age, sm.age());

    // Apply INITIAL_WAIT_TIMER to follower.  It should emit scouting request.
    sm.applyTimer(&output, ElectorTimerEventType::e_INITIAL_WAIT_TIMER);
    ASSERT_EQ(false, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_FOLLOWER, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_NONE, sm.reason());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(term, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_SCOUTING_REQUEST, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_SCOUTING_RESULT_TIMER, output.timer());
    ASSERT_EQ(ElectorStateMachine::k_ALL_NODES_ID, output.destination());
    ASSERT_EQ(false, output.cancelTimerEventsFlag());
    ASSERT_EQ(age, sm.age());

    // Apply 1 scouting response.
    sm.applyScout(&output,
                  true,           // Will vote
                  sm.term() + 1,  // Scouting happens with a higher term
                  k_SELFID + 1);  // Peer node Id
    ASSERT_EQ(false, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_FOLLOWER, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_NONE, sm.reason());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(term, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_NONE, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_NONE, output.timer());
    ASSERT_EQ(k_INVALID_NODE, output.destination());
    ASSERT_EQ(false, output.cancelTimerEventsFlag());
    ASSERT_EQ(age, sm.age());

    // Apply election proposal from a node with higher term.  Elector should
    // remain follower and support this node (i.e., emit election response).
    ++term;
    sm.applyIOEvent(&output,
                    ElectorIOEventType::e_ELECTION_PROPOSAL,
                    term,           // Higher term
                    k_SELFID + 2);  // node id
    ASSERT_EQ(false, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_FOLLOWER, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_NONE, sm.reason());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(k_SELFID + 2, sm.tentativeLeaderNodeId());
    ASSERT_EQ(term, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_ELECTION_RESPONSE, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_HEARTBEAT_CHECK_TIMER, output.timer());
    ASSERT_EQ(k_SELFID + 2, output.destination());
    ASSERT_EQ(true, output.cancelTimerEventsFlag());
    ASSERT_EQ(age, sm.age());

    // Apply scouting response from another node.  Ensure that its a no-op.
    sm.applyScout(&output,
                  true,           // Will vote
                  sm.term() - 1,  // Original term used in scouting request
                  k_SELFID + 3);  // node Id
    ASSERT_EQ(false, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_FOLLOWER, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_NONE, sm.reason());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(k_SELFID + 2, sm.tentativeLeaderNodeId());
    ASSERT_EQ(term, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_NONE, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_NONE, output.timer());
    ASSERT_EQ(k_INVALID_NODE, output.destination());
    ASSERT_EQ(false, output.cancelTimerEventsFlag());
    ASSERT_EQ(age, sm.age());
}

static void test17()
// ------------------------------------------------------------------------
// Testing:
//   Dormant -> Follower -> Follower sends scouting request -> Receives
//   positive scouting response from 1 node -> Receives negative scouting
//   response from another node -> Does not transition to candidate or
//   propose election.
//
// ------------------------------------------------------------------------
{
    s_ignoreCheckDefAlloc = true;
    // Logging infrastructure allocates using the default allocator, and
    // that logging is beyond the control of this function.

    bmqtst::TestHelper::printTestName("TEST 17");

    const int k_SELFID             = 0;
    const int k_QUORUM             = 3;
    const int k_TOTAL_NODES        = 4;
    const int k_INACTIVITY_INTV_MS = 6 * bdlt::TimeUnitRatio::k_MS_PER_S;
    const int k_INVALID_NODE       = ElectorStateMachine::k_INVALID_NODE_ID;

    bsls::Types::Uint64       age = 0;
    ElectorStateMachine       sm(s_allocator_p);
    ElectorStateMachineOutput output;
    bsls::Types::Uint64       term = 0;

    ASSERT_EQ(ElectorState::e_DORMANT, sm.state());

    // Dormant -D1-> Follower
    sm.enable(k_SELFID, k_QUORUM, k_TOTAL_NODES, k_INACTIVITY_INTV_MS);
    ASSERT_EQ(ElectorState::e_FOLLOWER, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_STARTED, sm.reason());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(k_INVALID_NODE, sm.tentativeLeaderNodeId());
    ASSERT_EQ(term, sm.term());
    ASSERT_EQ(++age, sm.age());

    // Apply INITIAL_WAIT_TIMER to follower.  It should emit scouting request.
    sm.applyTimer(&output, ElectorTimerEventType::e_INITIAL_WAIT_TIMER);
    ASSERT_EQ(false, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_FOLLOWER, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_NONE, sm.reason());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(term, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_SCOUTING_REQUEST, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_SCOUTING_RESULT_TIMER, output.timer());
    ASSERT_EQ(ElectorStateMachine::k_ALL_NODES_ID, output.destination());
    ASSERT_EQ(false, output.cancelTimerEventsFlag());
    ASSERT_EQ(age, sm.age());

    // Apply 1st positive scouting response.
    sm.applyScout(&output,
                  true,           // Will vote
                  sm.term() + 1,  // Scouting happens with a higher term
                  k_SELFID + 1);  // Peer node Id
    ASSERT_EQ(false, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_FOLLOWER, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_NONE, sm.reason());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(term, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_NONE, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_NONE, output.timer());
    ASSERT_EQ(k_INVALID_NODE, output.destination());
    ASSERT_EQ(false, output.cancelTimerEventsFlag());
    ASSERT_EQ(age, sm.age());

    // Apply 2nd scouting response, but a negative one.  No change expected.
    sm.applyScout(&output,
                  false,          // Won't vote
                  sm.term() + 1,  // Scouting happens with a higher term
                  k_SELFID + 2);  // Peer node Id
    ASSERT_EQ(false, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_FOLLOWER, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_NONE, sm.reason());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(term, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_NONE, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_NONE, output.timer());
    ASSERT_EQ(k_INVALID_NODE, output.destination());
    ASSERT_EQ(false, output.cancelTimerEventsFlag());
    ASSERT_EQ(age, sm.age());

    // Apply 3rd scouting response, but a negative one.  Elector should not
    // transition to candidate or propose election.  It should emit random wait
    // timer though.
    sm.applyScout(&output,
                  false,          // Won't vote
                  sm.term() + 1,  // Scouting happens with a higher term
                  k_SELFID + 3);  // Peer node Id
    ASSERT_EQ(false, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_FOLLOWER, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_NONE, sm.reason());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(term, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_NONE, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_RANDOM_WAIT_TIMER, output.timer());
    ASSERT_EQ(k_INVALID_NODE, output.destination());
    ASSERT_EQ(true, output.cancelTimerEventsFlag());
    ASSERT_EQ(age, sm.age());
}

static void test18()
// ------------------------------------------------------------------------
// Concerns:
//   When a follower receives a heartbeat from a leader with the same term,
//   the follower submits to the leader.
//
// Testing:
//   Dormant -> Follower -> Follower receives heartbeat from leader ->
//   Leader becomes unavailable -> Leader becomes available and follower
//   receives heartbeat from it (follower submits to leader).
// ------------------------------------------------------------------------
{
    s_ignoreCheckDefAlloc = true;
    // Logging infrastructure allocates using the default allocator, and
    // that logging is beyond the control of this function.

    bmqtst::TestHelper::printTestName("TEST 18");

    const int k_SELFID             = 0;
    const int k_QUORUM             = 3;
    const int k_TOTAL_NODES        = 4;
    const int k_INACTIVITY_INTV_MS = 6 * bdlt::TimeUnitRatio::k_MS_PER_S;
    const int k_INVALID_NODE       = ElectorStateMachine::k_INVALID_NODE_ID;

    bsls::Types::Uint64 age = 0;
    ElectorStateMachine sm(s_allocator_p);

    ASSERT_EQ(ElectorState::e_DORMANT, sm.state());

    // Dormant -D1-> Follower
    sm.enable(k_SELFID, k_QUORUM, k_TOTAL_NODES, k_INACTIVITY_INTV_MS);
    ASSERT_EQ(ElectorState::e_FOLLOWER, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_STARTED, sm.reason());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(k_INVALID_NODE, sm.tentativeLeaderNodeId());
    ASSERT_EQ(0ULL, sm.term());
    ASSERT_EQ(++age, sm.age());

    // Follower receives heartbeat from leader.
    ElectorStateMachineOutput output;
    bsls::Types::Uint64       leaderTerm   = 1;
    int                       leaderNodeId = 123;

    sm.applyIOEvent(&output,
                    ElectorIOEventType::e_LEADER_HEARTBEAT,
                    leaderTerm,
                    leaderNodeId);

    ASSERT_EQ(true, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_FOLLOWER, sm.state());
    ASSERT_EQ(leaderNodeId, sm.leaderNodeId());
    ASSERT_EQ(k_INVALID_NODE, sm.tentativeLeaderNodeId());
    ASSERT_EQ(leaderTerm, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_ELECTION_RESPONSE, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_HEARTBEAT_CHECK_TIMER, output.timer());
    ASSERT_EQ(leaderNodeId, output.destination());
    ASSERT_EQ(true, output.cancelTimerEventsFlag());
    ASSERT_EQ(++age, sm.age());

    // Leader becomes unavailable
    sm.applyAvailability(&output,
                         ElectorIOEventType::e_NODE_UNAVAILABLE,
                         leaderNodeId);

    ASSERT_EQ(true, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_FOLLOWER, sm.state());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(k_INVALID_NODE, sm.tentativeLeaderNodeId());
    ASSERT_EQ(leaderTerm, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_NONE, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_RANDOM_WAIT_TIMER, output.timer());
    ASSERT_EQ(k_INVALID_NODE, output.destination());
    ASSERT_EQ(true, output.cancelTimerEventsFlag());
    ASSERT_EQ(++age, sm.age());

    // Leader becomes available and follower receives heartbeat from it
    // (follower submits to leader).
    sm.applyIOEvent(&output,
                    ElectorIOEventType::e_LEADER_HEARTBEAT,
                    leaderTerm,
                    leaderNodeId);

    ASSERT_EQ(true, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_FOLLOWER, sm.state());
    ASSERT_EQ(leaderNodeId, sm.leaderNodeId());
    ASSERT_EQ(k_INVALID_NODE, sm.tentativeLeaderNodeId());
    ASSERT_EQ(leaderTerm, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_ELECTION_RESPONSE, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_HEARTBEAT_CHECK_TIMER, output.timer());
    ASSERT_EQ(leaderNodeId, output.destination());
    ASSERT_EQ(true, output.cancelTimerEventsFlag());
    ASSERT_EQ(++age, sm.age());
}

static void test19()
// ------------------------------------------------------------------------
// Concerns:
//   When a follower receives a leadershipCession event from a leader,
//   the follower stops following the leader.
//
// Testing:
//   Dormant -> Follower -> Follower receives heartbeat from leader ->
//   Follower receives leadershipCession event from leader (follower stops
//   following the leader).
// ------------------------------------------------------------------------
{
    s_ignoreCheckDefAlloc = true;
    // Logging infrastructure allocates using the default allocator, and
    // that logging is beyond the control of this function.

    bmqtst::TestHelper::printTestName("TEST 19");

    const int k_SELFID             = 0;
    const int k_QUORUM             = 3;
    const int k_TOTAL_NODES        = 4;
    const int k_INACTIVITY_INTV_MS = 6 * bdlt::TimeUnitRatio::k_MS_PER_S;
    const int k_INVALID_NODE       = ElectorStateMachine::k_INVALID_NODE_ID;

    bsls::Types::Uint64 age = 0;
    ElectorStateMachine sm(s_allocator_p);

    ASSERT_EQ(ElectorState::e_DORMANT, sm.state());

    // Dormant -D1-> Follower
    sm.enable(k_SELFID, k_QUORUM, k_TOTAL_NODES, k_INACTIVITY_INTV_MS);
    ASSERT_EQ(ElectorState::e_FOLLOWER, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_STARTED, sm.reason());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(k_INVALID_NODE, sm.tentativeLeaderNodeId());
    ASSERT_EQ(0ULL, sm.term());
    ASSERT_EQ(++age, sm.age());

    // Follower receives heartbeat from leader.
    ElectorStateMachineOutput output;
    bsls::Types::Uint64       leaderTerm   = 1;
    int                       leaderNodeId = 123;

    sm.applyIOEvent(&output,
                    ElectorIOEventType::e_LEADER_HEARTBEAT,
                    leaderTerm,
                    leaderNodeId);

    ASSERT_EQ(true, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_FOLLOWER, sm.state());
    ASSERT_EQ(leaderNodeId, sm.leaderNodeId());
    ASSERT_EQ(k_INVALID_NODE, sm.tentativeLeaderNodeId());
    ASSERT_EQ(leaderTerm, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_ELECTION_RESPONSE, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_HEARTBEAT_CHECK_TIMER, output.timer());
    ASSERT_EQ(leaderNodeId, output.destination());
    ASSERT_EQ(true, output.cancelTimerEventsFlag());
    ASSERT_EQ(++age, sm.age());

    // Leader sends leadershipCession event.
    sm.applyIOEvent(&output,
                    ElectorIOEventType::e_LEADERSHIP_CESSION,
                    leaderTerm,
                    leaderNodeId);

    ASSERT_EQ(true, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_FOLLOWER, sm.state());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(k_INVALID_NODE, sm.tentativeLeaderNodeId());
    ASSERT_EQ(leaderTerm, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_NONE, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_RANDOM_WAIT_TIMER, output.timer());
    ASSERT_EQ(k_INVALID_NODE, output.destination());
    ASSERT_EQ(true, output.cancelTimerEventsFlag());
    ASSERT_EQ(++age, sm.age());
}

static void test20()
// ------------------------------------------------------------------------
// Testing:
//   * Dormant -D1-> Follower -F2-> Candidate -C4-> Leader -> Receive
//     stale leader heartbeat -> sends leader heartbeat to sender node.
//   * This case tests the scenario where a leader receives stale leader
//     heartbeat with same or smaller term as self and sends leader
//     heartbeat in return so that sender node can follow self.
// ------------------------------------------------------------------------
{
    s_ignoreCheckDefAlloc = true;
    // Logging infrastructure allocates using the default allocator, and
    // that logging is beyond the control of this function.

    bmqtst::TestHelper::printTestName("TEST 20");

    const int k_SELFID             = 0;
    const int k_QUORUM             = 3;
    const int k_TOTAL_NODES        = 4;
    const int k_INVALID_NODE       = ElectorStateMachine::k_INVALID_NODE_ID;
    const int k_ALL_NODES          = ElectorStateMachine::k_ALL_NODES_ID;
    const int k_INACTIVITY_INTV_MS = 6 * bdlt::TimeUnitRatio::k_MS_PER_S;

    bsls::Types::Uint64 age = 0;
    ElectorStateMachine sm(s_allocator_p);

    ASSERT_EQ(ElectorState::e_DORMANT, sm.state());

    // Dormant -D1-> Follower
    sm.enable(k_SELFID, k_QUORUM, k_TOTAL_NODES, k_INACTIVITY_INTV_MS);
    ASSERT_EQ(ElectorState::e_FOLLOWER, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_STARTED, sm.reason());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(k_INVALID_NODE, sm.tentativeLeaderNodeId());
    ASSERT_EQ(0ULL, sm.term());
    ASSERT_EQ(++age, sm.age());

    ElectorStateMachineOutput output;
    bsls::Types::Uint64       term = 1;

    // Apply INITIAL_WAIT_TIMER to follower.  It should emit scouting request.
    sm.applyTimer(&output, ElectorTimerEventType::e_INITIAL_WAIT_TIMER);
    ASSERT_EQ(false, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_FOLLOWER, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_NONE, sm.reason());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(0ULL, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_SCOUTING_REQUEST, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_SCOUTING_RESULT_TIMER, output.timer());
    ASSERT_EQ(ElectorStateMachine::k_ALL_NODES_ID, output.destination());
    ASSERT_EQ(false, output.cancelTimerEventsFlag());
    ASSERT_EQ(age, sm.age());

    // Apply 2 scouting responses.

    // Apply 1st scouting response.
    sm.applyScout(&output,
                  true,           // Will vote
                  sm.term() + 1,  // Scouting happens with a higher term
                  k_SELFID + 1);  // Peer node Id
    ASSERT_EQ(false, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_FOLLOWER, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_NONE, sm.reason());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(0ULL, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_NONE, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_NONE, output.timer());
    ASSERT_EQ(k_INVALID_NODE, output.destination());
    ASSERT_EQ(false, output.cancelTimerEventsFlag());
    ASSERT_EQ(age, sm.age());

    // Apply 2nd scouting response.  Quorum will be achieved, and elector will
    // transition to candidate.
    // Follower -F2-> Candidate
    sm.applyScout(&output,
                  true,           // Will vote
                  sm.term() + 1,  // Scouting happens with a higher term
                  k_SELFID + 2);  // Peer node Id
    ASSERT_EQ(true, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_CANDIDATE, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_NONE, sm.reason());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(term, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_ELECTION_PROPOSAL, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_ELECTION_RESULT_TIMER, output.timer());
    ASSERT_EQ(ElectorStateMachine::k_ALL_NODES_ID, output.destination());
    ASSERT_EQ(true, output.cancelTimerEventsFlag());
    ASSERT_EQ(++age, sm.age());

    // Apply 2 election responses

    // 1st election response
    sm.applyIOEvent(&output,
                    ElectorIOEventType::e_ELECTION_RESPONSE,
                    term,
                    k_SELFID + 1);  // nodeId

    ASSERT_EQ(false, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_CANDIDATE, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_NONE, sm.reason());
    ASSERT_EQ(k_INVALID_NODE, sm.leaderNodeId());
    ASSERT_EQ(term, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_NONE, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_NONE, output.timer());
    ASSERT_EQ(k_INVALID_NODE, output.destination());
    ASSERT_EQ(false, output.cancelTimerEventsFlag());
    ASSERT_EQ(age, sm.age());

    // 2nd election response
    // Candidate -C4-> Leader
    sm.applyIOEvent(&output,
                    ElectorIOEventType::e_ELECTION_RESPONSE,
                    term,           // term
                    k_SELFID + 2);  // nodeId

    ASSERT_EQ(true, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_LEADER, sm.state());
    ASSERT_EQ(ElectorTransitionReason::e_NONE, sm.reason());
    ASSERT_EQ(k_SELFID, sm.leaderNodeId());
    ASSERT_EQ(term, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_LEADER_HEARTBEAT, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_HEARTBEAT_BROADCAST_TIMER,
              output.timer());
    ASSERT_EQ(k_ALL_NODES, output.destination());
    ASSERT_EQ(true, output.cancelTimerEventsFlag());
    ASSERT_EQ(++age, sm.age());

    // Stale leader's heartbeat arrives.
    sm.applyIOEvent(&output,
                    ElectorIOEventType::e_LEADER_HEARTBEAT,
                    term,  // Stale term
                    100);  // Stale leader's nodeId

    ASSERT_EQ(false, output.stateChangedFlag());
    ASSERT_EQ(ElectorState::e_LEADER, sm.state());
    ASSERT_EQ(k_SELFID, sm.leaderNodeId());
    ASSERT_EQ(k_INVALID_NODE, sm.tentativeLeaderNodeId());
    ASSERT_EQ(term, sm.term());
    ASSERT_EQ(ElectorIOEventType::e_LEADER_HEARTBEAT, output.io());
    ASSERT_EQ(ElectorTimerEventType::e_NONE, output.timer());
    ASSERT_EQ(100, output.destination());
    ASSERT_EQ(false, output.cancelTimerEventsFlag());
    ASSERT_EQ(age, sm.age());
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    using namespace mqbnet;

    // Setup the test clock (once per task)
    s_electorClock = new (*s_allocator_p) bmqsys::MockTime;

    switch (_testCase) {
    case 0:
    case 20: test20(); break;
    case 19: test19(); break;
    case 18: test18(); break;
    case 17: test17(); break;
    case 16: test16(); break;
    case 15: test15(); break;
    case 14: test14(); break;
    case 13: test13(); break;
    case 12: test12(); break;
    case 11: test11(); break;
    case 10: test10(); break;
    case 9: test9(); break;
    case 8: test8(); break;
    case 7: test7(); break;
    case 6: test6(); break;
    case 5: test5(); break;
    case 4: test4(); break;
    case 3: test3(); break;
    case 2: test2(); break;
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        s_testStatus = -1;
    } break;
    }

    s_allocator_p->deallocate(s_electorClock);

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
