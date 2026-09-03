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
#include <mqbnet_initialconnectioncontext.h>
#include <mqbnet_negotiationcontext.h>
#include <mqbnet_session.h>

// BMQ
#include <bmqio_testchannel.h>
#include <bmqp_ctrlmsg_messages.h>

// BDE
#include <bdlf_bind.h>
#include <bsl_memory.h>
#include <bsl_ostream.h>
#include <bsl_string.h>
#include <bsl_variant.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_platform.h>
#include <bsls_protocoltest.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

namespace {

/// Message payload carried through the initial connection state machine.
typedef bsl::variant<bsl::monostate,
                     bmqp_ctrlmsg::AuthenticationMessage,
                     bmqp_ctrlmsg::NegotiationMessage>
    InitialConnectionMessage;

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

/// Outcome recorded by the initial connection completion callback.
struct CompletionResult {
    int         d_numCalls;
    int         d_status;
    bsl::string d_error;

    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(CompletionResult, bslma::UsesBslmaAllocator)

    explicit CompletionResult(bslma::Allocator* allocator)
    : d_numCalls(0)
    , d_status(0)
    , d_error(allocator)
    {
    }
};

void onComplete(CompletionResult*                       result,
                int                                     status,
                const bsl::string&                      errorDescription,
                const bsl::shared_ptr<mqbnet::Session>& session,
                const bsl::shared_ptr<bmqio::Channel>&  channel)
{
    BSLS_ASSERT_SAFE(result);

    ++result->d_numCalls;
    result->d_status = status;
    result->d_error  = errorDescription;

    (void)session;
    (void)channel;
}

// Mock client-side authenticator
struct MockAuthenticationClient : public mqbnet::AuthenticationClient {
    int d_authenticateRc;
    int d_handleResponseRc;
    int d_numAuthenticateCalls;
    int d_numHandleResponseCalls;
    int d_numOnCloseCalls;

    MockAuthenticationClient()
    : d_authenticateRc(0)
    , d_handleResponseRc(0)
    , d_numAuthenticateCalls(0)
    , d_numHandleResponseCalls(0)
    , d_numOnCloseCalls(0)
    {
    }

    int authenticate(bsl::ostream& errorDescription) BSLS_KEYWORD_OVERRIDE
    {
        ++d_numAuthenticateCalls;
        if (d_authenticateRc != 0) {
            errorDescription << "failed to send AuthenticationRequest";
        }
        return d_authenticateRc;
    }

    int handleResponse(bsl::ostream& errorDescription,
                       const bmqp_ctrlmsg::AuthenticationMessage&)
        BSLS_KEYWORD_OVERRIDE
    {
        ++d_numHandleResponseCalls;
        if (d_handleResponseRc != 0) {
            errorDescription << "authentication rejected by peer";
        }
        return d_handleResponseRc;
    }

    void onClose() BSLS_KEYWORD_OVERRIDE { ++d_numOnCloseCalls; }
};

// Mock authenticator
struct MockAuthenticator : public mqbnet::Authenticator {
  private:
    bsl::optional<mqbcfg::Credential> d_anonymousCredential;

  public:
    /// Handed out by `createAuthenticationClient`.  Left empty to emulate a
    /// broker without a credential provider.
    bsl::shared_ptr<mqbnet::AuthenticationClient> d_authenticationClient_sp;

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
    bsl::shared_ptr<mqbnet::AuthenticationClient> createAuthenticationClient(
        const bsl::shared_ptr<bmqio::Channel>&) BSLS_KEYWORD_OVERRIDE
    {
        return d_authenticationClient_sp;
    }
    const bsl::optional<mqbcfg::Credential>&
    anonymousCredential() const BSLS_KEYWORD_OVERRIDE
    {
        return d_anonymousCredential;
    }
};

// Mock negotiator
struct MockNegotiator : public mqbnet::Negotiator {
    int d_numCreateSessionCalls;
    int d_numNegotiateOutboundCalls;

    MockNegotiator()
    : d_numCreateSessionCalls(0)
    , d_numNegotiateOutboundCalls(0)
    {
    }

