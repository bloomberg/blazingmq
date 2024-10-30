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

// mqbnet_elector.cpp                                                 -*-C++-*-
#include <mqbcmd_messages.h>
#include <mqbnet_elector.h>
#include <mqbscm_version.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_schemaeventbuilder.h>

#include <bmqio_status.h>
#include <bmqsys_threadutil.h>
#include <bmqsys_time.h>

// BDE
#include <bdlb_print.h>
#include <bdlb_string.h>
#include <bdlb_stringrefutil.h>
#include <bdlf_bind.h>
#include <bdlt_timeunitratio.h>
#include <bsl_algorithm.h>
#include <bsl_cstddef.h>  // for NULL
#include <bsl_cstdlib.h>  // for bsl::rand()
#include <bsl_iostream.h>
#include <bsl_list.h>
#include <bsl_utility.h>
#include <bslim_printer.h>
#include <bslmt_mutexassert.h>
#include <bsls_annotation.h>
#include <bsls_systemclocktype.h>
#include <bsls_timeinterval.h>

namespace BloombergLP {
namespace mqbnet {

namespace {

/// Return the ElectorIOEventType corresponding to the specified elector
/// schema `message`.
ElectorIOEventType::Enum
electorSchemaMsgToIOEventType(const bmqp_ctrlmsg::ElectorMessage& message)
{
    using namespace bmqp_ctrlmsg;

    switch (message.choice().selectionId()) {
    case ElectorMessageChoice::SELECTION_ID_ELECTION_PROPOSAL:
        return ElectorIOEventType::e_ELECTION_PROPOSAL;  // RETURN

    case ElectorMessageChoice::SELECTION_ID_ELECTION_RESPONSE:
        return ElectorIOEventType::e_ELECTION_RESPONSE;  // RETURN

    case ElectorMessageChoice::SELECTION_ID_LEADER_HEARTBEAT:
        return ElectorIOEventType::e_LEADER_HEARTBEAT;  // RETURN

    case ElectorMessageChoice::SELECTION_ID_LEADERSHIP_CESSION_NOTIFICATION:
        return ElectorIOEventType::e_LEADERSHIP_CESSION;  // RETURN

    case ElectorMessageChoice::SELECTION_ID_ELECTOR_NODE_STATUS: {
        const bmqp_ctrlmsg::ElectorNodeStatus& status =
            message.choice().electorNodeStatus();
        if (status.isAvailable()) {
            return ElectorIOEventType::e_NODE_AVAILABLE;  // RETURN
        }

        return ElectorIOEventType::e_NODE_UNAVAILABLE;  // RETURN
    }

    case ElectorMessageChoice::SELECTION_ID_HEARTBEAT_RESPONSE:
        return ElectorIOEventType::e_HEARTBEAT_RESPONSE;  // RETURN

    case ElectorMessageChoice::SELECTION_ID_SCOUTING_REQUEST:
        return ElectorIOEventType::e_SCOUTING_REQUEST;  // RETURN

    case ElectorMessageChoice::SELECTION_ID_SCOUTING_RESPONSE:
        return ElectorIOEventType::e_SCOUTING_RESPONSE;  // RETURN

    default: return ElectorIOEventType::e_NONE;  // RETURN
    }
}

}  // close unnamed namespace

// -------------------------
// struct ElectorIOEventType
// -------------------------

bsl::ostream& ElectorIOEventType::print(bsl::ostream&            stream,
                                        ElectorIOEventType::Enum value,
                                        int                      level,
                                        int spacesPerLevel)
{
    bdlb::Print::indent(stream, level, spacesPerLevel);
    stream << ElectorIOEventType::toAscii(value);

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

const char* ElectorIOEventType::toAscii(ElectorIOEventType::Enum value)
{
#define CASE(X)                                                               \
    case e_##X: return #X;

    switch (value) {
        CASE(NONE)
        CASE(ELECTION_PROPOSAL)
        CASE(ELECTION_RESPONSE)
        CASE(LEADER_HEARTBEAT)
        CASE(LEADERSHIP_CESSION)
        CASE(NODE_UNAVAILABLE)
        CASE(NODE_AVAILABLE)
        CASE(HEARTBEAT_RESPONSE)
        CASE(SCOUTING_REQUEST)
        CASE(SCOUTING_RESPONSE)
    default: return "(* UNKNOWN *)";
    }

#undef CASE
}

bool ElectorIOEventType::fromAscii(ElectorIOEventType::Enum* out,
                                   const bslstl::StringRef&  str)
{
#define CHECKVALUE(M)                                                         \
    if (bdlb::String::areEqualCaseless(toAscii(ElectorIOEventType::e_##M),    \
                                       str.data(),                            \
                                       static_cast<int>(str.length()))) {     \
        *out = ElectorIOEventType::e_##M;                                     \
        return true;                                                          \
    }

    CHECKVALUE(NONE)
    CHECKVALUE(ELECTION_PROPOSAL)
    CHECKVALUE(ELECTION_RESPONSE)
    CHECKVALUE(LEADER_HEARTBEAT)
    CHECKVALUE(NODE_UNAVAILABLE)
    CHECKVALUE(HEARTBEAT_RESPONSE)
    CHECKVALUE(SCOUTING_REQUEST)
    CHECKVALUE(SCOUTING_RESPONSE)

    // Invalid string
    return false;

#undef CHECKVALUE
}

// ----------------------------
// struct ElectorTimerEventType
// ----------------------------

bsl::ostream& ElectorTimerEventType::print(bsl::ostream&               stream,
                                           ElectorTimerEventType::Enum value,
                                           int                         level,
                                           int spacesPerLevel)
{
    bdlb::Print::indent(stream, level, spacesPerLevel);
    stream << ElectorTimerEventType::toAscii(value);

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

const char* ElectorTimerEventType::toAscii(ElectorTimerEventType::Enum value)
{
#define CASE(X)                                                               \
    case e_##X: return #X;

    switch (value) {
        CASE(NONE)
        CASE(INITIAL_WAIT_TIMER)
        CASE(RANDOM_WAIT_TIMER)
        CASE(ELECTION_RESULT_TIMER)
        CASE(HEARTBEAT_CHECK_TIMER)
        CASE(HEARTBEAT_BROADCAST_TIMER)
        CASE(SCOUTING_RESULT_TIMER)
    default: return "(* UNKNOWN *)";
    }

#undef CASE
}

bool ElectorTimerEventType::fromAscii(ElectorTimerEventType::Enum* out,
                                      const bslstl::StringRef&     str)
{
#define CHECKVALUE(M)                                                         \
    if (bdlb::String::areEqualCaseless(toAscii(ElectorTimerEventType::e_##M), \
                                       str.data(),                            \
                                       static_cast<int>(str.length()))) {     \
        *out = ElectorTimerEventType::e_##M;                                  \
        return true;                                                          \
    }

    CHECKVALUE(NONE)
    CHECKVALUE(INITIAL_WAIT_TIMER)
    CHECKVALUE(RANDOM_WAIT_TIMER)
    CHECKVALUE(ELECTION_RESULT_TIMER)
    CHECKVALUE(HEARTBEAT_CHECK_TIMER)
    CHECKVALUE(HEARTBEAT_BROADCAST_TIMER)
    CHECKVALUE(SCOUTING_RESULT_TIMER)

    // Invalid string
    return false;

#undef CHECKVALUE
}

// -------------------
// struct ElectorState
// -------------------

bsl::ostream& ElectorState::print(bsl::ostream&      stream,
                                  ElectorState::Enum value,
                                  int                level,
                                  int                spacesPerLevel)
{
    bdlb::Print::indent(stream, level, spacesPerLevel);
    stream << ElectorState::toAscii(value);

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

const char* ElectorState::toAscii(ElectorState::Enum value)
{
#define CASE(X)                                                               \
    case e_##X: return #X;

    switch (value) {
        CASE(DORMANT)
        CASE(FOLLOWER)
        CASE(CANDIDATE)
        CASE(LEADER)
    default: return "(* UNKNOWN *)";
    }

#undef CASE
}

bool ElectorState::fromAscii(ElectorState::Enum*      out,
                             const bslstl::StringRef& str)
{
#define CHECKVALUE(M)                                                         \
    if (bdlb::String::areEqualCaseless(toAscii(ElectorState::e_##M),          \
                                       str.data(),                            \
                                       static_cast<int>(str.length()))) {     \
        *out = ElectorState::e_##M;                                           \
        return true;                                                          \
    }

    CHECKVALUE(DORMANT)
    CHECKVALUE(FOLLOWER)
    CHECKVALUE(CANDIDATE)
    CHECKVALUE(LEADER)

    // Invalid string
    return false;

#undef CHECKVALUE
}

// ------------------------------
// struct ElectorTransitionReason
// ------------------------------

bsl::ostream&
ElectorTransitionReason::print(bsl::ostream&                 stream,
                               ElectorTransitionReason::Enum value,
                               int                           level,
                               int                           spacesPerLevel)
{
    bdlb::Print::indent(stream, level, spacesPerLevel);
    stream << ElectorTransitionReason::toAscii(value);

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

const char*
ElectorTransitionReason::toAscii(ElectorTransitionReason::Enum value)
{
#define CASE(X)                                                               \
    case e_##X: return #X;

    switch (value) {
        CASE(NONE)
        CASE(STARTED)
        CASE(LEADER_NO_HEARTBEAT)
        CASE(LEADER_UNAVAILABLE)
        CASE(LEADER_PREEMPTED)
        CASE(ELECTION_PREEMPTED)
        CASE(QUORUM_NOT_ACHIEVED)
    default: return "(* UNKNOWN *)";
    }

#undef CASE
}

bool ElectorTransitionReason::fromAscii(ElectorTransitionReason::Enum* out,
                                        const bslstl::StringRef&       str)
{
#define CHECKVALUE(M)                                                         \
    if (bdlb::String::areEqualCaseless(                                       \
            toAscii(ElectorTransitionReason::e_##M),                          \
            str)) {                                                           \
        *out = ElectorTransitionReason::e_##M;                                \
        return true;                                                          \
    }

    CHECKVALUE(NONE)
    CHECKVALUE(STARTED)
    CHECKVALUE(LEADER_NO_HEARTBEAT)
    CHECKVALUE(LEADER_PREEMPTED)
    CHECKVALUE(ELECTION_PREEMPTED)
    CHECKVALUE(QUORUM_NOT_ACHIEVED)

    // Invalid string
    return false;

#undef CHECKVALUE
}

// -------------------------
// class ElectorStateMachine
// -------------------------

// CONSTANTS
const int ElectorStateMachine::k_INVALID_NODE_ID;
const int ElectorStateMachine::k_ALL_NODES_ID = Cluster::k_ALL_NODES_ID;

// PRIVATE MANIPULATORS
void ElectorStateMachine::applyLeadershipCessionEventToFollower(
    ElectorStateMachineOutput* out,
    bsls::Types::Uint64        term,
    int                        sourceNodeId)
{
    if (ElectorState::e_FOLLOWER != d_state) {
        return;  // RETURN
    }

    BSLS_ASSERT(d_supporters.empty());

    if (d_term > term) {
        // Ignore leadershipCession event from a stale leader.
        return;  // RETURN
    }

    if (sourceNodeId == d_leaderNodeId) {
        BSLS_ASSERT_SAFE(k_INVALID_NODE_ID == d_tentativeLeaderNodeId);

        // Leader gave up its leadership. Notify app.
        d_leaderNodeId = k_INVALID_NODE_ID;
        d_reason       = ElectorTransitionReason::e_LEADER_NO_HEARTBEAT;
        d_lastLeaderHeartbeatTime = 0;

        // Schedule random wait timer event to initiate election, and also
        // cancel the hearbeat check timer.
        out->setTimer(ElectorTimerEventType::e_RANDOM_WAIT_TIMER);
        out->setCancelTimerEventsFlag(true);

        // Indicate state change.
        out->setStateChangedFlag(true);

        return;  // RETURN
    }

    if (sourceNodeId == d_tentativeLeaderNodeId) {
        BSLS_ASSERT_SAFE(k_INVALID_NODE_ID == d_leaderNodeId);

        // Leader gave up leadership before sending 1st heartbeat.
        d_tentativeLeaderNodeId   = k_INVALID_NODE_ID;
        d_reason                  = ElectorTransitionReason::e_NONE;
        d_lastLeaderHeartbeatTime = 0;

        // Schedule random wait timer event to initiate election, and also
        // cancel the heartbeat check timer.
        out->setTimer(ElectorTimerEventType::e_RANDOM_WAIT_TIMER);
        out->setCancelTimerEventsFlag(true);

        // No state change
        return;  // RETURN
    }
}

void ElectorStateMachine::applyLeadershipCessionEventToCandidate(
    BSLS_ANNOTATION_UNUSED ElectorStateMachineOutput* out,
    bsls::Types::Uint64                               term,
    int                                               sourceNodeId)
{
    if (ElectorState::e_CANDIDATE != d_state) {
        return;  // RETURN
    }

    BALL_LOG_INFO << "#ELECTOR_LEADERSHIP_CESSION "
                  << "CANDIDATE received LEADERSHIP_CESSION with term ["
                  << term << "] from node [" << sourceNodeId
                  << "]. Current term [" << d_term << "].";

    // No Output emitted.
    return;
}

void ElectorStateMachine::applyLeadershipCessionEventToLeader(
    ElectorStateMachineOutput* out,
    bsls::Types::Uint64        term,
    int                        sourceNodeId)
{
    if (ElectorState::e_LEADER != d_state) {
        return;  // RETURN
    }

    if (term <= d_term) {
        BALL_LOG_INFO << "#ELECTOR_LEADERSHIP_CESSION "
                      << "LEADER received LEADERSHIP_CESSION with term ["
                      << term << "] from node [" << sourceNodeId
                      << "]. Current term [" << d_term << "].";

        // Emit leader heartbeat to the sender 'sourceNodeId' hoping it follows
        // me.
        out->setIo(ElectorIOEventType::e_LEADER_HEARTBEAT);
        out->setDestination(sourceNodeId);
    }
    else {
        BALL_LOG_WARN << "#ELECTOR_LEADERSHIP_CESSION "
                      << "LEADER received LEADERSHIP_CESSION with higher term "
                      << "[" << term << "] from node [" << sourceNodeId
                      << "]. Current term [" << d_term << "].";
    }

    return;
}

void ElectorStateMachine::applyLeaderHeartbeatEventToFollower(
    ElectorStateMachineOutput* out,
    bsls::Types::Uint64        term,
    int                        sourceNodeId)
{
    // Need to handle heartbeats with stale and newer term

    if (d_term == term) {
        if (sourceNodeId == d_tentativeLeaderNodeId) {
            // First heartbeat from the leader which this elector instance
            // voted for.
            BSLS_ASSERT_SAFE(k_INVALID_NODE_ID == d_leaderNodeId);
            d_leaderNodeId            = sourceNodeId;
            d_tentativeLeaderNodeId   = k_INVALID_NODE_ID;
            d_reason                  = ElectorTransitionReason::e_NONE;
            d_lastLeaderHeartbeatTime = bmqsys::Time::highResolutionTimer();
            d_scoutingInfo.reset();

            // Indicate elector to schedule a recurring heart beat check event,
            // and cancel any existing timers.
            out->setTimer(ElectorTimerEventType::e_HEARTBEAT_CHECK_TIMER);
            out->setCancelTimerEventsFlag(true);

            // Indicate state change after 1st heartbeat from leader
            out->setStateChangedFlag(true);

            return;  // RETURN
        }

        if (sourceNodeId == d_leaderNodeId) {
            // This follower already knows about this leader
            BSLS_ASSERT_SAFE(k_INVALID_NODE_ID == d_tentativeLeaderNodeId);
            d_lastLeaderHeartbeatTime = bmqsys::Time::highResolutionTimer();

            // No state change
            return;  // RETURN
        }

        if (d_leaderNodeId == k_INVALID_NODE_ID &&
            d_tentativeLeaderNodeId == k_INVALID_NODE_ID) {
            // This follower does not have a leader and just received a
            // heartbeat from a peer node (claiming to be leader).  That node
            // must be the leader, so this node should submit to it.
            d_tentativeLeaderNodeId   = k_INVALID_NODE_ID;
            d_leaderNodeId            = sourceNodeId;
            d_reason                  = ElectorTransitionReason::e_NONE;
            d_lastLeaderHeartbeatTime = bmqsys::Time::highResolutionTimer();
            d_scoutingInfo.reset();

            // Indicate elector to schedule a recurring heart beat check event,
            // and cancel any existing timers.
            out->setTimer(ElectorTimerEventType::e_HEARTBEAT_CHECK_TIMER);
            out->setCancelTimerEventsFlag(true);

            // Also indicate your support to the leader.  This is important so
            // that leader can bump up its support count.
            out->setIo(ElectorIOEventType::e_ELECTION_RESPONSE);
            out->setDestination(sourceNodeId);

            // Indicate state change with new leader
            out->setStateChangedFlag(true);

            return;  // RETURN
        }

        // We have a valid 'd_tentativeLeaderNodeId' or a valid
        // 'd_leaderNodeId' but it is different from the 'sourceNodeId'.  In
        // that case, we should just ignore this heartbeat.
        BALL_LOG_INFO << "Elector:FOLLOWER received LEADER_HEARTBEAT from"
                      << " a non-leader node [" << sourceNodeId
                      << "] with same term [" << term << "]. Self has a valid"
                      << " 'd_tentativeLeaderNodeId' ["
                      << d_tentativeLeaderNodeId << "] or a valid"
                      << " 'd_leaderNodeId' [" << d_leaderNodeId << "], but it"
                      << " is different from 'sourceNodeId' [" << sourceNodeId
                      << "], hence ignoring this heartbeat.";

        // No state change
        return;  // RETURN
    }

    if (d_term > term) {
        BALL_LOG_WARN
            << "#ELECTOR_LEADER_HEARTBEAT "
            << "FOLLOWER received LEADER_HEARTBEAT with stale term [" << term
            << "] from node [" << sourceNodeId << "]. Current term [" << d_term
            << "] Current leader [" << d_leaderNodeId << "]. "
            << "Ignoring this heartbeat, but sending a stale heartbeat "
            << "response to the sender node.";

        // Emit stale heartbeat response to the sender 'sourceNodeId'.
        out->setIo(ElectorIOEventType::e_HEARTBEAT_RESPONSE);
        out->setDestination(sourceNodeId);

        // No state change
        return;  // RETURN
    }

    // d_term < term. This means that there is a new leader with higher term,
    // and this elector didn't vote for that (otherwise its 'd_term' would have
    // been equal to 'term')
    BALL_LOG_INFO << "Elector:FOLLOWER received LEADER_HEARTBEAT from "
                  << "node [" << sourceNodeId << "] with newer term [" << term
                  << "]. Current term [" << d_term << "], current leader ["
                  << d_leaderNodeId << "], current tentative leader ["
                  << d_tentativeLeaderNodeId << "]. Following new node as "
                  << "the leader.";

    d_tentativeLeaderNodeId   = k_INVALID_NODE_ID;
    d_leaderNodeId            = sourceNodeId;
    d_term                    = term;
    d_reason                  = ElectorTransitionReason::e_NONE;
    d_lastLeaderHeartbeatTime = bmqsys::Time::highResolutionTimer();
    d_scoutingInfo.reset();

    // Indicate elector to schedule a recurring heart beat check event, and to
    // cancel any existing timers.
    out->setTimer(ElectorTimerEventType::e_HEARTBEAT_CHECK_TIMER);
    out->setCancelTimerEventsFlag(true);

    // Also indicate your support to the leader.  This is important so that
    // leader can bump up its support count.
    out->setIo(ElectorIOEventType::e_ELECTION_RESPONSE);
    out->setDestination(sourceNodeId);
    out->setTerm(d_term);

    // Indicate state change with new leader
    out->setStateChangedFlag(true);
}

void ElectorStateMachine::applyLeaderHeartbeatEventToCandidate(
    ElectorStateMachineOutput* out,
    bsls::Types::Uint64        term,
    int                        sourceNodeId)
{
    if (d_term > term) {
        BALL_LOG_WARN
            << "#ELECTOR_LEADER_HEARTBEAT "
            << "CANDIDATE received LEADER_HEARTBEAT with stale term [" << term
            << "] from node [" << sourceNodeId << "]. Current term [" << d_term
            << "]. Ignoring this heartbeat, but sending a stale heartbeat "
            << "response to the sender node.";

        // Emit stale heartbeat response to the sender 'sourceNodeId'.
        out->setIo(ElectorIOEventType::e_HEARTBEAT_RESPONSE);
        out->setDestination(sourceNodeId);

        return;  // RETURN
    }

    // Got heartbeat with equal or higher term.
    BALL_LOG_INFO << "Elector:CANDIDATE received LEADER_HEARTBEAT from node ["
                  << sourceNodeId << "] with equal or higher term [" << term
                  << "]. Current term [" << d_term
                  << "]. Following new node as the leader.";

    d_supporters.clear();
    d_state                   = ElectorState::e_FOLLOWER;
    d_term                    = term;
    d_tentativeLeaderNodeId   = k_INVALID_NODE_ID;
    d_leaderNodeId            = sourceNodeId;
    d_reason                  = ElectorTransitionReason::e_NONE;
    d_lastLeaderHeartbeatTime = bmqsys::Time::highResolutionTimer();

    // Indicate elector to schedule a recurring heart beat check event.
    out->setTimer(ElectorTimerEventType::e_HEARTBEAT_CHECK_TIMER);

    // Also indicate your support to the leader.  This is important so that
    // leader can bump up its support count.
    out->setIo(ElectorIOEventType::e_ELECTION_RESPONSE);
    out->setDestination(sourceNodeId);
    out->setTerm(d_term);
    out->setCancelTimerEventsFlag(true);

    // Indicate state change.
    out->setStateChangedFlag(true);
}

void ElectorStateMachine::applyLeaderHeartbeatEventToLeader(
    ElectorStateMachineOutput* out,
    bsls::Types::Uint64        term,
    int                        sourceNodeId)
{
    if (d_term >= term) {
        BALL_LOG_WARN
            << "#ELECTOR_LEADER_HEARTBEAT "
            << "LEADER received LEADER_HEARTBEAT with stale term [" << term
            << "] from node [" << sourceNodeId << "]. Current term [" << d_term
            << "]. Ignoring this heartbeat, and sending leader heartbeat to "
            << "the sender node.";

        // Send leader heartbeat to stale leader 'sourceNodeId' so that it can
        // follow me.
        out->setIo(ElectorIOEventType::e_LEADER_HEARTBEAT);
        out->setDestination(sourceNodeId);

        return;  // RETURN
    }

    // Got heartbeat with higher term.
    BALL_LOG_INFO << "Elector:LEADER received LEADER_HEARTBEAT from node ["
                  << sourceNodeId << "] with newer term [" << term
                  << "]. Current term [" << d_term
                  << "]. Following new node as the leader.";

    d_supporters.clear();
    d_state                   = ElectorState::e_FOLLOWER;
    d_term                    = term;
    d_tentativeLeaderNodeId   = k_INVALID_NODE_ID;
    d_leaderNodeId            = sourceNodeId;
    d_lastLeaderHeartbeatTime = bmqsys::Time::highResolutionTimer();
    d_reason                  = ElectorTransitionReason::e_NONE;
    // TBD: specify 'e_LEADER_PREEMPTED' as the reason, instead of 'e_NONE' ?

    // Indicate elector to schedule a recurring heart beat check event.
    out->setTimer(ElectorTimerEventType::e_HEARTBEAT_CHECK_TIMER);
    out->setTerm(d_term);
    out->setCancelTimerEventsFlag(true);

    // Proactively give up leadership by sending a signal to all nodes
    out->setIo(ElectorIOEventType::e_LEADERSHIP_CESSION);
    out->setDestination(k_ALL_NODES_ID);

    // Indicate state change.
    out->setStateChangedFlag(true);
}

void ElectorStateMachine::applyElectionProposalEventToFollower(
    ElectorStateMachineOutput* out,
    bsls::Types::Uint64        term,
    int                        sourceNodeId)
{
    BSLS_ASSERT_SAFE(d_supporters.empty());

    if (k_INVALID_NODE_ID != d_leaderNodeId) {
        // Support the notion of sticky leader: if self sees a healthy leader,
        // don't respond to the election proposal, even if term in the election
        // proposal is higher.

        BALL_LOG_INFO << "Elector:FOLLOWER received ELECTION_PROPOSAL from "
                      << "node [" << sourceNodeId << "] with term [" << term
                      << "]. But self perceives node [" << d_leaderNodeId
                      << "] as a valid leader, with term [" << d_term
                      << "]. Ignoring this event.";
        return;  // RETURN
    }

    if (term <= d_term) {
        // We don't vote if the term in proposed election is not greater than
        // our term
        BALL_LOG_INFO << "Elector:FOLLOWER received ELECTION_PROPOSAL from "
                      << "node [" << sourceNodeId << "] with term [" << term
                      << "]. Current term [" << d_term
                      << "]. Ignoring this event.";
        return;  // RETURN
    }

    BALL_LOG_INFO << "Elector:FOLLOWER received ELECTION_PROPOSAL from node ["
                  << sourceNodeId << "] with newer term [" << term
                  << "]. Current term [" << d_term << "] Supporting this "
                  << "node.";

    // Support the candidate
    out->setIo(ElectorIOEventType::e_ELECTION_RESPONSE);
    out->setDestination(sourceNodeId);
    out->setTimer(ElectorTimerEventType::e_HEARTBEAT_CHECK_TIMER);
    out->setTerm(term);
    out->setCancelTimerEventsFlag(true);

    d_term                    = term;
    d_tentativeLeaderNodeId   = sourceNodeId;
    d_leaderNodeId            = k_INVALID_NODE_ID;
    d_lastLeaderHeartbeatTime = bmqsys::Time::highResolutionTimer();
    d_scoutingInfo.reset();
}

void ElectorStateMachine::applyElectionProposalEventToCandidate(
    ElectorStateMachineOutput* out,
    bsls::Types::Uint64        term,
    int                        sourceNodeId)
{
    if (term <= d_term) {
        // We don't vote if the term in proposed election is not greater than
        // our term
        BALL_LOG_INFO << "Elector:CANDIDATE received ELECTION_PROPOSAL from "
                      << "node [" << sourceNodeId << "] with term [" << term
                      << "]. Current term [" << d_term
                      << "]. Ignoring this event.";
        return;  // RETURN
    }

    BALL_LOG_INFO << "Elector:CANDIDATE received ELECTION_PROPOSAL from node ["
                  << sourceNodeId << "] with newer term [" << term
                  << "]. Current term [" << d_term << "]. Supporting this "
                  << "node";

    // Support the new candidate
    out->setIo(ElectorIOEventType::e_ELECTION_RESPONSE);
    out->setDestination(sourceNodeId);
    out->setTimer(ElectorTimerEventType::e_HEARTBEAT_CHECK_TIMER);
    out->setTerm(term);
    out->setCancelTimerEventsFlag(true);

    d_state                   = ElectorState::e_FOLLOWER;
    d_term                    = term;
    d_leaderNodeId            = k_INVALID_NODE_ID;
    d_tentativeLeaderNodeId   = sourceNodeId;
    d_reason                  = ElectorTransitionReason::e_ELECTION_PREEMPTED;
    d_lastLeaderHeartbeatTime = bmqsys::Time::highResolutionTimer();
    d_supporters.clear();

    // Indicate state change.
    out->setStateChangedFlag(true);
}

void ElectorStateMachine::applyElectionProposalEventToLeader(
    BSLS_ANNOTATION_UNUSED ElectorStateMachineOutput* out,
    bsls::Types::Uint64                               term,
    int                                               sourceNodeId)
{
    // Support the notion of sticky leader: since self is a healthy leader (has
    // quorum and has not received any stale heartbeat response from any
    // follower), don't respond to the election proposal, even if term in the
    // election is higher than self's term.

    BALL_LOG_INFO << "Elector:LEADER received ELECTION_PROPOSAL from "
                  << "node [" << sourceNodeId << "] with term [" << term
                  << "]. But self is a healthy leader with term [" << d_term
                  << "]. Ignoring this event.";
}

void ElectorStateMachine::applyElectionResponseEventToFollower(
    BSLS_ANNOTATION_UNUSED ElectorStateMachineOutput* out,
    bsls::Types::Uint64                               term,
    int                                               sourceNodeId)
{
    // This could occur as follows:
    // 1) This node initiates election
    // 2) Becomes FOLLOWER immediately after that
    // 3) Receives its proposed election's response

    BALL_LOG_INFO << "Elector:FOLLOWER received ELECTION_RESPONSE with term ["
                  << term << "] from node [" << sourceNodeId
                  << "]. Current term [" << d_term << "] Ignoring this event.";
}

void ElectorStateMachine::applyElectionResponseEventToCandidate(
    ElectorStateMachineOutput* out,
    bsls::Types::Uint64        term,
    int                        sourceNodeId)
{
    if (term < d_term) {
        BALL_LOG_INFO << "Elector:CANDIDATE received ELECTION_RESPONSE from "
                      << "node [" << sourceNodeId << "] with stale term ["
                      << term << "]. Current term [" << d_term
                      << "]. Ignoring this event.";
        return;  // RETURN
    }

    if (d_supporters.end() !=
        bsl::find(d_supporters.begin(), d_supporters.end(), sourceNodeId)) {
        // This node already responded to election proposal.  Simply ignore the
        // event.
        BALL_LOG_INFO << "Elector:CANDIDATE received ELECTION_RESPONSE from "
                      << "node [" << sourceNodeId << "] which already "
                      << "responded to election proposal earlier. "
                      << "Ignoring this event.";
        return;  // RETURN
    }

    // Update quorum
    d_supporters.push_back(sourceNodeId);

    BALL_LOG_INFO << "Elector:CANDIDATE received ELECTION_RESPONSE from "
                  << "node [" << sourceNodeId << "] with term [" << term
                  << "], current term [" << d_term
                  << "]. New quorum: " << d_supporters.size();

    if (static_cast<int>(d_supporters.size()) == d_quorum) {
        // Achieved quorum. Become leader.
        BALL_LOG_INFO << "Elector:CANDIDATE achieved quorum of " << d_quorum
                      << ". Transitioning to LEADER.";

        d_leaderNodeId          = d_selfId;
        d_tentativeLeaderNodeId = k_INVALID_NODE_ID;
        d_state                 = ElectorState::e_LEADER;
        d_reason                = ElectorTransitionReason::e_NONE;

        // Send heartbeat right away and schedule recurring heartbeat timer
        out->setIo(ElectorIOEventType::e_LEADER_HEARTBEAT);
        out->setDestination(k_ALL_NODES_ID);
        out->setTimer(ElectorTimerEventType::e_HEARTBEAT_BROADCAST_TIMER);
        out->setCancelTimerEventsFlag(true);

        // Indicate state change.
        out->setStateChangedFlag(true);

        return;  // RETURN
    }
}

void ElectorStateMachine::applyElectionResponseEventToLeader(
    ElectorStateMachineOutput* out,
    bsls::Types::Uint64        term,
    int                        sourceNodeId)
{
    if (term == d_term) {
        // A node accepted this elector's proposal and sent response. But this
        // elector received that response *after* it had already achieved
        // quorum and declared itself the LEADER.

        if (d_supporters.end() != bsl::find(d_supporters.begin(),
                                            d_supporters.end(),
                                            sourceNodeId)) {
            // Node has already declared its support for this elector (i.e.,
            // voted with the same term).  This could occur due to bug in
            // implementation.
            BALL_LOG_INFO << "Elector:LEADER received ELECTION_RESPONSE again "
                          << "from node [" << sourceNodeId << "] with term ["
                          << term << "]. Current term [" << d_term
                          << "]. Ignoring this event.";
        }
        else {
            d_supporters.push_back(sourceNodeId);
            BALL_LOG_INFO << "Elector:LEADER received ELECTION_RESPONSE from "
                          << "node [" << sourceNodeId << "] for term [" << term
                          << "] Current term [" << d_term
                          << "]. Quorum already achieved but adding this node "
                          << "to the list of supporters.";
        }

        return;  // RETURN
    }

    if (term < d_term) {
        BALL_LOG_INFO << "Elector:LEADER received ELECTION_RESPONSE from"
                      << " node [" << sourceNodeId << "] with stale term ["
                      << term << "]. Current term [" << d_term
                      << "]. Ignoring this event.";
        return;  // RETURN
    }

    // term > d_term. Err on the side of caution. Relinquish leadership,
    // delcare yourself FOLLOWER, and schedule 'randomWaitTimerCb'

    BALL_LOG_WARN
        << "#ELECTOR_ELECTION_RESPONSE "
        << "LEADER received ELECTION_RESPONSE from node [" << sourceNodeId
        << "] with higher term [" << term << "]. Current term [" << d_term
        << "]. Giving up leadership, will schedule election after a random "
        << "duration.";

    d_state                   = ElectorState::e_FOLLOWER;
    d_term                    = term;
    d_leaderNodeId            = k_INVALID_NODE_ID;
    d_tentativeLeaderNodeId   = k_INVALID_NODE_ID;
    d_lastLeaderHeartbeatTime = 0;
    d_reason                  = ElectorTransitionReason::e_LEADER_PREEMPTED;
    d_supporters.clear();

    out->setTerm(d_term);
    out->setTimer(ElectorTimerEventType::e_RANDOM_WAIT_TIMER);
    out->setCancelTimerEventsFlag(true);

    // Proactively give up leadership by sending a signal to all nodes
    out->setIo(ElectorIOEventType::e_LEADERSHIP_CESSION);
    out->setDestination(k_ALL_NODES_ID);

    // Indicate state change.
    out->setStateChangedFlag(true);
}

void ElectorStateMachine::applyHeartbeatResponseEventToLeader(
    ElectorStateMachineOutput* out,
    bsls::Types::Uint64        term,
    int                        sourceNodeId)
{
    if (d_term >= term) {
        // Leader has received a correct (d_term == term) or a stale (d_term >
        // term) heartbeat response from a peer.  Nothing to do.
        return;  // RETURN
    }

    // Leader has received a heartbeat response from a peer with higher term.
    // This likely means that there is another leader which self cannot see,
    // but peer ('sourceNodeId') can see.  Relinquish leadership.

    BALL_LOG_WARN
        << "#ELECTOR_HEARTBEAT_RESPONSE "
        << "LEADER received HEARTBEAT_RESPONSE from node [" << sourceNodeId
        << "] with higher term [" << term << "]. Current term [" << d_term
        << "]. Giving up leadership, will schedule election after a random "
        << "duration.";

    d_state                   = ElectorState::e_FOLLOWER;
    d_term                    = term;
    d_leaderNodeId            = k_INVALID_NODE_ID;
    d_tentativeLeaderNodeId   = k_INVALID_NODE_ID;
    d_lastLeaderHeartbeatTime = 0;
    d_reason                  = ElectorTransitionReason::e_LEADER_PREEMPTED;
    d_supporters.clear();

    out->setTimer(ElectorTimerEventType::e_RANDOM_WAIT_TIMER);
    out->setTerm(d_term);
    out->setCancelTimerEventsFlag(true);

    // Proactively give up leadership by sending a signal to all nodes.
    out->setIo(ElectorIOEventType::e_LEADERSHIP_CESSION);
    out->setDestination(k_ALL_NODES_ID);

    // Indicate state change.
    out->setStateChangedFlag(true);
}

void ElectorStateMachine::applyNodeStatusEventToFollower(
    ElectorStateMachineOutput* out,
    ElectorIOEventType::Enum   event,
    int                        sourceNodeId)
{
    BSLS_ASSERT_SAFE(ElectorIOEventType::e_NODE_UNAVAILABLE == event ||
                     ElectorIOEventType::e_NODE_AVAILABLE == event);

    if (ElectorIOEventType::e_NODE_AVAILABLE == event) {
        // No action needed, no state change.

        return;  // RETURN
    }

    if (sourceNodeId == d_leaderNodeId) {
        BALL_LOG_INFO << "Elector:FOLLOWER received NODE_UNAVAILABLE from "
                      << "node [" << sourceNodeId << "], current leader node ["
                      << d_leaderNodeId << "]. No leader from this point.";

        BSLS_ASSERT_SAFE(k_INVALID_NODE_ID == d_tentativeLeaderNodeId);
        BSLS_ASSERT_SAFE(d_supporters.empty());
        d_leaderNodeId            = k_INVALID_NODE_ID;
        d_lastLeaderHeartbeatTime = 0;
        d_reason = ElectorTransitionReason::e_LEADER_UNAVAILABLE;
        // No change in 'd_state' (continues to be FOLLOWER) and 'd_term'

        out->setTimer(ElectorTimerEventType::e_RANDOM_WAIT_TIMER);
        out->setCancelTimerEventsFlag(true);

        // Indicate state change.
        out->setStateChangedFlag(true);

        return;  // RETURN
    }

    if (sourceNodeId == d_tentativeLeaderNodeId) {
        BALL_LOG_INFO << "Elector:FOLLOWER received NODE_UNAVAILABLE from "
                      << "node [" << sourceNodeId
                      << "], current tentative leader node [" << d_leaderNodeId
                      << "]. No leader from this point.";

        BSLS_ASSERT_SAFE(k_INVALID_NODE_ID == d_leaderNodeId);
        BSLS_ASSERT_SAFE(d_supporters.empty());
        d_tentativeLeaderNodeId   = k_INVALID_NODE_ID;
        d_lastLeaderHeartbeatTime = 0;

        out->setTimer(ElectorTimerEventType::e_RANDOM_WAIT_TIMER);

        // No state change notification for application since we didn't notify
        // app of the tentative leader.
        return;  // RETURN
    }

    // A peer FOLLOWER is no longer available. This doesn't affect us.  Just
    // remove it's scouting response if applicable.
    d_scoutingInfo.removeNodeResponse(sourceNodeId);
    BALL_LOG_INFO << "Elector:FOLLOWER received NODE_UNAVAILABLE from a "
                  << "FOLLOWER node [" << sourceNodeId
                  << "]. Ignoring this event.";
}

void ElectorStateMachine::applyNodeStatusEventToCandidate(
    BSLS_ANNOTATION_UNUSED ElectorStateMachineOutput* out,
    ElectorIOEventType::Enum                          event,
    int                                               sourceNodeId)
{
    BSLS_ASSERT_SAFE(ElectorIOEventType::e_NODE_UNAVAILABLE == event ||
                     ElectorIOEventType::e_NODE_AVAILABLE == event);

    if (ElectorIOEventType::e_NODE_AVAILABLE == event) {
        // No action needed, no state change.

        return;  // RETURN
    }

    // If unavailable 'sourceNodeId' responded to election proposal, reduce
    // quorum count.  Ignore otherwise.

    bsl::vector<int>::const_iterator nodeIter = bsl::find(d_supporters.begin(),
                                                          d_supporters.end(),
                                                          sourceNodeId);
    if (nodeIter != d_supporters.end()) {
        // This node voted for this instance. Remove the node from the list of
        // supporters.
        BALL_LOG_INFO
            << "Elector:CANDIDATE received NODE_UNAVAILABLE from node"
            << " [" << sourceNodeId << "] which voted for this "
            << "CANDIDATE. Removing node from list of voters.";
        d_supporters.erase(nodeIter);
    }
}

void ElectorStateMachine::applyNodeStatusEventToLeader(
    ElectorStateMachineOutput* out,
    ElectorIOEventType::Enum   event,
    int                        sourceNodeId)
{
    BSLS_ASSERT_SAFE(ElectorIOEventType::e_NODE_UNAVAILABLE == event ||
                     ElectorIOEventType::e_NODE_AVAILABLE == event);

    BSLS_ASSERT_SAFE(k_INVALID_NODE_ID == d_tentativeLeaderNodeId);

    if (ElectorIOEventType::e_NODE_AVAILABLE == event) {
        // Send a heartbeat immediately, so that new node can see this leader
        // as soon as possible.  Note that leader (self) will not add this node
        // to the list of supporters yet.  That will be done when new node sees
        // self as leader *and* sends a 'fake' election response upon receiving
        // 1st heart beat from the leader.  This is not required for the
        // correct functioning of the election algo, but in some cases, it is
        // desirable for a new node to discover the leader as soon as possible
        // to minimize convergence time with its peer nodes.  There is no state
        // change though.

        out->setIo(ElectorIOEventType::e_LEADER_HEARTBEAT);
        out->setDestination(sourceNodeId);
        return;  // RETURN
    }

    // If 'sourceNodeId' voted for this elector instance, *and* its
    // unavailability makes this instance lose quorum, then:
    //   o Become FOLLOWER
    //   o Cancel all clocks
    //   o Schedule 'randomWaitTimerCb'

    bsl::vector<int>::const_iterator nodeIter = bsl::find(d_supporters.begin(),
                                                          d_supporters.end(),
                                                          sourceNodeId);
    if (nodeIter != d_supporters.end()) {
        // This node voted for this instance. Remove the node from the list of
        // supporters.

        d_supporters.erase(nodeIter);
        BALL_LOG_INFO << "Elector:LEADER received NODE_UNAVAILABLE from node "
                      << "[" << sourceNodeId << "] which voted for this "
                      << "LEADER. Removed node from list of voters. "
                      << "Updated quorum: " << d_supporters.size();
    }

    if (static_cast<int>(d_supporters.size()) >= d_quorum) {
        // Still have quorum. Nothing to do.
        return;  // RETURN
    }

    // Lost quorum.
    BALL_LOG_INFO << "Elector:LEADER lost quorum due to node [" << sourceNodeId
                  << "] unavailability. Expected quorum [" << d_quorum
                  << "], new quorum [" << d_supporters.size()
                  << "]. Will wait random time before proposing election";

    out->setTimer(ElectorTimerEventType::e_RANDOM_WAIT_TIMER);
    out->setCancelTimerEventsFlag(true);
    d_state                   = ElectorState::e_FOLLOWER;
    d_leaderNodeId            = k_INVALID_NODE_ID;
    d_lastLeaderHeartbeatTime = 0;
    d_tentativeLeaderNodeId   = k_INVALID_NODE_ID;
    d_reason                  = ElectorTransitionReason::e_QUORUM_NOT_ACHIEVED;
    d_supporters.clear();

    // Indicate state change.
    out->setStateChangedFlag(true);

    // Proactively give up leadership by signaling to all the nodes.
    out->setIo(ElectorIOEventType::e_LEADERSHIP_CESSION);
    out->setDestination(k_ALL_NODES_ID);
}

void ElectorStateMachine::applyLeaderHeartbeatEvent(
    ElectorStateMachineOutput* out,
    bsls::Types::Uint64        term,
    int                        sourceNodeId)
{
    if (!isValidSourceNode(sourceNodeId) || term == k_INVALID_TERM) {
        return;  // RETURN
    }

    switch (d_state) {
    case ElectorState::e_FOLLOWER:
        applyLeaderHeartbeatEventToFollower(out, term, sourceNodeId);
        break;
    case ElectorState::e_CANDIDATE:
        applyLeaderHeartbeatEventToCandidate(out, term, sourceNodeId);
        break;
    case ElectorState::e_LEADER:
        applyLeaderHeartbeatEventToLeader(out, term, sourceNodeId);
        break;
    case ElectorState::e_DORMANT:
    default: break;
    }
}

void ElectorStateMachine::applyLeadershipCessionEvent(
    ElectorStateMachineOutput* out,
    bsls::Types::Uint64        term,
    int                        sourceNodeId)
{
    if (!isValidSourceNode(sourceNodeId) || term == k_INVALID_TERM) {
        return;  // RETURN
    }

    switch (d_state) {
    case ElectorState::e_FOLLOWER:
        applyLeadershipCessionEventToFollower(out, term, sourceNodeId);
        break;
    case ElectorState::e_CANDIDATE:
        applyLeadershipCessionEventToCandidate(out, term, sourceNodeId);
        break;
    case ElectorState::e_LEADER:
        applyLeadershipCessionEventToLeader(out, term, sourceNodeId);
        break;
    case ElectorState::e_DORMANT:
    default: break;
    }
}

void ElectorStateMachine::applyElectionProposalEvent(
    ElectorStateMachineOutput* out,
    bsls::Types::Uint64        term,
    int                        sourceNodeId)
{
    if (!isValidSourceNode(sourceNodeId) || term == k_INVALID_TERM) {
        return;  // RETURN
    }

    switch (d_state) {
    case ElectorState::e_FOLLOWER:
        applyElectionProposalEventToFollower(out, term, sourceNodeId);
        break;
    case ElectorState::e_CANDIDATE:
        applyElectionProposalEventToCandidate(out, term, sourceNodeId);
        break;
    case ElectorState::e_LEADER:
        applyElectionProposalEventToLeader(out, term, sourceNodeId);
        break;
    case ElectorState::e_DORMANT:
    default: break;
    }
}

void ElectorStateMachine::applyElectionResponseEvent(
    ElectorStateMachineOutput* out,
    bsls::Types::Uint64        term,
    int                        sourceNodeId)
{
    if (!isValidSourceNode(sourceNodeId) || term == k_INVALID_TERM) {
        return;  // RETURN
    }

    switch (d_state) {
    case ElectorState::e_FOLLOWER:
        applyElectionResponseEventToFollower(out, term, sourceNodeId);
        break;
    case ElectorState::e_CANDIDATE:
        applyElectionResponseEventToCandidate(out, term, sourceNodeId);
        break;
    case ElectorState::e_LEADER:
        applyElectionResponseEventToLeader(out, term, sourceNodeId);
        break;
    case ElectorState::e_DORMANT:
    default: break;
    }
}

void ElectorStateMachine::applyHeartbeatResponseEvent(
    ElectorStateMachineOutput* out,
    bsls::Types::Uint64        term,
    int                        sourceNodeId)
{
    if (!isValidSourceNode(sourceNodeId) || term == k_INVALID_TERM) {
        return;  // RETURN
    }

    // Heartbeat response needs to be processed only if elector is in 'leader'
    // state.
    if (d_state != ElectorState::e_LEADER) {
        return;  // RETURN
    }

    applyHeartbeatResponseEventToLeader(out, term, sourceNodeId);
}

void ElectorStateMachine::applyScoutingRequestEvent(
    ElectorStateMachineOutput* out,
    bsls::Types::Uint64        term,
    int                        sourceNodeId)
{
    if (!isValidSourceNode(sourceNodeId) || term == k_INVALID_TERM) {
        return;  // RETURN
    }

    // A scouting response is always sent.
    out->setIo(ElectorIOEventType::e_SCOUTING_RESPONSE);
    out->setDestination(sourceNodeId);

    if (k_INVALID_NODE_ID != d_leaderNodeId) {
        // Support the notion of sticky leader: if self sees a healthy leader,
        // notify sender that self won't support it.

        BALL_LOG_INFO << "Elector received SCOUTING_REQUEST from "
                      << "node [" << sourceNodeId << "] with term [" << term
                      << "]. But self perceives node [" << d_leaderNodeId
                      << "] as a valid leader, with term [" << d_term
                      << "]. Not supporting the scouting node.";
        out->setScoutingResponseFlag(false);
    }
    else {
        // Self does not perceive any node as valid leader.

        // Self will support 'sourceNodeId' only if it proposes an election
        // with a 'term' greater than self's term.
        out->setScoutingResponseFlag(term > d_term);
    }

    // No state change.
}

void ElectorStateMachine::applyScoutingResponseEvent(
    ElectorStateMachineOutput* out,
    bool                       willVote,
    BSLS_ANNOTATION_UNUSED bsls::Types::Uint64 term,
    int                                        sourceNodeId)
{
    if (!isValidSourceNode(sourceNodeId)) {
        return;  // RETURN
    }

    if (ElectorState::e_FOLLOWER != d_state) {
        return;  // RETURN
    }

    if (k_INVALID_NODE_ID != d_leaderNodeId ||
        k_INVALID_NODE_ID != d_tentativeLeaderNodeId) {
        // Elector sees either valid (tentative) leader.  This response can be
        // ignored.
        return;  // RETURN
    }

    if (k_INVALID_TERM == d_scoutingInfo.term()) {
        // There is no 'live' scouting request.  This could occur if this is a
        // delayed response, or self node 'terminated' the scouting request
        // because it received scouting responses from majority of the nodes.

        return;  // RETURN
    }

    d_scoutingInfo.addNodeResponse(sourceNodeId, willVote);

    if (d_scoutingInfo.numSupportingNodes() >= static_cast<size_t>(d_quorum)) {
        // Majority of the nodes will support an election with the specified
        // 'term'.  Transition to candidate.

        BALL_LOG_INFO << "Elector:FOLLOWER received pre-election scouting "
                      << "support from " << d_quorum
                      << " peer nodes. Transitioning to candidate and "
                      << "proposing an election with term ["
                      << d_scoutingInfo.term() << "].";

        BSLS_ASSERT_SAFE(d_supporters.empty());
        d_supporters.push_back(d_selfId);  // you always support yourself

        // Become candidate and emit appropriate IO & timer events
        d_term   = d_scoutingInfo.term();
        d_state  = ElectorState::e_CANDIDATE;
        d_reason = ElectorTransitionReason::e_NONE;
        out->setTerm(d_term);
        out->setIo(ElectorIOEventType::e_ELECTION_PROPOSAL);
        out->setTimer(ElectorTimerEventType::e_ELECTION_RESULT_TIMER);
        out->setDestination(k_ALL_NODES_ID);
        out->setCancelTimerEventsFlag(true);

        // Clear out scouting info.. no need to remember it now.
        d_scoutingInfo.reset();

        // Indicate state change.
        out->setStateChangedFlag(true);

        return;  // RETURN
    }

    if (d_scoutingInfo.numResponses() ==
        static_cast<size_t>(d_numTotalPeers)) {
        // All nodes have responded, but as per previous 'if' check, majority
        // of the nodes did not express support.  This means that this round of
        // scouting request failed.  Elector needs to wait random time before
        // starting next round of scouting request.

        BALL_LOG_INFO << "Elector:FOLLOWER received pre-election scouting "
                      << "responses from all [" << d_numTotalPeers
                      << "] nodes but majority of the nodes did not express "
                      << "support for scouting term [" << d_scoutingInfo.term()
                      << "].  Elector will wait random time before "
                      << "initiating scouting round if applicable.";

        out->setTimer(ElectorTimerEventType::e_RANDOM_WAIT_TIMER);
        out->setCancelTimerEventsFlag(true);
        d_scoutingInfo.reset();
    }
}

void ElectorStateMachine::applyElectionResultTimerEvent(
    ElectorStateMachineOutput* out)
{
    if (ElectorState::e_CANDIDATE != d_state) {
        return;  // RETURN
    }

    BSLS_ASSERT_SAFE(k_INVALID_NODE_ID == d_leaderNodeId);
    BSLS_ASSERT_SAFE(k_INVALID_NODE_ID == d_tentativeLeaderNodeId);
    BSLS_ASSERT_SAFE(0 == d_lastLeaderHeartbeatTime);

    // In the case where quorum == 1, the node only realizes that it has
    // achieved quorum upon election timeout.
    if (static_cast<int>(d_supporters.size()) >= d_quorum) {
        // Achieved quorum. Become leader.
        BALL_LOG_INFO << "Elector:CANDIDATE achieved quorum of " << d_quorum
                      << ". Transitioning to LEADER.";

        d_leaderNodeId          = d_selfId;
        d_tentativeLeaderNodeId = k_INVALID_NODE_ID;
        d_state                 = ElectorState::e_LEADER;
        d_reason                = ElectorTransitionReason::e_NONE;

        // Send heartbeat right away and schedule recurring heartbeat timer
        out->setIo(ElectorIOEventType::e_LEADER_HEARTBEAT);
        out->setDestination(k_ALL_NODES_ID);
        out->setTimer(ElectorTimerEventType::e_HEARTBEAT_BROADCAST_TIMER);
        out->setCancelTimerEventsFlag(true);

        // Indicate state change.
        out->setStateChangedFlag(true);

        return;  // RETURN
    }

    BALL_LOG_INFO << "Elector:CANDIDATE couldn't achieve quorum. Achieved "
                  << "number [" << d_supporters.size() << "], expected ["
                  << d_quorum << "]";

    d_state  = ElectorState::e_FOLLOWER;
    d_reason = ElectorTransitionReason::e_QUORUM_NOT_ACHIEVED;
    d_supporters.clear();

    // Schedule random wait event
    out->setTimer(ElectorTimerEventType::e_RANDOM_WAIT_TIMER);
    out->setCancelTimerEventsFlag(true);

    // Indicate state change.
    out->setStateChangedFlag(true);
}

void ElectorStateMachine::applyHeartbeatCheckTimerEvent(
    ElectorStateMachineOutput* out)
{
    if (ElectorState::e_FOLLOWER != d_state) {
        return;  // RETURN
    }

    BSLS_ASSERT(d_supporters.empty());

    bsls::Types::Int64 currTime = bmqsys::Time::highResolutionTimer();
    if ((currTime - d_lastLeaderHeartbeatTime) < d_leaderInactivityInterval) {
        // Received heart beat from leader within the configured time interval.
        // No state change.
        return;  // RETURN
    }

    // Heart beat not received for the configured inactivity interval.
    if (k_INVALID_NODE_ID != d_leaderNodeId) {
        BSLS_ASSERT_SAFE(k_INVALID_NODE_ID == d_tentativeLeaderNodeId);

        // Missed not-1st heartbeat from the leader. Notify app.
        d_leaderNodeId = k_INVALID_NODE_ID;
        d_reason       = ElectorTransitionReason::e_LEADER_NO_HEARTBEAT;
        d_lastLeaderHeartbeatTime = 0;

        // Schedule random wait timer event to initiate election, and also
        // cancel the hearbeat check timer.
        out->setTimer(ElectorTimerEventType::e_RANDOM_WAIT_TIMER);
        out->setCancelTimerEventsFlag(true);

        // Indicate state change.
        out->setStateChangedFlag(true);

        return;  // RETURN
    }

    if (k_INVALID_NODE_ID != d_tentativeLeaderNodeId) {
        BSLS_ASSERT_SAFE(k_INVALID_NODE_ID == d_leaderNodeId);

        // Missed 1st heartbeat from the leader.  Since we didn't notify app
        // app of tentative leader coming up, we don't need to notify it when
        // tentative leader goes down/missing.  Just update internal state.
        d_tentativeLeaderNodeId   = k_INVALID_NODE_ID;
        d_reason                  = ElectorTransitionReason::e_NONE;
        d_lastLeaderHeartbeatTime = 0;

        // Schedule random wait timer event to initiate election, and also
        // cancel the heartbeat check timer.
        out->setTimer(ElectorTimerEventType::e_RANDOM_WAIT_TIMER);
        out->setCancelTimerEventsFlag(true);

        // No state change
        return;  // RETURN
    }

    // It may be tempting to add a assert at this point (eg, "unreachable by
    // design"), but depending upon the timing of cancellation of scheduler
    // events, it may be possible to reach here.
}

void ElectorStateMachine::applyRandomWaitTimerEvent(
    ElectorStateMachineOutput* out)
{
    if (ElectorState::e_FOLLOWER != d_state) {
        return;  // RETURN
    }

    if (k_INVALID_NODE_ID != d_tentativeLeaderNodeId ||
        k_INVALID_NODE_ID != d_leaderNodeId) {
        // We have a leader (or a tentative leader). So don't schedule election

        BALL_LOG_INFO << "Elector:FOLLOWER not sending pre-election "
                      << "scouting request to peers after waiting random time "
                      << "because of valid leader [" << d_leaderNodeId
                      << "] or tentative leader [" << d_tentativeLeaderNodeId
                      << "].";
        return;  // RETURN
    }

    // Self is follower with no leader or no tentative leader.  Propose a
    // pre-election scouting request.
    d_scoutingInfo.reset();
    d_scoutingInfo.setTerm(d_term + 1);  // 'd_term' remains unchanged

    // Support yourself in scouting step.
    d_scoutingInfo.addNodeResponse(d_selfId, true);

    // Use appropriate outgoing term, which will be different from 'd_term'.
    out->setTerm(d_scoutingInfo.term());

    // In case quorum is 1, self node can directly propose an election and
    // transition to candidate.
    if (d_scoutingInfo.numSupportingNodes() >= static_cast<size_t>(d_quorum)) {
        BALL_LOG_INFO << "Elector:FOLLOWER achieved pre-election scouting "
                      << "support from " << d_quorum
                      << " peer nodes. Transitioning to candidate and "
                      << "proposing an election with term [" << d_term + 1
                      << "].";

        BSLS_ASSERT_SAFE(d_supporters.empty());
        d_supporters.push_back(d_selfId);  // you always support yourself

        // Become candidate and emit appropriate IO & timer events
        d_term   = d_scoutingInfo.term();
        d_state  = ElectorState::e_CANDIDATE;
        d_reason = ElectorTransitionReason::e_NONE;
        out->setIo(ElectorIOEventType::e_ELECTION_PROPOSAL);
        out->setTimer(ElectorTimerEventType::e_ELECTION_RESULT_TIMER);
        out->setDestination(k_ALL_NODES_ID);
        out->setCancelTimerEventsFlag(true);

        // Clear out scouting info.. no need to remember it now.
        d_scoutingInfo.reset();

        // Indicate state change.
        out->setStateChangedFlag(true);

        return;  // RETURN
    }

    BALL_LOG_INFO << "Elector:FOLLOWER sending pre-election scouting request "
                  << "with term [" << d_scoutingInfo.term() << "] to peers "
                  << "after waiting random time because of no leader.";

    d_reason = ElectorTransitionReason::e_NONE;
    out->setIo(ElectorIOEventType::e_SCOUTING_REQUEST);
    out->setTimer(ElectorTimerEventType::e_SCOUTING_RESULT_TIMER);
    out->setDestination(k_ALL_NODES_ID);
}

void ElectorStateMachine::applyHeartbeatBroadcastTimerEvent(
    ElectorStateMachineOutput* out)
{
    if (ElectorState::e_LEADER != d_state) {
        return;  // RETURN
    }

    BSLS_ASSERT_SAFE(d_selfId == d_leaderNodeId);
    BSLS_ASSERT_SAFE(false == d_supporters.empty());
    BSLS_ASSERT_SAFE(0 == d_lastLeaderHeartbeatTime);
    BSLS_ASSERT_SAFE(k_INVALID_NODE_ID == d_tentativeLeaderNodeId);
    BSLS_ASSERT_SAFE(0 < d_term);

    out->setIo(ElectorIOEventType::e_LEADER_HEARTBEAT);
    out->setDestination(k_ALL_NODES_ID);

    return;
}

void ElectorStateMachine::applyScoutingResultTimerEvent(
    ElectorStateMachineOutput* out)
{
    if (ElectorState::e_FOLLOWER != d_state) {
        return;  // RETURN
    }

    if (k_INVALID_TERM == d_scoutingInfo.term()) {
        // There is no pending scouting request "round".  This check is correct
        // because when scouting round is cancelled, 'd_scoutingInfo' is reset.
        // We could also check for '0 == d_scoutingInfo.numResponses()'
        // instead.
        return;  // RETURN
    }

    BALL_LOG_INFO << "Elector:FOLLOWER didn't get scouting support from at "
                  << "least " << d_quorum << " nodes for term ["
                  << d_scoutingInfo.term() << "].";

    d_scoutingInfo.reset();

    // Schedule random wait event.
    out->setTimer(ElectorTimerEventType::e_RANDOM_WAIT_TIMER);
}

// MANIPULATORS
void ElectorStateMachine::enable(int selfId,
                                 int quorum,
                                 int numTotalPeers,
                                 int leaderInactivityIntervalMs)
{
    BSLS_ASSERT_SAFE(0 < quorum);
    BSLS_ASSERT_SAFE(k_INVALID_NODE_ID != selfId);
    BSLS_ASSERT_SAFE(0 < leaderInactivityIntervalMs);
    BSLS_ASSERT_SAFE(quorum <= numTotalPeers);

    if (isEnabled()) {
        return;  // RETURN
    }

    // DO NOT set `d_term` to 0 because the Elector could have started with a
    // non-zero initial term.
    d_state                    = ElectorState::e_FOLLOWER;
    d_reason                   = ElectorTransitionReason::e_STARTED;
    d_quorum                   = quorum;
    d_numTotalPeers            = numTotalPeers;
    d_selfId                   = selfId;
    d_leaderNodeId             = k_INVALID_NODE_ID;
    d_tentativeLeaderNodeId    = k_INVALID_NODE_ID;
    d_lastLeaderHeartbeatTime  = 0;
    d_leaderInactivityInterval = leaderInactivityIntervalMs *
                                 bdlt::TimeUnitRatio::k_NS_PER_MS;
    d_supporters.clear();
    ++d_age;
}

void ElectorStateMachine::disable()
{
    if (false == isEnabled()) {
        return;  // RETURN
    }

    d_state                    = ElectorState::e_DORMANT;
    d_reason                   = ElectorTransitionReason::e_NONE;
    d_term                     = k_INVALID_TERM;
    d_quorum                   = 0;
    d_numTotalPeers            = 0;
    d_selfId                   = k_INVALID_NODE_ID;
    d_leaderNodeId             = k_INVALID_NODE_ID;
    d_lastLeaderHeartbeatTime  = 0;
    d_leaderInactivityInterval = 0;
    d_tentativeLeaderNodeId    = k_INVALID_NODE_ID;
    d_supporters.clear();
    ++d_age;
}

void ElectorStateMachine::applyIOEvent(ElectorStateMachineOutput* out,
                                       ElectorIOEventType::Enum   type,
                                       bsls::Types::Uint64        term,
                                       int                        sourceNodeId)
{
    out->reset();

    if (!isEnabled()) {
        return;  // RETURN
    }

    out->setTerm(d_term);

    switch (type) {
    case ElectorIOEventType::e_LEADER_HEARTBEAT: {
        applyLeaderHeartbeatEvent(out, term, sourceNodeId);
    } break;  // BREAK

    case ElectorIOEventType::e_ELECTION_PROPOSAL: {
        applyElectionProposalEvent(out, term, sourceNodeId);
    } break;  // BREAK

    case ElectorIOEventType::e_ELECTION_RESPONSE: {
        applyElectionResponseEvent(out, term, sourceNodeId);
    } break;  // BREAK

    case ElectorIOEventType::e_HEARTBEAT_RESPONSE: {
        applyHeartbeatResponseEvent(out, term, sourceNodeId);
    } break;  // BREAK

    case ElectorIOEventType::e_SCOUTING_REQUEST: {
        applyScoutingRequestEvent(out, term, sourceNodeId);
    } break;  // BREAK

    case ElectorIOEventType::e_LEADERSHIP_CESSION: {
        applyLeadershipCessionEvent(out, term, sourceNodeId);

    } break;

    case ElectorIOEventType::e_NODE_UNAVAILABLE:
    case ElectorIOEventType::e_NODE_AVAILABLE:
    case ElectorIOEventType::e_SCOUTING_RESPONSE:
    case ElectorIOEventType::e_NONE:
    default: {
        BALL_LOG_ERROR << "#ELECTOR_INVALID_EVENT "
                       << "Attempt to apply invalid event type: "
                       << static_cast<int>(type) << " [" << type
                       << "]. Term: " << term
                       << ", SourceNodeId: " << sourceNodeId;
    } break;  // BREAK
    }

    if (out->stateChangedFlag()) {
        ++d_age;
    }
}

void ElectorStateMachine::applyAvailability(ElectorStateMachineOutput* out,
                                            ElectorIOEventType::Enum   type,
                                            int sourceNodeId)
{
    BSLS_ASSERT_SAFE(ElectorIOEventType::e_NODE_UNAVAILABLE == type ||
                     ElectorIOEventType::e_NODE_AVAILABLE == type);

    out->reset();

    if (!isEnabled()) {
        return;  // RETURN
    }

    out->setTerm(d_term);

    if (!isValidSourceNode(sourceNodeId)) {
        return;  // RETURN
    }

    switch (d_state) {
    case ElectorState::e_FOLLOWER: {
        applyNodeStatusEventToFollower(out, type, sourceNodeId);
    } break;  // BREAK
    case ElectorState::e_CANDIDATE: {
        applyNodeStatusEventToCandidate(out, type, sourceNodeId);
    } break;  // BREAK
    case ElectorState::e_LEADER: {
        applyNodeStatusEventToLeader(out, type, sourceNodeId);
    } break;  // BREAK
    case ElectorState::e_DORMANT:
    default: break;  // BREAK
    }

    if (out->stateChangedFlag()) {
        ++d_age;
    }
}

void ElectorStateMachine::applyScout(ElectorStateMachineOutput* out,
                                     bool                       willVote,
                                     bsls::Types::Uint64        term,
                                     int                        sourceNodeId)
{
    out->reset();

    if (!isEnabled()) {
        return;  // RETURN
    }

    out->setTerm(d_term);
    applyScoutingResponseEvent(out, willVote, term, sourceNodeId);

    if (out->stateChangedFlag()) {
        ++d_age;
    }
}

void ElectorStateMachine::applyTimer(ElectorStateMachineOutput*  out,
                                     ElectorTimerEventType::Enum type)
{
    out->reset();

    if (!isEnabled()) {
        return;  // RETURN
    }

    out->setTerm(d_term);

    switch (type) {
    case ElectorTimerEventType::e_ELECTION_RESULT_TIMER: {
        applyElectionResultTimerEvent(out);
    } break;  // BREAK

    case ElectorTimerEventType::e_HEARTBEAT_CHECK_TIMER: {
        applyHeartbeatCheckTimerEvent(out);
    } break;  // BREAK

    case ElectorTimerEventType::e_INITIAL_WAIT_TIMER:
    case ElectorTimerEventType::e_RANDOM_WAIT_TIMER: {
        applyRandomWaitTimerEvent(out);
    } break;  // BREAK

    case ElectorTimerEventType::e_HEARTBEAT_BROADCAST_TIMER: {
        applyHeartbeatBroadcastTimerEvent(out);
    } break;  // BREAK

    case ElectorTimerEventType::e_SCOUTING_RESULT_TIMER: {
        applyScoutingResultTimerEvent(out);
    } break;  // BREAK

    case ElectorTimerEventType::e_NONE:
    default: {
        BALL_LOG_ERROR << "#ELECTOR_INVALID_EVENT "
                       << "Attempt to apply invalid timer event type: "
                       << static_cast<int>(type) << " [" << type << "]";
    } break;  // BREAK
    }

    if (out->stateChangedFlag()) {
        ++d_age;
    }
}

// -------------
// class Elector
// -------------

// CONSTANTS
const int Elector::k_INVALID_NODE_ID;
const int Elector::k_ALL_NODES_ID = Cluster::k_ALL_NODES_ID;

// PRIVATE MANIPULATORS
void Elector::electorStateInternalCb(ElectorState::Enum            state,
                                     ElectorTransitionReason::Enum code,
                                     int                 leaderNodeId,
                                     bsls::Types::Uint64 term,
                                     bsls::Types::Uint64 age)
{
    // executed by the *CLUSTER* dispatcher thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));

    if (age <= d_previousEventAge) {
        BALL_LOG_WARN
            << "#ELECTOR_STALE_EVENT "
            << "Not invoking elector state callback because event is stale. "
            << "Event's age: " << age
            << ", age of last notified event: " << d_previousEventAge
            << ". Stale event's details: [state: " << state
            << ", code: " << code << ", leaderNodeId: " << leaderNodeId
            << ", term: " << term << "].";
        return;  // RETURN
    }

    // Invoke the user-specified callback, and bump up the notification age.
    // Note that user-specified callback is only invoked from one thread (the
    // cluster-dispatcher thread), so the notification age can be bumped either
    // before or after the callback is invoked -- the order does not matter.
    d_callback(state, code, leaderNodeId, term);
    d_previousEventAge = age;
}

void Elector::processStateMachineOutput(
    const ElectorStateMachineOutput& output,
    bool                             invokeStateChangeCbInline)
{
    // executed by the cluster-dispatcher or scheduler thread

    // PRECONDITIONS
    BSLMT_MUTEXASSERT_IS_LOCKED_SAFE(&d_lock);

    if (output.cancelTimerEventsFlag()) {
        cancelSchedulerEvents();
    }

    if (output.stateChangedFlag()) {
        if (invokeStateChangeCbInline) {
            dispatchElectorCallback();
        }
        else {
            scheduleElectorCallback();
        }
    }

    scheduleTimer(output.timer());
    emitIOEvent(output);
}

void Elector::dispatchElectorCallback()
{
    // executed by the *CLUSTER* dispatcher thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));

    electorStateInternalCb(d_state.state(),
                           d_state.reason(),
                           d_state.leaderNodeId(),
                           d_state.term(),
                           d_state.age());
}

void Elector::getSchedulerDispatcherThreadHandle()
{
    // executed by the *SCHEDULER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(bslmt::ThreadUtil::invalidHandle() ==
                     d_schedDispThreadId);

    // Note that this method is invoked only in the scheduler thread, but we
    // have no way of asserting that at this point.

    d_schedDispThreadId = bslmt::ThreadUtil::self();
}

void Elector::scheduleElectorCallback()
{
    // executed by the *SCHEDULER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(bslmt::ThreadUtil::self() == d_schedDispThreadId);
    BSLMT_MUTEXASSERT_IS_LOCKED_SAFE(&d_lock);

    // Dispatch the elector state change callback in the associated cluster's
    // dispatcher thread.
    dispatcher()->execute(
        bdlf::BindUtil::bind(&Elector::electorStateInternalCb,
                             this,
                             d_state.state(),
                             d_state.reason(),
                             d_state.leaderNodeId(),
                             d_state.term(),
                             d_state.age()),
        d_cluster_p);
}

void Elector::cancelSchedulerEvents()
{
    // executed by *ANY* thread (including scheduler's dispatcher thread)

    d_scheduler.cancelEvent(&d_initialWaitTimeoutHandle);
    d_scheduler.cancelEvent(&d_randomWaitTimeoutHandle);
    d_scheduler.cancelEvent(&d_electionResultTimeoutHandle);
    d_scheduler.cancelEvent(&d_heartbeatSenderRecurringHandle);
    d_scheduler.cancelEvent(&d_heartbeatCheckerRecurringHandle);
    d_scheduler.cancelEvent(&d_scoutingResultTimeoutHandle);
}

void Elector::scheduleTimer(ElectorTimerEventType::Enum type)
{
    // executed by the *ANY* thread

    if (ElectorTimerEventType::e_NONE == type) {
        return;  // RETURN
    }

    switch (type) {
    case ElectorTimerEventType::e_INITIAL_WAIT_TIMER: {
        bsls::TimeInterval after(bmqsys::Time::nowMonotonicClock());
        after.addMilliseconds(d_config.initialWaitTimeoutMs());

        d_scheduler.scheduleEvent(
            &d_initialWaitTimeoutHandle,
            after,
            bdlf::BindUtil::bind(&Elector::initialWaitTimeoutCb, this));
    } break;  // BREAK

    case ElectorTimerEventType::e_RANDOM_WAIT_TIMER: {
        // random number in a range of [min, max]:
        //     randomNum = min + (rand() % (max - min + 1))
        // where, min = 0 & max = d_config.maxRandomWaitTimeoutMs() for us

        int randomMs = bsl::rand() % (d_config.maxRandomWaitTimeoutMs() + 1);

        bsls::TimeInterval after(bmqsys::Time::nowMonotonicClock());
        after.addMilliseconds(randomMs);

        d_scheduler.scheduleEvent(
            &d_randomWaitTimeoutHandle,
            after,
            bdlf::BindUtil::bind(&Elector::randomWaitTimerCb, this));
    } break;  // BREAK

    case ElectorTimerEventType::e_ELECTION_RESULT_TIMER: {
        // Schedule election timer
        bsls::TimeInterval after(bmqsys::Time::nowMonotonicClock());
        after.addMilliseconds(d_config.electionResultTimeoutMs());

        d_scheduler.scheduleEvent(
            &d_electionResultTimeoutHandle,
            after,
            bdlf::BindUtil::bind(&Elector::electionResultTimeoutCb, this));
    } break;  // BREAK

    case ElectorTimerEventType::e_HEARTBEAT_CHECK_TIMER: {
        bsls::TimeInterval interval;
        interval.setTotalMilliseconds(d_config.heartbeatCheckPeriodMs());

        d_scheduler.scheduleRecurringEvent(
            &d_heartbeatCheckerRecurringHandle,
            interval,
            bdlf::BindUtil::bind(&Elector::heartbeatCheckerRecurringTimerCb,
                                 this));
    } break;  // BREAK

    case ElectorTimerEventType::e_HEARTBEAT_BROADCAST_TIMER: {
        bsls::TimeInterval interval;
        interval.setTotalMilliseconds(d_config.heartbeatBroadcastPeriodMs());

        d_scheduler.scheduleRecurringEvent(
            &d_heartbeatSenderRecurringHandle,
            interval,
            bdlf::BindUtil::bind(&Elector::heartbeatSenderRecurringTimerCb,
                                 this));
    } break;  // BREAK

    case ElectorTimerEventType::e_SCOUTING_RESULT_TIMER: {
        bsls::TimeInterval after(bmqsys::Time::nowMonotonicClock());
        after.addMilliseconds(d_config.scoutingResultTimeoutMs());

        d_scheduler.scheduleEvent(
            &d_scoutingResultTimeoutHandle,
            after,
            bdlf::BindUtil::bind(&Elector::scoutingResultTimeoutCb, this));
    } break;  // BREAK

    case ElectorTimerEventType::e_NONE:
    default: break;  // BREAK
    }
}

void Elector::emitIOEvent(const ElectorStateMachineOutput& output)
{
    // executed by the *ANY* thread

    if (ElectorIOEventType::e_NONE == output.io()) {
        return;  // RETURN
    }

    // Valid 'type' event, which means 'destinationNodeId' must be valid as
    // well.
    BSLS_ASSERT_SAFE(k_INVALID_NODE_ID != output.destination());

    bmqp_ctrlmsg::ElectorMessage message;
    message.term() = output.term();

    switch (output.io()) {
    case ElectorIOEventType::e_ELECTION_PROPOSAL: {
        message.choice().makeElectionProposal();
    } break;  // BREAK

    case ElectorIOEventType::e_ELECTION_RESPONSE: {
        message.choice().makeElectionResponse();
    } break;  // BREAK

    case ElectorIOEventType::e_LEADER_HEARTBEAT: {
        message.choice().makeLeaderHeartbeat();
    } break;  // BREAK

    case ElectorIOEventType::e_LEADERSHIP_CESSION: {
        message.choice().makeLeadershipCessionNotification();
    } break;  // BREAK

    case ElectorIOEventType::e_NODE_UNAVAILABLE: {
        message.choice().makeElectorNodeStatus().isAvailable() = false;
        message.term() = 0;  // Term is not applicable to node unavailability
    } break;                 // BREAK

    case ElectorIOEventType::e_NODE_AVAILABLE: {
        message.choice().makeElectorNodeStatus().isAvailable() = true;
        message.term() = 0;  // Term is not applicable to node availability
    } break;                 // BREAK

    case ElectorIOEventType::e_HEARTBEAT_RESPONSE: {
        message.choice().makeHeartbeatResponse();
    } break;  // BREAK

    case ElectorIOEventType::e_SCOUTING_REQUEST: {
        message.choice().makeScoutingRequest();
    } break;  // BREAK

    case ElectorIOEventType::e_SCOUTING_RESPONSE: {
        // Populate scouting response boolean flag.
        message.choice().makeScoutingResponse().willVote() =
            output.scoutingResponseFlag();
    } break;  // BREAK

    case ElectorIOEventType::e_NONE:
    default: return;  // RETURN
    }

    // Encode the message.
    //
    // 'emitIOEvent' is currently always called while 'd_lock' is held, but
    // that's an implementation side effect, not part of contract.  That's why
    // we create the builder on stack instead of making it a class member.
    bmqp::SchemaEventBuilder builder(d_blobSpPool_p,
                                     bmqp::EncodingType::e_BER,
                                     d_allocator_p);

    int rc = builder.setMessage(message, bmqp::EventType::e_ELECTOR);
    if (0 != rc) {
        BALL_LOG_ERROR << "#ELECTOR_ENCODE_FAILURE "
                       << "Failed to encode elector message: " << message
                       << ", rc: " << rc;
        return;  // RETURN
    }

    // Retrieve the encoded event
    const bsl::shared_ptr<bdlbb::Blob> blob = builder.blob_sp();
    if (k_ALL_NODES_ID == output.destination()) {
        // Broadcast to cluster, using the unicast channel to ensure ordering
        // of events
        d_netCluster_p->writeAll(blob, bmqp::EventType::e_ELECTOR);
    }
    else {
        // Unicast to the specified 'destinationNodeId'.
        NodesMapIter it = d_nodes.find(output.destination());
        if (d_nodes.end() == it) {
            BALL_LOG_ERROR << "#ELECTOR_INVALID_NODEID "
                           << "Invalid nodeId [" << output.destination()
                           << "] specified while trying to emit event ["
                           << output.io() << "].";
            return;  // RETURN
        }

        ClusterNode*              node = it->second;
        bmqt::GenericResult::Enum status =
            node->write(blob, bmqp::EventType::e_ELECTOR);

        if (bmqt::GenericResult::e_SUCCESS != status) {
            BALL_LOG_ERROR << "#ELECTOR_WRITE_ERROR "
                           << "Failed to write to cluster nodeId ["
                           << node->nodeId()
                           << "] while trying to emit event [" << output.io()
                           << "]. Status [" << status << "]";
        }
    }
}

void Elector::initialWaitTimeoutCb()
{
    // executed by the *SCHEDULER* thread

    bslmt::LockGuard<bslmt::Mutex> guard(&d_lock);  // LOCK

    ElectorStateMachineOutput output;
    d_state.applyTimer(&output, ElectorTimerEventType::e_INITIAL_WAIT_TIMER);

    processStateMachineOutput(output, false);
}

void Elector::electionResultTimeoutCb()
{
    // executed by the *SCHEDULER* thread

    bslmt::LockGuard<bslmt::Mutex> guard(&d_lock);  // LOCK

    ElectorStateMachineOutput output;
    d_state.applyTimer(&output,
                       ElectorTimerEventType::e_ELECTION_RESULT_TIMER);

    processStateMachineOutput(output, false);
}

void Elector::randomWaitTimerCb()
{
    // executed by the *SCHEDULER* thread

    bslmt::LockGuard<bslmt::Mutex> guard(&d_lock);  // LOCK

    ElectorStateMachineOutput output;
    d_state.applyTimer(&output, ElectorTimerEventType::e_RANDOM_WAIT_TIMER);

    processStateMachineOutput(output, false);
}

void Elector::heartbeatSenderRecurringTimerCb()
{
    // executed by the *SCHEDULER* thread

    bslmt::LockGuard<bslmt::Mutex> guard(&d_lock);  // LOCK

    ElectorStateMachineOutput output;
    d_state.applyTimer(&output,
                       ElectorTimerEventType::e_HEARTBEAT_BROADCAST_TIMER);

    processStateMachineOutput(output, false);
}

void Elector::heartbeatCheckerRecurringTimerCb()
{
    // executed by the *SCHEDULER* thread

    bslmt::LockGuard<bslmt::Mutex> guard(&d_lock);  // LOCK

    ElectorStateMachineOutput output;
    d_state.applyTimer(&output,
                       ElectorTimerEventType::e_HEARTBEAT_CHECK_TIMER);

    processStateMachineOutput(output, false);
}

void Elector::scoutingResultTimeoutCb()
{
    // executed by the *SCHEDULER* thread

    bslmt::LockGuard<bslmt::Mutex> guard(&d_lock);  // LOCK

    ElectorStateMachineOutput output;
    d_state.applyTimer(&output,
                       ElectorTimerEventType::e_SCOUTING_RESULT_TIMER);

    processStateMachineOutput(output, false);
}

// CREATORS
Elector::Elector(mqbcfg::ElectorConfig&      config,
                 mqbi::Cluster*              cluster,
                 const ElectorStateCallback& callback,
                 bsls::Types::Uint64         initialTerm,
                 BlobSpPool*                 blobSpPool_p,
                 bslma::Allocator*           allocator)
: d_allocator_p(allocator)
, d_blobSpPool_p(blobSpPool_p)
, d_cluster_p(cluster)
, d_netCluster_p(0)
, d_config(config)
, d_scheduler(bsls::SystemClockType::e_MONOTONIC, allocator)
, d_schedDispThreadId(bslmt::ThreadUtil::invalidHandle())
, d_lock()
, d_initialWaitTimeoutHandle()
, d_randomWaitTimeoutHandle()
, d_electionResultTimeoutHandle()
, d_heartbeatSenderRecurringHandle()
, d_heartbeatCheckerRecurringHandle()
, d_scoutingResultTimeoutHandle()
, d_callback(bsl::allocator_arg, allocator, callback)
, d_state(allocator)
, d_nodes(allocator)
, d_previousEventAge(0)  // first state transition's age will be 1
{
    BSLS_ASSERT_SAFE(d_allocator_p);
    BSLS_ASSERT_SAFE(d_blobSpPool_p);
    BSLS_ASSERT_SAFE(d_cluster_p);
    BSLS_ASSERT_SAFE(d_callback);

    d_netCluster_p = &d_cluster_p->netCluster();
    BSLS_ASSERT_SAFE(d_netCluster_p);

    d_state.setTerm(initialTerm);

    if (bmqsys::ThreadUtil::k_SUPPORT_THREAD_NAME) {
        // Per scheduler's contract, it's ok to schedule events before its
        // started.

        d_scheduler.scheduleEvent(
            bsls::TimeInterval(0, 0),  // now
            bdlf::BindUtil::bind(&bmqsys::ThreadUtil::setCurrentThreadName,
                                 "bmqSchedElec"));
    }
}

Elector::~Elector()
{
    BSLS_ASSERT_SAFE(bslmt::ThreadUtil::invalidHandle() ==
                     d_schedDispThreadId);
    BSLS_ASSERT_SAFE(!d_state.isEnabled() &&
                     "Elector must be stopped before destruction");
}

// MANIPULATORS
int Elector::start()
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));

    enum {
        rc_SUCCESS           = 0,
        rc_ALREADY_STARTED   = -1,
        rc_INVALID_NODE_ID   = -2,
        rc_DUPLICATE_NODE_ID = -3,
        rc_QUORUM_TOO_SMALL  = -4,
        rc_QUORUM_TOO_LARGE  = -5,
        rc_MISC_FAILURE      = -6
    };

    bslmt::LockGuard<bslmt::Mutex> guard(&d_lock);  // LOCK

    if (d_state.isEnabled()) {
        return rc_ALREADY_STARTED;  // RETURN
    }

    BALL_LOG_INFO << "Elector Configuration: " << d_config;

    if (k_INVALID_NODE_ID == d_cluster_p->netCluster().selfNodeId()) {
        return rc_INVALID_NODE_ID;  // RETURN
    }

    typedef Cluster::NodesList       NodeList;
    typedef NodeList::const_iterator NodeListType;
    const NodeList&                  nodeList = d_netCluster_p->nodes();

    for (NodeListType nit = nodeList.begin(); nit != nodeList.end(); ++nit) {
        if (k_INVALID_NODE_ID == (*nit)->nodeId()) {
            return rc_INVALID_NODE_ID;  // RETURN
        }

        if (false ==
            d_nodes.insert(bsl::make_pair((*nit)->nodeId(), *nit)).second) {
            return rc_DUPLICATE_NODE_ID;  // RETURN
        }
    }

    if (0 == d_config.quorum()) {
        d_config.quorum() = static_cast<int>(d_nodes.size()) / 2 + 1;
    }
    else {
        if (1 > d_config.quorum()) {
            return rc_QUORUM_TOO_SMALL;  // RETURN
        }

        if (d_config.quorum() > static_cast<int>(d_nodes.size())) {
            return rc_QUORUM_TOO_LARGE;  // RETURN
        }
    }

    int rc = d_scheduler.start();
    if (0 != rc) {
        return rc_MISC_FAILURE + 10 * rc;  // RETURN
    }

    // Schedule an event to capture the threadId of scheduler's dispatcher
    // thread, which can be used in assertions.  Since an elector instance (
    // and thus, the scheduler) can be started/stopped multiple times,
    // scheduler's threadId can change every time, and thus we capture is
    // every time scheduler is started.

    d_scheduler.scheduleEvent(
        bsls::TimeInterval(0, 0),  // now
        bdlf::BindUtil::bind(&Elector::getSchedulerDispatcherThreadHandle,
                             this));

    // Enable the state machine.

    d_state.enable(d_cluster_p->netCluster().selfNodeId(),
                   d_config.quorum(),
                   d_nodes.size(),
                   (d_config.heartbeatMissCount() *
                    d_config.heartbeatBroadcastPeriodMs()));

    dispatchElectorCallback();  // state change is implicit when state machine
                                // is enabled

    // Schedule initial wait timer and return success
    bsls::TimeInterval after(bmqsys::Time::nowMonotonicClock());
    after.addMilliseconds(d_config.initialWaitTimeoutMs());

    d_scheduler.scheduleEvent(
        &d_initialWaitTimeoutHandle,
        after,
        bdlf::BindUtil::bind(&Elector::initialWaitTimeoutCb, this));
    return rc_SUCCESS;
}

void Elector::stop()
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));

    bslmt::LockGuard<bslmt::Mutex> guard(&d_lock);  // LOCK

    if (!d_state.isEnabled()) {
        return;  // RETURN
    }

    cancelSchedulerEvents();
    d_state.disable();
    dispatchElectorCallback();  // state change is implicit when state machine
                                // is disabled
    d_nodes.clear();

    // Let your peers know that you are not participating in election anymore.
    // This also means that current leader (if any) has lost this node's
    // support, and may relinquish its leadership if it loses quorum.  Note
    // that if this node is going down, then peer electors will also get
    // NodeDown notification from the net layer (after the below
    // NODE_UNAVAILABLE event).  If this node is not going down, and it is just
    // desired not to participate in the election, then this notification is
    // needed to inform the peers.

    emitIOEvent(
        ElectorStateMachineOutput(ElectorStateMachine::k_INVALID_TERM,
                                  ElectorIOEventType::e_NODE_UNAVAILABLE,
                                  ElectorTimerEventType::e_NONE,
                                  k_ALL_NODES_ID));

    d_scheduler.stop();
    d_schedDispThreadId = bslmt::ThreadUtil::invalidHandle();
}

void Elector::processEvent(const bmqp::Event& event, ClusterNode* source)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));

