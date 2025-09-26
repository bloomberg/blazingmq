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

// mqba_initialconnectionhandler.cpp                           -*-C++-*-
#include <mqba_initialconnectionhandler.h>

#include <mqbscm_version.h>
// MQB
#include <mqba_sessionnegotiator.h>
#include <mqbblp_clustercatalog.h>
#include <mqbnet_authenticationcontext.h>
#include <mqbnet_initialconnectioncontext.h>
#include <mqbnet_negotiationcontext.h>

// BMQ
#include <bmqio_channel.h>
#include <bmqio_channelutil.h>
#include <bmqio_status.h>
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_event.h>
#include <bmqp_protocol.h>
#include <bmqu_memoutstream.h>

// BDE
#include <ball_log.h>
#include <bdlb_scopeexit.h>
#include <bdlf_bind.h>
#include <bdlma_localsequentialallocator.h>
#include <bsl_memory.h>
#include <bsl_ostream.h>
#include <bsl_string.h>
#include <bsl_variant.h>
#include <bsls_assert.h>
#include <bsls_nullptr.h>
#include <bsls_timeinterval.h>

namespace BloombergLP {
namespace mqba {

namespace {

BALL_LOG_SET_NAMESPACE_CATEGORY("MQBNET.INITIALCONNECTIONHANDLER");

const int k_INITIALCONNECTION_READTIMEOUT = 3 * 60;  // 3 minutes

}  // close unnamed namespace

// ------------------------------
// class InitialConnectionHandler
// ------------------------------

void InitialConnectionHandler::readCallback(
    const bmqio::Status&              status,
    int*                              numNeeded,
    bdlbb::Blob*                      blob,
    const InitialConnectionContextSp& context)
{
    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS            = 0,
        rc_READ_BLOB_ERROR    = -1,
        rc_PROCESS_BLOB_ERROR = -2,
    };

    BALL_LOG_TRACE << "InitialConnectionHandler readCb: [status: " << status
                   << ", peer: '" << context->channel()->peerUri() << "']";

    bdlbb::Blob        outPacket;
    bool               isFullBlob = true;
    bmqu::MemOutStream errStream;
    int                rc = rc_SUCCESS;

    rc = readBlob(errStream, &outPacket, &isFullBlob, status, numNeeded, blob);
    if (rc != rc_SUCCESS) {
        handleEvent((rc * 10) + rc_READ_BLOB_ERROR,
                    errStream.str(),
                    Event::e_ERROR,
                    context);
        return;  // RETURN
    }

    if (!isFullBlob) {
        return;  // RETURN
    }

    rc = processBlob(errStream, outPacket, context);
    if (rc != rc_SUCCESS) {
        handleEvent((rc * 10) + rc_PROCESS_BLOB_ERROR,
                    errStream.str(),
                    Event::e_ERROR,
                    context);
        return;  // RETURN
    }
}

int InitialConnectionHandler::readBlob(bsl::ostream&        errorDescription,
                                       bdlbb::Blob*         outPacket,
                                       bool*                isFullBlob,
                                       const bmqio::Status& status,
                                       int*                 numNeeded,
                                       bdlbb::Blob*         blob)
{
    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS                  = 0,
        rc_READ_ERROR               = -1,
        rc_UNRECOVERABLE_READ_ERROR = -2
    };

    if (!status) {
        errorDescription << "Read error: " << status;
        return (10 * status.category()) + rc_READ_ERROR;  // RETURN
    }

    int rc = bmqio::ChannelUtil::handleRead(outPacket, numNeeded, blob);
    if (rc != 0) {
        // This indicates a non recoverable error...
        errorDescription << "Unrecoverable read error:\n"
                         << bmqu::BlobStartHexDumper(blob);
        return (rc * 10) + rc_UNRECOVERABLE_READ_ERROR;  // RETURN
    }

    if (outPacket->length() == 0) {
        // Don't yet have a full blob
        *isFullBlob = false;
        return rc_SUCCESS;  // RETURN
    }

    // Have a full blob, indicate no more bytes needed (we have to do this
    // because 'handleRead' above set it back to 4 at the end).
    *numNeeded = 0;

    return rc_SUCCESS;
}

