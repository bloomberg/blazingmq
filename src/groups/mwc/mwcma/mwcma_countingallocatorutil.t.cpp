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

// mwcma_countingallocatorutil.t.cpp                                  -*-C++-*-
#include <mwcma_countingallocatorutil.h>

// MWC
#include <mwcma_countingallocator.h>
#include <mwcma_countingallocatorstore.h>
#include <mwcst_basictableinfoprovider.h>
#include <mwcst_statcontext.h>
#include <mwcst_statcontexttableinfoprovider.h>
#include <mwcst_statvalue.h>
#include <mwcst_table.h>

// BDE
#include <bslma_default.h>
#include <bsls_timeutil.h>
#include <bsls_types.h>

// TEST DRIVER
#include <mwctst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {
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
    mwctst::TestHelper::printTestName("BREATHING TEST");

    // Prior to initialization, calling the accessors is undefined behavior.
    ASSERT_SAFE_FAIL(mwcma::CountingAllocatorUtil::globalStatContext());
    ASSERT_SAFE_FAIL(mwcma::CountingAllocatorUtil::topAllocatorStore());

    mwcma::CountingAllocatorUtil::initGlobalAllocators("testStatContext",
                                                       "testAllocatorName");

    ASSERT_SAFE_FAIL(mwcma::CountingAllocatorUtil::initGlobalAllocators(
        "testStatContext",
        "testAllocatorName"));
    ASSERT(mwcma::CountingAllocatorUtil::globalStatContext() != 0);
    ASSERT_EQ(mwcma::CountingAllocatorUtil::globalStatContext()->name(),
              "testStatContext");
}

static void test2_initGlobalAllocators()
// ------------------------------------------------------------------------
// INITIALIZE GLOBAL ALLOCATORS
//
// Concerns:
//   1. Ensure that 'initGlobalAllocators' creates a top-level allocator,
//      top-level statContext, and top-level
//      'mwcma::CountingAllocatorStore'.
//   2. Ensure that 'initGlobalAllocators' sets the default and global
//      allocators to counting allocators created from the top-level
//      'mwcma::CountingAllocatorStore' and having the names "Default
//       Allocator" and "Global Allocator", respectively.
//
// Testing:
//   initGlobalAllocators
//   globalStatContext
//   topAllocatorStore
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("INITIALIZE GLOBAL ALLOCATORS");

    // 1. Ensure that 'initGlobalAllocators' creates a top-level allocator,
    //    top-level statContext, and top-level 'mwcma::CountingAllocatorStore'.
    mwcma::CountingAllocatorUtil::initGlobalAllocators("testStatContext",
                                                       "testAllocatorName");

    ASSERT(mwcma::CountingAllocatorUtil::globalStatContext() != 0);
    ASSERT_EQ(mwcma::CountingAllocatorUtil::globalStatContext()->name(),
              "testStatContext");

    mwcma::CountingAllocatorStore& topAllocatorStore =
        mwcma::CountingAllocatorUtil::topAllocatorStore();

    // 2. Ensure that 'initGlobalAllocators' sets the default and global
    // allocators to counting allocators created from the top-level
    // 'mwcma::CountingAllocatorStore' and having the names "Default Allocator"
    // and "Global Allocator", respectively.
    bslma::Allocator* globalAlloc  = topAllocatorStore.get("Global Allocator");
    bslma::Allocator* defaultAlloc = topAllocatorStore.get(
        "Default Allocator");

    ASSERT(dynamic_cast<mwcma::CountingAllocator*>(globalAlloc) != 0);
    ASSERT(dynamic_cast<mwcma::CountingAllocator*>(defaultAlloc) != 0);

    ASSERT_EQ(globalAlloc, bslma::Default::globalAllocator());
    ASSERT_EQ(defaultAlloc, bslma::Default::defaultAllocator());

    // Check that the corresponding statContexts have been properly registered
    // under the top-level counting allocator for the default and global
    // allocators.
    mwcma::CountingAllocatorUtil::globalStatContext()->snapshot();

    const mwcst::StatContext* topAllocatorStatContext =
        mwcma::CountingAllocatorUtil::globalStatContext()->getSubcontext(
            "testAllocatorName");
    ASSERT(topAllocatorStatContext != 0);

    ASSERT_EQ(topAllocatorStatContext->numSubcontexts(), 2);
    ASSERT_EQ(
        topAllocatorStatContext->getSubcontext("Default Allocator"),
        dynamic_cast<mwcma::CountingAllocator*>(defaultAlloc)->context());
    ASSERT_EQ(topAllocatorStatContext->getSubcontext("Global Allocator"),
              dynamic_cast<mwcma::CountingAllocator*>(globalAlloc)->context());
}

//=============================================================================
//                              MAIN PROGRAM
//-----------------------------------------------------------------------------

int main(int argc, char** argv)
{
    TEST_PROLOG(mwctst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 2: test2_initGlobalAllocators(); break;
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        s_testStatus = -1;
    } break;
    }

    TEST_EPILOG(mwctst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
