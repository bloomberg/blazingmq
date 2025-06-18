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

// mqbnet_transportmanager.cpp                                        -*-C++-*-
#include <mqbnet_transportmanager.h>

#include <mqbscm_version.h>
// BMQ
#include <bmqp_ctrlmsg_messages.h>

// MQB
#include <mqbcfg_brokerconfig.h>
#include <mqbcfg_messages.h>
#include <mqbnet_cluster.h>
#include <mqbnet_clusterimp.h>
#include <mqbnet_session.h>
#include <mqbnet_tcpsessionfactory.h>

#include <bmqio_status.h>
#include <bmqsys_time.h>
#include <bmqu_printutil.h>
#include <bmqu_stringutil.h>

// BDE
#include <bdlf_bind.h>
#include <bdlf_placeholder.h>
#include <bsl_functional.h>
#include <bsl_iostream.h>
#include <bsl_list.h>
#include <bsl_utility.h>
#include <bslalg_swaputil.h>
#include <bslma_allocator.h>
#include <bslmt_lockguard.h>
#include <bslmt_mutexassert.h>
#include <bsls_assert.h>
#include <bsls_systemclocktype.h>
#include <bsls_timeinterval.h>

namespace BloombergLP {
namespace mqbnet {

namespace {

bsl::ostream& operator<<(bsl::ostream& os, const bmqio::Channel* channel)
{
    // 'pretty-print' the specified 'channel' to the specified 'os'.  The
    // printed channel from that function includes the address of the channel
    // for easy tracking and matching of logs.

    if (channel) {
        os << channel->peerUri() << "#" << static_cast<const void*>(channel);
    }
    else {
        os << "*null*";
    }

    return os;
}

}  // close unnamed namespace

// ----------------------
// class TransportManager
// ----------------------

void TransportManager::onClusterReleased(void* object, void* transportManager)
{
    mqbnet::Cluster*  cluster = reinterpret_cast<Cluster*>(object);
    TransportManager* self    = reinterpret_cast<TransportManager*>(
        transportManager);

    {
        bslmt::LockGuard<bslmt::Mutex> guard(&self->d_mutex);  // d_mutex LOCK

        BALL_LOG_INFO << "Deleting Cluster '" << cluster->name() << "'";

        // Invalidate all nodes belonging to that cluster in their associated
        // ConnectionState and disable auto-reconnect.
        ConnectionsStateMap::const_iterator it;
        for (it = self->d_connectionsState.begin();
             it != self->d_connectionsState.end();
             ++it) {
            ConnectionState& connectionState = *(it->first);
            if (connectionState.d_node_p &&
                connectionState.d_node_p->cluster() == cluster) {
                connectionState.d_node_p = 0;
            }
        }
    }  // mutex guard scope

    // Close all cluster channels
    cluster->closeChannels();

    // And delete the cluster
    self->d_allocators.get(cluster->name())->deleteObject(cluster);
}

int TransportManager::createAndStartTcpInterface(
    bsl::ostream&                     errorDescription,
    const mqbcfg::TcpInterfaceConfig& config)
{
    // executed by the *MAIN* thread

    bslma::Allocator* alloc = d_allocators.get("Interface" +
                                               bsl::to_string(config.port()));
    d_tcpSessionFactory_mp.load(
        new (*alloc) TCPSessionFactory(config,
                                       d_scheduler_p,
                                       d_blobBufferFactory_p,
                                       d_initialConnectionHandler_mp.get(),
                                       d_statController_p,
                                       alloc),
        alloc);

    return d_tcpSessionFactory_mp->start(errorDescription);
}

bool TransportManager::processSession(
    Cluster*                            cluster,
    ConnectionState*                    state,
    const bsl::shared_ptr<Session>&     session,
    const bmqio::Channel::ReadCallback& readCb)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(session);

    // If this is not part of a cluster, enable read.  Otherwise, call
    // 'mqbnet::ClusterNode::setChannel' and let 'mqbnet::Cluster' enable read
    // when ready.

    const bmqp_ctrlmsg::NegotiationMessage& negoMsg =
        session->negotiationMessage();
    const bmqp_ctrlmsg::ClientIdentity& peerIdentity =
        negoMsg.isClientIdentityValue()
            ? negoMsg.clientIdentity()
            : negoMsg.brokerResponse().brokerIdentity();