    int createSessionOnMsgType(bsl::ostream&,
                               bsl::shared_ptr<mqbnet::Session>*,
                               mqbnet::InitialConnectionContext*)
        BSLS_KEYWORD_OVERRIDE
    {
        ++d_numCreateSessionCalls;
        return 0;
    }
    int negotiateOutbound(bsl::ostream&, mqbnet::InitialConnectionContext*)
        BSLS_KEYWORD_OVERRIDE
    {
        ++d_numNegotiateOutboundCalls;
        return 0;
    }
};

/// Return an AuthenticationResponse message.
bmqp_ctrlmsg::AuthenticationMessage makeAuthenticationResponse()
{
    bmqp_ctrlmsg::AuthenticationMessage   msg;
    bmqp_ctrlmsg::AuthenticationResponse& response =
        msg.makeAuthenticationResponse();
    response.status().category() = bmqp_ctrlmsg::StatusCategory::E_SUCCESS;
    response.status().code()     = 0;
    return msg;
}

/// Return a BrokerResponse negotiation message.
bmqp_ctrlmsg::NegotiationMessage makeBrokerResponse()
{
    bmqp_ctrlmsg::NegotiationMessage msg;
    msg.makeBrokerResponse();
    return msg;
}

/// Test harness holding the collaborators of an
/// @bbref{mqbnet::InitialConnectionContext} under test.
class TestBench {
  public:
    // DATA
    bslma::Allocator*                         d_allocator_p;
    bsl::shared_ptr<MockAuthenticator>        d_authenticator_sp;
    bsl::shared_ptr<MockNegotiator>           d_negotiator_sp;
    bsl::shared_ptr<MockAuthenticationClient> d_client_sp;
    bsl::shared_ptr<bmqio::TestChannel>       d_channel_sp;
    CompletionResult                          d_result;

    // CREATORS

    /// Create a test bench using the specified `allocator`.  The specified
    /// `withCredentialProvider` emulates a broker configured to authenticate
    /// with other brokers.
    explicit TestBench(bool              withCredentialProvider,
                       bslma::Allocator* allocator)
    : d_allocator_p(allocator)
    , d_authenticator_sp(bsl::allocate_shared<MockAuthenticator>(allocator))
    , d_negotiator_sp(bsl::allocate_shared<MockNegotiator>(allocator))
    , d_client_sp(bsl::allocate_shared<MockAuthenticationClient>(allocator))
    , d_channel_sp(bsl::allocate_shared<bmqio::TestChannel>(allocator))
    , d_result(allocator)
    {
        if (withCredentialProvider) {
            d_authenticator_sp->d_authenticationClient_sp = d_client_sp;
        }
    }

    // MANIPULATORS

