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

#include <mqbnet_initialconnectioncontext.h>

// MQB
#include <mqbnet_authenticationclient.h>
#include <mqbnet_initialconnectioncontext.h>
#include <mqbnet_negotiationcontext.h>
#include <mqbnet_session.h>

// BMQ
#include <bmqio_testchannel.h>
#include <bmqp_ctrlmsg_messages.h>

// BDE
#include <bsl_memory.h>
#include <bsl_ostream.h>
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
              const bsl::shared_ptr<bmqio::Channel>&  channel)
{
    BSLS_ASSERT_SAFE(check);

    BSLS_ASSERT_SAFE(*check == 0);

    *check = status;

    (void)errorDescription;
    (void)session;
    (void)channel;
}

// Mock authentication client
struct MockAuthenticationClient : public mqbnet::AuthenticationClient {
    bool d_authenticateCalled;
    int  d_authenticateRc;
    bool d_handleResponseCalled;
    int  d_handleResponseRc;
    bool d_stopCalled;

    int authenticate(bsl::ostream&) BSLS_KEYWORD_OVERRIDE
    {
        d_authenticateCalled = true;
        return d_authenticateRc;
    }

    int handleResponse(bsl::ostream&,
                       const bmqp_ctrlmsg::AuthenticationMessage&)
        BSLS_KEYWORD_OVERRIDE
    {
        d_handleResponseCalled = true;
        return d_handleResponseRc;
    }
};

// Mock authenticator
struct MockAuthenticator : public mqbnet::Authenticator {
    bsl::optional<mqbcfg::Credential>         d_anonymousCredential;
    bool                                      d_hasOutboundAuthentication;
    int                                       d_authenticateRc;
    int                                       d_handleResponseRc;
    bsl::shared_ptr<MockAuthenticationClient> d_mockClient;
    bool                                      d_negotiateOutboundCalled;

    int  start(bsl::ostream&) BSLS_KEYWORD_OVERRIDE { return 0; }
    void stop() BSLS_KEYWORD_OVERRIDE {}
    int  handleAuthentication(bsl::ostream&,
                              mqbnet::InitialConnectionContext*,
                              const bmqp_ctrlmsg::AuthenticationMessage&)
        BSLS_KEYWORD_OVERRIDE
    {
        return 0;
    }
    int handleReauthentication(
        bsl::ostream&,
        const bsl::shared_ptr<mqbnet::AuthenticationContext>&,
        const bsl::shared_ptr<bmqio::Channel>&) BSLS_KEYWORD_OVERRIDE
    {
        return 0;
    }

    bool hasOutboundAuthentication() const BSLS_KEYWORD_OVERRIDE
    {
        return d_hasOutboundAuthentication;
    }

    bsl::shared_ptr<mqbnet::AuthenticationClient> createAuthenticationClient(
        const bsl::shared_ptr<bmqio::Channel>&,
        bslma::Allocator* allocator) const BSLS_KEYWORD_OVERRIDE
    {
        // const_cast because the mock needs to store the client for later
        // inspection by the test, but the interface is const.
        MockAuthenticator* self = const_cast<MockAuthenticator*>(this);
        self->d_mockClient = bsl::allocate_shared<MockAuthenticationClient>(
            allocator);
        self->d_mockClient->d_authenticateRc   = d_authenticateRc;
        self->d_mockClient->d_handleResponseRc = d_handleResponseRc;
        return self->d_mockClient;
    }

    const bsl::optional<mqbcfg::Credential>&
    anonymousCredential() const BSLS_KEYWORD_OVERRIDE
    {
        return d_anonymousCredential;
    }
};

// Mock negotiator
struct MockNegotiator : public mqbnet::Negotiator {
    bool d_negotiateOutboundCalled;

    int createSessionOnMsgType(bsl::ostream&,
                               bsl::shared_ptr<mqbnet::Session>*,
                               mqbnet::InitialConnectionContext*)
        BSLS_KEYWORD_OVERRIDE
    {
        return 0;
    }
    int negotiateOutbound(bsl::ostream&, mqbnet::InitialConnectionContext*)
        BSLS_KEYWORD_OVERRIDE
    {
        d_negotiateOutboundCalled = true;
        return 0;
    }
};

