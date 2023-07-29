// Copyright 2020-2023 Bloomberg Finance L.P.
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

// mwcu_sharedresource.h                                              -*-C++-*-
#ifndef INCLUDED_MWCU_SHAREDRESOURCE
#define INCLUDED_MWCU_SHAREDRESOURCE

//@PURPOSE: Provides a mechanism to manage the lifetime of a shared resource.
//
//@CLASSES:
//  mwcu::SharedResource: a shared resource
//
//@SEE ALSO:
//  mwcu_weakmemfn
//
//@DESCRIPTION:
// This component provides a utility mechanism 'mwcu::SharedResource' used to
// manage the lifetime of a shared resource.
//
// The way this works is that a 'mwcu::SharedResource<R>' holds a shared
// pointer to the target resource of type 'R'.  The shared pointer doesn't own
// that resource, but keeps track of its usage, containing a custom deleter
// that will signal when all copies of the pointer are invalidated.  The user
// may obtain such copies, or equivalent weak pointer, via the 'acquire' and
// 'acquireWeak' functions respectively.  A call to 'invalidate' will reset the
// stored shared pointer and block the calling thread until all its copies are
// invalidated.
//
/// Deleter
///-------
// This components also provides two deleter class that can be used with
// 'mwcu::SharedResource' to free the resource upon invalidation.
//
// The first one, used by default, is 'mwcu::SharedResourceNoOpDeleter'.  As
// the name suggests this deleter does nothing to the managed resource.
//
// The second one, 'mwcu::SharedResourceFactoryDeleter', is a wrapper around a
// pointer to any factory-like type, i.e. a type that has a 'deleteObject'
// function.  All types implementing the 'bslma::Allocator' or 'bdlma::Deleter'
// protocol fall into this category.
//
/// Comparison to 'mwcu::AtomicValidator'
///-------------------------------------
// This component is intended as a replacement for 'mwcu_atomicvalidator'.
// What the validator can do, this component can do better.  The two main
// differences are the interface and the approach to synchronization.
//
// Interface wise, the validator is more intrusive and requires more
// boilerplate code to achieve the same results.  For example, in a common use
// case, when the validator is used to synchronize with the completion of an
// async operation, the validator has to be bind to the callback, which
// affects the signature of that function.  The example below, copied from the
// components documentation, demonstrates that.
//..
//  void MyResource::myCallbackFunction(const AtomicValidatorSp& validator)
//  {
//     mwcu::AtomicValidatorGuard guard(validator.ptr());
//     if (!guard.isValid()) {
//         // 'this' got destroyed before the callback and is not usable
//         return;
//     }
//     // At this point, we have guarantee that this won't be destroy through
//     // the entire lifecycle of guard.
//  }
//..
// 'mwcu::SharedResource', on the other hand, is less intrusive, as will be
// shown in in the usage example below.
//
// Implementation wise, the validator relies on atomics and implements the
// waiting as a spinlock, while this component is implemented using a semaphore
// and waits passively.  Given that the primary use case for both components is
// synchronization with the completion of async operations, passive waiting is
// preferable, as such operations may take arbitrary time to complete.
//
/// Thread safety
///-------------
// With the exception of the 'reset' and (partially) 'invalidate' member
// functions, 'mwcu::SharedResource' is fully thread-safe, meaning that
// multiple threads may use their own instances of the class or use a shared
// instance without further synchronization. 'invalidate' may be invoked
// concurrently with other thread-safe function, but not with itself.
//
/// Usage
///-----
//..
//  class MyService {
//      // Provides a usage example.
//
//    private:
//      // PRIVATE DATA
//      bdlmt::ThreadPool               *d_threadPool_p;
//          // Used to spawn async operations.
//
//      mwcu::SharedResource<MyService>  d_self;
//          // Used to synchronize with async operations completion on
//          // destruction.
//
//    private:
//      // PRIVATE MANIPULATORS
//      void doStuff();
//
//    public:
//      // CREATORS
//      explicit
//      MyService(bdlmt::ThreadPool *threadPool);
//          // Create a 'MyService' object. Specify a 'threadPool' used to
//          // spawn async operations.
//
//    ~MyService();
//        // Destroy this object. Block the calling thread until all async
//        // operations are completed.
//
//    public:
//      // MANIPULATORS
//      void asyncDoStuff();
//          // Initiate an async operation that does something.
//  };
//..
// Here we have a simple 'MyService' class that is able to perform
// asynchronous operations initiated via its 'asyncDoStuff' function.  In this
// example the service is injected a thread pool that is used as a context of
// execution for such operations, but the way these operations are spawn is not
// really important.  What is important is that each such operation accesses
// the object internal data, so it is crucial that the 'MyService' object lives
// as long as there is at least one operation in progress.  Hence, lets make
// the destructor block until all async operation are completed.
//
// For that we'll need a 'mwcu::SharedResource<MyService>' object held as a
// data member of 'MyService'.  The Object is initialized in the constructor.
//..
//  MyService::MyService(bdlmt::ThreadPool *threadPool)
//  : d_threadPool_p(threadPool)
//  , d_self(this)
//  {
//      // NOTHING
//  }
//..
// Now, each time we spawn an async operation, we bind the shared resource to
// to the callback.
//..
//  void
//  MyService::asyncDoStuff()
//  {
//      // acquire the shared resource and bind to the the async operation
//      d_threadPool_p->enqueueJob(bdlf::BindUtil::bind(&MyService::doStuff,
//                                                      d_self.acquire()));
//  }
//..
// That works nicely, because 'bind' works with shared pointers out-of-the-box.
//
// When 'MyService' is destroyed we invalidate the shared resource.  That will
// block the calling thread until the shared resource is out of use.
//..
//  MyService::~MyService()
//  {
//      d_self.invalidate();
//  }
//..
//
// So far so good.  But what if we don't want the destructor to wait for async
// operations to complete?  In that case we might bind a weak pointer to the
// async operation.  Unfortunately, 'bind' doesn't work with weak pointers the
// same way it does with shared pointers.  Helpfully, the 'mwcu' package
// provides an utility component 'mwcu_weakmemfn', that can help us with that.
//..
//  void
//  MyService::asyncDoStuff()
//  {
//      // acquire the shared resource and bind to the the async operation
//      d_threadPool_p->enqueueJob(mwcu::WeakMemFnUtil::weakMemFn(
//                                                      &MyService::doStuff,
//                                                      d_self.acquireWeak()));
//  }
//..
// In the implementation above we bind a weak pointer to the callback in a way
// that 'MyService::doStuff' will only be invoked if at the moment of the
// callback invocation the pointer is still valid.  This way, unless there is
// an invocation of 'MyService::doStuff' in progress, the destructor of
// 'MyService' will no block.

