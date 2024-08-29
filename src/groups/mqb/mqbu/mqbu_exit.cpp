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

// mqbu_exit.cpp                                                      -*-C++-*-
#include <mqbu_exit.h>

#include <mqbscm_version.h>
// BDE
#include <bdlb_print.h>
#include <bdlb_string.h>
#include <bdls_processutil.h>
#include <bsl_c_signal.h>  // For 'kill'
#include <bsl_csignal.h>
#include <bsl_cstdlib.h>
#include <bsl_ostream.h>
#include <bsls_atomic.h>

namespace BloombergLP {
namespace mqbu {

// ---------------
// struct ExitCode
// ---------------

bsl::ostream& ExitCode::print(bsl::ostream&  stream,
                              ExitCode::Enum value,
                              int            level,
                              int            spacesPerLevel)
{
    bdlb::Print::indent(stream, level, spacesPerLevel);
    stream << ExitCode::toAscii(value);

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

const char* ExitCode::toAscii(ExitCode::Enum value)
{
#define CASE(X)                                                               \
    case e_##X: return #X;

    switch (value) {
        CASE(SUCCESS)
        CASE(COMMAND_LINE)
        CASE(CONFIG_GENERATION)
        CASE(TASK_INITIALIZE)
        CASE(BENCH_START)
        CASE(APP_INITIALIZE)
        CASE(RUN)
        CASE(QUEUEID_FULL)
        CASE(SUBQUEUEID_FULL)
        CASE(RECOVERY_FAILURE)
        CASE(STORAGE_OUT_OF_SYNC)
        CASE(UNSUPPORTED_SCENARIO)
        CASE(MEMORY_LIMIT)
        CASE(REQUESTED)

    default: return "(* UNKNOWN *)";
    }

#undef CASE
}

bool ExitCode::fromAscii(ExitCode::Enum* out, const bslstl::StringRef& str)
{
#define CHECKVALUE(M)                                                         \
    if (bdlb::String::areEqualCaseless(toAscii(ExitCode::e_##M),              \
                                       str.data(),                            \
                                       static_cast<int>(str.length()))) {     \
        *out = ExitCode::e_##M;                                               \
        return true;                                                          \
    }

    CHECKVALUE(SUCCESS)
    CHECKVALUE(COMMAND_LINE)
    CHECKVALUE(CONFIG_GENERATION)
    CHECKVALUE(TASK_INITIALIZE)
    CHECKVALUE(BENCH_START)
    CHECKVALUE(APP_INITIALIZE)
    CHECKVALUE(RUN)
    CHECKVALUE(QUEUEID_FULL)
    CHECKVALUE(SUBQUEUEID_FULL)
    CHECKVALUE(RECOVERY_FAILURE)
    CHECKVALUE(STORAGE_OUT_OF_SYNC)
    CHECKVALUE(UNSUPPORTED_SCENARIO)
    CHECKVALUE(MEMORY_LIMIT)
    CHECKVALUE(REQUESTED)

    // Invalid string
    return false;

#undef CHECKVALUE
}

// ---------------
// struct ExitUtil
// ---------------

void ExitUtil::terminate(ExitCode::Enum reason)
{
    static bsls::AtomicBool s_terminated(false);
    if (s_terminated.testAndSwap(false, true) != false) {
        // Terminated was already initiated, no need to call 'exit' again, as
        // this seems to be leading to issues when concurrently called from
        // multiple threads.
        return;  // RETURN
    }

    BALL_LOG_ERROR << "#EXIT "
                   << "Terminating BMQBRKR with error '" << reason << "'";
    bsl::exit(reason);
}

void ExitUtil::shutdown(ExitCode::Enum reason)
{
    BALL_LOG_ERROR << "#SHUTDOWN "
                   << "Shutting down BMQBRKR with error '" << reason << "'";
    // We can't use 'bsl::raise(SIGINT)' because it notifies the current
    // thread, while we want to target the main-thread (i.e. the process) as
    // this is the one having registered the signal handler.
    kill(bdls::ProcessUtil::getProcessId(), SIGINT);
}

}  // close package namespace
}  // close enterprise namespace
