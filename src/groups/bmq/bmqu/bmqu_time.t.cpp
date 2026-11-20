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

#include <bmqu_time.h>

// BDE
#include <bdlf_bind.h>
#include <bsl_functional.h>
#include <bsl_string_view.h>
#include <bsls_assert.h>
#include <bsls_systemtime.h>
#include <bsls_timeinterval.h>
#include <bsls_timeutil.h>
#include <bsls_types.h>

// TEST DRIVER
#include <bmqtst_table.h>
#include <bmqtst_testhelper.h>

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

/// @brief Functor calling the monotonic clock directly.
struct DirectCall {
    /// @brief Return the current monotonic clock time.
    bsls::TimeInterval operator()() const
    {
        return bsls::SystemTime::nowMonotonicClock();
    }
};

/// @brief Call the specified `fn` the specified `numCalls` times and add the
///        timing results as a row with the specified `name` to the specified
///        `table`.
///
/// @param table    The table to add the row to.
/// @param name     The name of the row.
/// @param fn       The function to call.
/// @param numCalls The number of calls to make.
template <class FN>
void addResult(bmqtst::Table*     table,
               bsl::string_view   name,
               FN                 fn,
               bsls::Types::Int64 numCalls)
{
    BSLS_ASSERT_OPT(table);

    const bsls::Types::Int64 start = bsls::TimeUtil::getTimer();
    for (bsls::Types::Int64 i = 0; i < numCalls; ++i) {
        bsls::TimeInterval value = fn();
        (void)value;
    }
    const bsls::Types::Int64 end = bsls::TimeUtil::getTimer();

    table->column("Name").insertValue(name);
    table->column("Calls").insertValue(
        static_cast<bsls::Types::Uint64>(numCalls));
    table->column("Total time (ns)")
        .insertValue(static_cast<bsls::Types::Uint64>(end - start));
    table->column("Per call (ns)")
        .insertValue(
            static_cast<bsls::Types::Uint64>((end - start) / numCalls));
}

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
//   - Create a TestClock, initialize the bmqu::TimeUtil with it, and
//     verify each timer function returns the properly set value.
//
// Testing:
//   Basic functionality
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("BREATHING TEST");

    TestClock testClock;
    bmqu::Time::initialize(
        bdlf::BindUtil::bind(&TestClock::realtimeClock, &testClock),
        bdlf::BindUtil::bind(&TestClock::monotonicClock, &testClock),
        bdlf::BindUtil::bind(&TestClock::highResTimer, &testClock));

    PV("Testing real time clock");
    {
        testClock.d_realtimeClock.setTotalSeconds(5);
        BMQTST_ASSERT_EQ(bsls::TimeInterval(5, 0),
                         bmqu::Time::nowRealtimeClock());

        testClock.d_realtimeClock.setTotalSeconds(0);
        BMQTST_ASSERT_EQ(bsls::TimeInterval(0, 0),
                         bmqu::Time::nowRealtimeClock());
    }

    PV("Testing monotonic clock");
    {
        testClock.d_monotonicClock.setTotalSeconds(5);
        BMQTST_ASSERT_EQ(bsls::TimeInterval(5, 0),
                         bmqu::Time::nowMonotonicClock());

        testClock.d_monotonicClock.setTotalSeconds(0);
        BMQTST_ASSERT_EQ(bsls::TimeInterval(0, 0),
                         bmqu::Time::nowMonotonicClock());
    }

    PV("Testing high resolution timer");
    {
        testClock.d_highResTimer = 1000000;
        BMQTST_ASSERT_EQ(1000000, bmqu::Time::highResolutionTimer());

        testClock.d_highResTimer = 0;
        BMQTST_ASSERT_EQ(0, bmqu::Time::highResolutionTimer());
    }

    bmqu::Time::shutdown();
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
    BMQTST_ASSERT_SAFE_PASS(bmqu::Time::initialize());

    // Initialize should be a no-op.
    BMQTST_ASSERT_SAFE_PASS(bmqu::Time::initialize());

    // Shutdown the Time is a no-op.
    BMQTST_ASSERT_SAFE_PASS(bmqu::Time::shutdown());

    // Shutdown the Time.
    BMQTST_ASSERT_SAFE_PASS(bmqu::Time::shutdown());

    // Shutdown again should assert
    BMQTST_ASSERT_SAFE_FAIL(bmqu::Time::shutdown());
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
    BMQTST_ASSERT_SAFE_PASS(bmqu::Time::initialize(
        bdlf::BindUtil::bind(&TestClock::realtimeClock, &testClock),
        bdlf::BindUtil::bind(&TestClock::monotonicClock, &testClock),
        bdlf::BindUtil::bind(&TestClock::highResTimer, &testClock)));

    // Initialize should be a no-op.
    BMQTST_ASSERT_SAFE_PASS(bmqu::Time::initialize(
        bdlf::BindUtil::bind(&TestClock::realtimeClock, &testClock),
        bdlf::BindUtil::bind(&TestClock::monotonicClock, &testClock),
        bdlf::BindUtil::bind(&TestClock::highResTimer, &testClock)));

    // Shutdown the Time is a no-op.
    BMQTST_ASSERT_SAFE_PASS(bmqu::Time::shutdown());

    // Shutdown the Time.
    BMQTST_ASSERT_SAFE_PASS(bmqu::Time::shutdown());

    // Shutdown again should assert
    BMQTST_ASSERT_SAFE_FAIL(bmqu::Time::shutdown());
}

// ============================================================================
//                              PERFORMANCE TESTS
// ----------------------------------------------------------------------------
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
//     clock using each of the 5 ways to call them, and print average time
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
//   (5): call through bmqu::Time
//          bmqu::Time::nowMonotonicClock();
//
// Results (RelWithDebInfo / Linux):
//
//             Name |     Calls | Total time (ns) | Per call (ns)
// ==============================================================
//      Direct call | 100000000 |      2038425572 |            20
// Function pointer | 100000000 |      2024406280 |            20
//    bsl::function | 100000000 |      2068457219 |            20
//       bdlf::Bind | 100000000 |      2109586223 |            21
//       bmqu::Time | 100000000 |      2216486878 |            22
// --------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("PERFORMANCE");

    const bsls::Types::Int64 k_NUM_CALLS = 100000000;  // 100 million

    typedef bsl::function<bsls::TimeInterval()> Fn;

    bsls::TimeInterval (*fnPtr)() = &bsls::SystemTime::nowMonotonicClock;
    Fn bslFn                      = &bsls::SystemTime::nowMonotonicClock;
    Fn bindFn = bdlf::BindUtil::bind(&bsls::SystemTime::nowMonotonicClock);

    bmqtst::Table table(bmqtst::TestHelperUtil::allocator());
    addResult(&table, "Direct call", DirectCall(), k_NUM_CALLS);
    addResult(&table, "Function pointer", fnPtr, k_NUM_CALLS);
    addResult(&table, "bsl::function", bslFn, k_NUM_CALLS);
    addResult(&table, "bdlf::Bind", bindFn, k_NUM_CALLS);

    bmqu::Time::initialize(bmqtst::TestHelperUtil::allocator());
    addResult(&table,
              "bmqu::Time",
              &bmqu::Time::nowMonotonicClock,
              k_NUM_CALLS);
    bmqu::Time::shutdown();

    table.print(bsl::cout);
}

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
    case -1: testN1_performance(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << "\n";
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
