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

// mqbs_filebackedstorage.cpp                                         -*-C++-*-
#include <mqbs_filebackedstorage.h>

#include <mqbscm_version.h>
/// IMPLEMENTATION NOTES
///--------------------
//
// FileBackedStorage needs to maintain a map of guid->list(handles) because
// when a message is deleted by GUID, its MessageRecord, ConfirmRecord(s) &
// DeletionRecord need to be explicitly removed from mqbs::FileStore.d_records.
// Each GUID represents multiple records in the file store.

// MQB
#include <mqbi_domain.h>
#include <mqbi_queue.h>
#include <mqbi_queueengine.h>
#include <mqbstat_queuestats.h>

// BMQ
#include <bmqp_protocolutil.h>

// MWC
#include <mwcma_countingallocatorstore.h>
#include <mwcsys_time.h>
#include <mwctsk_alarmlog.h>
#include <mwcu_printutil.h>

// BDE
#include <bdlt_currenttime.h>
#include <bdlt_epochutil.h>
#include <bsl_algorithm.h>
#include <bsl_cstring.h>
#include <bsl_iostream.h>
#include <bsl_utility.h>
#include <bslma_allocator.h>
#include <bsls_annotation.h>

namespace BloombergLP {
namespace mqbs {

namespace {

const int k_GC_MESSAGES_BATCH_SIZE = 1000;  // how many to process in one run

}
// -----------------------
// class FileBackedStorage
// -----------------------

// PRIVATE MANIPULATORS
void FileBackedStorage::purgeCommon(const mqbu::StorageKey& appKey)
{
    // This method is common to both primary and replica nodes, when a queue or
    // a specified virtual storage is purged.  QueueEngine should not be
    // manipulated in this routine.  If 'appKey' is null, entire storage needs
    // to be purged, otherwise only the virtual storage associated with the
    // specified 'appKey'.

    d_virtualStorageCatalog.removeAll(appKey);

    if (appKey.isNull()) {
        // Remove all records from the physical storage as well.

        for (RecordHandleMapConstIter it = d_handles.begin();
             it != d_handles.end();
             ++it) {
            const RecordHandlesArray& array = it->second.d_array;
            for (unsigned int i = 0; i < array.size(); ++i) {
                d_store_p->removeRecordRaw(array[i]);
            }
        }

        d_handles.clear();

        // Update stats
        d_capacityMeter.clear();

        if (d_queue_p) {
            d_queue_p->stats()->onEvent(
                mqbstat::QueueStatsDomain::EventType::e_PURGE,
                0);
        }
    }
}

// CREATORS
FileBackedStorage::FileBackedStorage(
    DataStore*                     dataStore,
    const bmqt::Uri&               queueUri,
    const mqbu::StorageKey&        queueKey,
    const mqbconfm::Domain&        config,
    mqbu::CapacityMeter*           parentCapacityMeter,
    const bmqp::RdaInfo&           defaultRdaInfo,
    bslma::Allocator*              allocator,
    mwcma::CountingAllocatorStore* allocatorStore)
: d_allocator_p(allocator)
, d_store_p(dataStore)
, d_queue_p(0)
, d_queueKey(queueKey)
, d_config()
, d_queueUri(queueUri, allocator)
, d_virtualStorageCatalog(
      this,
      allocatorStore ? allocatorStore->get("VirtualHandles") : d_allocator_p)
, d_ttlSeconds(config.messageTtl())
, d_capacityMeter("queue [" + queueUri.asString() + "]",
                  parentCapacityMeter,
                  allocator)
, d_handles(bsls::TimeInterval()
                .addMilliseconds(config.deduplicationTimeMs())
                .totalNanoseconds(),
            allocatorStore ? allocatorStore->get("Handles") : d_allocator_p)
, d_queueOpRecordHandles(allocator)
, d_emptyAppId(allocator)
, d_nullAppKey()
, d_isEmpty(1)
, d_defaultRdaInfo(defaultRdaInfo)
, d_hasReceipts(!config.consistency().isStrongValue())
{
    BSLS_ASSERT(d_store_p);

    if (config.maxDeliveryAttempts()) {
        d_defaultRdaInfo.setCounter(config.maxDeliveryAttempts());
    }
    else {
        d_defaultRdaInfo.setUnlimited();
    }
    // Note that the specified 'parentCapacityMeter' (and thus
    // 'd_capacityMeter.parent()') can be zero, so we can't assert on it being
    // non zero.  This is possible when a node comes up, recovers a queue,
    // creates a 'mqbblp::Domain' instance and passes that domain's capacity
    // meter to the queue's 'FileBackedStorage' instance.  Since the queue has
    // migrated, the domain instance will have a 'mqbblp::ClusterProxy'
    // instance associated with it (instead of a 'mqbblp::Cluster' instance),
    // and domain instance will return a zero capacity meter when queries to be
    // passed to the 'FileBackedStorage' instance.
}

FileBackedStorage::~FileBackedStorage()
{
    // NOTHING
}

mqbi::StorageResult::Enum
FileBackedStorage::get(bsl::shared_ptr<bdlbb::Blob>*   appData,
                       bsl::shared_ptr<bdlbb::Blob>*   options,
                       mqbi::StorageMessageAttributes* attributes,
                       const bmqt::MessageGUID&        msgGUID) const
{
    RecordHandleMap::const_iterator it = d_handles.find(msgGUID);
    if (it == d_handles.end()) {
        return mqbi::StorageResult::e_GUID_NOT_FOUND;  // RETURN
    }

    const RecordHandlesArray& handles = it->second.d_array;
    BSLS_ASSERT(!handles.empty());

    d_store_p->loadMessageRaw(appData, options, attributes, handles[0]);

    if (handles[0].primaryLeaseId() < d_store_p->primaryLeaseId()) {
        // Consider this the past that needs translation
        bmqp::SchemaLearner& learner = d_queue_p->schemaLearner();

        attributes->setMessagePropertiesInfo(learner.multiplex(
            learner.createContext(handles[0].primaryLeaseId()),
            attributes->messagePropertiesInfo()));

    }  // else this record does not need the translation

    return mqbi::StorageResult::e_SUCCESS;
}

mqbi::StorageResult::Enum
FileBackedStorage::get(mqbi::StorageMessageAttributes* attributes,
                       const bmqt::MessageGUID&        msgGUID) const
{
    BSLS_ASSERT_SAFE(d_queue_p);

    RecordHandleMap::const_iterator it = d_handles.find(msgGUID);
    if (it == d_handles.end()) {
        return mqbi::StorageResult::e_GUID_NOT_FOUND;  // RETURN
    }

    const RecordHandlesArray& handles = it->second.d_array;
    BSLS_ASSERT(!handles.empty());
    d_store_p->loadMessageAttributesRaw(attributes, handles[0]);

    if (handles[0].primaryLeaseId() < d_store_p->primaryLeaseId()) {
        // Consider this the past that needs translation
        bmqp::SchemaLearner& learner = d_queue_p->schemaLearner();

        attributes->setMessagePropertiesInfo(learner.multiplex(
            learner.createContext(handles[0].primaryLeaseId()),
            attributes->messagePropertiesInfo()));

    }  // else this record does not need the translation

    return mqbi::StorageResult::e_SUCCESS;
}

bool FileBackedStorage::hasReceipt(const bmqt::MessageGUID& msgGUID) const
{
    if (d_hasReceipts) {
        // Weak consistency
        return true;  // RETURN
    }

    RecordHandleMap::const_iterator it = d_handles.find(msgGUID);
    if (it == d_handles.end()) {
        return false;  // RETURN
    }

    const RecordHandlesArray& handles = it->second.d_array;
    BSLS_ASSERT(!handles.empty());
    return d_store_p->hasReceipt(handles[0]);
}

int FileBackedStorage::configure(
    BSLS_ANNOTATION_UNUSED bsl::ostream& errorDescription,
    const mqbconfm::Storage&             config,
    const mqbconfm::Limits&              limits,
    const bsls::Types::Int64             messageTtl,
    const int                            maxDeliveryAttempts)
{
    d_config = config;
    d_capacityMeter.setLimits(limits.messages(), limits.bytes())
        .setWatermarkThresholds(limits.messagesWatermarkRatio(),
                                limits.bytesWatermarkRatio());
    d_ttlSeconds = messageTtl;

    if (maxDeliveryAttempts > 0) {
        d_defaultRdaInfo.setCounter(maxDeliveryAttempts);
    }
    else {
        d_defaultRdaInfo.setUnlimited();
    }
    return 0;
}

void FileBackedStorage::setQueue(mqbi::Queue* queue)
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
                      << queue->partitionId() << "] with its storage having "
                      << mwcu::PrintUtil::prettyNumber(numMessage)
                      << " messages and "
                      << mwcu::PrintUtil::prettyNumber(numByte)
                      << " bytes of outstanding data.";
    }
}

