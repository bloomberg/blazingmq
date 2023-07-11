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

// mwcsys_mocktime.cpp                                                -*-C++-*-
#include <mwcsys_mocktime.h>

#include <mwcscm_version.h>
// MWC
#include <mwcsys_time.h>

// BDE
#include <bdlf_memfn.h>

namespace BloombergLP {
namespace mwcsys {

// --------------
// class MockTime
// --------------

MockTime::MockTime()
: d_realtimeClock(0, 0)
, d_monotonicClock(0, 0)
, d_highResTimer(0)
{
    mwcsys::Time::initialize(
        bdlf::MemFnUtil::memFn(&MockTime::realtimeClock, this),
        bdlf::MemFnUtil::memFn(&MockTime::monotonicClock, this),
        bdlf::MemFnUtil::memFn(&MockTime::highResTimer, this));
}

MockTime::~MockTime()
{
    mwcsys::Time::shutdown();
}

MockTime& MockTime::setRealTimeClock(const bsls::TimeInterval& value)
{
    d_realtimeClock = value;
    return *this;
}

MockTime& MockTime::setMonotonicClock(const bsls::TimeInterval& value)
{
    d_monotonicClock = value;
    return *this;
}

MockTime& MockTime::setHighResTimer(bsls::Types::Int64 value)
{
    d_highResTimer = value;
    return *this;
}

MockTime& MockTime::advanceRealTimeClock(const bsls::TimeInterval& offset)
{
    d_realtimeClock += offset;
    return *this;
}

MockTime& MockTime::advanceMonotonicClock(const bsls::TimeInterval& offset)
{
    d_monotonicClock += offset;
    return *this;
}

MockTime& MockTime::advanceHighResTimer(bsls::Types::Int64 offset)
{
    d_highResTimer += offset;
    return *this;
}
MockTime& MockTime::reset()
{
    d_realtimeClock  = bsls::TimeInterval(0, 0);
    d_monotonicClock = bsls::TimeInterval(0, 0);
    d_highResTimer   = 0;

    return *this;
}

bsls::TimeInterval MockTime::realtimeClock() const
{
    return d_realtimeClock;
}

bsls::TimeInterval MockTime::monotonicClock() const
{
    return d_monotonicClock;
}

bsls::Types::Int64 MockTime::highResTimer() const
{
    return d_highResTimer;
}

}  // close package namespace
}  // close enterprise namespace
