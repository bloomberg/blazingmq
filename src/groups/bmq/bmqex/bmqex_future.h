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

// bmqex_future.h                                                     -*-C++-*-
#ifndef INCLUDED_BMQEX_FUTURE
#define INCLUDED_BMQEX_FUTURE

//@PURPOSE: Provides a mechanism to access the result of an async operation.
//
//@CLASSES:
//  bmqex::Future:            an async result provider
//  bmqex::FutureStatus:      codes returned by public API
//  bmqex::FutureResult:      an established async result provider
//  bmqex::FutureSharedState: a shared state
//
//@SEE ALSO:
//  bmqex_promise
//
//@DESCRIPTION:
// This component provides a mechanism, 'bmqex::Future', to access the result
// of an asynchronous operation. In addition the generalized 'bmqex::Future<R>'
// class template two specializations, 'bmqex::Future<void>', and
// 'bmqex::Future<R&>' are provided.
//
// This component also provides a mechanism, 'bmqex::FutureResult', to access
// the result of an asynchronous operation that is already established. Objects
// of that type are used as parameters for notification callbacks (see usage
// examples below). In addition the generalized 'bmqex::FutureResult<R>' class
// template two specializations, 'bmqex::FutureResult<void>', and
// 'bmqex::FutureResult<R&>' are provided.
//
// Behind a 'bmqex::Future' or 'bmqex::FutureResult' lies an
// 'bmqex::FutureSharedState' object called the shared state. The shared state
// is what will actually hold the async result (or the exception). Note that
// Future objects are not meant to be created by users directly, but rather to
// be obtained from an asynchronous result provider (an example of such
// provider is 'bmqex::Promise'). Result providers do create a shared state and
// assign it the result (or the exception) when it becomes ready. That result
// may then be retrieved via an associated future object.
//
/// Thread safety
///-------------
// With the exception of assignment operators, as well as the 'swap' member
// function, 'bmqex::Future' is thread-safe, meaning that multiple threads may
// use their own instances of the class or use a shared instance without
// further synchronization.
//
// With the exception of assignment operators, as well as the 'swap' member
// function, 'bmqex::FutureResult' is thread-safe, meaning that multiple
// threads may use their own instances of the class or use a shared instance
// without further synchronization.
//
// 'bmqex::FutureSharedState' is fully thread-safe, meaning that multiple
// threads may use their own instances of the class or use a shared instance
// without further synchronization. However, note that the shared state can
// only be made ready once, making concurrent calls to 'setValue',
//'emplaceValue' or 'setException' undefined behavior. The same rule
// applies to attaching the notification callback via a call to 'whenReady'.
//
/// Usage
///-----
// The simplest way to obtain a future object is from an async result
// provider, such as 'bmqex::Promise'.
//..
//  bmqex::Promise<int> promise;
//  bmqex::Future<int>  future = promise.future();
//..
// This future does not contain a result yet. The result (in this example an
// 'int') is to be supplied by the result provider, possibly from another
// thread.
//..
//  // supply the result
//  promise.setValue(42);
//..
// The async result can be retrieved via a call to 'get' on the future
// object. 'get' will block the calling thread until the result is available.
//..
//  int result = future.get();
//  BSLS_ASSERT(result == 42);
//..
// It is also possible to retrieve the result asynchronously, by attaching a
// callback to the future via a call to 'whenReady'.
//..
//  future.whenReady([](FutureResult<int> result) {
//                       BSLS_ASSERT(result.get() == 42);
//                   });
//..
// The attached callback shall accept an argument of type
// 'bmqex::FutureResult<R>', where 'R' is the type of the result. Objects of
// this type are implicitly convertible to the result type, so it is also
// possible for the callback to accept an argument of type 'R', as long as 'R'
// is not 'void'.
//..
//  future.whenReady([](int result) {
//                       BSLS_ASSERT(result == 42);
//                   });
//..
// Currently, only one callback can be attached to a future, or more
// specifically, to the future's associated shared state. An important thing to
// keep in mind is that the attached callback is always invoked from the
// result-supplier thread.

#include <bmqu_objectplaceholder.h>

// BDE
#include <bdlb_nullablevalue.h>
#include <bdlf_noop.h>
#include <bsl_algorithm.h>  // bsl::swap
#include <bsl_exception.h>
#include <bsl_functional.h>  // bsl::reference_wrapper
#include <bsl_memory.h>
#include <bslalg_constructorproxy.h>
#include <bslma_allocator.h>
#include <bslma_constructionutil.h>
#include <bslma_default.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_decay.h>
#include <bslmf_isbitwisemoveable.h>
#include <bslmf_movableref.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bslmf_nil.h>
#include <bslmf_removeconst.h>
#include <bslmf_util.h>
#include <bslmt_condition.h>
#include <bslmt_lockguard.h>
#include <bslmt_mutex.h>
#include <bsls_annotation.h>
#include <bsls_assert.h>
#include <bsls_compilerfeatures.h>
#include <bsls_keyword.h>
#include <bsls_libraryfeatures.h>
#include <bsls_objectbuffer.h>
#include <bsls_platform.h>
#include <bsls_systemclocktype.h>
#include <bsls_systemtime.h>
#include <bsls_timeinterval.h>

#if BSLS_COMPILERFEATURES_SIMULATE_CPP11_FEATURES
// clang-format off
// Include version that can be compiled with C++03
// Generated on Thu May 22 13:07:14 2025
// Command line: sim_cpp11_features.pl bmqex_future.h

# define COMPILING_BMQEX_FUTURE_H
# include <bmqex_future_cpp03.h>
# undef COMPILING_BMQEX_FUTURE_H

// clang-format on
#else

namespace BloombergLP {

namespace bmqex {

template <class>
class FutureResult;
template <class>
class FutureSharedState;

// ======================
// class Future_Exception
// ======================

/// Provides a polymorphic exception wrapper that can hold and emit any type
/// of exception.
class Future_Exception {
  private:
    // PRIVATE TYPES

    /// Provides an interface used to implement the type erasure technique.
    /// When creating a polymorphic exception with a target of type `E`, an
    /// instance of derived class template `Target<E>` is instantiated and
    /// stored via a pointer to its base class (this one). Then, calls to
    /// `bmqex::Future_Exception`s public methods are forwarded to this
    /// class.
    class TargetBase {
      public:
        // CREATORS

        /// Destroy this object and the contained exception target with it.
        virtual ~TargetBase();

      public:
        // ACCESSORS

        /// Throw a copy of the contained exception object.
        BSLS_ANNOTATION_NORETURN virtual void emit() const = 0;
    };

    /// Provides an implementation of the `TargetBase` interface containing
    /// the exception target.
    template <class EXCEPTION>
    class Target : public TargetBase {
      private:
        // PRIVATE DATA
        bslalg::ConstructorProxy<EXCEPTION> d_exception;

      private:
        // NOT IMPLEMENTED
        Target(const Target&) BSLS_KEYWORD_DELETED;
        Target& operator=(const Target&) BSLS_KEYWORD_DELETED;

      public:
        // CREATORS

        /// Create a `Target` object containing an exception target of type
        /// `EXCEPTION` direct-non-list-initialized by
        /// `bsl::forward<EXCEPTION_PARAM>(exception)`. Specify an
        /// `allocator` used to supply memory.
        template <class EXCEPTION_PARAM>
        Target(BSLS_COMPILERFEATURES_FORWARD_REF(EXCEPTION_PARAM) exception,
               bslma::Allocator* allocator);

