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

// bmqimp_queuemanager.cpp                                            -*-C++-*-
#include <bmqimp_queuemanager.h>

#include <bmqscm_version.h>
// BMQ
#include <bmqp_optionsview.h>
#include <bmqp_protocol.h>
#include <bmqt_queueflags.h>

// BDE
#include <bdlma_localsequentialallocator.h>
#include <bsl_utility.h>
#include <bsla_annotations.h>
#include <bslmf_assert.h>
#include <bslmt_mutexassert.h>
#include <bsls_assert.h>
#include <bsls_performancehint.h>

namespace BloombergLP {
namespace bmqimp {

namespace {
BSLMF_ASSERT(bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID <
             QueueManager::k_INITIAL_SUBQUEUE_ID);
// It is assumed the default SubQueueId is smaller than the initial
// SubQueueId for the purpose of correctly generating an increasing
// sequence of non-default 'subQueueId' integers that are used when
// communicating with upstream.
}  // close unnamed namespace

// ------------------
// class QueueManager
// ------------------

// PRIVATE ACCESSORS
QueueManager::QueueSp
QueueManager::lookupQueueLocked(const bmqp::QueueId& queueId) const
{
    // PRECONDITIONS
    // d_queuesLock locked

    // Lookup queue
    QueuesMap::const_iterator it = d_queues.findByKey1(queueId);
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(it == d_queues.end())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return QueueSp();  // RETURN
    }

    BSLS_ASSERT_SAFE(it->value());

    return it->value();
}

QueueManager::QueueSp QueueManager::lookupQueueBySubscriptionIdLocked(
    bmqt::CorrelationId* correlationId,
    unsigned int*        subscriptionHandleId,
    int                  qId,
    unsigned int         internalSubscriptionId) const
{
    // PRECONDITIONS
    // d_queuesLock locked

    if (internalSubscriptionId == bmqp::Protocol::k_DEFAULT_SUBSCRIPTION_ID) {
        // Look up by 'bmqp::QueueId'
        const QueueSp& result = lookupQueueLocked(bmqp::QueueId(qId));

        // This is the case when we receive no Subscription - Options are
        // missing or packed.  We have two options here - empty CorrelationId
        // or the one of the queue.  Perhaps, the empty one is a better choice.
        *correlationId = bmqt::CorrelationId();
        return result;
    }
    // lookup by 'subscriptionId'
    SubscriptionId id(qId, internalSubscriptionId);

    QueuesBySubscriptions::const_iterator cit = d_queuesBySubscriptionIds.find(
        id);

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            cit == d_queuesBySubscriptionIds.end())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return QueueSp();  // RETURN
    }

    BSLS_ASSERT_SAFE(cit->second.d_queue);

    *subscriptionHandleId = cit->second.d_subscriptionHandle.first;
    *correlationId        = cit->second.d_subscriptionHandle.second;

    return cit->second.d_queue;
}

// CREATORS
QueueManager::QueueManager(bslma::Allocator* allocator)
: d_queuesLock(bsls::SpinLock::s_unlocked)
, d_queues(allocator)
, d_uris(allocator)
, d_nextQueueId(0)
, d_queuesBySubscriptionIds(allocator)
, d_schemaLearner(allocator)
, d_allocator_p(allocator)
{
    // NOTHING
}

QueueManager::~QueueManager()
{
    // NOTHING
}

