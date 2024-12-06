// Copyright 2015-2023 Bloomberg Finance L.P.
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

// bmqt_queueoptions.t.cpp                                            -*-C++-*-
#include <bmqt_queueoptions.h>

#include <bmqu_memoutstream.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_breathingTest()
{
    bmqtst::TestHelper::printTestName("BREATHING TEST");

    bmqt::QueueOptions obj(bmqtst::TestHelperUtil::allocator());

    const int  msgs     = 8;
    const int  bytes    = 1024;
    const int  priority = bmqt::QueueOptions::k_CONSUMER_PRIORITY_MIN;
    const bool suspendsOnBadHostHealth = false;

    BMQTST_ASSERT_EQ(bmqt::QueueOptions::k_CONSUMER_PRIORITY_MIN,
                     bmqt::Subscription::k_CONSUMER_PRIORITY_MIN);
    BMQTST_ASSERT_EQ(bmqt::QueueOptions::k_CONSUMER_PRIORITY_MAX,
                     bmqt::Subscription::k_CONSUMER_PRIORITY_MAX);
    BMQTST_ASSERT_EQ(bmqt::QueueOptions::k_DEFAULT_MAX_UNCONFIRMED_MESSAGES,
                     bmqt::Subscription::k_DEFAULT_MAX_UNCONFIRMED_MESSAGES);
    BMQTST_ASSERT_EQ(bmqt::QueueOptions::k_DEFAULT_MAX_UNCONFIRMED_BYTES,
                     bmqt::Subscription::k_DEFAULT_MAX_UNCONFIRMED_BYTES);
    BMQTST_ASSERT_EQ(bmqt::QueueOptions::k_DEFAULT_CONSUMER_PRIORITY,
                     bmqt::Subscription::k_DEFAULT_CONSUMER_PRIORITY);

    PV("Manipulators and accessors");
    obj.setMaxUnconfirmedMessages(msgs)
        .setMaxUnconfirmedBytes(bytes)
        .setConsumerPriority(priority);

    BMQTST_ASSERT_EQ(msgs, obj.maxUnconfirmedMessages());
    BMQTST_ASSERT_EQ(bytes, obj.maxUnconfirmedBytes());
    BMQTST_ASSERT_EQ(priority, obj.consumerPriority());
    BMQTST_ASSERT_EQ(suspendsOnBadHostHealth, obj.suspendsOnBadHostHealth());

    PV("Copy constructor");
    bmqt::QueueOptions obj1(obj, bmqtst::TestHelperUtil::allocator());

    BMQTST_ASSERT_EQ(obj1.maxUnconfirmedMessages(),
                     obj.maxUnconfirmedMessages());
    BMQTST_ASSERT_EQ(obj1.maxUnconfirmedBytes(), obj.maxUnconfirmedBytes());
    BMQTST_ASSERT_EQ(obj1.consumerPriority(), obj.consumerPriority());
    BMQTST_ASSERT_EQ(obj1.suspendsOnBadHostHealth(),
                     obj.suspendsOnBadHostHealth());

    PV("Equality and inequality");
    BMQTST_ASSERT_EQ(obj == obj1, true);
    BMQTST_ASSERT_EQ(obj != obj1, false);

    obj1.setConsumerPriority(bmqt::QueueOptions::k_CONSUMER_PRIORITY_MAX);

    BMQTST_ASSERT_EQ(obj == obj1, false);
    BMQTST_ASSERT_EQ(obj != obj1, true);

    PV("Print");
    obj.setConsumerPriority(0);

    const char* expected = "[ maxUnconfirmedMessages = 8"
                           " maxUnconfirmedBytes = 1024"
                           " consumerPriority = 0"
                           " suspendsOnBadHostHealth = false ]";
    {
        PVV("Print (print function)");
        bmqu::MemOutStream out(bmqtst::TestHelperUtil::allocator());
        obj.print(out, 0, -1);
        BMQTST_ASSERT_EQ(out.str(), expected);
    }

    {
        PVV("Print (stream operator)");
        bmqu::MemOutStream out(bmqtst::TestHelperUtil::allocator());
        out << obj;
        BMQTST_ASSERT_EQ(out.str(), expected);
    }

    {
        PVV("Print (bad stream)");
        bmqu::MemOutStream out(bmqtst::TestHelperUtil::allocator());
        out.setstate(bsl::ios_base::badbit);
        obj.print(out, 0, -1);
        BMQTST_ASSERT_EQ(out.str(), "");
    }
}

