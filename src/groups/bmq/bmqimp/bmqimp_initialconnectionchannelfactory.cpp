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

// bmqimp_initialconnectionchannelfactory.cpp -*-C++-*-
#include <bmqimp_initialconnectionchannelfactory.h>

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

BALL_LOG_SET_NAMESPACE_CATEGORY("BMQIMP.INITIALCONNECTIONCHANNELFACTORY");

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
// class InitialConnectionChannelFactoryConfig
// ------------------------------------

InitialConnectionChannelFactoryConfig::InitialConnectionChannelFactoryConfig(
    bmqio::ChannelFactory*                     base,
    const bmqp_ctrlmsg::AuthenticationMessage& authenticationMessage,
    const bsls::TimeInterval&                  authenticationTimeout,
    const bmqp_ctrlmsg::NegotiationMessage&    negotiationMessage,
    const bsls::TimeInterval&                  negotiationTimeout,
    BlobSpPool*                                blobSpPool_p,
    bslma::Allocator*                          basicAllocator)
: d_baseFactory_p(base)
, d_authenticationMessage(authenticationMessage, basicAllocator)
, d_authenticationTimeout(authenticationTimeout)
, d_negotiationMessage(negotiationMessage, basicAllocator)
, d_negotiationTimeout(negotiationTimeout)
, d_blobSpPool_p(blobSpPool_p)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    // PRECONDITIONS
    BSLS_ASSERT(base);
}

InitialConnectionChannelFactoryConfig::InitialConnectionChannelFactoryConfig(
    const InitialConnectionChannelFactoryConfig& original,
    bslma::Allocator*                            basicAllocator)
: d_baseFactory_p(original.d_baseFactory_p)
, d_authenticationMessage(original.d_authenticationMessage, basicAllocator)
, d_authenticationTimeout(original.d_authenticationTimeout)
, d_negotiationMessage(original.d_negotiationMessage, basicAllocator)
, d_negotiationTimeout(original.d_negotiationTimeout)
, d_blobSpPool_p(original.d_blobSpPool_p)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    // NOTHING
}

// ------------------------------
// class InitialConnectionChannelFactory
// ------------------------------

/// Temporary; shall remove after 2nd roll out of "new style" brokers.
const char* InitialConnectionChannelFactory::k_CHANNEL_PROPERTY_MPS_EX =
    "broker.response.mps.ex";

/// Temporary safety switch to control configure request.
const char*
    InitialConnectionChannelFactory::k_CHANNEL_PROPERTY_CONFIGURE_STREAM =
        "broker.response.configure_stream";

const char*
    InitialConnectionChannelFactory::k_CHANNEL_PROPERTY_HEARTBEAT_INTERVAL_MS =
        "broker.response.heartbeat_interval_ms";

const char*
    InitialConnectionChannelFactory::k_CHANNEL_PROPERTY_MAX_MISSED_HEARTBEATS =
        "broker.response.max_missed_heartbeats";

// PRIVATE ACCESSORS
void InitialConnectionChannelFactory::baseResultCallback(
    const ResultCallback&                  userCb,
    bmqio::ChannelFactoryEvent::Enum       event,
    const bmqio::Status&                   status,
    const bsl::shared_ptr<bmqio::Channel>& channel) const
{
    if (event != bmqio::ChannelFactoryEvent::e_CHANNEL_UP) {
        userCb(event, status, channel);
        return;  // RETURN
    }

    authenticate(channel, userCb);
}

