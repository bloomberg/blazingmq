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

// bmqsys_time.t.cpp                                                  -*-C++-*-
#include <bmqsys_time.h>

#include <bmqu_printutil.h>

// BDE
#include <bdlf_bind.h>
#include <bsl_functional.h>
#include <bsls_systemtime.h>
#include <bsls_timeinterval.h>
#include <bsls_timeutil.h>
#include <bsls_types.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// BENCHMARKING LIBRARY
#ifdef BSLS_PLATFORM_OS_LINUX
#include <benchmark/benchmark.h>
#endif

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {

/// A struct to provide controllable timer functions.
struct TestClock {
    // DATA
    bsls::TimeInterval d_realtimeClock;
    bsls::TimeInterval d_monotonicClock;

    /// The value to return by the next call of the corresponding timer
    /// function.
    bsls::Types::Int64 d_highResTimer;

    // MANIPULATORS
    bsls::TimeInterval realtimeClock() { return d_realtimeClock; }

    bsls::TimeInterval monotonicClock() { return d_monotonicClock; }

    bsls::Types::Int64 highResTimer() { return d_highResTimer; }
};

}  // close unnamed namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_breathingTest()
// ------------------------------------------------------------------------
// BREATHING TEST
//
// Concerns:
//   Exercise the basic functionality of the component.
//
// Plan:
//   - Create a TestClock, initialize the bmqsys::TimeUtil with it, and
//     verify each timer function returns the properly set value.
//
// Testing:
//   Basic functionality
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("BREATHING TEST");

    TestClock testClock;
    bmqsys::Time::initialize(
        bdlf::BindUtil::bind(&TestClock::realtimeClock, &testClock),
        bdlf::BindUtil::bind(&TestClock::monotonicClock, &testClock),
        bdlf::BindUtil::bind(&TestClock::highResTimer, &testClock));

    PV("Testing real time clock");
    {
        testClock.d_realtimeClock.setTotalSeconds(5);
        ASSERT_EQ(bsls::TimeInterval(5, 0), bmqsys::Time::nowRealtimeClock());

        testClock.d_realtimeClock.setTotalSeconds(0);
        ASSERT_EQ(bsls::TimeInterval(0, 0), bmqsys::Time::nowRealtimeClock());
    }

    PV("Testing monotonic clock");
    {
        testClock.d_monotonicClock.setTotalSeconds(5);
        ASSERT_EQ(bsls::TimeInterval(5, 0), bmqsys::Time::nowMonotonicClock());

        testClock.d_monotonicClock.setTotalSeconds(0);
        ASSERT_EQ(bsls::TimeInterval(0, 0), bmqsys::Time::nowMonotonicClock());
    }

    PV("Testing high resolution timer");
    {
        testClock.d_highResTimer = 1000000;
        ASSERT_EQ(1000000, bmqsys::Time::highResolutionTimer());

        testClock.d_highResTimer = 0;
        ASSERT_EQ(0, bmqsys::Time::highResolutionTimer());
    }

    bmqsys::Time::shutdown();
}

static void test2_defaultInitializeShutdown()
// ------------------------------------------------------------------------
// DEFAULT INITIALIZE & SHUTDOWN
//
// Concerns:
//   Ensure that platform-provided mechanism to provide system clocks and
//   high resolution timer initialize and shutdown.
//
// Plan:
//   - Verify that initialization and shutdown pass.
//
// Testing:
//   initialize()
//   shutdown()
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("DEFAULT INITIALIZE & SHUTDOWN");

    // Initialize the 'Time'.
    ASSERT_SAFE_PASS(bmqsys::Time::initialize());

    // Initialize should be a no-op.
    ASSERT_SAFE_PASS(bmqsys::Time::initialize());

    // Shutdown the Time is a no-op.
    ASSERT_SAFE_PASS(bmqsys::Time::shutdown());

    // Shutdown the Time.
    ASSERT_SAFE_PASS(bmqsys::Time::shutdown());

    // Shutdown again should assert
    ASSERT_SAFE_FAIL(bmqsys::Time::shutdown());
}