      public:
        // ACCESSORS

        /// Implements `TargetBase::emit`.
        BSLS_ANNOTATION_NORETURN void emit() const BSLS_KEYWORD_OVERRIDE;
    };

  private:
    // PRIVATE TYPES

    /// Provides a "small" dummy object which size is used to calculate the
    /// size of the on-stack buffer used for optimization.
    struct Dummy : public bdlf::NoOp {
        void* d_padding[3];
    };

  private:
    // PRIVATE DATA

    // Uses an on-stack buffer to allocate memory for "small" objects, and
    // falls back to requesting memory from the supplied allocator if
    // the buffer is not large enough. Note that the size of the on-stack
    // buffer is an arbitrary value.
    bmqu::ObjectPlaceHolder<sizeof(Target<Dummy>)> d_target;

  private:
    // NOT IMPLEMENTED
    Future_Exception(const Future_Exception&) BSLS_KEYWORD_DELETED;
    Future_Exception& operator=(const Future_Exception&) BSLS_KEYWORD_DELETED;

  public:
    // CREATORS

    /// Create a `Future_Exception` object containing an exception target
    /// of type `bsl::decay_t<EXCEPTION>` direct-non-list-initialized by
    /// `bsl::forward<EXCEPTION>(exception)`. Specify an `allocator` used to
    /// supply memory.
    ///
    /// `bsl::decay_t<EXCEPTION>` must meet the requirements of Destructible
    /// and CopyConstructible as specified in the C++ standard.
    template <class EXCEPTION>
    Future_Exception(BSLS_COMPILERFEATURES_FORWARD_REF(EXCEPTION) exception,
                     bslma::Allocator* allocator);

    /// Destroy this object and the contained exception target with it.
    ~Future_Exception();

  public:
    // ACCESSORS
#ifdef BSLS_PLATFORM_CMP_CLANG
    BSLS_ANNOTATION_NORETURN void emit() const;
#else
    void emit() const;
#endif
    // Throw a copy of the contained exception object.  Implementation
    // note: this function is annotated as 'noreturn' only on clang because
    // only that compiler needs this annotation in our builds.

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(Future_Exception, bslma::UsesBslmaAllocator)
};

// =====================
// class Future_Callback
// =====================

/// Provides a polymorphic callback wrapper with small buffer optimization.
class Future_Callback {
  public:
    // TYPES

    /// Provides a tag type to specify the type of the async result accepted
    /// by the callback,
    template <class R>
    struct AsyncResultTypeTag {};

  private:
    // PRIVATE TYPES

    /// Provides an interface used to implement the type erasure technique.
    /// When creating a polymorphic wrapper with an async result type `R`
    /// and a callback of type `F`, an instance of derived class template
    /// `Target<R, F>` is instantiated and stored via a pointer to its base
    /// class (this one). Then, calls to `bmqex::Future_Callback`s public
    /// methods are forwarded to this class.
    class TargetBase {
      public:
        // CREATORS

        /// Destroy this object and the contained function object with it.
        virtual ~TargetBase();

      public:
        // MANIPULATORS

        /// Perform `bsl::move(f)(FutureResult<R>((S*)sharedState))`, where
        /// `f` is the contained function object, `R` the type of the async
        /// result, and `S` is `Future<R>::SharedStateType`.
        virtual void invoke(void* sharedState) = 0;
    };

    /// Provides an implementation of the `TargetBase` interface containing
    /// the function object.
    template <class R, class FUNCTION>
    class Target : public TargetBase {
      private:
        // PRIVATE DATA
        bslalg::ConstructorProxy<FUNCTION> d_function;

      private:
        // NOT IMPLEMENTED
        Target(const Target&) BSLS_KEYWORD_DELETED;
        Target& operator=(const Target&) BSLS_KEYWORD_DELETED;

      public:
        // CREATORS

        /// Create a `Target` object containing a function object of type
        /// `FUNCTION` direct-non-list-initialized by
        /// `bsl::forward<FUNCTION_PARAM>(function)`. Specify an `allocator`
        /// used to supply memory.
        template <class FUNCTION_PARAM>
        Target(BSLS_COMPILERFEATURES_FORWARD_REF(FUNCTION_PARAM) function,
               bslma::Allocator* allocator);

      public:
        // MANIPULATORS

        /// Implements `TargetBase::invoke`.
        void invoke(void* sharedState) BSLS_KEYWORD_OVERRIDE;
    };

  private:
    // PRIVATE TYPES

    /// Provides a "small" dummy object which size is used to calculate the
    /// size of the on-stack buffer used for optimization.
    struct Dummy : public bdlf::NoOp {
        void* d_padding[3];
    };

  private:
    // PRIVATE DATA

    // Uses an on-stack buffer to allocate memory for "small" objects, and
    // falls back to requesting memory from the supplied allocator if
    // the buffer is not large enough. Note that the size of the on-stack
    // buffer is an arbitrary value.
    bmqu::ObjectPlaceHolder<sizeof(Target<void, Dummy>)> d_target;

  private:
    // NOT IMPLEMENTED
    Future_Callback(const Future_Callback&) BSLS_KEYWORD_DELETED;
    Future_Callback& operator=(const Future_Callback&) BSLS_KEYWORD_DELETED;

  public:
    // CREATORS

    /// Create a `Future_Callback` object containing a function object of
    /// type `FUNCTION` direct-non-list-initialized by
    /// `bsl::forward<FUNCTION_PARAM>(function)`. Specify an `allocator`
    /// used to supply memory.
    ///
    /// `bsl::decay_t<FUNCTION>` must meet the requirements of Destructible
    /// and MoveConstructible as specified in the C++ standard. Given an
    /// object `f` of type `bsl::decay_t<FUNCTION>`,
    /// `f(bsl::declval<FutureResult<R>>())` shall be a valid
    /// expression.
    template <class R, class FUNCTION>
    Future_Callback(AsyncResultTypeTag<R>,
                    BSLS_COMPILERFEATURES_FORWARD_REF(FUNCTION) function,
                    bslma::Allocator* allocator);

    /// Destroy this object and the contained function object with it.
    ~Future_Callback();

  public:
    // MANIPULATORS

    /// Perform `bsl::move(f)(FutureResult<R>((S*)sharedState))`, where `f`
    /// is the contained function object, `R` is the type of the async
    /// result, and `S` is `Future<R>::SharedStateType`.
    void invoke(void* sharedState);

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(Future_Callback, bslma::UsesBslmaAllocator)
};

// ===================
// struct FutureStatus
// ===================

/// Specifies state of a future as returned by `waitFor` and `waitUntil`
/// functions of `bmqex::Future` and `bmqex::FutureSharedState`.
struct FutureStatus {
    enum Enum {
        e_READY  // the shared state is ready
        ,
        e_TIMEOUT  // the shared state did not become ready before timeout
    };
};

// ============
// class Future
// ============

/// Provides a mechanism to access the result of an asynchronous operation.
///
/// `R` must meet the requirements of Destructible as specified in the C++
/// standard.
template <class R>
class Future {
  public:
    // TYPES

    /// Defines the type of the shared state accepted by the future
    /// constructor.
    typedef FutureSharedState<R> SharedStateType;

  protected:
    // PROTECTED DATA
    bsl::shared_ptr<SharedStateType> d_sharedState;

