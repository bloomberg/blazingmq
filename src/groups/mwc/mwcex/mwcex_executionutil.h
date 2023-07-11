// Copyright 2019-2023 Bloomberg Finance L.P.
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

// mwcex_executionutil.h                                              -*-C++-*-
#ifndef INCLUDED_MWCEX_EXECUTIONUTIL
#define INCLUDED_MWCEX_EXECUTIONUTIL

//@PURPOSE: Provides utility functions to execute function objects.
//
//@CLASSES:
//  mwcex::ExecutionUtil: a namespace for utility functions to execute functors
//
//@SEE ALSO:
//  mwcex_executionpolicy
//
//@DESCRIPTION:
// This component provides a struct, 'mwcex::ExecutionUtil', that serves as a
// namespace for utility functions to execute function objects of arbitrary
// (template parameter) type according to a set of rules defined by a specific
// execution policy (see 'mwcex_executionpolicy').
//
// Those utility functions are commonly referred as execution functions. There
// are two types of them:
//: o Execution functions for immediate execution;
//: o Execution functions for deferred execution.
//
/// Execution functions for immediate execution
///-------------------------------------------
// Those are functions, all named 'mwcex::ExecutionUtil::execute', to initiate
// immediate execution of function objects. Function objects executed should
// not take any argument, and may or may not return a value.
//..
//  // a function object to be executed
//  auto myFunction = []() -> int
//                    {
//                        bsl::cout << "It Works!\n";
//                        return 42;
//                    };
//..
// 'execute' takes an execution policy (see 'mwcex_executionpolicy') specifying
// how the function object is to be executed, and the function object itself.
//
// Below are several usage examples of 'mwcex::ExecutionUtil::execute' for
// different use cases.
//
// 1. All we want is the function object to be executed. Don't want the result,
//    don't want to wait for the function object completion. That use-case is
//    commonly referred as "fire and forget". For that we may use a One-Way
//    Never Blocking execution policy.
//..
//  using mwcex;
//
//  // initiate the execution and "forget" about it
//  ExecutionUtil::execute(ExecutionPolicyUtil::oneWay()
//                                             .neverBlocking(),
//                         myFunction);
//..
// 2. We want to execute a function object and don't want to wait for its
//    completion, but still want to retrieve the invocation result. Lets use a
//    Two-Way Never Blocking execution policy.
//..
//  using mwcex;
//
//  // initiate the execution, obtain a future
//  Future<int> future = ExecutionUtil::execute(
//                                         ExecutionPolicyUtil::twoWay()
//                                                             .neverBlocking()
//                                         myFunction);
//
//  // do something else
//  // ...
//
//  // retrieve the result
//  int result = result.get();
//  BSLS_ASSERT(result == 42);
//..
// There are more exotic uses cases. For example, we may want to execute a
// function object and wait for its completion, but don't want the invocation
// result. In that case, we may use a One-Way Always blocking execution
// policy.
//
/// Execution functions for deferred execution
///------------------------------------------
// Those are functions, all named 'mwcex::ExecutionUtil::thenExecute', to
// execute continuation callbacks. A continuation callback being a function
// object that is executed after the completion of a previously initiated
// asynchronous operation and is passed the result of that operation encoded in
// a 'mwcex::FutureResult' object. The callback itself may or may not return a
// value.
//..
//  // a continuation callback to be executed
//  auto myCallback = [](mwcex::FutureResult<int> result) -> bsl::string
//                    {
//                        bsl::cout << result.get() << "\n";
//                        return bsl::to_string(result.get());
//                    };
//..
//
// Like 'execute', 'thenExecute' takes an execution policy and a function
// object (the continuation callback), but also a 'mwcex::Future' object
// representing the asynchronous operation after the completion of which the
// callback is to be executed.
//..
//  using mwcex;
//
//  // initiate an async operation, obtain an associated future
//  Future<int> future = doAsyncStuff();
//
//  // execute 'myCallback' when 'future' becomes ready
//  ExecutionUtil::thenExecute(ExecutionPolicyUtil::oneWay()
//                                                 .neverBlocking(),
//                             future,
//                             myCallback);
//..
// In the example below we attach a continuation callback to a future object,
// making it to be executed after the future becomes ready. As we use a One-Way
// execution policy, the callback result is discarded. If we want to obtain the
// callback invocation result, we have to use a Two-Way policy.
//..
//  using mwcex;
//
//  // initiate an async operation, obtain an associated future
//  Future<int> future1 = doAsyncStuff();
//
//  // execute 'myCallback' when 'future1' becomes ready and obtain the result
//  Future<bsl::string> future2 =
//             ExecutionUtil::thenExecute(ExecutionPolicyUtil::twoWay()
//                                                            .neverBlocking(),
//                                        future1,
//                                        myCallback);
//..
//
///'mwcex::ExecutionUtil::invoke'
///------------------------------
// 'mwcex::ExecutionUtil::invoke' is a special execution function that is
// functionally equivalent to a Two-Way Always Blocking execution function,
// except that it returns the invocation result directly, rather than via a
// 'Future' object. That optimization allows to avoid unnecessary memory
// allocations.
//..
//  using mwcex;
//
//  // initiate the execution, wait for its completion, and get the result
//  int result = ExecutionUtil::invoke(SystemExecutor(), myFunction);
//
//  BSLS_ASSERT(result == 42);
//..

// MWC

#include <mwcex_executionproperty.h>
#include <mwcex_executortraits.h>
#include <mwcex_future.h>

// BDE
#include <bdlb_nullablevalue.h>
#include <bdlf_memfn.h>
#include <bsl_memory.h>
#include <bslalg_constructorproxy.h>
#include <bslma_allocator.h>
#include <bslma_default.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_assert.h>
#include <bslmf_decay.h>
#include <bslmf_invokeresult.h>
#include <bslmf_isvoid.h>
#include <bslmf_movableref.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bslmf_nil.h>
#include <bslmf_removeconst.h>
#include <bslmf_removereference.h>
#include <bslmf_util.h>
#include <bslmt_latch.h>
#include <bsls_annotation.h>
#include <bsls_assert.h>
#include <bsls_compilerfeatures.h>
#include <bsls_exceptionutil.h>  // BSLS_NOTHROW_SPEC
#include <bsls_keyword.h>
#include <bsls_libraryfeatures.h>
#include <bsls_spinlock.h>

namespace BloombergLP {

namespace mwcex {

// FORWARD DECLARATION
template <class, class>
class ExecutionPolicy;

// ==================================
// struct ExecutionUtil_ExecuteResult
// ==================================

/// Provides a metafunction that determines, at compile time, the type
/// returned by `mwcex::ExecutionUtil::execute`.
template <class POLICY, class FUNCTION>
struct ExecutionUtil_ExecuteResult {};

/// Provides a specialization of `ExecutionUtil_ExecuteResult` for One-Way
/// policies.
template <class EXECUTOR, class FUNCTION>
struct ExecutionUtil_ExecuteResult<
    ExecutionPolicy<ExecutionProperty::OneWay, EXECUTOR>,
    FUNCTION> {
    // TYPES
    typedef void Type;
};

/// Provides a specialization of `ExecutionUtil_ExecuteResult` for Two-Way
/// policies with no encoded result type.
template <class EXECUTOR, class FUNCTION>
struct ExecutionUtil_ExecuteResult<
    ExecutionPolicy<ExecutionProperty::TwoWay, EXECUTOR>,
    FUNCTION> {
    // TYPES
#ifdef BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES
    typedef Future<typename bsl::invoke_result<
        typename bsl::decay<FUNCTION>::type&&>::type>
#else
    typedef Future<
        typename bsl::invoke_result<typename bsl::decay<FUNCTION>::type>::type>
#endif
        Type;
};

/// Provides a specialization of `ExecutionUtil_ExecuteResult` for Two-Way
/// policies with an encoded result type.
template <class R, class EXECUTOR, class FUNCTION>
struct ExecutionUtil_ExecuteResult<
    ExecutionPolicy<ExecutionProperty::TwoWayR<R>, EXECUTOR>,
    FUNCTION> {
    // TYPES
    typedef Future<R> Type;
};

// ======================================
// struct ExecutionUtil_ThenExecuteResult
// ======================================

/// Provides a metafunction that determines, at compile time, the type
/// returned by `mwcex::ExecutionUtil::thenExecute`.
template <class POLICY, class FUTURE, class FUNCTION>
struct ExecutionUtil_ThenExecuteResult {};

/// Provides a specialization of `ExecutionUtil_ThenExecuteResult` for
/// One-Way policies.
template <class EXECUTOR, class FUTURE, class FUNCTION>
struct ExecutionUtil_ThenExecuteResult<
    ExecutionPolicy<ExecutionProperty::OneWay, EXECUTOR>,
    FUTURE,
    FUNCTION> {
    // TYPES
    typedef void Type;
};

/// Provides a specialization of `ExecutionUtil_ThenExecuteResult` for
/// Two-Way policies with no encoded result type.
template <class EXECUTOR, class R, class FUNCTION>
struct ExecutionUtil_ThenExecuteResult<
    ExecutionPolicy<ExecutionProperty::TwoWay, EXECUTOR>,
    Future<R>,
    FUNCTION> {
    // TYPES
#ifdef BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES
    typedef Future<
        typename bsl::invoke_result<typename bsl::decay<FUNCTION>::type&&,
                                    FutureResult<R> >::type>
#else
    typedef Future<
        typename bsl::invoke_result<typename bsl::decay<FUNCTION>::type,
                                    FutureResult<R> >::type>
#endif
        Type;
};

/// Provides a specialization of `ExecutionUtil_ThenExecuteResult` for
/// Two-Way policies with an encoded result type.
template <class R, class EXECUTOR, class FUTURE, class FUNCTION>
struct ExecutionUtil_ThenExecuteResult<
    ExecutionPolicy<ExecutionProperty::TwoWayR<R>, EXECUTOR>,
    FUTURE,
    FUNCTION> {
    // TYPES
    typedef Future<R> Type;
};

// ===============================
// class ExecutionUtil_FunctionRef
// ===============================

/// Provides a reference wrapper for nullary function objects preserving the
/// l-value/r-value reference type of the object, as well as other type
/// modifiers.
///
/// `R` must satisfy one of the following requirements:
/// * be `void`;
/// * be a reference type, such that `bsl::invoke_result_t<F>` is
///   implicitly convertible to `R`;
/// * satisfy the requirement of Destructible and MoveConstructible as
///   specified in the C++ standard, and be constructible from
///   `bsl::invoke_result_t<F>`.
///
/// Given an object `f` of type `F`, `f()` shall be a valid expression.
template <class R, class F>
class ExecutionUtil_FunctionRef {
  public:
    // TYPES