void FileBackedStorage::close()
{
    // NOTHING
}

mqbi::StorageResult::Enum
FileBackedStorage::put(mqbi::StorageMessageAttributes*     attributes,
                       const bmqt::MessageGUID&            msgGUID,
                       const bsl::shared_ptr<bdlbb::Blob>& appData,
                       const bsl::shared_ptr<bdlbb::Blob>& options,
                       const StorageKeys&                  storageKeys)
{
    const int msgSize = appData->length();

    if (storageKeys.empty()) {
        // Store the specified message in the 'physical' as well as *all*
        // virtual storages.

        if (d_handles.isInHistory(msgGUID)) {
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

        // Update
        DataStoreRecordHandle handle;
        int                   rc = d_store_p->writeMessageRecord(attributes,
                                               &handle,
                                               msgGUID,
                                               appData,
                                               options,
                                               d_queueKey);
        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(rc != 0)) {
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

            // Rollback reserved capacity.
            d_capacityMeter.remove(1, msgSize);
            return mqbi::StorageResult::e_WRITE_FAILURE;  // RETURN
        }

        InsertRc irc = d_handles.insert(bsl::make_pair(msgGUID, Item()),
                                        attributes->arrivalTimepoint());

        irc.first->second.d_array.push_back(handle);
        irc.first->second.d_refCount = attributes->refCount();

        // Looks like extra lookup in
        // VirtualStorageIterator::loadMessageAndAttributes() can be avoided
        // if we keep `irc` (like we keep 'DataStoreRecordHandle').
        d_virtualStorageCatalog.put(msgGUID,
                                    msgSize,
                                    d_defaultRdaInfo,
                                    bmqp::Protocol::k_DEFAULT_SUBSCRIPTION_ID,
                                    mqbu::StorageKey::k_NULL_KEY);

        BSLS_ASSERT_SAFE(d_queue_p);
        d_queue_p->stats()->onEvent(
            mqbstat::QueueStatsDomain::EventType::e_ADD_MESSAGE,
            msgSize);

        d_isEmpty.storeRelaxed(0);

        return mqbi::StorageResult::e_SUCCESS;  // RETURN
    }

    // Store the specified message only in the virtual storages identified by
    // the specified 'storageKeys'.  Note that since message is not added to
    // the 'physical' storage, we don't modify 'd_capacityMeter', 'd_isEmpty',
    // etc variables.
    BSLS_ASSERT(hasMessage(msgGUID));

    for (size_t i = 0; i < storageKeys.size(); ++i) {
        d_virtualStorageCatalog.put(msgGUID,
                                    msgSize,
                                    d_defaultRdaInfo,
                                    bmqp::Protocol::k_DEFAULT_SUBSCRIPTION_ID,
                                    storageKeys[i]);
    }

    // Note that unlike 'InMemoryStorage', we don't add the message to the
    // 'physical' storage in this case, because proxies don't use
    // FileBackedStorage.
    // TBD: this logic needs to be cleaned up.

    return mqbi::StorageResult::e_SUCCESS;  // RETURN
}

