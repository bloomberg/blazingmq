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

// mqbnet_negotiator.t.cpp                                            -*-C++-*-
#include <mqbnet_negotiator.h>

// MQB
#include <mqbnet_session.h>

// MWC
#include <mwcio_channel.h>

// BDE
#include <bsls_platform.h>
#include <bsls_protocoltest.h>

// TEST DRIVER
#include <mwctst_testhelper.h>

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
//  mqbnet_negotiator.t.cpp:50:1: error: 'NegotiatorTestImp' has no
//  out-of-line virtual method definitions; its vtable will be emitted in
//  every translation unit [-Werror,-Wweak-vtables]
//..
#endif  // BSLS_PLATFORM_CMP_CLANG

// ============================================================================
//                 HELPER CLASSES AND FUNCTIONS FOR TESTING
// ----------------------------------------------------------------------------

/// A test implementation of the `mqbnet::Negotiator` protocol
struct NegotiatorTestImp : bsls::ProtocolTestImp<mqbnet::Negotiator> {
    void negotiate(mqbnet::NegotiatorContext*               context,
                   const bsl::shared_ptr<mwcio::Channel>&   channel,
                   const mqbnet::Negotiator::NegotiationCb& negotiationCb)
        BSLS_KEYWORD_OVERRIDE
    {
        markDone();
    }
};

/// A mock implementation of SessionEventProcessor protocol, for testing
/// `NegotiatorContext`.
class MockSessionEventProcessor : public mqbnet::SessionEventProcessor {
  public:
    ~MockSessionEventProcessor() BSLS_KEYWORD_OVERRIDE {}

    void processEvent(const bmqp::Event&   event,
                      mqbnet::ClusterNode* source) BSLS_KEYWORD_OVERRIDE
    {
    }
};

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_Negotiator()
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
//: 1 Define a concrete derived implementation, 'NegotiatorTestImp', of the
//:   protocol.
//:
//: 2 Create an object of the 'bsls::ProtocolTest' class template
//:   parameterized by 'NegotiatorTestImp', and use it to verify that:
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
    mwctst::TestHelper::printTestName("Negotiator");

    PV("Creating a test object");
    bsls::ProtocolTest<NegotiatorTestImp> testObj(s_verbosityLevel > 2);

    PV("Verify that the protocol is abstract");
    ASSERT(testObj.testAbstract());

    PV("Verify that there are no data members");
    ASSERT(testObj.testNoDataMembers());

    PV("Verify that the destructor is virtual");
    ASSERT(testObj.testVirtualDestructor());

    {
        PV("Verify that methods are public and virtual");

        mqbnet::NegotiatorContext*        dummyNegotiatorContext_p = 0;
        bsl::shared_ptr<mwcio::Channel>   dummyChannelSp;
        mqbnet::Negotiator::NegotiationCb dummyNegotiationCb;

        BSLS_PROTOCOLTEST_ASSERT(testObj,
                                 negotiate(dummyNegotiatorContext_p,
                                           dummyChannelSp,
                                           dummyNegotiationCb));
    }
}

static void test2_NegotiatorContext()
{
    mwctst::TestHelper::printTestName("NegotiatorContext");

    {
        PV("Constructor");
        mqbnet::NegotiatorContext obj1(true);
        ASSERT_EQ(obj1.isIncoming(), true);
        ASSERT_EQ(obj1.maxMissedHeartbeat(), 0);
        ASSERT_EQ(obj1.eventProcessor(), static_cast<void*>(0));
        ASSERT_EQ(obj1.resultState(), static_cast<void*>(0));
        ASSERT_EQ(obj1.userData(), static_cast<void*>(0));

        mqbnet::NegotiatorContext obj2(false);
        ASSERT_EQ(obj2.isIncoming(), false);
    }

    {
        PV("Manipulators/Accessors");

        mqbnet::NegotiatorContext obj(true);

        {  // MaxMissedHeartbeat
            const char value = 5;
            ASSERT_EQ(&(obj.setMaxMissedHeartbeat(value)), &obj);
            ASSERT_EQ(obj.maxMissedHeartbeat(), value);
        }

        {  // UserData
            int value = 7;
            ASSERT_EQ(&(obj.setUserData(&value)), &obj);
            ASSERT_EQ(obj.userData(), &value);
        }

        {  // ResultState
            int value = 9;
            ASSERT_EQ(&(obj.setResultState(&value)), &obj);
            ASSERT_EQ(obj.resultState(), &value);
        }

        {  // EventProcessor
            MockSessionEventProcessor value;
            ASSERT_EQ(&(obj.setEventProcessor(&value)), &obj);
            ASSERT_EQ(obj.eventProcessor(), &value);
        }
    }
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(mwctst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 2: test2_NegotiatorContext(); break;
    case 1: test1_Negotiator(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        s_testStatus = -1;
    } break;
    }

    TEST_EPILOG(mwctst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
