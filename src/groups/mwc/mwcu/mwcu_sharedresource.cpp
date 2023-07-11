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

// mwcu_sharedresource.cpp                                            -*-C++-*-
#include <mwcu_sharedresource.h>

#include <mwcscm_version.h>
// BDE
#include <bslma_sharedptroutofplacerep.h>

namespace BloombergLP {
namespace mwcu {

namespace {

/// Provides a deleter for the shared state.
struct NotifyingDeleter {
    // ACCESSORS
    void operator()(void* sharedResource) const BSLS_KEYWORD_NOEXCEPT
    {
        SharedResource_Base* sharedResourcePtr =
            static_cast<SharedResource_Base*>(sharedResource);

        // PRECONDITIONS
        BSLS_ASSERT(sharedResourcePtr);
        BSLS_ASSERT(sharedResourcePtr->d_deleter_p);
        BSLS_ASSERT(sharedResourcePtr->d_resource_p);

        // free the resource
        sharedResourcePtr->d_deleter_p->deleteObject(
            sharedResourcePtr->d_resource_p);

        // notify the semaphore
        sharedResourcePtr->d_semaphore.post();
    }
};

/// Provides a guard that does cleanup in case `SharedResource_Base::reset`
/// fails with an exception.
struct ResetGuard {
    // DATA
    SharedResource_Base* d_sharedResource_p;

    // CREATORS
    ResetGuard(SharedResource_Base* sharedResource) BSLS_KEYWORD_NOEXCEPT
    : d_sharedResource_p(sharedResource)
    {
        // PRECONDITIONS
        BSLS_ASSERT(sharedResource);
    }

    ~ResetGuard()
    {
        if (d_sharedResource_p) {
            d_sharedResource_p->d_resource_p     = 0;
            d_sharedResource_p->d_sharedPtrRep_p = 0;

            d_sharedResource_p->d_semaphore.wait();
        }
    }

    // MANIPULATORS
    void release() BSLS_KEYWORD_NOEXCEPT { d_sharedResource_p = 0; }
};

}  // close unnamed namespace

// -------------------------
// class SharedResource_Base
// -------------------------

// CREATORS
SharedResource_Base::SharedResource_Base(bdlma::Deleter<void>* deleter,
                                         bslma::Allocator*     allocator)
    BSLS_KEYWORD_NOEXCEPT : d_semaphore(),
                            d_resource_p(0),
                            d_deleter_p(deleter),
                            d_sharedPtrRep_p(0),
                            d_allocator_p(allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT(deleter);
    BSLS_ASSERT(allocator);
}

SharedResource_Base::SharedResource_Base(void*                 resource,
                                         bdlma::Deleter<void>* deleter,
                                         bslma::Allocator*     allocator)
: d_semaphore()
, d_resource_p(resource)
, d_deleter_p(deleter)
, d_sharedPtrRep_p(0)
, d_allocator_p(allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT(resource);
    BSLS_ASSERT(deleter);
    BSLS_ASSERT(allocator);

    // initialize the shared state
    reset(resource);
}

SharedResource_Base::~SharedResource_Base()
{
    // release the shared state, if not already
    reset();
}

// MANIPULATORS
bsl::shared_ptr<void> SharedResource_Base::acquire() BSLS_KEYWORD_NOEXCEPT
{
    // try acquire a strong reference
    if (!d_sharedPtrRep_p || !d_sharedPtrRep_p->tryAcquireRef()) {
        return bsl::nullptr_t();  // RETURN
    }

    // create a shared pointer
    return bsl::shared_ptr<void>(d_resource_p, d_sharedPtrRep_p);
}

void SharedResource_Base::invalidate() BSLS_KEYWORD_NOEXCEPT
{
    // PRECONDITIONS
    BSLS_ASSERT(isValid());

    // release the strong reference we hold and block until all other strong
    // references are released
    d_sharedPtrRep_p->releaseRef();
    d_semaphore.wait();
}

void SharedResource_Base::reset() BSLS_KEYWORD_NOEXCEPT
{
    if (!d_sharedPtrRep_p) {
        return;  // RETURN
    }

    // invalidate the resource, if not already
    if (d_sharedPtrRep_p->numReferences()) {
        invalidate();
    }

    // release the weak reference we hold allowing the shared state to be
    // destroyed
    d_sharedPtrRep_p->releaseWeakRef();

    // reset resource and shared state pointers
    d_resource_p     = 0;
    d_sharedPtrRep_p = 0;
}

void SharedResource_Base::reset(void* resource)
{
    typedef bslma::SharedPtrOutofplaceRep<void, NotifyingDeleter> RepMaker;

    // PRECONDITIONS
    BSLS_ASSERT(resource);
    BSLS_ASSERT(!isValid());

    // do cleanup, if 'makeOutofplaceRep' fails with an exception
    ResetGuard resetGuard(this);

    // reset the resource pointer
    d_resource_p = resource;

    // allocate a new shared state
    d_sharedPtrRep_p = RepMaker::makeOutofplaceRep(this,
                                                   NotifyingDeleter(),
                                                   d_allocator_p);

    // NOTE: If 'makeOutofplaceRep' fails with an exception, the supplied
    //       deleter is invoked.

    // acquire a weak reference to prevent premature destruction of the shared
    // state
    d_sharedPtrRep_p->acquireWeakRef();

    // success
    resetGuard.release();
}

// ACCESSORS
bool SharedResource_Base::isValid() const BSLS_KEYWORD_NOEXCEPT
{
    return d_sharedPtrRep_p && d_sharedPtrRep_p->numReferences();
}

}  // close package namespace
}  // close enterprise namespace
