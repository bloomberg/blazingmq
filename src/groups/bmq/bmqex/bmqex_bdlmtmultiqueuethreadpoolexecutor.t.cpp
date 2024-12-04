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

// bmqex_bdlmtmultiqueuethreadpoolexecutor.t.cpp                      -*-C++-*-
#include <bmqex_bdlmtmultiqueuethreadpoolexecutor.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// BDE
#include <bdlf_bind.h>
#include <bdlmt_multiqueuethreadpool.h>
#include <bslma_testallocator.h>
#include <bslmt_threadattributes.h>

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
//   Construct an instance of 'bmqex::BdlmtMultiQueueThreadPoolExecutor'
//   and check postconditions.
//
// Testing:
//   bmqex::BdlmtMultiQueueThreadPoolExecutor's constructor
// ------------------------------------------------------------------------
{
    bslma::TestAllocator        alloc;
    bdlmt::MultiQueueThreadPool threadPool(bslmt::ThreadAttributes(),
                                           1,  // minThreads
                                           1,  // maxThreads
                                           1,  // maxIdleTime
                                           &alloc);

    // create executor
    bmqex::BdlmtMultiQueueThreadPoolExecutor ex(&threadPool,
                                                42);  // queueId

    // check postconditions
    ASSERT_EQ(&ex.context(), &threadPool);
    ASSERT_EQ(ex.queueId(), 42);
}

