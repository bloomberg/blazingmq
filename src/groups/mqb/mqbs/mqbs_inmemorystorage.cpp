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

// mqbs_inmemorystorage.cpp                                           -*-C++-*-
#include <mqbs_inmemorystorage.h>

#include <mqbscm_version.h>
// MQB
#include <mqbi_queue.h>
#include <mqbi_queueengine.h>
#include <mqbstat_queuestats.h>

// MWC
#include <mwcma_countingallocatorstore.h>
#include <mwcsys_time.h>
#include <mwcu_printutil.h>

// BDE
#include <bsl_algorithm.h>
#include <bsl_iostream.h>
#include <bsl_utility.h>
#include <bslma_allocator.h>
#include <bsls_annotation.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace mqbs {

namespace {

const int k_GC_MESSAGES_BATCH_SIZE = 1000;  // how many to process in one run

}

// ---------------------
// class InMemoryStorage
// ---------------------

// CREATORS
InMemoryStorage::InMemoryStorage(const bmqt::Uri&        uri,
                                 const mqbu::StorageKey& queueKey,
                                 int                     partitionId,
                                 const mqbconfm::Domain& config,
                                 mqbu::CapacityMeter*    parentCapacityMeter,
                                 const bmqp::RdaInfo&    defaultRdaInfo,
                                 bslma::Allocator*       allocator,
                                 mwcma::CountingAllocatorStore* allocatorStore)
: d_allocator_p(allocator)
, d_queue_p(0)
, d_key(queueKey)
, d_uri(uri, allocator)
, d_partitionId(partitionId)
, d_config()
, d_capacityMeter("queue [" + uri.asString() + "]",
                  parentCapacityMeter,
                  allocator)
, d_items(bsls::TimeInterval()
              .addMilliseconds(config.deduplicationTimeMs())
              .totalNanoseconds(),
          allocatorStore ? allocatorStore->get("Handles") : d_allocator_p)
, d_virtualStorageCatalog(
      this,
      allocatorStore ? allocatorStore->get("VirtualHandles") : d_allocator_p)
, d_ttlSeconds(config.messageTtl())
, d_emptyAppId(allocator)
, d_nullAppKey()
, d_isEmpty(1)
, d_defaultRdaInfo(defaultRdaInfo)
{
    BSLS_ASSERT_SAFE(0 <= d_ttlSeconds);  // Broadcast queues can use 0 for TTL
}

InMemoryStorage::~InMemoryStorage()
{
    // NOTHING
}

// MANIPULATORS
//   (virtual mqbi::Storage)
int InMemoryStorage::configure(
    BSLS_ANNOTATION_UNUSED bsl::ostream& errorDescription,
    const mqbconfm::Storage&             config,
    const mqbconfm::Limits&              limits,
    const bsls::Types::Int64             messageTtl,
    BSLS_ANNOTATION_UNUSED const int     maxDeliveryAttempts)
{
    d_config = config;
    d_capacityMeter.setLimits(limits.messages(), limits.bytes())
        .setWatermarkThresholds(limits.messagesWatermarkRatio(),
                                limits.bytesWatermarkRatio());
    d_ttlSeconds = messageTtl;

    return 0;
}

void InMemoryStorage::setQueue(mqbi::Queue* queue)
{
    d_queue_p = queue;

    // Update queue stats if a queue has been associated with the storage.

    if (d_queue_p) {
        const bsls::Types::Int64 numMessage = numMessages(
            mqbu::StorageKey::k_NULL_KEY);
        const bsls::Types::Int64 numByte = numBytes(
            mqbu::StorageKey::k_NULL_KEY);

        d_queue_p->stats()->setQueueContentRaw(numMessage, numByte);

        BALL_LOG_INFO << "Associated queue [" << queue->uri() << "] with key ["
                      << queueKey() << "] and PartitionId ["
                      << queue->partitionId() << "] with its storage having ["
                      << mwcu::PrintUtil::prettyNumber(numMessage)
                      << " messages and "
                      << mwcu::PrintUtil::prettyNumber(numByte)
                      << " bytes of outstanding.";
    }
}

void InMemoryStorage::close()
{
    // NOTHING
}

bslma::ManagedPtr<mqbi::StorageIterator>
InMemoryStorage::getIterator(const mqbu::StorageKey& appKey)
{
    if (appKey.isNull()) {
        bslma::ManagedPtr<mqbi::StorageIterator> mp(
            new (*d_allocator_p)
                InMemoryStorageIterator(this, d_items.begin()),
            d_allocator_p);

        return mp;  // RETURN
    }

    return d_virtualStorageCatalog.getIterator(appKey);
}