bslma::ManagedPtr<mqbi::StorageIterator>
FileBackedStorage::getIterator(const mqbu::StorageKey& appKey)
{
    if (appKey.isNull()) {
        bslma::ManagedPtr<mqbi::StorageIterator> mp(
            new (*d_allocator_p)
                FileBackedStorageIterator(this, d_handles.begin()),
            d_allocator_p);

        return mp;  // RETURN
    }

    return d_virtualStorageCatalog.getIterator(appKey);
}

mqbi::StorageResult::Enum
FileBackedStorage::getIterator(bslma::ManagedPtr<mqbi::StorageIterator>* out,
                               const mqbu::StorageKey&  appKey,
                               const bmqt::MessageGUID& msgGUID)
{
    if (appKey.isNull()) {
        RecordHandleMap::const_iterator it = d_handles.find(msgGUID);
        if (it == d_handles.end()) {
            return mqbi::StorageResult::e_GUID_NOT_FOUND;  // RETURN
        }

        out->load(new (*d_allocator_p) FileBackedStorageIterator(this, it),
                  d_allocator_p);

        return mqbi::StorageResult::e_SUCCESS;  // RETURN
    }

    return d_virtualStorageCatalog.getIterator(out, appKey, msgGUID);
}

mqbi::StorageResult::Enum
FileBackedStorage::releaseRef(const bmqt::MessageGUID& msgGUID,
                              const mqbu::StorageKey&  appKey,
                              bsls::Types::Int64       timestamp,
                              bool                     onReject)
{
    RecordHandleMap::iterator it = d_handles.find(msgGUID);
    if (it == d_handles.end()) {
        return mqbi::StorageResult::e_GUID_NOT_FOUND;  // RETURN
    }

    if (!appKey.isNull()) {
        mqbi::StorageResult::Enum rc = d_virtualStorageCatalog.remove(msgGUID,
                                                                      appKey);
        if (mqbi::StorageResult::e_SUCCESS != rc) {
            return rc;  // RETURN
        }
    }

    RecordHandlesArray& handles = it->second.d_array;
    BSLS_ASSERT_SAFE(!handles.empty());

    DataStoreRecordHandle handle;
    int                   rc = d_store_p->writeConfirmRecord(&handle,
                                           msgGUID,
                                           d_queueKey,
                                           appKey,
                                           timestamp,
                                           onReject);
    if (0 != rc) {
        // If 'appKey' isn't null, we have already removed 'msgGUID' from the
        // virtual storage of 'appKey'.  This is ok, because if above 'write'
        // has failed, its game over for this node anyways.

        return mqbi::StorageResult::e_WRITE_FAILURE;  // RETURN
    }

    handles.push_back(handle);

    if (0 == --it->second.d_refCount) {
        return mqbi::StorageResult::e_ZERO_REFERENCES;  // RETURN
    }

    return mqbi::StorageResult::e_NON_ZERO_REFERENCES;
}

