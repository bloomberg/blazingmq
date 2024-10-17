// Copyright 2017-2023 Bloomberg Finance L.P.
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

// bmqu_throttledaction.h                                             -*-C++-*-
#ifndef INCLUDED_BMQU_THROTTLEDACTION
#define INCLUDED_BMQU_THROTTLEDACTION

//@PURPOSE: Provide macros and utilities to throttle an action.
//
//@CLASSES:
//  bmqu::ThrottledActionParams: struct holding the state of a throttleable
//                               action.
//
//@MACROS:
//  BMQU_THROTTLEDACTION_THROTTLE(P, ACTION):                   Reference below
//  BMQU_THROTTLEDACTION_THROTTLE_WITH_RESET(P, ACTION, RESET): Reference below
//  BMQU_THROTTLEDACTION_THROTTLE_NO_RESET(P, ACTION):          Reference below
//
//@DESCRIPTION: This component provides preprocessor macros and utility
// functions to throttle a user specified action.  A throttled action is a
// piece of code that will be executed at most N times per a certain interval
// of time.  Some user code can be executed when the interval is reset, with
// the number of skipped executions of the action.
//
// Note that this component is particularly useful for example to throttle a
// log message created based on occurrences of events that are not under the
// control of the application's code.  For example, a service could use it to
// log bad packets received from a client application.  Since the client
// application is not under our control and may misbehave forever, this
// component can be used to make sure that the bad packets are recorded in the
// logs but not more than a number of times over a given period of time.
//
/// Thread-Safety
///-------------
// All macros defined in this component are thread-safe, and can be invoked
// concurrently by multiple threads.
//
// Note that this component uses the 'bsls::TimeUtil' timer component, and per
// documentation, this component is thread-safe only once the one-time
// initialization 'bsls::TimeUtil::initialize' has been called.
//
/// Macro Reference
///----------------
// This component defines the following macros:
//..
//  BMQU_THROTTLEDACTION_THROTTLE(P, ACTION)
//      // Execute the specified 'ACTION' having the specified throttling
//      // parameters 'P' and use the default reset function.  The default
//      // reset function will print a BALL_LOG_INFO having the name of the
//      // variable 'P' (to identify the throttled action) and the number of
//      // items that have been skipped since the last reset.
//
//  BMQU_THROTTLEDACTION_THROTTLE_WITH_RESET(P, ACTION, RESET)
//      // Execute the specified 'ACTION' having the specified throttling
//      // parameters 'P' and execute the specify 'RESET' code when the
//      // throttling gets reset.  Inside the context of the 'RESET' code, the
//      // '_numSkipped' variable represents the number of times 'ACTION' has
//      // been skipped due to throttling since the last reset.
//
//  BMQU_THROTTLEDACTION_THROTTLE_NO_RESET(P, ACTION)
//      // Execute the specified 'ACTION' having the specified throttling
//      // parameters 'P' and don't do anything for reset.
//..
//
/// Macro Usage
///-----------
// The following code fragments illustrate the standard pattern of macro usage.
//..
//  bmqu::ThrottledActionParams myThrottledLog(5000, 3); // No more than 3 logs
//                                                       //  in a 5s timeframe
//  // Note that the ThrottledActionParams should be a class member, or a
//  // static variable.
//
//  BMQU_THROTTLEDACTION_THROTTLE(myThrottledLog,
//                                BALL_LOG_ERROR << "An error occurred");
//..

// BDE
#include <ball_log.h>
#include <bsls_atomic.h>
#include <bsls_timeutil.h>
#include <bsls_types.h>

namespace BloombergLP {
namespace bmqu {

// ============================
// struct ThrottledActionParams
// ============================

/// This struct represent the parameters and state associated to a throttled
/// action.  Its members should not be manipulated by the user, but only
/// internally by the macros.
struct ThrottledActionParams {
  public:
    // PUBLIC DATA
    bsls::Types::Int64 d_intervalNano;  // Interval window (in nanoseconds) of
                                        // the throttling.

    int d_maxCountPerInterval;
    // Maximum number of times per
    // 'interval' frame the 'ACTION' can be
    // executed before getting skipped due
    // to throttling.

    bsls::AtomicInt d_countSinceLastReset;
    // Number of times 'ACTION' has been
    // executed since the last reset.

    bsls::AtomicInt64 d_lastResetTime;  // Time of last reset

    // CREATORS

    /// Helper constructor to create a `bmqu::ThrottledActionParams` object
    /// defining an action that should be executed at most the specified
    /// `maxCountPerInterval` times during the specified `intervalMs`
    /// milliseconds time frame.
    explicit ThrottledActionParams(int intervalMs          = 3000,
                                   int maxCountPerInterval = 1);
};

// ======
// MACROS
// ======

#define BMQU_THROTTLEDACTION_THROTTLE(P, ACTION)                              \
    BMQU_THROTTLEDACTION_THROTTLE_INTERNAL(P, ACTION, {                       \
        BALL_LOG_INFO << "THROTTLED_ACTION - resetting throttling "           \
                      << "counter '" << #P << "' (" << _numSkipped            \
                      << " actions skipped).";                                \
    })
// Throttle the specified 'ACTION' using the specified parameters 'P' and
// use a standard 'RESET' action that displays a BALL_LOG_INFO of the
// number of times 'ACTION' got skipped due to throttling.

#define BMQU_THROTTLEDACTION_THROTTLE_WITH_RESET(P, ACTION, RESET)            \
    BMQU_THROTTLEDACTION_THROTTLE_INTERNAL(P, ACTION, RESET)
// Throttle the specified 'ACTION' using the specified parameters 'P' and
// use the specified 'RESET' code.  The variable '_numSkipped' can be used
// inside the 'RESET' code to reference the number of times 'ACTION' got
// skipped due to throttling.

#define BMQU_THROTTLEDACTION_THROTTLE_NO_RESET(P, ACTION)                     \
    BMQU_THROTTLEDACTION_THROTTLE_INTERNAL(P, ACTION, {})
// Throttle the specified 'ACTION' using the specified parameters 'P' and
// use a void reset action.

#define BMQU_THROTTLEDACTION_THROTTLE_INTERNAL(P, ACTION, RESET)              \
    {                                                                         \
        const bsls::Types::Int64 _now =                                       \
            BloombergLP::bsls::TimeUtil::getTimer();                          \
                                                                              \
        if ((_now - P.d_lastResetTime) >= P.d_intervalNano) {                 \
            int                _numSkipped = P.d_countSinceLastReset;         \
            bsls::Types::Int64 _resetTime  = P.d_lastResetTime;               \
            if (P.d_lastResetTime.testAndSwap(_resetTime, _now) ==            \
                _resetTime) {                                                 \
                P.d_countSinceLastReset -= _numSkipped;                       \
                if (_numSkipped > P.d_maxCountPerInterval) {                  \
                    _numSkipped -= P.d_maxCountPerInterval;                   \
                    {                                                         \
                        RESET;                                                \
                    }                                                         \
                }                                                             \
            }                                                                 \
        }                                                                     \
                                                                              \
        if (++P.d_countSinceLastReset <= P.d_maxCountPerInterval) {           \
            ACTION;                                                           \
        }                                                                     \
        else {                                                                \
            /* Action was skipped because of throttling */                    \
        }                                                                     \
    }
// Throttle the specified 'ACTION' using the specified parameters 'P' and
// the specified reset action 'RESET'.

}  // close package namespace
}  // close enterprise namespace

#endif
