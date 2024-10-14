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

// bmqex_sequentialcontext.t.cpp                                      -*-C++-*-
#include <bmqex_sequentialcontext.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// BDE
#include <bdlf_bind.h>
#include <bsl_functional.h>
#include <bsl_memory.h>  // bsl::allocator_arg
#include <bsl_vector.h>
#include <bslma_testallocator.h>
#include <bslmt_semaphore.h>
#include <bslmt_threadattributes.h>
#include <bslmt_threadutil.h>
#include <bsls_assert.h>

// CONVENIENCE
using namespace BloombergLP;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {

// ===========
// struct NoOp
// ===========

/// Provides a no-op function object.
struct NoOp {
    // TYPES

    /// Defines the result type of the call operator.
    typedef void ResultType;

    // MANIPULATORS
    void operator()() const
    {
        // NOTHING
    }
};

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

// ===========
// struct Post
// ===========

/// Provides a function object that calls `post` on a specified `executor`
/// with a specified function object argument `f`.
struct Post {
    // TYPES

    /// Defines the result type of the call operator.
    typedef void ResultType;

    template <class EXECUTOR, class FUNCTION>
    void operator()(const EXECUTOR& executor, const FUNCTION& f) const
    {
        executor.post(f);
    }
};

// ===============
// struct Dispatch
// ===============

/// Provides a function object that calls `dispatch` on a specified
/// `executor` with a specified function object argument `f`.
struct Dispatch {
    // TYPES

    /// Defines the result type of the call operator.
    typedef void ResultType;

    template <class EXECUTOR, class FUNCTION>
    void operator()(const EXECUTOR& executor, const FUNCTION& f) const
    {
        executor.dispatch(f);
    }
};

// ==================
// struct Synchronize
// ==================

/// Provides a function object to synchronize with. First calls `post` on
/// the specified `startedSignal` semaphore and then, calls `wait` on the
/// specified `continueSignal` semaphore.
struct Synchronize {
    // TYPES

    /// Defines the result type of the call operator.
    typedef void ResultType;

    void operator()(bslmt::Semaphore* startedSignal,
                    bslmt::Semaphore* continueSignal) const
    {
        startedSignal->post();
        continueSignal->wait();
    }
};

// ==================
// struct ThrowOnCopy
// ==================

/// Provides a no-op function object that throws an instance of
/// `ThrowOnCopy::ExceptionType` on copy.
struct ThrowOnCopy {
    // TYPES

    /// Defines the result type of the call operator.
    typedef void ResultType;

    /// Defines the type of the thrown exception.
    struct ExceptionType {};

    // CREATORS
    ThrowOnCopy()
    {
        // NOTHING
    }

    BSLS_ANNOTATION_NORETURN
    ThrowOnCopy(const ThrowOnCopy&) { throw ExceptionType(); }

    // MANIPULATORS
    BSLS_ANNOTATION_NORETURN
    ThrowOnCopy& operator=(const ThrowOnCopy&) { throw ExceptionType(); }

    // ACCESSORS
    void operator()() const
    {
        // NOTHING
    }
};

// UTILITY FUNCTIONS
void postOnSemaphoreOnThreadExit(void* sem)
{
    static_cast<bslmt::Semaphore*>(sem)->post();
}

}  // close anonymous namespace

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
//   Default-construct an instance of 'bmqex::SequentialContext'. Check
//   postconditions. Destroy the object and check that no memory is leaked.
//
// Testing:
//   bmqex::SequentialContext's constructor
//   bmqex::SequentialContext's destructor
// ------------------------------------------------------------------------
{
    bslma::TestAllocator alloc;

    // create context
    bmqex::SequentialContext ctx(&alloc);

    // check postconditions
    ASSERT_EQ(ctx.outstandingJobs(), static_cast<size_t>(0));
    ASSERT_EQ(ctx.allocator(), &alloc);
}

