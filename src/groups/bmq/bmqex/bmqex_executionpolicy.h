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

// bmqex_executionpolicy.h                                            -*-C++-*-
#ifndef INCLUDED_BMQEX_EXECUTIONPOLICY
#define INCLUDED_BMQEX_EXECUTIONPOLICY

//@PURPOSE: Provides an execution policy to customize execution functions.
//
//@CLASSES:
//  bmqex::ExecutionPolicy:     an execution policy
//  bmqex::ExecutionPolicyUtil: a namespace for factory functions
//
//@SEE ALSO:
//  bmqex_executionutil
//
//@DESCRIPTION
// This component provides a value-semantic class, 'bmqex::ExecutionPolicy',
// used to customize the behavior of execution functions provided by the
// 'bmqex_executionutil' component. A policy object contains the following
// properties:
//: o Directionality. Is One-Way (for more information see package
//:   documentation).
//:
//: o Blocking behavior. Is Never Blocking, Possibly Blocking or Always
//:   Blocking (for more information see package documentation).
//:
//: o The associated executor. Is the executor used by the execution function
//:   to submit function objects. By default, the system executor is used (see
//:   'bmqex_systemexecutor').
//:
//: o The associated allocator. Is the allocator used by the execution function
//:   for all internal purposes. By default, the currently installed default
//:   allocator is used.
//
/// Building policies
///-----------------
// Policies are lightweight immutable objects. Given a policy object 'p1'
// having some set of properties, a new policy 'p2' having a different set of
// properties can be "built" from the first one by applying transformation
// operations to it. For example, lets say we have a One-Way Never Blocking
// policy 'p', and we want to transform it to a One-Way Always Blocking policy.
// The corresponding expression looks like:
//..
//  p.alwaysBlocking();
//..
// Here, 'alwaysBlocking' is a member function performing a transformation
// operation, returning a new policy object having the same set of properties
// as the original one, except for the transformed property. An initial policy
// object can be obtained using factory methods defined in the
// 'ExecutionPolicyUtil' namespace, they mirror each transformation operation
// accessible on a policy object. For example:
//..
//  bmqex::ExecutionPolicyUtil::oneWay()
//                             .neverBlocking()
//                             .useExecutor(myExecutor)
//                             .useAllocator(myAllocator);
//..
// The result of the expression below is a One-Way Never Blocking execution
// policy that has its associated executor and its associated allocator set to
// the specified 'myExecutor' and 'myAllocator' respectively.
//
/// Using policies to execute function objects
///------------------------------------------
// Execution policies are meant to be used in pair with the
// 'bmqex_executionutil' component that provides a set of execution functions
// named 'execute', defined in the 'bmqex::ExecutionUtil' namespace. 'execute'
// accepts an execution policy and a function object to be executed according
// to the specified policy. One-Way execution functions return 'void'. The
// exact type of the execution function return value can be obtained at compile
// time using the 'bmqex::ExecutionUtil::ExecuteResult' metafunction (see
// 'bmqex_executionutil').
//
// Lets say we are to execute a function object 'myFunction' on an unspecified
// execution context associated with an executor object 'myExecutor'. If what
// we desire is "fire and forget", then we would use a One-Way policy:
//..
//  using bmqex;
//
//  auto myFunction = []() -> void { bsl::cout << "It works!\n"; };
//
//  ExecutionUtil::execute(ExecutionPolicyUtil::oneWay()
//                                             .neverBlocking()
//                                             .useExecutor(myExecutor),
//                         myFunction);
//..

#include <bmqex_executionproperty.h>
#include <bmqex_systemexecutor.h>

// BDE
#include <bslma_allocator.h>
#include <bslma_default.h>
#include <bslmf_issame.h>
#include <bslmf_movableref.h>
#include <bslmf_util.h>
#include <bsls_assert.h>
#include <bsls_keyword.h>

namespace BloombergLP {

namespace bmqex {

// =====================
// class ExecutionPolicy
// =====================

/// Provides an execution policy having its direction property and
/// associated executor type defined by the types of the specified
/// `DIRECTION` and `EXECUTOR` template parameters.
///
/// Note that instances of this class should not be created explicitly,
/// instead use the `ExecutionPolicyUtil` factory methods.
template <class DIRECTION = ExecutionProperty::OneWay,
          class EXECUTOR  = SystemExecutor>
class ExecutionPolicy {
  public:
    // TYPES

    /// Defines the type of the associated executor.
    typedef EXECUTOR ExecutorType;

    /// Provides a way to obtain the type of a One-Way execution policy
    /// otherwise having the same properties as this one.
    struct RebindOneWay {
        // TYPES
        typedef ExecutionPolicy<ExecutionProperty::OneWay, EXECUTOR> Type;
    };

    /// Provides a way to obtain the type of an execution policy having
    /// the specified associated `EXECUTOR_T` type and otherwise having
    /// the same properties as this one.
    template <class EXECUTOR_T>
    struct RebindExecutor {
        // TYPES
        typedef ExecutionPolicy<DIRECTION, EXECUTOR_T> Type;
    };

  public:
    // CLASS DATA

