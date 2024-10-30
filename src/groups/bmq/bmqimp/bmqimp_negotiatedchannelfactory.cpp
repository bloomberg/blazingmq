// Copyright 2019-2023 Bloomberg Finance L.P.
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

// bmqimp_negotiatedchannelfactory.cpp                                -*-C++-*-
#include <bmqimp_negotiatedchannelfactory.h>

#include <bmqscm_version.h>
// BMQ
#include <bmqp_event.h>
#include <bmqp_protocol.h>
#include <bmqp_schemaeventbuilder.h>

#include <bmqio_channelutil.h>
#include <bmqu_blob.h>
#include <bmqu_memoutstream.h>
#include <bmqu_weakmemfn.h>

// BDE
#include <ball_log.h>
#include <bdlf_bind.h>
#include <bdlf_placeholder.h>
#include <bdlma_localsequentialallocator.h>
#include <bsl_iostream.h>
#include <bsl_memory.h>
#include <bslma_default.h>
#include <bsls_annotation.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace bmqimp {

namespace {

BALL_LOG_SET_NAMESPACE_CATEGORY("BMQIMP.NEGOTIATEDCHANNELFACTORY");

enum RcEnum {
    rc_SUCCESS                 = 0,
    rc_PACKET_ENCODE_FAILURE   = -1,
    rc_WRITE_FAILURE           = -2,
    rc_READ_FAILURE            = -3,
    rc_READ_CALLBACK_FAILURE   = -4,
    rc_INVALID_MESSAGE_FAILURE = -5,
    rc_INVALID_BROKER_RESPONSE = -6,
    rc_NEGOTIATION_FAILURE     = -7
};

}  // close unnamed namespace

// ------------------------------------
// class NegotiatedChannelFactoryConfig
// ------------------------------------

NegotiatedChannelFactoryConfig::NegotiatedChannelFactoryConfig(
    bmqio::ChannelFactory*                  base,
    const bmqp_ctrlmsg::NegotiationMessage& negotiationMessage,
    const bsls::TimeInterval&               negotiationTimeout,
    bdlbb::BlobBufferFactory*               bufferFactory,
    BlobSpPool*                             blobSpPool_p,
    bslma::Allocator*                       basicAllocator)
: d_baseFactory_p(base)
, d_negotiationMessage(negotiationMessage, basicAllocator)
, d_negotiationTimeout(negotiationTimeout)
, d_bufferFactory_p(bufferFactory)
, d_blobSpPool_p(blobSpPool_p)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    // PRECONDITIONS
    BSLS_ASSERT(base);
    BSLS_ASSERT(bufferFactory);
}

NegotiatedChannelFactoryConfig::NegotiatedChannelFactoryConfig(
    const NegotiatedChannelFactoryConfig& original,
    bslma::Allocator*                     basicAllocator)
: d_baseFactory_p(original.d_baseFactory_p)
, d_negotiationMessage(original.d_negotiationMessage, basicAllocator)
, d_negotiationTimeout(original.d_negotiationTimeout)
, d_bufferFactory_p(original.d_bufferFactory_p)
, d_blobSpPool_p(original.d_blobSpPool_p)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    // NOTHING
}

// ------------------------------
// class NegotiatedChannelFactory
// ------------------------------

/// Temporary; shall remove after 2nd roll out of "new style" brokers.
const char* NegotiatedChannelFactory::k_CHANNEL_PROPERTY_MPS_EX =
    "broker.response.mps.ex";

/// Temporary safety switch to control configure request.
const char* NegotiatedChannelFactory::k_CHANNEL_PROPERTY_CONFIGURE_STREAM =
    "broker.response.confgiure_stream";

// PRIVATE ACCESSORS
void NegotiatedChannelFactory::baseResultCallback(
    const ResultCallback&                  userCb,
    bmqio::ChannelFactoryEvent::Enum       event,
    const bmqio::Status&                   status,
    const bsl::shared_ptr<bmqio::Channel>& channel) const
{
    if (event != bmqio::ChannelFactoryEvent::e_CHANNEL_UP) {
        userCb(event, status, channel);
        return;  // RETURN
    }

    negotiate(channel, userCb);
}

