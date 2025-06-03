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

// mqbnet_initialconnectionhandler.t.cpp                        -*-C++-*-
#include <mqbnet_initialconnectionhandler.h>

// MQB
#include <mqbnet_initialconnectioncontext.h>
#include <mqbnet_negotiationcontext.h>
#include <mqbnet_session.h>

// BMQ
#include <bmqio_channel.h>

// BDE
#include <bsls_nullptr.h>
#include <bsls_platform.h>
#include <bsls_protocoltest.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                 HELPER CLASSES AND FUNCTIONS FOR TESTING
// ----------------------------------------------------------------------------

/// A test implementation of the `mqbnet::InitialConnectionHandler` protocol
struct InitialConnectionHandlerTestImp
: bsls::ProtocolTestImp<mqbnet::InitialConnectionHandler> {
    int start(bsl::ostream& errorDescription) BSLS_KEYWORD_OVERRIDE;

    void stop() BSLS_KEYWORD_OVERRIDE;

    void handleInitialConnection(const InitialConnectionContextSp& context)
        BSLS_KEYWORD_OVERRIDE;
};

int InitialConnectionHandlerTestImp::start(
    BSLS_ANNOTATION_UNUSED bsl::ostream& errorDescription)
{
    markDone();
    return 0;
}

void InitialConnectionHandlerTestImp::stop()
{
    markDone();
}

void InitialConnectionHandlerTestImp::handleInitialConnection(
    BSLA_UNUSED const InitialConnectionContextSp& context)
{
    markDone();
}

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_InitialConnectionHandler()
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
//: 'InitialConnectionHandlerTestImp', of the
//:   protocol.
//:
//: 2 Create an object of the 'bsls::ProtocolTest' class template
//:   parameterized by 'InitialConnectionHandlerTestImp', and use it to verify
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
    bmqtst::TestHelper::printTestName("InitialConnectionHandler");

    PV("Creating a test object");
    bsls::ProtocolTest<InitialConnectionHandlerTestImp> testObj(
        bmqtst::TestHelperUtil::verbosityLevel() > 2);

    PV("Verify that the protocol is abstract");
    BMQTST_ASSERT(testObj.testAbstract());

    PV("Verify that there are no data members");
    BMQTST_ASSERT(testObj.testNoDataMembers());

    PV("Verify that the destructor is virtual");
    BMQTST_ASSERT(testObj.testVirtualDestructor());

    {
        PV("Verify that methods are public and virtual");

        mqbnet::InitialConnectionHandler::InitialConnectionContextSp
            dummyContext_sp;

        BSLS_PROTOCOLTEST_ASSERT(testObj,
                                 handleInitialConnection(dummyContext_sp));
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
    case 1: test1_InitialConnectionHandler(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