// MWC

// BDE
#include <bdlma_deleter.h>
#include <bsl_memory.h>
#include <bslma_allocator.h>
#include <bslma_default.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bslmt_semaphore.h>
#include <bsls_assert.h>
#include <bsls_keyword.h>
#include <bsls_nullptr.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace bslma {
class SharedPtrRep;
}

namespace mwcu {

template <class, class>
class SharedResourceFactoryDeleter;
template <class>
class SharedResourceNoOpDeleter;

// ============================
// class SharedResource_Deleter
// ============================

/// Provides an implementation of the `bdlma::Deleter` protocol wrapping a
/// custom deleter object.
///
/// `DELETER` must satisfy the requirements of noexcept-CopyConstructible
/// and Destructible as specified in the C++ standard.  Given an object `d`
/// of type `DELETER` and a valid resource pointer `r` of type `RESOURCE*`,
/// `d(r)` shall be a valid expression, and shall not throw.
template <class RESOURCE, class DELETER>
class SharedResource_Deleter : public bdlma::Deleter<void> {
  private:
    // PRIVATE DATA
    DELETER d_deleter;

  public:
    // CREATORS

    /// Create a `SharedResource_Deleter` object holding a copy of the
    /// specified `deleter`.
    explicit SharedResource_Deleter(const DELETER& deleter)
        BSLS_KEYWORD_NOEXCEPT;

