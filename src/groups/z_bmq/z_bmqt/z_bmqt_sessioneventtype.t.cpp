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

// z_bmqt_sessioneventtype.t.cpp                                        -*-C++-*-
#include <z_bmqt_sessioneventtype.h>
#include <bmqt_sessioneventtype.h>

// MWC
#include <mwcu_memoutstream.h>

// BDE
#include <bsl_ios.h>
#include <bsl_string.h>

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

    z_bmqt_SessionEventType::Enum obj;
    bsl::string                  str;
    bool                         res;

    PV("Testing toAscii");
    str = z_bmqt_SessionEventType::toAscii(z_bmqt_SessionEventType::ec_CONNECTED);
    ASSERT_EQ(str, "CONNECTED");

    PV("Testing fromAscii");
    res = z_bmqt_SessionEventType::fromAscii(&obj, "QUEUE_OPEN_RESULT");
    ASSERT_EQ(res, true);
    ASSERT_EQ(obj, z_bmqt_SessionEventType::ec_QUEUE_OPEN_RESULT);
    res = z_bmqt_SessionEventType::fromAscii(&obj, "invalid");
    ASSERT_EQ(res, false);

    PV("Testing: fromAscii(toAscii(value)) = value");
    res = z_bmqt_SessionEventType::fromAscii(
        &obj,
        z_bmqt_SessionEventType::toAscii(z_bmqt_SessionEventType::ec_TIMEOUT));
    ASSERT_EQ(res, true);
    ASSERT_EQ(obj, z_bmqt_SessionEventType::ec_TIMEOUT);

    PV("Testing: toAscii(fromAscii(value)) = value");
    res = z_bmqt_SessionEventType::fromAscii(&obj, "DISCONNECTED");
    ASSERT_EQ(res, true);
    str = z_bmqt_SessionEventType::toAscii(obj);
    ASSERT_EQ(str, "DISCONNECTED");
}

static void test2_toAsciiTest()
{
    mwctst::TestHelper::printTestName("TOASCII");

    PV("Testing print");

    struct Test {
        z_bmqt_SessionEventType::Enum d_type;
        const char*                  d_expected;
    } k_DATA[] = {
        {z_bmqt_SessionEventType::ec_UNDEFINED, "UNDEFINED"},
        {z_bmqt_SessionEventType::ec_CONNECTED, "CONNECTED"},
        {z_bmqt_SessionEventType::ec_DISCONNECTED, "DISCONNECTED"},
        {z_bmqt_SessionEventType::ec_CONNECTION_LOST, "CONNECTION_LOST"},
        {z_bmqt_SessionEventType::ec_RECONNECTED, "RECONNECTED"},
        {z_bmqt_SessionEventType::ec_STATE_RESTORED, "STATE_RESTORED"},
        {z_bmqt_SessionEventType::ec_CONNECTION_TIMEOUT, "CONNECTION_TIMEOUT"},
        {z_bmqt_SessionEventType::ec_QUEUE_OPEN_RESULT, "QUEUE_OPEN_RESULT"},
        {z_bmqt_SessionEventType::ec_QUEUE_REOPEN_RESULT, "QUEUE_REOPEN_RESULT"},
        {z_bmqt_SessionEventType::ec_QUEUE_CLOSE_RESULT, "QUEUE_CLOSE_RESULT"},
        {z_bmqt_SessionEventType::ec_SLOWCONSUMER_NORMAL, "SLOWCONSUMER_NORMAL"},
        {z_bmqt_SessionEventType::ec_SLOWCONSUMER_HIGHWATERMARK,
         "SLOWCONSUMER_HIGHWATERMARK"},
        {z_bmqt_SessionEventType::ec_QUEUE_CONFIGURE_RESULT,
         "QUEUE_CONFIGURE_RESULT"},
        {z_bmqt_SessionEventType::ec_HOST_UNHEALTHY, "HOST_UNHEALTHY"},
        {z_bmqt_SessionEventType::ec_HOST_HEALTH_RESTORED,
         "HOST_HEALTH_RESTORED"},
        {z_bmqt_SessionEventType::ec_QUEUE_SUSPENDED, "QUEUE_SUSPENDED"},
        {z_bmqt_SessionEventType::ec_QUEUE_RESUMED, "QUEUE_RESUMED"},
        {z_bmqt_SessionEventType::ec_ERROR, "ERROR"},
        {z_bmqt_SessionEventType::ec_TIMEOUT, "TIMEOUT"},
        {z_bmqt_SessionEventType::ec_CANCELED, "CANCELED"},
        {static_cast<z_bmqt_SessionEventType::Enum>(-1234), "(* UNKNOWN *)"},
    };

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test&        test = k_DATA[idx];
        const char* result = z_bmqt_SessionEventType::toAscii(test.d_type);
        ASSERT_EQ(strcmp(result, test.d_expected), 0);
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
    case 2: test2_toAsciiTest(); break;
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        s_testStatus = -1;
    } break;
    }

    TEST_EPILOG(mwctst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
