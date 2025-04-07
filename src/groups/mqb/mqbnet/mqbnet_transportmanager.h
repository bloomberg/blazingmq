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

// mqbnet_transportmanager.h                                          -*-C++-*-
#ifndef INCLUDED_MQBNET_TRANSPORTMANAGER
#define INCLUDED_MQBNET_TRANSPORTMANAGER

//@PURPOSE: Provide a manager for all transport interfaces and clusters.
//
//@CLASSES:
//  mqbnet::TransportManager: Manager for all transport interfaces and clusters
//  mqbnet::TransportManagerIterator: Mechanism to iterate over sessions
//
//@DESCRIPTION: 'mqbnet::TransportManager' provides a mechanism to manage all
// network interfaces as well as clusters of nodes of abstracted transport
// protocol and individual outgoing connections.
// 'mqbnet::TransportManagerIterator' provides thread safe iteration through
// all the sessions of a transport manager.  The order of the iteration is
// implementation defined.  Thread safe iteration is provided by locking the
// transport manager during the iterator's construction and unlocking it at the
// iterator's destruction.  This guarantees that during the life time of an
// iterator, the manager can't be modified.
//
// CONNECTIONS
//-----------
// TransportManager handles 3 kind of connections, all related to clusters:
//: o inter-cluster connections: connections between peer nodes within a same
//:   cluster
//: o proxy-cluster connections: connections from a 'proxy' broker to the nodes
//:   composing a cluster
//: o reversed connections: outgoing connections established from this node to
//:   a remote peer, typically used to go around connectivity restrictions
//:   (firewall) and allow that remote peer to use this connection channel as a
//:   proxy-cluster communication.
//
/// CLUSTER
///-------
// There are two kinds of cluster at the application layer:
//: o a cluster for which the broker is member of, aka 'Cluster'
//: o a cluster for a broker not being part of the cluster, aka 'ProxyCluster'
// A single class, 'mqbnet::Cluster', is used to represent both types.
// However, their creation is slightly different: in the case of a
// 'ProxyCluster', a connection is established from this broker to each node
// composing the 'Cluster', because effectively, only this broker cares about
// those connections - the 'real cluster' isn't aware of proxies, and as far as
// it is concerned, this broker will just appear as a regular client connected
// to it.  On the other hand, for the 'Cluster' clusters, each broker needs a
// full mesh of connectivity with all other nodes composing the cluster.  In
// order to not have two connections between each node, connections are
// established only from a node having a lower node Id to a node having a
// higher node Id.

// MQB

#include <mqbcfg_messages.h>
#include <mqbnet_channel.h>
#include <mqbnet_initialconnectionhandler.h>
#include <mqbnet_tcpsessionfactory.h>
#include <mqbstat_statcontroller.h>

#include <bmqio_channel.h>
#include <bmqio_channelfactory.h>

// BDE
#include <ball_log.h>
#include <bdlbb_blob.h>
#include <bdlmt_eventscheduler.h>
#include <bsl_memory.h>
#include <bsl_ostream.h>
#include <bsl_string.h>
#include <bsl_unordered_map.h>
#include <bsl_vector.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bslmt_mutex.h>
#include <bsls_assert.h>
#include <bsls_atomic.h>

namespace BloombergLP {

namespace mqbnet {

// FORWARD DECLARATION
class Cluster;
class ClusterNode;
class Negotiator;
class Session;
class SessionEventProcessor;
class TransportManagerIterator;

// ======================
// class TransportManager
// ======================

/// Manager for all transport interfaces and clusters of abstracted
/// transport protocol.
class TransportManager {
  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBNET.TRANSPORTMANAGER");

  public:
    // TYPES
    enum ConnectionMode {
        // Enum describing how the cluster should establish connection with the
        // nodes of the cluster.

        e_LISTEN_ALL = 0  // Expect incoming connection from all nodes
        ,
        e_CONNECT_ALL = 1  // Connect out to all nodes
        ,
        e_MIXED = 2  // Connect out to higher node Ids only, expect
                     // incoming connection from lower node Ids
    };

  private:
    // FRIENDS
    friend class TransportManagerIterator;

    // PRIVATE TYPES

    enum State {
        e_STOPPED  = 0,  // Stopped.
        e_STARTING = 1,  // Starting (and reconnecting) but not listening.
        e_STARTED  = 2,  // Listening / reconnecting.
        e_STOPPING = 3   // Shutting down.
    };

    /// Structure representing the state associated to a connection with a
    /// remote peer, which may either belong to a cluster (wether proxy or
    /// not) or a reversed connection.  Note that even for `listen`
    /// connections of a `cluster`, such a state is created.
    struct ConnectionState {
        // PUBLIC DATA
        bsl::string d_endpoint;
        // Endpoint addressing the remote
        // peer.

