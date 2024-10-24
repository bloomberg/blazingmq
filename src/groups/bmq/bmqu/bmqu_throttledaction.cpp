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

// bmqu_throttledaction.cpp                                           -*-C++-*-
#include <bmqu_throttledaction.h>

#include <bmqscm_version.h>
// BDE
#include <bdlt_timeunitratio.h>

namespace BloombergLP {
namespace bmqu {

// ----------------------------
// struct ThrottledActionParams
// ----------------------------

ThrottledActionParams::ThrottledActionParams(int intervalMs,
                                             int maxCountPerInterval)
: d_intervalNano(static_cast<bsls::Types::Int64>(
      intervalMs * bdlt::TimeUnitRatio::k_NANOSECONDS_PER_MILLISECOND))
, d_maxCountPerInterval(maxCountPerInterval)
, d_countSinceLastReset(0)
, d_lastResetTime(bsls::TimeUtil::getTimer())  // We want a fixed-point origin
                                               // against which to compare
                                               // subsequent invocations of
                                               // 'bsls::TimeUtil::getTimer'
{
    // NOTHING
}

}  // close package namespace
}  // close enterprise namespace
