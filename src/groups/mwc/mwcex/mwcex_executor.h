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

// mwcex_executor.h                                                   -*-C++-*-
#ifndef INCLUDED_MWCEX_EXECUTOR
#define INCLUDED_MWCEX_EXECUTOR

//@PURPOSE: Provides a polymorphic wrapper for executor types.
//
//@CLASSES:
//  mwcex::Executor: a polymorphic wrapper for executor types
//
//@DESCRIPTION:
// This component provides a polymorphic wrapper for types that satisfy the
// Executor requirements (see package documentation).
//
// Objects of type 'mwcex::Executor' are generally used to avoid templatizing
// a function or class, like shown in the example below. In short,
// 'mwcex::Executor' is to an executor object what 'bsl::function' is to a
// function object.
//
// Note that to meet the 'noexcept' requirements for executor copy and move
// constructors, the implementation may share a target between two or more
// executor objects.
//
/// Usage
///-----
// Here is simple example demonstrating the intended usage of this component.
// Notice how the 'Notificator's constructor takes an polymorphic executor,
// but is supplied an executor of type 'mwcex::SystemExecutor'.
//..
//  class Notificator {
//      // Provides a class that asynchronously invokes a user-supplied
//      // callback when "something" happens. The invocation is done in
//      // the context of a user-supplied executor.
//
//    public:
//      // TYPES
//      typedef bsl::function<void()> Callback;
//          // Notification callback.
//
//    private:
//      // PRIVATE DATA
//      mwcex::Executor d_executor;
//
//      Callback        d_callback;
//
//    private:
//      // NOT IMPLEMENTED
//      Notificator(const Notificator&)            = delete;
//      Notificator& operator=(const Notificator&) = delete;
//
//    public:
//      // CREATORS
//      Notificator(const mwcex::Executor& executor,
//                  const Callback&        callback);
//          // Create a 'Notificator' object that asynchronously invokes the
//          // specified 'callback' in the context of the specified 'executor'
//          // when an "event" occur.
//
//    public:
//      // MANIPULATORS
//      void somethingHappend();
//          // Simulate an "event".
//  };
//
//  Notificator::Notificator(const mwcex::Executor& executor,
//                           const Callback&        callback)
//  : d_executor(executor)
//  , d_callback(callback)
//  {
//      BSLS_ASSERT(executor);
//      BSLS_ASSERT(callback);
//  }
//
//  void Notificator::somethingHappend()
//  {
//      d_executor.post(d_callback);
//  }
//..
//
//..
//  // create a notificator with the system executor
//  Notificator notificator(mwces::SystemExecutor(),
//                          [](){
//                                   bsl::cout << "Got it!" << bsl::endl;
//                          });
//
//  // simulate an "event"
//  notificator.somethingHappend();
//
//  // our handler is invoked somewhere in another thread ...
//..

// MWC
#include <mwcex_executortraits.h>

// BDE
#include <bdlb_variant.h>
#include <bsl_functional.h>
#include <bsl_memory.h>
#include <bsl_typeinfo.h>
#include <bslma_allocator.h>
#include <bslma_constructionutil.h>
#include <bslma_default.h>
#include <bslmf_assert.h>
#include <bslmf_conditional.h>
#include <bslmf_movableref.h>
#include <bslmf_util.h>
#include <bsls_alignedbuffer.h>
#include <bsls_assert.h>
#include <bsls_compilerfeatures.h>
#include <bsls_keyword.h>

namespace BloombergLP {

namespace mwcex {

// =========================
// class Executor_TargetBase
// =========================

/// Provides an interface used to implement the type erasure technique.
/// When creating a polymorphic executor with a target of type `EX`, an
/// instance of derived class template `Executor_Target<EX>` is
/// instantiated and stored via a pointer to its base class (this one).
/// Then, calls to `mwcex::Executor`s public methods are forwarded to this
/// class.
class Executor_TargetBase {
  public:
    // CREATORS

    /// Destroy this object and the contained executor target with it.
    virtual ~Executor_TargetBase();