mqbi::StorageResult::Enum
FileBackedStorage::remove(const bmqt::MessageGUID& msgGUID,
                          int*                     msgSize,
                          bool                     clearAll)
{
    RecordHandleMap::iterator it = d_handles.find(msgGUID);
    if (it == d_handles.end()) {
        return mqbi::StorageResult::e_GUID_NOT_FOUND;  // RETURN
    }

    if (clearAll) {
        d_virtualStorageCatalog.remove(msgGUID, mqbu::StorageKey::k_NULL_KEY);
    }

    BSLS_ASSERT_SAFE(!d_virtualStorageCatalog.hasMessage(msgGUID));

    const RecordHandlesArray& handles = it->second.d_array;
    BSLS_ASSERT_SAFE(!handles.empty());

    int msgLen = static_cast<int>(d_store_p->getMessageLenRaw(handles[0]));
    int rc     = d_store_p->writeDeletionRecord(
        msgGUID,
        d_queueKey,
        DeletionRecordFlag::e_NONE,
        bdlt::EpochUtil::convertToTimeT64(bdlt::CurrentTime::utc()));

    if (0 != rc) {
        return mqbi::StorageResult::e_WRITE_FAILURE;  // RETURN
    }

    // Delete all items pointed by all handles for this GUID.
    for (unsigned int i = 0; i < handles.size(); ++i) {
        d_store_p->removeRecordRaw(handles[i]);
    }

    // Erase entry from 'd_handles' now that all records for the GUID have been
    // deleted.
    d_handles.erase(it);

    // Update stats
    d_capacityMeter.remove(1, msgLen);

    BSLS_ASSERT_SAFE(d_queue_p);
    d_queue_p->stats()->onEvent(
        mqbstat::QueueStatsDomain::EventType::e_DEL_MESSAGE,
        msgLen);

    if (msgSize) {
        *msgSize = msgLen;
    }

    if (d_handles.empty()) {
        d_isEmpty.storeRelaxed(1);
    }

    return mqbi::StorageResult::e_SUCCESS;
}