mqbi::StorageResult::Enum
InMemoryStorage::getIterator(bslma::ManagedPtr<mqbi::StorageIterator>* out,
                             const mqbu::StorageKey&                   appKey,
                             const bmqt::MessageGUID&                  msgGUID)
{
    if (appKey.isNull()) {
        ItemsMapConstIter it = d_items.find(msgGUID);
        if (it == d_items.end()) {
            return mqbi::StorageResult::e_GUID_NOT_FOUND;  // RETURN
        }

        out->load(new (*d_allocator_p) InMemoryStorageIterator(this, it),
                  d_allocator_p);

        return mqbi::StorageResult::e_SUCCESS;  // RETURN
    }

    return d_virtualStorageCatalog.getIterator(out, appKey, msgGUID);
}

mqbi::StorageResult::Enum
InMemoryStorage::put(mqbi::StorageMessageAttributes*     attributes,
                     const bmqt::MessageGUID&            msgGUID,
                     const bsl::shared_ptr<bdlbb::Blob>& appData,
                     const bsl::shared_ptr<bdlbb::Blob>& options,
                     const StorageKeys&                  storageKeys)
{
    const int msgSize = appData->length();

    if (storageKeys.empty()) {
        // Store the specified message in the 'physical' as well as *all*
        // virtual storages.

        if (d_items.isInHistory(msgGUID)) {
            return mqbi::StorageResult::e_DUPLICATE;
        }

        // Verify if we have enough capacity.
        mqbu::CapacityMeter::CommitResult capacity =
            d_capacityMeter.commitUnreserved(1, msgSize);

        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
                capacity != mqbu::CapacityMeter::e_SUCCESS)) {
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

            return (capacity == mqbu::CapacityMeter::e_LIMIT_MESSAGES
                        ? mqbi::StorageResult::e_LIMIT_MESSAGES
                        : mqbi::StorageResult::e_LIMIT_BYTES);  // RETURN
        }

        d_items.insert(bsl::make_pair(msgGUID,
                                      Item(appData, options, *attributes)),
                       attributes->arrivalTimepoint());

        d_virtualStorageCatalog.put(msgGUID,
                                    msgSize,
                                    d_defaultRdaInfo,
                                    bmqp::Protocol::k_DEFAULT_SUBSCRIPTION_ID,
                                    mqbu::StorageKey::k_NULL_KEY);

        d_currentlyAutoConfirming = bmqt::MessageGUID();
        d_numAutoConfirms         = 0;

        if (d_queue_p) {
            d_queue_p->stats()->onEvent(
                mqbstat::QueueStatsDomain::EventType::e_ADD_MESSAGE,
                msgSize);
        }

        d_isEmpty.storeRelaxed(0);

        // We don't verify uniqueness of the insertion because in the case of a
        // proxy, it uses this inMemoryStorage, and when some upstream node
        // crashes, the primary may deliver again the same messages to us.
        return mqbi::StorageResult::e_SUCCESS;  // RETURN
    }
    // 'storageKeys' is not empty only when proxy receives PUSH.
    // Auto confirming does no apply then.

    // Specific appKeys have been specified.  Insert the guid in the
    // corresponding virtual storages.

    for (size_t i = 0; i < storageKeys.size(); ++i) {
        d_virtualStorageCatalog.put(msgGUID,
                                    msgSize,
                                    d_defaultRdaInfo,
                                    bmqp::Protocol::k_DEFAULT_SUBSCRIPTION_ID,
                                    storageKeys[i]);
    }

    // If the guid also exists in the 'physical' storage, bump up its reference
    // count by appropriate value.  Note that in-memory storage is used at the
    // proxy as well, and the way messages routed to a proxy in fanout mode,
    // the message may or may not exist in the storage.

    ItemsMapIter it = d_items.find(msgGUID);
    if (it != d_items.end()) {
        mqbi::StorageMessageAttributes& attribs = it->second.attributes();
        attribs.setRefCount(attribs.refCount() +
                            storageKeys.size());  // Bump up
    }
    else {
        d_items.insert(bsl::make_pair(msgGUID,
                                      Item(appData, options, *attributes)),
                       attributes->arrivalTimepoint());
    }

    return mqbi::StorageResult::e_SUCCESS;  // RETURN
}

