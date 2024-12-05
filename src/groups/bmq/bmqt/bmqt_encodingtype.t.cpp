// Copyright 2014-2023 Bloomberg Finance L.P.
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

// bmqt_encodingtype.t.cpp                                            -*-C++-*-
#include <bmqt_encodingtype.h>

#include <bmqu_memoutstream.h>

// BDE
#include <bsl_ios.h>
#include <bsl_string.h>
#include <bslmf_assert.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_toAscii()
{
    bmqtst::TestHelper::printTestName("TO ASCII");

    struct Test {
        int         d_line;
        int         d_value;
        const char* d_expected;
    } k_DATA[] = {{L_, 1, "RAW"},
                  {L_, 2, "BER"},
                  {L_, 3, "BDEX"},
                  {L_, 4, "XML"},
                  {L_, 5, "JSON"},
                  {L_, 6, "TEXT"},
                  {L_, 7, "MULTIPARTS"},
                  {L_, -1, "(* UNKNOWN *)"}};

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];

        bsl::string ascii(bmqtst::TestHelperUtil::allocator());
        ascii = bmqt::EncodingType::toAscii(
            bmqt::EncodingType::Enum(test.d_value));
        ASSERT_EQ_D(test.d_line, ascii, test.d_expected);
    }
}

static void test2_fromAscii()
{
    bmqtst::TestHelper::printTestName("FROM ASCII");

    struct Test {
        int         d_line;
        const char* d_input;
        bool        d_isValid;
        int         d_expected;
    } k_DATA[] = {{L_, "RAW", true, 1},
                  {L_, "BER", true, 2},
                  {L_, "BDEX", true, 3},
                  {L_, "XML", true, 4},
                  {L_, "JSON", true, 5},
                  {L_, "TEXT", true, 6},
                  {L_, "MULTIPARTS", true, 7},
                  {L_, "invalid", false, -1}};
    // NOTE: Using the 'integer' value instead of the enum to ensure the
    //       numeric values are *never* changed.

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];

        bmqu::MemOutStream errorDescription(
            bmqtst::TestHelperUtil::allocator());

        bsl::string str(test.d_input, bmqtst::TestHelperUtil::allocator());
        bool isValid = bmqt::EncodingType::isValid(&str, errorDescription);
        ASSERT_EQ_D(test.d_line, isValid, test.d_isValid);

        bmqt::EncodingType::Enum obj;
        isValid = bmqt::EncodingType::fromAscii(&obj, test.d_input);
        ASSERT_EQ_D(test.d_line, isValid, test.d_isValid);
        if (isValid) {
            ASSERT_EQ_D(test.d_line, obj, test.d_expected);
        }
    }
}

static void test3_isomorphism()
{
    bmqtst::TestHelper::printTestName("ISOMORPHISM");

    bmqt::EncodingType::Enum obj;
    bsl::string              str(bmqtst::TestHelperUtil::allocator());
    bool                     res;

    PV("Testing: fromAscii(toAscii(value)) = value");
    res = bmqt::EncodingType::fromAscii(
        &obj,
        bmqt::EncodingType::toAscii(bmqt::EncodingType::e_TEXT));
    ASSERT_EQ(res, true);
    ASSERT_EQ(obj, bmqt::EncodingType::e_TEXT);

    PV("Testing: toAscii(fromAscii(value)) = value");
    res = bmqt::EncodingType::fromAscii(&obj, "MULTIPARTS");
    ASSERT_EQ(res, true);
    str = bmqt::EncodingType::toAscii(obj);
    ASSERT_EQ(str, "MULTIPARTS");
}

static void test4_printTest()
{
    bmqtst::TestHelper::printTestName("PRINT");

    PV("Testing print");

    BSLMF_ASSERT(bmqt::EncodingType::k_LOWEST_SUPPORTED_ENCODING_TYPE ==
                 bmqt::EncodingType::e_RAW);

    BSLMF_ASSERT(bmqt::EncodingType::k_HIGHEST_SUPPORTED_ENCODING_TYPE ==
                 bmqt::EncodingType::e_MULTIPARTS);

    struct Test {
        bmqt::EncodingType::Enum d_type;
        const char*              d_expected;
    } k_DATA[] = {{bmqt::EncodingType::e_UNDEFINED, "UNDEFINED"},
                  {bmqt::EncodingType::e_RAW, "RAW"},
                  {bmqt::EncodingType::e_BER, "BER"},
                  {bmqt::EncodingType::e_BDEX, "BDEX"},
                  {bmqt::EncodingType::e_XML, "XML"},
                  {bmqt::EncodingType::e_JSON, "JSON"},
                  {bmqt::EncodingType::e_TEXT, "TEXT"},
                  {bmqt::EncodingType::e_MULTIPARTS, "MULTIPARTS"},
                  {static_cast<bmqt::EncodingType::Enum>(-1),
                   "(* UNKNOWN *)"}};

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test&        test = k_DATA[idx];
        bmqu::MemOutStream out(bmqtst::TestHelperUtil::allocator());
        bmqu::MemOutStream expected(bmqtst::TestHelperUtil::allocator());

        expected << test.d_expected;

        out.setstate(bsl::ios_base::badbit);
        bmqt::EncodingType::print(out, test.d_type, 0, -1);

        ASSERT_EQ(out.str(), "");

        out.clear();
        bmqt::EncodingType::print(out, test.d_type, 0, -1);

        ASSERT_EQ(out.str(), expected.str());

        out.reset();
        out << test.d_type;

        ASSERT_EQ(out.str(), expected.str());
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
    case 4: test4_printTest(); break;
    case 3: test3_isomorphism(); break;
    case 2: test2_fromAscii(); break;
    case 1: test1_toAscii(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