    // FRIENDS
    template <class>
    friend class Future;

    template <class>
    friend class FutureResult;

  public:
    // CREATORS

    /// Create a `Future` object having no shared state.
    Future() BSLS_KEYWORD_NOEXCEPT;

    /// Create a `Future` object having the specified `sharedState`.
    explicit Future(const bsl::shared_ptr<SharedStateType>& sharedState)
        BSLS_KEYWORD_NOEXCEPT;

  public:
    // MANIPULATORS

    /// If `isReady()` is `true`, call 'DECAY_COPY(bsl::forward<FUNCTION>(
    /// callback))(FutureResult<R>(*this))'. Otherwise, store the specified
    /// `callback` as if by direct-non-list-initializing an object `f` of
    /// type `bsl::decay_t<FUNCTION>` with 'bsl::forward<FUNCTION>(
    /// callback)` to be invoked as `bsl::move(f)(FutureResult<R>(*this))'
    /// as soon as the shared state becomes ready. Effectively calls
    /// `whenReady` on the underlying shared state. The behavior is
    /// undefined if a callback is already attached to the shared state, or
    /// if the future has no shared state.
    ///
    /// Throws any exception thrown by the selected constructor of
    /// `bsl::decay_t<FUNCTION>`, any exception thrown by 'DECAY_COPY(
    /// bsl::forward<FUNCTION>(callback))(FutureResult<R>(*this))', or
    /// `bsl::bad_alloc` if memory allocation fails. If an exception is
    /// thrown, this function has no effect.
    ///
    /// Note that, unless otherwise specified, it is safe to invoke any
    /// function on `*this` from the context of the attached callback.
    ///
    /// `bsl::decay_t<FUNCTION>` must meet the requirements of Destructible
    /// and MoveConstructible as specified in the C++ standard. Given an
    /// object `f` of type `bsl::decay_t<FUNCTION>`,
    /// `f(bsl::declval<FutureResult<R>>())` shall be a valid expression.
    template <class FUNCTION>
    void whenReady(BSLS_COMPILERFEATURES_FORWARD_REF(FUNCTION) callback);

    /// Swap the contents of `*this` and `other`.
    void swap(Future& other) BSLS_KEYWORD_NOEXCEPT;

  public:
    // ACCESSORS

    /// Return `true` if the future has a shared state, and `false`
    /// otherwise.
    bool isValid() const BSLS_KEYWORD_NOEXCEPT;

    /// Return `true` if the shared state is ready, and `false` otherwise.
    /// Effectively calls `isReady` on the underlying shared state. The
    /// behavior is undefined unless the future has a shared state.
    bool isReady() const BSLS_KEYWORD_NOEXCEPT;

    /// Block the calling thread until the shared state is ready. Then,
    /// return the contained value or throw the contained exception.
    /// Effectively calls `get` on the underlying shared state. The
    /// behavior is undefined unless the future has a shared state.
    R&       get();
    const R& get() const;

    /// Block the calling thread until the shared state is ready.
    /// Effectively calls `wait` on the underlying shared state. The
    /// behavior is undefined unless the future has a shared state.
    void wait() const BSLS_KEYWORD_NOEXCEPT;

    /// Block the calling thread until the shared state becomes ready or the
    /// specified `duration` time has elapsed, whichever comes first. The
    /// `duration` is an offset from the current point in time, which is
    /// determined by the clock indicated at the construction of the
    /// underlying shared state. Return a value identifying the state of the
    /// result. Effectively calls `waitFor` on the underlying shared
    /// state. The behavior is undefined unless the future has a shared
    /// state.
    FutureStatus::Enum
    waitFor(const bsls::TimeInterval& duration) const BSLS_KEYWORD_NOEXCEPT;

    /// Block the calling thread until the shared state becomes ready or
    /// until the specified `timeout`, whichever comes first. The `timeout`
    /// is an absolute time represented as an interval from some epoch,
    /// which is determined by the clock indicated at the construction of
    /// the underlying shared state. Return a value identifying the state
    /// of the result. Effectively calls `waitUntil` on the underlying
    /// shared state. The behavior is undefined unless the future has a
    /// shared state.
    FutureStatus::Enum
    waitUntil(const bsls::TimeInterval& timeout) const BSLS_KEYWORD_NOEXCEPT;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(Future, bslmf::IsBitwiseMoveable)
};

// ==================
// class Future<void>
// ==================

/// Provides a specialization of `Future` for `void` result type.
template <>
class Future<void> : private Future<bslmf::Nil> {
  public:
    // TYPES

    /// Defines the type of the shared state accepted by the future
    /// constructor.
    typedef FutureSharedState<bslmf::Nil> SharedStateType;

  private:
    // PRIVATE TYPES
    typedef Future<bslmf::Nil> Impl;

    // FRIENDS
    template <class>
    friend class FutureResult;

  public:
    // CREATORS

    /// Same as for the non-specialized class template.
    Future() BSLS_KEYWORD_NOEXCEPT;

    /// Same as for the non-specialized class template.
    explicit Future(const bsl::shared_ptr<SharedStateType>& sharedState)
        BSLS_KEYWORD_NOEXCEPT;

  public:
    // MANIPULATORS

    /// Same as for the non-specialized class template.
    template <class FUNCTION>
    void whenReady(BSLS_COMPILERFEATURES_FORWARD_REF(FUNCTION) callback);

    /// Same as for the non-specialized class template.
    void swap(Future& other) BSLS_KEYWORD_NOEXCEPT;

  public:
    // ACCESSORS

    /// Same as for the non-specialized class template.
    bool isValid() const BSLS_KEYWORD_NOEXCEPT;

    /// Same as for the non-specialized class template.
    bool isReady() const BSLS_KEYWORD_NOEXCEPT;

    /// Block the calling thread until the shared state is ready. Then, if
    /// the shared state contains an exception, throw the contained
    /// exception. Effectively calls `get` on the underlying shared
    /// state. The behavior is undefined unless the future has a shared
    /// state.
    void get() const;

    /// Same as for the non-specialized class template.
    void wait() const BSLS_KEYWORD_NOEXCEPT;

    /// Same as for the non-specialized class template.
    FutureStatus::Enum
    waitFor(const bsls::TimeInterval& duration) const BSLS_KEYWORD_NOEXCEPT;

    /// Same as for the non-specialized class template.
    FutureStatus::Enum
    waitUntil(const bsls::TimeInterval& timeout) const BSLS_KEYWORD_NOEXCEPT;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(Future, bslmf::IsBitwiseMoveable)
};

// ================
// class Future<R&>
// ================

/// Provides a specialization of `Future` for reference result types.
template <class R>
class Future<R&> : private Future<bsl::reference_wrapper<R> > {
  public:
    // TYPES

    /// Defines the type of the shared state accepted by the future
    /// constructor.
    typedef FutureSharedState<bsl::reference_wrapper<R> > SharedStateType;

  private:
    // PRIVATE TYPES
    typedef Future<bsl::reference_wrapper<R> > Impl;

    // FRIENDS
    template <class>
    friend class FutureResult;

  public:
    // CREATORS

    /// Same as for the non-specialized class template.
    Future() BSLS_KEYWORD_NOEXCEPT;

    /// Same as for the non-specialized class template.
    explicit Future(const bsl::shared_ptr<SharedStateType>& sharedState)
        BSLS_KEYWORD_NOEXCEPT;