void NegotiatedChannelFactory::negotiate(
    const bsl::shared_ptr<bmqio::Channel>& channel,
    const ResultCallback&                  cb) const
{
    static const bmqp::EncodingType::Enum k_DEFAULT_ENCODING =
        bmqp::EncodingType::e_BER;
    bmqp::SchemaEventBuilder builder(d_config.d_blobSpPool_p,
                                     k_DEFAULT_ENCODING,
                                     d_config.d_allocator_p);
    const int rc = builder.setMessage(d_config.d_negotiationMessage,
                                      bmqp::EventType::e_CONTROL);
    if (rc != 0) {
        BALL_LOG_ERROR << "Negotiation failed [reason: 'packet failed to "
                       << "encode', rc: " << rc << "]";
        bmqio::Status status(bmqio::StatusCategory::e_GENERIC_ERROR,
                             "negotiationError",
                             rc_PACKET_ENCODE_FAILURE);
        cb(bmqio::ChannelFactoryEvent::e_CONNECT_FAILED, status, channel);
        return;  // RETURN
    }

    BALL_LOG_INFO << "Sending negotiation message to '" << channel->peerUri()
                  << "': " << d_config.d_negotiationMessage;
    bmqio::Status status;
    BALL_LOG_TRACE << "Sending blob:\n"
                   << bmqu::BlobStartHexDumper(&builder.blob());
    channel->write(&status, builder.blob());
    if (!status) {
        BALL_LOG_ERROR << "Negotiation failed [reason: 'failed sending packet'"
                       << ", status: " << status << "]";
        status.properties().set("negotiationError", rc_WRITE_FAILURE);
        cb(bmqio::ChannelFactoryEvent::e_CONNECT_FAILED, status, channel);
        return;  // RETURN
    }

    // Initiate a read for the broker's negotiation response message
    bmqio::Channel::ReadCallback readCb = bdlf::BindUtil::bind(
        bmqu::WeakMemFnUtil::weakMemFn(
            &NegotiatedChannelFactory::readPacketsCb,
            d_self.acquireWeak()),
        channel,
        cb,
        bdlf::PlaceHolders::_1,   // status
        bdlf::PlaceHolders::_2,   // numNeeded
        bdlf::PlaceHolders::_3);  // blob

    channel->read(&status,
                  bmqp::Protocol::k_PACKET_MIN_SIZE,
                  readCb,
                  d_config.d_negotiationTimeout);
    if (!status) {
        BALL_LOG_ERROR << "Negotiation failed [reason: 'failed reading "
                       << "packets', status: " << status << "]";
        status.properties().set("negotiationError", rc_READ_FAILURE);
        cb(bmqio::ChannelFactoryEvent::e_CONNECT_FAILED, status, channel);
    }
}

void NegotiatedChannelFactory::readPacketsCb(
    const bsl::shared_ptr<bmqio::Channel>& channel,
    const ResultCallback&                  cb,
    const bmqio::Status&                   status,
    int*                                   numNeeded,
    bdlbb::Blob*                           blob) const
{
    if (!status) {
        // Read failure.
        BALL_LOG_ERROR << "Broker negotiation read callback failed [peer: "
                       << channel->peerUri() << ", status: " << status << "]";
        bmqio::Status st(status);
        st.properties().set("negotiationError", rc_READ_CALLBACK_FAILURE);
        cb(bmqio::ChannelFactoryEvent::e_CONNECT_FAILED, st, channel);
        return;  // RETURN
    }

    bdlbb::Blob packet;
    const int   rc = bmqio::ChannelUtil::handleRead(&packet, numNeeded, blob);
    if (rc != 0) {
        BALL_LOG_ERROR << "Broker negotiation read callback failed [peer: "
                       << channel->peerUri() << ", rc: " << rc << "]";
        bmqio::Status st(bmqio::StatusCategory::e_GENERIC_ERROR,
                         "negotiationError",
                         rc_INVALID_MESSAGE_FAILURE);
        cb(bmqio::ChannelFactoryEvent::e_CONNECT_FAILED, st, channel);
        // No more packets are expected
        *numNeeded = 0;
        return;  // RETURN
    }

    if (packet.length() == 0) {
        // Don't yet have a full packet
        return;  // RETURN
    }

    // We have the whole message.
    *numNeeded = 0;  // No more packets are expected
    BALL_LOG_TRACE << "Read blob:\n" << bmqu::BlobStartHexDumper(&packet);
    onBrokerNegotiationResponse(packet, cb, channel);
}

