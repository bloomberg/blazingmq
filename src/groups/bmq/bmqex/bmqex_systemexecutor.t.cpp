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

#include <bmqex_systemexecutor.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// BDE
#include <bdlf_bind.h>
#include <bsl_functional.h>  // bsl::cref
#include <bsl_iostream.h>
#include <bsl_ostream.h>
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

static void test1_context_creators()
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
    int                    rc = 0;
    rc = bslmt::ThreadUtil::createKey(&tlsKey, onThreadExit);
    BSLS_ASSERT_OPT(rc == 0);

    bsls::AtomicInt threadsCompleted(0);
    {
        bmqex::SystemExecutor_Context context(&alloc);

        BMQTST_ASSERT_EQ(context.allocator(), &alloc);

        for (int i = 0; i < k_NUM_JOBS; ++i) {
            context.executeAsync(
                bdlf::BindUtil::bind(bslmt::ThreadUtil::setSpecific,
                                     bsl::cref(tlsKey),
                                     &threadsCompleted),
                bslmt::ThreadAttributes());
        }
    }

    // all threads owned by the execution context have completed
    BMQTST_ASSERT_EQ(threadsCompleted, k_NUM_JOBS);

    // delete the thread-local storage
    rc = bslmt::ThreadUtil::deleteKey(tlsKey);
    BSLS_ASSERT_OPT(rc == 0);
}

static void test2_executor_creators()
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

    // 1. default-construct
    {
        bmqex::SystemExecutor ex(&alloc);
        (void)ex;
    }

    // 2. ThreadAttributes-construct
    {
        bslmt::ThreadAttributes attr(&alloc);
        attr.setDetachedState(bslmt::ThreadAttributes::e_CREATE_JOINABLE);

        bmqex::SystemExecutor ex(attr, &alloc);
        (void)ex;
    }
}

static void test3_executor_post()
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

    bmqex::SystemExecutor ex(&alloc);

    bslmt::Semaphore      sem1, sem2;
    int                   jobNo    = 0;
    bslmt::ThreadUtil::Id threadId = bslmt::ThreadUtil::selfId();

    for (int i = 0; i < k_NUM_JOBS; ++i) {
        // schedule function object execution
        ex.post(bdlf::BindUtil::bindR<void>(AsyncTestCallback(),
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

static void test4_executor_dispatch()
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

    bmqex::SystemExecutor ex(&alloc);

    int                   jobNo = 0;
    bslmt::ThreadUtil::Id threadId;

    for (int i = 0; i < k_NUM_JOBS; ++i) {
        // execute function object
        ex.dispatch(bdlf::BindUtil::bindR<void>(InPlaceTestCallback(),
                                                &jobNo,
                                                &threadId));

        // function object executed in this thread
        BMQTST_ASSERT_EQ(jobNo, i + 1);
        BMQTST_ASSERT_EQ(threadId, bslmt::ThreadUtil::selfId());
    }
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 1: test1_context_creators(); break;
    case 2: test2_executor_creators(); break;
    case 3: test3_executor_post(); break;
    case 4: test4_executor_dispatch(); break;

    default: {
        bsl::cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND."
                  << bsl::endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
