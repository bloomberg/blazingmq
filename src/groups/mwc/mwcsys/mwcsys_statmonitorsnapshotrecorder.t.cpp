// Copyright 2023 Bloomberg Finance L.P.
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

// mwcsys_statmonitorsnapshotrecorder.t.cpp                           -*-C++-*-
#include <mwcsys_statmonitorsnapshotrecorder.h>

// MWC
#include <mwcsys_mocktime.h>
#include <mwcsys_time.h>
#include <mwcu_memoutstream.h>

// BDE
#include <bdlpcre_regex.h>
#include <bsl_cstddef.h>
#include <bsl_string.h>
#include <bslmt_turnstile.h>
#include <bsls_stopwatch.h>

// TEST DRIVER
#include <mwctst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {

/// Match the specified `str` against the specified `pattern` using the
/// specified `allocator` and return true on success, false otherwise.  The
/// matching is performed with the flags `k_FLAGS_CASELESS` and
/// `k_FLAG_DOTMATCHESALL` (as described in `bdlpcre_regex.h`).  The
/// behavior is undefined unless `pattern` represents a valid pattern.
static bool regexMatch(const bslstl::StringRef& str,
                       const char*              pattern,
                       bslma::Allocator*        allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(allocator);

    bsl::string    error(allocator);
    size_t         errorOffset;
    bdlpcre::RegEx regex(allocator);

    int rc = regex.prepare(&error,
                           &errorOffset,
                           pattern,
                           bdlpcre::RegEx::k_FLAG_CASELESS |
                               bdlpcre::RegEx::k_FLAG_DOTMATCHESALL);
    BSLS_ASSERT_SAFE(rc == 0);
    BSLS_ASSERT_SAFE(regex.isPrepared() == true);

    rc = regex.match(str.data(), str.length());

    return rc == 0;
}

int doWork_cpuIntensive()
{
    const int k_1K = 1000;

    int                   multiplier = 1;
    volatile unsigned int sum        = 0;
    for (int i = 0; i < (k_1K * multiplier); ++i) {
        volatile int a = 50 * 9873 / 3;
        sum += a;

        volatile int b = a * (a ^ (~0U));
        sum += b;

        volatile int c = (a * (b ^ (~0U))) % 999983;
        sum += c;
    }

    return sum;
}

