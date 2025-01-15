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

// bmqc_monitoredqueue.t.cpp                                          -*-C++-*-
#include <bmqc_monitoredqueue.h>

#include <bmqu_memoutstream.h>
#include <bmqu_printutil.h>

// BDE
#include <bdlcc_fixedqueue.h>
#include <bdlf_bind.h>
#include <bdlmt_threadpool.h>
#include <bdlt_timeunitratio.h>
#include <bsl_functional.h>
#include <bsl_iostream.h>
#include <bsl_limits.h>
#include <bsl_string.h>
#include <bslmt_semaphore.h>
#include <bslmt_threadattributes.h>
#include <bslmt_threadutil.h>
#include <bsls_timeinterval.h>
#include <bsls_timeutil.h>
#include <bsls_types.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_MonitoredQueueState_toAscii()
// ------------------------------------------------------------------------
// MONITORED  QUEUE STATE - TO ASCII
//
// Concerns:
//   Proper behavior of the 'MonitoredQueueState::toAscii' method.
//
// Plan:
//   Verify that the 'toAscii' method returns the string representation of
//   every enum value of 'MonitoredQueueState'.
//
// Testing:
//   'MonitoredQueueState::toAscii'
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("MONITORED  QUEUE STATE "
                                      "- TO ASCII");

    struct Test {
        int         d_line;
        int         d_value;
        const char* d_expected;
    } k_DATA[] = {{L_, 0, "NORMAL"},
                  {L_, 1, "HIGH_WATERMARK_REACHED"},
                  {L_, 2, "HIGH_WATERMARK_2_REACHED"},
                  {L_, 3, "QUEUE_FILLED"}};
    // NOTE: Using the 'integer' value instead of the enum to ensure the
    //       numeric values are *never* changed.

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];

        PVV(test.d_line << ": Testing: toAscii(" << test.d_value
                        << ") == " << test.d_expected);

        bsl::string ascii(bmqtst::TestHelperUtil::allocator());
        ascii = bmqc::MonitoredQueueState::toAscii(
            bmqc::MonitoredQueueState::Enum(test.d_value));

        BMQTST_ASSERT_EQ_D(test.d_line, ascii, test.d_expected);
    }
}

static void test2_MonitoredQueueState_print()
// ------------------------------------------------------------------------
// MONITORED QUEUE STATE - PRINT
//
// Concerns:
//   Proper behavior of the 'MonitoredQueueState::print' method.
//
// Plan:
//   1. Verify that the 'print' and 'operator<<' methods output the
//   expected
//      string representation of every enum value of
//      'MonitoredQueueState'.
//   2. Verify that the 'print' method outputs nothing when the stream has
//      the bad bit set.
//
// Testing:
//   'MonitoredQueueState::print'
//   operator<<(bsl::ostream&, MonitoredQueueState::Enum)
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("MONITORED QUEUE STATE - PRINT");

    struct Test {
        int         d_line;
        int         d_value;
        const char* d_expected;
    } k_DATA[] = {{L_, 0, "NORMAL"},
                  {L_, 1, "HIGH_WATERMARK_REACHED"},
                  {L_, 2, "HIGH_WATERMARK_2_REACHED"},
                  {L_, 3, "QUEUE_FILLED"}};
    // NOTE: Using the 'integer' value instead of the enum to ensure the
    //       numeric values are *never* changed.

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];

        PVV(test.d_line << ": Testing: print(" << test.d_value
                        << ") == " << test.d_expected);

        // 1.
        bmqu::MemOutStream out(bmqtst::TestHelperUtil::allocator());
        bmqc::MonitoredQueueState::Enum obj(
            static_cast<bmqc::MonitoredQueueState::Enum>(test.d_value));

        // print
        bmqc::MonitoredQueueState::print(out, obj, 0, 0);

        PVV(test.d_line << ": '" << out.str());

        bsl::string expected(bmqtst::TestHelperUtil::allocator());
        expected.assign(test.d_expected);
        expected.append("\n");
        BMQTST_ASSERT_EQ_D(test.d_line, out.str(), expected);

        // operator<<
        out.reset();
        out << obj;

        BMQTST_ASSERT_EQ_D(test.d_line, out.str(), test.d_expected);

        // 2. 'badbit' set
        out.reset();
        out.setstate(bsl::ios_base::badbit);
        bmqc::MonitoredQueueState::print(out, obj, 0, -1);

        BMQTST_ASSERT_EQ_D(test.d_line, out.str(), "");
    }
}

//=============================================================================
//                                MAIN PROGRAM
//-----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 2: test2_MonitoredQueueState_print(); break;
    case 1: test1_MonitoredQueueState_toAscii(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
