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

// mwcu_weakmemfn.h                                                   -*-C++-*-
#ifndef INCLUDED_MWCU_WEAKMEMFN
#define INCLUDED_MWCU_WEAKMEMFN

//@PURPOSE: Provides a wrapper type for invoking member functions on a weak_ptr
//
//@CLASSES:
//  mwcu::WeakMemFn:         mem. fn. wrapper
//  mwcu::WeakMemFnInstance: mem. fn. wrapper with embedded instance pointer
//  mwcu::WeakMemFnResult:   result of a mem. fn. wrapper invocation
//  mwcu::WeakMemFnUtil:     utility for constructing wrapper objects
//
//@DESCRIPTION:
// This component provides a member function pointer wrapper that wraps a
// member function pointer such that it can be invoked safely on a weak object
// pointer in syntactically the same manner as a free function.  Two wrappers,
// each supporting member function pointers that accept from zero to nine
// arguments are provided, as well as a utility to create such wrappers.
//
// The first wrapper, 'mwcu::WeakMemFn', contains a member function pointer and
// must be invoked with the first argument being a weak pointer to the instance
// on which the function should be invoked, with the remaining arguments passed
// as arguments to the member function; that is, a wrapper 'weakMemFn'
// containing a pointer to a given 'memberFunction' can be invoked as
// 'weakMemFn(weakObjPtr, args...)', which does result in the following
// behavior.  If the weak pointer is still valid, i.e. 'weakObjPtr.lock()'
// produces a non-empty shared pointer 'objPtr', the member function is invoked
// on that shared pointer as if by 'objPtr->memberFunction(args...)'.
// Otherwise, the member function is not invoked.
//
// The second wrapper, 'mwcu::WeakMemFnInstance', contains both the member
// function pointer and a weak pointer to an instance of the type which
// contains the member, and is invoked with arguments which are passed as
// arguments to the member function; that is, a wrapper 'weakMemFnInstance'
// containing pointers to both, a given 'memberFunction' and to an 'object'
// instance can be invoked as 'weakMemFnInstance(args...)', which results in
// the exact same behavior as described for 'mwcu::WeakMemFn'.
//
// The result of a 'mwcu::WeakMemFn' or 'mwcu::WeakMemFnInstance' wrapper
// invocation is represented by an instance of 'mwcu::WeakMemFnResult'.  This
// class is semantically similar to 'bdlb::NullableValue' (or 'std::optional'),
// but also supports 'void' and reference value types.  Like a nullable value,
// the state of such object may be either null, or not null (i.e. contain the
// invocation result), depending on whether the target object's weak pointer
// has expired or not.
//
// Finally, the 'mwcu::WeakMemFnUtil' utility class provides utility functions
// for constructing 'mwcu::WeakMemFn' and 'mwcu::WeakMemFnInstance' objects.
//
///'mwcu::WeakMemFn<PROT>' call operator
///-------------------------------------
// 'mwcu::WeakMemFn' provides a call operator that, in C++11, would be defined
// and behave exactly as follows:
//..
//  template <class OBJ, class... ARGS>
//  WeakMemFnResult<bsl::invoke_result_t<PROT>>
//  operator()(const bsl::weak_ptr<OBJ>& obj,
//             ARGS&&...                 args) const;
//      // If 'obj' has not expired, return the result of
//      // '((*obj.lock()).*d_memFn)(bsl::forward<ARGS>(args)...)' wrapped in
//      // an instance of 'mwcu::WeakMemFnResult', where 'd_memFn' is the
//      // stored member function pointer. Otherwise, return a null
//      // 'mwcu::WeakMemFnResult' object.
//..
// The actual behavior of the call operator is virtually identical to the
// description below, except that the maximum number of arguments is limited
// to 9.
//
///'mwcu::WeakMemFnInstance<PROT>' call operator
///---------------------------------------------
// 'mwcu::WeakMemFnInstance' provides a call operator that, in C++11, would be
// defined and behave exactly as follows:
//..
//  template <class... ARGS>
//  WeakMemFnResult<bsl::invoke_result_t<PROT>>
//  operator()(ARGS&&... args) const;
//      // If the stored weak pointer 'd_obj' has not expired, return the
//      // result of '((*d_obj.lock()).*d_memFn)(bsl::forward<ARGS>(args)...)'
//      // wrapped in an instance of 'mwcu::WeakMemFnResult', where 'd_memFn'
//      // is the stored member function pointer. Otherwise, return a null
//      // 'mwcu::WeakMemFnResult' object.
//..
// The actual behavior of the call operator is virtually identical to the
// description below, except that the maximum number of arguments is limited
// to 9.
//
/// Usage
///-----
// Lets say there is an 'EventGenerator' class, that has a 'callOnEvent()'
// member function taking a callback, that is invoked from an unspecified
// thread of execution each time there is an "event".  For the sake of this
// example lets assume that 'EventGenerator' does not allow to unsubscribe
// individual callbacks.
//
// You have to implement a listener class (lets call it 'EventListener'), that
// receives and processes events generated by the 'EventGenerator'.  Multiple
// listeners may be created and destroyed during the application lifetime, and
// you do want listeners to release managed resources as soon as they are no
// longer needed.
//
// One way to approach such task is to make listeners "shared_from_this"
// objects and, when subscribing to events, bind the listener's event-
// processing member function to a weak pointer to self, in such a way that the
// member function only gets invoked if the object itself is still alive.
//
// Here is a possible implementation of 'EventListener' that makes use of this
// component.
//..
//  class EventListener : public bsl::enable_shared_from_this<EventListener> {
//
//    private:
//      // PRIVATE DATA
//      EventGenerator *eventGenerator_p;
//
//    private:
//      // PRIVATE CREATORS
//      explicit EventListener(EventGenerator *eventGenerator)
//      : eventGenerator_p(eventGenerator)
//      { }
//
//      // PRIVATE MANIPULATORS
//      void processEvent(int event)
//          // Event handler. Invoked only if this instance of 'EventListener'
//          // is alive.
//      {
//          // do something ...
//      }
//
//      void start()
//      {
//          // subscribe to events
//          eventGenerator_p->callOnEvent(
//                            WeakMemFnUtil::weakMemFn(
//                                           &EventListener::processEvent,
//                                           weak_from_this()));
//      }
//
//    public:
//      // CLASS METHODS
//      static
//      bsl::shared_ptr<EventListener> create(EventGenerator *eventGenerator)
//      {
//          auto instance = bsl::make_shared<EventListener>(eventGenerator);
//
//          instance->start();
//          return instance;
//      }
//  };
//..
//
// TBD:
//-----
//: o Provide a move c-tor for 'mwcu::WeakMemFnResult' that does not take an
//:   allocator parameter, but borrows it's allocator from the moved-from
//:   object?

// MWC

// BDE
#include <bdlb_nullablevalue.h>
#include <bsl_memory.h>
#include <bslalg_constructorproxy.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_forwardingtype.h>
#include <bslmf_memberfunctionpointertraits.h>
#include <bslmf_movableref.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bslmf_typelist.h>
#include <bsls_assert.h>
#include <bsls_compilerfeatures.h>
#include <bsls_keyword.h>
#include <bsls_util.h>

