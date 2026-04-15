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

// bmqex_executionutil.h                                              -*-C++-*-
#ifndef INCLUDED_BMQEX_EXECUTIONUTIL
#define INCLUDED_BMQEX_EXECUTIONUTIL

//@PURPOSE: Provides utility functions to execute function objects.
//
//@CLASSES:
//  bmqex::ExecutionUtil: a namespace for utility functions to execute functors
//
//@SEE ALSO:
//  bmqex_executionpolicy
//
//@DESCRIPTION:
// This component provides a struct, 'bmqex::ExecutionUtil', that serves as a
// namespace for utility functions to execute function objects of arbitrary
// (template parameter) type according to a set of rules defined by a specific
// execution policy (see 'bmqex_executionpolicy').
//
// Those utility functions are commonly referred as execution functions.
//
/// Execution functions for immediate execution
///-------------------------------------------
// Those are functions, all named 'bmqex::ExecutionUtil::execute', to initiate
// immediate execution of function objects. Function objects executed should
// not take any argument.
//..
//  // a function object to be executed
//  auto myFunction = []() -> void
//                    {
//                        bsl::cout << "It Works!\n";
//                    };
//..
// 'execute' takes an execution policy (see 'bmqex_executionpolicy') specifying
// how the function object is to be executed, and the function object itself.
//
// Below are several usage examples of 'bmqex::ExecutionUtil::execute' for
// different use cases.
//
// 1. All we want is the function object to be executed. Don't want to wait
//    for the function object completion. That use-case is commonly referred
//    as "fire and forget". For that we may use a Never Blocking execution
//    policy.
//..
//  using bmqex;
//
//  // initiate the execution and "forget" about it
//  ExecutionUtil::execute(ExecutionPolicyUtil::neverBlocking(),
//                         myFunction);
//..
// 2. We want to execute a function object and wait for its completion. In that
//    case, we may use an Always Blocking execution policy.
//..
//  using bmqex;
//
//  // initiate the execution and wait for completion
//  ExecutionUtil::execute(ExecutionPolicyUtil::alwaysBlocking(),
//                         myFunction);
//..

#include <bmqex_executionproperty.h>
#include <bmqex_executortraits.h>

// BDE
#include <bdlb_nullablevalue.h>
#include <bdlf_memfn.h>
#include <bsl_type_traits.h>
#include <bsl_utility.h>
#include <bsla_annotations.h>
#include <bslalg_constructorproxy.h>
#include <bslma_allocator.h>
#include <bslmf_decay.h>
#include <bslmf_isvoid.h>
#include <bslmf_removeconst.h>
#include <bslmf_removereference.h>
#include <bslmf_util.h>
#include <bslmt_latch.h>
#include <bsls_assert.h>
#include <bsls_compilerfeatures.h>
#include <bsls_keyword.h>

namespace BloombergLP {

namespace bmqex {

// FORWARD DECLARATION
template <class>
class ExecutionPolicy;

// ==================================
// struct ExecutionUtil_ExecuteResult
// ==================================

/// Provides a metafunction that determines, at compile time, the type
/// returned by `bmqex::ExecutionUtil::execute`.
template <class POLICY, class FUNCTION>
struct ExecutionUtil_ExecuteResult {
    // TYPES
    typedef void Type;
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
// class ExecutionUtil_UniqueTask
// ====================================

/// Provides a wrapper for a function object allowing to synchronize with
/// the completion of the function object invocation.
///
/// `F` must meet the requirements of Destructible as specified in the C++
/// standard. Given an object `f` of type `F`, `f()` shall be a valid
/// expression.
template <class F>
class ExecutionUtil_UniqueTask {
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
    ExecutionUtil_UniqueTask(const ExecutionUtil_UniqueTask&)
        BSLS_KEYWORD_DELETED;
    ExecutionUtil_UniqueTask&
    operator=(const ExecutionUtil_UniqueTask&) BSLS_KEYWORD_DELETED;

  public:
    // CREATORS

    /// Create a `ExecutionUtil_UniqueTask` object containing an
    /// object of type `F` direct-non-list-initialized with
    /// `bsl::forward<F_PARAM>(f)`. Specify an `allocator` used to supply
    /// memory.
    ///
    /// `F` must be constructible from `bsl::forward<F_PARAM>(f)`.
    template <class F_PARAM>
    ExecutionUtil_UniqueTask(BSLS_COMPILERFEATURES_FORWARD_REF(F_PARAM) f,
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

// ====================
// struct ExecutionUtil
// ====================

/// Provides a namespace for utility functions used to execute function
/// objects on executors according to specified execution policies.
struct ExecutionUtil {
    // TYPES

    /// Provides a metafunction that determines, at compile time, the type
    /// returned by `bmqex::ExecutionUtil::execute`.
    ///
    /// Given:
    /// * A type `P`, that is an execution policy type (see
    ///:  `bmqex_executionpolicy`).
    ///:
    /// * A type `F`, such that `bsl::decay_t<F>` satisfies the
    ///   requirements of Destructible and MoveConstructible as specified
    ///   in the C++ standard, and such that and object `f` of type `F` is
    ///   callable as `DECAY_COPY(std::forward<F>(f))()`.
    ///
    /// The type of `bmqex::ExecutionUtil::ExecuteResult<P, F>::Type`
    /// expression is `void` for One-Way policies.
    template <class POLICY, class FUNCTION>
    struct ExecuteResult {
        // TYPES
        typedef
            typename ExecutionUtil_ExecuteResult<POLICY, FUNCTION>::Type Type;
    };

    // CLASS METHODS

    /// Execute the specified function object `f` according to the specified
    /// execution `policy`, as if by 'ExecutorTraits<EXECUTOR>::
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
    static typename ExecuteResult<ExecutionPolicy<EXECUTOR>, FUNCTION>::Type
    execute(const ExecutionPolicy<EXECUTOR>& policy,
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
    BSLA_MAYBE_UNUSED bsl::true_type voidResultTag) const
{
#ifdef BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES
    bsl::forward<F> (*d_function_p)();
#else
    (*d_function_p)();
#endif
}

template <class R, class F>
inline R ExecutionUtil_FunctionRef<R, F>::invoke(
    BSLA_MAYBE_UNUSED bsl::false_type voidResultTag) const
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
    BSLA_MAYBE_UNUSED bsl::true_type voidResultTag)
{
    bslmf::Util::moveIfSupported(d_functionValue.object().value())();
}

template <class R, class F>
inline R ExecutionUtil_OneOffFunction<R, F>::invoke(
    BSLA_MAYBE_UNUSED bsl::false_type voidResultTag)
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
// class ExecutionUtil_UniqueTask
// ------------------------------------

// CREATORS
template <class F>
template <class F_PARAM>
inline ExecutionUtil_UniqueTask<F>::ExecutionUtil_UniqueTask(
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
inline void ExecutionUtil_UniqueTask<F>::operator()()
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
inline void ExecutionUtil_UniqueTask<F>::wait() const BSLS_KEYWORD_NOEXCEPT
{
    d_latch.wait();
}

// --------------------
// struct ExecutionUtil
// --------------------

// CLASS METHODS
template <class EXECUTOR, class FUNCTION>
inline typename ExecutionUtil::ExecuteResult<ExecutionPolicy<EXECUTOR>,
                                             FUNCTION>::Type
ExecutionUtil::execute(const ExecutionPolicy<EXECUTOR>& policy,
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
        typedef ExecutionUtil_UniqueTask<typename bsl::decay<FUNCTION>::type>
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

}  // close package namespace
}  // close enterprise namespace

#endif
