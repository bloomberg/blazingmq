// Copyright 2026 Bloomberg Finance L.P.
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

// bmqu_atomicgate.cpp                                                -*-C++-*-
#include <bmqu_atomicgate.h>

#include <bmqscm_version.h>

// BDE
#include <bslmt_threadutil.h>

namespace BloombergLP {
namespace bmqu {

// ----------------
// class AtomicGate
// ----------------

// MANIPULATORS
void AtomicGate::closeAndDrain()
{
    int result = d_value.add(e_CLOSE);

    BSLS_ASSERT_SAFE(result & e_CLOSE);
    // Do not support more than one writer

    // Spin while locked result > e_CLOSE

    while (result > e_CLOSE) {
        bslmt::ThreadUtil::yield();
        result = d_value;
    }
}

}  // close package namespace
}  // close enterprise namespace
