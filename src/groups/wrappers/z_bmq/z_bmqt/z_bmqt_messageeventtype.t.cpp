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

// z_bmqt_messageeventtype.t.cpp -*-C++-*-
#include <bmqt_messageeventtype.h>
#include <z_bmqt_messageeventtype.h>

// MWC
#include <mwcu_memoutstream.h>

// BDE
#include <bsl_ios.h>
#include <bsl_string.h>
#include <bslmf_assert.h>

// TEST DRIVER
#include <mwctst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_breathingTest()
{
    mwctst::TestHelper::printTestName("BREATHING TEST");

    z_bmqt_MessageEventType::Enum obj;
    bsl::string                   str;
    bool                          res;

    PV("Testing toAscii");
    str = z_bmqt_MessageEventType::toAscii(z_bmqt_MessageEventType::ec_PUSH);
    ASSERT_EQ(str, "PUSH");

    obj = static_cast<z_bmqt_MessageEventType::Enum>(-1);
    str = z_bmqt_MessageEventType::toAscii(obj);
    ASSERT_EQ(str, "(* UNKNOWN *)");

    PV("Testing fromAscii");
    res = z_bmqt_MessageEventType::fromAscii(&obj, "PUT");
    ASSERT_EQ(res, true);
    ASSERT_EQ(obj, z_bmqt_MessageEventType::ec_PUT);

    res = z_bmqt_MessageEventType::fromAscii(&obj, "invalid");
    ASSERT_EQ(res, false);

    PV("Testing: toAscii(fromAscii(value)) = value");
    res = z_bmqt_MessageEventType::fromAscii(&obj, "ACK");
    ASSERT_EQ(res, true);
    str = z_bmqt_MessageEventType::toAscii(obj);
    ASSERT_EQ(str, "ACK");
}

static void test2_conversionTest()
{
    mwctst::TestHelper::printTestName("BREATHING TEST");

    // Ensure that the values are equivalent in number
    ASSERT_EQ(
        static_cast<int64_t>(z_bmqt_MessageEventType::Enum::ec_UNDEFINED),
        static_cast<int64_t>(bmqt::MessageEventType::Enum::e_UNDEFINED));
    ASSERT_EQ(static_cast<int64_t>(z_bmqt_MessageEventType::Enum::ec_PUT),
              static_cast<int64_t>(bmqt::MessageEventType::Enum::e_PUT));
    ASSERT_EQ(static_cast<int64_t>(z_bmqt_MessageEventType::Enum::ec_PUSH),
              static_cast<int64_t>(bmqt::MessageEventType::Enum::e_PUSH));
    ASSERT_EQ(static_cast<int64_t>(z_bmqt_MessageEventType::Enum::ec_ACK),
              static_cast<int64_t>(bmqt::MessageEventType::Enum::e_ACK));
    // Ensure that static cast works correctly
    struct Test {
        z_bmqt_MessageEventType::Enum c_type;
        bmqt::MessageEventType::Enum  cpp_type;
    } k_DATA[] = {{z_bmqt_MessageEventType::Enum::ec_UNDEFINED,
                   bmqt::MessageEventType::e_UNDEFINED},
                  {z_bmqt_MessageEventType::Enum::ec_PUT,
                   bmqt::MessageEventType::e_PUT},
                  {z_bmqt_MessageEventType::Enum::ec_PUSH,
                   bmqt::MessageEventType::e_PUSH},
                  {z_bmqt_MessageEventType::Enum::ec_ACK,
                   bmqt::MessageEventType::e_ACK}};

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];

        ASSERT_EQ(static_cast<bmqt::MessageEventType::Enum>(test.c_type),
                  test.cpp_type);
        ASSERT_EQ(test.c_type,
                  static_cast<z_bmqt_MessageEventType::Enum>(test.cpp_type));
    }
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(mwctst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 2: test2_conversionTest(); break;
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        s_testStatus = -1;
    } break;
    }

    TEST_EPILOG(mwctst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
