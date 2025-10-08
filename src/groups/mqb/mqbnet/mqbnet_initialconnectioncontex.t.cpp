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
#include <bslstl_sharedptr.h>
#include <cstddef>
#include <mqbnet_initialconnectioncontext.h>

// MQB
#include <mqbnet_initialconnectioncontext.h>
#include <mqbnet_negotiationcontext.h>
#include <mqbnet_session.h>

// BMQ
#include <bmqio_testchannel.h>

// BDE
#include <bsl_memory.h>
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

// Mock authenticator
struct MockAuthenticator : public mqbnet::Authenticator {
  private:
    bsl::optional<mqbcfg::Credential> d_anonymousCredential;

  public:
    int  start(bsl::ostream&) BSLS_KEYWORD_OVERRIDE { return 0; }
    void stop() BSLS_KEYWORD_OVERRIDE {}
    int  handleAuthentication(
         bsl::ostream&,
         const bsl::shared_ptr<mqbnet::InitialConnectionContext>&,
         const bmqp_ctrlmsg::AuthenticationMessage&) BSLS_KEYWORD_OVERRIDE
    {
        return 0;
    }
    int authenticationOutbound(
        const bsl::shared_ptr<mqbnet::AuthenticationContext>&)
        BSLS_KEYWORD_OVERRIDE
    {
        return 0;
    }
    int reauthenticateAsync(
        bsl::ostream&,
        const bsl::shared_ptr<mqbnet::AuthenticationContext>&,
        const bsl::shared_ptr<bmqio::Channel>&) BSLS_KEYWORD_OVERRIDE
    {
        return 0;
    }
    const bsl::optional<mqbcfg::Credential>&
    anonymousCredential() const BSLS_KEYWORD_OVERRIDE
    {
        return d_anonymousCredential;
    }
};

// Mock negotiator
struct MockNegotiator : public mqbnet::Negotiator {
    int createSessionOnMsgType(bsl::ostream&,
                               bsl::shared_ptr<mqbnet::Session>*,
                               mqbnet::InitialConnectionContext*)
        BSLS_KEYWORD_OVERRIDE
    {
        return 0;
    }
    int
    negotiateOutbound(bsl::ostream&,
                      const bsl::shared_ptr<mqbnet::InitialConnectionContext>&)
        BSLS_KEYWORD_OVERRIDE
    {
        return 0;
    }
};

}  // close unnamed namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_initialConnectionContext()
{
    bslma::Allocator* alloc = bmqtst::TestHelperUtil::allocator();
    bsl::shared_ptr<MockAuthenticator> authenticator =
        bsl::allocate_shared<MockAuthenticator>(alloc);
    bsl::shared_ptr<MockNegotiator> negotiator =
        bsl::allocate_shared<MockNegotiator>(alloc);
    bsl::shared_ptr<bmqio::TestChannel> channel =
        bsl::allocate_shared<bmqio::TestChannel>(alloc);

    bsl::shared_ptr<int> check = bsl::allocate_shared<int>(alloc, 0);
    mqbnet::InitialConnectionContext::InitialConnectionCompleteCb completeCb =
        bdlf::BindUtil::bind(
            &complete,
            check,
            bdlf::PlaceHolders::_1,  // status
            bdlf::PlaceHolders::_2,  // errorDescription
            bdlf::PlaceHolders::_3,  // session
            bdlf::PlaceHolders::_4,  // channel
            bdlf::PlaceHolders::_5   // initialConnectionContext
        );

    bmqtst::TestHelper::printTestName("test1_basicConstruction");
    {
        PV("Constructor");
        mqbnet::InitialConnectionContext obj1(false,
                                              authenticator.get(),
                                              negotiator.get(),
                                              static_cast<void*>(0),
                                              static_cast<void*>(0),
                                              channel,
                                              completeCb);
        BMQTST_ASSERT_EQ(obj1.isIncoming(), false);
        BMQTST_ASSERT_EQ(obj1.resultState(), static_cast<void*>(0));
        BMQTST_ASSERT_EQ(obj1.userData(), static_cast<void*>(0));
    }

    {
        PV("Manipulators/Accessors");

        mqbnet::InitialConnectionContext obj(true,
                                             authenticator.get(),
                                             negotiator.get(),
                                             static_cast<void*>(0),
                                             static_cast<void*>(0),
                                             channel,
                                             completeCb);

        {  // ResultState
            int value = 9;
            obj.setResultState(&value);
            BMQTST_ASSERT_EQ(obj.resultState(), &value);
        }

        {  // AuthenticationContext
            bsl::shared_ptr<mqbnet::AuthenticationContext> authnCtx =
                bsl::allocate_shared<mqbnet::AuthenticationContext>(alloc);
            obj.setAuthenticationContext(authnCtx);
            BMQTST_ASSERT_EQ(authnCtx, obj.authenticationContext());
        }

        {
            // CompletionCb
            int rc = 1;
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
