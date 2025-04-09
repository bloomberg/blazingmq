// Copyright 2018-2023 Bloomberg Finance L.P.
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

// bmqex_promise_cpp03.h                                              -*-C++-*-

// Automatically generated file.  **DO NOT EDIT**

#ifndef INCLUDED_BMQEX_PROMISE_CPP03
#define INCLUDED_BMQEX_PROMISE_CPP03

//@PURPOSE: Provide C++03 implementation for bmqex_promise.h
//
//@CLASSES: See bmqex_promise.h for list of classes
//
//@SEE_ALSO: bmqex_promise
//
//@DESCRIPTION:  This component is the C++03 translation of a C++11 component,
// generated by the 'sim_cpp11_features.pl' program.  If the original header
// contains any specially delimited regions of C++11 code, then this generated
// file contains the C++03 equivalent, i.e., with variadic templates expanded
// and rvalue-references replaced by 'bslmf::MovableRef' objects.  The header
// code in this file is designed to be '#include'd into the original header
// when compiling with a C++03 compiler.  If there are no specially delimited
// regions of C++11 code, then this header contains no code and is not
// '#include'd in the original header.
//
// Generated on Wed Apr  2 14:55:22 2025
// Command line: sim_cpp11_features.pl bmqex_promise.h

#ifdef COMPILING_BMQEX_PROMISE_H

namespace BloombergLP {

namespace bmqex {

// =============
// class Promise
// =============

/// Provides a mechanism to store a value or an exception that is later
/// acquired asynchronously via a `bmqex::Future` object created by the
/// `bmqex::Promise` object.
///
/// `R` must meet the requirements of Destructible as specified in the C++
/// standard.
template <class R>
class Promise {
  private:
    // PRIVATE TYPES
    typedef typename Future<R>::SharedStateType SharedStateType;

  private:
    // PRIVATE DATA
    bsl::shared_ptr<SharedStateType> d_sharedState;

    // FRIENDS
    template <class>
    friend class Promise;

  private:
    // NOT IMPLEMENTED
    Promise(const Promise&) BSLS_KEYWORD_DELETED;
    Promise& operator=(const Promise&) BSLS_KEYWORD_DELETED;

  public:
    // CREATORS

    /// Create a `Promise` object holding a shared state of type
    /// `bmqex::Future<R>::SharedStateType` initialized with the specified
    /// `basicAllocator`.
    explicit Promise(bslma::Allocator* basicAllocator = 0);

    /// Create a `Promise` object holding a shared state of type
    /// `bmqex::Future<R>::SharedStateType` initialized with the specified
    /// `clockType` and `basicAllocator`.
    explicit Promise(bsls::SystemClockType::Enum clockType,
                     bslma::Allocator*           basicAllocator = 0);

    /// Create a `Promise` object that refers to and assumes management of
    /// the same shared state (if any) as the specified `original` object,
    /// and leave `original` having no shared state.
    Promise(bslmf::MovableRef<Promise> original) BSLS_KEYWORD_NOEXCEPT;

    /// Destroy this object. If `*this` has a shared state and the shared
    /// state is not ready, store a `bmqex::PromiseBroken` exception object
    /// into the shared state and make the state ready, then release the
    /// underlying shared state.
    ~Promise();

  public:
    // MANIPULATORS

    /// Make this promise refer to and assume management of the same shared
    /// state (if any) as the specified `rhs` promise, and leave `rhs`
    /// having no shared state. Return `*this`.
    Promise& operator=(bslmf::MovableRef<Promise> rhs) BSLS_KEYWORD_NOEXCEPT;

    /// Atomically store the specified `value` into the shared state as if
    /// by direct-non-list-initializing an object of type `R` with `value`
    /// and make the state ready. If a callback is attached to the shared
    /// state, invoke, and then destroy it. Effectively calls `setValue`
    /// on the underlying shared state. The behavior is undefined if the
    /// shared state is ready or if the promise has no shared state.
    ///
    /// Throws any exception thrown by the selected constructor of `R`, or
    /// any exception thrown by the attached callback. If an exception is
    /// thrown by `R`s constructor, this function has no effect. If an
    /// exception is thrown by the attached callback, the stored value stays
    /// initialized and the callback is destroyed.
    ///
    /// `R` must meet the requirements of CopyConstructible as specified in
    /// the C++ standard.
    void setValue(const R& value);