mqbi::StorageResult::Enum InMemoryStorage::releaseRef(
    const bmqt::MessageGUID& msgGUID,
    const mqbu::StorageKey&  appKey,
    BSLS_ANNOTATION_UNUSED bsls::Types::Int64 timestamp,
    BSLS_ANNOTATION_UNUSED bool               onReject)
{
    ItemsMapIter it = d_items.find(msgGUID);
    if (it == d_items.end()) {
        return mqbi::StorageResult::e_GUID_NOT_FOUND;  // RETURN
    }

    if (!appKey.isNull()) {
        mqbi::StorageResult::Enum rc = d_virtualStorageCatalog.remove(msgGUID,
                                                                      appKey);
        if (mqbi::StorageResult::e_SUCCESS != rc) {
            return rc;  // RETURN
        }
    }

    unsigned int refCount = it->second.attributes().refCount();

    it->second.attributes().setRefCount(--refCount);
    if (0 == refCount) {
        return mqbi::StorageResult::e_ZERO_REFERENCES;  // RETURN
    }

    return mqbi::StorageResult::e_NON_ZERO_REFERENCES;
}

mqbi::StorageResult::Enum
InMemoryStorage::remove(const bmqt::MessageGUID& msgGUID,
                        int*                     msgSize,
                        bool                     clearAll)
{
    ItemsMapIter it = d_items.find(msgGUID);
    if (it == d_items.end()) {
        return mqbi::StorageResult::e_GUID_NOT_FOUND;  // RETURN
    }

    if (clearAll) {
        d_virtualStorageCatalog.remove(msgGUID, mqbu::StorageKey::k_NULL_KEY);
    }

    BSLS_ASSERT_SAFE(!d_virtualStorageCatalog.hasMessage(msgGUID));

    int msgLen = it->second.appData()->length();

    d_items.erase(it);

    // Update resource usage
    d_capacityMeter.remove(1, msgLen);

    if (d_queue_p) {
        d_queue_p->stats()->onEvent(
            mqbstat::QueueStatsDomain::EventType::e_DEL_MESSAGE,
            msgLen);
    }

    if (msgSize) {
        *msgSize = msgLen;
    }

    if (d_items.empty()) {
        d_isEmpty.storeRelaxed(1);
    }

    return mqbi::StorageResult::e_SUCCESS;
}

mqbi::StorageResult::Enum
InMemoryStorage::removeAll(const mqbu::StorageKey& appKey)
{
    if (appKey.isNull()) {
        // Clear the 'physical' queue, as well as all virtual storages.

        d_virtualStorageCatalog.removeAll(mqbu::StorageKey::k_NULL_KEY);
        d_items.clear();
        d_capacityMeter.clear();

        if (d_queue_p) {
            d_queue_p->stats()->onEvent(
                mqbstat::QueueStatsDomain::EventType::e_PURGE,
                0);
        }

        d_isEmpty.storeRelaxed(1);

        return mqbi::StorageResult::e_SUCCESS;  // RETURN
    }

    bsl::string appId;
    if (!d_virtualStorageCatalog.hasVirtualStorage(appKey, &appId)) {
        return mqbi::StorageResult::e_APPKEY_NOT_FOUND;  // RETURN
    }

    // A valid AppKey has been specified.  For each outstanding guid in the
    // virtual storage associated with the 'appKey', decrement its outstanding
    // refCount, and if updated refCount is zero, delete that msg from the
    // underlying (this) storage.

    bslma::ManagedPtr<mqbi::StorageIterator> iter =
        d_virtualStorageCatalog.getIterator(appKey);
    while (!iter->atEnd()) {
        const bmqt::MessageGUID& guid = iter->guid();
        ItemsMapIter             it   = d_items.find(guid);
        if (it == d_items.end()) {
            BALL_LOG_WARN
                << "#STORAGE_PURGE_ERROR "
                << "Attempting to purge GUID '" << guid
                << "' from virtual storage with appId '" << appId
                << "' & appKey '" << appKey << "' for queue '" << queueUri()
                << "' & queueKey '" << queueKey()
                << "', but GUID does not exist in the underlying storage.";
            iter->advance();
            continue;  // CONTINUE
        }

        unsigned int refCount = it->second.attributes().refCount();
        if (0 == refCount) {
            // Outstanding refCount for this message is already zero.
            BALL_LOG_WARN << "#STORAGE_PURGE_ERROR "
                          << "Attempting to purge GUID '" << guid
                          << "' from virtual storage with appId '" << appId
                          << "' & appKey '" << appKey << "' for queue '"
                          << queueUri() << "' & queueKey '" << queueKey()
                          << "], for which refCount is already zero.";
            iter->advance();
            continue;  // CONTINUE
        }
        it->second.attributes().setRefCount(--refCount);

        if (0 == refCount) {
            // This appKey was the last outstanding client for this message.
            // Message can now be deleted.

            int msgLen = it->second.appData()->length();
            d_capacityMeter.remove(1, msgLen);
            if (d_queue_p) {
                d_queue_p->queueEngine()->beforeMessageRemoved(guid);
                d_queue_p->stats()->onEvent(
                    mqbstat::QueueStatsDomain::EventType::e_DEL_MESSAGE,
                    msgLen);
            }

            // There is not really a need to remove the guid from all virtual
            // storages, because we can be here only if guid doesn't exist in
            // any virtual storage apart from the one associated with the
            // specified 'appKey' (because updated outstanding refCount is
            // zero).  So we just delete the guid from the underlying (this)
            // storage.

            d_items.erase(it);
        }

        iter->advance();
    }

    // Clear out the virtual storage associated with the specified 'appKey'.
    // Note that this cannot be done while iterating over the it in the above
    // 'while' loop for obvious reasons.
    d_virtualStorageCatalog.removeAll(appKey);

    if (d_items.empty()) {
        d_isEmpty.storeRelaxed(1);
    }

    return mqbi::StorageResult::e_SUCCESS;
}

