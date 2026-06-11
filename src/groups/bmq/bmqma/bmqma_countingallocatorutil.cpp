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
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
bsls::ObjectBuffer<bmqst::StatContext> g_statContext;

/// Main counting allocator; provided to the top allocator store
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
bsls::ObjectBuffer<bmqma::CountingAllocator> g_topAllocator;

/// Top-level allocator store providing `bmqma::CountingAllocator` objects
/// created from the main counting allocator.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
bsls::ObjectBuffer<bmqma::CountingAllocatorStore> g_topAllocatorStore;

/// Atomic flag to keep track of whether `initGlobalAllocators` has been
/// called.  If the value is `false`, then it has not been called, otherwise
/// it has.  Each call to `initGlobalAllocators` atomically tests and sets
/// the value of this flag, triggering an assertion error if it is `true`
/// (i.e. the method was previously called) and setting it to `true`
/// otherwise.
// NOLINTNEXTLINE(cert-err58-cpp,cppcoreguidelines-avoid-non-const-global-variables)
bsls::AtomicBool g_initialized(false);

}  // close unnamed namespace

// ----------------------------
// struct CountingAllocatorUtil
// ----------------------------

// CLASS METHODS
void CountingAllocatorUtil::initGlobalAllocators(
    bsl::string_view             globalStatContextName,
    bsl::string_view             topAllocatorName,
    bsls::Types::Uint64          allocationLimit,
    const bsl::function<void()>& allocationLimitCb)
// NOLINTBEGIN(cppcoreguidelines-pro-type-union-access)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(g_initialized.testAndSwap(false, true) != true);
    // Ensure this is the first invocation of this method and set the
    // 'g_initialized' flag atomically.

    bslma::Allocator* alloc = &bslma::NewDeleteAllocator::singleton();

    // Here we create the main CountingAllocator and its StatContext.
    new (g_statContext.buffer()) bmqst::StatContext(
        bmqst::StatContextConfiguration(globalStatContextName),
        alloc);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
    bmqst::StatContext& stats = g_statContext.object();

    new (g_topAllocator.buffer())
        bmqma::CountingAllocator(topAllocatorName, &stats, alloc);

    // Create the topAllocatorStore and the default and global allocators
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
    bmqma::CountingAllocator& topAllocator = g_topAllocator.object();
    if (allocationLimit > 0) {
        topAllocator.setAllocationLimit(allocationLimit, allocationLimitCb);
    }

    new (g_topAllocatorStore.buffer())
        bmqma::CountingAllocatorStore(&topAllocator);

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
    bmqma::CountingAllocatorStore& topStore = g_topAllocatorStore.object();
    bslma::Default::setGlobalAllocator(topStore.get("Global Allocator"));
    bslma::Default::setDefaultAllocatorRaw(topStore.get("Default Allocator"));
}
// NOLINTEND(cppcoreguidelines-pro-type-union-access)

bmqst::StatContext* CountingAllocatorUtil::globalStatContext()
// NOLINTBEGIN(cppcoreguidelines-pro-type-union-access)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(g_initialized);

    return &g_statContext.object();
}
// NOLINTEND(cppcoreguidelines-pro-type-union-access)

bmqma::CountingAllocatorStore& CountingAllocatorUtil::topAllocatorStore()
// NOLINTBEGIN(cppcoreguidelines-pro-type-union-access)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(g_initialized);

    return g_topAllocatorStore.object();
}
// NOLINTEND(cppcoreguidelines-pro-type-union-access)

void CountingAllocatorUtil::printAllocations(bsl::ostream&             stream,
                                             const bmqst::StatContext& context)
// NOLINTBEGIN(*-magic-numbers)
{
    bdlma::LocalSequentialAllocator<2048> localAllocator;

    bmqst::StatContextTableInfoProvider tip(&localAllocator);
    CountingAllocator::configureStatContextTableInfoProvider(&tip);
    tip.setContext(&context);
    tip.update();

    bmqst::TableUtil::printTable(stream, tip);
}
// NOLINTEND(*-magic-numbers)

}  // close package namespace
}  // close enterprise namespace
