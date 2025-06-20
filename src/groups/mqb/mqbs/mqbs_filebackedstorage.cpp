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

#include <bmqma_countingallocatorstore.h>
#include <bmqsys_time.h>
#include <bmqtsk_alarmlog.h>
#include <bmqu_printutil.h>

// BDE
#include <bdlt_currenttime.h>
#include <bdlt_epochutil.h>
#include <bsl_algorithm.h>
#include <bsl_cstring.h>
#include <bsl_iostream.h>
#include <bsl_utility.h>
#include <bsla_annotations.h>
#include <bslma_allocator.h>

namespace BloombergLP {
namespace mqbs {

namespace {

const int k_GC_MESSAGES_BATCH_SIZE = 1000;  // how many to process in one run

}
// -----------------------
// class FileBackedStorage
// -----------------------

// PRIVATE MANIPULATORS
void FileBackedStorage::purgeCommon(const mqbu::StorageKey& appKey,
                                    bool                    asPrimary)
{
    // This method is common to both primary and replica nodes, when a queue or
    // a specified virtual storage is purged.  QueueEngine should not be
    // manipulated in this routine.  If 'appKey' is null, entire storage needs
    // to be purged, otherwise only the virtual storage associated with the
    // specified 'appKey'.

    if (appKey.isNull()) {
        d_virtualStorageCatalog.removeAll();
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

        d_queueStats_sp
            ->onEvent<mqbstat::QueueStatsDomain::EventType::e_PURGE>(0);
        d_queueStats_sp
            ->onEvent<mqbstat::QueueStatsDomain::EventType::e_UPDATE_HISTORY>(
                d_handles.historySize());
    }
    else {
        d_virtualStorageCatalog.removeAll(appKey, asPrimary);
    }
}

// CREATORS
FileBackedStorage::FileBackedStorage(
    DataStore*                     dataStore,
    const bmqt::Uri&               queueUri,
    const mqbu::StorageKey&        queueKey,
    mqbi::Domain*                  domain,
    bslma::Allocator*              allocator,
    bmqma::CountingAllocatorStore* allocatorStore)
: d_allocator_p(allocator)
, d_store_p(dataStore)
, d_queueKey(queueKey)
, d_config()
, d_queueUri(queueUri, allocator)
, d_virtualStorageCatalog(
      this,
      allocatorStore ? allocatorStore->get("VirtualHandles") : d_allocator_p)
, d_ttlSeconds(domain->config().messageTtl())
, d_capacityMeter(
      "queue [" + queueUri.asString() + "]",
      domain->capacityMeter(),
      allocator,
      bdlf::BindUtil::bind(&FileBackedStorage::logAppsSubscriptionInfoCb,
                           this,
                           bdlf::PlaceHolders::_1)  // stream
      )
, d_handles(bsls::TimeInterval()
                .addMilliseconds(domain->config().deduplicationTimeMs())
                .totalNanoseconds(),
            allocatorStore ? allocatorStore->get("Handles") : d_allocator_p)
, d_queueOpRecordHandles(allocator)
, d_isEmpty(1)
, d_hasReceipts(!domain->config().consistency().isStrongValue())
, d_currentlyAutoConfirming()
, d_autoConfirms(d_allocator_p)
, d_queueStats_sp()
{
    BSLS_ASSERT(d_store_p);

    // Note that the specified 'parentCapacityMeter' (and thus
    // 'd_capacityMeter.parent()') can be zero, so we can't assert on it being
    // non zero.  This is possible when a node comes up, recovers a queue,
    // creates a 'mqbblp::Domain' instance and passes that domain's capacity
    // meter to the queue's 'FileBackedStorage' instance.  Since the queue has
    // migrated, the domain instance will have a 'mqbblp::ClusterProxy'
    // instance associated with it (instead of a 'mqbblp::Cluster' instance),
    // and domain instance will return a zero capacity meter when queries to be
    // passed to the 'FileBackedStorage' instance.

    d_virtualStorageCatalog.setDefaultRda(
        domain->config().maxDeliveryAttempts());

    d_queueStats_sp.createInplace(d_allocator_p, d_allocator_p);
    d_queueStats_sp->initialize(queueUri, domain);
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
        bmqp::SchemaLearner& learner = queue()->schemaLearner();

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
    BSLS_ASSERT_SAFE(queue());

    RecordHandleMap::const_iterator it = d_handles.find(msgGUID);
    if (it == d_handles.end()) {
        return mqbi::StorageResult::e_GUID_NOT_FOUND;  // RETURN
    }

    const RecordHandlesArray& handles = it->second.d_array;
    BSLS_ASSERT(!handles.empty());
    d_store_p->loadMessageAttributesRaw(attributes, handles[0]);

    if (handles[0].primaryLeaseId() < d_store_p->primaryLeaseId()) {
        // Consider this the past that needs translation
        bmqp::SchemaLearner& learner = queue()->schemaLearner();

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

int FileBackedStorage::configure(BSLA_UNUSED bsl::ostream& errorDescription,
                                 const mqbconfm::Storage&  config,
                                 const mqbconfm::Limits&   limits,
                                 const bsls::Types::Int64  messageTtl,
                                 int                       maxDeliveryAttempts)
{
    d_config = config;
    d_capacityMeter.setLimits(limits.messages(), limits.bytes())
        .setWatermarkThresholds(limits.messagesWatermarkRatio(),
                                limits.bytesWatermarkRatio());
    d_ttlSeconds = messageTtl;

    d_virtualStorageCatalog.setDefaultRda(maxDeliveryAttempts);

    return 0;
}

void FileBackedStorage::setConsistency(const mqbconfm::Consistency& value)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(value.isEventualValue() || value.isStrongValue());

    d_hasReceipts = value.isEventualValue();
}

void FileBackedStorage::setQueue(mqbi::Queue* queue)
{
    d_virtualStorageCatalog.setQueue(queue);

    // Update queue stats if a queue has been associated with the storage.
    if (queue) {
        queue->setStats(d_queueStats_sp);

        const bsls::Types::Int64 numMessage = numMessages(
            mqbu::StorageKey::k_NULL_KEY);
        const bsls::Types::Int64 numByte = numBytes(
            mqbu::StorageKey::k_NULL_KEY);

        BALL_LOG_INFO << "Associated queue [" << queue->uri() << "] with key ["
                      << queueKey() << "] and Partition ["
                      << queue->partitionId() << "] with its storage having "
                      << bmqu::PrintUtil::prettyNumber(numMessage)
                      << " messages and "
                      << bmqu::PrintUtil::prettyNumber(numByte)
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
                       mqbi::DataStreamMessage**           out)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(appData);
    BSLS_ASSERT_SAFE(appData->length() == attributes->appDataLen());

    const int msgSize = attributes->appDataLen();

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

    if (d_autoConfirms.empty()) {
        d_virtualStorageCatalog.put(
            msgGUID,
            msgSize,
            d_virtualStorageCatalog.numVirtualStorages(),
            out);
    }
    else {
        mqbi::DataStreamMessage* dataStreamMessage = 0;
        if (out == 0) {
            out = &dataStreamMessage;
        }
        d_virtualStorageCatalog.put(
            msgGUID,
            msgSize,
            d_virtualStorageCatalog.numVirtualStorages(),
            out);

        // Move auto confirms to the data record
        for (AutoConfirms::const_iterator it = d_autoConfirms.begin();
             it != d_autoConfirms.end();
             ++it) {
            irc.first->second.d_array.push_back(it->d_confirmRecordHandle);
            d_virtualStorageCatalog.autoConfirm(*out, it->d_appKey);
        }
        d_autoConfirms.clear();
    }
    d_currentlyAutoConfirming = bmqt::MessageGUID();

    BSLS_ASSERT_SAFE(queue());
    queue()
        ->stats()
        ->onEvent<mqbstat::QueueStatsDomain::EventType::e_ADD_MESSAGE>(

            msgSize);

    d_isEmpty.storeRelaxed(0);

    return mqbi::StorageResult::e_SUCCESS;  // RETURN
}

bslma::ManagedPtr<mqbi::StorageIterator>
FileBackedStorage::getIterator(const mqbu::StorageKey& appKey)
{
    return d_virtualStorageCatalog.getIterator(appKey);
}

mqbi::StorageResult::Enum
FileBackedStorage::getIterator(bslma::ManagedPtr<mqbi::StorageIterator>* out,
                               const mqbu::StorageKey&  appKey,
                               const bmqt::MessageGUID& msgGUID)
{
    return d_virtualStorageCatalog.getIterator(out, appKey, msgGUID);
}

mqbi::StorageResult::Enum
FileBackedStorage::confirm(const bmqt::MessageGUID& msgGUID,
                           const mqbu::StorageKey&  appKey,
                           bsls::Types::Int64       timestamp,
                           bool                     onReject)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!appKey.isNull());