  public:
    // MANIPULATORS

    /// Assign the specified `deleter` to this object.
    ///
    /// `DELETER` must satisfy the requirements of noexcept-CopyAssignable
    /// specified in the C++ standard.
    void assign(const DELETER& deleter) BSLS_KEYWORD_NOEXCEPT;

    void
    deleteObject(void* resource) BSLS_KEYWORD_NOEXCEPT BSLS_KEYWORD_OVERRIDE;
    // Delete the specified 'resource' by applying the stored 'deleter', as
    // 'deleter(static_cast<RESOURCE*>(resource))'.
};

// ==========================
// struct SharedResource_Base
// ==========================

/// Provides a partial implementation for `SharedResource` to reduce the
/// amount of generated template code.
struct SharedResource_Base {
    // PUBLIC DATA

    /// Semaphore notified when all strong references on the shared state
    /// are released.
    bslmt::Semaphore d_semaphore;

    /// Pointer to the resource.  Note that we could have stored this
    /// pointer in the shared state, but store there a pointer to `this`
    /// object instead as a minor optimization.
    void* d_resource_p;

    /// Pointer to the deleter used to free the resource upon invalidation.
    /// Note that the deleter is stored in the parent `SharedResource`
    /// object.
    bdlma::Deleter<void>* d_deleter_p;

    /// Pointer to the owned shared state.  When the last strong reference
    /// is released the deleter of the shared state calls `post` on
    /// `d_semaphore`.
    bslma::SharedPtrRep* d_sharedPtrRep_p;

    /// Used to allocate the shared state.
    bslma::Allocator* d_allocator_p;

    // CREATORS

    /// Create a `SharedResource_Base` object managing no resource.  Specify
    /// a `deleter` used to free the resource upon invalidation.  Specify an
    /// `allocator` used to supply memory.
    ///
    /// Note that the supplied allocator shall remain valid until this
    /// object is destroyed, and all shared/weak pointers obtained from this
    /// object release ownership.
    explicit SharedResource_Base(bdlma::Deleter<void>* deleter,
                                 bslma::Allocator*     allocator)
        BSLS_KEYWORD_NOEXCEPT;

    /// Create a `SharedResource_Base` object managing the specified
    /// `resource`.  Specify a `deleter` used to free the resource upon
    /// invalidation.  Optionally specify an `allocator` used to supply
    /// memory.  In case of exception, the resource is freed using the
    /// supplied deleter.
    ///
    /// Note that the supplied allocator shall remain valid until this
    /// object is destroyed, and all shared/weak pointers obtained from this
    /// object release ownership.
    SharedResource_Base(void*                 resource,
                        bdlma::Deleter<void>* deleter,
                        bslma::Allocator*     allocator);

    /// Destroy this object.  If this object manages a resource, call
    /// `invalidate()`.
    ~SharedResource_Base();

    // MANIPULATORS

    /// Return a shared pointer to the managed resource, or a null pointer
    /// if this object manages no resource.
    ///
    /// Note that if `invalidate` is currently being invoked, it is still
    /// possible to obtain a valid shared pointer until `invalidate`
    /// returns, i.e. until all shared pointer obtained from this object
    /// release ownership.
    bsl::shared_ptr<void> acquire() BSLS_KEYWORD_NOEXCEPT;

    /// Invalidate the managed resource and block the calling thread until
    /// all shared pointers obtained from this object release ownership.
    /// Leave this object managing no resource.  The behavior is undefined
    /// if this object manages no resource.
    ///
    /// Note that this function shall not be invoked concurrently.
    void invalidate() BSLS_KEYWORD_NOEXCEPT;

    /// Reset this object leaving it managing no resource.  If prior to this
    /// call this object was managing a resource, the resource is released
    /// via a call to `invalidate`.
    ///
    /// Note that this function is not thread-safe.
    void reset() BSLS_KEYWORD_NOEXCEPT;

    /// Reset this object leaving it managing the specified `resource`.  In
    /// case of exception, the resource is freed using the supplied deleter
    /// and this object is left managing no resource.  The behavior is
    /// undefined if this object already manages a resource.
    ///
    /// Note that this function is not thread-safe.
    void reset(void* resource);

