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
    context->readCallback(status, numNeeded, blob);
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
    bslma::Allocator*                      allocator)
: d_negotiator_mp(negotiator)
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

    // Create an NegotiationContext for that connection
    bsl::shared_ptr<mqbnet::NegotiationContext> negotiationContext =
        bsl::allocate_shared<mqbnet::NegotiationContext>(d_allocator_p,
                                                         context.get());

    context->setNegotiationContext(negotiationContext);

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

    // This line won't be hit. Since if `scheduleRead` succeeds, the same
    // callback will be triggered in `readCallback`.
    guard.release();
}

}  // close package namespace
}  // close enterprise namespace
