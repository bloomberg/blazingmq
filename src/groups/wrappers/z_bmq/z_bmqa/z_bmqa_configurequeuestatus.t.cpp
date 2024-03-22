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

// bmqa_configurequeuestatus.t.cpp                                    -*-C++-*-
#include <z_bmqa_configurequeuestatus.h>

// BMQ
#include <z_bmqa_queueid.h>
#include <bmqimp_queue.h>
#include <z_bmqt_correlationid.h>
#include <z_bmqt_uri.h>

// MWC
#include <mwcu_memoutstream.h>

// BDE
#include <bsl_memory.h>
#include <bsl_string.h>

// TEST DRIVER
#include <mwctst_testhelper.h>

#include <stdbool.h> 

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
    mwctst::TestHelper::printTestName("BREATHING TEST");

    PV("Default Constructor");
    {
        z_bmqa_ConfigureQueueStatus *obj; 
        z_bmqa_ConfigureQueueStatus__create(&obj);
        ASSERT_EQ(bool(obj), true);
        ASSERT_EQ(z_bmqa_ConfigureQueueStatus__result(obj), bmqt::ConfigureQueueResult::e_SUCCESS);
        ASSERT_EQ(z_bmqa_ConfigureQueueStatus__errorDescription(obj), bsl::string("", s_allocator_p));
    }

    PV("Valued Constructor");
    {
        z_bmqt_CorrelationId *correlationId; 
        z_bmqt_CorrelationId__create(&correlationId); 
    
        z_bmqa_QueueId *queueId; 
        z_bmqa_QueueId__create(&queueId);

        const bmqt::ConfigureQueueResult::Enum result =
            bmqt::ConfigureQueueResult::e_UNKNOWN;
        const bsl::string errorDescription = bsl::string("ERROR",
                                                         s_allocator_p);

        z_bmqa_ConfigureQueueStatus *obj;
        z_bmqa_ConfigureQueueStatus__create(&obj); 


        ASSERT_EQ(bool(obj), false);
        ASSERT_EQ(z_bmqa_ConfigureQueueStatus__queueId(obj, &queueId), queueId);
        ASSERT_EQ(z_bmqa_ConfigureQueueStatus__result(obj), result);
        ASSERT_EQ(z_bmqa_ConfigureQueueStatus__errorDescription(obj), errorDescription);
    }

    PV("Copy Constructor");
    {
        const bmqt::CorrelationId correlationId =
            bmqt::CorrelationId::autoValue();
        const bmqa::QueueId queueId = bmqa::QueueId(correlationId,
                                                    s_allocator_p);
        const bmqt::ConfigureQueueResult::Enum result =
            bmqt::ConfigureQueueResult::e_TIMEOUT;
        const bsl::string errorDescription = bsl::string("ERROR",
                                                         s_allocator_p);

        bmqa::ConfigureQueueStatus obj1(queueId,
                                        result,
                                        errorDescription,
                                        s_allocator_p);
        bmqa::ConfigureQueueStatus obj2(obj1, s_allocator_p);

        ASSERT_EQ(bool(obj1), bool(obj2));
        ASSERT_EQ(obj1.queueId(), obj2.queueId());
        ASSERT_EQ(obj1.result(), obj2.result());
        ASSERT_EQ(obj1.errorDescription(), obj2.errorDescription());
    }

    PV("Assignment Operator");
    {
        const bmqt::CorrelationId correlationId =
            bmqt::CorrelationId::autoValue();
        const bmqa::QueueId queueId = bmqa::QueueId(correlationId,
                                                    s_allocator_p);
        const bmqt::ConfigureQueueResult::Enum result =
            bmqt::ConfigureQueueResult::e_UNKNOWN;
        const bsl::string errorDescription = bsl::string("ERROR",
                                                         s_allocator_p);

        bmqa::ConfigureQueueStatus obj1(queueId,
                                        result,
                                        errorDescription,
                                        s_allocator_p);
        bmqa::ConfigureQueueStatus obj2(s_allocator_p);
        obj2 = obj1;

        ASSERT_EQ(bool(obj1), bool(obj2));
        ASSERT_EQ(obj1.queueId(), obj2.queueId());
        ASSERT_EQ(obj1.result(), obj2.result());
        ASSERT_EQ(obj1.errorDescription(), obj2.errorDescription());
    }
}