void InMemoryStorage::dispatcherFlush(bool, bool)
{
    // NOTHING
}

int InMemoryStorage::gcExpiredMessages(
    bsls::Types::Uint64* latestMsgTimestampEpoch,
    bsls::Types::Int64*  configuredTtlValue,
    bsls::Types::Uint64  secondsFromEpoch)
{
    *configuredTtlValue = d_ttlSeconds;

    int                      numMsgsDeleted = 0;
    const bsls::Types::Int64 now   = mwcsys::Time::highResolutionTimer();
    int                      limit = k_GC_MESSAGES_BATCH_SIZE;

    for (ItemsMapIter next = d_items.begin(), cit;
         --limit && next != d_items.end();) {
        cit = next++;

        const mqbi::StorageMessageAttributes& attribs =
            cit->second.attributes();
        *latestMsgTimestampEpoch = attribs.arrivalTimestamp();

        if ((secondsFromEpoch - attribs.arrivalTimestamp()) <=
            static_cast<bsls::Types::Uint64>(d_ttlSeconds)) {
            break;  // BREAK
        }

        int msgLen = cit->second.appData()->length();
        d_capacityMeter.remove(1, msgLen);
        if (d_queue_p) {
            d_queue_p->queueEngine()->beforeMessageRemoved(cit->first);
            d_queue_p->stats()->onEvent(
                mqbstat::QueueStatsDomain::EventType::e_DEL_MESSAGE,
                msgLen);
        }

        // Remove message from all virtual storages and the physical (this)
        // storage.
        d_virtualStorageCatalog.remove(cit->first,
                                       mqbu::StorageKey::k_NULL_KEY);
        d_items.erase(cit, now);
        ++numMsgsDeleted;
    }

    if (d_queue_p && (numMsgsDeleted > 0)) {
        d_queue_p->stats()->onEvent(
            mqbstat::QueueStatsDomain::EventType::e_GC_MESSAGE,
            numMsgsDeleted);
    }

    if (d_items.empty()) {
        d_isEmpty.storeRelaxed(1);
    }

    return numMsgsDeleted;
}

bool InMemoryStorage::gcHistory()
{
    return d_items.gc(mwcsys::Time::highResolutionTimer(),
                      k_GC_MESSAGES_BATCH_SIZE);
}

void InMemoryStorage::selectForAutoConfirming(const bmqt::MessageGUID& msgGUID)
{
    d_numAutoConfirms         = 0;
    d_currentlyAutoConfirming = msgGUID;
}

mqbi::StorageResult::Enum
InMemoryStorage::autoConfirm(const mqbu::StorageKey& appKey,
                             bsls::Types::Uint64     timestamp)
{
    (void)timestamp;
    d_virtualStorageCatalog.autoConfirm(d_currentlyAutoConfirming, appKey);

    ++d_numAutoConfirms;

    return mqbi::StorageResult::e_SUCCESS;
}