int InitialConnectionHandler::processBlob(
    bsl::ostream&                     errorDescription,
    const bdlbb::Blob&                blob,
    const InitialConnectionContextSp& context)
{
    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS                           = 0,
        rc_INVALID_INITIALCONNECTION_MESSAGE = -1,
    };

    bsl::optional<bsl::variant<bmqp_ctrlmsg::AuthenticationMessage,
                               bmqp_ctrlmsg::NegotiationMessage> >
        message;

    int rc = decodeInitialConnectionMessage(errorDescription,
                                            &message,
                                            blob,
                                            context);

    if (rc != rc_SUCCESS) {
        return (rc * 10) + rc_INVALID_INITIALCONNECTION_MESSAGE;  // RETURN
    }

    if (!message.has_value()) {
        errorDescription << "Decode AuthenticationMessage or "
                            "NegotiationMessage succeeds but nothing gets "
                            "loaded in.";
        return (rc * 10) + rc_INVALID_INITIALCONNECTION_MESSAGE;
    }

    if (bsl::holds_alternative<bmqp_ctrlmsg::AuthenticationMessage>(
            message.value())) {
        handleEvent(rc,
                    bsl::string(),
                    Event::e_AUTH_REQUEST,
                    context,
                    message);
    }
    else {
        handleEvent(rc,
                    bsl::string(),
                    Event::e_NEGOTIATION_MESSAGE,
                    context,
                    message);
    }

    return rc_SUCCESS;
}

int InitialConnectionHandler::decodeInitialConnectionMessage(
    bsl::ostream& errorDescription,
    bsl::optional<bsl::variant<bmqp_ctrlmsg::AuthenticationMessage,
                               bmqp_ctrlmsg::NegotiationMessage> >* message,
    const bdlbb::Blob&                                              blob,
    const InitialConnectionContextSp&                               context)
{
    BSLS_ASSERT(message);

    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS                      = 0,
        rc_INVALID_MESSAGE              = -1,
        rc_INVALID_EVENT                = -2,
        rc_INVALID_AUTHENTICATION_EVENT = -3,
        rc_INVALID_CONTROL_EVENT        = -4
    };

    bdlma::LocalSequentialAllocator<2048> localAllocator(d_allocator_p);

    bmqp::Event event(&blob, &localAllocator);

    if (!event.isValid()) {
        errorDescription << "Invalid negotiation message received "
                         << "(packet is not a valid BlazingMQ event):\n"
                         << bmqu::BlobStartHexDumper(&blob);
        return rc_INVALID_MESSAGE;  // RETURN
    }

    bmqp_ctrlmsg::AuthenticationMessage authenticationMessage;
    bmqp_ctrlmsg::NegotiationMessage    negotiationMessage;

    if (event.isAuthenticationEvent()) {
        BALL_LOG_DEBUG << "Received AuthenticationEvent: "
                       << bmqu::BlobStartHexDumper(&blob);
        const int rc = event.loadAuthenticationEvent(&authenticationMessage);
        if (rc != 0) {
            errorDescription
                << "Invalid message received [reason: 'authentication "
                   "event is not an AuthenticationMessage', rc: "
                << rc << "]:" << bmqu::BlobStartHexDumper(&blob);
            return rc_INVALID_AUTHENTICATION_EVENT;  // RETURN
        }

        context->setAuthenticationEncodingType(
            event.authenticationEventEncodingType());
        *message = authenticationMessage;
    }
    else if (event.isControlEvent()) {
        BALL_LOG_DEBUG << "Received ControlEvent: "
                       << bmqu::BlobStartHexDumper(&blob);
        const int rc = event.loadControlEvent(&negotiationMessage);
        if (rc != 0) {
            errorDescription << "Invalid message received [reason: 'control "
                                "event is not a NegotiationMessage', rc: "
                             << rc << "]:" << bmqu::BlobStartHexDumper(&blob);
            return rc_INVALID_CONTROL_EVENT;  // RETURN
        }

        *message = negotiationMessage;
    }
    else {
        errorDescription
            << "Invalid initial connection message received "
            << "(packet is not an AuthenticationEvent or ControlEvent):\n"
            << bmqu::BlobStartHexDumper(&blob);
        return rc_INVALID_EVENT;  // RETURN
    }

    return rc_SUCCESS;
}

int InitialConnectionHandler::scheduleRead(
    bsl::ostream&                     errorDescription,
    const InitialConnectionContextSp& context)
{
    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS    = 0,
        rc_READ_ERROR = -1
    };

    // Schedule a TimedRead
    bmqio::Status status;
    context->channel()->read(
        &status,
        bmqp::Protocol::k_PACKET_MIN_SIZE,
        bdlf::BindUtil::bind(&InitialConnectionHandler::readCallback,
                             this,
                             bdlf::PlaceHolders::_1,  // status
                             bdlf::PlaceHolders::_2,  // numNeeded
                             bdlf::PlaceHolders::_3,  // blob
                             context),
        bsls::TimeInterval(k_INITIALCONNECTION_READTIMEOUT));

    if (!status) {
        errorDescription << "Read failed while negotiating: " << status;
        return rc_READ_ERROR;  // RETURN
    }

    return rc_SUCCESS;
}