static void test2_comparison()
// ------------------------------------------------------------------------
// COMPARISION
//
// Concerns:
//   Exercise 'bmqa::ConfigureQueueStatus' comparison operators
//
// Plan:
//   1) Create two equivalent 'bmqa::ConfigureQueueStatus' objects and
//      verify that they compare equal.
//   2) Create two non-equivalent 'bmqa::ConfigureQueueStatus' objects and
//      verify that they do not compare equal.
//
// Testing:
//   bool operator==(const bmqa::ConfigureQueueStatus& lhs,
//                   const bmqa::ConfigureQueueStatus& rhs);
//   bool operator!=(const bmqa::ConfigureQueueStatus& lhs,
//                   const bmqa::ConfigureQueueStatus& rhs);
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("COMPARISON");

    PV("Equality");
    {
        const bmqt::CorrelationId correlationId =
            bmqt::CorrelationId::autoValue();
        const bmqa::QueueId queueId = bmqa::QueueId(correlationId,
                                                    s_allocator_p);
        const bmqt::ConfigureQueueResult::Enum result =
            bmqt::ConfigureQueueResult::e_UNKNOWN;
        const bsl::string errorDescription = bsl::string("ERROR",
                                                         s_allocator_p);

        bmqa::ConfigureQueueStatus obj1(queueId,
                                        result,
                                        errorDescription,
                                        s_allocator_p);
        bmqa::ConfigureQueueStatus obj2(obj1, s_allocator_p);

        ASSERT(obj1 == obj2);
    }

    PV("Inequality");
    {
        const bmqt::CorrelationId correlationId =
            bmqt::CorrelationId::autoValue();
        const bmqa::QueueId queueId = bmqa::QueueId(correlationId,
                                                    s_allocator_p);
        const bmqt::ConfigureQueueResult::Enum result1 =
            bmqt::ConfigureQueueResult::e_SUCCESS;
        const bmqt::ConfigureQueueResult::Enum result2 =
            bmqt::ConfigureQueueResult::e_UNKNOWN;
        const bsl::string errorDescription = bsl::string("ERROR",
                                                         s_allocator_p);

        bmqa::ConfigureQueueStatus obj1(queueId,
                                        result1,
                                        errorDescription,
                                        s_allocator_p);
        bmqa::ConfigureQueueStatus obj2(queueId,
                                        result2,
                                        errorDescription,
                                        s_allocator_p);

        ASSERT(obj1 != obj2);
    }
}

static void test3_print()
// ------------------------------------------------------------------------
// PRINT
//
// Concerns:
//   Proper behavior of printing 'bmqa::ConfigureQueueStatus'.
//
// Plan:
//   1. Verify that the 'print' and 'operator<<' methods output the
//      expected string representations
//
// Testing:
//   ConfigureQueueStatus::print()
//   bmqa::operator<<(bsl::ostream&                     stream,
//                    const bmqa::ConfigureQueueStatus& rhs);
// ------------------------------------------------------------------------
{
    s_ignoreCheckDefAlloc = true;
    // Can't check the default allocator: 'bmqa::OpenQueueResult::print' and
    // operator '<<' temporarily allocate a string using the default allocator.

    mwctst::TestHelper::printTestName("PRINT");

    const bmqt::CorrelationId correlationId(2);
    bmqa::QueueId queueId = bmqa::QueueId(correlationId, s_allocator_p);
    const bmqt::ConfigureQueueResult::Enum result =
        bmqt::ConfigureQueueResult::e_TIMEOUT;
    const bsl::string errorDescription = bsl::string("ERROR", s_allocator_p);

    // Set URI on the queueId
    bsl::shared_ptr<bmqimp::Queue>& queue =
        reinterpret_cast<bsl::shared_ptr<bmqimp::Queue>&>(queueId);
    queue->setUri(bmqt::Uri("bmq://bmq.test.mem.priority/q1", s_allocator_p));

    bmqa::ConfigureQueueStatus obj(queueId,
                                   result,
                                   errorDescription,
                                   s_allocator_p);

    PVV(obj);
    const char* expected = "[ queueId = [ uri = bmq://bmq.test.mem.priority/q1"
                           " correlationId = [ numeric = 2 ] ]"
                           " result = \"TIMEOUT (-2)\""
                           " errorDescription = \"ERROR\" ]";

    mwcu::MemOutStream out(s_allocator_p);
    // operator<<
    out << obj;

    ASSERT_EQ(out.str(), expected);

    // Print
    out.reset();
    obj.print(out, 0, -1);

    ASSERT_EQ(out.str(), expected);
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(mwctst::TestHelper::e_DEFAULT);

    bmqt::UriParser::initialize(s_allocator_p);

    switch (_testCase) {
    case 0:
    case 3: test3_print(); break;
    case 2: test2_comparison(); break;
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        s_testStatus = -1;
    } break;
    }

    bmqt::UriParser::shutdown();

    TEST_EPILOG(mwctst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
