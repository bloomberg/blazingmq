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

#include <bmqtst_scopedlogobserver.h>
#include <bmqu_memoutstream.h>

// BDE
#include <ball_log.h>
#include <ball_severity.h>
#include <bsls_assert.h>
#include <bsls_types.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                                    HELPERS
// ----------------------------------------------------------------------------

inline bsl::ostream& logEnhancedStorageInfoCb(bsl::ostream& stream)
{
    stream << "Test enhanced storage Info";
    return stream;
}

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
    bmqtst::TestHelper::printTestName("BREATHING TEST");

    const char* k_NAME = "dummy";

    mqbu::CapacityMeter capacityMeter(k_NAME,
                                      0,
                                      bmqtst::TestHelperUtil::allocator());

    BMQTST_ASSERT_EQ(capacityMeter.name(), k_NAME);
    BMQTST_ASSERT_EQ(capacityMeter.messages(), 0);
    BMQTST_ASSERT_EQ(capacityMeter.bytes(), 0);
    BMQTST_ASSERT_EQ(capacityMeter.parent(),
                     static_cast<mqbu::CapacityMeter*>(0));
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
    bmqtst::TestHelper::printTestName("LOG STATE CHANGE");

    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
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

        bmqtst::ScopedLogObserver observer(
            ball::Severity::e_WARN,
            bmqtst::TestHelperUtil::allocator());
        mqbu::CapacityMeter capacityMeter("dummy",
                                          0,
                                          bmqtst::TestHelperUtil::allocator());
        capacityMeter.setLimits(k_MSGS_LIMIT, k_BYTES_LIMIT);
        capacityMeter.setWatermarkThresholds(k_MSGS_THRESHOLD,
                                             k_BYTES_THRESHOLD);

        capacityMeter.commitUnreserved(k_MSGS_HIGH_WATERMARK_VALUE - 1,
                                       k_BYTES_HIGH_WATERMARK_VALUE - 1);

        BMQTST_ASSERT_EQ(observer.records().empty(), true);
    }

    // Set resource to the high watermark, it should log one
    {
        PV("STATE - HIGH WATERMARK");

        bmqtst::ScopedLogObserver observer(
            ball::Severity::e_WARN,
            bmqtst::TestHelperUtil::allocator());
        mqbu::CapacityMeter capacityMeter("dummy",
                                          0,
                                          bmqtst::TestHelperUtil::allocator());
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

        BMQTST_ASSERT(observer.records().empty());

        capacityMeter.commit(k_MSGS_HIGH_WATERMARK_VALUE, 10);

        BMQTST_ASSERT_EQ(observer.records().size(), 1U);

        const ball::Record& record = observer.records()[0];
        BMQTST_ASSERT_EQ(record.fixedFields().severity(),
                         ball::Severity::e_ERROR);

        BMQTST_ASSERT(bmqtst::ScopedLogObserverUtil::recordMessageMatch(
            record,
            "ALARM \\[CAPACITY_STATE_HIGH_WATERMARK\\]",
            bmqtst::TestHelperUtil::allocator()));
        // This pattern is looked for to generate an alarm
        BMQTST_ASSERT(bmqtst::ScopedLogObserverUtil::recordMessageMatch(
            record,
            "dummy.*Messages.*HIGH_WATERMARK",
            bmqtst::TestHelperUtil::allocator()));
    }

    // Set resource to 100%, the full capacity, it should log one
    {
        PV("STATE - FULL");

        bmqtst::ScopedLogObserver observer(
            ball::Severity::e_WARN,
            bmqtst::TestHelperUtil::allocator());
        mqbu::CapacityMeter capacityMeter("dummy",
                                          0,
                                          bmqtst::TestHelperUtil::allocator());
        capacityMeter.setLimits(k_MSGS_LIMIT, k_BYTES_LIMIT);
        capacityMeter.setWatermarkThresholds(k_MSGS_THRESHOLD,
                                             k_BYTES_THRESHOLD);

        capacityMeter.commitUnreserved(k_MSGS_LIMIT, k_BYTES_LIMIT);

        BMQTST_ASSERT_EQ(observer.records().size(), 1U);

        BMQTST_ASSERT_EQ(observer.records()[0].fixedFields().severity(),
                         ball::Severity::e_ERROR);
        BMQTST_ASSERT(bmqtst::ScopedLogObserverUtil::recordMessageMatch(
            observer.records()[0],
            "ALARM \\[CAPACITY_STATE_FULL\\]",
            bmqtst::TestHelperUtil::allocator()));
        // This pattern is looked for to generate an alarm
        BMQTST_ASSERT(bmqtst::ScopedLogObserverUtil::recordMessageMatch(
            observer.records()[0],
            "dummy.*Messages.*FULL",
            bmqtst::TestHelperUtil::allocator()));
    }

    // Set resource to the high watermark, and then to 100%, the full capacity,
    // it should log one for the high watermark and one for the full.
    {
        PV("STATE - HIGH WATERMARK TO FULL");

        bmqtst::ScopedLogObserver observer(
            ball::Severity::e_WARN,
            bmqtst::TestHelperUtil::allocator());
        mqbu::CapacityMeter capacityMeter("dummy",
                                          0,
                                          bmqtst::TestHelperUtil::allocator());
        capacityMeter.setLimits(k_MSGS_LIMIT, k_BYTES_LIMIT);
        capacityMeter.setWatermarkThresholds(k_MSGS_THRESHOLD,
                                             k_BYTES_THRESHOLD);

        PVV("Commit resources -> High Watermark");
        capacityMeter.commitUnreserved(k_MSGS_HIGH_WATERMARK_VALUE, 10);

        BMQTST_ASSERT_EQ(observer.records().size(), 1U);

        BMQTST_ASSERT(bmqtst::ScopedLogObserverUtil::recordMessageMatch(
            observer.records()[0],
            "ALARM \\[CAPACITY_STATE_HIGH_WATERMARK\\]",
            bmqtst::TestHelperUtil::allocator()));
        // This pattern is looked for to generate an alarm
        BMQTST_ASSERT(bmqtst::ScopedLogObserverUtil::recordMessageMatch(
            observer.records()[0],
            "dummy.*Messages.*HIGH_WATERMARK",
            bmqtst::TestHelperUtil::allocator()));
        BMQTST_ASSERT_EQ(observer.records()[0].fixedFields().severity(),
                         ball::Severity::e_ERROR);

        PVV("Commit resources -> Full");
        capacityMeter.commitUnreserved(k_MSGS_LIMIT -
                                           k_MSGS_HIGH_WATERMARK_VALUE,
                                       10);
        BMQTST_ASSERT_EQ(observer.records().size(), 2U);
        BMQTST_ASSERT(bmqtst::ScopedLogObserverUtil::recordMessageMatch(
            observer.records()[1],
            "ALARM \\[CAPACITY_STATE_FULL\\]",
            bmqtst::TestHelperUtil::allocator()));
        // This pattern is looked for to generate an alarm
        BMQTST_ASSERT(bmqtst::ScopedLogObserverUtil::recordMessageMatch(
            observer.records()[1],
            "dummy.*Messages.*FULL",
            bmqtst::TestHelperUtil::allocator()));
    }
}

