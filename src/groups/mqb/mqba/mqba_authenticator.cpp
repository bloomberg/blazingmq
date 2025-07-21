// Copyright 2025 Bloomberg Finance L.P.
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

// mqba_authenticator.h                                           -*-C++-*-
#include <mqba_authenticator.h>

#include <mqbscm_version.h>

/// Implementation Notes
///====================
/// The 'Authenticator' class is responsible for handling authentication when
/// the InitialConnectionHanlder receives and passes over an authentication
/// message. It authenticates based on the received AuthenticationRequest and
/// sends back an AuthenticationResponse.

// MQB
#include <mqbblp_clustercatalog.h>
#include <mqbcfg_messages.h>
#include <mqbnet_authenticationcontext.h>
#include <mqbnet_initialconnectioncontext.h>
#include <mqbplug_authenticator.h>

// BMQ
#include <bmqio_channel.h>
#include <bmqio_status.h>
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_protocol.h>
#include <bmqp_schemaeventbuilder.h>
#include <bmqsys_threadutil.h>
#include <bmqu_memoutstream.h>

// BDE
#include <ball_log.h>
#include <bdlb_scopeexit.h>
#include <bdlf_bind.h>
#include <bdlma_sequentialallocator.h>
#include <bdlmt_eventscheduler.h>
#include <bdlmt_threadpool.h>
#include <bsl_memory.h>
#include <bsl_ostream.h>
#include <bsl_string_view.h>
#include <bsls_nullptr.h>
#include <bsls_timeinterval.h>

namespace BloombergLP {
namespace mqba {

namespace {
BALL_LOG_SET_NAMESPACE_CATEGORY("MQBA.AUTHENTICATOR");

}

// -------------------
// class Authenticator
// -------------------

int Authenticator::onAuthenticationRequest(
    bsl::ostream&                              errorDescription,
    const bmqp_ctrlmsg::AuthenticationMessage& authenticationMsg,
    const InitialConnectionContextSp&          context)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(authenticationMsg.isAuthenticateRequestValue());
    BSLS_ASSERT_SAFE(context->isIncoming());

    BALL_LOG_DEBUG << "Received authentication message from '"
                   << context->channel()->peerUri()
                   << "': " << authenticationMsg;

    // Create an AuthenticationContext for that connection
    bsl::shared_ptr<mqbnet::AuthenticationContext> authenticationContext =
        bsl::allocate_shared<mqbnet::AuthenticationContext>(
            d_allocator_p,
            context.get(),      // initialConnectionContext
            authenticationMsg,  // authenticationMessage
            context
                ->authenticationEncodingType(),  // authenticationEncodingType
            bdlf::BindUtil::bind(&Authenticator::reauthenticateAsync,
                                 this,                    // authenticator
                                 bdlf::PlaceHolders::_1,  // errorDescription,
                                 bdlf::PlaceHolders::_2,  // context,
                                 bdlf::PlaceHolders::_3   // channel
                                 ),                       // reauthenticateCb
            State::e_AUTHENTICATING,                      // state
            mqbnet::ConnectionType::e_UNKNOWN             // connectionType
        );

    context->setAuthenticationContext(authenticationContext);

    // Authenticate
    int rc = authenticateAsync(errorDescription, context, context->channel());

    return rc;
}

int Authenticator::onAuthenticationResponse(
    bsl::ostream&                              errorDescription,
    const bmqp_ctrlmsg::AuthenticationMessage& authenticationMsg,
    const InitialConnectionContextSp&          context)
{
    BALL_LOG_ERROR << "Not Implemented";

    return -1;
}

int Authenticator::sendAuthenticationMessage(
    bsl::ostream&                              errorDescription,
    const bmqp_ctrlmsg::AuthenticationMessage& message,
    const bsl::shared_ptr<bmqio::Channel>&     channel,
    bmqp::EncodingType::Enum                   authenticationEncodingType)
{
    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS       = 0,
        rc_BUILD_FAILURE = -1,
        rc_WRITE_FAILURE = -2
    };