    /// Atomically store the specified `value` into the shared state as if
    /// by direct-non-list-initializing an object of type `R` with
    /// `bsl::move(value)` and make the state ready. If a callback is
    /// attached to the shared state, invoke, and then destroy it.
    /// Effectively calls `setValue` on the underlying shared state. The
    /// behavior is undefined if the shared state is ready or if the promise
    /// has no shared state.
    ///
    /// Throws any exception thrown by the selected constructor of `R`, or
    /// any exception thrown by the attached callback. If an exception is
    /// thrown by `R`s constructor, this function has no effect. If an
    /// exception is thrown by the attached callback, the stored value stays
    /// initialized and the callback is destroyed.
    ///
    /// `R` must meet the requirements of MoveConstructible as specified in
    /// the C++ standard.
    void setValue(bslmf::MovableRef<R> value);

#if BSLS_COMPILERFEATURES_SIMULATE_VARIADIC_TEMPLATES
// {{{ BEGIN GENERATED CODE
// Command line: sim_cpp11_features.pl bmqex_promise.h
#ifndef BMQEX_PROMISE_VARIADIC_LIMIT
#define BMQEX_PROMISE_VARIADIC_LIMIT 9
#endif
#ifndef BMQEX_PROMISE_VARIADIC_LIMIT_A
#define BMQEX_PROMISE_VARIADIC_LIMIT_A BMQEX_PROMISE_VARIADIC_LIMIT
#endif

#if BMQEX_PROMISE_VARIADIC_LIMIT_A >= 0
    void emplaceValue();
#endif  // BMQEX_PROMISE_VARIADIC_LIMIT_A >= 0

#if BMQEX_PROMISE_VARIADIC_LIMIT_A >= 1
    template <class ARGS_1>
    void emplaceValue(BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_1) args_1);
#endif  // BMQEX_PROMISE_VARIADIC_LIMIT_A >= 1

#if BMQEX_PROMISE_VARIADIC_LIMIT_A >= 2
    template <class ARGS_1, class ARGS_2>
    void emplaceValue(BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_1) args_1,
                      BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_2) args_2);
#endif  // BMQEX_PROMISE_VARIADIC_LIMIT_A >= 2

#if BMQEX_PROMISE_VARIADIC_LIMIT_A >= 3
    template <class ARGS_1, class ARGS_2, class ARGS_3>
    void emplaceValue(BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_1) args_1,
                      BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_2) args_2,
                      BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_3) args_3);
#endif  // BMQEX_PROMISE_VARIADIC_LIMIT_A >= 3

#if BMQEX_PROMISE_VARIADIC_LIMIT_A >= 4
    template <class ARGS_1, class ARGS_2, class ARGS_3, class ARGS_4>
    void emplaceValue(BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_1) args_1,
                      BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_2) args_2,
                      BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_3) args_3,
                      BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_4) args_4);
#endif  // BMQEX_PROMISE_VARIADIC_LIMIT_A >= 4

#if BMQEX_PROMISE_VARIADIC_LIMIT_A >= 5
    template <class ARGS_1,
              class ARGS_2,
              class ARGS_3,
              class ARGS_4,
              class ARGS_5>
    void emplaceValue(BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_1) args_1,
                      BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_2) args_2,
                      BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_3) args_3,
                      BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_4) args_4,
                      BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_5) args_5);
#endif  // BMQEX_PROMISE_VARIADIC_LIMIT_A >= 5

#if BMQEX_PROMISE_VARIADIC_LIMIT_A >= 6
    template <class ARGS_1,
              class ARGS_2,
              class ARGS_3,
              class ARGS_4,
              class ARGS_5,
              class ARGS_6>
    void emplaceValue(BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_1) args_1,
                      BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_2) args_2,
                      BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_3) args_3,
                      BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_4) args_4,
                      BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_5) args_5,
                      BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_6) args_6);
#endif  // BMQEX_PROMISE_VARIADIC_LIMIT_A >= 6

#if BMQEX_PROMISE_VARIADIC_LIMIT_A >= 7
    template <class ARGS_1,
              class ARGS_2,
              class ARGS_3,
              class ARGS_4,
              class ARGS_5,
              class ARGS_6,
              class ARGS_7>
    void emplaceValue(BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_1) args_1,
                      BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_2) args_2,
                      BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_3) args_3,
                      BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_4) args_4,
                      BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_5) args_5,
                      BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_6) args_6,
                      BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_7) args_7);
#endif  // BMQEX_PROMISE_VARIADIC_LIMIT_A >= 7

