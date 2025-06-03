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
#include <mqbnet_authenticationcontext.h>
#include <mqbnet_initialconnectioncontext.h>
#include <mqbplug_authenticator.h>

// BMQ
#include <bmqio_status.h>
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_protocol.h>
#include <bmqp_schemaeventbuilder.h>
#include <bmqsys_threadutil.h>
#include <bmqu_memoutstream.h>

// BDE
#include <ball_log.h>
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

const int k_AUTHENTICATION_READTIMEOUT = 3 * 60;  // 3 minutes

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
            context.get(),           // initialConnectionContext
            authenticationMsg,       // authenticationMessage
            State::e_AUTHENTICATING  // state
        );

    context->setAuthenticationContext(authenticationContext);
    authenticationContext->setInitialConnectionContext(context.get());

    // Authenticate
    int rc = d_threadPool.enqueueJob(
        bdlf::BindUtil::bind(&Authenticator::authenticate, this, context));

    if (rc != 0) {
        errorDescription << "Failed to enqueue authentication job for '"
                         << context->channel()->peerUri() << "' [rc: " << rc
                         << ", message: " << authenticationMsg << "]";
        return rc_FAIL_TO_ENQUEUE_JOB;  // RETURN
    }

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
    const InitialConnectionContextSp&          context)
{
    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS       = 0,
        rc_BUILD_FAILURE = -1,
        rc_WRITE_FAILURE = -2
    };

    bdlma::LocalSequentialAllocator<2048> localAllocator(d_allocator_p);

    bmqp::SchemaEventBuilder builder(d_blobSpPool_p,
                                     context->authenticationEncodingType(),
                                     &localAllocator);

    int rc = builder.setMessage(message, bmqp::EventType::e_AUTHENTICATION);
    if (rc != 0) {
        errorDescription << "Failed building AuthenticationMessage "
                         << "[rc: " << rc << ", message: " << message << "]";
        return rc_BUILD_FAILURE;  // RETURN
    }

    // Send response event
    bmqio::Status status;

    context->channel()->write(&status, *builder.blob());

    if (!status) {
        errorDescription << "Failed sending AuthenticationMessage "
                         << "[status: " << status << ", message: " << message
                         << "]";
        return rc_WRITE_FAILURE;  // RETURN
    }

    return rc_SUCCESS;
}

void Authenticator::authenticate(const InitialConnectionContextSp& context)
{
    // PRECONDITIONS
    BSLS_ASSERT(context->authenticationContext());
    BSLS_ASSERT(context->authenticationContext()->initialConnectionContext());

    const bmqp_ctrlmsg::AuthenticateRequest& authenticateRequest =
        context->authenticationContext()
            ->authenticationMessage()
            .authenticateRequest();

    bsl::shared_ptr<mqbplug::AuthenticationResult> result;
    mqbplug::AuthenticationData                    authenticationData(
        authenticateRequest.data().value(),
        context->channel()->peerUri());

    bmqp_ctrlmsg::AuthenticationMessage authenticationResponse;
    bmqp_ctrlmsg::AuthenticateResponse& response =
        authenticationResponse.makeAuthenticateResponse();

    bmqu::MemOutStream authenticationErrorStream;

    BALL_LOG_INFO << "Authenticating connection '"
                  << context->channel()->peerUri() << "' with mechanism '"
                  << authenticateRequest.mechanism() << "'";

    const int authn_rc = d_authnController_p->authenticate(
        authenticationErrorStream,
        &result,
        authenticateRequest.mechanism(),
        authenticationData);
    if (authn_rc != 0) {
        BALL_LOG_ERROR << "Authentication failed for connection '"
                       << context->channel()->peerUri() << "' with mechanism '"
                       << authenticateRequest.mechanism()
                       << "' [rc: " << authn_rc
                       << ", error: " << authenticationErrorStream.str()
                       << "]";

        response.status().code()     = authn_rc;
        response.status().category() = bmqp_ctrlmsg::StatusCategory::E_REFUSED;
        response.status().message()  = authenticationErrorStream.str();

        context->setAuthenticationContext(bsl::nullptr_t());
    }
    else {
        response.status().code()     = 0;
        response.status().category() = bmqp_ctrlmsg::StatusCategory::E_SUCCESS;
        response.lifetimeMs()        = result->lifetimeMs();

        context->authenticationContext()->setAuthenticationResult(result);

        context->authenticationContext()->state().testAndSwap(
            State::e_AUTHENTICATING,
            State::e_AUTHENTICATED);
    }

    bmqu::MemOutStream sendResponseErrorStream;

    const int send_rc = sendAuthenticationMessage(sendResponseErrorStream,
                                                  authenticationResponse,
                                                  context);

    // TODO: if we close channel here, for initial connection, we won't be able
    // to logOpenSessionTime
    // Maybe this shoule be a callback. So can be triggered under different
    // senarios
    if (authn_rc != 0) {
        bmqio::Status status(bmqio::StatusCategory::e_GENERIC_ERROR,
                             "AuthenticationError",
                             authn_rc,
                             d_allocator_p);
        context->channel()->close(status);
    }
    else if (send_rc != 0) {
        BALL_LOG_ERROR << sendResponseErrorStream.str();

        bmqio::Status status(bmqio::StatusCategory::e_GENERIC_ERROR,
                             "AuthenticationError",
                             send_rc,
                             d_allocator_p);
        context->channel()->close(status);
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

int Authenticator::authenticationOutbound(
    const AuthenticationContextSp& context)
{
    BALL_LOG_ERROR << "Not Implemented";

    return -1;
}

}  // close package namespace
}  // close enterprise namespace