mqbi::StorageResult::Enum
FileBackedStorage::removeAll(const mqbu::StorageKey& appKey)
{
    bsl::string appId;
    if (!appKey.isNull()) {
        if (!d_virtualStorageCatalog.hasVirtualStorage(appKey, &appId)) {
            return mqbi::StorageResult::e_APPKEY_NOT_FOUND;  // RETURN
        }
    }

    DataStoreRecordHandle handle;
    int                   rc = d_store_p->writeQueuePurgeRecord(
        &handle,
        d_queueKey,
        appKey,
        bdlt::EpochUtil::convertToTimeT64(bdlt::CurrentTime::utc()));

    if (0 != rc) {
        return mqbi::StorageResult::e_WRITE_FAILURE;  // RETURN
    }

    d_queueOpRecordHandles.push_back(handle);

    if (appKey.isNull()) {
        purgeCommon(appKey);  // or 'mqbu::StorageKey::k_NULL_KEY'
        dispatcherFlush(false);
        d_isEmpty.storeRelaxed(1);
        return mqbi::StorageResult::e_SUCCESS;  // RETURN
    }

    // A specific appKey is being purged.

    bslma::ManagedPtr<mqbi::StorageIterator> iter =
        d_virtualStorageCatalog.getIterator(appKey);
    while (!iter->atEnd()) {
        const bmqt::MessageGUID& guid = iter->guid();
        RecordHandleMapIter      it   = d_handles.find(guid);
        if (it == d_handles.end()) {
            BALL_LOG_WARN
                << "#STORAGE_PURGE_ERROR "
                << "PartitionId [" << partitionId() << "]"
                << ": Attempting to purge GUID '" << guid
                << "' from virtual storage with appId '" << appId
                << "' & appKey '" << appKey << "' for queue '" << queueUri()
                << "' & queueKey '" << queueKey()
                << "', but GUID does not exist in the underlying storage.";
            iter->advance();
            continue;  // CONTINUE
        }

        if (0 == it->second.d_refCount) {
            // Outstanding refCount for this message is already zero.

            MWCTSK_ALARMLOG_ALARM("REPLICATION")
                << "PartitionId [" << partitionId() << "]"
                << ": Attempting to purge GUID '" << guid
                << "' from virtual storage with appId '" << appId
                << "' & appKey '" << appKey << "] for queue '" << queueUri()
                << "' & queueKey '" << queueKey()
                << "', for which refCount is already zero."
                << MWCTSK_ALARMLOG_END;
            iter->advance();
            continue;  // CONTINUE
        }

        const RecordHandlesArray& handles = it->second.d_array;
        BSLS_ASSERT_SAFE(!handles.empty());

        if (0 == --it->second.d_refCount) {
            // This appKey was the last outstanding client for this message.
            // Message can now be deleted.

            int msgLen = static_cast<int>(
                d_store_p->getMessageLenRaw(handles[0]));

            rc = d_store_p->writeDeletionRecord(
                guid,
                d_queueKey,
                DeletionRecordFlag::e_NONE,
                bdlt::EpochUtil::convertToTimeT64(bdlt::CurrentTime::utc()));

            if (0 != rc) {
                MWCTSK_ALARMLOG_ALARM("FILE_IO")
                    << "PartitionId [" << partitionId() << "] failed to write "
                    << "DELETION record for GUID: " << guid << ", for queue '"
                    << d_queueUri << "', queueKey '" << d_queueKey
                    << "' while attempting to purge the message, rc: " << rc
                    << MWCTSK_ALARMLOG_END;
                iter->advance();
                continue;  // CONTINUE
            }

            // If a queue is associated, inform it about the message being
            // deleted, and update queue stats.
            // The same 'e_DEL_MESSAGE' is about 3 cases: TTL, no SC quorum,
            // and a purge.
            if (d_queue_p) {
                d_queue_p->queueEngine()->beforeMessageRemoved(guid);
                d_queue_p->stats()->onEvent(
                    mqbstat::QueueStatsDomain::EventType::e_DEL_MESSAGE,
                    msgLen);
            }

            // There is not really a need to remove the guid from all virtual
            // storages, because we can be here only if guid doesn't exist in
            // any virtual storage apart from 'vs' (because updated outstanding
            // refCount is zero).  So we just delete records associated with
            // the guid from the underlying (this) storage.

            for (unsigned int i = 0; i < handles.size(); ++i) {
                d_store_p->removeRecordRaw(handles[i]);
            }

            d_capacityMeter.remove(1, msgLen);
            d_handles.erase(it);
        }

        iter->advance();
    }

    purgeCommon(appKey);
    dispatcherFlush(false);

    if (d_handles.empty()) {
        d_isEmpty.storeRelaxed(1);
    }

    return mqbi::StorageResult::e_SUCCESS;
}

void FileBackedStorage::dispatcherFlush(bool isQueueIdle)
{
    d_store_p->dispatcherFlush(isQueueIdle);
}