  public:
    // MANIPULATORS

    /// Perform `ExecutorTraits<E>::post(e, f)`, where `e` is the
    /// contained executor target of type `E`.
    virtual void post(const bsl::function<void()>& f) const = 0;

    /// Perform `ExecutorTraits<E>::dispatch(e, f)`, where `e` is the
    /// contained executor target of type `E`.
    virtual void dispatch(const bsl::function<void()>& f) const = 0;

    /// Move-construct `*this` into the specified `dst` address.
    virtual void move(void* dst) BSLS_KEYWORD_NOEXCEPT = 0;

  public:
    // ACCESSORS

    /// Return `e1 == e2`, where `e1` and `e2` are contained executor
    /// targets (of type `E`) of `*this` and `other` respectively. The
    /// behavior is undefined unless `typeid(e1) == typeid(e2)` is `true`.
    virtual bool
    equal(const Executor_TargetBase& other) const BSLS_KEYWORD_NOEXCEPT = 0;

    /// Return a pointer to the contained executor target.
    virtual void*       target() BSLS_KEYWORD_NOEXCEPT       = 0;
    virtual const void* target() const BSLS_KEYWORD_NOEXCEPT = 0;

    /// Return `typeid(e)`, where `e` is the contained executor target.
    virtual const bsl::type_info& targetType() const BSLS_KEYWORD_NOEXCEPT = 0;

    /// Copy-construct `*this` into the specified `dst` address.
    virtual void copy(void* dst) const BSLS_KEYWORD_NOEXCEPT = 0;
};

// =====================
// class Executor_Target
// =====================

/// Provides an implementation of the `Executor_TargetBase` interface
/// containing the executor target.
///
/// `EXECUTOR` must meet the requirements of Executor (see package
/// documentation).
template <class EXECUTOR>
class Executor_Target : public Executor_TargetBase {
  private:
    // PRIVATE DATA
    EXECUTOR d_executor;

  private:
    // NOT IMPLEMENTED
    Executor_Target(const Executor_Target&) BSLS_KEYWORD_DELETED;
    Executor_Target& operator=(const Executor_Target&) BSLS_KEYWORD_DELETED;

  public:
    // CREATORS

    /// Create a `Executor_Target` object containing an executor target of
    /// type `EXECUTOR` direct-non-list-initialized by
    /// `bsl::forward<EXECUTOR_PARAM>(executor)`.
    ///
    /// `bsl::decay_t<EXECUTOR_PARM>` shall be the same type as `EXECUTOR`.
    template <class EXECUTOR_PARAM>
    Executor_Target(BSLS_COMPILERFEATURES_FORWARD_REF(EXECUTOR_PARAM)
                        executor) BSLS_KEYWORD_NOEXCEPT;

  public:
    // MANIPULATORS

    /// Implements `Executor_TargetBase::post()`.
    void post(const bsl::function<void()>&) const BSLS_KEYWORD_OVERRIDE;

    /// Implements `Executor_TargetBase::dispatch()`.
    void dispatch(const bsl::function<void()>&) const BSLS_KEYWORD_OVERRIDE;

    /// Implements `Executor_TargetBase::move()`.
    void move(void* dst) BSLS_KEYWORD_NOEXCEPT BSLS_KEYWORD_OVERRIDE;

  public:
    // ACCESSORS
    bool equal(const Executor_TargetBase& other) const BSLS_KEYWORD_NOEXCEPT
        BSLS_KEYWORD_OVERRIDE;
    // Implements 'Executor_TargetBase::equal()'.

    void* target() BSLS_KEYWORD_NOEXCEPT BSLS_KEYWORD_OVERRIDE;

    /// Implements `Executor_TargetBase::target()`.
    const void* target() const BSLS_KEYWORD_NOEXCEPT BSLS_KEYWORD_OVERRIDE;

    const bsl::type_info&
    targetType() const BSLS_KEYWORD_NOEXCEPT BSLS_KEYWORD_OVERRIDE;
    // Implements 'Executor_TargetBase::targetType'.

