// Copyright 2015-2023 Bloomberg Finance L.P.
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

// mqbnet_tcpsessionfactory.h                                         -*-C++-*-
#ifndef INCLUDED_MQBNET_TCPSESSIONFACTORY
#define INCLUDED_MQBNET_TCPSESSIONFACTORY

//@PURPOSE: Provide a mechanism to generate sessions over a TCP connection.
//
//@CLASSES:
//  mqbnet::TCPSessionFactory:         mechanism to generate sessions overs TCP
//  mqbnet::TCPSessionFactoryIterator: iterator over TCPSessionFactory
//
//@SEE_ALSO: mqbnet::Negotiator
//
//@DESCRIPTION: 'mqbnet::TCPSessionFactory' is a mechanism allowing to listen
// to, or establish connection with a remote peer over a TCP connection.  This
// component allows to establish TCP channels with remote peers, and uses the
// provided 'mqbnet::Negotiator' to convert a 'bmqio::Channel' to an
// 'mqbnet::Session': a session is a negotiated channel decorated inside an
// associated session object.  The 'TCPSessionFactory' owns the sessions having
// been created.  'mqbnet::TCPSessionFactoryIterator' provides thread safe
// iteration through all the sessions of a TCP session factory.  The order of
// the iteration is implementation defined.  Thread safe iteration is provided
// by locking the factory during the iterator's construction and unlocking it
// at the iterator's destruction.  This guarantees that during the life time of
// an iterator, the factory can't be modified.
//
/// SMART HEARTBEAT
///---------------
// Smart-Heartbeat is a feature allowing to detect, with minimal overhead,
// stale or unresponsive remote peer (this could happen for example if the
// machine where the remote peer is running got a hardware crash and the TCP
// stack did not cleanly shutdown).  When enabled, if no data has been received
// on the channel during a configurable time-window, a 'HeartbeatReq' message
// will be sent, to which the peer is expected to immediately respond with a
// 'HeartbeatRsp' event, effectively generating incoming traffic.  This
// implementation allows for minimal network overhead as data is only generated
// if no normal regular traffic was received from that peer.
//
// This can be enabled on a per channel basis, by setting the
// 'maxMissedHeartbeat' on the 'InitialConnectionHandlerContext' to a non-zero
// value (cf. 'SessionNegotiator' implementation for the per-connection type
// value of that setting).  Note that a value of 1 for 'maxMissedHeartbeat'
// will not work as the channel will be closed immediately, without allowing
// for the 'HeartbeatRsp' to be received.
//
// It is implemented by keeping track of events received on the channel; and a
// recurring scheduler event (default to every 3s, called the 'heartbeat
// interval') checks received activity on all enabled channel, emitting
// 'heartbeatReq' if no data was received, and resetting the channel if no
// data is received after at least 'maxMissedHeartbeat' heartbeat intervals, as
// explained below:
//..
//   |..........|..........|..........|..........|..........|.....>
//   0          1          2          3          4          5
//            ^   ^
//            |   |
//      Pt1 --+   +-- Pt2
//..
// In the above diagram, where times flows to the right and each unit represent
// a heartbeat interval, with a 'maxMissedHeartbeat' value of 3, the channel
// will be reset at [4] if the last packet received was at time [Pt1] and at
// [5] if the last packet received was at time [Pt2].  In short, with a default
// 'heartbeatInterval' value of 3s, and a 'maxMissedHeartbeat' value of 4,
// stale connection will be dropped after a time of ']12;16]' seconds.

// MQB

#include <mqbcfg_messages.h>
#include <mqbnet_initialconnectionhandler.h>
#include <mqbnet_negotiator.h>
#include <mqbstat_statcontroller.h>

#include <bmqex_sequentialcontext.h>
#include <bmqio_channel.h>
#include <bmqio_channelfactory.h>
#include <bmqio_reconnectingchannelfactory.h>
#include <bmqio_resolvingchannelfactory.h>
#include <bmqio_statchannelfactory.h>
#include <bmqio_status.h>
#include <bmqp_heartbeatmonitor.h>
#include <bmqst_statcontext.h>
#include <bmqu_sharedresource.h>

// BDE
#include <bdlbb_blob.h>
#include <bdlmt_eventscheduler.h>
#include <bsl_functional.h>
#include <bsl_memory.h>
#include <bsl_ostream.h>
#include <bsl_string.h>
#include <bsl_unordered_map.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bslmt_condition.h>
#include <bslmt_mutex.h>
#include <bsls_assert.h>
#include <bsls_atomic.h>

