// Copyright 2021-2023 Bloomberg Finance L.P.
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

// mqbblp_relayqueueengine.h                                          -*-C++-*-
#ifndef INCLUDED_MQBBLP_RELAYQUEUEENGINE
#define INCLUDED_MQBBLP_RELAYQUEUEENGINE

//@PURPOSE: Provide a QueueEngine in a relay BlazingMQ node (replica or proxy)
//
//@CLASSES:
//  mqbblp::RelayQueueEngine: QueueEngine in a relay BlazingMQ node
//  (replica/proxy)
//
//@DESCRIPTION: 'mqbblp::RelayQueueEngine' provides an 'mqbi::QueueEngine'
// implementation for a relay BlazingMQ node (replica or proxy).
//
/// KNOWN Issues:
///------------
// Since messages are not rejected back to upstream, there is a potential
// situation of pseudo-starvation: let's imagine a proxy has two readers, one
// with maxUnackedMessages of 10, and one with maxUnackedMessages of 100,
// upstream will then deliver up to 110 messages to this proxy.  If the '100
// unackedMessages' goes down, this proxy will keep those messages to
// distribute them to the '10 unackedMessages', which might be undesirable;
// instead the proxy should 'reject' up to 100 messages.

// MQB

#include <mqbblp_queueengineutil.h>
#include <mqbblp_queuehandlecatalog.h>
#include <mqbblp_routers.h>
#include <mqbi_dispatcher.h>
#include <mqbi_queue.h>
#include <mqbi_queueengine.h>
#include <mqbi_storage.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_queueutil.h>
#include <bmqt_messageguid.h>

// MWC
#include <mwcc_orderedhashmap.h>
#include <mwcu_sharedresource.h>

// BDE
#include <ball_log.h>
#include <bdlmt_throttle.h>
#include <bsl_list.h>
#include <bsl_memory.h>
#include <bsl_ostream.h>
#include <bsl_unordered_map.h>
#include <bsl_utility.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_atomic.h>
#include <bsls_keyword.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace bdlmt {
class EventScheduler;
}
namespace mqbcmd {
class QueueEngine;
}
namespace mqbi {
class DispatcherClient;
}
namespace mqbs {
class VirtualStorageCatalog;
}

namespace mqbblp {

// FORWARD DECLARATION
class QueueState;

// ================================
// struct RelayQueueEngine_AppState
// ================================

/// RelayQueueEngine needs to keep track of all parameters advertised
/// upstream but not necessarily confirmed per each AppId (including
/// default).
///
/// The difference with RootQueueEngine is that they update queue handle
/// immediately upon `configureHandle` while RelayQueueEngine waits for
/// `onHandleConfiguredDispatched` to update the handle.  Hence the need to
/// have the (extra) cache which is updated immediately.
struct RelayQueueEngine_AppState : QueueEngineUtil_AppState {
    struct CachedParameters {
        bmqp_ctrlmsg::QueueHandleParameters d_handleParameters;
        bmqp_ctrlmsg::StreamParameters      d_streamParameters;
        unsigned int                        d_downstreamSubQueueId;

        CachedParameters(
            const bmqp_ctrlmsg::QueueHandleParameters& handleParameters,
            unsigned int                               downstreamSubQueueId,
            bslma::Allocator*                          allocator);
    };

    typedef bsl::unordered_map<mqbi::QueueHandle*, CachedParameters>
        CachedParametersMap;

    CachedParametersMap d_cache;
    // Parameters to advertise upstream for this app

    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(RelayQueueEngine_AppState,
                                   bslma::UsesBslmaAllocator)

