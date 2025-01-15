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

// bmqex_systemexecutor.t.cpp                                         -*-C++-*-
#include <bmqex_systemexecutor.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

#include <bmqsys_threadutil.h>

// BDE
#include <bdlf_bind.h>
#include <bsl_functional.h>  // bsl::cref
#include <bslma_testallocator.h>
#include <bslmt_semaphore.h>
#include <bslmt_threadattributes.h>
#include <bslmt_threadutil.h>
#include <bsls_assert.h>
#include <bsls_atomic.h>
#include <bsls_timeinterval.h>

// CONVENIENCE
using namespace BloombergLP;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {

// ========================
// struct AsyncTestCallback
// ========================

/// Provides a functor to be invoked asynchronously.
struct AsyncTestCallback {
    void operator()(bslmt::Semaphore*      waitSem,
                    int*                   incIndex,
                    bslmt::ThreadUtil::Id* threadId,
                    bslmt::Semaphore*      postSem) const
    {
        waitSem->wait();  // wait permission to continue
        ++(*incIndex);
        *threadId = bslmt::ThreadUtil::selfId();
        postSem->post();  // notify we are done
    }
};

// ==========================
// struct InPlaceTestCallback
// ==========================

/// Provides a functor to be invoked in-place.
struct InPlaceTestCallback {
    void operator()(int* incIndex, bslmt::ThreadUtil::Id* threadId) const
    {
        ++(*incIndex);
        *threadId = bslmt::ThreadUtil::selfId();
    }
};

// UTILITY FUNCTIONS
void onThreadExit(void* threadsCompleted)
{
    // sleep a bit
    bslmt::ThreadUtil::sleep(bsls::TimeInterval(0.25));

    // increment the number of threads completed
    ++(*static_cast<bsls::AtomicInt*>(threadsCompleted));
}

}  // close unnamed namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_context_singleton()
// ------------------------------------------------------------------------
// CONTEXT SINGLETON
//
// Concerns:
//   Ensure proper behavior of the 'singleton' function.
//
// Plan:
//   Check that 'singleton()' always returns a reference to the same
//   object.
//
// Testing:
//   bmqex::SystemExecutor::singleton
// ------------------------------------------------------------------------
{
    bmqex::SystemExecutor_Context* ctx1 =
        &bmqex::SystemExecutor_Context::singleton();
    bmqex::SystemExecutor_Context* ctx2 =
        &bmqex::SystemExecutor_Context::singleton();

    BMQTST_ASSERT_EQ(ctx1, ctx2);
}

static void test2_context_creators()
// ------------------------------------------------------------------------
// CONTEXT CREATORS
//
// Concerns:
//   Ensure proper behavior of creator methods.
//
// Plan:
//   Create an execution context. Submit several function objects by
//   calling 'executeAsync'. Destroy the execution context without
//   waiting for submitted function objects to complete. Check that after
//   the destructor returns all threads used to execute submitted function
//   objects have completed.
//
// Testing:
//   'bmqex::SystemExecutor_Context's constructor
//   'bmqex::SystemExecutor_Context's destructor
// ------------------------------------------------------------------------
{
    static const int k_NUM_JOBS = 10;

    bslma::TestAllocator alloc;

    // create a thread-local storage associated with a thread-exit handler
    bslmt::ThreadUtil::Key tlsKey;
    int rc = bslmt::ThreadUtil::createKey(&tlsKey, onThreadExit);
    BSLS_ASSERT_OPT(rc == 0);

    // initialize the context singleton object
    bmqex::SystemExecutor_Context& context =
        bmqex::SystemExecutor_Context::initSingleton(&alloc);

    // context initialized with the test allocator
    BMQTST_ASSERT_EQ(context.allocator(), &alloc);

    bsls::AtomicInt threadsCompleted(0);
    for (int i = 0; i < k_NUM_JOBS; ++i) {
        // submit a function object that attaches the thread-exit handler
        // to the current thread
        context.executeAsync(
            bdlf::BindUtil::bind(bslmt::ThreadUtil::setSpecific,
                                 bsl::cref(tlsKey),
                                 &threadsCompleted),
            bmqsys::ThreadUtil::defaultAttributes());
    }

    // destroy context singleton object
    bmqex::SystemExecutor_Context::shutdownSingleton();

    // all thread owned by the execution context have completed
    BMQTST_ASSERT_EQ(threadsCompleted, k_NUM_JOBS);

    // delete the thread-local storage
    rc = bslmt::ThreadUtil::deleteKey(tlsKey);
    BSLS_ASSERT_OPT(rc == 0);
}

