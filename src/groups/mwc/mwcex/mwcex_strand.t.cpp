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

// mwcex_strand.t.cpp                                                 -*-C++-*-
#include <mwcex_strand.h>

// TEST DRIVER
#include <mwctst_testhelper.h>

// BDE
#include <bdlf_bind.h>
#include <bdlmt_threadpool.h>
#include <bsl_functional.h>
#include <bsl_limits.h>
#include <bsl_memory.h>  // bsl::allocator_arg
#include <bsl_numeric.h>
#include <bsl_vector.h>
#include <bslma_testallocator.h>
#include <bslmt_semaphore.h>
#include <bslmt_threadattributes.h>
#include <bsls_annotation.h>
#include <bsls_assert.h>
#include <bsls_keyword.h>

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

    // ACCESSORS
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

    // ACCESSORS
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

    // ACCESSORS
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
/// with a specified function object `f`.
struct Post {
    // TYPES

    /// Defines the result type of the call operator.
    typedef void ResultType;

    // ACCESSORS
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
/// `executor` with a specified function object `f`.
struct Dispatch {
    // TYPES

    /// Defines the result type of the call operator.
    typedef void ResultType;

    // ACCESSORS
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

    // ACCESSORS
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

// ==========================
// class IdentifiableExecutor
// ==========================

/// Provides a dummy executor that can be identified by its Id.
class IdentifiableExecutor {
  private:
    // PRIVATE DATA
    int d_id;

  public:
    // CREATORS
    explicit IdentifiableExecutor(int id = 0)
    : d_id(id)
    {
        // NOTHING
    }

  public:
    // MANIPULATORS
    template <class FUNCTION>
    void post(FUNCTION) const
    {
        // not a valid operation
        BSLS_ASSERT_OPT(false);
    }

    template <class FUNCTION>
    void dispatch(FUNCTION) const
    {
        // not a valid operation
        BSLS_ASSERT_OPT(false);
    }

  public:
    // ACCESSORS
    int id() const { return d_id; }

    bool operator==(const IdentifiableExecutor& rhs) const
    {
        return d_id == rhs.d_id;
    }
};

// ======================
// class ThrowingExecutor
// ======================

/// Provides an executor that throws an exception on functor submission.
class ThrowingExecutor {
  public:
    // TYPES

    /// Defines the type of the thrown exception.
    struct ExceptionType {};

  public:
    // MANIPULATORS
    template <class FUNCTION>
    BSLS_ANNOTATION_NORETURN void post(FUNCTION) const
    {
        throw ExceptionType();
    }

    template <class FUNCTION>
    BSLS_ANNOTATION_NORETURN void dispatch(FUNCTION) const
    {
        throw ExceptionType();
    }

  public:
    // ACCESSORS
    bool operator==(const ThrowingExecutor&) const { return true; }
};

// ==========================
// class TestExecutionContext
// ==========================

/// Provides an multi-threaded execution context for test purposes that
/// executes submitted function objects in the context of an owned thread.
///
/// Note that, unless initialized with `numThreads` equal to 1, this
/// execution context does not provide guarantees of ordering and non-
/// concurrency.
class TestExecutionContext {
  public:
    // TYPES

    /// Provides an executor to submit function objects on the test
    /// execution context.
    class ExecutorType {
      private:
        // PRIVATE DATA
        TestExecutionContext* d_context_p;

        // FRIENDS
        friend class TestExecutionContext;

      private:
        // PRIVATE CREATORS
        ExecutorType(TestExecutionContext* context) BSLS_KEYWORD_NOEXCEPT
        // IMPLICIT
        // Create a 'ExecutorType' object having the specified 'context' as
        // its associated execution context.
        : d_context_p(context)
        {
            // PRECONDITIONS
            BSLS_ASSERT(context);
        }

      public:
        // MANIPULATORS

        /// Submit the specified function object `f` for execution.
        template <class FUNCTION>
        void post(BSLS_COMPILERFEATURES_FORWARD_REF(FUNCTION) f) const
        {
            int rc = d_context_p->d_threadPool.enqueueJob(
                BSLS_COMPILERFEATURES_FORWARD(FUNCTION, f));
            BSLS_ASSERT_OPT(rc == 0);
        }

