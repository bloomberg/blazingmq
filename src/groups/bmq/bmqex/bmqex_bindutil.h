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

// bmqex_bindutil.h                                                   -*-C++-*-
#ifndef INCLUDED_BMQEX_BINDUTIL
#define INCLUDED_BMQEX_BINDUTIL

//@PURPOSE: Provides utility functions to bind functors to execution functions
//
//@CLASSES:
//  BindUtil: a namespace for utility functions to bind functors to ex. funcs.
//
//@DESCRIPTION:
// This component provides a struct, 'bmqex::BindUtil', that serves as a
// namespace for utility functions to bind function objects of arbitrary
// (template parameter) type to an invocation of an execution function.
//
// Consider the following example. You want to schedule a recurring event
// performing some heavy time consuming computations every 10 seconds. You are
// using the same instance of 'bdlmt::EventScheduler' across your application,
// so you don't want the event to be executed in the scheduler's thread. What
// you do is schedule an event that, when executed, will spawn another thread
// that will actually perform the work.
//..
//  void onDoHeavyStuff()
//      // This function is executed in the scheduler's thread.
//  {
//      // execute 'doHeavyStuff' in a separate thread (use system executor)
//      bmqex::ExecutionUtil::execute(
//                                 bmqex::ExecutionPolicyUtil::oneWay()
//                                                            .neverBlocking(),
//                                 &doHeavyStuff);
//  }
//
//  void doHeavyStuff()
//      // This function is executed in a separate thread.
//  {
//      // HEAVY STUFF ...
//  }
//..
//
//..
//  // schedule a recurring event
//  g_eventScheduler.scheduleRecurringEvent(&g_heavyStuffEvent,
//                                          bsls::TimeInterval(10.0), // 10 sec
//                                          &onDoHeavyStuff);
//..
//
// Having 2 functions ('onDoHeavyStuff' and 'doHeavyStuff') seems redundant.
// Using 'BindUtil::bindExecute' we can bind the invocation of the execution
// function (i.e. 'bmqex::ExecutionUtil::execute') directly into the
// scheduler's callback.
//..
//  // schedule a recurring event
//  g_eventScheduler.scheduleRecurringEvent(
//                             &g_heavyStuffEvent,
//                             bsls::TimeInterval(10.0), // 10 sec
//                             bmqex::BindUtil::bindExecute(
//                                 bmqex::ExecutionPolicyUtil::oneWay()
//                                                            .neverBlocking(),
//                                 &doHeavyStuff));
//..
//
// Note that the in the example above the scheduler's callback is expected to
// be a nullary function (i.e. to not take any argument). That, however, is not
// always the case. For instance, consider the following example. You are a
// user of a protocol that allows you to register a callback to be invoked
// each time an event happens.
//..
//  class Observable {
//      // Provides an protocol for an observable value.
//
//    public:
//      // CREATORS
//      virtual ~Observable() { }
//
//    public:
//      // MANIPULATORS
//      virtual void subscribe(const bsl::function<void(int)>& callback) = 0;
//          // Register a 'callback' to be invoked each time an event happens.
//          // The callback is passed the code of the event.
//  };
//..
// Like in the previous example, you want the callback to be executed in a
// separate thread. Without the bind utility the code may look like this.
//..
//  void onEvent(int eventCode)
//      // This function is invoked directly by 'Observable' in an unspecified
//      // thread.
//  {
//      // execute 'processEvent' in a separate thread (use system executor)
//      bmqex::ExecutionUtil::execute(
//                             bmqex::ExecutionPolicyUtil::oneWay()
//                                                        .neverBlocking(),
//                             bdlf::BindUtil::bind(&processEvent, eventCode));
//  }
//
//  void processEvent(int eventCode)
//      // This function is executed in a separate thread.
//  {
//      // PROCESS THE EVENT ...
//  }
//..
//
//..
//  // subscribe to notifications
//  g_observable_p->subscribe(&onEvent);
//..
//
// And again, using 'BindUtil::bindExecute' we can bind the invocation of the
// execution function (i.e. 'bmqex::ExecutionUtil::execute') directly into the
// notification callback.
//..
//  // subscribe to notifications
//  g_observable_p->subscribe(bmqex::BindUtil::bindExecute(
//                                 bmqex::ExecutionPolicyUtil::oneWay()
//                                                            .neverBlocking(),
//                                 &processEvent));
//..
//
// TBD: The bind wrapper's variadic call operator uses 'bdlf::BindUtil::bind'
//      to bind the function object to its arguments. Unfortunately that does
//      not work if the bound function object's call operator is non-const.
//      Should solve that.