void doWork(double rate, double duration)
{
    bsls::Stopwatch timer;
    timer.start();

    bslmt::Turnstile turnstile(rate);

    int sum = 0;
    while (true) {
        turnstile.waitTurn();
        if (timer.elapsedTime() >= duration) {
            break;  // BREAK
        }
        int tmpSum = doWork_cpuIntensive();
        sum        = tmpSum * sum;
    }
}

}  // close unnamed namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_usageExample()
// ------------------------------------------------------------------------
// BREATHING TEST
//
// Concerns:
//   Ensure that the usage example compiles and works.
//
// Plan:
//   1 Run the example's code and verify that the monitor's behavior and
//     state are as expected.
//
// Testing:
//   Usage example
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("USAGE EXAMPLE");

    mwcsys::Time::initialize(s_allocator_p);

    const bsl::string k_HEADER("Cluster (testCluster): ", s_allocator_p);

    // Begin testing
    PV("Valued constructor");
    {
        mwcsys::StatMonitorSnapshotRecorder statRecorder(k_HEADER);

        // Perform some intensive work - step 1
        doWork(7500,  // rate
               3);    // duration

        ASSERT_GT(statRecorder.totalElapsed(), 0);

        // Log stats for step 1 (resets the stat recorder)
        mwcu::MemOutStream os1(s_allocator_p);
        statRecorder.print(os1, "EXPENSIVE OPERATION - STEP 1");
        PVV(os1.str());

        ASSERT_EQ(regexMatch(os1.str(),
                             "Cluster \\(testCluster\\): "
                             "'EXPENSIVE OPERATION - STEP 1'.*"
                             "\\(Total elapsed: \\d+\\.\\d+.*\\).*"
                             "WALL TIME.*: \\d+\\.?\\d*.*"
                             "CPU USER AVG.*: (0|\\d+\\.?\\d*.*) %.*"
                             "CPU SYSTEM AVG.*: (0|\\d+\\.?\\d*.*) %.*"
                             "CPU ALL AVG.*: (0|\\d+\\.?\\d*.*) %.*"
                             "VOLUNTARY CONTEXT SWITCHES.*: \\d+.*",
                             s_allocator_p),
                  true);

        // Perform some intensive work - step 2
        doWork(10000,  // rate
               3);     // duration

        // Log stats for step 2 (resets the stat recorder)
        mwcu::MemOutStream os2(s_allocator_p);
        statRecorder.print(os2, "EXPENSIVE OPERATION - STEP 2");
        PVV(os2.str());

        ASSERT_EQ(regexMatch(os2.str(),
                             "Cluster \\(testCluster\\): "
                             "'EXPENSIVE OPERATION - STEP 2'.*"
                             "\\(Total elapsed: \\d+\\.\\d+.*\\).*"
                             "CPU USER AVG.*: (0|\\d+\\.?\\d*.*) %.*"
                             "CPU SYSTEM AVG.*: (0|\\d+\\.?\\d*.*) %.*"
                             "CPU ALL AVG.*: (0|\\d+\\.?\\d*.*) %.*"
                             "VOLUNTARY CONTEXT SWITCHES.*: \\d+.*",
                             s_allocator_p),
                  true);

        // Finally, assert stats have changed
        ASSERT_NE(os1.str(), os2.str());
    }

    PV("Copy constructor");
    {
        mwcsys::StatMonitorSnapshotRecorder statRecorderTemp(k_HEADER);
        mwcsys::StatMonitorSnapshotRecorder statRecorder(statRecorderTemp,
                                                         s_allocator_p);

        // Perform some intensive work - step 1
        doWork(7500,  // rate
               3);    // duration

        // Log stats for step 1 (resets the stat recorder)
        mwcu::MemOutStream os1(s_allocator_p);
        statRecorder.print(os1, "EXPENSIVE OPERATION - STEP 1");
        PVV(os1.str());

        ASSERT_EQ(regexMatch(os1.str(),
                             "Cluster \\(testCluster\\): "
                             "'EXPENSIVE OPERATION - STEP 1'.*"
                             "\\(Total elapsed: \\d+\\.\\d+.*\\).*"
                             "WALL TIME.*: \\d+\\.\\d+.*.*"
                             "CPU USER AVG.*: (0|\\d+\\.?\\d*.*) %.*"
                             "CPU SYSTEM AVG.*: (0|\\d+\\.?\\d*.*) %.*"
                             "CPU ALL AVG.*: (0|\\d+\\.?\\d*.*) %.*"
                             "VOLUNTARY CONTEXT SWITCHES.*: \\d+.*",
                             s_allocator_p),
                  true);

        // Perform some intensive work - step 2
        doWork(10000,  // rate
               3);     // duration

        // Log stats for step 2 (resets the stat recorder)
        mwcu::MemOutStream os2(s_allocator_p);
        statRecorder.print(os2, "EXPENSIVE OPERATION - STEP 2");
        PVV(os2.str());

        ASSERT_EQ(regexMatch(os2.str(),
                             "Cluster \\(testCluster\\): "
                             "'EXPENSIVE OPERATION - STEP 2'.*"
                             "\\(Total elapsed: \\d+\\.\\d+.*\\).*"
                             "WALL TIME.*: \\d+\\.\\d+.*.*"
                             "CPU USER AVG.*: (0|\\d+\\.?\\d*.*) %.*"
                             "CPU SYSTEM AVG.*: (0|\\d+\\.?\\d*.*) %.*"
                             "CPU ALL AVG.*: (0|\\d+\\.?\\d*.*) %.*"
                             "VOLUNTARY CONTEXT SWITCHES.*: \\d+.*",
                             s_allocator_p),
                  true);

        // Finally, assert stats have changed
        ASSERT_NE(os1.str(), os2.str());
    }

    mwcsys::Time::shutdown();
}

static void test2_totalElapsed()
// ------------------------------------------------------------------------
// TOTAL ELAPSED
//
// Concerns:
//   Ensure that the totalElapsed accessor returns the right value.
//
// Plan:
//   1 Manipulate the time by using custom time accessor with mwcsys::Time
//
// Testing:
//   'bsls::Types::Int64 totalElapsed() const'
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("TOTAL ELAPSED");

    mwcsys::MockTime mockTime;

    mwcsys::StatMonitorSnapshotRecorder obj("HEADER");

    ASSERT_EQ(obj.totalElapsed(), 0);

    // 'advance' time
    mockTime.advanceHighResTimer(123);

    ASSERT_EQ(obj.totalElapsed(), 123);
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(mwctst::TestHelper::e_DEFAULT);

    s_ignoreCheckDefAlloc = true;
    // 'snapshot' in 'mwcsys::StatMonitor' passes a string using default
    // alloc.

    switch (_testCase) {
    case 0:
    case 2: test2_totalElapsed(); break;
    case 1: test1_usageExample(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        s_testStatus = -1;
    } break;
    }

    TEST_EPILOG(mwctst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