struct TestBench {
    bsl::shared_ptr<MockAuthenticator>  d_authenticator;
    bsl::shared_ptr<MockNegotiator>     d_negotiator;
    bsl::shared_ptr<bmqio::TestChannel> d_channel;
    bsl::shared_ptr<int>                d_completionStatus;

    mqbnet::InitialConnectionContext::InitialConnectionCompleteCb d_completeCb;

    explicit TestBench(bslma::Allocator* allocator)
    : d_authenticator(bsl::allocate_shared<MockAuthenticator>(allocator))
    , d_negotiator(bsl::allocate_shared<MockNegotiator>(allocator))
    , d_channel(bsl::allocate_shared<bmqio::TestChannel>(allocator))
    , d_completionStatus(bsl::allocate_shared<int>(allocator, 0))
    , d_completeCb(bdlf::BindUtil::bind(&complete,
                                        d_completionStatus,
                                        bdlf::PlaceHolders::_1,
                                        bdlf::PlaceHolders::_2,
                                        bdlf::PlaceHolders::_3,
                                        bdlf::PlaceHolders::_4))
    {
    }
};

}  // close unnamed namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_initialConnectionContext()
{
    bslma::Allocator* alloc = bmqtst::TestHelperUtil::allocator();
    TestBench         tb(alloc);

    bmqtst::TestHelper::printTestName("test1_basicConstruction");
    {
        PV("Constructor");
        mqbnet::InitialConnectionContext obj1(false,
                                              0,
                                              0,
                                              static_cast<void*>(0),
                                              static_cast<void*>(0),
                                              tb.d_channel,
                                              tb.d_completeCb,
                                              alloc);
        BMQTST_ASSERT_EQ(obj1.isIncoming(), false);
        BMQTST_ASSERT_EQ(obj1.resultState(), static_cast<void*>(0));
        BMQTST_ASSERT_EQ(obj1.userData(), static_cast<void*>(0));
    }

    {
        PV("Manipulators/Accessors");

        mqbnet::InitialConnectionContext obj(true,
                                             tb.d_authenticator.get(),
                                             tb.d_negotiator.get(),
                                             static_cast<void*>(0),
                                             static_cast<void*>(0),
                                             tb.d_channel,
                                             tb.d_completeCb,
                                             alloc);

        {  // ResultState
            int value = 9;
            obj.setResultState(&value);
            BMQTST_ASSERT_EQ(obj.resultState(), &value);
        }

        {  // AuthenticationContext
            bmqp_ctrlmsg::AuthenticationMessage            authnMsg;
            bsl::shared_ptr<mqbnet::AuthenticationContext> authnCtx =
                bsl::allocate_shared<mqbnet::AuthenticationContext>(
                    alloc,
                    &obj,
                    "testMechanism",
                    authnMsg,
                    bmqp::EncodingType::e_BER,
                    mqbnet::AuthenticationState::e_AUTHENTICATING);
            obj.setAuthenticationContext(authnCtx);
            BMQTST_ASSERT_EQ(authnCtx, obj.authenticationContext());
        }

        {
            // CompletionCb
            int rc = 1;
            obj.complete(rc,
                         bsl::string(),
                         bsl::shared_ptr<mqbnet::Session>());

            BMQTST_ASSERT_EQ(*tb.d_completionStatus, rc);
        }
    }
}