    // ACCESSORS

    /// Return `true` if this object manages a resource, and `false`
    /// otherwise.
    bool isValid() const BSLS_KEYWORD_NOEXCEPT;
};

// ====================
// class SharedResource
// ====================

/// Provides an utility mechanism to manage the lifetime of a shared
/// resource.
///
/// `DELETER` must satisfy the requirements of noexcept-CopyConstructible
/// and Destructible as specified in the C++ standard.  Given an object `d`
/// of type `DELETER` and a valid resource pointer `r` of type `RESOURCE*`,
/// `d(r)` shall be a valid expression, and shall not throw.
template <class RESOURCE, class DELETER = SharedResourceNoOpDeleter<RESOURCE> >
class SharedResource {
  private:
    // PRIVATE DATA

    // Deleter used to free the resource upon invalidation.
    SharedResource_Deleter<RESOURCE, DELETER> d_deleter;

    // Partial implementation for this class.  Note that this object shall
    // be destroyed before `d_deleter`.
    SharedResource_Base d_base;

  private:
    // NOT IMPLEMENTED
    SharedResource(const SharedResource&) BSLS_KEYWORD_DELETED;
    SharedResource& operator=(const SharedResource&) BSLS_KEYWORD_DELETED;

  public:
    // CREATORS
    explicit SharedResource(bslma::Allocator* basicAllocator = 0)
        BSLS_KEYWORD_NOEXCEPT;
    SharedResource(bsl::nullptr_t,
                   bslma::Allocator* basicAllocator = 0) BSLS_KEYWORD_NOEXCEPT;
    // IMPLICIT
    // Create a 'SharedResource' object managing no resource.  Optionally
    // specify a 'basicAllocator' used to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.
    //
    // Note that the supplied allocator shall remain valid until this
    // object is destroyed, and all shared/weak pointers obtained from this
    // object release ownership.
    //
    // Note also that that the 'nullptr_t' overload is identical in
    // behavior to the first one and is provided for convenience reasons,
    // as it might be useful when emplacing instances of this class into
    // containers.
    //
    // 'DELETER' must be noexcept-DefaultConstructible as specified in the
    //  C++ standard.

    /// Create a `SharedResource` object managing the specified `resource`.
    /// Optionally specify a `deleter` used to free the resource upon
    /// invalidation.  If the deleter is not specified, a
    /// default-constructed instance is used.  Optionally specify a
    /// `basicAllocator` used to supply memory.  If `basicAllocator` is 0,
    /// the currently installed default allocator is used.  In case of
    /// exception, the resource is freed using the supplied deleter.
    ///
    /// Note that the supplied allocator shall remain valid until this
    /// object is destroyed, and all shared/weak pointers obtained from this
    /// object release ownership.
    ///
    /// `DELETER` must be either noexcept-DefaultConstructible or
    /// noexcept-CopyConstructible as specified in the C++ standard,
    /// depending on the chosen overload.
    explicit SharedResource(RESOURCE*         resource,
                            bslma::Allocator* basicAllocator = 0);
    SharedResource(RESOURCE*         resource,
                   const DELETER&    deleter,
                   bslma::Allocator* basicAllocator = 0);

  public:
    // MANIPULATORS

    /// Return a shared pointer to the managed resource, or an null pointer
    /// if this object manages no resource.
    ///
    /// Note that if `invalidate` is currently being invoked, it is still
    /// possible to obtain a valid shared pointer until `invalidate`
    /// returns, i.e. until all shared pointer obtained from this object
    /// release ownership.
    bsl::shared_ptr<RESOURCE> acquire() BSLS_KEYWORD_NOEXCEPT;

    /// Return a weak pointer to the managed resource, as if by
    /// `return bsl::weak_ptr<RESOURCE>(acquire())`.
    bsl::weak_ptr<RESOURCE> acquireWeak() BSLS_KEYWORD_NOEXCEPT;

