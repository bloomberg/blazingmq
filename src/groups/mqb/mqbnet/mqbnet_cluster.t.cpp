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

// mqbnet_cluster.t.cpp                                               -*-C++-*-
#include <mqbnet_cluster.h>

// BDE
#include <bdlbb_blob.h>
#include <bsls_platform.h>
#include <bsls_protocoltest.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>
#include <bsl_limits.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// Disable some compiler warning for simplified write of the
// 'AbstractSessionTestImp'.
#if defined(BSLS_PLATFORM_CMP_CLANG)
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wweak-vtables"
// Disabling 'weak-vtables' so that we can define all interface methods
// inline, without the following warning:
//..
//  mqbnet_cluster.t.cpp:31:1: error: 'ClusterObserverTestImp' has no
// out-of-line virtual method definitions; its vtable will be emitted in
//  every translation unit [-Werror,-Wweak-vtables]
//..
#endif  // BSLS_PLATFORM_CMP_CLANG

// ============================================================================
//                 HELPER CLASSES AND FUNCTIONS FOR TESTING
// ----------------------------------------------------------------------------

/// A test implementation of the `mqbnet::ClusterObserver` protocol
struct ClusterObserverTestImp
: bsls::ProtocolTestImp<mqbnet::ClusterObserver> {
    void onNodeStateChange(mqbnet::ClusterNode* node,
                           bool isAvailable) BSLS_KEYWORD_OVERRIDE
    {
        markDone();
    }
};

/// A test implementation of the `mqbnet::ClusterNode` protocol
struct ClusterNodeTestImp : bsls::ProtocolTestImp<mqbnet::ClusterNode> {
    mqbnet::ClusterNode* setChannel(
        const bsl::weak_ptr<bmqio::Channel>& value,
        const bmqp_ctrlmsg::ClientIdentity&  identity,
        const bmqio::Channel::ReadCallback&  readCb) BSLS_KEYWORD_OVERRIDE
    {
        return markDone();
    }

    bool enableRead() BSLS_KEYWORD_OVERRIDE { return markDone(); }

    mqbnet::ClusterNode* resetChannel() BSLS_KEYWORD_OVERRIDE
    {
        return markDone();
    }

    void closeChannel() BSLS_KEYWORD_OVERRIDE { markDone(); }

    bmqt::GenericResult::Enum
    write(const bsl::shared_ptr<bdlbb::Blob>& blob,
          bmqp::EventType::Enum type = bmqp::EventType::e_CONTROL)
        BSLS_KEYWORD_OVERRIDE
    {
        return markDone();
    }

    mqbnet::Channel& channel() BSLS_KEYWORD_OVERRIDE { return markDoneRef(); }

    const bmqp_ctrlmsg::ClientIdentity& identity() const BSLS_KEYWORD_OVERRIDE
    {
        return markDoneRef();
    }

    int nodeId() const BSLS_KEYWORD_OVERRIDE { return markDone(); }

    const bsl::string& hostName() const BSLS_KEYWORD_OVERRIDE
    {
        return markDoneRef();
    }

    const bsl::string& nodeDescription() const BSLS_KEYWORD_OVERRIDE
    {
        return markDoneRef();
    }

    const bsl::string& dataCenter() const BSLS_KEYWORD_OVERRIDE
    {
        return markDoneRef();
    }

    const mqbnet::Cluster* cluster() const BSLS_KEYWORD_OVERRIDE
    {
        return markDone();
    }

    bool isAvailable() const BSLS_KEYWORD_OVERRIDE { return markDone(); }
};

/// A test implementation of the `mqbnet::Cluster` protocol
struct ClusterTestImp : bsls::ProtocolTestImp<mqbnet::Cluster> {
    mqbnet::Cluster*
    registerObserver(mqbnet::ClusterObserver* observer) BSLS_KEYWORD_OVERRIDE
    {
        return markDone();
    }

    mqbnet::Cluster*
    unregisterObserver(mqbnet::ClusterObserver* observer) BSLS_KEYWORD_OVERRIDE
    {
        return markDone();
    }

    int writeAll(const bsl::shared_ptr<bdlbb::Blob>& blob,
                 bmqp::EventType::Enum type = bmqp::EventType::e_CONTROL)
        BSLS_KEYWORD_OVERRIDE
    {
        return markDone();
    }

    int
    broadcast(const bsl::shared_ptr<bdlbb::Blob>& blob) BSLS_KEYWORD_OVERRIDE
    {
        return markDone();
    }

    void closeChannels() BSLS_KEYWORD_OVERRIDE { markDone(); }

    mqbnet::ClusterNode* lookupNode(int nodeId) BSLS_KEYWORD_OVERRIDE
    {
        return markDone();
    }

    mqbnet::Cluster::NodesList& nodes() BSLS_KEYWORD_OVERRIDE
    {
        return markDoneRef();
    }

    void enableRead() BSLS_KEYWORD_OVERRIDE { markDone(); }