    RecordHandleMap::iterator it = d_handles.find(msgGUID);
    if (it == d_handles.end()) {
        return mqbi::StorageResult::e_GUID_NOT_FOUND;  // RETURN
    }

    const mqbi::StorageResult::Enum rc =
        d_virtualStorageCatalog.confirm(msgGUID, appKey);
    if (mqbi::StorageResult::e_SUCCESS != rc) {
        return rc;  // RETURN
    }

    RecordHandlesArray& handles = it->second.d_array;
    BSLS_ASSERT_SAFE(!handles.empty());

    if (0 == --it->second.d_refCount) {
        // Outstanding refCount for this message is zero now.
        // In this case we intentionally skip recording the last CONFIRM
        // due to optimization of journal file usage

        return mqbi::StorageResult::e_ZERO_REFERENCES;  // RETURN
    }

    DataStoreRecordHandle handle;
    const int             writeResult = d_store_p->writeConfirmRecord(
        &handle,
        msgGUID,
        d_queueKey,
        appKey,
        timestamp,
        onReject ? ConfirmReason::e_REJECTED : ConfirmReason::e_CONFIRMED);
    if (0 != writeResult) {
        // If 'appKey' isn't null, we have already removed 'msgGUID' from the
        // virtual storage of 'appKey'.  This is ok, because if above 'write'
        // has failed, its game over for this node anyways.

        return mqbi::StorageResult::e_WRITE_FAILURE;  // RETURN
    }

