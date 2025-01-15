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

// bmqu_throttledaction.t.cpp                                         -*-C++-*-
#include <bmqu_throttledaction.h>

#include <bmqtst_scopedlogobserver.h>

// BDE
#include <ball_log.h>
#include <ball_severity.h>
#include <bdlf_bind.h>
#include <bdlt_timeunitratio.h>
#include <bslmt_threadutil.h>
#include <bsls_timeutil.h>
#include <bsls_types.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------

namespace {

const char k_LOG_CATEGORY[] = "BMQU.THROTTLEDACTION.TESTDRIVER";

/// Increment the specified `n` by one.
static void incrementInteger(int* n)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(n && "'n' must be specified");

    ++(*n);
}

/// Reset the specified `n` to zero.
static void resetInteger(int* n)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(n && "'n' must be specified");

    *n = 0;
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
//   1. Instantiate an 'bmqu::ThrottledActionParams' with parameters and
//      verify expected state.
//   2. Do a 'BMQU_THROTTLEDACTION_THROTTLE' and ensure the specified
//      'ACTION' was executed.
//
// Testing:
//   ThrottledActionParams
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("BREATHING TEST");

    BALL_LOG_SET_CATEGORY(k_LOG_CATEGORY);

    // CONSTANTS
    const int k_INTERVAL_MS            = 2000;
    const int k_MAX_COUNT_PER_INTERVAL = 3;

    // 1. Instantiate an 'bmqu::ThrottledActionParams' with parameters and
    //    verify expected state.

    // No more than 3 logs in a 5s timeframe
    bmqu::ThrottledActionParams obj(k_INTERVAL_MS, k_MAX_COUNT_PER_INTERVAL);
    BMQTST_ASSERT_EQ(obj.d_intervalNano,
                     k_INTERVAL_MS *
                         bdlt::TimeUnitRatio::k_NANOSECONDS_PER_MILLISECOND);
    BMQTST_ASSERT_EQ(obj.d_maxCountPerInterval, k_MAX_COUNT_PER_INTERVAL);
    BMQTST_ASSERT_EQ(obj.d_countSinceLastReset, 0);
    BMQTST_ASSERT_GE(obj.d_lastResetTime, 0);

    // 2. Do a 'BMQU_THROTTLEDACTION_THROTTLE' and ensure the specified
    //    'ACTION' was executed.
    int n = 0;
    BMQU_THROTTLEDACTION_THROTTLE(obj, incrementInteger(&n));

    BMQTST_ASSERT_EQ(n, 1);
    BMQTST_ASSERT_EQ(obj.d_intervalNano,
                     k_INTERVAL_MS *
                         bdlt::TimeUnitRatio::k_NANOSECONDS_PER_MILLISECOND);
    BMQTST_ASSERT_EQ(obj.d_maxCountPerInterval, k_MAX_COUNT_PER_INTERVAL);
    BMQTST_ASSERT_EQ(obj.d_countSinceLastReset, 1);
    BMQTST_ASSERT_GE(obj.d_lastResetTime, 0LL);
}

