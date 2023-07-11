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

// mqbu_statetable.h                                                  -*-C++-*-
#ifndef INCLUDED_MQBU_STATETABLE
#define INCLUDED_MQBU_STATETABLE

//@PURPOSE: Provide a state table for FSM use.
//
//@CLASSES:
//  mqbu::StateTable: State table for FSM use
//
//@DESCRIPTION: 'mqbu::StateTable' is a state table for FSM use.
//
/// Thread Safety
///-------------
// The 'mqbu::StateTable' object is not thread safe.

// MQB

// BDE
#include <bsl_utility.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace mqbu {

// ================
// class StateTable
// ================

/// This class is a state table for FSM use.
template <size_t NUM_STATES, size_t NUM_EVENTS, typename ACTION>
class StateTable {
  public:
    // TYPES

    /// Pair of <nextState, action>
    typedef bsl::pair<int, ACTION> Transition;

  private:
    // DATA
    Transition d_transitions[NUM_STATES][NUM_EVENTS];
    // Matrix (State, Event) -> Transition of all possible
    // transitions in this state table.

  public:
    // CREATORS

    /// This ctor initializes all elements to no state change, specified
    /// `action`.
    StateTable(ACTION action = ACTION());

    // MANIPULATORS

    /// Configure this table to have the specified `transition` when the
    /// specified `state` receives the specified input `event`.
    void configure(int state, int event, Transition transition);

    // ACCESSORS

    /// Return the transition for when the specified `state` receives the
    /// specified input `event`.
    Transition transition(int state, int event) const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ----------------
// class StateTable
// ----------------

// CREATORS
template <size_t NUM_STATES, size_t NUM_EVENTS, typename ACTION>
inline StateTable<NUM_STATES, NUM_EVENTS, ACTION>::StateTable(ACTION action)
{
    BSLS_ASSERT_SAFE(sizeof(d_transitions) / sizeof(d_transitions[0]) >=
                     static_cast<int>(NUM_STATES));

    for (size_t i = 0; i < NUM_STATES; i++) {
        BSLS_ASSERT_SAFE(sizeof(d_transitions[0]) >=
                         static_cast<int>(NUM_EVENTS));

        for (size_t j = 0; j < NUM_EVENTS; j++) {
            d_transitions[i][j] = Transition(i, action);
        }
    }
}

// MANIPULATORS
template <size_t NUM_STATES, size_t NUM_EVENTS, typename ACTION>
inline void
StateTable<NUM_STATES, NUM_EVENTS, ACTION>::configure(int        state,
                                                      int        event,
                                                      Transition transition)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(0 <= state && state < static_cast<int>(NUM_STATES));
    BSLS_ASSERT_SAFE(0 <= event && event < static_cast<int>(NUM_EVENTS));

    d_transitions[state][event] = transition;
}

// ACCESSORS
template <size_t NUM_STATES, size_t NUM_EVENTS, typename ACTION>
inline typename StateTable<NUM_STATES, NUM_EVENTS, ACTION>::Transition
StateTable<NUM_STATES, NUM_EVENTS, ACTION>::transition(int state,
                                                       int event) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(0 <= state && state < static_cast<int>(NUM_STATES));
    BSLS_ASSERT_SAFE(0 <= event && event < static_cast<int>(NUM_EVENTS));

    return d_transitions[state][event];
}

}  // close package namespace
}  // close enterprise namespace

#endif
