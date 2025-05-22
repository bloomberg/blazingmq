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

// mqbc_partitionfsm.cpp                                              -*-C++-*-
#include <mqbc_partitionfsm.h>

#include <mqbscm_version.h>
// BDE
#include <bsla_annotations.h>

namespace BloombergLP {
namespace mqbc {

// ==================
// class PartitionFSM
// ==================

// MANIPULATORS
PartitionFSM& PartitionFSM::registerObserver(PartitionFSMObserver* observer)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(observer);

    d_observers.insert(observer);
    BALL_LOG_DEBUG << "PartitionFSM: Registered 1 new observer (" << observer
                   << "). Total number of observers is now "
                   << d_observers.size();

    return *this;
}

PartitionFSM& PartitionFSM::unregisterObserver(PartitionFSMObserver* observer)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(observer);

    d_observers.erase(observer);
    BALL_LOG_DEBUG << "PartitionFSM: Unregistered 1 observer (" << observer
                   << "). Total number of observers is now "
                   << d_observers.size();

    return *this;
}

void PartitionFSM::applyEvent(
    const bsl::shared_ptr<bsl::queue<EventWithData> >& eventsQueueSp)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!eventsQueueSp->empty());

    EventWithData eventWithData = eventsQueueSp->front();
    BSLS_ASSERT_SAFE(!eventWithData.second.empty());
    const int partitionId = eventWithData.second[0].partitionId();

    State::Enum oldState   = d_state;
    Transition  transition = d_stateTable.transition(oldState,
                                                    eventWithData.first);

    // Transition state
    d_state = static_cast<State::Enum>(transition.first);

    if (eventWithData.first != PartitionStateTableEvent::e_RECOVERY_DATA &&
        eventWithData.first != PartitionStateTableEvent::e_LIVE_DATA) {
        BALL_LOG_INFO << "Partition FSM for Partition [" << partitionId
                      << "] on Event '" << eventWithData.first
                      << "', transition: State '" << oldState
                      << "' =>  State '" << d_state << "'";
    }

    // Perform action
    PartitionFSMArgsSp argsSp(new (*d_allocator_p)
                                  PartitionFSMArgs(eventsQueueSp.get()),
                              d_allocator_p);
    (d_actions.*(transition.second))(argsSp);

    // Notify observers
    if (d_state != oldState) {
        switch (d_state) {
        case State::e_UNKNOWN: {
            for (ObserversSetIter it = d_observers.begin();
                 it != d_observers.end();
                 ++it) {
                (*it)->onTransitionToUnknown(partitionId, oldState);
            }

            break;  // BREAK
        }
        case State::e_PRIMARY_HEALED: {
            for (ObserversSetIter it = d_observers.begin();
                 it != d_observers.end();
                 ++it) {
                (*it)->onTransitionToPrimaryHealed(partitionId, oldState);
            }

            break;  // BREAK
        }
        case State::e_REPLICA_HEALED: {
            for (ObserversSetIter it = d_observers.begin();
                 it != d_observers.end();
                 ++it) {
                (*it)->onTransitionToReplicaHealed(partitionId, oldState);
            }

            break;  // BREAK
        }
        case State::e_NUM_STATES: {
            BSLS_ASSERT_SAFE(false && "Code unreachable by design");

            break;  // BREAK
        }
        case State::e_PRIMARY_HEALING_STG1: BSLA_FALLTHROUGH;
        case State::e_PRIMARY_HEALING_STG2: BSLA_FALLTHROUGH;
        case State::e_REPLICA_HEALING: BSLA_FALLTHROUGH;
        default: {
            break;  // BREAK
        }
        }
    }

    eventsQueueSp->pop();
    if (!eventsQueueSp->empty()) {
        applyEvent(eventsQueueSp);
    }
    else {
        // NOTHING
        // There are no successive events to be processed so its safe to exit.
    }
}

}  // close package namespace
}  // close enterprise namespace
