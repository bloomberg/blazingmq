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

// mwcma_countingallocatorstore.cpp                                   -*-C++-*-
#include <mwcma_countingallocatorstore.h>

#include <mwcscm_version.h>
// MWC
#include <mwcma_countingallocator.h>

// BDE
#include <bsl_utility.h>
#include <bslma_default.h>

namespace BloombergLP {
namespace mwcma {

// ----------------------------
// class CountingAllocatorStore
// ----------------------------

// CREATORS
CountingAllocatorStore::CountingAllocatorStore(bslma::Allocator* allocator)
: d_allocators(allocator)
, d_spinLock(bsls::SpinLock::s_unlocked)
, d_allocator_p(bslma::Default::allocator(allocator))
{
    // NOTHING
}

CountingAllocatorStore::~CountingAllocatorStore()
{
    clear();
}

// MANIPULATORS
bslma::Allocator* CountingAllocatorStore::baseAllocator()
{
    return d_allocator_p;
}

bslma::Allocator* CountingAllocatorStore::get(const bsl::string& name)
{
    if (!dynamic_cast<mwcma::CountingAllocator*>(d_allocator_p)) {
        return d_allocator_p;  // RETURN
    }

    bsls::SpinLockGuard guard(&d_spinLock);  // SPINLOCK LOCK

    AllocatorMap::iterator iter = d_allocators.find(name);
    if (iter != d_allocators.end()) {
        // Allocator with 'name' already exists, so simply return it.
        return iter->second;  // RETURN
    }

    // Allocator with this name does not exist, so create one and insert it.
    mwcma::CountingAllocator* newAlloc = new (*d_allocator_p)
        mwcma::CountingAllocator(name, d_allocator_p);

    const bsl::pair<AllocatorMap::iterator, bool> insertRC =
        d_allocators.emplace(name, newAlloc);

    return insertRC.first->second;
}

void CountingAllocatorStore::clear()
{
    bsls::SpinLockGuard guard(&d_spinLock);  // SPINLOCK LOCK
    for (AllocatorMap::iterator iter = d_allocators.begin();
         iter != d_allocators.end();
         ++iter) {
        d_allocator_p->deleteObject(iter->second);
    }

    d_allocators.clear();
}

}  // close package namespace
}  // close enterprise namespace
