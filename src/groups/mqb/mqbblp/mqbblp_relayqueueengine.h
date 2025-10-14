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

/// @file mqbblp_relayqueueengine.h
///
/// @brief Provide a `QueueEngine` in a relay BlazingMQ node (replica or proxy)
///
/// @bbref{mqbblp::RelayQueueEngine} provides an @bbref{mqbi::QueueEngine}
/// implementation for a relay BlazingMQ node (replica or proxy).
///
/// KNOWN Issues                              {#mqbblp_relayqueueengine_issues}
/// ============
///
/// Since messages are not rejected back to upstream, there is a potential
/// situation of pseudo-starvation: let's imagine a proxy has two readers, one
/// with maxUnackedMessages of 10, and one with maxUnackedMessages of 100,
/// upstream will then deliver up to 110 messages to this proxy.  If the 100
/// unackedMessages reader goes down, this proxy will keep those messages to
/// distribute them to the 10 unackedMessages reader, which might be
/// undesirable; instead the proxy should reject up to 100 messages.

// MQB
#include <mqbblp_pushstream.h>
#include <mqbblp_queueengineutil.h>
#include <mqbblp_queuehandlecatalog.h>
#include <mqbblp_routers.h>
#include <mqbi_dispatcher.h>
#include <mqbi_queue.h>
#include <mqbi_queueengine.h>
#include <mqbi_storage.h>

// BMQ
#include <bmqc_orderedhashmap.h>
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_queueutil.h>
#include <bmqt_messageguid.h>
#include <bmqu_sharedresource.h>

// BDE
#include <ball_log.h>
#include <bdlmt_throttle.h>
#include <bsl_cstddef.h>
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

/// RelayQueueEngine needs to keep track of all parameters advertised upstream
/// but not necessarily confirmed per each AppId (including default).
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

    /// Parameters to advertise upstream for this app.
    CachedParametersMap d_cache;

    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(RelayQueueEngine_AppState,
                                   bslma::UsesBslmaAllocator)

    // CREATORS
    explicit RelayQueueEngine_AppState(
        unsigned int                  upstreamSubQueueId,
        const bsl::string&            appId,
        mqbi::Queue*                  queue,
        bdlmt::EventScheduler*        scheduler,
        Routers::QueueRoutingContext& queueContext,
        bslma::Allocator*             allocator = 0);
};

// ======================
// class RelayQueueEngine
// ======================

/// QueueEngine implementation for a BlazingMQ relay node (replica/proxy).
class RelayQueueEngine BSLS_KEYWORD_FINAL : public mqbi::QueueEngine {
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

    /// (appId) -> App_State map
    typedef bsl::unordered_map<bsl::string, AppStateSp> AppIds;

    /// A guard helper class.
    class AutoPurger;

    /// Has to have access to private member variables.
    friend class AutoPurger;

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

        void setStatus(const bmqp_ctrlmsg::Status& status);

        void invokeCallback();

        void resetCallback();

        void reset();

