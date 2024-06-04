// Copyright 2017-2023 Bloomberg Finance L.P.
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

// mqbblp_queueengineutil.h                                           -*-C++-*-
#ifndef INCLUDED_MQBBLP_QUEUEENGINEUTIL
#define INCLUDED_MQBBLP_QUEUEENGINEUTIL

//@PURPOSE: Provide utilities for Queue Engine
//
//@CLASSES:
//  mqbblp::QueueEngineUtil: namespace for Queue Engine utilities
//
//@DESCRIPTION: This component provides a utility 'struct',
// 'mqbblp::QueueEngineUtil', that serves as a namespace for a collection of
// functions used across the different 'mqbblp::QueueEngine'.
//

// MQB

#include <mqbblp_routers.h>
#include <mqbcfg_messages.h>
#include <mqbconfm_messages.h>
#include <mqbi_queue.h>
#include <mqbi_storage.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>

// BDE
#include <ball_log.h>
#include <bdlmt_eventscheduler.h>
#include <bdlmt_throttle.h>
#include <bsl_ostream.h>
#include <bsl_unordered_set.h>
#include <bsl_vector.h>
#include <bslma_allocator.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace bdlbb {
class Blob;
}
namespace bmqp {
class MessageProperties;
}
namespace bsls {
class TimeInterval;
}
namespace mqbblp {
class QueueState;
}
namespace mqbcmd {
class AppState;
}
namespace mqbstat {
class QueueStatsDomain;
}

namespace mqbblp {

// FORWARD DECLARATION
class QueueState;

// ======================
// struct QueueEngineUtil
// ======================

/// This struct provides a collection of functions used across the different
/// `mqbblp::QueueEngine` implementations.
struct QueueEngineUtil {
  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBBLP.QUEUEENGINEUTIL");

  public:
    // CLASS METHODS

    /// Checks if the current number of connected producers and consumers
    /// for the appId in the specified `handleParameters` in the specified
    /// `queueState`, plus the read and write counters in the
    /// `handleParameters` exceeds the predefined limit.  The limit checked
    /// against is the corresponding `maxConsumers` and `maxProducers` as
    /// set in the `Domain` structure.  If any of those limits is set to 0,
    /// that means no limit.  Returns `false` if any of the limits are
    /// exceeded and `true` otherwise.  If limits are exceeded an
    /// appropriate error message will be written into the specified
    /// `errorDescription`.
    static bool consumerAndProducerLimitsAreValid(
        QueueState*                                queueState,
        bsl::ostream&                              errorDescription,
        const bmqp_ctrlmsg::QueueHandleParameters& handleParameters);

    /// Return 0 if the specified `handleParameters` contains a uri that
    /// matches the uri of the queue associated with the specified `handle`,
    /// and a non-zero error code otherwise.  Use the specified
    /// `clientContext` and `handle` to log an error in case of a mismatch.
    static int
    validateUri(const bmqp_ctrlmsg::QueueHandleParameters& handleParameters,
                mqbi::QueueHandle*                         handle,
                const mqbi::QueueHandleRequesterContext&   clientContext =
                    mqbi::QueueHandleRequesterContext());

    /// Report to the specified `domainStats` the queue-time metric of the
    /// message having specified `attributes` and `appId`. Note that this
    /// method must be invoked only at the primary node.
    static void
    reportQueueTimeMetric(mqbstat::QueueStatsDomain*            domainStats,
                          const mqbi::StorageMessageAttributes& attributes,
                          const bsl::string&                    appId);

    /// Return true is the specified `queue` is of the broadcast type.
    static bool isBroadcastMode(const mqbi::Queue* queue);

    /// Calculate the delay (if any) using the specified `rdaInfo` and
    /// load the result into the specified `delay`. Return false if the
    /// delay is zero and true otherwise.
    static bool loadMessageDelay(
        const bmqp::RdaInfo&                 rdaInfo,
        const mqbcfg::MessageThrottleConfig& messageThrottleConfig,
        bsls::TimeInterval*                  delay);

