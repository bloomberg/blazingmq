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

// bmqex_executionutil.t.cpp                                          -*-C++-*-
#include <bmqex_executionutil.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

#include <bmqex_executionpolicy.h>
#include <bmqex_executionproperty.h>

// BDE
#include <bdlf_bind.h>
#include <bdlmt_threadpool.h>
#include <bsl_functional.h>
#include <bsl_iostream.h>
#include <bsl_limits.h>
#include <bsl_numeric.h>
#include <bsl_ostream.h>
#include <bsla_annotations.h>
#include <bslma_testallocator.h>
#include <bslmf_issame.h>
#include <bslmt_semaphore.h>
#include <bslmt_threadattributes.h>
#include <bsls_assert.h>
#include <bsls_atomic.h>
#include <bsls_keyword.h>

// CONVENIENCE
using namespace BloombergLP;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {

// ===========================
// struct DummyNullaryFunction
// ===========================

/// Provides a dummy function object which call operator returns a value of
/// type `R`.
template <class R>
struct DummyNullaryFunction {
    /// Defines the result type of the call operator.
    typedef R ResultType;

    // ACCESSORS

    /// Not implemented.
    R operator()() const;
};

// ====================
// struct SetFlagOnCall
// ====================

/// Provides a function object that sets a boolean flag.
struct SetFlagOnCall {
    // TYPES

    /// Defines the result type of the call operator.
    typedef void ResultType;

    // DATA
    bool* d_flag_p;

    // CREATORS
    explicit SetFlagOnCall(bool* flag)
    : d_flag_p(flag)
    {
        // NOTHING
    }

    // ACCESSORS
    void operator()() const { *d_flag_p = true; }
};

// ==================
// struct ThrowOnCall
// ==================

/// Provides a function object that throws an instance of
/// `ThrowOnCall::ExceptionType`.
struct ThrowOnCall {
    // TYPES

    /// Defines the type of the throwed exception.
    struct ExceptionType {};

    /// Defines the result type of the call operator.
    typedef void ResultType;

    // ACCESSORS
    BSLA_NORETURN
    void operator()() const { throw ExceptionType(); }
};

// ==========================
// struct SynchronizeCallable
// ==========================

/// Provides a function object to synchronize with a callable object.
struct SynchronizeCallable {
    /// Defines the result type of the call operator.
    typedef void ResultType;

    template <class F>
    void operator()(bslmt::Semaphore* waitSemaphore,
                    bslmt::Semaphore* postSemaphore,
                    const F&          function) const
    {
        waitSemaphore->wait();
        try {
            function();
        }
        catch (...) {
            postSemaphore->post();
            throw;
        }
        postSemaphore->post();
    }
};

// ===========================
// struct TestExecutionContext
// ===========================

/// Provides an single-threaded execution context for test purposes that
/// executes submitted function objects in the context of an owned thread.
class TestExecutionContext {
  public:
    // TYPES

    /// Defines the type of accepted function objects.
    typedef bsl::function<void()> Job;

    /// Provides statistics.
    struct Statistics {
        /// The number of times a function object has been submitted for
        /// execution via a call to `post` an the context's executor.
        bsls::AtomicInt d_postCount;

        /// The number of times a function object has been submitted for
        /// execution via a call to `dispatch` an the context's executor.
        bsls::AtomicInt d_dispatchCount;

        /// The number of times a function object has been executed without
        /// throwing an exception.
        bsls::AtomicInt d_successfulExecutionCount;

        /// The number of times a function object has been executed with
        /// throwing an exception.
        bsls::AtomicInt d_exceptionalExecutionCount;
    };

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
        void post(const Job& f) const
        {
            ++d_context_p->d_statistics.d_postCount;

            int rc = d_context_p->d_threadPool.enqueueJob(
                bdlf::BindUtil::bind(&TestExecutionContext::doInvoke,
                                     d_context_p,
                                     f));
            BSLS_ASSERT_OPT(rc == 0);
        }

        /// Submit the specified function object `f` for execution.
        void dispatch(const Job& f) const
        {
            ++d_context_p->d_statistics.d_dispatchCount;

            int rc = d_context_p->d_threadPool.enqueueJob(
                bdlf::BindUtil::bind(&TestExecutionContext::doInvoke,
                                     d_context_p,
                                     f));
            BSLS_ASSERT_OPT(rc == 0);
        }
    };

  private:
    // PRIVATE DATA

    // Collected statistics.
    Statistics d_statistics;

    // Underlying mechanism used to submit function objects.
    bdlmt::ThreadPool d_threadPool;

  private:
    // PRIVATE MANIPULATORS
    void doInvoke(const Job& f) BSLS_KEYWORD_NOEXCEPT
    {
        try {
            f();
            ++d_statistics.d_successfulExecutionCount;
        }
        catch (...) {
            ++d_statistics.d_exceptionalExecutionCount;
        }
    }

  private:
    // NOT IMPLEMENTED
    TestExecutionContext(const TestExecutionContext&) BSLS_KEYWORD_DELETED;
    TestExecutionContext&
    operator=(const TestExecutionContext&) BSLS_KEYWORD_DELETED;

  public:
    // CREATORS

    /// Create a `TestExecutionContext` object. Specify an `allocator` used
    /// to supply memory.
    explicit TestExecutionContext(bslma::Allocator* allocator)
    : d_statistics()
    , d_threadPool(bslmt::ThreadAttributes(),        // use default attributes
                   1,                                // minThreads
                   1,                                // maxThreads
                   bsl::numeric_limits<int>::max(),  // maxIdleTime
                   allocator)
    {
        // PRECONDITIONS
        BSLS_ASSERT(allocator);

        int rc = d_threadPool.start();
        BSLS_ASSERT_OPT(rc == 0);
    }

    /// Destroy this object. Wait till all pending function objects
    /// complete. Then, join the owned thread.
    ~TestExecutionContext() { d_threadPool.stop(); }

  public:
    // MANIPULATORS

    /// Block the calling thread until all pending function objects are
    /// executed.
    void drain()
    {
        d_threadPool.drain();

        // re-enable queuing
        int rc = d_threadPool.start();
        BSLS_ASSERT_OPT(rc == 0);
    }

  public:
    // ACCESSORS

    /// Return an executor that may be used for submitting function objects
    /// to this execution context.
    ExecutorType executor() const BSLS_KEYWORD_NOEXCEPT
    {
        return const_cast<TestExecutionContext*>(this);
    }

    /// Return collected statistics.
    const Statistics& statistics() const BSLS_KEYWORD_NOEXCEPT
    {
        return d_statistics;
    }
};

}  // close unnamed namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_executeResult()
// ------------------------------------------------------------------------
// EXECUTE RESULT
//
// Concerns:
//   Ensure proper behavior of the 'ExecuteResult' metafunction
//
// Plan:
//   Check that the 'bmqex::ExecutionUtil::ExecuteResult' metafunction
//   conforms to the specification. That is:
//
//   Given:
//:  o A type 'P', that is an execution policy type (see
//:    'bmqex_executionpolicy').
//:
//:  o A type 'F', such that 'bsl::decay_t<F>' satisfies the requirements
//:    of Destructible and MoveConstructible as specified in the C++
//:    standard, and such that and object 'f' of type 'F' is callable as
//:    'DECAY_COPY(std::forward<F>(f))()'.
//
//   The type of 'bmqex::ExecutionUtil::ExecuteResult<P, F>::Type'
//   expression is 'void' for One-Way policies.
//
// Testing:
//   bmqex::ExecutionUtil::ExecuteResult
// ------------------------------------------------------------------------
{
    // One-Way policy
    {
        typedef bmqex::ExecutionPolicy<bmqex::ExecutionProperty::OneWay>
            PolicyType;

        BMQTST_ASSERT((bsl::is_same<bmqex::ExecutionUtil::ExecuteResult<
                                        PolicyType,
                                        DummyNullaryFunction<void> >::Type,
                                    void>::value));

        BMQTST_ASSERT((bsl::is_same<bmqex::ExecutionUtil::ExecuteResult<
                                        PolicyType,
                                        DummyNullaryFunction<int> >::Type,
                                    void>::value));

        BMQTST_ASSERT((bsl::is_same<bmqex::ExecutionUtil::ExecuteResult<
                                        PolicyType,
                                        DummyNullaryFunction<int&> >::Type,
                                    void>::value));
    }
}

