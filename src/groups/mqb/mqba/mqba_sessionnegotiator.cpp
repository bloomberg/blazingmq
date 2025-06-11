// Copyright 2014-2023 Bloomberg Finance L.P.
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

// mqba_sessionnegotiator.cpp                                         -*-C++-*-
#include <mqba_sessionnegotiator.h>

#include <mqbscm_version.h>
/// Implementation Notes
///====================
// The negotiation is always performed the following way: the task in listen
// mode waits for incoming connection; when one is established, it will
// schedule a read with a short timeout ('k_NEGOTIATION_READTIMEOUT' below).
// If a valid negotiation message is received, it will reply with its identity
// and create a session.  If either the read times out, the received
// negotiation message is invalid, or the received identity is marked as not
// allowed, the negotiation will fail and the connection will be closed.
//
/// Session creation logic
///----------------------
// Once received the remote peer identity, the following logic is used to
// determine the kind of Session to instantiate:
//: o If the session is an outgoing one, this is a broker connecting to another
//:   broker either as part of a Cluster Proxy or as a Cluster Member; in
//:   either case, we create a Dummy session that will be associated to the
//:   corresponding 'mqbnet::Cluster'.
//: o If the session is an incoming one, this can be one of three cases:
//:     - a regular client connecting to us,
//:     - a broker connecting to us while establishing a Cluster Proxy,
//:     - or a broker connecting to us while establishing a Cluster member
//:   If the remote peer didn't specify a 'clusterName', it is a regular
//:   client, so create a ClientSession; if a 'clusterName' was specified,
//:   but the 'nodeId' indicated the remote peer was not part of the
//:   cluster (i.e., 'nodeId' == -1), this means the connection is from a
//:   remote peer establishing a Cluster Proxy, in which case we also
//:   create a ClientSession; otherwise, this is a Cluster member incoming
//:   connection, so create a DummySession.

// MQB
#include <mqba_adminsession.h>
#include <mqba_clientsession.h>
#include <mqbblp_clustercatalog.h>
#include <mqbcfg_brokerconfig.h>
#include <mqbcfg_messages.h>
#include <mqbnet_cluster.h>
#include <mqbnet_dummysession.h>
#include <mqbnet_negotiationcontext.h>
#include <mqbnet_tcpsessionfactory.h>
#include <mqbu_sdkversionutil.h>

// BMQ
#include <bmqp_event.h>
#include <bmqp_protocol.h>
#include <bmqp_schemaeventbuilder.h>
#include <bmqscm_version.h>

#include <bmqio_channelutil.h>
#include <bmqio_tcpendpoint.h>
#include <bmqst_statcontext.h>
#include <bmqsys_time.h>
#include <bmqu_blob.h>
#include <bmqu_memoutstream.h>

// BDE
#include <ball_log.h>
#include <bdlf_bind.h>
#include <bdlf_placeholder.h>
#include <bdlma_localsequentialallocator.h>
#include <bdls_pathutil.h>
#include <bdls_processutil.h>
#include <bsl_functional.h>
#include <bsl_iostream.h>
#include <bsl_string.h>
#include <bslma_managedptr.h>
#include <bsls_assert.h>
#include <bsls_atomic.h>
#include <bsls_timeinterval.h>

// NTC
#include <ntsa_ipaddress.h>

