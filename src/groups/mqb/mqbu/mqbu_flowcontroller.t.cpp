// Copyright 2026 Bloomberg Finance L.P.
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

// mqbu_flowcontroller.t.cpp                                          -*-C++-*-
#include <mqbu_flowcontroller.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_breathingTest()
// ------------------------------------------------------------------------
// BREATHING TEST
//
// Test flow control.
//
// Plan:
//   Generate some traffic at irregular intervals and verify
//      1) rate measurement
//      2) rate enforcement after policy change
//      3) rate scaling down
//
// Testing:
//   Basic functionality
{
    bmqtst::TestHelper::printTestName("BREATHING TEST");

    mqbu::FlowController                  fc;
    mqbu::FlowController::Watermark::Enum result;
    bsls::Types::Int64                    time = 0;
    mqbu::FlowController::Config          config;

    config = fc.survey(mqbu::FlowController::Policy::e_NONE);

    // 500 at 0.5 sec
    result = fc.add(500);
    BMQTST_ASSERT_EQ(result, mqbu::FlowController::Watermark::e_HIGH);
    fc.update(time += 500, 100);

    BMQTST_ASSERT_EQ(fc.averageWatermark(), 100);

    // 500 at 0.5 sec
    result = fc.add(500);
    BMQTST_ASSERT_EQ(result, mqbu::FlowController::Watermark::e_HIGH);
    fc.update(time += 500, 900);

    BMQTST_ASSERT_EQ(fc.averageWatermark(), 900);

    // 2000 at 2 sec
    fc.update(time += 2000, 100);
    result = fc.add(2000);
    BMQTST_ASSERT_EQ(result, mqbu::FlowController::Watermark::e_HIGH);

    // Make full 5 secs (0.5 + 0.5 + 2 + 2)
    // 2000 at 2 sec
    result = fc.add(2000);
    BMQTST_ASSERT_EQ(result, mqbu::FlowController::Watermark::e_HIGH);
    fc.update(time += 2000, 100);

    config = fc.survey(mqbu::FlowController::Policy::e_LIMIT);
    BMQTST_ASSERT_EQ(config.policy(), mqbu::FlowController::Policy::e_LIMIT);
    BMQTST_ASSERT_EQ(config.ratePerMs(), 1000);
    // Lost the burst after 2 sec, use rate * 4
    BMQTST_ASSERT_EQ(config.burst(), 2000);

    // An additional 6th sec
    fc.update(time += 1000, 100);
    result = fc.add(1000);
    BMQTST_ASSERT_EQ(result, mqbu::FlowController::Watermark::e_HIGH);

    config = fc.survey(mqbu::FlowController::Policy::e_LIMIT);
    BMQTST_ASSERT_EQ(config.policy(), mqbu::FlowController::Policy::e_LIMIT);
    BMQTST_ASSERT_EQ(config.ratePerMs(), 1000);
    BMQTST_ASSERT_EQ(config.burst(), 1000);

    fc.configure(config);

    // The bucket starts full, empty it.
    fc.update(time += 1000, 100);

    // fill the burst
    result = fc.add(1000);
    BMQTST_ASSERT_EQ(result, mqbu::FlowController::Watermark::e_LOW);

    // over the burst
    result = fc.add(1);
    BMQTST_ASSERT_EQ(result, mqbu::FlowController::Watermark::e_STRICT);

    fc.update(time += 1, 100);
    result = fc.add(1);
    BMQTST_ASSERT_EQ(result, mqbu::FlowController::Watermark::e_LOW);

    config.scale(75, 100);
    BMQTST_ASSERT_EQ(config.policy(), mqbu::FlowController::Policy::e_LIMIT);
    BMQTST_ASSERT_EQ(config.ratePerMs(), 750);
    BMQTST_ASSERT_EQ(config.burst(), 750);
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 1: test1_breathingTest(); break;
    default: {
        bsl::cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND."
                  << bsl::endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
