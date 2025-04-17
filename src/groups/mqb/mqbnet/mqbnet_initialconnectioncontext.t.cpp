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

// mqbnet_initialconnectioncontext.t.cpp                 -*-C++-*-
#include <mqbnet_initialconnectioncontext.h>

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

// ============================================================================
//                 HELPER CLASSES AND FUNCTIONS FOR TESTING
// ----------------------------------------------------------------------------

/// A mock implementation of SessionEventProcessor protocol, for testing
/// `InitialConnectionContext`.
class MockSessionEventProcessor : public mqbnet::SessionEventProcessor {
  public:
    ~MockSessionEventProcessor() BSLS_KEYWORD_OVERRIDE = default;

    void processEvent(const bmqp::Event&   event,
                      mqbnet::ClusterNode* source) BSLS_KEYWORD_OVERRIDE;
};

void MockSessionEventProcessor::processEvent(const bmqp::Event&   event,
                                             mqbnet::ClusterNode* source)
{
}

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_initialConnectionContext()
{
    bmqtst::TestHelper::printTestName("test1_initialConnectionContext");
    {
        PV("Constructor");
        mqbnet::InitialConnectionContext obj1(true);
        BMQTST_ASSERT_EQ(obj1.isIncoming(), true);
        BMQTST_ASSERT_EQ(obj1.resultState(), static_cast<void*>(0));
        BMQTST_ASSERT_EQ(obj1.userData(), static_cast<void*>(0));

        mqbnet::InitialConnectionContext obj2(false);
        BMQTST_ASSERT_EQ(obj2.isIncoming(), false);
    }

    {
        PV("Manipulators/Accessors");

        mqbnet::InitialConnectionContext obj(true);

        {  // UserData
            int value = 7;
            BMQTST_ASSERT_EQ(&(obj.setUserData(&value)), &obj);
            BMQTST_ASSERT_EQ(obj.userData(), &value);
        }

        {  // ResultState
            int value = 9;
            BMQTST_ASSERT_EQ(&(obj.setResultState(&value)), &obj);
            BMQTST_ASSERT_EQ(obj.resultState(), &value);
        }
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
    case 1: test1_initialConnectionContext(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