#include <bmqex_executionutil.h>

// BDE
#include <bdlf_bind.h>
#include <bslalg_constructorproxy.h>
#include <bslma_default.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_decay.h>
#include <bslmf_invokeresult.h>
#include <bslmf_movableref.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bslmf_util.h>
#include <bsls_assert.h>
#include <bsls_compilerfeatures.h>

#if BSLS_COMPILERFEATURES_SIMULATE_CPP11_FEATURES
// Include version that can be compiled with C++03
// Generated on Tue Oct 15 18:12:34 2024
// Command line: sim_cpp11_features.pl bmqex_bindutil.h
#define COMPILING_BMQEX_BINDUTIL_H
#include <bmqex_bindutil_cpp03.h>
#undef COMPILING_BMQEX_BINDUTIL_H
#else

namespace BloombergLP {
namespace bmqex {

// ====================================
// struct BindUtil_DummyNullaryFunction
// ====================================

#if !BSLS_COMPILERFEATURES_SIMULATE_CPP11_FEATURES  // $var-args=9

/// Provides a dummy nullary function object which result type is the same
/// as the result type of the specified `FUNCTION` when invoked with
/// arguments of the specified `ARGS...` types.
template <class FUNCTION, class... ARGS>
struct BindUtil_DummyNullaryFunction {
    // TYPES

    /// Defines the result type of the call operator.
    typedef typename bsl::invoke_result<FUNCTION, ARGS...>::type ResultType;

    // ACCESSORS

    /// Not implemented.
    ResultType operator()() const;
};

#endif

// ==========================
// class BindUtil_BindWrapper
// ==========================

/// Provides a callable bind wrapper containing an execution policy and a
/// function object bound to the invocation of `ExecutionUtil::execute`.
///
/// `POLICY` must be an execution policy type (see `bmqex_executionpolicy`).
///
/// `FUNCTION` must meet the requirements of Destructible as specified in
/// the C++ standard.
template <class POLICY, class FUNCTION>
class BindUtil_BindWrapper {
  private:
    // PRIVATE DATA
    POLICY d_policy;

    bslalg::ConstructorProxy<FUNCTION> d_function;

  public:
    // CREATORS

    /// Create a `BindUtil_BindWrapper` object containing an execution
    /// policy of type `POLICY` direct-non-list-initialized with
    /// `bsl::forward<POLICY_PARAM>(policy)` and a function object of type
    /// `FUNCTION` direct-non-list-initialized with
    /// `bsl::forward<FUNCTION_PARAM>(function)`. Specify an `allocator`
    /// used to supply memory.
    ///
    /// `POLICY` must be constructible from
    /// `bsl::forward<POLICY_PARAM>(policy)`.
    ///
    /// `FUNCTION` must be constructible from
    /// `bsl::forward<FUNCTION_PARAM>(function)`.
    template <class POLICY_PARAM, class FUNCTION_PARAM>
    BindUtil_BindWrapper(BSLS_COMPILERFEATURES_FORWARD_REF(POLICY_PARAM)
                             policy,
                         BSLS_COMPILERFEATURES_FORWARD_REF(FUNCTION_PARAM)
                             function,
                         bslma::Allocator* allocator);

    /// Create a `BindUtil_BindWrapper` object having the same state as the
    /// specified `original` object. Optionally specify an `allocator` used
    /// to supply memory. If `allocator` is 0, the default memory allocator
    /// is used.
    BindUtil_BindWrapper(const BindUtil_BindWrapper& original,
                         bslma::Allocator*           allocator = 0);