namespace BloombergLP {
namespace mqba {

namespace {
BALL_LOG_SET_NAMESPACE_CATEGORY("MQBA.SESSIONNEGOTIATOR");

/// Load into the specified `identity` the identity of this broker.
/// The specified `shouldBroadcastToProxies` controls whether we advertise
/// that feature.
void loadBrokerIdentity(bmqp_ctrlmsg::ClientIdentity* identity,
                        bool                          shouldBroadcastToProxies,
                        bool shouldExtendMessageProperties)

{
    static bsls::AtomicInt s_sessionInstanceCount(0);

    bsl::string features;
    features.append(bmqp::EncodingFeature::k_FIELD_NAME)
        .append(":")
        .append(bmqp::EncodingFeature::k_ENCODING_BER)
        .append(",")
        .append(bmqp::EncodingFeature::k_ENCODING_JSON)
        .append(";")
        .append(bmqp::HighAvailabilityFeatures::k_FIELD_NAME)
        .append(":")
        .append(bmqp::HighAvailabilityFeatures::k_GRACEFUL_SHUTDOWN)
        .append(",")
        .append(bmqp::HighAvailabilityFeatures::k_GRACEFUL_SHUTDOWN_V2);

    if (shouldBroadcastToProxies) {
        features.append(",").append(
            bmqp::HighAvailabilityFeatures::k_BROADCAST_TO_PROXIES);
    }

    if (shouldExtendMessageProperties) {
        // Advertise support for new style message properties (v2 or "EX")
        features.append(";")
            .append(bmqp::MessagePropertiesFeatures::k_FIELD_NAME)
            .append(":")
            .append(bmqp::MessagePropertiesFeatures::k_MESSAGE_PROPERTIES_EX);

        const mqbcfg::AppConfig& theConfig = mqbcfg::BrokerConfig::get();

        if (theConfig.brokerVersion() == 999999 ||
            (theConfig.configureStream() &&
             theConfig.advertiseSubscriptions())) {
            features.append(";")
                .append(bmqp::SubscriptionsFeatures::k_FIELD_NAME)
                .append(":")
                .append(bmqp::SubscriptionsFeatures::k_CONFIGURE_STREAM);
        }
    }

    identity->protocolVersion() = bmqp::Protocol::k_VERSION;
    identity->sdkVersion()      = bmqscm::Version::versionAsInt();
    identity->clientType()      = bmqp_ctrlmsg::ClientType::E_TCPBROKER;
    identity->pid()             = bdls::ProcessUtil::getProcessId();
    identity->sessionId()       = ++s_sessionInstanceCount;
    identity->hostName()        = mqbcfg::BrokerConfig::get().hostName();
    identity->features()        = features;
    if (bdls::ProcessUtil::getProcessName(&identity->processName()) != 0) {
        identity->processName() = "*unknown*";
    }
    identity->sdkLanguage() = bmqp_ctrlmsg::ClientLanguage::E_CPP;
}

void loadBrokerIdentity(bmqp_ctrlmsg::ClientIdentity* identity,
                        bool                          shouldBroadcastToProxies,
                        const bslstl::StringRef&      name,
                        int                           nodeId)
{
    bool shouldExtendMessageProperties = false;

    // TODO: make this unconditional.  Currently, 'V2' is controlled by config
    // as a means to prevent SDK from generating 'V2'.
    // Regardless of SDK, brokers now decompress MPs and send ConfigureStream.

    const mqbcfg::AppConfig& theConfig = mqbcfg::BrokerConfig::get();
    if (theConfig.brokerVersion() == 999999) {
        // Always advertise v2 (EX) support in test build (developer workflow,
        // CI, Jenkins, etc).
        shouldExtendMessageProperties = true;
    }
    else if (theConfig.messagePropertiesV2().advertiseV2Support()) {
        // In non test build (i.e., dev and non-dev deployments, advertise v2
        // (EX) support only if configured like that.

        shouldExtendMessageProperties = true;
    }

    loadBrokerIdentity(identity,
                       shouldBroadcastToProxies,
                       shouldExtendMessageProperties);

    identity->clusterName()   = name;
    identity->clusterNodeId() = nodeId;
}

/// Load in the specified `out` the short description representing the
/// specified `identity` from the specified `peerChannel`.  The format is as
/// follow:
///    tskName:pid.sessionId[\@hostId]
/// Where:
///  - tskName   : the task name, without any optional leading path
///  - pid       : the pid of the task
///  - sessionId : the sessionId, omitted if 1
///  - hostId    : the identity of the host where the peer is running
///                (omitted if local), note that the format is either
///                `ip:port`, or `ip~resolvedHostname:port` depending on
///                whether the async DNS resolution already took place or
///                not.
void loadSessionDescription(bsl::string*                        out,
                            const bmqp_ctrlmsg::ClientIdentity& identity,
                            const bmqio::Channel&               peerChannel)
{
    bmqu::MemOutStream os;

    // Task Name
    bsl::string baseName;
    if (bdls::PathUtil::getBasename(&baseName, identity.processName()) != 0) {
        // Failed, use the full processName
        os << identity.processName();
    }
    else {
        os << baseName;
    }

    // PID
    os << ":" << identity.pid();

    // InstanceId
    if (identity.sessionId() != 1) {
        os << "." << identity.sessionId();
    }

    // Host
    int peerAddress;
    peerChannel.properties().load(
        &peerAddress,
        mqbnet::TCPSessionFactory::k_CHANNEL_PROPERTY_PEER_IP);

    ntsa::Ipv4Address ipv4Address(static_cast<bsl::uint32_t>(peerAddress));
    ntsa::IpAddress   ipAddress(ipv4Address);
    if (!bmqio::ChannelUtil::isLocalHost(ipAddress)) {
        os << "@" << identity.hostName();
    }

    out->assign(os.str().data(), os.str().length());
}
}  // close unnamed namespace

// -----------------------
// class SessionNegotiator
// -----------------------

int SessionNegotiator::createSessionOnMsgType(
    bsl::ostream&                     errorDescription,
    bsl::shared_ptr<mqbnet::Session>* session,
    bool*                             isContinueRead,
    const NegotiationContextSp&       context)
{
    BSLS_ASSERT(session);
    BSLS_ASSERT(isContinueRead);

    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS                        = 0,
        rc_NO_SESSION                     = -1,
        rc_NO_ADMIN_CALLBACK              = -2,
        rc_SEND_NEGOTIATION_MESSAGE_ERROR = -3,
        rc_INVALID_NEGOTIATION_TYPE       = -4
    };