namespace BloombergLP {

namespace mqbnet {

// FORWARD DECLARATION
class Cluster;
class Session;
class SessionEventProcessor;
class TCPSessionFactoryIterator;
struct TCPSessionFactory_OperationContext;

// =======================
// class TCPSessionFactory
// =======================

/// Mechanism to generate sessions over TCP channels.
class TCPSessionFactory {
    // FRIENDS
    friend class TCPSessionFactoryIterator;

  public:
    // CONSTANTS

    /// Name of a property set on the channel representing the peer's IP.
    static const char* k_CHANNEL_PROPERTY_PEER_IP;

    /// Name of a property set on the channel representing the local port.
    static const char* k_CHANNEL_PROPERTY_LOCAL_PORT;

    /// Name of a property set on the channel representing the BTE channel
    /// id.
    static const char* k_CHANNEL_PROPERTY_CHANNEL_ID;

    /// Name of a property set on the channel status representing if the
    /// channel was closed due to the broker shutting down.
    static const char* k_CHANNEL_STATUS_CLOSE_REASON;

    // TYPES

    /// Signature of the callback method for a `connect` or `listen` call
    /// where the specified `event` indicates the reason of this call, with
    /// the specified `status` representing whether it was a success or some
    /// failure, and the specified `session` being populated in case of
    /// `CHANNEL_UP` `event`.  The specified `resultState` is a user data
    /// provided by the Negotiator in the `InitialConnectionHandlerContext`
    /// struct used during negotiation of the session.  The specified `readCb`
    /// serves as the read data callback when `enableRead` is called.  If the
    /// negotiation has specified cluster name (as in the case of proxy or
    /// cluster node) connection, the specified `cluster` is the
    /// corresponding cluster.  Otherwise, if the negotiation has not
    /// specified cluster name (as in the case of Client connection), the
    /// `cluster` is 0.  The callback returns `true` upon successful
    /// registration / read enabling; `false` otherwise.
    typedef bsl::function<bool(bmqio::ChannelFactoryEvent::Enum    event,
                               const bmqio::Status&                status,
                               const bsl::shared_ptr<Session>&     session,
                               Cluster*                            cluster,
                               void*                               resultState,
                               const bmqio::Channel::ReadCallback& readCb)>
        ResultCallback;

  private:
    // PRIVATE TYPES

    /// Struct holding internal data associated to an active channel
    struct ChannelInfo {
        bmqio::Channel* d_channel_p;
        // The channel

        bsl::shared_ptr<Session> d_session_sp;
        // The session tied to the channel

        SessionEventProcessor* d_eventProcessor_p;
        // The event processor of Events received on
        // this channel.

        bmqp::HeartbeatMonitor d_monitor;

        explicit ChannelInfo(const bsl::shared_ptr<bmqio::Channel>& channel,
                             const InitialConnectionHandlerContext& context,
                             int initialMissedHeartbeatCounter,
                             const bsl::shared_ptr<Session>& monitoredSession);
    };

    /// This class provides mechanism to store a map of port stat contexts.
    class PortManager {
      public:
        // PUBLIC TYPES
        struct PortContext {
            bsl::shared_ptr<bmqst::StatContext> d_portContext;
            bsl::size_t                         d_numChannels;
        };
        typedef bsl::unordered_map<bsl::uint16_t, PortContext> PortMap;

      private:
        // PRIVATE DATA

        /// A map of all ports
        PortMap d_portMap;

        /// Allocator to use
        bslma::Allocator* d_allocator_p;

      public:
        // CREATORS
        explicit PortManager(bslma::Allocator* allocator = 0);

        // PUBLIC METHODS
        /// Create a sub context of the specified 'parent' with the specified
        /// 'endpoint' as the StatContext's name. Increases the number of
        /// channels on the specified 'port'.
        bslma::ManagedPtr<bmqst::StatContext>
        addChannelContext(bmqst::StatContext* parent,
                          const bsl::string&  endpoint,
                          bsl::uint16_t       port);

        /// Handle the deletion of a StatContext associated with a channel
        /// connected to the specified 'port'.
        void onDeleteChannelContext(bsl::uint16_t port);
    };

    typedef bsl::shared_ptr<ChannelInfo> ChannelInfoSp;