// ACCESSORS
//   (virtual mqbi::Storage)
mqbi::StorageResult::Enum
InMemoryStorage::get(bsl::shared_ptr<bdlbb::Blob>*   appData,
                     bsl::shared_ptr<bdlbb::Blob>*   options,
                     mqbi::StorageMessageAttributes* attributes,
                     const bmqt::MessageGUID&        msgGUID) const
{
    ItemsMapConstIter it = d_items.find(msgGUID);
    if (it == d_items.end()) {
        return mqbi::StorageResult::e_GUID_NOT_FOUND;  // RETURN
    }

    *appData    = it->second.appData();
    *options    = it->second.options();
    *attributes = it->second.attributes();

    return mqbi::StorageResult::e_SUCCESS;
}

mqbi::StorageResult::Enum
InMemoryStorage::get(mqbi::StorageMessageAttributes* attributes,
                     const bmqt::MessageGUID&        msgGUID) const
{
    ItemsMapConstIter it = d_items.find(msgGUID);
    if (it == d_items.end()) {
        return mqbi::StorageResult::e_GUID_NOT_FOUND;  // RETURN
    }

    *attributes = it->second.attributes();
    return mqbi::StorageResult::e_SUCCESS;
}

// MANIPULATORS
//   (virtual mqbs::ReplicatedStorage)
void InMemoryStorage::processMessageRecord(
    BSLS_ANNOTATION_UNUSED const bmqt::MessageGUID&     guid,
    BSLS_ANNOTATION_UNUSED unsigned int                 msgLen,
    BSLS_ANNOTATION_UNUSED unsigned int                 refCount,
    BSLS_ANNOTATION_UNUSED const DataStoreRecordHandle& handle)
{
    // Replicated in-memory storage is not yet supported.

    BSLS_ASSERT_OPT(false && "Invalid operation on in-memory storage");
}

void InMemoryStorage::processConfirmRecord(
    BSLS_ANNOTATION_UNUSED const bmqt::MessageGUID& guid,
    BSLS_ANNOTATION_UNUSED const mqbu::StorageKey& appKey,
    BSLS_ANNOTATION_UNUSED ConfirmReason::Enum          reason,
    BSLS_ANNOTATION_UNUSED const DataStoreRecordHandle& handle)
{
    // Replicated in-memory storage is not yet supported.

    BSLS_ASSERT_OPT(false && "Invalid operation on in-memory storage");
}

void InMemoryStorage::processDeletionRecord(
    BSLS_ANNOTATION_UNUSED const bmqt::MessageGUID& guid)
{
    // Replicated in-memory storage is not yet supported.

    BSLS_ASSERT_OPT(false && "Invalid operation on in-memory storage");
}

void InMemoryStorage::addQueueOpRecordHandle(
    const DataStoreRecordHandle& handle)
{
    // In order to support at-most-once queues in a clustered setup, every node
    // in the cluster creates an in-memory storage for that queue, and every
    // node needs to keep track of its queue-creation and queue-deletion
    // records.  This routine is implemented as part of that logic.

    BSLS_ASSERT_SAFE(handle.isValid());
    d_queueOpRecordHandles.push_back(handle);
}

void InMemoryStorage::purge(
    BSLS_ANNOTATION_UNUSED const mqbu::StorageKey& appKey)
{
    // Replicated in-memory storage is not yet supported.

    BSLS_ASSERT_OPT(false && "Invalid operation on in-memory storage");
}

// ACCESSORS (for mqbs::ReplicatedStorage)
const ReplicatedStorage::RecordHandles&
InMemoryStorage::queueOpRecordHandles() const
{
    // In order to support at-most-once queues in a clustered setup, every node
    // in the cluster creates an in-memory storage for that queue, and every
    // node needs to keep track of its queue-creation and queue-deletion
    // records.  This routine is implemented as part of that logic.

    return d_queueOpRecordHandles;
}

bool InMemoryStorage::isStrongConsistency() const
{
    return false;
}

// -----------------------------
// class InMemoryStorageIterator
// -----------------------------

InMemoryStorageIterator::InMemoryStorageIterator(
    InMemoryStorage*         storage,
    const ItemsMapConstIter& initialPosition)
: d_storage_p(storage)
, d_iterator(initialPosition)
{
    // NOTHING
}

InMemoryStorageIterator::~InMemoryStorageIterator()
{
    // NOTHING
}

}  // close package namespace
}  // close enterprise namespace