    BSLS_ASSERT_SAFE(event.isValid());
    BSLS_ASSERT_SAFE(event.isElectorEvent());

    bslmt::LockGuard<bslmt::Mutex> guard(&d_lock);  // LOCK

    bmqp_ctrlmsg::ElectorMessage electorMsg;

    int rc = event.loadElectorEvent(&electorMsg);
    if (0 != rc) {
        BALL_LOG_ERROR << "#CORRUPTED_MESSAGE "
                       << "Failed to load elector message, rc: " << rc;
        return;  // RETURN
    }

    ElectorIOEventType::Enum incomingIOEvent = electorSchemaMsgToIOEventType(
        electorMsg);
    if (ElectorIOEventType::e_NONE == incomingIOEvent) {
        BALL_LOG_ERROR << "#ELECTOR_INVALID_MESSAGE "
                       << "Invalid elector message choice type: "
                       << electorMsg.choice().selectionId();
        return;  // RETURN
    }

    ElectorStateMachineOutput output;

    if (ElectorIOEventType::e_NODE_UNAVAILABLE == incomingIOEvent ||
        ElectorIOEventType::e_NODE_AVAILABLE == incomingIOEvent) {
        d_state.applyAvailability(&output, incomingIOEvent, source->nodeId());
    }
    else if (ElectorIOEventType::e_SCOUTING_RESPONSE == incomingIOEvent) {
        d_state.applyScout(&output,
                           electorMsg.choice().scoutingResponse().willVote(),
                           electorMsg.term(),
                           source->nodeId());
    }
    else {
        d_state.applyIOEvent(&output,
                             incomingIOEvent,
                             electorMsg.term(),
                             source->nodeId());
    }

