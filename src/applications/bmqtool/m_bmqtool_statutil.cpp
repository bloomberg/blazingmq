// Copyright 2024 Bloomberg Finance L.P.
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

// BMQTOOL
#include <m_bmqtool_parameters.h>
#include <m_bmqtool_statutil.h>

// BDE
#include <bdlt_currenttime.h>
#include <bsls_timeutil.h>
#include <bsls_types.h>

namespace BloombergLP {
namespace m_bmqtool {

bsls::Types::Int64
StatUtil::getNowAsNs(ParametersLatency::Value resolutionTimer)
{
    if (resolutionTimer == ParametersLatency::e_EPOCH) {
        bsls::TimeInterval now = bdlt::CurrentTime::now();
        return now.totalNanoseconds();  // RETURN
    }
    else if (resolutionTimer == ParametersLatency::e_HIRES) {
        return bsls::TimeUtil::getTimer();  // RETURN
    }
    else {
        BSLS_ASSERT_OPT(false && "Unsupported latency mode");
    }

    return 0;
}

}  // close package namespace
}  // close enterprise namespace