    /// Implements `Executor_TargetBase::copy`.
    void copy(void* dst) const BSLS_KEYWORD_NOEXCEPT BSLS_KEYWORD_OVERRIDE;
};

// =========================
// class Executor_Box_DefImp
// =========================

/// Provides an implementation for `Executor_Box` that shares an executor
/// target with all copies of the same `Executor_Box` object.
class Executor_Box_DefImp {
  private:
    // PRIVATE DATA
    bsl::shared_ptr<Executor_TargetBase> d_target;

  public:
    // CREATORS

    /// Create a `Executor_Box_DefImp` object containing an object of type
    /// `Executor_Target<EXECUTOR>` direct-non-list-initialized by
    /// `bsl::move(executor)`.  Use the specified `allocator` to supply
    /// memory.
    ///
    /// `EXECUTOR` must meet the requirements of Executor (see package
    /// documentation).
    template <class EXECUTOR>
    Executor_Box_DefImp(EXECUTOR executor, bslma::Allocator* allocator);

  public:
    // MANIPULATORS

    /// Swap the contents of `*this` and `other`.
    void swap(Executor_Box_DefImp& other) BSLS_KEYWORD_NOEXCEPT;

  public:
    // ACCESSORS
    Executor_TargetBase* target() BSLS_KEYWORD_NOEXCEPT;

    /// Return a pointer to the contained `Executor_TargetBase` object.
    const Executor_TargetBase* target() const BSLS_KEYWORD_NOEXCEPT;
};

// =========================
// class Executor_Box_SboImp
// =========================

/// Provides an implementation for `Executor_Box` that allocates memory on
/// the stack.
class Executor_Box_SboImp {
  private:
    // PRIVATE TYPES

    /// Provides a "small" dummy object which size is used to calculate the
    /// size of the on-stack buffer used to store the executor target.
    struct Dummy {
        void* d_padding[4];

        bool operator==(const Dummy&) const BSLS_KEYWORD_NOEXCEPT
        {
            return false;
        }

        void post(const bsl::function<void()>&) const
        {
            // NOTHING
        }
    };

  private:
    // PRIVATE DATA

    // On-stack buffer used to store the executor target.  Note that the
    // size of the buffer is an arbitrary value.
    bsls::AlignedBuffer<sizeof(Executor_Target<Dummy>)> d_buffer;

  public:
    // CREATORS

    /// Create a `Executor_Box_SboImp` object containing an object of type
    /// `Executor_Target<EXECUTOR>` direct-non-list-initialized by
    /// `bsl::move(executor)`.  Ignore the specified `allocator`.  Note that
    /// this function will not compile unless
    /// `Executor_Box_SboImpCanHold<EXECUTOR>::value` is `true`.
    ///
    /// `EXECUTOR` must meet the requirements of Executor (see package
    /// documentation).
    template <class EXECUTOR>
    Executor_Box_SboImp(EXECUTOR          executor,
                        bslma::Allocator* allocator = 0) BSLS_KEYWORD_NOEXCEPT;

    Executor_Box_SboImp(const Executor_Box_SboImp& original)
        BSLS_KEYWORD_NOEXCEPT;

    /// Create a `Executor_Box_SboImp` object containing a copy of the
    /// specified `original` object's target.
    Executor_Box_SboImp(bslmf::MovableRef<Executor_Box_SboImp> original)
        BSLS_KEYWORD_NOEXCEPT;

    /// Destroy this object and the contained target with it.
    ~Executor_Box_SboImp();

  public:
    // MANIPULATORS
    Executor_Box_SboImp&
    operator=(const Executor_Box_SboImp& rhs) BSLS_KEYWORD_NOEXCEPT;

    /// Assign the contents of `rhs` to `*this`. Return `*this`.
    Executor_Box_SboImp& operator=(bslmf::MovableRef<Executor_Box_SboImp> rhs)
        BSLS_KEYWORD_NOEXCEPT;