InitialConnectionHandler::InitialConnectionHandler(
    mqbnet::Negotiator*    negotiator,
    mqbnet::Authenticator* authenticator,
    bslma::Allocator*      allocator)
: d_authenticator_p(authenticator)
, d_negotiator_p(negotiator)
, d_allocator_p(allocator)
{
}

InitialConnectionHandler::~InitialConnectionHandler()
{
}

void InitialConnectionHandler::handleInitialConnection(
    const InitialConnectionContextSp& context)
{
    // The only counted references to 'InitialConnectionContextSp' are two
    // callbacks:
    //  1.  'InitialConnectionHandler::complete' which is constructed and
    //      destructed on stack.
    //  2.  'InitialConnectionHandler::readCallback' which the channel holds
    //      (see 'InitialConnectionHandler::scheduleRead').
    // That means 'InitialConnectionContext' lives as long as there is the need
    // to read from the channel.  As soon as it sets '*numNeeded = 0', it gets
    // destructed after 'InitialConnectionHandler::readCallback' returns.
    // If there is a need to keep 'InitialConnectionContext' longer, there
    // should be explicit 'bsl::shared_ptr<mqbnet::InitialConnectionContext>'.
    // Reading for inbound request or continue to read
    // after sending a request ourselves

    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS = 0,
    };

    context->setHandleEventCb(
        bdlf::BindUtil::bind(&InitialConnectionHandler::handleEvent,
                             this,
                             bdlf::PlaceHolders::_1,  // statusCode
                             bdlf::PlaceHolders::_2,  // errorDescription
                             bdlf::PlaceHolders::_3,  // input
                             bdlf::PlaceHolders::_4,  // context
                             bdlf::PlaceHolders::_5   // message
                             ));

    if (!context->isIncoming()) {
        // TODO: When we are ready to move on to the next step, we should
        // call `authenticationOutbound` here instead before calling
        // `negotiateOutbound`.
        handleEvent(rc_SUCCESS,
                    bsl::string(),
                    Event::e_OUTBOUND_NEGOTATION,
                    context);
    }
    else {
        bmqu::MemOutStream errStream;
        const int          rc = scheduleRead(errStream, context);
        if (rc != 0) {
            handleEvent(rc, errStream.str(), Event::e_ERROR, context);
        }
    }
}

