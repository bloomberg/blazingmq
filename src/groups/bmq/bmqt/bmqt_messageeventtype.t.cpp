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

// bmqt_messageeventtype.t.cpp                                        -*-C++-*-
#include <bmqt_messageeventtype.h>

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

static void test1_breathingTest()
{
    bmqtst::TestHelper::printTestName("BREATHING TEST");

    bmqt::MessageEventType::Enum obj;
    bsl::string                  str;
    bool                         res;

    PV("Testing toAscii");
    str = bmqt::MessageEventType::toAscii(bmqt::MessageEventType::e_PUSH);
    BMQTST_ASSERT_EQ(str, "PUSH");

    obj = static_cast<bmqt::MessageEventType::Enum>(-1);
    str = bmqt::MessageEventType::toAscii(obj);
    BMQTST_ASSERT_EQ(str, "(* UNKNOWN *)");

    PV("Testing fromAscii");
    res = bmqt::MessageEventType::fromAscii(&obj, "PUT");
    BMQTST_ASSERT_EQ(res, true);
    BMQTST_ASSERT_EQ(obj, bmqt::MessageEventType::e_PUT);

    res = bmqt::MessageEventType::fromAscii(&obj, "invalid");
    BMQTST_ASSERT_EQ(res, false);

    PV("Testing: toAscii(fromAscii(value)) = value");
    res = bmqt::MessageEventType::fromAscii(&obj, "ACK");
    BMQTST_ASSERT_EQ(res, true);
    str = bmqt::MessageEventType::toAscii(obj);
    BMQTST_ASSERT_EQ(str, "ACK");
}

static void test2_printTest()
{
    bmqtst::TestHelper::printTestName("PRINT");

    PV("Testing print");

    BSLMF_ASSERT(bmqt::MessageEventType::k_LOWEST_SUPPORTED_EVENT_TYPE ==
                 bmqt::MessageEventType::e_PUT);

    BSLMF_ASSERT(bmqt::MessageEventType::k_HIGHEST_SUPPORTED_EVENT_TYPE ==
                 bmqt::MessageEventType::e_ACK);

    struct Test {
        bmqt::MessageEventType::Enum d_type;
        const char*                  d_expected;
    } k_DATA[] = {{bmqt::MessageEventType::e_UNDEFINED, "UNDEFINED"},
                  {bmqt::MessageEventType::e_PUT, "PUT"},
                  {bmqt::MessageEventType::e_PUSH, "PUSH"},
                  {bmqt::MessageEventType::e_ACK, "ACK"},
                  {static_cast<bmqt::MessageEventType::Enum>(-1),
                   "(* UNKNOWN *)"}};

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test&        test = k_DATA[idx];
        bmqu::MemOutStream out(bmqtst::TestHelperUtil::allocator());
        bmqu::MemOutStream expected(bmqtst::TestHelperUtil::allocator());

        expected << test.d_expected;

        out.setstate(bsl::ios_base::badbit);
        bmqt::MessageEventType::print(out, test.d_type, 0, -1);

        BMQTST_ASSERT_EQ(out.str(), "");

        out.clear();
        bmqt::MessageEventType::print(out, test.d_type, 0, -1);

        BMQTST_ASSERT_EQ(out.str(), expected.str());

        out.reset();
        out << test.d_type;

        BMQTST_ASSERT_EQ(out.str(), expected.str());
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
    case 2: test2_printTest(); break;
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