        /// Submit the specified function object `f` for execution.
        template <class FUNCTION>
        void dispatch(BSLS_COMPILERFEATURES_FORWARD_REF(FUNCTION) f) const
        {
            int rc = d_context_p->d_threadPool.enqueueJob(
                BSLS_COMPILERFEATURES_FORWARD(FUNCTION, f));
            BSLS_ASSERT_OPT(rc == 0);
        }
    };

  private:
    // PRIVATE DATA

    // Underlying mechanism used to submit function objects.
    bdlmt::ThreadPool d_threadPool;

  private:
    // NOT IMPLEMENTED
    TestExecutionContext(const TestExecutionContext&) BSLS_KEYWORD_DELETED;
    TestExecutionContext&
    operator=(const TestExecutionContext&) BSLS_KEYWORD_DELETED;

  public:
    // CREATORS

    /// Create a `TestExecutionContext` object. Swawn `numThreads` threads
    /// of execution. Specify an `allocator` used to supply memory.
    TestExecutionContext(unsigned          numThreads,
                         bslma::Allocator* allocator)
    : d_threadPool(bslmt::ThreadAttributes(),        // use default attributes
                   numThreads,                       // minThreads
                   numThreads,                       // maxThreads
                   bsl::numeric_limits<int>::max(),  // maxIdleTime
                   allocator)
    {
        // PRECONDITIONS
        BSLS_ASSERT(allocator);

        int rc = d_threadPool.start();
        BSLS_ASSERT_OPT(rc == 0);
    }

    /// Destroy this object. Wait till all pending function objects
    /// complete. Then, join all owned threads.
    ~TestExecutionContext() { d_threadPool.stop(); }

  public:
    // ACCESSORS

