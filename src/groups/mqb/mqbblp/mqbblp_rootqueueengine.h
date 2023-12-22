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

// mqbblp_rootqueueengine.h                                           -*-C++-*-
#ifndef INCLUDED_MQBBLP_ROOTQUEUEENGINE
#define INCLUDED_MQBBLP_ROOTQUEUEENGINE

/// @file mqbblp_rootqueueengine.h
///
/// @brief Provide a `QueueEngine` for use at the primary node.
///
/// @bbref{mqbblp::RootQueueEngine} provides an @bbref{mqbi::QueueEngine}
/// implementation for use at the primary node.

// MQB
#include <mqbblp_queueconsumptionmonitor.h>
#include <mqbblp_queueengineutil.h>
#include <mqbconfm_messages.h>
#include <mqbi_dispatcher.h>
#include <mqbi_queue.h>
#include <mqbi_queueengine.h>
#include <mqbi_storage.h>
#include <mqbs_virtualstorage.h>
#include <mqbu_storagekey.h>

// BMQ
#include <bmqc_twokeyhashmap.h>
#include <bmqp_ctrlmsg_messages.h>
#include <bmqt_messageguid.h>

// BDE
#include <ball_log.h>
#include <bdlmt_throttle.h>
#include <bsl_list.h>
#include <bsl_memory.h>
#include <bsl_ostream.h>
#include <bsl_string.h>
#include <bsl_unordered_map.h>
#include <bsl_utility.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_keyword.h>

#include <bslstl_stringref.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace bdlmt {
class EventScheduler;
}
namespace mqbcmd {
class QueueEngine;
}

namespace mqbblp {

// FORWARD DECLARATION
class QueueState;

// =======================
// class RootQueueEngine
// =======================

/// QueueEngine implementation for use at the primary node.
class RootQueueEngine BSLS_KEYWORD_FINAL : public mqbi::QueueEngine {
  private:
    // PRIVATE TYPES
    typedef QueueEngineUtil_AppState AppState;

    /// instead of bsl::unique_ptr
    typedef bsl::shared_ptr<AppState> AppStateSp;

    /// appId -> AppStateSp
    typedef bsl::unordered_map<bsl::string, AppStateSp> Apps;

  private:
    // DATA

    QueueState* d_queueState_p;

    QueueConsumptionMonitor d_consumptionMonitor;

    /// Map of appId to AppState
    Apps d_apps;

    /// Does this queue engine have any application subscriptions configured?
    bool d_hasAppSubscriptions;

    const bool d_isFanout;

    /// Event scheduler currently used for message throttling and consumption
    /// monitor.  Held, not owned.
    bdlmt::EventScheduler* d_scheduler_p;

    /// Thread pool for any standalone work that can be offloaded to
    /// non-queue-dispatcher threads.  It is used to hex dump the payload of a
    /// rejected message.
    bdlmt::FixedThreadPool* d_miscWorkThreadPool_p;

    /// Throttler for REJECTs.
    bdlmt::Throttle d_throttledRejectedMessages;

    /// Throttler for when reject messages are dumped into temp files.
    bdlmt::Throttle d_throttledRejectMessageDump;

    /// Reusable apps delivery context.
    QueueEngineUtil_AppsDeliveryContext d_appsDeliveryContext;

    /// Storage iterator to the logical stream of messages.  Queue Engine
    /// iterates this one sequentially.
    bslma::ManagedPtr<mqbi::StorageIterator> d_storageIter_mp;

    /// Storage iterator to access storage state.  Queue Engine uses this one
    /// to access random message (as in the case of redelivery).
    bslma::ManagedPtr<mqbi::StorageIterator> d_realStorageIter_mp;

    /// Allocator to use.
    bslma::Allocator* d_allocator_p;

  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBBLP.ROOTQUEUEENGINE");

    // NOT IMPLEMENTED
    RootQueueEngine(const RootQueueEngine&) BSLS_KEYWORD_DELETED;
    RootQueueEngine& operator=(const RootQueueEngine&) BSLS_KEYWORD_DELETED;

    // PRIVATE MANIPULATORS

    /// Attempt to deliver outstanding messages, if any, to the consumers
    /// of the Fanout appId corresponding to the specified `app`.   If at least
    /// one message has been delivered, update `d_consumptionMonitor` for the
    /// key of the 'app'.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    void deliverMessages(AppState* app);

    // PRIVATE ACCESSORS

