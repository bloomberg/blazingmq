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

// mqbnet_dummysession.t.cpp                                          -*-C++-*-
#include <mqbnet_dummysession.h>

// MQB
#include <mqbnet_mockcluster.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>

#include <bmqio_testchannel.h>

// BDE
#include <bdlbb_pooledblobbufferfactory.h>
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

static void test1_BreathingTest()
{
    bmqtst::TestHelper::printTestName("BreathingTest");

    // Create some needed dummy/mocked objects
    bsl::string description("DummyDescription",
                            bmqtst::TestHelperUtil::allocator());

    bmqp_ctrlmsg::NegotiationMessage negotiationMessage(
        bmqtst::TestHelperUtil::allocator());
    negotiationMessage.makeClientIdentity().hostName() = "dummyIdentity";

    mqbcfg::ClusterDefinition clusterConfig(
        bmqtst::TestHelperUtil::allocator());
    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    mqbnet::MockCluster mockCluster(clusterConfig,
                                    &bufferFactory,
                                    bmqtst::TestHelperUtil::allocator());

    mqbcfg::ClusterNode clusterNodeConfig(bmqtst::TestHelperUtil::allocator());
    mqbnet::MockClusterNode mockClusterNode(
        &mockCluster,
        clusterNodeConfig,
        &bufferFactory,
        bmqtst::TestHelperUtil::allocator());

    bsl::shared_ptr<bmqio::TestChannel> testChannel;
    testChannel.createInplace(bmqtst::TestHelperUtil::allocator());

    // Create a test object
    mqbnet::DummySession obj(testChannel,
                             negotiationMessage,
                             &mockClusterNode,
                             description,
                             bmqtst::TestHelperUtil::allocator());

    {
        PV("Test Accessors");
        BMQTST_ASSERT_EQ(obj.negotiationMessage(), negotiationMessage);
        BMQTST_ASSERT_EQ(obj.description(), description);
        BMQTST_ASSERT_EQ(obj.clusterNode(), &mockClusterNode);
        BMQTST_ASSERT_EQ(obj.channel(), testChannel);
    }

    {
        PV("Ensure that processEvent asserts");
        bmqp::Event event(bmqtst::TestHelperUtil::allocator());
        BMQTST_ASSERT_OPT_FAIL(obj.processEvent(event, &mockClusterNode));
    }

    {  // teardown is a no-op, just invoke it for coverage's sake
        bsl::shared_ptr<void> handle;
        obj.tearDown(handle, false);
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
    case 1: test1_BreathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    // NOTE: Can't check default allocation because of BALL logging from
    //       constructor/desctructor of the object under test.
    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_GBL_ALLOC);
}
