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

// mwcex_bdlmteventschedulerexecutor.cpp                              -*-C++-*-
#include <mwcex_bdlmteventschedulerexecutor.h>

#include <mwcscm_version.h>
// BDE
#include <bdlmt_eventscheduler.h>

namespace BloombergLP {
namespace mwcex {

// ---------------------------------
// class BdlmtEventSchedulerExecutor
// ---------------------------------

// CLASS DATA
const bsls::TimeInterval BdlmtEventSchedulerExecutor::k_DEFAULT_TIME_POINT(0);

// MANIPULATORS
void BdlmtEventSchedulerExecutor::post(const bsl::function<void()>& f) const
{
    // PRECONDITIONS
    BSLS_ASSERT(f);

    d_context_p->scheduleEvent(d_timePoint, f);
}

}  // close package namespace
}  // close enterprise namespace
