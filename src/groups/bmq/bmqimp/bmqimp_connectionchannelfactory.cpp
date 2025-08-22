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

// bmqimp_connectionchannelfactory.cpp                      -*-C++-*-
#include <bmqimp_connectionchannelfactory.h>

#include <bmqscm_version.h>
// BMQ
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

// ------------------------------------
// class ConnectionChannelFactoryConfig
// ------------------------------------

ConnectionChannelFactoryConfig::ConnectionChannelFactoryConfig(
    bmqio::ChannelFactory*                  base,
    const bmqp_ctrlmsg::NegotiationMessage& negotiationMessage,
    const bsls::TimeInterval&               connectTimeout,
    AuthnCredentialCb                       authnCredentialCb,
    BlobSpPool*                             blobSpPool_p,
    bslma::Allocator*                       basicAllocator)
: d_baseFactory_p(base)
, d_negotiationMessage(negotiationMessage, basicAllocator)
, d_authenticationMessage()
, d_connectTimeout(connectTimeout)
, d_authnCredentialCb(authnCredentialCb)
, d_blobSpPool_p(blobSpPool_p)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    // PRECONDITIONS
    BSLS_ASSERT(base);
}

ConnectionChannelFactoryConfig::ConnectionChannelFactoryConfig(
    const ConnectionChannelFactoryConfig& original,
    bslma::Allocator*                     basicAllocator)
: d_baseFactory_p(original.d_baseFactory_p)
, d_negotiationMessage(original.d_negotiationMessage, basicAllocator)
, d_authenticationMessage(original.d_authenticationMessage, basicAllocator)
, d_connectTimeout(original.d_connectTimeout)
, d_authnCredentialCb(original.d_authnCredentialCb)
, d_blobSpPool_p(original.d_blobSpPool_p)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    // NOTHING
}

// ------------------------------
// class ConnectionChannelFactory
// ------------------------------

/// Temporary; shall remove after 2nd roll out of "new style" brokers.
const char* ConnectionChannelFactory::k_CHANNEL_PROPERTY_MPS_EX =
    "broker.response.mps.ex";

/// Temporary safety switch to control configure request.
const char* ConnectionChannelFactory::k_CHANNEL_PROPERTY_CONFIGURE_STREAM =
    "broker.response.configure_stream";

const char*
    ConnectionChannelFactory::k_CHANNEL_PROPERTY_HEARTBEAT_INTERVAL_MS =
        "broker.response.heartbeat_interval_ms";

const char*
    ConnectionChannelFactory::k_CHANNEL_PROPERTY_MAX_MISSED_HEARTBEATS =
        "broker.response.max_missed_heartbeats";

// PRIVATE ACCESSORS
void ConnectionChannelFactory::baseResultCallback(
    const ResultCallback&                  userCb,
    bmqio::ChannelFactoryEvent::Enum       event,
    const bmqio::Status&                   status,
    const bsl::shared_ptr<bmqio::Channel>& channel)
{
    if (event != bmqio::ChannelFactoryEvent::e_CHANNEL_UP) {
        userCb(event, status, channel);
        return;  // RETURN
    }

    initialConnect(channel, userCb);
}

