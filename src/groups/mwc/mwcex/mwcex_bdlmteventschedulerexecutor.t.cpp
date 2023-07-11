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

// mwcex_bdlmteventschedulerexecutor.t.cpp                            -*-C++-*-
#include <mwcex_bdlmteventschedulerexecutor.h>

// TEST DRIVER
#include <mwctst_testhelper.h>

// BDE
#include <bdlf_bind.h>
#include <bdlmt_eventscheduler.h>
#include <bslma_testallocator.h>
#include <bsls_systemtime.h>
#include <bsls_timeinterval.h>

// CONVENIENCE
using namespace BloombergLP;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {

// =============
// struct Assign
// =============

/// Provides a function object that assigns the specified `src` to the
/// specified `*dst`.
struct Assign {
    // TYPES

    /// Defines the result type of the call operator.
    typedef void ResultType;

    template <class DST, class SRC>
    void operator()(DST* dst, const SRC& src) const
    {
        *dst = src;
    }
};

}  // close anonymous namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_constructor()
// ------------------------------------------------------------------------
// CONSTRUCTOR
//
// Concerns:
//   Ensure proper behavior of the constructor.
//
// Plan:
//   Construct an instance of 'mwcex::BdlmtEventSchedulerExecutor'
//   and check postconditions.
//
// Testing:
//   mwcex::BdlmtEventSchedulerExecutor's constructor
// ------------------------------------------------------------------------
{
    bslma::TestAllocator  alloc;
    bdlmt::EventScheduler eventScheduler(&alloc);

    // create executor
    mwcex::BdlmtEventSchedulerExecutor ex(&eventScheduler,
                                          bsls::TimeInterval(42));

    // check postconditions
    ASSERT_EQ(&ex.context(), &eventScheduler);
    ASSERT_EQ(ex.timePoint(), bsls::TimeInterval(42));
}

static void test2_post()
// ------------------------------------------------------------------------
// POST
//
// Concerns:
//   Ensure proper behavior of the 'post' method.
//
// Plan:
//   Check that 'post' is:
//   - Forwarding the specified function object to the executor's
//     associated execution context;
//   - Are using the executor's associated time point.
//
// Testing:
//   mwcex::BdlmtEventSchedulerExecutor::post
// ------------------------------------------------------------------------
{
    bslma::TestAllocator                alloc;
    bdlmt::EventScheduler               eventScheduler(&alloc);
    bdlmt::EventSchedulerTestTimeSource eventSchedulerTimeSource(
        &eventScheduler);

    // start scheduler
    int rc = eventScheduler.start();
    BSLS_ASSERT_OPT(rc == 0);

    // create an executor with a time point at 1 second from now
    mwcex::BdlmtEventSchedulerExecutor ex1(
        &eventScheduler,
        eventSchedulerTimeSource.now().addSeconds(1));

    // create an executor with a time point at 2 second from now
    mwcex::BdlmtEventSchedulerExecutor ex2(
        &eventScheduler,
        eventSchedulerTimeSource.now().addSeconds(2));

    bool job1Executed = false;
    bool job2Executed = false;

    // 'post' a job to be executed in 1 second
    ex1.post(bdlf::BindUtil::bind(Assign(), &job1Executed, 1));

    // 'post' a job to be executed in 2 second
    ex2.post(bdlf::BindUtil::bind(Assign(), &job2Executed, 2));

    // advance time to 1 sec
    eventSchedulerTimeSource.advanceTime(bsls::TimeInterval(0).addSeconds(1));

    // job1 executed, job2 is not
    ASSERT_EQ(job1Executed, true);
    ASSERT_EQ(job2Executed, false);

    // advance time to 1 sec
    eventSchedulerTimeSource.advanceTime(bsls::TimeInterval(0).addSeconds(1));

    // both jobs executed
    ASSERT_EQ(job1Executed, true);
    ASSERT_EQ(job2Executed, true);

    // stop scheduler
    eventScheduler.stop();
}

