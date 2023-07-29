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

// mwcu_atomicvalidator.cpp                                           -*-C++-*-
#include <mwcu_atomicvalidator.h>

#include <mwcscm_version.h>
// BDE
#include <bslmt_threadutil.h>

namespace BloombergLP {
namespace mwcu {

// ---------------------
// class AtomicValidator
// ---------------------

// CREATORS
AtomicValidator::AtomicValidator(bool isValid)
: d_count(static_cast<int>(!isValid))  // d_count = 0 if 'isValid'
                                       //           1 otherwise
{
    // NOTHING
}

// MANIPULATORS
void AtomicValidator::reset()
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(d_count == 1);

    d_count = 0;
}

void AtomicValidator::invalidate()
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(!(d_count & 1));  // Can only be invalidated once

    // Since the first bit of 'd_count' isn't set, incrementing it will set
    // the bit
    ++d_count;

    // Wait until all acquisitions have been released
    while (d_count > 1) {
        bslmt::ThreadUtil::yield();
    }
}

// --------------------------
// class AtomicValidatorGuard
// --------------------------

// MANIPULATORS
AtomicValidator* AtomicValidatorGuard::release()
{
    // Clear the validator
    AtomicValidator* v = d_validator_p;
    d_validator_p      = 0;

    if (BSLS_PERFORMANCEHINT_PREDICT_LIKELY(d_isAcquired)) {
        // Clear acquisition flag
        d_isAcquired = false;
        return v;  // RETURN
    }

    // 1. Validator, if any, has not been successfully acquired; or
    // 2. this guard has been released
    BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

    return 0;
}

}  // close package namespace
}  // close enterprise namespace