    /// Defines if this policy is One-Way.
    static BSLS_KEYWORD_CONSTEXPR_MEMBER bool k_IS_ONE_WAY =
        bsl::is_same<DIRECTION, ExecutionProperty::OneWay>::value;

  private:
    // PRIVATE DATA
    ExecutionProperty::Blocking d_blocking;

    EXECUTOR d_executor;

    bslma::Allocator* d_allocator_p;

    // FRIENDS
    template <class, class>
    friend class ExecutionPolicy;

  private:
    // NOT IMPLEMENTED
    ExecutionPolicy& operator=(const ExecutionPolicy&) BSLS_KEYWORD_DELETED;

  public:
    // CREATORS

    /// Create a `ExecutionPolicy` object having the specified `blocking`
    /// property, the specified associated `executor`, and the specified
    /// associated `allocator`.
    explicit BSLS_KEYWORD_CONSTEXPR_CPP14
    ExecutionPolicy(ExecutionProperty::Blocking blocking,
                    EXECUTOR                    executor,
                    bslma::Allocator*           allocator);

    /// Create a `ExecutionPolicy` object having the same properties as the
    /// specified `original` object.
    ///
    /// `EXECUTOR` shall be constructible from `OTHER_EXECUTOR`.
    template <class OTHER_EXECUTOR>
    BSLS_KEYWORD_CONSTEXPR_CPP14 ExecutionPolicy(
        const ExecutionPolicy<DIRECTION, OTHER_EXECUTOR>& original);

  public:
    // ACCESSORS

    /// Return a policy object having the same properties as this one,
    /// except that it is One-Way.
    typename RebindOneWay::Type oneWay() const;

    /// Return a policy object having the same properties as this one,
    /// except that it is Never Blocking.
    ExecutionPolicy neverBlocking() const;

    /// Return a policy object having the same properties as this one,
    /// except that it is Possibly Blocking.
    ExecutionPolicy possiblyBlocking() const;

    /// Return a policy object having the same properties as this one,
    /// except that it is Always Blocking.
    ExecutionPolicy alwaysBlocking() const;

    /// Return a policy object having the same properties as this one,
    /// except that it uses the specified `executor`.
    template <class EXECUTOR_PARAM>
    typename RebindExecutor<EXECUTOR_PARAM>::Type
    useExecutor(EXECUTOR_PARAM executor) const;

    /// Return a policy object having the same properties as this one,
    /// except that it uses the specified `allocator`.
    ExecutionPolicy useAllocator(bslma::Allocator* allocator) const;

    /// Return the associated blocking property.
    ExecutionProperty::Blocking blocking() const BSLS_KEYWORD_NOEXCEPT;

    /// Return the associated executor.
    const EXECUTOR& executor() const BSLS_KEYWORD_NOEXCEPT;

    /// Return the associated allocator.
    bslma::Allocator* allocator() const BSLS_KEYWORD_NOEXCEPT;
};

// ==========================
// struct ExecutionPolicyUtil
// ==========================

/// Provides a namespace for factory functions to create execution policies.
struct ExecutionPolicyUtil {
    // CLASS METHODS

    /// Return the default execution policy, that is a One-Way Possibly
    /// Blocking policy using a default-constructed `bmqex::SystemExecutor`
    /// and the currently installed default allocator.
    static ExecutionPolicy<> defaultPolicy();

    /// Return a One-Way execution policy as if by
    /// `return defaultPolicy().oneWay()`.
    static ExecutionPolicy<>::RebindOneWay::Type oneWay();

    /// Return a Never Blocking execution policy as if by
    /// `return defaultPolicy().neverBlocking()`.
    static ExecutionPolicy<> neverBlocking();

    /// Return a Possibly Blocking execution policy as if by
    /// `return defaultPolicy().possiblyBlocking()`.
    static ExecutionPolicy<> possiblyBlocking();

    /// Return a Always Blocking execution policy as if by
    /// `return defaultPolicy().alwaysBlocking()`.
    static ExecutionPolicy<> alwaysBlocking();

    /// Return an execution policy using the specified `executor` as if by
    /// `return defaultPolicy().useExecutor(bsl::move(executor))`.
    template <class EXECUTOR>
    static typename ExecutionPolicy<>::RebindExecutor<EXECUTOR>::Type
    useExecutor(EXECUTOR executor);