static void test3_swap()
// ------------------------------------------------------------------------
// SWAP
//
// Concerns:
//   Ensure proper behavior of the 'swap' method.
//
// Plan:
//   Given two executor objects 'ex1' and 'ex2' which associated execution
//   contexts are 'eventScheduler1' and 'eventScheduler2' and associated
//   time points are #1 and #2 respectively, call 'ex1.swap(ex2)' and check
//   that 'ex1' now refers to 'eventScheduler2' and have a time point of
//   #2, and 'ex2' now refers to 'eventScheduler1' and have a time point of
//   #1.
//
// Testing:
//   mwcex::BdlmtEventSchedulerExecutor::swap
// ------------------------------------------------------------------------
{
    bslma::TestAllocator  alloc;
    bdlmt::EventScheduler eventScheduler1(&alloc);
    bdlmt::EventScheduler eventScheduler2(&alloc);

    // create executors
    mwcex::BdlmtEventSchedulerExecutor ex1(&eventScheduler1,
                                           bsls::TimeInterval(1));
    mwcex::BdlmtEventSchedulerExecutor ex2(&eventScheduler2,
                                           bsls::TimeInterval(2));

    // do swap
    ex1.swap(ex2);

    // check
    ASSERT_EQ(&ex1.context(), &eventScheduler2);
    ASSERT_EQ(ex1.timePoint(), bsls::TimeInterval(2));

    ASSERT_EQ(&ex2.context(), &eventScheduler1);
    ASSERT_EQ(ex2.timePoint(), bsls::TimeInterval(1));
}

static void test4_context()
// ------------------------------------------------------------------------
// CONTEXT
//
// Concerns:
//   Ensure proper behavior of the 'context' method.
//
// Plan:
//   Check that 'context()' returns a reference to the executor's
//   associated 'bdlmt::EventScheduler' object.
//
// Testing:
//   mwcex::BdlmtEventSchedulerExecutor::context
// ------------------------------------------------------------------------
{
    bslma::TestAllocator  alloc;
    bdlmt::EventScheduler eventScheduler1(&alloc);
    bdlmt::EventScheduler eventScheduler2(&alloc);

    mwcex::BdlmtEventSchedulerExecutor ex1(&eventScheduler1);
    ASSERT_EQ(&ex1.context(), &eventScheduler1);

    mwcex::BdlmtEventSchedulerExecutor ex2(&eventScheduler2);
    ASSERT_EQ(&ex2.context(), &eventScheduler2);
}

static void test5_timePoint()
// ------------------------------------------------------------------------
// TIME POINT
//
// Concerns:
//   Ensure proper behavior of the 'timePoint' method.
//
// Plan:
//   Check that 'timePoint()' returns the executor's associated time point.
//
// Testing:
//   mwcex::BdlmtEventSchedulerExecutor::timePoint
// ------------------------------------------------------------------------
{
    bslma::TestAllocator  alloc;
    bdlmt::EventScheduler eventScheduler(&alloc);

    mwcex::BdlmtEventSchedulerExecutor ex1(&eventScheduler,
                                           bsls::TimeInterval(1));
    ASSERT_EQ(ex1.timePoint(), bsls::TimeInterval(1));

    mwcex::BdlmtEventSchedulerExecutor ex2(&eventScheduler,
                                           bsls::TimeInterval(2));
    ASSERT_EQ(ex2.timePoint(), bsls::TimeInterval(2));
}

static void test6_rebindTimePoint()
// ------------------------------------------------------------------------
// REBIND TIME POINT
//
// Concerns:
//   Ensure proper behavior of the 'rebindTimePoint' method.
//
// Plan:
//   Check that 'rebindTimePoint()' returns a new executor object having
//   the same execution context as the original object and the specified
//   time point.
//
// Testing:
//   mwcex::BdlmtEventSchedulerExecutor::rebindTimePoint
// ------------------------------------------------------------------------
{
    bslma::TestAllocator  alloc;
    bdlmt::EventScheduler eventScheduler(&alloc);

    mwcex::BdlmtEventSchedulerExecutor ex1(&eventScheduler,
                                           bsls::TimeInterval(1));

    mwcex::BdlmtEventSchedulerExecutor ex2 = ex1.rebindTimePoint(
        bsls::TimeInterval(2));
    ASSERT_EQ(&ex2.context(), &eventScheduler);
    ASSERT_EQ(ex2.timePoint(), bsls::TimeInterval(2));
}

