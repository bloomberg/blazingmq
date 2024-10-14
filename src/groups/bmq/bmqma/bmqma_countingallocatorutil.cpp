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

// bmqma_countingallocatorutil.cpp                                    -*-C++-*-
#include <bmqma_countingallocatorutil.h>

#include <bmqscm_version.h>

#include <bmqma_countingallocator.h>
#include <bmqma_countingallocatorstore.h>
#include <bmqst_statcontext.h>
#include <bmqst_statcontexttableinfoprovider.h>
#include <bmqst_tableutil.h>

// BDE
#include <bdlma_localsequentialallocator.h>
#include <bsl_iostream.h>
#include <bslma_default.h>
#include <bslma_newdeleteallocator.h>
#include <bslmt_once.h>
#include <bsls_assert.h>
#include <bsls_atomic.h>
#include <bsls_objectbuffer.h>

namespace BloombergLP {
namespace bmqma {

namespace {

/// statContext of the main counting allocator
bsls::ObjectBuffer<bmqst::StatContext> g_statContext;

/// Main counting allocator; provided to the top allocator store
bsls::ObjectBuffer<bmqma::CountingAllocator> g_topAllocator;

/// Top-level allocator store providing `bmqma::CountingAllocator` objects
/// created from the main counting allocator.
bsls::ObjectBuffer<bmqma::CountingAllocatorStore> g_topAllocatorStore;

/// Atomic flag to keep track of whether `initGlobalAllocators` has been
/// called.  If the value is `false`, then it has not been called, otherwise
/// it has.  Each call to `initGlobalAllocators` atomically tests and sets
/// the value of this flag, triggering an assertion error if it is `true`
/// (i.e. the method was previously called) and setting it to `true`
/// otherwise.
bsls::AtomicBool g_initialized(false);

}  // close unnamed namespace

// ----------------------------
// struct CountingAllocatorUtil
// ----------------------------

// CLASS METHODS
void CountingAllocatorUtil::initGlobalAllocators(
    const bmqst::StatContextConfiguration& globalStatContextConfiguration,
    const bslstl::StringRef&               topAllocatorName)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(g_initialized.testAndSwap(false, true) != true);
    // Ensure this is the first invocation of this method and set the
    // 'g_initialized' flag atomically.

    bslma::Allocator* alloc = &bslma::NewDeleteAllocator::singleton();

    // Here we create the main CountingAllocator and its StatContext.
    new (g_statContext.buffer())
        bmqst::StatContext(globalStatContextConfiguration, alloc);
    bmqst::StatContext& stats = g_statContext.object();

    new (g_topAllocator.buffer())
        bmqma::CountingAllocator(topAllocatorName, &stats, alloc);

    // Create the topAllocatorStore and the default and global allocators
    bmqma::CountingAllocator& topAllocator = g_topAllocator.object();
    new (g_topAllocatorStore.buffer())
        bmqma::CountingAllocatorStore(&topAllocator);

    bmqma::CountingAllocatorStore& topStore = g_topAllocatorStore.object();
    bslma::Default::setGlobalAllocator(topStore.get("Global Allocator"));
    bslma::Default::setDefaultAllocatorRaw(topStore.get("Default Allocator"));
}

void CountingAllocatorUtil::initGlobalAllocators(
    const bslstl::StringRef& globalStatContextName,
    const bslstl::StringRef& topAllocatorName)
{
    initGlobalAllocators(
        bmqst::StatContextConfiguration(globalStatContextName),
        topAllocatorName);
}

bmqst::StatContext* CountingAllocatorUtil::globalStatContext()
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(g_initialized);

    return &g_statContext.object();
}

bmqma::CountingAllocatorStore& CountingAllocatorUtil::topAllocatorStore()
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(g_initialized);

    return g_topAllocatorStore.object();
}

void CountingAllocatorUtil::printAllocations(bsl::ostream&             stream,
                                             const bmqst::StatContext& context)
{
    bdlma::LocalSequentialAllocator<2048> localAllocator;

    bmqst::StatContextTableInfoProvider tip(&localAllocator);
    CountingAllocator::configureStatContextTableInfoProvider(&tip);
    tip.setContext(&context);
    tip.update();

    bmqst::TableUtil::printTable(stream, tip);
}

}  // close package namespace
}  // close enterprise namespace
