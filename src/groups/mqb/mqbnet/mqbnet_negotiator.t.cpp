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
#include <mqbnet_initialconnectioncontext.h>
#include <mqbnet_negotiationcontext.h>
#include <mqbnet_session.h>

// BMQ
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
    int createSessionOnMsgType(
        bsl::ostream&                                      errorDescription,
        bsl::shared_ptr<mqbnet::Session>*                  session,
        bool*                                              isContinueRead,
        const bsl::shared_ptr<mqbnet::NegotiationContext>& context)
        BSLS_KEYWORD_OVERRIDE
    {
        markDone();
        return 0;
    }

    int negotiateOutboundOrReverse(
        bsl::ostream&                                      errorDescription,
        const bsl::shared_ptr<mqbnet::NegotiationContext>& negotiationContext)
        BSLS_KEYWORD_OVERRIDE
    {
        markDone();
        return 0;
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
    bmqtst::TestHelper::printTestName("Negotiator");

    PV("Creating a test object");
    bsls::ProtocolTest<NegotiatorTestImp> testObj(
        bmqtst::TestHelperUtil::verbosityLevel() > 2);

    PV("Verify that the protocol is abstract");
    BMQTST_ASSERT(testObj.testAbstract());

    PV("Verify that there are no data members");
    BMQTST_ASSERT(testObj.testNoDataMembers());

    PV("Verify that the destructor is virtual");
    BMQTST_ASSERT(testObj.testVirtualDestructor());

    {
        PV("Verify that methods are public and virtual");

        bsl::shared_ptr<mqbnet::NegotiationContext> dummyNegotiationContextSp;
        bsl::shared_ptr<mqbnet::Session>            dummySessionSp;
        bmqu::MemOutStream                          errStream;
        bool                                        isContinueRead = false;

        BSLS_PROTOCOLTEST_ASSERT(
            testObj,
            negotiateOutboundOrReverse(errStream, dummyNegotiationContextSp));

        BSLS_PROTOCOLTEST_ASSERT(
            testObj,
            createSessionOnMsgType(errStream,
                                   &dummySessionSp,
                                   &isContinueRead,
                                   dummyNegotiationContextSp));
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
    case 1: test1_Negotiator(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
