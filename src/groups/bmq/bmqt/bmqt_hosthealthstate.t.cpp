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

// bmqt_hosthealthstate.t.cpp                                         -*-C++-*-
#include <bmqt_hosthealthstate.h>

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

    bmqt::HostHealthState::Enum obj;
    bsl::string                 str;
    bool                        res;

    PV("Testing toAscii");
    str = bmqt::HostHealthState::toAscii(bmqt::HostHealthState::e_HEALTHY);
    ASSERT_EQ(str, "HEALTHY");

    PV("Testing fromAscii");
    res = bmqt::HostHealthState::fromAscii(&obj, "HEALTHY");
    ASSERT_EQ(res, true);
    ASSERT_EQ(obj, bmqt::HostHealthState::e_HEALTHY);
    res = bmqt::HostHealthState::fromAscii(&obj, "invalid");
    ASSERT_EQ(res, false);

    PV("Testing: fromAscii(toAscii(value)) = value");
    res = bmqt::HostHealthState::fromAscii(
        &obj,
        bmqt::HostHealthState::toAscii(bmqt::HostHealthState::e_HEALTHY));
    ASSERT_EQ(res, true);
    ASSERT_EQ(obj, bmqt::HostHealthState::e_HEALTHY);

    PV("Testing: toAscii(fromAscii(value)) = value");
    res = bmqt::HostHealthState::fromAscii(&obj, "UNHEALTHY");
    ASSERT_EQ(res, true);
    str = bmqt::HostHealthState::toAscii(obj);
    ASSERT_EQ(str, "UNHEALTHY");
}

static void test2_printTest()
{
    mwctst::TestHelper::printTestName("PRINT");

    PV("Testing print");

    struct Test {
        bmqt::HostHealthState::Enum d_type;
        const char*                 d_expected;
    } k_DATA[] = {{bmqt::HostHealthState::e_UNKNOWN, "UNKNOWN"},
                  {bmqt::HostHealthState::e_HEALTHY, "HEALTHY"},
                  {bmqt::HostHealthState::e_UNHEALTHY, "UNHEALTHY"},
                  {static_cast<bmqt::HostHealthState::Enum>(-1234),
                   "(* UNKNOWN *)"}};

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test&        test = k_DATA[idx];
        mwcu::MemOutStream out(s_allocator_p);
        mwcu::MemOutStream expected(s_allocator_p);

        expected << test.d_expected;

        out.setstate(bsl::ios_base::badbit);
        bmqt::HostHealthState::print(out, test.d_type, 0, -1);

        ASSERT_EQ(out.str(), "");

        out.clear();
        bmqt::HostHealthState::print(out, test.d_type, 0, -1);

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
    TEST_PROLOG(mwctst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 2: test2_printTest(); break;
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        s_testStatus = -1;
    } break;
    }

    TEST_EPILOG(mwctst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
