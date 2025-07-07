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
#include <bsls_nullptr.h>
#include <bsls_timeinterval.h>
#include <bslstl_variant.h>

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

    bsl::shared_ptr<mqbnet::Session> session;
    bmqu::MemOutStream               errStream;
    bdlbb::Blob                      outPacket;

    bool isFullBlob     = true;
    bool isContinueRead = false;

    int         rc = rc_SUCCESS;
    bsl::string error;

    // The completeCb is not triggered only when there's more to read
    // (didn't receive a full blob; or received a full blob and
    // successfully scheduled another read)
    bdlb::ScopeExitAny guard(
        bdlf::BindUtil::bind(&InitialConnectionHandler::complete,
                             context,
                             bsl::ref(rc),
                             bsl::ref(error),
                             bsl::ref(session)));

    rc = readBlob(errStream, &outPacket, &isFullBlob, status, numNeeded, blob);
    if (rc != rc_SUCCESS) {
        rc    = (rc * 10) + rc_READ_BLOB_ERROR;
        error = bsl::string(errStream.str().data(), errStream.str().length());
        return;  // RETURN
    }

    if (!isFullBlob) {
        guard.release();
        return;  // RETURN
    }

    rc = processBlob(errStream, &session, &isContinueRead, outPacket, context);
    if (rc != rc_SUCCESS) {
        rc    = (rc * 10) + rc_PROCESS_BLOB_ERROR;
        error = bsl::string(errStream.str().data(), errStream.str().length());
        return;  // RETURN
    }

    if (isContinueRead) {
        rc = scheduleRead(errStream, context);

        if (rc == rc_SUCCESS) {
            guard.release();
        }
    }

    if (rc != rc_SUCCESS) {
        rc    = (rc * 10) + rc_PROCESS_BLOB_ERROR;
        error = bsl::string(errStream.str().data(), errStream.str().length());
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
    bsl::shared_ptr<mqbnet::Session>* session,
    bool*                             isContinueRead,
    const bdlbb::Blob&                blob,
    const InitialConnectionContextSp& context)
{
    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS                               = 0,
        rc_INVALID_NEGOTIATION_MESSAGE           = -1,
        rc_ERROR_AUTHENTICATE_DEFAULT_CREDENTIAL = -1,
    };

    bsl::optional<bsl::variant<bmqp_ctrlmsg::AuthenticationMessage,
                               bmqp_ctrlmsg::NegotiationMessage> >
        message;

    int rc = decodeInitialConnectionMessage(errorDescription,
                                            &message,
                                            blob,
                                            context);

    if (rc != rc_SUCCESS) {
        return (rc * 10) + rc_INVALID_NEGOTIATION_MESSAGE;  // RETURN
    }

    if (!message.has_value()) {
        errorDescription << "Decode AuthenticationMessage or "
                            "NegotiationMessage succeeds but nothing gets "
                            "loaded in.";
        rc = (rc * 10) + rc_INVALID_NEGOTIATION_MESSAGE;
    }
    else if (bsl::holds_alternative<bmqp_ctrlmsg::AuthenticationMessage>(
                 message.value())) {
        rc = d_authenticator_mp->handleAuthentication(
            errorDescription,
            isContinueRead,
            context,
            bsl::get<bmqp_ctrlmsg::AuthenticationMessage>(message.value()));
    }
    else if (bsl::holds_alternative<bmqp_ctrlmsg::NegotiationMessage>(
                 message.value())) {
        // Create NegotiationContext
        bsl::shared_ptr<mqbnet::NegotiationContext> negotiationContext =
            bsl::allocate_shared<mqbnet::NegotiationContext>(
                d_allocator_p,
                context.get(),  // initialConnectionContext
                bsl::get<bmqp_ctrlmsg::NegotiationMessage>(
                    message.value()),               // negotiationMessage
                bsl::string(),                      // clusterName
                mqbnet::ConnectionType::e_UNKNOWN,  // connectionType
                0,                                  // maxMissedHeartbeat
                bsl::nullptr_t(),                   // eventProcessor
                bsl::nullptr_t()                    // cluster
            );

        context->setNegotiationContext(negotiationContext);

        // Received a NegotiationMessage before an AuthenticationMessage,
        // use the default authentication credential to authenticate.
        // In order not to block the IO thread, for default credential, we do
        // negotiation in authentication threads
        if (!context->authenticationContext()) {
            context->setNegotiationCb(bdlf::BindUtil::bind(
                &mqbnet::Negotiator::createSessionOnMsgType,
                d_negotiator_mp.get(),
                bdlf::PlaceHolders::_1,  // errorDescription
                bdlf::PlaceHolders::_2,  // session
                bdlf::PlaceHolders::_3   // context
                ));

            bmqp_ctrlmsg::AuthenticationMessage authenticationMsg;
            bmqp_ctrlmsg::AuthenticateRequest&  authenticateRequest =
                authenticationMsg.makeAuthenticateRequest();

            // Set the default authentication mechanism to "Basic" for now
            authenticateRequest.mechanism() = "Basic";
            authenticateRequest.data()      = bsl::vector<char>(
                d_authenticator_mp->defaultCredential().value().begin(),
                d_authenticator_mp->defaultCredential().value().end());

            rc = d_authenticator_mp->handleAuthentication(errorDescription,
                                                          isContinueRead,
                                                          context,
                                                          authenticationMsg);

            *isContinueRead = false;

            return rc;  // RETURN
        }

        rc = d_negotiator_mp->createSessionOnMsgType(errorDescription,
                                                     session,
                                                     context.get());
    }

    return rc;
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

    BALL_LOG_INFO << "Received blob: " << bmqu::BlobStartHexDumper(&blob);

    if (event.isAuthenticationEvent()) {
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
    // NOTE: In the above binding, we skip '_4' (i.e., Channel*) and
    //       replace it by the channel shared_ptr (inside the context)

    if (!status) {
        errorDescription << "Read failed while negotiating: " << status;
        return rc_READ_ERROR;  // RETURN
    }

    return rc_SUCCESS;
}

