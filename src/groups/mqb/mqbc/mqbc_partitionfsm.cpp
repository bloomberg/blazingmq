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
#include <ball_logthrottle.h>
#include <bdlt_timeunitratio.h>
#include <bsla_annotations.h>

namespace BloombergLP {
namespace mqbc {

namespace {

const int k_MAX_INSTANT_MESSAGES = 10;
// Maximum messages logged with throttling in a short period of time.

const bsls::Types::Int64 k_NS_PER_MESSAGE =
    bdlt::TimeUnitRatio::k_NANOSECONDS_PER_SECOND / k_MAX_INSTANT_MESSAGES;
// Time interval between messages logged with throttling.

#define BMQ_LOGTHROTTLE_INFO                                                  \
    BALL_LOGTHROTTLE_INFO(k_MAX_INSTANT_MESSAGES, k_NS_PER_MESSAGE)           \
        << "[THROTTLED] "

}  // close unnamed namespace

// ==================
// class PartitionFSM
// ==================

// PRIVATE MANIPULATORS
void PartitionFSM::notifyObservers(int partitionId, State::Enum oldState)
{
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
        case State::e_REPLICA_WAITING: BSLA_FALLTHROUGH;
        case State::e_REPLICA_HEALING: BSLA_FALLTHROUGH;
        case State::e_STOPPED: BSLA_FALLTHROUGH;
        default: {
            break;  // BREAK
        }
        }
    }
}

void PartitionFSM::processEvent(EventWithData& event)
{
    // executed by *QUEUE_DISPATCHER* thread associated with 'partitionId'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!d_isFrozen);
    BSLS_ASSERT_SAFE(!event.d_data.empty());

    const int   partitionId = event.d_data[0].partitionId();
    State::Enum oldState;

    event.d_isFreezeRequested = false;

    if (event.d_chainResumeIndex > 0) {
        // Resuming a frozen chain — state was already transitioned.
        oldState = d_frozenOldState;

        BALL_LOG_INFO << "Partition FSM for Partition [" << partitionId
                      << "] resuming frozen chain, State '" << d_state << "'";
    }
    else {
        oldState = d_state;
    }

    Transition transition = d_stateTable.transition(oldState, event.d_event);

    if (event.d_chainResumeIndex == 0) {
        // Fresh event — transition state and log.
        d_state = static_cast<State::Enum>(transition.first);

        if (event.d_event == PartitionStateTableEvent::e_RECOVERY_DATA ||
            event.d_event == PartitionStateTableEvent::e_LIVE_DATA) {
            BMQ_LOGTHROTTLE_INFO
                << "Partition FSM for Partition [" << partitionId
                << "] on Event '" << event.d_event << "', transition: State '"
                << oldState << "' =>  State '" << d_state << "'";
        }
        else {
            BALL_LOG_INFO << "Partition FSM for Partition [" << partitionId
                          << "] on Event '" << event.d_event
                          << "', transition: State '" << oldState
                          << "' =>  State '" << d_state << "'";
        }
    }

    (d_actions.*(transition.second))(event);

    if (event.d_isFreezeRequested) {
        // An action in the chain requested a freeze.  executeChain has
        // already saved the resume index in event.d_chainResumeIndex.
        // The state has already been set to the target; no events will be
        // processed until unfreeze.  The event remains at the front of
        // d_eventsQueue (the caller must not pop it).
        d_isFrozen       = true;
        d_frozenOldState = oldState;
        return;  // RETURN
    }

    // Notify observers
    notifyObservers(partitionId, oldState);
}

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

void PartitionFSM::enqueueEvent(const EventWithData& event)
{
    // executed by *QUEUE_DISPATCHER* thread associated with 'partitionId'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!event.d_data.empty());

    d_eventsQueue.push(event);
    if (d_eventsQueue.size() > 1) {
        // There is already an ongoing processing, so just return.
        return;  // RETURN
    }

    while (!d_eventsQueue.empty() && !d_isFrozen) {
        processEvent(d_eventsQueue.front());
        if (d_isFrozen) {
            // The event stays in the queue as a sentinel so that nested
            // enqueueEvent calls during unfreeze see size() > 1 and do
            // not process events inline.
            break;
        }
        d_eventsQueue.pop();
    }
}

void PartitionFSM::unfreeze()
{
    // executed by *QUEUE_DISPATCHER* thread associated with 'partitionId'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_isFrozen);
    BSLS_ASSERT_SAFE(!d_eventsQueue.empty());

    d_isFrozen = false;

    // The frozen event is at the front of the queue with
    // d_chainResumeIndex > 0.  processEvent detects this and resumes the
    // chain instead of starting a new transition.  Subsequent queued
    // events are drained in the same loop.
    while (!d_eventsQueue.empty() && !d_isFrozen) {
        processEvent(d_eventsQueue.front());
        if (d_isFrozen) {
            break;
        }
        d_eventsQueue.pop();
    }
}

}  // close package namespace
}  // close enterprise namespace