    // This is an outgoing connection to a cluster node
    if (state) {
        bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // d_mutex LOCK

        if (state->d_node_p == 0) {
            BALL_LOG_INFO << "Session is up, but cluster is no longer valid ["
                          << "channel: '" << session->channel().get() << "']";
        }
        else {
            BALL_LOG_INFO << "Cluster session is up [cluster: '"
                          << state->d_node_p->cluster()->name()
                          << "', channel: '" << session->channel().get()
                          << "']";

            // Set the (reduced) watermarks since the node will use
            // mqbnet::Channel.
            d_tcpSessionFactory_mp->setNodeWriteQueueWatermarks(*session);

            // Notify the node it now has a channel
            bsl::weak_ptr<bmqio::Channel> channel(session->channel());
            state->d_node_p->setChannel(channel, peerIdentity, readCb);
        }

        // Add self as observer of the channel (so that we can monitor when it
        // goes down, and eventually initiate a reconnection).
        bmqu::MemOutStream channelDescription;
        channelDescription << session->channel().get();

        session->channel()->onClose(
            bdlf::BindUtil::bind(&TransportManager::onClose,
                                 this,
                                 bsl::string(channelDescription.str()),
                                 state));

        return true;  // RETURN
    }

    // This is either proxy or client incoming connection.
    if (cluster) {
        BALL_LOG_INFO << "Proxy session is up [channel: '"
                      << session->channel().get() << "']";
        // Handle incoming proxy connection
        cluster->onProxyConnectionUp(session->channel(),
                                     peerIdentity,
                                     session->description());
    }
    else {
        BALL_LOG_INFO << "Client session is up [channel: '"
                      << session->channel().get() << "']";
    }

    bmqio::Status readStatus;
    session->channel()->read(&readStatus,
                             bmqp::Protocol::k_PACKET_MIN_SIZE,
                             readCb);