#if BMQEX_PROMISE_VARIADIC_LIMIT_A >= 8
    template <class ARGS_1,
              class ARGS_2,
              class ARGS_3,
              class ARGS_4,
              class ARGS_5,
              class ARGS_6,
              class ARGS_7,
              class ARGS_8>
    void emplaceValue(BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_1) args_1,
                      BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_2) args_2,
                      BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_3) args_3,
                      BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_4) args_4,
                      BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_5) args_5,
                      BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_6) args_6,
                      BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_7) args_7,
                      BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_8) args_8);
#endif  // BMQEX_PROMISE_VARIADIC_LIMIT_A >= 8

#if BMQEX_PROMISE_VARIADIC_LIMIT_A >= 9
    template <class ARGS_1,
              class ARGS_2,
              class ARGS_3,
              class ARGS_4,
              class ARGS_5,
              class ARGS_6,
              class ARGS_7,
              class ARGS_8,
              class ARGS_9>
    void emplaceValue(BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_1) args_1,
                      BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_2) args_2,
                      BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_3) args_3,
                      BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_4) args_4,
                      BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_5) args_5,
                      BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_6) args_6,
                      BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_7) args_7,
                      BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_8) args_8,
                      BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_9) args_9);
#endif  // BMQEX_PROMISE_VARIADIC_LIMIT_A >= 9

#else
    // The generated code below is a workaround for the absence of perfect
    // forwarding in some compilers.

    template <class... ARGS>
    void emplaceValue(BSLS_COMPILERFEATURES_FORWARD_REF(ARGS)... args);
// }}} END GENERATED CODE
#endif

#ifdef BSLS_LIBRARYFEATURES_HAS_CPP11_EXCEPTION_HANDLING
    /// Atomically store the specified `exception` pointer into the shared
    /// state and make the state ready. If a callback is attached to the
    /// shared state, invoke, and then destroy it. Effectively calls
    /// `setException` on the underlying shared state. The behavior is
    /// undefined if the shared state is ready or if the promise has no
    /// shared state.
    ///
    /// Throws any exception thrown by the attached callback. If an
    /// exception is thrown, the stored exception stays initialized and the
    /// callback is destroyed.
    void setException(bsl::exception_ptr exception);
#endif

    /// Atomically store the specified `exception` object into the shared
    /// state as if by direct-non-list-initializing an object of type
    /// `bsl::decay_t<EXCEPTION>` with `bsl::forward<EXCEPTION>(EXCEPTION)`
    /// and make the state ready. If a callback is attached to the shared
    /// state, invoke, and then destroy it. Effectively calls
    /// `setException` on the underlying shared state. The behavior is
    /// undefined if the shared state is ready or if the promise has no
    /// shared state.
    ///
    /// Throws any exception thrown by the selected constructor of
    /// `bsl::decay_t<EXCEPTION>`, any exception thrown by the attached
    /// callback, or `bsl::bad_alloc` if memory allocation fails. If an
    /// exception is thrown by `bsl::decay_t<EXCEPTION>`s constructor or due
    /// to memory allocation failure, this function has no effect. If an
    /// exception is thrown by the attached callback, the stored exception
    /// stays initialized and the callback is destroyed.
    ///
    /// `bsl::decay_t<EXCEPTION>` must meet the requirements of Destructible
    /// and CopyConstructible as specified in the C++ standard.
    template <class EXCEPTION>
    void setException(BSLS_COMPILERFEATURES_FORWARD_REF(EXCEPTION) exception);

    /// Swap the contents of `*this` and `other`.
    void swap(Promise& other) BSLS_KEYWORD_NOEXCEPT;

  public:
    // ACCESSORS

    /// Return a future associated with the shared state owned by `*this`.
    /// The behavior is undefined unless the promise has a shared state.
    Future<R> future() const BSLS_KEYWORD_NOEXCEPT;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(Promise, bslma::UsesBslmaAllocator)
    BSLMF_NESTED_TRAIT_DECLARATION(Promise, bslmf::IsBitwiseMoveable)
};

// ===================
// class Promise<void>
// ===================

/// Provides a specialization of `Promise` for `void` result type.
template <>
class Promise<void> : private Promise<bslmf::Nil> {
  private:
    // PRIVATE TYPES
    typedef Promise<bslmf::Nil> Impl;

  private:
    // NOT IMPLEMENTED
    Promise(const Promise&) BSLS_KEYWORD_DELETED;
    Promise& operator=(const Promise&) BSLS_KEYWORD_DELETED;

  public:
    // CREATORS

    /// Same as for the non-specialized class template.
    explicit Promise(bslma::Allocator* basicAllocator = 0);

    /// Same as for the non-specialized class template.
    explicit Promise(bsls::SystemClockType::Enum clockType,
                     bslma::Allocator*           basicAllocator = 0);