    /// Map associating a `Channel` to its corresponding `ChannelInfo` (as
    /// shared_ptr because of the atomicInt which has no copy constructor).
    typedef bsl::unordered_map<bmqio::Channel*, ChannelInfoSp> ChannelMap;

    /// Shortcut for a managedPtr to the `bmqio::TCPChannelFactory`
    typedef bslma::ManagedPtr<bmqio::ChannelFactory> TCPChannelFactoryMp;

    typedef bslma::ManagedPtr<bmqio::ResolvingChannelFactory>
        ResolvingChannelFactoryMp;

    typedef bslma::ManagedPtr<bmqio::ReconnectingChannelFactory>
        ReconnectingChannelFactoryMp;

    typedef bslma::ManagedPtr<bmqio::StatChannelFactory> StatChannelFactoryMp;

    typedef TCPSessionFactory_OperationContext OperationContext;

    typedef bsl::shared_ptr<bmqio::ChannelFactory::OpHandle> OpHandleSp;

    typedef bsl::unordered_map<bmqio::Channel*, bsls::Types::Int64>
        TimestampMap;

    typedef bsl::unordered_map<int, OpHandleSp> ListeningHandleMap;

  private:
    // DATA
    bmqu::SharedResource<TCPSessionFactory> d_self;
    // Used to make sure no callback
    // is invoked on a destroyed
    // object.

    bool d_isStarted;
    // Has this component been
    // started ?

    /// Config to use for setting up this SessionFactory
    mqbcfg::TcpInterfaceConfig d_config;

    bdlmt::EventScheduler* d_scheduler_p;
    // Event scheduler held not owned

    bdlbb::BlobBufferFactory* d_blobBufferFactory_p;
    // BlobBuffer factory to use
    // (passed to the ChannelFactory)

    // Initial Connection Handler to use for orchestraing
    // authentication and negotiation
    InitialConnectionHandler* d_initialConnectionHandler_p;

    Negotiator* d_negotiator_p;
    // Negotiator to use for
    // converting a Channel to a
    // Session

    mqbstat::StatController* d_statController_p;
    // Channels' stat context (passed
    // to TCPSessionFactory)

    TCPChannelFactoryMp d_tcpChannelFactory_mp;
    // ChannelFactory

    bmqex::SequentialContext d_resolutionContext;
    // Executor context used for
    // performing DNS resolution

    ResolvingChannelFactoryMp d_resolvingChannelFactory_mp;

    ReconnectingChannelFactoryMp d_reconnectingChannelFactory_mp;

    StatChannelFactoryMp d_statChannelFactory_mp;

    bsl::string d_threadName;
    // Name to use for the IO threads

    bsls::AtomicInt d_nbActiveChannels;
    // Number of active channels
    // (including the ones being
    // negotiated)

    bsls::AtomicInt d_nbOpenClients;
    // Number of open clients and
    // proxies (not including the
    // ones being negotiated)

    bsls::AtomicInt d_nbSessions;
    // The number of sessions
    // created; this does not need to
    // be atomic because it is always
    // manipulated under the
    // 'd_mutex' (due to usage of the
    // 'd_noSessionCondition'
    // condition variable), but it is
    // declared atomic so that we can
    // access it read-only outside
    // the mutex for logging
    // purposes.

    bslmt::Condition d_noSessionCondition;
    // Condition variable signaled
    // after the last session created
    // by this factory has been
    // destroyed.

    bslmt::Condition d_noClientCondition;
    // Condition variable signaled
    // after all clients and proxies
    // are destroyed.

    ChannelMap d_channels;
    // Map of all active channels

    PortManager d_ports;
    // Manager of all open ports

    bool d_heartbeatSchedulerActive;
    // True if the recurring
    // heartbeat check event is
    // active.

    bdlmt::EventSchedulerRecurringEventHandle d_heartbeatSchedulerHandle;
    // Scheduler handle for the
    // recurring event used to
    // heartbeat monitor the
    // channels.

    bsl::unordered_map<bmqio::Channel*, ChannelInfo*> d_heartbeatChannels;
    // Map of all channels which are
    // heartbeat enabled; only
    // manipulated from the event
    // scheduler thread.

    const int d_initialMissedHeartbeatCounter;
    // Value for initializing
    // 'ChannelInfo.d_missedHeartbeatCounter'.
    // See comments in
    // 'calculateInitialMissedHbCounter'.