void InitialConnectionHandler::complete(
    const InitialConnectionContextSp&       context,
    const int                               rc,
    const bsl::string&                      error,
    const bsl::shared_ptr<mqbnet::Session>& session)
{
    context->complete(rc, error, session);
}

InitialConnectionHandler::InitialConnectionHandler(
    bslma::ManagedPtr<mqbnet::Negotiator>& negotiator,
    mqbnet::Authenticator*                 authenticator,
    bslma::Allocator*                      allocator)
: d_authenticator_mp(authenticator)
, d_negotiator_mp(negotiator)
, d_allocator_p(allocator)
{
}

InitialConnectionHandler::~InitialConnectionHandler()
{
}

void InitialConnectionHandler::handleConnectionFlow(
    const InitialConnectionContextSp& context)
{
    // Reading for inbound request or continue to read
    // after sending a request ourselves

    int         rc = 0;
    bsl::string error;

    // The completeCb is not triggered only when `scheduleRead` succeeds
    // (with or without issuing an outbound message).
    bdlb::ScopeExitAny guard(
        bdlf::BindUtil::bind(&InitialConnectionHandler::complete,
                             context,
                             bsl::ref(rc),
                             bsl::ref(error),
                             bsl::shared_ptr<mqbnet::Session>()));

    bmqu::MemOutStream errStream;

    if (context->isIncoming()) {
        rc = scheduleRead(errStream, context);
    }
    else {
        // TODO: When we are ready to move on to the next step, we should
        // call `authenticationOutbound` here instead before calling
        // `negotiateOutbound`.
        rc = d_negotiator_mp->negotiateOutbound(errStream, context);

        // Send outbound request success, continue to read
        if (rc == 0) {
            rc = scheduleRead(errStream, context);
        }
    }

    if (rc != 0) {
        error = bsl::string(errStream.str().data(), errStream.str().length());
        return;
    }

    guard.release();
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
    handleConnectionFlow(context);
}

}  // close package namespace
}  // close enterprise namespace