    /// Dump the message contents into a temporary file using the specified
    /// `properties` and `payload`. Load the file path into the specified
    /// `filepath`. Return 0 on a successful write to the temporary file or
    /// a negative integer otherwise.  Note that return value of -1
    /// indicates failure to create a temporary file and return value of -2
    /// indicates failure to open the temporary file.  If `properties` is
    /// `0`, dump the `payload` only.
    static int
    dumpMessageInTempfile(bsl::string*                   filepath,
                          const bdlbb::Blob&             payload,
                          const bmqp::MessageProperties* properties,
                          bdlbb::BlobBufferFactory*      blobBufferFactory);

    /// Dump message contents in temporary file after message has been fully
    /// rejected (with RDA reaching zero). Raise an alarm with the message
    /// info.
    /// THREAD: This method is called from misc threadpool.
    static void
    logRejectMessage(const bmqt::MessageGUID&              msgGUID,
                     const bsl::string&                    appId,
                     unsigned int                          subQueueId,
                     const bsl::shared_ptr<bdlbb::Blob>&   appData,
                     const mqbi::StorageMessageAttributes& attributes,
                     const QueueState*                     queueState,
                     bslma::Allocator*                     allocator);
};

// ===========================================
// struct QueueEngineUtil_ReleaseHandleProctor
// ===========================================

/// Proctor mechanism managing ReleaseHandle callback invocation
struct QueueEngineUtil_ReleaseHandleProctor {
    // Discern the following (partially orthogonal) cases:
    //
    // - no more consumers or producers for all subStream for this handle
    // - no more consumers for this subStream across all handles
    // - no more producers for this subStream across all handles
    // - no more consumers for this subStream for this handle

    // DATA
    QueueState* d_queueState_p;
    // cached raw pointer
    // to the queue state

    bsl::shared_ptr<mqbi::QueueHandle> d_handleSp;
    // keep the handle from
    // destruction

    mqbi::QueueHandleReleaseResult d_result;
    // the outcome of
    // releasing handle

    bool d_isFinal;
    // the case of no
    // handle clients was
    // indicated

    const mqbi::QueueHandle::HandleReleasedCallback d_releasedCb;
    // the callback to
    // execute in dtor

    bool d_disableCallback;
    // do not invoke
    // callback if true

    bsls::AtomicInt d_refCount;

    // CREATORS
    QueueEngineUtil_ReleaseHandleProctor(
        QueueState*                                      queueState,
        bool                                             isFinal,
        const mqbi::QueueHandle::HandleReleasedCallback& releasedCb);

    ~QueueEngineUtil_ReleaseHandleProctor();

    // MANIPULATORS

    /// Decrement readCount and writeCount of the specified `handle` by
    /// readCount and writeCount in the specified `params`.  If after this
    /// operation `handle` ends up no longer representing any resource, it
    /// is removed from the queue internal map.  Return 0 on success, or a
    /// non-zero value on error.
    int releaseHandle(mqbi::QueueHandle*                         handle,
                      const bmqp_ctrlmsg::QueueHandleParameters& params);

    /// Decrement cumulative queue's readCount and writeCount of the stream
    /// identified by the specified `params` by readCount and writeCount in
    /// the `params`.  Return result indicating producer/consumer absence
    /// for the given queue.
    mqbi::QueueHandleReleaseResult
    releaseQueueStream(const bmqp_ctrlmsg::QueueHandleParameters& params);

    /// Decrement handle's readCount and writeCount of the stream identified
    /// by the specified `params` by readCount and writeCount in the
    /// `params`. Decrement cumulative queue's readCount and writeCount of
    /// the stream identified by the specified `params` by readCount and
    /// writeCount in the `params`.  Accumulate producer/consumer absence
    /// indications for the given handle and for the given queue.
    /// Return result for the specified stream.
    mqbi::QueueHandleReleaseResult
    releaseStream(const bmqp_ctrlmsg::QueueHandleParameters& params);

    void addRef();

    void release();