static void test2_context_start()
// ------------------------------------------------------------------------
// CONTEXT START
//
// Concerns:
//   Ensure proper behavior of the 'start' method.
//
// Plan:
//   1. Create an execution context. Submit several function objects. Call
//      'start'. Check that:
//      - 'start()' returns 0;
//      - The execution context starts executing submitted function
//        objects.
//
// Testing:
//   bmqex::SequentialContext::start
// ------------------------------------------------------------------------
{
    bslma::TestAllocator alloc;

    // 1. regular start
    {
        static const int NUM_JOBS = 10;

        bslmt::Semaphore startedSignal,  // signaled when job starts executing
            continueSignal;              // signaled to allow job to complete

        // create context
        bmqex::SequentialContext ctx(&alloc);

        // submit jobs
        for (int i = 0; i < NUM_JOBS; ++i) {
            ctx.executor().post(bdlf::BindUtil::bind(Synchronize(),
                                                     &startedSignal,
                                                     &continueSignal));
        }

        // do start
        ASSERT_EQ(ctx.start(), 0);

        // synchronize with the completion of each submitted function object
        for (int i = 0; i < NUM_JOBS; ++i) {
            startedSignal.wait();
            continueSignal.post();
        }
    }
}

static void test3_context_stop()
// ------------------------------------------------------------------------
// CONTEXT STOP
//
// Concerns:
//   Ensure proper behavior of the 'stop' method.
//
// Plan:
//   Create an execution context. Submit function objects #1 and #2. Start
//   the context and wait till function object #1 starts executing. Call
//   'stop' while the function object #1 is still executing. Check that:
//   - 'stop' had not blocked the calling thread pending completion of
//     function object #1;
//   - after function object #1 completes, the context working thread stops
//     without executing function object #2.
//
// Testing:
//   bmqex::SequentialContext::stop
// ------------------------------------------------------------------------
{
    bslma::TestAllocator alloc;

    bslmt::Semaphore startedSignal,  // signaled when job #1 starts executing
        continueSignal,              // signaled to allow job #1 to complete
        threadExitSignal;            // signaled when working thread exits

    bsl::vector<int> job2Storage;  // modified by job #2

    // create a thread-local storage associated with a thread-exit handler
    bslmt::ThreadUtil::Key tlsKey;
    int                    rc = bslmt::ThreadUtil::createKey(&tlsKey,
                                          postOnSemaphoreOnThreadExit);
    BSLS_ASSERT_OPT(rc == 0);

    // create context
    bmqex::SequentialContext ctx(&alloc);

    // submit a function object that attaches the thread-exit handler to the
    // current thread
    ctx.executor().post(bdlf::BindUtil::bind(bslmt::ThreadUtil::setSpecific,
                                             bsl::cref(tlsKey),
                                             &threadExitSignal));

    // submit function object #1
    ctx.executor().post(
        bdlf::BindUtil::bind(Synchronize(), &startedSignal, &continueSignal));

    // submit function object #2
    ctx.executor().post(bdlf::BindUtil::bind(PushBack(), &job2Storage, 42));

    // start executing function object #1
    ctx.start();
    startedSignal.wait();
    // function object #1 is currently executing

    // stop the context
    ctx.stop();
    // 'stop()' has returned, but the working thread is still running
    // and function object #1 still executing

    // complete function object #1 execution
    continueSignal.post();
    threadExitSignal.wait();
    // the working thread completed, function object #1 completed execution

    // function object #2 had not been executed
    ASSERT(job2Storage.empty());

    // delete the thread-local storage
    rc = bslmt::ThreadUtil::deleteKey(tlsKey);
    BSLS_ASSERT_OPT(rc == 0);
}

