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

// bmqvt_rcdescriptionerror.t.cpp -*-C++-*-
#include <bmqvt_rcdescriptionerror.h>

#include <bmqu_memoutstream.h>

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
//   Exercise the basic functionality of the component.
//
// Testing:
//   Basic functionality
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("BREATHING TEST");

    {
        PV("Default constructor");

        bmqvt::RcDescriptionError obj(bmqtst::TestHelperUtil::allocator());
        BMQTST_ASSERT_EQ(obj.rc(), 0);
        BMQTST_ASSERT_EQ(obj.description(), "");
    }

    {
        PV("With value constructor");

        bmqvt::RcDescriptionError obj(123,
                                      "abc",
                                      bmqtst::TestHelperUtil::allocator());
        BMQTST_ASSERT_EQ(obj.rc(), 123);
        BMQTST_ASSERT_EQ(obj.description(), "abc");
    }

    {
        PV("Copy constructor and assignment operator");

        bmqvt::RcDescriptionError obj(123,
                                      "abc",
                                      bmqtst::TestHelperUtil::allocator());

        bmqvt::RcDescriptionError copy(obj,
                                       bmqtst::TestHelperUtil::allocator());
        BMQTST_ASSERT_EQ(copy.rc(), 123);
        BMQTST_ASSERT_EQ(copy.description(), "abc");

        bmqvt::RcDescriptionError assignment(
            987,
            "zyx",
            bmqtst::TestHelperUtil::allocator());
        assignment = obj;
        BMQTST_ASSERT_EQ(assignment.rc(), 123);
        BMQTST_ASSERT_EQ(assignment.description(), "abc");
    }
}

static void test2_equality()
// ------------------------------------------------------------------------
// EQUALITY
//
// Concerns:
//   Verify equality operator.
//
// Testing:
//   operator==
//   operator!=
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("EQUALITY");

    struct Test {
        int         d_line;
        int         d_lrc;
        const char* d_ldescription;
        int         d_rrc;
        const char* d_rdescription;
        bool        d_equal;
    } k_DATA[] = {{L_, 0, "", 0, "", true},
                  {L_, 1, "a", 1, "a", true},
                  {L_, 0, "", 0, "a", false},
                  {L_, 0, "a", 0, "", false},
                  {L_, 0, "", 1, "", false},
                  {L_, 0, "a", 1, "a", false}};

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];

        bmqvt::RcDescriptionError lhs(test.d_lrc,
                                      test.d_ldescription,
                                      bmqtst::TestHelperUtil::allocator());
        bmqvt::RcDescriptionError rhs(test.d_rrc,
                                      test.d_rdescription,
                                      bmqtst::TestHelperUtil::allocator());
        BMQTST_ASSERT_EQ_D("line " << test.d_line, test.d_equal, lhs == rhs);
        BMQTST_ASSERT_NE_D("line " << test.d_line, test.d_equal, lhs != rhs);
    }
}

static void test3_print()
// ------------------------------------------------------------------------
// PRINT
//
// Concerns:
//
//
// Testing:
//   print
//   operator<<
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("PRINT");

    {
        PV("Explicit print");

        bmqvt::RcDescriptionError obj(123,
                                      "abc",
                                      bmqtst::TestHelperUtil::allocator());
        bmqu::MemOutStream        os(bmqtst::TestHelperUtil::allocator());
        bmqvt::RcDescriptionError::print(os, obj, 0, 0);
        BMQTST_ASSERT_EQ(os.str(), "[\nrc = 123\ndescription = \"abc\"\n]\n");
    }

    {
        PV("operator<<");

        bmqvt::RcDescriptionError obj(123,
                                      "abc",
                                      bmqtst::TestHelperUtil::allocator());
        bmqu::MemOutStream        os(bmqtst::TestHelperUtil::allocator());

        os << obj;
        BMQTST_ASSERT_EQ(os.str(), "[ rc = 123 description = \"abc\" ]");
    }

    {
        PV("Invalid stream ('badbit' set)");

        bmqvt::RcDescriptionError obj(123,
                                      "abc",
                                      bmqtst::TestHelperUtil::allocator());
        bmqu::MemOutStream        os(bmqtst::TestHelperUtil::allocator());

        os.setstate(bsl::ios_base::badbit);
        bmqvt::RcDescriptionError::print(os, obj, 0, 0);

        BMQTST_ASSERT_EQ(os.str(), "");
    }
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
    case 2: test2_equality(); break;
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