    void invokeCallback();

    // ACCESSORS

    /// Return delta between handle's readCount and writeCount of the stream
    /// identified by the specified `params` and the specified `counts`
    mqbi::QueueCounts countDiff(const bmqp_ctrlmsg::SubQueueIdInfo& info,
                                int                                 readCount,
                                int writeCount) const;

    /// Return result indicating absence of consumers/producers in the
    /// handle, the stream in the queue, the stream in the handle.
    const mqbi::QueueHandleReleaseResult& result() const;
};

// ===============================
// struct QueueEngineUtil_AppState
// ===============================

class RedeliveryList {
  private:
    struct Item {
        unsigned int d_stamp;

        Item();
    };

    typedef mwcc::OrderedHashMap<bmqt::MessageGUID,
                                 Item,
                                 bslh::Hash<bmqt::MessageGUIDHashAlgo> >
        Map;

  public:
    struct iterator {
        Map::iterator d_cit;

        iterator(const Map::iterator& cit);
        const bmqt::MessageGUID& operator*();
    };

  private:
    Map          d_map;
    unsigned int d_stamp;

  private:
    void trim(iterator* cit) const;

  public:
    // PUBLIC CREATORS
    RedeliveryList(bslma::Allocator* allocator);

    // PUBLIC MANIPULATORS

    /// Add the specified `guid` to the list.
    void add(const bmqt::MessageGUID& guid);

    /// Empty the list.
    void clear();

    /// Erase the item referenced by specified `cit` from the list.
    iterator erase(const iterator& cit);

    /// Erase the specified `guid` from the list.
    void erase(const bmqt::MessageGUID& guid);

    /// Load into the specified `cit` an iterator to next enabled (not
    /// disabled) item.  If there are no such items, load the iterator
    /// referencing the end of the list (for which `isEnd` returns `true`).
    void next(iterator* cit) const;

    /// Mark the item referenced by specified `cit` as disabled.
    void disable(iterator* cit) const;

    /// Change the state of the list so it ignores all previous marks
    /// (logically re-enable all items).
    void touch();

    /// Return iterator to the first available (not disabled) item or the
    /// iterator referencing the end of the list (for which `isEnd` returns
    /// `true`).
    iterator begin();

    // PUBLIC ACCESSORS

    /// Return iterator to the first item regardless of its mark (the item
    /// can be disabled) or the iterator referencing the end of the list.
    const bmqt::MessageGUID& first() const;

    /// Return iterator referencing the end of the list.
    bool isEnd(const iterator& cit) const;

    /// Return total number of items in the list including disabled ones.
    size_t size() const;

    /// Return `true` if there are no (including disabled) items.
    bool empty() const;
};

/// Mechanism managing state of a group of consumers supporting priorities.
struct QueueEngineUtil_AppState {
    // PUBLIC TYPES

    /// Set of alive consumers
    typedef Routers::Consumers Consumers;

    // PUBLIC DATA
    bslma::ManagedPtr<mqbi::StorageIterator> d_storageIter_mp;
    // Storage iterator to the logical
    // stream of messages for this fanout
    // consumer

    bsl::shared_ptr<Routers::AppContext> d_routing_sp;
    // Set of alive consumers and their
    // states

    RedeliveryList d_redeliveryList;
    // List of messages that need
    // redelivery, i.e., messages that were
    // sent to a client who went down
    // without confirming them.

    RedeliveryList d_putAsideList;
    // List of messages without matching
    // Subscription

    size_t d_priorityCount;

    mqbi::Queue* d_queue_p;

    bool d_isAuthorized;

    bdlmt::EventScheduler* d_scheduler_p;

    bdlmt::EventSchedulerEventHandle d_throttleEventHandle;
    // EventHandle for poison pill message
    // throttling.
    const mqbu::StorageKey d_appKey;
    const bsl::string      d_appId;

    unsigned int d_upstreamSubQueueId;

    bdlmt::Throttle d_throttledEarlyExits;

    bsls::AtomicBool d_isScheduled;