static void test2_execute_one_way_never_blocking()
// ------------------------------------------------------------------------
// EXECUTE ONE WAY NEVER BLOCKING
//
// Concerns:
//   Ensure proper behavior of the 'execute' function accepting a One-Way
//   Never Blocking execution policy.
//
// Plan:
//   1. Execute a function object 'f' on an executor 'ex' specifying a
//      One-Way Never Blocking execution policy. Check that:
//:     o The execution function does not block the calling thread pending
//:       completion of the submitted function object invocation;
//:     o The function object is executed as if by a call to 'ex.post(f)'.
//
//   2. Execute a function object 'f', according to a One-Way Never
//      Blocking execution policy, and such that 'f()' throws an exception.
//      Check that the execution was propagated to the caller of 'f' (but
//      not to the caller of the execution function).
//
// Testing:
//   bmqex::ExecutionUtil::execute
// ------------------------------------------------------------------------
{
    bslma::TestAllocator alloc;
    TestExecutionContext context(&alloc);

    // 1. General use-case.
    {
        bool          executed = false;
        SetFlagOnCall toBeExecuted(&executed);

        // semaphores used for synchronization with the submitted function
        // object
        bslmt::Semaphore sem1, sem2;

        // submit function object for execution
        bmqex::ExecutionUtil::execute(
            bmqex::ExecutionPolicyUtil::oneWay()
                .neverBlocking()
                .useExecutor(context.executor())
                .useAllocator(&alloc),
            bdlf::BindUtil::bind(SynchronizeCallable(),
                                 &sem1,
                                 &sem2,
                                 toBeExecuted));

        // function object was submitted for execution via a call to 'post()'
        // on the specified executor
        BMQTST_ASSERT_EQ(context.statistics().d_postCount, 1);

        // function object invocation *not* completed
        BMQTST_ASSERT(!executed);
        BMQTST_ASSERT_EQ(context.statistics().d_successfulExecutionCount, 0);

        sem1.post();  // allow the function object to continue execution
        sem2.wait();  // wait till the function object finish executing

        // make sure all postconditions on the context are established
        context.drain();

        // function object invocation completed
        BMQTST_ASSERT(executed);
        BMQTST_ASSERT_EQ(context.statistics().d_successfulExecutionCount, 1);
    }

    // 2. Submitted function object throws an exception.
    {
        // submit function object for execution
        bmqex::ExecutionUtil::execute(bmqex::ExecutionPolicyUtil::oneWay()
                                          .neverBlocking()
                                          .useExecutor(context.executor())
                                          .useAllocator(&alloc),
                                      ThrowOnCall());

        // make sure all postconditions on the context are established
        context.drain();

        // the exception was propagated to the caller
        BMQTST_ASSERT_EQ(context.statistics().d_exceptionalExecutionCount, 1);
    }
}