    /// Same as for the non-specialized class template.
    Promise(bslmf::MovableRef<Promise> original) BSLS_KEYWORD_NOEXCEPT;

  public:
    // MANIPULATORS

    /// Same as for the non-specialized class template.
    Promise& operator=(bslmf::MovableRef<Promise> rhs) BSLS_KEYWORD_NOEXCEPT;

    /// Atomically make the shared state ready. If a callback is attached to
    /// the shared state, invoke, and then destroy it. Effectively calls
    /// `setValue` on the underlying shared state. The behavior is
    /// undefined if the shared state is ready or if the promise has no
    /// shared state.
    ///
    /// Throws any exception thrown by the attached callback. If an
    /// exception is thrown, the stored value stays initialized and the
    /// callback is destroyed.
    void setValue();

#ifdef BSLS_LIBRARYFEATURES_HAS_CPP11_EXCEPTION_HANDLING
    /// Same as for the non-specialized class template.
    void setException(bsl::exception_ptr exception);
#endif

    /// Same as for the non-specialized class template.
    template <class EXCEPTION>
    void setException(BSLS_COMPILERFEATURES_FORWARD_REF(EXCEPTION) exception);

    /// Same as for the non-specialized class template.
    void swap(Promise& other) BSLS_KEYWORD_NOEXCEPT;

  public:
    // ACCESSORS

    /// Same as for the non-specialized class template.
    Future<void> future() const BSLS_KEYWORD_NOEXCEPT;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(Promise, bslma::UsesBslmaAllocator)
    BSLMF_NESTED_TRAIT_DECLARATION(Promise, bslmf::IsBitwiseMoveable)
};

// =================
// class Promise<R&>
// =================

/// Provides a specialization of `Promise` for reference result types.
template <class R>
class Promise<R&> : private Promise<bsl::reference_wrapper<R> > {
  private:
    // PRIVATE TYPES
    typedef Promise<bsl::reference_wrapper<R> > Impl;

  private:
    // NOT IMPLEMENTED
    Promise(const Promise&) BSLS_KEYWORD_DELETED;
    Promise& operator=(const Promise&) BSLS_KEYWORD_DELETED;

  public:
    // CREATORS

    /// Same as for the non-specialized class template.
    explicit Promise(bslma::Allocator* basicAllocator = 0);

    /// Same as for the non-specialized class template.
    explicit Promise(bsls::SystemClockType::Enum clockType,
                     bslma::Allocator*           basicAllocator = 0);

    /// Same as for the non-specialized class template.
    Promise(bslmf::MovableRef<Promise> original) BSLS_KEYWORD_NOEXCEPT;

  public:
    // MANIPULATORS

    /// Same as for the non-specialized class template.
    Promise& operator=(bslmf::MovableRef<Promise> rhs) BSLS_KEYWORD_NOEXCEPT;

    /// Atomically store the specified `value` reference into the shared
    /// state and make the state ready. If a callback is attached to the
    /// shared state, invoke, and then destroy it. Effectively calls
    /// `setValue` on the underlying shared state. The behavior is
    /// undefined if the shared state is ready or if the promise has no
    /// shared state.
    ///
    /// Throws any exception thrown by the attached callback. If an
    /// exception is thrown, the stored reference stays initialized and the
    /// callback is destroyed.
    void setValue(R& value);

#ifdef BSLS_LIBRARYFEATURES_HAS_CPP11_EXCEPTION_HANDLING
    /// Same as for the non-specialized class template.
    void setException(bsl::exception_ptr exception);
#endif

    /// Same as for the non-specialized class template.
    template <class EXCEPTION>
    void setException(BSLS_COMPILERFEATURES_FORWARD_REF(EXCEPTION) exception);

    /// Same as for the non-specialized class template.
    void swap(Promise& other) BSLS_KEYWORD_NOEXCEPT;

  public:
    // ACCESSORS

    /// Same as for the non-specialized class template.
    Future<R&> future() const BSLS_KEYWORD_NOEXCEPT;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(Promise, bslma::UsesBslmaAllocator)
    BSLMF_NESTED_TRAIT_DECLARATION(Promise, bslmf::IsBitwiseMoveable)
};

// ===================
// class PromiseBroken
// ===================

/// Provide an exception class to indicate a promise was broken.
class PromiseBroken : public bsl::exception {
  public:
    // CREATORS

    /// Create a `PromiseBroken` object.
    PromiseBroken() BSLS_KEYWORD_NOEXCEPT;

  public:
    // ACCESSORS