void ConnectionChannelFactory::sendRequest(
    const bsl::shared_ptr<bmqio::Channel>& channel,
    const ACTION                           action,
    const ResultCallback&                  cb) const
{
    bsl::string actionStr;
    bsl::string errorProperty;

    if (action == AUTHENTICATION) {
        actionStr     = "authentication";
        errorProperty = "authenticationError";
    }
    else if (action == NEGOTIATION) {
        actionStr     = "negotiation";
        errorProperty = "negotiationError";
    }

    static const bmqp::EncodingType::Enum k_DEFAULT_ENCODING =
        bmqp::EncodingType::e_BER;
    bmqp::SchemaEventBuilder builder(d_config.d_blobSpPool_p,
                                     k_DEFAULT_ENCODING,
                                     d_config.d_allocator_p);

    int rc = 0;

    if (action == AUTHENTICATION) {
        rc = builder.setMessage(d_config.d_authenticationMessage,
                                bmqp::EventType::e_AUTHENTICATION);
    }
    else if (action == NEGOTIATION) {
        rc = builder.setMessage(d_config.d_negotiationMessage,
                                bmqp::EventType::e_CONTROL);
    }

    if (rc != 0) {
        BALL_LOG_ERROR << actionStr << " failed [reason: 'packet failed to "
                       << "encode', rc: " << rc << "]";
        bmqio::Status status(bmqio::StatusCategory::e_GENERIC_ERROR,
                             errorProperty,
                             rc_PACKET_ENCODE_FAILURE);
        cb(bmqio::ChannelFactoryEvent::e_CONNECT_FAILED, status, channel);
        return;
    }

    bmqio::Status status;
    BALL_LOG_TRACE << "Sending blob:\n"
                   << bmqu::BlobStartHexDumper(builder.blob().get());
    channel->write(&status, *builder.blob());
    if (!status) {
        BALL_LOG_ERROR << actionStr
                       << " failed [reason: 'failed sending packet'"
                       << ", status: " << status << "]";
        status.properties().set(errorProperty, rc_WRITE_FAILURE);
        cb(bmqio::ChannelFactoryEvent::e_CONNECT_FAILED, status, channel);
        return;
    }
}

void ConnectionChannelFactory::readResponse(
    const bsl::shared_ptr<bmqio::Channel>& channel,
    const ACTION                           action,
    const ResultCallback&                  cb) const
{
    bmqio::Status status;

    bsl::string        actionStr;
    bsl::string        errorProperty;
    bsls::TimeInterval timeout = d_config.d_connectTimeout;

    if (action == AUTHENTICATION) {
        actionStr     = "authentication";
        errorProperty = "authenticationError";
    }
    else if (action == NEGOTIATION) {
        actionStr     = "negotiation";
        errorProperty = "negotiationError";
    }

    // Initiate a read for the broker's response message
    bmqio::Channel::ReadCallback readCb = bdlf::BindUtil::bind(
        bmqu::WeakMemFnUtil::weakMemFn(
            &ConnectionChannelFactory::readPacketsCb,
            d_self.acquireWeak()),
        channel,
        cb,
        bdlf::PlaceHolders::_1,   // status
        bdlf::PlaceHolders::_2,   // numNeeded
        bdlf::PlaceHolders::_3);  // blob

    channel->read(&status, bmqp::Protocol::k_PACKET_MIN_SIZE, readCb, timeout);
    if (!status) {
        BALL_LOG_ERROR << actionStr << " failed [reason: 'failed reading "
                       << "packets', status: " << status << "]";
        status.properties().set(errorProperty, rc_READ_FAILURE);
        cb(bmqio::ChannelFactoryEvent::e_CONNECT_FAILED, status, channel);
    }
}

void ConnectionChannelFactory::initialConnect(
    const bsl::shared_ptr<bmqio::Channel>& channel,
    const ResultCallback&                  cb)
{
    authenticate(channel, cb);
    negotiate(channel, cb);
}

void ConnectionChannelFactory::authenticate(
    const bsl::shared_ptr<bmqio::Channel>& channel,
    const ResultCallback&                  cb)
{
    // We will skip authentication if there's no authentication credential
    // callback.
    if (!d_config.d_authnCredentialCb) {
        return;  // RETURN
    }

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

    d_config.d_authenticationMessage = authenticaionMessage;

    BALL_LOG_DEBUG << "Sending AuthenticationMessage to channel: "
                   << channel->peerUri()
                   << " with AuthenticationMessage: " << authenticaionMessage;

    sendRequest(channel, AUTHENTICATION, cb);
    readResponse(channel, AUTHENTICATION, cb);
}