static void test4_context_join()
// ------------------------------------------------------------------------
// CONTEXT JOIN
//
// Concerns:
//   Ensure proper behavior of the 'join' method.
//
// Plan:
//   Create an execution context without starting it. Submit several
//   function objects. Start the context and immediately call 'join'.
//   Check that:
//   - join had  blocked the calling thread pending completion of all
//     submitted function objects;
//   - after 'join' completes the context working thread stops.
//
// Testing:
//   bmqex::SequentialContext::join
// ------------------------------------------------------------------------
{
    static const int NUM_JOBS = 100;

    bslma::TestAllocator alloc;

    bsl::vector<int> out(&alloc);       // job result storage
    bslmt::Semaphore threadExitSignal;  // signaled when working thread exits

    // create a thread-local storage associated with a thread-exit handler
    bslmt::ThreadUtil::Key tlsKey;
    int                    rc = bslmt::ThreadUtil::createKey(&tlsKey,
                                          postOnSemaphoreOnThreadExit);
    BSLS_ASSERT_OPT(rc == 0);

    // create context
    bmqex::SequentialContext ctx(&alloc);

    // submit a function object that attaches the thread-exit handler to the
    // current thread
    ctx.executor().post(bdlf::BindUtil::bind(bslmt::ThreadUtil::setSpecific,
                                             bsl::cref(tlsKey),
                                             &threadExitSignal));

    // submit jobs
    for (int i = 0; i < NUM_JOBS; ++i) {
        ctx.executor().post(bdlf::BindUtil::bind(PushBack(), &out, i));
    }

    // start context and join it
    ctx.start();
    ctx.join();

    // all jobs executed
    ASSERT_EQ(out.size(), static_cast<size_t>(NUM_JOBS));

    // wait till working thread exits
    threadExitSignal.wait();

    // delete the thread-local storage
    rc = bslmt::ThreadUtil::deleteKey(tlsKey);
    BSLS_ASSERT_OPT(rc == 0);
}

static void test5_context_dropPendingJobs()
// ------------------------------------------------------------------------
// CONTEXT DROP PENDING JOBS
//
// Concerns:
//   Ensure proper behavior of the 'dropPendingJobs' method.
//
// Plan:
//   1. Drop pending jobs while no job is executing on the context. Check
//      that all jobs are removed.
//
//   2. Drop pending jobs while one job is executing on the context. Check
//      that all jobs but the executing one are removed.
//
// Testing:
//   bmqex::SequentialContext::dropPendingJobs
// ------------------------------------------------------------------------
{
    bslma::TestAllocator alloc;

    // create context
    bmqex::SequentialContext ctx(&alloc);

    // 1. drop jobs while no job is executing
    {
        // push 3 jobs to the context
        ctx.executor().post(NoOp());
        ctx.executor().post(NoOp());
        ctx.executor().post(NoOp());
        ASSERT_EQ(ctx.outstandingJobs(), 3u);

        // drop all of them
        size_t jobsRemoved = ctx.dropPendingJobs();
        ASSERT_EQ(jobsRemoved, 3u);
        ASSERT_EQ(ctx.outstandingJobs(), 0u);
    }

    // 2. drop jobs while a job is executing
    {
        bslmt::Semaphore startedSignal,  // signaled when job starts executing
            continueSignal;              // signaled to allow job to complete

        // push 3 jobs to the context
        ctx.executor().post(bdlf::BindUtil::bind(Synchronize(),
                                                 &startedSignal,
                                                 &continueSignal));
        ctx.executor().post(NoOp());
        ctx.executor().post(NoOp());
        ASSERT_EQ(ctx.outstandingJobs(), 3u);

        // start executing the first job
        ctx.start();
        startedSignal.wait();

        // drop the other two jobs
        size_t jobsRemoved = ctx.dropPendingJobs();
        ASSERT_EQ(jobsRemoved, 2u);
        ASSERT_EQ(ctx.outstandingJobs(), 1u);

        // synchronize
        continueSignal.post();
        ctx.join();
        ctx.stop();
    }
}