    if (!readStatus) {
        BALL_LOG_ERROR << "#TCP_READ_ERROR " << session->description()
                       << ": Failed reading from the channel "
                       << "[status: " << readStatus << ", "
                       << "channel: '" << session->channel().get() << "']";

        return false;  // RETURN
    }
    return true;
}

bool TransportManager::sessionResult(
    bmqio::ChannelFactoryEvent::Enum    event,
    const bmqio::Status&                status,
    const bsl::shared_ptr<Session>&     session,
    Cluster*                            cluster,
    void*                               resultState,
    const bmqio::Channel::ReadCallback& readCb,
    bool                                isListen)
{
    // executed by one of the *IO* threads

    ConnectionState* state = reinterpret_cast<ConnectionState*>(resultState);

    BALL_LOG_DEBUG << "SessionResult: [event: " << event
                   << ", status: " << status << "]";

    bool result = false;

    switch (event) {
    case bmqio::ChannelFactoryEvent::e_CHANNEL_UP: {
        result = processSession(cluster, state, session, readCb);
    } break;
    case bmqio::ChannelFactoryEvent::e_CONNECT_ATTEMPT_FAILED: {
        // Nothing to do, it will keep retrying automatically
    } break;
    case bmqio::ChannelFactoryEvent::e_CONNECT_FAILED: {
        if (isListen) {
            BALL_LOG_INFO << "Accept failed: " << status;
            return result;  // RETURN
        }

        BSLS_ASSERT_SAFE(state &&
                         "Connect should always have an associated state");
    } break;
    default: {
        BALL_LOG_ERROR << "#NETWORK_UNEXPECTED_EVENT "
                       << "Unexpected connectResult event: " << event;
    }
    }

    return result;
}

int TransportManager::connect(ConnectionState* state)
{
    // executed by the *SCHEDULER* thread
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // d_mutex LOCK
    return connectLocked(state);
}

int TransportManager::connectLocked(ConnectionState* state)
{
    // executed by *ANY* thread

    // PRECONDITIONS
    BSLMT_MUTEXASSERT_IS_LOCKED_SAFE(
        &d_mutex);  // mutex was LOCKED
                    // The mutex lock is necessary to ensure the 'state'
                    // doesn't get invalidated while processing this method.

    bslma::ManagedPtr<void> negotiationUserData =
        state->d_userData_sp.managedPtr();
    return d_tcpSessionFactory_mp->connect(
        state->d_endpoint,
        bdlf::BindUtil::bind(&TransportManager::sessionResult,
                             this,
                             bdlf::PlaceHolders::_1,  // event
                             bdlf::PlaceHolders::_2,  // status
                             bdlf::PlaceHolders::_3,  // session
                             bdlf::PlaceHolders::_4,  // cluster
                             bdlf::PlaceHolders::_5,  // resultState
                             bdlf::PlaceHolders::_6,  // readCb
                             false),                  // isListen
        &negotiationUserData,
        state,
        true);  // shouldAutoReconnect
}

void TransportManager::onClose(const bsl::string& channelDescription,
                               ConnectionState*   state)
{
    // executed by one of the *IO* threads
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(state);

    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // d_mutex LOCK

    if (!state->d_node_p) {
        // The cluster was destroyed, we can just remove that ConnectionState
        // entry from the map
        BALL_LOG_INFO << "Channel onClose [channel: '" << channelDescription
                      << "']";
        d_connectionsState.erase(state);
        return;  // RETURN
    }

    BALL_LOG_INFO << "Channel onClose [channel: '" << channelDescription
                  << "', cluster: '" << state->d_node_p->cluster()->name()
                  << "']";

    // Notify node of lost of connectivity
    state->d_node_p->resetChannel();
}

int TransportManager::selfNodeIdLocked(
    const bsl::vector<mqbcfg::ClusterNode>& nodes) const
{
    // PRECONDITIONS
    BSLMT_MUTEXASSERT_IS_LOCKED_SAFE(&d_mutex);  // mutex was LOCKED

    bsl::vector<mqbcfg::ClusterNode>::const_iterator it;
    for (it = nodes.begin(); it != nodes.end(); ++it) {
        if (it->transport().isTcpValue() &&
            isEndpointLoopback(it->transport().tcp().endpoint())) {
            return it->id();  // RETURN
        }
    }

    return Cluster::k_INVALID_NODE_ID;
}

TransportManager::TransportManager(
    bdlmt::EventScheduler*                       scheduler,
    bdlbb::BlobBufferFactory*                    blobBufferFactory,
    bslma::ManagedPtr<InitialConnectionHandler>& initialConnectionHandler,
    mqbstat::StatController*                     statController,
    bslma::Allocator*                            allocator)
: d_allocators(allocator)
, d_state(e_STOPPED)
, d_scheduler_p(scheduler)
, d_blobBufferFactory_p(blobBufferFactory)
, d_initialConnectionHandler_mp(initialConnectionHandler)
, d_statController_p(statController)
, d_tcpSessionFactory_mp(0)
, d_connectionsState(allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(scheduler->clockType() ==
                     bsls::SystemClockType::e_MONOTONIC);
}

TransportManager::~TransportManager()
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(e_STOPPED == d_state &&
                    "stop() must be called before destroying this object");
}

int TransportManager::start(bsl::ostream& errorDescription)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(e_STOPPED == d_state &&
                    "start() can only be called once on this object");

    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS       = 0,
        rc_TCP_INTERFACE = -1
    };

    BALL_LOG_INFO << "Starting TransportManager";

    int rc = rc_SUCCESS;

    // Create and start the TCPInterface, if any
    const mqbcfg::AppConfig& brkrCfg = mqbcfg::BrokerConfig::get();

    // If the new network interfaces exist, use them. Otherwise, fall back to
    // the old network interface config.
    if (!brkrCfg.networkInterfaces().tcpInterface().isNull()) {
        rc = createAndStartTcpInterface(
            errorDescription,
            brkrCfg.networkInterfaces().tcpInterface().value());
        if (rc != 0) {
            return (rc * 10) + rc_TCP_INTERFACE;  // RETURN
        }
    }

    d_state = e_STARTING;

    return rc_SUCCESS;
}

