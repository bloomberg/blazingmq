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

// bmqsys_threadutil.cpp                                              -*-C++-*-
#include <bmqsys_threadutil.h>

#include <bmqscm_version.h>

#include <bmqu_tlsbool.h>

// BDE
#include <ball_log.h>
#include <bsl_cstring.h>
#include <bsl_iostream.h>
#include <bsl_ostream.h>
#include <bslma_default.h>
#include <bslmt_threadutil.h>
#include <bsls_annotation.h>
#include <bsls_performancehint.h>
#include <bsls_platform.h>

namespace BloombergLP {
namespace bmqsys {

namespace {
const char k_LOG_CATEGORY[] = "BMQSYS.THREADUTIL";
}  // close unnamed namespace

// -----------------
// struct ThreadUtil
// -----------------

bslmt::ThreadAttributes ThreadUtil::defaultAttributes()
{
    bslmt::ThreadAttributes attributes;
    return attributes;
}

void ThreadUtil::setCurrentThreadName(const bsl::string& value)
{
    bslmt::ThreadUtil::setThreadName(value);
}

void ThreadUtil::setCurrentThreadNameOnce(const bsl::string& value)
{
#ifdef BSLS_PLATFORM_CMP_CLANG
    // Suppress "exit-time-destructor" warning on Clang by qualifying the
    // static variable 's_named' with Clang-specific attribute.
    [[clang::no_destroy]]
#endif
    static bmqu::TLSBool s_named(false, true);

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!s_named)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        setCurrentThreadName(value);
        s_named = true;
    }
}

}  // close package namespace
}  // close enterprise namespace