    // CREATORS
    explicit RelayQueueEngine_AppState(
        unsigned int                             upstreamSubQueueId,
        const bsl::string&                       appId,
        bslma::ManagedPtr<mqbi::StorageIterator> iterator,
        mqbi::Queue*                             queue,
        bdlmt::EventScheduler*                   scheduler,
        const mqbu::StorageKey&                  appKey,
        Routers::QueueRoutingContext&            queueContext,
        bslma::Allocator*                        allocator = 0);
};

// ======================
// class RelayQueueEngine
// ======================

/// QueueEngine implementation for a BlazingMQ relay node (replica/proxy).
class RelayQueueEngine : public mqbi::QueueEngine {
  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBBLP.RELAYQUEUEENGINE");

  private:
    // PRIVATE TYPES
    typedef bslma::ManagedPtr<mqbi::StorageIterator> StorageIterMp;
    typedef bsl::shared_ptr<mqbi::StorageIterator>   StorageIterSp;

    /// (queueHandlePtr, downstreamSubQueueId)
    typedef QueueHandleCatalog::DownstreamKey DownstreamKey;

    typedef RelayQueueEngine_AppState  App_State;
    typedef bsl::shared_ptr<App_State> AppStateSp;

    /// (subId) -> App_State map
    typedef bsl::unordered_map<unsigned int, AppStateSp> AppsMap;

    class AutoPurger;
    // A guard helper class.

    friend class AutoPurger;
    // Has to have access to private member variables.

    /// This struct serves as multiplexor when sending configure request(s)
    /// (plural in the case of wildcard) to upstream.  Once all responses
    /// are collected (there are no more references), it calls the specified
    /// callback
    struct ConfigureContext {
        // TRAITS
        BSLMF_NESTED_TRAIT_DECLARATION(ConfigureContext,
                                       bslma::UsesBslmaAllocator)

        mqbi::QueueHandle::HandleConfiguredCallback d_configuredCb;
        bmqp_ctrlmsg::Status                        d_status;
        const bmqp_ctrlmsg::StreamParameters        d_streamParameters;

        /// Routing data advertised upstream (including Subscription Ids).
        /// Upon response from upstream, this becomes effective.
        bsl::shared_ptr<Routers::AppContext> d_routing_sp;

        bslma::Allocator* d_allocator_p;

        ConfigureContext(
            const mqbi::QueueHandle::HandleConfiguredCallback& configuredCb,
            const bmqp_ctrlmsg::StreamParameters& streamParameters,
            bslma::Allocator*                     allocator);

        ~ConfigureContext();

        void setStatus(const bmqp_ctrlmsg::StatusCategory::Value& category,
                       int                                        code,
                       const bslstl::StringRef&                   message);

        void invokeCallback();

        void resetCallback();

        void reset();

        void initializeRouting(Routers::QueueRoutingContext& queueContext);
    };

  private:
    // DATA
    QueueState* d_queueState_p;

    mqbs::VirtualStorageCatalog* d_subStreamMessages_p;
    // List of messages that need to be
    // delivered to sub-streams, as indicated
    // by the upstream node.  Note that this
    // variable is non null only if self node
    // is a replica *and* queue is not in
    // broadcast mode.  If non null, the object
    // itself is owned by the associated
    // RemoteQueue instance.

    const mqbconfm::Domain d_domainConfig;

    AppsMap d_apps;
    // Map of (appId) to App_State.

    mwcu::SharedResource<RelayQueueEngine> d_self;
    // Used to avoid executing a callback if
    // the engine has been destroyed.  For
    // example, upon queue converting to local.

    bdlmt::EventScheduler* d_scheduler_p;
    // Event scheduler currently used for
    // message throttling. Held, not owned.

    bdlmt::Throttle d_throttledRejectedMessages;
    // Throttler for REJECTs.

    QueueEngineUtil_AppsDeliveryContext d_appsDeliveryContext;
    // Reusable apps delivery context

    bslma::Allocator* d_allocator_p;
    // Allocator to use.
  private:
    // PRIVATE CLASS METHODS