    /// Set up data structures for the specified `appId`.  Return 0 on
    /// success otherwise load errors into the specified `errorDescription`
    /// and return on-zero.
    ///
    /// THREAD: This method is called from any thread.
    int initializeAppId(const bsl::string& appId,
                        bsl::ostream&      errorDescription,
                        unsigned int       upstreamSubQueueId,
                        bool               isReconfigure);

    /// Return true if the specified `handle` is registered for the
    /// specified `appId`.  Return false otherwise.
    bool hasHandle(const bsl::string& appId, mqbi::QueueHandle* handle) const;

    /// Insert the specified `streamParameters` of the specified `handle`
    /// into consumers map corresponding to the appId in the
    /// `streamParameters` if the `streamParameters` have highest priority
    /// and the specified `itApp` references the same appId.
    void rebuildSelectedApp(mqbi::QueueHandle*                   handle,
                            const mqbi::QueueHandle::StreamInfo& info,
                            const Apps::iterator&                itApp,
                            const Routers::AppContext*           previous);

    Apps::iterator makeSubStream(const bsl::string&      appId,
                                 const mqbu::StorageKey& appKey,
                                 unsigned int            upstreamSubQueueId);

    bool validate(unsigned int upstreamSubQueueId) const;

    const AppStateSp& subQueue(unsigned int upstreamSubQueueId) const;

    /// Callback called by `d_consumptionMonitor` to check un-delivered
    /// messages. Return managed pointer with iterator pointing to the oldest
    /// un-delivered message if there are un-delivered messages for the
    /// specified `appId`, or empty managed pointer otherwise.  If there are
    /// un-delivered messages for the specified `appId` it sets in the
    /// specified `alarmTime_p` calculated alarm time for the oldest
    /// un-delivered message.
    bslma::ManagedPtr<mqbi::StorageIterator>
    haveUndeliveredCb(bsls::TimeInterval*       alarmTime_p,
                      const bsl::string&        appId,
                      const bsls::TimeInterval& now) const;

    /// Callback called by `d_consumptionMonitor` to log the alarm for the
    /// specified `appId` and `oldestMsgIt`.
    void logAlarmCb(
        const bsl::string&                              appId,
        const bslma::ManagedPtr<mqbi::StorageIterator>& oldestMsgIt) const;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(RootQueueEngine, bslma::UsesBslmaAllocator)

  public:
    // CLASS METHODS

    /// Routine executed after `getHandle` method on this queue engine
    /// instance has been invoked, where the specified `ptr` is a pointer to
    /// the queue engine instance, and the specified `cookie` is a pointer
    /// to a boolean flag indicating whether the handle was created or not.
    static void onHandleCreation(void* ptr, void* cookie);

    /// Loads the specified `queueEngine` with a new `RootQueueEngine`
    /// initialized using the specified `queueState`, `domainConfig`,
    /// `scheduler` and `allocator`.
    static void create(bslma::ManagedPtr<mqbi::QueueEngine>* queueEngine,
                       QueueState*                           queueState,
                       const mqbconfm::Domain&               domainConfig,
                       bslma::Allocator*                     allocator);

    /// Loads the specified `config` with the appropriate values for
    /// fanout delivery mode.
    struct FanoutConfiguration {
        static void
        loadRoutingConfiguration(bmqp_ctrlmsg::RoutingConfiguration* config);
    };

    /// Loads the specified `config` with the appropriate values for
    /// round robin priority delivery mode.
    struct PriorityConfiguration {
        static void
        loadRoutingConfiguration(bmqp_ctrlmsg::RoutingConfiguration* config);
    };

    /// Loads the specified `config` with the appropriate values for
    /// broadcast delivery mode.
    struct BroadcastConfiguration {
        static void
        loadRoutingConfiguration(bmqp_ctrlmsg::RoutingConfiguration* config);
    };

    // CREATORS
    RootQueueEngine(QueueState*             queueState,
                    const mqbconfm::Domain& domainConfig,
                    bslma::Allocator*       allocator);

    // MANIPULATORS
    //   (virtual mqbi::QueueEngine)

    /// Configure this instance.  The specified `isReconfigure` flag indicates
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
    getHandle(const mqbi::OpenQueueConfirmationCookie& context,
              const bsl::shared_ptr<mqbi::QueueHandleRequesterContext>&
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

    /// Called by the mqbi::Queue when a new message with the specified
    /// `msgGUID` is available on the queue and ready to be sent to eventual
    /// interested clients.  If available, the specified `source` points to
    /// the originator of the message.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    void afterNewMessage(const bmqt::MessageGUID& msgGUID,
                         mqbi::QueueHandle* source) BSLS_KEYWORD_OVERRIDE;

