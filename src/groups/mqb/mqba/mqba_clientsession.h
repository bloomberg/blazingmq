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

// mqba_clientsession.h                                               -*-C++-*-
#ifndef INCLUDED_MQBA_CLIENTSESSION
#define INCLUDED_MQBA_CLIENTSESSION

//@PURPOSE: Provide a session for interaction with BlazingMQ broker clients.
//
//@CLASSES:
//  mqba::ClientSession     : mechanism representing a session with a client
//  mqba::ClientSessionState: VST representing the state of a session
//
//@DESCRIPTION: This component provides a mechanism, 'mqba::ClientSession',
// that allows BlazingMQ broker to send and receive messages from a client
// connected to the broker, whether it is a producer or a consumer or a peer
// BlazingMQ broker.  'mqba::ClientSessionState' is a value semantic type
// holding the state associated to an 'mqba::Session'.

// MQB

#include <mqbblp_queuesessionmanager.h>
#include <mqbconfm_messages.h>
#include <mqbi_dispatcher.h>
#include <mqbi_domain.h>
#include <mqbi_queue.h>
#include <mqbnet_session.h>
#include <mqbstat_queuestats.h>

// BMQ
#include <bmqp_ackeventbuilder.h>
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_protocol.h>
#include <bmqp_pusheventbuilder.h>
#include <bmqp_queueid.h>
#include <bmqp_schemaeventbuilder.h>
#include <bmqt_uri.h>

// MWC
#include <mwcio_channel.h>
#include <mwcio_channelfactory.h>
#include <mwcsys_time.h>
#include <mwcu_operationchain.h>
#include <mwcu_sharedresource.h>

// BDE
#include <bdlb_nullablevalue.h>
#include <bdlbb_blob.h>
#include <bdlcc_objectpool.h>
#include <bdlcc_sharedobjectpool.h>
#include <bdlmt_eventscheduler.h>
#include <bdlmt_throttle.h>
#include <bsl_deque.h>
#include <bsl_memory.h>
#include <bsl_ostream.h>
#include <bsl_string.h>
#include <bsl_unordered_map.h>
#include <bsl_utility.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_atomic.h>
#include <bsls_types.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace bslmt {
class Semaphore;
}
namespace bmqp {
class Event;
}
namespace bmqt {
class MessageGUID;
}
namespace mqbi {
class QueueHandle;
}
namespace mqbblp {
class ClusterCatalog;
}
namespace mwcst {
class StatContext;
}

namespace mqba {

// =========================
// struct ClientSessionState
// =========================

/// VST representing the state of a session
struct ClientSessionState {
  public:
    // TYPES

    /// VST containing information about a message that has been put but not
    /// yet acked.
    struct UnackedMessageInfo {
        // DATA
        int                d_correlationId;  // Correlation Id of the message.
        bsls::Types::Int64 d_timeStamp;      // The time when the message was
                                             // received, in absolute
                                             // nanoseconds referenced to an
                                             // arbitrary but fixed origin.

        // CREATORS
        UnackedMessageInfo(int correlationId, bsls::Types::Int64 timeStamp);
    };

    /// Pool of shared pointers to Blobs
    typedef bdlcc::SharedObjectPool<
        bdlbb::Blob,
        bdlcc::ObjectPoolFunctors::DefaultCreator,
        bdlcc::ObjectPoolFunctors::RemoveAll<bdlbb::Blob> >
        BlobSpPool;

    typedef mqbblp::QueueSessionManager::QueueState QueueState;

    typedef mqbblp::QueueSessionManager::SubQueueInfo SubQueueInfo;

    /// Map of queueId to QueueState
    typedef mqbblp::QueueSessionManager::QueueStateMap QueueStateMap;

    typedef QueueStateMap::iterator QueueStateMapIter;

    typedef QueueStateMap::const_iterator QueueStateMapCIter;

    typedef QueueState::StreamsMap StreamsMap;

    /// Map of MessageGUID -> UnackedMessageInfo
    typedef bsl::unordered_map<bmqt::MessageGUID,
                               UnackedMessageInfo,
                               bslh::Hash<bmqt::MessageGUIDHashAlgo> >
        UnackedMessageInfoMap;

    typedef bsl::pair<UnackedMessageInfoMap::iterator, bool>
        UnackedMessageInfoMapInsertRc;