    /// Return an executor that may be used for submitting function objects
    /// to this execution context.
    ExecutorType executor() const BSLS_KEYWORD_NOEXCEPT
    {
        return const_cast<TestExecutionContext*>(this);
    }
};

}  // close unnamed namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_strand_creators()
// ------------------------------------------------------------------------
// STRAND CREATORS
//
// Concerns:
//   Ensure proper behavior of creator methods.
//
// Plan:
//   1. Default-construct an instance of 'mwcex::Strand'. Check
//      postconditions. Destroy the object and check that no memory
//      is leaked.
//
//   2. Executor-construct an instance of 'mwcex::Strand'. Check
//      postconditions. Destroy the object and check that no memory
//      is leaked.
//
//   3. Create a strand without starting it. Submit several function
//      objects. Destroy the strand. Check that no memory is leaked.
//
// Testing:
//   mwcex::Strand's constructor
//   mwcex::Strand's destructor
// ------------------------------------------------------------------------
{
    typedef mwcex::Strand<IdentifiableExecutor> Strand;

    // compiler hapiness
    {
        (void)&ThrowingExecutor::dispatch<int>;
        (void)&ThrowingExecutor::operator==;
    }

    // 1. default constructor
    {
        bslma::TestAllocator alloc;

        // create context
        Strand strand(&alloc);

        // check postconditions
        ASSERT_EQ(strand.outstandingJobs(), 0u);
        ASSERT_EQ(strand.innerExecutor().id(), 0);
        ASSERT_EQ(strand.allocator(), &alloc);
    }

    // 2. executor constructor
    {
        bslma::TestAllocator alloc;

        // create context
        Strand strand(IdentifiableExecutor(42), &alloc);

        // check postconditions
        ASSERT_EQ(strand.outstandingJobs(), 0u);
        ASSERT_EQ(strand.innerExecutor().id(), 42);
        ASSERT_EQ(strand.allocator(), &alloc);
    }

    // 3. create, sumbit, destroy
    {
        bslma::TestAllocator alloc;

        // create context
        Strand strand(&alloc);

        // submit function objects
        strand.executor().post(NoOp());
        strand.executor().post(NoOp());
        strand.executor().post(NoOp());

        // destroy the context ...
    }

    // compiler happiness (unused function template)
    {
        bslma::TestAllocator alloc;

        // IdentifiableExecutor
        {
            mwcex::Strand<IdentifiableExecutor> strand(&alloc);
            ASSERT_OPT_FAIL(strand.innerExecutor().post(NoOp()));
            ASSERT_OPT_FAIL(strand.innerExecutor().dispatch(NoOp()));
            ASSERT(strand.innerExecutor() == strand.innerExecutor());
        }

        {
            // ExecutorType
            TestExecutionContext                              ctx(1, &alloc);
            mwcex::Strand<TestExecutionContext::ExecutorType> strand(
                ctx.executor(),
                &alloc);
            strand.innerExecutor().dispatch(NoOp());
        }
    }
}

static void test2_strand_start()
// ------------------------------------------------------------------------
// STRAND START
//
// Concerns:
//   Ensure proper behavior of the 'start' method.
//
// Plan:
//   Create a strand. Submit several function objects. Call 'start' and
//   check that the strand has started executing submitted function
//   objects.
//
// Testing:
//   mwcex::Strand::start
// ------------------------------------------------------------------------
{
    typedef mwcex::Strand<TestExecutionContext::ExecutorType> Strand;

    static const int k_NUM_THREADS = 1;
    static const int k_NUM_JOBS    = 10;

    bslma::TestAllocator alloc;
    bslmt::Semaphore     startedSignal,  // signaled when job starts executing
        continueSignal;                  // signaled to allow job to complete

    // create a strand on top a single-threaded context
    TestExecutionContext ctx(k_NUM_THREADS, &alloc);
    Strand               strand(ctx.executor(), &alloc);

    // submit jobs
    for (int i = 0; i < k_NUM_JOBS; ++i) {
        strand.executor().post(bdlf::BindUtil::bind(Synchronize(),
                                                    &startedSignal,
                                                    &continueSignal));
    }

    // do start
    strand.start();
    strand.start();  // call 'start' twice just to check it's okay

    // synchronize with the completion of each submitted function object
    for (int i = 0; i < k_NUM_JOBS; ++i) {
        startedSignal.wait();
        continueSignal.post();
    }
}

static void test3_strand_stop()
// ------------------------------------------------------------------------
// STRAND STOP
//
// Concerns:
//   Ensure proper behavior of the 'stop' method.
//
// Plan:
//   Create a strand. Submit function objects #1 and #2. Start the strand
//   and wait till function object #1 starts executing. Call 'stop' while
//   the function object #1 is still executing. Check that:
//   - 'stop' had not blocked the calling thread pending completion of
//     function object #1;
//   - function object #2 is not invoked.
//
// Testing:
//   mwcex::Strand::stop
// ------------------------------------------------------------------------
{
    typedef mwcex::Strand<TestExecutionContext::ExecutorType> Strand;

    static const int k_NUM_THREADS = 4;

    bslma::TestAllocator alloc;

    bslmt::Semaphore startedSignal,  // signaled when job #1 starts executing
        continueSignal;              // signaled to allow job #1 to complete

    // create a strand on top a multi-threaded context
    TestExecutionContext ctx(k_NUM_THREADS, &alloc);
    Strand               strand(ctx.executor(), &alloc);

    // submit function object #1
    strand.executor().post(
        bdlf::BindUtil::bind(Synchronize(), &startedSignal, &continueSignal));

    // submit function object #2
    bool job2Executed = false;
    strand.executor().post(
        bdlf::BindUtil::bind(Assign(), &job2Executed, true));

    // start executing function object #1
    strand.start();
    startedSignal.wait();
    // function object #1 is currently executing

    // stop the strand
    strand.stop();
    // 'stop()' has returned, but function object #1 still executing

    // complete function object #1 execution
    continueSignal.post();
    // function object #1 completed execution

    // wait till all function objects are executed
    strand.join();

    // function object #2 had not been executed
    ASSERT(!job2Executed);
}

static void test4_strand_join()
// ------------------------------------------------------------------------
// STRAND JOIN
//
// Concerns:
//   Ensure proper behavior of the 'join' method.
//
// Plan:
//   Create an execution context without starting it. Submit several
//   function objects. Start the context and immediately call 'join'.
//   Check that 'join' had  blocked the calling thread pending completion
//   of all submitted function objects.
//
// Testing:
//   mwcex::Strand::join
// ------------------------------------------------------------------------
{
    typedef mwcex::Strand<TestExecutionContext::ExecutorType> Strand;

    static const int k_NUM_THREADS = 4;
    static const int k_NUM_JOBS    = 100;

    bslma::TestAllocator alloc;
    bsl::vector<int>     out(&alloc);  // job result storage

    // create a strand on top a multi-threaded context
    TestExecutionContext ctx(k_NUM_THREADS, &alloc);
    Strand               strand(ctx.executor(), &alloc);

    // submit jobs
    for (int i = 0; i < k_NUM_JOBS; ++i) {
        strand.executor().post(bdlf::BindUtil::bind(PushBack(), &out, i));
    }

    // start context and join it
    strand.start();
    strand.join();

    // all jobs executed
    ASSERT_EQ(out.size(), static_cast<size_t>(k_NUM_JOBS));
}

static void test5_strand_dropPendingJobs()
// ------------------------------------------------------------------------
// STRAND DROP PENDING JOBS
//
// Concerns:
//   Ensure proper behavior of the 'dropPendingJobs' method.
//
// Plan:
//   1. Drop pending jobs while no job is executing on the strand. Check
//      that all jobs are removed from the strand.
//
//   2. Drop pending jobs while one job is executing on the strand. Check
//      that all jobs but the executing one are removed from the strand.
//
// Testing:
//   mwcex::Strand::dropPendingJobs
// ------------------------------------------------------------------------
{
    typedef mwcex::Strand<TestExecutionContext::ExecutorType> Strand;

    static const int k_NUM_THREADS = 1;

    bslma::TestAllocator alloc;

    // create a strand on top a single-threaded context
    TestExecutionContext ctx(k_NUM_THREADS, &alloc);
    Strand               strand(ctx.executor(), &alloc);

    // 1. drop jobs while no job is executing
    {
        // push 3 jobs to the strand
        strand.executor().post(NoOp());
        strand.executor().post(NoOp());
        strand.executor().post(NoOp());
        ASSERT_EQ(strand.outstandingJobs(), 3u);

        // drop all of them
        size_t jobsRemoved = strand.dropPendingJobs();
        ASSERT_EQ(jobsRemoved, 3u);
        ASSERT_EQ(strand.outstandingJobs(), 0u);
    }

    // 2. drop jobs while a job is executing
    {
        bslmt::Semaphore startedSignal,  // signaled when job starts executing
            continueSignal;              // signaled to allow job to complete

        // push 3 jobs to the strand
        strand.executor().post(bdlf::BindUtil::bind(Synchronize(),
                                                    &startedSignal,
                                                    &continueSignal));
        strand.executor().post(NoOp());
        strand.executor().post(NoOp());
        ASSERT_EQ(strand.outstandingJobs(), 3u);

        // start executing the first job
        strand.start();
        startedSignal.wait();

        // drop the other two jobs
        size_t jobsRemoved = strand.dropPendingJobs();
        ASSERT_EQ(jobsRemoved, 2u);
        ASSERT_EQ(strand.outstandingJobs(), 1u);

        // synchronize
        continueSignal.post();
        strand.join();
        strand.stop();
    }
}

static void test6_strand_outstandingJobs()
// ------------------------------------------------------------------------
// STRAND OUTSTANDING JOBS
//
// Concerns:
//   Ensure proper behavior of the 'outstandingJobs' method.
//
// Plan:
//   Check that 'outstandingJobs' returns a value that is the sum of:
//   - The number of function objects that have been added to the
//     'mwcex::Strand' via the 'mwcex::Strand's executor, but not yet
//     invoked;
//   - The number of function objects that are currently being invoked
//     within the 'mwcex::Strand'.
//
// Testing:
//   mwcex::Strand::outstandingJobs
// ------------------------------------------------------------------------
{
    typedef mwcex::Strand<TestExecutionContext::ExecutorType> Strand;

    static const int k_NUM_THREADS = 4;
    static const int k_NUM_JOBS    = 10;

    bslma::TestAllocator alloc;
    bslmt::Semaphore     startedSignal,  // signaled when job starts executing
        continueSignal;                  // signaled to allow job to complete

    // create a strand on top a multi-threaded context
    TestExecutionContext ctx(k_NUM_THREADS, &alloc);
    Strand               strand(ctx.executor(), &alloc);

    // outstanding jobs is 0
    ASSERT_EQ(strand.outstandingJobs(), 0u);

    // submit 'k_NUM_JOBS' function objects
    for (int i = 0; i < k_NUM_JOBS; ++i) {
        // submit a function object
        strand.executor().post(bdlf::BindUtil::bind(Synchronize(),
                                                    &startedSignal,
                                                    &continueSignal));

        // outstanding jobs increased by 1
        ASSERT_EQ(strand.outstandingJobs(), static_cast<size_t>(i + 1));
    }

    // make the context execute submitted function objects one by one
    for (int i = k_NUM_JOBS; i > 0; --i) {
        // start executing the next function object
        strand.start();
        startedSignal.wait();
        // the function object is currently executing

        // outstanding jobs not decreased yet
        ASSERT_EQ(strand.outstandingJobs(), static_cast<size_t>(i));

        // complete the function object execution
        strand.stop();
        continueSignal.post();
        strand.join();
        // function object finished execution

        // outstanding jobs has decrease
        ASSERT_EQ(strand.outstandingJobs(), static_cast<size_t>(i - 1));
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
//   1. Create a strand without starting it. Submit several function
//      objects by calling 'post' on the strand's executor. Check that
//      no function object has been executed so far. Then, call 'start'
//      followed by 'join'. Check that all submitted function objects
//      have been executed in the submission order.
//
//   2. Create a strand without starting it. Submit function objects #1,
//      #2, #3 ... #N by calling 'post' on the strand's executor, given
//      that the submitted function object #i itself submits a function
//      object #(N + i) via a call to 'post' on the same executor. Then,
//      call 'start' followed by 'join'. Check that all submitted
//      function objects have been executed in the submission order.
//
//   3. Create a strand without starting it. Submit function objects #1,
//      #2 and #3 by calling 'post' for each object, given that object
//      #3 throws on copy. Check that:
//      - No memory is leaked;
//      - The strand state is the same as before the exception;
//      - The strand stays usable, i.e. new function objects can be
//        submitted.
//
//   4. Create a strand with an inner executor such that it throws on
//      submission. Start the strand and submit a function object to it via
//      a call to 'post'. Check that:
//      - No memory is leaked;
//      - The strand state is the same as before the exception;
//      - The strand stays usable, i.e. new function objects can be
//        submitted.
//
// Testing:
//   mwcex::StrandExecutor::post
// ------------------------------------------------------------------------
{
    typedef mwcex::Strand<TestExecutionContext::ExecutorType> Strand;

    bslma::TestAllocator alloc;

    // 1. general use-case
    {
        static const int k_NUM_THREADS = 4;
        static const int k_NUM_JOBS    = 10;

        bsl::vector<int> out(&alloc);

        // create a strand on top a multi-threaded context
        TestExecutionContext ctx(k_NUM_THREADS, &alloc);
        Strand               strand(ctx.executor(), &alloc);

        // submit function objects
        for (int i = 0; i < k_NUM_JOBS; ++i) {
            strand.executor().post(bdlf::BindUtil::bind(PushBack(), &out, i));

            ASSERT_EQ(strand.outstandingJobs(), static_cast<size_t>(i + 1));
        }

        // no function object executed so far
        ASSERT(out.empty());

        // start the context and join it
        strand.start();
        strand.join();

        // all function objects executed in submission order
        ASSERT_EQ(out.size(), static_cast<size_t>(k_NUM_JOBS));
        for (int i = 0; i < k_NUM_JOBS; ++i) {
            ASSERT_EQ(out[i], i);
        }
    }

    // 2. call 'post' from whitin an executing function object
    {
        static const int k_NUM_THREADS = 4;
        static const int k_NUM_JOBS    = 10;

        bsl::vector<int> out(&alloc);

        // create a strand on top a multi-threaded context
        TestExecutionContext ctx(k_NUM_THREADS, &alloc);
        Strand               strand(ctx.executor(), &alloc);

        // submit function objects
        for (int i = 0; i < k_NUM_JOBS; ++i) {
            bsl::function<void()> pushBack(
                bsl::allocator_arg,
                &alloc,
                bdlf::BindUtil::bind(PushBack(), &out, i));

            // submit a function object that submits another function object
            // via a call to 'post()'
            strand.executor().post(bdlf::BindUtil::bindS(&alloc,
                                                         Post(),
                                                         strand.executor(),
                                                         pushBack));
        }

        // start the context and join it
        strand.start();
        strand.join();

        // all function objects executed in submission order
        ASSERT_EQ(out.size(), static_cast<size_t>(k_NUM_JOBS));
        for (int i = 0; i < k_NUM_JOBS; ++i) {
            ASSERT_EQ(out[i], i);
        }
    }

    // 3. exception safety
    {
        static const int k_NUM_THREADS = 4;

        // create a strand on top a multi-threaded context
        TestExecutionContext ctx(k_NUM_THREADS, &alloc);
        Strand               strand(ctx.executor(), &alloc);

        // sumbit function objects #1 and #2
        strand.executor().post(NoOp());
        strand.executor().post(NoOp());
        ASSERT_EQ(strand.outstandingJobs(), 2u);

        ThrowOnCopy throwOnCopy;
        bool        exceptionThrown = false;

        // submit function object #3 that throws on copy
        try {
            strand.executor().post(throwOnCopy);
        }
        catch (const ThrowOnCopy::ExceptionType&) {
            exceptionThrown = true;
        }

        // exception thrown
        ASSERT_EQ(exceptionThrown, true);

        // strand not affected
        ASSERT_EQ(strand.outstandingJobs(), 2u);

        // strand stays usable
        strand.executor().post(NoOp());
        strand.executor().post(NoOp());
        ASSERT_EQ(strand.outstandingJobs(), 4u);

        // NOTE: The test allocator will check that no memory is leaked on
        //       destruction.
    }

    // 4. exception safety
    {
        // create a strand with a throwing executor
        mwcex::Strand<ThrowingExecutor> strand(ThrowingExecutor(), &alloc);

        // start the strand
        strand.start();

        for (int i = 0; i < 3; ++i) {
            // Repeat several times.

            // post a job, the inner executor will throw
            bool exceptionThrown = false;
            try {
                strand.executor().post(NoOp());
            }
            catch (const ThrowingExecutor::ExceptionType&) {
                exceptionThrown = true;
            }

            // exception thrown
            ASSERT_EQ(exceptionThrown, true);

            // strand not affected
            ASSERT_EQ(strand.outstandingJobs(), 0u);
        }

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
//   1. Create a strand without starting it. Submit several function
//      objects by calling 'dispatch' on the strand's executor. Check
//      that no function object has been executed so far. Then, call
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
//   mwcex::StrandExecutor::dispatch
// ------------------------------------------------------------------------
{
    typedef mwcex::Strand<TestExecutionContext::ExecutorType> Strand;

    bslma::TestAllocator alloc;

    // 1. call 'dispatch' from "outside"
    {
        static const int k_NUM_THREADS = 4;
        static const int k_NUM_JOBS    = 10;

        bsl::vector<int> out(&alloc);

        // create a strand on top a multi-threaded context
        TestExecutionContext ctx(k_NUM_THREADS, &alloc);
        Strand               strand(ctx.executor(), &alloc);

        // submit function objects
        for (int i = 0; i < k_NUM_JOBS; ++i) {
            strand.executor().dispatch(
                bdlf::BindUtil::bind(PushBack(), &out, i));

            ASSERT_EQ(strand.outstandingJobs(), static_cast<size_t>(i + 1));
        }

        // no function object executed so far
        ASSERT(out.empty());

        // start the context and join it
        strand.start();
        strand.join();

        // all function objects executed in submission order
        ASSERT_EQ(out.size(), static_cast<size_t>(k_NUM_JOBS));
        for (int i = 0; i < k_NUM_JOBS; ++i) {
            ASSERT_EQ(out[i], i);
        }
    }

    // 2. call 'dispatch' from "inside"
    {
        static const int k_NUM_THREADS = 4;
        static const int k_NUM_JOBS    = 10;

        bsl::vector<int> out(&alloc);
        bslmt::Semaphore startedSignal,  // signaled when job starts executing
            continueSignal;              // signaled to allow job to finish

        // create a strand on top a multi-threaded context
        TestExecutionContext ctx(k_NUM_THREADS, &alloc);
        Strand               strand(ctx.executor(), &alloc);

        // submit function objects
        for (int i = 0; i < k_NUM_JOBS; ++i) {
            bsl::function<void()> pushBack(
                bsl::allocator_arg,
                &alloc,
                bdlf::BindUtil::bind(PushBack(), &out, i));

            // submit a function object that submits another function object
            // via a call to 'dispatch'
            strand.executor().post(bdlf::BindUtil::bindS(&alloc,
                                                         Dispatch(),
                                                         strand.executor(),
                                                         pushBack));

            // submit a function object we will synchronize with
            strand.executor().post(bdlf::BindUtil::bind(Synchronize(),
                                                        &startedSignal,
                                                        &continueSignal));
        }

        // start the strand
        strand.start();

        for (int i = 0; i < k_NUM_JOBS; ++i) {
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
//   contexts are 'strand1' and 'strand2' respectively, call
//   'ex1.swap(ex2)' and check that 'ex1' now refers to 'strand2' and 'ex2'
//   now refers to 'strand1'.
//
// Testing:
//   mwcex::StrandExecutor::swap
// ------------------------------------------------------------------------
{
    typedef mwcex::Strand<IdentifiableExecutor> Strand;

    bslma::TestAllocator alloc;
    Strand               strand1(&alloc), strand2(&alloc);

    Strand::ExecutorType ex1 = strand1.executor();
    Strand::ExecutorType ex2 = strand2.executor();

    // swap
    ex1.swap(ex2);

    // check
    ASSERT_EQ(&ex1.context(), &strand2);
    ASSERT_EQ(&ex2.context(), &strand1);
}

static void test10_executor_runningInThisThread()
// ------------------------------------------------------------------------
// EXECUTOR RUNNING IN THIS THREAD
//
// Concerns:
//   Ensure proper behavior of the 'runningInThisThread' method.
//
// Plan:
//   Check that 'runningInThisThread' returns 'true' if the current
//   thread of execution is running a function object that was submitted
//   to the executor's associated strand, and 'false otherwise.
//
// Testing:
//   mwcex::Strand::ExecutorType::runningInThisThread
// ------------------------------------------------------------------------
{
    typedef mwcex::Strand<TestExecutionContext::ExecutorType> Strand;
    typedef Strand::ExecutorType                              StrandExecutor;

    static const int k_NUM_THREADS = 4;

    bslma::TestAllocator alloc;

    // storage for the result of a 'runningInThisThread' invocation
    bool result1 = false;
    bool result2 = false;
    bool result3 = false;

    // create a strand on top a multi-threaded context
    TestExecutionContext ctx(k_NUM_THREADS, &alloc);
    Strand               strand(ctx.executor(), &alloc);

    // obtain executor
    StrandExecutor ex = strand.executor();

    // submit several jobs that saves the result of a 'runningInThisThread'
    // invocation
    ex.post(bdlf::BindUtil::bind(
        Assign(),
        &result1,
        bdlf::BindUtil::bind(&StrandExecutor::runningInThisThread, &ex)));

    ex.post(bdlf::BindUtil::bind(
        Assign(),
        &result2,
        bdlf::BindUtil::bind(&StrandExecutor::runningInThisThread, &ex)));

    ex.post(bdlf::BindUtil::bind(
        Assign(),
        &result3,
        bdlf::BindUtil::bind(&StrandExecutor::runningInThisThread, &ex)));

    strand.start();
    strand.join();

    // the saved result is 'true'
    ASSERT_EQ(result1, true);
    ASSERT_EQ(result2, true);
    ASSERT_EQ(result3, true);

    // when calling 'runningInThisThread' from "outside"t he result is
    // 'false'
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
//   associated 'mwcex::Strand' object.
//
// Testing:
//   mwcex::StrandExecutor::context
// ------------------------------------------------------------------------
{
    typedef mwcex::Strand<IdentifiableExecutor> Strand;

    bslma::TestAllocator alloc;
    Strand               strand1(&alloc), strand2(&alloc);

    Strand::ExecutorType ex1 = strand1.executor();
    ASSERT_EQ(&ex1.context(), &strand1);

    Strand::ExecutorType ex2 = strand2.executor();
    ASSERT_EQ(&ex2.context(), &strand2);
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(mwctst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    // mwcex::Strand
    case 1: test1_strand_creators(); break;
    case 2: test2_strand_start(); break;
    case 3: test3_strand_stop(); break;
    case 4: test4_strand_join(); break;
    case 5: test5_strand_dropPendingJobs(); break;
    case 6: test6_strand_outstandingJobs(); break;

    // mwcex::StrandExecutor
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

    TEST_EPILOG(mwctst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
