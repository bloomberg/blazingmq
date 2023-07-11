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

// mwcu_samethreadchecker.t.cpp                                       -*-C++-*-
#include <mwcu_samethreadchecker.h>

// TEST DRIVER
#include <mwctst_testhelper.h>

// BDE
#include <bdlf_memfn.h>
#include <bslmt_threadutil.h>
#include <bsls_assert.h>

// CONVENIENCE
using namespace BloombergLP;

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_breathingTest()
// ------------------------------------------------------------------------
// BREATHING TEST
//
// Concerns:
//   Ensure proper behavior of 'mwcu::SameThreadChecker' in a common
//   use-case.
//
// Plan:
//   Check basic functionality.
//
// Testing:
//   Basic functionality
// ------------------------------------------------------------------------
{
    mwcu::SameThreadChecker sameThreadChecker;

    // set the checker's thread to be this thread
    ASSERT_EQ(sameThreadChecker.inSameThread(), true);

    // we are in the checker's thread
    ASSERT_EQ(sameThreadChecker.inSameThread(), true);

    // reset the checker's thread to be another thread
    sameThreadChecker.reset();

    bslmt::ThreadUtil::Handle thread;
    int                       rc = bslmt::ThreadUtil::createWithAllocator(
        &thread,
        bdlf::MemFnUtil::memFn(&mwcu::SameThreadChecker::inSameThread,
                               &sameThreadChecker),
        s_allocator_p);
    BSLS_ASSERT_OPT(rc == 0);

    rc = bslmt::ThreadUtil::join(thread);
    BSLS_ASSERT_OPT(rc == 0);

    // we are not in the checker's thread
    ASSERT_EQ(sameThreadChecker.inSameThread(), false);
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(mwctst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 1: test1_breathingTest(); break;

    default: {
        bsl::cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND."
                  << bsl::endl;
        s_testStatus = -1;
    } break;
    }

    TEST_EPILOG(mwctst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
