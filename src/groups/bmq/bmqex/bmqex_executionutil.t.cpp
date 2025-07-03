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
#include <bmqex_future.h>

// BDE
#include <bdlf_bind.h>
#include <bdlmt_threadpool.h>
#include <bsl_functional.h>
#include <bsl_iostream.h>
#include <bsl_limits.h>
#include <bsl_memory.h>
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
#include <bsls_libraryfeatures.h>

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
    template <class A1>
    void operator()(const A1&) const
    {
        // NOTHING
    }
};

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

// ================================
// struct DummyContinuationFunction
// ================================

/// Provides a dummy function object which call operator takes a value of
/// type `bmqex::FutureResult<R1>` and returns a value of type `R2`.
template <class R1, class R2>
struct DummyContinuationFunction {
    /// Defines the result type of the call operator.
    typedef R2 ResultType;

    // ACCESSORS

    /// Not implemented.
    R2 operator()(bmqex::FutureResult<R1>) const;
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

// ========================
// struct ReturnValueOnCall
// ========================

/// Provides a function object that retuns `ReturnValueOnCall::k_VALUE`.
struct ReturnValueOnCall {
    // TYPES

    /// Defines the type of the returned value.
    typedef int ValueType;

    /// Defines the result type of the call operator.
    typedef ValueType ResultType;

    // CLASS DATA
    static const ValueType k_VALUE;

    // ACCESSORS
    int operator()() const { return k_VALUE; }
};
const ReturnValueOnCall::ValueType ReturnValueOnCall::k_VALUE = 42;

// ============================
// struct ReturnReferenceOnCall
// ============================

/// Provides a function object that retuns a reference to
/// `ReturnReferenceOnCall::k_VALUE`.
struct ReturnReferenceOnCall {
    // TYPES

    /// Defines the type of the returned value.
    typedef int ValueType;

    /// Defines the result type of the call operator.
    typedef ValueType& ResultType;

    // CLASS DATA
    static ValueType k_VALUE;

    // ACCESSORS
    int& operator()() const { return k_VALUE; }
};
ReturnReferenceOnCall::ValueType ReturnReferenceOnCall::k_VALUE = 42;

// =======================
// struct ReturnVoidOnCall
// =======================

/// Provides a function object that retuns `void`.
struct ReturnVoidOnCall {
    // TYPES

    /// Defines the result type of the call operator.
    typedef void ResultType;

    // ACCESSORS
    void operator()() const
    {
        // NOTHING
    }
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

// ===========================
// struct SetValueContinuation
// ===========================

/// Provides a continuation callback taking a result of type `VALUE`
/// and than assigning that result to an output value.
template <class VALUE>
struct SetValueContinuation {
    // TYPES

    /// Defines the result type of the call operator.
    typedef void ResultType;

    // DATA
    VALUE* d_value_p;

    // CREATORS
    explicit SetValueContinuation(VALUE* value)
    : d_value_p(value)
    {
        // NOTHING
    }

    // ACCESSORS
    void operator()(bmqex::FutureResult<VALUE> result) const
    {
        *d_value_p = result.get();
    }
};

// ===============================
// struct ForwardValueContinuation
// ===============================

/// Provides a continuation callback taking a result of type `VALUE`
/// and than returning that result as is.
template <class VALUE>
struct ForwardValueContinuation {
    // TYPES

    /// Defines the result type of the call operator.
    typedef VALUE ResultType;

    // ACCESSORS
    VALUE operator()(bmqex::FutureResult<VALUE> result) const
    {
        return result.get();
    }
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
//:
//:  o A type 'R', that is the result type encoded in 'P', if explicitly
//:    specified, or the result type of 'DECAY_COPY(std::forward<F>(f))()'
//:    otherwise.
//
//   The type of 'bmqex::ExecutionUtil::ExecuteResult<P, F>::Type'
//   expression is the follow:
//:  o If 'P' is a One-Way policy - 'void'.
//:
//:  o If 'P' is a Two-Way policy - 'bmqex::Future<R>'.
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

    // Two-Way policy
    {
        typedef bmqex::ExecutionPolicy<bmqex::ExecutionProperty::TwoWay>
            PolicyType;

        BMQTST_ASSERT((bsl::is_same<bmqex::ExecutionUtil::ExecuteResult<
                                        PolicyType,
                                        DummyNullaryFunction<void> >::Type,
                                    bmqex::Future<void> >::value));

        BMQTST_ASSERT((bsl::is_same<bmqex::ExecutionUtil::ExecuteResult<
                                        PolicyType,
                                        DummyNullaryFunction<int> >::Type,
                                    bmqex::Future<int> >::value));

        BMQTST_ASSERT((bsl::is_same<bmqex::ExecutionUtil::ExecuteResult<
                                        PolicyType,
                                        DummyNullaryFunction<int&> >::Type,
                                    bmqex::Future<int&> >::value));
    }