static void test3_customInitializeShutdown()
// ------------------------------------------------------------------------
// CUSTOM INITIALIZE & SHUTDOWN
//
// Concerns:
//   Ensure that custom-provided mechanism to provide system clocks and
//   high resolution timer initialize and shutdown.
//
// Plan:
//   - Verify that initialization and shutdown pass.
//
// Testing:
//   initialize()
//   shutdown()
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("CUSTOM INITIALIZE & SHUTDOWN");

    TestClock testClock;

    // Initialize the 'Time'.
    ASSERT_SAFE_PASS(bmqsys::Time::initialize(
        bdlf::BindUtil::bind(&TestClock::realtimeClock, &testClock),
        bdlf::BindUtil::bind(&TestClock::monotonicClock, &testClock),
        bdlf::BindUtil::bind(&TestClock::highResTimer, &testClock)));

    // Initialize should be a no-op.
    ASSERT_SAFE_PASS(bmqsys::Time::initialize(
        bdlf::BindUtil::bind(&TestClock::realtimeClock, &testClock),
        bdlf::BindUtil::bind(&TestClock::monotonicClock, &testClock),
        bdlf::BindUtil::bind(&TestClock::highResTimer, &testClock)));

    // Shutdown the Time is a no-op.
    ASSERT_SAFE_PASS(bmqsys::Time::shutdown());

    // Shutdown the Time.
    ASSERT_SAFE_PASS(bmqsys::Time::shutdown());

    // Shutdown again should assert
    ASSERT_SAFE_FAIL(bmqsys::Time::shutdown());
}

// ============================================================================
//                              PERFORMANCE TESTS
// ----------------------------------------------------------------------------
BSLA_MAYBE_UNUSED
static void testN1_performance()
// ------------------------------------------------------------------------
// PERFORMANCE (micro-benchmark)
//
// Concerns:
//   Make sure that the added layer of bsl::function for each timer are not
//   impacting performance.
//
// Plan:
//   - Run a micro-benchmark (making 100 million calls) to the monotonic
//     clock using each of the 4 ways to call them, and print average time
//     per call.
//
// Testing:
//   - Compare performance of bdlf::Bind vs bdlf::MemFn vs raw function ptr
//
// Results:
//   (1): straight call to the function
//          bsls::SystemTime::nowMonotonicClock();
//   (2): call to the function via a C function pointer
//          bsls::TimeInterval(*fn)();
//          fn = &bsls::SystemTime::nowMonotonicClock;
//   (3): call to the function via a bsl::function typedef
//          typedef bsl::function<bsls::TimeInterval()> Fn;
//          Fn fn = &bsls::SystemTime::nowMonotonicClock;
//   (4): call to the function via a bdlf::Bind type
//          typedef bsl::function<bsls::TimeInterval()> Fn;
//           Fn fn = bdlf::BindUtil::bind(
//                                   &bsls::SystemTime::nowMonotonicClock);
//
//          | Debug | Release |
//          +-------+---------+
//      (1) |   60  |    36   |    Times are in nanoseconds per call,
//      (2) |   60  |    36   |    average from 100 million calls on a
//      (3) |   70  |    36   |    Linux system.
//      (4) |  100  |    36   |
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("PERFORMANCE");

    const bsls::Types::Int64 k_NUM_CALLS = 100000000;  // 1O0 million

    PV("Direct call");
    {
        bsls::Types::Int64 start = bsls::TimeUtil::getTimer();
        for (bsls::Types::Int64 i = 0; i < k_NUM_CALLS; ++i) {
            bsls::TimeInterval value = bsls::SystemTime::nowMonotonicClock();
            (void)value;
        }
        bsls::Types::Int64 end = bsls::TimeUtil::getTimer();

        cout << k_NUM_CALLS << " function calls in "
             << bmqu::PrintUtil::prettyTimeInterval(end - start)
             << " (average of " << (end - start) / k_NUM_CALLS
             << " nano seconds per call)." << endl;
    }

    PV("Function pointer");
    {
        bsls::TimeInterval (*fn)();
        fn = &bsls::SystemTime::nowMonotonicClock;

        bsls::Types::Int64 start = bsls::TimeUtil::getTimer();
        for (bsls::Types::Int64 i = 0; i < k_NUM_CALLS; ++i) {
            bsls::TimeInterval value = fn();
            (void)value;
        }
        bsls::Types::Int64 end = bsls::TimeUtil::getTimer();

        cout << k_NUM_CALLS << " function calls in "
             << bmqu::PrintUtil::prettyTimeInterval(end - start)
             << " (average of " << (end - start) / k_NUM_CALLS
             << " nano seconds per call)." << endl;
    }

    PV("bsl::function");
    {
        typedef bsl::function<bsls::TimeInterval()> Fn;
        Fn fn = &bsls::SystemTime::nowMonotonicClock;

        bsls::Types::Int64 start = bsls::TimeUtil::getTimer();
        for (bsls::Types::Int64 i = 0; i < k_NUM_CALLS; ++i) {
            bsls::TimeInterval value = fn();
            (void)value;
        }
        bsls::Types::Int64 end = bsls::TimeUtil::getTimer();

        cout << k_NUM_CALLS << " function calls in "
             << bmqu::PrintUtil::prettyTimeInterval(end - start)
             << " (average of " << (end - start) / k_NUM_CALLS
             << " nano seconds per call)." << endl;
    }

    PV("bdlf::Bind");
    {
        typedef bsl::function<bsls::TimeInterval()> Fn;
        Fn fn = bdlf::BindUtil::bind(&bsls::SystemTime::nowMonotonicClock);

        bsls::Types::Int64 start = bsls::TimeUtil::getTimer();
        for (bsls::Types::Int64 i = 0; i < k_NUM_CALLS; ++i) {
            bsls::TimeInterval value = fn();
            (void)value;
        }
        bsls::Types::Int64 end = bsls::TimeUtil::getTimer();

        cout << k_NUM_CALLS << " function calls in "
             << bmqu::PrintUtil::prettyTimeInterval(end - start)
             << " (average of " << (end - start) / k_NUM_CALLS
             << " nano seconds per call)." << endl;
    }
}