    /// Return a completion callback recording into `d_result`.
    mqbnet::InitialConnectionContext::InitialConnectionCompleteCb completeCb()
    {
        return bdlf::BindUtil::bindS(d_allocator_p,
                                     &onComplete,
                                     &d_result,
                                     bdlf::PlaceHolders::_1,   // status
                                     bdlf::PlaceHolders::_2,   // error
                                     bdlf::PlaceHolders::_3,   // session
                                     bdlf::PlaceHolders::_4);  // channel
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
        bdlf::BindUtil::bind(&complete,
                             check,
                             bdlf::PlaceHolders::_1,  // status
                             bdlf::PlaceHolders::_2,  // errorDescription
                             bdlf::PlaceHolders::_3,  // session
                             bdlf::PlaceHolders::_4   // channel
        );

    bmqtst::TestHelper::printTestName("test1_basicConstruction");
    {
        PV("Constructor");
        mqbnet::InitialConnectionContext obj1(
            false,
            authenticator.get(),
            negotiator.get(),
            bsl::shared_ptr<mqbnet::NegotiationUserData>(),
            static_cast<void*>(0),
            channel,
            completeCb);
        BMQTST_ASSERT_EQ(obj1.isIncoming(), false);
        BMQTST_ASSERT_EQ(obj1.resultState(), static_cast<void*>(0));
        BMQTST_ASSERT_EQ(obj1.userData(),
                         bsl::shared_ptr<mqbnet::NegotiationUserData>());
    }

    {
        PV("Manipulators/Accessors");

        mqbnet::InitialConnectionContext obj(
            true,
            authenticator.get(),
            negotiator.get(),
            bsl::shared_ptr<mqbnet::NegotiationUserData>(),
            static_cast<void*>(0),
            channel,
            completeCb);

        {  // ResultState
            int value = 9;
            obj.setResultState(&value);
            BMQTST_ASSERT_EQ(obj.resultState(), &value);
        }

        {  // AuthenticationContext
            bdlmt::EventScheduler                          scheduler(alloc);
            bmqp_ctrlmsg::AuthenticationMessage            authnMsg;
            bsl::shared_ptr<mqbnet::AuthenticationContext> authnCtx =
                bsl::allocate_shared<mqbnet::AuthenticationContext>(
                    alloc,
                    &scheduler,
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

            BMQTST_ASSERT_EQ(*check, rc);
        }
    }
}

static void test2_outboundAuthenticationSuccess()
// ------------------------------------------------------------------------
// OUTBOUND AUTHENTICATION SUCCESS
//
// Concerns:
//   - An outbound connection on a broker configured to authenticate with
//     other brokers sends an AuthenticationRequest before negotiating.
//   - Negotiation starts only once the peer accepts the authentication.
//   - The session is created after the peer's BrokerResponse.
//
// Plan:
//   1) Start the initial connection and verify authentication is in flight
//      and that negotiation has not started.
//   2) Feed a successful AuthenticationResponse and verify negotiation is
//      sent.
//   3) Feed a BrokerResponse and verify the session is created and the
//      connection completes successfully.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("OUTBOUND AUTHENTICATION SUCCESS");

    bslma::Allocator* alloc = bmqtst::TestHelperUtil::allocator();
    TestBench         tb(true, alloc);

    mqbnet::InitialConnectionContext obj(
        false,  // isIncoming
        tb.d_authenticator_sp.get(),
        tb.d_negotiator_sp.get(),
        bsl::shared_ptr<mqbnet::NegotiationUserData>(),
        static_cast<void*>(0),
        tb.d_channel_sp,
        tb.completeCb(),
        alloc);

    // 1)
    obj.handleInitialConnection();

    BMQTST_ASSERT_EQ(
        obj.state(),
        mqbnet::InitialConnectionState::e_AUTHENTICATING_OUTBOUND);
    BMQTST_ASSERT_EQ(obj.authenticationClient(), tb.d_client_sp);
    BMQTST_ASSERT_EQ(tb.d_client_sp->d_numAuthenticateCalls, 1);
    BMQTST_ASSERT_EQ(tb.d_negotiator_sp->d_numNegotiateOutboundCalls, 0);
    BMQTST_ASSERT_EQ(tb.d_channel_sp->numReadCalls(), 1u);
    BMQTST_ASSERT_EQ(tb.d_result.d_numCalls, 0);

    // 2)
    InitialConnectionMessage message;
    message = makeAuthenticationResponse();
    obj.handleEvent(bsl::string(),
                    mqbnet::InitialConnectionEvent::e_AUTHN_RESPONSE,
                    message);

    BMQTST_ASSERT_EQ(obj.state(),
                     mqbnet::InitialConnectionState::e_NEGOTIATING_OUTBOUND);
    BMQTST_ASSERT_EQ(tb.d_client_sp->d_numHandleResponseCalls, 1);
    BMQTST_ASSERT_EQ(tb.d_negotiator_sp->d_numNegotiateOutboundCalls, 1);
    BMQTST_ASSERT_EQ(tb.d_channel_sp->numReadCalls(), 2u);
    BMQTST_ASSERT_EQ(tb.d_result.d_numCalls, 0);

    // 3)
    message = makeBrokerResponse();
    obj.handleEvent(bsl::string(),
                    mqbnet::InitialConnectionEvent::e_NEGOTIATION_MESSAGE,
                    message);

    BMQTST_ASSERT_EQ(obj.state(),
                     mqbnet::InitialConnectionState::e_NEGOTIATED);
    BMQTST_ASSERT_EQ(tb.d_negotiator_sp->d_numCreateSessionCalls, 1);
    BMQTST_ASSERT_EQ(tb.d_result.d_numCalls, 1);
    BMQTST_ASSERT_EQ(tb.d_result.d_status, 0);
}

static void test3_outboundAuthenticationRejected()
// ------------------------------------------------------------------------
// OUTBOUND AUTHENTICATION REJECTED
//
// Concerns:
//   - A peer rejecting the authentication fails the connection.
//   - Negotiation is not attempted on a connection that failed to
//     authenticate.
//
// Plan:
//   1) Start the initial connection with a client that rejects the peer's
//      response.
//   2) Feed an AuthenticationResponse and verify the connection completes
//      with a failure and that no negotiation message was sent.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("OUTBOUND AUTHENTICATION REJECTED");