    /// Handles that can be used to stop listening. Empty unless listening.
    ListeningHandleMap d_listeningHandles;

    bsls::AtomicBool d_isListening;
    // Set to 'true' before calling
    // 'listen'.  Set to 'false' in
    // 'stopListening'.

    mutable bslmt::Mutex d_mutex;
    // Mutex for thread safety of
    // this component.

    // Maintain ownership of 'OperationContext' instead of passing it to
    // 'ChannelFactory::listen' because it may delete the context (on
    // stopListening) while operation (readCallback/ negotiation) is in
    // progress.
    bsl::unordered_map<int, bsl::shared_ptr<OperationContext> >
        d_listenContexts;

    TimestampMap d_timestampMap;
    // Map of HiRes timestamp of the session beginning per channel.

    bslma::Allocator* d_allocator_p;
    // Allocator to use

  private:
    // PRIVATE MANIPULATORS

    /// Create and return the statContext to be used for tracking stats of
    /// the specified `channel` obtained from the specified `handle`.
    bslma::ManagedPtr<bmqst::StatContext> channelStatContextCreator(
        const bsl::shared_ptr<bmqio::Channel>&                  channel,
        const bsl::shared_ptr<bmqio::StatChannelFactoryHandle>& handle);

    /// Asynchronously initial connection on the specified `channel` using the
    /// specified `context`.
    void initialConnect(const bsl::shared_ptr<bmqio::Channel>&   channel,
                        const bsl::shared_ptr<OperationContext>& context);

    // PRIVATE MANIPULATORS

    /// Process a protocol packet received from the specified `channel` with
    /// the associated specified `channelInfo`.  If the specified `status`
    /// is 0, this indicates data is available in the specified `blob`;
    /// otherwise this indicates an error.  Should fill in the specified
    /// `numNeeded` with the number of extra bytes needed to get a complete
    /// packet.  Invoke the `processEvent` method of the `eventProcessor`
    /// member of `channelInfo` with the event.  Note that once the channel
    /// onClose event has been fired, `channelInfo` will be dangling and
    /// should not be accessed.
    void readCallback(const bmqio::Status& status,
                      int*                 numNeeded,
                      bdlbb::Blob*         blob,
                      ChannelInfo*         channelInfo);

    /// Method invoked when the negotiation of the specified `channel` is
    /// complete, whether it be success or failure.  The specified
    /// `userData` is the `OperationContext` struct created during the
    /// listen or connect call that is responsible for this negotiation (and
    /// hence, in the case of `listen`, is common for all sessions
    /// negotiated); while the specified `negotiatorContext` corresponds to
    /// the unique context passed it to the `negotiate` method of the
    /// Negotiator, for that `channel`.  If the specified `statusCode` is 0,
    /// the negotiation was a success and the specified `session` contains
    /// the negotiated session.  If `status` is non-zero, the negotiation
    /// was a failure and `session` will be null, with the specified
    /// `errorDescription` containing a description of the error.  In either
    /// case, the specified `callback` must be invoked to notify the channel
    /// factory of the status.
    void
    negotiationComplete(int                             statusCode,
                        const bsl::string&              errorDescription,
                        const bsl::shared_ptr<Session>& session,
                        const bsl::shared_ptr<bmqio::Channel>&   channel,
                        const bsl::shared_ptr<OperationContext>& context,
                        const bsl::shared_ptr<InitialConnectionHandlerContext>&
                            negotiatorContext);

    /// Custom deleter of the session's shared_ptr for the specified
    /// `session` (of type `Session`) associated with the specified `sprep`
    /// (of type `bslma::SharedPtrRep`).  The specified `self` must be
    /// locked to ensure this object is alive.
    void onSessionDestroyed(const bsl::weak_ptr<TCPSessionFactory>& self,
                            void*                                   session,
                            void*                                   sprep);

    /// ChannelFactory channel state callback method provided to `connect`
    /// or `listen` to be informed of events.  The specified `event`
    /// indicates the type of `event`.  The specified `status` provides
    /// information about the source of error when 'event ==
    /// e_CONNECT_FAILED'.  The specified `context` is the operation context
    /// associated with the `listen` or `connect` associated with the
    /// invocation.  The specified `channel` is provided on `e_CHANNEL_UP`.
    void
    channelStateCallback(bmqio::ChannelFactoryEvent::Enum         event,
                         const bmqio::Status&                     status,
                         const bsl::shared_ptr<bmqio::Channel>&   channel,
                         const bsl::shared_ptr<OperationContext>& context);