    handles.push_back(handle);

    return mqbi::StorageResult::e_NON_ZERO_REFERENCES;
}

mqbi::StorageResult::Enum
FileBackedStorage::releaseRef(const bmqt::MessageGUID& guid, bool asPrimary)
{
    RecordHandleMapIter it = d_handles.find(guid);
    if (it == d_handles.end()) {
        return mqbi::StorageResult::e_GUID_NOT_FOUND;
    }

    if (0 == it->second.d_refCount) {
        // Outstanding refCount for this message is already zero.

        return mqbi::StorageResult::e_INVALID_OPERATION;
    }

    const RecordHandlesArray& handles = it->second.d_array;
    BSLS_ASSERT_SAFE(!handles.empty());

    if (0 == --it->second.d_refCount) {
        if (asPrimary) {
            // This appKey was the last outstanding client for this message.
            // Message can now be deleted.

            unsigned int msgLen = d_store_p->getMessageLenRaw(handles[0]);

            int rc = d_store_p->writeDeletionRecord(
                guid,
                d_queueKey,
                DeletionRecordFlag::e_NONE,
                bdlt::EpochUtil::convertToTimeT64(bdlt::CurrentTime::utc()));

            if (0 != rc) {
                BMQTSK_ALARMLOG_ALARM("FILE_IO")
                    << "Partition [" << partitionId() << "] failed to write "
                    << "DELETION record for GUID: " << guid << ", for queue '"
                    << d_queueUri << "', queueKey '" << d_queueKey
                    << "' while attempting to purge the message, rc: " << rc
                    << BMQTSK_ALARMLOG_END;
            }

            // If a queue is associated, inform it about the message being
            // deleted, and update queue stats.
            // The same 'e_DEL_MESSAGE' is about 3 cases: TTL, no SC quorum,
            // and a purge.
            if (queue()) {
                queue()->queueEngine()->beforeMessageRemoved(guid);
            }
            d_queueStats_sp
                ->onEvent<mqbstat::QueueStatsDomain::EventType::e_DEL_MESSAGE>(
                    msgLen);

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

            d_queueStats_sp->onEvent<
                mqbstat::QueueStatsDomain::EventType::e_UPDATE_HISTORY>(
                d_handles.historySize());
        }

        return mqbi::StorageResult::e_ZERO_REFERENCES;
    }
    else {
        return mqbi::StorageResult::e_NON_ZERO_REFERENCES;
    }
}

mqbi::StorageResult::Enum
FileBackedStorage::remove(const bmqt::MessageGUID& msgGUID, int* msgSize)
{
    RecordHandleMap::iterator it = d_handles.find(msgGUID);
    if (it == d_handles.end()) {
        return mqbi::StorageResult::e_GUID_NOT_FOUND;  // RETURN
    }

    d_virtualStorageCatalog.remove(msgGUID);

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

    BSLS_ASSERT_SAFE(queue());
    queue()
        ->stats()
        ->onEvent<mqbstat::QueueStatsDomain::EventType::e_DEL_MESSAGE>(

            msgLen);
    queue()
        ->stats()
        ->onEvent<mqbstat::QueueStatsDomain::EventType::e_UPDATE_HISTORY>(

            d_handles.historySize());

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
    mqbi::StorageResult::Enum rc;
    const bsls::Types::Uint64 timestamp = bdlt::EpochUtil::convertToTimeT64(
        bdlt::CurrentTime::utc());

    if (!appKey.isNull()) {
        rc = d_virtualStorageCatalog.purge(
            appKey,
            bdlf::BindUtil::bind(&FileBackedStorage::writeAppPurgeRecord,
                                 this,
                                 timestamp,
                                 bdlf::PlaceHolders::_1,
                                 bdlf::PlaceHolders::_2));
        if (d_handles.empty()) {
            d_isEmpty.storeRelaxed(1);
        }
    }
    else {
        // writeQueuePurgeRecord
        rc = writePurgeRecordImpl(timestamp,
                                  mqbu::StorageKey::k_NULL_KEY,
                                  DataStoreRecordHandle());

        if (mqbi::StorageResult::e_SUCCESS == rc) {
            purgeCommon(mqbu::StorageKey::k_NULL_KEY, true);

            d_isEmpty.storeRelaxed(1);

            return mqbi::StorageResult::e_SUCCESS;  // RETURN
        }
    }

    d_queueStats_sp
        ->onEvent<mqbstat::QueueStatsDomain::EventType::e_UPDATE_HISTORY>(
            d_handles.historySize());

    return rc;
}

bool FileBackedStorage::removeVirtualStorage(const mqbu::StorageKey& appKey,
                                             bool                    asPrimary)
{
    BSLS_ASSERT_SAFE(!appKey.isNull());

    VirtualStorageCatalog::PurgeCallback  onPurge;
    VirtualStorageCatalog::RemoveCallback onRemove;

    if (asPrimary) {
        const bsls::Types::Uint64 timestamp =
            bdlt::EpochUtil::convertToTimeT64(bdlt::CurrentTime::utc());

        onPurge = bdlf::BindUtil::bind(&FileBackedStorage::writeAppPurgeRecord,
                                       this,
                                       timestamp,
                                       bdlf::PlaceHolders::_1,
                                       bdlf::PlaceHolders::_2);
        onRemove = bdlf::BindUtil::bind(
            &FileBackedStorage::writeAppDeletionRecord,
            this,
            timestamp,
            bdlf::PlaceHolders::_1);
    }

    mqbi::StorageResult::Enum rc =
        d_virtualStorageCatalog.removeVirtualStorage(appKey,
                                                     asPrimary,
                                                     onPurge,
                                                     onRemove);

    if (d_handles.empty()) {
        d_isEmpty.storeRelaxed(1);
    }

    return mqbi::StorageResult::e_SUCCESS == rc;
}

mqbi::StorageResult::Enum
FileBackedStorage::writePurgeRecordImpl(bsls::Types::Uint64         timestamp,
                                        const mqbu::StorageKey&     appKey,
                                        const DataStoreRecordHandle start)
{
    DataStoreRecordHandle handle;
    int                   rc = d_store_p->writeQueuePurgeRecord(&handle,
                                              d_queueKey,
                                              appKey,
                                              timestamp,
                                              start);

    if (0 != rc) {
        return mqbi::StorageResult::e_WRITE_FAILURE;  // RETURN
    }

    d_queueOpRecordHandles.push_back(handle);

    flushStorage();

    return mqbi::StorageResult::e_SUCCESS;
}

mqbi::StorageResult::Enum FileBackedStorage::writeAppPurgeRecord(
    const bsls::Types::Uint64                        timestamp,
    const mqbu::StorageKey&                          appKey,
    const VirtualStorageCatalog::DataStreamIterator& first)
{
    // double lookup
    RecordHandleMap::iterator itRecord = d_handles.find(first->first);
    BSLS_ASSERT_SAFE(itRecord != d_handles.end());

    DataStoreRecordHandle     start;  // !isValid()
    const RecordHandlesArray& handles = itRecord->second.d_array;
    BSLS_ASSERT(!handles.empty());
    start = handles[0];

    return writePurgeRecordImpl(timestamp, appKey, start);
}

mqbi::StorageResult::Enum
FileBackedStorage::writeAppDeletionRecord(const bsls::Types::Uint64 timestamp,
                                          const mqbu::StorageKey&   appKey)
{
    // Write QueueDeletionRecord to data store for removed appIds.
    //
    // TODO_CSL Do not write this record when we logically delete the
    // QLIST file
    DataStoreRecordHandle handle;
    int writeResult = d_store_p->writeQueueDeletionRecord(&handle,
                                                          d_queueKey,
                                                          appKey,
                                                          timestamp);
    if (0 != writeResult) {
        return mqbi::StorageResult::e_WRITE_FAILURE;  // RETURN
    }

    d_queueOpRecordHandles.push_back(handle);

    flushStorage();

    return mqbi::StorageResult::e_SUCCESS;
}

void FileBackedStorage::flushStorage()
{
    d_store_p->flushStorage();
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
    bsls::Types::Int64 now   = bmqsys::Time::highResolutionTimer();
    int                limit = k_GC_MESSAGES_BATCH_SIZE;
    bsls::Types::Int64 deduplicationTimeNs =
        queue() ? queue()->domain()->config().deduplicationTimeMs() *
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
            BMQTSK_ALARMLOG_ALARM("FILE_IO")
                << "Partition [" << partitionId() << "]"
                << " failed to write DELETION record for "
                << "GUID: " << cit->first << ", for queue '" << d_queueUri
                << "', queueKey '" << d_queueKey << "' while attempting to GC "
                << "the message due to TTL/ACK expiration, rc: " << rc
                << BMQTSK_ALARMLOG_END;
            // Do NOT remove the expired record without replicating Deletion.
            return numMsgsDeleted;  // RETURN
        }