    mqbconfm::Expression d_subcriptionExpression;
    // The auto subscription expression if any.

    Routers::Expression d_autoSubscription;
    // Evaluator of the auto subscription

    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(QueueEngineUtil_AppState,
                                   bslma::UsesBslmaAllocator)
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBBLP.QUEUEENGINEUTIL_APPSTATE");

    // CREATORS
    QueueEngineUtil_AppState(bslma::ManagedPtr<mqbi::StorageIterator> iterator,
                             mqbi::Queue*                             queue,
                             bdlmt::EventScheduler*        scheduler,
                             bool                          isAuthorized,
                             Routers::QueueRoutingContext& queueContext,
                             unsigned int                  upstreamSubQueueId,
                             const bsl::string&            appId,
                             const mqbu::StorageKey&       appKey,
                             bslma::Allocator*             allocator = 0);

    ~QueueEngineUtil_AppState();

    // MANIPULATORS

    /// Called by Queue Engine before a message with the specified `msgGUID`
    /// is removed.  If the specified `isExpired` is `true`, the message is
    /// TTL expired, otherwise it was confirmed or rejected.
    void beforeMessageRemoved(const bmqt::MessageGUID& msgGUID,
                              bool                     isExpired);

    /// Reset the internal state to have no consumers.
    void reset();

    /// Deliver all messages in the storage to the consumer represented by
    /// this instance.  Load the message delay into the specified `delay`.
    /// Note that depending upon queue's mode, messages are delivered either
    /// to all consumers (broadcast mode), or in a round-robin manner (every
    /// other mode).
    size_t deliverMessages(bsls::TimeInterval*     delay,
                           const mqbu::StorageKey& appKey,
                           mqbi::Storage&          storage,
                           const bsl::string&      appId);

    /// Try to deliver to the next available consumer message having
    /// specified `appData`, `options`, `guid`, `attributes` and `rdaInfo`.
    /// Return true if the message was successfully delivered, or false if
    /// all consumers were busy and no one could handle the message.  The
    /// algorithm will try to deliver to highest priority consumers in a
    /// round-robin manner, respecting their `readCount`.  Behavior is
    /// undefined unless `appData` is non-null.
    Routers::Result tryDeliverOneMessage(bsls::TimeInterval*          delay,
                                         const mqbi::StorageIterator* message,
                                         bool isOutOfOrder);

    /// Broadcast to all available consumers, the message having specified
    /// `appData`, `options`, `guid` and `attributes`.  Behavior is
    /// undefined unless `appData` is non-null.
    void broadcastOneMessage(const mqbi::StorageIterator* storageIter);
    bool visitBroadcast(const mqbi::StorageIterator* message,
                        const Routers::Subscription* subscription);

    size_t processDeliveryLists(bsls::TimeInterval*     delay,
                                const mqbu::StorageKey& appKey,
                                mqbi::Storage&          storage,
                                const bsl::string&      appId);

    /// Process delivery of messages in the redelivery list.  The specified
    /// `getMessageCb` provides message details for redelivery.  Load the
    /// lowest handle delay into the specified `delay`. Return number of
    /// re-delivered messages.
    size_t processDeliveryList(bsls::TimeInterval*     delay,
                               const mqbu::StorageKey& appKey,
                               mqbi::Storage&          storage,
                               const bsl::string&      appId,
                               RedeliveryList&         list);

    /// Load into the specified `out` object' internal information about
    /// this consumers group and associated queue handles.
    void loadInternals(mqbcmd::AppState* out) const;

    /// If there are still consumers, transfer the specified `handle`
    /// unconfirmed messages to internal redelivery list  and return true.
    /// Otherwise, remove `handle` unconfirmed messages, clear internal
    /// redelivery list, and reset internal storage iterator to its
    /// beginning and return false.
    bool
    transferUnconfirmedMessages(mqbi::QueueHandle*                  handle,
                                const bmqp_ctrlmsg::SubQueueIdInfo& subQueue);