namespace BloombergLP {

namespace mwcu {

// FORWARD DECLARATION
template <class>
class WeakMemFnResult;

// ==========================
// struct WeakMemFn_Invocable
// ==========================

/// Provides a call operator for the derived class `mwcu::WeakMemFn`, such
/// that its call signature is compatible with the specified return type
/// `RET` and the specified argument types `ARGS`.
template <class WEAKMEMFN, class RET, class ARGS>
struct WeakMemFn_Invocable {
};

/// Provides a specialization of `WeakMemFn_Invocable` for a non-void return
/// types and 0 argument types.
template <class WEAKMEMFN, class RET>
struct WeakMemFn_Invocable<WEAKMEMFN, RET, bslmf::TypeList0> {
    // ACCESSORS
    template <class OBJ>
    WeakMemFnResult<RET> operator()(const bsl::weak_ptr<OBJ>& obj) const;
};

/// Provides a specialization of `WeakMemFn_Invocable` for a non-void return
/// types and 1 argument types.
template <class WEAKMEMFN, class RET, class ARG1>
struct WeakMemFn_Invocable<WEAKMEMFN, RET, bslmf::TypeList1<ARG1> > {
    // ACCESSORS
    template <class OBJ>
    WeakMemFnResult<RET>
    operator()(const bsl::weak_ptr<OBJ>&                  obj,
               typename bslmf::ForwardingType<ARG1>::Type arg1) const;
};

/// Provides a specialization of `WeakMemFn_Invocable` for a non-void return
/// types and 2 argument types.
template <class WEAKMEMFN, class RET, class ARG1, class ARG2>
struct WeakMemFn_Invocable<WEAKMEMFN, RET, bslmf::TypeList2<ARG1, ARG2> > {
    // ACCESSORS
    template <class OBJ>
    WeakMemFnResult<RET>
    operator()(const bsl::weak_ptr<OBJ>&                  obj,
               typename bslmf::ForwardingType<ARG1>::Type arg1,
               typename bslmf::ForwardingType<ARG2>::Type arg2) const;
};

/// Provides a specialization of `WeakMemFn_Invocable` for a non-void return
/// types and 3 argument types.
template <class WEAKMEMFN, class RET, class ARG1, class ARG2, class ARG3>
struct WeakMemFn_Invocable<WEAKMEMFN,
                           RET,
                           bslmf::TypeList3<ARG1, ARG2, ARG3> > {
    // ACCESSORS
    template <class OBJ>
    WeakMemFnResult<RET>
    operator()(const bsl::weak_ptr<OBJ>&                  obj,
               typename bslmf::ForwardingType<ARG1>::Type arg1,
               typename bslmf::ForwardingType<ARG2>::Type arg2,
               typename bslmf::ForwardingType<ARG3>::Type arg3) const;
};

/// Provides a specialization of `WeakMemFn_Invocable` for a non-void return
/// types and 4 argument types.
template <class WEAKMEMFN,
          class RET,
          class ARG1,
          class ARG2,
          class ARG3,
          class ARG4>
struct WeakMemFn_Invocable<WEAKMEMFN,
                           RET,
                           bslmf::TypeList4<ARG1, ARG2, ARG3, ARG4> > {
    // ACCESSORS
    template <class OBJ>
    WeakMemFnResult<RET>
    operator()(const bsl::weak_ptr<OBJ>&                  obj,
               typename bslmf::ForwardingType<ARG1>::Type arg1,
               typename bslmf::ForwardingType<ARG2>::Type arg2,
               typename bslmf::ForwardingType<ARG3>::Type arg3,
               typename bslmf::ForwardingType<ARG4>::Type arg4) const;
};

/// Provides a specialization of `WeakMemFn_Invocable` for a non-void return
/// types and 5 argument types.
template <class WEAKMEMFN,
          class RET,
          class ARG1,
          class ARG2,
          class ARG3,
          class ARG4,
          class ARG5>
struct WeakMemFn_Invocable<WEAKMEMFN,
                           RET,
                           bslmf::TypeList5<ARG1, ARG2, ARG3, ARG4, ARG5> > {
    // ACCESSORS
    template <class OBJ>
    WeakMemFnResult<RET>
    operator()(const bsl::weak_ptr<OBJ>&                  obj,
               typename bslmf::ForwardingType<ARG1>::Type arg1,
               typename bslmf::ForwardingType<ARG2>::Type arg2,
               typename bslmf::ForwardingType<ARG3>::Type arg3,
               typename bslmf::ForwardingType<ARG4>::Type arg4,
               typename bslmf::ForwardingType<ARG5>::Type arg5) const;
};

/// Provides a specialization of `WeakMemFn_Invocable` for a non-void return
/// types and 6 argument types.
template <class WEAKMEMFN,
          class RET,
          class ARG1,
          class ARG2,
          class ARG3,
          class ARG4,
          class ARG5,
          class ARG6>
struct WeakMemFn_Invocable<
    WEAKMEMFN,
    RET,
    bslmf::TypeList6<ARG1, ARG2, ARG3, ARG4, ARG5, ARG6> > {
    // ACCESSORS
    template <class OBJ>
    WeakMemFnResult<RET>
    operator()(const bsl::weak_ptr<OBJ>&                  obj,
               typename bslmf::ForwardingType<ARG1>::Type arg1,
               typename bslmf::ForwardingType<ARG2>::Type arg2,
               typename bslmf::ForwardingType<ARG3>::Type arg3,
               typename bslmf::ForwardingType<ARG4>::Type arg4,
               typename bslmf::ForwardingType<ARG5>::Type arg5,
               typename bslmf::ForwardingType<ARG6>::Type arg6) const;
};

/// Provides a specialization of `WeakMemFn_Invocable` for a non-void return
/// types and 7 argument types.
template <class WEAKMEMFN,
          class RET,
          class ARG1,
          class ARG2,
          class ARG3,
          class ARG4,
          class ARG5,
          class ARG6,
          class ARG7>
struct WeakMemFn_Invocable<
    WEAKMEMFN,
    RET,
    bslmf::TypeList7<ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7> > {
    // ACCESSORS
    template <class OBJ>
    WeakMemFnResult<RET>
    operator()(const bsl::weak_ptr<OBJ>&                  obj,
               typename bslmf::ForwardingType<ARG1>::Type arg1,
               typename bslmf::ForwardingType<ARG2>::Type arg2,
               typename bslmf::ForwardingType<ARG3>::Type arg3,
               typename bslmf::ForwardingType<ARG4>::Type arg4,
               typename bslmf::ForwardingType<ARG5>::Type arg5,
               typename bslmf::ForwardingType<ARG6>::Type arg6,
               typename bslmf::ForwardingType<ARG7>::Type arg7) const;
};

/// Provides a specialization of `WeakMemFn_Invocable` for a non-void return
/// types and 8 argument types.
template <class WEAKMEMFN,
          class RET,
          class ARG1,
          class ARG2,
          class ARG3,
          class ARG4,
          class ARG5,
          class ARG6,
          class ARG7,
          class ARG8>
struct WeakMemFn_Invocable<
    WEAKMEMFN,
    RET,
    bslmf::TypeList8<ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8> > {
    // ACCESSORS
    template <class OBJ>
    WeakMemFnResult<RET>
    operator()(const bsl::weak_ptr<OBJ>&                  obj,
               typename bslmf::ForwardingType<ARG1>::Type arg1,
               typename bslmf::ForwardingType<ARG2>::Type arg2,
               typename bslmf::ForwardingType<ARG3>::Type arg3,
               typename bslmf::ForwardingType<ARG4>::Type arg4,
               typename bslmf::ForwardingType<ARG5>::Type arg5,
               typename bslmf::ForwardingType<ARG6>::Type arg6,
               typename bslmf::ForwardingType<ARG7>::Type arg7,
               typename bslmf::ForwardingType<ARG8>::Type arg8) const;
};

/// Provides a specialization of `WeakMemFn_Invocable` for a non-void return
/// types and 9 argument types.
template <class WEAKMEMFN,
          class RET,
          class ARG1,
          class ARG2,
          class ARG3,
          class ARG4,
          class ARG5,
          class ARG6,
          class ARG7,
          class ARG8,
          class ARG9>
struct WeakMemFn_Invocable<
    WEAKMEMFN,
    RET,
    bslmf::TypeList9<ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9> > {
    // ACCESSORS
    template <class OBJ>
    WeakMemFnResult<RET>
    operator()(const bsl::weak_ptr<OBJ>&                  obj,
               typename bslmf::ForwardingType<ARG1>::Type arg1,
               typename bslmf::ForwardingType<ARG2>::Type arg2,
               typename bslmf::ForwardingType<ARG3>::Type arg3,
               typename bslmf::ForwardingType<ARG4>::Type arg4,
               typename bslmf::ForwardingType<ARG5>::Type arg5,
               typename bslmf::ForwardingType<ARG6>::Type arg6,
               typename bslmf::ForwardingType<ARG7>::Type arg7,
               typename bslmf::ForwardingType<ARG8>::Type arg8,
               typename bslmf::ForwardingType<ARG9>::Type arg9) const;
};

/// Provides a specialization of `WeakMemFn_Invocable` for a `void` return
/// type and 0 argument types.
template <class WEAKMEMFN>
struct WeakMemFn_Invocable<WEAKMEMFN, void, bslmf::TypeList0> {
    // ACCESSORS
    template <class OBJ>
    WeakMemFnResult<void> operator()(const bsl::weak_ptr<OBJ>& obj) const;
};

/// Provides a specialization of `WeakMemFn_Invocable` for a `void` return
/// type and 1 argument types.
template <class WEAKMEMFN, class ARG1>
struct WeakMemFn_Invocable<WEAKMEMFN, void, bslmf::TypeList1<ARG1> > {
    // ACCESSORS
    template <class OBJ>
    WeakMemFnResult<void>
    operator()(const bsl::weak_ptr<OBJ>&                  obj,
               typename bslmf::ForwardingType<ARG1>::Type arg1) const;
};

/// Provides a specialization of `WeakMemFn_Invocable` for a `void` return
/// type and 2 argument types.
template <class WEAKMEMFN, class ARG1, class ARG2>
struct WeakMemFn_Invocable<WEAKMEMFN, void, bslmf::TypeList2<ARG1, ARG2> > {
    // ACCESSORS
    template <class OBJ>
    WeakMemFnResult<void>
    operator()(const bsl::weak_ptr<OBJ>&                  obj,
               typename bslmf::ForwardingType<ARG1>::Type arg1,
               typename bslmf::ForwardingType<ARG2>::Type arg2) const;
};

/// Provides a specialization of `WeakMemFn_Invocable` for a `void` return
/// type and 3 argument types.
template <class WEAKMEMFN, class ARG1, class ARG2, class ARG3>
struct WeakMemFn_Invocable<WEAKMEMFN,
                           void,
                           bslmf::TypeList3<ARG1, ARG2, ARG3> > {
    // ACCESSORS
    template <class OBJ>
    WeakMemFnResult<void>
    operator()(const bsl::weak_ptr<OBJ>&                  obj,
               typename bslmf::ForwardingType<ARG1>::Type arg1,
               typename bslmf::ForwardingType<ARG2>::Type arg2,
               typename bslmf::ForwardingType<ARG3>::Type arg3) const;
};

/// Provides a specialization of `WeakMemFn_Invocable` for a `void` return
/// type and 4 argument types.
template <class WEAKMEMFN, class ARG1, class ARG2, class ARG3, class ARG4>
struct WeakMemFn_Invocable<WEAKMEMFN,
                           void,
                           bslmf::TypeList4<ARG1, ARG2, ARG3, ARG4> > {
    // ACCESSORS
    template <class OBJ>
    WeakMemFnResult<void>
    operator()(const bsl::weak_ptr<OBJ>&                  obj,
               typename bslmf::ForwardingType<ARG1>::Type arg1,
               typename bslmf::ForwardingType<ARG2>::Type arg2,
               typename bslmf::ForwardingType<ARG3>::Type arg3,
               typename bslmf::ForwardingType<ARG4>::Type arg4) const;
};

/// Provides a specialization of `WeakMemFn_Invocable` for a `void` return
/// type and 5 argument types.
template <class WEAKMEMFN,
          class ARG1,
          class ARG2,
          class ARG3,
          class ARG4,
          class ARG5>
struct WeakMemFn_Invocable<WEAKMEMFN,
                           void,
                           bslmf::TypeList5<ARG1, ARG2, ARG3, ARG4, ARG5> > {
    // ACCESSORS
    template <class OBJ>
    WeakMemFnResult<void>
    operator()(const bsl::weak_ptr<OBJ>&                  obj,
               typename bslmf::ForwardingType<ARG1>::Type arg1,
               typename bslmf::ForwardingType<ARG2>::Type arg2,
               typename bslmf::ForwardingType<ARG3>::Type arg3,
               typename bslmf::ForwardingType<ARG4>::Type arg4,
               typename bslmf::ForwardingType<ARG5>::Type arg5) const;
};

/// Provides a specialization of `WeakMemFn_Invocable` for a `void` return
/// type and 6 argument types.
template <class WEAKMEMFN,
          class ARG1,
          class ARG2,
          class ARG3,
          class ARG4,
          class ARG5,
          class ARG6>
struct WeakMemFn_Invocable<
    WEAKMEMFN,
    void,
    bslmf::TypeList6<ARG1, ARG2, ARG3, ARG4, ARG5, ARG6> > {
    // ACCESSORS
    template <class OBJ>
    WeakMemFnResult<void>
    operator()(const bsl::weak_ptr<OBJ>&                  obj,
               typename bslmf::ForwardingType<ARG1>::Type arg1,
               typename bslmf::ForwardingType<ARG2>::Type arg2,
               typename bslmf::ForwardingType<ARG3>::Type arg3,
               typename bslmf::ForwardingType<ARG4>::Type arg4,
               typename bslmf::ForwardingType<ARG5>::Type arg5,
               typename bslmf::ForwardingType<ARG6>::Type arg6) const;
};

/// Provides a specialization of `WeakMemFn_Invocable` for a `void` return
/// type and 7 argument types.
template <class WEAKMEMFN,
          class ARG1,
          class ARG2,
          class ARG3,
          class ARG4,
          class ARG5,
          class ARG6,
          class ARG7>
struct WeakMemFn_Invocable<
    WEAKMEMFN,
    void,
    bslmf::TypeList7<ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7> > {
    // ACCESSORS
    template <class OBJ>
    WeakMemFnResult<void>
    operator()(const bsl::weak_ptr<OBJ>&                  obj,
               typename bslmf::ForwardingType<ARG1>::Type arg1,
               typename bslmf::ForwardingType<ARG2>::Type arg2,
               typename bslmf::ForwardingType<ARG3>::Type arg3,
               typename bslmf::ForwardingType<ARG4>::Type arg4,
               typename bslmf::ForwardingType<ARG5>::Type arg5,
               typename bslmf::ForwardingType<ARG6>::Type arg6,
               typename bslmf::ForwardingType<ARG7>::Type arg7) const;
};

/// Provides a specialization of `WeakMemFn_Invocable` for a `void` return
/// type and 8 argument types.
template <class WEAKMEMFN,
          class ARG1,
          class ARG2,
          class ARG3,
          class ARG4,
          class ARG5,
          class ARG6,
          class ARG7,
          class ARG8>
struct WeakMemFn_Invocable<
    WEAKMEMFN,
    void,
    bslmf::TypeList8<ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8> > {
    // ACCESSORS
    template <class OBJ>
    WeakMemFnResult<void>
    operator()(const bsl::weak_ptr<OBJ>&                  obj,
               typename bslmf::ForwardingType<ARG1>::Type arg1,
               typename bslmf::ForwardingType<ARG2>::Type arg2,
               typename bslmf::ForwardingType<ARG3>::Type arg3,
               typename bslmf::ForwardingType<ARG4>::Type arg4,
               typename bslmf::ForwardingType<ARG5>::Type arg5,
               typename bslmf::ForwardingType<ARG6>::Type arg6,
               typename bslmf::ForwardingType<ARG7>::Type arg7,
               typename bslmf::ForwardingType<ARG8>::Type arg8) const;
};

/// Provides a specialization of `WeakMemFn_Invocable` for a `void` return
/// type and 9 argument types.
template <class WEAKMEMFN,
          class ARG1,
          class ARG2,
          class ARG3,
          class ARG4,
          class ARG5,
          class ARG6,
          class ARG7,
          class ARG8,
          class ARG9>
struct WeakMemFn_Invocable<
    WEAKMEMFN,
    void,
    bslmf::TypeList9<ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9> > {
    // ACCESSORS
    template <class OBJ>
    WeakMemFnResult<void>
    operator()(const bsl::weak_ptr<OBJ>&                  obj,
               typename bslmf::ForwardingType<ARG1>::Type arg1,
               typename bslmf::ForwardingType<ARG2>::Type arg2,
               typename bslmf::ForwardingType<ARG3>::Type arg3,
               typename bslmf::ForwardingType<ARG4>::Type arg4,
               typename bslmf::ForwardingType<ARG5>::Type arg5,
               typename bslmf::ForwardingType<ARG6>::Type arg6,
               typename bslmf::ForwardingType<ARG7>::Type arg7,
               typename bslmf::ForwardingType<ARG8>::Type arg8,
               typename bslmf::ForwardingType<ARG9>::Type arg9) const;
};

// ==================================
// struct WeakMemFnInstance_Invocable
// ==================================

/// Provides a call operator for the derived class
/// `mwcu::WeakMemFnInstance`, such that its call signature is compatible
/// with the specified return type `RET` and the specified argument types
/// `ARGS`.
template <class WEAKMEMFN, class RET, class ARGS>
struct WeakMemFnInstance_Invocable {
};

/// Provides a specialization of `WeakMemFnInstance_Invocable` for 0
/// argument types.
template <class WEAKMEMFN, class RET>
struct WeakMemFnInstance_Invocable<WEAKMEMFN, RET, bslmf::TypeList0> {
    // ACCESSORS
    WeakMemFnResult<RET> operator()() const;
};

/// Provides a specialization of `WeakMemFnInstance_Invocable` for 1
/// argument types.
template <class WEAKMEMFN, class RET, class ARG1>
struct WeakMemFnInstance_Invocable<WEAKMEMFN, RET, bslmf::TypeList1<ARG1> > {
    // ACCESSORS
    WeakMemFnResult<RET>
    operator()(typename bslmf::ForwardingType<ARG1>::Type arg1) const;
};

/// Provides a specialization of `WeakMemFnInstance_Invocable` for 2
/// argument types.
template <class WEAKMEMFN, class RET, class ARG1, class ARG2>
struct WeakMemFnInstance_Invocable<WEAKMEMFN,
                                   RET,
                                   bslmf::TypeList2<ARG1, ARG2> > {
    // ACCESSORS
    WeakMemFnResult<RET>
    operator()(typename bslmf::ForwardingType<ARG1>::Type arg1,
               typename bslmf::ForwardingType<ARG2>::Type arg2) const;
};

/// Provides a specialization of `WeakMemFnInstance_Invocable` for 3
/// argument types.
template <class WEAKMEMFN, class RET, class ARG1, class ARG2, class ARG3>
struct WeakMemFnInstance_Invocable<WEAKMEMFN,
                                   RET,
                                   bslmf::TypeList3<ARG1, ARG2, ARG3> > {
    // ACCESSORS
    WeakMemFnResult<RET>
    operator()(typename bslmf::ForwardingType<ARG1>::Type arg1,
               typename bslmf::ForwardingType<ARG2>::Type arg2,
               typename bslmf::ForwardingType<ARG3>::Type arg3) const;
};

/// Provides a specialization of `WeakMemFnInstance_Invocable` for 4
/// argument types.
template <class WEAKMEMFN,
          class RET,
          class ARG1,
          class ARG2,
          class ARG3,
          class ARG4>
struct WeakMemFnInstance_Invocable<WEAKMEMFN,
                                   RET,
                                   bslmf::TypeList4<ARG1, ARG2, ARG3, ARG4> > {
    // ACCESSORS
    WeakMemFnResult<RET>
    operator()(typename bslmf::ForwardingType<ARG1>::Type arg1,
               typename bslmf::ForwardingType<ARG2>::Type arg2,
               typename bslmf::ForwardingType<ARG3>::Type arg3,
               typename bslmf::ForwardingType<ARG4>::Type arg4) const;
};

/// Provides a specialization of `WeakMemFnInstance_Invocable` for 5
/// argument types.
template <class WEAKMEMFN,
          class RET,
          class ARG1,
          class ARG2,
          class ARG3,
          class ARG4,
          class ARG5>
struct WeakMemFnInstance_Invocable<
    WEAKMEMFN,
    RET,
    bslmf::TypeList5<ARG1, ARG2, ARG3, ARG4, ARG5> > {
    // ACCESSORS
    WeakMemFnResult<RET>
    operator()(typename bslmf::ForwardingType<ARG1>::Type arg1,
               typename bslmf::ForwardingType<ARG2>::Type arg2,
               typename bslmf::ForwardingType<ARG3>::Type arg3,
               typename bslmf::ForwardingType<ARG4>::Type arg4,
               typename bslmf::ForwardingType<ARG5>::Type arg5) const;
};

/// Provides a specialization of `WeakMemFnInstance_Invocable` for 6
/// argument types.
template <class WEAKMEMFN,
          class RET,
          class ARG1,
          class ARG2,
          class ARG3,
          class ARG4,
          class ARG5,
          class ARG6>
struct WeakMemFnInstance_Invocable<
    WEAKMEMFN,
    RET,
    bslmf::TypeList6<ARG1, ARG2, ARG3, ARG4, ARG5, ARG6> > {
    // ACCESSORS
    WeakMemFnResult<RET>
    operator()(typename bslmf::ForwardingType<ARG1>::Type arg1,
               typename bslmf::ForwardingType<ARG2>::Type arg2,
               typename bslmf::ForwardingType<ARG3>::Type arg3,
               typename bslmf::ForwardingType<ARG4>::Type arg4,
               typename bslmf::ForwardingType<ARG5>::Type arg5,
               typename bslmf::ForwardingType<ARG6>::Type arg6) const;
};

/// Provides a specialization of `WeakMemFnInstance_Invocable` for 7
/// argument types.
template <class WEAKMEMFN,
          class RET,
          class ARG1,
          class ARG2,
          class ARG3,
          class ARG4,
          class ARG5,
          class ARG6,
          class ARG7>
struct WeakMemFnInstance_Invocable<
    WEAKMEMFN,
    RET,
    bslmf::TypeList7<ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7> > {
    // ACCESSORS
    WeakMemFnResult<RET>
    operator()(typename bslmf::ForwardingType<ARG1>::Type arg1,
               typename bslmf::ForwardingType<ARG2>::Type arg2,
               typename bslmf::ForwardingType<ARG3>::Type arg3,
               typename bslmf::ForwardingType<ARG4>::Type arg4,
               typename bslmf::ForwardingType<ARG5>::Type arg5,
               typename bslmf::ForwardingType<ARG6>::Type arg6,
               typename bslmf::ForwardingType<ARG7>::Type arg7) const;
};

/// Provides a specialization of `WeakMemFnInstance_Invocable` for 8
/// argument types.
template <class WEAKMEMFN,
          class RET,
          class ARG1,
          class ARG2,
          class ARG3,
          class ARG4,
          class ARG5,
          class ARG6,
          class ARG7,
          class ARG8>
struct WeakMemFnInstance_Invocable<
    WEAKMEMFN,
    RET,
    bslmf::TypeList8<ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8> > {
    // ACCESSORS
    WeakMemFnResult<RET>
    operator()(typename bslmf::ForwardingType<ARG1>::Type arg1,
               typename bslmf::ForwardingType<ARG2>::Type arg2,
               typename bslmf::ForwardingType<ARG3>::Type arg3,
               typename bslmf::ForwardingType<ARG4>::Type arg4,
               typename bslmf::ForwardingType<ARG5>::Type arg5,
               typename bslmf::ForwardingType<ARG6>::Type arg6,
               typename bslmf::ForwardingType<ARG7>::Type arg7,
               typename bslmf::ForwardingType<ARG8>::Type arg8) const;
};

/// Provides a specialization of `WeakMemFnInstance_Invocable` for 9
/// argument types.
template <class WEAKMEMFN,
          class RET,
          class ARG1,
          class ARG2,
          class ARG3,
          class ARG4,
          class ARG5,
          class ARG6,
          class ARG7,
          class ARG8,
          class ARG9>
struct WeakMemFnInstance_Invocable<
    WEAKMEMFN,
    RET,
    bslmf::TypeList9<ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9> > {
    // ACCESSORS
    WeakMemFnResult<RET>
    operator()(typename bslmf::ForwardingType<ARG1>::Type arg1,
               typename bslmf::ForwardingType<ARG2>::Type arg2,
               typename bslmf::ForwardingType<ARG3>::Type arg3,
               typename bslmf::ForwardingType<ARG4>::Type arg4,
               typename bslmf::ForwardingType<ARG5>::Type arg5,
               typename bslmf::ForwardingType<ARG6>::Type arg6,
               typename bslmf::ForwardingType<ARG7>::Type arg7,
               typename bslmf::ForwardingType<ARG8>::Type arg8,
               typename bslmf::ForwardingType<ARG9>::Type arg9) const;
};

// ================================
// class WeakMemFnResult_InPlaceTag
// ================================

/// Provides a tag type used to initialize a `WeakMemFnResult` object.
class WeakMemFnResult_InPlaceTag {};

// ===============
// class WeakMemFn
// ===============

/// Provides a `WeakMemFn` wrapper object for pointers to member functions,
/// that can store, copy, and invoke a pointer to member function on a weak
/// object pointer, assuming that the object pointer is still valid.
template <class PROT>
class WeakMemFn
: public WeakMemFn_Invocable<
      WeakMemFn<PROT>,
      typename bslmf::MemberFunctionPointerTraits<PROT>::ResultType,
      typename bslmf::MemberFunctionPointerTraits<PROT>::ArgumentList> {
  public:
    // TYPES

    /// Defines the result type of the call operator.
    typedef WeakMemFnResult<
        typename bslmf::MemberFunctionPointerTraits<PROT>::ResultType>
        ResultType;

  private:
    // PRIVATE DATA

    // Pointer to member function.
    PROT d_memFn;

    // FRIENDS
    template <class, class, class>
    friend struct WeakMemFn_Invocable;

  public:
    // CREATORS

    /// Create a `WeakMemFn` object holding the address of the specified
    /// `memFn` member function.
    WeakMemFn(PROT memFn) BSLS_KEYWORD_NOEXCEPT;  // IMPLICIT
};

// =======================
// class WeakMemFnInstance
// =======================

/// Provides a `WeakMemFnInstance` wrapper object for pointers to member
/// functions, that can store, copy, and invoke a pointer to member function
/// on a weak object pointer, assuming that the object pointer is still
/// valid.
template <class PROT>
class WeakMemFnInstance
: public WeakMemFnInstance_Invocable<
      WeakMemFnInstance<PROT>,
      typename bslmf::MemberFunctionPointerTraits<PROT>::ResultType,
      typename bslmf::MemberFunctionPointerTraits<PROT>::ArgumentList> {
  public:
    // TYPES

    /// Defines the result type of the call operator.
    typedef WeakMemFnResult<
        typename bslmf::MemberFunctionPointerTraits<PROT>::ResultType>
        ResultType;

  private:
    // PRIVATE TYPES
    typedef typename bslmf::MemberFunctionPointerTraits<PROT>::ClassType
        ObjectType;

  private:
    // PRIVATE DATA

    // Pointer to member function wrapped in an instance of `WeakMemFn`.
    WeakMemFn<PROT> d_memFn;

    // Weak reference to the target object.
    bsl::weak_ptr<ObjectType> d_obj;

    // FRIENDS
    template <class, class, class>
    friend struct WeakMemFnInstance_Invocable;

  public:
    // CREATORS

    /// Create a `WeakMemFnInstance` object holding the address of the
    /// specified `memFn` member function and the specified `obj` object
    /// weak pointer.
    template <class OBJ>
    WeakMemFnInstance(PROT                      memFn,
                      const bsl::weak_ptr<OBJ>& obj) BSLS_KEYWORD_NOEXCEPT;
};

// =====================
// class WeakMemFnResult
// =====================

/// Provides a value-semantic type to be returned by an invocation of
/// `mwcu::WeakMemFn` or `mwcu::WeakMemFnInstance`, that is semantically
/// similar to `bdlb::NullableValue` (or `std::optional`), but also supports
/// `void` and reference value types.
///
/// `VALUE` must meet the requirements of Destructible as specified in the
/// C++ standard.
template <class VALUE>
class WeakMemFnResult {
  private:
    // PRIVATE DATA
    bslalg::ConstructorProxy<bdlb::NullableValue<VALUE> > d_value;