static void test2_defaultsTest()
// --------------------------------------------------------------------
// DEFAULTS TEST
//
// Concerns:
//   Test that objects begin with default values, and can distinguish
//   between default values and explicitly set values via 'has*' methods.
//
// Plan:
//   1) Construct a default 'QueueOptions' and test its values.
//   2) Explicitly override a field with the default value, and test that
//      'has*()' return 'true'.
//   3) Set a different field with a different value, and test again.
//
// Testing:
//   Basic functionality
// --------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("DEFAULTS TEST");

    PVV("Step 1. Construct default instance and test its values");
    bmqt::QueueOptions options(bmqtst::TestHelperUtil::allocator());
    BMQTST_ASSERT(!options.hasMaxUnconfirmedBytes());
    BMQTST_ASSERT(!options.hasMaxUnconfirmedMessages());
    BMQTST_ASSERT(!options.hasConsumerPriority());
    BMQTST_ASSERT(!options.hasSuspendsOnBadHostHealth());
    BMQTST_ASSERT_EQ(options.maxUnconfirmedMessages(),
                     bmqt::QueueOptions::k_DEFAULT_MAX_UNCONFIRMED_MESSAGES);
    BMQTST_ASSERT_EQ(options.maxUnconfirmedBytes(),
                     bmqt::QueueOptions::k_DEFAULT_MAX_UNCONFIRMED_BYTES);
    BMQTST_ASSERT_EQ(options.consumerPriority(),
                     bmqt::QueueOptions::k_DEFAULT_CONSUMER_PRIORITY);
    BMQTST_ASSERT_EQ(
        options.suspendsOnBadHostHealth(),
        bmqt::QueueOptions::k_DEFAULT_SUSPENDS_ON_BAD_HOST_HEALTH);

    PVV("Step 2. Explicitly override a field with the default value");
    options.setMaxUnconfirmedMessages(654321);
    BMQTST_ASSERT(options.hasMaxUnconfirmedMessages());
    BMQTST_ASSERT_EQ(options.maxUnconfirmedMessages(), 654321);
    BMQTST_ASSERT(!options.hasMaxUnconfirmedBytes());

    PVV("Step 3. Set a different field with a different value");
    options.setMaxUnconfirmedBytes(9876);
    BMQTST_ASSERT(options.hasMaxUnconfirmedBytes());
    BMQTST_ASSERT_EQ(options.maxUnconfirmedBytes(), 9876);
}

static void test3_mergeTest()
// --------------------------------------------------------------------
// MERGE TEST
//
// Concerns:
//   Test merging of 'bmqt::QueueOptions' objects.
//
// Plan:
//   1) Construct two objects with partially overlapping set fields.
//   2) Merge them together and verify that the resulting object is as
//      expected.
//
// Testing:
//   - bmqt::QueueOptions::merge
// --------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("MERGE TEST");

    PVV("Step 1. Construct two objects");
    bmqt::QueueOptions options(bmqtst::TestHelperUtil::allocator());
    options.setMaxUnconfirmedMessages(54321).setMaxUnconfirmedBytes(0);
    BMQTST_ASSERT(options.hasMaxUnconfirmedMessages());
    BMQTST_ASSERT(options.hasMaxUnconfirmedBytes());
    BMQTST_ASSERT(!options.hasConsumerPriority());

    bmqt::QueueOptions diff(bmqtst::TestHelperUtil::allocator());
    diff.setMaxUnconfirmedBytes(7890).setConsumerPriority(42);
    BMQTST_ASSERT(!diff.hasMaxUnconfirmedMessages());
    BMQTST_ASSERT(diff.hasMaxUnconfirmedBytes());
    BMQTST_ASSERT(diff.hasConsumerPriority());

    PVV("Step 2. Merge them together");
    options.merge(diff);
    BMQTST_ASSERT(options.hasMaxUnconfirmedMessages());
    BMQTST_ASSERT_EQ(options.maxUnconfirmedMessages(), 54321);
    BMQTST_ASSERT(options.hasMaxUnconfirmedBytes());
    BMQTST_ASSERT_EQ(options.maxUnconfirmedBytes(), 7890);
    BMQTST_ASSERT(options.hasConsumerPriority());
    BMQTST_ASSERT_EQ(options.consumerPriority(), 42);
    BMQTST_ASSERT(!options.hasSuspendsOnBadHostHealth());
    BMQTST_ASSERT_EQ(
        options.suspendsOnBadHostHealth(),
        bmqt::QueueOptions::k_DEFAULT_SUSPENDS_ON_BAD_HOST_HEALTH);
}

