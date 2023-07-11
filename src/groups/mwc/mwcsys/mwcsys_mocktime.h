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

// mwcsys_mocktime.h                                                  -*-C++-*-
#ifndef INCLUDED_MWCSYS_MOCKTIME
#define INCLUDED_MWCSYS_MOCKTIME

//@PURPOSE: Provide a mock of the time accessors usable with 'mwcsys::Time'.
//
//@CLASSES:
//   mwcsys::MockTime: mock utility of the 'mwcsys::Time' accessors
//
//@DESCRIPTION: 'mwcsys::MockTime' provides a utility that can be used with
//'mwcsys::Time' to control the time.
//
/// Usage Example
///-------------
// The following example illustrates typical intended usage of this component.
//
//..
//  MockTime mockTime;
//
//  mockTime.advanceHighResTimer(10);
//
//  mwcsys::Time::shutdown();
//..

// MWC

// BDE
#include <bsls_keyword.h>
#include <bsls_timeinterval.h>
#include <bsls_types.h>

namespace BloombergLP {
namespace mwcsys {

// ==============
// class MockTime
// ==============

/// A mock utility of the time accessors usable with `mwcsys::Time`.
class MockTime {
  private:
    // DATA
    bsls::TimeInterval d_realtimeClock;
    bsls::TimeInterval d_monotonicClock;
    bsls::Types::Int64 d_highResTimer;

  private:
    // NOT IMPLEMENTED
    MockTime(const MockTime&) BSLS_KEYWORD_DELETED;
    MockTime& operator=(const MockTime&) BSLS_KEYWORD_DELETED;
    // Copy constructor and assignment operator not implemented (because
    // MockTime, for convenience, registers itself to 'mwcsys::Time' in
    // its constructor).

  public:
    // CREATORS

    /// Default constructor - and register this object to the mwcsys::Time
    MockTime();

    /// Destructor
    ~MockTime();

    // MANIPULATORS
    MockTime& setRealTimeClock(const bsls::TimeInterval& value);
    MockTime& setMonotonicClock(const bsls::TimeInterval& value);

    /// Set the corresponding time to the specified `value` and return a
    /// reference offering modifiable access to this object.
    MockTime& setHighResTimer(bsls::Types::Int64 value);

    MockTime& advanceRealTimeClock(const bsls::TimeInterval& offset);
    MockTime& advanceMonotonicClock(const bsls::TimeInterval& offset);

    /// Advance (i.e., increment) the corresponding time by the specified
    /// `offset` and return a reference offering modifiable access to this
    /// object.
    MockTime& advanceHighResTimer(bsls::Types::Int64 offset);

    /// Reset all time counters to 0 and return a reference offering
    /// modifiable access to this object.
    MockTime& reset();

    // ACCESSORS
    bsls::TimeInterval realtimeClock() const;
    bsls::TimeInterval monotonicClock() const;

    /// Return the value representing the corresponding time.
    bsls::Types::Int64 highResTimer() const;
};

}  // close package namespace
}  // close enterprise namespace

#endif
