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

// bmqimp_authenticatedchannelfactory.cpp                      -*-C++-*-
#include <bmqimp_authenticatedchannelfactory.h>

#include <bmqscm_version.h>
// BMQ
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_event.h>
#include <bmqp_protocol.h>
#include <bmqp_schemaeventbuilder.h>
#include <bmqsys_time.h>

#include <bmqio_channelutil.h>
#include <bmqt_authncredential.h>
#include <bmqu_blob.h>
#include <bmqu_memoutstream.h>
#include <bmqu_weakmemfn.h>

// BDE
#include <ball_log.h>
#include <bdlf_bind.h>
#include <bdlf_placeholder.h>
#include <bdlma_localsequentialallocator.h>
#include <bsl_algorithm.h>
#include <bsl_iostream.h>
#include <bsl_memory.h>
#include <bsl_variant.h>
#include <bsla_annotations.h>
#include <bsla_unused.h>
#include <bslma_default.h>
#include <bsls_assert.h>
#include <bsls_timeinterval.h>

namespace BloombergLP {
namespace bmqimp {

namespace {

BALL_LOG_SET_NAMESPACE_CATEGORY("BMQIMP.NEGOTIATEDCHANNELFACTORY");

enum RcEnum {
    rc_SUCCESS                         = 0,
    rc_PACKET_ENCODE_FAILURE           = -1,
    rc_WRITE_FAILURE                   = -2,
    rc_READ_FAILURE                    = -3,
    rc_READ_CALLBACK_FAILURE           = -4,
    rc_INVALID_MESSAGE_FAILURE         = -5,
    rc_INVALID_AUTHENTICATION_RESPONSE = -7,
    rc_INVALID_BROKER_RESPONSE         = -8,
    rc_AUTHENTICATION_FAILURE          = -9,
    rc_NEGOTIATION_FAILURE             = -10
};

}  // close unnamed namespace

// ---------------------------------------
// class AuthenticatedChannelFactoryConfig
// ---------------------------------------

AuthenticatedChannelFactoryConfig::AuthenticatedChannelFactoryConfig(
    bmqio::ChannelFactory*    base,
    AuthnCredentialCb         authnCredentialCb,
    const bsls::TimeInterval& authenticationTimeout,
    BlobSpPool*               blobSpPool_p,
    bslma::Allocator*         basicAllocator)
: d_baseFactory_p(base)
, d_authnCredentialCb(authnCredentialCb)
, d_authenticationTimeout(authenticationTimeout)
, d_blobSpPool_p(blobSpPool_p)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    // PRECONDITIONS
    BSLS_ASSERT(base);
}

AuthenticatedChannelFactoryConfig::AuthenticatedChannelFactoryConfig(
    const AuthenticatedChannelFactoryConfig& original,
    bslma::Allocator*                        basicAllocator)
: d_baseFactory_p(original.d_baseFactory_p)
, d_authnCredentialCb(original.d_authnCredentialCb)
, d_authenticationTimeout(original.d_authenticationTimeout)
, d_blobSpPool_p(original.d_blobSpPool_p)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    // NOTHING
}

// ---------------------------------
// class AuthenticatedChannelFactory
// ---------------------------------

// PRIVATE ACCESSORS
void AuthenticatedChannelFactory::baseResultCallback(
    const ResultCallback&                  cb,
    bmqio::ChannelFactoryEvent::Enum       event,
    const bmqio::Status&                   status,
    const bsl::shared_ptr<bmqio::Channel>& channel)
{
    if (event != bmqio::ChannelFactoryEvent::e_CHANNEL_UP) {
        cb(event, status, channel);
        return;  // RETURN
    }

    // We will skip authentication if no authentication credential
    // callback provided.
    if (d_config.d_authnCredentialCb) {
        authenticate(channel, cb);
    }
    else {
        cb(event, status, channel);
    }
}