    typedef bslma::ManagedPtr<mwcst::StatContext> StatContextMp;

  public:
    // PUBLIC DATA
    bslma::Allocator* d_allocator_p;
    // Allocator to use.

    bsl::deque<bdlbb::Blob> d_channelBufferQueue;
    // Queue of data pending being sent to
    // the client.  This should almost
    // always be empty, and is meant to
    // provide buffer for a 'throttling'
    // mechanism when sending huge burst of
    // data (typically at queue open) that
    // would go beyond the channel high
    // watermark.  This should only be
    // manipulated from the dispatcher
    // thread. Note that it really should
    // be a queue and not a deque, but
    // queue doesn't have a 'clear' method.

    UnackedMessageInfoMap d_unackedMessageInfos;
    // Map containing the
    // GUID->UnackedMessageInfo entries.

    mqbi::DispatcherClientData d_dispatcherClientData;
    // Dispatcher client data associated to
    // this session.

    StatContextMp d_statContext_mp;
    // Stat context dedicated to this
    // domain, to use as the parent stat
    // context for any queue in this
    // domain.

    bdlbb::BlobBufferFactory* d_bufferFactory_p;
    // Blob buffer factory to use.

    BlobSpPool* d_blobSpPool_p;
    // Pool of shared pointers to blob to
    // use.

    bmqp::SchemaEventBuilder d_schemaEventBuilder;
    // Builder for schema messages.  To be
    // used only in client dispatcher
    // thread.

    bmqp::PushEventBuilder d_pushBuilder;
    // Builder for push messages.  To be
    // used only in client dispatcher
    // thread.

    bmqp::AckEventBuilder d_ackBuilder;
    // Builder for ack messages.  To be
    // used only in client dispatcher
    // thread.

    bdlmt::Throttle d_throttledFailedAckMessages;
    // Throttler for failed ACK messages.

    bdlmt::Throttle d_throttledFailedPutMessages;
    // Throttler for failed PUT messages.

    bdlb::NullableValue<mqbstat::QueueStatsClient> d_invalidQueueStats;
    // Stats associated with an unknown
    // queue, lazily created when the first
    // usage of an unknown queue is
    // encountered

  private:
    // NOT IMPLEMENTED

    /// Copy constructor and assignment operator are not implemented.
    ClientSessionState(const ClientSessionState&);             // = delete;
    ClientSessionState& operator=(const ClientSessionState&);  // = delete;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(ClientSessionState,
                                   bslma::UsesBslmaAllocator)

    // CREATORS

    /// Constructor of a new session state using the specified `dispatcher`,
    /// `clientStatContext`, `blobSpPool` and `bufferFactory`.  The
    /// specified `encodingType` is the encoding which the schema event
    /// builder will use. Memory allocations are performed using the
    /// specified `allocator`.
    ClientSessionState(
        bslma::ManagedPtr<mwcst::StatContext>& clientStatContext,
        BlobSpPool*                            blobSpPool,
        bdlbb::BlobBufferFactory*              bufferFactory,
        bmqp::EncodingType::Enum               encodingType,
        bslma::Allocator*                      allocator);
};

// ===================
// class ClientSession
// ===================