        bool d_isClusterConnection;
        // Indicates whether this connection
        // represents a cluster connection
        // (whether proxy or real cluster),
        // or not (i.e., a reversed
        // connection).

        ClusterNode* d_node_p;
        // Node associated to this
        // connection.  Note that this field
        // is only relevant if
        // 'd_isClusterConnection', and is
        // invalidated when the cluster is
        // destroyed.

        bsl::shared_ptr<void> d_userData_sp;
        // The user data, if any, provided
        // to the 'createCluster' or the
        // 'connectOut' methods.  This
        // correspond to the negotiator
        // user data.
    };

    /// A map for holding all the connection states (ptr to its associated
    /// shared ptr).
    typedef bsl::unordered_map<ConnectionState*,
                               bsl::shared_ptr<ConnectionState> >
        ConnectionsStateMap;

  private:
    // DATA
    /// Allocator store to spawn new allocators for sub-components
    bmqma::CountingAllocatorStore d_allocators;

    bsls::AtomicInt d_state;
    // enum State.  Always changed on
    // the main thread.

    bdlmt::EventScheduler* d_scheduler_p;
    // Event scheduler, held not owned

    bdlbb::BlobBufferFactory* d_blobBufferFactory_p;
    // BlobBufferFactory to use by the
    // sessions

    // Initial Connection to use
    bslma::ManagedPtr<InitialConnectionHandler> d_initialConnectionHandler_mp;

    mqbstat::StatController* d_statController_p;
    // Stat controller

    bslma::ManagedPtr<TCPSessionFactory> d_tcpSessionFactory_mp;
    // TCPSessionFactory

    ConnectionsStateMap d_connectionsState;
    // Map of all connections state.
    // This map is expected to be
    // relatively small: there will be
    // one entry per node per cluster
    // this broker is either member of,
    // or proxying to; plus one entry
    // for each reverse connection this
    // broker should establish.

    mutable bslmt::Mutex d_mutex;
    // Mutex for thread safety of this
    // component

  private:
    // PRIVATE MANIPULATORS

    /// Create and start the TCPInterface using the specified `config`.
    /// Return 0 on success or a non-zero value and populate the specified
    /// `errorDescription` with a description of the error otherwise.
    int createAndStartTcpInterface(bsl::ostream& errorDescription,
                                   const mqbcfg::TcpInterfaceConfig& config);

    /// Notify ClusterNode about the specified `session` and add self as
    /// an observer of the channel if the `session` represents a cluster
    /// node.  Otherwise, enable reading for the `session`.  If the
    /// negotiation has specified cluster name (as in the case of proxy or
    /// cluster node) connection, the specified `cluster` is the
    /// corresponding cluster.  Otherwise, if the negotiation has not
    /// specified cluster name (as in the case of Client connection), the
    /// `cluster` is 0.  Return `true` upon successful read enabling;
    /// `false` otherwise.
    bool processSession(Cluster*                            cluster,
                        ConnectionState*                    state,
                        const bsl::shared_ptr<Session>&     session,
                        const bmqio::Channel::ReadCallback& readCb);

    /// Signature of the callback method for a `connect` or `listen` call
    /// (as indicated by the specified `isListen` flags) where the specified
    /// `event` indicates the reason of this call, with the specified
    /// `status` representing whether it was a success or some failure, and
    /// the specified `session` being populated in case of `CHANNEL_UP`
    /// `event`.  The specified `resultState` is a user data provided by the
    /// negotiator in the `InitialConnectionHandlerContext` struct used during
    /// negotiation of the session.  The specified `readCb` serves as read data
    /// callback when `enableRead` is called.  If the negotiation has specified
    /// cluster name (as in the case of proxy or cluster node) connection,
    /// the specified `cluster` is the corresponding cluster.  Otherwise, if
    /// the negotiation has not specified cluster name (as in the case of
    /// Client connection), the `cluster` is 0.  Return `true` upon
    /// successful registration / read enabling; `false` otherwise.
    bool sessionResult(bmqio::ChannelFactoryEvent::Enum    event,
                       const bmqio::Status&                status,
                       const bsl::shared_ptr<Session>&     session,
                       Cluster*                            cluster,
                       void*                               resultState,
                       const bmqio::Channel::ReadCallback& readCb,
                       bool                                isListen);

    int connect(ConnectionState* state);

    /// Connect to the endpoint described in the specified `state` and
    /// return 0 on success, or a non-zero return code on error.  The
    /// `locked` version requires `d_mutex` to be locked before being
    /// called.
    int connectLocked(ConnectionState* state);

    // PRIVATE MANIPULATORS

