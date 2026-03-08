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

// bmqa_closequeuestatus.t.cpp                                        -*-C++-*-
#include <bmqa_closequeuestatus.h>

// BMQ
#include <bmqa_queueid.h>
#include <bmqimp_queue.h>
#include <bmqt_correlationid.h>
#include <bmqt_uri.h>

#include <bmqu_memoutstream.h>

// BDE
#include <bsl_memory.h>
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
// ------------------------------------------------------------------------
// BREATHING TEST
//
// Concerns:
//   Exercise basic functionality before beginning testing in earnest.
//   Probe that functionality to discover basic errors.
//
// Testing:
//   Basic functionality.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("BREATHING TEST");

    PV("Default Constructor");
    {
        bmqa::CloseQueueStatus obj(bmqtst::TestHelperUtil::allocator());
        BMQTST_ASSERT_EQ(bool(obj), true);
        BMQTST_ASSERT_EQ(obj.result(), bmqt::CloseQueueResult::e_SUCCESS);
        BMQTST_ASSERT_EQ(obj.errorDescription(),
                         bsl::string("", bmqtst::TestHelperUtil::allocator()));
    }

    PV("Valued Constructor");
    {
        const bmqt::CorrelationId correlationId =
            bmqt::CorrelationId::autoValue();
        const bmqa::QueueId queueId =
            bmqa::QueueId(correlationId, bmqtst::TestHelperUtil::allocator());
        const bmqt::CloseQueueResult::Enum statusCode =
            bmqt::CloseQueueResult::e_CANCELED;
        const bsl::string errorDescription =
            bsl::string("ERROR", bmqtst::TestHelperUtil::allocator());

        bmqa::CloseQueueStatus obj(queueId,
                                   statusCode,
                                   errorDescription,
                                   bmqtst::TestHelperUtil::allocator());

        BMQTST_ASSERT_EQ(bool(obj), false);
        BMQTST_ASSERT_EQ(obj.queueId(), queueId);
        BMQTST_ASSERT_EQ(obj.result(), statusCode);
        BMQTST_ASSERT_EQ(obj.errorDescription(), errorDescription);
    }

    PV("Copy Constructor");
    {
        const bmqt::CorrelationId correlationId =
            bmqt::CorrelationId::autoValue();
        const bmqa::QueueId queueId =
            bmqa::QueueId(correlationId, bmqtst::TestHelperUtil::allocator());
        const bmqt::CloseQueueResult::Enum statusCode =
            bmqt::CloseQueueResult::e_UNKNOWN;
        const bsl::string errorDescription =
            bsl::string("ERROR", bmqtst::TestHelperUtil::allocator());

        bmqa::CloseQueueStatus obj1(queueId,
                                    statusCode,
                                    errorDescription,
                                    bmqtst::TestHelperUtil::allocator());
        bmqa::CloseQueueStatus obj2(obj1, bmqtst::TestHelperUtil::allocator());

        BMQTST_ASSERT_EQ(bool(obj1), bool(obj2));
        BMQTST_ASSERT_EQ(obj1.queueId(), obj2.queueId());
        BMQTST_ASSERT_EQ(obj1.result(), obj2.result());
        BMQTST_ASSERT_EQ(obj1.errorDescription(), obj2.errorDescription());
    }

    PV("Assignment Operator");
    {
        const bmqt::CorrelationId correlationId =
            bmqt::CorrelationId::autoValue();
        const bmqa::QueueId queueId =
            bmqa::QueueId(correlationId, bmqtst::TestHelperUtil::allocator());
        const bmqt::CloseQueueResult::Enum statusCode =
            bmqt::CloseQueueResult::e_UNKNOWN;
        const bsl::string errorDescription =
            bsl::string("ERROR", bmqtst::TestHelperUtil::allocator());

        bmqa::CloseQueueStatus obj1(queueId,
                                    statusCode,
                                    errorDescription,
                                    bmqtst::TestHelperUtil::allocator());
        bmqa::CloseQueueStatus obj2(bmqtst::TestHelperUtil::allocator());
        obj2 = obj1;

        BMQTST_ASSERT_EQ(bool(obj1), bool(obj2));
        BMQTST_ASSERT_EQ(obj1.queueId(), obj2.queueId());
        BMQTST_ASSERT_EQ(obj1.result(), obj2.result());
        BMQTST_ASSERT_EQ(obj1.errorDescription(), obj2.errorDescription());
    }
}