int FileBackedStorage::gcExpiredMessages(
    bsls::Types::Uint64* latestMsgTimestampEpoch,
    bsls::Types::Int64*  configuredTtlValue,
    bsls::Types::Uint64  secondsFromEpoch)
{
    BSLS_ASSERT_SAFE(d_store_p);
    BSLS_ASSERT_SAFE(latestMsgTimestampEpoch);
    BSLS_ASSERT_SAFE(configuredTtlValue);

    *configuredTtlValue      = d_ttlSeconds;
    *latestMsgTimestampEpoch = 0;

    int                numMsgsDeleted     = 0;
    int                numMsgsUnreceipted = 0;
    bsls::Types::Int64 now   = mwcsys::Time::highResolutionTimer();
    int                limit = k_GC_MESSAGES_BATCH_SIZE;
    bsls::Types::Int64 deduplicationTimeNs =
        d_queue_p ? d_queue_p->domain()->config().deduplicationTimeMs() *
                        bdlt::TimeUnitRatio::k_NANOSECONDS_PER_MILLISECOND
                  : 0;

    for (RecordHandleMapIter next = d_handles.begin(), cit;
         next != d_handles.end() && --limit;) {
        cit = next++;

        const RecordHandlesArray& handles = cit->second.d_array;
        BSLS_ASSERT_SAFE(!handles.empty());

        const DataStoreRecordHandle& handle       = handles[0];
        DeletionRecordFlag::Enum     deletionFlag = DeletionRecordFlag::e_NONE;

        *latestMsgTimestampEpoch = handle.timestamp();
        if ((secondsFromEpoch - handle.timestamp()) <=
            static_cast<bsls::Types::Uint64>(d_ttlSeconds)) {
            // Current message hasn't expired and subsequent messages are only
            // "younger" (have a larger timestamp), so we can check if the SC
            // waiting for Receipts has exceeded the deduplicationTimeNs.

            // Expire if we have no quorum Receipts for longer time than
            // deduplicationTimeUs
            if (handle.hasReceipt() || deduplicationTimeNs == 0 ||
                (handle.timepoint() + deduplicationTimeNs) > now) {
                break;  // BREAK
            }
            ++numMsgsUnreceipted;
            deletionFlag = DeletionRecordFlag::e_NO_SC_QUORUM;
            // else do the same as for TTL expiration including calling
            // 'FileStore::removeRecordRaw' which will NACK if this is
            // unReceipted GUID
        }
        else {
            deletionFlag = DeletionRecordFlag::e_TTL_EXPIRATION;
        }

        int msgLen = static_cast<int>(d_store_p->getMessageLenRaw(handles[0]));
        int rc     = d_store_p->writeDeletionRecord(cit->first,
                                                d_queueKey,
                                                deletionFlag,
                                                secondsFromEpoch);
        if (0 != rc) {
            MWCTSK_ALARMLOG_ALARM("FILE_IO")
                << "PartitionId [" << partitionId() << "]"
                << " failed to write DELETION record for "
                << "GUID: " << cit->first << ", for queue '" << d_queueUri
                << "', queueKey '" << d_queueKey << "' while attempting to GC "
                << "the message due to TTL/ACK expiration, rc: " << rc
                << MWCTSK_ALARMLOG_END;
            // Do NOT remove the expired record without replicating Deletion.
            return numMsgsDeleted;  // RETURN
        }

        // If a queue is associated, inform it about the message being deleted,
        // and update queue stats.

        // The same 'e_DEL_MESSAGE' is about 3 cases: TTL, no SC quorum, purge.
        if (d_queue_p) {
            d_queue_p->queueEngine()->beforeMessageRemoved(cit->first);
            d_queue_p->stats()->onEvent(
                mqbstat::QueueStatsDomain::EventType::e_DEL_MESSAGE,
                msgLen);
        }

        // Remove message from all virtual storages.
        d_virtualStorageCatalog.remove(cit->first,
                                       mqbu::StorageKey::k_NULL_KEY);

        // Delete all items pointed by all handles for this GUID (i.e., delete
        // message from the underlying storage).

        for (unsigned int i = 0; i < handles.size(); ++i) {
            d_store_p->removeRecordRaw(handles[i]);
        }

        d_capacityMeter.remove(1, msgLen);
        d_handles.erase(cit, now);
        ++numMsgsDeleted;
    }

    if (d_queue_p && numMsgsDeleted > 0) {
        if (numMsgsDeleted > numMsgsUnreceipted) {
            d_queue_p->stats()->onEvent(
                mqbstat::QueueStatsDomain::EventType::e_GC_MESSAGE,
                numMsgsDeleted - numMsgsUnreceipted);
        }
        if (numMsgsUnreceipted) {
            d_queue_p->stats()->onEvent(
                mqbstat::QueueStatsDomain::EventType::e_NO_SC_MESSAGE,
                numMsgsUnreceipted);
        }
    }

    if (d_handles.empty()) {
        d_isEmpty.storeRelaxed(1);
    }

    return numMsgsDeleted;
}

bool FileBackedStorage::gcHistory()
{
    return d_handles.gc(mwcsys::Time::highResolutionTimer(),
                        k_GC_MESSAGES_BATCH_SIZE);
}

void FileBackedStorage::processMessageRecord(
    const bmqt::MessageGUID&     guid,
    unsigned int                 msgLen,
    unsigned int                 refCount,
    const DataStoreRecordHandle& handle)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(RecordType::e_MESSAGE == handle.type());

    RecordHandleMapIter it = d_handles.find(guid);
    if (d_handles.end() == it) {
        InsertRc irc = d_handles.insert(bsl::make_pair(guid, Item()),
                                        mwcsys::Time::highResolutionTimer());
        irc.first->second.d_array.push_back(handle);
        irc.first->second.d_refCount = refCount;

        // Add 'guid' to all virtual storages, if any.
        d_virtualStorageCatalog.put(guid,
                                    msgLen,
                                    d_defaultRdaInfo,
                                    bmqp::Protocol::k_DEFAULT_SUBSCRIPTION_ID,
                                    mqbu::StorageKey::k_NULL_KEY);

        // Update the messages & bytes monitors, and the stats.
        d_capacityMeter.forceCommit(1, msgLen);  // Return value ignored.

        if (d_queue_p) {
            d_queue_p->stats()->onEvent(
                mqbstat::QueueStatsDomain::EventType::e_ADD_MESSAGE,
                msgLen);
        }

        d_isEmpty.storeRelaxed(0);
    }
    else {
        // Received a message record for a guid for which an entry already
        // exists.  This is an error.

        MWCTSK_ALARMLOG_ALARM("REPLICATION")
            << "PartitionId [" << partitionId() << "]"
            << " received MESSAGE record for GUID '" << guid << "' for queue '"
            << queueUri() << "', queueKey '" << queueKey()
            << "' for which an entry already exists. Ignoring this message."
            << MWCTSK_ALARMLOG_END;
    }
}