    // Two-Way policy with encoded result type
    {
        typedef bmqex::ExecutionPolicy<
            bmqex::ExecutionProperty::TwoWayR<float> >
            PolicyType;

        BMQTST_ASSERT((bsl::is_same<bmqex::ExecutionUtil::ExecuteResult<
                                        PolicyType,
                                        DummyNullaryFunction<void> >::Type,
                                    bmqex::Future<float> >::value));

        BMQTST_ASSERT((bsl::is_same<bmqex::ExecutionUtil::ExecuteResult<
                                        PolicyType,
                                        DummyNullaryFunction<int> >::Type,
                                    bmqex::Future<float> >::value));

        BMQTST_ASSERT((bsl::is_same<bmqex::ExecutionUtil::ExecuteResult<
                                        PolicyType,
                                        DummyNullaryFunction<int&> >::Type,
                                    bmqex::Future<float> >::value));
    }
}

static void test2_thenExecuteResult()
// ------------------------------------------------------------------------
// THEN EXECUTE RESULT
//
// Concerns:
//   Ensure proper behavior of the 'ThenExecuteResult' metafunction
//
// Plan:
//   Check that the 'bmqex::ExecutionUtil::ThenExecuteResult' metafunction
//   conforms to the specification. That is:
//
//   Given:
//:  o A type 'P', that is an execution policy type (see
//:   'bmqex_executionpolicy').
//:
//:  o A type 'FT', that is 'bmqex::Future<R1>'.
//:
//:  o A type 'FN', such that 'bsl::decay_t<FN>' satisfies the requirements
//:    of Destructible and MoveConstructible as specified in the C++
//:    standard, and such that and object 'fn' of type 'FN' is callable as
//:    'DECAY_COPY(std::forward<FN>(fn))(bsl::declval<FutureResult<R1>>())'
//:
//:  o A type 'R2', that is the result type encoded in 'P', if explicitly
//:    specified, or the result type of
//:    'DECAY_COPY(std::forward<FN>(fn))(bsl::declval<FutureResult<R1>>())'
//:    otherwise.
//
//   The type of 'bmqex::ExecutionUtil::ThenExecuteResult<P, FT, FN>::Type'
//   expression is the follow:
//:  o If 'P' is a One-Way policy - 'void'.
//:
//:  o If 'P' is a Two-Way policy - 'bmqex::Future<R2>'.
//
// Testing:
//   bmqex::ExecutionUtil::ThenExecuteResult
// ------------------------------------------------------------------------
{
    // One-Way policy
    {
        typedef bmqex::ExecutionPolicy<bmqex::ExecutionProperty::OneWay>
            PolicyType;

        BMQTST_ASSERT(
            (bsl::is_same<bmqex::ExecutionUtil::ThenExecuteResult<
                              PolicyType,
                              bmqex::Future<void>,
                              DummyContinuationFunction<void, void> >::Type,
                          void>::value));

        BMQTST_ASSERT(
            (bsl::is_same<bmqex::ExecutionUtil::ThenExecuteResult<
                              PolicyType,
                              bmqex::Future<int>,
                              DummyContinuationFunction<int, int> >::Type,
                          void>::value));

        BMQTST_ASSERT(
            (bsl::is_same<bmqex::ExecutionUtil::ThenExecuteResult<
                              PolicyType,
                              bmqex::Future<int&>,
                              DummyContinuationFunction<int&, int&> >::Type,
                          void>::value));
    }

    // Two-Way policy
    {
        typedef bmqex::ExecutionPolicy<bmqex::ExecutionProperty::TwoWay>
            PolicyType;

        BMQTST_ASSERT(
            (bsl::is_same<bmqex::ExecutionUtil::ThenExecuteResult<
                              PolicyType,
                              bmqex::Future<void>,
                              DummyContinuationFunction<void, void> >::Type,
                          bmqex::Future<void> >::value));

        BMQTST_ASSERT(
            (bsl::is_same<bmqex::ExecutionUtil::ThenExecuteResult<
                              PolicyType,
                              bmqex::Future<int>,
                              DummyContinuationFunction<int, int> >::Type,
                          bmqex::Future<int> >::value));

        BMQTST_ASSERT(
            (bsl::is_same<bmqex::ExecutionUtil::ThenExecuteResult<
                              PolicyType,
                              bmqex::Future<int&>,
                              DummyContinuationFunction<int&, int&> >::Type,
                          bmqex::Future<int&> >::value));
    }

    // Two-Way policy with encoded result type
    {
        typedef bmqex::ExecutionPolicy<
            bmqex::ExecutionProperty::TwoWayR<float> >
            PolicyType;

        BMQTST_ASSERT(
            (bsl::is_same<bmqex::ExecutionUtil::ThenExecuteResult<
                              PolicyType,
                              bmqex::Future<void>,
                              DummyContinuationFunction<void, void> >::Type,
                          bmqex::Future<float> >::value));

        BMQTST_ASSERT(
            (bsl::is_same<bmqex::ExecutionUtil::ThenExecuteResult<
                              PolicyType,
                              bmqex::Future<int>,
                              DummyContinuationFunction<int, int> >::Type,
                          bmqex::Future<float> >::value));

        BMQTST_ASSERT(
            (bsl::is_same<bmqex::ExecutionUtil::ThenExecuteResult<
                              PolicyType,
                              bmqex::Future<int&>,
                              DummyContinuationFunction<int&, int&> >::Type,
                          bmqex::Future<float> >::value));
    }
}

static void test3_execute_one_way_never_blocking()
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

static void test4_execute_one_way_possibly_blocking()
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

static void test5_execute_one_way_always_blocking()
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

static void test6_execute_two_way_never_blocking()
// ------------------------------------------------------------------------
// EXECUTE TWO WAY NEVER BLOCKING
//
// Concerns:
//   Ensure proper behavior of the 'execute' function accepting a Two-Way
//   Never Blocking execution policy.
//
// Plan:
//   1. Execute a function object 'f' on an executor 'ex' specifying a
//      Two-Way Never Blocking execution policy. Check that:
//:     o The execution function does not block the calling thread pending
//:       completion of the submitted function object invocation;
//:     o The function object is executed as if by a call to 'ex.post(f)';
//:     o The execution function returns a 'bmqex::Future' object that
//:       becomes ready after the submitted function object completes.
//
//   2. Execute a function object 'f', according to a Two-Way Never
//      Blocking execution policy, and such that 'f()' returns a value.
//      Check that the execution function returns a 'bmqex::Future' object
//      to be containing the returned value.
//
//   3. Execute a function object 'f', according to a Two-Way Never
//      Blocking execution policy, and such that 'f()' returns a reference.
//      Check that the execution function returns a 'bmqex::Future' object
//      to be containing the returned reference.
//
//   4. Execute a function object 'f', according to a Two-Way Never
//      Blocking execution policy, and such that 'f()' returns 'void'.
//      Check that the execution function returns a 'bmqex::Future' object
//      to become ready.
//
//   5. Execute a function object 'f', according to a Two-Way Never
//      Blocking execution policy, and such that 'f()' throws an exception.
//      Check that the execution function returns a 'bmqex::Future' object
//      to be containing the thrown exception.
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
        bmqex::Future<void> result = bmqex::ExecutionUtil::execute(
            bmqex::ExecutionPolicyUtil::twoWay()
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

        // async result *not* ready
        BMQTST_ASSERT(!result.isReady());

        sem1.post();  // allow the function object to continue execution
        sem2.wait();  // wait till the function object finish executing

        // make sure all postconditions on the context are established
        context.drain();

        // async result ready
        BMQTST_ASSERT(result.isReady());

        // function object invocation completed
        BMQTST_ASSERT(executed);
        BMQTST_ASSERT_EQ(context.statistics().d_successfulExecutionCount, 1);
    }