  public:
    // MANIPULATORS

    /// Same as for the non-specialized class template.
    template <class FUNCTION>
    void whenReady(BSLS_COMPILERFEATURES_FORWARD_REF(FUNCTION) callback);

    /// Same as for the non-specialized class template.
    void swap(Future& other) BSLS_KEYWORD_NOEXCEPT;

  public:
    // ACCESSORS

    /// Same as for the non-specialized class template.
    bool isValid() const BSLS_KEYWORD_NOEXCEPT;

    /// Same as for the non-specialized class template.
    bool isReady() const BSLS_KEYWORD_NOEXCEPT;

    /// Block the calling thread until the shared state is ready. Then,
    /// return the contained reference or throw the contained exception.
    /// Effectively calls `get` on the underlying shared state. The
    /// behavior is undefined unless the future has a shared state.
    R& get() const;

    /// Same as for the non-specialized class template.
    void wait() const BSLS_KEYWORD_NOEXCEPT;

    /// Same as for the non-specialized class template.
    FutureStatus::Enum
    waitFor(const bsls::TimeInterval& duration) const BSLS_KEYWORD_NOEXCEPT;

    /// Same as for the non-specialized class template.
    FutureStatus::Enum
    waitUntil(const bsls::TimeInterval& timeout) const BSLS_KEYWORD_NOEXCEPT;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(Future, bslmf::IsBitwiseMoveable)
};

// ==================
// class FutureResult
// ==================

/// Provides a mechanism to access the result of an asynchronous operation
/// that is already established. Objects of this type are passed as
/// parameters to notification callbacks.
///
/// `R` must meet the requirements of Destructible as specified in the C++
/// standard.
template <class R>
class FutureResult {
  private:
    // PRIVATE DATA
    typename Future<R>::SharedStateType* d_sharedState_p;

  public:
    /// Create a `FutureResult` object having the same shared state as the
    /// specified `future`. The behavior is undefined unless
    /// `future.isValid()` and `future.isReady()` are `true`.
    ///
    /// Note that no copy of the specified `future` is stored in this
    /// object, meaning that the reference counting for the shared state
    /// is not affected.
    explicit FutureResult(const Future<R>& future) BSLS_KEYWORD_NOEXCEPT;

    /// Create a `FutureResult` object having the specified `sharedState`.
    /// The behavior is undefined unless `sharedState->isReady()` is `true`.
    explicit FutureResult(typename Future<R>::SharedStateType* sharedState)
        BSLS_KEYWORD_NOEXCEPT;

    /// Swap the contents of `*this` and `other`.
    void swap(FutureResult& other) BSLS_KEYWORD_NOEXCEPT;

  public:
    // MANIPULATORS
  public:
    // ACCESSORS

    /// Perform `return get();`.
    operator R&();
    operator const R&() const;

    /// Return the contained value or throw the contained exception.
    /// Effectively calls `get` on the underlying shared state.
    R&       get();
    const R& get() const;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(FutureResult, bslmf::IsBitwiseMoveable)
};

// ========================
// class FutureResult<void>
// ========================

/// Provides a specialization of `FutureResult` for `void` result type.
template <>
class FutureResult<void> {
  private:
    // PRIVATE DATA
    Future<void>::SharedStateType* d_sharedState_p;

  public:
    // CREATORS

    /// Same as for the non-specialized class template.
    explicit FutureResult(const Future<void>& future) BSLS_KEYWORD_NOEXCEPT;

    /// Same as for the non-specialized class template.
    explicit FutureResult(Future<void>::SharedStateType* sharedState)
        BSLS_KEYWORD_NOEXCEPT;

  public:
    // MANIPULATORS

    /// Same as for the non-specialized class template.
    void swap(FutureResult& other) BSLS_KEYWORD_NOEXCEPT;

  public:
    // ACCESSORS

    /// If the shared state contains an exception, throw the contained
    /// exception. Effectively calls `get` on the underlying shared state.
    void get() const;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(FutureResult, bslmf::IsBitwiseMoveable)
};

// ======================
// class FutureResult<R&>
// ======================

/// Provides a specialization of `FutureResult` for reference result types.
template <class R>
class FutureResult<R&> {
  private:
    // PRIVATE DATA
    typename Future<R&>::SharedStateType* d_sharedState_p;

  public:
    // CREATORS

    /// Same as for the non-specialized class template.
    explicit FutureResult(const Future<R&>& future) BSLS_KEYWORD_NOEXCEPT;

    /// Same as for the non-specialized class template.
    explicit FutureResult(typename Future<R&>::SharedStateType* sharedState)
        BSLS_KEYWORD_NOEXCEPT;

  public:
    // MANIPULATORS

    /// Same as for the non-specialized class template.
    void swap(FutureResult& other) BSLS_KEYWORD_NOEXCEPT;

  public:
    // ACCESSORS

    /// Perform `return get();`.
    operator R&() const;

    /// Return the contained reference or throw the contained exception.
    /// Effectively calls `get` on the underlying shared state.
    R& get() const;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(FutureResult, bslmf::IsBitwiseMoveable)
};

// =======================
// class FutureSharedState
// =======================

/// Provides a shared state that stores the result of an asynchronous
/// operation.
///
/// `R` must meet the requirements of Destructible as specified in the C++
/// standard.
template <class R>
class FutureSharedState {
  private:
    // PRIVATE TYPES
    enum State {
        e_NOT_READY = 0  // shared state not ready
        ,
        e_READY_VALUE = 1  // shared state contains a value
#ifdef BSLS_LIBRARYFEATURES_HAS_CPP11_EXCEPTION_HANDLING
        ,
        e_READY_EXCEPTION_PTR =
            2  // shared state contains an exception pointer
#endif
        ,
        e_READY_EXCEPTION_OBJ = 3  // shared state contains an exception object
    };

    typedef typename bsl::remove_const<R>::type ValueType;
#ifdef BSLS_LIBRARYFEATURES_HAS_CPP11_EXCEPTION_HANDLING
    typedef bsl::exception_ptr ExceptionPtrType;
#endif
    typedef Future_Exception ExceptionObjType;

    /// Shared state result. May contain a value, an exception pointer
    /// (C++11 only), or an exception object.
    union Result {
        bsls::ObjectBuffer<ValueType> d_value;
#ifdef BSLS_LIBRARYFEATURES_HAS_CPP11_EXCEPTION_HANDLING
        bsls::ObjectBuffer<ExceptionPtrType> d_exceptionPtr;
#endif
        bsls::ObjectBuffer<ExceptionObjType> d_exceptionObj;
    };

  private:
    // PRIVATE DATA
    mutable bslmt::Mutex d_mutex;

    mutable bslmt::Condition d_condition;

    bsls::SystemClockType::Enum d_clockType;

    State d_state;

    Result d_result;

    bdlb::NullableValue<Future_Callback> d_callback;

    bslma::Allocator* d_allocator_p;

  private:
    // PRIVATE MANIPULATORS

    /// If a callback is attached to the shared state, invoke, and then
    /// destroy it.
    void invokeAndDestroyCallback();

  private:
    // NOT IMPLEMENTED
    FutureSharedState(const FutureSharedState&) BSLS_KEYWORD_DELETED;
    FutureSharedState&
    operator=(const FutureSharedState&) BSLS_KEYWORD_DELETED;

  public:
    // CREATORS
    explicit FutureSharedState(bslma::Allocator* basicAllocator = 0);