// MANIPULATORS
void QueueManager::insertQueue(const QueueSp& queue)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(queue);
    BSLS_ASSERT_SAFE(queue->id() >= 0 && queue->id() < d_nextQueueId);

    bsls::SpinLockGuard guard(&d_queuesLock);  // d_queuesLock LOCKED

    UrisMap::iterator uriIter = d_uris.find(
        bsl::string(queue->uri().canonical(), d_allocator_p));
    if (uriIter == d_uris.end()) {
        // Canonical URI was not found, this is the first time we insert a
        // queue having this canonical URI (or the first time after closing
        // one)
#ifdef BSLS_ASSERT_SAFE_IS_ACTIVE
        // Either the 'queue' has a SubQueueId and it is the initial SubQueueId
        // or it does not have a SubQueueId at all (in which case its
        // subQueueId value must equal the default)
        BSLS_ASSERT_SAFE(queue->hasDefaultSubQueueId() ||
                         queue->subQueueId() == k_INITIAL_SUBQUEUE_ID);
#endif  // BSLS_ASSERT_SAFE_IS_ACTIVE

        // Insert into URIs map
        QueueManager_QueueInfo queueInfo(queue->id(), d_allocator_p);
        if (!queue->hasDefaultSubQueueId()) {
            // Set the next subId for this specific canonical URI to one more
            // than the first subQueueId generated through a previous call to
            // 'generateQueueAndSubQueueId'.
            queueInfo.d_nextSubQueueId = queue->subQueueId() + 1;
        }

        bsl::pair<UrisMap::iterator, bool> insertRet = d_uris.emplace(
            UrisMap::value_type(bsl::string(queue->uri().canonical(),
                                            d_allocator_p),
                                queueInfo));

        BSLS_ASSERT_SAFE(insertRet.second);  // Insertion was successful

        // Set the iterator
        uriIter = insertRet.first;
    }

    // Insert into subQueueIds map
    const unsigned int      subId     = queue->subQueueId();
    QueueManager_QueueInfo& queueInfo = uriIter->second;
    BSLS_ASSERT_SAFE(queueInfo.d_queueId == queue->id());
    // queueId we have for this canonical URI matches queueId of 'queue'

    typedef QueueManager_QueueInfo::SubQueueIdsMap SubQueueIdsMap;

    BSLS_ASSERT_SAFE(queueInfo.d_subQueueIdsMap.find(queue->uri().id()) ==
                     queueInfo.d_subQueueIdsMap.end());
    BSLA_MAYBE_UNUSED bsl::pair<SubQueueIdsMap::iterator, bool> insertRet =
        queueInfo.d_subQueueIdsMap.insert(
            bsl::make_pair(queue->uri().id(), subId));
    BSLS_ASSERT_SAFE(insertRet.second);

    // Insert into queues map
    const bmqp::QueueId queueId(queue->id(), subId);
    BSLA_MAYBE_UNUSED   bsl::pair<QueuesMap::iterator, QueuesMap::InsertResult>
        addRet = d_queues.insert(queueId, queue->correlationId(), queue);
    BSLS_ASSERT_SAFE(addRet.second == QueuesMap::e_INSERTED);
}

QueueManager::QueueSp QueueManager::removeQueue(const Queue* queue)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(queue);
    BSLS_ASSERT_SAFE(queue->id() >= 0);

    const bmqp::QueueId queueId(queue->id(), queue->subQueueId());

    bsls::SpinLockGuard guard(&d_queuesLock);  // d_queuesLock LOCKED

    // Find queue and erase from queues map
    QueuesMap::iterator iter = d_queues.findByKey1(queueId);
    if (iter == d_queues.end()) {
        return QueueSp();  // RETURN
    }
    const QueueSp queueSp = iter->value();
    d_queues.erase(iter);

    // Erase appId from subQueueIds map
    UrisMap::iterator uriIter = d_uris.find(
        bsl::string(queue->uri().canonical(), d_allocator_p));
    BSLS_ASSERT_SAFE(uriIter != d_uris.end());

    QueueManager_QueueInfo::SubQueueIdsMap& subQueueIdsMap =
        uriIter->second.d_subQueueIdsMap;
    BSLS_ASSERT_SAFE(subQueueIdsMap.find(queue->uri().id()) !=
                     subQueueIdsMap.end());
    subQueueIdsMap.erase(queue->uri().id());

    if (subQueueIdsMap.empty()) {
        // No more appIds for this canonical URI, so we can remove it from the
        // URIs map
        BSLA_MAYBE_UNUSED UrisMap::size_type numErasedUris = d_uris.erase(
            bsl::string(queue->uri().canonical(), d_allocator_p));
        BSLS_ASSERT_SAFE(numErasedUris == 1);
    }

    return queueSp;
}

void QueueManager::generateQueueAndSubQueueId(bmqp::QueueId*      queueId,
                                              const bmqt::Uri&    uri,
                                              bsls::Types::Uint64 flags)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(queueId);
    BSLS_ASSERT_SAFE(uri.isValid());

    bsls::SpinLockGuard guard(&d_queuesLock);  // d_queuesLock LOCKED

    // Set the 'queueId'
    UrisMap::iterator uriIter = d_uris.find(
        bsl::string(uri.canonical(), d_allocator_p));
    if (uriIter == d_uris.end()) {
        // Canonical URI was not found, generate a new 'queueId'
        queueId->setId(generateNextQueueId());
    }
    else {
        // Canonical URI was found, so there is an associated 'queueId'
        BSLS_ASSERT_SAFE(uriIter->second.d_queueId !=
                         Queue::k_INVALID_QUEUE_ID);
        queueId->setId(uriIter->second.d_queueId);
    }

    // Set the 'subQueueId'
    if (!(bmqt::QueueFlagsUtil::isReader(flags) && !uri.id().empty())) {
        // Not a reader having an appId, so set to the default 'subQueueId'
        queueId->setSubId(bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID);
        return;  // RETURN
    }

    if (uriIter == d_uris.end()) {
        // Canonical URI was not found
        queueId->setSubId(k_INITIAL_SUBQUEUE_ID);
    }
    else {
        // Canonical URI was found, so set to the next 'subQueueId' and
        // increment it
        queueId->setSubId(uriIter->second.d_nextSubQueueId++);
    }
}

void QueueManager::resetState()
{
    bsls::SpinLockGuard guard(&d_queuesLock);  // d_queuesLock LOCKED

    // Before giving up our own internal handle to each queue, set their sate
    // to 'closed', so that if the user still have their own reference, the
    // queue will be considered invalid when they try to use it.
    for (QueuesMap::iterator iter = d_queues.begin(); iter != d_queues.end();
         ++iter) {
        iter->value()->setState(QueueState::e_CLOSED);
    }

    d_nextQueueId = 0;
    d_queues.clear();
    d_uris.clear();
}

void QueueManager::observePushEvent(Event*                          queueEvent,
                                    const bmqp::EventUtilQueueInfo& info)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(queueEvent);

    bmqt::CorrelationId correlationId;
    unsigned int        subscriptionHandleId;

    int queueId = info.d_header.queueId();

    // Update stats
    const QueueSp queue = lookupQueueBySubscriptionId(&correlationId,
                                                      &subscriptionHandleId,
                                                      queueId,
                                                      info.d_subscriptionId);

    BSLS_ASSERT_SAFE(queue);
    BSLS_ASSERT_SAFE(queue->id() == info.d_header.queueId());

    queue->statUpdateOnMessage(info.d_applicationDataSize, false);

    // Use 'subscriptionHandle' instead of the internal
    // 'info.d_subscriptionId' so that
    // 'bmqimp::Event::subscriptionId()' returns 'subscriptionHandle'

    queueEvent->insertQueue(info.d_subscriptionId, queue);

    queueEvent->addContext(correlationId,
                           subscriptionHandleId,
                           info.d_schema_sp);
}

int QueueManager::onPushEvent(QueueManager::EventInfos* eventInfos,
                              int*                      messageCount,
                              bool* hasMessageWithMultipleSubQueueIds,
                              const bmqp::PushMessageIterator& iterator)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(eventInfos);
    BSLS_ASSERT_SAFE(messageCount);
    BSLS_ASSERT_SAFE(iterator.isValid());

    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS = 0  // No error
        ,
        rc_ITERATION_ERROR = -1  // An error was encountered while
                                 // iterating
        ,
        rc_OPTIONS_LOAD_ERROR = -2  // An error occurred while loading the
                                    // options of a message
        ,
        rc_SUB_QUEUE_IDS_LOAD_ERROR = -3  // An error occurred while loading a
                                          // SubQueueId option
    };

    *messageCount                      = 0;
    *hasMessageWithMultipleSubQueueIds = false;

    bmqp::PushMessageIterator msgIterator(iterator, d_allocator_p);
    int                       rc = rc_SUCCESS;

    bsls::SpinLockGuard guard(&d_queuesLock);  // d_queuesLock LOCKED

    while (
        BSLS_PERFORMANCEHINT_PREDICT_LIKELY((rc = msgIterator.next()) == 1)) {
        // Check options
        BSLS_ASSERT_SAFE(msgIterator.hasOptions());

        bmqp::OptionsView optionsView(d_allocator_p);
        rc = msgIterator.loadOptionsView(&optionsView);
        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY((rc != 0) ||
                                                  !optionsView.isValid())) {
            // Corrupted message options
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
            return rc * 10 + rc_OPTIONS_LOAD_ERROR;  // RETURN
        }

        const bmqp::PushHeader&           header  = msgIterator.header();
        int                               queueId = header.queueId();
        const bmqp::MessagePropertiesInfo input(header);

        bmqp::MessageProperties::SchemaPtr* schema_p = d_schemaLearner.observe(
            d_schemaLearner.createContext(queueId),
            input);

        bmqp::MessageProperties::SchemaPtr schema;

        if (schema_p) {
            if (schema_p->get() == 0) {
                // Learn new Schema by reading all MessageProperties.
                bdlbb::Blob appData(d_allocator_p);
                if (msgIterator.loadApplicationData(&appData)) {
                    bmqp::MessageProperties mps(d_allocator_p);

                    if (mps.streamIn(appData, input.isExtended()) == 0) {
                        // Learn new schema.
                        schema   = mps.getSchema(d_allocator_p);
                        schema_p = &schema;
                    }
                }
            }
        }
        else {
            schema_p = &schema;
        }

        BSLS_ASSERT_SAFE(schema_p);

        eventInfos->resize(eventInfos->size() + 1);
        if ((optionsView.find(bmqp::OptionType::e_SUB_QUEUE_INFOS) !=
             optionsView.end()) ||
            (optionsView.find(bmqp::OptionType::e_SUB_QUEUE_IDS_OLD) !=
             optionsView.end())) {
            bdlma::LocalSequentialAllocator<16 * sizeof(int)> lsa(
                d_allocator_p);

            bmqp::Protocol::SubQueueInfosArray subQueueInfos(&lsa);
            rc = optionsView.loadSubQueueInfosOption(&subQueueInfos);
            if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(rc != 0)) {
                BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
                return rc * 10 + rc_SUB_QUEUE_IDS_LOAD_ERROR;  // RETURN
            }

            if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(subQueueInfos.size() >
                                                      1)) {
                BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
                *hasMessageWithMultipleSubQueueIds = true;
            }
            else {
                eventInfos->back().d_ids.push_back(
                    bmqp::EventUtilQueueInfo(msgIterator.header(),
                                             subQueueInfos[0].id(),
                                             msgIterator.applicationDataSize(),
                                             *schema_p));
            }

            // Update message count
            *messageCount += subQueueInfos.size();
        }
        else {
            eventInfos->back().d_ids.push_back(bmqp::EventUtilQueueInfo(
                msgIterator.header(),
                bmqp::Protocol::k_DEFAULT_SUBSCRIPTION_ID,
                msgIterator.applicationDataSize(),
                *schema_p));

            // Update message count
            ++(*messageCount);
        }
    }

    // Check if encountered an error while iterating
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(rc != 0)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return rc * 10 + rc_ITERATION_ERROR;  // RETURN
    }

    return rc;
}

int QueueManager::updateStatsOnPutEvent(
    int*                            messageCount,
    const bmqp::PutMessageIterator& iterator)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(messageCount);
    BSLS_ASSERT_SAFE(iterator.isValid());

    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS = 0  // No error
        ,
        rc_ITERATION_ERROR = -1  // An error was encountered while iterating
        ,
        rc_INVALID_QUEUE = -2  // Queue not valid or not opened with write
                               // flags
    };

    *messageCount = 0;

    bmqp::PutMessageIterator putIterator(iterator, d_allocator_p);
    int                      rc = rc_SUCCESS;

    bsls::SpinLockGuard guard(&d_queuesLock);  // d_queuesLock LOCKED

    // Iterate over the messages to validate queues and update their associated
    // stats.
    while (
        BSLS_PERFORMANCEHINT_PREDICT_LIKELY((rc = putIterator.next()) == 1)) {
        // NOTE: We don't need to verify that queueId is valid (i.e.
        //       '< d_numQueues') because we assert that looking up a queue
        //       succeeds.
        const bmqp::QueueId queueId(putIterator.header().queueId());
        const QueueSp&      queue = lookupQueueLocked(queueId);
        BSLS_ASSERT_SAFE(queue);

        // Check that the queue is writable and is valid, which means not only
        // the OPENED state but it also may be in PENDING or REOPENING states
        // and still accept PUT messages.
        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
                !queue->isValid() ||
                !bmqt::QueueFlagsUtil::isWriter(queue->flags()))) {
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
            return rc * 10 + rc_INVALID_QUEUE;  // RETURN
        }

        // When executed as a part of unit test, queue stat context may be
        // not initialized
        if (BSLS_PERFORMANCEHINT_PREDICT_LIKELY(queue->statContext())) {
            queue->statUpdateOnMessage(putIterator.applicationDataSize(),
                                       true);
        }

        ++(*messageCount);
    }

    // Check if encountered an error while iterating
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(rc != 0)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return rc * 10 + rc_ITERATION_ERROR;  // RETURN
    }

    return rc;
}

void QueueManager::incrementSubStreamCount(const bsl::string& canonicalUri)
{
    UrisMap::iterator uriIter = d_uris.find(canonicalUri);
    BSLS_ASSERT_SAFE(uriIter != d_uris.end());

    uriIter->second.d_subStreamCount += 1;
}

void QueueManager::decrementSubStreamCount(const bsl::string& canonicalUri)
{
    UrisMap::iterator uriIter = d_uris.find(canonicalUri);
    BSLS_ASSERT_SAFE(uriIter != d_uris.end());
    BSLS_ASSERT_SAFE(uriIter->second.d_subStreamCount != 0);

    uriIter->second.d_subStreamCount -= 1;
}

void QueueManager::resetSubStreamCount(const bsl::string& canonicalUri)
{
    UrisMap::iterator uriIter = d_uris.find(canonicalUri);
    BSLS_ASSERT_SAFE(uriIter != d_uris.end());

    uriIter->second.d_subStreamCount = 0;
}

void QueueManager::updateSubscriptions(
    const bsl::shared_ptr<Queue>&         queue,
    const bmqp_ctrlmsg::StreamParameters& config)
{
    BSLS_ASSERT_SAFE(queue);

    const bmqp_ctrlmsg::StreamParameters& previous = queue->config();

    for (size_t i = 0; i < previous.subscriptions().size(); ++i) {
        unsigned int internalSubscriptionId =
            previous.subscriptions()[i].sId();

        SubscriptionId id(queue->id(), internalSubscriptionId);

        d_queuesBySubscriptionIds.erase(id);
    }

    for (size_t i = 0; i < config.subscriptions().size(); ++i) {
        unsigned int internalSubscriptionId = config.subscriptions()[i].sId();

        d_queuesBySubscriptionIds.insert(bsl::make_pair(
            SubscriptionId(queue->id(), internalSubscriptionId),
            QueueBySubscription(internalSubscriptionId, queue)));
    }
}

// ACCESSORS
QueueManager::QueueSp QueueManager::lookupQueue(const bmqt::Uri& uri) const
{
    bsls::SpinLockGuard guard(&d_queuesLock);  // d_queuesLock LOCKED

    // Find by canonical URI in URIs map
    UrisMap::const_iterator uriIt = d_uris.find(
        bsl::string(uri.canonical(), d_allocator_p));
    if (uriIt == d_uris.end()) {
        // Canonical URI was not found
        return QueueSp();  // RETURN
    }

    // Find by appId in SubQueueIds map
    const QueueManager_QueueInfo& queueInfo = uriIt->second;

    typedef QueueManager_QueueInfo::SubQueueIdsMap SubQueueIdsMap;

    SubQueueIdsMap::const_iterator sqidIt = queueInfo.d_subQueueIdsMap.find(
        uri.id());
    if (sqidIt == queueInfo.d_subQueueIdsMap.end()) {
        // appId was not found
        return QueueSp();  // RETURN
    }

    // Construct queueId
    const int           id    = queueInfo.d_queueId;
    const unsigned int  subId = sqidIt->second;
    const bmqp::QueueId queueId(id, subId);

    QueuesMap::const_iterator queueIt = d_queues.findByKey1(queueId);
    BSLS_ASSERT_SAFE(queueIt != d_queues.end());
    BSLS_ASSERT_SAFE(queueIt->value());
    BSLS_ASSERT_SAFE(queueIt->value()->uri() == uri);
    // Ensure found queue has the specified 'uri'

    return queueIt->value();
}

QueueManager::QueueSp
QueueManager::lookupQueue(const bmqt::CorrelationId& correlationId) const
{
    bsls::SpinLockGuard guard(&d_queuesLock);  // d_queuesLock LOCKED

    QueuesMap::const_iterator it = d_queues.findByKey2(correlationId);
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(it == d_queues.end())) {
        return QueueSp();  // RETURN
    }

    BSLS_ASSERT_SAFE(it->value());

    return it->value();
}

void QueueManager::lookupQueuesByState(bsl::vector<QueueSp>* queues,
                                       QueueState::Enum      state) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(queues);

    bsls::SpinLockGuard guard(&d_queuesLock);  // d_queuesLock LOCKED

    for (QueuesMap::const_iterator iter =
             d_queues.begin(QueuesMap::e_SECOND_KEY);
         iter != d_queues.end();
         ++iter) {
        const QueueSp& queue = iter->value();
        if (queue->state() == state) {
            queues->push_back(queue);
        }
    }
}

void QueueManager::getAllQueues(bsl::vector<QueueSp>* queues) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(queues);

    bsls::SpinLockGuard guard(&d_queuesLock);  // d_queuesLock LOCKED

    for (QueuesMap::const_iterator iter =
             d_queues.begin(QueuesMap::e_SECOND_KEY);
         iter != d_queues.end();
         ++iter) {
        queues->push_back(iter->value());
    }
}

unsigned int
QueueManager::subStreamCount(const bsl::string& canonicalUri) const
{
    // Find by canonical URI in URIs map
    UrisMap::const_iterator uriCiter = d_uris.find(canonicalUri);
    BSLS_ASSERT_SAFE(uriCiter != d_uris.end());

    return uriCiter->second.d_subStreamCount;
}

}  // close package namespace
}  // close enterprise namespace