    /// Defines the result type of the call operator.
    typedef R ResultType;

  private:
    // PRIVATE DATA

    // Pointer to the referred-to function object.
    typename bsl::remove_reference<F>::type* d_function_p;

  private:
    // PRIVATE ACCESSORS
    void invoke(bsl::true_type voidResultTag) const;
    R    invoke(bsl::false_type voidResultTag) const;

  public:
    // CREATORS

    /// Create a `ExecutionUtil_FunctionRef` containing the specified
    /// `function` pointer.
    explicit BSLS_KEYWORD_CONSTEXPR_CPP14 ExecutionUtil_FunctionRef(
        typename bsl::remove_reference<F>::type* function)
        BSLS_KEYWORD_NOEXCEPT;

  public:
    // ACCESSORS

    /// Invoke the referred-to function object `f` as if by
    /// `bsl::forward<F>(f)()`, and  return the result of that invocation,
    /// or, if `R` is `void`, discard the result.
    R operator()() const;
};

// ==================================
// class ExecutionUtil_OneOffFunction
// ==================================

/// Provides a wrapper for nullary function objects destroying the object
/// immediately after invocation.
///
/// `R` must satisfy one of the following requirements:
/// * be `void`;
/// * be a reference type, such that
///   `bsl::invoke_result_t<bsl::decay_t<F>&&>` is implicitly convertible
///   to `R`;
/// * satisfy the requirement of Destructible and MoveConstructible as
///   specified in the C++ standard, and be constructible from
///   `bsl::invoke_result_t<bsl::decay_t<F>&&>`.
///
/// `F` must meet the requirements of Destructible as specified in the C++
/// standard. Given an object `f` of type `F`, `f()` shall be a valid
/// expression.
template <class R, class F>
class ExecutionUtil_OneOffFunction {
  public:
    // TYPES

    /// Defines the result type of the call operator.
    typedef R ResultType;

  private:
    // PRIVATE TYPES
    typedef typename bsl::remove_const<F>::type FunctionType;

    typedef bslalg::ConstructorProxy<bdlb::NullableValue<FunctionType> >
        FunctionValueType;

    /// Provides a utility object that destroys the stored function object
    /// on destruction.
    struct DestroyFunctionOnScopeExit {
        // DATA
        FunctionValueType* d_functionValue_p;

        // CREATORS

        /// Create a `DestroyFunctionOnScopeExit` referring to the specified
        /// `functionValue`.
        explicit BSLS_KEYWORD_CONSTEXPR_CPP14 DestroyFunctionOnScopeExit(
            FunctionValueType* functionValue) BSLS_KEYWORD_NOEXCEPT;

        /// Destroy the referred to function object.
        ~DestroyFunctionOnScopeExit();
    };

  private:
    // PRIVATE DATA

    // Function object to be invoked. Destroyed immediately after
    // invocation.
    FunctionValueType d_functionValue;

  private:
    // PRIVATE ACCESSORS
    void invoke(bsl::true_type voidResultTag);
    R    invoke(bsl::false_type voidResultTag);

  private:
    // NOT IMPLEMENTED
    ExecutionUtil_OneOffFunction(const ExecutionUtil_OneOffFunction&)
        BSLS_KEYWORD_DELETED;
    ExecutionUtil_OneOffFunction&
    operator=(const ExecutionUtil_OneOffFunction&) BSLS_KEYWORD_DELETED;

  public:
    // CREATORS

    /// Create a `ExecutionUtil_OneOffFunction` object containing an object
    /// of type `F` direct-non-list-initialized with
    /// `bsl::forward<F_PARAM>(f)`. Specify an `allocator` used to supply
    /// memory.
    ///
    /// `F` must be constructible from `bsl::forward<F_PARAM>(f)`.
    template <class F_PARAM>
    BSLS_KEYWORD_CONSTEXPR_CPP14
    ExecutionUtil_OneOffFunction(BSLS_COMPILERFEATURES_FORWARD_REF(F_PARAM) f,
                                 bslma::Allocator* allocator);

  public:
    // MANIPULATORS

    /// Invoke the contained function object `f` as if by `bsl::move(f)()`,
    /// and return the result of that invocation, or, if `R` is `void`,
    /// discard the result. Destroy the contained function object
    /// immediately after invocation. The behavior is undefined if this
    /// function is invoked more than once.
    R operator()();
};

// ====================================
// class ExecutionUtil_UniqueOneWayTask
// ====================================

/// Provides a wrapper for a function object allowing to synchronize with
/// the completion of the function object invocation.
///
/// `F` must meet the requirements of Destructible as specified in the C++
/// standard. Given an object `f` of type `F`, `f()` shall be a valid
/// expression.
template <class F>
class ExecutionUtil_UniqueOneWayTask {
  public:
    // TYPES

    /// Defines the result type of the call operator.
    typedef void ResultType;

  private:
    // PRIVATE TYPES
    typedef ExecutionUtil_OneOffFunction<void, F> FunctionType;

  private:
    // PRIVATE DATA

    // Used to synchronize with the invocation of the function object.
    mutable bslmt::Latch d_latch;

    // Function object to be invoked. Destroyed immediately after
    // invocation.
    FunctionType d_function;

  private:
    // NOT IMPLEMENTED
    ExecutionUtil_UniqueOneWayTask(const ExecutionUtil_UniqueOneWayTask&)
        BSLS_KEYWORD_DELETED;
    ExecutionUtil_UniqueOneWayTask&
    operator=(const ExecutionUtil_UniqueOneWayTask&) BSLS_KEYWORD_DELETED;

  public:
    // CREATORS

    /// Create a `ExecutionUtil_UniqueOneWayTask` object containing an
    /// object of type `F` direct-non-list-initialized with
    /// `bsl::forward<F_PARAM>(f)`. Specify an `allocator` used to supply
    /// memory.
    ///
    /// `F` must be constructible from `bsl::forward<F_PARAM>(f)`.
    template <class F_PARAM>
    ExecutionUtil_UniqueOneWayTask(BSLS_COMPILERFEATURES_FORWARD_REF(F_PARAM)
                                       f,
                                   bslma::Allocator* allocator);

  public:
    // MANIPULATORS

    /// Invoke the contained function object `f` as if by `bsl::move(f)()`.
    /// Then, destroy the contained function object and unblock all threads
    /// waiting on `wait`. Propagate any exception thrown by `f` to the
    /// caller of this function. The behavior is undefined if this function
    /// is invoked more than once.
    ///
    /// The completion of this function synchronizes with the completion of
    /// any subsequent call to `wait`.
    void operator()();

  public:
    // ACCESSORS

    /// Block the calling thread pending completion of the contained
    /// function object invocation.
    void wait() const BSLS_KEYWORD_NOEXCEPT;
};

// ====================================
// class ExecutionUtil_UniqueTwoWayTask
// ====================================

/// Provides a wrapper for a function object allowing to synchronize with
/// the completion of the function object invocation and retrieve its
/// result. Holds a shared state used to store the result of the function
/// object invocation.
///
/// `R` must satisfy one of the following requirements:
/// * be `void`;
/// * be a reference type, such that
///   `bsl::invoke_result_t<bsl::decay_t<F>&&>` is implicitly convertible
///   to `R`;
/// * satisfy the requirement of Destructible and MoveConstructible as
///   specified in the C++ standard, and be constructible from
///   `bsl::invoke_result_t<bsl::decay_t<F>&&>`.
///
/// `F` must meet the requirements of Destructible as specified in the C++
/// standard. Given an object `f` of type `F`, `f()` shall be a valid
/// expression.
template <class R, class F>
class ExecutionUtil_UniqueTwoWayTask {
  public:
    // TYPES

    /// Defines the result type of the call operator.
    typedef void ResultType;

  private:
    // PRIVATE TYPES
    typedef ExecutionUtil_OneOffFunction<R, F>  FunctionType;
    typedef typename Future<R>::SharedStateType SharedStateType;

    /// Provides a synchronization utility object that locks and then
    /// immediately unlocks a spinlock on destruction.
    ///
    /// This utility is used to synchronize the completion of `get` with
    /// the completion of the call operator while still benefiting from the
    /// RVO optimization in `get`.
    struct SynchronizeOnScopeExit {
        // DATA
        bsls::SpinLock* d_spinlock_p;

        // CREATORS

        /// Create a `SynchronizeOnScopeExit` object with the specified
        /// `spinlock`.
        explicit BSLS_KEYWORD_CONSTEXPR_CPP14
        SynchronizeOnScopeExit(bsls::SpinLock* spinlock) BSLS_KEYWORD_NOEXCEPT;

        /// Lock and unlock the associated spinlock.
        ~SynchronizeOnScopeExit();
    };