static void test4_subscriptionsTest()
{
    bmqtst::TestHelper::printTestName("SUBSCRIPTIONS TEST");

    bmqt::QueueOptions obj(bmqtst::TestHelperUtil::allocator());

    const int msgs     = 8;
    const int bytes    = 1024;
    const int priority = 1;

    int in = 1;

    bsl::set<bmqt::SubscriptionHandle> handles(
        bmqtst::TestHelperUtil::allocator());

    for (; in <= 3; ++in) {
        const bmqt::CorrelationId          cid(in);
        bmqt::SubscriptionHandle           handle(cid);
        bmqt::Subscription                 subscription;
        const bmqt::SubscriptionExpression expression(
            "fake",
            bmqt::SubscriptionExpression::e_VERSION_1);

        subscription.setMaxUnconfirmedMessages(msgs)
            .setMaxUnconfirmedBytes(bytes)
            .setConsumerPriority(priority)
            .setExpression(expression);

        bsl::string error;
        BMQTST_ASSERT(
            obj.addOrUpdateSubscription(&error, handle, subscription));
        BMQTST_ASSERT(error.empty());

        // Assert handle uniqueness
        BMQTST_ASSERT(handles.emplace(handle).second);
    }

    // Subscription expression validation
    {
        bmqt::Subscription       subscription;
        bmqt::SubscriptionHandle handle(bmqt::CorrelationId::autoValue());

        const bmqt::SubscriptionExpression expression(
            "0invalid",
            bmqt::SubscriptionExpression::e_VERSION_1);

        subscription.setExpression(expression);
        bsl::string error;
        BMQTST_ASSERT(
            !obj.addOrUpdateSubscription(&error, handle, subscription));
        BMQTST_ASSERT(!error.empty());
    }

    bmqt::QueueOptions::SubscriptionsSnapshot snapshot(
        bmqtst::TestHelperUtil::allocator());
    obj.loadSubscriptions(&snapshot);

    for (bmqt::QueueOptions::SubscriptionsSnapshot::const_iterator citOut =
             snapshot.begin();
         citOut != snapshot.end();
         ++citOut) {
        bsl::set<bmqt::SubscriptionHandle>::const_iterator citIn =
            handles.find(citOut->first);
        BMQTST_ASSERT(citIn != handles.end());

        BMQTST_ASSERT_EQ(citOut->first.correlationId(),
                         citIn->correlationId());
        BMQTST_ASSERT(citOut->first == *citIn);

        BMQTST_ASSERT_EQ(msgs, citOut->second.maxUnconfirmedMessages());
        BMQTST_ASSERT_EQ(bytes, citOut->second.maxUnconfirmedBytes());
        BMQTST_ASSERT_EQ(priority, citOut->second.consumerPriority());
    }
    BMQTST_ASSERT_EQ(handles.size(), snapshot.size());
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 4: test4_subscriptionsTest(); break;
    case 3: test3_mergeTest(); break;
    case 2: test2_defaultsTest(); break;
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_GBL_ALLOC);
}