    switch (context->d_negotiationMessage.selectionId()) {
    case bmqp_ctrlmsg::NegotiationMessage::SELECTION_INDEX_CLIENT_IDENTITY: {
        // This is the first message of the negotiation protocol; can either
        // represent:
        // - a client connecting to us
        // - a proxy connecting to us (implying we are a cluster member)
        // - a cluster peer connecting to us (implying we are a cluster member)
        // - an admin client connecting to us

        if (context->d_negotiationMessage.clientIdentity().clientType() ==
            bmqp_ctrlmsg::ClientType::E_TCPADMIN) {
            // Remote client is connecting to us as an admin.
            context->d_connectionType = mqbnet::ConnectionType::e_ADMIN;

            if (!d_adminCb) {
                errorDescription
                    << "Admin callback is not specified: admin session "
                    << "is not supported";
                return rc_NO_ADMIN_CALLBACK;  // RETURN
            }
        }
        else if (mqbnet::ClusterUtil::isClientOrProxy(
                     context->d_negotiationMessage)) {
            // - clusterName empty implies the remote peer is a regular client
            // - clusterName non empty but nodeId invalid means remote side is
            //   connecting to us as a proxy to 'clusterName' (with nodeId
            //   being invalid because remote peer is NOT part of the cluster,
            //   hence it's a proxy).
            context->d_connectionType = mqbnet::ConnectionType::e_CLIENT;
        }
        else {
            // Remote peer provided a valid clusterName and nodeId, those are
            // its identity inside the cluster.
            context->d_connectionType =
                mqbnet::ConnectionType::e_CLUSTER_MEMBER;
        }
        *session = onClientIdentityMessage(errorDescription, context);
    } break;
    case bmqp_ctrlmsg::NegotiationMessage::SELECTION_INDEX_BROKER_RESPONSE: {
        // This is the second part of the negotiation protocol.  If we haven't
        // yet made a 'connectionType' decision, this means we are a cluster
        // member.
        // TBD: should find a better way to detect that situation
        if (context->d_connectionType == mqbnet::ConnectionType::e_UNKNOWN) {
            errorDescription
                << "Received BrokerResponse message, but haven't sent a "
                   "ClientIdentity message yet";
            return rc_INVALID_NEGOTIATION_TYPE;  // RETURN
        }

        *session = onBrokerResponseMessage(errorDescription, context);
    } break;
    default: {
        errorDescription
            << "Invalid negotiation message received (unknown type): "
            << context->d_negotiationMessage;
        return rc_INVALID_NEGOTIATION_TYPE;  // RETURN
    }
    }

