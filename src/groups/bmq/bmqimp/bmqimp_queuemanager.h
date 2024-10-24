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

// bmqimp_queuemanager.h                                              -*-C++-*-
#ifndef INCLUDED_BMQIMP_QUEUEMANAGER
#define INCLUDED_BMQIMP_QUEUEMANAGER

//@PURPOSE: Provide a mechanism to manage queues and interact with BlazingMQ
// broker.
//
//@CLASSES:
//  bmqimp::QueueManager: Mechanism to manage queues and interact with broker
//  bmqimp::QueueManager_QueueInfo: Type containing queue information (by URI)
//
//@DESCRIPTION: This component defines a mechanism, 'bmqimp::QueueManager', to
// simplify queue management and interactions with the BlazingMQ broker.  It
// also defines 'bmqimp::QueueManager_QueueInfo', a type containing information
// used for communicating with the BlazingMQ broker with respect to a canonical
// URI.
//
/// Active and Expired Queues
//--------------------------
// An 'active' queue is a queue that is currently in active use by the
// application (e.g. can be reconfigured, can be posted to, etc.).  An
// 'expired' queue is a queue that is no longer in active use by the
// application, and is pending a response from the upstream BlazingMQ broker
// (for example, a queue that failed to open due to a local timeout during
// 'openQueue', but it is still pending for a response to arrive from the
// upstream BlazingMQ broker -- for the purpose of rolling back the original
// request that might have succeeded upstream).
//
// An 'active' queue is inserted when an 'openQueue' request was successfully
// sent upstream.  It is removed from the active queues managed by this object
// when processing a failure response from upstream for either the 1st
// (openQueue) or 2nd (configureQueue) stages of 'openQueue', or when the queue
// is fully closed.
//
// A queue becomes 'expired' when processing a failure due to local timeout
// either in the 1st (openQueue) or 2nd (configureQueue) stages of 'openQueue'
// or when processing a failure due to local timeout in the 1st stage
// (configureQueue) or the 2nd stage (closeQueue) of 'closeQueue'.  It is
// removed from the queues container managed by this object when processing a
// (late) response to a locally timed out request.
//
// The 'expired' queues are not removed from the container immediately because
// we want to prevent the user from opening a queue with the same uri and we
// need to lookup the queue when we eventually receive a (late) response from
// upstream.
//
/// Thread Safety
///-------------
// Thread safe.

// BMQ

#include <bmqimp_event.h>
#include <bmqimp_queue.h>
#include <bmqp_eventutil.h>
#include <bmqp_pushmessageiterator.h>
#include <bmqp_putmessageiterator.h>
#include <bmqp_queueid.h>
#include <bmqt_correlationid.h>
#include <bmqt_uri.h>

#include <bmqc_twokeyhashmap.h>

// BDE
#include <bsl_memory.h>
#include <bsl_string.h>
#include <bsl_unordered_map.h>
#include <bsl_vector.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_atomic.h>
#include <bsls_cpp11.h>
#include <bsls_spinlock.h>
#include <bsls_types.h>

namespace BloombergLP {

namespace bmqimp {

// =============================
// struct QueueManager_QueueInfo
// =============================

/// Type containing information used for communicating with the BlazingMQ
/// broker with respect to a canonical URI.
struct QueueManager_QueueInfo {
  public:
    // TYPES

    /// Map of appId to associated subQueueId
    typedef bsl::unordered_map<bsl::string, unsigned int> SubQueueIdsMap;

    // PUBLIC DATA
    SubQueueIdsMap d_subQueueIdsMap;  // Map of appId to associated
                                      // subQueueId

    int d_queueId;  // The unique queueId associated with
                    // the canonical URI corresponding to
                    // this object

    unsigned int d_nextSubQueueId;  // The next unique subQueueId
                                    // associated with the canonical URI
                                    // corresponding to this object and an
                                    // appId