static void test6_context_outstandingJobs()
// ------------------------------------------------------------------------
// CONTEXT OUTSTANDING JOBS
//
// Concerns:
//   Ensure proper behavior of the 'outstandingJobs' method.
//
// Plan:
//   Check that 'outstandingJobs' returns a value that is the sum of:
//   - The number of function objects that have been added to the
//     'bmqex::SequentialContext' via the 'bmqex::SequentialContext's
//     executor, but not yet invoked;
//   - The number of function objects that are currently being invoked
//     within the 'bmqex::SequentialContext'.
//
// Testing:
//   bmqex::SequentialContext::outstandingJobs
// ------------------------------------------------------------------------
{
    static const int NUM_JOBS = 10;

    bslma::TestAllocator alloc;

    bslmt::Semaphore startedSignal,  // signaled when job starts executing
        continueSignal;              // signaled to allow the job to finish

    // create context
    bmqex::SequentialContext ctx(&alloc);

    // outstanding jobs is 0
    ASSERT_EQ(ctx.outstandingJobs(), static_cast<size_t>(0));

    // submit 'NUM_JOBS' function objects
    for (int i = 0; i < NUM_JOBS; ++i) {
        // submit a function object
        ctx.executor().post(bdlf::BindUtil::bind(Synchronize(),
                                                 &startedSignal,
                                                 &continueSignal));

        // outstanding jobs increased by 1
        ASSERT_EQ(ctx.outstandingJobs(), static_cast<size_t>(i + 1));
    }

    // make the context execute submitted function objects one by one
    for (int i = NUM_JOBS; i > 0; --i) {
        // start executing the next function object
        ctx.start();
        startedSignal.wait();
        // the function object is currently executing

        // outstanding jobs not decreased yet
        ASSERT_EQ(ctx.outstandingJobs(), static_cast<size_t>(i));

        // complete the function object execution
        ctx.stop();
        continueSignal.post();
        ctx.join();
        // function object finished execution

        // outstanding jobs has decrease
        ASSERT_EQ(ctx.outstandingJobs(), static_cast<size_t>(i - 1));
    }
}

static void test7_executor_post()
// ------------------------------------------------------------------------
// EXECUTOR POST
//
// Concerns:
//   Ensure proper behavior of the 'post' method.
//
// Plan:
//   1. Create an execution context without starting it. Submit several
//      function objects by calling 'post' on the context's executor.
//      Check that no function object has been executed so far. Then, call
//      'start' followed by 'join'. Check that all submitted function
//      objects have been executed in the submission order.
//
//   2. Create an execution context without starting it. Submit function
//      objects #1, #2, #3 ... #N by calling 'post' on the context's
//      executor, given that the submitted function object #i itself
//      submits a function object #(N + i) via a call to 'post' on the
//      same executor. Then, call 'start' followed by 'join'. Check
//      that all submitted function objects have been executed in the
//      submission order.
//
//   3. Create an execution context without starting it. Submit function
//      objects #1, #2 and #3 by calling 'post' for each object, given
//      that object #3 throws on copy. Check that:
//      - No memory is leaked;
//      - The context state is the same as before the exception;
//      - The context stays usable, i.e. new function objects can be
//        submitted.
//
// Testing:
//   bmqex::SequentialContextExecutor::post
// ------------------------------------------------------------------------
{
    bslma::TestAllocator alloc;

    // 1. general use-case
    {
        static const int NUM_JOBS = 10;

        bsl::vector<int> out(&alloc);

        // create context
        bmqex::SequentialContext ctx(&alloc);

        // submit function objects
        for (int i = 0; i < NUM_JOBS; ++i) {
            ctx.executor().post(bdlf::BindUtil::bind(PushBack(), &out, i));
            ASSERT_EQ(ctx.outstandingJobs(), static_cast<size_t>(i + 1));
        }

        // no function object executed so far
        ASSERT(out.empty());

        // start the context and join it
        ctx.start();
        ctx.join();

        // all function objects executed in submission order
        ASSERT_EQ(out.size(), static_cast<size_t>(NUM_JOBS));
        for (int i = 0; i < NUM_JOBS; ++i) {
            ASSERT_EQ(out[i], i);
        }
    }

    // 2. call 'post' from the working thread
    {
        static const int NUM_JOBS = 10;

        bsl::vector<int> out(&alloc);

        // create context
        bmqex::SequentialContext ctx(&alloc);

        // submit function objects
        for (int i = 0; i < NUM_JOBS; ++i) {
            bsl::function<void()> pushBack(
                bsl::allocator_arg,
                &alloc,
                bdlf::BindUtil::bind(PushBack(), &out, i));

            // submit a function object that submits another function object
            // via a call to 'post'
            ctx.executor().post(bdlf::BindUtil::bindS(&alloc,
                                                      Post(),
                                                      ctx.executor(),
                                                      pushBack));
        }

        // start the context and join it
        ctx.start();
        ctx.join();

        // all function objects executed in submission order
        ASSERT_EQ(out.size(), static_cast<size_t>(NUM_JOBS));
        for (int i = 0; i < NUM_JOBS; ++i) {
            ASSERT_EQ(out[i], i);
        }
    }

    // 3. exception safety
    {
        bmqex::SequentialContext ctx(&alloc);

        // sumbit function objects #1 and #2
        ctx.executor().post(NoOp());
        ctx.executor().post(NoOp());
        ASSERT_EQ(ctx.outstandingJobs(), static_cast<size_t>(2));

        ThrowOnCopy throwOnCopy;
        bool        exceptionThrown = false;

        // submit function object #3 that throws on copy
        try {
            ctx.executor().post(throwOnCopy);
        }
        catch (const ThrowOnCopy::ExceptionType&) {
            exceptionThrown = true;
        }

        // exception thrown
        ASSERT_EQ(exceptionThrown, true);

        // context not affected
        ASSERT_EQ(ctx.outstandingJobs(), static_cast<size_t>(2));

        // context stays usable
        ctx.executor().post(NoOp());
        ctx.executor().post(NoOp());
        ASSERT_EQ(ctx.outstandingJobs(), static_cast<size_t>(4));

        // NOTE: The test allocator will check that no memory is leaked on
        //       destruction.
    }
}