    /// Create a `BindUtil_BindWrapper` object having the same state as the
    /// specified `original` object, leaving `original` in an unspecified
    /// state. Optionally specify an `allocator` used to supply memory. If
    /// `allocator` is 0, the default memory allocator is used.
    BindUtil_BindWrapper(bslmf::MovableRef<BindUtil_BindWrapper> original,
                         bslma::Allocator* allocator = 0);

  public:
    // ACCESSORS

    /// Call `ExecutionUtil::execute(p, f)` and return the result of that
    /// operation, where `p` is the contained execution policy and `f` is
    /// the contained function object.
    ///
    /// Given an object `f` of type `FUNCTION`, `f()` shall be a valid
    /// expression.
    typename ExecutionUtil::ExecuteResult<POLICY, FUNCTION>::Type
    operator()() const;

#if !BSLS_COMPILERFEATURES_SIMULATE_CPP11_FEATURES  // $var-args=8

    /// Call `ExecutionUtil::execute(p, bsl::move(f2))` and return the
    /// result of that operation, where `p` is the contained execution
    /// policy, `f` is the contained function object, `f1` is the result of
    /// `DECAY_COPY(f)` evaluated in this thread, `args1...` is the result
    /// of `DECAY_COPY(args)...` evaluated in this thread, and `f2` is a
    /// function object of unspecified type that, when called as `f2()`,
    /// performs `return f1(args1...)`.
    ///
    /// Given an object `f` of type `FUNCTION`, `f(DECAY_COPY(args)...)`
    /// shall be a valid expression.
    template <class ARG1, class... ARGS>
    typename ExecutionUtil::ExecuteResult<
        POLICY,
        BindUtil_DummyNullaryFunction<FUNCTION,
                                      typename bsl::decay<ARG1>::type,
                                      typename bsl::decay<ARGS>::type...> >::
        Type
        operator()(ARG1&& arg1, ARGS&&... args) const;
#endif

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(BindUtil_BindWrapper,
                                   bslma::UsesBslmaAllocator)
};

// ===============
// struct BindUtil
// ===============

/// Provides a namespace for utility functions to bind functors to execution
/// functions.
struct BindUtil {
    // CLASS METHODS