    /// Return a pointer to the string literal "PromiseBroken", with a
    /// storage duration of the lifetime of the program.
    const char* what() const BSLS_EXCEPTION_WHAT_NOTHROW BSLS_KEYWORD_OVERRIDE;
};

// FREE OPERATORS

/// Swap the contents of `lhs` and `rhs`.
template <class R>
void swap(Promise<R>& lhs, Promise<R>& rhs) BSLS_KEYWORD_NOEXCEPT;

// ============================================================================
//                           INLINE DEFINITIONS
// ============================================================================

// -------------
// class Promise
// -------------

// CREATORS
template <class R>
inline Promise<R>::Promise(bslma::Allocator* basicAllocator)
: d_sharedState(bsl::allocate_shared<SharedStateType>(basicAllocator))
{
    // NOTHING
}

template <class R>
inline Promise<R>::Promise(bsls::SystemClockType::Enum clockType,
                           bslma::Allocator*           basicAllocator)
: d_sharedState(
      bsl::allocate_shared<SharedStateType>(basicAllocator, clockType))
{
    // NOTHING
}

template <class R>
inline Promise<R>::Promise(bslmf::MovableRef<Promise> original)
    BSLS_KEYWORD_NOEXCEPT
: d_sharedState(bslmf::MovableRefUtil::move(
      bslmf::MovableRefUtil::access(original).d_sharedState))
{
    // NOTHING
}

template <class R>
inline Promise<R>::~Promise()
{
    if (d_sharedState && !d_sharedState->isReady()) {
        // The shared state is not ready. Store a 'PromiseBroken' exception.
        d_sharedState->setException(PromiseBroken());
    }
}

// MANIPULATORS
template <class R>
inline Promise<R>&
Promise<R>::operator=(bslmf::MovableRef<Promise> rhs) BSLS_KEYWORD_NOEXCEPT
{
    d_sharedState = bslmf::MovableRefUtil::move(
        bslmf::MovableRefUtil::access(rhs).d_sharedState);
    return *this;
}

template <class R>
inline void Promise<R>::setValue(const R& value)
{
    // PRECONDITIONS
    BSLS_ASSERT(d_sharedState);

    d_sharedState->setValue(value);
}

template <class R>
inline void Promise<R>::setValue(bslmf::MovableRef<R> value)
{
    // PRECONDITIONS
    BSLS_ASSERT(d_sharedState);

    d_sharedState->setValue(bslmf::MovableRefUtil::move(value));
}

#if BSLS_COMPILERFEATURES_SIMULATE_VARIADIC_TEMPLATES
// {{{ BEGIN GENERATED CODE
// Command line: sim_cpp11_features.pl bmqex_promise.h
#ifndef BMQEX_PROMISE_VARIADIC_LIMIT
#define BMQEX_PROMISE_VARIADIC_LIMIT 9
#endif
#ifndef BMQEX_PROMISE_VARIADIC_LIMIT_B
#define BMQEX_PROMISE_VARIADIC_LIMIT_B BMQEX_PROMISE_VARIADIC_LIMIT
#endif
#if BMQEX_PROMISE_VARIADIC_LIMIT_B >= 0
template <class R>
inline void Promise<R>::emplaceValue()
{
    BSLS_ASSERT(d_sharedState);

    d_sharedState->emplaceValue();
}
#endif  // BMQEX_PROMISE_VARIADIC_LIMIT_B >= 0

#if BMQEX_PROMISE_VARIADIC_LIMIT_B >= 1
template <class R>
template <class ARGS_1>
inline void Promise<R>::emplaceValue(BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_1)
                                         args_1)
{
    BSLS_ASSERT(d_sharedState);

    d_sharedState->emplaceValue(bslmf::Util::forward<ARGS_1>(args_1));
}
#endif  // BMQEX_PROMISE_VARIADIC_LIMIT_B >= 1

#if BMQEX_PROMISE_VARIADIC_LIMIT_B >= 2
template <class R>
template <class ARGS_1, class ARGS_2>
inline void
Promise<R>::emplaceValue(BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_1) args_1,
                         BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_2) args_2)
{
    BSLS_ASSERT(d_sharedState);

    d_sharedState->emplaceValue(bslmf::Util::forward<ARGS_1>(args_1),
                                bslmf::Util::forward<ARGS_2>(args_2));
}
#endif  // BMQEX_PROMISE_VARIADIC_LIMIT_B >= 2

