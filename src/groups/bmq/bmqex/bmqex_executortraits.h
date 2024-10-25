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

// bmqex_executortraits.h                                             -*-C++-*-
#ifndef INCLUDED_BMQEX_EXECUTORTRAITS
#define INCLUDED_BMQEX_EXECUTORTRAITS

//@PURPOSE: Provides a traits class to access various properties of executors.
//
//@CLASSES:
//  ExecutorTraits: executor traits.
//
//@DESCRIPTION:
// This component provides a class template, 'bmqex::ExecutorTraits', to access
// various properties of Executors. Other components provided by the 'bmqex'
// package access executors through this template, which makes it possible to
// use any class type as an executor, as long as the user-provided
// specialization of 'ExecutorTraits' implements all required functionality.

// BDE
#include <bsl_utility.h>
#include <bslmf_enableif.h>
#include <bslmf_integralconstant.h>
#include <bslmf_util.h>
#include <bslmf_voidtype.h>
#include <bsls_compilerfeatures.h>

// BDE (C++03)
#if !defined(BSLS_COMPILERFEATURES_SUPPORT_DECLTYPE) ||                       \
    !defined(BSLS_COMPILERFEATURES_SUPPORT_VARIADIC_TEMPLATES) ||             \
    !defined(BSLS_COMPILERFEATURES_SUPPORT_DEFAULT_TEMPLATE_ARGS)
#include <bsl_functional.h>
#include <bslmf_movableref.h>
#endif

namespace BloombergLP {
namespace bmqex {

#if defined(BSLS_COMPILERFEATURES_SUPPORT_DECLTYPE) &&                        \
    defined(BSLS_COMPILERFEATURES_SUPPORT_VARIADIC_TEMPLATES) &&              \
    defined(BSLS_COMPILERFEATURES_SUPPORT_DEFAULT_TEMPLATE_ARGS)
// NOTE: We assume that if the compiler supports 'decltype', variadic templates
//       and default template arguments, it also supports expression SFINAE.

// =================================
// struct ExecutorTraits_CanDispatch
// =================================

/// Provides a metafunction to detect if an executor has a proper `dispatch`
/// member function.
template <class EXECUTOR, class FUNCTION, class = void>
struct ExecutorTraits_CanDispatch : bsl::false_type {};

template <class EXECUTOR, class FUNCTION>
struct ExecutorTraits_CanDispatch<
    EXECUTOR,
    FUNCTION,
    bsl::void_t<decltype(bsl::declval<const EXECUTOR&>().dispatch(
        bsl::declval<FUNCTION>()))> > : bsl::true_type {};
#else

template <class EXECUTOR, class FUNCTION>
struct ExecutorTraits_CanDispatch {
    // Provides a metafunction to detect if an executor has a proper 'dispatch'
    // member function in C++03.
    //
    // Note that due to the lack of expression SFINAE in C++03, this
    // metafunction tries to detect the presence of a 'dispatch' member
    // function on the 'EXECUTOR' class having 'void(EXECUTOR::*)(F) const'
    // signature, 'F' being any of 'F1', 'const F1&', 'bslmf::MovableRef<F1>',
    // and 'F1' being 'bsl::function<void()>'.

    // TYPES
    typedef bsl::function<void()>                     Job;
    typedef bslmf::MovableRef<bsl::function<void()> > JobRef;

    template <class U, void (U::*)(Job) const>
    struct Test1 {};
    template <class U, void (U::*)(const Job&) const>
    struct Test2 {};
    template <class U, void (U::*)(JobRef) const>
    struct Test3 {};

    // CLASS METHODS
    template <class U>
    static char test1(Test1<U, &U::dispatch>*);
    template <class U>
    static int test1(...);

    template <class U>
    static char test2(Test2<U, &U::dispatch>*);
    template <class U>
    static int test2(...);

    template <class U>
    static char test3(Test3<U, &U::dispatch>*);
    template <class U>
    static int test3(...);

    // CLASS DATA
    static const bool value = sizeof(test1<EXECUTOR>(0)) == sizeof(char) ||
                              sizeof(test2<EXECUTOR>(0)) == sizeof(char) ||
                              sizeof(test3<EXECUTOR>(0)) == sizeof(char);
};

#endif  //    defined(BSLS_COMPILERFEATURES_SUPPORT_DECLTYPE)
        // && defined(BSLS_COMPILERFEATURES_SUPPORT_VARIADIC_TEMPLATES)
        // && defined(BSLS_COMPILERFEATURES_SUPPORT_DEFAULT_TEMPLATE_ARGS)

// =========================
// struct ExecutorTraits_Imp
// =========================

/// Provides an implementation for `ExecutorTraits`.
struct ExecutorTraits_Imp {
    // CLASS METHODS