void NegotiatedChannelFactory::onBrokerNegotiationResponse(
    const bdlbb::Blob&                     packet,
    const ResultCallback&                  cb,
    const bsl::shared_ptr<bmqio::Channel>& channel) const
{
    BALL_LOG_TRACE << "Received a packet:\n"
                   << bmqu::BlobStartHexDumper(&packet);

    bmqp::Event event(&packet, d_config.d_allocator_p);
    if (!event.isValid()) {
        BALL_LOG_ERROR << "Invalid response from broker [reason: 'packet is "
                       << "not a valid BlazingMQ event']: " << event;
        bmqio::Status status(bmqio::StatusCategory::e_GENERIC_ERROR,
                             "negotiationError",
                             rc_INVALID_BROKER_RESPONSE);
        cb(bmqio::ChannelFactoryEvent::e_CONNECT_FAILED, status, channel);
        return;  // RETURN
    }

    if (!event.isControlEvent()) {
        BALL_LOG_ERROR << "Invalid response from broker [reason: 'packet is "
                       << "not a control event']: " << event;
        bmqio::Status status(bmqio::StatusCategory::e_GENERIC_ERROR,
                             "negotiationError",
                             rc_INVALID_BROKER_RESPONSE);
        cb(bmqio::ChannelFactoryEvent::e_CONNECT_FAILED, status, channel);
        return;  // RETURN
    }

    bmqp_ctrlmsg::NegotiationMessage response;
    const int                        rc = event.loadControlEvent(&response);
    if (rc != 0) {
        BALL_LOG_ERROR << "Invalid response from broker [reason: 'control "
                       << "event is not a NegotiationMessage', rc: " << rc
                       << "]: " << event;
        bmqio::Status status(bmqio::StatusCategory::e_GENERIC_ERROR,
                             "negotiationError",
                             rc_INVALID_BROKER_RESPONSE);
        cb(bmqio::ChannelFactoryEvent::e_CONNECT_FAILED, status, channel);
        return;  // RETURN
    }

    if (!response.isBrokerResponseValue()) {
        BALL_LOG_ERROR << "Invalid response from broker [reason: 'control "
                       << "event is not a brokerResponse']: " << response;
        bmqio::Status status(bmqio::StatusCategory::e_GENERIC_ERROR,
                             "negotiationError",
                             rc_INVALID_BROKER_RESPONSE);
        cb(bmqio::ChannelFactoryEvent::e_CONNECT_FAILED, status, channel);
        return;  // RETURN
    }

    // Check if it's an error response
    if (response.brokerResponse().result().category() !=
        bmqp_ctrlmsg::StatusCategory::E_SUCCESS) {
        BALL_LOG_ERROR << "Negotiation with broker failed: " << response;

        // Alarm if the client is using an unsupported SDK version.  We can't
        // use a LogAlarmObserver to write to cerr because the task may not
        // have BALL observer.
        if (response.brokerResponse().result().category() ==
            bmqp_ctrlmsg::StatusCategory::E_NOT_SUPPORTED) {
            bdlma::LocalSequentialAllocator<512> localAllocator(
                d_config.d_allocator_p);
            bmqu::MemOutStream os(&localAllocator);
            os << "BMQALARM [UNSUPPORTED_SDK]: The BlazingMQ version used by "
               << "this client (" << bmqscm::Version::version()
               << ") is no longer supported. Please relink and deploy this "
               << "task.";
            bsl::cerr << os.str() << '\n' << bsl::flush;
            // Also print warning in users log
            BALL_LOG_ERROR << os.str();
        }

        bmqio::Status status(bmqio::StatusCategory::e_GENERIC_ERROR,
                             "negotiationError",
                             rc_NEGOTIATION_FAILURE);
        cb(bmqio::ChannelFactoryEvent::e_CONNECT_FAILED, status, channel);
        return;  // RETURN
    }

    // Print an ALMN catchable string to act.log if the client is using a
    // deprecated SDK version.  We can't use a LogAlarmObserver to write to
    // cerr because the task may not have a BALL observer.
    if (response.brokerResponse().isDeprecatedSdk()) {
        bdlma::LocalSequentialAllocator<512> localAllocator(
            d_config.d_allocator_p);
        bmqu::MemOutStream os(&localAllocator);
        os << "BMQALARM [DEPRECATED_SDK]: The BlazingMQ version used by this "
           << "client (" << bmqscm::Version::version() << ") is deprecated. "
           << "Please relink and deploy this task.";
        bsl::cerr << os.str() << '\n' << bsl::flush;
        // Also print warning in users log
        BALL_LOG_WARN << os.str();
    }

    // Negotiation SUCCEEDED
    BALL_LOG_INFO << "Negotiation with broker was successfull: " << response;

    // Temporary; shall remove after 2nd roll out of "new style" brokers.
    if (bmqp::ProtocolUtil::hasFeature(
            bmqp::MessagePropertiesFeatures::k_FIELD_NAME,
            bmqp::MessagePropertiesFeatures::k_MESSAGE_PROPERTIES_EX,
            response.brokerResponse().brokerIdentity().features())) {
        channel->properties().set(k_CHANNEL_PROPERTY_MPS_EX, 1);
    }

    if (bmqp::ProtocolUtil::hasFeature(
            bmqp::SubscriptionsFeatures::k_FIELD_NAME,
            bmqp::SubscriptionsFeatures::k_CONFIGURE_STREAM,
            response.brokerResponse().brokerIdentity().features())) {
        channel->properties().set(k_CHANNEL_PROPERTY_CONFIGURE_STREAM, 1);
    }

    cb(bmqio::ChannelFactoryEvent::e_CHANNEL_UP, bmqio::Status(), channel);
}