#if BMQEX_PROMISE_VARIADIC_LIMIT_B >= 3
template <class R>
template <class ARGS_1, class ARGS_2, class ARGS_3>
inline void
Promise<R>::emplaceValue(BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_1) args_1,
                         BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_2) args_2,
                         BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_3) args_3)
{
    BSLS_ASSERT(d_sharedState);

    d_sharedState->emplaceValue(bslmf::Util::forward<ARGS_1>(args_1),
                                bslmf::Util::forward<ARGS_2>(args_2),
                                bslmf::Util::forward<ARGS_3>(args_3));
}
#endif  // BMQEX_PROMISE_VARIADIC_LIMIT_B >= 3

#if BMQEX_PROMISE_VARIADIC_LIMIT_B >= 4
template <class R>
template <class ARGS_1, class ARGS_2, class ARGS_3, class ARGS_4>
inline void
Promise<R>::emplaceValue(BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_1) args_1,
                         BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_2) args_2,
                         BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_3) args_3,
                         BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_4) args_4)
{
    BSLS_ASSERT(d_sharedState);

    d_sharedState->emplaceValue(bslmf::Util::forward<ARGS_1>(args_1),
                                bslmf::Util::forward<ARGS_2>(args_2),
                                bslmf::Util::forward<ARGS_3>(args_3),
                                bslmf::Util::forward<ARGS_4>(args_4));
}
#endif  // BMQEX_PROMISE_VARIADIC_LIMIT_B >= 4

#if BMQEX_PROMISE_VARIADIC_LIMIT_B >= 5
template <class R>
template <class ARGS_1, class ARGS_2, class ARGS_3, class ARGS_4, class ARGS_5>
inline void
Promise<R>::emplaceValue(BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_1) args_1,
                         BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_2) args_2,
                         BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_3) args_3,
                         BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_4) args_4,
                         BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_5) args_5)
{
    BSLS_ASSERT(d_sharedState);

    d_sharedState->emplaceValue(bslmf::Util::forward<ARGS_1>(args_1),
                                bslmf::Util::forward<ARGS_2>(args_2),
                                bslmf::Util::forward<ARGS_3>(args_3),
                                bslmf::Util::forward<ARGS_4>(args_4),
                                bslmf::Util::forward<ARGS_5>(args_5));
}
#endif  // BMQEX_PROMISE_VARIADIC_LIMIT_B >= 5

#if BMQEX_PROMISE_VARIADIC_LIMIT_B >= 6
template <class R>
template <class ARGS_1,
          class ARGS_2,
          class ARGS_3,
          class ARGS_4,
          class ARGS_5,
          class ARGS_6>
inline void
Promise<R>::emplaceValue(BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_1) args_1,
                         BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_2) args_2,
                         BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_3) args_3,
                         BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_4) args_4,
                         BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_5) args_5,
                         BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_6) args_6)
{
    BSLS_ASSERT(d_sharedState);

    d_sharedState->emplaceValue(bslmf::Util::forward<ARGS_1>(args_1),
                                bslmf::Util::forward<ARGS_2>(args_2),
                                bslmf::Util::forward<ARGS_3>(args_3),
                                bslmf::Util::forward<ARGS_4>(args_4),
                                bslmf::Util::forward<ARGS_5>(args_5),
                                bslmf::Util::forward<ARGS_6>(args_6));
}
#endif  // BMQEX_PROMISE_VARIADIC_LIMIT_B >= 6

#if BMQEX_PROMISE_VARIADIC_LIMIT_B >= 7
template <class R>
template <class ARGS_1,
          class ARGS_2,
          class ARGS_3,
          class ARGS_4,
          class ARGS_5,
          class ARGS_6,
          class ARGS_7>
inline void
Promise<R>::emplaceValue(BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_1) args_1,
                         BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_2) args_2,
                         BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_3) args_3,
                         BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_4) args_4,
                         BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_5) args_5,
                         BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_6) args_6,
                         BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_7) args_7)
{
    BSLS_ASSERT(d_sharedState);

    d_sharedState->emplaceValue(bslmf::Util::forward<ARGS_1>(args_1),
                                bslmf::Util::forward<ARGS_2>(args_2),
                                bslmf::Util::forward<ARGS_3>(args_3),
                                bslmf::Util::forward<ARGS_4>(args_4),
                                bslmf::Util::forward<ARGS_5>(args_5),
                                bslmf::Util::forward<ARGS_6>(args_6),
                                bslmf::Util::forward<ARGS_7>(args_7));
}
#endif  // BMQEX_PROMISE_VARIADIC_LIMIT_B >= 7

#if BMQEX_PROMISE_VARIADIC_LIMIT_B >= 8
template <class R>
template <class ARGS_1,
          class ARGS_2,
          class ARGS_3,
          class ARGS_4,
          class ARGS_5,
          class ARGS_6,
          class ARGS_7,
          class ARGS_8>