void ConnectionChannelFactory::negotiate(
    const bsl::shared_ptr<bmqio::Channel>& channel,
    const ResultCallback&                  cb) const
{
    BALL_LOG_DEBUG << "Sending NegotiationMessage to channel: "
                   << channel->peerUri() << " with NegotiationMessage: "
                   << d_config.d_negotiationMessage;

    sendRequest(channel, NEGOTIATION, cb);
    readResponse(channel, NEGOTIATION, cb);
}

void ConnectionChannelFactory::readPacketsCb(
    const bsl::shared_ptr<bmqio::Channel>& channel,
    const ResultCallback&                  cb,
    const bmqio::Status&                   status,
    int*                                   numNeeded,
    bdlbb::Blob*                           blob)
{
    if (!status) {
        // Read failure.
        BALL_LOG_ERROR << "Broker read callback failed [peer: "
                       << channel->peerUri() << ", status: " << status << "]";
        bmqio::Status st(status);
        st.properties().set("initialConnectionError",
                            rc_READ_CALLBACK_FAILURE);
        cb(bmqio::ChannelFactoryEvent::e_CONNECT_FAILED, st, channel);
        return;  // RETURN
    }

    bdlbb::Blob packet;
    int         rc = bmqio::ChannelUtil::handleRead(&packet, numNeeded, blob);
    if (rc != 0) {
        BALL_LOG_ERROR << "Broker read callback failed [peer: "
                       << channel->peerUri() << ", rc: " << rc << "]";
        bmqio::Status st(bmqio::StatusCategory::e_GENERIC_ERROR,
                         "initialConnectionError",
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

    // Process the received blob
    bsl::optional<bsl::variant<bmqp_ctrlmsg::AuthenticationMessage,
                               bmqp_ctrlmsg::NegotiationMessage> >
        message;

    rc = decodeInitialConnectionMessage(packet, &message, cb, channel);

    // Handle authentication or negotiation based on the response message type

    if (rc != 0) {
        return;  // RETURN
    }

    if (!message.has_value()) {
        BALL_LOG_ERROR << "Decode AuthenticationMessage or NegotiationMessage "
                          "succeeds but nothing gets loaded in.";
        return;  // RETURN
    }

    if (bsl::holds_alternative<bmqp_ctrlmsg::AuthenticationMessage>(
            message.value())) {
        onBrokerAuthenticationResponse(
            bsl::get<bmqp_ctrlmsg::AuthenticationMessage>(message.value()),
            cb,
            channel);
    }
    else {
        onBrokerNegotiationResponse(
            bsl::get<bmqp_ctrlmsg::NegotiationMessage>(message.value()),
            cb,
            channel);
    }
}

int ConnectionChannelFactory::decodeInitialConnectionMessage(
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

void ConnectionChannelFactory::onBrokerAuthenticationResponse(
    const bmqp_ctrlmsg::AuthenticationMessage& response,
    const ResultCallback&                      cb,
    const bsl::shared_ptr<bmqio::Channel>&     channel)
{
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
    if (authenticateResponse.lifetimeMs()) {
        int lifetimeMs = authenticateResponse.lifetimeMs().value();
        int intervalMs = bsl::min(lifetimeMs - k_REAUTHN_EARLY_BUFFER,
                                  lifetimeMs * k_REAUTHN_EARLY_RATIO);
        d_scheduler.scheduleRecurringEvent(
            &d_authnEventHandle,
            bsls::TimeInterval(intervalMs),
            bdlf::BindUtil::bind(bmqu::WeakMemFnUtil::weakMemFn(
                                     &ConnectionChannelFactory::authenticate,
                                     d_self.acquireWeak()),
                                 channel,
                                 cb));
    }

    negotiate(channel, cb);
}

void ConnectionChannelFactory::onBrokerNegotiationResponse(
    const bmqp_ctrlmsg::NegotiationMessage& response,
    const ResultCallback&                   cb,
    const bsl::shared_ptr<bmqio::Channel>&  channel) const
{
    const bmqp_ctrlmsg::BrokerResponse& brokerResponse =
        response.brokerResponse();

    // Check if it's an error response
    if (brokerResponse.result().category() !=
        bmqp_ctrlmsg::StatusCategory::E_SUCCESS) {
        BALL_LOG_ERROR << "Negotiation with broker failed: " << response;

        // Alarm if the client is using an unsupported SDK version.  We can't
        // use a LogAlarmObserver to write to cerr because the task may not
        // have BALL observer.
        if (brokerResponse.result().category() ==
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
    if (brokerResponse.isDeprecatedSdk()) {
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
    BALL_LOG_INFO << "Negotiation with broker was successful: " << response;

    // Temporary; shall remove after 2nd roll out of "new style" brokers.
    if (bmqp::ProtocolUtil::hasFeature(
            bmqp::MessagePropertiesFeatures::k_FIELD_NAME,
            bmqp::MessagePropertiesFeatures::k_MESSAGE_PROPERTIES_EX,
            brokerResponse.brokerIdentity().features())) {
        channel->properties().set(k_CHANNEL_PROPERTY_MPS_EX, 1);
    }

    if (bmqp::ProtocolUtil::hasFeature(
            bmqp::SubscriptionsFeatures::k_FIELD_NAME,
            bmqp::SubscriptionsFeatures::k_CONFIGURE_STREAM,
            brokerResponse.brokerIdentity().features())) {
        channel->properties().set(k_CHANNEL_PROPERTY_CONFIGURE_STREAM, 1);
    }

    channel->properties().set(k_CHANNEL_PROPERTY_HEARTBEAT_INTERVAL_MS,
                              brokerResponse.heartbeatIntervalMs());
    channel->properties().set(k_CHANNEL_PROPERTY_MAX_MISSED_HEARTBEATS,
                              brokerResponse.maxMissedHeartbeats());

    cb(bmqio::ChannelFactoryEvent::e_CHANNEL_UP, bmqio::Status(), channel);
}

// CREATORS
ConnectionChannelFactory::ConnectionChannelFactory(
    const Config&     config,
    bslma::Allocator* basicAllocator)
: d_config(config, basicAllocator)
, d_scheduler(bsls::SystemClockType::e_MONOTONIC, basicAllocator)
, d_authnEventHandle()
, d_self(this)  // use default allocator
{
    // NOTHING
}

ConnectionChannelFactory::~ConnectionChannelFactory()
{
    // synchronize with any currently running callbacks and prevent future
    // callback invocations
    d_self.invalidate();
}

int ConnectionChannelFactory::start()
{
    int rc = d_scheduler.start();
    if (rc != 0) {
        BALL_LOG_ERROR << "Failed to start event scheduler [rc: " << rc << "]";
    }
    return rc;
}

void ConnectionChannelFactory::stop()
{
    // Cancel any scheduled reauthentication events
    d_scheduler.cancelEvent(&d_authnEventHandle);
    d_scheduler.stop();
}

// MANIPULATORS
void ConnectionChannelFactory::listen(
    BSLA_UNUSED bmqio::Status* status,
    BSLA_UNUSED bslma::ManagedPtr<OpHandle>* handle,
    BSLA_UNUSED const bmqio::ListenOptions& options,
    BSLA_UNUSED const ResultCallback&       cb)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(false && "SDK should not be listening");
}

void ConnectionChannelFactory::connect(bmqio::Status*               status,
                                       bslma::ManagedPtr<OpHandle>* handle,
                                       const bmqio::ConnectOptions& options,
                                       const ResultCallback&        cb)
{
    d_config.d_baseFactory_p->connect(
        status,
        handle,
        options,
        bdlf::BindUtil::bind(bmqu::WeakMemFnUtil::weakMemFn(
                                 &ConnectionChannelFactory::baseResultCallback,
                                 d_self.acquireWeak()),
                             cb,
                             bdlf::PlaceHolders::_1,    // event
                             bdlf::PlaceHolders::_2,    // status
                             bdlf::PlaceHolders::_3));  // channel
}

}  // close package namespace
}  // close enterprise namespace