    /// Perform `executor.post(bsl::forward<FUNCTION>(f))`.
    template <class EXECUTOR, class FUNCTION>
    static void post(const EXECUTOR& executor,
                     BSLS_COMPILERFEATURES_FORWARD_REF(FUNCTION) f);

    /// Perform `executor.dispatch(bsl::forward<FUNCTION>(f))`.
    template <class EXECUTOR, class FUNCTION>
    static typename bsl::enable_if<
        ExecutorTraits_CanDispatch<EXECUTOR, FUNCTION>::value>::type
    dispatch(const EXECUTOR& executor,
             BSLS_COMPILERFEATURES_FORWARD_REF(FUNCTION) f);

    /// Perform `executor.post(bsl::forward<FUNCTION>(f))`.
    template <class EXECUTOR, class FUNCTION>
    static typename bsl::enable_if<
        !ExecutorTraits_CanDispatch<EXECUTOR, FUNCTION>::value>::type
    dispatch(const EXECUTOR& executor,
             BSLS_COMPILERFEATURES_FORWARD_REF(FUNCTION) f);
};

// =====================
// struct ExecutorTraits
// =====================

/// Provides the default, non-specialized, executor traits to access various
/// properties of Executors.
template <class EXECUTOR>
struct ExecutorTraits {
    // TYPES

    /// Defines the type of the executor.
    typedef EXECUTOR ExecutorType;

    // CLASS METHODS

    /// Perform `executor.post(bsl::forward<FUNCTION>(f))`.
    template <class FUNCTION>
    static void post(const EXECUTOR& executor,
                     BSLS_COMPILERFEATURES_FORWARD_REF(FUNCTION) f);

    /// If `executor.dispatch(bsl::forward<FUNCTION>(f))` is a well-formed
    /// expression, perform `executor.dispatch(bsl::forward<FUNCTION>(f))`.
    /// Otherwise, perform `executor.post(bsl::forward<FUNCTION>(f))`.
    ///
    /// Note that in C++03, due to the lack of expression SFINAE, this
    /// function tries to detect the presence of a `dispatch` member
    /// function on the `EXECUTOR` class having `void(EXECUTOR::*)(F) const`
    /// signature, and falls back to `post` if no such function is detected,
    /// `F` being any of `F1`, `const F1&`, `bslmf::MovableRef<F1>`, and
    /// `F1` being `bsl::function<void()>`.
    template <class FUNCTION>
    static void dispatch(const EXECUTOR& executor,
                         BSLS_COMPILERFEATURES_FORWARD_REF(FUNCTION) f);
};

// ============================================================================
//                           INLINE DEFINITIONS
// ============================================================================

// -------------------------
// struct ExecutorTraits_Imp
// -------------------------

// CLASS METHODS
template <class EXECUTOR, class FUNCTION>
inline void
ExecutorTraits_Imp::post(const EXECUTOR& executor,
                         BSLS_COMPILERFEATURES_FORWARD_REF(FUNCTION) f)
{
    executor.post(BSLS_COMPILERFEATURES_FORWARD(FUNCTION, f));
}

template <class EXECUTOR, class FUNCTION>
inline typename bsl::enable_if<
    ExecutorTraits_CanDispatch<EXECUTOR, FUNCTION>::value>::type
ExecutorTraits_Imp::dispatch(const EXECUTOR& executor,
                             BSLS_COMPILERFEATURES_FORWARD_REF(FUNCTION) f)
{
    executor.dispatch(BSLS_COMPILERFEATURES_FORWARD(FUNCTION, f));
}

template <class EXECUTOR, class FUNCTION>
inline typename bsl::enable_if<
    !ExecutorTraits_CanDispatch<EXECUTOR, FUNCTION>::value>::type
ExecutorTraits_Imp::dispatch(const EXECUTOR& executor,
                             BSLS_COMPILERFEATURES_FORWARD_REF(FUNCTION) f)
{
    executor.post(BSLS_COMPILERFEATURES_FORWARD(FUNCTION, f));
}

// ---------------------
// struct ExecutorTraits
// ---------------------

// CLASS METHODS
template <class EXECUTOR>
template <class FUNCTION>
inline void
ExecutorTraits<EXECUTOR>::post(const EXECUTOR& executor,
                               BSLS_COMPILERFEATURES_FORWARD_REF(FUNCTION) f)
{
    ExecutorTraits_Imp::post(executor,
                             BSLS_COMPILERFEATURES_FORWARD(FUNCTION, f));
}

template <class EXECUTOR>
template <class FUNCTION>
inline void
ExecutorTraits<EXECUTOR>::dispatch(const EXECUTOR& executor,
                                   BSLS_COMPILERFEATURES_FORWARD_REF(FUNCTION)
                                       f)
{
    ExecutorTraits_Imp::dispatch(executor,
                                 BSLS_COMPILERFEATURES_FORWARD(FUNCTION, f));
}

}  // close package namespace
}  // close enterprise namespace

#endif