    /// Create a `FutureSharedState` object. Optionally specify a
    /// `clockType` indicating the type of the system clock against which
    /// the `bsls::TimeInterval` timeouts passed to `waitFor` and
    /// `waitUntil` methods are to be interpreted. If `clockType` is not
    /// specified, the monotonic system clock is used. Optionally specify a
    /// `basicAllocator` used to supply memory. If `basicAllocator` is not
    /// specified, the currently installed default allocator is used.
    explicit FutureSharedState(bsls::SystemClockType::Enum clockType,
                               bslma::Allocator*           basicAllocator = 0);

    /// Destroy this object. Destroy any contained value, exception pointer,
    /// exception object or callback. The behavior is undefined if this
    /// function is invoked from the context of a callback attached to this
    /// shared state.
    ~FutureSharedState();

  public:
    // MANIPULATORS

    /// Atomically initialize the stored value as if by direct-non-list-
    /// initializing an object of type `R` with `value` and make the state
    /// ready. If a callback is attached to the shared state, invoke, and
    /// then destroy it. The behavior is undefined if the shared state is
    /// ready.
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

    /// Atomically initialize the stored value as if by direct-non-list-
    /// initializing an object of type `R` with bsl::move(value)' and make
    /// the state ready. If a callback is attached to the shared state,
    /// invoke, and then destroy it. The behavior is undefined if the shared
    /// state is ready.
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

#if !BSLS_COMPILERFEATURES_SIMULATE_CPP11_FEATURES // $var-args=9

    /// Atomically initialize the stored value as if by direct-non-list-
    /// initializing an object of type `R` with 'bsl::forward<ARGS>(
    /// args)...' and make the state ready. If a callback is attached to
    /// the shared state, invoke, and then destroy it. The behavior is
    /// undefined if the shared state is ready.
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
    /// shared state, invoke, and then destroy it. The behavior is undefined
    /// if the shared state is ready.
    ///
    /// Throws any exception thrown by the attached callback. If an
    /// exception is thrown, the stored exception stays initialized and the
    /// callback is destroyed.
    void setException(bsl::exception_ptr exception);
#endif

    /// Atomically initialize the stored exception object as if by direct-
    /// non-list-initializing an object of type `bsl::decay_t<EXCEPTION>`
    /// with `bsl::forward<EXCEPTION>(EXCEPTION)` and make the state ready.
    /// If a callback is attached to the shared state, invoke, and then
    /// destroy it. The behavior is undefined if the shared state is ready.
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

    /// If `isReady()` is `true`, call 'DECAY_COPY(bsl::forward<FUNCTION>(
    /// callback))(FutureResult<R_T>(this))'. Otherwise, store the specified
    /// `callback` as if by direct-non-list-initializing an object `f` of
    /// type `bsl::decay_t<FUNCTION>` with 'bsl::forward<FUNCTION>(
    /// callback)` to be invoked as `bsl::move(f)(FutureResult<R_T>(this))'
    /// as soon as the shared state becomes ready. The behavior is undefined
    /// if a callback is already attached to this shared state.
    ///
    /// Throws any exception thrown by the selected constructor of
    /// `bsl::decay_t<FUNCTION>`, any exception thrown by 'DECAY_COPY(
    /// bsl::forward<FUNCTION>(callback))(FutureResult<R_T>(this))', or
    /// `bsl::bad_alloc` if memory allocation fails. If an exception is
    /// thrown, this function has no effect.
    ///
    /// Note that, unless otherwise specified, it is safe to invoke any
    /// function on `*this` from the context of the attached callback.
    ///
    /// `bsl::decay_t<FUNCTION>` must meet the requirements of Destructible
    /// and MoveConstructible as specified in the C++ standard. Given an
    /// object `f` of type `bsl::decay_t<FUNCTION>`,
    /// `f(bsl::declval<FutureResult<R_T>>())` shall be a valid expression.
    template <class R_T, class FUNCTION>
    void whenReady(BSLS_COMPILERFEATURES_FORWARD_REF(FUNCTION) callback);

  public:
    // ACCESSORS

    /// Return `true` if the shared state is ready, and `false` otherwise.
    bool isReady() const BSLS_KEYWORD_NOEXCEPT;

    /// Block the calling thread until the shared state is ready. Then, if
    /// the state contains a value, return a reference to the contained
    /// value. Otherwise, if the state contains an exception, throw the
    /// contained exception.
    R&       get();
    const R& get() const;

    /// Block the calling thread until the shared state is ready.
    void wait() const BSLS_KEYWORD_NOEXCEPT;

    /// Block the calling thread until the shared state becomes ready or the
    /// specified `duration` time has elapsed, whichever comes first. The
    /// `duration` is an offset from the current point in time, which is
    /// determined by the clock indicated at construction. Return a value
    /// identifying the state of the result.
    FutureStatus::Enum
    waitFor(const bsls::TimeInterval& duration) const BSLS_KEYWORD_NOEXCEPT;

    /// Block the calling thread until the shared state becomes ready or
    /// until the specified `timeout`, whichever comes first. The `timeout`
    /// is an absolute time represented as an interval from some epoch,
    /// which is determined by the clock indicated at construction. Return a
    /// value identifying the state of the result.
    FutureStatus::Enum
    waitUntil(const bsls::TimeInterval& timeout) const BSLS_KEYWORD_NOEXCEPT;

    /// Return the system clock type used by this object for timed
    /// operations.
    bsls::SystemClockType::Enum clockType() const BSLS_KEYWORD_NOEXCEPT;