static void test2_throttleNoReset()
// ------------------------------------------------------------------------
// THROTTLE NO RESET
//
// Concerns:
//   1. Throttling with no reset action executes the specified 'ACTION' no
//      more than the specified number of times during the specified
//      time interval.
//   2. Throttling with no reset action does nothing upon reset.
//
// Plan:
//   1. Do a 'BMQU_THROTTLEDACTION_THROTTLE_NO_RESET' up to and including
//      the maximum number of times (within the specified time interval)
//      and ensure that the specified 'ACTION' was executed exactly the
//      maximum number of times.
//   2. Do more 'BMQU_THROTTLEDACTION_THROTTLE_NO_RESET' (withing the
//      specified time interval) and ensure that the specified 'ACTION' was
//      *not* executed and nor reset action was taken.
//   3. Wait until the specified time interval elapses (as determined by
//      'bsls::TimeUtil::getTimer') and repeat steps 1-2 above.
//
// Testing:
//   BMQU_THROTTLEDACTION_THROTTLE_NO_RESET(P, ACTION)
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("THROTTLE NO RESET");

    // CONSTANTS
    const int k_INTERVAL_MS            = 2000;
    const int k_MAX_COUNT_PER_INTERVAL = 3;

    bmqtst::ScopedLogObserver logObserver(ball::Severity::INFO,
                                          bmqtst::TestHelperUtil::allocator());

    // No more than 3 logs in a 5s timeframe
    bmqu::ThrottledActionParams obj(k_INTERVAL_MS, k_MAX_COUNT_PER_INTERVAL);

    // 1. Do a 'BMQU_THROTTLEDACTION_THROTTLE_NO_RESET' up to and including the
    //    maximum number of times (within the specified time interval) and
    //    ensure that the specified 'ACTION' was executed exactly the maximum
    //    number of times.
    int n = 0;
    for (int i = 0; i < k_MAX_COUNT_PER_INTERVAL; ++i) {
        BMQU_THROTTLEDACTION_THROTTLE_NO_RESET(obj, incrementInteger(&n));
    }

    BMQTST_ASSERT_EQ(n, k_MAX_COUNT_PER_INTERVAL);
    BMQTST_ASSERT_EQ(obj.d_intervalNano,
                     k_INTERVAL_MS *
                         bdlt::TimeUnitRatio::k_NANOSECONDS_PER_MILLISECOND);
    BMQTST_ASSERT_EQ(obj.d_maxCountPerInterval, k_MAX_COUNT_PER_INTERVAL);
    BMQTST_ASSERT_EQ(obj.d_countSinceLastReset, k_MAX_COUNT_PER_INTERVAL);
    BMQTST_ASSERT_GE(obj.d_lastResetTime, 0LL);

    // 2. Do more 'BMQU_THROTTLEDACTION_THROTTLE_NO_RESET' (withing the
    //    specified time interval) and ensure that the specified 'ACTION' was
    //    *not* executed and no reset action was taken.
    BMQU_THROTTLEDACTION_THROTTLE_NO_RESET(obj, incrementInteger(&n));
    BMQU_THROTTLEDACTION_THROTTLE_NO_RESET(obj, incrementInteger(&n));

    BMQTST_ASSERT_EQ(n, k_MAX_COUNT_PER_INTERVAL);
    // 'n' is unchanged
    BMQTST_ASSERT_EQ(obj.d_intervalNano,
                     k_INTERVAL_MS *
                         bdlt::TimeUnitRatio::k_NANOSECONDS_PER_MILLISECOND);
    BMQTST_ASSERT_EQ(obj.d_maxCountPerInterval, k_MAX_COUNT_PER_INTERVAL);
    BMQTST_ASSERT_EQ(obj.d_countSinceLastReset, k_MAX_COUNT_PER_INTERVAL + 2);
    BMQTST_ASSERT_GE(obj.d_lastResetTime, 0);

    // 3. Wait until the specified time interval elapses (as determined by
    //   'bsls::TimeUtil::getTimer') and repeat steps 1-2 above.
    bsls::Types::Int64 now = BloombergLP::bsls::TimeUtil::getTimer();
    while ((now - obj.d_lastResetTime) < obj.d_intervalNano) {
        now = BloombergLP::bsls::TimeUtil::getTimer();
    }

    // Do a 'BMQU_THROTTLEDACTION_THROTTLE_NO_RESET' up to and including the
    // maximum number of times (within the specified time interval) and
    // ensure that the specified 'ACTION' was executed exactly the maximum
    // number of times.
    n = 0;
    for (int i = 0; i < k_MAX_COUNT_PER_INTERVAL; ++i) {
        BMQU_THROTTLEDACTION_THROTTLE_NO_RESET(obj, incrementInteger(&n));
    }

    BMQTST_ASSERT_EQ(logObserver.records().size(),
                     0U);  // Nothing was logged on reset
    BMQTST_ASSERT_EQ(n, k_MAX_COUNT_PER_INTERVAL);
    BMQTST_ASSERT_EQ(obj.d_intervalNano,
                     k_INTERVAL_MS *
                         bdlt::TimeUnitRatio::k_NANOSECONDS_PER_MILLISECOND);
    BMQTST_ASSERT_EQ(obj.d_maxCountPerInterval, k_MAX_COUNT_PER_INTERVAL);
    BMQTST_ASSERT_EQ(obj.d_countSinceLastReset, k_MAX_COUNT_PER_INTERVAL);
    BMQTST_ASSERT_GT(obj.d_lastResetTime, 0);

    // Do more 'BMQU_THROTTLEDACTION_THROTTLE_NO_RESET' (withing the specified
    // time interval) and ensure that the specified 'ACTION' was *not* executed
    // and no reset action was taken.
    BMQU_THROTTLEDACTION_THROTTLE_NO_RESET(obj, incrementInteger(&n));
    BMQU_THROTTLEDACTION_THROTTLE_NO_RESET(obj, incrementInteger(&n));

    BMQTST_ASSERT_EQ(logObserver.records().size(),
                     0U);  // Nothing was logged on reset
    BMQTST_ASSERT_EQ(n, k_MAX_COUNT_PER_INTERVAL);
    // 'n' is unchanged
    BMQTST_ASSERT_EQ(obj.d_intervalNano,
                     k_INTERVAL_MS *
                         bdlt::TimeUnitRatio::k_NANOSECONDS_PER_MILLISECOND);
    BMQTST_ASSERT_EQ(obj.d_maxCountPerInterval, k_MAX_COUNT_PER_INTERVAL);
    BMQTST_ASSERT_EQ(obj.d_countSinceLastReset, k_MAX_COUNT_PER_INTERVAL + 2);
    BMQTST_ASSERT_GT(obj.d_lastResetTime, 0LL);
}