        // If a queue is associated, inform it about the message being deleted,
        // and update queue stats.

        // The same 'e_DEL_MESSAGE' is about 3 cases: TTL, no SC quorum, purge.
        if (queue()) {
            queue()->queueEngine()->beforeMessageRemoved(cit->first);
        }
        d_queueStats_sp
            ->onEvent<mqbstat::QueueStatsDomain::EventType::e_DEL_MESSAGE>(
                msgLen);

        // Remove message from all virtual storages.
        d_virtualStorageCatalog.gc(cit->first);

        // Delete all items pointed by all handles for this GUID (i.e., delete
        // message from the underlying storage).

        for (unsigned int i = 0; i < handles.size(); ++i) {
            d_store_p->removeRecordRaw(handles[i]);
        }

        d_capacityMeter.remove(1, msgLen);
        d_handles.erase(cit, now);
        ++numMsgsDeleted;
    }

    if (numMsgsDeleted > 0) {
        if (numMsgsDeleted > numMsgsUnreceipted) {
            d_queueStats_sp
                ->onEvent<mqbstat::QueueStatsDomain::EventType::e_GC_MESSAGE>(
                    numMsgsDeleted - numMsgsUnreceipted);
        }
        if (numMsgsUnreceipted) {
            d_queueStats_sp->onEvent<
                mqbstat::QueueStatsDomain::EventType::e_NO_SC_MESSAGE>(
                numMsgsUnreceipted);
        }
        d_queueStats_sp
            ->onEvent<mqbstat::QueueStatsDomain::EventType::e_UPDATE_HISTORY>(
                d_handles.historySize());
    }

    if (d_handles.empty()) {
        d_isEmpty.storeRelaxed(1);
    }

    return numMsgsDeleted;
}

