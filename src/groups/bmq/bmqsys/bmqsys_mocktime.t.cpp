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

// bmqsys_mocktime.t.cpp                                              -*-C++-*-
#include <bmqsys_mocktime.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_basicFunctionality()
{
    bmqtst::TestHelper::printTestName("BASIC FUNCTIONALITY");

    bmqsys::MockTime obj;

    PV("Default constructed state");
    ASSERT_EQ(obj.realtimeClock(), bsls::TimeInterval(0));
    ASSERT_EQ(obj.monotonicClock(), bsls::TimeInterval(0));
    ASSERT_EQ(obj.highResTimer(), 0);

    PV("Set");
    obj.setRealTimeClock(bsls::TimeInterval(1))
        .setMonotonicClock(bsls::TimeInterval(3))
        .setHighResTimer(5);
    ASSERT_EQ(obj.realtimeClock(), bsls::TimeInterval(1));
    ASSERT_EQ(obj.monotonicClock(), bsls::TimeInterval(3));
    ASSERT_EQ(obj.highResTimer(), 5);

    PV("Advance");
    obj.advanceRealTimeClock(bsls::TimeInterval(7))
        .advanceMonotonicClock(bsls::TimeInterval(11))
        .advanceHighResTimer(15);
    ASSERT_EQ(obj.realtimeClock(), bsls::TimeInterval(8));
    ASSERT_EQ(obj.monotonicClock(), bsls::TimeInterval(14));
    ASSERT_EQ(obj.highResTimer(), 20);

    PV("RESET");
    obj.reset();
    ASSERT_EQ(obj.realtimeClock(), bsls::TimeInterval(0));
    ASSERT_EQ(obj.monotonicClock(), bsls::TimeInterval(0));
    ASSERT_EQ(obj.highResTimer(), 0);
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 1: test1_basicFunctionality(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
