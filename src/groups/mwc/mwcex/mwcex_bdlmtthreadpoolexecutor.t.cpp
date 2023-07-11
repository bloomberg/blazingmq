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

// mwcex_bdlmtthreadpoolexecutor.t.cpp                                -*-C++-*-
#include <mwcex_bdlmtthreadpoolexecutor.h>

// TEST DRIVER
#include <mwctst_testhelper.h>

// BDE
#include <bdlf_bind.h>
#include <bdlmt_threadpool.h>
#include <bslma_testallocator.h>
#include <bslmt_threadattributes.h>
#include <bsls_assert.h>

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
//   Construct an instance of 'mwcex::BdlmtThreadPoolExecutor' and check
//   postconditions.
//
// Testing:
//   mwcex::BdlmtThreadPoolExecutor's constructor
// ------------------------------------------------------------------------
{
    bslma::TestAllocator alloc;
    bdlmt::ThreadPool    threadPool(bslmt::ThreadAttributes(),
                                 1,  // minThreads
                                 1,  // maxThreads
                                 1,  // maxIdleTime
                                 &alloc);

    // create executor
    mwcex::BdlmtThreadPoolExecutor ex(&threadPool);

    // check postconditions
    ASSERT_EQ(&ex.context(), &threadPool);
}

static void test2_post()
// ------------------------------------------------------------------------
// POST
//
// Concerns:
//   Ensure proper behavior of the 'post' method.
//
// Plan:
//   Check that 'post' is forwarding the specified function object to the
//   executor's associated execution context.
//
// Testing:
//   mwcex::BdlmtThreadPoolExecutor::post
// ------------------------------------------------------------------------
{
    bslma::TestAllocator alloc;
    bdlmt::ThreadPool    threadPool(bslmt::ThreadAttributes(),
                                 1,  // minThreads
                                 1,  // maxThreads
                                 1,  // maxIdleTime
                                 &alloc);

    // start thread pool
    mwcex::BdlmtThreadPoolExecutor ex(&threadPool);

    // start the execution context
    int rc = threadPool.start();
    BSLS_ASSERT_OPT(rc == 0);

    // 'post' a job
    bool job1Complete = false;
    ex.post(bdlf::BindUtil::bind(Assign(), &job1Complete, true));

    // 'post' a job
    bool job2Complete = false;
    ex.post(bdlf::BindUtil::bind(Assign(), &job2Complete, true));

    // join and stop the thread pool
    threadPool.stop();

    // both jobs executed
    ASSERT(job1Complete);
    ASSERT(job2Complete);
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
//   contexts are 'threadPool1' and 'threadPool2' respectively, call
//   'ex1.swap(ex2)' and check that 'ex1' now refers to 'threadPool2' and
//   'ex2' now refers to 'threadPool1'.
//
// Testing:
//   mwcex::BdlmtThreadPoolExecutor::swap
// ------------------------------------------------------------------------
{
    bslma::TestAllocator alloc;
    bdlmt::ThreadPool    threadPool1(bslmt::ThreadAttributes(),
                                  1,  // minThreads
                                  1,  // maxThreads
                                  1,  // maxIdleTime
                                  &alloc);
    bdlmt::ThreadPool    threadPool2(bslmt::ThreadAttributes(),
                                  1,  // minThreads
                                  1,  // maxThreads
                                  1,  // maxIdleTime
                                  &alloc);

    // create executors
    mwcex::BdlmtThreadPoolExecutor ex1(&threadPool1);
    mwcex::BdlmtThreadPoolExecutor ex2(&threadPool2);

    // do swap
    ex1.swap(ex2);

    // check
    ASSERT_EQ(&ex1.context(), &threadPool2);
    ASSERT_EQ(&ex2.context(), &threadPool1);
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
//   associated 'bdlmt::ThreadPool' object.
//
// Testing:
//   mwcex::BdlmtThreadPoolExecutor::context
// ------------------------------------------------------------------------
{
    bslma::TestAllocator alloc;
    bdlmt::ThreadPool    threadPool1(bslmt::ThreadAttributes(),
                                  1,  // minThreads
                                  1,  // maxThreads
                                  1,  // maxIdleTime
                                  &alloc);
    bdlmt::ThreadPool    threadPool2(bslmt::ThreadAttributes(),
                                  1,  // minThreads
                                  1,  // maxThreads
                                  1,  // maxIdleTime
                                  &alloc);

    mwcex::BdlmtThreadPoolExecutor ex1(&threadPool1);
    ASSERT_EQ(&ex1.context(), &threadPool1);

    mwcex::BdlmtThreadPoolExecutor ex2(&threadPool2);
    ASSERT_EQ(&ex2.context(), &threadPool2);
}

static void test5_comparison()
// ------------------------------------------------------------------------
// COMPARISON
//
// Concerns:
//   Ensure proper behavior of comparison operators.
//
// Plan:
//   Check that two executors compares equal if they refer to the same
//   execution context and vice versa.
//
// Testing:
//   mwcex::BdlmtThreadPoolExecutor's equality comparison operator
//   mwcex::BdlmtThreadPoolExecutor's inequality comparison operator
// ------------------------------------------------------------------------
{
    bslma::TestAllocator alloc;
    bdlmt::ThreadPool    threadPool1(bslmt::ThreadAttributes(),
                                  1,  // minThreads
                                  1,  // maxThreads
                                  1,  // maxIdleTime
                                  &alloc);
    bdlmt::ThreadPool    threadPool2(bslmt::ThreadAttributes(),
                                  1,  // minThreads
                                  1,  // maxThreads
                                  1,  // maxIdleTime
                                  &alloc);

    // equality
    {
        ASSERT_EQ(mwcex::BdlmtThreadPoolExecutor(&threadPool1) ==
                      mwcex::BdlmtThreadPoolExecutor(&threadPool1),
                  true);

        ASSERT_EQ(mwcex::BdlmtThreadPoolExecutor(&threadPool1) ==
                      mwcex::BdlmtThreadPoolExecutor(&threadPool2),
                  false);
    }

    // inequality
    {
        ASSERT_EQ(mwcex::BdlmtThreadPoolExecutor(&threadPool1) !=
                      mwcex::BdlmtThreadPoolExecutor(&threadPool1),
                  false);

        ASSERT_EQ(mwcex::BdlmtThreadPoolExecutor(&threadPool1) !=
                      mwcex::BdlmtThreadPoolExecutor(&threadPool2),
                  true);
    }
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
    case 5: test5_comparison(); break;

    default: {
        bsl::cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND."
                  << bsl::endl;
        s_testStatus = -1;
    } break;
    }

    TEST_EPILOG(mwctst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