    // 2. Submitted function object returns a value.
    {
        // submit function object for execution
        bmqex::Future<ReturnValueOnCall::ResultType> result =
            bmqex::ExecutionUtil::execute(bmqex::ExecutionPolicyUtil::twoWay()
                                              .neverBlocking()
                                              .useExecutor(context.executor())
                                              .useAllocator(&alloc),
                                          ReturnValueOnCall());

        // retrieve the result
        BMQTST_ASSERT_EQ(result.get(), ReturnValueOnCall::k_VALUE);
    }

    // 3. Submitted function object returns a reference.
    {
        // submit function object for execution
        bmqex::Future<ReturnReferenceOnCall::ResultType> result =
            bmqex::ExecutionUtil::execute(bmqex::ExecutionPolicyUtil::twoWay()
                                              .neverBlocking()
                                              .useExecutor(context.executor())
                                              .useAllocator(&alloc),
                                          ReturnReferenceOnCall());

        // retrieve the result
        BMQTST_ASSERT_EQ(&result.get(), &ReturnReferenceOnCall::k_VALUE);
    }

    // 4. Submitted function object returns 'void'.
    {
        // submit function object for execution
        bmqex::Future<ReturnVoidOnCall::ResultType> result =
            bmqex::ExecutionUtil::execute(bmqex::ExecutionPolicyUtil::twoWay()
                                              .neverBlocking()
                                              .useExecutor(context.executor())
                                              .useAllocator(&alloc),
                                          ReturnVoidOnCall());

        // retrieve the result
        result.get();
        BMQTST_ASSERT(result.isReady());
    }

#ifdef BSLS_LIBRARYFEATURES_HAS_CPP11_EXCEPTION_HANDLING
    // 5. Submitted function object throws an exception.
    {
        bslma::TestAllocator localAlloc;
        TestExecutionContext localContext(&localAlloc);

        // submit function object for execution
        bmqex::Future<ThrowOnCall::ResultType> result =
            bmqex::ExecutionUtil::execute(
                bmqex::ExecutionPolicyUtil::twoWay()
                    .neverBlocking()
                    .useExecutor(localContext.executor())
                    .useAllocator(&localAlloc),
                ThrowOnCall());

        // retrieve the result
        bool exceptionThrown = false;
        try {
            result.get();
        }
        catch (const ThrowOnCall::ExceptionType&) {
            exceptionThrown = true;
        }

        BMQTST_ASSERT(exceptionThrown);
    }
#endif
}

static void test7_execute_two_way_possibly_blocking()
// ------------------------------------------------------------------------
// EXECUTE TWO WAY POSSIBLY BLOCKING
//
// Concerns:
//   Ensure proper behavior of the 'execute' function accepting a Two-Way
//   Possibly Blocking execution policy.
//
// Plan:
//   1. Execute a function object 'f' on an executor 'ex' specifying a
//      Two-Way Possibly Blocking execution policy. Check that:
//:     o The execution function does not block the calling thread pending
//:       completion of the submitted function object;
//:     o The function object is executed as if by a call to
//:       'ex.dispatch(f)';
//:     o The execution function returns a 'bmqex::Future' object that
//:       becomes ready after the submitted function object completes.
//
//   2. Execute a function object 'f', according to a Two-Way Possibly
//      Blocking execution policy, and such that 'f()' returns a value.
//      Check that the execution function returns a 'bmqex::Future' object
//      to be containing the returned value.
//
//   3. Execute a function object 'f', according to a Two-Way Possibly
//      Blocking execution policy, and such that 'f()' returns a reference.
//      Check that the execution function returns a 'bmqex::Future' object
//      to be containing the returned reference.
//
//   4. Execute a function object 'f', according to a Two-Way Possibly
//      Blocking execution policy, and such that 'f()' returns 'void'.
//      Check that the execution function returns a 'bmqex::Future' object
//      to become ready.
//
//   5. Execute a function object 'f', according to a Two-Way Possibly
//      Blocking execution policy, and such that 'f()' throws an exception.
//      Check that the execution function return a 'bmqex::Future' object
//      to be containing the thrown exception.
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
        bmqex::Future<void> result = bmqex::ExecutionUtil::execute(
            bmqex::ExecutionPolicyUtil::twoWay()
                .possiblyBlocking()
                .useExecutor(context.executor())
                .useAllocator(&alloc),
            bdlf::BindUtil::bind(SynchronizeCallable(),
                                 &sem1,
                                 &sem2,
                                 toBeExecuted));