void AuthenticatedChannelFactory::sendRequest(
    const bsl::shared_ptr<bmqio::Channel>& channel,
    const ResultCallback&                  cb) const
{
    bmqu::MemOutStream errStream;

    bsl::optional<bmqt::AuthnCredential> credential =
        d_config.d_authnCredentialCb(errStream);

    if (!credential.has_value()) {
        BALL_LOG_ERROR << "Failed to get authentication credential: "
                       << errStream.str();
        return;  // RETURN
    }

    bmqp_ctrlmsg::AuthenticationMessage authenticaionMessage;
    bmqp_ctrlmsg::AuthenticateRequest&  ar =
        authenticaionMessage.makeAuthenticateRequest();
    ar.mechanism() = credential.value().mechanism();
    ar.data()      = credential.value().data();

    BALL_LOG_DEBUG << "Sending AuthenticationMessage to channel: "
                   << channel->peerUri()
                   << " with AuthenticationMessage: " << authenticaionMessage;

    static const bmqp::EncodingType::Enum k_DEFAULT_ENCODING =
        bmqp::EncodingType::e_BER;
    bmqp::SchemaEventBuilder builder(d_config.d_blobSpPool_p,
                                     k_DEFAULT_ENCODING,
                                     d_config.d_allocator_p);

    const int rc = builder.setMessage(authenticaionMessage,
                                      bmqp::EventType::e_AUTHENTICATION);

    if (rc != 0) {
        BALL_LOG_ERROR << "Authentication failed [reason: 'packet failed to "
                       << "encode', rc: " << rc << "]";
        bmqio::Status status(bmqio::StatusCategory::e_GENERIC_ERROR,
                             "AuthenticationError",
                             rc_PACKET_ENCODE_FAILURE);
        cb(bmqio::ChannelFactoryEvent::e_CONNECT_FAILED, status, channel);
        return;
    }

    BALL_LOG_INFO << "Sending authentication message to '"
                  << channel->peerUri() << "': " << authenticaionMessage;
    bmqio::Status status;
    BALL_LOG_TRACE << "Sending blob:\n"
                   << bmqu::BlobStartHexDumper(builder.blob().get());
    channel->write(&status, *builder.blob());
    if (!status) {
        BALL_LOG_ERROR
            << "Authentication failed [reason: 'failed sending packet'"
            << ", status: " << status << "]";
        status.properties().set("AuthenticationError", rc_WRITE_FAILURE);
        cb(bmqio::ChannelFactoryEvent::e_CONNECT_FAILED, status, channel);
        return;
    }
}

void AuthenticatedChannelFactory::readResponse(
    const bsl::shared_ptr<bmqio::Channel>& channel,
    const ResultCallback&                  cb) const
{
    // Initiate a read for the broker's authentication response message
    bmqio::Channel::ReadCallback readCb = bdlf::BindUtil::bind(
        bmqu::WeakMemFnUtil::weakMemFn(
            &AuthenticatedChannelFactory::readPacketsCb,
            d_self.acquireWeak()),
        channel,
        cb,
        bdlf::PlaceHolders::_1,   // status
        bdlf::PlaceHolders::_2,   // numNeeded
        bdlf::PlaceHolders::_3);  // blob

    bmqio::Status status;
    channel->read(&status,
                  bmqp::Protocol::k_PACKET_MIN_SIZE,
                  readCb,
                  d_config.d_authenticationTimeout);
    if (!status) {
        BALL_LOG_ERROR << "Authentication failed [reason: 'failed reading "
                       << "packets', status: " << status << "]";
        status.properties().set("authenticationError", rc_READ_FAILURE);
        cb(bmqio::ChannelFactoryEvent::e_CONNECT_FAILED, status, channel);
    }
}

void AuthenticatedChannelFactory::authenticate(
    const bsl::shared_ptr<bmqio::Channel>& channel,
    const ResultCallback&                  cb)
{
    sendRequest(channel, cb);
    readResponse(channel, cb);
}

