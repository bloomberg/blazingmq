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

// mwcma_countingallocatorutil.cpp                                    -*-C++-*-
#include <mwcma_countingallocatorutil.h>

#include <mwcscm_version.h>
// MWC
#include <mwcma_countingallocator.h>
#include <mwcma_countingallocatorstore.h>
#include <mwcst_statcontext.h>
#include <mwcst_statcontexttableinfoprovider.h>
#include <mwcst_tableutil.h>

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
namespace mwcma {

namespace {

/// statContext of the main counting allocator
bsls::ObjectBuffer<mwcst::StatContext> g_statContext;

/// Main counting allocator; provided to the top allocator store
bsls::ObjectBuffer<mwcma::CountingAllocator> g_topAllocator;

/// Top-level allocator store providing `mwcma::CountingAllocator` objects
/// created from the main counting allocator.
bsls::ObjectBuffer<mwcma::CountingAllocatorStore> g_topAllocatorStore;

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
    const mwcst::StatContextConfiguration& globalStatContextConfiguration,
    const bslstl::StringRef&               topAllocatorName)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(g_initialized.testAndSwap(false, true) != true);
    // Ensure this is the first invocation of this method and set the
    // 'g_initialized' flag atomically.

    bslma::Allocator* alloc = &bslma::NewDeleteAllocator::singleton();

    // Here we create the main CountingAllocator and its StatContext.
    new (g_statContext.buffer())
        mwcst::StatContext(globalStatContextConfiguration, alloc);
    mwcst::StatContext& stats = g_statContext.object();

    new (g_topAllocator.buffer())
        mwcma::CountingAllocator(topAllocatorName, &stats, alloc);

    // Create the topAllocatorStore and the default and global allocators
    mwcma::CountingAllocator& topAllocator = g_topAllocator.object();
    new (g_topAllocatorStore.buffer())
        mwcma::CountingAllocatorStore(&topAllocator);

    mwcma::CountingAllocatorStore& topStore = g_topAllocatorStore.object();
    bslma::Default::setGlobalAllocator(topStore.get("Global Allocator"));
    bslma::Default::setDefaultAllocatorRaw(topStore.get("Default Allocator"));
}

void CountingAllocatorUtil::initGlobalAllocators(
    const bslstl::StringRef& globalStatContextName,
    const bslstl::StringRef& topAllocatorName)
{
    initGlobalAllocators(
        mwcst::StatContextConfiguration(globalStatContextName),
        topAllocatorName);
}

mwcst::StatContext* CountingAllocatorUtil::globalStatContext()
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(g_initialized);

    return &g_statContext.object();
}

mwcma::CountingAllocatorStore& CountingAllocatorUtil::topAllocatorStore()
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(g_initialized);

    return g_topAllocatorStore.object();
}

void CountingAllocatorUtil::printAllocations(bsl::ostream&             stream,
                                             const mwcst::StatContext& context)
{
    bdlma::LocalSequentialAllocator<2048> localAllocator;

    mwcst::StatContextTableInfoProvider tip(&localAllocator);
    CountingAllocator::configureStatContextTableInfoProvider(&tip);
    tip.setContext(&context);
    tip.update();

    mwcst::TableUtil::printTable(stream, tip);
}

}  // close package namespace
}  // close enterprise namespace