        // function object was submitted for execution via a call to
        // 'dispatch()' an the specified executor
        BMQTST_ASSERT_EQ(context.statistics().d_dispatchCount, 1);

        // function object invocation *not* completed
        BMQTST_ASSERT(!executed);
        BMQTST_ASSERT_EQ(context.statistics().d_successfulExecutionCount, 0);

        // async result *not* ready
        BMQTST_ASSERT(!result.isReady());

        sem1.post();  // allow the function object to continue execution
        sem2.wait();  // wait till the function object finish executing

        // make sure all postconditions on the context are established
        context.drain();

        // async result ready
        BMQTST_ASSERT(result.isReady());

        // function object invocation completed
        BMQTST_ASSERT(executed);
        BMQTST_ASSERT_EQ(context.statistics().d_successfulExecutionCount, 1);
    }

    // 2. Submitted function object returns a value.
    {
        // submit function object for execution
        bmqex::Future<ReturnValueOnCall::ResultType> result =
            bmqex::ExecutionUtil::execute(bmqex::ExecutionPolicyUtil::twoWay()
                                              .possiblyBlocking()
                                              .useExecutor(context.executor())
                                              .useAllocator(&alloc),
                                          ReturnValueOnCall());

        // retrieve the result
        BMQTST_ASSERT_EQ(result.get(), ReturnValueOnCall::k_VALUE);
    }

    // 3. Submitted function object returns a reference.
    {
        // submit function object for execution
        bmqex::Future<ReturnReferenceOnCall::ResultType> result =
            bmqex::ExecutionUtil::execute(bmqex::ExecutionPolicyUtil::twoWay()
                                              .possiblyBlocking()
                                              .useExecutor(context.executor())
                                              .useAllocator(&alloc),
                                          ReturnReferenceOnCall());

        // retrieve the result
        BMQTST_ASSERT_EQ(&result.get(), &ReturnReferenceOnCall::k_VALUE);
    }

    // 4. Submitted function object returns 'void'.
    {
        // submit function object for execution
        bmqex::Future<ReturnVoidOnCall::ResultType> result =
            bmqex::ExecutionUtil::execute(bmqex::ExecutionPolicyUtil::twoWay()
                                              .possiblyBlocking()
                                              .useExecutor(context.executor())
                                              .useAllocator(&alloc),
                                          ReturnVoidOnCall());

        // retrieve the result
        result.get();
        BMQTST_ASSERT(result.isReady());
    }