static void test3_executor_creators()
// ------------------------------------------------------------------------
// EXECUTOR CREATORS
//
// Concerns:
//   Ensure proper behavior of creator methods.
//
// Plan:
//   1. Default-construct an instance of 'bmqex::SystemExecutor'. Check
//      postconditions.
//
//   2. ThreadAttributes-construct an instance of 'bmqex::SystemExecutor'.
//      Check postconditions.
//
// Testing:
//   bmqex::SystemExecutor's creators
// ------------------------------------------------------------------------
{
    bslma::TestAllocator alloc;

    // initialize the context singleton with the test allocator
    bmqex::SystemExecutorContextGuard contextGuard(&alloc);

    // 1. default-construct
    {
        bmqex::SystemExecutor ex;

        BMQTST_ASSERT(!ex.threadAttributes());
    }

    // 2. ThreadAttributes-construct
    {
        // prepare thread attributes with non-default values (except for the
        // DETACHED attribute, that has to be 'e_CREATE_JOINABLE')
        bslmt::ThreadAttributes attr(&alloc);
        attr.setDetachedState(bslmt::ThreadAttributes::e_CREATE_JOINABLE);
        attr.setGuardSize(100);
        attr.setInheritSchedule(false);
        attr.setSchedulingPolicy(bslmt::ThreadAttributes::e_SCHED_FIFO);
        attr.setSchedulingPriority(200);
        attr.setStackSize(300);
        attr.setThreadName("myThread");

        // create executor
        bmqex::SystemExecutor ex(attr, &alloc);

        // executor holds a copy of the thread attributes
        BMQTST_ASSERT(ex.threadAttributes());
        BMQTST_ASSERT(ex.threadAttributes() != &attr);

        // the copy is identical to the original value
        BMQTST_ASSERT_EQ(ex.threadAttributes()->detachedState(),
                         bslmt::ThreadAttributes::e_CREATE_JOINABLE);
        BMQTST_ASSERT_EQ(ex.threadAttributes()->guardSize(), 100);
        BMQTST_ASSERT_EQ(ex.threadAttributes()->inheritSchedule(), false);
        BMQTST_ASSERT_EQ(ex.threadAttributes()->schedulingPolicy(),
                         bslmt::ThreadAttributes::e_SCHED_FIFO);
        BMQTST_ASSERT_EQ(ex.threadAttributes()->schedulingPriority(), 200);
        BMQTST_ASSERT_EQ(ex.threadAttributes()->stackSize(), 300);
        BMQTST_ASSERT_EQ(ex.threadAttributes()->threadName(), "myThread");
    }
}

static void test4_executor_post()
// ------------------------------------------------------------------------
// EXECUTOR POST
//
// Concerns:
//   Ensure proper behavior of the 'post' method.
//
// Plan:
//   Submit several function objects to an executor by calling 'post'.
//   Check that 'post' does not block the calling thread pending completion
//   of submitted function objects, executing them in a thread other than
//   the calling one.
//
// Testing:
//   bmqex::SystemExecutor::post
// ------------------------------------------------------------------------
{
    static const int k_NUM_JOBS = 32;

    bslma::TestAllocator alloc;

    // initialize the context singleton with the test allocator
    bmqex::SystemExecutorContextGuard contextGuard(&alloc);

    bslmt::Semaphore      sem1, sem2;
    int                   jobNo    = 0;
    bslmt::ThreadUtil::Id threadId = bslmt::ThreadUtil::selfId();

    for (int i = 0; i < k_NUM_JOBS; ++i) {
        // schedule function object execution
        bmqex::SystemExecutor().post(
            bdlf::BindUtil::bindR<void>(AsyncTestCallback(),
                                        &sem1,
                                        &jobNo,
                                        &threadId,
                                        &sem2));

        // function object not executed yet
        BMQTST_ASSERT_EQ(jobNo, i);

        sem1.post();  // allow job to continue
        sem2.wait();  // wait till work is done

        // function object executed in another thread
        BMQTST_ASSERT_EQ(jobNo, i + 1);
        BMQTST_ASSERT_NE(threadId, bslmt::ThreadUtil::selfId());
    }
}