    bdlma::LocalSequentialAllocator<2048> localAllocator(d_allocator_p);

    bmqp::SchemaEventBuilder builder(d_blobSpPool_p,
                                     authenticationEncodingType,
                                     &localAllocator);

    int rc = builder.setMessage(message, bmqp::EventType::e_AUTHENTICATION);
    if (rc != 0) {
        errorDescription << "Failed building AuthenticationMessage "
                         << "[rc: " << rc << ", message: " << message << "]";
        return rc_BUILD_FAILURE;  // RETURN
    }

    // Send response event
    bmqio::Status status;

    channel->write(&status, *builder.blob());

    if (!status) {
        errorDescription << "Failed sending AuthenticationMessage "
                         << "[status: " << status << ", message: " << message
                         << "]";
        return rc_WRITE_FAILURE;  // RETURN
    }

    return rc_SUCCESS;
}

int Authenticator::authenticateAsync(
    bsl::ostream&                          errorDescription,
    const InitialConnectionContextSp&      context,
    const bsl::shared_ptr<bmqio::Channel>& channel)
{
    int rc = d_threadPool.enqueueJob(
        bdlf::BindUtil::bind(&Authenticator::authenticate,
                             this,
                             context,
                             channel));

    if (rc != 0) {
        errorDescription
            << "Failed to enqueue authentication job for '"
            << channel->peerUri() << "' [rc: " << rc << ", message: "
            << context->authenticationContext()->authenticationMessage()
            << "]";
    }

    return rc;
}

void Authenticator::authenticate(
    const InitialConnectionContextSp&      context,
    const bsl::shared_ptr<bmqio::Channel>& channel)
{
    // PRECONDITIONS
    BSLS_ASSERT(context);

    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS                             = 0,
        rc_PROCESS_AUTHENTICATION_FAILED       = -1,
        rc_AUTHENTICATION_FAILED               = -2,
        rc_NEGOTIATION_FAILED                  = -3,
        rc_SEND_AUTHENTICATION_RESPONSE_FAILED = -4,
        rc_CONTINUE_READ_FAILED                = -5,
    };

    const AuthenticationContextSp& authenticationContext =
        context->authenticationContext();

    const bmqp_ctrlmsg::AuthenticateRequest& authenticateRequest =
        authenticationContext->authenticationMessage().authenticateRequest();

    bmqp_ctrlmsg::AuthenticationMessage authenticationResponse;
    bmqp_ctrlmsg::AuthenticateResponse& response =
        authenticationResponse.makeAuthenticateResponse();

    bsl::shared_ptr<mqbnet::Session> session;
    int                              rc = rc_SUCCESS;
    bsl::string                      error;

    bdlb::ScopeExitAny connectionCompletionGuard(
        bdlf::BindUtil::bind(&mqbnet::InitialConnectionContext::complete,
                             context.get(),
                             bsl::ref(rc),
                             bsl::ref(error),
                             bsl::ref(session)));

    BALL_LOG_INFO << "Authenticating connection '" << channel->peerUri()
                  << "' with mechanism '" << authenticateRequest.mechanism()
                  << "'";

    int processRc = processAuthentication(error,
                                          &response,
                                          authenticateRequest,
                                          channel,
                                          context->authenticationContext());

    // If the authentication failed, we still need to continue sending the
    // AuthenticationResponse back to the client.
    if (processRc != rc_SUCCESS) {
        rc = (processRc * 10) + rc_PROCESS_AUTHENTICATION_FAILED;
        return;
    }

    // This is when we authenticate with default credentials
    // No need to send authentication response
    if (context->negotiationContext()) {
        if (response.status().category() !=
            bmqp_ctrlmsg::StatusCategory::E_SUCCESS) {
            rc    = rc_AUTHENTICATION_FAILED;
            error = response.status().message();
            return;  // RETURN
        }

        bmqu::MemOutStream negotiationErrStream;
        const int negoRc = context->negotiationCb()(negotiationErrStream,
                                                    &session,
                                                    context.get());
        if (negoRc != rc_SUCCESS) {
            rc    = (negoRc * 10) + rc_NEGOTIATION_FAILED;
            error = negotiationErrStream.str();
        }
        return;  // RETURN
    }

    // Send authentication response back to the client
    bmqu::MemOutStream sendResponseErrorStream;
    const int          sendRc = sendAuthenticationMessage(
        sendResponseErrorStream,
        authenticationResponse,
        channel,
        context->authenticationEncodingType());

    if (response.status().category() !=
        bmqp_ctrlmsg::StatusCategory::E_SUCCESS) {
        rc    = rc_AUTHENTICATION_FAILED;
        error = response.status().message();
        return;  // RETURN
    }

    if (sendRc != rc_SUCCESS) {
        rc    = (sendRc * 10) + rc_SEND_AUTHENTICATION_RESPONSE_FAILED;
        error = sendResponseErrorStream.str();
        return;  // RETURN
    }

    // Authentication succeeded, continue to read
    bmqu::MemOutStream readErrorStream;
    const int readRc = context->scheduleReadCb()(readErrorStream, context);
    if (readRc != rc_SUCCESS) {
        rc    = (readRc * 10) + rc_CONTINUE_READ_FAILED;
        error = readErrorStream.str();
        return;  // RETURN
    }

    return;
}