  private:
    // PRIVATE DATA

    // Used to synchronize the completion of `get` with the completion of
    // the call operator.
    bsls::SpinLock d_syncLock;

    // Stores the result of the function object invocation.
    SharedStateType d_sharedState;

    // Function object to be invoked. Destroyed immediately after
    // invocation.
    FunctionType d_function;

  private:
    // NOT IMPLEMENTED
    ExecutionUtil_UniqueTwoWayTask(const ExecutionUtil_UniqueTwoWayTask&)
        BSLS_KEYWORD_DELETED;
    ExecutionUtil_UniqueTwoWayTask&
    operator=(const ExecutionUtil_UniqueTwoWayTask&) BSLS_KEYWORD_DELETED;

  private:
    // PRIVATE MANIPULATORS
    bslmf::Nil doInvoke(bsl::true_type voidResultTag);
    R          doInvoke(bsl::false_type voidResultTag);

    void doGet(bsl::true_type voidResultTag);
    R    doGet(bsl::false_type voidResultTag);

  public:
    // CREATORS

    /// Create a `ExecutionUtil_UniqueTwoWayTask` object containing an
    /// object of type `F` direct-non-list-initialized with
    /// `bsl::forward<F_PARAM>(f)`. Specify an `allocator` used to supply
    /// memory.
    ///
    /// `F` must be constructible from `bsl::forward<F_PARAM>(f)`.
    template <class F_PARAM>
    ExecutionUtil_UniqueTwoWayTask(BSLS_COMPILERFEATURES_FORWARD_REF(F_PARAM)
                                       f,
                                   bslma::Allocator* allocator);

  public:
    // MANIPULATORS

    /// If `R` is `void`, invoke the contained function object `f` as if by
    /// `bsl::move(f)()` and make the contained shared state ready.
    /// Otherwise, invoke the contained function object `f` as if by
    /// `bsl::move(f)()` and make the shared state ready as if by direct-
    /// non-list-initializing a value of type `R` with the result of that
    /// call. If the invocation of `f` throws, store the thrown exception in
    /// the share state. Then, destroy the contained function object. The
    /// behavior is undefined if this function is invoked more than once.
    ///
    /// The completion of this function synchronizes with the completion of
    /// any subsequent call to `get`.
    ///
    /// Note that in C++03 any exception thrown as the result of the
    /// function object invocation results in a call to `bsl::terminate`.
    void operator()() BSLS_NOTHROW_SPEC;

    /// Block the calling thread until the contained shared state is ready.
    /// Then, if the shared state contains a value `v`, return the contained
    /// value as if by `bsl::move(v)`. Otherwise, if the shared state
    /// contains a reference, return the contained reference. Otherwise, if
    /// the shared state contains an exception, throw the contained
    /// exception.
    R get();
};

// ====================================
// class ExecutionUtil_SharedTwoWayTask
// ====================================

/// Provides a wrapper for a function object allowing to synchronize with
/// the completion of the function object invocation and retrieve its
/// result. Holds a shared state used to store the result of the function
/// object invocation. Copies of this object could be made, each referring
/// to the same shared state and function object.
///
/// `R` must satisfy one of the following requirements:
/// * be `void`;
/// * be a reference type, such that
///   `bsl::invoke_result_t<bsl::decay_t<F>&&>` is implicitly convertible
///   to `R`;
/// * satisfy the requirement of Destructible and MoveConstructible as
///   specified in the C++ standard, and be constructible from
///   `bsl::invoke_result_t<bsl::decay_t<F>&&>`.
///
/// `F` must meet the requirements of Destructible as specified in the C++
/// standard. Given an object `f` of type `F`, `f()` shall be a valid
/// expression.
template <class R, class F>
class ExecutionUtil_SharedTwoWayTask {
  public:
    // TYPES

    /// Defines the result type of the call operator.
    typedef void ResultType;

  private:
    // PRIVATE TYPES
    typedef ExecutionUtil_OneOffFunction<R, F>  FunctionType;
    typedef typename Future<R>::SharedStateType SharedStateType;

  private:
    // PRIVATE TYPES

    /// Holds the shared state and function object.
    struct State {
        // DATA

        /// Stores the result of the function object invocation.
        SharedStateType d_sharedState;

        /// Function object to be invoked. Destroyed immediately after
        /// invocation.
        FunctionType d_function;

        // CREATORS
        template <class F_PARAM>
        State(BSLS_COMPILERFEATURES_FORWARD_REF(F_PARAM) f,
              bslma::Allocator* allocator);
    };

  private:
    // PRIVATE DATA
    bsl::shared_ptr<State> d_state;

  private:
    // PRIVATE MANIPULATORS
    bslmf::Nil doInvoke(bsl::true_type voidResultTag);
    R          doInvoke(bsl::false_type voidResultTag);

  public:
    // CREATORS

    /// Create a `ExecutionUtil_SharedTwoWayTask` object containing an
    /// object of type `F` direct-non-list-initialized with
    /// `bsl::forward<F_PARAM>(f)`. Specify an `allocator` used to supply
    /// memory.
    ///
    /// `F` must be constructible from `bsl::forward<F_PARAM>(f)`.
    template <class F_PARAM>
    ExecutionUtil_SharedTwoWayTask(BSLS_COMPILERFEATURES_FORWARD_REF(F_PARAM)
                                       f,
                                   bslma::Allocator* allocator);

  public:
    // MANIPULATORS

    /// If `R` is `void`, invoke the contained function object `f` as if by
    /// `bsl::move(f)()` and make the contained shared state ready.
    /// Otherwise, invoke the contained function object `f` as if by
    /// `bsl::move(f)()` and make the shared state ready as if by direct-
    /// non-list-initializing a value of type `R` with the result of that
    /// call. If the invocation of `f` throws, store the thrown exception in
    /// the share state. Then, destroy the contained function object. The
    /// behavior is undefined if this function is invoked more than once.
    ///
    /// Note that in C++03 any exception thrown as the result of the
    /// function object invocation results in a call to `bsl::terminate`.
    void operator()() BSLS_NOTHROW_SPEC;

  public:
    // ACCESSORS

    /// Return a `Future` object associated with the shared state owned by
    /// `*this`.
    Future<R> future() const BSLS_KEYWORD_NOEXCEPT;
};

// ===============================================
// class ExecutionUtil_ContinuationCallbackWrapper
// ===============================================

/// Provides a wrapper around a continuation callback holding the
/// callback itself and an associated `Future` object, passed to the
/// held callback on this object's invocation.
///
/// `R1` must be such as `Future<R1>` is a valid type.
///
/// `R2` must satisfy one of the following requirements:
/// * be `void`;
/// * be a reference type, such that
///   `bsl::invoke_result_t<bsl::decay_t<F>&&, FutureResult<R1>>` is
///   implicitly convertible to `R2`;
/// * satisfy the requirement of Destructible and MoveConstructible as
///   specified in the C++ standard, and be constructible from
///   `bsl::invoke_result_t<bsl::decay_t<F>&&, FutureResult<R1>>`.
///
/// `F` must meet the requirements of Destructible and MoveConstructible as
/// specified in the C++ standard. Given an object `f` of type `F`,
/// `f(bsl::declval<FutureResult<R1>>())` shall be a valid expression.
template <class R1, class R2, class F>
class ExecutionUtil_ContinuationCallbackWrapper {
  private:
    // PRIVATE TYPES
    typedef typename bsl::remove_const<F>::type FunctionType;

  private:
    // PRIVATE DATA
    Future<R1> d_future;

    bslalg::ConstructorProxy<FunctionType> d_function;

  public:
    // CREATORS

    /// Create a `ExecutionUtil_ContinuationCallbackWrapper` object
    /// containing an object of type `Future<R1>`
    /// direct-non-list-initialized with `future` and an object of type `F`
    /// direct-non-list-initialized with `bsl::forward<F_PARAM>(function)`.
    /// Specify an `allocator` used to supply memory. The behavior is
    /// undefined unless `future.isValid()` is `true`.
    template <class F_PARAM>
    ExecutionUtil_ContinuationCallbackWrapper(
        const Future<R1>& future,
        BSLS_COMPILERFEATURES_FORWARD_REF(F_PARAM) function,
        bslma::Allocator* allocator);

    /// Create a `ExecutionUtil_ContinuationCallbackWrapper` object having
    /// the same state as the specified `original` object. Optionally
    /// specify an `allocator` used to supply memory. If `allocator` is 0,
    /// the default memory allocator is used.
    ExecutionUtil_ContinuationCallbackWrapper(
        const ExecutionUtil_ContinuationCallbackWrapper& original,
        bslma::Allocator*                                allocator = 0);

    /// Create a `ExecutionUtil_ContinuationCallbackWrapper` object having
    /// the same state as the specified `original` object, leaving
    /// `original` in an unspecified state. Optionally specify an
    /// `allocator` used to supply memory. If `allocator` is 0, the default
    /// memory allocator is used.
    ExecutionUtil_ContinuationCallbackWrapper(
        bslmf::MovableRef<ExecutionUtil_ContinuationCallbackWrapper> original,
        bslma::Allocator* allocator = 0);

  public:
    // MANIPULATORS