/// A session with a BlazingMQ client application
class ClientSession : public mqbnet::Session,
                      public mqbi::DispatcherClient,
                      public mqbi::QueueHandleRequester {
  private:
    // PRIVATE TYPES
    typedef ClientSessionState::QueueState QueueState;

    typedef ClientSessionState::QueueStateMap QueueStateMap;

    typedef ClientSessionState::QueueStateMapIter QueueStateMapIter;

    typedef ClientSessionState::QueueStateMapCIter QueueStateMapCIter;

    typedef ClientSessionState::SubQueueInfo SubQueueInfo;

    typedef ClientSessionState::StreamsMap StreamsMap;

    typedef bsl::function<void(void)> VoidFunctor;

    /// Enum to signify the session's operation state.
    enum OperationState {
        e_RUNNING  // Running normally
        ,
        e_SHUTTING_DOWN  // Shutting down due to 'initiateShutdown' request
        ,
        e_DISCONNECTING  // Disconnecting due to the client disconnect request
        ,
        e_DISCONNECTED  // The session is disconnected and no more valid
        ,
        e_DEAD  // The session cannot do anything
    };

    struct ShutdownContext {
        ShutdownCb         d_callback;
        bsls::TimeInterval d_stopTime;
        bsls::AtomicInt64  d_numUnconfirmedTotal;

        // TRAITS
        BSLMF_NESTED_TRAIT_DECLARATION(ShutdownContext,
                                       bslma::UsesBslmaAllocator)

        // CREATORS
        ShutdownContext(const ShutdownCb&         callback,
                        const bsls::TimeInterval& timeout);

        ~ShutdownContext();
    };
    // Struct to be used as a context for shutdown operation.

    typedef bsl::shared_ptr<ShutdownContext> ShutdownContextSp;

  private:
    // DATA
    mwcu::SharedResource<ClientSession> d_self;
    // This object is used to avoid
    // executing a callback if the session
    // has been destroyed: this is *ONLY* to
    // be used with the callbacks that will
    // be called from outside of the
    // dispatcher's thread (such as a remote
    // configuration service IO thread);
    // because we can't guarantee this queue
    // is drained before destroying the
    // session.

    OperationState d_operationState;
    // Show whether the session is running
    // or shutting down due to either stop
    // request, or client's disconnect
    // request, or the channel is down.
    // Once the channel has been destroyed
    // and is no longer valid or we sent the
    // 'DisconnectResponse' to the client,
    // *NO* messages of any sort should be
    // delivered to the client.

    bool d_isDisconnecting;
    // Set to true when receiving a
    // 'DisconnectRequest' from the client;
    // only used in the processEvent (in IO
    // thread) to validate that the client
    // honors the contract and doesn't send
    // any thing after the 'Disconnect'
    // notification.

    bmqp_ctrlmsg::NegotiationMessage d_negotiationMessage;
    // Negotiation message received from the
    // remote peer.

    bmqp_ctrlmsg::ClientIdentity* d_clientIdentity_p;
    // Raw pointer to the right field in
    // 'd_negotiationMessage' (depending
    // whether it's a 'client' or a
    // 'broker').

    const bool d_isClientGeneratingGUIDs;
    // Set to true when the client identity
    // 'guidInfo' struct contains non-empty
    // 'clientId' field.  If this broker is
    // first hop then the client is SDK
    // which generates GUIDs for PUTs using
    // 'bmqp::MessageGUIDGenerator' and
    // doesn't provide correlation ids.

    bsl::string d_description;
    // Short identifier for this session.

    bsl::shared_ptr<mwcio::Channel> d_channel_sp;
    // Channel associated to this session.

    ClientSessionState d_state;
    // The state associated to this session.

    mqbblp::QueueSessionManager d_queueSessionManager;
    // Queue session manager for this
    // session.

    mqbblp::ClusterCatalog* d_clusterCatalog_p;
    // Cluster catalog to query for cluster
    // information

    bdlmt::EventScheduler* d_scheduler_p;
    // Pointer to the event scheduler to
    // use (held, not owned)

    bdlmt::EventSchedulerEventHandle d_periodicUnconfirmedCheckHandler;
    // Handler to manage the scheduled
    // event that triggers the checking of
    // the unconfirmed messages during the
    // session shutdown.

    mwcu::OperationChain d_shutdownChain;
    // Mechanism used for the session
    // graceful shutdown to serialize
    // execution of the queue handle
    // deconfigure callbacks.

    ShutdownCb d_shutdownCallback;
    // If present, call when 'tearDownAllQueuesDone'.
    // This is the callback given in 'initiateShutdown'

    bsls::Types::Int64 d_beginTimestamp;
    // HiRes timer value of the begin session/queue operation

    mwcu::MemOutStream d_currentOpDescription;
    // Stream for constructing current session/queue operation description.

  private:
    // NOT IMPLEMENTED

    /// Copy constructor and assignment operator are not implemented.
    ClientSession(const ClientSession&);             // = delete;
    ClientSession& operator=(const ClientSession&);  // = delete;

  private:
    // PRIVATE MANIPULATORS

    /// Send a control message of type error having the specified
    /// `failureCategory`, `errorDescription` and `code` attributes, in
    /// response to the specified `request`.
    void sendErrorResponse(bmqp_ctrlmsg::StatusCategory::Value failureCategory,
                           const bslstl::StringRef& errorDescription,
                           const int                code,
                           const bmqp_ctrlmsg::ControlMessage& request);

    /// Internal method to send the specified `blob` to the client.  *All*
    /// send operations from session should use this method, so that it
    /// takes care of the throttling and guarantees ordering.  If the
    /// specified `flushBuilders` is `true`, all internal builders (PUSH and
    /// ACK) will be flushed and sent before `blob`; this is necessary to
    /// guarantee strict serialization of events when sending a control
    /// message.
    void sendPacket(const bdlbb::Blob& blob, bool flushBuilders);

    /// Flush as much as possible of the content of the internal
    /// `channelBufferQueue`.
    void flushChannelBufferQueue();

    /// Append an ack message to the session's ack builder, with the
    /// specified `status`, and the specified `correlationId`, `messageGUID`
    /// and `queueId`, associated with the queue having the specified
    /// `queueState` (which may be null if the queue was not found).  The
    /// specified `isSelfGenerated` flag indicates whether the ACK is
    /// originally generated from this object, or just relayed through it.
    /// The specified `source` is used when logging, to indicate the origin
    /// of the ack.
    void sendAck(bmqt::AckResult::Enum    status,
                 int                      correlationId,
                 const bmqt::MessageGUID& messageGUID,
                 const QueueState*        queueState,
                 int                      queueId,
                 bool                     isSelfGenerated,
                 const bslstl::StringRef& source);

    /// Implementation of the teardown process, with the specified `session`
    /// representing this session and posting on the specified `semaphore`
    /// once processing is done. The specified `isBrokerShutdown` is set to
    /// `true` if the shutdown was initiated by a broker shutdown and
    /// `false` otherwise.
    void tearDownImpl(bslmt::Semaphore*            semaphore,
                      const bsl::shared_ptr<void>& session,
                      bool                         isBrokerShutdown);

    /// This method is invoked during the teardown sequence, after all
    /// queues' dispatcher queues have been drained, with the specified
    /// `session` representing a cookie to the current object allowing to
    /// control when it gets destroyed.
    void tearDownAllQueuesDone(const bsl::shared_ptr<void>& session);

    void onHandleConfigured(
        const bmqp_ctrlmsg::Status&           status,
        const bmqp_ctrlmsg::StreamParameters& streamParameters,
        const bmqp_ctrlmsg::ControlMessage&   streamParamsCtrlMsg);

    /// Called when handle configure response comes to the client session
    /// with the specified `status`, `streamParameters` and
    /// `streamParamsCtrlMsg`.
    void onHandleConfiguredDispatched(
        const bmqp_ctrlmsg::Status&           status,
        const bmqp_ctrlmsg::StreamParameters& streamParameters,
        const bmqp_ctrlmsg::ControlMessage&   streamParamsCtrlMsg);

    /// Initiate the shutdown of the session and invoke the specified
    /// `callback` upon completion of (asynchronous) shutdown sequence or
    /// if the specified `timeout` is expired.
    void initiateShutdownDispatched(const ShutdownCb&         callback,
                                    const bsls::TimeInterval& timeout);

    void invalidateDispatched();

    void checkUnconfirmed(const ShutdownContextSp& shutdownCtx,
                          const VoidFunctor&       completionCb);

    /// Initiate checking of the unconfirmed messages for each open queue
    /// handle unless the current time is greater than the stop time from
    /// the specified `shutdownCtx`.  In this case invoke the shutdown
    /// callback from the `shutdownCtx`.
    /// Always call the specified `completionCb` before return.
    void checkUnconfirmedDispatched(const ShutdownContextSp& shutdownCtx,
                                    const VoidFunctor&       completionCb);

    void finishCheckUnconfirmed(const ShutdownContextSp& shutdownCtx,
                                const VoidFunctor&       completionCb);

    /// Invoked when all the queue handles have reported their number of the
    /// unconfirmed messages.  The sum of those numbers is stored in the
    /// counter located in the specified `shutdownCtx`.  Schedule the next
    /// check of the unconfirmed messages unless the counter is already zero
    /// or the next check time is greater than stop time value from
    /// `shutdownCtx`.  In this case invoke shutdown callback from the
    /// context and return without scheduling the next check.
    /// Always call the specified `completionCb` before return.
    void finishCheckUnconfirmedDispatched(const ShutdownContextSp& shutdownCtx,
                                          const VoidFunctor& completionCb);

    void countUnconfirmed(mqbi::QueueHandle*       handle,
                          const ShutdownContextSp& shutdownCtx,
                          const VoidFunctor&       completionCb);

    /// Called from the queue dispatcher context to get the number of the
    /// unconfirmed messages that the specified `handle` has and add this
    /// number to the atomic counter stored in the specified `shutdownCtx`.
    /// Always call the specified `completionCb` before return.
    void countUnconfirmedDispatched(mqbi::QueueHandle*       handle,
                                    const ShutdownContextSp& shutdownCtx,
                                    const VoidFunctor&       completionCb);

    void processDisconnect(const bmqp_ctrlmsg::ControlMessage& controlMessage);

    void processDisconnectAllQueues(
        const bmqp_ctrlmsg::ControlMessage& controlMessage);

    /// Process the disconnect request in the specified `controlMessage`.
    /// When a `disconnect` request is received, on the IO thread, from the
    /// client, `processDisconnectAllQueues` is enqueued to be executed from
    /// the client dispatcher thread.  It will enqueue a job to the
    /// dispatcher that will execute the `processDisconnectAllQueuesDone`
    /// from one of the queue thread, once all of them has seen that event.
    /// This method will then enqueue execution of the `processDisconnect`
    /// from the client's dispatcher thread.  This complex multi enqueue is
    /// needed in order to guarantee that the `disconnect` response is the
    /// last one sent.  For example, if the client calls closeQueue async
    /// and immediately after calls stop, since closeQueue is async with a
    /// round trip to the queue dispatcher thread, we must ensure the close
    /// queue response will be delivered before the disconnect response.
    void processDisconnectAllQueuesDone(
        const bmqp_ctrlmsg::ControlMessage& controlMessage);

    void
    processOpenQueue(const bmqp_ctrlmsg::ControlMessage& handleParamsCtrlMsg);

    void
    processCloseQueue(const bmqp_ctrlmsg::ControlMessage& handleParamsCtrlMsg);

    void processConfigureStream(
        const bmqp_ctrlmsg::ControlMessage& streamParamsCtrlMsg);

    /// Process the specified ack `event`.
    void onAckEvent(const mqbi::DispatcherAckEvent& event);

    /// Process the specified confirm `event`.
    void onConfirmEvent(const mqbi::DispatcherConfirmEvent& event);

    /// Process the specified reject `event`.
    void onRejectEvent(const mqbi::DispatcherRejectEvent& event);

    /// Process the specified push `event` received from the dispatcher.
    void onPushEvent(const mqbi::DispatcherPushEvent& event);

    /// Process the specified put `event`.
    void onPutEvent(const mqbi::DispatcherPutEvent& event);

    /// Validate a message of the specified `eventType` using the specified
    /// `queueId`. Return true if the message is valid and false otherwise.
    /// Populate the specified `queueHandle` if the queue is found and load
    /// a descriptive error message into the `errorStream` if the message is
    /// invalid.
    bool validateMessage(mqbi::QueueHandle**   queueHandle,
                         bsl::ostream*         errorStream,
                         const bmqp::QueueId&  queueId,
                         bmqp::EventType::Enum eventType);

    void openQueueCb(const bmqp_ctrlmsg::Status&            status,
                     mqbi::QueueHandle*                     handle,
                     const bmqp_ctrlmsg::OpenQueueResponse& openQueueResponse,
                     const bmqp_ctrlmsg::ControlMessage& handleParamsCtrlMsg);

    void closeQueueCb(const bsl::shared_ptr<mqbi::QueueHandle>& handle,
                      const bmqp_ctrlmsg::ControlMessage& handleParamsCtrlMsg);

    /// Return a pointer to the stats associated with an unknown queue.
    /// Note that these stats are lazily created in the first invocation of
    /// this method.
    ///
    /// THREAD: This method is called from the Client's dispatcher thread.
    mqbstat::QueueStatsClient* invalidQueueStats();

    /// Validate the PUT message currently pointed to by the specified
    /// `putIt` and populate the specified `queueStateIter` with the
    /// corresponding queue state iterator if the queue is found per the
    /// queueId in the current message's header, the specified `appDataSp`
    /// with the application data of the current message if loading it is
    /// successful, and the specified `optionsSp` with the options of the
    /// current message if they are set and loading them is successful.
    /// Return true if the PUT message currently pointed to by `putIt` is
    /// valid, and false otherwise.
    ///
    /// THREAD: This method is called from the Client's dispatcher thread.
    bool validatePutMessage(QueueState**                    queueState,
                            SubQueueInfo**                  subQueueInfoIter,
                            bsl::shared_ptr<bdlbb::Blob>*   appDataSp,
                            bsl::shared_ptr<bdlbb::Blob>*   optionsSp,
                            const bmqp::PutMessageIterator& putIt);

    void closeChannel();

    /// Log session/queue operation time for the specified `opDescription`
    /// using the stored operation begin timestamp. After logging reset
    /// `opDescription` and set begin timestamp to 0.
    void logOperationTime(mwcu::MemOutStream& opDescription);

    // PRIVATE ACCESSORS

    /// Return true if the session is `e_DISCONNECTED` or worse (`e_DEAD`).
    bool isDisconnected() const;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(ClientSession, bslma::UsesBslmaAllocator)

  public:
    // CREATORS

    /// Constructor of a new session associated to the specified `channel`
    /// and using the specified `dispatcher`, `domainFactory`, `blobSpPool`,
    /// `bufferFactory` and `scheduler`.  The specified `clientStatContext`
    /// should be used as the top level for statistics associated to this
    /// session.  The specified `negotiationMessage` represents the identity
    /// received from the peer during negotiation, and the specified
    /// `sessionDescription` is the short form description of the session.
    /// Memory allocations are performed using the specified `allocator`.
    ClientSession(const bsl::shared_ptr<mwcio::Channel>&  channel,
                  const bmqp_ctrlmsg::NegotiationMessage& negotiationMessage,
                  const bsl::string&                      sessionDescription,
                  mqbi::Dispatcher*                       dispatcher,
                  mqbblp::ClusterCatalog*                 clusterCatalog,
                  mqbi::DomainFactory*                    domainFactory,
                  bslma::ManagedPtr<mwcst::StatContext>&  clientStatContext,
                  ClientSessionState::BlobSpPool*         blobSpPool,
                  bdlbb::BlobBufferFactory*               bufferFactory,
                  bdlmt::EventScheduler*                  scheduler,
                  bslma::Allocator*                       allocator);

    /// Destructor
    ~ClientSession() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS
    //   (virtual: mqbnet::Session)

    /// Process the specified `event` received from the optionally specified
    /// `source` node.  Note that this method is the entry point for all
    /// incoming events coming from the remote peer.
    void processEvent(const bmqp::Event&   event,
                      mqbnet::ClusterNode* source = 0) BSLS_KEYWORD_OVERRIDE;

    /// Method invoked when the channel associated with the session is going
    /// down.  The session object will be destroyed once the specified
    /// `session` goes out of scope.  The specified `isBrokerShutdown`
    /// indicates if the channel is going down from a shutdown. This method
    /// is executed on the IO thread, so if the session object's destruction
    /// must execute some long synchronous or heavy operation, it could
    /// offload it to a separate thread, passing in the `session` to prevent
    /// destruction of the session object until the shutdown sequence
    /// completes.
    void tearDown(const bsl::shared_ptr<void>& session,
                  bool isBrokerShutdown) BSLS_KEYWORD_OVERRIDE;

    /// Initiate the shutdown of the session and invoke the specified
    /// `callback` upon completion of (asynchronous) shutdown sequence or
    /// if the specified `timeout` is expired.
    /// The shutdown is complete when 'tearDownAllQueuesDone'.
    void
    initiateShutdown(const ShutdownCb&         callback,
                     const bsls::TimeInterval& timeout) BSLS_KEYWORD_OVERRIDE;

    /// Make the session abandon any work it has.
    void invalidate() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS
    void onWatermark(mwcio::ChannelWatermarkType::Enum type);
    void onHighWatermark();

    /// Watermark notification methods from observing the specified
    /// `channel`, which is the one associated to this object, and with the
    /// specified `userData` corresponding to the one provided when calling
    /// `addObserver`.
    void onLowWatermark();

    // MANIPULATORS
    //   (virtual: mqbi::DispatcherClient)

    /// Process the specified `event` routed by the dispatcher to this
    /// instance.  Dispatcher guarantees that all events to this instance
    /// are dispatched in a single thread.
    void onDispatcherEvent(const mqbi::DispatcherEvent& event)
        BSLS_KEYWORD_OVERRIDE;

    /// Called by the dispatcher to flush any pending operation.. mainly
    /// used to provide batch and nagling mechanism.
    void flush() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS
    //  (virtual: mqbnet::Session)

    /// Return the channel associated to this session.
    bsl::shared_ptr<mwcio::Channel> channel() const BSLS_KEYWORD_OVERRIDE;

    /// Return the clusterNode associated to this session, or 0 if there are
    /// none.
    mqbnet::ClusterNode* clusterNode() const BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS
    //   (virtual: mqbnet::Session)
    const bmqp_ctrlmsg::NegotiationMessage&
    negotiationMessage() const BSLS_KEYWORD_OVERRIDE;
    // Return a reference not offering modifiable access to the negotiation
    // message received from the remote peer of this session during the
    // negotiation phase.

    /// Return a printable description of the client (e.g. for logging).
    const bsl::string& description() const BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS
    //   (virtual: mqbi::DispatcherClient)

    /// Return a pointer to the dispatcher this client is associated with.
    mqbi::Dispatcher* dispatcher() BSLS_KEYWORD_OVERRIDE;

    /// Return a reference to the dispatcherClientData.
    mqbi::DispatcherClientData& dispatcherClientData() BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS
    //   (virtual: mqbi::DispatcherClient)

    /// Return a pointer to the dispatcher this client is associated with.
    const mqbi::Dispatcher* dispatcher() const BSLS_KEYWORD_OVERRIDE;

    const mqbi::DispatcherClientData&
    dispatcherClientData() const BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS
    //   (virtual: mqbi::QueueHandleRequester)

    /// Return a non-modifiable reference to the context of this requester
    /// of a QueueHandle.
    const bsl::shared_ptr<mqbi::QueueHandleRequesterContext>&
    handleRequesterContext() const BSLS_KEYWORD_OVERRIDE;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ---------------------------------------------
// struct ClientSessionState::UnackedMessageInfo
// ---------------------------------------------
// CREATORS
inline ClientSessionState::UnackedMessageInfo::UnackedMessageInfo(
    int                correlationId,
    bsls::Types::Int64 timeStamp)
: d_correlationId(correlationId)
, d_timeStamp(timeStamp)
{
    // NOTHING
}

// -------------------------------------
// struct ClientSession::ShutdownContext
// -------------------------------------

// CREATORS
inline ClientSession::ShutdownContext::ShutdownContext(
    const ShutdownCb&         callback,
    const bsls::TimeInterval& timeout)
: d_callback(callback)
, d_stopTime(mwcsys::Time::nowMonotonicClock())
, d_numUnconfirmedTotal(0)
{
    BSLS_ASSERT_SAFE(d_callback);
    d_stopTime += timeout;
}

inline ClientSession::ShutdownContext::~ShutdownContext()
{
    // Assume 'd_callback' does not require specific thread

    d_callback();
}

// -------------------
// class ClientSession
// -------------------

// ACCESSORS

inline bool ClientSession::isDisconnected() const
{
    return d_operationState == e_DISCONNECTED || d_operationState == e_DEAD;
}

inline bsl::shared_ptr<mwcio::Channel> ClientSession::channel() const
{
    return d_channel_sp;
}

inline mqbnet::ClusterNode* ClientSession::clusterNode() const
{
    // A ClientSession has no cluster node associated
    return 0;
}

inline const bmqp_ctrlmsg::NegotiationMessage&
ClientSession::negotiationMessage() const
{
    return d_negotiationMessage;
}

inline const bsl::string& ClientSession::description() const
{
    return d_description;
}

inline mqbi::Dispatcher* ClientSession::dispatcher()
{
    return d_state.d_dispatcherClientData.dispatcher();
}

inline const mqbi::Dispatcher* ClientSession::dispatcher() const
{
    return d_state.d_dispatcherClientData.dispatcher();
}

inline const mqbi::DispatcherClientData&
ClientSession::dispatcherClientData() const
{
    return d_state.d_dispatcherClientData;
}

inline mqbi::DispatcherClientData& ClientSession::dispatcherClientData()
{
    return d_state.d_dispatcherClientData;
}

inline const bsl::shared_ptr<mqbi::QueueHandleRequesterContext>&
ClientSession::handleRequesterContext() const
{
    return d_queueSessionManager.requesterContext();
}

}  // close package namespace
}  // close enterprise namespace

#endif