        void initializeRouting(Routers::QueueRoutingContext& queueContext);
    };

  private:
    // DATA
    QueueState* d_queueState_p;

    PushStream d_pushStream;

    const mqbconfm::Domain d_domainConfig;

    /// Map of (appId) to App_State.
    AppsMap d_apps;

    /// (appId) -> App_State map
    AppIds d_appIds;

    /// Used to avoid executing a callback if the engine has been destroyed.
    /// For example, upon queue converting to local.
    bmqu::SharedResource<RelayQueueEngine> d_self;

    /// Throttler for REJECTs.
    bdlmt::Throttle d_throttledRejectedMessages;

    /// Reusable apps delivery context
    QueueEngineUtil_AppsDeliveryContext d_appsDeliveryContext;

    /// Storage iterator to the PushStream.
    bslma::ManagedPtr<PushStreamIterator> d_storageIter_mp;

    /// Storage iterator to access storage state.
    bslma::ManagedPtr<mqbi::StorageIterator> d_realStorageIter_mp;

    /// Allocator to use.
    bslma::Allocator* d_allocator_p;

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
    void processAppRedelivery(unsigned int upstreamSubQueueId, App_State* app);

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

    mqbi::Storage* storage() const;

    App_State* findApp(unsigned int upstreamSubQueueId) const;

    bool isDuplicate(const App_State*         app,
                     const bmqt::MessageGUID& msgGUID) const;

    void
    storePushIfProxy(mqbi::StorageMessageAttributes*           attributes,
                     const bmqt::MessageGUID&                  msgGUID,
                     const bsl::shared_ptr<bdlbb::Blob>&       appData,
                     const bmqp::Protocol::SubQueueInfosArray& subQueueIds);

    void beforeOneAppRemoved(unsigned int upstreamSubQueueId);

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
    RelayQueueEngine(QueueState*             queueState,
                     const mqbconfm::Domain& domainConfig,
                     bslma::Allocator*       allocator);

    /// Destructor
    ~RelayQueueEngine() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS
    //   (virtual mqbi::QueueEngine)

    /// Configure this instance.  The specified `isReconfigure` flag indicate
    /// if queue is being reconfigured. Return zero on success, non-zero value
    /// otherwise and populate the specified `errorDescription`.
    int configure(bsl::ostream& errorDescription,
                  bool          isReconfigure) BSLS_KEYWORD_OVERRIDE;

    /// Reset the internal state of this engine.  If the optionally specified
    /// 'keepConfirming' is 'true', keep the data structures for CONFIRMs
    /// processing.
    void resetState(bool isShuttingDown = false) BSLS_KEYWORD_OVERRIDE;

    /// Rebuild the internal state of this engine.  This method is invoked
    /// when the queue this engine is associated with is created from an
    /// existing one, and takes ownership of the already created handles
    /// (typically happens when the queue gets converted between local and
    /// remote).  Return zero on success, non-zero value otherwise and
    /// populate the specified `errorDescription`.  Note that
    /// `rebuildInternalState` must be called on an empty-state object
    /// (i.e., which has just been constructed, or following a call to
    /// `resetState`) after it has been configured.
    int
    rebuildInternalState(bsl::ostream& errorDescription) BSLS_KEYWORD_OVERRIDE;

    /// Obtain and return a handle to this queue for the client identified
    /// with the specified `clientContext`, using the specified
    /// `handleParameters`, and invoke the specified `callback` when
    /// finished. In case of error, return a null pointer.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    mqbi::QueueHandle*
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
    void configureHandle(
        mqbi::QueueHandle*                                 handle,
        const bmqp_ctrlmsg::StreamParameters&              streamParameters,
        const mqbi::QueueHandle::HandleConfiguredCallback& configuredCb)
        BSLS_KEYWORD_OVERRIDE;

    /// Reconfigure the specified `handle` by releasing the specified
    /// `parameters` from its current settings and invoke the specified
    /// `releasedCb` when done.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    void
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
    void
    onHandleUsable(mqbi::QueueHandle* handle,
                   unsigned int upstreamSubscriptionId) BSLS_KEYWORD_OVERRIDE;

    /// Called by the mqbi::Queue when a new message is available on the queue
    /// and ready to be sent to eventual interested clients.
    void afterNewMessage() BSLS_KEYWORD_OVERRIDE;

    /// Called by the `mqbi::Queue` when the message identified by the
    /// specified `msgGUID` is confirmed for the specified
    /// `upstreamSubQueueId` stream of the queue on behalf of the client
    /// identified by the specified `handle`.  Return a negative value on
    /// error (GUID was not found, etc.), 0 if this confirm was for the last
    /// reference to that message and it can be deleted from the queue's
    /// associated storage, or 1 if there are still references.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    int
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
    void beforeMessageRemoved(const bmqt::MessageGUID& msgGUID)
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
    void
    afterQueuePurged(const bsl::string&      appId,
                     const mqbu::StorageKey& appKey) BSLS_KEYWORD_OVERRIDE;

    /// Notify this queue engine that a message has been posted and
    /// saved in the storage.
    /// See also: `mqbblp::LocalQueue::postMessage`,
    ///           `mqbblp::RemoteQueue::postMessage`
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    void afterPostMessage() BSLS_KEYWORD_OVERRIDE;

    /// Called after creation of a new storage for the specified
    /// `appIdKeyPair`.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    void registerStorage(const bsl::string&      appId,
                         const mqbu::StorageKey& appKey,
                         unsigned int appOrdinal) BSLS_KEYWORD_OVERRIDE;

    /// Called after removal of the storage for the specified
    /// `appIdKeyPair`.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    void unregisterStorage(const bsl::string&      appId,
                           const mqbu::StorageKey& appKey,
                           unsigned int appOrdinal) BSLS_KEYWORD_OVERRIDE;

    /// Not valid for 'RelayQueueEngine'
    mqbi::StorageResult::Enum evaluateAppSubscriptions(
        const bmqp::PutHeader&              putHeader,
        const bsl::shared_ptr<bdlbb::Blob>& appData,
        const bmqp::MessagePropertiesInfo&  mpi,
        bsls::Types::Uint64                 timestamp) BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    /// If the specified `isOutOfOrder` is `true`, insert the specified
    // `msgGUID` into the corresponding App redelivery list.  Otherwise, insert
    // the `msgGUID` into the PushStream; insert PushStream Elements
    // (`mqbi::AppMessage`, `upstreamSubQueueId`) pairs for each recognized App
    /// in the specified `subscriptions`.

    void push(mqbi::StorageMessageAttributes*     attributes,
              const bmqt::MessageGUID&            msgGUID,
              const bsl::shared_ptr<bdlbb::Blob>& appData,
              bmqp::Protocol::SubQueueInfosArray& subscriptions,
              bool                                isOutOfOrder);
    // ACCESSORS

    /// Return the reference count that should be applied to a message
    /// posted to the queue managed by this engine.  Note that returned
    /// value may or may not be equal to `numOpenReaderHandles()` depending
    /// upon the specific type of this engine.
    unsigned int messageReferenceCount() const BSLS_KEYWORD_OVERRIDE;

    /// Load into the specified `out` object the internal information about
    /// this queue engine and associated queue handles.
    void loadInternals(mqbcmd::QueueEngine* out) const BSLS_KEYWORD_OVERRIDE;

    /// Load upstream subQueue id into the specified `subQueueId` given the
    /// specified upstream `subscriptionId`.
    /// Each subStream has unique Subscription ids.
    bool subscriptionId2upstreamSubQueueId(const bmqt::MessageGUID& msgGUID,
                                           unsigned int*            subQueueId,
                                           unsigned int subscriptionId) const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// --------------------------------