    /// Routine executed after `getHandle` method on this queue engine
    /// instance has been invoked, where the specified `ptr` is a pointer
    /// to the queue engine instance, and the specified `cookie` is a
    /// pointer to a boolean flag indicating whether the handle was created
    /// or not.
    static void onHandleCreation(void* ptr, void* cookie);

  private:
    // PRIVATE MANIPULATORS

    /// Schedule processing of the stream configuration response from
    /// upstream of the specified `handle` with the specified
    /// `downStreamParameters` per the specified `status`.  The specified
    /// `upStreamParameters` is the upstream view of the stream parameters
    /// of this node.  When the specified `context` reference count drops to
    /// zero, invoke associated callback. Note that the specified `self`
    /// must be locked to ensure the engine is still alive at the time the
    /// callback is invoked.
    ///
    /// THREAD: This method is called from any thread.
    void onHandleConfigured(
        const bsl::weak_ptr<RelayQueueEngine>&   self,
        const bmqp_ctrlmsg::Status&              status,
        const bmqp_ctrlmsg::StreamParameters&    upStreamParameters,
        mqbi::QueueHandle*                       handle,
        const bmqp_ctrlmsg::StreamParameters&    downStreamParameters,
        const bsl::shared_ptr<ConfigureContext>& context);

    /// Process stream configuration response from upstream of the specified
    /// `handle` with the specified `downStreamParameters` per the specified
    /// `status`.  The specified `upStreamParameters` is the upstream view
    /// of the stream parameters of this node.  When the specified `context`
    /// reference count drops to zero, invoke associated callback.  Note
    /// that the specified `self` must be locked to ensure the engine is
    /// still alive at the time the callback is invoked.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    void onHandleConfiguredDispatched(
        const bsl::weak_ptr<RelayQueueEngine>&   self,
        const bmqp_ctrlmsg::Status&              status,
        const bmqp_ctrlmsg::StreamParameters&    upStreamParameters,
        mqbi::QueueHandle*                       handle,
        const bmqp_ctrlmsg::StreamParameters&    downStreamParameters,
        const bsl::shared_ptr<ConfigureContext>& context);

    /// Schedule processing of the release response from upstream of the
    /// specified `handle` with the specified `handleParameters` per the
    /// specified `status` and associated with the specified `clientCb`.
    /// The specified `isFinal` indicates if the handle operation is final.
    ///
    /// THREAD: This method is called from any thread.
    void onHandleReleased(
        const bmqp_ctrlmsg::Status&                                  status,
        mqbi::QueueHandle*                                           handle,
        const bmqp_ctrlmsg::QueueHandleParameters&                   hp,
        const bsl::shared_ptr<QueueEngineUtil_ReleaseHandleProctor>& context);

    void onHandleReleasedDispatched(
        const bmqp_ctrlmsg::Status&                                  status,
        mqbi::QueueHandle*                                           handle,
        const bmqp_ctrlmsg::QueueHandleParameters&                   hp,
        const bsl::shared_ptr<QueueEngineUtil_ReleaseHandleProctor>& context);

    /// Attempt to deliver outstanding messages, if any, to the active
    /// consumers.  Behavior is undefined unless there is at least one
    /// active consumer.
    void deliverMessages();

    void processAppRedelivery(App_State& state, const bsl::string& appId);

