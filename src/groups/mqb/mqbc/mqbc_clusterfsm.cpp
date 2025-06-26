// Copyright 2020-2023 Bloomberg Finance L.P.
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

// mqbc_clusterfsm.cpp                                                -*-C++-*-
#include <mqbc_clusterfsm.h>

#include <mqbscm_version.h>
// BDE
#include <bsla_annotations.h>

namespace BloombergLP {
namespace mqbc {

// ------------------------
// class ClusterFSMObserver
// ------------------------

// CREATORS
ClusterFSMObserver::~ClusterFSMObserver()
{
    // NOTHING
}

// MANIPULATORS
void ClusterFSMObserver::onUnknown()
{
    // NOTHING
}

void ClusterFSMObserver::onHealedLeader()
{
    // NOTHING
}

void ClusterFSMObserver::onHealedFollower()
{
    // NOTHING
}

void ClusterFSMObserver::onStopping()
{
    // NOTHING
}

// ================
// class ClusterFSM
// ================

// MANIPULATORS
ClusterFSM& ClusterFSM::registerObserver(ClusterFSMObserver* observer)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(observer);

    d_observers.insert(observer);
    BALL_LOG_DEBUG << "ClusterFSM: Registered 1 new observer (" << observer
                   << "). Total number of observers is now "
                   << d_observers.size();

    return *this;
}

ClusterFSM& ClusterFSM::unregisterObserver(ClusterFSMObserver* observer)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(observer);

    d_observers.erase(observer);
    BALL_LOG_DEBUG << "ClusterFSM: Unregistered 1 observer. (" << observer
                   << "). Total number of observers is now "
                   << d_observers.size();

    return *this;
}

void ClusterFSM::popEventAndProcess(ClusterFSMArgsSp& eventsQueue)
{
    while (!eventsQueue->empty()) {
        State::Enum oldState = d_state;
        Transition  transition =
            d_stateTable.transition(oldState, eventsQueue->front().first);

        // Transition state
        d_state = static_cast<State::Enum>(transition.first);

        BALL_LOG_INFO << "Cluster FSM on Event '" << eventsQueue->front().first
                      << "', transition: State '" << oldState
                      << "' =>  State '" << d_state << "'";

        // Perform action
        (d_actions.*(transition.second))(eventsQueue);

        // Notify observers
        if (d_state != oldState) {
            switch (d_state) {
            case State::e_UNKNOWN: {
                for (ObserversSetIter it = d_observers.begin();
                     it != d_observers.end();
                     ++it) {
                    (*it)->onUnknown();
                }
                break;  // BREAK
            }
            case State::e_FOL_HEALED: {
                for (ObserversSetIter it = d_observers.begin();
                     it != d_observers.end();
                     ++it) {
                    (*it)->onHealedFollower();
                }
                break;  // BREAK
            }
            case State::e_LDR_HEALED: {
                for (ObserversSetIter it = d_observers.begin();
                     it != d_observers.end();
                     ++it) {
                    (*it)->onHealedLeader();
                }
                break;  // BREAK
            }
            case State::e_STOPPING: {
                for (ObserversSetIter it = d_observers.begin();
                     it != d_observers.end();
                     ++it) {
                    (*it)->onStopping();
                }
                break;  // BREAK
            }
            case State::e_NUM_STATES: {
                BSLS_ASSERT_SAFE(false && "Code unreachable by design");
                break;  // BREAK
            }
            case State::e_FOL_HEALING: BSLA_FALLTHROUGH;
            case State::e_LDR_HEALING_STG1: BSLA_FALLTHROUGH;
            case State::e_LDR_HEALING_STG2: BSLA_FALLTHROUGH;
            case State::e_STOPPED: BSLA_FALLTHROUGH;
            default: {
                break;  // BREAK
            }
            }
        }
        eventsQueue->pop();
    }
    // There are no successive events to be processed so its safe to exit.
}

}  // close package namespace
}  // close enterprise namespace