static void test2_postDispatch()
// ------------------------------------------------------------------------
// POST DISPATCH
//
// Concerns:
//   Ensure proper behavior of 'post' and 'dispatch' methods.
//
// Plan:
//   Check that 'post' and 'dispatch' are:
//   - Forwarding the specified function object to the executor's
//     associated execution context;
//   - Are using the executor's associated queue ids.
//
// Testing:
//   bmqex::BdlmtMultiQueueThreadPoolExecutor::post
//   bmqex::BdlmtMultiQueueThreadPoolExecutor::dispatch
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // MultiQueueThreadPool_Queue::executeFront() creates a vector on the
    // stack, so uses the default allocator.

    bmqtst::TestHelperUtil::ignoreCheckGblAlloc() = true;
    // MultiQueueThreadPool::start() implementation uses the global
    // allocator.

    bslma::TestAllocator        alloc;
    bdlmt::MultiQueueThreadPool threadPool(bslmt::ThreadAttributes(),
                                           1,  // minThreads
                                           1,  // maxThreads
                                           1,  // maxIdleTime
                                           &alloc);

    // start thread pool
    int rc = threadPool.start();
    BSLS_ASSERT_OPT(rc == 0);

    // create queues
    int queue1Id = threadPool.createQueue();
    BSLS_ASSERT_OPT(queue1Id);

    int queue2Id = threadPool.createQueue();
    BSLS_ASSERT_OPT(queue2Id);

    // pause queues
    rc = threadPool.pauseQueue(queue1Id);
    BSLS_ASSERT_OPT(rc == 0);

    rc = threadPool.pauseQueue(queue2Id);
    BSLS_ASSERT_OPT(rc == 0);

    // create executors
    bmqex::BdlmtMultiQueueThreadPoolExecutor ex1(&threadPool, queue1Id);
    bmqex::BdlmtMultiQueueThreadPoolExecutor ex2(&threadPool, queue2Id);

    // both queues are empty
    ASSERT_EQ(threadPool.numElements(queue1Id), 0);
    ASSERT_EQ(threadPool.numElements(queue2Id), 0);

    // 'post' a job
    bool job1Complete = false;
    ex1.post(bdlf::BindUtil::bind(Assign(), &job1Complete, true));

    // job added to first queue
    ASSERT_EQ(threadPool.numElements(queue1Id), 1);
    ASSERT_EQ(threadPool.numElements(queue2Id), 0);

    // 'dispatch' a job
    bool job2Complete = false;
    ex2.dispatch(bdlf::BindUtil::bind(Assign(), &job2Complete, true));

    // job added to second queue
    ASSERT_EQ(threadPool.numElements(queue1Id), 1);
    ASSERT_EQ(threadPool.numElements(queue2Id), 1);

    // resume queues
    rc = threadPool.resumeQueue(queue1Id);
    BSLS_ASSERT_OPT(rc == 0);

    rc = threadPool.resumeQueue(queue2Id);
    BSLS_ASSERT_OPT(rc == 0);

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
//   contexts are 'threadPool1' and 'threadPool2' and associated queue ids
//   are 1 and 2 respectively, call 'ex1.swap(ex2)' and check that 'ex1'
//   now refers to 'threadPool2' and have a queue id of 2, and 'ex2' now
//   refers to 'threadPool1' and have a queue id of 1.
//
// Testing:
//   bmqex::BdlmtMultiQueueThreadPoolExecutor::swap
// ------------------------------------------------------------------------
{
    bslma::TestAllocator        alloc;
    bdlmt::MultiQueueThreadPool threadPool1(bslmt::ThreadAttributes(),
                                            1,  // minThreads
                                            1,  // maxThreads
                                            1,  // maxIdleTime
                                            &alloc);
    bdlmt::MultiQueueThreadPool threadPool2(bslmt::ThreadAttributes(),
                                            1,  // minThreads
                                            1,  // maxThreads
                                            1,  // maxIdleTime
                                            &alloc);

    // create executors
    bmqex::BdlmtMultiQueueThreadPoolExecutor ex1(&threadPool1,
                                                 1);  // queueId
    bmqex::BdlmtMultiQueueThreadPoolExecutor ex2(&threadPool2,
                                                 2);  // queueId

    // do swap
    ex1.swap(ex2);

    // check
    ASSERT_EQ(&ex1.context(), &threadPool2);
    ASSERT_EQ(ex1.queueId(), 2);

    ASSERT_EQ(&ex2.context(), &threadPool1);
    ASSERT_EQ(ex2.queueId(), 1);
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
//   associated 'bdlmt::MultiQueueThreadPool' object.
//
// Testing:
//   bmqex::BdlmtMultiQueueThreadPoolExecutor::context
// ------------------------------------------------------------------------
{
    bslma::TestAllocator        alloc;
    bdlmt::MultiQueueThreadPool threadPool1(bslmt::ThreadAttributes(),
                                            1,  // minThreads
                                            1,  // maxThreads
                                            1,  // maxIdleTime
                                            &alloc);
    bdlmt::MultiQueueThreadPool threadPool2(bslmt::ThreadAttributes(),
                                            1,  // minThreads
                                            1,  // maxThreads
                                            1,  // maxIdleTime
                                            &alloc);

    bmqex::BdlmtMultiQueueThreadPoolExecutor ex1(&threadPool1,
                                                 42);  // queueId
    ASSERT_EQ(&ex1.context(), &threadPool1);

    bmqex::BdlmtMultiQueueThreadPoolExecutor ex2(&threadPool2,
                                                 42);  // queueId
    ASSERT_EQ(&ex2.context(), &threadPool2);
}

static void test5_queueId()
// ------------------------------------------------------------------------
// QUEUE ID
//
// Concerns:
//   Ensure proper behavior of the 'queueId' method.
//
// Plan:
//   Check that 'queueId()' returns the executors's associated queue id.
//
// Testing:
//   bmqex::BdlmtMultiQueueThreadPoolExecutor::queueId
// ------------------------------------------------------------------------
{
    bslma::TestAllocator        alloc;
    bdlmt::MultiQueueThreadPool threadPool(bslmt::ThreadAttributes(),
                                           1,  // minThreads
                                           1,  // maxThreads
                                           1,  // maxIdleTime
                                           &alloc);

    bmqex::BdlmtMultiQueueThreadPoolExecutor ex1(&threadPool,
                                                 1);  // queueId
    ASSERT_EQ(ex1.queueId(), 1);

    bmqex::BdlmtMultiQueueThreadPoolExecutor ex2(&threadPool,
                                                 2);  // queueId
    ASSERT_EQ(ex2.queueId(), 2);
}

static void test6_rebindQueueId()
// ------------------------------------------------------------------------
// REBIND QUEUE ID
//
// Concerns:
//   Ensure proper behavior of the 'rebindQueueId' method.
//
// Plan:
//   Check that 'rebindQueueId()' returns a new executor object having the
//   same execution context as the original object and the specified queue
//   id.
//
// Testing:
//   bmqex::BdlmtMultiQueueThreadPoolExecutor::rebindQueueId
// ------------------------------------------------------------------------
{
    bslma::TestAllocator        alloc;
    bdlmt::MultiQueueThreadPool threadPool(bslmt::ThreadAttributes(),
                                           1,  // minThreads
                                           1,  // maxThreads
                                           1,  // maxIdleTime
                                           &alloc);

    bmqex::BdlmtMultiQueueThreadPoolExecutor ex1(&threadPool,
                                                 1);  // queueId

    bmqex::BdlmtMultiQueueThreadPoolExecutor ex2 = ex1.rebindQueueId(2);
    ASSERT_EQ(&ex2.context(), &threadPool);
    ASSERT_EQ(ex2.queueId(), 2);
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
//   execution context and have the same queue id, and compares unequal
//   otherwise.
//
// Testing:
//   bmqex::BdlmtMultiQueueThreadPoolExecutor's equality comp. operator
//   bmqex::BdlmtMultiQueueThreadPoolExecutor's inequality comp. operator
// ------------------------------------------------------------------------
{
    bslma::TestAllocator        alloc;
    bdlmt::MultiQueueThreadPool threadPool1(bslmt::ThreadAttributes(),
                                            1,  // minThreads
                                            1,  // maxThreads
                                            1,  // maxIdleTime
                                            &alloc);
    bdlmt::MultiQueueThreadPool threadPool2(bslmt::ThreadAttributes(),
                                            1,  // minThreads
                                            1,  // maxThreads
                                            1,  // maxIdleTime
                                            &alloc);

    // equality
    {
        ASSERT_EQ(bmqex::BdlmtMultiQueueThreadPoolExecutor(&threadPool1, 1) ==
                      bmqex::BdlmtMultiQueueThreadPoolExecutor(&threadPool1,
                                                               1),
                  true);

        ASSERT_EQ(bmqex::BdlmtMultiQueueThreadPoolExecutor(&threadPool1, 1) ==
                      bmqex::BdlmtMultiQueueThreadPoolExecutor(&threadPool2,
                                                               1),
                  false);

        ASSERT_EQ(bmqex::BdlmtMultiQueueThreadPoolExecutor(&threadPool1, 1) ==
                      bmqex::BdlmtMultiQueueThreadPoolExecutor(&threadPool1,
                                                               2),
                  false);
    }

    // inequality
    {
        ASSERT_EQ(bmqex::BdlmtMultiQueueThreadPoolExecutor(&threadPool1, 1) !=
                      bmqex::BdlmtMultiQueueThreadPoolExecutor(&threadPool1,
                                                               1),
                  false);

        ASSERT_EQ(bmqex::BdlmtMultiQueueThreadPoolExecutor(&threadPool1, 1) !=
                      bmqex::BdlmtMultiQueueThreadPoolExecutor(&threadPool2,
                                                               1),
                  true);

        ASSERT_EQ(bmqex::BdlmtMultiQueueThreadPoolExecutor(&threadPool1, 1) !=
                      bmqex::BdlmtMultiQueueThreadPoolExecutor(&threadPool1,
                                                               2),
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
    case 2: test2_postDispatch(); break;
    case 3: test3_swap(); break;
    case 4: test4_context(); break;
    case 5: test5_queueId(); break;
    case 6: test6_rebindQueueId(); break;
    case 7: test7_comparison(); break;

    default: {
        bsl::cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND."
                  << bsl::endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
