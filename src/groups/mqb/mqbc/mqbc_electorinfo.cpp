// Copyright 2019-2023 Bloomberg Finance L.P.
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

// mqbc_electorinfo.cpp                                               -*-C++-*-
#include <mqbc_electorinfo.h>

#include <mqbscm_version.h>
// MQB
#include <mqbnet_cluster.h>
#include <mqbscm_version.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>

// BDE
#include <bsla_annotations.h>
#include <bsls_assert.h>
#include <bsls_types.h>

namespace BloombergLP {
namespace mqbc {

// ------------------------------
// struct ElectorInfoLeaderStatus
// ------------------------------

bsl::ostream&
ElectorInfoLeaderStatus::print(bsl::ostream&                 stream,
                               ElectorInfoLeaderStatus::Enum value,
                               int                           level,
                               int                           spacesPerLevel)
{
    bdlb::Print::indent(stream, level, spacesPerLevel);
    stream << ElectorInfoLeaderStatus::toAscii(value);

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

const char*
ElectorInfoLeaderStatus::toAscii(ElectorInfoLeaderStatus::Enum value)
{
#define CASE(X)                                                               \
    case e_##X: return #X;

    switch (value) {
        CASE(UNDEFINED)
        CASE(PASSIVE)
        CASE(ACTIVE)
    default: return "(* UNKNOWN *)";
    }

#undef CASE
}

// -------------------------
// class ElectorInfoObserver
// -------------------------

// CREATORS
ElectorInfoObserver::~ElectorInfoObserver()
{
    // NOTHING
}

void ElectorInfoObserver::onClusterLeader(
    BSLA_UNUSED mqbnet::ClusterNode* node,
    BSLA_UNUSED ElectorInfoLeaderStatus::Enum status)
{
    // NOTHING
}

// -----------------
// class ElectorInfo
// -----------------

// MANIPULATORS
//   (virtual: mqbc::ClusterFSMObserver)
void ElectorInfo::onHealedLeader()
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_electorState == mqbnet::ElectorState::e_LEADER);

    BALL_LOG_INFO << "#ELECTOR_INFO: onHealedLeader()"
                  << ", LSN = " << d_leaderMessageSequence << ".";

    onSelfActiveLeader();
}

void ElectorInfo::onHealedFollower()
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_electorState == mqbnet::ElectorState::e_FOLLOWER);

    BALL_LOG_INFO << "#ELECTOR_INFO: onHealedFollower()"
                  << ", leader = " << d_leaderNode_p->nodeDescription()
                  << ", LSN = " << d_leaderMessageSequence << ".";

    setLeaderStatus(mqbc::ElectorInfoLeaderStatus::e_ACTIVE);
}

// MANIPULATORS
ElectorInfo& ElectorInfo::registerObserver(ElectorInfoObserver* observer)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(observer);

    BALL_LOG_INFO << "#ELECTOR_INFO: Registered 1 new observer.";

    d_observers.insert(observer);
    return *this;
}

ElectorInfo& ElectorInfo::unregisterObserver(ElectorInfoObserver* observer)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(observer);

    BALL_LOG_INFO << "#ELECTOR_INFO: Unregistered 1 observer.";

    d_observers.erase(observer);
    return *this;
}

ElectorInfo& ElectorInfo::setLeaderStatus(ElectorInfoLeaderStatus::Enum value)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        (d_leaderNode_p && (ElectorInfoLeaderStatus::e_UNDEFINED != value)) ||
        (!d_leaderNode_p && (ElectorInfoLeaderStatus::e_UNDEFINED == value)));
    // It is **prohibited** to set leader status directly from e_UNDEFINED to
    // e_ACTIVE.
    if (d_leaderStatus == ElectorInfoLeaderStatus::e_UNDEFINED) {
        BSLS_ASSERT_SAFE(value != ElectorInfoLeaderStatus::e_ACTIVE);
    }

    BALL_LOG_INFO << "#ELECTOR_INFO: leader: "
                  << (d_leaderNode_p ? d_leaderNode_p->nodeDescription()
                                     : "** NULL **")
                  << ", LSN: " << d_leaderMessageSequence
                  << ", transitioning status from " << d_leaderStatus << " to "
                  << value;

    // We update internal state *before* notifying observers.
    ElectorInfoLeaderStatus::Enum oldStatus = d_leaderStatus;
    d_leaderStatus                          = value;

    if (value == ElectorInfoLeaderStatus::e_ACTIVE &&
        oldStatus != ElectorInfoLeaderStatus::e_ACTIVE) {
        // We only notify the observers if we are the leader transitioning to
        // *active* state, and only the first time this happens.
        for (ObserversSet::iterator it = d_observers.begin();
             it != d_observers.end();
             ++it) {
            (*it)->onClusterLeader(d_leaderNode_p, d_leaderStatus);
        }
    }

    return *this;
}

ElectorInfo& ElectorInfo::setElectorInfo(mqbnet::ElectorState::Enum    state,
                                         bsls::Types::Uint64           term,
                                         mqbnet::ClusterNode*          node,
                                         ElectorInfoLeaderStatus::Enum status)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        (node && (ElectorInfoLeaderStatus::e_UNDEFINED != status)) ||
        (!node && (ElectorInfoLeaderStatus::e_UNDEFINED == status)));
    // It is **prohibited** to set leader status directly from e_UNDEFINED to
    // e_ACTIVE.
    if (d_leaderStatus == ElectorInfoLeaderStatus::e_UNDEFINED) {
        BSLS_ASSERT_SAFE(status != ElectorInfoLeaderStatus::e_ACTIVE);
    }

    mqbnet::ClusterNode* oldLeader = d_leaderNode_p;

    BALL_LOG_INFO << "#ELECTOR_INFO: transition old leader: "
                  << (oldLeader ? oldLeader->nodeDescription() : "** NULL **")
                  << ", status: " << d_leaderStatus
                  << ", LSN: " << d_leaderMessageSequence << " to new leader: "
                  << (node ? node->nodeDescription() : "** NULL **")
                  << ", status: " << status << ", electorTerm: " << term
                  << ". Transition elector state from " << d_electorState
                  << " to " << state << ".";

    d_electorState = state;
    setLeaderNode(node);
    d_leaderStatus = status;
    setElectorTerm(term);

    if (!node || !oldLeader) {
        // We could have been called because of a change of state, which may
        // not imply a change of leader (for example from dormant to
        // candidate).  Here, we only notify the observers when the leader is
        // gone; notifying of the leader elected is taken care of by the
        // 'setLeaderStatus' method; unless there is a new leader.
        for (ObserversSet::iterator it = d_observers.begin();
             it != d_observers.end();
             ++it) {
            (*it)->onClusterLeader(d_leaderNode_p, status);
        }
    }

    return *this;
}

void ElectorInfo::onSelfActiveLeader()
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_electorState == mqbnet::ElectorState::e_LEADER);
    BSLS_ASSERT_SAFE(d_leaderNode_p);

    BALL_LOG_INFO << "#ELECTOR_INFO: onSelfActiveLeader(): "
                  << "leader = " << d_leaderNode_p->nodeDescription()
                  << ", LSN = " << d_leaderMessageSequence << ".";

    setLeaderStatus(mqbc::ElectorInfoLeaderStatus::e_ACTIVE);
}

}  // close package namespace
}  // close enterprise namespace