int TransportManager::startListening(bsl::ostream& errorDescription)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(e_STARTING == d_state &&
                     "TransportManager must be started first");

    int rc = 0;

    const mqbcfg::AppConfig& brkrCfg = mqbcfg::BrokerConfig::get();
    if (!brkrCfg.networkInterfaces().tcpInterface().isNull()) {
        BSLS_ASSERT_SAFE(d_tcpSessionFactory_mp);

        rc = d_tcpSessionFactory_mp->startListening(
            errorDescription,
            bdlf::BindUtil::bind(&TransportManager::sessionResult,
                                 this,
                                 bdlf::PlaceHolders::_1,  // event
                                 bdlf::PlaceHolders::_2,  // status
                                 bdlf::PlaceHolders::_3,  // session
                                 bdlf::PlaceHolders::_4,  // cluster
                                 bdlf::PlaceHolders::_5,  // resultState
                                 bdlf::PlaceHolders::_6,  // readCb
                                 true));                  // isListen
        if (rc != 0) {
            return rc;  // RETURN
        }
    }

    d_state = e_STARTED;

    return rc;
}

void TransportManager::initiateShutdown()
{
    if (d_state != e_STARTED && d_state != e_STARTING) {
        return;  // RETURN
    }

    d_state = e_STOPPING;

    const mqbcfg::AppConfig& brkrCfg = mqbcfg::BrokerConfig::get();
    if (!brkrCfg.networkInterfaces().tcpInterface().isNull()) {
        BSLS_ASSERT_SAFE(d_tcpSessionFactory_mp);
        d_tcpSessionFactory_mp->stopListening();
    }
}

void TransportManager::closeClients()
{
    d_tcpSessionFactory_mp->closeClients();
}

void TransportManager::stop()
{
    if (d_state == e_STOPPED) {
        return;  // RETURN
    }

    BSLS_ASSERT_SAFE(e_STOPPING == d_state || e_STARTING == d_state);

    d_state = e_STOPPED;

    // Stop interfaces
    if (d_tcpSessionFactory_mp) {
        d_tcpSessionFactory_mp->stop();
    }

    // Destroy interfaces
    if (d_tcpSessionFactory_mp) {
        d_tcpSessionFactory_mp.clear();
    }

    // Clear map (so that a 'start' called after 'stop' will start 'fresh')
    d_connectionsState.clear();
}