inline void
Promise<R>::emplaceValue(BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_1) args_1,
                         BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_2) args_2,
                         BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_3) args_3,
                         BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_4) args_4,
                         BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_5) args_5,
                         BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_6) args_6,
                         BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_7) args_7,
                         BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_8) args_8)
{
    BSLS_ASSERT(d_sharedState);

    d_sharedState->emplaceValue(bslmf::Util::forward<ARGS_1>(args_1),
                                bslmf::Util::forward<ARGS_2>(args_2),
                                bslmf::Util::forward<ARGS_3>(args_3),
                                bslmf::Util::forward<ARGS_4>(args_4),
                                bslmf::Util::forward<ARGS_5>(args_5),
                                bslmf::Util::forward<ARGS_6>(args_6),
                                bslmf::Util::forward<ARGS_7>(args_7),
                                bslmf::Util::forward<ARGS_8>(args_8));
}
#endif  // BMQEX_PROMISE_VARIADIC_LIMIT_B >= 8

#if BMQEX_PROMISE_VARIADIC_LIMIT_B >= 9
template <class R>
template <class ARGS_1,
          class ARGS_2,
          class ARGS_3,
          class ARGS_4,
          class ARGS_5,
          class ARGS_6,
          class ARGS_7,
          class ARGS_8,
          class ARGS_9>
inline void
Promise<R>::emplaceValue(BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_1) args_1,
                         BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_2) args_2,
                         BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_3) args_3,
                         BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_4) args_4,
                         BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_5) args_5,
                         BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_6) args_6,
                         BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_7) args_7,
                         BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_8) args_8,
                         BSLS_COMPILERFEATURES_FORWARD_REF(ARGS_9) args_9)
{
    BSLS_ASSERT(d_sharedState);

    d_sharedState->emplaceValue(bslmf::Util::forward<ARGS_1>(args_1),
                                bslmf::Util::forward<ARGS_2>(args_2),
                                bslmf::Util::forward<ARGS_3>(args_3),
                                bslmf::Util::forward<ARGS_4>(args_4),
                                bslmf::Util::forward<ARGS_5>(args_5),
                                bslmf::Util::forward<ARGS_6>(args_6),
                                bslmf::Util::forward<ARGS_7>(args_7),
                                bslmf::Util::forward<ARGS_8>(args_8),
                                bslmf::Util::forward<ARGS_9>(args_9));
}
#endif  // BMQEX_PROMISE_VARIADIC_LIMIT_B >= 9

#else
// The generated code below is a workaround for the absence of perfect
// forwarding in some compilers.
template <class R>
template <class... ARGS>
inline void
Promise<R>::emplaceValue(BSLS_COMPILERFEATURES_FORWARD_REF(ARGS)... args)
{
    BSLS_ASSERT(d_sharedState);

    d_sharedState->emplaceValue(bslmf::Util::forward<ARGS>(args)...);
}
// }}} END GENERATED CODE
#endif

#ifdef BSLS_LIBRARYFEATURES_HAS_CPP11_EXCEPTION_HANDLING
template <class R>
inline void Promise<R>::setException(bsl::exception_ptr exception)
{
    // PRECONDITIONS
    BSLS_ASSERT(d_sharedState);

    d_sharedState->setException(exception);
}
#endif

template <class R>
template <class EXCEPTION>
inline void
Promise<R>::setException(BSLS_COMPILERFEATURES_FORWARD_REF(EXCEPTION)
                             exception)
{
    // PRECONDITIONS
    BSLS_ASSERT(d_sharedState);

    d_sharedState->setException(
        BSLS_COMPILERFEATURES_FORWARD(EXCEPTION, exception));
}

template <class R>
inline void Promise<R>::swap(Promise& other) BSLS_KEYWORD_NOEXCEPT
{
    d_sharedState.swap(other.d_sharedState);
}

// ACCESSORS
template <class R>
inline Future<R> Promise<R>::future() const BSLS_KEYWORD_NOEXCEPT
{
    // PRECONDITIONS
    BSLS_ASSERT(d_sharedState);

    return Future<R>(d_sharedState);
}

// -------------------
// class Promise<void>
// -------------------

// CREATORS
inline Promise<void>::Promise(bslma::Allocator* basicAllocator)
: Impl(basicAllocator)
{
    // NOTHING
}

inline Promise<void>::Promise(bsls::SystemClockType::Enum clockType,
                              bslma::Allocator*           basicAllocator)