    /// Called by the `mqbi::Queue` when the message identified by the
    /// specified `msgGUID` is confirmed for the specified `subQueueId`
    /// stream of the queue on behalf of the client identified by the
    /// specified `handle`.  Return a negative value on error (GUID was not
    /// found, etc.), 0 if this confirm was for the last reference to that
    /// message and it can be deleted from the queue's associated storage,
    /// or 1 if there are still references.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    int onConfirmMessage(mqbi::QueueHandle*       handle,
                         const bmqt::MessageGUID& msgGUID,
                         unsigned int subQueueId) BSLS_KEYWORD_OVERRIDE;

    /// Called by the `mqbi::Queue` when the message identified by the
    /// specified `msgGUID` is rejected for the specified
    /// `downstreamSubQueueId` stream of the queue on behalf of the client
    /// identified by the specified `handle`.  Return resulting RDA counter.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    int
    onRejectMessage(mqbi::QueueHandle*       handle,
                    const bmqt::MessageGUID& msgGUID,
                    unsigned int downstreamSubQueueId) BSLS_KEYWORD_OVERRIDE;

    /// Called by the mqbi::Queue before a message with the specified
    /// `msgGUID` is removed from the queue (either it's TTL expired, it was
    /// confirmed by all recipients, etc). The QueueEngine may use this to
    /// update the positions of the QueueHandles it manages.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
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

    /// Called after the specified `addedAppIds` have been dynamically
    /// registered.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    void afterAppIdRegistered(const mqbi::Storage::AppInfos& addedAppIds)
        BSLS_KEYWORD_OVERRIDE;

    /// Called after the specified `removedAppIds` have been dynamically
    /// unregistered.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    void afterAppIdUnregistered(const mqbi::Storage::AppInfos& removedAppIds)
        BSLS_KEYWORD_OVERRIDE;

    /// Called after creation of a new storage for the  specified
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

    /// Given the specified 'putHeader', 'appData', 'mpi', and 'timestamp',
    /// evaluate all application subscriptions and exclude applications with
    /// negative results from message delivery.  Return 0 on success or an
    /// non-zero error code on failure.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    mqbi::StorageResult::Enum evaluateAppSubscriptions(
        const bmqp::PutHeader&              putHeader,
        const bsl::shared_ptr<bdlbb::Blob>& appData,
        const bmqp::MessagePropertiesInfo&  mpi,
        bsls::Types::Uint64                 timestamp) BSLS_KEYWORD_OVERRIDE;

    /// Return storage iterator to the 1st un-delivered message including
    /// 'put-aside' messages (those without matching Subscriptions).
    bslma::ManagedPtr<mqbi::StorageIterator> head(const AppStateSp app) const;

    // ACCESSORS
    //   (virtual mqbi::QueueEngine)

    /// Return the reference count that should be applied to a message
    /// posted to the queue managed by this engine.  Note that returned
    /// value may or may not be equal to `numOpenReaderHandles()` depending
    /// upon the specific type of this engine.
    unsigned int messageReferenceCount() const BSLS_KEYWORD_OVERRIDE;

    /// Load into the specified `out` object the internal information about
    /// this queue engine and associated queue handles.
    ///
    /// THREAD: This method is called from the Queue's dispatcher thread.
    void loadInternals(mqbcmd::QueueEngine* out) const BSLS_KEYWORD_OVERRIDE;

    /// Log application subscription info for the specified `appId` into the
    /// specified `stream`.
    ///
    /// THREAD: This method is called from the Queue's
    /// dispatcher thread.
    bsl::ostream& logAppSubscriptionInfo(bsl::ostream&      stream,
                                         const bsl::string& appId) const
        BSLS_KEYWORD_OVERRIDE;

  private:
    /// Log application subscription info for the specified `appState` into
    /// the specified `stream`.
    bsl::ostream& logAppSubscriptionInfo(bsl::ostream&     stream,
                                         const AppStateSp& appState) const;

    const mqbconfm::Domain& config() const;
};

// ============================================================================
//                           INLINE DEFINITIONS
// ============================================================================

// ----------------------
// struct RootQueueEngine
// ----------------------

inline const mqbconfm::Domain& RootQueueEngine::config() const
{
    return d_queueState_p->queue()->domain()->config();
}

}  // close package namespace
}  // close enterprise namespace

#endif