    /// Swap the contents of `*this` and `other`.
    void swap(Executor_Box_SboImp& other) BSLS_KEYWORD_NOEXCEPT;

  public:
    // ACCESSORS
    Executor_TargetBase* target() BSLS_KEYWORD_NOEXCEPT;

    /// Return a pointer to the contained `Executor_TargetBase` object.
    const Executor_TargetBase* target() const BSLS_KEYWORD_NOEXCEPT;
};

/// Provides a metafunction to determine whether an `Executor_Box_SboImp`
/// can hold an executor of type `EXECUTOR`.
template <class EXECUTOR>
struct Executor_Box_SboImpCanHold {
    // CLASS DATA
    static BSLS_KEYWORD_CONSTEXPR_MEMBER bool value =
        sizeof(Executor_Box_SboImp) >= sizeof(Executor_Target<EXECUTOR>);
};

// ==================
// class Executor_Box
// ==================

/// Provides a polymorphic copyable container for executor targets.
class Executor_Box {
  private:
    // PRIVATE DATA
    bdlb::Variant2<Executor_Box_DefImp, Executor_Box_SboImp> d_imp;

  public:
    // CREATORS

    /// Create a `Executor_Box` object containing no target.
    Executor_Box() BSLS_KEYWORD_NOEXCEPT;

    /// Create a `Executor_Box` object containing an object of type
    /// `Executor_Target<EXECUTOR>` direct-non-list-initialized by
    /// `bsl::move(executor)`.  Use the specified `allocator` to supply
    /// memory.
    ///
    /// `EXECUTOR` must meet the requirements of Executor (see package
    /// documentation).
    template <class EXECUTOR>
    Executor_Box(EXECUTOR executor, bslma::Allocator* allocator);

  public:
    // MANIPULATORS

    /// Swap the contents of `*this` and `other`.
    void swap(Executor_Box& other) BSLS_KEYWORD_NOEXCEPT;

  public:
    // ACCESSORS
    Executor_TargetBase* target() BSLS_KEYWORD_NOEXCEPT;

    /// Return a pointer to the contained `Executor_TargetBase` object.
    const Executor_TargetBase* target() const BSLS_KEYWORD_NOEXCEPT;
};

// ==============
// class Executor
// ==============

/// Provides a polymorphic wrapper for executor types.
class Executor {
  private:
    // PRIVATE DATA
    Executor_Box d_box;

  public:
    // CREATORS

    /// Create a `Executor` object having no target.
    Executor() BSLS_KEYWORD_NOEXCEPT;

    /// Create a `Executor` object containing an executor target of type
    /// `EXECUTOR` direct-non-list-initialized by `bsl::move(executor)`.
    /// Optionally specify a `basicAllocator` used to supply memory.  If
    /// `basicAllocator` is 0, the currently installed default allocator is
    /// used.
    ///
    /// `EXECUTOR` must meet the requirements of Executor (see package
    /// documentation).
    template <class EXECUTOR>
    Executor(EXECUTOR          executor,
             bslma::Allocator* basicAllocator = 0);  // IMPLICIT

    /// Create a `Executor` object sharing a target with the specified
    /// `original` object, or a containing a copy of that target.
    ///
    /// Note that the specified `dummyAllocator` is ignored.  It is only
    /// provided so this constructor overload would be selected against the
    /// template one.
    Executor(const Executor&   original,
             bslma::Allocator* dummyAllocator = 0) BSLS_KEYWORD_NOEXCEPT;

    /// Create a `Executor` object sharing a target with the specified
    /// `original` object, or a containing a copy of that target, leaving
    /// `original` in an unspecified state.
    ///
    /// Note that the specified `dummyAllocator` is ignored.  It is only
    /// provided so this constructor overload would be selected against the
    /// template one.
    Executor(bslmf::MovableRef<Executor> original,
             bslma::Allocator* dummyAllocator = 0) BSLS_KEYWORD_NOEXCEPT;

  public:
    // MANIPULATORS

