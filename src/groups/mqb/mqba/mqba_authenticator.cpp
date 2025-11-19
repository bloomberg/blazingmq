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
#include <mqbcfg_messages.h>
#include <mqbnet_authenticationcontext.h>
#include <mqbnet_initialconnectioncontext.h>
#include <mqbplug_authenticator.h>

// BMQ
#include <bmqex_systemexecutor.h>
#include <bmqio_channel.h>
#include <bmqio_status.h>
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_protocol.h>
#include <bmqp_schemaeventbuilder.h>
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
#include <bsl_vector.h>
#include <bsla_annotations.h>
#include <bslmt_threadutil.h>
#include <bsls_timeinterval.h>

namespace BloombergLP {
namespace mqba {

namespace {

const int k_MIN_THREADS = 1;  // Minimum number of threads in the thread pool
const int k_MAX_THREADS = 3;  // Maximum number of threads in the thread pool

}

// -------------------
// class Authenticator
// -------------------

int Authenticator::onAuthenticationRequest(
    bsl::ostream&                              errorDescription,
    const bmqp_ctrlmsg::AuthenticationMessage& authenticationMsg,
    const InitialConnectionContextSp&          context)
{
    // executed by one of the *IO* threads

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(authenticationMsg.isAuthenticationRequestValue());
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
            mqbnet::AuthenticationState::e_AUTHENTICATING  // state
        );

    context->setAuthenticationContext(authenticationContext);

    // Authenticate
    int rc = authenticateAsync(
        errorDescription,
        authenticationContext,
        context->channel(),
        context->state() == InitialConnectionState::e_ANON_AUTHENTICATING,
        false);

    return rc;
}

int Authenticator::onAuthenticationResponse(
    BSLA_UNUSED bsl::ostream& errorDescription,
    BSLA_UNUSED const bmqp_ctrlmsg::AuthenticationMessage& authenticationMsg,
    BSLA_UNUSED const InitialConnectionContextSp&          context)
{
    // executed by one of the *IO* threads

    BALL_LOG_ERROR << "Not Implemented";

    return -1;
}

int Authenticator::sendAuthenticationResponse(
    bsl::ostream&                            errorDescription,
    int                                      authnRc,
    bsl::string_view                         errorMsg,
    const bsl::optional<bsls::Types::Int64>& lifetimeMs,
    const bsl::shared_ptr<bmqio::Channel>&   channel,
    bmqp::EncodingType::Enum                 authenticationEncodingType)
{
    // executed by an *AUTHENTICATION* thread
    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS       = 0,
        rc_BUILD_FAILURE = -1,
        rc_WRITE_FAILURE = -2
    };

    // Build authentication response message
    bmqp_ctrlmsg::AuthenticationMessage   message;
    bmqp_ctrlmsg::AuthenticationResponse& response =
        message.makeAuthenticationResponse();

    response.status().code()    = authnRc;
    response.status().message() = errorMsg;

    if (authnRc != 0) {
        response.status().category() = bmqp_ctrlmsg::StatusCategory::E_REFUSED;
    }
    else {
        response.status().category() = bmqp_ctrlmsg::StatusCategory::E_SUCCESS;
        response.lifetimeMs()        = lifetimeMs;
    }

    // Send authentication response message
    bdlma::LocalSequentialAllocator<2048> localAllocator(d_allocator_p);
    bmqp::SchemaEventBuilder              builder(d_blobSpPool_p,
                                     authenticationEncodingType,
                                     &localAllocator);

    int rc = builder.setMessage(response, bmqp::EventType::e_AUTHENTICATION);
    if (rc != 0) {
        errorDescription << "Failed building AuthenticationMessage "
                         << "[rc: " << rc << ", message: " << message << "]";
        return rc_BUILD_FAILURE;  // RETURN
    }