static void test8_executor_dispatch()
// ------------------------------------------------------------------------
// EXECUTOR DISPATCH
//
// Concerns:
//   Ensure proper behavior of the 'dispatch' method.
//
// Plan:
//   1. Create an execution context without starting it. Submit several
//      function objects by calling 'dispatch' on the context's executor.
//      Check that no function object has been executed so far. Then, call
//      'start' followed by 'join'. Check that all submitted function
//      objects have been executed in the submission order (as if submitted
//      via 'post').
//
//   2. Create an execution context without starting it. Submit function
//      objects #1, #2, #3 ... #N by calling 'post' on the context's
//      executor, given that each submitted function object #i itself
//      submits a function object #(N + i) via a call to 'dispatch' on
//      the same executor. Start the context. Check that calls to
//      'dispatch' results in submitted function objects being executed
//      in-place.
//
// Testing:
//   bmqex::SequentialContextExecutor::dispatch
// ------------------------------------------------------------------------
{
    bslma::TestAllocator alloc;

    // 1. call 'dispatch' outside the working thread
    {
        static const int NUM_JOBS = 10;

        bsl::vector<int> out(&alloc);

        // create context
        bmqex::SequentialContext ctx(&alloc);

        // submit function objects
        for (int i = 0; i < NUM_JOBS; ++i) {
            ctx.executor().dispatch(bdlf::BindUtil::bind(PushBack(), &out, i));
            ASSERT_EQ(ctx.outstandingJobs(), static_cast<size_t>(i + 1));
        }

        // no function object executed so far
        ASSERT(out.empty());

        // start the context and join it
        ctx.start();
        ctx.join();

        // all function objects executed in submission order
        ASSERT_EQ(out.size(), static_cast<size_t>(NUM_JOBS));
        for (int i = 0; i < NUM_JOBS; ++i) {
            ASSERT_EQ(out[i], i);
        }
    }

    // 2. call 'dispatch' from the working thread
    {
        static const int NUM_JOBS = 10;

        bsl::vector<int> out(&alloc);
        bslmt::Semaphore startedSignal,  // signaled when job starts executing
            continueSignal;              // signaled to allow job to finish

        // create context
        bmqex::SequentialContext ctx(&alloc);

        // submit function objects
        for (int i = 0; i < NUM_JOBS; ++i) {
            bsl::function<void()> pushBack(
                bsl::allocator_arg,
                &alloc,
                bdlf::BindUtil::bind(PushBack(), &out, i));

            // submit a function object that submits another function object
            // via a call to 'dispatch'
            ctx.executor().post(bdlf::BindUtil::bindS(&alloc,
                                                      Dispatch(),
                                                      ctx.executor(),
                                                      pushBack));

            // submit a function object we will synchronize with
            ctx.executor().post(bdlf::BindUtil::bind(Synchronize(),
                                                     &startedSignal,
                                                     &continueSignal));
        }

        // start the context
        ctx.start();

        for (int i = 0; i < NUM_JOBS; ++i) {
            // wait till the next function object is executed
            startedSignal.wait();

            // function object executed in-place
            ASSERT_EQ(out.size(), static_cast<size_t>(i + 1));
            ASSERT_EQ(out[i], i);

            // continue
            continueSignal.post();
        }
    }
}

