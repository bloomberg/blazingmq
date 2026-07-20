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

#include <bmqa_startstatus.h>

// BMQ
#include <bmqt_resultcode.h>

#include <bmqu_memoutstream.h>

// BDE
#include <bsl_memory.h>
#include <bsl_string.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

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

    PV("Default Constructor");
    {
        bmqa::StartStatus obj(bmqtst::TestHelperUtil::allocator());
        BMQTST_ASSERT_EQ(bool(obj), true);
        BMQTST_ASSERT_EQ(obj.result(), bmqt::GenericResult::e_SUCCESS);
        BMQTST_ASSERT_EQ(obj.errorDescription(),
                         bsl::string("", bmqtst::TestHelperUtil::allocator()));
    }

    PV("Valued Constructor");
    {
        const bmqt::GenericResult::Enum result =
            bmqt::GenericResult::e_TIMEOUT;
        const bsl::string errorDescription =
            bsl::string("ERROR", bmqtst::TestHelperUtil::allocator());

        bmqa::StartStatus obj(result,
                              errorDescription,
                              bmqtst::TestHelperUtil::allocator());

        BMQTST_ASSERT_EQ(bool(obj), false);
        BMQTST_ASSERT_EQ(obj.result(), result);
        BMQTST_ASSERT_EQ(obj.errorDescription(), errorDescription);
    }

    PV("Copy Constructor");
    {
        const bmqt::GenericResult::Enum result =
            bmqt::GenericResult::e_TIMEOUT;
        const bsl::string errorDescription =
            bsl::string("ERROR", bmqtst::TestHelperUtil::allocator());

        bmqa::StartStatus obj1(result,
                               errorDescription,
                               bmqtst::TestHelperUtil::allocator());
        bmqa::StartStatus obj2(obj1, bmqtst::TestHelperUtil::allocator());

        BMQTST_ASSERT_EQ(bool(obj2), bool(obj1));
        BMQTST_ASSERT_EQ(obj1.result(), obj2.result());
        BMQTST_ASSERT_EQ(obj1.errorDescription(), obj2.errorDescription());
    }

    PV("Assignment Operator");
    {
        const bmqt::GenericResult::Enum result =
            bmqt::GenericResult::e_TIMEOUT;
        const bsl::string errorDescription =
            bsl::string("ERROR", bmqtst::TestHelperUtil::allocator());

        bmqa::StartStatus obj1(result,
                               errorDescription,
                               bmqtst::TestHelperUtil::allocator());
        bmqa::StartStatus obj2(bmqtst::TestHelperUtil::allocator());
        obj2 = obj1;

        BMQTST_ASSERT_EQ(bool(obj1), bool(obj2));
        BMQTST_ASSERT_EQ(obj1.result(), obj2.result());
        BMQTST_ASSERT_EQ(obj1.errorDescription(), obj2.errorDescription());
    }
}

static void test2_comparison()
// ------------------------------------------------------------------------
// COMPARISON
//
// Concerns:
//   Exercise 'bmqa::StartStatus' comparison operators
//
// Testing:
//   bool operator==(const bmqa::StartStatus& lhs,
//                   const bmqa::StartStatus& rhs);
//   bool operator!=(const bmqa::StartStatus& lhs,
//                   const bmqa::StartStatus& rhs);
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("COMPARISON");

    PV("Equality");
    {
        const bmqt::GenericResult::Enum result =
            bmqt::GenericResult::e_TIMEOUT;
        const bsl::string errorDescription =
            bsl::string("ERROR", bmqtst::TestHelperUtil::allocator());

        bmqa::StartStatus obj1(result,
                               errorDescription,
                               bmqtst::TestHelperUtil::allocator());
        bmqa::StartStatus obj2(obj1, bmqtst::TestHelperUtil::allocator());

        BMQTST_ASSERT(obj1 == obj2);
    }

    PV("Inequality");
    {
        const bsl::string errorDescription =
            bsl::string("ERROR", bmqtst::TestHelperUtil::allocator());

        bmqa::StartStatus obj1(bmqt::GenericResult::e_SUCCESS,
                               errorDescription,
                               bmqtst::TestHelperUtil::allocator());
        bmqa::StartStatus obj2(bmqt::GenericResult::e_TIMEOUT,
                               errorDescription,
                               bmqtst::TestHelperUtil::allocator());

        BMQTST_ASSERT(obj1 != obj2);
    }
}

static void test3_print()
// ------------------------------------------------------------------------
// PRINT
//
// Concerns:
//   Proper behavior of printing 'bmqa::StartStatus'.
//
// Testing:
//   StartStatus::print()
//   bmqa::operator<<(bsl::ostream& stream, const bmqa::StartStatus& rhs);
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // Can't check the default allocator: 'bmqa::StartStatus::print' and
    // operator '<<' temporarily allocate a string using the default allocator.

    bmqtst::TestHelper::printTestName("PRINT");

    const bmqt::GenericResult::Enum result = bmqt::GenericResult::e_SUCCESS;
    const bsl::string               errorDescription =
        bsl::string("ERROR", bmqtst::TestHelperUtil::allocator());

    bmqa::StartStatus obj(result,
                          errorDescription,
                          bmqtst::TestHelperUtil::allocator());

    PVV(obj);
    const char* expected = "[ result = \"SUCCESS (0)\""
                           " errorDescription = \"ERROR\" ]";

    bmqu::MemOutStream out(bmqtst::TestHelperUtil::allocator());
    // operator<<
    out << obj;

    BMQTST_ASSERT_EQ(out.str(), expected);

    // Print
    out.reset();
    obj.print(out, 0, -1);

    BMQTST_ASSERT_EQ(out.str(), expected);
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 3: test3_print(); break;
    case 2: test2_comparison(); break;
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
