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

// bmqma_countingallocatorstore.t.cpp                                 -*-C++-*-
#include <bmqma_countingallocatorstore.h>

#include <bmqma_countingallocator.h>
#include <bmqst_statcontext.h>
#include <bmqst_statcontexttableinfoprovider.h>
#include <bmqst_tableutil.h>

// BDE
#include <bsl_iostream.h>
#include <bsl_sstream.h>
#include <bsl_vector.h>
#include <bslma_allocator.h>
#include <bsls_systemtime.h>
#include <bsls_timeinterval.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

using namespace BloombergLP;
using namespace bsl;

//=============================================================================
//                             TEST PLAN
//-----------------------------------------------------------------------------
// The component under test provides a container and factory for counting
// allocators.  This test only contains a usage example.
//
//-----------------------------------------------------------------------------
// [1] Breathing test
// [2]
//-----------------------------------------------------------------------------

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
// Plan:
//   1. Supply a 'bmqma::CountingAllocator' ancestor allocator and retrieve
//      multiple allocators from the allocator store using the 'get'
//      method, verifying that the returned allocators distinct if they
//      have different names and the same if they have the same name.
//
// Testing:
//   Basic functionality.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("BREATHING TEST");

    bmqma::CountingAllocator      topAllocator("Top Allocator",
                                          bmqtst::TestHelperUtil::allocator());
    bmqma::CountingAllocatorStore allocatorStore(&topAllocator);

    BMQTST_ASSERT_EQ(allocatorStore.baseAllocator(), &topAllocator);

    bslma::Allocator* allocatorA = allocatorStore.get("a");
    bslma::Allocator* allocatorB = allocatorStore.get("b");
    bslma::Allocator* allocatorC = allocatorStore.get("a");

    BMQTST_ASSERT_EQ(allocatorA, allocatorC);
    BMQTST_ASSERT_NE(allocatorA, allocatorB);
}

static void test2_ancestorNotCountingAllocator()
// ------------------------------------------------------------------------
// ANCESTOR NOT COUNTING ALLOCATOR
//
// Concerns:
//   If the allocator provided at construction is not a
//   'bmqma::CountingAllocator' then calls to 'get' will simply return that
//   allocator.
//
// Plan:
//   1. Supply an ancestor allocator that is not a
//      'bmqma::CountingAllocator' and retrieve multiple allocators from
//      the allocator store using the 'get' method, verifying that the
//      returned allocators are the ancestor allocator provided at
//      construction.
//
// Testing:
//   CountingAllocatorStore
//   get
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("ANCESTOR NOT COUNTING ALLOCATOR");

    BSLS_ASSERT_OPT(!dynamic_cast<bmqma::CountingAllocator*>(
                        bmqtst::TestHelperUtil::allocator()) &&
                    "Test allocator is assumed to not be a"
                    " 'bmqma::CountingAllocator'");

    bslma::Allocator* topAllocator_p = bmqtst::TestHelperUtil::allocator();
    bmqma::CountingAllocatorStore allocatorStore(topAllocator_p);

    BMQTST_ASSERT_EQ(allocatorStore.baseAllocator(), topAllocator_p);

    bslma::Allocator* allocatorA = allocatorStore.get("a");
    bslma::Allocator* allocatorB = allocatorStore.get("b");

    BMQTST_ASSERT_EQ(allocatorA, topAllocator_p);
    BMQTST_ASSERT_EQ(allocatorB, topAllocator_p);
}

//=============================================================================
//                              MAIN PROGRAM
//-----------------------------------------------------------------------------

int main(int argc, char** argv)
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 2: test2_ancestorNotCountingAllocator(); break;
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