    // Send authnResponse event
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
    const AuthenticationContextSp&         context,
    const bsl::shared_ptr<bmqio::Channel>& channel,
    bool                                   isDefaultAuthn,
    bool                                   isReauthn)
{
    // executed by one of the *IO* threads

    const int rc = d_threadPool.enqueueJob(
        bdlf::BindUtil::bindS(d_allocator_p,
                              &Authenticator::authenticate,
                              this,
                              context,
                              channel,
                              isDefaultAuthn,
                              isReauthn));

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
    const bsl::shared_ptr<bmqio::Channel>& channel,
    bool                                   isDefaultAuthn,
    bool                                   isReauthn)
{
    // executed by an *AUTHENTICATION* thread

    // PRECONDITIONS
    BSLS_ASSERT(context);

    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS                             = 0,
        rc_AUTHENTICATION_FAILED               = -1,
        rc_SCHEDULE_REAUTHN_FAILED             = -2,
        rc_SEND_AUTHENTICATION_RESPONSE_FAILED = -3,
    };

    int         rc = rc_SUCCESS;
    bsl::string error;

    // Setup error handler based on whether this is initial authn or reauthn
    bsl::optional<mqbnet::InitialConnectionEvent::Enum> input;
    bsl::optional<bdlb::ScopeExitAny>                   scopeGuard;

    if (isReauthn) {
        // For reauthentication: setup error guard to handle failures
        scopeGuard.emplace(bdlf::BindUtil::bind(
            &mqbnet::AuthenticationContext::onReauthenticateErrorOrTimeout,
            context.get(),
            bsl::ref(rc),
            "reauthenticationError",
            bsl::ref(error),
            channel));
    }
    else {
        // For initial authentication: setup state machine transition
        input.emplace(InitialConnectionEvent::e_ERROR);
        scopeGuard.emplace(bdlf::BindUtil::bind(
            &mqbnet::InitialConnectionContext::handleEvent,
            context->initialConnectionContext(),
            bsl::ref(rc),
            bsl::ref(error),
            bsl::ref(input.value()),
            bsl::monostate()));
    }

    const bmqp_ctrlmsg::AuthenticationRequest& authenticationRequest =
        context->authenticationMessage().authenticationRequest();

    BALL_LOG_INFO << (isReauthn ? "Reauthenticating" : "Authenticating")
                  << " connection '" << channel->peerUri()
                  << "' with mechanism '" << authenticationRequest.mechanism()
                  << "'";

    // Authenticate
    bmqu::MemOutStream                             authnErrStream;
    bsl::shared_ptr<mqbplug::AuthenticationResult> result;
    const bsl::vector<char>&    data = authenticationRequest.data().isNull()
                                           ? bsl::vector<char>()
                                           : authenticationRequest.data().value();
    mqbplug::AuthenticationData authenticationData(data, channel->peerUri());
    const int                   authnRc = d_authnController_p->authenticate(
        authnErrStream,
        &result,
        authenticationRequest.mechanism(),
        authenticationData);

    // For anonymous authentication, skip sending the response and proceed
    // directly to the next negotiation step
    if (isDefaultAuthn) {
        BSLS_ASSERT(!isReauthn);

        if (authnRc != 0) {
            rc    = (authnRc * 10) + rc_AUTHENTICATION_FAILED;
            error = authnErrStream.str();
        }
        else {
            input.value() = InitialConnectionEvent::e_AUTHN_SUCCESS;
        }
        return;  // RETURN
    }

    // Set authentication result, state and schedule reauthentication timer
    if (authnRc == 0) {
        bmqu::MemOutStream scheduleErrStream;
        context->setAuthenticationResult(result);
        const int scheduleRc = context->setAuthenticatedAndScheduleReauthn(
            scheduleErrStream,
            d_scheduler_p,
            result->lifetimeMs(),
            channel);
        if (scheduleRc != 0) {
            rc    = (scheduleRc * 10) + rc_SCHEDULE_REAUTHN_FAILED;
            error = scheduleErrStream.str();
            return;  // RETURN
        }
    }

    // Build authentication response and send it back to the client
    bmqu::MemOutStream sendResponseErrStream;
    const int sendRc = sendAuthenticationResponse(sendResponseErrStream,
                                                  authnRc,
                                                  authnErrStream.str(),
                                                  result->lifetimeMs(),
                                                  channel,
                                                  context->encodingType());

    if (authnRc != 0) {
        rc    = (authnRc * 10) + rc_AUTHENTICATION_FAILED;
        error = authnErrStream.str();
        return;  // RETURN
    }

    if (sendRc != 0) {
        rc    = (sendRc * 10) + rc_SEND_AUTHENTICATION_RESPONSE_FAILED;
        error = sendResponseErrStream.str();
        return;  // RETURN
    }

    // Transition to the next state
    input.value() = InitialConnectionEvent::e_AUTHN_SUCCESS;
}

// CREATORS
Authenticator::Authenticator(
    mqbauthn::AuthenticationController* authnController,
    BlobSpPool*                         blobSpPool,
    bdlmt::EventScheduler*              scheduler,
    bslma::Allocator*                   allocator)
: d_allocator_p(allocator)
, d_authnController_p(authnController)
, d_threadPool(bmqsys::ThreadUtil::defaultAttributes(),
               k_MIN_THREADS,                                // min threads
               k_MAX_THREADS,                                // max threads
               bsls::TimeInterval(120).totalMilliseconds(),  // idle time
               allocator)
, d_blobSpPool_p(blobSpPool)
, d_scheduler_p(scheduler)
, d_isStarted(false)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_allocator_p);
    BSLS_ASSERT_SAFE(d_authnController_p);
    BSLS_ASSERT_SAFE(d_blobSpPool_p);
    BSLS_ASSERT_SAFE(d_scheduler_p);
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
    if (d_isStarted) {
        errorDescription << "start() can only be called once on this object";
        return -1;
    }

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
    // executed by one of the *IO* threads

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
        SELECTION_ID_AUTHENTICATION_REQUEST: {
        rc = onAuthenticationRequest(errorDescription,
                                     authenticationMsg,
                                     context);
    } break;  // BREAK
    case bmqp_ctrlmsg::AuthenticationMessage::
        SELECTION_ID_AUTHENTICATION_RESPONSE: {
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
    BSLA_UNUSED bsl::ostream&                  errorDescription,
    BSLA_UNUSED const AuthenticationContextSp& context)
{
    BALL_LOG_ERROR << "Not Implemented";

    return -1;
}

// ACCESSORS
const bsl::optional<mqbcfg::Credential>&
Authenticator::anonymousCredential() const
{
    return d_authnController_p->anonymousCredential();
}

}  // close package namespace
}  // close enterprise namespace