    /// If `*this` has a target, release the shared ownership, or destroy
    /// it.  Then direct-non-list-initialize a new executor target of type
    /// `EXECUTOR` by `bsl::move(executor)`. Return `*this`. Equivalent to
    /// `Executor(bsl::move(executor)).swap(*this)`.
    ///
    /// `EXECUTOR` must meet the requirements of Executor (see package
    /// documentation).
    template <class EXECUTOR>
    Executor& operator=(EXECUTOR executor);

    /// Make `*this` share a target with the specified `rhs` object, or
    /// contain a copy of that target.  Return `*this`.
    Executor& operator=(const Executor& rhs) BSLS_KEYWORD_NOEXCEPT;

    /// Make `*this` share a target with the specified `rhs` object, or
    /// contain a copy of that target, leaving `rhs` in an unspecified
    /// state.  Return `*this`.
    Executor& operator=(bslmf::MovableRef<Executor> rhs) BSLS_KEYWORD_NOEXCEPT;

    /// If `*this` has a target, release the shared ownership, or destroy
    /// it.  Then direct-non-list-initialize a new executor target of type
    /// `EXECUTOR` by `bsl::move(executor)` using the optionally specified
    /// `basicAllocator` to supply memory. Return `*this`.  Equivalent to
    /// `Executor(bsl::move(executor), basicAllocator).swap(*this)`.
    ///
    /// `EXECUTOR` must meet the requirements of Executor (see package
    /// documentation).
    template <class EXECUTOR>
    void assign(EXECUTOR executor, bslma::Allocator* basicAllocator = 0);

    /// Invoke `ExecutorTraits<E>::post(e, f2)`, where `e` is the target
    /// object (of type `E`) of `*this`, `f1` is the result of 'DECAY_COPY(
    /// bsl::forward<FUNCTION>(f))`, and `f2' is a function object of
    /// unspecified type that, when called as `f2()`, performs `f1()`. The
    /// behavior is undefined unless `*this` has a target.
    ///
    /// `FUNCTION` must meet the requirements of Destructible and
    /// CopyConstructible as specified in the C++ standard.  'DECAY_COPY(
    /// bsl::forward<FUNCTION>(f))()' shall be a valid expression.
    template <class FUNCTION>
    void post(BSLS_COMPILERFEATURES_FORWARD_REF(FUNCTION) f) const;

    /// Invoke `ExecutorTraits<E>::dispatch(e, f2)`, where `e` is the target
    /// object (of type `E`) of `*this`, `f1` is the result of 'DECAY_COPY(
    /// bsl::forward<FUNCTION>(f))`, and `f2' is a function object of
    /// unspecified type that, when called as `f2()`, performs `f1()`. The
    /// behavior is undefined unless `*this` has a target.
    ///
    /// `FUNCTION` must meet the requirements of Destructible and
    /// CopyConstructible as specified in the C++ standard.  'DECAY_COPY(
    /// bsl::forward<FUNCTION>(f))()' shall be a valid expression.
    template <class FUNCTION>
    void dispatch(BSLS_COMPILERFEATURES_FORWARD_REF(FUNCTION) f) const;

    /// Swap the contents of `*this` and `other`.
    void swap(Executor& other) BSLS_KEYWORD_NOEXCEPT;

  public:
    // ACCESSORS

    /// * If both `*this` and `rhs` contain no target, return `true`;
    /// * Otherwise, if only one of `*this` and `rhs` contains a target,
    ///   return `false`;
    /// * Otherwise, if `*this` and `rhs` share the same target, return
    ///   `true`;
    /// * Otherwise, if `*this` and `rhs` contain targets of different
    ///   types, return `false`;
    /// * Otherwise, return `e1 == e2`, where `e1` and `e2` are targets
    ///   (of type `E`) of `*this` and `rhs` respectively.
    bool operator==(const Executor& rhs) const BSLS_KEYWORD_NOEXCEPT;

    /// Return `!(*this == rhs)`.
    bool operator!=(const Executor& rhs) const BSLS_KEYWORD_NOEXCEPT;