    if (!(*session)) {
        return rc_NO_SESSION;  // RETURN
    }

    return rc_SUCCESS;
}

bsl::shared_ptr<mqbnet::Session>
SessionNegotiator::onClientIdentityMessage(bsl::ostream& errorDescription,
                                           const NegotiationContextSp& context)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(context->d_negotiationMessage.isClientIdentityValue());
    BSLS_ASSERT_SAFE(context->d_initialConnectionContext_p->isIncoming());
    // We should be receiving a ClientIdentity message only if this is a
    // 'listen'-established connection.

    bmqp_ctrlmsg::ClientIdentity& clientIdentity =
        context->d_negotiationMessage.clientIdentity();

    BALL_LOG_DEBUG
        << "Received negotiation message from '"
        << context->d_initialConnectionContext_p->channel()->peerUri()
        << "': " << clientIdentity;

    bsl::shared_ptr<mqbnet::Session> session;

    // Inject the hostName in the negotiation message if not provided by the
    // connecting peer.
    switch (clientIdentity.clientType()) {
    case bmqp_ctrlmsg::ClientType::E_TCPCLIENT:
    case bmqp_ctrlmsg::ClientType::E_TCPBROKER:
    case bmqp_ctrlmsg::ClientType::E_TCPADMIN: {
        if (clientIdentity.hostName().empty()) {
            clientIdentity.hostName() =
                context->d_initialConnectionContext_p->channel()->peerUri();
        }
    } break;
    case bmqp_ctrlmsg::ClientType::E_UNKNOWN:
    default: {
        errorDescription << "Unknown ClientIdentity client type: "
                         << clientIdentity;
        return session;  // RETURN
    }
    }

    bmqp_ctrlmsg::NegotiationMessage negotiationResponse;
    bmqp_ctrlmsg::BrokerResponse&    response =
        negotiationResponse.makeBrokerResponse();

    const int clientVersion = clientIdentity.sdkVersion();
    const bmqp_ctrlmsg::ClientLanguage::Value& sdkLanguage =
        clientIdentity.sdkLanguage();
    const mqbcfg::AppConfig& appConfig = mqbcfg::BrokerConfig::get();

    if (checkIsUnsupportedSdkVersion(*context)) {
        response.result().category() =
            bmqp_ctrlmsg::StatusCategory::E_NOT_SUPPORTED;
        response.result().code() = -1;

        const int minVersion = mqbu::SDKVersionUtil::minSdkVersionSupported(
            sdkLanguage);

        bmqu::MemOutStream os;
        os << "Client is using an unsupported version of libbmq "
           << "(minimum supported version: " << minVersion
           << ", client version: " << clientVersion << ").";
        response.result().message() = os.str();
        response.isDeprecatedSdk()  = true;
    }
    else {
        response.result().category() = bmqp_ctrlmsg::StatusCategory::E_SUCCESS;
        response.result().code()     = 0;
        response.isDeprecatedSdk()   = checkIsDeprecatedSdkVersion(*context);
    }
    response.protocolVersion() = bmqp::Protocol::k_VERSION;
    response.brokerVersion()   = appConfig.brokerVersion();

    const bsl::string& clusterName = clientIdentity.clusterName();