static void test5_executor_dispatch()
// ------------------------------------------------------------------------
// EXECUTOR DISPATCH
//
// Concerns:
//   Ensure proper behavior of the 'dispatch' method.
//
// Plan:
//   Submit several function objects to an executor by calling 'dispatch'.
//   Check that submitted function objects are executed in the calling
//   thread.
//
// Testing:
//   bmqex::SystemExecutor::dispatch
// ------------------------------------------------------------------------
{
    static const int k_NUM_JOBS = 32;

    bslma::TestAllocator alloc;

    // initialize the context singleton with the test allocator
    bmqex::SystemExecutorContextGuard contextGuard(&alloc);

    int                   jobNo = 0;
    bslmt::ThreadUtil::Id threadId;

    for (int i = 0; i < k_NUM_JOBS; ++i) {
        // execute function object
        bmqex::SystemExecutor().dispatch(
            bdlf::BindUtil::bindR<void>(InPlaceTestCallback(),
                                        &jobNo,
                                        &threadId));

        // function object executed in this thread
        BMQTST_ASSERT_EQ(jobNo, i + 1);
        BMQTST_ASSERT_EQ(threadId, bslmt::ThreadUtil::selfId());
    }
}

static void test6_executor_comparison()
// ------------------------------------------------------------------------
// EXECUTOR COMPARISON
//
// Concerns:
//   Ensure proper behavior of the comparison operators.
//
// Plan:
//   1. Check that two executors having no associated thread attributes
//      compares equal.
//
//   2. Check that two executors sharing a 'bslmt::ThreadAttribute' object
//      compares equal.
//
//   3. Check that two executors compares unequal if one of them has
//      associated thread attributes and the other doesn't.
//
//   4. Check that two executors compares equal if they have an identical
//      set of thread attributes.
//
//   5. Check that two executors compares unequal if the have a different
//      set of thread attributes.
//
// Testing:
//   bmqex::SystemExecutor's comparison operators
// ------------------------------------------------------------------------
{
    bslma::TestAllocator alloc;

    // initialize the context singleton with the test allocator
    bmqex::SystemExecutorContextGuard contextGuard(&alloc);

    // 1. both executors has no attributes
    {
        bmqex::SystemExecutor ex1, ex2;

        BMQTST_ASSERT(!ex1.threadAttributes());
        BMQTST_ASSERT(!ex2.threadAttributes());

        BMQTST_ASSERT_EQ(ex1 == ex2, true);
        BMQTST_ASSERT_EQ(ex1 != ex2, false);
    }

    // 2. both executors share same attributes
    {
        bmqex::SystemExecutor ex1(bslmt::ThreadAttributes(), &alloc);
        bmqex::SystemExecutor ex2 = ex1;

        BMQTST_ASSERT(ex1.threadAttributes() == ex2.threadAttributes());

        BMQTST_ASSERT_EQ(ex1 == ex2, true);
        BMQTST_ASSERT_EQ(ex1 != ex2, false);
    }

    // 3. one executor has attributes, the other doesn't
    {
        bmqex::SystemExecutor ex1(bslmt::ThreadAttributes(), &alloc);
        bmqex::SystemExecutor ex2;

        BMQTST_ASSERT(ex1.threadAttributes());
        BMQTST_ASSERT(!ex2.threadAttributes());

        BMQTST_ASSERT_EQ(ex1 == ex2, false);
        BMQTST_ASSERT_EQ(ex1 != ex2, true);
    }

    // 4. both executors has identical attributes
    {
        bmqex::SystemExecutor ex1(bslmt::ThreadAttributes(), &alloc);
        bmqex::SystemExecutor ex2(bslmt::ThreadAttributes(), &alloc);

        BMQTST_ASSERT(ex1.threadAttributes() && ex2.threadAttributes());
        BMQTST_ASSERT(ex1.threadAttributes() != ex2.threadAttributes());

        BMQTST_ASSERT_EQ(ex1 == ex2, true);
        BMQTST_ASSERT_EQ(ex1 != ex2, false);
    }

    // 5. executors has non-identical attributes
    {
        bslmt::ThreadAttributes attr1(&alloc);
        attr1.setThreadName("thread_1");

        bslmt::ThreadAttributes attr2(&alloc);
        attr1.setThreadName("thread_2");

        bmqex::SystemExecutor ex1(attr1, &alloc);
        bmqex::SystemExecutor ex2(attr2, &alloc);

        BMQTST_ASSERT(ex1.threadAttributes() && ex2.threadAttributes());
        BMQTST_ASSERT(ex1.threadAttributes() != ex2.threadAttributes());

        BMQTST_ASSERT_EQ(ex1 == ex2, false);
        BMQTST_ASSERT_EQ(ex1 != ex2, true);
    }
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 1: test1_context_singleton(); break;
    case 2: test2_context_creators(); break;
    case 3: test3_executor_creators(); break;
    case 4: test4_executor_post(); break;
    case 5: test5_executor_dispatch(); break;
    case 6: test6_executor_comparison(); break;

    default: {
        bsl::cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND."
                  << bsl::endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