int TransportManager::createCluster(
    bsl::ostream&                           errorDescription,
    bslma::ManagedPtr<mqbnet::Cluster>*     out,
    const bsl::string&                      name,
    const bsl::vector<mqbcfg::ClusterNode>& nodes,
    ConnectionMode                          connectionMode,
    bslma::ManagedPtr<void>*                userData)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(out);

    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS           = 0,
        rc_INVALID_NODE_TYPE = -1,
        rc_FAILED_CONNECT    = -2
    };

    // The mutex is protecting the d_connectionsState map, but locking it
    // outside the loop to lock only once
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // d_mutex LOCK

    // Extract selfNodeId
    const int myNodeId = selfNodeIdLocked(nodes);

    BALL_LOG_INFO_BLOCK
    {
        BALL_LOG_OUTPUT_STREAM << "Creating new cluster '" << name
                               << "' (selfNodeId: " << myNodeId
                               << "), using mode " << connectionMode
                               << " and config: ";
        bmqu::Printer<bsl::vector<mqbcfg::ClusterNode> > printer(&nodes);
        BALL_LOG_OUTPUT_STREAM << printer;
    }

    // Create a shared pointer to hold the user data, if any
    bsl::shared_ptr<void> userDataSp;
    if (userData) {
        userDataSp = *userData;
    }

    bslma::Allocator*          alloc = d_allocators.get(name);
    bslma::ManagedPtr<Cluster> cluster(
        new (*alloc)
            ClusterImp(name, nodes, myNodeId, d_blobBufferFactory_p, alloc),
        alloc);

    // At the moment, only TCP is supported, validate that
    bsl::vector<mqbcfg::ClusterNode>::const_iterator nodeIt;
    for (nodeIt = nodes.begin(); nodeIt != nodes.end(); ++nodeIt) {
        if (!nodeIt->transport().isTcpValue()) {
            errorDescription << "Unsupported Cluster Node transport: "
                             << *nodeIt;
            return rc_INVALID_NODE_TYPE;  // RETURN
        }
    }

    // Now create the connections
    for (nodeIt = nodes.begin(); nodeIt != nodes.end(); ++nodeIt) {
        const mqbcfg::TcpClusterNodeConnection& tcpConfig =
            nodeIt->transport().tcp();

        // Skip node if it's referring to localhost
        if (nodeIt->id() == myNodeId) {
            BALL_LOG_DEBUG << "Skipping connection to local host '"
                           << tcpConfig.endpoint() << "' "
                           << "[cluster: '" << name << "']";
            continue;  // CONTINUE
        }

        // Create the ClusterNode
        ClusterNode* node = cluster->lookupNode(nodeIt->id());
        BSLS_ASSERT_OPT(node);  // We just created the cluster with the same
                                // config, the node must exist

        bsl::shared_ptr<ConnectionState> connectionState;
        connectionState.createInplace(d_allocators.get("ConnectionStates"));

        connectionState->d_endpoint    = tcpConfig.endpoint();
        connectionState->d_node_p      = node;
        connectionState->d_userData_sp = userDataSp;

        d_connectionsState[connectionState.get()] = connectionState;

        // Skip connection to node depending on the mode
        if (connectionMode == e_MIXED && myNodeId > nodeIt->id()) {
            BALL_LOG_INFO_BLOCK
            {
                BALL_LOG_OUTPUT_STREAM
                    << "Skipping connection to '" << nodeIt->name()
                    << "' (reason: its node id " << nodeIt->id()
                    << " is lesser than current machine node id " << myNodeId
                    << ")";
            }
            continue;  // CONTINUE
        }

        int rc = connectLocked(connectionState.get());
        if (rc != 0) {
            errorDescription << "Error establishing connection with '"
                             << tcpConfig.endpoint() << "': [rc: " << rc
                             << ", cluster: '" << name << "']";
            // Remove any connections about that cluster
            ConnectionsStateMap::iterator stateIt = d_connectionsState.begin();
            while (stateIt != d_connectionsState.end()) {
                if (stateIt->first->d_node_p &&
                    stateIt->first->d_node_p->cluster() == cluster.get()) {
                    stateIt = d_connectionsState.erase(stateIt);
                }
                else {
                    ++stateIt;
                }
            }

            return (rc * 10) + rc_FAILED_CONNECT;  // RETURN
        }
    }

    // Return a managedPtr with a custom deleter
    out->load(cluster.get(), this, &TransportManager::onClusterReleased);
    cluster.release();

    return rc_SUCCESS;
}

void* TransportManager::getClusterNodeAndState(
    bsl::ostream&            errorDescription,
    ClusterNode**            clusterNode,
    const bslstl::StringRef& clusterName,
    int                      nodeId)
{
    // executed by one of the *IO* threads

    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // d_mutex LOCK

    ConnectionState* state = 0;
    for (ConnectionsStateMap::iterator it = d_connectionsState.begin();
         it != d_connectionsState.end();
         ++it) {
        if (it->first->d_node_p && it->first->d_node_p->nodeId() == nodeId &&
            it->first->d_node_p->cluster()->name() == clusterName) {
            state = it->first;
            break;  // BREAK
        }
    }

    if (!state) {
        errorDescription << "No connection state found for "
                         << "[cluster: '" << clusterName
                         << "', nodeId: " << nodeId << "]";
        return 0;  // RETURN
    }

    if (state->d_node_p->isAvailable()) {
        errorDescription << "The node already has a channel associated "
                         << "[cluster: '" << clusterName
                         << "', nodeId: " << nodeId << "]";
        return 0;  // RETURN
    }

    *clusterNode = state->d_node_p;

    return state;
}

bool TransportManager::isEndpointLoopback(const bslstl::StringRef& uri) const
{
    if (bmqu::StringUtil::startsWith(uri, "tcp://")) {
        // NOTE: If we ever will listen to multiple TCP interfaces, we should
        //       update here and return true if *any* one returns true.
        return d_tcpSessionFactory_mp &&
               d_tcpSessionFactory_mp->isEndpointLoopback(uri);  // RETURN
    }
    else {
        BSLS_ASSERT_SAFE(false && "Unhandled 'uri' format.");
        return false;  // RETURN
    }
}

int TransportManager::selfNodeId(
    const bsl::vector<mqbcfg::ClusterNode>& nodes) const
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // d_mutex LOCK
    return selfNodeIdLocked(nodes);
}

}  // close package namespace
}  // close enterprise namespace