  public:
    // CREATORS

    /// Create a `WeakMemFnResult` object having the null value.  Optionally
    /// specify a `basicAllocator` that is used to supply memory to the
    /// contained value if `VALUE` declares the `bslma::UsesBslmaAllocator`
    /// trait.  If `basicAllocator` is 0, the default memory allocator is
    /// used.
    explicit WeakMemFnResult(bslma::Allocator* basicAllocator = 0)
        BSLS_KEYWORD_NOEXCEPT;

    /// Create a `WeakMemFnResult` object containing the specified `value`
    /// by direct-non-list initializing an object of type `VALUE` with
    /// `bsl::forward<VALUE_T>(value)`.  Optionally specify a
    /// `basicAllocator` that is used to supply memory to the contained
    /// value if `VALUE` declares the `bslma::UsesBslmaAllocator` trait.  If
    /// `basicAllocator` is 0, the default memory allocator is used.
    ///
    /// Note that this constructor should not be used by users directly and
    /// is only made public for test purposes.
    ///
    /// `VALUE` must be constructible from `bsl::forward<VALUE_T>(value)`.
    template <class VALUE_T>
    WeakMemFnResult(WeakMemFnResult_InPlaceTag,
                    BSLS_COMPILERFEATURES_FORWARD_REF(VALUE_T) value,
                    bslma::Allocator* basicAllocator = 0);