int FileBackedStorage::gcHistory(bsls::Types::Int64 now)
{
    const int rc = d_handles.gc(now, k_GC_MESSAGES_BATCH_SIZE);
    if (0 != rc) {
        d_queueStats_sp
            ->onEvent<mqbstat::QueueStatsDomain::EventType::e_UPDATE_HISTORY>(
                d_handles.historySize());
    }
    return rc;
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
                                        bmqsys::Time::highResolutionTimer());
        irc.first->second.d_array.push_back(handle);
        irc.first->second.d_refCount = refCount;

        if (d_autoConfirms.empty()) {
            d_virtualStorageCatalog.put(guid, msgLen, refCount);
        }
        else {
            if (!d_currentlyAutoConfirming.isUnset()) {
                if (d_currentlyAutoConfirming == guid) {
                    mqbi::DataStreamMessage* dataStreamMessage = 0;
                    d_virtualStorageCatalog.put(guid,
                                                msgLen,
                                                refCount +
                                                    d_autoConfirms.size(),
                                                &dataStreamMessage);

                    // Move auto confirms to the data record
                    for (AutoConfirms::const_iterator cit =
                             d_autoConfirms.begin();
                         cit != d_autoConfirms.end();
                         ++cit) {
                        irc.first->second.d_array.push_back(
                            cit->d_confirmRecordHandle);
                        d_virtualStorageCatalog.autoConfirm(dataStreamMessage,
                                                            cit->d_appKey);
                    }
                }
                else {
                    clearSelection();
                }
            }
            d_autoConfirms.clear();
        }
        d_currentlyAutoConfirming = bmqt::MessageGUID();

        // Update the messages & bytes monitors, and the stats.
        d_capacityMeter.forceCommit(1, msgLen);  // Return value ignored.

        d_queueStats_sp
            ->onEvent<mqbstat::QueueStatsDomain::EventType::e_ADD_MESSAGE>(
                msgLen);

        d_isEmpty.storeRelaxed(0);
    }
    else {
        // Received a message record for a guid for which an entry already
        // exists.  This is an error.

        BMQTSK_ALARMLOG_ALARM("REPLICATION")
            << "Partition [" << partitionId() << "]"
            << " received MESSAGE record for GUID '" << guid << "' for queue '"
            << queueUri() << "', queueKey '" << queueKey()
            << "' for which an entry already exists. Ignoring this message."
            << BMQTSK_ALARMLOG_END;
    }
}