    if (!clusterName.empty()) {
        // If a cluster name was specified in the requester's identity, include
        // the cluster name and the node Id in the response if we are part of
        // this cluster.
        int nodeId = mqbnet::Cluster::k_INVALID_NODE_ID;

        nodeId = d_clusterCatalog_p->selfNodeIdInCluster(clusterName);

        if (nodeId == mqbnet::Cluster::k_INVALID_NODE_ID) {
            // The client connected to us as part of a cluster connection,
            // but we are not member of that cluster; emit an error (but
            // still accept the connection).
            BALL_LOG_ERROR << "#CONNECTION_UNEXPECTED Client '"
                           << clientIdentity
                           << "' connected to me as part of cluster '"
                           << clusterName << "' to which I do not belong!";
        }
        // Virtual clusters do not advertise node status.  Therefore, the
        // identity should not advertise k_BROADCAST_TO_PROXIES feature.
        bool isVirtual = d_clusterCatalog_p->isClusterVirtual(clusterName);

        loadBrokerIdentity(&response.brokerIdentity(),
                           !isVirtual,
                           clusterName,
                           nodeId);
    }
    else {
        bool shouldExtendMessageProperties = false;

        // TODO: make this unconditional.  Currently, 'V2' is controlled by
        // config as a means to prevent SDK from generating 'V2'.
        // Regardless of SDK, brokers now decompress MPs and send
        // ConfigureStream.

        if (appConfig.brokerVersion() == 999999) {
            // Always advertise v2 (EX) support in test build (developer
            // workflow, CI, Jenkins, etc).
            shouldExtendMessageProperties = true;
        }
        else if (appConfig.messagePropertiesV2().advertiseV2Support()) {
            // In non test build (i.e., dev and non-dev deployments, advertise
            // v2 (EX) support only if configured like that *and* if the SDK
            // version is the configured one.

            shouldExtendMessageProperties =
                mqbu::SDKVersionUtil::isMinExtendedMessagePropertiesVersion(
                    sdkLanguage,
                    clientVersion);
        }

        loadBrokerIdentity(&response.brokerIdentity(),
                           true,
                           shouldExtendMessageProperties);
    }

    // Create the session.  That also calculates 'maxMissedHeartbeats'
    bsl::string description;
    loadSessionDescription(
        &description,
        clientIdentity,
        *(context->d_initialConnectionContext_p->channel().get()));

    createSession(errorDescription, &session, context, description);

    // Communicate heartbeat settings.  Currently, only for SDK use
    const mqbcfg::NetworkInterfaces& niConfig = appConfig.networkInterfaces();

    response.maxMissedHeartbeats() = context->d_maxMissedHeartbeat;

    if (niConfig.tcpInterface().has_value()) {
        response.heartbeatIntervalMs() =
            niConfig.tcpInterface().value().heartbeatIntervalMs();
    }

    int rc = sendNegotiationMessage(errorDescription,
                                    negotiationResponse,
                                    context);
    if (rc != 0) {
        session.reset();
    }

    return session;
}

bsl::shared_ptr<mqbnet::Session>
SessionNegotiator::onBrokerResponseMessage(bsl::ostream& errorDescription,
                                           const NegotiationContextSp& context)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(context->d_negotiationMessage.isBrokerResponseValue());

    bmqp_ctrlmsg::BrokerResponse& brokerResponse =
        context->d_negotiationMessage.brokerResponse();

    BALL_LOG_DEBUG
        << "Received negotiation message from '"
        << context->d_initialConnectionContext_p->channel()->peerUri()
        << "': " << brokerResponse;

    bsl::shared_ptr<mqbnet::Session> session;

    if (brokerResponse.result().category() !=
        bmqp_ctrlmsg::StatusCategory::E_SUCCESS) {
        errorDescription << "Failure broker's response [" << brokerResponse
                         << "]";
        return session;  // RETURN
    }

    // Resolve 'hostName' of the brokerIdentity
    bmqio::TCPEndpoint endpoint(
        context->d_initialConnectionContext_p->channel()->peerUri());
    brokerResponse.brokerIdentity().hostName() = endpoint.host();

    bsl::string description;
    loadSessionDescription(
        &description,
        brokerResponse.brokerIdentity(),
        *(context->d_initialConnectionContext_p->channel().get()));

    createSession(errorDescription, &session, context, description);

    return session;
}