    /// Create a `WeakMemFnResult` object having the same state as the
    /// specified `original`.  Optionally specify a `basicAllocator` that is
    /// used to supply memory to the contained value if `VALUE` declares the
    /// `bslma::UsesBslmaAllocator` trait.  If `basicAllocator` is 0, the
    /// default memory allocator is used.
    WeakMemFnResult(const WeakMemFnResult& original,
                    bslma::Allocator*      basicAllocator = 0);

    /// Create a `WeakMemFnResult` object having the same state as the
    /// specified `original`, leaving `original` in a valid but unspecified
    /// state.  Optionally specify a `basicAllocator` that is used to supply
    /// memory to the contained value if `VALUE` declares the
    /// `bslma::UsesBslmaAllocator` trait.  If `basicAllocator` is 0, the
    /// default memory allocator is used.
    WeakMemFnResult(bslmf::MovableRef<WeakMemFnResult> original,
                    bslma::Allocator*                  basicAllocator);

  public:
    // MANIPULATORS

    /// Make this object assume the state the specified `original`.  Return
    /// `*this`.
    WeakMemFnResult& operator=(const WeakMemFnResult& original);

    /// Make this object assume the state the specified `original`, leaving
    /// `original` in a valid but unspecified state.  Return `*this`.
    WeakMemFnResult& operator=(bslmf::MovableRef<WeakMemFnResult> original);

  public:
    // ACCESSORS
    VALUE& value() BSLS_KEYWORD_NOEXCEPT;

    /// Return a reference to the contained value.  The behavior is
    /// undefined unless `isNull()` is `false`.
    const VALUE& value() const BSLS_KEYWORD_NOEXCEPT;

    /// Return `true` if this object is null, and `false` otherwise.
    bool isNull() const BSLS_KEYWORD_NOEXCEPT;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(WeakMemFnResult, bslma::UsesBslmaAllocator)
};

/// Provides a specialization of `WeakMemFnResult` for `void` value type.
template <>
class WeakMemFnResult<void> {
  private:
    // PRIVATE DATA
    bool d_isNull;

  public:
    // CREATORS

    /// Create a `WeakMemFnResult` object having the null value.  The
    /// specified `basicAllocator` is ignored.
    explicit WeakMemFnResult(bslma::Allocator* basicAllocator = 0)
        BSLS_KEYWORD_NOEXCEPT;

    /// Create a `WeakMemFnResult` object having a not null value.
    ///
    /// Note that this constructor should not be used by users directly and
    /// is only made public for test purposes.
    explicit WeakMemFnResult(WeakMemFnResult_InPlaceTag);

    /// Create a `WeakMemFnResult` object having the same value as the
    /// specified `original` object.  The specified `basicAllocator` is
    /// ignored.
    WeakMemFnResult(const WeakMemFnResult& original,
                    bslma::Allocator*      basicAllocator = 0);

  public:
    // MANIPULATORS
  public:
    // ACCESSORS

    /// Does nothing.  The behavior is undefined unless `isNull()` is
    /// `false`.  Note that this function is provided to simplify template
    /// metaprogramming.
    void value() const BSLS_KEYWORD_NOEXCEPT;

    /// Return `true` if this object is null, and `false` otherwise.
    bool isNull() const BSLS_KEYWORD_NOEXCEPT;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(WeakMemFnResult, bslma::UsesBslmaAllocator)
};

/// Provides a specialization of `WeakMemFnResult` for reference value
/// types.
template <class VALUE>
class WeakMemFnResult<VALUE&> {
  private:
    // PRIVATE DATA

    // NOTE: We assume that a non-empty value might contain a null pointer.
    //       That might happen if a function returns a poisoned reference
    //       that points to null.
    bdlb::NullableValue<VALUE*> d_value;

  public:
    // CREATORS

    /// Create a `WeakMemFnResult` object having the null value.  The
    /// specified `basicAllocator` is ignored.
    explicit WeakMemFnResult(bslma::Allocator* basicAllocator = 0)
        BSLS_KEYWORD_NOEXCEPT;

    /// Create a `WeakMemFnResult` object containing the specified `value`
    /// reference.
    ///
    /// Note that this constructor should not be used by users directly and
    /// is only made public for test purposes.
    WeakMemFnResult(WeakMemFnResult_InPlaceTag, VALUE& value);

    /// Create a `WeakMemFnResult` object having the same value as the
    /// specified `original` object.  The specified `basicAllocator` is
    /// ignored.
    WeakMemFnResult(const WeakMemFnResult& original,
                    bslma::Allocator*      basicAllocator = 0);

  public:
    // MANIPULATORS
  public:
    // ACCESSORS

    /// Return the contained reference.  The behavior is undefined unless
    /// `isNull()` is `false`.
    VALUE& value() const BSLS_KEYWORD_NOEXCEPT;

    /// Return `true` if this object is null, and `false` otherwise.
    bool isNull() const BSLS_KEYWORD_NOEXCEPT;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(WeakMemFnResult, bslma::UsesBslmaAllocator)
};

// ====================
// struct WeakMemFnUtil
// ====================

/// Provides an utility class for creating weak member function pointer call
/// wrappers.
struct WeakMemFnUtil {
    // CLASS METHODS

    /// Return `WeakMemFn<PROT>(memFn)`.
    template <class PROT>
    static WeakMemFn<PROT> weakMemFn(PROT memFn) BSLS_KEYWORD_NOEXCEPT;

    /// Return `WeakMemFnInstance<PROT>(memFn, obj)`.
    template <class PROT, class OBJ>
    static WeakMemFnInstance<PROT>
    weakMemFn(PROT memFn, const bsl::weak_ptr<OBJ>& obj) BSLS_KEYWORD_NOEXCEPT;