static void test3_execute_one_way_possibly_blocking()
// ------------------------------------------------------------------------
// EXECUTE ONE WAY POSSIBLY BLOCKING
//
// Concerns:
//   Ensure proper behavior of the 'execute' function accepting a One-Way
//   Possibly Blocking execution policy.
//
// Plan:
//   1. Execute a function object 'f' on an executor 'ex' specifying a
//      One-Way Possibly Blocking execution policy. Check that:
//:     o The execution function does not block the calling thread pending
//:       completion of the submitted function object invocation;
//:     o The function object is executed as if by a call to
//:       'ex.dispatch(f)'.
//
//   2. Execute a function object 'f', according to a One-Way Possibly
//      Blocking execution policy, and such that 'f()' throws an exception.
//      Check that the execution was propagated to the caller of 'f' (but
//      not to the caller of the execution function).
//
// Testing:
//   bmqex::ExecutionUtil::execute
// ------------------------------------------------------------------------
{
    bslma::TestAllocator alloc;
    TestExecutionContext context(&alloc);

    // 1. General use-case.
    {
        bool          executed = false;
        SetFlagOnCall toBeExecuted(&executed);

        // semaphores used for synchronization with the submitted function
        // object
        bslmt::Semaphore sem1, sem2;

        // submit function object for execution
        bmqex::ExecutionUtil::execute(
            bmqex::ExecutionPolicyUtil::oneWay()
                .possiblyBlocking()
                .useExecutor(context.executor())
                .useAllocator(&alloc),
            bdlf::BindUtil::bind(SynchronizeCallable(),
                                 &sem1,
                                 &sem2,
                                 toBeExecuted));

        // function object was submitted for execution via a call to
        // 'dispatch()' on the specified executor
        BMQTST_ASSERT_EQ(context.statistics().d_dispatchCount, 1);

        // function object invocation *not* completed
        BMQTST_ASSERT(!executed);
        BMQTST_ASSERT_EQ(context.statistics().d_successfulExecutionCount, 0);

        sem1.post();  // allow the function object to continue execution
        sem2.wait();  // wait till the function object finish executing

        // make sure all postconditions on the context are established
        context.drain();

        // function object invocation completed
        BMQTST_ASSERT(executed);
        BMQTST_ASSERT_EQ(context.statistics().d_successfulExecutionCount, 1);
    }

    // 2. Submitted function object throws an exception.
    {
        // submit function object for execution
        bmqex::ExecutionUtil::execute(bmqex::ExecutionPolicyUtil::oneWay()
                                          .possiblyBlocking()
                                          .useExecutor(context.executor())
                                          .useAllocator(&alloc),
                                      ThrowOnCall());

        // make sure all postconditions on the context are established
        context.drain();

        // the exception was propagated to the caller
        BMQTST_ASSERT_EQ(context.statistics().d_exceptionalExecutionCount, 1);
    }
}

