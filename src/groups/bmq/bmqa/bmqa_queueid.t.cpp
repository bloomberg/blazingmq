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

// bmqa_queueid.t.cpp                                                 -*-C++-*-
#include <bmqa_queueid.h>

// BMQ
#include <bmqimp_queue.h>
#include <bmqp_protocol.h>
#include <bmqt_correlationid.h>
#include <bmqt_queueflags.h>
#include <bmqt_queueoptions.h>
#include <bmqt_resultcode.h>
#include <bmqt_uri.h>
#include <bmqu_memoutstream.h>

// BDE
#include <bsl_memory.h>
#include <bsls_assert.h>
#include <bsls_types.h>

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

    const bmqt::QueueOptions k_NULL_OPTIONS(
        bmqt::QueueOptions()
            .setMaxUnconfirmedMessages(0)
            .setMaxUnconfirmedBytes(0)
            .setConsumerPriority(bmqp::Protocol::k_CONSUMER_PRIORITY_INVALID));

    PV("Default Constructor");
    {
        bmqa::QueueId obj(bmqtst::TestHelperUtil::allocator());
        bmqu::MemOutStream reason(bmqtst::TestHelperUtil::allocator());
        BMQTST_ASSERT_EQ(obj.isValid(&reason), false);
        BMQTST_ASSERT_EQ(reason.str(), "Invalid QueueId: 4294967295");
    }

    PV("Valued Constructor - correlationId");
    {
        const bsls::Types::Int64  id = 5;
        const bmqt::CorrelationId corrId(id);

        bmqa::QueueId obj(corrId, bmqtst::TestHelperUtil::allocator());
        BMQTST_ASSERT_EQ(obj.correlationId(), corrId);
        BMQTST_ASSERT_EQ(obj.flags(), bmqt::QueueFlagsUtil::empty());
        BMQTST_ASSERT_EQ(obj.uri(),
                         bmqt::Uri(bmqtst::TestHelperUtil::allocator()));
        BMQTST_ASSERT_EQ(obj.options(), k_NULL_OPTIONS);
        BMQTST_ASSERT_EQ(obj.isValid(), false);
    }

    PV("Valued Constructor - numeric");
    {
        const bsls::Types::Int64 id = 5;

        bmqa::QueueId obj(id, bmqtst::TestHelperUtil::allocator());
        BMQTST_ASSERT_EQ(obj.correlationId(), bmqt::CorrelationId(id));
        BMQTST_ASSERT_EQ(obj.flags(), bmqt::QueueFlagsUtil::empty());
        BMQTST_ASSERT_EQ(obj.uri(),
                         bmqt::Uri(bmqtst::TestHelperUtil::allocator()));
        BMQTST_ASSERT_EQ(obj.options(), k_NULL_OPTIONS);
        BMQTST_ASSERT_EQ(obj.isValid(), false);
    }

    PV("Valued Constructor - void ptr");
    {
        const char* buffer = "1234";
        void*       ptr    = static_cast<void*>(const_cast<char*>(buffer));

        bmqa::QueueId obj(ptr, bmqtst::TestHelperUtil::allocator());
        BMQTST_ASSERT_EQ(obj.correlationId(), bmqt::CorrelationId(ptr));
        BMQTST_ASSERT_EQ(obj.flags(), bmqt::QueueFlagsUtil::empty());
        BMQTST_ASSERT_EQ(obj.uri(),
                         bmqt::Uri(bmqtst::TestHelperUtil::allocator()));
        BMQTST_ASSERT_EQ(obj.options(), k_NULL_OPTIONS);
        BMQTST_ASSERT_EQ(obj.isValid(), false);
    }

    PV("Valued Constructor - shared ptr to void");
    {
        const int k_VALUE = 11;

        bsl::shared_ptr<int> sptr;
        sptr.createInplace(bmqtst::TestHelperUtil::allocator(), k_VALUE);

        bmqa::QueueId obj(sptr, bmqtst::TestHelperUtil::allocator());
        BMQTST_ASSERT_EQ(obj.correlationId(), bmqt::CorrelationId(sptr));
        BMQTST_ASSERT_EQ(obj.flags(), bmqt::QueueFlagsUtil::empty());
        BMQTST_ASSERT_EQ(obj.uri(),
                         bmqt::Uri(bmqtst::TestHelperUtil::allocator()));
        BMQTST_ASSERT_EQ(obj.options(), k_NULL_OPTIONS);
        BMQTST_ASSERT_EQ(obj.isValid(), false);
    }

    PV("Copy Constructor");
    {
        const bsls::Types::Int64 id = 5;

        bmqa::QueueId obj1(id, bmqtst::TestHelperUtil::allocator());
        bmqa::QueueId obj2(obj1, bmqtst::TestHelperUtil::allocator());
        BMQTST_ASSERT_EQ(obj1.correlationId(), obj2.correlationId());
        BMQTST_ASSERT_EQ(obj1.flags(), obj2.flags());
        BMQTST_ASSERT_EQ(obj1.uri(), obj2.uri());
        BMQTST_ASSERT_EQ(obj1.options(), obj2.options());
        BMQTST_ASSERT_EQ(obj1.isValid(), obj2.isValid());
    }

    PV("Assignment Operator");
    {
        const bsls::Types::Int64 id = 5;

        bmqa::QueueId obj1(id, bmqtst::TestHelperUtil::allocator());
        bmqa::QueueId obj2(bmqtst::TestHelperUtil::allocator());
        obj2 = obj1;
        BMQTST_ASSERT_EQ(obj1.correlationId(), obj2.correlationId());
        BMQTST_ASSERT_EQ(obj1.flags(), obj2.flags());
        BMQTST_ASSERT_EQ(obj1.uri(), obj2.uri());
        BMQTST_ASSERT_EQ(obj1.options(), obj2.options());
        BMQTST_ASSERT_EQ(obj1.isValid(), obj2.isValid());
    }

    PV("Uri Method");
    {
        const bsls::Types::Int64 id = 5;
        const char k_QUEUE_URL[] = "bmq://ts.trades.myapp.~bt/my.queue?id=foo";

        bmqa::QueueId obj(id, bmqtst::TestHelperUtil::allocator());
        BMQTST_ASSERT_EQ(obj.correlationId(), bmqt::CorrelationId(id));
        BMQTST_ASSERT_EQ(obj.flags(), bmqt::QueueFlagsUtil::empty());
        BMQTST_ASSERT_EQ(obj.uri(),
                         bmqt::Uri(bmqtst::TestHelperUtil::allocator()));
        BMQTST_ASSERT_EQ(obj.options(), k_NULL_OPTIONS);
        BMQTST_ASSERT_EQ(obj.isValid(), false);

        // Convert to bmqimp::Queue
        bsl::shared_ptr<bmqimp::Queue>& queue =
            reinterpret_cast<bsl::shared_ptr<bmqimp::Queue>&>(obj);

        // Set uri to impl object
        const bmqt::Uri uri(k_QUEUE_URL, bmqtst::TestHelperUtil::allocator());
        BMQTST_ASSERT_EQ(uri.isValid(), true);

        queue->setUri(uri);

        BMQTST_ASSERT_EQ(obj.uri().asString(), k_QUEUE_URL);
    }
}