    /// Return the allocator used by this object to supply memory.
    bslma::Allocator* allocator() const BSLS_KEYWORD_NOEXCEPT;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(FutureSharedState,
                                   bslma::UsesBslmaAllocator)
};

// FREE OPERATORS

/// Swap the contents of `lhs` and `rhs`.
template <class R>
void swap(Future<R>& lhs, Future<R>& rhs) BSLS_KEYWORD_NOEXCEPT;

/// Swap the contents of `lhs` and `rhs`.
template <class R>
void swap(FutureResult<R>& lhs, FutureResult<R>& rhs) BSLS_KEYWORD_NOEXCEPT;

// ============================================================================
//                           INLINE DEFINITIONS
// ============================================================================

// ----------------------
// class Future_Exception
// ----------------------

// CREATORS
template <class EXCEPTION>
inline Future_Exception::Future_Exception(
    BSLS_COMPILERFEATURES_FORWARD_REF(EXCEPTION) exception,
    bslma::Allocator* allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT(allocator);

    typedef Target<typename bsl::decay<EXCEPTION>::type> T;
    d_target.createObject<T>(allocator,
                             BSLS_COMPILERFEATURES_FORWARD(EXCEPTION,
                                                           exception),
                             allocator);
}

// ------------------------------
// class Future_Exception::Target
// ------------------------------

// CREATORS
template <class EXCEPTION>
template <class EXCEPTION_PARAM>
inline Future_Exception::Target<EXCEPTION>::Target(
    BSLS_COMPILERFEATURES_FORWARD_REF(EXCEPTION_PARAM) exception,
    bslma::Allocator* allocator)
: d_exception(BSLS_COMPILERFEATURES_FORWARD(EXCEPTION_PARAM, exception),
              allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT(allocator);
}

// ACCESSORS
template <class EXCEPTION>
BSLS_ANNOTATION_NORETURN inline void
Future_Exception::Target<EXCEPTION>::emit() const
{
    throw d_exception.object();
}

// ---------------------
// class Future_Callback
// ---------------------

// CREATORS
template <class R, class FUNCTION>
inline Future_Callback::Future_Callback(
    AsyncResultTypeTag<R>,
    BSLS_COMPILERFEATURES_FORWARD_REF(FUNCTION) function,
    bslma::Allocator* allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT(allocator);

    typedef Target<R, typename bsl::decay<FUNCTION>::type> T;
    d_target.createObject<T>(allocator,
                             BSLS_COMPILERFEATURES_FORWARD(FUNCTION, function),
                             allocator);
}

// -----------------------------
// class Future_Callback::Target
// -----------------------------

// CREATORS
template <class R, class FUNCTION>
template <class FUNCTION_PARAM>
inline Future_Callback::Target<R, FUNCTION>::Target(
    BSLS_COMPILERFEATURES_FORWARD_REF(FUNCTION_PARAM) function,
    bslma::Allocator* allocator)
: d_function(BSLS_COMPILERFEATURES_FORWARD(FUNCTION_PARAM, function),
             allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT(allocator);
}

// ACCESSORS
template <class R, class FUNCTION>
inline void Future_Callback::Target<R, FUNCTION>::invoke(void* sharedState)
{
    typedef typename Future<R>::SharedStateType SharedState;

    FutureResult<R> result(static_cast<SharedState*>(sharedState));
    bslmf::Util::moveIfSupported(d_function.object())(result);
}

// ------------
// class Future
// ------------

// CREATORS
template <class R>
inline Future<R>::Future() BSLS_KEYWORD_NOEXCEPT : d_sharedState()
{
    // NOTHING
}

template <class R>
inline Future<R>::Future(const bsl::shared_ptr<SharedStateType>& sharedState)
    BSLS_KEYWORD_NOEXCEPT : d_sharedState(sharedState)
{
    // PRECONDITIONS
    BSLS_ASSERT(sharedState);
}

// MANIPULATORS
template <class R>
template <class FUNCTION>
inline void Future<R>::whenReady(BSLS_COMPILERFEATURES_FORWARD_REF(FUNCTION)
                                     callback)
{
    // PRECONDITIONS
    BSLS_ASSERT(d_sharedState);

    d_sharedState->template whenReady<R>(
        BSLS_COMPILERFEATURES_FORWARD(FUNCTION, callback));
}

template <class R>
inline void Future<R>::swap(Future& other) BSLS_KEYWORD_NOEXCEPT
{
    d_sharedState.swap(other.d_sharedState);
}

// ACCESSORS
template <class R>
inline bool Future<R>::isValid() const BSLS_KEYWORD_NOEXCEPT
{
    return static_cast<bool>(d_sharedState);
}

template <class R>
inline bool Future<R>::isReady() const BSLS_KEYWORD_NOEXCEPT
{
    // PRECONDITIONS
    BSLS_ASSERT(d_sharedState);

    return d_sharedState->isReady();
}

template <class R>
inline R& Future<R>::get()
{
    // PRECONDITIONS
    BSLS_ASSERT(d_sharedState);

    return d_sharedState->get();
}

template <class R>
inline const R& Future<R>::get() const
{
    // PRECONDITIONS
    BSLS_ASSERT(d_sharedState);

    return d_sharedState->get();
}

template <class R>
inline void Future<R>::wait() const BSLS_KEYWORD_NOEXCEPT
{
    // PRECONDITIONS
    BSLS_ASSERT(d_sharedState);

    d_sharedState->wait();
}

template <class R>
inline FutureStatus::Enum Future<R>::waitFor(
    const bsls::TimeInterval& duration) const BSLS_KEYWORD_NOEXCEPT
{
    // PRECONDITIONS
    BSLS_ASSERT(d_sharedState);

    return d_sharedState->waitFor(duration);
}

template <class R>
inline FutureStatus::Enum Future<R>::waitUntil(
    const bsls::TimeInterval& timeout) const BSLS_KEYWORD_NOEXCEPT
{
    // PRECONDITIONS
    BSLS_ASSERT(d_sharedState);

    return d_sharedState->waitUntil(timeout);
}

// ------------------
// class Future<void>
// ------------------

// CREATORS
inline Future<void>::Future() BSLS_KEYWORD_NOEXCEPT : Impl()
{
    // NOTHING
}

inline Future<void>::Future(
    const bsl::shared_ptr<SharedStateType>& sharedState) BSLS_KEYWORD_NOEXCEPT
: Impl(sharedState)
{
    // NOTHING
}

// MANIPULATORS
template <class FUNCTION>
inline void Future<void>::whenReady(BSLS_COMPILERFEATURES_FORWARD_REF(FUNCTION)
                                        callback)
{
    // PRECONDITIONS
    BSLS_ASSERT(this->d_sharedState);

    this->d_sharedState->template whenReady<void>(
        BSLS_COMPILERFEATURES_FORWARD(FUNCTION, callback));
}

inline void Future<void>::swap(Future& other) BSLS_KEYWORD_NOEXCEPT
{
    Impl::swap(other);
}

// ACCESSORS
inline bool Future<void>::isValid() const BSLS_KEYWORD_NOEXCEPT
{
    return Impl::isValid();
}

inline bool Future<void>::isReady() const BSLS_KEYWORD_NOEXCEPT
{
    return Impl::isReady();
}

// ACCESSORS
inline void Future<void>::get() const
{
    Impl::get();
}

inline void Future<void>::wait() const BSLS_KEYWORD_NOEXCEPT
{
    Impl::wait();
}

inline FutureStatus::Enum Future<void>::waitFor(
    const bsls::TimeInterval& duration) const BSLS_KEYWORD_NOEXCEPT
{
    return Impl::waitUntil(duration);
}

inline FutureStatus::Enum Future<void>::waitUntil(
    const bsls::TimeInterval& timeout) const BSLS_KEYWORD_NOEXCEPT
{
    return Impl::waitUntil(timeout);
}

// ----------------
// class Future<R&>
// ----------------

// CREATORS
template <class R>
inline Future<R&>::Future() BSLS_KEYWORD_NOEXCEPT : Impl()
{
    // NOTHING
}

template <class R>
inline Future<R&>::Future(const bsl::shared_ptr<SharedStateType>& sharedState)
    BSLS_KEYWORD_NOEXCEPT : Impl(sharedState)
{
    // NOTHING
}

// MANIPULATORS
template <class R>
template <class FUNCTION>
inline void Future<R&>::whenReady(BSLS_COMPILERFEATURES_FORWARD_REF(FUNCTION)
                                      callback)
{
    // PRECONDITIONS
    BSLS_ASSERT(this->d_sharedState);

    this->d_sharedState->template whenReady<R&>(
        BSLS_COMPILERFEATURES_FORWARD(FUNCTION, callback));
}

template <class R>
inline void Future<R&>::swap(Future& other) BSLS_KEYWORD_NOEXCEPT
{
    Impl::swap(other);
}

// ACCESSORS
template <class R>
inline bool Future<R&>::isValid() const BSLS_KEYWORD_NOEXCEPT
{
    return Impl::isValid();
}

template <class R>
inline bool Future<R&>::isReady() const BSLS_KEYWORD_NOEXCEPT
{
    return Impl::isReady();
}

// ACCESSORS
template <class R>
inline R& Future<R&>::get() const
{
    return Impl::get();
}

template <class R>
inline void Future<R&>::wait() const BSLS_KEYWORD_NOEXCEPT
{
    Impl::wait();
}

template <class R>
inline FutureStatus::Enum Future<R&>::waitFor(
    const bsls::TimeInterval& duration) const BSLS_KEYWORD_NOEXCEPT
{
    return Impl::waitUntil(duration);
}

template <class R>
inline FutureStatus::Enum Future<R&>::waitUntil(
    const bsls::TimeInterval& timeout) const BSLS_KEYWORD_NOEXCEPT
{
    return Impl::waitUntil(timeout);
}

// ------------------
// class FutureResult
// ------------------

// CREATORS
template <class R>
inline FutureResult<R>::FutureResult(const Future<R>& future)
    BSLS_KEYWORD_NOEXCEPT : d_sharedState_p(future.d_sharedState.get())
{
    // PRECONDITIONS
    BSLS_ASSERT(future.isValid());
    BSLS_ASSERT(future.isReady());
}

template <class R>
inline FutureResult<R>::FutureResult(
    typename Future<R>::SharedStateType* sharedState) BSLS_KEYWORD_NOEXCEPT
: d_sharedState_p(sharedState)
{
    // PRECONDITIONS
    BSLS_ASSERT(sharedState);
    BSLS_ASSERT(sharedState->isReady());
}

// MANIPULATORS
template <class R>
inline void FutureResult<R>::swap(FutureResult& other) BSLS_KEYWORD_NOEXCEPT
{
    using bsl::swap;
    swap(d_sharedState_p, other.d_sharedState_p);
}

// ACCESSORS
template <class R>
inline FutureResult<R>::operator R&()
{
    return get();
}

template <class R>
inline FutureResult<R>::operator const R&() const
{
    return get();
}

template <class R>
inline R& FutureResult<R>::get()
{
    return d_sharedState_p->get();
}

template <class R>
inline const R& FutureResult<R>::get() const
{
    return d_sharedState_p->get();
}

// ------------------------
// class FutureResult<void>
// ------------------------

// CREATORS
inline FutureResult<void>::FutureResult(const Future<void>& future)
    BSLS_KEYWORD_NOEXCEPT : d_sharedState_p(future.d_sharedState.get())
{
    // PRECONDITIONS
    BSLS_ASSERT(future.isValid());
    BSLS_ASSERT(future.isReady());
}

inline FutureResult<void>::FutureResult(
    Future<void>::SharedStateType* sharedState) BSLS_KEYWORD_NOEXCEPT
: d_sharedState_p(sharedState)
{
    // PRECONDITIONS
    BSLS_ASSERT(sharedState);
    BSLS_ASSERT(sharedState->isReady());
}

// MANIPULATORS
inline void FutureResult<void>::swap(FutureResult& other) BSLS_KEYWORD_NOEXCEPT
{
    using bsl::swap;
    swap(d_sharedState_p, other.d_sharedState_p);
}

// ACCESSORS
inline void FutureResult<void>::get() const
{
    d_sharedState_p->get();
}

// ----------------------
// class FutureResult<R&>
// ----------------------

// CREATORS
template <class R>
inline FutureResult<R&>::FutureResult(const Future<R&>& future)
    BSLS_KEYWORD_NOEXCEPT : d_sharedState_p(future.d_sharedState.get())
{
    // PRECONDITIONS
    BSLS_ASSERT(future.isValid());
    BSLS_ASSERT(future.isReady());
}

template <class R>
inline FutureResult<R&>::FutureResult(
    typename Future<R&>::SharedStateType* sharedState) BSLS_KEYWORD_NOEXCEPT
: d_sharedState_p(sharedState)
{
    // PRECONDITIONS
    BSLS_ASSERT(sharedState);
    BSLS_ASSERT(sharedState->isReady());
}

// MANIPULATORS
template <class R>
inline void FutureResult<R&>::swap(FutureResult& other) BSLS_KEYWORD_NOEXCEPT
{
    using bsl::swap;
    swap(d_sharedState_p, other.d_sharedState_p);
}

// ACCESSORS
template <class R>
inline FutureResult<R&>::operator R&() const
{
    return get();
}

template <class R>
inline R& FutureResult<R&>::get() const
{
    return d_sharedState_p->get();
}

// -----------------------
// class FutureSharedState
// -----------------------

// PRIVATE MANIPULATORS
template <class R>
inline void FutureSharedState<R>::invokeAndDestroyCallback()
{
    if (d_callback.isNull()) {
        return;  // RETURN
    }

    try {
        d_callback.value().invoke(this);
        d_callback.reset();
    }
    catch (...) {
        d_callback.reset();
        throw;  // THROW
    }
}

// CREATORS
template <class R>
inline FutureSharedState<R>::FutureSharedState(
    bslma::Allocator* basicAllocator)
: d_mutex()
, d_condition(bsls::SystemClockType::e_MONOTONIC)
, d_clockType(bsls::SystemClockType::e_MONOTONIC)
, d_state(e_NOT_READY)
, d_result()
, d_callback(basicAllocator)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    // NOTHING
}

template <class R>
inline FutureSharedState<R>::FutureSharedState(
    bsls::SystemClockType::Enum clockType,
    bslma::Allocator*           basicAllocator)
: d_mutex()
, d_condition(clockType)
, d_clockType(clockType)
, d_state(e_NOT_READY)
, d_result()
, d_callback(basicAllocator)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    // NOTHING
}

