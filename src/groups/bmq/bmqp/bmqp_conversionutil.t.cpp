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

#include <bmqp_conversionutil.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_queueid.h>

// BDE
#include <bdlb_nullablevalue.h>

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
//
// Plan:
//   - Verify 'convert' roundtrips between QueueStreamParameters and
//     StreamParameters.
//   - Verify 'makeSubQueueIdInfo' and 'verify'.
//
// Testing:
//   Basic functionality
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("BREATHING TEST");

    bslma::Allocator* alloc = bmqtst::TestHelperUtil::allocator();

    // 1. convert: StreamParameters -> QueueStreamParameters ->
    // StreamParameters
    {
        bmqp_ctrlmsg::StreamParameters sp(alloc);
        sp.subscriptions().resize(1);
        sp.subscriptions()[0].consumers().resize(1);

        bmqp_ctrlmsg::ConsumerInfo& ci = sp.subscriptions()[0].consumers()[0];
        ci.consumerPriority()          = 1;
        ci.consumerPriorityCount()     = 2;
        ci.maxUnconfirmedMessages()    = 100;
        ci.maxUnconfirmedBytes()       = 2048;

        bmqp_ctrlmsg::QueueStreamParameters               qsp(alloc);
        bdlb::NullableValue<bmqp_ctrlmsg::SubQueueIdInfo> subIdInfo(alloc);
        bmqp::ConversionUtil::convert(&qsp, sp, subIdInfo);

        BMQTST_ASSERT_EQ(qsp.consumerPriority(), 1);
        BMQTST_ASSERT_EQ(qsp.consumerPriorityCount(), 2);
        BMQTST_ASSERT_EQ(qsp.maxUnconfirmedMessages(), 100);
        BMQTST_ASSERT_EQ(qsp.maxUnconfirmedBytes(), 2048);

        bmqp_ctrlmsg::StreamParameters sp2(alloc);
        bmqp::ConversionUtil::convert(&sp2, qsp);

        BMQTST_ASSERT_EQ(sp2.subscriptions().size(), 1u);
        BMQTST_ASSERT_EQ(sp2.subscriptions()[0].consumers().size(), 1u);

        const bmqp_ctrlmsg::ConsumerInfo& ci2 =
            sp2.subscriptions()[0].consumers()[0];
        BMQTST_ASSERT_EQ(ci2.consumerPriority(), 1);
        BMQTST_ASSERT_EQ(ci2.consumerPriorityCount(), 2);
        BMQTST_ASSERT_EQ(ci2.maxUnconfirmedMessages(), 100);
        BMQTST_ASSERT_EQ(ci2.maxUnconfirmedBytes(), 2048);
    }

    // 2. makeSubQueueIdInfo
    {
        bdlb::NullableValue<bmqp_ctrlmsg::SubQueueIdInfo> result =
            bmqp::ConversionUtil::makeSubQueueIdInfo(
                "myApp",
                bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID);
        BMQTST_ASSERT(result.isNull());

        result = bmqp::ConversionUtil::makeSubQueueIdInfo("myApp", 42);
        BMQTST_ASSERT(!result.isNull());
        BMQTST_ASSERT_EQ(result.value().appId(), "myApp");
        BMQTST_ASSERT_EQ(result.value().subId(), 42u);
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
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