    unsigned int d_subStreamCount;  // The number of subStreams associated
                                    // with the canonical URI corresponding
                                    // to this object

    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(QueueManager_QueueInfo,
                                   bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a `bmqimp::QueueManager_QueueInfo` object having the
    /// specified `queueId`.  All memory allocations will be done using the
    /// specified `allocator`.
    QueueManager_QueueInfo(int queueId, bslma::Allocator* allocator);

    /// Copy constructor from the specified `original` using the specified
    /// `allocator`.
    QueueManager_QueueInfo(const QueueManager_QueueInfo& original,
                           bslma::Allocator*             allocator);
};

// ==================
// class QueueManager
// ==================

/// Mechanism to manage queues and their interactions with the BlazingMQ
/// broker.
class QueueManager {
  public:
    // TYPES

    typedef bsl::shared_ptr<Queue>                       QueueSp;
    typedef bsl::pair<unsigned int, bmqt::CorrelationId> SubscriptionUandle;

    struct QueueBySubscription {
        const QueueSp d_queue;

        const SubscriptionUandle d_subscriptionHandle;
        // The Subscription correlationId as
        // specified in the configure request.

        QueueBySubscription(unsigned int internalSubscriptionId,
                            const bsl::shared_ptr<Queue>& queue);
    };

    /// Subscription id -> {Queue, Subscription correlationId}
    typedef bsl::unordered_map<SubscriptionId, QueueBySubscription>
        QueuesBySubscriptions;

    typedef bsl::vector<bmqp::EventUtilEventInfo> EventInfos;

  private:
    // PRIVATE TYPES

    /// ((queueId, subQueueId), correlationId) -> queueSp
    typedef bmqc::TwoKeyHashMap<bmqp::QueueId, bmqt::CorrelationId, QueueSp>
        QueuesMap;

    /// canonicalUri -> queueInfo
    typedef bsl::unordered_map<bsl::string, QueueManager_QueueInfo> UrisMap;

  public:
    // CLASS DATA

    /// Initial SubQueueId.  This is the first value used for correctly
    /// generating an increasing sequence of non-default `subQueueId`
    /// integers for the purpose of identifying a logical stream when
    /// communicating with upstream.  Hence, it is assumed that this value
    /// is greater than the default subQueueId.
    static const unsigned int k_INITIAL_SUBQUEUE_ID = 1;

  private:
    // DATA
    mutable bsls::SpinLock d_queuesLock;
    // Lock for 'd_queues', 'd_uris', and
    // 'd_expiredQueues'

    QueuesMap d_queues;  // Map of queues in active use

    UrisMap d_uris;  // Map of canonical URIs to queueInfo
                     // for queues in active use

    bsls::AtomicInt d_nextQueueId;
    // Next id for a new queue.  This value
    // is always incremented and never
    // decremented.

    QueuesBySubscriptions d_queuesBySubscriptionIds;
    // When generated for the new configure
    // request, Subscription id is globally
    // unique.  But the old style request
    // uses subQueue id for backward-
    // compatibility, and that is not
    // unique.  Therefore, have to use
    // both Queue id and Subscription/
    // SubQueue id,

    bslma::Allocator* d_allocator_p;
    // Allocator to use

  private:
    // NOT IMPLEMENTED
    QueueManager(const QueueManager&) BSLS_CPP11_DELETED;
    QueueManager& operator=(const QueueManager&) BSLS_CPP11_DELETED;

    // PRIVATE MANIPULATORS

    /// Return the next queueId.  This method always returns a value greater
    /// than or equal to 0 and never returns the same value twice.  Note
    /// that this method is useful when setting the queueId for a new queue.
    int generateNextQueueId();

    // PRIVATE ACCESSORS

    /// Lookup the queue with the specified `queueId` and return a shared
    /// pointer to the Queue object (if found), or an empty shared pointer
    /// (if not found).  The behaviour is undefined unless the
    /// `d_queuesLock` is locked.
    QueueSp lookupQueueLocked(const bmqp::QueueId& queueId) const;