#ifdef BSLS_LIBRARYFEATURES_HAS_CPP11_EXCEPTION_HANDLING
    // 5. Submitted function object throws an exception.
    {
        // submit function object for execution
        bmqex::Future<ThrowOnCall::ResultType> result =
            bmqex::ExecutionUtil::execute(bmqex::ExecutionPolicyUtil::twoWay()
                                              .possiblyBlocking()
                                              .useExecutor(context.executor())
                                              .useAllocator(&alloc),
                                          ThrowOnCall());

        // retrieve the result
        bool exceptionThrown = false;
        try {
            result.get();
        }
        catch (const ThrowOnCall::ExceptionType&) {
            exceptionThrown = true;
        }

        BMQTST_ASSERT(exceptionThrown);
    }
#endif
}

static void test8_execute_two_way_always_blocking()
// ------------------------------------------------------------------------
// EXECUTE TWO WAY ALWAYS BLOCKING
//
// Concerns:
//   Ensure proper behavior of the 'execute' function accepting a Two-Way
//   Always Blocking execution policy.
//
// Plan:
//   1. Execute a function object 'f' on an executor 'ex' specifying a
//      Two-Way Always Blocking execution policy. Check that:
//:     o The execution function does block the calling thread pending
//:       completion of the submitted function object;
//:     o The function object is executed as if by a call to
//:       'ex.dispatch(f)';
//:     o The execution function returns a 'bmqex::Future' object that
//:       is ready.
//
//   2. Execute a function object 'f', according to a Two-Way Always
//      Blocking execution policy, and such that 'f()' returns a value.
//      Check that the execution function returns a ready 'bmqex::Future'
//      object that contains that value.
//
//   3. Execute a function object 'f', according to a Two-Way Always
//      Blocking execution policy, and such that 'f()' returns a reference.
//      Check that the execution function returns a ready 'bmqex::Future'
//      object that contains that reference.
//
//   4. Execute a function object 'f', according to a Two-Way Always
//      Blocking execution policy, and such that 'f()' returns 'void'.
//      Check that the execution function returns a ready 'bmqex::Future'
//      object.
//
//   5. Execute a function object 'f', according to a Two-Way Always
//      Blocking execution policy, and such that 'f()' throws an exception.
//      Check that the execution function returns a ready 'bmqex::Future'
//      object that contains that exception.
//
// Testing:
//   bmqex::ExecutionUtil::execute
// ------------------------------------------------------------------------
{
    bslma::TestAllocator alloc;
    TestExecutionContext context(&alloc);

    // 1. Genral use-case.
    {
        bool          executed = false;
        SetFlagOnCall toBeExecuted(&executed);

        // submit function object for execution
        bmqex::Future<void> result = bmqex::ExecutionUtil::execute(
            bmqex::ExecutionPolicyUtil::twoWay()
                .alwaysBlocking()
                .useExecutor(context.executor())
                .useAllocator(&alloc),
            toBeExecuted);

        // function object invocation completed
        BMQTST_ASSERT(executed);

        // function object was submitted for execution via a call to
        // 'dispatch()' an the specified executor
        BMQTST_ASSERT_EQ(context.statistics().d_dispatchCount, 1);

        // returned future is ready
        BMQTST_ASSERT(result.isReady());

        // make sure all postconditions on the context are established
        context.drain();

        // function object executed
        BMQTST_ASSERT_EQ(context.statistics().d_successfulExecutionCount, 1);
    }

    // 2. Submitted function object returns a value.
    {
        // submit function object for execution
        bmqex::Future<ReturnValueOnCall::ResultType> result =
            bmqex::ExecutionUtil::execute(bmqex::ExecutionPolicyUtil::twoWay()
                                              .alwaysBlocking()
                                              .useExecutor(context.executor())
                                              .useAllocator(&alloc),
                                          ReturnValueOnCall());

        // retrieve the result
        BMQTST_ASSERT_EQ(result.isReady(), true);
        BMQTST_ASSERT_EQ(result.get(), ReturnValueOnCall::k_VALUE);
    }

    // 3. Submitted function object returns a reference.
    {
        // submit function object for execution
        bmqex::Future<ReturnReferenceOnCall::ResultType> result =
            bmqex::ExecutionUtil::execute(bmqex::ExecutionPolicyUtil::twoWay()
                                              .alwaysBlocking()
                                              .useExecutor(context.executor())
                                              .useAllocator(&alloc),
                                          ReturnReferenceOnCall());

        // retrieve the result
        BMQTST_ASSERT_EQ(result.isReady(), true);
        BMQTST_ASSERT_EQ(&result.get(), &ReturnReferenceOnCall::k_VALUE);
    }

    // 4. Submitted function object returns 'void'.
    {
        // submit function object for execution
        bmqex::Future<ReturnVoidOnCall::ResultType> result =
            bmqex::ExecutionUtil::execute(bmqex::ExecutionPolicyUtil::twoWay()
                                              .alwaysBlocking()
                                              .useExecutor(context.executor())
                                              .useAllocator(&alloc),
                                          ReturnVoidOnCall());

        // retrieve the result
        BMQTST_ASSERT(result.isReady());
        result.get();
    }

#ifdef BSLS_LIBRARYFEATURES_HAS_CPP11_EXCEPTION_HANDLING
    // 5. Submitted function object throws an exception.
    {
        // submit function object for execution
        bmqex::Future<ThrowOnCall::ResultType> result =
            bmqex::ExecutionUtil::execute(bmqex::ExecutionPolicyUtil::twoWay()
                                              .alwaysBlocking()
                                              .useExecutor(context.executor())
                                              .useAllocator(&alloc),
                                          ThrowOnCall());

        // retrieve the result
        BMQTST_ASSERT(result.isReady());

        bool exceptionThrown = false;
        try {
            result.get();
        }
        catch (const ThrowOnCall::ExceptionType&) {
            exceptionThrown = true;
        }

        BMQTST_ASSERT(exceptionThrown);
    }
#endif
}