    bslma::Allocator* alloc = bmqtst::TestHelperUtil::allocator();
    TestBench         tb(true, alloc);

    tb.d_client_sp->d_handleResponseRc = -1;

    mqbnet::InitialConnectionContext obj(
        false,  // isIncoming
        tb.d_authenticator_sp.get(),
        tb.d_negotiator_sp.get(),
        bsl::shared_ptr<mqbnet::NegotiationUserData>(),
        static_cast<void*>(0),
        tb.d_channel_sp,
        tb.completeCb(),
        alloc);

    // 1)
    obj.handleInitialConnection();

    BMQTST_ASSERT_EQ(
        obj.state(),
        mqbnet::InitialConnectionState::e_AUTHENTICATING_OUTBOUND);

    // 2)
    InitialConnectionMessage message;
    message = makeAuthenticationResponse();
    obj.handleEvent(bsl::string(),
                    mqbnet::InitialConnectionEvent::e_AUTHN_RESPONSE,
                    message);

    BMQTST_ASSERT_EQ(obj.state(), mqbnet::InitialConnectionState::e_FAILED);
    BMQTST_ASSERT_EQ(tb.d_client_sp->d_numHandleResponseCalls, 1);
    BMQTST_ASSERT_EQ(tb.d_negotiator_sp->d_numNegotiateOutboundCalls, 0);
    BMQTST_ASSERT_EQ(tb.d_negotiator_sp->d_numCreateSessionCalls, 0);
    BMQTST_ASSERT_EQ(tb.d_result.d_numCalls, 1);
    BMQTST_ASSERT_NE(tb.d_result.d_status, 0);
    BMQTST_ASSERT_EQ(tb.d_result.d_error, "authentication rejected by peer");
}

static void test4_outboundAuthenticationSendFailure()
// ------------------------------------------------------------------------
// OUTBOUND AUTHENTICATION SEND FAILURE
//
// Concerns:
//   - Failing to send the AuthenticationRequest fails the connection
//     immediately, without waiting for a response.
//
// Plan:
//   1) Start the initial connection with a client that fails to send.
//   2) Verify the connection completes with a failure and no read was
//      scheduled.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("OUTBOUND AUTHENTICATION SEND FAILURE");

    bslma::Allocator* alloc = bmqtst::TestHelperUtil::allocator();
    TestBench         tb(true, alloc);

    tb.d_client_sp->d_authenticateRc = -1;

    mqbnet::InitialConnectionContext obj(
        false,  // isIncoming
        tb.d_authenticator_sp.get(),
        tb.d_negotiator_sp.get(),
        bsl::shared_ptr<mqbnet::NegotiationUserData>(),
        static_cast<void*>(0),
        tb.d_channel_sp,
        tb.completeCb(),
        alloc);

    // 1)
    obj.handleInitialConnection();

    // 2)
    BMQTST_ASSERT_EQ(obj.state(), mqbnet::InitialConnectionState::e_FAILED);
    BMQTST_ASSERT_EQ(tb.d_client_sp->d_numAuthenticateCalls, 1);
    BMQTST_ASSERT_EQ(tb.d_channel_sp->numReadCalls(), 0u);
    BMQTST_ASSERT_EQ(tb.d_negotiator_sp->d_numNegotiateOutboundCalls, 0);
    BMQTST_ASSERT_EQ(tb.d_result.d_numCalls, 1);
    BMQTST_ASSERT_NE(tb.d_result.d_status, 0);
}

