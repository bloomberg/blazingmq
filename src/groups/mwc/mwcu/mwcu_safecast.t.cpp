// Copyright 2024 Bloomberg Finance L.P.
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

// mwcu_safecast.t.cpp                                                -*-C++-*-
#include <mwcu_safecast.h>

// TEST DRIVER
#include <mwctst_testhelper.h>

// MWC
#include <mwcu_memoutstream.h>

// BDE
#include <balst_stacktraceprintutil.h>
#include <bsls_assert.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace mwcu::SafeCast;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {

template <typename FROM, typename TO>
bool testFail(FROM value)
{
    bool asserted = false;
    try {
        BSLA_MAYBE_UNUSED TO result = safe_cast<TO>(value);
    }
    catch (const bsls::AssertTestException& e) {
        asserted = true;
    }
    return asserted;  // RETURN
}

template <typename FROM, typename TO>
void testTypePair()
{
    // Prepare stack trace to distingush potential test fails
    mwcu::MemOutStream stackTrace(s_allocator_p);
    balst::StackTracePrintUtil::printStackTrace(stackTrace);
    stackTrace << bsl::ends;
    const bslstl::StringRef& desc = stackTrace.str();

    typedef unsigned long long int ULLong;
    typedef long long int          LLong;
    FROM                           maxFrom = bsl::numeric_limits<FROM>::max();
    FROM                           minFrom = bsl::numeric_limits<FROM>::min();
    TO                             maxTo   = bsl::numeric_limits<TO>::max();
    TO                             minTo   = bsl::numeric_limits<TO>::min();

    if (static_cast<ULLong>(maxTo) < static_cast<ULLong>(maxFrom)) {
        FROM value = static_cast<FROM>(maxTo);
        ASSERT_EQ_D(desc, safe_cast<TO>(value), static_cast<TO>(value));
        ASSERT_D(desc, (testFail<FROM, TO>(value + 1)));
        ASSERT_D(desc, (testFail<FROM, TO>(maxFrom)));
    }
    else {
        ASSERT_EQ_D(desc, safe_cast<TO>(maxFrom), static_cast<TO>(maxFrom));
    }
    if (static_cast<LLong>(minFrom) < static_cast<LLong>(minTo)) {
        FROM value = static_cast<FROM>(minTo);
        ASSERT_EQ_D(desc, safe_cast<TO>(value), static_cast<TO>(value));
        ASSERT_D(desc, (testFail<FROM, TO>(value - 1)));
        ASSERT_D(desc, (testFail<FROM, TO>(minFrom)));
    }
    else {
        ASSERT_EQ_D(desc, safe_cast<TO>(minFrom), static_cast<TO>(minFrom));
    }

    ASSERT_EQ_D(desc, static_cast<TO>(0), static_cast<TO>(0));
}

template <typename FROM>
void testType()
{
    testTypePair<FROM, unsigned long long>();
    testTypePair<FROM, unsigned long>();
    testTypePair<FROM, unsigned>();
    testTypePair<FROM, unsigned short>();
    testTypePair<FROM, unsigned char>();
    testTypePair<FROM, long long>();
    testTypePair<FROM, long>();
    testTypePair<FROM, int>();
    testTypePair<FROM, short>();
    testTypePair<FROM, char>();
}

}  // close anonymous namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_breathingTest()
// ------------------------------------------------------------------------
// BREATHING TEST
//
// Concerns:
//   Ensure proper behavior of 'mwcu::SafeCast' in a common use-case.
//
// Plan:
//   Check basic functionality.
//
// Testing:
//   Basic functionality
// ------------------------------------------------------------------------
{
    testType<unsigned long long>();
    testType<unsigned long>();
    testType<unsigned>();
    testType<unsigned short>();
    testType<unsigned char>();
    testType<long long>();
    testType<long>();
    testType<int>();
    testType<short>();
    testType<char>();
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