    /// Return an execution policy using the specified `allocator` as if by
    /// `return defaultPolicy().useAllocator(allocator)`.
    static ExecutionPolicy<> useAllocator(bslma::Allocator* allocator);
};

// ============================================================================
//                           INLINE DEFINITIONS
// ============================================================================

// ---------------------
// class ExecutionPolicy
// ---------------------

// CREATORS
template <class DIRECTION, class EXECUTOR>
inline BSLS_KEYWORD_CONSTEXPR_CPP14
ExecutionPolicy<DIRECTION, EXECUTOR>::ExecutionPolicy(
    ExecutionProperty::Blocking blocking,
    EXECUTOR                    executor,
    bslma::Allocator*           allocator)
: d_blocking(blocking)
, d_executor(bslmf::MovableRefUtil::move(executor))
, d_allocator_p(allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT(allocator);
}

template <class DIRECTION, class EXECUTOR>
template <class OTHER_EXECUTOR>
inline BSLS_KEYWORD_CONSTEXPR_CPP14
ExecutionPolicy<DIRECTION, EXECUTOR>::ExecutionPolicy(
    const ExecutionPolicy<DIRECTION, OTHER_EXECUTOR>& original)
: d_blocking(original.d_blocking)
, d_executor(original.d_executor)
, d_allocator_p(original.d_allocator_p)
{
    // NOTHING
}

// ACCESSORS
template <class DIRECTION, class EXECUTOR>
inline typename ExecutionPolicy<DIRECTION, EXECUTOR>::RebindOneWay::Type
ExecutionPolicy<DIRECTION, EXECUTOR>::oneWay() const
{
    return typename RebindOneWay::Type(d_blocking, d_executor, d_allocator_p);
}

template <class DIRECTION, class EXECUTOR>
inline ExecutionPolicy<DIRECTION, EXECUTOR>
ExecutionPolicy<DIRECTION, EXECUTOR>::neverBlocking() const
{
    return ExecutionPolicy(ExecutionProperty::e_NEVER_BLOCKING,
                           d_executor,
                           d_allocator_p);
}

template <class DIRECTION, class EXECUTOR>
inline ExecutionPolicy<DIRECTION, EXECUTOR>
ExecutionPolicy<DIRECTION, EXECUTOR>::possiblyBlocking() const
{
    return ExecutionPolicy(ExecutionProperty::e_POSSIBLY_BLOCKING,
                           d_executor,
                           d_allocator_p);
}

template <class DIRECTION, class EXECUTOR>
inline ExecutionPolicy<DIRECTION, EXECUTOR>
ExecutionPolicy<DIRECTION, EXECUTOR>::alwaysBlocking() const
{
    return ExecutionPolicy(ExecutionProperty::e_ALWAYS_BLOCKING,
                           d_executor,
                           d_allocator_p);
}

template <class DIRECTION, class EXECUTOR>
template <class EXECUTOR_PARAM>
inline typename ExecutionPolicy<DIRECTION, EXECUTOR>::template RebindExecutor<
    EXECUTOR_PARAM>::Type
ExecutionPolicy<DIRECTION, EXECUTOR>::useExecutor(
    EXECUTOR_PARAM executor) const
{
    return typename RebindExecutor<EXECUTOR_PARAM>::Type(
        d_blocking,
        bslmf::Util::moveIfSupported(executor),
        d_allocator_p);
}

template <class DIRECTION, class EXECUTOR>
inline ExecutionPolicy<DIRECTION, EXECUTOR>
ExecutionPolicy<DIRECTION, EXECUTOR>::useAllocator(
    bslma::Allocator* allocator) const
{
    // PRECONDITIONS
    BSLS_ASSERT(allocator);

    return ExecutionPolicy(d_blocking, d_executor, allocator);
}

template <class DIRECTION, class EXECUTOR>
inline ExecutionProperty::Blocking
ExecutionPolicy<DIRECTION, EXECUTOR>::blocking() const BSLS_KEYWORD_NOEXCEPT
{
    return d_blocking;
}

template <class DIRECTION, class EXECUTOR>
inline const EXECUTOR&
ExecutionPolicy<DIRECTION, EXECUTOR>::executor() const BSLS_KEYWORD_NOEXCEPT
{
    return d_executor;
}

template <class DIRECTION, class EXECUTOR>
inline bslma::Allocator*
ExecutionPolicy<DIRECTION, EXECUTOR>::allocator() const BSLS_KEYWORD_NOEXCEPT
{
    return d_allocator_p;
}

// -------------------------
// class ExecutionPolicyUtil
// -------------------------

inline ExecutionPolicy<> ExecutionPolicyUtil::defaultPolicy()
{
    return ExecutionPolicy<>(ExecutionProperty::e_POSSIBLY_BLOCKING,
                             SystemExecutor(),
                             bslma::Default::allocator());
}

inline ExecutionPolicy<>::RebindOneWay::Type ExecutionPolicyUtil::oneWay()
{
    return defaultPolicy().oneWay();
}

inline ExecutionPolicy<> ExecutionPolicyUtil::neverBlocking()
{
    return defaultPolicy().neverBlocking();
}

inline ExecutionPolicy<> ExecutionPolicyUtil::possiblyBlocking()
{
    return defaultPolicy().possiblyBlocking();
}

inline ExecutionPolicy<> ExecutionPolicyUtil::alwaysBlocking()
{
    return defaultPolicy().alwaysBlocking();
}

template <class EXECUTOR>
inline typename ExecutionPolicy<>::RebindExecutor<EXECUTOR>::Type
ExecutionPolicyUtil::useExecutor(EXECUTOR executor)
{
    return defaultPolicy().useExecutor<EXECUTOR>(
        bslmf::Util::moveIfSupported(executor));
}

inline ExecutionPolicy<>
ExecutionPolicyUtil::useAllocator(bslma::Allocator* allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT(allocator);

    return defaultPolicy().useAllocator(allocator);
}

}  // close package namespace
}  // close enterprise namespace

#endif