    /// Return a callable object of unspecified type containing an execution
    /// policy `p` of type `bsl::decay_t<POLICY>` direct-non-list-
    /// initialized with `bsl::forward<POLICY>(policy)` and a function
    /// object `f` of type `bsl::decay_t<FUNCTION>` direct-non-list-
    /// initialized with `bsl::forward<FUNCTION>(function)`, such that:
    /// * When called with no arguments, performs
    ///:  `ExecutionUtil::execute(p, f)` and returns the result of that
    ///:  operation.
    /// * When called with 1 or more arguments `args...` of type `ARGS...`,
    ///   performs `ExecutionUtil::execute(p, bsl::move(f2))` and return
    ///   the result of that operation, where `f1` is the result of
    ///   `DECAY_COPY(f)` evaluated in the calling thread, `args1...` is
    ///   the result of `DECAY_COPY(args)...` evaluated in the calling
    ///   thread, and `f2` is a function object of unspecified type that,
    ///   when called as `f2()`, performs `return f1(args1...)`.
    ///
    /// `POLICY` must be an execution policy type (see
    /// `bmqex_executionpolicy`).
    ///
    /// `bsl::decay_t<FUNCTION>` must meet the requirements of Destructible
    /// and MoveConstructible as specified in the C++ standard.
    template <class POLICY, class FUNCTION>
    static BindUtil_BindWrapper<typename bsl::decay<POLICY>::type,
                                typename bsl::decay<FUNCTION>::type>
        bindExecute(BSLS_COMPILERFEATURES_FORWARD_REF(POLICY) policy,
                    BSLS_COMPILERFEATURES_FORWARD_REF(FUNCTION) function);
};

// ============================================================================
//                           INLINE DEFINITIONS
// ============================================================================

// --------------------------
// class BindUtil_BindWrapper
// --------------------------

// CREATORS
template <class POLICY, class FUNCTION>
template <class POLICY_PARAM, class FUNCTION_PARAM>
inline BindUtil_BindWrapper<POLICY, FUNCTION>::BindUtil_BindWrapper(
    BSLS_COMPILERFEATURES_FORWARD_REF(POLICY_PARAM) policy,
    BSLS_COMPILERFEATURES_FORWARD_REF(FUNCTION_PARAM) function,
    bslma::Allocator* allocator)
: d_policy(policy)
, d_function(BSLS_COMPILERFEATURES_FORWARD(FUNCTION_PARAM, function),
             allocator)
{
    // NOTHING
}

template <class POLICY, class FUNCTION>
inline BindUtil_BindWrapper<POLICY, FUNCTION>::BindUtil_BindWrapper(
    const BindUtil_BindWrapper& original,
    bslma::Allocator*           allocator)
: d_policy(original.d_policy)
, d_function(original.d_function.object(), allocator)
{
    // NOTHING
}

template <class POLICY, class FUNCTION>
inline BindUtil_BindWrapper<POLICY, FUNCTION>::BindUtil_BindWrapper(
    bslmf::MovableRef<BindUtil_BindWrapper> original,
    bslma::Allocator*                       allocator)
: d_policy(bslmf::MovableRefUtil::move(
      bslmf::MovableRefUtil::access(original).d_policy))
, d_function(bslmf::MovableRefUtil::move(
                 bslmf::MovableRefUtil::access(original).d_function.object()),
             allocator)
{
    // NOTHING
}

// ACCESSORS
template <class POLICY, class FUNCTION>
inline typename ExecutionUtil::ExecuteResult<POLICY, FUNCTION>::Type
BindUtil_BindWrapper<POLICY, FUNCTION>::operator()() const
{
    return ExecutionUtil::execute(d_policy, d_function.object());
}

#if !BSLS_COMPILERFEATURES_SIMULATE_CPP11_FEATURES  // $var-args=8
template <class POLICY, class FUNCTION>
template <class ARG1, class... ARGS>
inline typename ExecutionUtil::ExecuteResult<
    POLICY,
    BindUtil_DummyNullaryFunction<FUNCTION,
                                  typename bsl::decay<ARG1>::type,
                                  typename bsl::decay<ARGS>::type...> >::Type
BindUtil_BindWrapper<POLICY, FUNCTION>::operator()(ARG1&& arg1,
                                                   ARGS&&... args) const
{
    typedef typename ExecutionUtil::ExecuteResult<
        POLICY,
        BindUtil_DummyNullaryFunction<FUNCTION,
                                      typename bsl::decay<ARG1>::type,
                                      typename bsl::decay<ARGS>::type...> >::
        Type ResultType;

    return ExecutionUtil::execute(d_policy,
                                  bdlf::BindUtil::bindR<ResultType>(
                                      d_function.object(),
                                      bslmf::Util::forward<ARG1>(arg1),
                                      bslmf::Util::forward<ARGS>(args)...));
}
#endif

// ---------------
// struct BindUtil
// ---------------

// CLASS METHODS
template <class POLICY, class FUNCTION>
inline BindUtil_BindWrapper<typename bsl::decay<POLICY>::type,
                            typename bsl::decay<FUNCTION>::type>
BindUtil::bindExecute(BSLS_COMPILERFEATURES_FORWARD_REF(POLICY) policy,
                      BSLS_COMPILERFEATURES_FORWARD_REF(FUNCTION) function)
{
    return BindUtil_BindWrapper<typename bsl::decay<POLICY>::type,
                                typename bsl::decay<FUNCTION>::type>(
        BSLS_COMPILERFEATURES_FORWARD(POLICY, policy),
        BSLS_COMPILERFEATURES_FORWARD(FUNCTION, function),
        bslma::Default::allocator());
}

}  // close package namespace
}  // close enterprise namespace

#endif  // End C++11 code

#endif
