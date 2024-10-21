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

// mqbnet_session.t.cpp                                               -*-C++-*-
#include <mqbnet_session.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>

#include <bmqio_channel.h>

// BDE
#include <bsls_platform.h>
#include <bsls_protocoltest.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

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
//  mqbnet_session.t.cpp:50:1: error: 'SessionTestImp' has no out-of-line
//  virtual method definitions; its vtable will be emitted in
//  every translation unit [-Werror,-Wweak-vtables]
//..
#endif  // BSLS_PLATFORM_CMP_CLANG

// ============================================================================
//                 HELPER CLASSES AND FUNCTIONS FOR TESTING
// ----------------------------------------------------------------------------

/// A test implementation of the `mqbnet::SessionEventProcessor` protocol
struct SessionEventProcessorTestImp
: bsls::ProtocolTestImp<mqbnet::SessionEventProcessor> {
    void processEvent(const bmqp::Event&   event,
                      mqbnet::ClusterNode* source = 0) BSLS_KEYWORD_OVERRIDE
    {
        markDone();
    }
};

/// A test implementation of the `mqbnet::Session` protocol
struct SessionTestImp : bsls::ProtocolTestImp<mqbnet::Session> {
    void tearDown(const bsl::shared_ptr<void>& handle,
                  bool isShutdown) BSLS_KEYWORD_OVERRIDE
    {
        markDone();
    }

    void initiateShutdown(const ShutdownCb&         callback,
                          const bsls::TimeInterval& timeout,
                          bool supportShutdownV2 = false) BSLS_KEYWORD_OVERRIDE
    {
        markDone();
    }

    void invalidate() BSLS_KEYWORD_OVERRIDE { markDone(); }

    bsl::shared_ptr<bmqio::Channel> channel() const BSLS_KEYWORD_OVERRIDE
    {
        return markDone();
    }

    mqbnet::ClusterNode* clusterNode() const BSLS_KEYWORD_OVERRIDE
    {
        return markDone();
    }

    const bmqp_ctrlmsg::NegotiationMessage&
    negotiationMessage() const BSLS_KEYWORD_OVERRIDE
    {
        return markDoneRef();
    }

    const bsl::string& description() const BSLS_KEYWORD_OVERRIDE
    {
        return markDoneRef();
    }

    // SessionEventProcessor
    void processEvent(const bmqp::Event&   event,
                      mqbnet::ClusterNode* source = 0) BSLS_KEYWORD_OVERRIDE
    {
        markDone();
    }
};

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_SessionEventProcessor()
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
//: 1 Define a concrete derived implementation,
//:   'SessionEventProcessorTestImp', of the protocol.
//:
//: 2 Create an object of the 'bsls::ProtocolTest' class template
//:   parameterized by 'SessionEventProcessorTestImp', and use it to verify
//:   that:
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
    bmqtst::TestHelper::printTestName("SessionEventProcessor");

    PV("Creating a test object");
    bsls::ProtocolTest<SessionEventProcessorTestImp> testObj(s_verbosityLevel >
                                                             2);

    PV("Verify that the protocol is abstract");
    ASSERT(testObj.testAbstract());

    PV("Verify that there are no data members");
    ASSERT(testObj.testNoDataMembers());

    PV("Verify that the destructor is virtual");
    ASSERT(testObj.testVirtualDestructor());

    {
        PV("Verify that methods are public and virtual");

        bmqp::Event          dummyEvent(0);
        mqbnet::ClusterNode* dummyClusterNode_p = 0;

        BSLS_PROTOCOLTEST_ASSERT(testObj,
                                 processEvent(dummyEvent, dummyClusterNode_p));
    }
}

static void test2_Session()
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
//: 1 Define a concrete derived implementation, 'SessionTestImp', of the
//:   protocol.
//:
//: 2 Create an object of the 'bsls::ProtocolTest' class template
//:   parameterized by 'SessionTestImp', and use it to verify that:
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
    bmqtst::TestHelper::printTestName("Session");

    PV("Creating a test object");
    bsls::ProtocolTest<SessionTestImp> testObj(s_verbosityLevel > 2);

    PV("Verify that the protocol is abstract");
    ASSERT(testObj.testAbstract());

    PV("Verify that there are no data members");
    ASSERT(testObj.testNoDataMembers());

    PV("Verify that the destructor is virtual");
    ASSERT(testObj.testVirtualDestructor());

    {
        PV("Verify that methods are public and virtual");

        bsl::shared_ptr<void> dummyHandle;
        bmqp::Event           dummyEvent(0);
        mqbnet::ClusterNode*  dummyClusterNode_p = 0;

        BSLS_PROTOCOLTEST_ASSERT(testObj, tearDown(dummyHandle, false));
        BSLS_PROTOCOLTEST_ASSERT(testObj, channel());
        BSLS_PROTOCOLTEST_ASSERT(testObj, clusterNode());
        BSLS_PROTOCOLTEST_ASSERT(testObj, negotiationMessage());
        BSLS_PROTOCOLTEST_ASSERT(testObj, description());
        BSLS_PROTOCOLTEST_ASSERT(testObj, invalidate());
        BSLS_PROTOCOLTEST_ASSERT(testObj,
                                 processEvent(dummyEvent, dummyClusterNode_p));
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
    case 2: test2_Session(); break;
    case 1: test1_SessionEventProcessor(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        s_testStatus = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