int SessionNegotiator::sendNegotiationMessage(
    bsl::ostream&                           errorDescription,
    const bmqp_ctrlmsg::NegotiationMessage& message,
    const NegotiationContextSp&             context)
{
    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS       = 0,
        rc_BUILD_FAILURE = -1,
        rc_WRITE_FAILURE = -2
    };

    // This method is used to send two types of negotiation message:
    //   - the initial negotiation request,
    //   - its response,
    // In the second case, where broker is responding to an incoming
    // negotiation from a client (whether an application client, or a
    // downstream broker), we want to encode using the requesters preferred
    // encoding; default to BER in all other cases.
    bmqp::EncodingType::Enum encodingType = bmqp::EncodingType::e_BER;

    if (message.isBrokerResponseValue() &&
        context->d_negotiationMessage.isClientIdentityValue()) {
        encodingType = bmqp::SchemaEventBuilderUtil::bestEncodingSupported(
            context->d_negotiationMessage.clientIdentity().features());

        if (encodingType == bmqp::EncodingType::e_UNKNOWN) {
            errorDescription
                << "Failed building NegotiationMessage: client "
                << "did not advertise a supported encoding type. "
                << "Client features: '"
                << context->d_negotiationMessage.clientIdentity().features()
                << "'";
            return rc_BUILD_FAILURE;  // RETURN
        }
    }

    // Build connection response event
    bdlma::LocalSequentialAllocator<2048> localAllocator(d_allocator_p);

    bmqp::SchemaEventBuilder builder(d_blobSpPool_p,
                                     encodingType,
                                     &localAllocator);

    int rc = builder.setMessage(message, bmqp::EventType::e_CONTROL);
    if (rc != 0) {
        errorDescription << "Failed building NegotiationMessage "
                         << "[rc: " << rc << ", message: " << message << "]";
        return rc_BUILD_FAILURE;  // RETURN
    }

    // Send response event
    bmqio::Status status;
    context->d_initialConnectionContext_p->channel()->write(&status,
                                                            *builder.blob());
    if (!status) {
        errorDescription << "Failed sending NegotiationMessage "
                         << "[status: " << status << ", message: " << message
                         << "]";
        return rc_WRITE_FAILURE;  // RETURN
    }

    return rc_SUCCESS;
}

