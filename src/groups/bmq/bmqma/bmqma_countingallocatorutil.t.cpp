// Copyright 2023 Bloomberg Finance L.P.
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

#include <bmqma_countingallocator.h>
#include <bmqma_countingallocatorstore.h>
#include <bmqst_basictableinfoprovider.h>
#include <bmqst_statcontext.h>
#include <bmqst_statcontexttableinfoprovider.h>
#include <bmqst_statvalue.h>
#include <bmqst_table.h>

// BDE
#include <bslma_default.h>
#include <bslmf_isbitwisemoveable.h>
#include <bsls_timeutil.h>
#include <bsls_types.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>
#include <bsl_vector.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {

struct AllocationLimitCb {
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(AllocationLimitCb, bslmf::IsBitwiseMoveable)

    // DATA
    int* d_triggered_p;

    // CREATORS
    AllocationLimitCb(int* triggered)
    : d_triggered_p(triggered)
    {
        BSLS_ASSERT_OPT(d_triggered_p);
    }

    // MANIPULATORS
    void operator()() { ++(*d_triggered_p); }
};

}  // close unnamed namespace

//=============================================================================
//                              TEST CASES
//-----------------------------------------------------------------------------

static void test1_breathingTest()
// ------------------------------------------------------------------------
// BREATHING TEST
//
// Concerns:
//   Exercise basic functionality before beginning testing in earnest.
//   Probe that functionality to discover basic errors.
//
// Testing:
//   Basic functionality.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("BREATHING TEST");

    // Prior to initialization, calling the accessors is undefined behavior.
    BMQTST_ASSERT_SAFE_FAIL(bmqma::CountingAllocatorUtil::globalStatContext());
    BMQTST_ASSERT_SAFE_FAIL(bmqma::CountingAllocatorUtil::topAllocatorStore());

    bmqma::CountingAllocatorUtil::initGlobalAllocators(
        "testStatContext",
        "testAllocatorName",
        0,
        bsl::function<void()>());

    BMQTST_ASSERT_SAFE_FAIL(bmqma::CountingAllocatorUtil::initGlobalAllocators(
        "testStatContext",
        "testAllocatorName",
        0,
        bsl::function<void()>()));
    BMQTST_ASSERT(bmqma::CountingAllocatorUtil::globalStatContext() != 0);
    BMQTST_ASSERT_EQ(bmqma::CountingAllocatorUtil::globalStatContext()->name(),
                     "testStatContext");
}

static void test2_initGlobalAllocators()
// ------------------------------------------------------------------------
// INITIALIZE GLOBAL ALLOCATORS
//
// Concerns:
//   1. Ensure that 'initGlobalAllocators' creates a top-level allocator,
//      top-level statContext, and top-level
//      'bmqma::CountingAllocatorStore'.
//   2. Ensure that 'initGlobalAllocators' sets the default and global
//      allocators to counting allocators created from the top-level
//      'bmqma::CountingAllocatorStore' and having the names "Default
//       Allocator" and "Global Allocator", respectively.
//
// Testing:
//   initGlobalAllocators
//   globalStatContext
//   topAllocatorStore
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("INITIALIZE GLOBAL ALLOCATORS");

    // 1. Ensure that 'initGlobalAllocators' creates a top-level allocator,
    //    top-level statContext, and top-level 'bmqma::CountingAllocatorStore'.
    bmqma::CountingAllocatorUtil::initGlobalAllocators(
        "testStatContext",
        "testAllocatorName",
        0,
        bsl::function<void()>());

    BMQTST_ASSERT(bmqma::CountingAllocatorUtil::globalStatContext() != 0);
    BMQTST_ASSERT_EQ(bmqma::CountingAllocatorUtil::globalStatContext()->name(),
                     "testStatContext");

    bmqma::CountingAllocatorStore& topAllocatorStore =
        bmqma::CountingAllocatorUtil::topAllocatorStore();

    // 2. Ensure that 'initGlobalAllocators' sets the default and global
    // allocators to counting allocators created from the top-level
    // 'bmqma::CountingAllocatorStore' and having the names "Default Allocator"
    // and "Global Allocator", respectively.
    bslma::Allocator* globalAlloc  = topAllocatorStore.get("Global Allocator");
    bslma::Allocator* defaultAlloc = topAllocatorStore.get(
        "Default Allocator");

    BMQTST_ASSERT(dynamic_cast<bmqma::CountingAllocator*>(globalAlloc) != 0);
    BMQTST_ASSERT(dynamic_cast<bmqma::CountingAllocator*>(defaultAlloc) != 0);

    BMQTST_ASSERT_EQ(globalAlloc, bslma::Default::globalAllocator());
    BMQTST_ASSERT_EQ(defaultAlloc, bslma::Default::defaultAllocator());

    // Check that the corresponding statContexts have been properly registered
    // under the top-level counting allocator for the default and global
    // allocators.
    bmqma::CountingAllocatorUtil::globalStatContext()->snapshot();

    const bmqst::StatContext* topAllocatorStatContext =
        bmqma::CountingAllocatorUtil::globalStatContext()->getSubcontext(
            "testAllocatorName");
    BMQTST_ASSERT(topAllocatorStatContext != 0);

    BMQTST_ASSERT_EQ(topAllocatorStatContext->numSubcontexts(), 2);
    BMQTST_ASSERT_EQ(
        topAllocatorStatContext->getSubcontext("Default Allocator"),
        dynamic_cast<bmqma::CountingAllocator*>(defaultAlloc)->context());
    BMQTST_ASSERT_EQ(
        topAllocatorStatContext->getSubcontext("Global Allocator"),
        dynamic_cast<bmqma::CountingAllocator*>(globalAlloc)->context());
}