    /// Invoke the contained function object `fn` as if by
    /// `bsl::move(fn)(FutureResult<R1>(ft))`, where `ft` is the contained
    /// `Future` object. Return the result of that invocation, or, if `R2`
    /// is `void`, discard the result. The behavior is undefined unless
    /// `ft.isReady()` is `true`.
    R2 operator()();

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(ExecutionUtil_ContinuationCallbackWrapper,
                                   bslma::UsesBslmaAllocator)
};

// ======================================
// class ExecutionUtil_OneWayContinuation
// ======================================

/// Provides a wrapper for a continuation callback. Copies of this object
/// could be made, each creating a new copy of the contained callback
/// object.
///
/// `P` must be a One-Way execution policy type (see
/// `mwcex_executionpolicy`).
///
/// `R` must be such as `Future<R>` is a valid type.
///
/// `F` must meet the requirements of Destructible and MoveConstructible as
/// specified in the C++ standard. Given an object `f` of type `F`,
/// `f(bsl::declval<FutureResult<R>>())` shall be a valid expression.
template <class P, class R, class F>
class ExecutionUtil_OneWayContinuation {
  private:
    // PRIVATE TYPES
    typedef ExecutionUtil_ContinuationCallbackWrapper<R, void, F> Callback;

  private:
    // PRIVATE DATA
    P d_policy;

    Callback d_callback;

  public:
    // CREATORS

    /// Create a `ExecutionUtil_OneWayContinuation` object containing an
    /// object of type `P` direct-non-list-initialized with `policy`, an
    /// object of type `Future<R>` direct-non-list-initialized with `future`
    /// and an object of type `F` direct-non-list-initialized with
    /// `bsl::forward<F_PARAM>(function)`. Specify an `allocator` used to
    /// supply memory. The behavior is undefined unless `future.isValid()`
    /// is `true`.
    template <class F_PARAM>
    ExecutionUtil_OneWayContinuation(const P&         policy,
                                     const Future<R>& future,
                                     BSLS_COMPILERFEATURES_FORWARD_REF(F_PARAM)
                                         function,
                                     bslma::Allocator* allocator);

    /// Create a `ExecutionUtil_OneWayContinuation` object having the same
    /// state as the specified `original` object. Optionally specify an
    /// `allocator` used to supply memory. If `allocator` is 0, the default
    /// memory allocator is used.
    ExecutionUtil_OneWayContinuation(
        const ExecutionUtil_OneWayContinuation& original,
        bslma::Allocator*                       allocator = 0);

    /// Create a `ExecutionUtil_OneWayContinuation` object having the same
    /// state as the specified `original` object, leaving `original` in an
    /// unspecified state. Optionally specify an `allocator` used to supply
    /// memory. If `allocator` is 0, the default memory allocator is used.
    ExecutionUtil_OneWayContinuation(
        bslmf::MovableRef<ExecutionUtil_OneWayContinuation> original,
        bslma::Allocator*                                   allocator = 0);

  public:
    // MANIPULATORS

    /// Execute the contained function object `fn` according to the contained
    /// One-Way execution policy `p` as if by
    /// `ExecutionUtil::execute(p, bsl::move(fn2))`, where `fn1` is the
    /// result of `DECAY_COPY(bsl::move(fn))`, with the call to `DECAY_COPY`
    /// being evaluated in this thread, and `fn2` is a function object of
    /// unspecified type that, when called as `fn2()`, performs
    /// `fn1(FutureResult<R>(ft))`, where `ft` is the contained future
    /// object. The behavior is undefined unless `ft.isReady()` is `true`.
    void operator()(FutureResult<R>);

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(ExecutionUtil_OneWayContinuation,
                                   bslma::UsesBslmaAllocator)
};

// ======================================
// class ExecutionUtil_TwoWayContinuation
// ======================================

/// Provides a wrapper for a continuation callback allowing to retrieve
/// its result. Holds a shared state used to store the result of the
/// callback invocation. Copies of this object could be made, each referring
/// to the same shared state and callback object.
///
/// `P` must be a Two-Way execution policy type (see
/// `mwcex_executionpolicy`).
///
/// `R1` must be such as `Future<R1>` is a valid type.
///
/// `R2` must satisfy one of the following requirements:
/// * be `void`;
/// * be a reference type, such that
///   `bsl::invoke_result_t<bsl::decay_t<F>&&, FutureResult<R1>>` is
///   implicitly convertible to `R2`;
/// * satisfy the requirement of Destructible and MoveConstructible as
///   specified in the C++ standard, and be constructible from
///   `bsl::invoke_result_t<bsl::decay_t<F>&&, FutureResult<R1>>`.
///
/// `F` must meet the requirements of Destructible and MoveConstructible as
/// specified in the C++ standard. Given an object `f` of type `F`,
/// `f(bsl::declval<FutureResult<R1>>())` shall be a valid expression.
template <class P, class R1, class R2, class F>
class ExecutionUtil_TwoWayContinuation {
  private:
    // PRIVATE TYPES
    typedef ExecutionUtil_ContinuationCallbackWrapper<R1, R2, F> Callback;

    typedef ExecutionUtil_SharedTwoWayTask<R2, Callback> Task;

  private:
    // PRIVATE DATA
    P d_policy;

    Task d_task;

  public:
    // CREATORS

    /// Create a `ExecutionUtil_TwoWayContinuation` object containing an
    /// object of type `P` direct-non-list-initialized with `policy`, an
    /// object of type `Future<R1>` direct-non-list-initialized with
    /// `future` and an object of type `F` direct-non-list-initialized with
    /// `bsl::forward<F_PARAM>(function)`. Specify an `allocator` used to
    /// supply memory. The behavior is undefined unless `future.isValid()`
    /// is `true`.
    template <class F_PARAM>
    ExecutionUtil_TwoWayContinuation(const P&          policy,
                                     const Future<R1>& future,
                                     BSLS_COMPILERFEATURES_FORWARD_REF(F_PARAM)
                                         function,
                                     bslma::Allocator* allocator);

  public:
    // MANIPULATORS

    /// Execute the contained function object `fn` according to the contained
    /// Two-Way execution policy `p` as if by
    /// `ExecutionUtil::execute(p.oneWay(), bsl::move(fn2))`, where `fn2` is
    /// a function object of unspecified type that, when called as `fn2()`,
    /// performs `bsl::move(fn)(FutureResult<R1>(ft))` and makes the
    /// contained shared state ready with the result of that operation,
    /// where `ft` is the contained future object. If `R1` is `void`, the
    /// result of the contained function object invocation is discarded. The
    /// behavior is undefined unless `ft.isReady()` is `true`.
    void operator()(FutureResult<R1>);

  public:
    // ACCESSORS

    /// Return a `Future` object associated with the shared state owned by
    /// `*this`.
    Future<R2> future() const BSLS_KEYWORD_NOEXCEPT;
};

// ====================
// struct ExecutionUtil
// ====================

/// Provides a namespace for utility functions used to execute function
/// objects on executors according to specified execution policies.
struct ExecutionUtil {
    // TYPES

    /// Provides a metafunction that determines, at compile time, the type
    /// returned by `mwcex::ExecutionUtil::execute`.
    ///
    /// Given:
    /// * A type `P`, that is an execution policy type (see
    ///:  `mwcex_executionpolicy`).
    ///:
    /// * A type `F`, such that `bsl::decay_t<F>` satisfies the
    ///   requirements of Destructible and MoveConstructible as specified
    ///   in the C++ standard, and such that and object `f` of type `F` is
    ///   callable as `DECAY_COPY(std::forward<F>(f))()`.
    /// * A type `R`, that is the result type encoded in `P`, if explicitly
    ///   specified, or the result type of
    ///   `DECAY_COPY(std::forward<F>(f))()` otherwise.
    ///
    /// The type of `mwcex::ExecutionUtil::ExecuteResult<P, F>::Type`
    /// expression is the follow:
    /// * If `P` is a One-Way policy - `void`.
    /// * If `P` is a Two-Way policy - `mwcex::Future<R>`.
    template <class POLICY, class FUNCTION>
    struct ExecuteResult {
        // TYPES
        typedef
            typename ExecutionUtil_ExecuteResult<POLICY, FUNCTION>::Type Type;
    };

    /// Provides a metafunction that determines, at compile time, the type
    /// returned by `mwcex::ExecutionUtil::thenExecute`.
    ///
    /// Given:
    /// * A type `P`, that is an execution policy type (see
    ///:  `mwcex_executionpolicy`).
    ///:
    /// * A type `FT`, that is `mwcex::Future<R1>`.
    /// * A type `FN`, such that `bsl::decay_t<FN>` satisfies the
    ///   requirements of Destructible and MoveConstructible as specified
    ///   in the C++ standard, and such that and object `fn` of type `FN`
    ///   is callable as 'DECAY_COPY(std::forward<FN>(fn))(
    ///   bsl::declval<FutureResult<R1>>())'.
    /// * A type `R2`, that is the result type encoded in `P`, if
    ///   explicitly specified, or the result type of 'DECAY_COPY(
    ///   std::forward<FN>(fn))(bsl::declval<FutureResult<R1>>())'
    ///   otherwise.
    ///
    /// The type of `mwcex::ThenExecuteResult<P, FT, FN>::Type` expression
    /// is the follow:
    /// * If `P` is a One-Way policy - `void`.
    /// * If `P` is a Two-Way policy - `mwcex::Future<R2>`.
    template <class POLICY, class FUTURE, class FUNCTION>
    struct ThenExecuteResult {
        // TYPES
        typedef typename ExecutionUtil_ThenExecuteResult<POLICY,
                                                         FUTURE,
                                                         FUNCTION>::Type Type;
    };

    // CLASS METHODS