    /// Configure the specified `handle` with the specified
    /// `streamParameters` for the specified `appState`.  When the specified
    /// `context` reference count drops to zero, invoke associated callback.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    void configureApp(App_State&                            appState,
                      mqbi::QueueHandle*                    handle,
                      const bmqp_ctrlmsg::StreamParameters& streamParameters,
                      const bsl::shared_ptr<ConfigureContext>& context);

    void releaseHandleImpl(
        mqbi::QueueHandle*                                           handle,
        const bmqp_ctrlmsg::QueueHandleParameters&                   hp,
        const bsl::shared_ptr<QueueEngineUtil_ReleaseHandleProctor>& proctor);

    /// Rebuild the effective upstream state for the specified
    /// `upstreamSubQueueId` using parameters cached in the specified
    /// `appState`.  The `upstreamSubQueueId` is k_DEFAULT_SUBQUEUE_ID
    /// unless the queue is of the fanout type.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    void rebuildUpstreamState(Routers::AppContext* context,
                              App_State*           appState,
                              unsigned int         upstreamSubQueueId,
                              const bsl::string&   appId);

    /// Make effective the previously built routing results containing
    /// Subscription ids advertised upstream.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    void applyConfiguration(App_State& appState, ConfigureContext& context);

  private:
    // NOT IMPLEMENTED

    /// Copy constructor and assignment operator are not implemented.
    RelayQueueEngine(const RelayQueueEngine&);
    RelayQueueEngine& operator=(const RelayQueueEngine&);

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(RelayQueueEngine, bslma::UsesBslmaAllocator)

  public:
    // CREATORS
    RelayQueueEngine(QueueState*                  queueState,
                     mqbs::VirtualStorageCatalog* subStreamMessages,
                     const mqbconfm::Domain&      domainConfig,
                     bslma::Allocator*            allocator);

    /// Destructor
    virtual ~RelayQueueEngine() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS
    //   (virtual mqbi::QueueEngine)

    /// Configure this instance.  Return zero on success, non-zero value
    /// otherwise and populate the specified `errorDescription`.
    virtual int
    configure(bsl::ostream& errorDescription) BSLS_KEYWORD_OVERRIDE;

    /// Reset the internal state of this engine.
    virtual void resetState() BSLS_KEYWORD_OVERRIDE;

    /// Rebuild the internal state of this engine.  This method is invoked
    /// when the queue this engine is associated with is created from an
    /// existing one, and takes ownership of the already created handles
    /// (typically happens when the queue gets converted between local and
    /// remote).  Return zero on success, non-zero value otherwise and
    /// populate the specified `errorDescription`.  Note that
    /// `rebuildInternalState` must be called on an empty-state object
    /// (i.e., which has just been constructed, or following a call to
    /// `resetState`) after it has been configured.
    virtual int
    rebuildInternalState(bsl::ostream& errorDescription) BSLS_KEYWORD_OVERRIDE;

    /// Obtain and return a handle to this queue for the client identified
    /// with the specified `clientContext`, using the specified
    /// `handleParameters`, and invoke the specified `callback` when
    /// finished. In case of error, return a null pointer.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    virtual mqbi::QueueHandle*
    getHandle(const bsl::shared_ptr<mqbi::QueueHandleRequesterContext>&
                                                          clientContext,
              const bmqp_ctrlmsg::QueueHandleParameters&  handleParameters,
              unsigned int                                upstreamSubQueueId,
              const mqbi::QueueHandle::GetHandleCallback& callback)
        BSLS_KEYWORD_OVERRIDE;

    /// Configure the specified `handle` with the specified
    /// `streamParameters` and invoke the specified `configuredCb` when
    /// finished.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    virtual void configureHandle(
        mqbi::QueueHandle*                                 handle,
        const bmqp_ctrlmsg::StreamParameters&              streamParameters,
        const mqbi::QueueHandle::HandleConfiguredCallback& configuredCb)
        BSLS_KEYWORD_OVERRIDE;

    /// Reconfigure the specified `handle` by releasing the specified
    /// `parameters` from its current settings and invoke the specified
    /// `releasedCb` when done.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    virtual void
    releaseHandle(mqbi::QueueHandle*                         handle,
                  const bmqp_ctrlmsg::QueueHandleParameters& handleParameters,
                  bool                                       isFinal,
                  const mqbi::QueueHandle::HandleReleasedCallback& releasedCb)
        BSLS_KEYWORD_OVERRIDE;

    /// Called when the specified `handle` is usable and ready to receive
    /// messages (usually meaning its client has become available) for the
    /// specified `upstreamSubscriptionId` subscription of the queue.  When
    /// this method is called, the queue engine should deliver outstanding
    /// messages to the `handle`.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    virtual void
    onHandleUsable(mqbi::QueueHandle* handle,
                   unsigned int upstreamSubscriptionId) BSLS_KEYWORD_OVERRIDE;

    /// Called by the mqbi::Queue when a new message with the specified
    /// `msgGUID` is available on the queue and ready to be sent to eventual
    /// interested clients.  If available, the specified `source` points to
    /// the originator of the message.
    virtual void
    afterNewMessage(const bmqt::MessageGUID& msgGUID,
                    mqbi::QueueHandle*       source) BSLS_KEYWORD_OVERRIDE;

    /// Called by the `mqbi::Queue` when the message identified by the
    /// specified `msgGUID` is confirmed for the specified
    /// `upstreamSubQueueId` stream of the queue on behalf of the client
    /// identified by the specified `handle`.  Return a negative value on
    /// error (GUID was not found, etc.), 0 if this confirm was for the last
    /// reference to that message and it can be deleted from the queue's
    /// associated storage, or 1 if there are still references.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    virtual int
    onConfirmMessage(mqbi::QueueHandle*       handle,
                     const bmqt::MessageGUID& msgGUID,
                     unsigned int upstreamSubQueueId) BSLS_KEYWORD_OVERRIDE;

    /// Called by the `mqbi::Queue` when the message identified by the
    /// specified `msgGUID` is rejected for the specified
    /// `upstreamSubQueueId` stream of the queue on behalf of the client
    /// identified by the specified `handle`.  Return resulting RDA counter.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    int onRejectMessage(mqbi::QueueHandle*       handle,
                        const bmqt::MessageGUID& msgGUID,
                        unsigned int upstreamSubQueueId) BSLS_KEYWORD_OVERRIDE;

    /// Called by the mqbi::Queue before a message with the specified
    /// `msgGUID` is removed from the queue (either it's TTL expired, it was
    /// confirmed by all recipients, etc). The QueueEngine may use this to
    /// update the positions of the QueueHandles it manages.
    virtual void beforeMessageRemoved(const bmqt::MessageGUID& msgGUID)
        BSLS_KEYWORD_OVERRIDE;

    /// Called by the mqbi::Queue *after* *all* messages are removed from
    /// the storage for the client identified by the specified `appId` and
    /// `appKey` (queue has been deleted or purged by admin task, etc).
    /// QueueEngine may use this to update the positions of the QueueHandles
    /// it manages.  Note that `appKey` may be null, in which case the
    /// `purge` action is applicable to the entire queue.  Also note that
    /// `appId` must be empty if and only if `appKey` is null.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    virtual void
    afterQueuePurged(const bsl::string&      appId,
                     const mqbu::StorageKey& appKey) BSLS_KEYWORD_OVERRIDE;

    /// Periodically invoked with the current time provided in the specified
    /// `currentTimer`; can be used for regular status check, such as for
    /// ensuring messages on the queue are flowing and not accumulating.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    virtual void
    onTimer(bsls::Types::Int64 currentTimer) BSLS_KEYWORD_OVERRIDE;

    /// Not valid for 'RelayQueueEngine'
    mqbi::StorageResult::Enum evaluateAutoSubscriptions(
        const bmqp::PutHeader&              putHeader,
        const bsl::shared_ptr<bdlbb::Blob>& appData,
        const bmqp::MessagePropertiesInfo&  mpi,
        bsls::Types::Uint64                 timestamp) BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS

    /// Return the reference count that should be applied to a message
    /// posted to the queue managed by this engine.  Note that returned
    /// value may or may not be equal to `numOpenReaderHandles()` depending
    /// upon the specific type of this engine.
    virtual unsigned int messageReferenceCount() const BSLS_KEYWORD_OVERRIDE;

    /// Load into the specified `out` object the internal information about
    /// this queue engine and associated queue handles.
    virtual void
    loadInternals(mqbcmd::QueueEngine* out) const BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS

    /// Load upstream subQueue id into the specified `subQueueId` given the
    /// specified upstream `subscriptionId`.
    /// Each subStream has unique Subscription ids.
    bool subscriptionId2upstreamSubQueueId(unsigned int* subQueueId,
                                           unsigned int  subscriptionId) const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// --------------------------------