static void test2_comparison()
// ------------------------------------------------------------------------
// COMPARISION
//
// Concerns:
//   Exercise 'bmqa::QueueId' comparison operators
//
// Plan:
//   1) Create two different 'bmqa::QueueId' objects and verify that they
//      do not compare equal.
//   2) Create two default 'bmqa::QueueId' objects and verify that they
//      compare equal, then create two different ones, assign one to the
//      other, and verify that they compare equal.
//
// Testing:
//   bool operator==(const bmqa::QueueId& lhs, const bmqa::QueueId& rhs);
//   bool operator!=(const bmqa::QueueId& lhs, const bmqa::QueueId& rhs);
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("COMPARISON");

    PV("Inequality");
    {
        const bsls::Types::Int64 k_ID = 18;

        bmqa::QueueId obj1(k_ID, bmqtst::TestHelperUtil::allocator());
        bmqa::QueueId obj2(k_ID, bmqtst::TestHelperUtil::allocator());
        BMQTST_ASSERT_NE(obj1, obj2);
    }

    PV("Equality");
    {
        // Different defaults are never equal
        BMQTST_ASSERT_NE(bmqa::QueueId(bmqtst::TestHelperUtil::allocator()),
                         bmqa::QueueId(bmqtst::TestHelperUtil::allocator()));

        // Assignment makes equal
        const bsls::Types::Int64 k_ID1 = 5;
        const bsls::Types::Int64 k_ID2 = 11;

        bmqa::QueueId obj1(k_ID1, bmqtst::TestHelperUtil::allocator());
        bmqa::QueueId obj2(k_ID2, bmqtst::TestHelperUtil::allocator());
        BSLS_ASSERT_OPT(obj1 != obj2);

        obj1 = obj2;
        BMQTST_ASSERT_EQ(obj1, obj2);
    }
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 2: test2_comparison(); break;
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_ALLOC);
}