void FileBackedStorage::processConfirmRecord(
    const bmqt::MessageGUID&     guid,
    const mqbu::StorageKey&      appKey,
    const DataStoreRecordHandle& handle)
{
    BSLS_ASSERT_SAFE(RecordType::e_CONFIRM == handle.type());

    RecordHandleMapIter it = d_handles.find(guid);
    if (it == d_handles.end()) {
        MWCTSK_ALARMLOG_ALARM("REPLICATION")
            << "PartitionId [" << partitionId() << "]"
            << " received CONFIRM record for GUID '" << guid << "' for queue '"
            << queueUri() << "', queueKey '" << queueKey()
            << "' for which no entry exists. Ignoring this message."
            << MWCTSK_ALARMLOG_END;
        return;  // RETURN
    }

    if (0 == it->second.d_refCount) {
        // Outstanding refCount for this message is already zero at this node.
        MWCTSK_ALARMLOG_ALARM("REPLICATION")
            << "PartitionId [" << partitionId() << "]"
            << "' received CONFIRM record for GUID '" << guid
            << "' for queue '" << queueUri() << "', queueKey '" << queueKey()
            << "' for which refCount is already zero. Ignoring this message."
            << MWCTSK_ALARMLOG_END;
        return;  // RETURN
    }

    RecordHandlesArray& handles = it->second.d_array;
    BSLS_ASSERT_SAFE(!handles.empty());
    BSLS_ASSERT_SAFE(RecordType::e_MESSAGE == handles[0].type());

    handles.push_back(handle);
    --it->second.d_refCount;  // Update outstanding refCount

    if (!appKey.isNull()) {
        mqbi::StorageResult::Enum rc = d_virtualStorageCatalog.remove(guid,
                                                                      appKey);
        if (mqbi::StorageResult::e_SUCCESS != rc) {
            BALL_LOG_ERROR << "#STORAGE_INVALID_CONFIRM "
                           << "PartitionId [" << partitionId() << "]"
                           << "' attempting to confirm GUID '" << guid
                           << "' for appKey '" << appKey
                           << "' which does not exist in its virtual storage, "
                           << "rc: " << rc << ". Queue '" << queueUri()
                           << "', queueKey '" << queueKey()
                           << "'. Ignoring this message.";
            return;  // RETURN
        }
    }
}

void FileBackedStorage::processDeletionRecord(const bmqt::MessageGUID& guid)
{
    RecordHandleMapIter it = d_handles.find(guid);
    if (it == d_handles.end()) {
        MWCTSK_ALARMLOG_ALARM("REPLICATION")
            << "PartitionId [" << partitionId() << "]"
            << " received DELETION record for GUID '" << guid
            << "' for queue '" << queueUri() << "', queueKey '" << queueKey()
            << "' for which no entry exists. Ignoring this message."
            << MWCTSK_ALARMLOG_END;
        return;  // RETURN
    }

    // Delete all handles from underlying data store.  Update stats.  Note that
    // we pass 'silentMode=true' flag to 'CapacityMeter::remove'.  This routine
    // ('FileBackedStorage::processDeletionRecord()') is only called in
    // replica nodes, and if its a storage-only node, it will not have the
    // correct domain limits, and thus on everytime invocation of
    // 'CapacityMeter::remove', a 'low watermark reached' log at WARN level
    // will be printed.  Also note that 'appKey' should be null, but we don't
    // assert it here.

    // TBD: check that outstanding refCount maintained by self is zero?

    // Update stats.
    const RecordHandlesArray& handles = it->second.d_array;
    const unsigned int        msgLen = d_store_p->getMessageLenRaw(handles[0]);

    if (d_queue_p) {
        d_queue_p->queueEngine()->beforeMessageRemoved(guid);
        d_queue_p->stats()->onEvent(
            mqbstat::QueueStatsDomain::EventType::e_DEL_MESSAGE,
            msgLen);
    }

    // Delete 'guid' from all virtual storages, if any.  Note that 'guid'
    // should have already been removed from each virtual storage when confirm
    // records were received earlier for each appKey, but we remove the guid
    // again, just in case.  When the code is mature enough, we could remove
    // this.
    d_virtualStorageCatalog.remove(guid, mqbu::StorageKey::k_NULL_KEY);

    d_capacityMeter.remove(1, msgLen, true /* silent mode; don't log */);

    // Delete all existing handles.

    for (unsigned int i = 0; i < handles.size(); ++i) {
        d_store_p->removeRecordRaw(handles[i]);
    }

    // Finally erase entry from 'd_handles' now that all records for the GUID
    // have been deleted.

    d_handles.erase(it);

    if (d_handles.empty()) {
        d_isEmpty.storeRelaxed(1);
    }
}

