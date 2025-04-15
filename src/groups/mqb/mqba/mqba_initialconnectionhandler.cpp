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

// MQB
#include <mqba_initialconnectionhandler.h>

/// Implementation Notes
///====================

// MQB
#include <mqba_sessionnegotiator.h>
#include <mqbblp_clustercatalog.h>

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
#include <bdlma_localsequentialallocator.h>
#include <bsls_timeinterval.h>

namespace BloombergLP {
namespace mqba {

namespace {

BALL_LOG_SET_NAMESPACE_CATEGORY("MQBNET.INITIALCONNECTIONHANDLER");

const int k_INITIALCONNECTION_READTIMEOUT = 3 * 60;  // 3 minutes

}  // close unnamed namespace

// ------------------
// class InitialConnectionHandler
// ------------------

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

    BALL_LOG_TRACE << "InitialConnectionHandler readCb: "
                   << "[status: " << status << ", peer: '"
                   << context->channel()->peerUri() << "']";

    bsl::shared_ptr<mqbnet::Session> session;
    bmqu::MemOutStream               errStream;
    bdlbb::Blob                      outPacket;

    bool isFullBlob     = true;
    bool isContinueRead = false;

    int rc =
        readBlob(errStream, &outPacket, &isFullBlob, status, numNeeded, blob);

    if (!isFullBlob) {
        return;  // RETURN
    }

    if (rc != 0) {
        bsl::string error(errStream.str().data(), errStream.str().length());
        context->initialConnectionCompleteCb()((rc * 10) + rc_READ_BLOB_ERROR,
                                               error,
                                               session);
    }

    rc = processBlob(errStream, &session, &isContinueRead, outPacket, context);

    if (isContinueRead) {
        return scheduleRead(context);  // RETURN
    }

    if (rc != 0) {
        bsl::string error(errStream.str().data(), errStream.str().length());
        context->initialConnectionCompleteCb()((rc * 10) +
                                                   rc_PROCESS_BLOB_ERROR,
                                               error,
                                               session);
    }

    context->initialConnectionCompleteCb()(rc_SUCCESS, "", session);
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
        rc_SUCCESS                     = 0,
        rc_INVALID_NEGOTIATION_MESSAGE = -1,
    };

    bsl::optional<bmqp_ctrlmsg::NegotiationMessage> negotiationMsg;

    int rc = decodeInitialConnectionMessage(errorDescription,
                                            blob,
                                            &negotiationMsg);

    if (rc != 0) {
        return (rc * 10) + rc_INVALID_NEGOTIATION_MESSAGE;  // RETURN
    }

    if (negotiationMsg.has_value()) {
        context->negotiationContext()->d_negotiationMessage =
            negotiationMsg.value();

        rc = d_negotiator_mp->createSessionOnMsgType(
            errorDescription,
            session,
            isContinueRead,
            context->negotiationContext());
    }
    else {
        errorDescription
            << "Decode NegotiationMessage succeeds but nothing is "
               "loaded into the NegotiationMessage.";
        rc = (rc * 10) + rc_INVALID_NEGOTIATION_MESSAGE;
    }

    return rc;
}

int InitialConnectionHandler::decodeInitialConnectionMessage(
    bsl::ostream&                                    errorDescription,
    const bdlbb::Blob&                               blob,
    bsl::optional<bmqp_ctrlmsg::NegotiationMessage>* message)
{
    BSLS_ASSERT(message);

    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS               = 0,
        rc_INVALID_MESSAGE       = -1,
        rc_NOT_CONTROL_EVENT     = -2,
        rc_INVALID_CONTROL_EVENT = -3
    };

    bdlma::LocalSequentialAllocator<2048> localAllocator(d_allocator_p);

    bmqp::Event event(&blob, &localAllocator);

    if (!event.isValid()) {
        errorDescription << "Invalid negotiation message received "
                         << "(packet is not a valid BlazingMQ event):\n"
                         << bmqu::BlobStartHexDumper(&blob);
        return rc_INVALID_MESSAGE;  // RETURN
    }

    if (!event.isControlEvent()) {
        errorDescription << "Invalid negotiation message received "
                         << "(packet is not a ControlEvent):\n"
                         << bmqu::BlobStartHexDumper(&blob);
        return rc_NOT_CONTROL_EVENT;  // RETURN
    }

    bmqp_ctrlmsg::NegotiationMessage negotiationMessage;

    int rc = event.loadControlEvent(&negotiationMessage);

    if (rc != 0) {
        errorDescription << "Invalid negotiation message received (failed "
                         << "decoding ControlEvent): [rc: " << rc << "]:\n"
                         << bmqu::BlobStartHexDumper(&blob);
        return rc_INVALID_CONTROL_EVENT;  // RETURN
    }

    *message = negotiationMessage;

    return rc_SUCCESS;
}

void InitialConnectionHandler::scheduleRead(
    const InitialConnectionContextSp& context)
{
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
        bmqu::MemOutStream errStream;
        errStream << "Read failed while negotiating: " << status;
        bsl::string error(errStream.str().data(), errStream.str().length());
        context->initialConnectionCompleteCb()(
            -1,
            error,
            bsl::shared_ptr<mqbnet::Session>());
        return;  // RETURN
    }
}

InitialConnectionHandler::InitialConnectionHandler(
    bslma::ManagedPtr<mqbnet::Negotiator>& negotiator,
    bslma::Allocator*                      allocator)
: d_negotiator_mp(negotiator)
, d_threadPool(1, 100, allocator)
, d_allocator_p(allocator)
{
}

InitialConnectionHandler::~InitialConnectionHandler()
{
}

void InitialConnectionHandler::handleInitialConnection(
    const InitialConnectionContextSp& context)
{
    // Create an NegotiationContext for that connection
    bsl::shared_ptr<mqbnet::NegotiationContext> negotiationContext;
    negotiationContext.createInplace(d_allocator_p);

    negotiationContext->d_initialConnectionContext_p = context.get();
    negotiationContext->d_isReversed                 = false;
    negotiationContext->d_clusterName                = "";
    negotiationContext->d_connectionType = mqbnet::ConnectionType::e_UNKNOWN;

    context->setNegotiationContext(negotiationContext);

    // Reading for inbound request or continue to read
    // after sending a request ourselves

    bmqu::MemOutStream errStream;

    if (context->isIncoming()) {
        scheduleRead(context);
    }
    else {
        int rc = d_negotiator_mp->negotiateOutboundOrReverse(
            errStream,
            context->negotiationContext());
        if (rc == 0) {
            scheduleRead(context);
        }
        else {
            bsl::string error(errStream.str().data(),
                              errStream.str().length());
            context->initialConnectionCompleteCb()(
                rc,
                error,
                bsl::shared_ptr<mqbnet::Session>());
        }
    }
}

}  // close package namespace
}  // close enterprise namespace