static void test2_comparison()
// ------------------------------------------------------------------------
// COMPARISION
//
// Concerns:
//   Exercise 'bmqa::CloseQueueStatus' comparison operators
//
// Plan:
//   1) Create two equivalent 'bmqa::CloseQueueStatus' objects and verify
//      that they compare equal.
//   2) Create two non-equivalent 'bmqa::CloseQueueStatus' objects and
//      verify that they do not compare equal.
//
// Testing:
//   bool operator==(const bmqa::CloseQueueStatus& lhs,
//                   const bmqa::CloseQueueStatus& rhs);
//   bool operator!=(const bmqa::CloseQueueStatus& lhs,
//                   const bmqa::CloseQueueStatus& rhs);
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("COMPARISON");

    PV("Equality");
    {
        const bmqt::CorrelationId correlationId =
            bmqt::CorrelationId::autoValue();
        const bmqa::QueueId queueId =
            bmqa::QueueId(correlationId, bmqtst::TestHelperUtil::allocator());
        const bmqt::CloseQueueResult::Enum statusCode =
            bmqt::CloseQueueResult::e_UNKNOWN;
        const bsl::string errorDescription =
            bsl::string("ERROR", bmqtst::TestHelperUtil::allocator());

        bmqa::CloseQueueStatus obj1(queueId,
                                    statusCode,
                                    errorDescription,
                                    bmqtst::TestHelperUtil::allocator());
        bmqa::CloseQueueStatus obj2(obj1, bmqtst::TestHelperUtil::allocator());

        BMQTST_ASSERT(obj1 == obj2);
    }

    PV("Inequality");
    {
        const bmqt::CorrelationId correlationId =
            bmqt::CorrelationId::autoValue();
        const bmqa::QueueId queueId =
            bmqa::QueueId(correlationId, bmqtst::TestHelperUtil::allocator());
        const bmqt::CloseQueueResult::Enum statusCode1 =
            bmqt::CloseQueueResult::e_UNKNOWN;
        const bmqt::CloseQueueResult::Enum statusCode2 =
            bmqt::CloseQueueResult::e_TIMEOUT;
        const bsl::string errorDescription =
            bsl::string("ERROR", bmqtst::TestHelperUtil::allocator());

        bmqa::CloseQueueStatus obj1(queueId,
                                    statusCode1,
                                    errorDescription,
                                    bmqtst::TestHelperUtil::allocator());
        bmqa::CloseQueueStatus obj2(queueId,
                                    statusCode2,
                                    errorDescription,
                                    bmqtst::TestHelperUtil::allocator());

        BMQTST_ASSERT(obj1 != obj2);
    }
}

static void test3_print()
// ------------------------------------------------------------------------
// PRINT
//
// Concerns:
//   Proper behavior of printing 'bmqa::CloseQueueStatus'.
//
// Plan:
//   1. Verify that the 'print' and 'operator<<' methods output the
//      expected string representations
//
// Testing:
//   CloseQueueStatus::print()
//   bmqa::operator<<(bsl::ostream&                 stream,
//                    const bmqa::CloseQueueStatus& rhs);
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // Can't check the default allocator: 'bmqa::OpenQueueResult::print' and
    // operator '<<' temporarily allocate a string using the default allocator.

    bmqtst::TestHelper::printTestName("PRINT");

    const bmqt::CorrelationId correlationId(2);
    bmqa::QueueId                      queueId = bmqa::QueueId(correlationId,
                                          bmqtst::TestHelperUtil::allocator());
    const bmqt::CloseQueueResult::Enum statusCode =
        bmqt::CloseQueueResult::e_TIMEOUT;
    const bsl::string errorDescription =
        bsl::string("ERROR", bmqtst::TestHelperUtil::allocator());

    // Set URI on the queueId
    bsl::shared_ptr<bmqimp::Queue>& queue =
        reinterpret_cast<bsl::shared_ptr<bmqimp::Queue>&>(queueId);
    queue->setUri(bmqt::Uri("bmq://bmq.test.mem.priority/q1",
                            bmqtst::TestHelperUtil::allocator()));

    bmqa::CloseQueueStatus obj(queueId,
                               statusCode,
                               errorDescription,
                               bmqtst::TestHelperUtil::allocator());

    PVV(obj);
    const char* expected = "[ queueId = [ uri = bmq://bmq.test.mem.priority/q1"
                           " correlationId = [ numeric = 2 ] ]"
                           " result = \"TIMEOUT (-2)\""
                           " errorDescription = \"ERROR\" ]";

    bmqu::MemOutStream out(bmqtst::TestHelperUtil::allocator());
    // operator<<
    out << obj;

    BMQTST_ASSERT_EQ(out.str(), expected);

    // Print
    out.reset();
    obj.print(out, 0, -1);

    BMQTST_ASSERT_EQ(out.str(), expected);
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 3: test3_print(); break;
    case 2: test2_comparison(); break;
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