    /// Execute the specified function object `f` according to the specified
    /// One-Way execution `policy`, as if by 'ExecutorTraits<EXECUTOR>::
    /// post(policy.executor(), bsl::forward<FUNCTION>(f))' if the policy is
    /// Never Blocking, and as if 'ExecutorTraits<EXECUTOR>::dispatch(
    /// policy.executor(), bsl::forward<FUNCTION>(f))' otherwise. Use the
    /// specified `policy.allocator()` to allocate memory for all internal
    /// purposes. If the specified `policy` is Always Blocking, block the
    /// calling thread pending completion of the submitted function object
    /// invocation.
    ///
    /// Note that if execution of the submitted function object throws an
    /// exception, that exception is not propagated to the caller of this
    /// function.
    ///
    /// `EXECUTOR` must meet the requirements of Executor (see package
    /// documentation).
    ///
    /// `bsl::decay_t<FUNCTION>` must meet the requirements of Destructible
    /// and MoveConstructible as specified in the C++ standard. 'DECAY_COPY(
    /// bsl::forward<FUNCTION>(f))()' shall be a valid expression.
    template <class EXECUTOR, class FUNCTION>
    static typename ExecuteResult<
        ExecutionPolicy<ExecutionProperty::OneWay, EXECUTOR>,
        FUNCTION>::Type
    execute(const ExecutionPolicy<ExecutionProperty::OneWay, EXECUTOR>& policy,
            BSLS_COMPILERFEATURES_FORWARD_REF(FUNCTION) f);

    /// Call `execute(policy.twoWayR<R>(), bsl::forward<FUNCTION>(f))` and
    /// return the result of that invocation, where `R` is the type of
    /// `bsl::invoke_result_t<bsl::decay_t<FUNCTION>&&>`.
    template <class EXECUTOR, class FUNCTION>
    static typename ExecuteResult<
        ExecutionPolicy<ExecutionProperty::TwoWay, EXECUTOR>,
        FUNCTION>::Type
    execute(const ExecutionPolicy<ExecutionProperty::TwoWay, EXECUTOR>& policy,
            BSLS_COMPILERFEATURES_FORWARD_REF(FUNCTION) f);

    /// Execute the specified function object `f` according to the specified
    /// Two-Way execution `policy`, as if by 'ExecutorTraits<EXECUTOR>::
    /// post(policy.executor(), bsl::forward<FUNCTION>(f))' if the policy is
    /// Never Blocking, and as if 'ExecutorTraits<EXECUTOR>::dispatch(
    /// policy.executor(), bsl::forward<FUNCTION>(f))' otherwise. Use the
    /// specified `policy.allocator()` to allocate the shared state
    /// associated with the returned `Future`, as well as for all internal
    /// purposes. If the specified `policy` is Always Blocking, block the
    /// calling thread pending completion of the submitted function object
    /// invocation. Return a `Future` object [to be] containing the result
    /// of the submitted function object invocation (or the thrown
    /// exception).
    ///
    /// Note that in C++03 any exception thrown as the result of the
    /// submitted function object invocation results in a call to
    /// `bsl::terminate`.
    ///
    /// `R` must satisfy one of the following requirements:
    /// * be `void`;
    /// * be a reference type, such that
    ///   `bsl::invoke_result_t<bsl::decay_t<FUNCTION>&&>` is implicitly
    ///   convertible to `R`;
    /// * satisfy the requirement of Destructible and MoveConstructible as
    ///   specified in the C++ standard, and be constructible from
    ///   `bsl::invoke_result_t<bsl::decay_t<FUNCTION>&&>`.
    ///
    /// `EXECUTOR` must meet the requirements of Executor (see package
    /// documentation).
    ///
    /// `bsl::decay_t<FUNCTION>` must meet the requirements of Destructible
    /// and MoveConstructible as specified in the C++ standard. 'DECAY_COPY(
    /// bsl::forward<FUNCTION>(f))()' shall be a valid expression.
    template <class R, class EXECUTOR, class FUNCTION>
    static typename ExecuteResult<
        ExecutionPolicy<ExecutionProperty::TwoWayR<R>, EXECUTOR>,
        FUNCTION>::Type
    execute(
        const ExecutionPolicy<ExecutionProperty::TwoWayR<R>, EXECUTOR>& policy,
        BSLS_COMPILERFEATURES_FORWARD_REF(FUNCTION) f);

    /// After the specified `future` becomes ready, execute the specified
    /// function object `f` according to the specfied One-Way execution
    /// `policy` as if by `execute(policy, bsl::move(f2))`, where `f1` is
    /// the result of `DECAY_COPY(bsl::forward<FUNCTION>(f))`, with the call
    /// to `DECAY_COPY` being evaluated in this thread, and `f2` is a
    /// function object of unspecified type that, when called as `f2()`,
    /// performs `f1(FutureResult<R1>(future))`. Use the specified
    /// `policy.allocator()` to allocate memory for all internal purposes.
    /// The behavior is undefined unless `future.isValid()` is `true`, or
    /// if a callback is already attached to the associated shared state of
    /// the specified `future`.
    ///
    /// Note that the lifetime of the shared state associated with the
    /// specified `future` is extended until the end of the submitted
    /// function object execution.
    ///
    /// `EXECUTOR` must meet the requirements of Executor (see package
    /// documentation).
    ///
    /// `bsl::decay_t<FUNCTION>` must meet the requirements of Destructible
    /// and MoveConstructible as specified in the C++ standard. 'DECAY_COPY(
    /// bsl::forward<FUNCTION>(f))(bsl::declval<FutureResult<R1>>())' shall
    /// be a valid expression.
    template <class EXECUTOR, class R1, class FUNCTION>
    static typename ThenExecuteResult<
        ExecutionPolicy<ExecutionProperty::OneWay, EXECUTOR>,
        Future<R1>,
        FUNCTION>::Type
    thenExecute(
        const ExecutionPolicy<ExecutionProperty::OneWay, EXECUTOR>& policy,
        const Future<R1>&                                           future,
        BSLS_COMPILERFEATURES_FORWARD_REF(FUNCTION) f);

    /// Call 'thenExecute(policy.twoWayR<R2>(), future,
    /// bsl::forward<FUNCTION>(f))' and return the result of that
    /// invocation, where `R2` is the type of
    /// `bsl::invoke_result_t<bsl::decay_t<FUNCTION>&&, FutureResult<R1>>`.
    template <class EXECUTOR, class R1, class FUNCTION>
    static typename ThenExecuteResult<
        ExecutionPolicy<ExecutionProperty::TwoWay, EXECUTOR>,
        Future<R1>,
        FUNCTION>::Type
    thenExecute(
        const ExecutionPolicy<ExecutionProperty::TwoWay, EXECUTOR>& policy,
        const Future<R1>&                                           future,
        BSLS_COMPILERFEATURES_FORWARD_REF(FUNCTION) f);

    /// After the specified `future` becomes ready, execute the specified
    /// function object `f` according to the specfied Two-Way execution
    /// `policy` as if by `execute(policy, bsl::move(f2))`, where `f1` is
    /// the result of `DECAY_COPY(bsl::forward<FUNCTION>(f))`, with the call
    /// to `DECAY_COPY` being evaluated in this thread, and `f2` is a
    /// function object of unspecified type that, when called as `f2()`,
    /// performs `return f1(FutureResult<R1>(future))`. Use the specified
    /// `policy.allocator()` to allocate the shared state associated with
    /// the returned `Future`, as well as for all internal purposes. Return
    /// a `Future` object to be containing the result of the submitted
    /// function object invocation (or the thrown exception). The behavior
    /// is undefined unless `future.isValid()` is `true`, or if a callback
    /// is already attached to the associated shared state of the specified
    /// `future`.
    ///
    /// Note that the lifetime of the shared state associated with the
    /// specified `future` is extended until the end of the submitted
    /// function object execution.
    ///
    /// `R2` must satisfy one of the following requirements:
    /// * be `void`;
    /// * be a reference type, such that
    ///   `bsl::invoke_result_t<bsl::decay_t<F>&&, FutureResult<R1>>` is
    ///   implicitly convertible to `R2`;
    /// * satisfy the requirement of Destructible and MoveConstructible as
    ///   specified in the C++ standard, and be constructible from
    ///   `bsl::invoke_result_t<bsl::decay_t<F>&&, FutureResult<R1>>`.
    ///
    /// `EXECUTOR` must meet the requirements of Executor (see package
    /// documentation).
    ///
    /// `bsl::decay_t<FUNCTION>` must meet the requirements of Destructible
    /// and MoveConstructible as specified in the C++ standard. 'DECAY_COPY(
    /// bsl::forward<FUNCTION>(f))(bsl::declval<FutureResult<R1>>())' shall
    /// be a valid expression.
    template <class R2, class EXECUTOR, class R1, class FUNCTION>
    static typename ThenExecuteResult<
        ExecutionPolicy<ExecutionProperty::TwoWayR<R2>, EXECUTOR>,
        Future<R1>,
        FUNCTION>::Type
    thenExecute(const ExecutionPolicy<ExecutionProperty::TwoWayR<R2>,
                                      EXECUTOR>& policy,
                const Future<R1>&                future,
                BSLS_COMPILERFEATURES_FORWARD_REF(FUNCTION) f);

    /// Call `invokeR<R>(executor, bsl::forward<FUNCTION>(f))` and return
    /// the result of that invocation, where `R` is the type of
    /// `bsl::invoke_result_t<FUNCTION>`.
    template <class EXECUTOR, class FUNCTION>
    static typename bsl::invoke_result<FUNCTION>::type
    invoke(const EXECUTOR& executor,
           BSLS_COMPILERFEATURES_FORWARD_REF(FUNCTION) f);