    /// Rebuild internal state representing highest priority consumers for
    /// the specified appId.
    void
    rebuildConsumers(const char*                                 appId,
                     bsl::ostream*                               errorStream,
                     const QueueState*                           queueState,
                     const bsl::shared_ptr<Routers::AppContext>& replacement);

    /// Reset the d_timeLastMessageSent attribute of the queueHandleContext
    /// of the specified `handle` and stop message throttling if the
    /// `msgGUID` equals d_lastMessageSent.
    void tryCancelThrottle(mqbi::QueueHandle*       handle,
                           const bmqt::MessageGUID& msgGUID);

    /// Schedule messages to be delivered on this app at the specified
    /// `executionTime` using the specified `deliverMessageFn`.
    void scheduleThrottle(bsls::TimeInterval           executionTime,
                          const bsl::function<void()>& deliverMessageFn);

    Consumers& consumers();

    Routers::Consumers::SharedItem find(mqbi::QueueHandle* handle);

    void
    executeInQueueDispatcher(const bsl::function<void()>& deliverMessageFn);

    /// Cancel scheduled message delivery (if any) on this app.
    void cancelThrottle();

    void setUpstreamSubQueueId(unsigned int value);

    void invalidate(mqbi::QueueHandle* handle);

    Routers::Result
    selectConsumer(const Routers::Visitor&      visitor,
                   const mqbi::StorageIterator* currentMessage);

    // Set the auto subscription
    int setSubscription(const mqbconfm::Expression& value);

    // Evaluate the auto subscription
    bool evaluateAutoSubcription();

    // ACCESSORS
    size_t redeliveryListSize() const;

    Routers::Consumer* findQueueHandleContext(mqbi::QueueHandle* handle);

    unsigned int upstreamSubQueueId() const;

    bool hasConsumers() const;

    /// Returns storage iterator to the 1st un-delivered message including
    /// `put-aside` messages (those without matching Subscriptions).
    bslma::ManagedPtr<mqbi::StorageIterator> head() const;
};

// ==========================================
// struct QueueEngineUtil_AppsDeliveryContext
// ==========================================

/// Mechanism to cache handle to SubQueueInfosArray map for a given message.
struct QueueEngineUtil_AppsDeliveryContext {
    typedef bsl::unordered_map<mqbi::QueueHandle*,
                               bmqp::Protocol::SubQueueInfosArray>
        Consumers;

    Consumers                      d_consumers;
    bool                           d_doRepeat;
    mqbi::StorageIterator*         d_currentMessage;
    mqbi::Queue*                   d_queue_p;
    bsl::vector<bslstl::StringRef> d_activeAppIds;

    QueueEngineUtil_AppsDeliveryContext(mqbi::Queue*      queue,
                                        bslma::Allocator* allocator);

    /// Prepare the context to pick up and deliver next message.
    void reset();

    /// Return true if the specified `app` has next available handle to
    /// deliver the current message and prepare for `deliverMessage` call.
    bool processApp(QueueEngineUtil_AppState& app);
    bool visit(const Routers::Subscription* subscription,
               const mqbi::StorageIterator* message);
    bool visitBroadcast(const Routers::Subscription* subscription);