static void test3_throttleWithDefaultReset()
// ------------------------------------------------------------------------
// THROTTLE WITH DEFAULT RESET
//
// Concerns:
//   1. Throttling with the default reset function executes the specified
//      'ACTION' no more than the specified number of times during the
//      specified time interval.
//   2. Upon reset, the default reset function should print a BALL_LOG_INFO
//      having the name of the 'bmqu::ThrottledActionParams' variable and
//      the number of items that have been skipped since last reset.
//
// Plan:
//   1. Do a 'BMQU_THROTTLEDACTION_THROTTLE' up to and including the
//      maximum number of times (within the specified time interval) and
//      ensure that the specified 'ACTION' was executed exactly the
//      maximum number of times and the default reset function was not
//      called.
//   2. Do more 'BMQU_THROTTLEDACTION_THROTTLE' (withing the specified time
//      interval) and ensure that the specified 'ACTION' was *not* executed
//      and nor was the default reset function called.
//   3. Wait until the specified time interval elapses (as determined by
//      'bsls::TimeUtil::getTimer') and repeat steps 1-2 above.  While
//      doing so, ensure that the default reset function was properly
//      invoked.
//
// Testing:
//   BMQU_THROTTLEDACTION_THROTTLE(P, ACTION)
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // Logging infrastructure allocates using the default allocator, and
    // that logging is beyond the control of this function.

    bmqtst::TestHelper::printTestName("THROTTLE WITH DEFAULT RESET");

    BALL_LOG_SET_CATEGORY(k_LOG_CATEGORY);

    // CONSTANTS
    const int k_INTERVAL_MS            = 2000;
    const int k_MAX_COUNT_PER_INTERVAL = 3;

    bmqtst::ScopedLogObserver logObserver(ball::Severity::INFO,
                                          bmqtst::TestHelperUtil::allocator());

    // No more than 3 logs in a 5s timeframe
    bmqu::ThrottledActionParams obj(k_INTERVAL_MS, k_MAX_COUNT_PER_INTERVAL);

    // 1. Do a 'BMQU_THROTTLEDACTION_THROTTLE' up to and including the
    //    maximum number of times (within the specified time interval)
    //    and ensure that the specified 'ACTION' was executed exactly the
    //      maximum number of times and the default reset function was not
    //      called.
    int n = 0;
    for (int i = 0; i < k_MAX_COUNT_PER_INTERVAL; ++i) {
        BMQU_THROTTLEDACTION_THROTTLE(obj, incrementInteger(&n));
    }

    BMQTST_ASSERT_EQ(n, k_MAX_COUNT_PER_INTERVAL);
    BMQTST_ASSERT_EQ(obj.d_intervalNano,
                     k_INTERVAL_MS *
                         bdlt::TimeUnitRatio::k_NANOSECONDS_PER_MILLISECOND);
    BMQTST_ASSERT_EQ(obj.d_maxCountPerInterval, k_MAX_COUNT_PER_INTERVAL);
    BMQTST_ASSERT_EQ(obj.d_countSinceLastReset, k_MAX_COUNT_PER_INTERVAL);
    BMQTST_ASSERT_GE(obj.d_lastResetTime, 0);

    // 2. Do more 'BMQU_THROTTLEDACTION_THROTTLE' (withing the specified time
    //    interval) and ensure that the specified 'ACTION' was *not* executed
    //    and nor was the default reset function called.
    BMQU_THROTTLEDACTION_THROTTLE(obj, incrementInteger(&n));
    BMQU_THROTTLEDACTION_THROTTLE(obj, incrementInteger(&n));

    BMQTST_ASSERT_EQ(logObserver.records().size(), 0U);
    BMQTST_ASSERT_EQ(n, k_MAX_COUNT_PER_INTERVAL);
    // 'n' is unchanged
    BMQTST_ASSERT_EQ(obj.d_intervalNano,
                     k_INTERVAL_MS *
                         bdlt::TimeUnitRatio::k_NANOSECONDS_PER_MILLISECOND);
    BMQTST_ASSERT_EQ(obj.d_maxCountPerInterval, k_MAX_COUNT_PER_INTERVAL);
    BMQTST_ASSERT_EQ(obj.d_countSinceLastReset, k_MAX_COUNT_PER_INTERVAL + 2);
    BMQTST_ASSERT_GE(obj.d_lastResetTime, 0LL);

    // 3. Wait until the specified time interval elapses (as determined by
    //   'bsls::TimeUtil::getTimer') and repeat steps 1-2 above.
    bsls::Types::Int64 now = BloombergLP::bsls::TimeUtil::getTimer();
    while ((now - obj.d_lastResetTime) < obj.d_intervalNano) {
        now = BloombergLP::bsls::TimeUtil::getTimer();
    }

    // Do a 'BMQU_THROTTLEDACTION_THROTTLE' up to and including the maximum
    // number of times (within the specified time interval) and ensure that the
    // specified 'ACTION' was executed exactly the maximum number of times.
    n = 0;
    for (int i = 0; i < k_MAX_COUNT_PER_INTERVAL; ++i) {
        BMQU_THROTTLEDACTION_THROTTLE(obj, incrementInteger(&n));
    }

    // Ensure the default reset function was invoked.
    if (bmqtst::TestHelperUtil::verbosityLevel() >= 1) {
        BMQTST_ASSERT_EQ(logObserver.records().size(), 1U);
        PV("Logged during reset: " << logObserver.records()[0]);
        BMQTST_ASSERT(bmqtst::ScopedLogObserverUtil::recordMessageMatch(
            logObserver.records()[0],
            "'obj'.*2",
            bmqtst::TestHelperUtil::allocator()));
    }
    BMQTST_ASSERT_EQ(n, k_MAX_COUNT_PER_INTERVAL);
    BMQTST_ASSERT_EQ(obj.d_intervalNano,
                     k_INTERVAL_MS *
                         bdlt::TimeUnitRatio::k_NANOSECONDS_PER_MILLISECOND);
    BMQTST_ASSERT_EQ(obj.d_maxCountPerInterval, k_MAX_COUNT_PER_INTERVAL);
    BMQTST_ASSERT_EQ(obj.d_countSinceLastReset, k_MAX_COUNT_PER_INTERVAL);
    BMQTST_ASSERT_GT(obj.d_lastResetTime, 0LL);

    // Do more 'BMQU_THROTTLEDACTION_THROTTLE' (withing the specified time
    // interval) and ensure that the specified 'ACTION' was *not* executed
    // and no additional reset action was taken.
    BMQU_THROTTLEDACTION_THROTTLE(obj, incrementInteger(&n));
    BMQU_THROTTLEDACTION_THROTTLE(obj, incrementInteger(&n));

    if (bmqtst::TestHelperUtil::verbosityLevel() >= 1) {
        BMQTST_ASSERT_EQ(logObserver.records().size(), 1U);
    }
    BMQTST_ASSERT_EQ(n, k_MAX_COUNT_PER_INTERVAL);
    // 'n' is unchanged
    BMQTST_ASSERT_EQ(obj.d_intervalNano,
                     k_INTERVAL_MS *
                         bdlt::TimeUnitRatio::k_NANOSECONDS_PER_MILLISECOND);
    BMQTST_ASSERT_EQ(obj.d_maxCountPerInterval, k_MAX_COUNT_PER_INTERVAL);
    BMQTST_ASSERT_EQ(obj.d_countSinceLastReset, k_MAX_COUNT_PER_INTERVAL + 2);
    BMQTST_ASSERT_GT(obj.d_lastResetTime, 0LL);
}

