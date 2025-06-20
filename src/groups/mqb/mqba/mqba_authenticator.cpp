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
#include <bdlmt_timereventscheduler.h>
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
#include <bdlmt_threadpool.h>
#include <bsl_memory.h>
#include <bsl_ostream.h>
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
    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS                    = 0,
        rc_AUTHENTICATING_IN_PROGRESS = -1,
        rc_FAIL_TO_ENQUEUE_JOB        = -2,
    };

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
            bdlf::BindUtil::bind(&Authenticator::reAuthenticateAsync,
                                 this,                    // authenticator
                                 bdlf::PlaceHolders::_1,  // errorDescription,
                                 bdlf::PlaceHolders::_2,  // context,
                                 bdlf::PlaceHolders::_3   // channel
                                 ),                       // reAuthenticateCb
            false,                                        // isReversed
            State::e_AUTHENTICATING                       // state
        );

    context->setAuthenticationContext(authenticationContext);
    authenticationContext->setInitialConnectionContext(context.get());

    // Authenticate
    int rc = authenticateAsync(errorDescription,
                               authenticationContext,
                               context->channel());

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

void Authenticator::initiateOutboundAuthentication(
    const AuthenticationContextSp& context)
{
    BALL_LOG_ERROR << "Not Implemented";
}

int Authenticator::authenticateAsync(
    bsl::ostream&                          errorDescription,
    const AuthenticationContextSp&         context,
    const bsl::shared_ptr<bmqio::Channel>& channel)
{
    int rc = d_threadPool.enqueueJob(
        bdlf::BindUtil::bind(&Authenticator::authenticate,
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

void Authenticator::authenticate(
    const AuthenticationContextSp&         context,
    const bsl::shared_ptr<bmqio::Channel>& channel)
{
    // PRECONDITIONS
    BSLS_ASSERT(context);
    BSLS_ASSERT(context->initialConnectionContext());

    mqbnet::InitialConnectionContext* initialConnectionContext =
        context->initialConnectionContext();

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

        // send the error response back to the client before calling the
        // completion callback
        bmqu::MemOutStream sendResponseErrorStream;
        sendAuthenticationMessage(sendResponseErrorStream,
                                  authenticationResponse,
                                  channel,
                                  context->authenticationEncodingType());

        initialConnectionContext->initialConnectionCompleteCb()(
            rc,
            authenticationErrorStream.str() + sendResponseErrorStream.str(),
            bsl::shared_ptr<mqbnet::Session>());
    }
    else {
        response.status().code()     = 0;
        response.status().category() = bmqp_ctrlmsg::StatusCategory::E_SUCCESS;
        response.lifetimeMs()        = result->lifetimeMs();

        context->setAuthenticationResult(result);

        if (context->state().testAndSwap(State::e_AUTHENTICATING,
                                         State::e_AUTHENTICATED) !=
            State::e_AUTHENTICATING) {
            authenticationErrorStream
                << "Failed to set authentication state for '"
                << channel->peerUri()
                << "' to 'e_AUTHENTICATED' from 'e_AUTHENTICATING'";
            initialConnectionContext->initialConnectionCompleteCb()(
                rc,
                authenticationErrorStream.str(),
                bsl::shared_ptr<mqbnet::Session>());
            return;  // RETURN
        }

        // send the success response back to the client
        bmqu::MemOutStream sendResponseErrorStream;
        rc = sendAuthenticationMessage(sendResponseErrorStream,
                                       authenticationResponse,
                                       channel,
                                       context->authenticationEncodingType());

        if (rc != 0) {
            initialConnectionContext->initialConnectionCompleteCb()(
                rc,
                sendResponseErrorStream.str(),
                bsl::shared_ptr<mqbnet::Session>());
            return;  // RETURN
        }

        setTimer(context, result, channel);
    }
}

int Authenticator::reAuthenticateAsync(
    bsl::ostream&                          errorDescription,
    const AuthenticationContextSp&         context,
    const bsl::shared_ptr<bmqio::Channel>& channel)
{
    int rc = d_threadPool.enqueueJob(
        bdlf::BindUtil::bind(&Authenticator::reAuthenticate,
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

void Authenticator::reAuthenticate(
    const AuthenticationContextSp&         context,
    const bsl::shared_ptr<bmqio::Channel>& channel)
{
    // PRECONDITIONS
    BSLS_ASSERT(context);
    BSLS_ASSERT(context->initialConnectionContext());

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

        // send the error response back to the client before calling the
        // completion callback
        bmqu::MemOutStream sendResponseErrorStream;
        sendAuthenticationMessage(sendResponseErrorStream,
                                  authenticationResponse,
                                  channel,
                                  context->authenticationEncodingType());

        cleanupOnError(rc, "reauthenticationError", context, channel);
    }
    else {
        // Authentication succeeded
        BALL_LOG_INFO << "Reauthentication succeeded for connection '"
                      << channel->peerUri() << "' with mechanism '"
                      << authenticateRequest.mechanism() << "'";

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
            cleanupOnError(rc, "reauthenticationError", context, channel);
            return;  // RETURN
        }

        // send the success response back to the client
        bmqu::MemOutStream sendResponseErrorStream;
        rc = sendAuthenticationMessage(sendResponseErrorStream,
                                       authenticationResponse,
                                       channel,
                                       context->authenticationEncodingType());

        if (rc != 0) {
            cleanupOnError(rc, "reauthenticationError", context, channel);
            return;  // RETURN
        }

        setTimer(context, result, channel);
    }
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
, d_scheduler(bdlmt::TimerEventScheduler(bsls::SystemClockType::e_MONOTONIC,
                                         allocator))
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
        errorDescription << "Failed to start TimerEventScheduler for "
                            "Authenticator [rc: "
                         << rc << "]";
        d_threadPool.stop();
        return rc;  // RETURN
    }

    return 0;
}

void Authenticator::stop()
{
    d_threadPool.stop();
}

int Authenticator::handleAuthentication(
    bsl::ostream&                              errorDescription,
    bool*                                      isContinueRead,
    const InitialConnectionContextSp&          context,
    const bmqp_ctrlmsg::AuthenticationMessage& authenticationMsg)
{
    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS = 0,
        rc_ERROR   = -1,
    };

    bmqu::MemOutStream errStream;
    int                rc = rc_SUCCESS;

    switch (authenticationMsg.selectionId()) {
    case bmqp_ctrlmsg::AuthenticationMessage::
        SELECTION_ID_AUTHENTICATE_REQUEST: {
        rc = onAuthenticationRequest(errStream, authenticationMsg, context);
    } break;  // BREAK
    case bmqp_ctrlmsg::AuthenticationMessage::
        SELECTION_ID_AUTHENTICATE_RESPONSE: {
        rc = onAuthenticationResponse(errStream, authenticationMsg, context);
    } break;  // BREAK
    default: {
        errorDescription
            << "Invalid authentication message received (unknown type): "
            << authenticationMsg;
        return rc_ERROR;  // RETURN
    }
    }

    if (rc == rc_SUCCESS) {
        *isContinueRead = true;
    }

    return rc;
}

int Authenticator::handleReauthentication(
    bsl::ostream&                          errorDescription,
    const AuthenticationContextSp&         context,
    const bsl::shared_ptr<bmqio::Channel>& channel)
{
    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS = 0,
        rc_ERROR   = -1,
    };

    bmqu::MemOutStream errStream;
    int                rc = rc_SUCCESS;

    switch (context->authenticationMessage().selectionId()) {
    case bmqp_ctrlmsg::AuthenticationMessage::
        SELECTION_ID_AUTHENTICATE_REQUEST: {
        rc = reAuthenticateAsync(errorDescription, context, channel);
    } break;  // BREAK
    case bmqp_ctrlmsg::AuthenticationMessage::
        SELECTION_ID_AUTHENTICATE_RESPONSE: {
        BALL_LOG_ERROR << "Reauthentication when receiving authentication "
                          "message is not implemented";
    } break;  // BREAK
    default: {
        errorDescription
            << "Invalid authentication message received (unknown type): "
            << context->authenticationMessage();
        return rc_ERROR;  // RETURN
    }
    }

    return rc;
}

