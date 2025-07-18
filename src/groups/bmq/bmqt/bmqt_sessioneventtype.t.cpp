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

// bmqt_sessioneventtype.t.cpp                                        -*-C++-*-
#include <bmqt_sessioneventtype.h>

#include <bmqu_memoutstream.h>

// BDE
#include <bsl_ios.h>
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
{
    bmqtst::TestHelper::printTestName("BREATHING TEST");

    bmqt::SessionEventType::Enum obj;
    bsl::string                  str;
    bool                         res;

    PV("Testing toAscii");
    str = bmqt::SessionEventType::toAscii(bmqt::SessionEventType::e_CONNECTED);
    BMQTST_ASSERT_EQ(str, "CONNECTED");

    PV("Testing fromAscii");
    res = bmqt::SessionEventType::fromAscii(&obj, "QUEUE_OPEN_RESULT");
    BMQTST_ASSERT_EQ(res, true);
    BMQTST_ASSERT_EQ(obj, bmqt::SessionEventType::e_QUEUE_OPEN_RESULT);
    res = bmqt::SessionEventType::fromAscii(&obj, "invalid");
    BMQTST_ASSERT_EQ(res, false);

    PV("Testing: fromAscii(toAscii(value)) = value");
    res = bmqt::SessionEventType::fromAscii(
        &obj,
        bmqt::SessionEventType::toAscii(bmqt::SessionEventType::e_TIMEOUT));
    BMQTST_ASSERT_EQ(res, true);
    BMQTST_ASSERT_EQ(obj, bmqt::SessionEventType::e_TIMEOUT);

    PV("Testing: toAscii(fromAscii(value)) = value");
    res = bmqt::SessionEventType::fromAscii(&obj, "DISCONNECTED");
    BMQTST_ASSERT_EQ(res, true);
    str = bmqt::SessionEventType::toAscii(obj);
    BMQTST_ASSERT_EQ(str, "DISCONNECTED");
}

static void test2_printTest()
{
    bmqtst::TestHelper::printTestName("PRINT");

    PV("Testing print");

    struct Test {
        bmqt::SessionEventType::Enum d_type;
        const char*                  d_expected;
    } k_DATA[] = {
        {bmqt::SessionEventType::e_UNDEFINED, "UNDEFINED"},
        {bmqt::SessionEventType::e_CONNECTED, "CONNECTED"},
        {bmqt::SessionEventType::e_DISCONNECTED, "DISCONNECTED"},
        {bmqt::SessionEventType::e_CONNECTION_LOST, "CONNECTION_LOST"},
        {bmqt::SessionEventType::e_RECONNECTED, "RECONNECTED"},
        {bmqt::SessionEventType::e_STATE_RESTORED, "STATE_RESTORED"},
        {bmqt::SessionEventType::e_CONNECTION_TIMEOUT, "CONNECTION_TIMEOUT"},
        {bmqt::SessionEventType::e_QUEUE_OPEN_RESULT, "QUEUE_OPEN_RESULT"},
        {bmqt::SessionEventType::e_QUEUE_REOPEN_RESULT, "QUEUE_REOPEN_RESULT"},
        {bmqt::SessionEventType::e_QUEUE_CLOSE_RESULT, "QUEUE_CLOSE_RESULT"},
        {bmqt::SessionEventType::e_SLOWCONSUMER_NORMAL, "SLOWCONSUMER_NORMAL"},
        {bmqt::SessionEventType::e_SLOWCONSUMER_HIGHWATERMARK,
         "SLOWCONSUMER_HIGHWATERMARK"},
        {bmqt::SessionEventType::e_QUEUE_CONFIGURE_RESULT,
         "QUEUE_CONFIGURE_RESULT"},
        {bmqt::SessionEventType::e_HOST_UNHEALTHY, "HOST_UNHEALTHY"},
        {bmqt::SessionEventType::e_HOST_HEALTH_RESTORED,
         "HOST_HEALTH_RESTORED"},
        {bmqt::SessionEventType::e_QUEUE_SUSPENDED, "QUEUE_SUSPENDED"},
        {bmqt::SessionEventType::e_QUEUE_RESUMED, "QUEUE_RESUMED"},
        {bmqt::SessionEventType::e_ERROR, "ERROR"},
        {bmqt::SessionEventType::e_TIMEOUT, "TIMEOUT"},
        {bmqt::SessionEventType::e_CANCELED, "CANCELED"},
        {static_cast<bmqt::SessionEventType::Enum>(-1234), "(* UNKNOWN *)"},
    };

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];

        if (bmqtst::TestHelperUtil::k_UBSAN &&
            bsl::strcmp(test.d_expected, "(* UNKNOWN *)") == 0) {
            PV("Skip value ["
               << test.d_type
               << "] for UBSan due to out of range enum value casting");
            continue;
        }

        bmqu::MemOutStream out(bmqtst::TestHelperUtil::allocator());
        bmqu::MemOutStream expected(bmqtst::TestHelperUtil::allocator());

        expected << test.d_expected;

        out.setstate(bsl::ios_base::badbit);
        bmqt::SessionEventType::print(out, test.d_type, 0, -1);

        BMQTST_ASSERT_EQ(out.str(), "");

        out.clear();
        bmqt::SessionEventType::print(out, test.d_type, 0, -1);

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