    /// Method invoked by the channel factory to notify that the channel
    /// with the specified `channelDescription` went down; and with the
    /// specified `state` corresponding to the `ConnectionState` associated
    /// to this channel.  Note that we do not bind the channel but rather
    /// the `channelDescription` because we have a hierarchy of channel
    /// (`TCP < Resolving < Stat`); and when the close is invoked, it comes
    /// from the lower channel (`bmqio::TCPChannel`) while the higher
    /// channel (which is the one that would have been bindable at the time
    /// of the `onClose` slot registration) has already been destroyed.
    virtual void onClose(const bsl::string& channelDescription,
                         ConnectionState*   state);

    // PRIVATE ACCESSORS

    /// Return the nodeId of the current host in the specified cluster
    /// `nodes`, or `Cluster::k_INVALID_NODE_ID` if this host is not
    /// part of that cluster.
    int selfNodeIdLocked(const bsl::vector<mqbcfg::ClusterNode>& nodes) const;

  private:
    // NOT IMPLEMENTED

    /// Copy constructor and assignment operator are not implemented.
    TransportManager(const TransportManager&);             // = delete;
    TransportManager& operator=(const TransportManager&);  // = delete;

  private:
    // PRIVATE CLASS METHODS

    /// Custom deleter for the `managedPtr<Cluster>` (which raw pointer is
    /// the specified `object`) returned by the `createCluster()` method of
    /// the specified `transportManager` instance; used to clean up
    /// references to this cluster and its node.  Note that the method is
    /// static so it can access private members of the instance of the
    /// transport manager).
    static void onClusterReleased(void* object, void* transportManager);

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(TransportManager, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a new `TransportManager` using the specified `scheduler`,
    /// `blobBufferFactory`, `negotiator` and `statController` and the
    /// specified `allocator` for any memory allocation.
    TransportManager(
        bdlmt::EventScheduler*                       scheduler,
        bdlbb::BlobBufferFactory*                    blobBufferFactory,
        bslma::ManagedPtr<InitialConnectionHandler>& initialConnectionHandler,
        mqbstat::StatController*                     statController,
        bslma::Allocator*                            allocator);

    /// Destructor
    virtual ~TransportManager();

    // MANIPULATORS

    /// Start all interfaces from the configuration.  Return 0 on success or
    /// a non-zero value and populate the specified `errorDescription` with
    /// a description of the error otherwise.  Note that the interfaces are
    /// created and started but not listening to incoming connections (which
    /// is achieved by a call to `startListening()`).
    int start(bsl::ostream& errorDescription);

    /// Start the listening for any interfaces that was created and should
    /// be listening.  Return 0 on success or a non-zero value and populate
    /// the specified `errorDescription` with a description of the error
    /// otherwise.
    int startListening(bsl::ostream& errorDescription);

    /// Stop listening for any interfaces that were created and listening.
    /// This method has no effect if this instance has not been started, or
    /// if no interfaces are listening.
    void initiateShutdown();

    /// Close all client and proxy sessions and block until sessions are
    /// deleted.
    void closeClients();

    /// Stop all interfaces.
    void stop();

    /// Load into the specified `out` a cluster having the specified `name`
    /// and `nodes`, and using the specified `connectionMode` to establish
    /// connection with the nodes:
    /// * `e_CONNECT_ALL`:  connection out to all nodes from the
    ///   `definition` will be established; this is the case when the node
    ///   is a proxy, establishing connection to the cluster
    /// * `e_LISTEN_ALL`: no outbound connection wil be made, it is
    ///   expected that the remote nodes will be connecting to this node;
    ///   this is the case when the node is on a DMZ network and is setup
    ///   to go through a gateway cluster
    /// * `e_MIXED`: only connections to node Ids with a value higher than
    ///   the current node Id (detected by the local machine in the
    ///   `definition`) will be established (the idea is that in this mode,
    ///   the other end will be connecting to us; this is to prevent
    ///   double connections); this is the case when the node is part of a
    ///   cluster.
    /// The optionally specified `userData` will be passed in to the
    /// `negotiate` method of the negotiator (through the
    /// InitialConnectionHandlerContext) for any connections being established
    /// as a result of this cluster creation.  Return 0 on success, or a
    /// non-zero value and populate the specified `errorDescription` with a
    /// description of the error in case of failure.
    int createCluster(bsl::ostream&                           errorDescription,
                      bslma::ManagedPtr<mqbnet::Cluster>*     out,
                      const bsl::string&                      name,
                      const bsl::vector<mqbcfg::ClusterNode>& nodes,
                      ConnectionMode                          connectionMode,
                      bslma::ManagedPtr<void>*                userData = 0);

    /// Connect out to the specified `uri`.  Note that the connection will
    /// be considered `persistent` with auto-reconnection when it goes down.
    /// The optionally specified `userData` will be passed in to the
    /// `negotiate` method of the negotiator (through the
    /// InitialConnectionHandlerContext) for any connections being established
    /// as a result of this connection creation.  Return 0 on success, or a
    /// non-zero value and populate the specified `errorDescription` with a
    /// description of the error in case of failure.
    int connectOut(bsl::ostream&            errorDescription,
                   const bslstl::StringRef& uri,
                   bslma::ManagedPtr<void>* userData = 0);

    /// Return the raw pointer `cookie handle` to the internal state
    /// associated to the specified `nodeId` in the specified `clusterName`,
    /// or return 0 and populate the specified `errorDescription` with a
    /// description of the error in case of issues.  On success, populate
    /// the specified `clusterNode` with the corresponding
    /// `mqbnet::clusterNode` object.  Note that this is a method only
    /// intended to be used from the session negotiator in order to populate
    /// the `resultState` of the `InitialConnectionHandlerContext`.
    void* getClusterNodeAndState(bsl::ostream&            errorDescription,
                                 ClusterNode**            clusterNode,
                                 const bslstl::StringRef& clusterName,
                                 int                      nodeId);

    // ACCESSORS

    /// Return true if the endpoint in the specified `uri` represents a
    /// loopback connection, i.e., an endpoint to which this broker is a
    /// listener of; meaning that establishing a connection to `uri` would
    /// result in connecting to ourself.
    bool isEndpointLoopback(const bslstl::StringRef& uri) const;

    // ACCESSORS

    /// Return the nodeId of the current host in the specified cluster
    /// `nodes`, or `Cluster::k_INVALID_NODE_ID` if this host is not part
    /// of that cluster.
    int selfNodeId(const bsl::vector<mqbcfg::ClusterNode>& nodes) const;
};

// ==============================
// class TransportManagerIterator
// ==============================

/// Provide thread safe iteration through all the `mqbnet::Session`s owned
/// by the transport manager.  The order of the iteration is implementation
/// defined.  An iterator is *valid* if it is associated with a session in
/// the transport manager, otherwise it is *invalid*.  Thread-safe iteration
/// is provided by locking the manager during the iterator's construction
/// and unlocking it at the iterator's destruction.  This guarantees that
/// during the life time of an iterator, the manager can't be modified.
class TransportManagerIterator {
  private:
    // DATA
    const TransportManager* d_manager_p;