    // PRIVATE MANIPULATORS

    /// Method invoked by the channel factory to notify that the specified
    /// `channel` went down, with the specified `status` corresponding to
    /// the channel's status, `userData` corresponding to the one provided
    /// when calling `addObserver` to register this object as observer of
    /// the channel.
    virtual void onClose(const bsl::shared_ptr<bmqio::Channel>& channel,
                         const bmqio::Status&                   status);

    /// Reccuring scheduler event to check for all `heartbeat-enabled`
    /// channels : this will send a heartbeat if no data has been received
    /// on a given channel, or proactively reset the channel if too many
    /// heartbeats have been missed.
    void onHeartbeatSchedulerEvent();

    /// Enable heartbeat for the channel represented by the specified
    /// `channelInfo`.
    void enableHeartbeat(ChannelInfo* channelInfo);

    /// Disable heartbeat for the channel represented by the specified
    /// `channelInfo`.  Note that `channelInfo` is passed as a shared_ptr to
    /// guarantee thread safety and that the object is still alive until the
    /// event scheduler processes it.
    void disableHeartbeat(const bsl::shared_ptr<ChannelInfo>& channelInfo);

    /// Log open session time for the specified `sessionDescription` and
    /// `channel`, using the stored begin
    /// timestamp. After logging, begin timestamp is removed from
    /// timestamps map.
    void logOpenSessionTime(const bsl::string& sessionDescription,
                            const bsl::shared_ptr<bmqio::Channel>& channel);
    /// @brief Check that the TCP interfaces are valid.
    ///
    /// We require the following:
    /// - The name of each listener interface is unique
    /// - The port of each listener interface is unqiue
    ///
    /// @returns 0 on success, nonzero on failure.
    int validateTcpInterfaces() const;

    /// Cancel any open listener operations and clear them out.
    void cancelListeners();

  private:
    // NOT IMPLEMENTED

    /// Copy constructor and assignment operator are not implemented
    TCPSessionFactory(const TCPSessionFactory&);             // = delete;
    TCPSessionFactory& operator=(const TCPSessionFactory&);  // = delete;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(TCPSessionFactory,
                                   bslma::UsesBslmaAllocator)

  public:
    // CREATORS

    /// Create a new `TCPSessionFactory` configured with the specified
    /// `config`, `scheduler` and `blobBufferFactory`, and using the
    /// specified `negotiator` for session negotiation.  Use the specified
    /// `allocator` for any memory allocation.
    TCPSessionFactory(const mqbcfg::TcpInterfaceConfig& config,
                      bdlmt::EventScheduler*            scheduler,
                      bdlbb::BlobBufferFactory*         blobBufferFactory,
                      InitialConnectionHandler* initialConnectionHandler,
                      Negotiator*               negotiator,
                      mqbstat::StatController*  statController,
                      bslma::Allocator*         allocator);

    /// Destructor
    virtual ~TCPSessionFactory();

    // MANIPULATORS

    /// Start the session: create the channel factory but do not yet listen
    /// on the port defined in the configuration.  Returns 0 on success and
    /// non-zero otherwise writing in the specified `errorDescription`
    /// stream a description of the error.
    int start(bsl::ostream& errorDescription);

    /// Start the listening on the port defined in the configuration.
    /// Return 0 on success or a non-zero value and populate the specified
    /// `errorDescription` with a description of the error otherwise.
    /// Invoke the specified `resultCallback` when a session has been
    /// negotiated.
    int startListening(bsl::ostream&         errorDescription,
                       const ResultCallback& resultCallback);

    /// Stop listening on the port defined in the configuration.  If the
    /// port is not being listened on, this method has no effect.
    void stopListening();

    /// Stop the factory and close all currently active sessions.  This
    /// method will block until all sessions have been destroyed.
    void stop();

    /// Close all client and proxy sessions and block until sessions are
    /// deleted.
    void closeClients();

    /// Create a new listener interface specified by  `listener` for incoming
    /// connections and invoke the specified `resultCallback` when a connection
    /// has been negotiated. Return 0 on success, or non-zero on error.
    int listen(const mqbcfg::TcpInterfaceListener& listener,
               const ResultCallback&               resultCallback);