// struct RelayQueueEngine_AppState
// --------------------------------

inline RelayQueueEngine_AppState::RelayQueueEngine_AppState(
    unsigned int                             upstreamSubQueueId,
    const bsl::string&                       appId,
    bslma::ManagedPtr<mqbi::StorageIterator> iterator,
    mqbi::Queue*                             queue,
    bdlmt::EventScheduler*                   scheduler,
    const mqbu::StorageKey&                  appKey,
    Routers::QueueRoutingContext&            queueContext,
    bslma::Allocator*                        allocator)
: QueueEngineUtil_AppState(iterator,
                           queue,
                           scheduler,
                           true,
                           queueContext,
                           upstreamSubQueueId,
                           appId,
                           appKey,
                           allocator)
, d_cache(allocator)
{
    // NOTHING
}

// --------------------------------------------------
// struct RelayQueueEngine_AppState::CachedParameters
// --------------------------------------------------

inline RelayQueueEngine_AppState::CachedParameters::CachedParameters(
    const bmqp_ctrlmsg::QueueHandleParameters& handleParameters,
    unsigned int                               downstreamSubQueueId,
    bslma::Allocator*                          allocator)
: d_handleParameters(handleParameters)
, d_streamParameters(allocator)
, d_downstreamSubQueueId(downstreamSubQueueId)
{
    // NOTHING
}