void InitialConnectionChannelFactory::sendRequest(
    const bsl::shared_ptr<bmqio::Channel>& channel,
    const ACTION                           action,
    const ResultCallback&                  cb) const
{
    BALL_LOG_INFO << "At InitialConnectionChannelFactory::sendRequest";

    bsl::string actionStr;
    bsl::string errorProperty;

    if (action == AUTHENTICATION) {
        actionStr     = "authentication";
        errorProperty = "authenticationError";
        BALL_LOG_INFO << "User " << actionStr << " with "
                      << d_config.d_authenticationMessage;
    }
    else if (action == NEGOTIATION) {
        actionStr     = "negotiation";
        errorProperty = "negotiationError";
        BALL_LOG_INFO << "User " << actionStr << " with "
                      << d_config.d_negotiationMessage;
    }

    static const bmqp::EncodingType::Enum k_DEFAULT_ENCODING =
        bmqp::EncodingType::e_BER;
    bmqp::SchemaEventBuilder builder(d_config.d_blobSpPool_p,
                                     k_DEFAULT_ENCODING,
                                     d_config.d_allocator_p);

    int rc = 0;

    if (action == AUTHENTICATION) {
        BALL_LOG_INFO << "Trying to send authn message: "
                      << d_config.d_authenticationMessage;
        rc = builder.setMessage(d_config.d_authenticationMessage,
                                bmqp::EventType::e_AUTHENTICATION);
    }
    else if (action == NEGOTIATION) {
        BALL_LOG_INFO << "Trying to send nego message: "
                      << d_config.d_negotiationMessage;
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

    BALL_LOG_INFO << "Sending " << actionStr << " message to '"
                  << channel->peerUri();

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

void InitialConnectionChannelFactory::readResponse(
    const bsl::shared_ptr<bmqio::Channel>& channel,
    const ACTION                           action,
    const ResultCallback&                  cb) const
{
    bmqio::Status status;

    bsl::string        actionStr;
    bsl::string        errorProperty;
    bsls::TimeInterval timeout;

    if (action == AUTHENTICATION) {
        actionStr     = "authentication";
        errorProperty = "authenticationError";
        timeout       = d_config.d_authenticationTimeout;
    }
    else if (action == NEGOTIATION) {
        actionStr     = "negotiation";
        errorProperty = "negotiationError";
        timeout       = d_config.d_negotiationTimeout;
    }

    BALL_LOG_INFO << "Read response (" << actionStr << ")";

    // Initiate a read for the broker's response message
    bmqio::Channel::ReadCallback readCb = bdlf::BindUtil::bind(
        bmqu::WeakMemFnUtil::weakMemFn(
            &InitialConnectionChannelFactory::readPacketsCb,
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

void InitialConnectionChannelFactory::authenticate(
    const bsl::shared_ptr<bmqio::Channel>& channel,
    const ResultCallback&                  cb) const
{
    BALL_LOG_INFO << "Client authenticate";
    sendRequest(channel, AUTHENTICATION, cb);
    readResponse(channel, AUTHENTICATION, cb);
}

void InitialConnectionChannelFactory::negotiate(
    const bsl::shared_ptr<bmqio::Channel>& channel,
    const ResultCallback&                  cb) const
{
    BALL_LOG_INFO << "Client negotiate";
    sendRequest(channel, NEGOTIATION, cb);
    readResponse(channel, NEGOTIATION, cb);
}

void InitialConnectionChannelFactory::readPacketsCb(
    const bsl::shared_ptr<bmqio::Channel>& channel,
    const ResultCallback&                  cb,
    const bmqio::Status&                   status,
    int*                                   numNeeded,
    bdlbb::Blob*                           blob) const
{
    BALL_LOG_INFO << "At readPacketsCb";

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
    BALL_LOG_TRACE << "Read blob:\n" << bmqu::BlobStartHexDumper(&packet);

    // Process the received blob
    bsl::optional<bmqp_ctrlmsg::AuthenticationMessage> authenticationMsg;
    bsl::optional<bmqp_ctrlmsg::NegotiationMessage>    negotiationMsg;

    BALL_LOG_INFO << "[readPacketsCb]: decode Message (auth or nego)";
    rc = decodeInitialConnectionMessage(packet,
                                        &authenticationMsg,
                                        &negotiationMsg,
                                        cb,
                                        channel);

    // Handle authentication or negotiation based on the response message type
    if (rc == 0) {
        if (authenticationMsg.has_value()) {
            BALL_LOG_INFO
                << "[readPacketsCb]: read an AuthenticationMessage (response)";
            onBrokerAuthenticationResponse(authenticationMsg.value(),
                                           cb,
                                           channel);
        }
        else if (negotiationMsg.has_value()) {
            BALL_LOG_INFO
                << "[readPacketsCb]: read a NegotiationMessage (response)";
            onBrokerNegotiationResponse(negotiationMsg.value(), cb, channel);
        }
        else {
            BALL_LOG_ERROR << "HAS TO BE EITHER";
        }
    }
}

int InitialConnectionChannelFactory::decodeInitialConnectionMessage(
    const bdlbb::Blob&                                  packet,
    bsl::optional<bmqp_ctrlmsg::AuthenticationMessage>* authenticationMsg,
    bsl::optional<bmqp_ctrlmsg::NegotiationMessage>*    negotiationMsg,
    const ResultCallback&                               cb,
    const bsl::shared_ptr<bmqio::Channel>&              channel) const
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
        return bmqio::ChannelFactoryEvent::e_CONNECT_FAILED;  // RETURN
    }

    bmqp_ctrlmsg::AuthenticationMessage authenticationMessage;
    bmqp_ctrlmsg::NegotiationMessage    negotiationMessage;

    if (event.isAuthenticationEvent()) {
        const int rc = event.loadAuthenticationEvent(&authenticationMessage);
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

        if (!authenticationMessage.isAuthenticateResponseValue()) {
            BALL_LOG_ERROR
                << "Invalid response from broker [reason: 'authentication "
                << "event is not an authenticationResponse']: "
                << authenticationMessage;
            bmqio::Status status(bmqio::StatusCategory::e_GENERIC_ERROR,
                                 "authenticationError",
                                 rc_INVALID_AUTHENTICATION_RESPONSE);
            cb(bmqio::ChannelFactoryEvent::e_CONNECT_FAILED, status, channel);
            return bmqio::ChannelFactoryEvent::e_CONNECT_FAILED;  // RETURN
        }

        *authenticationMsg = authenticationMessage;
    }
    else if (event.isControlEvent()) {
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

        *negotiationMsg = negotiationMessage;
    }
    else {
        BALL_LOG_ERROR << "Invalid response from broker [reason: 'packet is "
                       << "not a control event or authentication event']: "
                       << event;
        bmqio::Status status(bmqio::StatusCategory::e_GENERIC_ERROR,
                             "initialConnectionError",
                             rc_INVALID_BROKER_RESPONSE);
        cb(bmqio::ChannelFactoryEvent::e_CONNECT_FAILED, status, channel);
        return bmqio::ChannelFactoryEvent::e_CONNECT_FAILED;  // RETURN
    }

    return 0;
}

void InitialConnectionChannelFactory::onBrokerAuthenticationResponse(
    const bmqp_ctrlmsg::AuthenticationMessage& response,
    const ResultCallback&                      cb,
    const bsl::shared_ptr<bmqio::Channel>&     channel) const
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

    negotiate(channel, cb);
}

void InitialConnectionChannelFactory::onBrokerNegotiationResponse(
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
InitialConnectionChannelFactory::InitialConnectionChannelFactory(
    const Config&     config,
    bslma::Allocator* basicAllocator)
: d_config(config, basicAllocator)
, d_self(this)  // use default allocator
{
    // NOTHING
}

InitialConnectionChannelFactory::~InitialConnectionChannelFactory()
{
    // synchronize with any currently running callbacks and prevent future
    // callback invocations
    d_self.invalidate();
}

// MANIPULATORS
void InitialConnectionChannelFactory::listen(
    BSLS_ANNOTATION_UNUSED bmqio::Status* status,
    BSLS_ANNOTATION_UNUSED bslma::ManagedPtr<OpHandle>* handle,
    BSLS_ANNOTATION_UNUSED const bmqio::ListenOptions& options,
    BSLS_ANNOTATION_UNUSED const ResultCallback&       cb)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(false && "SDK should not be listening");
}

void InitialConnectionChannelFactory::connect(
    bmqio::Status*               status,
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
                &InitialConnectionChannelFactory::baseResultCallback,
                d_self.acquireWeak()),
            cb,
            bdlf::PlaceHolders::_1,    // event
            bdlf::PlaceHolders::_2,    // status
            bdlf::PlaceHolders::_3));  // channel
}

}  // close package namespace
}  // close enterprise namespace