    /// Initiate a connection to the specified `endpoint` and return 0 if
    /// the connection has successfully been started; with the result being
    /// provided by a call to the specified `resultCallback`; or return a
    /// non-zero code on error, in which case `resultCallback` will never be
    /// invoked.  The optionally specified `negotiationUserData` will be
    /// passed in to the `negotiate` method of the Negotiator (through the
    /// InitialConnectionHandlerContext).  The optionally specified
    /// `resultState` will be used to set the initial value of the
    /// corresponding member of the `InitialConnectionHandlerContext` that will
    /// be created for negotiation of this session; so that it can be retrieved
    /// in the `negotiationComplete` callback method.  The optionally specified
    /// `shouldAutoReconnect` will be used to determine if the factory should
    /// attempt to reconnect upon loss of connection.
    int connect(const bslstl::StringRef& endpoint,
                const ResultCallback&    resultCallback,
                bslma::ManagedPtr<void>* negotiationUserData = 0,
                void*                    resultState         = 0,
                bool                     shouldAutoReconnect = false);

    /// Set the write queue low and high watermarks for the specified
    /// `session` to the `d_config.lowNodeWatermark()` and
    /// `d_config.highNodeWatermark()` values by calling underlying
    /// transport.  Return `true` on success, and `false` otherwise.
    bool setNodeWriteQueueWatermarks(const Session& session);

    // ACCESSORS

    /// Return true if the endpoint in the specified `uri` represents a
    /// loopback connection, i.e., an endpoint to which this interface is a
    /// listener of; meaning that establishing a connection to `uri` would
    /// result in connecting to ourself.
    bool isEndpointLoopback(const bslstl::StringRef& uri) const;
};

// ===============================
// class TCPSessionFactoryIterator
// ===============================

/// Provide thread safe iteration through all the `mqbnet::Session`s owned
/// by the TCP session factory.  The order of the iteration is
/// implementation defined.  An iterator is *valid* if it is associated with
/// a session in the factory, otherwise it is *invalid*.  Thread-safe
/// iteration is provided by locking the factory during the iterator's
/// construction and unlocking it at the iterator's destruction.  This
/// guarantees that during the life time of an iterator, the factory can't
/// be modified.
class TCPSessionFactoryIterator {
  private:
    // DATA
    const TCPSessionFactory* d_factory_p;

    TCPSessionFactory::ChannelMap::const_iterator d_iterator;

  private:
    // NOT IMPLEMENTED
    TCPSessionFactoryIterator(const TCPSessionFactoryIterator&);
    TCPSessionFactoryIterator& operator=(const TCPSessionFactoryIterator&);

  public:
    // CREATORS

    /// Create an iterator for the specified `factory` and associated it
    /// with the first session of the `factory`.  If the `factory` is empty
    /// then the iterator is initialized to be invalid.  The `factory` is
    /// locked for the duration of iterator's life time.  The behavior is
    /// undefined unless `factory` is not null.
    explicit TCPSessionFactoryIterator(const TCPSessionFactory* factory);

    /// Destroy this iterator and unlock the factory associated with it.
    ~TCPSessionFactoryIterator();

    // MANIPULATORS

    /// Advance this iterator to refer to the next session of the associated
    /// factory; if there is no next session in the associated factory, then
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

// -------------------------------
// class TCPSessionFactoryIterator
// -------------------------------

// CREATORS
inline TCPSessionFactoryIterator::TCPSessionFactoryIterator(
    const TCPSessionFactory* factory)
: d_factory_p(factory)
, d_iterator()
{
    BSLS_ASSERT_SAFE(factory);
    d_factory_p->d_mutex.lock();
    d_iterator = d_factory_p->d_channels.begin();
}

inline TCPSessionFactoryIterator::~TCPSessionFactoryIterator()
{
    d_factory_p->d_mutex.unlock();
}

// MANIPULATORS
inline void TCPSessionFactoryIterator::operator++()
{
    ++d_iterator;
}

// ACCESSORS
inline TCPSessionFactoryIterator::operator const void*() const
{
    return (d_iterator == d_factory_p->d_channels.end())
               ? 0
               : const_cast<TCPSessionFactoryIterator*>(this);
}

inline bsl::weak_ptr<mqbnet::Session>
TCPSessionFactoryIterator::session() const
{
    BSLS_ASSERT_SAFE(*this);

    return d_iterator->second->d_session_sp;
}

}  // close package namespace
}  // close enterprise namespace

#endif