void SessionNegotiator::createSession(bsl::ostream& errorDescription,
                                      bsl::shared_ptr<mqbnet::Session>* out,
                                      const NegotiationContextSp& context,
                                      const bsl::string&          description)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(context->d_connectionType !=
                     mqbnet::ConnectionType::e_UNKNOWN);
    BSLS_ASSERT_SAFE(!context->d_negotiationMessage.isUndefinedValue());

    const bmqp_ctrlmsg::NegotiationMessage& negoMsg =
        context->d_negotiationMessage;
    const bmqp_ctrlmsg::ClientIdentity& peerIdentity =
        negoMsg.isClientIdentityValue()
            ? negoMsg.clientIdentity()
            : negoMsg.brokerResponse().brokerIdentity();
    const mqbcfg::AppConfig&         brkrCfg  = mqbcfg::BrokerConfig::get();
    const mqbcfg::NetworkInterfaces& niConfig = brkrCfg.networkInterfaces();
    int                              maxMissedHeartbeats = 0;

    if (context->d_connectionType == mqbnet::ConnectionType::e_ADMIN) {
        mqba::AdminSession* session = new (*d_allocator_p)
            AdminSession(context->d_initialConnectionContext_p->channel(),
                         negoMsg,
                         description,
                         d_dispatcher_p,
                         d_blobSpPool_p,
                         d_scheduler_p,
                         d_adminCb,
                         d_allocator_p);

        out->reset(session, d_allocator_p);
    }
    else if (context->d_connectionType == mqbnet::ConnectionType::e_CLIENT) {
        // Create a dedicated stats subcontext for this client
        bmqst::StatContextConfiguration statContextCfg(description);
        statContextCfg.storeExpiredSubcontextValues(true);
        bslma::ManagedPtr<bmqst::StatContext> statContext =
            d_statContext_p->addSubcontext(statContextCfg);

        mqba::ClientSession* session = new (*d_allocator_p)
            ClientSession(context->d_initialConnectionContext_p->channel(),
                          negoMsg,
                          description,
                          d_dispatcher_p,
                          d_clusterCatalog_p,
                          d_domainFactory_p,
                          statContext,
                          d_blobSpPool_p,
                          d_bufferFactory_p,
                          d_scheduler_p,
                          d_allocator_p);

        out->reset(session, d_allocator_p);

        // Configure heartbeat
        if (negoMsg.clientIdentity().clientType() ==
            bmqp_ctrlmsg::ClientType::E_TCPCLIENT) {
            maxMissedHeartbeats = niConfig.heartbeats().client();
        }
        else if (negoMsg.clientIdentity().clientType() ==
                 bmqp_ctrlmsg::ClientType::E_TCPBROKER) {
            maxMissedHeartbeats = niConfig.heartbeats().downstreamBroker();
        }

        const bsl::string& clusterName = peerIdentity.clusterName();

        if (!clusterName.empty()) {
            // This is Proxy connection.  Need to inform mqbnet::Cluster
            bsl::shared_ptr<mqbi::Cluster> cluster;

            if (d_clusterCatalog_p->findCluster(&cluster, clusterName)) {
                context->d_cluster_p = &cluster->netCluster();
            }
        }
    }
    else {
        // This session should be mapped to the corresponding mqbnet::Cluster,
        // so query ClusterCatalog to look it up: if this is an incoming
        // connection, use the remote peer advertised identity; if this is an
        // outgoing connection, use our own identity (that we embedded in the
        // 'brokerResponse').

        mqbnet::ClusterNode* clusterNode = 0;
        clusterNode = d_clusterCatalog_p->onNegotiationForClusterSession(
            errorDescription,
            context->d_initialConnectionContext_p,
            peerIdentity.clusterName(),
            peerIdentity.clusterNodeId());

        if (!clusterNode) {
            return;  // RETURN
        }

        out->reset(new (*d_allocator_p) mqbnet::DummySession(
                       context->d_initialConnectionContext_p->channel(),
                       negoMsg,
                       clusterNode,
                       description,
                       d_allocator_p),
                   d_allocator_p);

        // Configure heartbeat
        if (clusterNode->cluster()->selfNodeId() ==
            mqbnet::Cluster::k_INVALID_NODE_ID) {
            maxMissedHeartbeats = niConfig.heartbeats().upstreamBroker();
        }
        else {
            maxMissedHeartbeats = niConfig.heartbeats().clusterPeer();
        }
    }

    context->d_maxMissedHeartbeat = maxMissedHeartbeats;
}

bool SessionNegotiator::checkIsDeprecatedSdkVersion(
    const mqbnet::NegotiationContext& context)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(context.d_negotiationMessage.isClientIdentityValue());

    const bmqp_ctrlmsg::ClientIdentity& clientIdentity =
        context.d_negotiationMessage.clientIdentity();
    if (!mqbu::SDKVersionUtil::isDeprecatedSdkVersion(
            clientIdentity.sdkLanguage(),
            clientIdentity.sdkVersion())) {
        return false;  // RETURN
    }

    // Not an alarm, we let the client print an ALMN catchable trace in
    // response to the negotiation message; we just warn in the broker to keep
    // a central location of all deprecated clients.
    BALL_LOG_WARN << "#CLIENT_SDKVERSION_DEPRECATED "
                  << "Client is using a deprecated SDK: "
                  << "[client: " << clientIdentity
                  << ", minimumSDKVersionRecommended: "
                  << mqbu::SDKVersionUtil::minSdkVersionRecommended(
                         clientIdentity.sdkLanguage())
                  << "]";

    return true;
}

