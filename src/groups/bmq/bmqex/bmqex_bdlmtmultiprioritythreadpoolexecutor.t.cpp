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

// bmqex_bdlmtmultiprioritythreadpoolexecutor.t.cpp                   -*-C++-*-
#include <bmqex_bdlmtmultiprioritythreadpoolexecutor.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// BDE
#include <bdlf_bind.h>
#include <bdlmt_multiprioritythreadpool.h>
#include <bsl_vector.h>
#include <bslma_testallocator.h>

// CONVENIENCE
using namespace BloombergLP;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {

// ===============
// struct PushBack
// ===============

/// Provides a function object that calls `push_back` on a specified
/// `container` with a specified `value` argument.
struct PushBack {
    // TYPES

    /// Defines the result type of the call operator.
    typedef void ResultType;

    template <class CONTAINER, class VALUE>
    void operator()(CONTAINER* container, const VALUE& value) const
    {
        container->push_back(value);
    }
};

}  // close unnamed namespace

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
//   Construct an instance of 'bmqex::BdlmtMultipriorityThreadPoolExecutor'
//   and check postconditions.
//
// Testing:
//   bmqex::BdlmtMultipriorityThreadPoolExecutor's constructor
// ------------------------------------------------------------------------
{
    bslma::TestAllocator           alloc;
    bdlmt::MultipriorityThreadPool threadPool(1,  // numThreads
                                              1,  // numPriorities
                                              &alloc);

    // create executor
    bmqex::BdlmtMultipriorityThreadPoolExecutor ex(&threadPool,
                                                   42);  // priority

    // check postconditions
    ASSERT_EQ(&ex.context(), &threadPool);
    ASSERT_EQ(ex.priority(), 42);
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
//   - Are using the executor's associated priority.
//
// Testing:
//   bmqex::BdlmtMultipriorityThreadPoolExecutor::post
// ------------------------------------------------------------------------
{
    bslma::TestAllocator           alloc;
    bdlmt::MultipriorityThreadPool threadPool(1,  // numThreads
                                              2,  // numPriorities
                                              &alloc);

    // start thread pool
    int rc = threadPool.startThreads();
    BSLS_ASSERT_OPT(rc == 0);

    // pause thread pool
    threadPool.suspendProcessing();

    // create executors with different priorities
    bmqex::BdlmtMultipriorityThreadPoolExecutor ex1(&threadPool, 1);
    bmqex::BdlmtMultipriorityThreadPoolExecutor ex2(&threadPool, 0);

    // store job results here
    bsl::vector<int> out(&alloc);

    // 'post' a job with priority 1
    ex1.post(bdlf::BindUtil::bind(PushBack(), &out, 100));

    // 'post' a job with priority 0
    ex2.post(bdlf::BindUtil::bind(PushBack(), &out, 200));

    // resume thread pool
    threadPool.resumeProcessing();

    // join and stop the thread pool
    threadPool.drainJobs();
    threadPool.stopThreads();

    // jobs executed in priority order
    ASSERT_EQ(out.size(), static_cast<size_t>(2));
    ASSERT_EQ(out[0], 200);
    ASSERT_EQ(out[1], 100);
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
//   contexts are 'threadPool1' and 'threadPool2' and associated priorities
//   are 1 and 2 respectively, call 'ex1.swap(ex2)' and check that 'ex1'
//   now refers to 'threadPool2' and have a priority of 2, and 'ex2' now
//   refers to 'threadPool1' and have a priority of 1.
//
// Testing:
//   bmqex::BdlmtMultipriorityThreadPoolExecutor::swap
// ------------------------------------------------------------------------
{
    bslma::TestAllocator           alloc;
    bdlmt::MultipriorityThreadPool threadPool1(1,  // numThreads
                                               1,  // numPriorities
                                               &alloc);
    bdlmt::MultipriorityThreadPool threadPool2(1,  // numThreads
                                               1,  // numPriorities
                                               &alloc);

    // create executors
    bmqex::BdlmtMultipriorityThreadPoolExecutor ex1(&threadPool1,
                                                    1);  // priority
    bmqex::BdlmtMultipriorityThreadPoolExecutor ex2(&threadPool2,
                                                    2);  // priority

    // do swap
    ex1.swap(ex2);

    // check
    ASSERT_EQ(&ex1.context(), &threadPool2);
    ASSERT_EQ(ex1.priority(), 2);

    ASSERT_EQ(&ex2.context(), &threadPool1);
    ASSERT_EQ(ex2.priority(), 1);
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
//   associated 'bdlmt::MultipriorityThreadPool' object.
//
// Testing:
//   bmqex::BdlmtMultipriorityThreadPoolExecutor::context
// ------------------------------------------------------------------------
{
    bslma::TestAllocator           alloc;
    bdlmt::MultipriorityThreadPool threadPool1(1,  // numThreads
                                               1,  // numPriorities
                                               &alloc);
    bdlmt::MultipriorityThreadPool threadPool2(1,  // numThreads
                                               1,  // numPriorities
                                               &alloc);

    bmqex::BdlmtMultipriorityThreadPoolExecutor ex1(&threadPool1);
    ASSERT_EQ(&ex1.context(), &threadPool1);

    bmqex::BdlmtMultipriorityThreadPoolExecutor ex2(&threadPool2);
    ASSERT_EQ(&ex2.context(), &threadPool2);
}

static void test5_priority()
// ------------------------------------------------------------------------
// PRIORITY
//
// Concerns:
//   Ensure proper behavior of the 'priority' method.
//
// Plan:
//   Check that 'priority()' returns the executors's associated priority.
//
// Testing:
//   bmqex::BdlmtMultipriorityThreadPoolExecutor::priority
// ------------------------------------------------------------------------
{
    bslma::TestAllocator           alloc;
    bdlmt::MultipriorityThreadPool threadPool(1,  // numThreads
                                              1,  // numPriorities
                                              &alloc);

    bmqex::BdlmtMultipriorityThreadPoolExecutor ex1(&threadPool,
                                                    1);  // priority
    ASSERT_EQ(ex1.priority(), 1);

    bmqex::BdlmtMultipriorityThreadPoolExecutor ex2(&threadPool,
                                                    2);  // priority
    ASSERT_EQ(ex2.priority(), 2);
}

static void test6_rebindPriority()
// ------------------------------------------------------------------------
// REBIND PRIORITY
//
// Concerns:
//   Ensure proper behavior of the 'rebindPriority' method.
//
// Plan:
//   Check that 'rebindPriority()' returns a new executor object having the
//   same execution context as the original object and the specified
//   priority.
//
// Testing:
//   bmqex::BdlmtMultipriorityThreadPoolExecutor::rebindPriority
// ------------------------------------------------------------------------
{
    bslma::TestAllocator           alloc;
    bdlmt::MultipriorityThreadPool threadPool(1,  // numThreads
                                              1,  // numPriorities
                                              &alloc);

    bmqex::BdlmtMultipriorityThreadPoolExecutor ex1(&threadPool,
                                                    1);  // priority

    bmqex::BdlmtMultipriorityThreadPoolExecutor ex2 = ex1.rebindPriority(2);
    ASSERT_EQ(&ex2.context(), &threadPool);
    ASSERT_EQ(ex2.priority(), 2);
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
//   execution context and have the same priority, and compares unequal
//   otherwise.
//
// Testing:
//   bmqex::BdlmtMultipriorityThreadPoolExecutor's equality comp. op.
//   bmqex::BdlmtMultipriorityThreadPoolExecutor's inequality comp. op.
// ------------------------------------------------------------------------
{
    bslma::TestAllocator           alloc;
    bdlmt::MultipriorityThreadPool threadPool1(1,  // numThreads
                                               1,  // numPriorities
                                               &alloc);
    bdlmt::MultipriorityThreadPool threadPool2(1,  // numThreads
                                               1,  // numPriorities
                                               &alloc);

    // equality
    {
        ASSERT_EQ(
            bmqex::BdlmtMultipriorityThreadPoolExecutor(&threadPool1, 1) ==
                bmqex::BdlmtMultipriorityThreadPoolExecutor(&threadPool1, 1),
            true);

        ASSERT_EQ(
            bmqex::BdlmtMultipriorityThreadPoolExecutor(&threadPool1, 1) ==
                bmqex::BdlmtMultipriorityThreadPoolExecutor(&threadPool2, 1),
            false);

        ASSERT_EQ(
            bmqex::BdlmtMultipriorityThreadPoolExecutor(&threadPool1, 1) ==
                bmqex::BdlmtMultipriorityThreadPoolExecutor(&threadPool1, 2),
            false);
    }

    // inequality
    {
        ASSERT_EQ(
            bmqex::BdlmtMultipriorityThreadPoolExecutor(&threadPool1, 1) !=
                bmqex::BdlmtMultipriorityThreadPoolExecutor(&threadPool1, 1),
            false);

        ASSERT_EQ(
            bmqex::BdlmtMultipriorityThreadPoolExecutor(&threadPool1, 1) !=
                bmqex::BdlmtMultipriorityThreadPoolExecutor(&threadPool2, 1),
            true);

        ASSERT_EQ(
            bmqex::BdlmtMultipriorityThreadPoolExecutor(&threadPool1, 1) !=
                bmqex::BdlmtMultipriorityThreadPoolExecutor(&threadPool1, 2),
            true);
    }
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 1: test1_constructor(); break;
    case 2: test2_post(); break;
    case 3: test3_swap(); break;
    case 4: test4_context(); break;
    case 5: test5_priority(); break;
    case 6: test6_rebindPriority(); break;
    case 7: test7_comparison(); break;

    default: {
        bsl::cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND."
                  << bsl::endl;
        s_testStatus = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