static void test3_enhancedLog()
// ------------------------------------------------------------------------
// ENHANCED ALARM LOG
//
// Concerns:
//   Ensure that enhanced alarm log is printed if callback is passed.
//
// Plan:
//   1. Pass LogEnhancedStorageInfoCb callback during initialization
//   2. Set resources to the high watermark and ensure the enhanced
//      error message was logged.
//
// Testing:
//   logging
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("ENHANCED ALARM LOG");

    // Set resource to the high watermark, it should log one
    PV("STATE - HIGH WATERMARK");

    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // Logging infrastructure allocates using the default allocator, and
    // that logging is beyond the control of this function.

    const bsls::Types::Int64 k_MSGS_LIMIT                = 10;
    const double             k_MSGS_THRESHOLD            = 0.5;
    const bsls::Types::Int64 k_MSGS_HIGH_WATERMARK_VALUE = k_MSGS_LIMIT *
                                                           k_MSGS_THRESHOLD;
    const bsls::Types::Int64 k_BYTES_LIMIT                = 1024;
    const double             k_BYTES_THRESHOLD            = 0.8;

    bmqtst::ScopedLogObserver observer(ball::Severity::e_WARN,
                                       bmqtst::TestHelperUtil::allocator());
    mqbu::CapacityMeter       capacityMeter(
        "dummy",
        bdlf::BindUtil::bindS(bmqtst::TestHelperUtil::allocator(),
                              &logEnhancedStorageInfoCb,
                              bdlf::PlaceHolders::_1),  // stream
        bmqtst::TestHelperUtil::allocator());
    capacityMeter.setLimits(k_MSGS_LIMIT, k_BYTES_LIMIT);
    capacityMeter.setWatermarkThresholds(k_MSGS_THRESHOLD, k_BYTES_THRESHOLD);

    bsls::Types::Int64 nbMessagesAvailable;
    bsls::Types::Int64 nbBytesAvailable;
    capacityMeter.reserve(&nbMessagesAvailable,
                          &nbBytesAvailable,
                          k_MSGS_HIGH_WATERMARK_VALUE,
                          10);
    BSLS_ASSERT_OPT(nbMessagesAvailable == k_MSGS_HIGH_WATERMARK_VALUE);
    BSLS_ASSERT_OPT(nbBytesAvailable == 10);

    BMQTST_ASSERT(observer.records().empty());

    capacityMeter.commit(k_MSGS_HIGH_WATERMARK_VALUE, 10);

    BMQTST_ASSERT_EQ(observer.records().size(), 1U);

    const ball::Record& record = observer.records()[0];
    BMQTST_ASSERT_EQ(record.fixedFields().severity(), ball::Severity::e_ERROR);

    BMQTST_ASSERT(bmqtst::ScopedLogObserverUtil::recordMessageMatch(
        record,
        "ALARM \\[CAPACITY_STATE_HIGH_WATERMARK\\]",
        bmqtst::TestHelperUtil::allocator()));
    // Check log from callback
    BMQTST_ASSERT(bmqtst::ScopedLogObserverUtil::recordMessageMatch(
        record,
        "Test enhanced storage Info",
        bmqtst::TestHelperUtil::allocator()));
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 2: test2_logStateChange(); break;
    case 1: test1_breathingTest(); break;
    case 3: test3_enhancedLog(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