    /// Invalidate the managed resource and block the calling thread until
    /// all shared pointers obtained from this object release ownership.
    /// Leave this object managing no resource.  The behavior is undefined
    /// if this object manages no resource.
    ///
    /// Note that this function shall not be invoked concurrently.
    void invalidate() BSLS_KEYWORD_NOEXCEPT;

    /// Reset this object leaving it managing no resource.  If prior to this
    /// call this object was managing a resource, the resource is released
    /// via a call to `invalidate`.
    ///
    /// Note that this function is not thread-safe.
    void reset() BSLS_KEYWORD_NOEXCEPT;

    /// Reset this object leaving it managing the specified `resource`.
    /// Optionally specify a `deleter` used to free the resource upon
    /// invalidation.  If prior to this call this object was managing a
    /// resource, the resource is released via a call to `invalidate`.  In
    /// case of exception, the resource is freed using the supplied deleter
    /// and this object is left managing no resource.
    ///
    /// Note that this function is not thread-safe.
    ///
    /// `DELETER` must be noexcept-CopyAssignable and, if the first
    /// overload is chosen, noexcept-DefaultConstructible as specified in
    /// the C++ standard.
    void reset(RESOURCE* resource);
    void reset(RESOURCE* resource, const DELETER& deleter);

  public:
    // ACCESSORS

    /// Return `true` if this object manages a resource, and `false`
    /// otherwise.
    bool isValid() const BSLS_KEYWORD_NOEXCEPT;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(SharedResource, bslma::UsesBslmaAllocator)
};

// ==================================
// class SharedResourceFactoryDeleter
// ==================================

/// Provides a resource deleter wrapping a pointer to a factory object.
///
/// Given an object `f` of type `FACTORY` and a valid resource pointer `r`
/// of type `RESOURCE*`, `f.deleteObject(r)` shall be a valid expression,
/// and shall not throw.
template <class RESOURCE, class FACTORY>
class SharedResourceFactoryDeleter {
  private:
    // PRIVATE DATA
    FACTORY* d_factory_p;

  public:
    // CREATORS

    /// Create a `SharedResourceBdlmaDeleterDeleter` object wrapping the
    /// specified `factory` pointer.
    explicit SharedResourceFactoryDeleter(FACTORY* factory)
        BSLS_KEYWORD_NOEXCEPT;

  public:
    // ACCESSORS

    /// Delete the specified `resource` by applying the stored `factory`,
    /// as `factory->deleteObject(resource)`.
    void operator()(RESOURCE* resource) const BSLS_KEYWORD_NOEXCEPT;
};

// ===============================
// class SharedResourceNoOpDeleter
// ===============================

/// Provides a resource deleter that does nothing.
template <class RESOURCE>
class SharedResourceNoOpDeleter {
  public:
    // ACCESSORS