static void test2_outboundAuthenticate()
// ------------------------------------------------------------------------
// OUTBOUND AUTHENTICATE
//
// Concerns:
//   - 'handleInitialConnection()' on an outbound context with
//     authentication enters 'e_AUTHENTICATING_OUTBOUND'.
//   - 'authenticate()' is called on the created client.
//   - After receiving a successful 'e_AUTHN_MESSAGE', the FSM
//     transitions to 'e_NEGOTIATING_OUTBOUND'.
//   - 'handleResponse()' is called on the client.
//   - 'negotiateOutbound()' is called on the negotiator.
//
// Plan:
//   1) Configure MockAuthenticator with hasOutboundAuthentication = true.
//   2) Construct an outbound context, call 'handleInitialConnection()'.
//   3) Verify state is e_AUTHENTICATING_OUTBOUND and authenticate() was
//      called.
//   4) Fire e_AUTHN_MESSAGE event with a success AuthenticationResponse.
//   5) Verify state is e_NEGOTIATING_OUTBOUND, handleResponse() was
//      called, and negotiateOutbound() was called.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("OUTBOUND AUTHENTICATE");

    bslma::Allocator* alloc = bmqtst::TestHelperUtil::allocator();
    TestBench         tb(alloc);

    // 1)
    tb.d_authenticator->d_hasOutboundAuthentication = true;

    // 2)
    mqbnet::InitialConnectionContext ctx(false,
                                         tb.d_authenticator.get(),
                                         tb.d_negotiator.get(),
                                         static_cast<void*>(0),
                                         static_cast<void*>(0),
                                         tb.d_channel,
                                         tb.d_completeCb,
                                         alloc);
    ctx.handleInitialConnection();

    // 3)
    BMQTST_ASSERT_EQ(
        ctx.state(),
        mqbnet::InitialConnectionState::e_AUTHENTICATING_OUTBOUND);
    BMQTST_ASSERT(tb.d_authenticator->d_mockClient);
    BMQTST_ASSERT(tb.d_authenticator->d_mockClient->d_authenticateCalled);

    // 4)
    bmqp_ctrlmsg::AuthenticationMessage   response;
    bmqp_ctrlmsg::AuthenticationResponse& resp =
        response.makeAuthenticationResponse();
    resp.status().category() = bmqp_ctrlmsg::StatusCategory::E_SUCCESS;
    resp.status().code()     = 0;
    resp.status().message()  = "OK";

    ctx.handleEvent(bsl::string(),
                    mqbnet::InitialConnectionEvent::e_AUTHN_MESSAGE,
                    response);

    // 5)
    BMQTST_ASSERT_EQ(ctx.state(),
                     mqbnet::InitialConnectionState::e_NEGOTIATING_OUTBOUND);
    BMQTST_ASSERT(tb.d_authenticator->d_mockClient->d_handleResponseCalled);
    BMQTST_ASSERT(tb.d_negotiator->d_negotiateOutboundCalled);
}

static void test3_outboundAuthenticateFailure()
// ------------------------------------------------------------------------
// OUTBOUND AUTHENTICATE FAILURE
//
// Concerns:
//   - When 'authenticate()' fails, the FSM transitions to 'e_FAILED'.
//   - The completion callback is invoked with a non-zero status.
//   - 'negotiateOutbound()' is never called.
//
// Plan:
//   1) Configure MockAuthenticator with hasOutboundAuthentication = true
//      and the mock client's authenticateRc = -1.
//   2) Construct an outbound context, call 'handleInitialConnection()'.
//   3) Verify state is e_FAILED, the completion callback fired with
//      non-zero status, and negotiateOutbound() was not called.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("OUTBOUND AUTHENTICATE FAILURE");

    bslma::Allocator* alloc = bmqtst::TestHelperUtil::allocator();
    TestBench         tb(alloc);

    // 1)
    tb.d_authenticator->d_hasOutboundAuthentication = true;
    tb.d_authenticator->d_authenticateRc            = -1;

    // 2)
    mqbnet::InitialConnectionContext ctx(false,
                                         tb.d_authenticator.get(),
                                         tb.d_negotiator.get(),
                                         static_cast<void*>(0),
                                         static_cast<void*>(0),
                                         tb.d_channel,
                                         tb.d_completeCb,
                                         alloc);
    ctx.handleInitialConnection();

    // 3)
    BMQTST_ASSERT_EQ(ctx.state(), mqbnet::InitialConnectionState::e_FAILED);
    BMQTST_ASSERT_NE(*tb.d_completionStatus, 0);
    BMQTST_ASSERT(!tb.d_negotiator->d_negotiateOutboundCalled);
}

