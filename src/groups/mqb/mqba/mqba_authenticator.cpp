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

// MQB
#include <mqbblp_clustercatalog.h>

// BMQ

// BDE
#include <ball_log.h>

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
    bsl::ostream&                  errorDescription,
    const AuthenticationContextSp& context)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        context->d_authenticationMessage.isAuthenticateRequestValue());
    BSLS_ASSERT_SAFE(context->d_initialConnectionContext_p->isIncoming() ||
                     context->d_isReversed);

    bmqp_ctrlmsg::AuthenticateRequest& authenticateRequest =
        context->d_authenticationMessage.authenticateRequest();

    BALL_LOG_DEBUG
        << "Received authentication message from '"
        << context->d_initialConnectionContext_p->channel()->peerUri()
        << "': " << authenticateRequest;

    bmqp_ctrlmsg::AuthenticationMessage authenticationResponse;
    bmqp_ctrlmsg::AuthenticateResponse& response =
        authenticationResponse.makeAuthenticateResponse();

    // TODO: authenticate
    if (authenticateRequest.mechanism() == "") {
        BALL_LOG_ERROR << "Error on authentication";

        bmqu::MemOutStream os;
        os << "Mechanism is unspecified";
        response.status().category() =
            bmqp_ctrlmsg::StatusCategory::E_NOT_SUPPORTED;
        response.status().message() = os.str();
        response.status().code()    = -1;
    }
    else {
        response.status().category() = bmqp_ctrlmsg::StatusCategory::E_SUCCESS;
        response.status().code()     = 0;
        response.lifetimeMs()        = 10 * 60 * 1000;
    }

    BALL_LOG_INFO << "send authn response " << authenticationResponse;

    int rc = sendAuthenticationMessage(errorDescription,
                                       authenticationResponse,
                                       context);

    return rc;
}

int Authenticator::onAuthenticationResponse(
    bsl::ostream&                  errorDescription,
    const AuthenticationContextSp& context)
{
    return 0;
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

    // TODO: why do we create a local allocator?
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
    context->d_initialConnectionContext_p->channel()->write(&status,
                                                            *builder.blob());
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
}

// CREATORS
Authenticator::Authenticator(BlobSpPool*       blobSpPool,
                             bslma::Allocator* allocator)
: d_allocator_p(allocator)
, d_blobSpPool_p(blobSpPool)
, d_clusterCatalog_p(0)
{
    // NOTHING
}

/// Destructor
Authenticator::~Authenticator()
{
    // NOTHING: (required because of inheritance)
}

int Authenticator::handleAuthenticationOnMsgType(
    const AuthenticationContextSp& context)
{
    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS = 0,
        rc_ERROR   = -1,
    };

    bmqu::MemOutStream errStream;
    int                rc = rc_SUCCESS;

    switch (context->d_authenticationMessage.selectionId()) {
    case bmqp_ctrlmsg::AuthenticationMessage::
        SELECTION_ID_AUTHENTICATE_REQUEST: {
        BALL_LOG_INFO << "Received authn request: "
                      << context->d_authenticationMessage;
        rc = onAuthenticationRequest(errStream, context);
    } break;  // BREAK
    case bmqp_ctrlmsg::AuthenticationMessage::
        SELECTION_ID_AUTHENTICATE_RESPONSE: {
        BALL_LOG_INFO << "Received authn response: "
                      << context->d_authenticationMessage;
    } break;  // BREAK
    default: {
        errStream << "Invalid authentication message received (unknown type): "
                  << context->d_authenticationMessage;
        bsl::string error(errStream.str().data(), errStream.str().length());
        context->d_initialConnectionContext_p->initialConnectionCompleteCb()(
            rc_ERROR,
            error,
            bsl::shared_ptr<mqbnet::Session>());
        return rc_ERROR;  // RETURN
    }
    }

    if (rc != rc_SUCCESS) {
        bsl::string error(errStream.str().data(), errStream.str().length());
        context->d_initialConnectionContext_p->initialConnectionCompleteCb()(
            rc_ERROR,
            error,
            bsl::shared_ptr<mqbnet::Session>());
    }

    return rc;
}

int Authenticator::authenticationOutboundOrReverse(
    const AuthenticationContextSp& context)
{
    return 0;
}

}  // close package namespace
}  // close enterprise namespace
