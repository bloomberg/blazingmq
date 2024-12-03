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

// bmqtst_testhelper.cpp                                              -*-C++-*-
#include <bmqtst_testhelper.h>

#include <bmqscm_version.h>
// BDE
#include <bsl_cmath.h>
#include <bsl_limits.h>

namespace BloombergLP {
namespace bmqtst {

namespace {

// ============================================================================
//                              GLOBAL VARIABLES
// ----------------------------------------------------------------------------
int               s_testStatus          = 0;
int               s_verbosityLevel      = 0;
bool              s_ignoreCheckDefAlloc = false;
bool              s_ignoreCheckGblAlloc = false;
bslmt::QLock      s_serializePrintLock  = BSLMT_QLOCK_INITIALIZER;
bslma::Allocator* s_allocator_p         = 0;

}

int& bmqtst::TestHelperUtil::testStatus()
{
    return s_testStatus;
}
int& bmqtst::TestHelperUtil::verbosityLevel()
{
    return s_verbosityLevel;
}
bool& bmqtst::TestHelperUtil::ignoreCheckDefAlloc()
{
    return s_ignoreCheckDefAlloc;
}
bool& bmqtst::TestHelperUtil::ignoreCheckGblAlloc()
{
    return s_ignoreCheckGblAlloc;
}
bslmt::QLock& bmqtst::TestHelperUtil::serializePrintLock()
{
    return s_serializePrintLock;
}
bslma::Allocator*& bmqtst::TestHelperUtil::allocator()
{
    return s_allocator_p;
}

// -----------------
// struct TestHelper
// -----------------

// CLASS METHODS
void TestHelper::printTestName(bslstl::StringRef value)
{
    if (bmqtst::TestHelperUtil::verbosityLevel() < 1) {
        return;  // RETURN
    }

    bsl::cout << "\n" << value << "\n";
    size_t length = value.length();
    while (length--) {
        bsl::cout << "=";
    }
    bsl::cout << "\n";
}

bool TestHelper::areFuzzyEqual(double x, double y)
{
    return bsl::fabs(x - y) < bsl::numeric_limits<double>::epsilon();
}

// -----------
// struct Test
// -----------

// CREATORS
Test::~Test()
{
    // NOTHING
}

// MANIPULATORS
void Test::SetUp()
{
    // NOTHING
}

void Test::TearDown()
{
    // NOTHING
}

// ----------------------
// struct TestHelper_Test
// ----------------------

// CLASS DATA
TestHelper_Test::TestFn
    TestHelper_Test::s_tests[TestHelper_Test::k_MAX_TESTS] = {};

int TestHelper_Test::s_numTests = 0;

// CREATORS
TestHelper_Test::TestHelper_Test(const TestFn& test)
{
    // PRECONDITIONS
    BSLS_ASSERT(s_numTests < k_MAX_TESTS);

    // add test
    s_tests[s_numTests++] = test;
}

// --------------
// Free Functions
// --------------
void runTest(int index)
{
    const int testCase = index - 1;

    if (testCase < 0 || testCase >= TestHelper_Test::s_numTests) {
        bsl::cerr << "WARNING: CASE '" << testCase << "' NOT FOUND.\n";
        bmqtst::TestHelperUtil::testStatus() = -1;

        return;  // RETURN
    }

    // Execute the test
    TestHelper_Test::s_tests[testCase]();
}

}  // close package namespace
}  // close enterprise namespace