    void
    onProxyConnectionUp(const bsl::shared_ptr<bmqio::Channel>& channel,
                        const bmqp_ctrlmsg::ClientIdentity&    identity,
                        const bsl::string& description) BSLS_KEYWORD_OVERRIDE
    {
        markDone();
    }

    const bsl::string& name() const BSLS_KEYWORD_OVERRIDE
    {
        return markDoneRef();
    }

    int selfNodeId() const BSLS_KEYWORD_OVERRIDE { return markDone(); }

    const mqbnet::ClusterNode* selfNode() const BSLS_KEYWORD_OVERRIDE
    {
        return markDone();
    }

    const mqbnet::Cluster::NodesList& nodes() const BSLS_KEYWORD_OVERRIDE
    {
        return markDoneRef();
    }
};

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_ClusterObserver()
// ------------------------------------------------------------------------
// PROTOCOL TEST:
//   Ensure this class is a properly defined protocol.
//
// Concerns:
//: 1 The protocol has no data members.
//:
//: 2 The protocol has a virtual destructor.
//:
//: 3 All methods of the protocol are publicly accessible.
//
// Plan:
//: 1 Define a concrete derived implementation, 'ClusterObserverTestImp',
//:   of the protocol.
//:
//: 2 Create an object of the 'bsls::ProtocolTest' class template
//:   parameterized by 'ClusterObserverTestImp', and use it to verify that:
//:
//:   1 The protocol has no data members. (C-1)
//:
//:   2 The protocol has a virtual destructor. (C-2)
//:
//: 3 Use the 'BSLS_PROTOCOLTEST_ASSERT' macro to verify that
//:   non-creator methods of the protocol are:
//:
//:   1 publicly accessible. (C-3)
//
// Testing:
//   PROTOCOL TEST
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("ClusterObserver");

    PV("Creating a test object");
    bsls::ProtocolTest<ClusterObserverTestImp> testObj(
        bmqtst::TestHelperUtil::verbosityLevel() > 2);

    // NOTE: 'ClusterObserver' is purposely not a pure protocol, each method
    //       has a default no-op implementation.
    // BMQTST_ASSERT(testObj.testAbstract());

    PV("Verify that there are no data members");
    BMQTST_ASSERT(testObj.testNoDataMembers());

    PV("Verify that the destructor is virtual");
    BMQTST_ASSERT(testObj.testVirtualDestructor());

    {
        PV("Verify that methods are public and virtual");

        mqbnet::ClusterNode* dummyClusterNode_p = 0;
        bool                 dummyBool          = false;

        BSLS_PROTOCOLTEST_ASSERT(testObj,
                                 onNodeStateChange(dummyClusterNode_p,
                                                   dummyBool));
    }
}

static void test2_ClusterNode()
// ------------------------------------------------------------------------
// PROTOCOL TEST:
//   Ensure this class is a properly defined protocol.
//
// Concerns:
//: 1 The protocol is abstract: no objects of it can be created.
//:
//: 2 The protocol has no data members.
//:
//: 3 The protocol has a virtual destructor.
//:
//: 4 All methods of the protocol are pure virtual.
//:
//: 5 All methods of the protocol are publicly accessible.
//
// Plan:
//: 1 Define a concrete derived implementation, 'ClusterNodeTestImp', of
//:   the protocol.
//:
//: 2 Create an object of the 'bsls::ProtocolTest' class template
//:   parameterized by 'ClusterNodeTestImp', and use it to verify that:
//:
//:   1 The protocol is abstract. (C-1)
//:
//:   2 The protocol has no data members. (C-2)
//:
//:   3 The protocol has a virtual destructor. (C-3)
//:
//: 3 Use the 'BSLS_PROTOCOLTEST_ASSERT' macro to verify that
//:   non-creator methods of the protocol are:
//:
//:   1 virtual, (C-4)
//:
//:   2 publicly accessible. (C-5)
//
// Testing:
//   PROTOCOL TEST
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("ClusterNode");

    PV("Creating a test object");
    bsls::ProtocolTest<ClusterNodeTestImp> testObj(
        bmqtst::TestHelperUtil::verbosityLevel() > 2);

    PV("Verify that the protocol is abstract");
    BMQTST_ASSERT(testObj.testAbstract());

    PV("Verify that there are no data members");
    BMQTST_ASSERT(testObj.testNoDataMembers());

    PV("Verify that the destructor is virtual");
    BMQTST_ASSERT(testObj.testVirtualDestructor());

    {
        PV("Verify that methods are public and virtual");

        bsl::weak_ptr<bmqio::Channel> dummyWeakChannel;
        bsl::shared_ptr<bdlbb::Blob>  dummyBlob_sp;
        bmqp_ctrlmsg::ClientIdentity  identity;

        BSLS_PROTOCOLTEST_ASSERT(testObj,
                                 setChannel(dummyWeakChannel,
                                            identity,
                                            bmqio::Channel::ReadCallback()));
        BSLS_PROTOCOLTEST_ASSERT(testObj, enableRead());
        BSLS_PROTOCOLTEST_ASSERT(testObj, closeChannel());
        BSLS_PROTOCOLTEST_ASSERT(testObj, resetChannel());
        BSLS_PROTOCOLTEST_ASSERT(testObj,
                                 write(dummyBlob_sp,
                                       bmqp::EventType::e_CONTROL));
        BSLS_PROTOCOLTEST_ASSERT(testObj, channel());
        BSLS_PROTOCOLTEST_ASSERT(testObj, identity());
        BSLS_PROTOCOLTEST_ASSERT(testObj, nodeId());
        BSLS_PROTOCOLTEST_ASSERT(testObj, hostName());
        BSLS_PROTOCOLTEST_ASSERT(testObj, nodeDescription());
        BSLS_PROTOCOLTEST_ASSERT(testObj, dataCenter());
        BSLS_PROTOCOLTEST_ASSERT(testObj, cluster());
        BSLS_PROTOCOLTEST_ASSERT(testObj, isAvailable());
    }
}