int Authenticator::reauthenticateAsync(
    bsl::ostream&                          errorDescription,
    const AuthenticationContextSp&         context,
    const bsl::shared_ptr<bmqio::Channel>& channel)
{
    int rc = d_threadPool.enqueueJob(
        bdlf::BindUtil::bind(&Authenticator::reauthenticate,
                             this,
                             context,
                             channel));

    if (rc != 0) {
        errorDescription << "Failed to enqueue authentication job for '"
                         << channel->peerUri() << "' [rc: " << rc
                         << ", message: " << context->authenticationMessage()
                         << "]";
    }

    return rc;
}

void Authenticator::reauthenticate(
    const AuthenticationContextSp&         context,
    const bsl::shared_ptr<bmqio::Channel>& channel)
{
    // PRECONDITIONS
    BSLS_ASSERT(context);

    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS                             = 0,
        rc_PROCESS_AUTHENTICATION_FAILED       = -1,
        rc_AUTHENTICATION_FAILED               = -2,
        rc_SEND_AUTHENTICATION_RESPONSE_FAILED = -3,
        rc_CONTINUE_READ_FAILED                = -4,
    };

    const bmqp_ctrlmsg::AuthenticateRequest& authenticateRequest =
        context->authenticationMessage().authenticateRequest();

    bmqp_ctrlmsg::AuthenticationMessage authenticationResponse;
    bmqp_ctrlmsg::AuthenticateResponse& response =
        authenticationResponse.makeAuthenticateResponse();

    bsl::shared_ptr<mqbnet::Session> session;
    int                              rc = rc_SUCCESS;
    bsl::string                      error;

    bdlb::ScopeExitAny reauthenticateErrorGuard(
        bdlf::BindUtil::bind(&Authenticator::reauthenticateErrorOrTimeout,
                             this,
                             bsl::ref(rc),
                             bsl::ref(error),
                             channel));

    BALL_LOG_INFO << "Reauthenticating connection '" << channel->peerUri()
                  << "' with mechanism '" << authenticateRequest.mechanism()
                  << "'";

    const int processRc = processAuthentication(error,
                                                &response,
                                                authenticateRequest,
                                                channel,
                                                context);

    // If the authentication failed, we still need to continue sending the
    // AuthenticationResponse back to the client.
    if (processRc != rc_SUCCESS) {
        BALL_LOG_ERROR << error;
        rc    = (processRc * 10) + rc_PROCESS_AUTHENTICATION_FAILED;
        error = "reauthenticationError";
        return;
    }

    // Send authentication response back to the client
    bmqu::MemOutStream sendResponseErrorStream;
    const int          sendRc = sendAuthenticationMessage(
        sendResponseErrorStream,
        authenticationResponse,
        channel,
        context->authenticationEncodingType());

    if (response.status().category() !=
        bmqp_ctrlmsg::StatusCategory::E_SUCCESS) {
        rc    = rc_AUTHENTICATION_FAILED;
        error = "reauthenticationError";
        return;  // RETURN
    }

    if (sendRc != rc_SUCCESS) {
        BALL_LOG_ERROR << sendResponseErrorStream.str();
        rc    = (sendRc * 10) + rc_SEND_AUTHENTICATION_RESPONSE_FAILED;
        error = "reauthenticationError";
        return;  // RETURN
    }

    // Reauthentication succeeded, release the error guard
    reauthenticateErrorGuard.release();

    return;
}

