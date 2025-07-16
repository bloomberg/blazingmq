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

// bmqtst_scopedlogobserver.t.cpp                                     -*-C++-*-
#include <bmqtst_scopedlogobserver.h>

// BDE
#include <ball_context.h>
#include <ball_log.h>
#include <ball_record.h>
#include <ball_severity.h>
#include <bsl_iostream.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>
#include <bsl_ios.h>

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
//   1. Instantiate an observer with severity indicating that the observer
//      is disabled and verify correct state.
//   2. Set the severity to a different value and verify correct state.
//
// Testing:
//   Constructor
//   setSeverityThreshold
{
    bmqtst::TestHelper::printTestName("BREATHING TEST");

    bmqtst::ScopedLogObserver observer(ball::Severity::e_OFF,
                                       bmqtst::TestHelperUtil::allocator());

    BMQTST_ASSERT_EQ(observer.severityThreshold(), ball::Severity::e_OFF);
    BMQTST_ASSERT(!observer.isEnabled());
    BMQTST_ASSERT(observer.records().empty());

    observer.setSeverityThreshold(ball::Severity::e_WARN);

    BMQTST_ASSERT_EQ(observer.severityThreshold(), ball::Severity::e_WARN);
    BMQTST_ASSERT(observer.isEnabled());
    BMQTST_ASSERT(observer.records().empty());
}

static void test2_publish()
// ------------------------------------------------------------------------
// PUBLISH
//
// Concerns:
//   Publishing causes the observer to capture records depending on the
//   observer's severity level.
//
// Plan:
//   1. Instantiate an observer with severity at ERROR.
//   2. Publish a log record having ERROR severity and verify that the
//      observer captured it.
//   3. Publish a log record having WARN severity and verify that the
//      observer did not capture it.
//
// Testing:
//   publish
{
    bmqtst::TestHelper::printTestName("PUBLISH");

    ball::Record  record1(bmqtst::TestHelperUtil::allocator());
    ball::Record  record2(bmqtst::TestHelperUtil::allocator());
    ball::Context context1(bmqtst::TestHelperUtil::allocator());
    ball::Context context2(bmqtst::TestHelperUtil::allocator());

    bmqtst::ScopedLogObserver observer(ball::Severity::e_ERROR,
                                       bmqtst::TestHelperUtil::allocator());

    BMQTST_ASSERT_EQ(observer.severityThreshold(), ball::Severity::e_ERROR);
    BMQTST_ASSERT(observer.isEnabled());
    BMQTST_ASSERT(observer.records().empty());

    record1.fixedFields().setSeverity(ball::Severity::e_ERROR);
    observer.publish(record1, context1);

    BMQTST_ASSERT_EQ(observer.severityThreshold(), ball::Severity::e_ERROR);
    BMQTST_ASSERT(observer.isEnabled());
    BMQTST_ASSERT(observer.records().size() == 1);
    BMQTST_ASSERT(observer.records()[0] == record1);

    record2.fixedFields().setSeverity(ball::Severity::e_WARN);
    observer.publish(record2, context2);

    BMQTST_ASSERT_EQ(observer.severityThreshold(), ball::Severity::e_ERROR);
    BMQTST_ASSERT(observer.isEnabled());
    BMQTST_ASSERT(observer.records().size() == 1);
    BMQTST_ASSERT(observer.records()[0] == record1);
}

static void test3_recordMessageMatch()
// ------------------------------------------------------------------------
// RECORD MESSAGE MATCH
//
// Concerns:
//   Matching the message of a record against a pattern works as expected.
//
// Plan:
//   1. Instantiate an observer with severity at ERROR.
//   2. Publish a log record having ERROR severity and verify that the
//      observer captured it.
//   3. Publish a log record having WARN severity and verify that the
//      observer did not capture it.
//
// Testing:
//   publish
{
    bmqtst::TestHelper::printTestName("RECORD MESSAGE MATCH");

    struct Test {
        int         d_line;
        const char* d_msg;
        const char* d_pattern;
        bool        d_isMatch;
    } k_DATA[] = {
        {L_,
         "Bytes (STATE_HIGH_WATERMARK)",
         "Bytes.*STate_HIGH_WatERmArk",
         true},
        {L_, "RANDOM DUMMY STRING", "DuMmY", true},
        {L_, "Normal full", "high_watermark", false},
    };

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];

        PV(test.d_line << ": Testing: 'ScopedLogObserverUtil::match(\""
                       << test.d_msg << "\", \"" << test.d_pattern << "\")'"
                       << " == " << bsl::boolalpha << test.d_isMatch);

        ball::Record  record(bmqtst::TestHelperUtil::allocator());
        ball::Context context(bmqtst::TestHelperUtil::allocator());

        record.fixedFields().setMessage(test.d_msg);

        BMQTST_ASSERT_EQ(bmqtst::ScopedLogObserverUtil::recordMessageMatch(
                             record,
                             test.d_pattern,
                             bmqtst::TestHelperUtil::allocator()),
                         test.d_isMatch);
    }
}

static void test4_usageExample()
// ------------------------------------------------------------------------
// USAGE EXAMPLE
//
// Concerns:
//   Test that the usage example provided in the documentation of the
//   component is correct.
//
// Plan:
//   TODO:
//
// Testing:
//   TODO:
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // Logging infrastructure allocates using the default allocator, and
    // that logging is beyond the control of this function.

    bmqtst::TestHelper::printTestName("USAGE EXAMPLE");

    BALL_LOG_SET_CATEGORY("TEST");

    bmqtst::ScopedLogObserver observer(ball::Severity::e_ERROR,
                                       bmqtst::TestHelperUtil::allocator());

    BALL_LOG_ERROR << "MySampleError";

    BMQTST_ASSERT_EQ(observer.records().size(), 1U);
    BMQTST_ASSERT(bmqtst::ScopedLogObserverUtil::recordMessageMatch(
        observer.records()[0],
        ".*Sample.*",
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
    case 4: test4_usageExample(); break;
    case 3: test3_recordMessageMatch(); break;
    case 2: test2_publish(); break;
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