static void test3_Cluster()
// ------------------------------------------------------------------------
// PROTOCOL TEST:
//   Ensure this class is a properly defined protocol.
//
// Concerns:
//: 1 The protocol is abstract: no objects of it can be created.
//:
//: 2 The protocol has no data members.
//:
//: 3 The protocol has a virtual destructor.
//:
//: 4 All methods of the protocol are pure virtual.
//:
//: 5 All methods of the protocol are publicly accessible.
//
// Plan:
//: 1 Define a concrete derived implementation, 'ClusterTestImp', of the
//:   protocol.
//:
//: 2 Create an object of the 'bsls::ProtocolTest' class template
//:   parameterized by 'ClusterTestImp', and use it to verify that:
//:
//:   1 The protocol is abstract. (C-1)
//:
//:   2 The protocol has no data members. (C-2)
//:
//:   3 The protocol has a virtual destructor. (C-3)
//:
//: 3 Use the 'BSLS_PROTOCOLTEST_ASSERT' macro to verify that
//:   non-creator methods of the protocol are:
//:
//:   1 virtual, (C-4)
//:
//:   2 publicly accessible. (C-5)
//
// Testing:
//   PROTOCOL TEST
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("Cluster");

    PV("Creating a test object");
    bsls::ProtocolTest<ClusterTestImp> testObj(
        bmqtst::TestHelperUtil::verbosityLevel() > 2);

    PV("Verify that the protocol is abstract");
    BMQTST_ASSERT(testObj.testAbstract());

    PV("Verify that there are no data members");
    BMQTST_ASSERT(testObj.testNoDataMembers());

    PV("Verify that the destructor is virtual");
    BMQTST_ASSERT(testObj.testVirtualDestructor());

    {
        PV("Verify that methods are public and virtual");

        mqbnet::ClusterObserver*     dummyClusterObserver_p = 0;
        bsl::shared_ptr<bdlbb::Blob> dummyBlob_sp;
        int                          dummyInt = 0;

        BSLS_PROTOCOLTEST_ASSERT(testObj,
                                 registerObserver(dummyClusterObserver_p));
        BSLS_PROTOCOLTEST_ASSERT(testObj,
                                 unregisterObserver(dummyClusterObserver_p));
        BSLS_PROTOCOLTEST_ASSERT(testObj,
                                 writeAll(dummyBlob_sp,
                                          bmqp::EventType::e_CONTROL));
        BSLS_PROTOCOLTEST_ASSERT(testObj, broadcast(dummyBlob_sp));
        BSLS_PROTOCOLTEST_ASSERT(testObj, closeChannels());
        BSLS_PROTOCOLTEST_ASSERT(testObj, lookupNode(dummyInt));
        BSLS_PROTOCOLTEST_ASSERT(testObj, enableRead());
        BSLS_PROTOCOLTEST_ASSERT(
            testObj,
            onProxyConnectionUp(bsl::shared_ptr<bmqio::Channel>(),
                                bmqp_ctrlmsg::ClientIdentity(),
                                bsl::string()));
        BSLS_PROTOCOLTEST_ASSERT(testObj, nodes());
        BSLS_PROTOCOLTEST_ASSERT(testObj, name());
        BSLS_PROTOCOLTEST_ASSERT(testObj, selfNodeId());
        BSLS_PROTOCOLTEST_ASSERT(testObj, selfNode());

        // ProtocolTest doesn't allow testing the overload 'const nodes()'
        // accessor.
        // BSLS_PROTOCOLTEST_ASSERT(testObj, nodes());
    }

    PV("Ensure constant value");
    BMQTST_ASSERT_EQ(mqbnet::Cluster::k_INVALID_NODE_ID, -1);
    BMQTST_ASSERT_EQ(mqbnet::Cluster::k_ALL_NODES_ID,
                     bsl::numeric_limits<int>::min());
    BMQTST_ASSERT_NE(mqbnet::Cluster::k_INVALID_NODE_ID,
                     mqbnet::Cluster::k_ALL_NODES_ID);
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 3: test3_Cluster(); break;
    case 2: test2_ClusterNode(); break;
    case 1: test1_ClusterObserver(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
