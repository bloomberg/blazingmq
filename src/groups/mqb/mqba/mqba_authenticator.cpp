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

// BMQ
#include <bmqio_status.h>
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_protocol.h>
#include <bmqp_schemaeventbuilder.h>
#include <bmqu_memoutstream.h>

// BDE
#include <ball_log.h>
#include <bdlma_sequentialallocator.h>
#include <bsl_memory.h>
#include <bsl_ostream.h>

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
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(authenticationMsg.isAuthenticateRequestValue());
    BSLS_ASSERT_SAFE(context->isIncoming());

    BALL_LOG_DEBUG << "Received authentication message from '"
                   << context->channel()->peerUri()
                   << "': " << authenticationMsg;

    bmqp_ctrlmsg::AuthenticationMessage authenticationResponse;
    bmqp_ctrlmsg::AuthenticateResponse& response =
        authenticationResponse.makeAuthenticateResponse();

    // Create an AuthenticationContext for that connection
    bsl::shared_ptr<mqbnet::AuthenticationContext> authenticationContext =
        bsl::allocate_shared<mqbnet::AuthenticationContext>(
            d_allocator_p,
            context.get(),           // initialConnectionContext
            authenticationMsg,       // authenticationMessage
            State::e_AUTHENTICATING  // state
        );

    context->setAuthenticationContext(authenticationContext);

    // Always succeeds for now
    // TODO: For later implementation, plugins will perform authentication,
    // taking the `AuthenticationContext` and updates it with the
    // authentication result.
    response.status().category() = bmqp_ctrlmsg::StatusCategory::E_SUCCESS;
    response.status().code()     = 0;
    response.lifetimeMs()        = 10 * 60 * 1000;

    authenticationContext->state().testAndSwap(
        mqbnet::AuthenticationContext::State::e_AUTHENTICATING,
        mqbnet::AuthenticationContext::State::e_AUTHENTICATED);

    int rc = sendAuthenticationMessage(errorDescription,
                                       authenticationResponse,
                                       authenticationContext);

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
    const AuthenticationContextSp&             context)
{
    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS       = 0,
        rc_BUILD_FAILURE = -1,
        rc_WRITE_FAILURE = -2
    };

    bmqp::EncodingType::Enum encodingType = bmqp::EncodingType::e_BER;

    bdlma::LocalSequentialAllocator<2048> localAllocator(d_allocator_p);

    bmqp::SchemaEventBuilder builder(d_blobSpPool_p,
                                     encodingType,
                                     &localAllocator);

    int rc = builder.setMessage(message, bmqp::EventType::e_AUTHENTICATION);
    if (rc != 0) {
        errorDescription << "Failed building AuthenticationMessage "
                         << "[rc: " << rc << ", message: " << message << "]";
        return rc_BUILD_FAILURE;  // RETURN
    }

    // Send response event
    bmqio::Status status;
    context->initialConnectionContext()->channel()->write(&status,
                                                          *builder.blob());
    if (!status) {
        errorDescription << "Failed sending AuthenticationMessage "
                         << "[status: " << status << ", message: " << message
                         << "]";
        return rc_WRITE_FAILURE;  // RETURN
    }

    return rc_SUCCESS;
}

// CREATORS
Authenticator::Authenticator(BlobSpPool*       blobSpPool,
                             bslma::Allocator* allocator)
: d_allocator_p(allocator)
, d_blobSpPool_p(blobSpPool)
{
    // NOTHING
}

/// Destructor
Authenticator::~Authenticator()
{
    // NOTHING: (required because of inheritance)
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