// -----------------------------------------
// struct RelayQueueEngine::ConfigureContext
// -----------------------------------------

inline RelayQueueEngine::ConfigureContext::ConfigureContext(
    const mqbi::QueueHandle::HandleConfiguredCallback& configuredCb,
    const bmqp_ctrlmsg::StreamParameters&              streamParameters,
    bslma::Allocator*                                  allocator)
: d_configuredCb(configuredCb)
, d_status()
, d_streamParameters(streamParameters, allocator)
, d_routing_sp()
, d_allocator_p(allocator)
{
    d_status.category() = bmqp_ctrlmsg::StatusCategory::E_SUCCESS;
    d_status.code()     = 0;
}

inline RelayQueueEngine::ConfigureContext::~ConfigureContext()
{
    invokeCallback();
}

inline void RelayQueueEngine::ConfigureContext::setStatus(
    const bmqp_ctrlmsg::StatusCategory::Value& category,
    int                                        code,
    const bslstl::StringRef&                   message)
{
    d_status.category() = category;
    d_status.code()     = code;
    d_status.message()  = message;
}

inline void RelayQueueEngine::ConfigureContext::invokeCallback()
{
    if (d_configuredCb) {
        d_configuredCb(d_status, d_streamParameters);
        resetCallback();
    }
}

inline void RelayQueueEngine::ConfigureContext::resetCallback()
{
    d_configuredCb = bsl::nullptr_t();
}

inline void RelayQueueEngine::ConfigureContext::initializeRouting(
    Routers::QueueRoutingContext& queueContext)
{
    d_routing_sp.reset(new (*d_allocator_p)
                           Routers::AppContext(queueContext, d_allocator_p),
                       d_allocator_p);
}

// ----------------------
// class RelayQueueEngine
// ----------------------

inline unsigned int RelayQueueEngine::messageReferenceCount() const
{
    // Irrespective of number of worker-consumers or their status (dead or
    // alive), we always return 1
    return 1;
}

}  // close package namespace
}  // close enterprise namespace

#endif