: Impl(clockType, basicAllocator)
{
    // NOTHING
}

inline Promise<void>::Promise(bslmf::MovableRef<Promise> original)
    BSLS_KEYWORD_NOEXCEPT
: Impl(bslmf::MovableRefUtil::move(
      static_cast<Impl&>(bslmf::MovableRefUtil::access(original))))
{
    // NOTHING
}

// MANIPULATORS
inline Promise<void>&
Promise<void>::operator=(bslmf::MovableRef<Promise> rhs) BSLS_KEYWORD_NOEXCEPT
{
    static_cast<Impl&>(*this) = bslmf::MovableRefUtil::move(
        static_cast<Impl&>(bslmf::MovableRefUtil::access(rhs)));
    return *this;
}

inline void Promise<void>::setValue()
{
    Impl::setValue(bslmf::Nil());
}

#ifdef BSLS_LIBRARYFEATURES_HAS_CPP11_EXCEPTION_HANDLING
inline void Promise<void>::setException(bsl::exception_ptr exception)
{
    Impl::setException(exception);
}
#endif

template <class EXCEPTION>
inline void
Promise<void>::setException(BSLS_COMPILERFEATURES_FORWARD_REF(EXCEPTION)
                                exception)
{
    Impl::setException(BSLS_COMPILERFEATURES_FORWARD(EXCEPTION, exception));
}

inline void Promise<void>::swap(Promise& other) BSLS_KEYWORD_NOEXCEPT
{
    Impl::swap(other);
}

// ACCESSORS
inline Future<void> Promise<void>::future() const BSLS_KEYWORD_NOEXCEPT
{
    // PRECONDITIONS
    BSLS_ASSERT(this->d_sharedState);

    return Future<void>(this->d_sharedState);
}

// -----------------
// class Promise<R&>
// -----------------

// CREATORS
template <class R>
inline Promise<R&>::Promise(bslma::Allocator* basicAllocator)
: Impl(basicAllocator)
{
    // NOTHING
}

template <class R>
inline Promise<R&>::Promise(bsls::SystemClockType::Enum clockType,
                            bslma::Allocator*           basicAllocator)
: Impl(clockType, basicAllocator)
{
    // NOTHING
}

template <class R>
inline Promise<R&>::Promise(bslmf::MovableRef<Promise> original)
    BSLS_KEYWORD_NOEXCEPT
: Impl(bslmf::MovableRefUtil::move(
      static_cast<Impl&>(bslmf::MovableRefUtil::access(original))))
{
    // NOTHING
}

// MANIPULATORS
template <class R>
inline Promise<R&>&
Promise<R&>::operator=(bslmf::MovableRef<Promise> rhs) BSLS_KEYWORD_NOEXCEPT
{
    static_cast<Impl&>(*this) = bslmf::MovableRefUtil::move(
        static_cast<Impl&>(bslmf::MovableRefUtil::access(rhs)));
    return *this;
}

template <class R>
inline void Promise<R&>::setValue(R& value)
{
    Impl::setValue(bsl::ref(value));
}

#ifdef BSLS_LIBRARYFEATURES_HAS_CPP11_EXCEPTION_HANDLING
template <class R>
inline void Promise<R&>::setException(bsl::exception_ptr exception)
{
    Impl::setException(exception);
}
#endif

template <class R>
template <class EXCEPTION>
inline void
Promise<R&>::setException(BSLS_COMPILERFEATURES_FORWARD_REF(EXCEPTION)
                              exception)
{
    Impl::setException(BSLS_COMPILERFEATURES_FORWARD(EXCEPTION, exception));
}

template <class R>
inline void Promise<R&>::swap(Promise& other) BSLS_KEYWORD_NOEXCEPT
{
    Impl::swap(other);
}

// ACCESSORS
template <class R>
inline Future<R&> Promise<R&>::future() const BSLS_KEYWORD_NOEXCEPT
{
    // PRECONDITIONS
    BSLS_ASSERT(this->d_sharedState);

    return Future<R&>(this->d_sharedState);
}

}  // close package namespace

// FREE OPERATORS
template <class R>
inline void bmqex::swap(Promise<R>& lhs, Promise<R>& rhs) BSLS_KEYWORD_NOEXCEPT
{
    lhs.swap(rhs);
}

}  // close enterprise namespace

#else  // if ! defined(DEFINED_BMQEX_PROMISE_H)
#error Not valid except when included from bmqex_promise.h
#endif  // ! defined(COMPILING_BMQEX_PROMISE_H)

#endif  // ! defined(INCLUDED_BMQEX_PROMISE_CPP03)