template <class R>
inline FutureSharedState<R>::~FutureSharedState()
{
    switch (d_state) {
    case e_NOT_READY: {
        // The shared state doesn't contain a result.

        break;  // BREAK
    }

    case e_READY_VALUE: {
        // The shared state contains a value.

        typedef bsls::ObjectBuffer<ValueType> Buffer;
        reinterpret_cast<Buffer*>(&d_result)->object().~ValueType();
        break;  // BREAK
    }

#ifdef BSLS_LIBRARYFEATURES_HAS_CPP11_EXCEPTION_HANDLING
    case e_READY_EXCEPTION_PTR: {
        // The shared state contains an exception pointer.

        typedef bsls::ObjectBuffer<ExceptionPtrType> Buffer;
        reinterpret_cast<Buffer*>(&d_result)->object().~ExceptionPtrType();
        break;  // BREAK
    }
#endif
    case e_READY_EXCEPTION_OBJ: {
        // The shared state contains an exception object.

        typedef bsls::ObjectBuffer<ExceptionObjType> Buffer;
        reinterpret_cast<Buffer*>(&d_result)->object().~ExceptionObjType();
        break;  // BREAK
    }
    }
}

// MANIPULATORS
template <class R>
inline void FutureSharedState<R>::setValue(const R& value)
{
    emplaceValue(value);
}

template <class R>
inline void FutureSharedState<R>::setValue(bslmf::MovableRef<R> value)
{
    emplaceValue(bslmf::MovableRefUtil::move(value));
}

#if !BSLS_COMPILERFEATURES_SIMULATE_CPP11_FEATURES // $var-args=9
template <class R>
template <class... ARGS>
inline void FutureSharedState<R>::emplaceValue(ARGS&&... args)
{
    typedef bsls::ObjectBuffer<ValueType> Buffer;

    {
        bslmt::LockGuard<bslmt::Mutex> lock(&d_mutex);  // LOCK

        // PRECONDITIONS
        BSLS_ASSERT(d_state == e_NOT_READY);

        bslma::ConstructionUtil::construct(
            reinterpret_cast<Buffer*>(&d_result)->address(),
            allocator(),
            bslmf::Util::forward<ARGS>(args)...);

        d_state = e_READY_VALUE;
    }  // UNLOCK

    d_condition.broadcast();
    invokeAndDestroyCallback();
}
#endif