static void test4_execute_one_way_always_blocking()
// ------------------------------------------------------------------------
// EXECUTE ONE WAY ALWAYS BLOCKING
//
// Concerns:
//   Ensure proper behavior of the 'execute' function accepting a One-Way
//   Always Blocking execution policy..
//
// Plan:
//   1. Execute a function object 'f' on an executor 'ex' specifying a
//      One-Way Always Blocking execution policy. Check that:
//:     o The execution function does block the calling thread pending
//:       completion of the submitted function object invocation;
//:     o The function object is executed as if by a call to
//:       'ex.dispatch(f)'.
//
//   2. Execute a function object 'f', according to a One-Way Always
//      Blocking execution policy, and such that 'f()' throws an exception.
//      Check that the execution was propagated to the caller of 'f' (but
//      not to the caller of the execution function).
//
// Testing:
//   bmqex::ExecutionUtil::execute
// ------------------------------------------------------------------------
{
    bslma::TestAllocator alloc;
    TestExecutionContext context(&alloc);

    // 1. General use-case.
    {
        bool          executed = false;
        SetFlagOnCall toBeExecuted(&executed);

        // submit function object for execution
        bmqex::ExecutionUtil::execute(bmqex::ExecutionPolicyUtil::oneWay()
                                          .alwaysBlocking()
                                          .useExecutor(context.executor())
                                          .useAllocator(&alloc),
                                      toBeExecuted);

        // function object was submitted for execution via a call to
        // 'dispatch()' on the specified executor
        BMQTST_ASSERT_EQ(context.statistics().d_dispatchCount, 1);

        // function object invocation completed
        BMQTST_ASSERT(executed);

        // make sure all postconditions on the context are established
        context.drain();

        // function object executed
        BMQTST_ASSERT_EQ(context.statistics().d_successfulExecutionCount, 1);
    }

    // 2. Submitted function object throws an exception.
    {
        // submit function object for execution
        bmqex::ExecutionUtil::execute(bmqex::ExecutionPolicyUtil::oneWay()
                                          .alwaysBlocking()
                                          .useExecutor(context.executor())
                                          .useAllocator(&alloc),
                                      ThrowOnCall());

        // make sure all postconditions on the context are established
        context.drain();

        // the exception was propagated to the caller
        BMQTST_ASSERT_EQ(context.statistics().d_exceptionalExecutionCount, 1);
    }
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    // traits
    case 1: test1_executeResult(); break;

    // One-Way execution
    case 2: test2_execute_one_way_never_blocking(); break;
    case 3: test3_execute_one_way_possibly_blocking(); break;
    case 4: test4_execute_one_way_always_blocking(); break;

    default: {
        bsl::cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND."
                  << bsl::endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_GBL_ALLOC);
}
