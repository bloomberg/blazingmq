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
/// The 'Authenticator' class manages both authentication and reauthentication
/// for connections.  For incoming connections, it authenticates using the
/// received AuthenticationRequest and responds with an AuthenticationResponse.
/// For outgoing connections, it initiates authentication by sending an
/// AuthenticationRequest and awaits an AuthenticationResponse.  Upon
/// successful authentication during initial connection, the negotiation
/// process continues.  All authentication operations are performed
/// asynchronously.

// MQB
#include <mqbblp_clustercatalog.h>
#include <mqbcfg_messages.h>
#include <mqbnet_authenticationcontext.h>
#include <mqbnet_initialconnectioncontext.h>
#include <mqbnet_initialconnectionhandler.h>
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
#include <bsl_optional.h>
#include <bsl_ostream.h>
#include <bsl_string_view.h>
#include <bsl_vector.h>
#include <bsls_nullptr.h>
#include <bsls_timeinterval.h>

namespace BloombergLP {
namespace mqba {

namespace {

BALL_LOG_SET_NAMESPACE_CATEGORY("MQBA.AUTHENTICATOR");

const int k_MIN_THREADS = 0;  // Minimum number of threads in the thread pool
const int k_MAX_THREADS = 1;  // Maximum number of threads in the thread pool

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
            State::e_AUTHENTICATING,             // state
            mqbnet::ConnectionType::e_UNKNOWN    // connectionType
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
    // executed by an *AUTHENTICATION* thread

    // PRECONDITIONS
    BSLS_ASSERT(context);

    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS                             = 0,
        rc_PROCESS_AUTHENTICATION_FAILED       = -1,
        rc_AUTHENTICATION_FAILED               = -2,
        rc_NEGOTIATION_FAILED                  = -3,
        rc_SEND_AUTHENTICATION_RESPONSE_FAILED = -4,
    };

    const AuthenticationContextSp& authenticationContext =
        context->authenticationContext();

    const bmqp_ctrlmsg::AuthenticateRequest& authenticateRequest =
        authenticationContext->authenticationMessage().authenticateRequest();

    bmqp_ctrlmsg::AuthenticationMessage authenticationResponse;
    bmqp_ctrlmsg::AuthenticateResponse& response =
        authenticationResponse.makeAuthenticateResponse();
    bmqp_ctrlmsg::Status& status = response.status();

    int                                  rc = rc_SUCCESS;
    bsl::string                          error;
    mqbnet::InitialConnectionEvent::Enum input;

    bdlb::ScopeExitAny Guard(bdlf::BindUtil::bind(context->handleEventCb(),
                                                  bsl::ref(rc),
                                                  bsl::ref(error),
                                                  bsl::ref(input),
                                                  context,
                                                  bsl::nullopt));

    BALL_LOG_INFO << "Authenticating connection '" << channel->peerUri()
                  << "' with mechanism '" << authenticateRequest.mechanism()
                  << "'";

    // Build the authentication response.
    bmqu::MemOutStream processErrStream;
    int                processRc = processAuthentication(processErrStream,
                                          &response,
                                          authenticateRequest,
                                          channel,
                                          context->authenticationContext());

    // Return code processRc is rc_SUCCESS even when authentication fails,
    // since we still need to continue sending the AuthenticationResponse back
    // to the client.
    if (processRc != rc_SUCCESS) {
        rc    = (processRc * 10) + rc_PROCESS_AUTHENTICATION_FAILED;
        error = processErrStream.str();
        input = InitialConnectionEvent::e_ERROR;
        return;  // RETURN
    }

    // In the case of a default authentication, we do not need to send
    // an AuthenticationResponse, we just need to continue the negotiation.
    if (context->state() ==
        mqbnet::InitialConnectionState::e_DEFAULT_AUTHENTICATING) {
        if (status.category() != bmqp_ctrlmsg::StatusCategory::E_SUCCESS) {
            rc    = (status.code() * 10) + rc_AUTHENTICATION_FAILED;
            error = status.message();
            input = InitialConnectionEvent::e_ERROR;
        }
        else {
            input = InitialConnectionEvent::e_AUTH_SUCCESS;
        }
        return;  // RETURN
    }

    // Send authentication response back to the client
    bmqu::MemOutStream sendResponseErrStream;
    const int          sendRc = sendAuthenticationMessage(
        sendResponseErrStream,
        authenticationResponse,
        channel,
        context->authenticationEncodingType());

    if (status.category() != bmqp_ctrlmsg::StatusCategory::E_SUCCESS) {
        rc    = (status.code() * 10) + rc_AUTHENTICATION_FAILED;
        error = status.message();
        input = InitialConnectionEvent::e_ERROR;
        return;  // RETURN
    }

    if (sendRc != rc_SUCCESS) {
        rc    = (sendRc * 10) + rc_SEND_AUTHENTICATION_RESPONSE_FAILED;
        error = sendResponseErrStream.str();
        input = InitialConnectionEvent::e_ERROR;
        return;  // RETURN
    }