static void test5_outboundWithoutCredentialProvider()
// ------------------------------------------------------------------------
// OUTBOUND WITHOUT CREDENTIAL PROVIDER
//
// Concerns:
//   - A broker that is not configured to authenticate with other brokers
//     negotiates an outbound connection directly.
//
// Plan:
//   1) Start the initial connection on a broker with no credential
//      provider.
//   2) Verify negotiation starts right away and no authentication client
//      is held.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("OUTBOUND WITHOUT CREDENTIAL PROVIDER");

    bslma::Allocator* alloc = bmqtst::TestHelperUtil::allocator();
    TestBench         tb(false, alloc);

    mqbnet::InitialConnectionContext obj(
        false,  // isIncoming
        tb.d_authenticator_sp.get(),
        tb.d_negotiator_sp.get(),
        bsl::shared_ptr<mqbnet::NegotiationUserData>(),
        static_cast<void*>(0),
        tb.d_channel_sp,
        tb.completeCb(),
        alloc);

    // 1)
    obj.handleInitialConnection();

    // 2)
    BMQTST_ASSERT_EQ(obj.state(),
                     mqbnet::InitialConnectionState::e_NEGOTIATING_OUTBOUND);
    BMQTST_ASSERT(!obj.authenticationClient());
    BMQTST_ASSERT_EQ(tb.d_client_sp->d_numAuthenticateCalls, 0);
    BMQTST_ASSERT_EQ(tb.d_negotiator_sp->d_numNegotiateOutboundCalls, 1);
    BMQTST_ASSERT_EQ(tb.d_channel_sp->numReadCalls(), 1u);
    BMQTST_ASSERT_EQ(tb.d_result.d_numCalls, 0);
}

static void test6_unexpectedAuthenticationResponse()
// ------------------------------------------------------------------------
// UNEXPECTED AUTHENTICATION RESPONSE
//
// Concerns:
//   - An AuthenticationResponse received on an incoming connection, which
//     never sends an AuthenticationRequest, fails the connection.
//
// Plan:
//   1) Start an incoming initial connection.
//   2) Feed an AuthenticationResponse and verify the connection completes
//      with a failure.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("UNEXPECTED AUTHENTICATION RESPONSE");

    bslma::Allocator* alloc = bmqtst::TestHelperUtil::allocator();
    TestBench         tb(true, alloc);

    mqbnet::InitialConnectionContext obj(
        true,  // isIncoming
        tb.d_authenticator_sp.get(),
        tb.d_negotiator_sp.get(),
        bsl::shared_ptr<mqbnet::NegotiationUserData>(),
        static_cast<void*>(0),
        tb.d_channel_sp,
        tb.completeCb(),
        alloc);

    // 1)
    obj.handleInitialConnection();

    BMQTST_ASSERT_EQ(obj.state(), mqbnet::InitialConnectionState::e_INITIAL);
    BMQTST_ASSERT(!obj.authenticationClient());

    // 2)
    InitialConnectionMessage message;
    message = makeAuthenticationResponse();
    obj.handleEvent(bsl::string(),
                    mqbnet::InitialConnectionEvent::e_AUTHN_RESPONSE,
                    message);

    BMQTST_ASSERT_EQ(obj.state(), mqbnet::InitialConnectionState::e_FAILED);
    BMQTST_ASSERT_EQ(tb.d_client_sp->d_numHandleResponseCalls, 0);
    BMQTST_ASSERT_EQ(tb.d_result.d_numCalls, 1);
    BMQTST_ASSERT_NE(tb.d_result.d_status, 0);
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 6: test6_unexpectedAuthenticationResponse(); break;
    case 5: test5_outboundWithoutCredentialProvider(); break;
    case 4: test4_outboundAuthenticationSendFailure(); break;
    case 3: test3_outboundAuthenticationRejected(); break;
    case 2: test2_outboundAuthenticationSuccess(); break;
    case 1: test1_initialConnectionContext(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_GBL_ALLOC);
}