    /// Lookup queue by the specified queue id `qid` and the specified
    /// subscription id `sid`.  Return `shared_ptr` to the queue if
    /// successful or empty `shared_ptr` if the queue is not found.  Upon
    /// successful lookup Load the Subscription correlationId into the
    /// specified `correlationId`.
    QueueSp
    lookupQueueBySubscriptionIdLocked(bmqt::CorrelationId* correlationId,
                                      unsigned int* subscriptionHandleId,
                                      int           qid,
                                      unsigned int  innerSubscriptionId) const;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(QueueManager, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a `bmqimp::QueueManager` object.  All memory allocations will
    /// be done using the specified `allocator`.
    explicit QueueManager(bslma::Allocator* allocator);

    /// Destroy this object.
    ~QueueManager();

    // MANIPULATORS

    /// Insert the specified `queue` to the queues in active use managed by
    /// this object.  The behavior is undefined unless the queueId of
    /// `queue` is a valid queueId (i.e. obtained through a call to
    /// `generateQueueAndSubQueueId`), or if `queue` was previously inserted
    /// using this method.
    void insertQueue(const QueueSp& queue);

    /// Remove the specified `queue` from the queues in active use managed
    /// by this object and return a shared pointer to the removed queue.
    /// Return an empty shared pointer to a queue if the queue is not found
    /// per its queueId and subQueueId pair.  The behavior is undefined
    /// unless the `queue` was previously inserted using `insertQueue`.
    QueueSp removeQueue(const Queue* queue);

    /// Load into the specified `queueId` a unique pair of queueId and
    /// subQueueId using the specified `uri` and `flags`.  If the canonical
    /// URI of `uri` is found, then the corresponding queueId is reused and
    /// a new subQueueId is generated; otherwise, a new queueId is
    /// generated, and subQueueId begins at the initial subQueueId.  The
    /// subQueueId of a writer (as determined by `flags`) or a non-fanout
    /// consumer (as determined by the id of `uri`) is always the default
    /// subQueueId.  The behavior is undefined unless `uri` represents a
    /// valid URI.
    void generateQueueAndSubQueueId(bmqp::QueueId*      queueId,
                                    const bmqt::Uri&    uri,
                                    bsls::Types::Uint64 flags);

    /// Reset the state of this object.
    void resetState();

    /// Update stats for the queue(s) corresponding to the messages pointed
    /// to by the specified `iterator`, populate the specified
    /// `messageCount` with the `flattened` number of messages (number of
    /// messages in the event if every message having more than one
    /// SubQueueId were to appear once for every SubQueueId), and set the
    /// specified `hasMessageWithMultipleSubQueueIds` to true if there is at
    /// least one message having more than one SubQueueId (in the options).
    /// Return 0 on success, and non-zero on error.  The behavior is
    /// undefined unless `iterator` is valid.
    int onPushEvent(QueueManager::EventInfos* eventInfos,
                    int*                      messageCount,
                    bool* hasMessageWithMultipleSubQueueIds,
                    const bmqp::PushMessageIterator& iterator);

    const QueueSp observePushEvent(bmqt::CorrelationId* correlationId,
                                   unsigned int*        subscriptionHandleId,
                                   const bmqp::EventUtilQueueInfo& info);

    /// Update stats for the queue(s) corresponding to the messages pointed
    /// to by the specified `iterator` and populate the specified
    /// `messageCount` with the number of messages iterated.  Return 0 on
    /// success, and non-zero on error.  The behavior is undefined unless
    /// `iterator` is valid.
    int updateStatsOnPutEvent(int*                            messageCount,
                              const bmqp::PutMessageIterator& iterator);

    /// Increment the count of active subStreams associated with the
    /// specified `canonicalUri`.  The behavior is undefined unless there is
    /// at least one active queue having `canonicalUri` that is in use by
    /// this object.
    void incrementSubStreamCount(const bsl::string& canonicalUri);

    /// Decrement the count of active subStreams associated with the
    /// specified `canonicalUri`.  The behavior is undefined unless there is
    /// at least one active queue having `canonicalUri` that is in use by
    /// this object.
    void decrementSubStreamCount(const bsl::string& canonicalUri);

    /// Reset the count of active subStreams associated with the specified
    /// `canonicalUri`.  The behavior is undefined unless there is at least
    /// one active queue having `canonicalUri` that is in use by this
    /// object.
    void resetSubStreamCount(const bsl::string& canonicalUri);

    void updateSubscriptions(const bsl::shared_ptr<Queue>&         queue,
                             const bmqp_ctrlmsg::StreamParameters& config);

    // ACCESSORS
    QueueSp lookupQueue(const bmqt::Uri& uri) const;
    QueueSp lookupQueue(const bmqt::CorrelationId& correlationId) const;
    QueueSp lookupQueue(const bmqp::QueueId& queueId) const;

    /// Lookup the queue with the specified `queueId`, or `uri`, or
    /// `correlationId`, and return a shared pointer to the Queue object (if
    /// found), or an empty shared pointer (if not found).
    QueueSp lookupQueueBySubscriptionId(bmqt::CorrelationId* correlationId,
                                        unsigned int* subscriptionHandleId,
                                        int           queueId,
                                        unsigned int  subscriptionId) const;

    // TBD: Temporary method to enable refactoring of 'BrokerSession'.
    //      Specifically, reopen logic in 'BrokerSession::processPacket'.

    /// Populate the specified `queues` with queues having the specified
    /// `state`.
    void lookupQueuesByState(bsl::vector<QueueSp>* queues,
                             QueueState::Enum      state) const;

    /// Populate the specified `queues` with all existing queues.
    void getAllQueues(bsl::vector<QueueSp>* queues) const;

    /// Return the number of active subStreams associated with the specified
    /// `canonicalUri`.
    unsigned int subStreamCount(const bsl::string& canonicalUri) const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ----------------------------------------
// struct QueueManager::QueueBySubscription
// ----------------------------------------

inline QueueManager::QueueBySubscription::QueueBySubscription(
    unsigned int                  internalSubscriptionId,
    const bsl::shared_ptr<Queue>& queue)
: d_queue(queue)
, d_subscriptionHandle(
      queue->extractSubscriptionHandle(internalSubscriptionId))
{
    // NOTHING
}

// -----------------------------
// struct QueueManager_QueueInfo
// -----------------------------

// CREATORS
inline QueueManager_QueueInfo::QueueManager_QueueInfo(
    int               queueId,
    bslma::Allocator* allocator)
: d_subQueueIdsMap(allocator)
, d_queueId(queueId)
, d_nextSubQueueId(QueueManager::k_INITIAL_SUBQUEUE_ID)
, d_subStreamCount(0)
{
    // NOTHING
}

inline QueueManager_QueueInfo::QueueManager_QueueInfo(
    const QueueManager_QueueInfo& original,
    bslma::Allocator*             allocator)
: d_subQueueIdsMap(original.d_subQueueIdsMap, allocator)
, d_queueId(original.d_queueId)
, d_nextSubQueueId(original.d_nextSubQueueId)
, d_subStreamCount(original.d_subStreamCount)
{
    // NOTHING
}

// ------------------
// class QueueManager
// ------------------

// PRIVATE MANIPULATORS
inline int QueueManager::generateNextQueueId()
{
    return d_nextQueueId++;
}

// MANIPULATORS
inline QueueManager::QueueSp
QueueManager::lookupQueue(const bmqp::QueueId& queueId) const
{
    bsls::SpinLockGuard guard(&d_queuesLock);  // d_queuesLock LOCKED

    return lookupQueueLocked(queueId);
}

inline QueueManager::QueueSp QueueManager::lookupQueueBySubscriptionId(
    bmqt::CorrelationId* correlationId,
    unsigned int*        subscriptionHandleId,
    int                  queueId,
    unsigned int         internalSubscriptionId) const
{
    bsls::SpinLockGuard guard(&d_queuesLock);  // d_queuesLock LOCKED

    return lookupQueueBySubscriptionIdLocked(correlationId,
                                             subscriptionHandleId,
                                             queueId,
                                             internalSubscriptionId);
}

}  // close package namespace
}  // close enterprise namespace

#endif