static void test4_throttleWithCustomReset()
// ------------------------------------------------------------------------
// THROTTLE WITH CUSTOM RESET
//
// Concerns:
//   1. Throttling with a custom reset action executes the specified
//      'ACTION' no more than the specified number of times during the
//      specified time interval.
//   2. Upon reset, the custom reset action is performed.
//
// Plan:
//   1. Do a 'BMQU_THROTTLEDACTION_THROTTLE_WITH_RESET' up to and including
//      the maximum number of times (within the specified time interval)
//      and ensure that the specified 'ACTION' was executed exactly the
//      maximum number of times and the custom reset action was not
//      performed.
//   2. Do more 'BMQU_THROTTLEDACTION_THROTTLE_WITH_RESET' (within the
//      specified time interval) and ensure that the specified 'ACTION' was
//      *not* executed and nor was the custom reset action performed.
//   3. Wait until the specified time interval elapses (as determined by
//      'bsls::TimeUtil::getTimer') and repeat steps 1-2 above.  While
//      doing so, ensure that the custom reset action was properly
//      performed.
//
// Testing:
//   BMQU_THROTTLEDACTION_THROTTLE_WITH_RESET(P, ACTION)
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // Logging infrastructure allocates using the default allocator, and
    // that logging is beyond the control of this function.

    bmqtst::TestHelper::printTestName("THROTTLE WITH CUSTOM RESET");

    // CONSTANTS
    const int k_INTERVAL_MS            = 2000;
    const int k_MAX_COUNT_PER_INTERVAL = 3;

    bmqtst::ScopedLogObserver logObserver(ball::Severity::INFO,
                                          bmqtst::TestHelperUtil::allocator());

    // No more than 3 logs in a 5s timeframe
    bmqu::ThrottledActionParams obj(k_INTERVAL_MS, k_MAX_COUNT_PER_INTERVAL);

    // 1. Do a 'BMQU_THROTTLEDACTION_THROTTLE_WITH_RESET' up to and including
    //    the maximum number of times (within the specified time interval)
    //    and ensure that the specified 'ACTION' was executed exactly the
    //    maximum number of times and the custom reset function was not called.
    int n = 0;
    for (int i = 0; i < k_MAX_COUNT_PER_INTERVAL; ++i) {
        BMQU_THROTTLEDACTION_THROTTLE_WITH_RESET(obj,
                                                 incrementInteger(&n),
                                                 resetInteger(&n));
    }

    BMQTST_ASSERT_EQ(n, k_MAX_COUNT_PER_INTERVAL);
    BMQTST_ASSERT_EQ(obj.d_intervalNano,
                     k_INTERVAL_MS *
                         bdlt::TimeUnitRatio::k_NANOSECONDS_PER_MILLISECOND);
    BMQTST_ASSERT_EQ(obj.d_maxCountPerInterval, k_MAX_COUNT_PER_INTERVAL);
    BMQTST_ASSERT_EQ(obj.d_countSinceLastReset, k_MAX_COUNT_PER_INTERVAL);
    BMQTST_ASSERT_GE(obj.d_lastResetTime, 0LL);

    // 2. Do more 'BMQU_THROTTLEDACTION_THROTTLE_WITH_RESET' (within the
    //    specified time interval) and ensure that the specified 'ACTION' was
    //    *not* executed and nor was the custom reset action performed.
    BMQU_THROTTLEDACTION_THROTTLE_WITH_RESET(obj,
                                             incrementInteger(&n),
                                             resetInteger(&n));
    BMQU_THROTTLEDACTION_THROTTLE_WITH_RESET(obj,
                                             incrementInteger(&n),
                                             resetInteger(&n));

    BMQTST_ASSERT_EQ(logObserver.records().size(), 0U);
    BMQTST_ASSERT_EQ(n, k_MAX_COUNT_PER_INTERVAL);
    // 'n' is unchanged
    BMQTST_ASSERT_EQ(obj.d_intervalNano,
                     k_INTERVAL_MS *
                         bdlt::TimeUnitRatio::k_NANOSECONDS_PER_MILLISECOND);
    BMQTST_ASSERT_EQ(obj.d_maxCountPerInterval, k_MAX_COUNT_PER_INTERVAL);
    BMQTST_ASSERT_EQ(obj.d_countSinceLastReset, k_MAX_COUNT_PER_INTERVAL + 2);
    BMQTST_ASSERT_GE(obj.d_lastResetTime, 0LL);

    // 3. Wait until the specified time interval elapses (as determined by
    //    'bsls::TimeUtil::getTimer') and repeat steps 1-2 above.  While doing
    //    doing so, ensure that the custom reset action was properly performed.
    bsls::Types::Int64 now = BloombergLP::bsls::TimeUtil::getTimer();
    while ((now - obj.d_lastResetTime) < obj.d_intervalNano) {
        now = BloombergLP::bsls::TimeUtil::getTimer();
    }

    // 1. Do a 'BMQU_THROTTLEDACTION_THROTTLE_WITH_RESET' up to and including
    //    the maximum number of times (within the specified time interval)
    //    and ensure that the specified 'ACTION' was executed exactly the
    //    maximum number of times and the custom reset function was not called.
    for (int i = 0; i < k_MAX_COUNT_PER_INTERVAL; ++i) {
        BMQU_THROTTLEDACTION_THROTTLE_WITH_RESET(obj,
                                                 incrementInteger(&n),
                                                 resetInteger(&n));
    }

    // Ensure the custom reset action was performed (and the default reset
    // action was not).
    BMQTST_ASSERT_EQ(logObserver.records().size(),
                     0U);  // default reset not performed

    BMQTST_ASSERT_EQ(n,
                     k_MAX_COUNT_PER_INTERVAL);  // custom reset was performed
    BMQTST_ASSERT_EQ(obj.d_intervalNano,
                     k_INTERVAL_MS *
                         bdlt::TimeUnitRatio::k_NANOSECONDS_PER_MILLISECOND);
    BMQTST_ASSERT_EQ(obj.d_maxCountPerInterval, k_MAX_COUNT_PER_INTERVAL);
    BMQTST_ASSERT_EQ(obj.d_countSinceLastReset, k_MAX_COUNT_PER_INTERVAL);
    BMQTST_ASSERT_GT(obj.d_lastResetTime, 0LL);

    // Do more 'BMQU_THROTTLEDACTION_THROTTLE' (withing the specified time
    // interval) and ensure that the specified 'ACTION' was *not* executed
    // and no additional reset action was taken.
    BMQU_THROTTLEDACTION_THROTTLE_WITH_RESET(obj,
                                             incrementInteger(&n),
                                             resetInteger(&n));
    BMQU_THROTTLEDACTION_THROTTLE_WITH_RESET(obj,
                                             incrementInteger(&n),
                                             resetInteger(&n));

    BMQTST_ASSERT_EQ(logObserver.records().size(),
                     0U);  // default reset not performed
    BMQTST_ASSERT_EQ(n, k_MAX_COUNT_PER_INTERVAL);
    // 'n' is unchanged
    BMQTST_ASSERT_EQ(obj.d_intervalNano,
                     k_INTERVAL_MS *
                         bdlt::TimeUnitRatio::k_NANOSECONDS_PER_MILLISECOND);
    BMQTST_ASSERT_EQ(obj.d_maxCountPerInterval, k_MAX_COUNT_PER_INTERVAL);
    BMQTST_ASSERT_EQ(obj.d_countSinceLastReset, k_MAX_COUNT_PER_INTERVAL + 2);
    BMQTST_ASSERT_GT(obj.d_lastResetTime, 0LL);
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    bsls::TimeUtil::initialize();

    switch (_testCase) {
    case 0:
    case 4: test4_throttleWithCustomReset(); break;
    case 3: test3_throttleWithDefaultReset(); break;
    case 2: test2_throttleNoReset(); break;
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
