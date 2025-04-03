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
#include "mqbnet_negotiator.h"
#include <mqba_initialconnectionhandler.h>

/// Implementation Notes
///====================

// MQB
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
    bmqu::MemOutStream errStream;

    if (!status) {
        // do something
        return;  // RETURN
    }

    bdlbb::Blob outPacket;
    int rc = bmqio::ChannelUtil::handleRead(&outPacket, numNeeded, blob);
    if (rc != 0) {
        // do something
        return;  // RETURN
    }

    if (outPacket.length() == 0) {
        // Don't yet have a full blob
        return;  // RETURN
    }

    // Have a full blob, indicate no more bytes needed (we have to do this
    // because 'handleRead' above set it back to 4 at the end).
    *numNeeded = 0;

    rc = decodeNegotiationMessage(errStream, context, outPacket);
    if (rc != 0) {
        // do something
        return;  // RETURN
    }

    switch (context->d_initialConnectionMessage.selectionId()) {
    case bmqp_ctrlmsg::NegotiationMessage::SELECTION_ID_AUTHENTICATE_REQUEST: {
    } break;  // BREAK
    }
}

int InitialConnectionHandler::decodeNegotiationMessage(
    bsl::ostream&                     errorDescription,
    const InitialConnectionContextSp& context,
    bdlbb::Blob&                      blob)
{
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

    int rc = event.loadControlEvent(&(context->d_initialConnectionMessage));
    if (rc != 0) {
        errorDescription << "Invalid negotiation message received (failed "
                         << "decoding ControlEvent): [rc: " << rc << "]:\n"
                         << bmqu::BlobStartHexDumper(&blob);
        return rc_INVALID_CONTROL_EVENT;  // RETURN
    }

    return rc_SUCCESS;
}

void InitialConnectionHandler::scheduleRead(
    const InitialConnectionContextSp& context)
{
    bmqio::Status status;
    context->d_channelSp->read(
        &status,
        bmqp::Protocol::k_PACKET_MIN_SIZE,
        bdlf::BindUtil::bind(&InitialConnectionHandler::readCallback),
        bsls::TimeInterval(k_INITIALCONNECTION_READTIMEOUT));

    if (!status) {
        bmqu::MemOutStream errStream;
        errStream << "Read failed while orchestrating: " << status;
        bsl::string error(errStream.str().data(), errStream.str().length());
        // TODO: remove
        // callback was set to
        //  TCPSessionFactory::negotiationComplete
        //  within the function, resultCallback was set to
        //  TransportManager::sessionResult
        context->d_initialConnectionCb(-1,
                                       error,
                                       bsl::shared_ptr<mqbnet::Session>());
        return;  // RETURN
    }
}

InitialConnectionHandler::InitialConnectionHandler(
    mqbnet::Authenticator* authenticator,
    mqbnet::Negotiator*    negotiator,
    bslma::Allocator*      allocator)
: d_authenticator_p(authenticator)
, d_negotiator_p(negotiator)
, d_threadPool(1, 100, allocator)
, d_allocator_p(allocator)
{
}

InitialConnectionHandler::~InitialConnectionHandler()
{
}

void InitialConnectionHandler::initialConnect(
    mqbnet::InitialConnectionHandlerContext* context,
    const bsl::shared_ptr<bmqio::Channel>&   channel,
    const InitialConnectionCb&               initialConnectionCb)
{
    InitialConnectionContextSp initialConnectionContext;
    initialConnectionContext.createInplace(d_allocator_p);

    initialConnectionContext->d_handlerContext_p    = context;
    initialConnectionContext->d_channelSp           = channel;
    initialConnectionContext->d_initialConnectionCb = initialConnectionCb;
    initialConnectionContext->d_isReversed          = false;
    initialConnectionContext->d_clusterName         = "";
    initialConnectionContext->d_connectionType = ConnectionType::e_UNKNOWN;

    // Create a unique NegotiatorContext for the channel, from the
    // OperationContext.  This shared_ptr is bound to the 'negotiationComplete'
    // callback below, which is what scopes its lifetime.
    bsl::shared_ptr<mqbnet::NegotiatorContext> negotiatorContextSp;
    negotiatorContextSp.createInplace(d_allocator_p, context->isIncoming());
    (*negotiatorContextSp)
        .setUserData(context->userData())
        .setResultState(context->resultState());

    if (!context->isIncoming()) {
        int rc = d_negotiator_p->negotiate(negotiatorContextSp.get(),
                                           channel,
                                           initialConnectionCb);
        if (rc != 0) {
            return;
        }
    }

    scheduleRead(initialConnectionContext);
}

}  // close package namespace
}  // close enterprise namespace