static void test9_thenExecute_one_way()
// ------------------------------------------------------------------------
// THEN EXECUTE ONE WAY
//
// Concerns:
//   Ensure proper behavior of the 'thenExecute' function accepting a
//   One-Way execution policy.
//
// Plan:
//   1. Attach a continuation to a non-ready 'Future' object specifying a
//      One-Way execution policy. Check that the continuation callback is
//      executed, according to the specified policy, when the future's
//      shared state becomes ready.
//
//   2. Attach a continuation to a non-ready 'Future' object specifying a
//      One-Way execution policy. Check that the lifetime of the shared
//      state associated with the 'Future' object is extended until the
//      end of the continuation callback execution.
//
// Testing:
//   bmqex::ExecutionUtil::thenExecute
// ------------------------------------------------------------------------
{
    bslma::TestAllocator alloc;
    TestExecutionContext context(&alloc);

    // 1. general use-case
    {
        // prepare a future
        bsl::shared_ptr<bmqex::FutureSharedState<int> > sharedState =
            bsl::allocate_shared<bmqex::FutureSharedState<int> >(&alloc);

        bmqex::Future<int> future(sharedState);
        BMQTST_ASSERT_EQ(future.isReady(), false);

        // attach a continuation
        int                       result = 0;
        SetValueContinuation<int> toBeExecuted(&result);

        bmqex::ExecutionUtil::thenExecute(bmqex::ExecutionPolicyUtil::oneWay()
                                              .alwaysBlocking()
                                              .useExecutor(context.executor())
                                              .useAllocator(&alloc),
                                          future,
                                          toBeExecuted);

        // continuation callback not invoked yet
        BMQTST_ASSERT_EQ(result, 0);

        // make the future ready
        sharedState->setValue(42);
        BMQTST_ASSERT_EQ(future.isReady(), true);

        // continuation callback invoked
        BMQTST_ASSERT_EQ(result, 42);

        // continuation callback was submitted for execution as if by a call to
        // 'bmqex::ExecutionUtil::execute' with the same policy as passed to
        // 'thenExecute'
        BMQTST_ASSERT_EQ(context.statistics().d_dispatchCount, 1);
    }

    // 2. shared state lifetime
    {
        // prepare a shared state
        bsl::shared_ptr<bmqex::FutureSharedState<int> > sharedState =
            bsl::allocate_shared<bmqex::FutureSharedState<int> >(&alloc);

        BMQTST_ASSERT_EQ(sharedState.use_count(), 1);

        // attach a continuation
        bmqex::ExecutionUtil::thenExecute(bmqex::ExecutionPolicyUtil::oneWay()
                                              .neverBlocking()
                                              .useExecutor(context.executor())
                                              .useAllocator(&alloc),
                                          bmqex::Future<int>(sharedState),
                                          NoOp());

        // the continuation holds a reference to the shared state
        BMQTST_ASSERT_EQ(sharedState.use_count(), 2);

        // execute the continuation
        sharedState->setValue(42);
        context.drain();

        // the reference to the shared state held by the continuation was
        // released
        BMQTST_ASSERT_EQ(sharedState.use_count(), 1);
    }
}

