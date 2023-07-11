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

// mwcex_promise.h                                                    -*-C++-*-
#ifndef INCLUDED_MWCEX_PROMISE
#define INCLUDED_MWCEX_PROMISE

//@PURPOSE: Provides a mechanism to store the result of an async operation.
//
//@CLASSES:
//  mwcex::Promise:       a promise
//  mwcex::PromiseBroken: exception class to indicate a promise was broken
//
//@SEE ALSO:
//  mwcex_future
//
//@DESCRIPTION:
// This component provides a mechanism, 'mwcex::Promise', to store a value (or
// an exception) that can later be retrieved from the corresponding
// 'mwcex::Future' object. 'mwcex::Promise' and 'mwcex::Future' communicate via
// their underlying shared state (see 'mwcex::FutureSharedState').
//
// In addition the generalized 'mwcex::Promise<R>' class template two
// specializations, 'mwcex::Promise<void>', and 'mwcex::Promise<R&>' are
// provided.
//
// This component also provides a class, 'mwcex::PromiseBroken', to indicate a
// promise was broken, that is, an 'mwcex::Promise' object was destroyed before
// its associated shared state has been made ready.
//
/// Thread safety
///-------------
// With the exception of assignment operators, as well as the 'swap' member
// function, 'mwcex::Promise' is fully thread-safe, meaning that multiple
// threads may use their own instances of the class or use a shared instance
// without further synchronization. However, note that the shared state can
// only be made ready once, making concurrent calls to 'setValue',
// 'emplaceValue' or 'setException' undefined behavior.
//
/// Usage
///-----
// Create a promise with an 'int' result type:
//..
//  mwcex::Promise<int> promise;
//..
// Obtain a future:
//..
//  mwcex::Future<int> future = promise.future();
//..
// Make the result ready:
//..
//  promise.setValue(42);
//..
// Obtain the result:
//..
//  int result = future.get();
//..

// MWC

#include <mwcex_future.h>

// BDE
#include <bsl_exception.h>
#include <bsl_functional.h>  // bsl::reference_wrapper
#include <bsl_memory.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_isbitwisemoveable.h>
#include <bslmf_movableref.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bslmf_nil.h>
#include <bslmf_util.h>
#include <bsls_assert.h>
#include <bsls_compilerfeatures.h>
#include <bsls_exceptionutil.h>
#include <bsls_keyword.h>
#include <bsls_libraryfeatures.h>
#include <bsls_systemclocktype.h>

#if BSLS_COMPILERFEATURES_SIMULATE_CPP11_FEATURES
// Include version that can be compiled with C++03
// Generated on Wed Jun 29 04:19:38 2022
// Command line: sim_cpp11_features.pl mwcex_promise.h
#define COMPILING_MWCEX_PROMISE_H
#include <mwcex_promise_cpp03.h>
#undef COMPILING_MWCEX_PROMISE_H
#else

namespace BloombergLP {

namespace mwcex {

// =============
// class Promise
// =============

/// Provides a mechanism to store a value or an exception that is later
/// acquired asynchronously via a `mwcex::Future` object created by the
/// `mwcex::Promise` object.
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
    /// `mwcex::Future<R>::SharedStateType` initialized with the specified
    /// `basicAllocator`.
    explicit Promise(bslma::Allocator* basicAllocator = 0);

    /// Create a `Promise` object holding a shared state of type
    /// `mwcex::Future<R>::SharedStateType` initialized with the specified
    /// `clockType` and `basicAllocator`.
    explicit Promise(bsls::SystemClockType::Enum clockType,
                     bslma::Allocator*           basicAllocator = 0);

    /// Create a `Promise` object that refers to and assumes management of
    /// the same shared state (if any) as the specified `original` object,
    /// and leave `original` having no shared state.
    Promise(bslmf::MovableRef<Promise> original) BSLS_KEYWORD_NOEXCEPT;

    /// Destroy this object. If `*this` has a shared state and the shared
    /// state is not ready, store a `mwcex::PromiseBroken` exception object
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

#if !BSLS_COMPILERFEATURES_SIMULATE_CPP11_FEATURES  // $var-args=9

    /// Atomically store a value into the shared state as if by direct-non-
    /// list-initializing an object of type `R` with 'bsl::forward<ARGS>(
    /// args)...' and make the state ready. If a callback is attached to the
    /// shared state, invoke, and then destroy it. Effectively calls
    /// `emplaceValue` on the underlying shared state. The behavior is
    /// undefined if the shared state is ready or if the promise has no
    /// shared state.
    ///
    /// Throws any exception thrown by the selected constructor of `R`, or
    /// any exception thrown by the attached callback. If an exception is
    /// thrown by `R`s constructor, this function has no effect. If an
    /// exception is thrown by the attached callback, the stored value stays
    /// initialized and the callback is destroyed.
    ///
    /// `R` must be constructible from `bsl::forward<ARGS>(args)...`.
    template <class... ARGS>
    void emplaceValue(ARGS&&... args);
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

#if !BSLS_COMPILERFEATURES_SIMULATE_CPP11_FEATURES  // $var-args=9
template <class R>
template <class... ARGS>
inline void Promise<R>::emplaceValue(ARGS&&... args)
{
    // PRECONDITIONS
    BSLS_ASSERT(d_sharedState);

    d_sharedState->emplaceValue(bslmf::Util::forward<ARGS>(args)...);
}
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
inline void mwcex::swap(Promise<R>& lhs, Promise<R>& rhs) BSLS_KEYWORD_NOEXCEPT
{
    lhs.swap(rhs);
}

}  // close enterprise namespace

#endif  // End C++11 code

#endif