void AuthenticatedChannelFactory::readPacketsCb(
    const bsl::shared_ptr<bmqio::Channel>& channel,
    const ResultCallback&                  cb,
    const bmqio::Status&                   status,
    int*                                   numNeeded,
    bdlbb::Blob*                           blob)
{
    if (!status) {
        // Read failure.
        BALL_LOG_ERROR << "Broker authentication read callback failed [peer: "
                       << channel->peerUri() << ", status: " << status << "]";
        bmqio::Status st(status);
        st.properties().set("authenticationError", rc_READ_CALLBACK_FAILURE);
        cb(bmqio::ChannelFactoryEvent::e_CONNECT_FAILED, st, channel);
        return;  // RETURN
    }

    bdlbb::Blob packet;
    int         rc = bmqio::ChannelUtil::handleRead(&packet, numNeeded, blob);
    if (rc != 0) {
        BALL_LOG_ERROR << "Broker authentication read callback failed [peer: "
                       << channel->peerUri() << ", rc: " << rc << "]";
        bmqio::Status st(bmqio::StatusCategory::e_GENERIC_ERROR,
                         "authenticationError",
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
    onBrokerAuthenticationResponse(packet, cb, channel);
}

int AuthenticatedChannelFactory::decodeInitialConnectionMessage(
    const bdlbb::Blob&                                              packet,
    bsl::optional<bsl::variant<bmqp_ctrlmsg::AuthenticationMessage,
                               bmqp_ctrlmsg::NegotiationMessage> >* message,
    const ResultCallback&                                           cb,
    const bsl::shared_ptr<bmqio::Channel>& channel) const
{
    BSLS_ASSERT(message);

    bmqp::Event event(&packet, d_config.d_allocator_p);
    if (!event.isValid()) {
        BALL_LOG_ERROR << "Invalid response from broker [reason: 'packet is "
                       << "not a valid BlazingMQ event']: " << event;
        bmqio::Status status(bmqio::StatusCategory::e_GENERIC_ERROR,
                             "negotiationError",
                             rc_INVALID_BROKER_RESPONSE);
        cb(bmqio::ChannelFactoryEvent::e_CONNECT_FAILED, status, channel);
        return bmqio::ChannelFactoryEvent::e_CONNECT_FAILED;  // RETURN
    }

    bmqp_ctrlmsg::AuthenticationMessage authenticaionMessage;
    bmqp_ctrlmsg::NegotiationMessage    negotiationMessage;

    if (event.isAuthenticationEvent()) {
        BALL_LOG_DEBUG << "Received AuthenticationEvent: "
                       << bmqu::BlobStartHexDumper(&packet);
        const int rc = event.loadAuthenticationEvent(&authenticaionMessage);
        if (rc != 0) {
            BALL_LOG_ERROR
                << "Invalid response from broker [reason: 'authentication "
                   "event is not an AuthenticationMessage', rc: "
                << rc << "]: " << event;
            bmqio::Status status(bmqio::StatusCategory::e_GENERIC_ERROR,
                                 "authenticationError",
                                 rc_INVALID_AUTHENTICATION_RESPONSE);
            cb(bmqio::ChannelFactoryEvent::e_CONNECT_FAILED, status, channel);
            return bmqio::ChannelFactoryEvent::e_CONNECT_FAILED;  // RETURN
        }

        if (!authenticaionMessage.isAuthenticateResponseValue()) {
            BALL_LOG_ERROR
                << "Invalid response from broker [reason: 'authentication "
                << "event is not an authenticationResponse']: "
                << authenticaionMessage;
            bmqio::Status status(bmqio::StatusCategory::e_GENERIC_ERROR,
                                 "authenticationError",
                                 rc_INVALID_AUTHENTICATION_RESPONSE);
            cb(bmqio::ChannelFactoryEvent::e_CONNECT_FAILED, status, channel);
            return bmqio::ChannelFactoryEvent::e_CONNECT_FAILED;  // RETURN
        }

        *message = authenticaionMessage;
    }
    else if (event.isControlEvent()) {
        BALL_LOG_DEBUG << "Received ControlEvent: "
                       << bmqu::BlobStartHexDumper(&packet);
        const int rc = event.loadControlEvent(&negotiationMessage);
        if (rc != 0) {
            BALL_LOG_ERROR << "Invalid response from broker [reason: 'control "
                           << "event is not a NegotiationMessage', rc: " << rc
                           << "]: " << event;
            bmqio::Status status(bmqio::StatusCategory::e_GENERIC_ERROR,
                                 "negotiationError",
                                 rc_INVALID_BROKER_RESPONSE);
            cb(bmqio::ChannelFactoryEvent::e_CONNECT_FAILED, status, channel);
            return bmqio::ChannelFactoryEvent::e_CONNECT_FAILED;  // RETURN
        }

        if (!negotiationMessage.isBrokerResponseValue()) {
            BALL_LOG_ERROR << "Invalid response from broker [reason: 'control "
                           << "event is not a brokerResponse']: "
                           << negotiationMessage;
            bmqio::Status status(bmqio::StatusCategory::e_GENERIC_ERROR,
                                 "negotiationError",
                                 rc_INVALID_BROKER_RESPONSE);
            cb(bmqio::ChannelFactoryEvent::e_CONNECT_FAILED, status, channel);
            return bmqio::ChannelFactoryEvent::e_CONNECT_FAILED;  // RETURN
        }

        *message = negotiationMessage;
    }
    else {
        BALL_LOG_ERROR
            << "Invalid response from broker [reason: 'packet is "
            << "not an Authentication event or Control (Negotiation) Event']: "
            << event;
        bmqio::Status status(bmqio::StatusCategory::e_GENERIC_ERROR,
                             "initialConnectionError",
                             rc_INVALID_BROKER_RESPONSE);
        cb(bmqio::ChannelFactoryEvent::e_CONNECT_FAILED, status, channel);
        return bmqio::ChannelFactoryEvent::e_CONNECT_FAILED;  // RETURN
    }

    return 0;
}

void AuthenticatedChannelFactory::onBrokerAuthenticationResponse(
    const bdlbb::Blob&                     packet,
    const ResultCallback&                  cb,
    const bsl::shared_ptr<bmqio::Channel>& channel)
{
    BALL_LOG_TRACE << "Received a packet:\n"
                   << bmqu::BlobStartHexDumper(&packet);

    bmqp::Event event(&packet, d_config.d_allocator_p);
    if (!event.isValid()) {
        BALL_LOG_ERROR << "Invalid response from broker [reason: 'packet is "
                       << "not a valid BlazingMQ event']: " << event;
        bmqio::Status status(bmqio::StatusCategory::e_GENERIC_ERROR,
                             "authenticationError",
                             rc_INVALID_BROKER_RESPONSE);
        cb(bmqio::ChannelFactoryEvent::e_CONNECT_FAILED, status, channel);
        return;  // RETURN
    }

    if (!event.isAuthenticationEvent()) {
        BALL_LOG_ERROR << "Invalid response from broker [reason: 'packet is "
                       << "not an authentication event']: " << event;
        bmqio::Status status(bmqio::StatusCategory::e_GENERIC_ERROR,
                             "authenticationError",
                             rc_INVALID_BROKER_RESPONSE);
        cb(bmqio::ChannelFactoryEvent::e_CONNECT_FAILED, status, channel);
        return;  // RETURN
    }

    bmqp_ctrlmsg::AuthenticationMessage response;
    const int rc = event.loadAuthenticationEvent(&response);
    if (rc != 0) {
        BALL_LOG_ERROR << "Invalid response from broker [reason: 'control "
                       << "event is not an AuthenticationMessage', rc: " << rc
                       << "]: " << event;
        bmqio::Status status(bmqio::StatusCategory::e_GENERIC_ERROR,
                             "authenticationError",
                             rc_INVALID_BROKER_RESPONSE);
        cb(bmqio::ChannelFactoryEvent::e_CONNECT_FAILED, status, channel);
        return;  // RETURN
    }

    if (!response.isAuthenticateResponseValue()) {
        BALL_LOG_ERROR << "Invalid response from broker [reason: 'control "
                       << "event is not an authenticateResponse']: "
                       << response;
        bmqio::Status status(bmqio::StatusCategory::e_GENERIC_ERROR,
                             "authenticationError",
                             rc_INVALID_BROKER_RESPONSE);
        cb(bmqio::ChannelFactoryEvent::e_CONNECT_FAILED, status, channel);
        return;  // RETURN
    }

    const bmqp_ctrlmsg::AuthenticateResponse& authenticateResponse =
        response.authenticateResponse();

    // Check if it's an error response
    if (authenticateResponse.status().category() !=
        bmqp_ctrlmsg::StatusCategory::E_SUCCESS) {
        BALL_LOG_ERROR << "Authentication with broker failed: " << response;

        bmqio::Status status(bmqio::StatusCategory::e_GENERIC_ERROR,
                             "authenticationError",
                             rc_AUTHENTICATION_FAILURE);
        cb(bmqio::ChannelFactoryEvent::e_CONNECT_FAILED, status, channel);
        return;  // RETURN
    }

    // Authentication SUCCEEDED
    BALL_LOG_INFO << "Authentication with broker was successful: " << response;

    // Schedule recurring reauthentication events if lifetime is specified in
    // the response.
    if (authenticateResponse.lifetimeMs().has_value() &&
        authenticateResponse.lifetimeMs().value() > 0) {
        bsl::cout << "has lifetime, response: " << response << bsl::endl;
        int lifetimeMs = authenticateResponse.lifetimeMs().value();

        const int intervalMsWithRatio =
            bsl::max(1, static_cast<int>(lifetimeMs * k_REAUTHN_EARLY_RATIO));
        const int intervalMsWithBuffer = lifetimeMs > k_REAUTHN_EARLY_BUFFER
                                             ? (lifetimeMs -
                                                k_REAUTHN_EARLY_BUFFER)
                                             : k_REAUTHN_EARLY_BUFFER;

        const int intervalMs = bsl::min(intervalMsWithRatio,
                                        intervalMsWithBuffer);

        d_scheduler.scheduleRecurringEvent(
            &d_authnEventHandle,
            bsls::TimeInterval(intervalMs),
            bdlf::BindUtil::bind(
                bmqu::WeakMemFnUtil::weakMemFn(
                    &AuthenticatedChannelFactory::authenticate,
                    d_self.acquireWeak()),
                channel,
                cb));
    }

    cb(bmqio::ChannelFactoryEvent::e_CHANNEL_UP, bmqio::Status(), channel);
}

// CREATORS
AuthenticatedChannelFactory::AuthenticatedChannelFactory(
    const Config&     config,
    bslma::Allocator* basicAllocator)
: d_config(config, basicAllocator)
, d_scheduler(bsls::SystemClockType::e_MONOTONIC, basicAllocator)
, d_authnEventHandle()
, d_self(this)  // use default allocator
{
    // NOTHING
}

AuthenticatedChannelFactory::~AuthenticatedChannelFactory()
{
    // synchronize with any currently running callbacks and prevent future
    // callback invocations
    d_self.invalidate();
}

int AuthenticatedChannelFactory::start()
{
    int rc = d_scheduler.start();
    if (rc != 0) {
        BALL_LOG_ERROR << "Failed to start event scheduler [rc: " << rc << "]";
    }
    return rc;
}

void AuthenticatedChannelFactory::stop()
{
    // Cancel any scheduled reauthentication events
    d_scheduler.cancelEvent(&d_authnEventHandle);
    d_scheduler.stop();
}

// MANIPULATORS
void AuthenticatedChannelFactory::listen(
    BSLA_UNUSED bmqio::Status* status,
    BSLA_UNUSED bslma::ManagedPtr<OpHandle>* handle,
    BSLA_UNUSED const bmqio::ListenOptions& options,
    BSLA_UNUSED const ResultCallback&       cb)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(false && "SDK should not be listening");
}

void AuthenticatedChannelFactory::connect(bmqio::Status*               status,
                                          bslma::ManagedPtr<OpHandle>* handle,
                                          const bmqio::ConnectOptions& options,
                                          const ResultCallback&        cb)
{
    d_config.d_baseFactory_p->connect(
        status,
        handle,
        options,
        bdlf::BindUtil::bind(
            bmqu::WeakMemFnUtil::weakMemFn(
                &AuthenticatedChannelFactory::baseResultCallback,
                d_self.acquireWeak()),
            cb,
            bdlf::PlaceHolders::_1,    // event
            bdlf::PlaceHolders::_2,    // status
            bdlf::PlaceHolders::_3));  // channel
}

}  // close package namespace
}  // close enterprise namespace
