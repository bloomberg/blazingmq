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
        rc_AUTHENTICATION_STATE_INCORRECT      = -1,
        rc_AUTHENTICATION_FAILED               = -2,
        rc_SEND_AUTHENTICATION_RESPONSE_FAILED = -3,
        rc_CONTINUE_READ_FAILED                = -4,
        rc_AUTHENTICATION_TIMEOUT              = -5
    };

    const AuthenticationContextSp& authenticationContext =
        context->authenticationContext();

    const bmqp_ctrlmsg::AuthenticateRequest& authenticateRequest =
        authenticationContext->authenticationMessage().authenticateRequest();

    bsl::shared_ptr<mqbplug::AuthenticationResult> result;
    mqbplug::AuthenticationData                    authenticationData(
        authenticateRequest.data().value(),
        channel->peerUri());

    bmqp_ctrlmsg::AuthenticationMessage authenticationResponse;
    bmqp_ctrlmsg::AuthenticateResponse& response =
        authenticationResponse.makeAuthenticateResponse();

    bmqu::MemOutStream authenticationErrorStream;

    BALL_LOG_INFO << "Authenticating connection '" << channel->peerUri()
                  << "' with mechanism '" << authenticateRequest.mechanism()
                  << "'";

    int rc = d_authnController_p->authenticate(authenticationErrorStream,
                                               &result,
                                               authenticateRequest.mechanism(),
                                               authenticationData);

    if (rc != 0) {
        BALL_LOG_ERROR << "Authentication failed for connection '"
                       << channel->peerUri() << "' with mechanism '"
                       << authenticateRequest.mechanism() << "' [rc: " << rc
                       << ", error: " << authenticationErrorStream.str()
                       << "]";

        response.status().code()     = rc;
        response.status().category() = bmqp_ctrlmsg::StatusCategory::E_REFUSED;
        response.status().message()  = authenticationErrorStream.str();
    }
    else {
        response.status().code()     = 0;
        response.status().category() = bmqp_ctrlmsg::StatusCategory::E_SUCCESS;
        response.lifetimeMs()        = result->lifetimeMs();

        authenticationContext->setAuthenticationResult(result);

        if (authenticationContext->state().testAndSwap(
                State::e_AUTHENTICATING,
                State::e_AUTHENTICATED) != State::e_AUTHENTICATING) {
            authenticationErrorStream
                << "Failed to set authentication state for '"
                << channel->peerUri()
                << "' to 'e_AUTHENTICATED' from 'e_AUTHENTICATING'";
            context->complete((rc * 10) + rc_AUTHENTICATION_STATE_INCORRECT,
                              authenticationErrorStream.str(),
                              bsl::shared_ptr<mqbnet::Session>());
            return;  // RETURN
        }

        // Schedule authentication timeout
        if (result->lifetimeMs().has_value()) {
            bslmt::LockGuard<bslmt::Mutex> guard(
                &authenticationContext->timeoutHandleMutex());  // MUTEX LOCKED

            if (authenticationContext->timeoutHandle()) {
                BALL_LOG_INFO << "timeout handle already has value";
            }

            d_scheduler.scheduleEvent(
                &authenticationContext->timeoutHandle(),
                bsls::TimeInterval(bmqsys::Time::nowMonotonicClock())
                    .addMilliseconds(result->lifetimeMs().value()),
                bdlf::BindUtil::bind(
                    &Authenticator::reauthenticateErrorOrTimeout,
                    this,                       // authenticator
                    rc_AUTHENTICATION_TIMEOUT,  // errorCode
                    "authenticationTimeout",    // errorName
                    channel                     // channel
                    ));
        }
    }

    // This is when we authenticate with default credentials
    // No need to send authentication response
    if (context->negotiationContext()) {
        if (response.status().category() !=
            bmqp_ctrlmsg::StatusCategory::E_SUCCESS) {
            // If the authentication failed, we do not create a session.
            context->complete(rc_AUTHENTICATION_FAILED,
                              authenticationErrorStream.str(),
                              bsl::shared_ptr<mqbnet::Session>());
            return;  // RETURN
        }

        bsl::shared_ptr<mqbnet::Session> session;
        bmqu::MemOutStream               errStream;
        bsl::string                      error;

        rc = context->negotiationCb()(errStream, &session, context.get());
        context->complete(rc, errStream.str(), session);

        return;  // RETURN
    }

    // Send authentication response back to the client
    bmqu::MemOutStream sendResponseErrorStream;
    rc = sendAuthenticationMessage(sendResponseErrorStream,
                                   authenticationResponse,
                                   channel,
                                   context->authenticationEncodingType());
    if (response.status().category() !=
        bmqp_ctrlmsg::StatusCategory::E_SUCCESS) {
        context->complete(rc_AUTHENTICATION_FAILED,
                          authenticationErrorStream.str(),
                          bsl::shared_ptr<mqbnet::Session>());
    }
    else if (rc != 0) {
        context->complete((rc * 10) + rc_SEND_AUTHENTICATION_RESPONSE_FAILED,
                          sendResponseErrorStream.str(),
                          bsl::shared_ptr<mqbnet::Session>());
    }

    // Authentication succeeded, continue to read
    bmqu::MemOutStream readErrorStream;
    rc = context->scheduleReadCb()(readErrorStream, context);
    if (rc != 0) {
        context->complete((rc * 10) + rc_CONTINUE_READ_FAILED,
                          readErrorStream.str(),
                          bsl::shared_ptr<mqbnet::Session>());
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
        rc_AUTHENTICATION_STATE_INCORRECT      = -1,
        rc_AUTHENTICATION_FAILED               = -2,
        rc_SEND_AUTHENTICATION_RESPONSE_FAILED = -3,
        rc_CONTINUE_READ_FAILED                = -4,
        rc_AUTHENTICATION_TIMEOUT              = -5
    };

    const bmqp_ctrlmsg::AuthenticateRequest& authenticateRequest =
        context->authenticationMessage().authenticateRequest();

    bsl::shared_ptr<mqbplug::AuthenticationResult> result;
    mqbplug::AuthenticationData                    authenticationData(
        authenticateRequest.data().value(),
        channel->peerUri());

    bmqp_ctrlmsg::AuthenticationMessage authenticationResponse;
    bmqp_ctrlmsg::AuthenticateResponse& response =
        authenticationResponse.makeAuthenticateResponse();

    bmqu::MemOutStream authenticationErrorStream;

    BALL_LOG_INFO << "Reauthenticating connection '" << channel->peerUri()
                  << "' with mechanism '" << authenticateRequest.mechanism()
                  << "'";

    int rc = d_authnController_p->authenticate(authenticationErrorStream,
                                               &result,
                                               authenticateRequest.mechanism(),
                                               authenticationData);

    if (rc != 0) {
        BALL_LOG_ERROR << "Reauthentication failed for connection '"
                       << channel->peerUri() << "' with mechanism '"
                       << authenticateRequest.mechanism() << "' [rc: " << rc
                       << ", error: " << authenticationErrorStream.str()
                       << "]";

        response.status().code()     = rc;
        response.status().category() = bmqp_ctrlmsg::StatusCategory::E_REFUSED;
        response.status().message()  = authenticationErrorStream.str();
    }
    else {
        response.status().code()     = 0;
        response.status().category() = bmqp_ctrlmsg::StatusCategory::E_SUCCESS;
        response.lifetimeMs()        = result->lifetimeMs();

        context->setAuthenticationResult(result);

        if (context->state().testAndSwap(State::e_AUTHENTICATING,
                                         State::e_AUTHENTICATED) !=
            State::e_AUTHENTICATING) {
            BALL_LOG_ERROR << "Failed to set (re)authentication state for '"
                           << channel->peerUri()
                           << "' to 'e_AUTHENTICATED' from 'e_AUTHENTICATING'";
            reauthenticateErrorOrTimeout((rc * 10) +
                                             rc_AUTHENTICATION_STATE_INCORRECT,
                                         "reauthenticationError",
                                         channel);
            return;  // RETURN
        }

        // Schedule authentication timeout
        if (result->lifetimeMs().has_value()) {
            bslmt::LockGuard<bslmt::Mutex> guard(
                &context->timeoutHandleMutex());  // MUTEX LOCKED

            if (context->timeoutHandle()) {
                d_scheduler.cancelEvent(&context->timeoutHandle());
            }

            d_scheduler.scheduleEvent(
                &context->timeoutHandle(),
                bsls::TimeInterval(bmqsys::Time::nowMonotonicClock() +
                                   result->lifetimeMs().value()),
                bdlf::BindUtil::bind(
                    &Authenticator::reauthenticateErrorOrTimeout,
                    this,                       // authenticator
                    rc_AUTHENTICATION_TIMEOUT,  // errorCode
                    "authenticationTimeout",    // errorName
                    channel                     // channel
                    ));
        }
    }

    // Send authentication response back to the client
    bmqu::MemOutStream sendResponseErrorStream;
    rc = sendAuthenticationMessage(sendResponseErrorStream,
                                   authenticationResponse,
                                   channel,
                                   context->authenticationEncodingType());
    if (response.status().category() !=
        bmqp_ctrlmsg::StatusCategory::E_SUCCESS) {
        reauthenticateErrorOrTimeout(rc_AUTHENTICATION_FAILED,
                                     "reauthenticationError",
                                     channel);
    }
    else if (rc != 0) {
        BALL_LOG_ERROR << sendResponseErrorStream.str();
        reauthenticateErrorOrTimeout(
            (rc * 10) + rc_SEND_AUTHENTICATION_RESPONSE_FAILED,
            "reauthenticationError",
            channel);
    }

    return;
}

void Authenticator::reauthenticateErrorOrTimeout(
    int                                    errorCode,
    bsl::string_view                       errorName,
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
