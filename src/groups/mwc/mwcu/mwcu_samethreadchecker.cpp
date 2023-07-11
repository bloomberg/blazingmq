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

// mwcu_samethreadchecker.cpp                                         -*-C++-*-
#include <mwcu_samethreadchecker.h>

#include <mwcscm_version.h>
// BDE
#include <bslmt_threadutil.h>
#include <bsls_types.h>

namespace BloombergLP {
namespace mwcu {

namespace {

// CONSTANTS
const bsls::Types::Uint64 k_INVALID_THREAD_ID = bslmt::ThreadUtil::idAsUint64(
    bslmt::ThreadUtil::handleToId(bslmt::ThreadUtil::invalidHandle()));

}  // close unnamed namespace

// -----------------------
// class SameThreadChecker
// -----------------------

// CREATORS
SameThreadChecker::SameThreadChecker() BSLS_KEYWORD_NOEXCEPT
: d_threadId(k_INVALID_THREAD_ID)
{
    // NOTHING
}

// MANIPULATORS
bool SameThreadChecker::inSameThread() BSLS_KEYWORD_NOEXCEPT
{
    const bsls::Types::Uint64 selfThreadId =
        bslmt::ThreadUtil::selfIdAsUint64();

    const bsls::Types::Uint64 threadId =
        d_threadId.testAndSwapAcqRel(k_INVALID_THREAD_ID, selfThreadId);

    return threadId == selfThreadId || threadId == k_INVALID_THREAD_ID;
}

void SameThreadChecker::reset() BSLS_KEYWORD_NOEXCEPT
{
    d_threadId.storeRelease(k_INVALID_THREAD_ID);
}

}  // close package namespace
}  // close enterprise namespace