void Authenticator::reauthenticateErrorOrTimeout(
    const int                              errorCode,
    const bsl::string&                     errorName,
    const bsl::shared_ptr<bmqio::Channel>& channel)
{
    BALL_LOG_ERROR << "Reauthentication error or timeout for '"
                   << channel->peerUri() << "' [error: " << errorName
                   << ", code: " << errorCode << "]";

    bmqio::Status status(bmqio::StatusCategory::e_GENERIC_ERROR,
                         errorName,
                         errorCode,
                         d_allocator_p);
    channel->close(status);
}

int Authenticator::processAuthentication(
    bsl::string&                             error,
    bmqp_ctrlmsg::AuthenticateResponse*      response,
    const bmqp_ctrlmsg::AuthenticateRequest& request,
    const bsl::shared_ptr<bmqio::Channel>&   channel,
    const AuthenticationContextSp&           authenticationContext)
{
    enum RcEnum {
        rc_SUCCESS                        = 0,
        rc_AUTHENTICATION_STATE_INCORRECT = -1,
    };

    bmqu::MemOutStream errorStream;

    bsl::shared_ptr<mqbplug::AuthenticationResult> result;
    mqbplug::AuthenticationData authenticationData(request.data().value(),
                                                   channel->peerUri());

    const int authnRc = d_authnController_p->authenticate(errorStream,
                                                          &result,
                                                          request.mechanism(),
                                                          authenticationData);

    if (authnRc != 0) {
        BALL_LOG_ERROR << "Authentication failed for connection '"
                       << channel->peerUri() << "' with mechanism '"
                       << request.mechanism() << "' [rc: " << authnRc
                       << ", error: " << errorStream.str() << "]";

        response->status().code() = authnRc;
        response->status().category() =
            bmqp_ctrlmsg::StatusCategory::E_REFUSED;
        response->status().message() = errorStream.str();
    }
    else {
        response->status().code() = 0;
        response->status().category() =
            bmqp_ctrlmsg::StatusCategory::E_SUCCESS;
        response->lifetimeMs() = result->lifetimeMs();

        authenticationContext->setAuthenticationResult(result);

        if (authenticationContext->state().testAndSwap(
                State::e_AUTHENTICATING,
                State::e_AUTHENTICATED) != State::e_AUTHENTICATING) {
            errorStream << "Failed to set authentication state for '"
                        << channel->peerUri()
                        << "' to 'e_AUTHENTICATED' from 'e_AUTHENTICATING'";
            error = errorStream.str();
            return rc_AUTHENTICATION_STATE_INCORRECT;
        }

        // Schedule authentication timeout
        if (result->lifetimeMs().has_value()) {
            bslmt::LockGuard<bslmt::Mutex> lockGuard(
                &authenticationContext->timeoutHandleMutex());  // MUTEX LOCKED

            d_scheduler.scheduleEvent(
                &authenticationContext->timeoutHandle(),
                bsls::TimeInterval(bmqsys::Time::nowMonotonicClock())
                    .addMilliseconds(result->lifetimeMs().value()),
                bdlf::BindUtil::bind(
                    &Authenticator::reauthenticateErrorOrTimeout,
                    this,                     // authenticator
                    -1,                       // errorCode
                    "authenticationTimeout",  // errorName
                    channel                   // channel
                    ));
        }
    }

    return rc_SUCCESS;
}

