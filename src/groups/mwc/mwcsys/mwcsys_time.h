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

// mwcsys_time.h                                                      -*-C++-*-
#ifndef INCLUDED_MWCSYS_TIME
#define INCLUDED_MWCSYS_TIME

//@PURPOSE: Provide a pluggable functional interface to system clocks.
//
//@CLASSES:
//   mwcsys::Time: namespace for pluggable interface to system clocks
//
//@DESCRIPTION: This component provides a 'struct', 'mwcsys::Time', in which
// are defined a series of static methods for retrieving the current system
// time from the currently installed mechanism.  This component provides access
// to monotonic clock, real-time (wall) clock and a high resolution timer.  The
// mechanism to retrieve system clock and the timer can be overridden, which is
// useful while testing components which rely on system clock or timer.

// MWC

// BDE
#include <bsl_functional.h>
#include <bsls_timeinterval.h>
#include <bsls_types.h>

namespace BloombergLP {
namespace mwcsys {

// ===========
// struct Time
// ===========

/// Provide a platform-neutral pluggable functional interface to system
/// clocks.
struct Time {
    // TYPES

    /// Signature of the callback for the real time and monotonic clocks.
    typedef bsl::function<bsls::TimeInterval()> SystemTimeCb;

    /// Signature of the callback for the high resolution timer.
    typedef bsl::function<bsls::Types::Int64()> HighResolutionTimeCb;

    // CLASS METHODS

    /// Initialize the utilities with platform-provided mechanism to provide
    /// system clocks and high resolution timer.  This method only needs to
    /// be called once before any other method, but can be called multiple
    /// times provided that for each call to `initialize` there is a
    /// corresponding call to `shutdown`.  Use the optionally specified
    /// `allocator` for any memory allocation, or the `global` allocator if
    /// none is provided.  Note that specifying the allocator is provided
    /// for test drivers only, and therefore users should let it default to
    /// the global allocator.
    static void initialize(bslma::Allocator* allocator = 0);

    /// Initialize the utilities with the specified `realTimeClockCb`,
    /// `monotonicClockCb` and `highResTimeCb` to provide system clocks and
    /// high resolution timer respectively.  This method only needs to be
    /// called once before any other method, but can be called multiple
    /// times provided that for each call to `initialize` there is a
    /// corresponding call to `shutdown`.  Use the optionally specified
    /// `allocator` for any memory allocation, or the `global` allocator if
    /// none is provided.  Note that specifying the allocator is provided
    /// for test drivers only, and therefore users should let it default to
    /// the global allocator.
    static void initialize(const SystemTimeCb&         realTimeClockCb,
                           const SystemTimeCb&         monotonicClockCb,
                           const HighResolutionTimeCb& highResTimeCb,
                           bslma::Allocator*           allocator = 0);

    /// Pendant operation of the `initialize` one to cleanup the utilities.
    /// The number of calls to `shutdown` must equal the number of calls to
    /// `initialize`, without corresponding `shutdown` calls, to fully
    /// destroy the objects.  It is safe to call `initialize` after calling
    /// `shutdown`.  The behaviour is undefined if `shutdown` is called
    /// without `initialize` first being called.
    static void shutdown();

    /// Return the `TimeInterval` value representing the current system time
    /// according to the currently installed real-time clock.  The behavior
    /// is undefined unless one of the flavors of `initialize()` has been
    /// called prior to calling this method.
    static bsls::TimeInterval nowRealtimeClock();

    /// Return the `TimeInterval` value representing the current system time
    /// according to the currently installed monotonic clock.  The behavior
    /// is undefined unless one of the flavors of `initialize()` has been
    /// called prior to calling this method.
    static bsls::TimeInterval nowMonotonicClock();

    /// Return the value representing the current high resolution time
    /// according to the currently installed high resolution timer.  The
    /// behavior is undefined unless one of the flavors of `initialize()`
    /// has been called prior to calling this method.
    static bsls::Types::Int64 highResolutionTimer();
};

}  // close package namespace
}  // close enterprise namespace

#endif