    /// Return `WeakMemFnInstance<PROT>(memFn, bsl::weak_ptr<OBJ>(obj))`.
    template <class PROT, class OBJ>
    static WeakMemFnInstance<PROT>
    weakMemFn(PROT                        memFn,
              const bsl::shared_ptr<OBJ>& obj) BSLS_KEYWORD_NOEXCEPT;
};

// ============================================================================
//                            INLINE DEFINITIONS
// ============================================================================

// --------------------------
// struct WeakMemFn_Invocable
// --------------------------

// ACCESSORS
template <class WEAKMEMFN, class RET>
template <class OBJ>
inline WeakMemFnResult<RET>
WeakMemFn_Invocable<WEAKMEMFN, RET, bslmf::TypeList0>::operator()(
    const bsl::weak_ptr<OBJ>& obj) const
{
    if (bsl::shared_ptr<OBJ> object = obj.lock()) {
        return WeakMemFnResult<RET>(
            WeakMemFnResult_InPlaceTag(),
            ((*object).*static_cast<const WEAKMEMFN&>(*this).d_memFn)());
        // RETURN
    }

    return WeakMemFnResult<RET>();
}

template <class WEAKMEMFN, class RET, class ARG1>
template <class OBJ>
inline WeakMemFnResult<RET>
WeakMemFn_Invocable<WEAKMEMFN, RET, bslmf::TypeList1<ARG1> >::operator()(
    const bsl::weak_ptr<OBJ>&                  obj,
    typename bslmf::ForwardingType<ARG1>::Type arg1) const
{
    if (bsl::shared_ptr<OBJ> object = obj.lock()) {
        return WeakMemFnResult<RET>(
            WeakMemFnResult_InPlaceTag(),
            ((*object).*static_cast<const WEAKMEMFN&>(*this).d_memFn)(
                bslmf::ForwardingTypeUtil<ARG1>::forwardToTarget(arg1)));
        // RETURN
    }

    return WeakMemFnResult<RET>();
}

template <class WEAKMEMFN, class RET, class ARG1, class ARG2>
template <class OBJ>
inline WeakMemFnResult<RET>
WeakMemFn_Invocable<WEAKMEMFN, RET, bslmf::TypeList2<ARG1, ARG2> >::operator()(
    const bsl::weak_ptr<OBJ>&                  obj,
    typename bslmf::ForwardingType<ARG1>::Type arg1,
    typename bslmf::ForwardingType<ARG2>::Type arg2) const
{
    if (bsl::shared_ptr<OBJ> object = obj.lock()) {
        return WeakMemFnResult<RET>(
            WeakMemFnResult_InPlaceTag(),
            ((*object).*static_cast<const WEAKMEMFN&>(*this).d_memFn)(
                bslmf::ForwardingTypeUtil<ARG1>::forwardToTarget(arg1),
                bslmf::ForwardingTypeUtil<ARG2>::forwardToTarget(arg2)));
        // RETURN
    }

    return WeakMemFnResult<RET>();
}

template <class WEAKMEMFN, class RET, class ARG1, class ARG2, class ARG3>
template <class OBJ>
inline WeakMemFnResult<RET>
WeakMemFn_Invocable<WEAKMEMFN, RET, bslmf::TypeList3<ARG1, ARG2, ARG3> >::
operator()(const bsl::weak_ptr<OBJ>&                  obj,
           typename bslmf::ForwardingType<ARG1>::Type arg1,
           typename bslmf::ForwardingType<ARG2>::Type arg2,
           typename bslmf::ForwardingType<ARG3>::Type arg3) const
{
    if (bsl::shared_ptr<OBJ> object = obj.lock()) {
        return WeakMemFnResult<RET>(
            WeakMemFnResult_InPlaceTag(),
            ((*object).*static_cast<const WEAKMEMFN&>(*this).d_memFn)(
                bslmf::ForwardingTypeUtil<ARG1>::forwardToTarget(arg1),
                bslmf::ForwardingTypeUtil<ARG2>::forwardToTarget(arg2),
                bslmf::ForwardingTypeUtil<ARG3>::forwardToTarget(arg3)));
        // RETURN
    }

    return WeakMemFnResult<RET>();
}

template <class WEAKMEMFN,
          class RET,
          class ARG1,
          class ARG2,
          class ARG3,
          class ARG4>
template <class OBJ>
inline WeakMemFnResult<RET>
WeakMemFn_Invocable<WEAKMEMFN,
                    RET,
                    bslmf::TypeList4<ARG1, ARG2, ARG3, ARG4> >::
operator()(const bsl::weak_ptr<OBJ>&                  obj,
           typename bslmf::ForwardingType<ARG1>::Type arg1,
           typename bslmf::ForwardingType<ARG2>::Type arg2,
           typename bslmf::ForwardingType<ARG3>::Type arg3,
           typename bslmf::ForwardingType<ARG4>::Type arg4) const
{
    if (bsl::shared_ptr<OBJ> object = obj.lock()) {
        return WeakMemFnResult<RET>(
            WeakMemFnResult_InPlaceTag(),
            ((*object).*static_cast<const WEAKMEMFN&>(*this).d_memFn)(
                bslmf::ForwardingTypeUtil<ARG1>::forwardToTarget(arg1),
                bslmf::ForwardingTypeUtil<ARG2>::forwardToTarget(arg2),
                bslmf::ForwardingTypeUtil<ARG3>::forwardToTarget(arg3),
                bslmf::ForwardingTypeUtil<ARG4>::forwardToTarget(arg4)));
        // RETURN
    }

    return WeakMemFnResult<RET>();
}

template <class WEAKMEMFN,
          class RET,
          class ARG1,
          class ARG2,
          class ARG3,
          class ARG4,
          class ARG5>
template <class OBJ>
inline WeakMemFnResult<RET>
WeakMemFn_Invocable<WEAKMEMFN,
                    RET,
                    bslmf::TypeList5<ARG1, ARG2, ARG3, ARG4, ARG5> >::
operator()(const bsl::weak_ptr<OBJ>&                  obj,
           typename bslmf::ForwardingType<ARG1>::Type arg1,
           typename bslmf::ForwardingType<ARG2>::Type arg2,
           typename bslmf::ForwardingType<ARG3>::Type arg3,
           typename bslmf::ForwardingType<ARG4>::Type arg4,
           typename bslmf::ForwardingType<ARG5>::Type arg5) const
{
    if (bsl::shared_ptr<OBJ> object = obj.lock()) {
        return WeakMemFnResult<RET>(
            WeakMemFnResult_InPlaceTag(),
            ((*object).*static_cast<const WEAKMEMFN&>(*this).d_memFn)(
                bslmf::ForwardingTypeUtil<ARG1>::forwardToTarget(arg1),
                bslmf::ForwardingTypeUtil<ARG2>::forwardToTarget(arg2),
                bslmf::ForwardingTypeUtil<ARG3>::forwardToTarget(arg3),
                bslmf::ForwardingTypeUtil<ARG4>::forwardToTarget(arg4),
                bslmf::ForwardingTypeUtil<ARG5>::forwardToTarget(arg5)));
        // RETURN
    }

    return WeakMemFnResult<RET>();
}

template <class WEAKMEMFN,
          class RET,
          class ARG1,
          class ARG2,
          class ARG3,
          class ARG4,
          class ARG5,
          class ARG6>
template <class OBJ>
inline WeakMemFnResult<RET>
WeakMemFn_Invocable<WEAKMEMFN,
                    RET,
                    bslmf::TypeList6<ARG1, ARG2, ARG3, ARG4, ARG5, ARG6> >::
operator()(const bsl::weak_ptr<OBJ>&                  obj,
           typename bslmf::ForwardingType<ARG1>::Type arg1,
           typename bslmf::ForwardingType<ARG2>::Type arg2,
           typename bslmf::ForwardingType<ARG3>::Type arg3,
           typename bslmf::ForwardingType<ARG4>::Type arg4,
           typename bslmf::ForwardingType<ARG5>::Type arg5,
           typename bslmf::ForwardingType<ARG6>::Type arg6) const
{
    if (bsl::shared_ptr<OBJ> object = obj.lock()) {
        return WeakMemFnResult<RET>(
            WeakMemFnResult_InPlaceTag(),
            ((*object).*static_cast<const WEAKMEMFN&>(*this).d_memFn)(
                bslmf::ForwardingTypeUtil<ARG1>::forwardToTarget(arg1),
                bslmf::ForwardingTypeUtil<ARG2>::forwardToTarget(arg2),
                bslmf::ForwardingTypeUtil<ARG3>::forwardToTarget(arg3),
                bslmf::ForwardingTypeUtil<ARG4>::forwardToTarget(arg4),
                bslmf::ForwardingTypeUtil<ARG5>::forwardToTarget(arg5),
                bslmf::ForwardingTypeUtil<ARG6>::forwardToTarget(arg6)));
        // RETURN
    }

    return WeakMemFnResult<RET>();
}

template <class WEAKMEMFN,
          class RET,
          class ARG1,
          class ARG2,
          class ARG3,
          class ARG4,
          class ARG5,
          class ARG6,
          class ARG7>
template <class OBJ>
inline WeakMemFnResult<RET> WeakMemFn_Invocable<
    WEAKMEMFN,
    RET,
    bslmf::TypeList7<ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7> >::
operator()(const bsl::weak_ptr<OBJ>&                  obj,
           typename bslmf::ForwardingType<ARG1>::Type arg1,
           typename bslmf::ForwardingType<ARG2>::Type arg2,
           typename bslmf::ForwardingType<ARG3>::Type arg3,
           typename bslmf::ForwardingType<ARG4>::Type arg4,
           typename bslmf::ForwardingType<ARG5>::Type arg5,
           typename bslmf::ForwardingType<ARG6>::Type arg6,
           typename bslmf::ForwardingType<ARG7>::Type arg7) const
{
    if (bsl::shared_ptr<OBJ> object = obj.lock()) {
        return WeakMemFnResult<RET>(
            WeakMemFnResult_InPlaceTag(),
            ((*object).*static_cast<const WEAKMEMFN&>(*this).d_memFn)(
                bslmf::ForwardingTypeUtil<ARG1>::forwardToTarget(arg1),
                bslmf::ForwardingTypeUtil<ARG2>::forwardToTarget(arg2),
                bslmf::ForwardingTypeUtil<ARG3>::forwardToTarget(arg3),
                bslmf::ForwardingTypeUtil<ARG4>::forwardToTarget(arg4),
                bslmf::ForwardingTypeUtil<ARG5>::forwardToTarget(arg5),
                bslmf::ForwardingTypeUtil<ARG6>::forwardToTarget(arg6),
                bslmf::ForwardingTypeUtil<ARG7>::forwardToTarget(arg7)));
        // RETURN
    }

    return WeakMemFnResult<RET>();
}

template <class WEAKMEMFN,
          class RET,
          class ARG1,
          class ARG2,
          class ARG3,
          class ARG4,
          class ARG5,
          class ARG6,
          class ARG7,
          class ARG8>
template <class OBJ>
inline WeakMemFnResult<RET> WeakMemFn_Invocable<
    WEAKMEMFN,
    RET,
    bslmf::TypeList8<ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8> >::
operator()(const bsl::weak_ptr<OBJ>&                  obj,
           typename bslmf::ForwardingType<ARG1>::Type arg1,
           typename bslmf::ForwardingType<ARG2>::Type arg2,
           typename bslmf::ForwardingType<ARG3>::Type arg3,
           typename bslmf::ForwardingType<ARG4>::Type arg4,
           typename bslmf::ForwardingType<ARG5>::Type arg5,
           typename bslmf::ForwardingType<ARG6>::Type arg6,
           typename bslmf::ForwardingType<ARG7>::Type arg7,
           typename bslmf::ForwardingType<ARG8>::Type arg8) const
{
    if (bsl::shared_ptr<OBJ> object = obj.lock()) {
        return WeakMemFnResult<RET>(
            WeakMemFnResult_InPlaceTag(),
            ((*object).*static_cast<const WEAKMEMFN&>(*this).d_memFn)(
                bslmf::ForwardingTypeUtil<ARG1>::forwardToTarget(arg1),
                bslmf::ForwardingTypeUtil<ARG2>::forwardToTarget(arg2),
                bslmf::ForwardingTypeUtil<ARG3>::forwardToTarget(arg3),
                bslmf::ForwardingTypeUtil<ARG4>::forwardToTarget(arg4),
                bslmf::ForwardingTypeUtil<ARG5>::forwardToTarget(arg5),
                bslmf::ForwardingTypeUtil<ARG6>::forwardToTarget(arg6),
                bslmf::ForwardingTypeUtil<ARG7>::forwardToTarget(arg7),
                bslmf::ForwardingTypeUtil<ARG8>::forwardToTarget(arg8)));
        // RETURN
    }

    return WeakMemFnResult<RET>();
}

template <class WEAKMEMFN,
          class RET,
          class ARG1,
          class ARG2,
          class ARG3,
          class ARG4,
          class ARG5,
          class ARG6,
          class ARG7,
          class ARG8,
          class ARG9>
template <class OBJ>
inline WeakMemFnResult<RET> WeakMemFn_Invocable<
    WEAKMEMFN,
    RET,
    bslmf::TypeList9<ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9> >::
operator()(const bsl::weak_ptr<OBJ>&                  obj,
           typename bslmf::ForwardingType<ARG1>::Type arg1,
           typename bslmf::ForwardingType<ARG2>::Type arg2,
           typename bslmf::ForwardingType<ARG3>::Type arg3,
           typename bslmf::ForwardingType<ARG4>::Type arg4,
           typename bslmf::ForwardingType<ARG5>::Type arg5,
           typename bslmf::ForwardingType<ARG6>::Type arg6,
           typename bslmf::ForwardingType<ARG7>::Type arg7,
           typename bslmf::ForwardingType<ARG8>::Type arg8,
           typename bslmf::ForwardingType<ARG9>::Type arg9) const
{
    if (bsl::shared_ptr<OBJ> object = obj.lock()) {
        return WeakMemFnResult<RET>(
            WeakMemFnResult_InPlaceTag(),
            ((*object).*static_cast<const WEAKMEMFN&>(*this).d_memFn)(
                bslmf::ForwardingTypeUtil<ARG1>::forwardToTarget(arg1),
                bslmf::ForwardingTypeUtil<ARG2>::forwardToTarget(arg2),
                bslmf::ForwardingTypeUtil<ARG3>::forwardToTarget(arg3),
                bslmf::ForwardingTypeUtil<ARG4>::forwardToTarget(arg4),
                bslmf::ForwardingTypeUtil<ARG5>::forwardToTarget(arg5),
                bslmf::ForwardingTypeUtil<ARG6>::forwardToTarget(arg6),
                bslmf::ForwardingTypeUtil<ARG7>::forwardToTarget(arg7),
                bslmf::ForwardingTypeUtil<ARG8>::forwardToTarget(arg8),
                bslmf::ForwardingTypeUtil<ARG9>::forwardToTarget(arg9)));
        // RETURN
    }

    return WeakMemFnResult<RET>();
}

template <class WEAKMEMFN>
template <class OBJ>
inline WeakMemFnResult<void>
WeakMemFn_Invocable<WEAKMEMFN, void, bslmf::TypeList0>::operator()(
    const bsl::weak_ptr<OBJ>& obj) const
{
    if (bsl::shared_ptr<OBJ> object = obj.lock()) {
        ((*object).*static_cast<const WEAKMEMFN&>(*this).d_memFn)();
        return WeakMemFnResult<void>(WeakMemFnResult_InPlaceTag());  // RETURN
    }

    return WeakMemFnResult<void>();
}

template <class WEAKMEMFN, class ARG1>
template <class OBJ>
inline WeakMemFnResult<void>
WeakMemFn_Invocable<WEAKMEMFN, void, bslmf::TypeList1<ARG1> >::operator()(
    const bsl::weak_ptr<OBJ>&                  obj,
    typename bslmf::ForwardingType<ARG1>::Type arg1) const
{
    if (bsl::shared_ptr<OBJ> object = obj.lock()) {
        ((*object).*static_cast<const WEAKMEMFN&>(*this).d_memFn)(
            bslmf::ForwardingTypeUtil<ARG1>::forwardToTarget(arg1));
        return WeakMemFnResult<void>(WeakMemFnResult_InPlaceTag());  // RETURN
    }

    return WeakMemFnResult<void>();
}

template <class WEAKMEMFN, class ARG1, class ARG2>
template <class OBJ>
inline WeakMemFnResult<void>
WeakMemFn_Invocable<WEAKMEMFN, void, bslmf::TypeList2<ARG1, ARG2> >::
operator()(const bsl::weak_ptr<OBJ>&                  obj,
           typename bslmf::ForwardingType<ARG1>::Type arg1,
           typename bslmf::ForwardingType<ARG2>::Type arg2) const
{
    if (bsl::shared_ptr<OBJ> object = obj.lock()) {
        ((*object).*static_cast<const WEAKMEMFN&>(*this).d_memFn)(
            bslmf::ForwardingTypeUtil<ARG1>::forwardToTarget(arg1),
            bslmf::ForwardingTypeUtil<ARG2>::forwardToTarget(arg2));
        return WeakMemFnResult<void>(WeakMemFnResult_InPlaceTag());  // RETURN
    }

    return WeakMemFnResult<void>();
}

template <class WEAKMEMFN, class ARG1, class ARG2, class ARG3>
template <class OBJ>
inline WeakMemFnResult<void>
WeakMemFn_Invocable<WEAKMEMFN, void, bslmf::TypeList3<ARG1, ARG2, ARG3> >::
operator()(const bsl::weak_ptr<OBJ>&                  obj,
           typename bslmf::ForwardingType<ARG1>::Type arg1,
           typename bslmf::ForwardingType<ARG2>::Type arg2,
           typename bslmf::ForwardingType<ARG3>::Type arg3) const
{
    if (bsl::shared_ptr<OBJ> object = obj.lock()) {
        ((*object).*static_cast<const WEAKMEMFN&>(*this).d_memFn)(
            bslmf::ForwardingTypeUtil<ARG1>::forwardToTarget(arg1),
            bslmf::ForwardingTypeUtil<ARG2>::forwardToTarget(arg2),
            bslmf::ForwardingTypeUtil<ARG3>::forwardToTarget(arg3));
        return WeakMemFnResult<void>(WeakMemFnResult_InPlaceTag());  // RETURN
    }

    return WeakMemFnResult<void>();
}

template <class WEAKMEMFN, class ARG1, class ARG2, class ARG3, class ARG4>
template <class OBJ>
inline WeakMemFnResult<void>
WeakMemFn_Invocable<WEAKMEMFN,
                    void,
                    bslmf::TypeList4<ARG1, ARG2, ARG3, ARG4> >::
operator()(const bsl::weak_ptr<OBJ>&                  obj,
           typename bslmf::ForwardingType<ARG1>::Type arg1,
           typename bslmf::ForwardingType<ARG2>::Type arg2,
           typename bslmf::ForwardingType<ARG3>::Type arg3,
           typename bslmf::ForwardingType<ARG4>::Type arg4) const
{
    if (bsl::shared_ptr<OBJ> object = obj.lock()) {
        ((*object).*static_cast<const WEAKMEMFN&>(*this).d_memFn)(
            bslmf::ForwardingTypeUtil<ARG1>::forwardToTarget(arg1),
            bslmf::ForwardingTypeUtil<ARG2>::forwardToTarget(arg2),
            bslmf::ForwardingTypeUtil<ARG3>::forwardToTarget(arg3),
            bslmf::ForwardingTypeUtil<ARG4>::forwardToTarget(arg4));
        return WeakMemFnResult<void>(WeakMemFnResult_InPlaceTag());  // RETURN
    }

    return WeakMemFnResult<void>();
}

template <class WEAKMEMFN,
          class ARG1,
          class ARG2,
          class ARG3,
          class ARG4,
          class ARG5>
template <class OBJ>
inline WeakMemFnResult<void>
WeakMemFn_Invocable<WEAKMEMFN,
                    void,
                    bslmf::TypeList5<ARG1, ARG2, ARG3, ARG4, ARG5> >::
operator()(const bsl::weak_ptr<OBJ>&                  obj,
           typename bslmf::ForwardingType<ARG1>::Type arg1,
           typename bslmf::ForwardingType<ARG2>::Type arg2,
           typename bslmf::ForwardingType<ARG3>::Type arg3,
           typename bslmf::ForwardingType<ARG4>::Type arg4,
           typename bslmf::ForwardingType<ARG5>::Type arg5) const
{
    if (bsl::shared_ptr<OBJ> object = obj.lock()) {
        ((*object).*static_cast<const WEAKMEMFN&>(*this).d_memFn)(
            bslmf::ForwardingTypeUtil<ARG1>::forwardToTarget(arg1),
            bslmf::ForwardingTypeUtil<ARG2>::forwardToTarget(arg2),
            bslmf::ForwardingTypeUtil<ARG3>::forwardToTarget(arg3),
            bslmf::ForwardingTypeUtil<ARG4>::forwardToTarget(arg4),
            bslmf::ForwardingTypeUtil<ARG5>::forwardToTarget(arg5));
        return WeakMemFnResult<void>(WeakMemFnResult_InPlaceTag());  // RETURN
    }

    return WeakMemFnResult<void>();
}

template <class WEAKMEMFN,
          class ARG1,
          class ARG2,
          class ARG3,
          class ARG4,
          class ARG5,
          class ARG6>
template <class OBJ>
inline WeakMemFnResult<void>
WeakMemFn_Invocable<WEAKMEMFN,
                    void,
                    bslmf::TypeList6<ARG1, ARG2, ARG3, ARG4, ARG5, ARG6> >::
operator()(const bsl::weak_ptr<OBJ>&                  obj,
           typename bslmf::ForwardingType<ARG1>::Type arg1,
           typename bslmf::ForwardingType<ARG2>::Type arg2,
           typename bslmf::ForwardingType<ARG3>::Type arg3,
           typename bslmf::ForwardingType<ARG4>::Type arg4,
           typename bslmf::ForwardingType<ARG5>::Type arg5,
           typename bslmf::ForwardingType<ARG6>::Type arg6) const
{
    if (bsl::shared_ptr<OBJ> object = obj.lock()) {
        ((*object).*static_cast<const WEAKMEMFN&>(*this).d_memFn)(
            bslmf::ForwardingTypeUtil<ARG1>::forwardToTarget(arg1),
            bslmf::ForwardingTypeUtil<ARG2>::forwardToTarget(arg2),
            bslmf::ForwardingTypeUtil<ARG3>::forwardToTarget(arg3),
            bslmf::ForwardingTypeUtil<ARG4>::forwardToTarget(arg4),
            bslmf::ForwardingTypeUtil<ARG5>::forwardToTarget(arg5),
            bslmf::ForwardingTypeUtil<ARG6>::forwardToTarget(arg6));
        return WeakMemFnResult<void>(WeakMemFnResult_InPlaceTag());  // RETURN
    }

    return WeakMemFnResult<void>();
}

template <class WEAKMEMFN,
          class ARG1,
          class ARG2,
          class ARG3,
          class ARG4,
          class ARG5,
          class ARG6,
          class ARG7>
template <class OBJ>
inline WeakMemFnResult<void> WeakMemFn_Invocable<
    WEAKMEMFN,
    void,
    bslmf::TypeList7<ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7> >::
operator()(const bsl::weak_ptr<OBJ>&                  obj,
           typename bslmf::ForwardingType<ARG1>::Type arg1,
           typename bslmf::ForwardingType<ARG2>::Type arg2,
           typename bslmf::ForwardingType<ARG3>::Type arg3,
           typename bslmf::ForwardingType<ARG4>::Type arg4,
           typename bslmf::ForwardingType<ARG5>::Type arg5,
           typename bslmf::ForwardingType<ARG6>::Type arg6,
           typename bslmf::ForwardingType<ARG7>::Type arg7) const
{
    if (bsl::shared_ptr<OBJ> object = obj.lock()) {
        ((*object).*static_cast<const WEAKMEMFN&>(*this).d_memFn)(
            bslmf::ForwardingTypeUtil<ARG1>::forwardToTarget(arg1),
            bslmf::ForwardingTypeUtil<ARG2>::forwardToTarget(arg2),
            bslmf::ForwardingTypeUtil<ARG3>::forwardToTarget(arg3),
            bslmf::ForwardingTypeUtil<ARG4>::forwardToTarget(arg4),
            bslmf::ForwardingTypeUtil<ARG5>::forwardToTarget(arg5),
            bslmf::ForwardingTypeUtil<ARG6>::forwardToTarget(arg6),
            bslmf::ForwardingTypeUtil<ARG7>::forwardToTarget(arg7));
        return WeakMemFnResult<void>(WeakMemFnResult_InPlaceTag());  // RETURN
    }

    return WeakMemFnResult<void>();
}

template <class WEAKMEMFN,
          class ARG1,
          class ARG2,
          class ARG3,
          class ARG4,
          class ARG5,
          class ARG6,
          class ARG7,
          class ARG8>
template <class OBJ>
inline WeakMemFnResult<void> WeakMemFn_Invocable<
    WEAKMEMFN,
    void,
    bslmf::TypeList8<ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8> >::
operator()(const bsl::weak_ptr<OBJ>&                  obj,
           typename bslmf::ForwardingType<ARG1>::Type arg1,
           typename bslmf::ForwardingType<ARG2>::Type arg2,
           typename bslmf::ForwardingType<ARG3>::Type arg3,
           typename bslmf::ForwardingType<ARG4>::Type arg4,
           typename bslmf::ForwardingType<ARG5>::Type arg5,
           typename bslmf::ForwardingType<ARG6>::Type arg6,
           typename bslmf::ForwardingType<ARG7>::Type arg7,
           typename bslmf::ForwardingType<ARG8>::Type arg8) const
{
    if (bsl::shared_ptr<OBJ> object = obj.lock()) {
        ((*object).*static_cast<const WEAKMEMFN&>(*this).d_memFn)(
            bslmf::ForwardingTypeUtil<ARG1>::forwardToTarget(arg1),
            bslmf::ForwardingTypeUtil<ARG2>::forwardToTarget(arg2),
            bslmf::ForwardingTypeUtil<ARG3>::forwardToTarget(arg3),
            bslmf::ForwardingTypeUtil<ARG4>::forwardToTarget(arg4),
            bslmf::ForwardingTypeUtil<ARG5>::forwardToTarget(arg5),
            bslmf::ForwardingTypeUtil<ARG6>::forwardToTarget(arg6),
            bslmf::ForwardingTypeUtil<ARG7>::forwardToTarget(arg7),
            bslmf::ForwardingTypeUtil<ARG8>::forwardToTarget(arg8));
        return WeakMemFnResult<void>(WeakMemFnResult_InPlaceTag());  // RETURN
    }

    return WeakMemFnResult<void>();
}

template <class WEAKMEMFN,
          class ARG1,
          class ARG2,
          class ARG3,
          class ARG4,
          class ARG5,
          class ARG6,
          class ARG7,
          class ARG8,
          class ARG9>
template <class OBJ>
inline WeakMemFnResult<void> WeakMemFn_Invocable<
    WEAKMEMFN,
    void,
    bslmf::TypeList9<ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9> >::
operator()(const bsl::weak_ptr<OBJ>&                  obj,
           typename bslmf::ForwardingType<ARG1>::Type arg1,
           typename bslmf::ForwardingType<ARG2>::Type arg2,
           typename bslmf::ForwardingType<ARG3>::Type arg3,
           typename bslmf::ForwardingType<ARG4>::Type arg4,
           typename bslmf::ForwardingType<ARG5>::Type arg5,
           typename bslmf::ForwardingType<ARG6>::Type arg6,
           typename bslmf::ForwardingType<ARG7>::Type arg7,
           typename bslmf::ForwardingType<ARG8>::Type arg8,
           typename bslmf::ForwardingType<ARG9>::Type arg9) const
{
    if (bsl::shared_ptr<OBJ> object = obj.lock()) {
        ((*object).*static_cast<const WEAKMEMFN&>(*this).d_memFn)(
            bslmf::ForwardingTypeUtil<ARG1>::forwardToTarget(arg1),
            bslmf::ForwardingTypeUtil<ARG2>::forwardToTarget(arg2),
            bslmf::ForwardingTypeUtil<ARG3>::forwardToTarget(arg3),
            bslmf::ForwardingTypeUtil<ARG4>::forwardToTarget(arg4),
            bslmf::ForwardingTypeUtil<ARG5>::forwardToTarget(arg5),
            bslmf::ForwardingTypeUtil<ARG6>::forwardToTarget(arg6),
            bslmf::ForwardingTypeUtil<ARG7>::forwardToTarget(arg7),
            bslmf::ForwardingTypeUtil<ARG8>::forwardToTarget(arg8),
            bslmf::ForwardingTypeUtil<ARG9>::forwardToTarget(arg9));
        return WeakMemFnResult<void>(WeakMemFnResult_InPlaceTag());  // RETURN
    }

    return WeakMemFnResult<void>();
}

// ----------------------------------
// struct WeakMemFnInstance_Invocable
// ----------------------------------

// ACCESSORS
template <class WEAKMEMFN, class RET>
inline WeakMemFnResult<RET>
WeakMemFnInstance_Invocable<WEAKMEMFN, RET, bslmf::TypeList0>::operator()()
    const
{
    return static_cast<const WEAKMEMFN&>(*this).d_memFn(
        static_cast<const WEAKMEMFN&>(*this).d_obj);
}

template <class WEAKMEMFN, class RET, class ARG1>
inline WeakMemFnResult<RET>
WeakMemFnInstance_Invocable<WEAKMEMFN, RET, bslmf::TypeList1<ARG1> >::
operator()(typename bslmf::ForwardingType<ARG1>::Type arg1) const
{
    return static_cast<const WEAKMEMFN&>(*this).d_memFn(
        static_cast<const WEAKMEMFN&>(*this).d_obj,
        bslmf::ForwardingTypeUtil<ARG1>::forwardToTarget(arg1));
}

template <class WEAKMEMFN, class RET, class ARG1, class ARG2>
inline WeakMemFnResult<RET>
WeakMemFnInstance_Invocable<WEAKMEMFN, RET, bslmf::TypeList2<ARG1, ARG2> >::
operator()(typename bslmf::ForwardingType<ARG1>::Type arg1,
           typename bslmf::ForwardingType<ARG2>::Type arg2) const
{
    return static_cast<const WEAKMEMFN&>(*this).d_memFn(
        static_cast<const WEAKMEMFN&>(*this).d_obj,
        bslmf::ForwardingTypeUtil<ARG1>::forwardToTarget(arg1),
        bslmf::ForwardingTypeUtil<ARG2>::forwardToTarget(arg2));
}

template <class WEAKMEMFN, class RET, class ARG1, class ARG2, class ARG3>
inline WeakMemFnResult<RET>
WeakMemFnInstance_Invocable<WEAKMEMFN,
                            RET,
                            bslmf::TypeList3<ARG1, ARG2, ARG3> >::
operator()(typename bslmf::ForwardingType<ARG1>::Type arg1,
           typename bslmf::ForwardingType<ARG2>::Type arg2,
           typename bslmf::ForwardingType<ARG3>::Type arg3) const
{
    return static_cast<const WEAKMEMFN&>(*this).d_memFn(
        static_cast<const WEAKMEMFN&>(*this).d_obj,
        bslmf::ForwardingTypeUtil<ARG1>::forwardToTarget(arg1),
        bslmf::ForwardingTypeUtil<ARG2>::forwardToTarget(arg2),
        bslmf::ForwardingTypeUtil<ARG3>::forwardToTarget(arg3));
}

template <class WEAKMEMFN,
          class RET,
          class ARG1,
          class ARG2,
          class ARG3,
          class ARG4>
inline WeakMemFnResult<RET>
WeakMemFnInstance_Invocable<WEAKMEMFN,
                            RET,
                            bslmf::TypeList4<ARG1, ARG2, ARG3, ARG4> >::
operator()(typename bslmf::ForwardingType<ARG1>::Type arg1,
           typename bslmf::ForwardingType<ARG2>::Type arg2,
           typename bslmf::ForwardingType<ARG3>::Type arg3,
           typename bslmf::ForwardingType<ARG4>::Type arg4) const
{
    return static_cast<const WEAKMEMFN&>(*this).d_memFn(
        static_cast<const WEAKMEMFN&>(*this).d_obj,
        bslmf::ForwardingTypeUtil<ARG1>::forwardToTarget(arg1),
        bslmf::ForwardingTypeUtil<ARG2>::forwardToTarget(arg2),
        bslmf::ForwardingTypeUtil<ARG3>::forwardToTarget(arg3),
        bslmf::ForwardingTypeUtil<ARG4>::forwardToTarget(arg4));
}

template <class WEAKMEMFN,
          class RET,
          class ARG1,
          class ARG2,
          class ARG3,
          class ARG4,
          class ARG5>
inline WeakMemFnResult<RET>
WeakMemFnInstance_Invocable<WEAKMEMFN,
                            RET,
                            bslmf::TypeList5<ARG1, ARG2, ARG3, ARG4, ARG5> >::
operator()(typename bslmf::ForwardingType<ARG1>::Type arg1,
           typename bslmf::ForwardingType<ARG2>::Type arg2,
           typename bslmf::ForwardingType<ARG3>::Type arg3,
           typename bslmf::ForwardingType<ARG4>::Type arg4,
           typename bslmf::ForwardingType<ARG5>::Type arg5) const
{
    return static_cast<const WEAKMEMFN&>(*this).d_memFn(
        static_cast<const WEAKMEMFN&>(*this).d_obj,
        bslmf::ForwardingTypeUtil<ARG1>::forwardToTarget(arg1),
        bslmf::ForwardingTypeUtil<ARG2>::forwardToTarget(arg2),
        bslmf::ForwardingTypeUtil<ARG3>::forwardToTarget(arg3),
        bslmf::ForwardingTypeUtil<ARG4>::forwardToTarget(arg4),
        bslmf::ForwardingTypeUtil<ARG5>::forwardToTarget(arg5));
}

template <class WEAKMEMFN,
          class RET,
          class ARG1,
          class ARG2,
          class ARG3,
          class ARG4,
          class ARG5,
          class ARG6>
inline WeakMemFnResult<RET> WeakMemFnInstance_Invocable<
    WEAKMEMFN,
    RET,
    bslmf::TypeList6<ARG1, ARG2, ARG3, ARG4, ARG5, ARG6> >::
operator()(typename bslmf::ForwardingType<ARG1>::Type arg1,
           typename bslmf::ForwardingType<ARG2>::Type arg2,
           typename bslmf::ForwardingType<ARG3>::Type arg3,
           typename bslmf::ForwardingType<ARG4>::Type arg4,
           typename bslmf::ForwardingType<ARG5>::Type arg5,
           typename bslmf::ForwardingType<ARG6>::Type arg6) const
{
    return static_cast<const WEAKMEMFN&>(*this).d_memFn(
        static_cast<const WEAKMEMFN&>(*this).d_obj,
        bslmf::ForwardingTypeUtil<ARG1>::forwardToTarget(arg1),
        bslmf::ForwardingTypeUtil<ARG2>::forwardToTarget(arg2),
        bslmf::ForwardingTypeUtil<ARG3>::forwardToTarget(arg3),
        bslmf::ForwardingTypeUtil<ARG4>::forwardToTarget(arg4),
        bslmf::ForwardingTypeUtil<ARG5>::forwardToTarget(arg5),
        bslmf::ForwardingTypeUtil<ARG6>::forwardToTarget(arg6));
}

template <class WEAKMEMFN,
          class RET,
          class ARG1,
          class ARG2,
          class ARG3,
          class ARG4,
          class ARG5,
          class ARG6,
          class ARG7>
inline WeakMemFnResult<RET> WeakMemFnInstance_Invocable<
    WEAKMEMFN,
    RET,
    bslmf::TypeList7<ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7> >::
operator()(typename bslmf::ForwardingType<ARG1>::Type arg1,
           typename bslmf::ForwardingType<ARG2>::Type arg2,
           typename bslmf::ForwardingType<ARG3>::Type arg3,
           typename bslmf::ForwardingType<ARG4>::Type arg4,
           typename bslmf::ForwardingType<ARG5>::Type arg5,
           typename bslmf::ForwardingType<ARG6>::Type arg6,
           typename bslmf::ForwardingType<ARG7>::Type arg7) const
{
    return static_cast<const WEAKMEMFN&>(*this).d_memFn(
        static_cast<const WEAKMEMFN&>(*this).d_obj,
        bslmf::ForwardingTypeUtil<ARG1>::forwardToTarget(arg1),
        bslmf::ForwardingTypeUtil<ARG2>::forwardToTarget(arg2),
        bslmf::ForwardingTypeUtil<ARG3>::forwardToTarget(arg3),
        bslmf::ForwardingTypeUtil<ARG4>::forwardToTarget(arg4),
        bslmf::ForwardingTypeUtil<ARG5>::forwardToTarget(arg5),
        bslmf::ForwardingTypeUtil<ARG6>::forwardToTarget(arg6),
        bslmf::ForwardingTypeUtil<ARG7>::forwardToTarget(arg7));
}

template <class WEAKMEMFN,
          class RET,
          class ARG1,
          class ARG2,
          class ARG3,
          class ARG4,
          class ARG5,
          class ARG6,
          class ARG7,
          class ARG8>
inline WeakMemFnResult<RET> WeakMemFnInstance_Invocable<
    WEAKMEMFN,
    RET,
    bslmf::TypeList8<ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8> >::
operator()(typename bslmf::ForwardingType<ARG1>::Type arg1,
           typename bslmf::ForwardingType<ARG2>::Type arg2,
           typename bslmf::ForwardingType<ARG3>::Type arg3,
           typename bslmf::ForwardingType<ARG4>::Type arg4,
           typename bslmf::ForwardingType<ARG5>::Type arg5,
           typename bslmf::ForwardingType<ARG6>::Type arg6,
           typename bslmf::ForwardingType<ARG7>::Type arg7,
           typename bslmf::ForwardingType<ARG8>::Type arg8) const
{
    return static_cast<const WEAKMEMFN&>(*this).d_memFn(
        static_cast<const WEAKMEMFN&>(*this).d_obj,
        bslmf::ForwardingTypeUtil<ARG1>::forwardToTarget(arg1),
        bslmf::ForwardingTypeUtil<ARG2>::forwardToTarget(arg2),
        bslmf::ForwardingTypeUtil<ARG3>::forwardToTarget(arg3),
        bslmf::ForwardingTypeUtil<ARG4>::forwardToTarget(arg4),
        bslmf::ForwardingTypeUtil<ARG5>::forwardToTarget(arg5),
        bslmf::ForwardingTypeUtil<ARG6>::forwardToTarget(arg6),
        bslmf::ForwardingTypeUtil<ARG7>::forwardToTarget(arg7),
        bslmf::ForwardingTypeUtil<ARG8>::forwardToTarget(arg8));
}

template <class WEAKMEMFN,
          class RET,
          class ARG1,
          class ARG2,
          class ARG3,
          class ARG4,
          class ARG5,
          class ARG6,
          class ARG7,
          class ARG8,
          class ARG9>
inline WeakMemFnResult<RET> WeakMemFnInstance_Invocable<
    WEAKMEMFN,
    RET,
    bslmf::TypeList9<ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9> >::
operator()(typename bslmf::ForwardingType<ARG1>::Type arg1,
           typename bslmf::ForwardingType<ARG2>::Type arg2,
           typename bslmf::ForwardingType<ARG3>::Type arg3,
           typename bslmf::ForwardingType<ARG4>::Type arg4,
           typename bslmf::ForwardingType<ARG5>::Type arg5,
           typename bslmf::ForwardingType<ARG6>::Type arg6,
           typename bslmf::ForwardingType<ARG7>::Type arg7,
           typename bslmf::ForwardingType<ARG8>::Type arg8,
           typename bslmf::ForwardingType<ARG9>::Type arg9) const
{
    return static_cast<const WEAKMEMFN&>(*this).d_memFn(
        static_cast<const WEAKMEMFN&>(*this).d_obj,
        bslmf::ForwardingTypeUtil<ARG1>::forwardToTarget(arg1),
        bslmf::ForwardingTypeUtil<ARG2>::forwardToTarget(arg2),
        bslmf::ForwardingTypeUtil<ARG3>::forwardToTarget(arg3),
        bslmf::ForwardingTypeUtil<ARG4>::forwardToTarget(arg4),
        bslmf::ForwardingTypeUtil<ARG5>::forwardToTarget(arg5),
        bslmf::ForwardingTypeUtil<ARG6>::forwardToTarget(arg6),
        bslmf::ForwardingTypeUtil<ARG7>::forwardToTarget(arg7),
        bslmf::ForwardingTypeUtil<ARG8>::forwardToTarget(arg8),
        bslmf::ForwardingTypeUtil<ARG9>::forwardToTarget(arg9));
}

// ---------------
// class WeakMemFn
// ---------------

// CREATORS
template <class PROT>
inline WeakMemFn<PROT>::WeakMemFn(PROT memFn) BSLS_KEYWORD_NOEXCEPT
: d_memFn(memFn)
{
    // PRECONDITIONS
    BSLS_ASSERT(memFn);
}

// -----------------------
// class WeakMemFnInstance
// -----------------------

// CREATORS
template <class PROT>
template <class OBJ>
inline WeakMemFnInstance<PROT>::WeakMemFnInstance(
    PROT                      memFn,
    const bsl::weak_ptr<OBJ>& obj) BSLS_KEYWORD_NOEXCEPT : d_memFn(memFn),
                                                           d_obj(obj)
{
    // PRECONDITIONS
    BSLS_ASSERT(memFn);
}

// ---------------------
// class WeakMemFnResult
// ---------------------

// CREATORS
template <class VALUE>
inline WeakMemFnResult<VALUE>::WeakMemFnResult(
    bslma::Allocator* basicAllocator) BSLS_KEYWORD_NOEXCEPT
: d_value(basicAllocator)
{
    // NOTHING
}

template <class VALUE>
template <class VALUE_T>
inline WeakMemFnResult<VALUE>::WeakMemFnResult(
    WeakMemFnResult_InPlaceTag,
    BSLS_COMPILERFEATURES_FORWARD_REF(VALUE_T) value,
    bslma::Allocator* basicAllocator)
: d_value(BSLS_COMPILERFEATURES_FORWARD(VALUE_T, value), basicAllocator)
{
    // NOTHING
}

template <class VALUE>
inline WeakMemFnResult<VALUE>::WeakMemFnResult(
    const WeakMemFnResult& original,
    bslma::Allocator*      basicAllocator)
: d_value(original.d_value.object(), basicAllocator)
{
    // NOTHING
}

template <class VALUE>
inline WeakMemFnResult<VALUE>::WeakMemFnResult(
    bslmf::MovableRef<WeakMemFnResult> original,
    bslma::Allocator*                  basicAllocator)
: d_value(bslmf::MovableRefUtil::move(
              bslmf::MovableRefUtil::access(original).d_value.object()),
          basicAllocator)
{
    // NOTHING
}

// MANIPULATORS
template <class VALUE>
inline WeakMemFnResult<VALUE>&
WeakMemFnResult<VALUE>::operator=(const WeakMemFnResult& original)
{
    d_value.object() = original.d_value.object();
    return *this;
}

template <class VALUE>
inline WeakMemFnResult<VALUE>&
WeakMemFnResult<VALUE>::operator=(bslmf::MovableRef<WeakMemFnResult> original)
{
    d_value.object() = bslmf::MovableRefUtil::move(
        bslmf::MovableRefUtil::access(original).d_value.object());
    return *this;
}

// ACCESSORS
template <class VALUE>
inline VALUE& WeakMemFnResult<VALUE>::value() BSLS_KEYWORD_NOEXCEPT
{
    // PRECONDITIONS
    BSLS_ASSERT(!isNull());

    return d_value.object().value();
}

template <class VALUE>
inline const VALUE& WeakMemFnResult<VALUE>::value() const BSLS_KEYWORD_NOEXCEPT
{
    // PRECONDITIONS
    BSLS_ASSERT(!isNull());

    return d_value.value();
}

template <class VALUE>
inline bool WeakMemFnResult<VALUE>::isNull() const BSLS_KEYWORD_NOEXCEPT
{
    return d_value.object().isNull();
}

// CREATORS
inline WeakMemFnResult<void>::WeakMemFnResult(bslma::Allocator*)
    BSLS_KEYWORD_NOEXCEPT : d_isNull(true)
{
    // NOTHING
}

inline WeakMemFnResult<void>::WeakMemFnResult(WeakMemFnResult_InPlaceTag)
: d_isNull(false)
{
    // NOTHING
}

inline WeakMemFnResult<void>::WeakMemFnResult(const WeakMemFnResult& original,
                                              bslma::Allocator*)
: d_isNull(original.d_isNull)
{
    // NOTHING
}

// ACCESSORS
inline void WeakMemFnResult<void>::value() const BSLS_KEYWORD_NOEXCEPT
{
    // PRECONDITIONS
    BSLS_ASSERT(!isNull());
}

inline bool WeakMemFnResult<void>::isNull() const BSLS_KEYWORD_NOEXCEPT
{
    return d_isNull;
}

// CREATORS
template <class VALUE>
inline WeakMemFnResult<VALUE&>::WeakMemFnResult(bslma::Allocator*)
    BSLS_KEYWORD_NOEXCEPT : d_value()
{
    // NOTHING
}

template <class VALUE>
inline WeakMemFnResult<VALUE&>::WeakMemFnResult(WeakMemFnResult_InPlaceTag,
                                                VALUE& value)
: d_value(bsls::Util::addressOf(value))
{
    // NOTHING
}

template <class VALUE>
inline WeakMemFnResult<VALUE&>::WeakMemFnResult(
    const WeakMemFnResult& original,
    bslma::Allocator*)
: d_value(original.d_value)
{
    // NOTHING
}

// ACCESSORS
template <class VALUE>
inline VALUE& WeakMemFnResult<VALUE&>::value() const BSLS_KEYWORD_NOEXCEPT
{
    // PRECONDITIONS
    BSLS_ASSERT(!isNull());

    return *d_value.value();
}

template <class VALUE>
inline bool WeakMemFnResult<VALUE&>::isNull() const BSLS_KEYWORD_NOEXCEPT
{
    return d_value.isNull();
}

// --------------------
// struct WeakMemFnUtil
// --------------------

// CLASS METHODS
template <class PROT>
inline WeakMemFn<PROT>
WeakMemFnUtil::weakMemFn(PROT memFn) BSLS_KEYWORD_NOEXCEPT
{
    return WeakMemFn<PROT>(memFn);
}

template <class PROT, class OBJ>
inline WeakMemFnInstance<PROT>
WeakMemFnUtil::weakMemFn(PROT                      memFn,
                         const bsl::weak_ptr<OBJ>& obj) BSLS_KEYWORD_NOEXCEPT
{
    return WeakMemFnInstance<PROT>(memFn, obj);
}

template <class PROT, class OBJ>
inline WeakMemFnInstance<PROT>
WeakMemFnUtil::weakMemFn(PROT                        memFn,
                         const bsl::shared_ptr<OBJ>& obj) BSLS_KEYWORD_NOEXCEPT
{
    return WeakMemFnInstance<PROT>(memFn, bsl::weak_ptr<OBJ>(obj));
}

}  // close package namespace
}  // close enterprise namespace

#endif
