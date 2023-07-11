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

// mwcu_atomicstate.cpp                                               -*-C++-*-
#include <mwcu_atomicstate.h>

#include <mwcscm_version.h>
// BDE
#include <bslmt_threadutil.h>

namespace BloombergLP {
namespace mwcu {

bool AtomicState::process()
{
    int result = d_value.add(e_PROCESS);

    // Spin while locked result == e_PROCESS + e_LOCK
    // Note that if result == (e_PROCESS + e_PROCESS + e_LOCK), then the state
    // is NOT locked because this is the second call to 'process' after
    // _successful_ previous call(s).  Successful 'process' means that the
    // state cannot be locked ever.  The 'tryLock' has failed.

    while (result == (e_PROCESS + e_LOCK)) {
        bslmt::ThreadUtil::yield();
        result = d_value;
    }

    // 'process' can be called second time.

    // Return 'true' if not cancelled ((result & e_CANCEL) == 0)
    return (result & e_CANCEL) == 0;
}

}  // close package namespace
}  // close enterprise namespace