static void test9_executor_swap()
// ------------------------------------------------------------------------
// EXECUTOR SWAP
//
// Concerns:
//   Ensure proper behavior of the 'swap' method.
//
// Plan:
//   Given two executor objects 'ex1' and 'ex2' which associated execution
//   contexts are 'ctx1' and 'ctx2' respectively, call 'ex1.swap(ex2)' and
//   check that 'ex1' now refers to 'ctx2' and 'ex2' now refers to 'ctx1'.
//
// Testing:
//   bmqex::SequentialContextExecutor::swap
// ------------------------------------------------------------------------
{
    bslma::TestAllocator     alloc;
    bmqex::SequentialContext ctx1(&alloc), ctx2(&alloc);

    bmqex::SequentialContextExecutor ex1 = ctx1.executor();
    bmqex::SequentialContextExecutor ex2 = ctx2.executor();

    // swap
    ex1.swap(ex2);

    // check
    ASSERT_EQ(&ex1.context(), &ctx2);
    ASSERT_EQ(&ex2.context(), &ctx1);
}

static void test10_executor_runningInThisThread()
// ------------------------------------------------------------------------
// EXECUTOR RUNNING IN THIS THREAD
//
// Concerns:
//   Ensure proper behavior of the 'runningInThisThread' method.
//
// Plan:
//   Check that 'runningInThisThread' returns 'true' if the calling
//   thread is the thread owned by the executor's associated execution
//   context, and 'false otherwise.
//
// Testing:
//   bmqex::SequentialContextExecutor::runningInThisThread
// ------------------------------------------------------------------------
{
    typedef bmqex::SequentialContextExecutor ExecutorType;

    bslma::TestAllocator alloc;

    // storage for the result of a 'runningInThisThread' invocation
    bool result = false;

    // create context
    bmqex::SequentialContext ctx(&alloc);

    // obtain executor
    ExecutorType ex = ctx.executor();

    // submit a job that saves the result of a 'runningInThisThread'
    // invocation
    ex.post(bdlf::BindUtil::bind(
        Assign(),
        &result,
        bdlf::BindUtil::bind(&ExecutorType::runningInThisThread, &ex)));

    ctx.start();
    ctx.join();

    // the saved result is 'true'
    ASSERT_EQ(result, true);

    // when calling 'runningInThisThread' outside the context working thread,
    // the result is 'false'
    ASSERT_EQ(ex.runningInThisThread(), false);
}

static void test11_executor_context()
// ------------------------------------------------------------------------
// EXECUTOR CONTEXT
//
// Concerns:
//   Ensure proper behavior of the 'context' method.
//
// Plan:
//   Check that 'context()' returns a reference to the executor's
//   associated 'bmqex::SequentialContext' object.
//
// Testing:
//   bmqex::SequentialContextExecutor::context
// ------------------------------------------------------------------------
{
    bslma::TestAllocator     alloc;
    bmqex::SequentialContext ctx1(&alloc), ctx2(&alloc);

    bmqex::SequentialContextExecutor ex1 = ctx1.executor();
    ASSERT_EQ(&ex1.context(), &ctx1);

    bmqex::SequentialContextExecutor ex2 = ctx2.executor();
    ASSERT_EQ(&ex2.context(), &ctx2);
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    // bmqex::SequentialContext
    case 1: test1_context_creators(); break;
    case 2: test2_context_start(); break;
    case 3: test3_context_stop(); break;
    case 4: test4_context_join(); break;
    case 5: test5_context_dropPendingJobs(); break;
    case 6: test6_context_outstandingJobs(); break;

    // bmqex::SequentialContextExecutor
    case 7: test7_executor_post(); break;
    case 8: test8_executor_dispatch(); break;
    case 9: test9_executor_swap(); break;
    case 10: test10_executor_runningInThisThread(); break;
    case 11: test11_executor_context(); break;

    default: {
        bsl::cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND."
                  << bsl::endl;
        s_testStatus = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