// struct RelayQueueEngine_AppState
// --------------------------------

inline RelayQueueEngine_AppState::RelayQueueEngine_AppState(
    unsigned int                  upstreamSubQueueId,
    const bsl::string&            appId,
    mqbi::Queue*                  queue,
    bdlmt::EventScheduler*        scheduler,
    Routers::QueueRoutingContext& queueContext,
    bslma::Allocator*             allocator)
: QueueEngineUtil_AppState(queue,
                           scheduler,
                           queueContext,
                           upstreamSubQueueId,
                           appId,
                           mqbu::StorageKey::k_NULL_KEY,
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

inline void RelayQueueEngine::ConfigureContext::setStatus(
    const bmqp_ctrlmsg::Status& status)
{
    d_status = status;
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

inline RelayQueueEngine::App_State*
RelayQueueEngine::findApp(unsigned int upstreamSubQueueId) const
{
    AppsMap::const_iterator cit = d_apps.find(upstreamSubQueueId);

    if (cit == d_apps.end()) {
        return 0;
    }
    else {
        return cit->second.get();
    }
}

inline unsigned int RelayQueueEngine::messageReferenceCount() const
{
    // Irrespective of number of worker-consumers or their status (dead or
    // alive), we always return 1
    return 1;
}

}  // close package namespace
}  // close enterprise namespace

#endif