static void test3_allocationLimitCb()
// ------------------------------------------------------------------------
// ALLOCATION LIMIT CALLBACK
//
// Concerns:
//   1. The allocation limit callback is not invoked when the total bytes
//      allocated across all counting allocators in the hierarchy remains
//      below the specified limit.
//   2. The allocation limit callback is invoked exactly once when the
//      total bytes allocated exceeds the specified limit.
//   3. Deallocated memory does not count toward the limit.
//   4. Allocations through any nested allocator (global, default, or
//      custom) contribute to the shared limit.
//
// Note:
//   The counting allocator tracks more bytes per allocation than the
//   requested size (alignment rounding and a per-block header), and
//   'initGlobalAllocators' itself performs internal allocations that
//   count toward the limit.  This test uses a large limit and large
//   allocation sizes so that this overhead is negligible.
//
// Testing:
//   AllocationLimitChecker
//   setAllocationLimit
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("ALLOCATION LIMIT CALLBACK");

    int alarmTriggeredCount = 0;

    bmqma::CountingAllocatorUtil::initGlobalAllocators(
        "testStatContext",
        "testAllocatorName",
        4096,
        bsl::function<void()>(bsl::allocator_arg,
                              bmqtst::TestHelperUtil::allocator(),
                              AllocationLimitCb(&alarmTriggeredCount)));

    // Expect no alarm
    //  [........] ~0/4096 bytes
    BMQTST_ASSERT_EQ(alarmTriggeredCount, 0);

    {
        // Expect no alarm: allocated less than the limit
        BSLA_MAYBE_UNUSED bsl::vector<char> globalVec(
            2048,
            bslma::Default::globalAllocator());
        //  [====....] ~2048/4096 bytes
        BMQTST_ASSERT_EQ(alarmTriggeredCount, 0);

        // Free the memory and make sure it doesn't contribute in triggering
        // the alarm early.
    }
    //  [........] ~0/4096 bytes

    // Expect no alarm: allocated less than the limit
    BSLA_MAYBE_UNUSED bsl::vector<char> globalVec(
        2048,
        bslma::Default::globalAllocator());
    //  [====....] ~2048/4096 bytes
    BMQTST_ASSERT_EQ(alarmTriggeredCount, 0);

    // Expect no alarm: allocated less than the limit
    BSLA_MAYBE_UNUSED bsl::vector<char> defaultVec(
        1024,
        bslma::Default::defaultAllocator());
    //  [======..] ~3072/4096 bytes
    BMQTST_ASSERT_EQ(alarmTriggeredCount, 0);

    // Expect 1 alarm: allocated more than the limit
    bslma::Allocator* customAllocator =
        bmqma::CountingAllocatorUtil::topAllocatorStore().get("custom");
    BSLA_MAYBE_UNUSED bsl::vector<char> customVec(2048, customAllocator);
    //  [========]== ~5120/4096 bytes ALARM
    BMQTST_ASSERT_EQ(alarmTriggeredCount, 1);

    // Expect 1 alarm: alarm was already triggered once, no more alarms
    BSLA_MAYBE_UNUSED bsl::vector<char> customVec2(2048, customAllocator);
    //  [========]======= ~7168/4096 bytes
    BMQTST_ASSERT_EQ(alarmTriggeredCount, 1);
}

//=============================================================================
//                              MAIN PROGRAM
//-----------------------------------------------------------------------------

int main(int argc, char** argv)
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 3: test3_allocationLimitCb(); break;
    case 2: test2_initGlobalAllocators(); break;
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