void InitialConnectionHandler::handleEvent(
    int                               statusCode,
    const bsl::string&                errorDescription,
    Event                             input,
    const InitialConnectionContextSp& context,
    const bsl::optional<bsl::variant<bmqp_ctrlmsg::AuthenticationMessage,
                                     bmqp_ctrlmsg::NegotiationMessage> >&
        message)
{
    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS = 0,
        rc_ERROR   = -1
    };

    bslmt::LockGuard<bslmt::Mutex> guard(&context->mutex());

    BALL_LOG_INFO << "Enter InitialConnectionHandler::handleEvent: "
                  << "state = " << context->state() << ", event = " << input
                  << "; peerUri =" << context->channel()->peerUri()
                  << "; context address = " << context.get();

    State oldState = context->state();
    State newState = context->state();

    bmqu::MemOutStream errStream;
    int                rc = rc_SUCCESS;

    switch (input) {
    case Event::e_OUTBOUND_NEGOTATION: {
        if (oldState == State::e_INITIAL) {
            newState = State::e_NEGOTIATING_OUTBOUND;
            context->setState(newState);

            createNegotiationContext(context);

            rc = d_negotiator_p->negotiateOutbound(errStream, context);
            if (rc == rc_SUCCESS) {
                rc = scheduleRead(errStream, context);
            }
        }
        else {
            rc = rc_ERROR;
            errStream << "Unexpected event received: " << oldState << " -> "
                      << input;
        }
        break;
    }
    case Event::e_AUTH_REQUEST: {
        const bmqp_ctrlmsg::AuthenticationMessage& authenticationMsg =
            bsl::get<bmqp_ctrlmsg::AuthenticationMessage>(message.value());

        if (oldState == State::e_INITIAL) {
            newState = State::e_AUTHENTICATING;
            context->setState(newState);

            rc = d_authenticator_p->handleAuthentication(errStream,
                                                         context,
                                                         authenticationMsg);
        }
        else {
            rc = rc_ERROR;
            errStream << "Unexpected event received: " << oldState << " -> "
                      << input;
        }
        break;
    }
    case Event::e_NEGOTIATION_MESSAGE: {
        const bmqp_ctrlmsg::NegotiationMessage& negotiationMsg =
            bsl::get<bmqp_ctrlmsg::NegotiationMessage>(message.value());

        if (oldState == State::e_INITIAL &&
            negotiationMsg.isClientIdentityValue()) {
            newState = State::e_DEFAULT_AUTHENTICATING;
            context->setState(newState);

            createNegotiationContext(context);
            context->negotiationContext()->setNegotiationMessage(
                negotiationMsg);

            rc = handleDefaultAuthentication(errStream, context);
        }
        else if (oldState == State::e_AUTHENTICATED &&
                 negotiationMsg.isClientIdentityValue()) {
            newState = State::e_NEGOTIATED;
            context->setState(newState);

            createNegotiationContext(context);
            context->negotiationContext()->setNegotiationMessage(
                negotiationMsg);
        }
        else if (oldState == State::e_NEGOTIATING_OUTBOUND &&
                 negotiationMsg.isBrokerResponseValue()) {
            // Received a BrokerResponse
            newState = State::e_NEGOTIATED;
            context->setState(newState);

            BSLS_ASSERT_SAFE(context->negotiationContext());
            context->negotiationContext()->setNegotiationMessage(
                negotiationMsg);
        }
        else {
            rc = rc_ERROR;
            errStream << "Unexpected event received: " << oldState << " -> "
                      << input << " [ negotiationMsg: " << negotiationMsg
                      << " ]";
        }
        break;
    }
    case Event::e_AUTH_SUCCESS: {
        if (oldState == State::e_AUTHENTICATING) {
            newState = State::e_AUTHENTICATED;
            context->setState(newState);

            // Now read Negotiation message
            rc = scheduleRead(errStream, context);
        }
        else if (oldState == State::e_DEFAULT_AUTHENTICATING) {
            newState = State::e_NEGOTIATED;
            context->setState(newState);

            BSLS_ASSERT_SAFE(context->negotiationContext());
            BSLS_ASSERT_SAFE(context->negotiationContext()
                                 ->negotiationMessage()
                                 .isClientIdentityValue());
        }
        else {
            rc = rc_ERROR;
            errStream << "Unexpected event received: " << oldState << " -> "
                      << input;
        }
        break;
    }
    case Event::e_ERROR: {
        rc = statusCode;
        errStream << errorDescription;
    } break;
    case Event::e_NONE: {
        // NOT IMPLEMENTED
        BSLS_ASSERT_SAFE(!"Unexpected event received: e_NONE");
        break;
    }
    default:
        rc = rc_ERROR;
        errStream << "InitialConnectionHandler: "
                  << "unexpected event received: " << input;
    }

    BALL_LOG_INFO << "In initial connection state transition: " << oldState
                  << " -> (" << input << ") -> " << newState;

    bsl::shared_ptr<mqbnet::Session> session;

    if (rc == 0 && newState == State::e_NEGOTIATED) {
        rc = d_negotiator_p->createSessionOnMsgType(errStream,
                                                    &session,
                                                    context.get());
        BALL_LOG_INFO << "Created a session with "
                      << context->channel()->peerUri();
    }

    if (rc != 0 || newState == State::e_NEGOTIATED) {
        BALL_LOG_INFO << "Finished initial connection with rc = " << rc
                      << ", error = '" << errStream.str() << "'";
        guard.release()->unlock();
        context->complete(rc, errStream.str(), session);
    }
}

void InitialConnectionHandler::createNegotiationContext(
    const InitialConnectionContextSp& context)
{
    if (context->negotiationContext()) {
        return;  // RETURN
    }

    bsl::shared_ptr<mqbnet::NegotiationContext> negotiationContext =
        bsl::allocate_shared<mqbnet::NegotiationContext>(
            d_allocator_p,
            context.get()  // initialConnectionContext
        );

    context->setNegotiationContext(negotiationContext);
}

int InitialConnectionHandler::handleDefaultAuthentication(
    bsl::ostream&                     errorDescription,
    const InitialConnectionContextSp& context)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!context->authenticationContext());

    if (!d_authenticator_p->anonymousCredential()) {
        errorDescription << "Anonymous credential is disallowed, "
                         << "cannot negotiate without authentication.";
        return -1;  // RETURN
    }

    bmqp_ctrlmsg::AuthenticationMessage authenticationMessage;
    bmqp_ctrlmsg::AuthenticateRequest&  authenticateRequest =
        authenticationMessage.makeAuthenticateRequest();

    const mqbcfg::Credential& anonymousCredential =
        d_authenticator_p->anonymousCredential().value();
    authenticateRequest.mechanism() = anonymousCredential.mechanism();
    authenticateRequest.data()      = bsl::vector<char>(
        anonymousCredential.identity().begin(),
        anonymousCredential.identity().end());

    const int rc = d_authenticator_p->handleAuthentication(
        errorDescription,
        context,
        authenticationMessage);

    return rc;
}

}  // close package namespace
}  // close enterprise namespace