#ifdef BSLS_LIBRARYFEATURES_HAS_CPP11_EXCEPTION_HANDLING
template <class R>
inline void FutureSharedState<R>::setException(bsl::exception_ptr exception)
{
    typedef bsls::ObjectBuffer<ExceptionPtrType> Buffer;

    // PRECONDITIONS
    BSLS_ASSERT(exception);

    {
        bslmt::LockGuard<bslmt::Mutex> lock(&d_mutex);  // LOCK

        // PRECONDITIONS
        BSLS_ASSERT(d_state == e_NOT_READY);

        bslma::ConstructionUtil::construct(
            reinterpret_cast<Buffer*>(&d_result)->address(),
            allocator(),  // not used
            exception);

        d_state = e_READY_EXCEPTION_PTR;
    }  // UNLOCK

    d_condition.broadcast();
    invokeAndDestroyCallback();
}
#endif

template <class R>
template <class EXCEPTION>
inline void
FutureSharedState<R>::setException(BSLS_COMPILERFEATURES_FORWARD_REF(EXCEPTION)
                                       exception)
{
    typedef bsls::ObjectBuffer<ExceptionObjType> Buffer;

    {
        bslmt::LockGuard<bslmt::Mutex> lock(&d_mutex);  // LOCK

        // PRECONDITIONS
        BSLS_ASSERT(d_state == e_NOT_READY);

        bslma::ConstructionUtil::construct(
            reinterpret_cast<Buffer*>(&d_result)->address(),
            allocator(),
            BSLS_COMPILERFEATURES_FORWARD(EXCEPTION, exception));

        d_state = e_READY_EXCEPTION_OBJ;
    }  // UNLOCK

    d_condition.broadcast();
    invokeAndDestroyCallback();
}

template <class R>
template <class R_T, class FUNCTION>
void FutureSharedState<R>::whenReady(
    BSLS_COMPILERFEATURES_FORWARD_REF(FUNCTION) callback)
{
    bslmt::LockGuard<bslmt::Mutex> lock(&d_mutex);  // LOCK

    // PRECONDITIONS
    BSLS_ASSERT(d_callback.isNull());

    if (d_state == e_NOT_READY) {
        // The shared state is not ready. Save the callback to be invoked
        // later.

        d_callback.makeValueInplace(Future_Callback::AsyncResultTypeTag<R_T>(),
                                    BSLS_COMPILERFEATURES_FORWARD(FUNCTION,
                                                                  callback));
    }
    else {
        // The shared state is ready. Invoke the callback in-place.

        lock.release()->unlock();  // UNLOCK

        // make a local, non-const copy of the function
        bslalg::ConstructorProxy<typename bsl::decay<FUNCTION>::type> f(
            BSLS_COMPILERFEATURES_FORWARD(FUNCTION, callback),
            d_allocator_p);

        // invoke in-place
        bslmf::Util::moveIfSupported(f.object())(FutureResult<R_T>(this));
    }
}

// ACCESSORS
template <class R>
inline bool FutureSharedState<R>::isReady() const BSLS_KEYWORD_NOEXCEPT
{
    bslmt::LockGuard<bslmt::Mutex> lock(&d_mutex);  // LOCK

    return d_state != e_NOT_READY;
}

template <class R>
inline R& FutureSharedState<R>::get()
{
    wait();  // wait until the shared state is ready

    switch (d_state) {
    case e_NOT_READY: {
        // Unreachable code, but makes the compiler happy.

        BSLS_ASSERT(false);
        BSLS_ASSERT_INVOKE_NORETURN("");
    }

    case e_READY_VALUE: {
        // The shared state contains a value.

        typedef bsls::ObjectBuffer<ValueType> Buffer;
        return reinterpret_cast<Buffer*>(&d_result)->object();  // RETURN
    }

#ifdef BSLS_LIBRARYFEATURES_HAS_CPP11_EXCEPTION_HANDLING
    case e_READY_EXCEPTION_PTR: {
        // The shared state contains an exception pointer.

        typedef bsls::ObjectBuffer<ExceptionPtrType> Buffer;
        bsl::rethrow_exception(
            reinterpret_cast<Buffer*>(&d_result)->object());  // THROW
    }
#endif
    case e_READY_EXCEPTION_OBJ: {
        // The shared state contains an exception object.

        typedef bsls::ObjectBuffer<ExceptionObjType> Buffer;
        reinterpret_cast<Buffer*>(&d_result)->object().emit();  // THROW
    }
    }

    // Unreachable code, but makes the compiler happy.
    BSLS_ASSERT(false);
    return *reinterpret_cast<R*>(0x42);
}

template <class R>
inline const R& FutureSharedState<R>::get() const
{
    return const_cast<FutureSharedState&>(*this).get();
}

template <class R>
inline void FutureSharedState<R>::wait() const BSLS_KEYWORD_NOEXCEPT
{
    bslmt::LockGuard<bslmt::Mutex> lock(&d_mutex);  // LOCK

    while (d_state == e_NOT_READY) {
        // wait until the state is ready
        int rc = d_condition.wait(&d_mutex);  // UNLOCK / LOCK
        BSLS_ASSERT_OPT(rc == 0);
    }
}

template <class R>
inline FutureStatus::Enum FutureSharedState<R>::waitFor(
    const bsls::TimeInterval& duration) const BSLS_KEYWORD_NOEXCEPT
{
    return waitUntil(bsls::SystemTime::now(clockType()) + duration);
}

template <class R>
inline FutureStatus::Enum FutureSharedState<R>::waitUntil(
    const bsls::TimeInterval& timeout) const BSLS_KEYWORD_NOEXCEPT
{
    bslmt::LockGuard<bslmt::Mutex> lock(&d_mutex);  // LOCK

    int rc = 0;
    while (d_state == e_NOT_READY && rc == 0) {
        // wait until the state is ready or until timeout
        rc = d_condition.timedWait(&d_mutex, timeout);  // UNLOCK / LOCK
    }

    BSLS_ASSERT_OPT(rc == 0 || rc == -1);
    return rc == 0 ? FutureStatus::e_READY : FutureStatus::e_TIMEOUT;
}

template <class R>
inline bsls::SystemClockType::Enum
FutureSharedState<R>::clockType() const BSLS_KEYWORD_NOEXCEPT
{
    return d_clockType;
}

template <class R>
inline bslma::Allocator*
FutureSharedState<R>::allocator() const BSLS_KEYWORD_NOEXCEPT
{
    return d_allocator_p;
}

}  // close package namespace

// FREE OPERATORS
template <class R>
inline void bmqex::swap(Future<R>& lhs, Future<R>& rhs) BSLS_KEYWORD_NOEXCEPT
{
    lhs.swap(rhs);
}

template <class R>
inline void bmqex::swap(FutureResult<R>& lhs,
                        FutureResult<R>& rhs) BSLS_KEYWORD_NOEXCEPT
{
    lhs.swap(rhs);
}

}  // close enterprise namespace

#endif // End C++11 code

#endif