void Authenticator::timeout(const ChannelSp& channel)
{
    BALL_LOG_DEBUG << "Authentication timeout for channel: "
                   << channel->peerUri();

    bmqio::Status status(bmqio::StatusCategory::e_TIMEOUT,
                         "authenticationTimeout",
                         -1,
                         d_allocator_p);
    channel->close(status);
}

void Authenticator::setTimer(const AuthenticationContextSp& context,
                             const AuthenticationResultSp&  result,
                             const ChannelSp&               channel)
{
    if (!result->lifetimeMs().has_value()) {
        BALL_LOG_INFO
            << "Lifetime not set for authentication timer for channel: "
            << channel->peerUri() << ", this indicates an infinite lifetime.";
        return;  // RETURN
    }

    bslmt::LockGuard<bslmt::Mutex> guard(
        &context->authenticationTimerHandleMutex());  // MUTEX LOCKED

    bdlmt::TimerEventScheduler::Handle handle =
        context->authenticationTimerHandle();

    // If the timer handle is invalid, it means that this is the initial
    // authentication and the timer has not been set yet.
    if (handle == bdlmt::TimerEventScheduler::INVALID_HANDLE) {
        handle = d_scheduler.scheduleEvent(
            bsls::TimeInterval(result->lifetimeMs().value()),
            bdlf::BindUtil::bind(&Authenticator::timeout, this, channel));
        context->setAuthenticationTimerHandle(handle);
    }
    else {
        d_scheduler.cancelEvent(handle);

        int rc = d_scheduler.rescheduleEvent(
            handle,
            bsls::TimeInterval(result->lifetimeMs().value()));

        if (rc != 0) {
            BALL_LOG_ERROR << "Failed to reschedule authentication timer for '"
                           << channel->peerUri() << "' [rc: " << rc
                           << ", lifetime: " << result->lifetimeMs().value()
                           << "]";
            bmqio::Status status(bmqio::StatusCategory::e_GENERIC_ERROR,
                                 "reAuthenticationError",
                                 rc,
                                 d_allocator_p);
            channel->close(status);
        }
    }
}

void Authenticator::cleanupOnError(int                errorCode,
                                   const bsl::string& errorDescription,
                                   const AuthenticationContextSp& context,
                                   const ChannelSp&               channel)
{
    {
        bslmt::LockGuard<bslmt::Mutex> guard(
            &context->authenticationTimerHandleMutex());  // MUTEX LOCKED
        d_scheduler.cancelEvent(context->authenticationTimerHandle());
    }

    bmqio::Status status(bmqio::StatusCategory::e_GENERIC_ERROR,
                         errorDescription,
                         errorCode,
                         d_allocator_p);
    channel->close(status);
}

int Authenticator::authenticationOutboundOrReverse(
    const AuthenticationContextSp& context)
{
    BALL_LOG_ERROR << "Not Implemented";

    return -1;
}

}  // close package namespace
}  // close enterprise namespace