static void test10_thenExecute_two_way()
// ------------------------------------------------------------------------
// THEN EXECUTE TWO WAY
//
// Concerns:
//   Ensure proper behavior of the 'thenExecute' function accepting a
//   Two-Way execution policy.
//
// Plan:
//   1. Attach a continuation to a non-ready (input) 'Future' object
//      specifying a Two-Way execution policy. Obtain a (output) 'Future'
//      object to be containing the callback result. Check that:
//:     o The continuation callback is executed, according to the
//:       specified policy, when the input future's shared state becomes
//:       ready;
//:     o The output future contains the result of the callback invocation.
//
//   2. Attach a continuation to a non-ready 'Future' object specifying a
//      Two-Way execution policy. Check that the lifetime of the shared
//      state associated with the 'Future' object is extended until the
//      end of the continuation callback execution.
//
// Testing:
//   bmqex::ExecutionUtil::thenExecute
// ------------------------------------------------------------------------
{
    bslma::TestAllocator alloc;
    TestExecutionContext context(&alloc);

    // 1. general use-case
    {
        // prepare the input future
        bsl::shared_ptr<bmqex::FutureSharedState<int> > sharedState =
            bsl::allocate_shared<bmqex::FutureSharedState<int> >(&alloc);

        bmqex::Future<int> future1(sharedState);
        BMQTST_ASSERT_EQ(future1.isReady(), false);

        // attach a continuation, obtain the output future
        ForwardValueContinuation<int> toBeExecuted;

        bmqex::Future<int> future2 = bmqex::ExecutionUtil::thenExecute(
            bmqex::ExecutionPolicyUtil::twoWay()
                .alwaysBlocking()
                .useExecutor(context.executor())
                .useAllocator(&alloc),
            future1,
            toBeExecuted);

        // make the future ready
        sharedState->setValue(42);
        BMQTST_ASSERT_EQ(future1.isReady(), true);

        // continuation callback was submitted for execution as if by a call to
        // 'bmqex::ExecutionUtil::execute' with the same policy as passed to
        // 'thenExecute'
        BMQTST_ASSERT_EQ(context.statistics().d_dispatchCount, 1);

        // the output future contains the callback result
        BMQTST_ASSERT_EQ(future2.isReady(), true);
        BMQTST_ASSERT_EQ(future2.get(), 42);
    }

    // 2. shared state lifetime
    {
        // prepare a shared state
        bsl::shared_ptr<bmqex::FutureSharedState<int> > sharedState =
            bsl::allocate_shared<bmqex::FutureSharedState<int> >(&alloc);

        BMQTST_ASSERT_EQ(sharedState.use_count(), 1);

        // attach a continuation
        bmqex::ExecutionUtil::thenExecute(bmqex::ExecutionPolicyUtil::twoWay()
                                              .neverBlocking()
                                              .useExecutor(context.executor())
                                              .useAllocator(&alloc),
                                          bmqex::Future<int>(sharedState),
                                          NoOp());

        // the continuation holds a reference to the shared state
        BMQTST_ASSERT_EQ(sharedState.use_count(), 2);

        // execute the continuation
        sharedState->setValue(42);
        context.drain();

        // the reference to the shared state held by the continuation was
        // released
        BMQTST_ASSERT_EQ(sharedState.use_count(), 1);
    }
}

