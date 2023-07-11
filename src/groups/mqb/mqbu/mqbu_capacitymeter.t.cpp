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

// mqbu_capacitymeter.t.cpp                                           -*-C++-*-
#include <mqbu_capacitymeter.h>

// MWC
#include <mwctst_scopedlogobserver.h>
#include <mwcu_memoutstream.h>

// BDE
#include <ball_log.h>
#include <ball_severity.h>
#include <bsls_assert.h>
#include <bsls_types.h>

// TEST DRIVER
#include <mwctst_testhelper.h>

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
// Plan:
//   TODO:
//
// Testing:
//   Basic functionality
{
    mwctst::TestHelper::printTestName("BREATHING TEST");

    const char* k_NAME = "dummy";

    mqbu::CapacityMeter capacityMeter(k_NAME, s_allocator_p);

    ASSERT_EQ(capacityMeter.name(), k_NAME);
    ASSERT_EQ(capacityMeter.messages(), 0);
    ASSERT_EQ(capacityMeter.bytes(), 0);
    ASSERT_EQ(capacityMeter.parent(), static_cast<mqbu::CapacityMeter*>(0));
}

static void test2_logStateChange()
// ------------------------------------------------------------------------
// LOG STATE CHANGE
//
// Concerns:
//   Log the correct state change upon comitting resources.
//
// Plan:
//   1. Set resouces to below the high watermark and ensure nothing was
//      logged.
//   2. Set resources to the high watermark and ensure the appropriate
//      error message was logged.
//   3. Set resources to exactly the full capacity and ensure the
//      appropriate error message was logged.
//   4. Set resources to exactly the high watermark, then increase to the
//      full capacity, and ensure the appropriate error messages were
//      logged for the (i) high watermark and (ii) full.
//
// Testing:
//   logging
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("LOG STATE CHANGE");

    s_ignoreCheckDefAlloc = true;
    // Logging infrastructure allocates using the default allocator, and
    // that logging is beyond the control of this function.

    const bsls::Types::Int64 k_MSGS_LIMIT                = 10;
    const double             k_MSGS_THRESHOLD            = 0.5;
    const bsls::Types::Int64 k_MSGS_HIGH_WATERMARK_VALUE = k_MSGS_LIMIT *
                                                           k_MSGS_THRESHOLD;
    const bsls::Types::Int64 k_BYTES_LIMIT                = 1024;
    const double             k_BYTES_THRESHOLD            = 0.8;
    const bsls::Types::Int64 k_BYTES_HIGH_WATERMARK_VALUE = k_BYTES_LIMIT *
                                                            k_BYTES_THRESHOLD;

    // Set resource to below the high watermark, should not log anything
    {
        PV("STATE - NORMAL");

        mwctst::ScopedLogObserver observer(ball::Severity::WARN,
                                           s_allocator_p);
        mqbu::CapacityMeter       capacityMeter("dummy", s_allocator_p);
        capacityMeter.setLimits(k_MSGS_LIMIT, k_BYTES_LIMIT);
        capacityMeter.setWatermarkThresholds(k_MSGS_THRESHOLD,
                                             k_BYTES_THRESHOLD);

        capacityMeter.commitUnreserved(k_MSGS_HIGH_WATERMARK_VALUE - 1,
                                       k_BYTES_HIGH_WATERMARK_VALUE - 1);

        ASSERT_EQ(observer.records().empty(), true);
    }

    // Set resource to the high watermark, it should log one
    {
        PV("STATE - HIGH WATERMARK");

        mwctst::ScopedLogObserver observer(ball::Severity::WARN,
                                           s_allocator_p);
        mqbu::CapacityMeter       capacityMeter("dummy", s_allocator_p);
        capacityMeter.setLimits(k_MSGS_LIMIT, k_BYTES_LIMIT);
        capacityMeter.setWatermarkThresholds(k_MSGS_THRESHOLD,
                                             k_BYTES_THRESHOLD);

        bsls::Types::Int64 nbMessagesAvailable;
        bsls::Types::Int64 nbBytesAvailable;
        capacityMeter.reserve(&nbMessagesAvailable,
                              &nbBytesAvailable,
                              k_MSGS_HIGH_WATERMARK_VALUE,
                              10);
        BSLS_ASSERT_OPT(nbMessagesAvailable == k_MSGS_HIGH_WATERMARK_VALUE);
        BSLS_ASSERT_OPT(nbBytesAvailable == 10);

        ASSERT(observer.records().empty());

        capacityMeter.commit(k_MSGS_HIGH_WATERMARK_VALUE, 10);

        ASSERT_EQ(observer.records().size(), 1U);

        const ball::Record& record = observer.records()[0];
        ASSERT_EQ(record.fixedFields().severity(), ball::Severity::ERROR);

        ASSERT(mwctst::ScopedLogObserverUtil::recordMessageMatch(
            record,
            "ALARM \\[CAPACITY_STATE_HIGH_WATERMARK\\]",
            s_allocator_p));
        // This pattern is looked for to generate an alarm
        ASSERT(mwctst::ScopedLogObserverUtil::recordMessageMatch(
            record,
            "dummy.*Messages.*HIGH_WATERMARK",
            s_allocator_p));
    }

    // Set resource to 100%, the full capacity, it should log one
    {
        PV("STATE - FULL");

        mwctst::ScopedLogObserver observer(ball::Severity::WARN,
                                           s_allocator_p);
        mqbu::CapacityMeter       capacityMeter("dummy", s_allocator_p);
        capacityMeter.setLimits(k_MSGS_LIMIT, k_BYTES_LIMIT);
        capacityMeter.setWatermarkThresholds(k_MSGS_THRESHOLD,
                                             k_BYTES_THRESHOLD);

        capacityMeter.commitUnreserved(k_MSGS_LIMIT, k_BYTES_LIMIT);

        ASSERT_EQ(observer.records().size(), 1U);

        ASSERT_EQ(observer.records()[0].fixedFields().severity(),
                  ball::Severity::ERROR);
        ASSERT(mwctst::ScopedLogObserverUtil::recordMessageMatch(
            observer.records()[0],
            "ALARM \\[CAPACITY_STATE_FULL\\]",
            s_allocator_p));
        // This pattern is looked for to generate an alarm
        ASSERT(mwctst::ScopedLogObserverUtil::recordMessageMatch(
            observer.records()[0],
            "dummy.*Messages.*FULL",
            s_allocator_p));
    }

    // Set resource to the high watermark, and then to 100%, the full capacity,
    // it should log one for the high watermark and one for the full.
    {
        PV("STATE - HIGH WATERMARK TO FULL");

        mwctst::ScopedLogObserver observer(ball::Severity::WARN,
                                           s_allocator_p);
        mqbu::CapacityMeter       capacityMeter("dummy", s_allocator_p);
        capacityMeter.setLimits(k_MSGS_LIMIT, k_BYTES_LIMIT);
        capacityMeter.setWatermarkThresholds(k_MSGS_THRESHOLD,
                                             k_BYTES_THRESHOLD);

        PVV("Commit resources -> High Watermark");
        capacityMeter.commitUnreserved(k_MSGS_HIGH_WATERMARK_VALUE, 10);

        ASSERT_EQ(observer.records().size(), 1U);

        ASSERT(mwctst::ScopedLogObserverUtil::recordMessageMatch(
            observer.records()[0],
            "ALARM \\[CAPACITY_STATE_HIGH_WATERMARK\\]",
            s_allocator_p));
        // This pattern is looked for to generate an alarm
        ASSERT(mwctst::ScopedLogObserverUtil::recordMessageMatch(
            observer.records()[0],
            "dummy.*Messages.*HIGH_WATERMARK",
            s_allocator_p));
        ASSERT_EQ(observer.records()[0].fixedFields().severity(),
                  ball::Severity::ERROR);

        PVV("Commit resources -> Full");
        capacityMeter.commitUnreserved(k_MSGS_LIMIT -
                                           k_MSGS_HIGH_WATERMARK_VALUE,
                                       10);
        ASSERT_EQ(observer.records().size(), 2U);
        ASSERT(mwctst::ScopedLogObserverUtil::recordMessageMatch(
            observer.records()[1],
            "ALARM \\[CAPACITY_STATE_FULL\\]",
            s_allocator_p));
        // This pattern is looked for to generate an alarm
        ASSERT(mwctst::ScopedLogObserverUtil::recordMessageMatch(
            observer.records()[1],
            "dummy.*Messages.*FULL",
            s_allocator_p));
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
    case 2: test2_logStateChange(); break;
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        s_testStatus = -1;
    } break;
    }

    TEST_EPILOG(mwctst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
