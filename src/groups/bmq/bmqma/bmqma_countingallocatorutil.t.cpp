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

// bmqma_countingallocatorutil.t.cpp                                  -*-C++-*-
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
#include <bsls_timeutil.h>
#include <bsls_types.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

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
    bmqtst::TestHelper::printTestName("BREATHING TEST");

    // Prior to initialization, calling the accessors is undefined behavior.
    ASSERT_SAFE_FAIL(bmqma::CountingAllocatorUtil::globalStatContext());
    ASSERT_SAFE_FAIL(bmqma::CountingAllocatorUtil::topAllocatorStore());

    bmqma::CountingAllocatorUtil::initGlobalAllocators("testStatContext",
                                                       "testAllocatorName");

    ASSERT_SAFE_FAIL(bmqma::CountingAllocatorUtil::initGlobalAllocators(
        "testStatContext",
        "testAllocatorName"));
    ASSERT(bmqma::CountingAllocatorUtil::globalStatContext() != 0);
    ASSERT_EQ(bmqma::CountingAllocatorUtil::globalStatContext()->name(),
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
    bmqma::CountingAllocatorUtil::initGlobalAllocators("testStatContext",
                                                       "testAllocatorName");

    ASSERT(bmqma::CountingAllocatorUtil::globalStatContext() != 0);
    ASSERT_EQ(bmqma::CountingAllocatorUtil::globalStatContext()->name(),
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

    ASSERT(dynamic_cast<bmqma::CountingAllocator*>(globalAlloc) != 0);
    ASSERT(dynamic_cast<bmqma::CountingAllocator*>(defaultAlloc) != 0);

    ASSERT_EQ(globalAlloc, bslma::Default::globalAllocator());
    ASSERT_EQ(defaultAlloc, bslma::Default::defaultAllocator());

    // Check that the corresponding statContexts have been properly registered
    // under the top-level counting allocator for the default and global
    // allocators.
    bmqma::CountingAllocatorUtil::globalStatContext()->snapshot();

    const bmqst::StatContext* topAllocatorStatContext =
        bmqma::CountingAllocatorUtil::globalStatContext()->getSubcontext(
            "testAllocatorName");
    ASSERT(topAllocatorStatContext != 0);

    ASSERT_EQ(topAllocatorStatContext->numSubcontexts(), 2);
    ASSERT_EQ(
        topAllocatorStatContext->getSubcontext("Default Allocator"),
        dynamic_cast<bmqma::CountingAllocator*>(defaultAlloc)->context());
    ASSERT_EQ(topAllocatorStatContext->getSubcontext("Global Allocator"),
              dynamic_cast<bmqma::CountingAllocator*>(globalAlloc)->context());
}

//=============================================================================
//                              MAIN PROGRAM
//-----------------------------------------------------------------------------

int main(int argc, char** argv)
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 2: test2_initGlobalAllocators(); break;
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        s_testStatus = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