// CREATORS
NegotiatedChannelFactory::NegotiatedChannelFactory(
    const Config&     config,
    bslma::Allocator* basicAllocator)
: d_config(config, basicAllocator)
, d_self(this)  // use default allocator
{
    // NOTHING
}

NegotiatedChannelFactory::~NegotiatedChannelFactory()
{
    // synchronize with any currently running callbacks and prevent future
    // callback invocations
    d_self.invalidate();
}

// MANIPULATORS
void NegotiatedChannelFactory::listen(
    BSLS_ANNOTATION_UNUSED bmqio::Status* status,
    BSLS_ANNOTATION_UNUSED bslma::ManagedPtr<OpHandle>* handle,
    BSLS_ANNOTATION_UNUSED const bmqio::ListenOptions& options,
    BSLS_ANNOTATION_UNUSED const ResultCallback&       cb)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(false && "SDK should not be listening");
}

void NegotiatedChannelFactory::connect(bmqio::Status*               status,
                                       bslma::ManagedPtr<OpHandle>* handle,
                                       const bmqio::ConnectOptions& options,
                                       const ResultCallback&        cb)
{
    d_config.d_baseFactory_p->connect(
        status,
        handle,
        options,
        bdlf::BindUtil::bind(bmqu::WeakMemFnUtil::weakMemFn(
                                 &NegotiatedChannelFactory::baseResultCallback,
                                 d_self.acquireWeak()),
                             cb,
                             bdlf::PlaceHolders::_1,    // event
                             bdlf::PlaceHolders::_2,    // status
                             bdlf::PlaceHolders::_3));  // channel
}

}  // close package namespace
}  // close enterprise namespace