void FileBackedStorage::addQueueOpRecordHandle(
    const DataStoreRecordHandle& handle)
{
    BSLS_ASSERT_SAFE(handle.isValid());

#ifdef BSLS_ASSERT_SAFE_IS_ACTIVE
    if (!d_queueOpRecordHandles.empty()) {
        QueueOpRecord rec;
        d_store_p->loadQueueOpRecordRaw(&rec, d_queueOpRecordHandles[0]);
        BSLS_ASSERT_SAFE(QueueOpType::e_CREATION == rec.type());
    }
#endif

    d_queueOpRecordHandles.push_back(handle);
}

void FileBackedStorage::purge(const mqbu::StorageKey& appKey)
{
    purgeCommon(appKey);

    if (d_queue_p) {
        bsl::string appId;
        if (appKey.isNull()) {
            appId = bmqp::ProtocolUtil::k_NULL_APP_ID;
        }
        else {
            const bool rc = d_virtualStorageCatalog.hasVirtualStorage(appKey,
                                                                      &appId);
            BSLS_ASSERT_SAFE(rc);
            static_cast<void>(rc);
        }

        d_queue_p->queueEngine()->afterQueuePurged(appId, appKey);
    }
}

// -------------------------------
// class FileBackedStorageIterator
// -------------------------------

// PRIVATE MANIPULATORS
void FileBackedStorageIterator::clear()
{
    // Clear previous state, if any.  This is required so that new state can be
    // loaded in 'appData', 'options' or 'attributes' routines.
    d_appData_sp.reset();
    d_options_sp.reset();
    d_attributes.reset();
}

// PRIVATE ACCESSORS
void FileBackedStorageIterator::loadMessageAndAttributes() const
{
    BSLS_ASSERT_SAFE(!atEnd());
    if (!d_appData_sp) {
        const RecordHandlesArray& array = d_iterator->second.d_array;
        BSLS_ASSERT_SAFE(!array.empty());
        d_storage_p->d_store_p->loadMessageRaw(&d_appData_sp,
                                               &d_options_sp,
                                               &d_attributes,
                                               array[0]);
    }
}

// CREATORS
FileBackedStorageIterator::FileBackedStorageIterator()
: d_storage_p(0)
, d_iterator()
, d_attributes()
, d_appData_sp()
, d_options_sp()
{
    // NOTHING
}

FileBackedStorageIterator::FileBackedStorageIterator(
    const FileBackedStorage*        storage,
    const RecordHandleMapConstIter& initialPosition)
: d_storage_p(storage)
, d_iterator(initialPosition)
, d_attributes()
{
}

FileBackedStorageIterator::~FileBackedStorageIterator()
{
    // NOTHING
}

// MANIPULATORS
bool FileBackedStorageIterator::advance()
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!atEnd());

    clear();
    ++d_iterator;
    return !atEnd();
}

void FileBackedStorageIterator::reset()
{
    clear();

    // Reset iterator to beginning.
    d_iterator = d_storage_p->d_handles.begin();
}

// ACCESSORS
const bmqt::MessageGUID& FileBackedStorageIterator::guid() const
{
    return d_iterator->first;
}

bmqp::RdaInfo& FileBackedStorageIterator::rdaInfo() const
{
    static bmqp::RdaInfo dummy;
    return dummy;
}

unsigned int FileBackedStorageIterator::subscriptionId() const
{
    return bmqp::Protocol::k_DEFAULT_SUBSCRIPTION_ID;
}

const bsl::shared_ptr<bdlbb::Blob>& FileBackedStorageIterator::appData() const
{
    loadMessageAndAttributes();
    return d_appData_sp;
}

const bsl::shared_ptr<bdlbb::Blob>& FileBackedStorageIterator::options() const
{
    loadMessageAndAttributes();
    return d_options_sp;
}

const mqbi::StorageMessageAttributes&
FileBackedStorageIterator::attributes() const
{
    loadMessageAndAttributes();
    return d_attributes;
}

bool FileBackedStorageIterator::atEnd() const
{
    return d_iterator == d_storage_p->d_handles.end();
}

bool FileBackedStorageIterator::hasReceipt() const
{
    return atEnd() ? false : d_iterator->second.d_array[0].hasReceipt();
}

}  // close package namespace
}  // close enterprise namespace