static void test11_invoke()
// ------------------------------------------------------------------------
// INVOKE
//
// Concerns:
//   Ensure proper behavior of the 'invoke' function.
//
// Plan:
//   1. Execute a function object 'f' on an executor 'ex'. Check that:
//:     o The execution function does block the calling thread pending
//:       completion of the submitted function object;
//:     o The function object is executed as if by a call to
//:       'ex.dispatch(f)';
//
//   2. Execute a function object 'f', such that 'f()' returns a value.
//      Check that the execution function returns that value.
//
//   3. Execute a function object 'f', such that 'f()' returns a reference.
//      Check that the execution function returns that reference.
//
//   4. Execute a function object 'f', such that 'f()' returns 'void'.
//
//   5. Execute a function object 'f', such that 'f()' throws an exception.
//      Check that the execution function (re)throws that exception.
//
// Testing:
//   bmqex::ExecutionUtil::invoke
// ------------------------------------------------------------------------
{
    bslma::TestAllocator alloc;
    TestExecutionContext context(&alloc);

    // 1. General use-case
    {
        bool          executed = false;
        SetFlagOnCall toBeExecuted(&executed);

        // submit function object for execution
        bmqex::ExecutionUtil::invoke(context.executor(), toBeExecuted);

        // function object was submitted for execution via a call to
        // 'dispatch()' an the specified executor
        BMQTST_ASSERT_EQ(context.statistics().d_dispatchCount, 1);

        // function object invocation completed
        BMQTST_ASSERT(executed);

        // make sure all postconditions on the context are established
        context.drain();

        // function object executed
        BMQTST_ASSERT_EQ(context.statistics().d_successfulExecutionCount, 1);
    }

    // 2. Submitted function object returns a value.
    {
        // submit function object for execution
        ReturnValueOnCall::ResultType result = bmqex::ExecutionUtil::invoke(
            context.executor(),
            ReturnValueOnCall());

        // chech the result
        BMQTST_ASSERT_EQ(result, ReturnValueOnCall::k_VALUE);
    }

    // 3. Submitted function object returns a reference.
    {
        // submit function object for execution
        ReturnReferenceOnCall::ResultType result =
            bmqex::ExecutionUtil::invoke(context.executor(),
                                         ReturnReferenceOnCall());

        // retrieve the result
        BMQTST_ASSERT_EQ(&result, &ReturnReferenceOnCall::k_VALUE);
    }

    // 4. Submitted function object returns 'void'.
    {
        // submit function object for execution
        bmqex::ExecutionUtil::invoke(context.executor(), ReturnVoidOnCall());
    }

#ifdef BSLS_LIBRARYFEATURES_HAS_CPP11_EXCEPTION_HANDLING
    // 5. Submitted function object throws an exception.
    {
        // execute the function object and retrieve the result
        bool exceptionThrown = false;
        try {
            bmqex::ExecutionUtil::invoke(context.executor(), ThrowOnCall());
        }
        catch (const ThrowOnCall::ExceptionType&) {
            exceptionThrown = true;
        }

        BMQTST_ASSERT(exceptionThrown);
    }
#endif
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
    case 2: test2_thenExecuteResult(); break;

    // One-Way execution
    case 3: test3_execute_one_way_never_blocking(); break;
    case 4: test4_execute_one_way_possibly_blocking(); break;
    case 5: test5_execute_one_way_always_blocking(); break;

    // Two-Way execution
    case 6: test6_execute_two_way_never_blocking(); break;
    case 7: test7_execute_two_way_possibly_blocking(); break;
    case 8: test8_execute_two_way_always_blocking(); break;

    // Continuation execution
    case 9: test9_thenExecute_one_way(); break;
    case 10: test10_thenExecute_two_way(); break;

    // invoke
    case 11: test11_invoke(); break;

    default: {
        bsl::cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND."
                  << bsl::endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_GBL_ALLOC);
}
