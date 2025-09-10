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
#include <bmqio_testchannel.h>

// BDE
#include <bsls_platform.h>
#include <bsls_protocoltest.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

namespace {

void complete(const bsl::shared_ptr<int>&             check,
              int                                     status,
              const bsl::string&                      errorDescription,
              const bsl::shared_ptr<mqbnet::Session>& session,
              const bsl::shared_ptr<bmqio::Channel>&  channel,
              const mqbnet::InitialConnectionContext* initialConnectionContext)
{
    BSLS_ASSERT_SAFE(check);

    BSLS_ASSERT_SAFE(*check == 0);

    *check = status;

    (void)errorDescription;
    (void)session;
    (void)channel;
    (void)initialConnectionContext;
}

}  // close unnamed namespace

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

        {  // Channel
            bsl::shared_ptr<bmqio::TestChannel> channel;
            bslma::Allocator* allocator = bmqtst::TestHelperUtil::allocator();
            channel.createInplace(allocator);
            BMQTST_ASSERT_EQ(&(obj.setChannel(channel)), &obj);
            BMQTST_ASSERT_EQ(obj.channel(), channel);
        }

        {
            // CompletionCb

            bsl::shared_ptr<int> check;
            int                  rc     = 1;
            bslma::Allocator* allocator = bmqtst::TestHelperUtil::allocator();

            check.createInplace(allocator, 0);

            obj.setCompleteCb(bdlf::BindUtil::bind(
                &complete,
                check,
                bdlf::PlaceHolders::_1,  // status
                bdlf::PlaceHolders::_2,  // errorDescription
                bdlf::PlaceHolders::_3,  // session
                bdlf::PlaceHolders::_4,  // channel
                bdlf::PlaceHolders::_5   // initialConnectionContext
                ));
            obj.complete(rc,
                         bsl::string(),
                         bsl::shared_ptr<mqbnet::Session>());

            BMQTST_ASSERT_EQ(*check, rc);
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

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_GBL_ALLOC);
}