    /// Do nothing.
    void operator()(RESOURCE*) const BSLS_KEYWORD_NOEXCEPT;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ----------------------------
// class SharedResource_Deleter
// ----------------------------

// CREATORS
template <class RESOURCE, class DELETER>
inline SharedResource_Deleter<RESOURCE, DELETER>::SharedResource_Deleter(
    const DELETER& deleter) BSLS_KEYWORD_NOEXCEPT : d_deleter(deleter)
{
    // NOTHING
}

// MANIPULATORS
template <class RESOURCE, class DELETER>
inline void SharedResource_Deleter<RESOURCE, DELETER>::assign(
    const DELETER& deleter) BSLS_KEYWORD_NOEXCEPT
{
    d_deleter = deleter;
}

template <class RESOURCE, class DELETER>
inline void SharedResource_Deleter<RESOURCE, DELETER>::deleteObject(
    void* resource) BSLS_KEYWORD_NOEXCEPT
{
    d_deleter(static_cast<RESOURCE*>(resource));
}

// --------------------
// class SharedResource
// --------------------

// CREATORS
template <class RESOURCE, class DELETER>
inline SharedResource<RESOURCE, DELETER>::SharedResource(
    bslma::Allocator* basicAllocator) BSLS_KEYWORD_NOEXCEPT
: d_deleter(DELETER()),
  d_base(&d_deleter, bslma::Default::allocator(basicAllocator))
{
    // NOTHING
}

template <class RESOURCE, class DELETER>
inline SharedResource<RESOURCE, DELETER>::SharedResource(
    bsl::nullptr_t,
    bslma::Allocator* basicAllocator) BSLS_KEYWORD_NOEXCEPT
: d_deleter(DELETER()),
  d_base(&d_deleter, bslma::Default::allocator(basicAllocator))
{
    // NOTHING
}

template <class RESOURCE, class DELETER>
inline SharedResource<RESOURCE, DELETER>::SharedResource(
    RESOURCE*         resource,
    bslma::Allocator* basicAllocator)
: d_deleter(DELETER())
, d_base(resource, &d_deleter, bslma::Default::allocator(basicAllocator))
{
    // NOTHING
}

template <class RESOURCE, class DELETER>
inline SharedResource<RESOURCE, DELETER>::SharedResource(
    RESOURCE*         resource,
    const DELETER&    deleter,
    bslma::Allocator* basicAllocator)
: d_deleter(deleter)
, d_base(resource, &d_deleter, bslma::Default::allocator(basicAllocator))
{
    // NOTHING
}

// MANIPULATORS
template <class RESOURCE, class DELETER>
inline bsl::shared_ptr<RESOURCE>
SharedResource<RESOURCE, DELETER>::acquire() BSLS_KEYWORD_NOEXCEPT
{
    return bsl::static_pointer_cast<RESOURCE>(d_base.acquire());
}

template <class RESOURCE, class DELETER>
inline bsl::weak_ptr<RESOURCE>
SharedResource<RESOURCE, DELETER>::acquireWeak() BSLS_KEYWORD_NOEXCEPT
{
    return bsl::weak_ptr<RESOURCE>(acquire());
}

template <class RESOURCE, class DELETER>
inline void
SharedResource<RESOURCE, DELETER>::invalidate() BSLS_KEYWORD_NOEXCEPT
{
    d_base.invalidate();
}

template <class RESOURCE, class DELETER>
inline void SharedResource<RESOURCE, DELETER>::reset() BSLS_KEYWORD_NOEXCEPT
{
    d_base.reset();
}

template <class RESOURCE, class DELETER>
inline void SharedResource<RESOURCE, DELETER>::reset(RESOURCE* resource)
{
    reset(resource, DELETER());
}

template <class RESOURCE, class DELETER>
inline void SharedResource<RESOURCE, DELETER>::reset(RESOURCE*      resource,
                                                     const DELETER& deleter)
{
    // cleanup before resetting the deleter, as the deleter is used to free the
    // resource
    d_base.reset();

    // reset the deleter
    d_deleter.assign(deleter);

    // reset the resource
    d_base.reset(resource);
}

// ACCESSORS
template <class RESOURCE, class DELETER>
inline bool
SharedResource<RESOURCE, DELETER>::isValid() const BSLS_KEYWORD_NOEXCEPT
{
    return d_base.isValid();
}

// ----------------------------------
// class SharedResourceFactoryDeleter
// ----------------------------------

// CREATORS
template <class RESOURCE, class FACTORY>
inline SharedResourceFactoryDeleter<RESOURCE, FACTORY>::
    SharedResourceFactoryDeleter(FACTORY* factory) BSLS_KEYWORD_NOEXCEPT
: d_factory_p(factory)
{
    // PRECONDITIONS
    BSLS_ASSERT(factory);
}

// ACCESSORS
template <class RESOURCE, class FACTORY>
inline void SharedResourceFactoryDeleter<RESOURCE, FACTORY>::operator()(
    RESOURCE* resource) const BSLS_KEYWORD_NOEXCEPT
{
    d_factory_p->deleteObject(resource);
}

// --------------------------------
// struct SharedResourceNoOpDeleter
// --------------------------------

// ACCESSORS
template <class RESOURCE>
inline void SharedResourceNoOpDeleter<RESOURCE>::operator()(RESOURCE*) const
    BSLS_KEYWORD_NOEXCEPT
{
    // NOTHING
}

}  // close package namespace
}  // close enterprise namespace

#endif