static void test7_comparison()
// ------------------------------------------------------------------------
// COMPARISON
//
// Concerns:
//   Ensure proper behavior of comparison operators.
//
// Plan:
//   Check that two executors compares equal only if they refer to the same
//   execution context and have the same time point, and compares unequal
//   otherwise.
//
// Testing:
//   mwcex::BdlmtEventSchedulerExecutor's equality comp. operator
//   mwcex::BdlmtEventSchedulerExecutor's inequality comp. operator
// ------------------------------------------------------------------------
{
    bslma::TestAllocator  alloc;
    bdlmt::EventScheduler eventScheduler1(&alloc);
    bdlmt::EventScheduler eventScheduler2(&alloc);

    // equality
    {
        ASSERT_EQ(
            mwcex::BdlmtEventSchedulerExecutor(&eventScheduler1,
                                               bsls::TimeInterval(1)) ==
                mwcex::BdlmtEventSchedulerExecutor(&eventScheduler1,
                                                   bsls::TimeInterval(1)),
            true);

        ASSERT_EQ(
            mwcex::BdlmtEventSchedulerExecutor(&eventScheduler1,
                                               bsls::TimeInterval(1)) ==
                mwcex::BdlmtEventSchedulerExecutor(&eventScheduler2,
                                                   bsls::TimeInterval(1)),
            false);

        ASSERT_EQ(
            mwcex::BdlmtEventSchedulerExecutor(&eventScheduler1,
                                               bsls::TimeInterval(1)) ==
                mwcex::BdlmtEventSchedulerExecutor(&eventScheduler1,
                                                   bsls::TimeInterval(2)),
            false);
    }

    // inequality
    {
        ASSERT_EQ(
            mwcex::BdlmtEventSchedulerExecutor(&eventScheduler1,
                                               bsls::TimeInterval(1)) !=
                mwcex::BdlmtEventSchedulerExecutor(&eventScheduler1,
                                                   bsls::TimeInterval(1)),
            false);

        ASSERT_EQ(
            mwcex::BdlmtEventSchedulerExecutor(&eventScheduler1,
                                               bsls::TimeInterval(1)) !=
                mwcex::BdlmtEventSchedulerExecutor(&eventScheduler2,
                                                   bsls::TimeInterval(1)),
            true);

        ASSERT_EQ(
            mwcex::BdlmtEventSchedulerExecutor(&eventScheduler1,
                                               bsls::TimeInterval(1)) !=
                mwcex::BdlmtEventSchedulerExecutor(&eventScheduler1,
                                                   bsls::TimeInterval(2)),
            true);
    }
}

static void test8_defaultTimePoint()
// ------------------------------------------------------------------------
// DEFAULT TIME POINT
//
// Concerns:
//   Ensure the default time point is in the past relative to any clock.
//
// Plan:
//   Check that 'k_DEFAULT_TIME_POINT' is in the past relative to monotonic
//   and realtime clocks.
//
// Testing:
//   mwcex::BdlmtEventSchedulerExecutor::k_DEFAULT_TIME_POINT
// ------------------------------------------------------------------------
{
    const bsls::TimeInterval defTimePoint =
        mwcex::BdlmtEventSchedulerExecutor::k_DEFAULT_TIME_POINT;

    ASSERT(defTimePoint < bsls::SystemTime::nowMonotonicClock());
    ASSERT(defTimePoint < bsls::SystemTime::nowRealtimeClock());
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(mwctst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 1: test1_constructor(); break;
    case 2: test2_post(); break;
    case 3: test3_swap(); break;
    case 4: test4_context(); break;
    case 5: test5_timePoint(); break;
    case 6: test6_rebindTimePoint(); break;
    case 7: test7_comparison(); break;
    case 8: test8_defaultTimePoint(); break;

    default: {
        bsl::cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND."
                  << bsl::endl;
        s_testStatus = -1;
    } break;
    }

    TEST_EPILOG(mwctst::TestHelper::e_CHECK_GBL_ALLOC);
}
