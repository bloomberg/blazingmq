// Copyright 2017-2023 Bloomberg Finance L.P.
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

// mwcu_atomicvalidator.h                                             -*-C++-*-
#ifndef INCLUDED_MWCU_ATOMICVALIDATOR
#define INCLUDED_MWCU_ATOMICVALIDATOR

//@PURPOSE: Provide a mechanism to acquire/release and invalidate a resource.
//
//@CLASSES:
//  mwcu::AtomicValidator:      acquire/release/invalidate a resource
//  mwcu::AtomicValidatorSp:    shared pointer to an 'mwcu::AtomicValidator'
//  mwcu::AtomicValidatorGuard: guard for on a 'mwcu::AtomicValidator' object
//
//@DESCRIPTION: The mechanisms provided by this component allows to manage the
// lifecycle of a resource thanks to the 'mwcu::AtomicValidator' object, which
// can be acquired with the associated 'mwcu::AtomicValidatorGuard'.  For usage
// convenience, a 'mwcu::AtomicValidatorSp' typedef is provided.  The resource
// can be acquired by many objects, but once invalidated, it cannot be acquired
// again until a subsequent reset.  This is particularly useful in asynchronous
// and multi-threaded applications to ensure a specific object is still valid
// before using it, and making sure it remains valid until all users of it have
// released their handles.
//
/// Usage
///-----
// First, create an object which will represent the resource under validation
// check:
//..
//  class MyResource {
//      typedef bsl::shared_ptr<mwcu::AtomicValidator> AtomicValidatorSp;
//      AtomicValidatorSp d_validator_sp;
//
//  public:
//      MyResource();
//      ~MyResource();
//      void callingCallback();
//      void myCallbackFunction(const AtomicValidatorSp& validator);
//  };
//..
// In the construction, we create the validator:
//..
//  MyResource::MyResource()
//  : d_validator_sp(new(*allocator) mwcu::AtomicValidator(), allocator)
//  {
//       // Nothing
//  }
//..
// In the destructor, we invalidate the resource: this will lock and prevent
// destruction of this object as long as there is someone using it, and will
// then mark it invalid so no one can acquire it anymore:
//..
//  MyResource::~MyResource()
//  {
//     d_validator_sp->invalidate();
//  }
//..
// Now, let's make an asynchronous call.  We bind the validator to the callback
// method, so that we can check whether the object is still valid:
//..
//  void MyResource::callingCallback()
//  {
//     makeAsyncCall(bdlf::BindUtil::bind(&MyResource::myCallbackFunction,
//                                        this,
//                                        d_validator_sp));
//  }
//..
// Finally, in the callback method, before using the resource object, we first
// create a 'mwcu::AtomicValidatorGuard' on the validator, and check whether
// acquisition was successful or not:
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

// MWC

// BDE
#include <bsl_memory.h>
#include <bsls_assert.h>
#include <bsls_atomic.h>
#include <bsls_performancehint.h>

namespace BloombergLP {
namespace mwcu {

// =====================
// class AtomicValidator
// =====================

/// Provide a mechanism to acquire/release and invalidate a resource.
class AtomicValidator {
  private:
    // DATA
    bsls::AtomicInt d_count;  // 2 * the number of outstanding calls to
                              // 'acquire' + 1 if this object is invalidated.
                              // See the implementation of 'acquire'.

  public:
    // CREATORS

    /// Create a `AtomicValidator`.  If the optionally specified `isValid`
    /// is `false`, initialize the object in an invalidated state.
    explicit AtomicValidator(bool isValid = true);

    // MANIPULATORS

    /// Reset this object to its default state after it has been
    /// invalidated.  The behavior is undefined unless this object is
    /// currently invalidated.
    void reset();

    /// Set this `Validator` to an INVALID state. This method will block
    /// until all locks on the `Validator` have been released.  Once a
    /// `Validator` is INVALID, no other objects can acquire it.  The
    /// behavior is undefined if the validator has already been invalidated.
    void invalidate();

    /// Try to acquire the resource. Returns true if the acquisition was
    /// successfull and the object is not INVALID.
    bool acquire();