    TCPSessionFactoryIterator d_tcpSessionFactoryIter;

  private:
    // NOT IMPLEMENTED
    TransportManagerIterator(const TransportManagerIterator&);
    TransportManagerIterator& operator=(const TransportManagerIterator&);

  public:
    // CREATORS

    /// Create an iterator for the specified `manager` and associated it
    /// with the first session of the `manager`.  If the `manager` is empty
    /// then the iterator is initialized to be invalid.  The `manager` is
    /// locked for the duration of iterator's life time.  The behavior is
    /// undefined unless `manager` is not null.
    explicit TransportManagerIterator(const TransportManager* manager);

    /// Destroy this iterator and unlock the manager associated with it.
    ~TransportManagerIterator();

    // MANIPULATORS

    /// Advance this iterator to refer to the next session of the associated
    /// manager; if there is no next session in the associated manager, then
    /// this iterator becomes *invalid*.  The behavior is undefined unless
    /// this iterator is valid.  Note that the order of the iteration is
    /// not specified.
    void operator++();

    // ACCESSORS

    /// Return non-zero if the iterator is *valid*, and 0 otherwise.
    operator const void*() const;

    /// Return a weak pointer to the session associated with this iterator.
    /// The behavior is undefined unless the iterator is *valid*.
    bsl::weak_ptr<mqbnet::Session> session() const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ------------------------------
// class TransportManagerIterator
// ------------------------------

// CREATORS
inline TransportManagerIterator::TransportManagerIterator(
    const TransportManager* manager)
: d_manager_p(manager)
, d_tcpSessionFactoryIter(manager->d_tcpSessionFactory_mp.get())
{
}

inline TransportManagerIterator::~TransportManagerIterator()
{
    // NOTHING: 'd_tcpSessionFactoryIter' will unlock the lock in its
    //          destructor.
}

// MANIPULATORS
inline void TransportManagerIterator::operator++()
{
    ++d_tcpSessionFactoryIter;
}

// ACCESSORS
inline TransportManagerIterator::operator const void*() const
{
    return d_tcpSessionFactoryIter;
}

inline bsl::weak_ptr<mqbnet::Session> TransportManagerIterator::session() const
{
    BSLS_ASSERT_SAFE(*this);
    return d_tcpSessionFactoryIter.session();
}

}  // close package namespace
}  // close enterprise namespace

#endif