void FileBackedStorage::processConfirmRecord(
    const bmqt::MessageGUID&     guid,
    const mqbu::StorageKey&      appKey,
    ConfirmReason::Enum          reason,
    const DataStoreRecordHandle& handle)
{
    BSLS_ASSERT_SAFE(RecordType::e_CONFIRM == handle.type());

    if (reason == ConfirmReason::e_AUTO_CONFIRMED) {
        if (d_currentlyAutoConfirming != guid) {
            if (!d_currentlyAutoConfirming.isUnset()) {
                clearSelection();
            }
            d_currentlyAutoConfirming = guid;
        }

        d_autoConfirms.emplace_back(appKey, handle);
        return;  // RETURN
    }

    RecordHandleMapIter it = d_handles.find(guid);
    if (it == d_handles.end()) {
        BMQTSK_ALARMLOG_ALARM("REPLICATION")
            << "Partition [" << partitionId() << "]"
            << " received CONFIRM record for GUID '" << guid << "' for queue '"
            << queueUri() << "', queueKey '" << queueKey()
            << "' for which no entry exists. Ignoring this message."
            << BMQTSK_ALARMLOG_END;
        return;  // RETURN
    }

    if (0 == it->second.d_refCount) {
        // Outstanding refCount for this message is already zero at this node.
        BMQTSK_ALARMLOG_ALARM("REPLICATION")
            << "Partition [" << partitionId() << "]"
            << "' received CONFIRM record for GUID '" << guid
            << "' for queue '" << queueUri() << "', queueKey '" << queueKey()
            << "' for which refCount is already zero. Ignoring this message."
            << BMQTSK_ALARMLOG_END;
        return;  // RETURN
    }

    RecordHandlesArray& handles = it->second.d_array;
    BSLS_ASSERT_SAFE(!handles.empty());
    BSLS_ASSERT_SAFE(RecordType::e_MESSAGE == handles[0].type());

    handles.push_back(handle);
    --it->second.d_refCount;  // Update outstanding refCount

    if (!appKey.isNull()) {
        const mqbi::StorageResult::Enum rc =
            d_virtualStorageCatalog.confirm(guid, appKey);
        if (mqbi::StorageResult::e_SUCCESS != rc) {
            BALL_LOG_ERROR << "#STORAGE_INVALID_CONFIRM " << "Partition ["
                           << partitionId() << "]"
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
        BMQTSK_ALARMLOG_ALARM("REPLICATION")
            << "Partition [" << partitionId() << "]"
            << " received DELETION record for GUID '" << guid
            << "' for queue '" << queueUri() << "', queueKey '" << queueKey()
            << "' for which no entry exists. Ignoring this message."
            << BMQTSK_ALARMLOG_END;
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

    if (queue()) {
        queue()->queueEngine()->beforeMessageRemoved(guid);
    }
    d_queueStats_sp
        ->onEvent<mqbstat::QueueStatsDomain::EventType::e_DEL_MESSAGE>(msgLen);

    // Delete 'guid' from all virtual storages, if any.  Note that 'guid'
    // should have already been removed from each virtual storage when confirm
    // records were received earlier for each appKey, but we remove the guid
    // again, just in case.  When the code is mature enough, we could remove
    // this.
    d_virtualStorageCatalog.remove(guid);

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

    d_queueStats_sp
        ->onEvent<mqbstat::QueueStatsDomain::EventType::e_UPDATE_HISTORY>(
            d_handles.historySize());
}

void FileBackedStorage::addQueueOpRecordHandle(
    const DataStoreRecordHandle& handle)
{
    BSLS_ASSERT_SAFE(handle.isValid());

    // The first Record must be 'e_CREATION'
#ifdef BSLS_ASSERT_SAFE_IS_ACTIVE
    if (d_queueOpRecordHandles.empty()) {
        QueueOpRecord rec;
        d_store_p->loadQueueOpRecordRaw(&rec, handle);
        BSLS_ASSERT_SAFE(QueueOpType::e_CREATION == rec.type());
    }
#endif

    d_queueOpRecordHandles.push_back(handle);
}

void FileBackedStorage::purge(const mqbu::StorageKey& appKey)
{
    purgeCommon(appKey, false);

    if (queue()) {
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

        queue()->queueEngine()->afterQueuePurged(appId, appKey);
    }
}

void FileBackedStorage::selectForAutoConfirming(
    const bmqt::MessageGUID& msgGUID)
{
    clearSelection();
    d_currentlyAutoConfirming = msgGUID;
}

mqbi::StorageResult::Enum
FileBackedStorage::autoConfirm(const mqbu::StorageKey& appKey,
                               bsls::Types::Uint64     timestamp)
{
    BSLS_ASSERT_SAFE(!appKey.isNull());
    BSLS_ASSERT_SAFE(!d_currentlyAutoConfirming.isUnset());

    DataStoreRecordHandle handle;
    int                   rc = d_store_p->writeConfirmRecord(&handle,
                                           d_currentlyAutoConfirming,
                                           d_queueKey,
                                           appKey,
                                           timestamp,
                                           ConfirmReason::e_AUTO_CONFIRMED);
    if (0 != rc) {
        return mqbi::StorageResult::e_WRITE_FAILURE;  // RETURN
    }
    d_autoConfirms.emplace_back(appKey, handle);

    return mqbi::StorageResult::e_SUCCESS;
}

void FileBackedStorage::setPrimary()
{
    d_queueStats_sp
        ->onEvent<mqbstat::QueueStatsDomain::EventType::e_CHANGE_ROLE>(
            mqbstat::QueueStatsDomain::Role::e_PRIMARY);
}

void FileBackedStorage::calibrate()
{
    d_virtualStorageCatalog.calibrate();
}

void FileBackedStorage::clearSelection()
{
    for (AutoConfirms::const_iterator it = d_autoConfirms.begin();
         it != d_autoConfirms.end();
         ++it) {
        d_store_p->removeRecordRaw(it->d_confirmRecordHandle);
    }
    d_autoConfirms.clear();

    d_currentlyAutoConfirming = bmqt::MessageGUID();
}

bsl::ostream&
FileBackedStorage::logAppsSubscriptionInfoCb(bsl::ostream& stream) const
{
    if (queue()) {
        mqbi::Storage::AppInfos appInfos(d_allocator_p);
        loadVirtualStorageDetails(&appInfos);

        for (mqbi::Storage::AppInfos::const_iterator cit = appInfos.begin();
             cit != appInfos.end();
             ++cit) {
            queue()->queueEngine()->logAppSubscriptionInfo(stream, cit->first);
        }
    }

    return stream;
}

}  // close package namespace
}  // close enterprise namespace