    /// Invoke the specified function object `f`, as if by
    /// `ExecutorTraits<EXECUTOR>::dispatch(executor, f2)`, where `f2` is a
    /// function object of unspecified type that, when called as `f2()`
    /// performs `return bsl::forward<FUNCTION>(f)()`. Block the calling
    /// thread pending completion of the submitted function object
    /// invocation. Return the result of the submitted function object
    /// invocation (or thrown the resulting exception).
    ///
    /// Note that in C++03 any exception thrown as the result of the
    /// submitted function object invocation results in a call to
    /// `bsl::terminate`.
    ///
    /// `R` must satisfy one of the following requirements:
    /// * be `void`;
    /// * be a reference type, such that
    ///   `bsl::invoke_result_t<bsl::decay_t<FUNCTION>&&>` is implicitly
    ///   convertible to `R`;
    /// * satisfy the requirement of Destructible and MoveConstructible as
    ///   specified in the C++ standard, and be constructible from
    ///   `bsl::invoke_result_t<bsl::decay_t<FUNCTION>&&>`.
    ///
    /// `EXECUTOR` must meet the requirements of Executor (see package
    /// documentation).
    ///
    /// `bsl::forward<FUNCTION>(f)()` shall be a valid expression.
    template <class R, class EXECUTOR, class FUNCTION>
    static R invokeR(const EXECUTOR& executor,
                     BSLS_COMPILERFEATURES_FORWARD_REF(FUNCTION) f);
};

// ============================================================================
//                           INLINE DEFINITIONS
// ============================================================================

// -------------------------------
// class ExecutionUtil_FunctionRef
// -------------------------------

// PRIVATE ACCESSORS
template <class R, class F>
inline void ExecutionUtil_FunctionRef<R, F>::invoke(
    BSLS_ANNOTATION_UNUSED bsl::true_type voidResultTag) const
{
#ifdef BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES
    bsl::forward<F> (*d_function_p)();
#else
    (*d_function_p)();
#endif
}

template <class R, class F>
inline R ExecutionUtil_FunctionRef<R, F>::invoke(
    BSLS_ANNOTATION_UNUSED bsl::false_type voidResultTag) const
{
#ifdef BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES
    return bsl::forward<F>(*d_function_p)();
#else
    return (*d_function_p)();
#endif
}

// CREATORS
template <class R, class F>
inline BSLS_KEYWORD_CONSTEXPR_CPP14
ExecutionUtil_FunctionRef<R, F>::ExecutionUtil_FunctionRef(
    typename bsl::remove_reference<F>::type* function) BSLS_KEYWORD_NOEXCEPT
: d_function_p(function)
{
    // PRECONDITIONS
    BSLS_ASSERT(function);
}

// ACCESSORS
template <class R, class F>
inline R ExecutionUtil_FunctionRef<R, F>::operator()() const
{
    return invoke(bsl::is_void<R>());
}

// ----------------------------------
// class ExecutionUtil_OneOffFunction
// ----------------------------------

// PRIVATE ACCESSORS
template <class R, class F>
inline void ExecutionUtil_OneOffFunction<R, F>::invoke(
    BSLS_ANNOTATION_UNUSED bsl::true_type voidResultTag)
{
    bslmf::Util::moveIfSupported(d_functionValue.object().value())();
}

template <class R, class F>
inline R ExecutionUtil_OneOffFunction<R, F>::invoke(
    BSLS_ANNOTATION_UNUSED bsl::false_type voidResultTag)
{
    return bslmf::Util::moveIfSupported(d_functionValue.object().value())();
}

// CREATORS
template <class R, class F>
template <class F_PARAM>
inline BSLS_KEYWORD_CONSTEXPR_CPP14
ExecutionUtil_OneOffFunction<R, F>::ExecutionUtil_OneOffFunction(
    BSLS_COMPILERFEATURES_FORWARD_REF(F_PARAM) f,
    bslma::Allocator* allocator)
: d_functionValue(BSLS_COMPILERFEATURES_FORWARD(F_PARAM, f), allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT(allocator);
}

// MANIPULATORS
template <class R, class F>
inline R ExecutionUtil_OneOffFunction<R, F>::operator()()
{
    // PRECONDITIONS
    BSLS_ASSERT(!d_functionValue.object().isNull());

    DestroyFunctionOnScopeExit guard(&d_functionValue);
    return invoke(bsl::is_void<R>());
}

// ---------------------------------------------------------------
// struct ExecutionUtil_OneOffFunction::DestroyFunctionOnScopeExit
// ---------------------------------------------------------------

// CREATORS
template <class R, class F>
inline BSLS_KEYWORD_CONSTEXPR_CPP14 ExecutionUtil_OneOffFunction<R, F>::
    DestroyFunctionOnScopeExit ::DestroyFunctionOnScopeExit(
        FunctionValueType* functionValue) BSLS_KEYWORD_NOEXCEPT
: d_functionValue_p(functionValue)
{
    // PRECONDITIONS
    BSLS_ASSERT(functionValue);
}

template <class R, class F>
inline ExecutionUtil_OneOffFunction<R, F>::DestroyFunctionOnScopeExit ::
    ~DestroyFunctionOnScopeExit()
{
    d_functionValue_p->object().reset();
}

// ------------------------------------
// class ExecutionUtil_UniqueOneWayTask
// ------------------------------------

// CREATORS
template <class F>
template <class F_PARAM>
inline ExecutionUtil_UniqueOneWayTask<F>::ExecutionUtil_UniqueOneWayTask(
    BSLS_COMPILERFEATURES_FORWARD_REF(F_PARAM) f,
    bslma::Allocator* allocator)
: d_latch(1)
, d_function(BSLS_COMPILERFEATURES_FORWARD(F_PARAM, f), allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT(allocator);
}

// MANIPULATORS
template <class F>
inline void ExecutionUtil_UniqueOneWayTask<F>::operator()()
{
    try {
        // invoke and destroy the function object
        d_function();

        // notify awaiting threads
        d_latch.arrive();
    }
    catch (...) {
        // notify awaiting threads
        d_latch.arrive();

        // propagate exception
        throw;  // THROW
    }

    // NOTE: Notifying awaiting threads is the last thing we do, as this object
    //       typically will be destroyed immediately after 'wait' return.
}

// ACCESSORS
template <class F>
inline void
ExecutionUtil_UniqueOneWayTask<F>::wait() const BSLS_KEYWORD_NOEXCEPT
{
    d_latch.wait();
}

// ------------------------------------
// class ExecutionUtil_UniqueTwoWayTask
// ------------------------------------

// PRIVATE MANIPULATORS
template <class R, class F>
inline bslmf::Nil ExecutionUtil_UniqueTwoWayTask<R, F>::doInvoke(
    BSLS_ANNOTATION_UNUSED bsl::true_type voidResultTag)
{
    d_function();
    return bslmf::Nil();
}

template <class R, class F>
inline R ExecutionUtil_UniqueTwoWayTask<R, F>::doInvoke(
    BSLS_ANNOTATION_UNUSED bsl::false_type voidResultTag)
{
    return d_function();
}

// PRIVATE MANIPULATORS
template <class R, class F>
inline void ExecutionUtil_UniqueTwoWayTask<R, F>::doGet(
    BSLS_ANNOTATION_UNUSED bsl::true_type voidResultTag)
{
    d_sharedState.get();
}

template <class R, class F>
inline R ExecutionUtil_UniqueTwoWayTask<R, F>::doGet(
    BSLS_ANNOTATION_UNUSED bsl::false_type voidResultTag)
{
    return bslmf::Util::moveIfSupported(d_sharedState.get());

    // NOTE: Could have used 'bslmf::MovableRefUtil::move', but it doesn't play
    //       nice with 'bsl::reference_wrapper'.
}

// CREATORS
template <class R, class F>
template <class F_PARAM>
inline ExecutionUtil_UniqueTwoWayTask<R, F>::ExecutionUtil_UniqueTwoWayTask(
    BSLS_COMPILERFEATURES_FORWARD_REF(F_PARAM) f,
    bslma::Allocator* allocator)
: d_syncLock()
, d_sharedState(allocator)
, d_function(BSLS_COMPILERFEATURES_FORWARD(F_PARAM, f), allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT(allocator);
}

// MANIPULATORS
template <class R, class F>
inline void
ExecutionUtil_UniqueTwoWayTask<R, F>::operator()() BSLS_NOTHROW_SPEC
{
    // NOTE: 'BSLS_NOTHROW_SPEC' specification ensures that 'bsl::terminate' is
    //       called in case of exception in C++03.

    bsls::SpinLockGuard lock(&d_syncLock);  // LOCK

    // NOTE: The spinlock is used to synchronize the completion of the call
    //       operator with the completion of 'get', as this object typically
    //       will be destroyed immediately after 'get' return.
    //
    //       We wouldn't need to perform such synchronization if the completion
    //       of 'mwcex::FutureSharedState's 'get' function were synchronizing
    //       with the completion of its setter functions, but, for performance
    //       reasons, it does not.

#ifdef BSLS_LIBRARYFEATURES_HAS_CPP11_EXCEPTION_HANDLING
    try {
#endif
        // invoke function object and make shared state ready with the result
        d_sharedState.setValue(doInvoke(bsl::is_void<R>()));
#ifdef BSLS_LIBRARYFEATURES_HAS_CPP11_EXCEPTION_HANDLING
    }
    catch (...) {
        // make shared state ready with the thrown exception
        d_sharedState.setException(bsl::current_exception());
    }
#endif
}

template <class R, class F>
inline R ExecutionUtil_UniqueTwoWayTask<R, F>::get()
{
    // lock / unlock the spinlock on scope exit to synchronize with the
    // completion of the call operator
    SynchronizeOnScopeExit syncGuard(&d_syncLock);

    return doGet(bsl::is_void<R>());
}

// ------------------------------------------------------------
// class ExecutionUtil_UniqueTwoWayTask::SynchronizeOnScopeExit
// ------------------------------------------------------------

// CREATORS
template <class R, class F>
inline BSLS_KEYWORD_CONSTEXPR_CPP14
ExecutionUtil_UniqueTwoWayTask<R, F>::SynchronizeOnScopeExit::
    SynchronizeOnScopeExit(bsls::SpinLock* spinlock) BSLS_KEYWORD_NOEXCEPT
: d_spinlock_p(spinlock)
{
    // PRECONDITIONS
    BSLS_ASSERT(spinlock);
}

template <class R, class F>
inline ExecutionUtil_UniqueTwoWayTask<R, F>::SynchronizeOnScopeExit::
    ~SynchronizeOnScopeExit()
{
    bsls::SpinLockGuard lock(d_spinlock_p);  // LOCK
}

// ------------------------------------
// class ExecutionUtil_SharedTwoWayTask
// ------------------------------------

// PRIVATE MANIPULATORS
template <class R, class F>
inline bslmf::Nil ExecutionUtil_SharedTwoWayTask<R, F>::doInvoke(
    BSLS_ANNOTATION_UNUSED bsl::true_type voidResultTag)
{
    d_state->d_function();
    return bslmf::Nil();
}

template <class R, class F>
inline R ExecutionUtil_SharedTwoWayTask<R, F>::doInvoke(
    BSLS_ANNOTATION_UNUSED bsl::false_type voidResultTag)
{
    return d_state->d_function();
}

// CREATORS
template <class R, class F>
template <class F_PARAM>
inline ExecutionUtil_SharedTwoWayTask<R, F>::ExecutionUtil_SharedTwoWayTask(
    BSLS_COMPILERFEATURES_FORWARD_REF(F_PARAM) f,
    bslma::Allocator* allocator)
: d_state(
      bsl::allocate_shared<State>(allocator,
                                  BSLS_COMPILERFEATURES_FORWARD(F_PARAM, f),
                                  allocator))
{
    // PRECONDITIONS
    BSLS_ASSERT(allocator);
}

// MANIPULATORS
template <class R, class F>
inline void
ExecutionUtil_SharedTwoWayTask<R, F>::operator()() BSLS_NOTHROW_SPEC
{
    // NOTE: 'BSLS_NOTHROW_SPEC' specification ensures that 'bsl::terminate' is
    //       called in case of exception in C++03.

#ifdef BSLS_LIBRARYFEATURES_HAS_CPP11_EXCEPTION_HANDLING
    try {
#endif
        // invoke function object and make shared state ready with the result
        d_state->d_sharedState.setValue(doInvoke(bsl::is_void<R>()));
#ifdef BSLS_LIBRARYFEATURES_HAS_CPP11_EXCEPTION_HANDLING
    }
    catch (...) {
        // make shared state ready with the thrown exception
        d_state->d_sharedState.setException(bsl::current_exception());
    }
#endif
}

// ACCESSORS
template <class R, class F>
inline Future<R>
ExecutionUtil_SharedTwoWayTask<R, F>::future() const BSLS_KEYWORD_NOEXCEPT
{
    return Future<R>(
        bsl::shared_ptr<SharedStateType>(d_state, &d_state->d_sharedState));
}

// -------------------------------------------
// class ExecutionUtil_SharedTwoWayTask::State
// -------------------------------------------

// CREATORS
template <class R, class F>
template <class F_PARAM>
inline ExecutionUtil_SharedTwoWayTask<R, F>::State::State(
    BSLS_COMPILERFEATURES_FORWARD_REF(F_PARAM) f,
    bslma::Allocator* allocator)
: d_sharedState(allocator)
, d_function(BSLS_COMPILERFEATURES_FORWARD(F_PARAM, f), allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT(allocator);
}

// -----------------------------------------------
// class ExecutionUtil_ContinuationCallbackWrapper
// -----------------------------------------------

// CREATORS
template <class R1, class R2, class F>
template <class F_PARAM>
inline ExecutionUtil_ContinuationCallbackWrapper<R1, R2, F>::
    ExecutionUtil_ContinuationCallbackWrapper(
        const Future<R1>& future,
        BSLS_COMPILERFEATURES_FORWARD_REF(F_PARAM) function,
        bslma::Allocator* allocator)
: d_future(future)
, d_function(BSLS_COMPILERFEATURES_FORWARD(F_PARAM, function), allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT(future.isValid());
    BSLS_ASSERT(allocator);
}

template <class R1, class R2, class F>
inline ExecutionUtil_ContinuationCallbackWrapper<R1, R2, F>::
    ExecutionUtil_ContinuationCallbackWrapper(
        const ExecutionUtil_ContinuationCallbackWrapper& original,
        bslma::Allocator*                                allocator)
: d_future(original.d_future)
, d_function(original.d_function.object(), allocator)
{
    // NOTHING
}

template <class R1, class R2, class F>
inline ExecutionUtil_ContinuationCallbackWrapper<R1, R2, F>::
    ExecutionUtil_ContinuationCallbackWrapper(
        bslmf::MovableRef<ExecutionUtil_ContinuationCallbackWrapper> original,
        bslma::Allocator*                                            allocator)
: d_future(bslmf::MovableRefUtil::move(
      bslmf::MovableRefUtil::access(original).d_future))
, d_function(bslmf::MovableRefUtil::move(
                 bslmf::MovableRefUtil::access(original).d_function.object()),
             allocator)
{
    // NOTHING
}

// MANIPULATORS
template <class R1, class R2, class F>
inline R2 ExecutionUtil_ContinuationCallbackWrapper<R1, R2, F>::operator()()
{
    // PRECONDITIONS
    BSLS_ASSERT(d_future.isReady());

    return static_cast<R2>(bslmf::Util::moveIfSupported(d_function.object())(
        FutureResult<R1>(d_future)));

    // NOTE: use 'static_cast' to discard the invocation result in case 'R2' is
    //       'void'. If 'R2' is not 'void' a temporary copy of the returned
    //       object might be created but, in practice, copy elision should
    //       prevent that.
}

// --------------------------------------
// class ExecutionUtil_OneWayContinuation
// --------------------------------------

// CREATORS
template <class P, class R, class F>
template <class F_PARAM>
inline ExecutionUtil_OneWayContinuation<P, R, F>::
    ExecutionUtil_OneWayContinuation(const P&         policy,
                                     const Future<R>& future,
                                     BSLS_COMPILERFEATURES_FORWARD_REF(F_PARAM)
                                         function,
                                     bslma::Allocator* allocator)
: d_policy(policy)
, d_callback(future,
             BSLS_COMPILERFEATURES_FORWARD(F_PARAM, function),
             allocator)
{
    // PRECONDITIONS
    BSLMF_ASSERT(P::k_IS_ONE_WAY);
    BSLS_ASSERT(future.isValid());
    BSLS_ASSERT(allocator);
}

template <class P, class R, class F>
inline ExecutionUtil_OneWayContinuation<P, R, F>::
    ExecutionUtil_OneWayContinuation(
        const ExecutionUtil_OneWayContinuation& original,
        bslma::Allocator*                       allocator)
: d_policy(original.d_policy)
, d_callback(original.d_callback, allocator)
{
    // NOTHING
}

template <class P, class R, class F>
inline ExecutionUtil_OneWayContinuation<P, R, F>::
    ExecutionUtil_OneWayContinuation(
        bslmf::MovableRef<ExecutionUtil_OneWayContinuation> original,
        bslma::Allocator*                                   allocator)
: d_policy(bslmf::MovableRefUtil::move(
      bslmf::MovableRefUtil::access(original).d_policy))
, d_callback(bslmf::MovableRefUtil::move(
                 bslmf::MovableRefUtil::access(original).d_callback),
             allocator)
{
    // NOTHING
}

// MANIPULATORS
template <class P, class R, class F>
void inline ExecutionUtil_OneWayContinuation<P, R, F>::operator()(
    FutureResult<R>)
{
    // The async result is ready. Initiate execution of the continuation
    // callback.
    ExecutionUtil::execute(bslmf::Util::moveIfSupported(d_policy),
                           bslmf::Util::moveIfSupported(d_callback));
}

// --------------------------------------
// class ExecutionUtil_TwoWayContinuation
// --------------------------------------

// CREATORS
template <class P, class R1, class R2, class F>
template <class F_PARAM>
inline ExecutionUtil_TwoWayContinuation<P, R1, R2, F>::
    ExecutionUtil_TwoWayContinuation(const P&          policy,
                                     const Future<R1>& future,
                                     BSLS_COMPILERFEATURES_FORWARD_REF(F_PARAM)
                                         function,
                                     bslma::Allocator* allocator)
: d_policy(policy)
, d_task(Callback(future,
                  BSLS_COMPILERFEATURES_FORWARD(F_PARAM, function),
                  allocator),
         allocator)
{
    // PRECONDITIONS
    BSLMF_ASSERT(P::k_IS_TWO_WAY);
    BSLS_ASSERT(future.isValid());
    BSLS_ASSERT(allocator);
}

// MANIPULATORS
template <class P, class R1, class R2, class F>
inline void
ExecutionUtil_TwoWayContinuation<P, R1, R2, F>::operator()(FutureResult<R1>)
{
    // The async result is ready. Initiate execution of the continuation
    // callback.
    ExecutionUtil::execute(bslmf::Util::moveIfSupported(d_policy).oneWay(),
                           d_task);

    // NOTE: We do not move 'd_task' because 'future' may be called after this
    //       function. But it's okay, the task is cheap to copy anyway.
}

// ACCESSORS
template <class P, class R1, class R2, class F>
inline Future<R2> ExecutionUtil_TwoWayContinuation<P, R1, R2, F>::future()
    const BSLS_KEYWORD_NOEXCEPT
{
    return d_task.future();
}

// --------------------
// struct ExecutionUtil
// --------------------

// CLASS METHODS
template <class EXECUTOR, class FUNCTION>
inline typename ExecutionUtil::ExecuteResult<
    ExecutionPolicy<ExecutionProperty::OneWay, EXECUTOR>,
    FUNCTION>::Type
ExecutionUtil::execute(
    const ExecutionPolicy<ExecutionProperty::OneWay, EXECUTOR>& policy,
    BSLS_COMPILERFEATURES_FORWARD_REF(FUNCTION) f)
{
    switch (policy.blocking()) {
    case ExecutionProperty::e_NEVER_BLOCKING: {
        // submit the function object for execution via 'post'
        ExecutorTraits<EXECUTOR>::post(policy.executor(),
                                       BSLS_COMPILERFEATURES_FORWARD(FUNCTION,
                                                                     f));
        break;  // BREAK
    }

    case ExecutionProperty::e_POSSIBLY_BLOCKING: {
        // submit the function object for execution via 'dispatch'
        ExecutorTraits<EXECUTOR>::dispatch(
            policy.executor(),
            BSLS_COMPILERFEATURES_FORWARD(FUNCTION, f));
        break;  // BREAK
    }

    case ExecutionProperty::e_ALWAYS_BLOCKING: {
        typedef ExecutionUtil_UniqueOneWayTask<
            typename bsl::decay<FUNCTION>::type>
            Task;

        // create a "task" to synchronize with the completion of the
        // submitted function object invocation
        Task task(BSLS_COMPILERFEATURES_FORWARD(FUNCTION, f),
                  policy.allocator());

        // submit the function object for execution via 'dispatch'
        ExecutorTraits<EXECUTOR>::dispatch(
            policy.executor(),
            bdlf::MemFnUtil::memFn(&Task::operator(), &task));

        // block the calling thread pending completion of the submitted
        // function object invocation
        task.wait();
        break;  // BREAK
    }

    default: {
        // shouldn't be here
        BSLS_ASSERT(false);
    }
    }
}

template <class EXECUTOR, class FUNCTION>
inline typename ExecutionUtil::ExecuteResult<
    ExecutionPolicy<ExecutionProperty::TwoWay, EXECUTOR>,
    FUNCTION>::Type
ExecutionUtil::execute(
    const ExecutionPolicy<ExecutionProperty::TwoWay, EXECUTOR>& policy,
    BSLS_COMPILERFEATURES_FORWARD_REF(FUNCTION) f)
{
#ifdef BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES
    typedef typename bsl::invoke_result<
        typename bsl::decay<FUNCTION>::type&&>::type Result;
#else
    typedef
        typename bsl::invoke_result<typename bsl::decay<FUNCTION>::type>::type
                                                                 Result;
#endif

    return execute(policy.template twoWayR<Result>(),
                   BSLS_COMPILERFEATURES_FORWARD(FUNCTION, f));
}

template <class R, class EXECUTOR, class FUNCTION>
inline typename ExecutionUtil::ExecuteResult<
    ExecutionPolicy<ExecutionProperty::TwoWayR<R>, EXECUTOR>,
    FUNCTION>::Type
ExecutionUtil::execute(
    const ExecutionPolicy<ExecutionProperty::TwoWayR<R>, EXECUTOR>& policy,
    BSLS_COMPILERFEATURES_FORWARD_REF(FUNCTION) f)
{
    typedef ExecutionUtil_SharedTwoWayTask<R,
                                           typename bsl::decay<FUNCTION>::type>
        Task;

    // create a "task" to synchronize with the completion of the submitted
    // function object invocation and retrieve its result
    Task task(BSLS_COMPILERFEATURES_FORWARD(FUNCTION, f), policy.allocator());

    switch (policy.blocking()) {
    case ExecutionProperty::e_NEVER_BLOCKING: {
        // submit the function object for execution via 'post'
        ExecutorTraits<EXECUTOR>::post(policy.executor(), task);
        break;  // BREAK
    }

    case ExecutionProperty::e_POSSIBLY_BLOCKING: {
        // submit the function object for execution via 'dispatch'
        ExecutorTraits<EXECUTOR>::dispatch(policy.executor(), task);
        break;  // BREAK
    }

    case ExecutionProperty::e_ALWAYS_BLOCKING: {
        // submit the function object for execution via 'dispatch'
        ExecutorTraits<EXECUTOR>::dispatch(policy.executor(), task);

        // block the calling thread pending completion of the submitted
        // function object invocation
        task.future().wait();
        break;  // BREAK
    }

    default: {
        // shouldn't be here
        BSLS_ASSERT(false);
    }
    }

    // return a future [to be] containing the result of the submitted function
    // object invocation
    return task.future();
}

template <class EXECUTOR, class R1, class FUNCTION>
inline typename ExecutionUtil::ThenExecuteResult<
    ExecutionPolicy<ExecutionProperty::OneWay, EXECUTOR>,
    Future<R1>,
    FUNCTION>::Type
ExecutionUtil::thenExecute(
    const ExecutionPolicy<ExecutionProperty::OneWay, EXECUTOR>& policy,
    const Future<R1>&                                           future,
    BSLS_COMPILERFEATURES_FORWARD_REF(FUNCTION) f)
{
    typedef ExecutionUtil_OneWayContinuation<
        ExecutionPolicy<ExecutionProperty::OneWay, EXECUTOR>,
        R1,
        typename bsl::decay<FUNCTION>::type>
        Continuation;

    // PRECONDITIONS
    BSLS_ASSERT(future.isValid());

    // create a continuation callback to be invoked when the input future is
    // ready
    Continuation continuation(policy,
                              future,
                              BSLS_COMPILERFEATURES_FORWARD(FUNCTION, f),
                              policy.allocator());

    // attach the continuation to the input future
    Future<R1>(future).whenReady(bslmf::Util::moveIfSupported(continuation));
    // NOTE: We make a non-const copy of the future because 'whenReady' is a
    //       manipulator.
}

template <class EXECUTOR, class R1, class FUNCTION>
inline typename ExecutionUtil::ThenExecuteResult<
    ExecutionPolicy<ExecutionProperty::TwoWay, EXECUTOR>,
    Future<R1>,
    FUNCTION>::Type
ExecutionUtil::thenExecute(
    const ExecutionPolicy<ExecutionProperty::TwoWay, EXECUTOR>& policy,
    const Future<R1>&                                           future,
    BSLS_COMPILERFEATURES_FORWARD_REF(FUNCTION) f)
{
#ifdef BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES
    typedef typename bsl::invoke_result<typename bsl::decay<FUNCTION>::type&&,
                                        FutureResult<R1> >::type Result;
#else
    typedef typename bsl::invoke_result<typename bsl::decay<FUNCTION>::type,
                                        FutureResult<R1> >::type Result;
#endif

    return thenExecute(policy.template twoWayR<Result>(),
                       future,
                       BSLS_COMPILERFEATURES_FORWARD(FUNCTION, f));
}

template <class R2, class EXECUTOR, class R1, class FUNCTION>
inline typename ExecutionUtil::ThenExecuteResult<
    ExecutionPolicy<ExecutionProperty::TwoWayR<R2>, EXECUTOR>,
    Future<R1>,
    FUNCTION>::Type
ExecutionUtil::thenExecute(
    const ExecutionPolicy<ExecutionProperty::TwoWayR<R2>, EXECUTOR>& policy,
    const Future<R1>&                                                future,
    BSLS_COMPILERFEATURES_FORWARD_REF(FUNCTION) f)
{
    typedef ExecutionUtil_TwoWayContinuation<
        ExecutionPolicy<ExecutionProperty::TwoWayR<R2>, EXECUTOR>,
        R1,
        R2,
        typename bsl::decay<FUNCTION>::type>
        Continuation;

    // PRECONDITIONS
    BSLS_ASSERT(future.isValid());

    // create a continuation callback to be invoked when the input future is
    // ready
    Continuation continuation(policy,
                              future,
                              BSLS_COMPILERFEATURES_FORWARD(FUNCTION, f),
                              policy.allocator());

    // attach the continuation to the input future
    Future<R1>(future).whenReady(continuation);
    // NOTE: We make a non-const copy of the future because 'whenReady' is a
    //       manipulator.

    // return a future to be containing the result of the submitted function
    // object invocation
    return continuation.future();
}

template <class EXECUTOR, class FUNCTION>
inline typename bsl::invoke_result<FUNCTION>::type
ExecutionUtil::invoke(const EXECUTOR& executor,
                      BSLS_COMPILERFEATURES_FORWARD_REF(FUNCTION) f)
{
    return invokeR<typename bsl::invoke_result<FUNCTION>::type>(
        executor,
        BSLS_COMPILERFEATURES_FORWARD(FUNCTION, f));
}

template <class R, class EXECUTOR, class FUNCTION>
inline R ExecutionUtil::invokeR(const EXECUTOR& executor,
                                BSLS_COMPILERFEATURES_FORWARD_REF(FUNCTION) f)
{
#ifdef BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES
    typedef FUNCTION FunctionArgType;
#else
    typedef const FUNCTION& FunctionArgType;
#endif
    typedef ExecutionUtil_UniqueTwoWayTask<
        R,
        ExecutionUtil_FunctionRef<R, FunctionArgType> >
        Task;

    // create a "task" to synchronize with the completion of the submitted
    // function object invocation and retrieve its result
    Task task(ExecutionUtil_FunctionRef<R, FunctionArgType>(&f),
              bslma::Default::allocator());

    // submit the function object for execution
    ExecutorTraits<EXECUTOR>::dispatch(
        executor,
        bdlf::MemFnUtil::memFn(&Task::operator(), &task));

    // block the calling thread until the result is ready, then return the
    // result
    return task.get();
}

}  // close package namespace
}  // close enterprise namespace

#endif