// CREATORS
Authenticator::Authenticator(
    mqbauthn::AuthenticationController* authnController,
    BlobSpPool*                         blobSpPool,
    bslma::Allocator*                   allocator)
: d_authnController_p(authnController)
, d_threadPool(bmqsys::ThreadUtil::defaultAttributes(),
               0,                                            // min threads
               100,                                          // max threads
               bsls::TimeInterval(120).totalMilliseconds(),  // idle time
               allocator)
, d_scheduler(bsls::SystemClockType::e_MONOTONIC, allocator)
, d_blobSpPool_p(blobSpPool)
, d_allocator_p(allocator)
{
    // NOTHING
}

/// Destructor
Authenticator::~Authenticator()
{
    // NOTHING: (required because of inheritance)
}

int Authenticator::start(bsl::ostream& errorDescription)
{
    int rc = d_threadPool.start();
    if (rc != 0) {
        errorDescription << "Failed to start thread pool for Authenticator"
                         << "[rc: " << rc << "]";
        return rc;  // RETURN
    }

    rc = d_scheduler.start();
    if (rc != 0) {
        errorDescription << "Failed to start event scheduler for Authenticator"
                         << "[rc: " << rc << "]";
        return rc;  // RETURN
    }

    return 0;
}

void Authenticator::stop()
{
    d_scheduler.stop();
    d_threadPool.stop();
}

int Authenticator::handleAuthentication(
    bsl::ostream&                              errorDescription,
    const InitialConnectionContextSp&          context,
    const bmqp_ctrlmsg::AuthenticationMessage& authenticationMsg)
{
    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS                  = 0,
        rc_DUPLICATE_AUTHENTICATION = -1,
        rc_HANDLE_MESSAGE_FAIL      = -2,
        rc_INVALID_MESSAGE          = -3,
    };

    int rc = rc_SUCCESS;

    if (context->authenticationContext()) {
        errorDescription
            << "Received another authentication message while waiting "
               "for negotiation message";
        return rc_DUPLICATE_AUTHENTICATION;
    }

    switch (authenticationMsg.selectionId()) {
    case bmqp_ctrlmsg::AuthenticationMessage::
        SELECTION_ID_AUTHENTICATE_REQUEST: {
        rc = onAuthenticationRequest(errorDescription,
                                     authenticationMsg,
                                     context);
    } break;  // BREAK
    case bmqp_ctrlmsg::AuthenticationMessage::
        SELECTION_ID_AUTHENTICATE_RESPONSE: {
        rc = onAuthenticationResponse(errorDescription,
                                      authenticationMsg,
                                      context);
    } break;  // BREAK
    default: {
        errorDescription
            << "Invalid authentication message received (unknown type): "
            << authenticationMsg;
        return rc_INVALID_MESSAGE;  // RETURN
    }
    }

    if (rc != rc_SUCCESS) {
        rc = (rc * 10) + rc_HANDLE_MESSAGE_FAIL;
    }

    return rc;
}

int Authenticator::authenticationOutbound(
    const AuthenticationContextSp& context)
{
    BALL_LOG_ERROR << "Not Implemented";

    return -1;
}

// ACCESSORS
const bsl::optional<mqbcfg::Credential>& Authenticator::anonymousCredential()
{
    return d_authnController_p->anonymousCredential();
}

}  // close package namespace
}  // close enterprise namespace