    processStateMachineOutput(output, true);
}

void Elector::processNodeStatus(ClusterNode* node, bool isAvailable)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));

    bslmt::LockGuard<bslmt::Mutex> guard(&d_lock);  // LOCK

    ElectorStateMachineOutput output;
    d_state.applyAvailability(&output,
                              isAvailable
                                  ? ElectorIOEventType::e_NODE_AVAILABLE
                                  : ElectorIOEventType::e_NODE_UNAVAILABLE,
                              node->nodeId());

    processStateMachineOutput(output, true);
}

int Elector::processCommand(mqbcmd::ElectorResult*        electorResult,
                            const mqbcmd::ElectorCommand& command)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));

    if (command.isSetTunableValue()) {
        const mqbcmd::SetTunable& tunable = command.setTunable();
        if (bdlb::StringRefUtil::areEqualCaseless(tunable.name(), "QUORUM")) {
            if (!tunable.value().isTheIntegerValue() ||
                tunable.value().theInteger() < 0) {
                bmqu::MemOutStream output;
                output << "The QUORUM tunable must be a non-negative integer, "
                          "but instead the following was specified: "
                       << tunable.value();
                electorResult->makeError();
                electorResult->error().message() = output.str();
                return -1;  // RETURN
            }

            // Lock for modifying state
            bslmt::LockGuard<bslmt::Mutex> guard(&d_lock);  // LOCK
            mqbcmd::TunableConfirmation&   tunableConfirmation =
                electorResult->makeTunableConfirmation();
            tunableConfirmation.name() = "Quorum";
            tunableConfirmation.oldValue().makeTheInteger(d_config.quorum());
            tunableConfirmation.newValue().makeTheInteger(
                tunable.value().theInteger());
            d_config.quorum() = tunable.value().theInteger();
            d_state.setQuorum(tunable.value().theInteger());
            return 0;  // RETURN
        }

        bmqu::MemOutStream output;
        output << "Unknown tunable name '" << tunable.name() << "'";
        electorResult->makeError();
        electorResult->error().message() = output.str();
        return -1;  // RETURN
    }
    else if (command.isGetTunableValue()) {
        const bsl::string& tunable = command.getTunable().name();
        if (bdlb::StringRefUtil::areEqualCaseless(tunable, "QUORUM")) {
            mqbcmd::Tunable& tunableObj = electorResult->makeTunable();
            tunableObj.name()           = "Quorum";
            tunableObj.value().makeTheInteger(d_config.quorum());
            return 0;  // RETURN
        }

        bmqu::MemOutStream output;
        output << "Unsupported tunable '" << tunable << "': Issue the "
               << "LIST_TUNABLES command for the list of supported tunables.";
        electorResult->makeError();
        electorResult->error().message() = output.str();
        return -1;  // RETURN
    }
    else if (command.isListTunablesValue()) {
        mqbcmd::Tunables& tunables = electorResult->makeTunables();
        tunables.tunables().resize(tunables.tunables().size() + 1);
        mqbcmd::Tunable& tunable = tunables.tunables().back();
        tunable.name()           = "QUORUM";
        tunable.value().makeTheInteger(d_config.quorum());
        tunable.description() = "non-negative integer count of the number of"
                                " peers required to have consensus";
        return 0;  // RETURN
    }

    bmqu::MemOutStream output;
    output << "Unknown command '" << command << "'";
    electorResult->makeError();
    electorResult->error().message() = output.str();
    return -1;
}

}  // close package namespace
}  // close enterprise namespace