    /// Release the resource (previously acquired by a call to `acquire`).
    /// The behavior is undefined unless `acquire` has been called
    /// previously and returned `true`.
    void release();
};

// ======================
// type AtomicValidatorSp
// ======================

typedef bsl::shared_ptr<AtomicValidator> AtomicValidatorSp;

// ==========================
// class AtomicValidatorGuard
// ==========================

/// This class implements a guard-like mechanism for acquisition and release
/// of an AtomicValidator object.
class AtomicValidatorGuard {
  private:
    // DATA
    AtomicValidator* d_validator_p;  // Pointer to the validator under guard,
                                     // held and not owned

    bool d_isAcquired;  // Flag indicating if the validator had
                        // been successfully acquired at
                        // construction time

  public:
    // CREATORS

    /// Create a guard object that conditionally manages the specified
    /// `validator` (if non-zero), and invokes the `acquire` method on
    /// `validator`.  Note that `validator` must remain valid throughout the
    /// lifetime of this guard, or until `release` is called.
    explicit AtomicValidatorGuard(AtomicValidator* validator);

    /// Create a guard object that conditionally manages the specified
    /// `validator` (if non-zero) and, unless the specified
    /// `preAcquiredFlag` is non-zero, invokes the `acquire` method on
    /// `validator`.  Note that `validator` must remain valid throughout the
    /// lifetime of this guard, or until `release` is called.
    AtomicValidatorGuard(AtomicValidator* validator, int preAcquiredFlag);

    /// Default destructor.  Destroy this guard object and invoke the
    /// `release` method on the validator object under management by this
    /// guard, if any.  If no validator is currently being managed, this
    /// method has no effect.
    ~AtomicValidatorGuard();

    // MANIPULATORS

    /// Return the address of the modifiable validator object under
    /// management by this guard if it has been acquired successfully and
    /// release the validator from further management by this guard.  If the
    /// validator has not been acquired successfully, or if no validator is
    /// currently being managed, return 0 with no other effect.  Note that
    /// this operation does *not* release the validator object (if any) that
    /// was under management.
    AtomicValidator* release();

    // ACCESSORS

    /// Return `true` if this guard object is valid, and `false` otherwise.
    /// A valid guard object is a guard object in which the validator object
    /// under management, if any, has been successfully acquired through
    /// invocation of the `acquire` method on the validator, meaning the
    /// validator object (if any) is still valid and can safely be used.
    /// Note that calling `isValid` on a guard object whose validator is
    /// NULL, or on a guard object that has been released, will always
    /// return true.
    bool isValid() const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ---------------------
// class AtomicValidator
// ---------------------

// MANIPULATORS
inline bool AtomicValidator::acquire()
{
    // Try to increment 'd_count' by 2 while the first bit isn't set.  If the
    // first bit is set, (by 'invalidate'), we've been invalidated and should
    // fail.
    int count = d_count;
    while (!(count & 1)) {
        int oldCount = d_count.testAndSwap(count, count + 2);
        if (oldCount == count) {
            return true;  // RETURN
        }

        count = oldCount;
    }

    return false;  // Invalidate has been called
}

inline void AtomicValidator::release()
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_count >= 2);

    d_count -= 2;

    // POSTCONDITIONS
    BSLS_ASSERT_SAFE(d_count >= 0);
}

// --------------------------
// class AtomicValidatorGuard
// --------------------------

// CREATORS
inline AtomicValidatorGuard::AtomicValidatorGuard(AtomicValidator* validator)
: d_validator_p(validator)
, d_isAcquired(validator ? validator->acquire() : false)
{
    // NOTHING
}

inline AtomicValidatorGuard::AtomicValidatorGuard(AtomicValidator* validator,
                                                  int preAcquiredFlag)
: d_validator_p(validator)
, d_isAcquired(preAcquiredFlag || (validator ? validator->acquire() : false))
{
    // NOTHING
}

inline AtomicValidatorGuard::~AtomicValidatorGuard()
{
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!d_isAcquired)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return;  // RETURN
    }

    if (BSLS_PERFORMANCEHINT_PREDICT_LIKELY(d_validator_p)) {
        d_validator_p->release();
    }
}

// ACCESSORS
inline bool AtomicValidatorGuard::isValid() const
{
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!d_validator_p)) {
        // A guard on a NULL validator is valid...
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return true;  // RETURN
    }

    return d_isAcquired;
}

}  // close package namespace
}  // close enterprise namespace

#endif