    // Authentication succeeded.  Transition to the next state.
    input = InitialConnectionEvent::e_AUTH_SUCCESS;
    return;
}

void Authenticator::reauthenticate(
    const AuthenticationContextSp&         context,
    const bsl::shared_ptr<bmqio::Channel>& channel)
{
    // executed by an *AUTHENTICATION* thread

    // PRECONDITIONS
    BSLS_ASSERT(context);

    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS                             = 0,
        rc_PROCESS_AUTHENTICATION_FAILED       = -1,
        rc_AUTHENTICATION_FAILED               = -2,
        rc_SEND_AUTHENTICATION_RESPONSE_FAILED = -3,
    };

    const bmqp_ctrlmsg::AuthenticateRequest& authenticateRequest =
        context->authenticationMessage().authenticateRequest();

    bmqp_ctrlmsg::AuthenticationMessage authenticationResponse;
    bmqp_ctrlmsg::AuthenticateResponse& response =
        authenticationResponse.makeAuthenticateResponse();

    bsl::shared_ptr<mqbnet::Session> session;
    int                              rc = rc_SUCCESS;
    bsl::string                      error;

    bdlb::ScopeExitAny reauthenticateErrorGuard(bdlf::BindUtil::bind(
        &mqbnet::AuthenticationContext::onReauthenticateErrorOrTimeout,
        context.get(),
        bsl::ref(rc),
        bsl::ref(error),
        channel));

    BALL_LOG_INFO << "Reauthenticating connection '" << channel->peerUri()
                  << "' with mechanism '" << authenticateRequest.mechanism()
                  << "'";

    bmqu::MemOutStream processAuthnErrStream;
    const int          processRc = processAuthentication(processAuthnErrStream,
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

int Authenticator::processAuthentication(
    bsl::ostream&                            errorDescription,
    bmqp_ctrlmsg::AuthenticateResponse*      response,
    const bmqp_ctrlmsg::AuthenticateRequest& request,
    const bsl::shared_ptr<bmqio::Channel>&   channel,
    const AuthenticationContextSp&           authenticationContext)
{
    // executed by an *AUTHENTICATION* thread

    enum RcEnum {
        rc_SUCCESS                        = 0,
        rc_AUTHENTICATION_STATE_INCORRECT = -1,
    };

    bmqu::MemOutStream errorStream;

    bsl::shared_ptr<mqbplug::AuthenticationResult> result;
    const bsl::vector<char>&    data = request.data().isNull()
                                           ? bsl::vector<char>()
                                           : request.data().value();
    mqbplug::AuthenticationData authenticationData(data, channel->peerUri());

    const int authnRc = d_authnController_p->authenticate(errorStream,
                                                          &result,
                                                          request.mechanism(),
                                                          authenticationData);

    if (authnRc != 0) {
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

        const int scheduleRc = authenticationContext->scheduleReauthn(
            errorDescription,
            d_scheduler,
            result->lifetimeMs(),
            channel);

        if (scheduleRc != 0) {
            return rc_AUTHENTICATION_STATE_INCORRECT;
        }
    }

    return rc_SUCCESS;
}

// CREATORS
Authenticator::Authenticator(
    mqbauthn::AuthenticationController* authnController,
    BlobSpPool*                         blobSpPool,
    bdlmt::EventScheduler*              scheduler,
    bslma::Allocator*                   allocator)
: d_isStarted(false)
, d_authnController_p(authnController)
, d_threadPool(bmqsys::ThreadUtil::defaultAttributes(),
               k_MIN_THREADS,                                // min threads
               k_MAX_THREADS,                                // max threads
               bsls::TimeInterval(120).totalMilliseconds(),  // idle time
               allocator)
, d_blobSpPool_p(blobSpPool)
, d_scheduler(scheduler)
, d_allocator_p(allocator)
{
    // NOTHING
}

/// Destructor
Authenticator::~Authenticator()
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(!d_isStarted &&
                    "stop() must be called before destroying this object");
}

int Authenticator::start(bsl::ostream& errorDescription)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(!d_isStarted &&
                    "start() can only be called once on this object");

    BALL_LOG_INFO << "Starting Authenticator";

    int rc = d_threadPool.start();
    if (rc != 0) {
        errorDescription << "Failed to start thread pool for Authenticator"
                         << "[rc: " << rc << "]";
        return rc;  // RETURN
    }

    d_isStarted = true;

    return 0;
}

void Authenticator::stop()
{
    if (!d_isStarted) {
        return;  // RETURN
    }

    d_isStarted = false;

    d_threadPool.stop();
}

int Authenticator::handleAuthentication(
    bsl::ostream&                              errorDescription,
    const InitialConnectionContextSp&          context,
    const bmqp_ctrlmsg::AuthenticationMessage& authenticationMsg)
{
    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS             = 0,
        rc_HANDLE_MESSAGE_FAIL = -1,
        rc_INVALID_MESSAGE     = -2,
    };

    int rc = rc_SUCCESS;

    BSLS_ASSERT_SAFE(!context->authenticationContext());

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

// ACCESSORS
const bsl::optional<mqbcfg::Credential>&
Authenticator::anonymousCredential() const
{
    return d_authnController_p->anonymousCredential();
}

}  // close package namespace
}  // close enterprise namespace