    /// If `*this` has a target, return `true`.  Otherwise, return `false`.
    BSLS_KEYWORD_EXPLICIT
    operator bool() const BSLS_KEYWORD_NOEXCEPT;

    template <class T>
    T* target() BSLS_KEYWORD_NOEXCEPT;

    /// If `targetType() == typeid(T)`, return a pointer to the contained
    /// executor target.  Otherwise, return a null pointer value.
    template <class T>
    const T* target() const BSLS_KEYWORD_NOEXCEPT;

    /// If `*this` has a target of type `T`, return `typeid(T)`.  Otherwise,
    /// return `typeid(void)`.
    const bsl::type_info& targetType() const BSLS_KEYWORD_NOEXCEPT;
};

// FREE OPERATORS

/// Swap the contents of `lhs` and `rhs`.
void swap(Executor& lhs, Executor& rhs) BSLS_KEYWORD_NOEXCEPT;

// ============================================================================
//                           INLINE DEFINITIONS
// ============================================================================

// ---------------------
// class Executor_Target
// ---------------------

// CREATORS
template <class EXECUTOR>
template <class EXECUTOR_PARAM>
inline Executor_Target<EXECUTOR>::Executor_Target(
    BSLS_COMPILERFEATURES_FORWARD_REF(EXECUTOR_PARAM)
        executor) BSLS_KEYWORD_NOEXCEPT
: d_executor(BSLS_COMPILERFEATURES_FORWARD(EXECUTOR_PARAM, executor))
{
    // NOTHING
}

// MANIPULATORS
template <class EXECUTOR>
inline void
Executor_Target<EXECUTOR>::post(const bsl::function<void()>& f) const
{
    ExecutorTraits<EXECUTOR>::post(d_executor, f);
}

template <class EXECUTOR>
inline void
Executor_Target<EXECUTOR>::dispatch(const bsl::function<void()>& f) const
{
    ExecutorTraits<EXECUTOR>::dispatch(d_executor, f);
}

template <class EXECUTOR>
inline void Executor_Target<EXECUTOR>::move(void* dst) BSLS_KEYWORD_NOEXCEPT
{
    // PRECONDITIONS
    BSLS_ASSERT(dst);

    bslma::ConstructionUtil::construct(
        reinterpret_cast<Executor_Target<EXECUTOR>*>(dst),
        static_cast<bslma::Allocator*>(0),
        bslmf::MovableRefUtil::move(d_executor));
}

// ACCESSORS
template <class EXECUTOR>
inline bool Executor_Target<EXECUTOR>::equal(
    const Executor_TargetBase& other) const BSLS_KEYWORD_NOEXCEPT
{
    // PRECONDITIONS
    BSLS_ASSERT(targetType() == other.targetType());

    return *static_cast<const EXECUTOR*>(target()) ==
           *static_cast<const EXECUTOR*>(other.target());
}

template <class EXECUTOR>
inline void* Executor_Target<EXECUTOR>::target() BSLS_KEYWORD_NOEXCEPT
{
    return static_cast<void*>(&d_executor);
}

template <class EXECUTOR>
inline const void*
Executor_Target<EXECUTOR>::target() const BSLS_KEYWORD_NOEXCEPT
{
    return static_cast<const void*>(&d_executor);
}

template <class EXECUTOR>
inline const bsl::type_info&
Executor_Target<EXECUTOR>::targetType() const BSLS_KEYWORD_NOEXCEPT
{
    return typeid(EXECUTOR);
}

template <class EXECUTOR>
inline void
Executor_Target<EXECUTOR>::copy(void* dst) const BSLS_KEYWORD_NOEXCEPT
{
    // PRECONDITIONS
    BSLS_ASSERT(dst);

    bslma::ConstructionUtil::construct(
        reinterpret_cast<Executor_Target<EXECUTOR>*>(dst),
        static_cast<bslma::Allocator*>(0),
        d_executor);
}

// -------------------------
// class Executor_Box_DefImp
// -------------------------

// CREATORS
template <class EXECUTOR>
inline Executor_Box_DefImp::Executor_Box_DefImp(EXECUTOR          executor,
                                                bslma::Allocator* allocator)
: d_target(bsl::allocate_shared<Executor_Target<EXECUTOR> >(
      allocator,
      bslmf::MovableRefUtil::move(executor)))
{
    // PRECONDITIONS
    BSLS_ASSERT(allocator);
}

// -------------------------
// class Executor_Box_SboImp
// -------------------------

// CREATORS
template <class EXECUTOR>
inline Executor_Box_SboImp::Executor_Box_SboImp(EXECUTOR          executor,
                                                bslma::Allocator* allocator)
    BSLS_KEYWORD_NOEXCEPT : d_buffer()
{
    // PRECONDITIONS
    BSLMF_ASSERT(Executor_Box_SboImpCanHold<EXECUTOR>::value);

    bslma::ConstructionUtil::construct(
        reinterpret_cast<Executor_Target<EXECUTOR>*>(d_buffer.buffer()),
        allocator,
        bslmf::MovableRefUtil::move(executor));
}

// ------------------
// class Executor_Box
// ------------------

// CREATORS
template <class EXECUTOR>
inline Executor_Box::Executor_Box(EXECUTOR          executor,
                                  bslma::Allocator* allocator)
: d_imp()
{
    typedef
        typename bsl::conditional<Executor_Box_SboImpCanHold<EXECUTOR>::value,
                                  Executor_Box_SboImp,
                                  Executor_Box_DefImp>::type Imp;

    // PRECONDITIONS
    BSLS_ASSERT(allocator);

    d_imp.createInPlace<Imp>(bslmf::Util::moveIfSupported(executor),
                             allocator);
}

// --------------
// class Executor
// --------------

// CREATORS
template <class EXECUTOR>
inline Executor::Executor(EXECUTOR executor, bslma::Allocator* basicAllocator)
: d_box(bslmf::Util::moveIfSupported(executor),
        bslma::Default::allocator(basicAllocator))
{
    // NOTHING
}

// MANIPULATORS
template <class EXECUTOR>
inline Executor& Executor::operator=(EXECUTOR executor)
{
    Executor(bslmf::Util::moveIfSupported(executor)).swap(*this);
    return *this;
}

template <class EXECUTOR>
inline void Executor::assign(EXECUTOR          executor,
                             bslma::Allocator* basicAllocator)
{
    Executor(bslmf::Util::moveIfSupported(executor), basicAllocator)
        .swap(*this);
}

template <class FUNCTION>
inline void Executor::post(BSLS_COMPILERFEATURES_FORWARD_REF(FUNCTION) f) const
{
    // PRECONDITIONS
    BSLS_ASSERT(d_box.target());

    d_box.target()->post(BSLS_COMPILERFEATURES_FORWARD(FUNCTION, f));
}

template <class FUNCTION>
inline void Executor::dispatch(BSLS_COMPILERFEATURES_FORWARD_REF(FUNCTION)
                                   f) const
{
    // PRECONDITIONS
    BSLS_ASSERT(d_box.target());

    d_box.target()->dispatch(BSLS_COMPILERFEATURES_FORWARD(FUNCTION, f));
}

// ACCESSORS
template <class T>
inline T* Executor::target() BSLS_KEYWORD_NOEXCEPT
{
    const bool match = targetType() == typeid(T);
    return match ? static_cast<T*>(d_box.target()->target()) : 0;
}

template <class T>
inline const T* Executor::target() const BSLS_KEYWORD_NOEXCEPT
{
    const bool match = targetType() == typeid(T);
    return match ? static_cast<const T*>(d_box.target()->target()) : 0;
}

}  // close package namespace

// FREE OPERATORS
inline void mwcex::swap(Executor& lhs, Executor& rhs) BSLS_KEYWORD_NOEXCEPT
{
    lhs.swap(rhs);
}

}  // close enterprise namespace

#endif