static void test4_outboundHandleResponseFailure()
// ------------------------------------------------------------------------
// OUTBOUND HANDLE RESPONSE FAILURE
//
// Concerns:
//   - When 'authenticate()' succeeds but 'handleResponse()' fails, the
//     FSM transitions to 'e_FAILED'.
//   - The completion callback is invoked with a non-zero status.
//   - 'negotiateOutbound()' is never called.
//
// Plan:
//   1) Configure MockAuthenticator with hasOutboundAuthentication = true
//      and the mock client's handleResponseRc = -1.
//   2) Construct an outbound context, call 'handleInitialConnection()'.
//   3) Verify state is e_AUTHENTICATING_OUTBOUND (authenticate succeeded).
//   4) Fire e_AUTHN_MESSAGE with an AuthenticationResponse.
//   5) Verify state is e_FAILED, completion callback fired with non-zero
//      status, and negotiateOutbound() was not called.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("OUTBOUND HANDLE RESPONSE FAILURE");

    bslma::Allocator* alloc = bmqtst::TestHelperUtil::allocator();
    TestBench         tb(alloc);

    // 1)
    tb.d_authenticator->d_hasOutboundAuthentication = true;
    tb.d_authenticator->d_handleResponseRc          = -1;

    // 2)
    mqbnet::InitialConnectionContext ctx(false,
                                         tb.d_authenticator.get(),
                                         tb.d_negotiator.get(),
                                         static_cast<void*>(0),
                                         static_cast<void*>(0),
                                         tb.d_channel,
                                         tb.d_completeCb,
                                         alloc);
    ctx.handleInitialConnection();

    // 3)
    BMQTST_ASSERT_EQ(
        ctx.state(),
        mqbnet::InitialConnectionState::e_AUTHENTICATING_OUTBOUND);
    BMQTST_ASSERT(tb.d_authenticator->d_mockClient->d_authenticateCalled);

    // 4)
    bmqp_ctrlmsg::AuthenticationMessage response;
    response.makeAuthenticationResponse();

    ctx.handleEvent(bsl::string(),
                    mqbnet::InitialConnectionEvent::e_AUTHN_MESSAGE,
                    response);

    // 5)
    BMQTST_ASSERT_EQ(ctx.state(), mqbnet::InitialConnectionState::e_FAILED);
    BMQTST_ASSERT(tb.d_authenticator->d_mockClient->d_handleResponseCalled);
    BMQTST_ASSERT_NE(*tb.d_completionStatus, 0);
    BMQTST_ASSERT(!tb.d_negotiator->d_negotiateOutboundCalled);
}

static void test5_outboundNoAuthentication()
// ------------------------------------------------------------------------
// OUTBOUND NO AUTHENTICATION
//
// Concerns:
//   - When 'hasOutboundAuthentication()' is false,
//     'handleInitialConnection()' skips authentication and goes directly
//     to 'e_NEGOTIATING_OUTBOUND'.
//   - No AuthenticationClient is created.
//   - 'negotiateOutbound()' is called.
//
// Plan:
//   1) Configure MockAuthenticator with hasOutboundAuthentication = false.
//   2) Construct an outbound context, call 'handleInitialConnection()'.
//   3) Verify state is e_NEGOTIATING_OUTBOUND, authenticationClient() is
//      null, and negotiateOutbound() was called.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("OUTBOUND NO AUTHENTICATION");

    bslma::Allocator* alloc = bmqtst::TestHelperUtil::allocator();
    TestBench         tb(alloc);

    // 1) d_hasOutboundAuthentication defaults to false

    // 2)
    mqbnet::InitialConnectionContext ctx(false,
                                         tb.d_authenticator.get(),
                                         tb.d_negotiator.get(),
                                         static_cast<void*>(0),
                                         static_cast<void*>(0),
                                         tb.d_channel,
                                         tb.d_completeCb,
                                         alloc);
    ctx.handleInitialConnection();

    // 3)
    BMQTST_ASSERT_EQ(ctx.state(),
                     mqbnet::InitialConnectionState::e_NEGOTIATING_OUTBOUND);
    BMQTST_ASSERT(!ctx.authenticationClient());
    BMQTST_ASSERT(tb.d_negotiator->d_negotiateOutboundCalled);
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 5: test5_outboundNoAuthentication(); break;
    case 4: test4_outboundHandleResponseFailure(); break;
    case 3: test3_outboundAuthenticateFailure(); break;
    case 2: test2_outboundAuthenticate(); break;
    case 1: test1_initialConnectionContext(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_GBL_ALLOC);
}