// Begin benchmarking tests
#ifdef BSLS_PLATFORM_OS_LINUX
static void testN1_defaultPerformance_GoogleBenchmark(benchmark::State& state)
{
    // Default function call
    bmqtst::TestHelper::printTestName("PERFORMANCE GOOGLE BENCHMARK");

    for (auto _ : state) {
        for (bsls::Types::Int64 i = 0; i < state.range(0); ++i) {
            bsls::TimeInterval value = bsls::SystemTime::nowMonotonicClock();
            (void)value;
        }
    }
}

static void testN1_funcptrPerformance_GoogleBenchmark(benchmark::State& state)
{
    // Function pointer
    bmqtst::TestHelper::printTestName("PERFORMANCE GOOGLE BENCHMARK");

    bsls::TimeInterval (*fn)();
    fn = &bsls::SystemTime::nowMonotonicClock;

    for (auto _ : state) {
        for (bsls::Types::Int64 i = 0; i < state.range(0); ++i) {
            bsls::TimeInterval value = fn();
            (void)value;
        }
    }
}

static void testN1_bslPerformance_GoogleBenchmark(benchmark::State& state)
{
    // BSL Typdef performance
    bmqtst::TestHelper::printTestName("PERFORMANCE GOOGLE BENCHMARK");

    typedef bsl::function<bsls::TimeInterval()> Fn;
    Fn fn = &bsls::SystemTime::nowMonotonicClock;

    for (auto _ : state) {
        for (bsls::Types::Int64 i = 0; i < state.range(0); ++i) {
            bsls::TimeInterval value = fn();
            (void)value;
        }
    }
}

static void testN1_bindPerformance_GoogleBenchmark(benchmark::State& state)
{
    // Binding performance
    bmqtst::TestHelper::printTestName("PERFORMANCE GOOGLE BENCHMARK");

    typedef bsl::function<bsls::TimeInterval()> Fn;
    Fn fn = bdlf::BindUtil::bind(&bsls::SystemTime::nowMonotonicClock);

    for (auto _ : state) {
        for (bsls::Types::Int64 i = 0; i < state.range(0); ++i) {
            bsls::TimeInterval value = fn();
            (void)value;
        }
    }
}
#endif
// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 3: test3_customInitializeShutdown(); break;
    case 2: test2_defaultInitializeShutdown(); break;
    case 1: test1_breathingTest(); break;
    case -1:
#ifdef BSLS_PLATFORM_OS_LINUX
        BENCHMARK(testN1_defaultPerformance_GoogleBenchmark)
            ->RangeMultiplier(10)
            ->Range(1000, 100000000)
            ->Unit(benchmark::kMillisecond);
        BENCHMARK(testN1_funcptrPerformance_GoogleBenchmark)
            ->RangeMultiplier(10)
            ->Range(1000, 100000000)
            ->Unit(benchmark::kMillisecond);
        BENCHMARK(testN1_bslPerformance_GoogleBenchmark)
            ->RangeMultiplier(10)
            ->Range(1000, 100000000)
            ->Unit(benchmark::kMillisecond);
        BENCHMARK(testN1_bindPerformance_GoogleBenchmark)
            ->RangeMultiplier(10)
            ->Range(1000, 100000000)
            ->Unit(benchmark::kMillisecond);
        benchmark::Initialize(&argc, argv);
        benchmark::RunSpecifiedBenchmarks();
#else
        testN1_performance();
#endif
        break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        s_testStatus = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}

// ----------------------------------------------------------------------------
// NOTICE: