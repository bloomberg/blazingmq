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

// bmqimp_messagecorrelationidcontainer.cpp                           -*-C++-*-
#include <bmqimp_messagecorrelationidcontainer.h>

#include <bmqscm_version.h>
// BMQ
#include <bmqp_messageguidgenerator.h>
#include <bmqp_protocol.h>
#include <bmqp_protocolutil.h>
#include <bmqp_queueid.h>

// BDE
#include <bsl_utility.h>
#include <bsl_vector.h>
#include <bsls_performancehint.h>

namespace BloombergLP {
namespace bmqimp {

// ----------------------------------------------------------
// class MessageCorrelationIdContainer::QueueAndCorrelationId
// ----------------------------------------------------------

MessageCorrelationIdContainer::QueueAndCorrelationId::QueueAndCorrelationId(
    bslma::Allocator* allocator)
: d_correlationId()
, d_queueId(bmqp::QueueId::k_UNASSIGNED_QUEUE_ID)
, d_messageType(bmqp::EventType::e_UNDEFINED)
, d_messageData(allocator)
{
    // NOTHING
}

MessageCorrelationIdContainer::QueueAndCorrelationId::QueueAndCorrelationId(
    const bmqt::CorrelationId& correlationId,
    const bmqp::QueueId&       queueId,
    bslma::Allocator*          allocator)
: d_correlationId(correlationId)
, d_queueId(queueId)
, d_messageType(bmqp::EventType::e_UNDEFINED)
, d_messageData(allocator)
{
    // NOTHING
}

MessageCorrelationIdContainer::QueueAndCorrelationId::QueueAndCorrelationId(
    const MessageCorrelationIdContainer::QueueAndCorrelationId& other,
    bslma::Allocator*                                           allocator)
: d_correlationId(other.d_correlationId)
, d_queueId(other.d_queueId)
, d_messageType(other.d_messageType)
, d_messageData(other.d_messageData, allocator)
, d_requestContext(other.d_requestContext)
{
    // NOTHING
}

// -----------------------------------
// class MessageCorrelationIdContainer
// -----------------------------------

void MessageCorrelationIdContainer::addQueueItem(
    const bmqp::QueueId&      queueId,
    const bmqt::MessageGUID&  itemGUID,
    const bsls::TimeInterval& sentTime)
{
    d_queueItems[queueId].insert(bsl::make_pair(itemGUID, sentTime));
}

void MessageCorrelationIdContainer::removeQueueItem(
    const bmqp::QueueId&     queueId,
    const bmqt::MessageGUID& itemGUID)
{
    QueueItemsMap::iterator cqit = d_queueItems.find(queueId);

    BSLS_ASSERT_SAFE(cqit != d_queueItems.end() && "Queue not found");

    HandleAndExpirationTimeMap::const_iterator chit = cqit->second.find(
        itemGUID);
    BSLS_ASSERT_SAFE(chit != cqit->second.end() && "Key not found");

    // Delete the queue item and if there are no more items related to the
    // queue remove the queue entry from the outer map.
    cqit->second.erase(chit);
    if (cqit->second.empty()) {
        d_queueItems.erase(cqit);
    }
}

MessageCorrelationIdContainer::MessageCorrelationIdContainer(
    bslma::Allocator* allocator)
: d_lock(bsls::SpinLock::s_unlocked)
, d_correlationIds(allocator)
, d_queueItems(allocator)
, d_numPuts(0)
, d_numControls(0)
, d_allocator_p(allocator)
{
    // NOTHING
}

void MessageCorrelationIdContainer::reset()
{
    bsls::SpinLockGuard guard(&d_lock);  // LOCK
    d_numPuts     = 0;
    d_numControls = 0;
    d_correlationIds.clear();
    d_queueItems.clear();
}

void MessageCorrelationIdContainer::add(
    const bmqt::MessageGUID&   key,
    const bmqt::CorrelationId& correlationId,
    const bmqp::QueueId&       queueId)
{
    bsls::SpinLockGuard guard(&d_lock);  // LOCK

    QueueAndCorrelationId toInsert(correlationId, queueId, d_allocator_p);
    d_correlationIds.insert(bsl::make_pair(key, toInsert));
}

bmqt::MessageGUID MessageCorrelationIdContainer::add(
    const RequestManagerType::RequestSp& context,
    const bmqp::QueueId&                 queueId,
    const bdlbb::Blob&                   blob)
{
    bsls::SpinLockGuard guard(&d_lock);  // LOCK

    QueueAndCorrelationId toInsert(d_allocator_p);
    toInsert.d_messageType    = bmqp::EventType::e_CONTROL;
    toInsert.d_requestContext = context;
    toInsert.d_queueId        = queueId;
    toInsert.d_messageData    = blob;

    // Use internal GUID as a key to add the control message
    bmqt::MessageGUID key = bmqp::MessageGUIDGenerator::testGUID();
    d_correlationIds.insert(bsl::make_pair(key, toInsert));
    ++d_numControls;

    return key;
}

MessageCorrelationIdContainer::CorrelationIdsMap::const_iterator
MessageCorrelationIdContainer::removeLocked(
    const CorrelationIdsMap::const_iterator& cit)
{
    BSLS_ASSERT_SAFE(cit != d_correlationIds.end());

    if (cit->second.d_messageType == bmqp::EventType::e_PUT) {
        BSLS_ASSERT_SAFE(d_numPuts > 0);
        --d_numPuts;
        const bool isAckRequested = bmqp::PutHeaderFlagUtil::isSet(
            cit->second.d_header.flags(),
            bmqp::PutHeaderFlags::e_ACK_REQUESTED);
        if (isAckRequested) {
            removeQueueItem(cit->second.d_queueId, cit->first);
        }
    }
    else if (cit->second.d_messageType == bmqp::EventType::e_CONTROL) {
        BSLS_ASSERT_SAFE(d_numControls > 0);
        BSLS_ASSERT_SAFE(cit->second.d_requestContext);

        cit->second.d_requestContext->adoptUserData(bdld::Datum::createNull());
        --d_numControls;
    }

    return d_correlationIds.erase(cit);
}

int MessageCorrelationIdContainer::remove(const bmqt::MessageGUID& key,
                                          bmqt::CorrelationId* correlationId)
{
    bsls::SpinLockGuard guard(&d_lock);  // LOCK

    CorrelationIdsMap::const_iterator cit = d_correlationIds.find(key);
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(d_correlationIds.end() == cit)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return -1;  // RETURN
    }

    if (correlationId) {
        *correlationId = cit->second.d_correlationId;
    }

    removeLocked(cit);

    return 0;
}

void MessageCorrelationIdContainer::associateMessageData(
    const bmqp::PutHeader&    header,
    const bdlbb::Blob&        appData,
    const bsls::TimeInterval& sentTime)
{
    bsls::SpinLockGuard guard(&d_lock);  // LOCK

    CorrelationIdsMap::iterator it = d_correlationIds.find(
        header.messageGUID());
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(it == d_correlationIds.end())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        BSLS_ASSERT_SAFE(false && "Key not found");
        return;  // RETURN
    }
    bmqp::QueueId qid(header.queueId());
    it->second.d_messageType = bmqp::EventType::e_PUT;
    it->second.d_header      = header;
    it->second.d_messageData = appData;
    it->second.d_queueId     = qid;
    ++d_numPuts;

    const bool isAckRequested = bmqp::PutHeaderFlagUtil::isSet(
        header.flags(),
        bmqp::PutHeaderFlags::e_ACK_REQUESTED);

    if (BSLS_PERFORMANCEHINT_PREDICT_LIKELY(isAckRequested)) {
        // Add a per queue item with sending timestamp
        addQueueItem(it->second.d_queueId, header.messageGUID(), sentTime);
    }
}

bool MessageCorrelationIdContainer::iterateAndInvoke(const KeyIdsCb& callback)
{
    bsls::SpinLockGuard guard(&d_lock);  // LOCK

    CorrelationIdsMap::const_iterator cit = d_correlationIds.begin();
    while (cit != d_correlationIds.end()) {
        bool       removeItem = false;
        const bool interrupt  = callback(&removeItem, cit->first, cit->second);
        if (removeItem) {
            cit = removeLocked(cit);
        }
        else {
            ++cit;
        }
        if (interrupt) {
            return false;  // RETURN
        }
    }

    return true;
}

bool MessageCorrelationIdContainer::iterateAndInvoke(
    const bsl::vector<bmqt::MessageGUID>& keys,
    const KeyIdsCb&                       callback)
{
    bsls::SpinLockGuard guard(&d_lock);  // LOCK

    for (size_t i = 0; i < keys.size(); ++i) {
        CorrelationIdsMap::const_iterator cit = d_correlationIds.find(keys[i]);
        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(cit ==
                                                  d_correlationIds.end())) {
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
            BSLS_ASSERT_SAFE(false && "Key not found");
            continue;  // CONTINUE
        }
        bool       removeItem = false;
        const bool interrupt  = callback(&removeItem, cit->first, cit->second);
        if (removeItem) {
            removeLocked(cit);
        }
        if (interrupt) {
            return false;  // RETURN
        }
    }
    return true;
}

bsls::TimeInterval MessageCorrelationIdContainer::getExpiredIds(
    bsl::vector<bmqt::MessageGUID>*     keys,
    const bsl::unordered_map<int, int>& queueExpirationTimeoutMap,
    const bsls::TimeInterval&           expirationTime)
{
    BSLS_ASSERT_SAFE(keys);

    bsls::SpinLockGuard guard(&d_lock);  // LOCK

    bsls::TimeInterval minTs(0);
    // Iterate over each queue
    for (QueueItemsMap::iterator qit = d_queueItems.begin();
         qit != d_queueItems.end();
         ++qit) {
        // Get the queue expiration timeout
        const int                                    qId = qit->first.id();
        bsl::unordered_map<int, int>::const_iterator cit =
            queueExpirationTimeoutMap.find(qId);

        // If queueId is absent in the queue timeout map that means this queue
        // is no longer opened. All its pending messages should be removed, so
        // add them to the expired list.
        const bool isOrphan = cit == queueExpirationTimeoutMap.end();
        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(isOrphan)) {
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
            for (HandleAndExpirationTimeMap::iterator hit =
                     qit->second.begin();
                 hit != qit->second.end();
                 ++hit) {
                keys->push_back(hit->first);
            }
            continue;  // CONTINUE
        }

        const int queueTimeoutMs = cit->second;

        BSLS_ASSERT_SAFE(queueTimeoutMs > 0);

        // Iterate over queue items (PUT message keys and timestamps)
        for (HandleAndExpirationTimeMap::iterator hit = qit->second.begin();
             hit != qit->second.end();
             ++hit) {
            // Calculate message expiration time (sentTime + queueTimeout)
            bsls::TimeInterval messageTimeout = hit->second;
            messageTimeout.addMilliseconds(queueTimeoutMs);

            if (BSLS_PERFORMANCEHINT_PREDICT_LIKELY(messageTimeout >
                                                    expirationTime)) {
                // No more expired items in the current queue.  Check the next
                // expiration time.
                if ((minTs == 0) || (minTs > messageTimeout)) {
                    minTs = messageTimeout;
                }
                break;  // BREAK
            }
            keys->push_back(hit->first);
        }
    }

    return minTs;
}

int MessageCorrelationIdContainer::find(bmqt::CorrelationId*     correlationId,
                                        const bmqt::MessageGUID& key) const
{
    bsls::SpinLockGuard guard(&d_lock);  // LOCK

    CorrelationIdsMap::const_iterator cit = d_correlationIds.find(key);
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(d_correlationIds.end() == cit)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return -1;  // RETURN
    }

    *correlationId = cit->second.d_correlationId;
    return 0;
}

}  // close package namespace
}  // close enterprise namespace