    /// Deliver message to the previously processed handles.
    void deliverMessage();
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ----------------------
// struct QueueEngineUtil
// ----------------------

// static
inline bool QueueEngineUtil::isBroadcastMode(const mqbi::Queue* queue)
{
    return queue->isDeliverAll() && queue->isAtMostOnce();
}

// -----------------------------------------------
// struct QueueEngineUtil_AppState::RedeliveryList
// -----------------------------------------------

inline RedeliveryList::Item::Item()
: d_stamp(0)
{
    // NOTHING
}

inline RedeliveryList::iterator::iterator(const Map::iterator& cit)
: d_cit(cit)
{
    // NOTHING
}

inline const bmqt::MessageGUID& RedeliveryList::iterator::operator*()
{
    return d_cit->first;
}

inline RedeliveryList::RedeliveryList(bslma::Allocator* allocator)
: d_map(allocator)
, d_stamp(1)
{
    // NOTHING
}

inline void RedeliveryList::add(const bmqt::MessageGUID& guid)
{
    d_map.insert(bsl::make_pair(guid, Item()));
}

inline void RedeliveryList::clear()
{
    d_map.clear();
}

inline RedeliveryList::iterator RedeliveryList::erase(const iterator& cit)
{
    iterator result(d_map.erase(cit.d_cit));

    trim(&result);

    return result;
}

inline void RedeliveryList::erase(const bmqt::MessageGUID& guid)
{
    d_map.erase(guid);
}

inline RedeliveryList::iterator RedeliveryList::begin()
{
    iterator result(d_map.begin());

    trim(&result);

    return result;
}

inline const bmqt::MessageGUID& RedeliveryList::first() const
{
    return d_map.begin()->first;
}

inline void RedeliveryList::next(iterator* cit) const
{
    ++cit->d_cit;
    trim(cit);
}

inline void RedeliveryList::disable(iterator* cit) const
{
    cit->d_cit->second.d_stamp = d_stamp;
}

inline void RedeliveryList::touch()
{
    if (++d_stamp == 0) {
        d_stamp = 1;
    }
}

inline bool RedeliveryList::isEnd(const iterator& cit) const
{
    return cit.d_cit == d_map.end();
}

inline size_t RedeliveryList::size() const
{
    return d_map.size();
}

inline bool RedeliveryList::empty() const
{
    return d_map.empty();
}

inline void RedeliveryList::trim(iterator* cit) const
{
    // Skip those which did not have subscription last time (after config)
    while (!isEnd(*cit)) {
        if (cit->d_cit->second.d_stamp != d_stamp) {
            // 'Item::d_stamp' is not valid anymore
            // This assumes that before 'RedeliveryList::d_stamp' wraps around
            // the 'cit->d_cit->second.d_stamp', this code resets the latter.
            cit->d_cit->second.d_stamp = 0;
            break;
        }
        ++cit->d_cit;
    }
}

// -------------------------------
// struct QueueEngineUtil_AppState
// -------------------------------

inline size_t QueueEngineUtil_AppState::redeliveryListSize() const
{
    return d_redeliveryList.size();
}

inline Routers::Consumer*
QueueEngineUtil_AppState::findQueueHandleContext(mqbi::QueueHandle* handle)
{
    Consumers::SharedItem it = find(handle);
    if (!it) {
        return 0;  // RETURN
    }
    return &it->value();
}

inline Routers::Consumers& QueueEngineUtil_AppState::consumers()
{
    BSLS_ASSERT_SAFE(d_routing_sp);

    return d_routing_sp->d_consumers;
}

inline Routers::Consumers::SharedItem
QueueEngineUtil_AppState::find(mqbi::QueueHandle* handle)
{
    return d_routing_sp->d_consumers.find(handle);
}

inline bool QueueEngineUtil_AppState::hasConsumers() const
{
    return d_routing_sp ? !d_routing_sp->d_consumers.empty() : false;
}

inline unsigned int QueueEngineUtil_AppState::upstreamSubQueueId() const
{
    return d_upstreamSubQueueId;
}

inline void QueueEngineUtil_AppState::setUpstreamSubQueueId(unsigned int value)
{
    d_upstreamSubQueueId = value;
}

inline void QueueEngineUtil_AppState::invalidate(mqbi::QueueHandle* handle)
{
    Routers::Consumers::SharedItem itConsumer = find(handle);

    if (itConsumer) {
        itConsumer->invalidate();

        d_priorityCount = d_routing_sp->finalize();

        d_routing_sp->registerSubscriptions();
    }
}

// ------------------------------------------
// class QueueEngineUtil_ReleaseHandleProctor
// ------------------------------------------

inline const mqbi::QueueHandleReleaseResult&
QueueEngineUtil_ReleaseHandleProctor::result() const
{
    return d_result;
}

}  // close namespace mqbblp
}  // close namespace BloombergLP

#endif
