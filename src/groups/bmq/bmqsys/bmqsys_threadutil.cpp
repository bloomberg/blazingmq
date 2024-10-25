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
#include <bsls_annotation.h>
#include <bsls_performancehint.h>
#include <bsls_platform.h>

// Linux
#if defined(BSLS_PLATFORM_OS_LINUX)
#include <sys/prctl.h>
#endif

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

// LINUX
// -----
#if defined(BSLS_PLATFORM_OS_LINUX)

const bool ThreadUtil::k_SUPPORT_THREAD_NAME = true;

void ThreadUtil::setCurrentThreadName(const bsl::string& value)
{
    int rc = prctl(PR_SET_NAME, value.c_str(), 0, 0, 0);
    // We should use 'modern' APIs: pthread_setname_no(pthread_self()).  But
    // Bloomberg is a bit old; API was added in glibc 2.12, and we have 2.5.
    if (rc != 0) {
        BALL_LOG_SET_CATEGORY(k_LOG_CATEGORY);
        BALL_LOG_ERROR << "Failed to set thread name " << "[name: '" << value
                       << "'" << ", rc: " << rc << ", strerr: '"
                       << bsl::strerror(rc) << "']";
    }
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

// UNSUPPORTED_PLATFORMS
// ---------------------
#else

const bool ThreadUtil::k_SUPPORT_THREAD_NAME = false;

void ThreadUtil::setCurrentThreadName(
    BSLS_ANNOTATION_UNUSED const bsl::string& value)
{
    // NOT AVAILABLE

    static_cast<void>(k_LOG_CATEGORY);  // suppress unused variable warning
}

void ThreadUtil::setCurrentThreadNameOnce(
    BSLS_ANNOTATION_UNUSED const bsl::string& value)
{
    // NOT AVAILABLE
}

#endif

}  // close package namespace
}  // close enterprise namespace