bool SessionNegotiator::checkIsUnsupportedSdkVersion(
    const mqbnet::NegotiationContext& context)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(context.d_negotiationMessage.isClientIdentityValue());

    const bmqp_ctrlmsg::ClientIdentity& clientIdentity =
        context.d_negotiationMessage.clientIdentity();
    if (mqbu::SDKVersionUtil::isSupportedSdkVersion(
            clientIdentity.sdkLanguage(),
            clientIdentity.sdkVersion())) {
        return false;  // RETURN
    }

    // Not an alarm, we let the client print an ALMN catchable trace in
    // response to the negotiation message; we just warn in the broker to keep
    // a central location of all rejected clients.
    BALL_LOG_WARN << "#CLIENT_SDKVERSION_UNSUPPORTED "
                  << "Client is using an unsupported SDK: "
                  << "[client: " << clientIdentity
                  << ", minimumSDKVersionSupported: "
                  << mqbu::SDKVersionUtil::minSdkVersionSupported(
                         clientIdentity.sdkLanguage())
                  << "]";

    return true;
}

// CREATORS
SessionNegotiator::SessionNegotiator(bdlbb::BlobBufferFactory* bufferFactory,
                                     mqbi::Dispatcher*         dispatcher,
                                     bmqst::StatContext*       statContext,
                                     BlobSpPool*               blobSpPool,
                                     bdlmt::EventScheduler*    scheduler,
                                     bslma::Allocator*         allocator)
: d_allocator_p(allocator)
, d_bufferFactory_p(bufferFactory)
, d_dispatcher_p(dispatcher)
, d_domainFactory_p(0)
, d_statContext_p(statContext)
, d_blobSpPool_p(blobSpPool)
, d_clusterCatalog_p(0)
, d_scheduler_p(scheduler)
{
    // NOTHING
}

SessionNegotiator::~SessionNegotiator()
{
    // NOTHING: (required because of inheritance)
}

int SessionNegotiator::initiateOutboundNegotiation(
    bsl::ostream&               errorDescription,
    const NegotiationContextSp& context)
{
    bmqp_ctrlmsg::NegotiationMessage negotiationMessage;

    int nodeId = d_clusterCatalog_p->selfNodeIdInCluster(
        context->d_clusterName);
    bool isVirtual = d_clusterCatalog_p->isClusterVirtual(
        context->d_clusterName);

    // Virtual clusters do not advertise node status.  Therefore, the identity
    // should not advertise k_BROADCAST_TO_PROXIES feature.
    loadBrokerIdentity(&negotiationMessage.makeClientIdentity(),
                       !isVirtual,
                       context->d_clusterName,
                       nodeId);

    int rc = sendNegotiationMessage(errorDescription,
                                    negotiationMessage,
                                    context);

    return rc;
}

int SessionNegotiator::negotiateOutbound(bsl::ostream& errorDescription,
                                         const NegotiationContextSp& context)
{
    BSLS_ASSERT_SAFE(context);
    BSLS_ASSERT_SAFE(context->d_initialConnectionContext_p);
    BSLS_ASSERT_SAFE(!context->d_initialConnectionContext_p->isIncoming());

    const mqbblp::ClusterCatalog::NegotiationUserData* userData =
        reinterpret_cast<mqbblp::ClusterCatalog::NegotiationUserData*>(
            context->d_initialConnectionContext_p->userData());
    BSLS_ASSERT_SAFE(userData);

    int rc = 0;

    context->d_clusterName = userData->d_clusterName;
    if (d_clusterCatalog_p->isMemberOf(context->d_clusterName)) {
        context->d_connectionType = mqbnet::ConnectionType::e_CLUSTER_MEMBER;
    }
    else {
        context->d_connectionType = mqbnet::ConnectionType::e_CLUSTER_PROXY;
    }

    rc = initiateOutboundNegotiation(errorDescription, context);

    return rc;
}

}  // close package namespace
}  // close enterprise namespace
