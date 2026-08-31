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
#include <bmqtsk_alarmlog.h>
#include <bmqu_printutil.h>
#include <bmqu_time.h>

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

/// The number of messages to remove from history on idle.
const int k_GC_HISTORY_BATCH_SIZE = 1000;
}

// -----------------------
// class FileBackedStorage
// -----------------------

// PRIVATE MANIPULATORS
void FileBackedStorage::clearAutoConfirming()
{
    // Staged auto CONFIRMs whose message is being purged have nothing left to
    // attach to.
    d_autoConfirmHandles.clear();
    d_autoConfirmApps.clear();
    d_currentlyAutoConfirming = bmqt::MessageGUID();
}

void FileBackedStorage::purgeApp(const mqbu::StorageKey& appKey)
{
    // Purges only the virtual storage of 'appKey'.  QueueEngine should not be
    // manipulated in this routine.

    BSLS_ASSERT_SAFE(!appKey.isNull());

    clearAutoConfirming();

    // The primary writes the DELETION for every message this drops to a zero
    // refCount; a replica waits for those to replicate.
    const mqbi::StorageResult::Enum rc =
        d_virtualStorageCatalog.purge(appKey, d_store_p->isLeader());
    BSLS_ASSERT_SAFE(mqbi::StorageResult::e_APPKEY_NOT_FOUND != rc);
    static_cast<void>(rc);
}

void FileBackedStorage::purgeQueue()
{
    // Purges every message of this storage.  QueueEngine should not be
    // manipulated in this routine.

    clearAutoConfirming();

    // Message by message: 'gc' is what keeps each App's numMessages/numBytes
    // right, decrementing exactly the messages an App had counted as removed,
    // so a purge of everything leaves every counter at zero -- where
    // 'resetStats' would have put them.
    for (RecordHandleMapIter it = d_handles.begin(); it != d_handles.end();) {
        const RecordHandlesArray& array = it->second->d_array;
        BSLS_ASSERT_SAFE(!array.empty());

        d_virtualStorageCatalog.gc(it->first);
        d_capacityMeter.remove(1,
                               d_store_p->getMessageLenRaw(array[0]),
                               true);  // silent

        for (unsigned int i = 0; i < array.size(); ++i) {
            d_store_p->removeRecordRaw(array[i]);
        }

        // 'erase' returns void, so step off the node first.
        RecordHandleMapIter next = it;
        ++next;
        d_handles.erase(it);
        it = next;
    }

    d_virtualStorageCatalog.stats()
        ->onEvent<mqbstat::QueueStatsDomain::EventType::e_PURGE>(0);
    d_virtualStorageCatalog.stats()
        ->onEvent<mqbstat::QueueStatsDomain::EventType::e_UPDATE_HISTORY>(
            d_handles.historySize());
}

// CREATORS
FileBackedStorage::FileBackedStorage(
    RecordStore*                   dataStore,
    const bmqt::Uri&               queueUri,
    const mqbu::StorageKey&        queueKey,
    mqbi::Domain*                  domain,
    const mqbconfm::Domain&        config,
    bslma::Allocator*              allocator,
    bmqma::CountingAllocatorStore* allocatorStore)
: d_allocator_p(allocator)
, d_store_p(dataStore)
, d_queueKey(queueKey)
, d_config()
, d_queueUri(queueUri, d_allocator_p)
, d_virtualStorageCatalog(
      this,
      allocatorStore ? allocatorStore->get("VirtualHandles") : d_allocator_p)
, d_ttlSeconds(config.messageTtl())
, d_capacityMeter(
      bsl::string("queue [", d_allocator_p) + queueUri.asString() + "]",
      domain->capacityMeter(),
      bdlf::BindUtil::bindS(d_allocator_p,
                            &FileBackedStorage::logAppsSubscriptionInfoCb,
                            this,
                            bdlf::PlaceHolders::_1),  // stream
      d_allocator_p)
, d_handles(bsls::TimeInterval()
                .addMilliseconds(config.deduplicationTimeMs())
                .totalNanoseconds(),
            allocatorStore ? allocatorStore->get("Handles") : d_allocator_p)
, d_queueOpRecordHandles(d_allocator_p)
, d_isEmpty(1)
, d_hasReceipts(!config.consistency().isStrongValue())
, d_currentlyAutoConfirming()
, d_autoConfirmHandles(d_allocator_p)
, d_autoConfirmApps(d_allocator_p)
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
    d_virtualStorageCatalog.stats()->initialize(queueUri, domain);
    d_virtualStorageCatalog.setDefaultRda(config.maxDeliveryAttempts());
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

    const RecordHandlesArray& handles = it->second->d_array;
    BSLS_ASSERT(!handles.empty());

    d_store_p->loadMessageRaw(appData, options, attributes, handles[0]);

    if (handles[0].primaryLeaseId() < d_store_p->writeHeadLeaseId()) {
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

    const RecordHandlesArray& handles = it->second->d_array;
    BSLS_ASSERT(!handles.empty());
    d_store_p->loadMessageAttributesRaw(attributes, handles[0]);

    if (handles[0].primaryLeaseId() < d_store_p->writeHeadLeaseId()) {
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
    // Require record presence and consult the record's durability, uniformly
    // for both consistency modes -- the record's 'd_hasReceipt' already
    // encodes consistency.  A normal weak-consistency write is receipted at
    // write time (its attributes default 'hasReceipt = true', carried onto the
    // record), so this still returns true immediately; a strong-consistency
    // write's record stays not-receipted until its receipt arrives.  The case
    // this newly gates is a write buffered during a rollover window: its
    // placeholder record is reserved not-yet-durable ('d_hasReceipt = false',
    // zero offset) until it drains, so it must not be reported as receipted --
    // otherwise it is delivered and its zero offset is read in
    // 'loadMessageAttributesRaw'.
    RecordHandleMap::const_iterator it = d_handles.find(msgGUID);
    if (it == d_handles.end()) {
        return false;  // RETURN
    }

    const RecordHandlesArray& handles = it->second->d_array;
    BSLS_ASSERT(!handles.empty());
    return d_store_p->hasReceipt(handles[0]);
}

void FileBackedStorage::configure(const mqbconfm::Storage& config,
                                  const mqbconfm::Limits&  limits,
                                  bsls::Types::Int64       messageTtl,
                                  int                      maxDeliveryAttempts)
{
    d_config = config;
    d_capacityMeter.setLimits(limits.messages(), limits.bytes())
        .setWatermarkThresholds(limits.messagesWatermarkRatio(),
                                limits.bytesWatermarkRatio());
    d_ttlSeconds = messageTtl;

    d_virtualStorageCatalog.setDefaultRda(maxDeliveryAttempts);
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
    BSLS_ASSERT_SAFE(static_cast<unsigned int>(appData->length()) ==
                     attributes->appDataLen());

    const int msgSize = static_cast<int>(attributes->appDataLen());

    // Store the specified message in the 'physical' as well as *all*
    // virtual storages.

    if (d_handles.isInHistory(msgGUID)) {
        return mqbi::StorageResult::e_DUPLICATE;
    }

    // Verify if we have enough capacity.  On Raft the message is not in this
    // storage yet and will not be until its record commits, so reserve rather
    // than charge: reservations count against the limit, so back-pressure
    // still accounts for messages in flight, and 'processMessageRecord'
    // converts the reservation with 'CapacityMeter::commit'.
    const bool isRaft = d_store_p->isRaft();

    const mqbu::CapacityMeter::CommitResult capacity =
        isRaft ? d_capacityMeter.tryReserve(1, msgSize)
               : d_capacityMeter.commitUnreserved(1, msgSize);

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            capacity != mqbu::CapacityMeter::e_SUCCESS)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        return (capacity == mqbu::CapacityMeter::e_LIMIT_MESSAGES
                    ? mqbi::StorageResult::e_LIMIT_MESSAGES
                    : mqbi::StorageResult::e_LIMIT_BYTES);  // RETURN
    }

    // On Raft this call only proposes.  Nothing enters this storage until the
    // record commits and 'processMessageRecord' inserts it -- the same writer
    // a replica uses.  A message the log later truncates therefore leaves no
    // trace here, which is what lets a node that loses primaryship keep
    // running: it has no storage state to roll back, and no handle into a
    // record the truncation erased.

    DataStoreRecordHandle handle;
    int                   rc = mqbi::StorageResult::e_SUCCESS;

    if (!d_autoConfirmApps.empty()) {
        // Auto confirms are journaled ahead of the message they belong to.
        for (AutoConfirmApps::const_iterator cit = d_autoConfirmApps.begin();
             cit != d_autoConfirmApps.end();
             ++cit) {
            rc = d_store_p->writeConfirmRecord(
                &handle,
                msgGUID,
                d_queueKey,
                *cit,
                attributes->arrivalTimestamp(),
                ConfirmReason::e_AUTO_CONFIRMED);

            if (0 != rc) {
                if (isRaft) {
                    d_capacityMeter.release(1, msgSize);
                }
                else {
                    // Roll back the confirms already staged for this message,
                    // and the charge taken above.  On Raft none are staged,
                    // and 'd_autoConfirmHandles' may hold an earlier
                    // message's; those the log sorts out.
                    removeAutoConfirmHandles();
                    d_capacityMeter.remove(1, msgSize);
                }
                d_autoConfirmApps.clear();
                return mqbi::StorageResult::e_WRITE_FAILURE;  // RETURN
            }

            if (!isRaft) {
                // Legacy applies below, in this same call, so stage the handle
                // now.  On Raft the commit of this record stages it, through
                // the branch a replica takes.
                processConfirmRecord(msgGUID,
                                     *cit,
                                     ConfirmReason::e_AUTO_CONFIRMED,
                                     handle,
                                     false);
            }
        }
        d_autoConfirmApps.clear();
    }

    // _After_ autoconfirms, write the PUT
    // If this write fails, the recovery process will ignore orphan confirms.
    rc = d_store_p->writeMessageRecord(attributes,
                                       &handle,
                                       msgGUID,
                                       appData,
                                       options,
                                       d_queueKey);
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(rc != 0)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        if (isRaft) {
            d_capacityMeter.release(1, msgSize);
        }
        else {
            // The auto confirms just staged have no message to attach to.  On
            // Raft nothing was staged for this message, and
            // 'd_autoConfirmHandles' may hold an earlier one's; its own commit
            // sorts them out.
            removeAutoConfirmHandles();
            d_capacityMeter.remove(1, msgSize);
        }

        return mqbi::StorageResult::e_WRITE_FAILURE;  // RETURN
    }

    if (!isRaft) {
        // Legacy has no commit point: the record is in the journal and on its
        // way to the replicas, so the message enters the storage now.  Same
        // writer, just a different moment -- a Raft partition gets here from
        // 'FileStore::onRecordCommittedPrimary' instead.
        processMessageRecord(msgGUID,
                             msgSize,
                             attributes->refCount(),
                             handle,
                             true);
    }

    // 'out' is only ever requested by 'RelayQueueEngine::storePushIfProxy',
    // which does not run against this storage.
    BSLS_ASSERT_SAFE(0 == out);

    return mqbi::StorageResult::e_SUCCESS;  // RETURN
}

void FileBackedStorage::undoCapacity(unsigned int msgLen)
{
    // A proposed message that will never commit -- the log truncated it, or
    // this node lost primaryship before it was replicated.  It never reached
    // 'processMessageRecord', so the reservation 'put' took is all there is
    // to give back.
    d_capacityMeter.release(1, msgLen);
}

void FileBackedStorage::undoConfirm(const bmqt::MessageGUID& guid,
                                    const mqbu::StorageKey&  appKey)
{
    // 'confirm' moved the App's view of this message when it wrote the
    // record.  The record will never commit, so the App holds it again: the
    // new primary still has it outstanding and will deliver it.
    d_virtualStorageCatalog.undoConfirm(guid, appKey);

    // 'guid' is the message's, and its entry is the one 'confirm' counted
    // against.  The dropped CONFIRM never made it into 'd_array'.
    RecordHandleMapIter it = d_handles.find(guid);
    if (it == d_handles.end()) {
        // A purge or a storage clear took the entry, count and all, while
        // this CONFIRM was in flight.
        return;  // RETURN
    }

    BSLS_ASSERT_SAFE(0 < it->second->d_pendingConfirms);
    --it->second->d_pendingConfirms;
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

    // The app view moves now rather than at commit.  This CONFIRM came from
    // the app's own client and the app's iterator has already passed the
    // message, so it is not state the journal owns -- and dropping the GUID
    // here is what makes a repeated CONFIRM for the same app a no-op.
    const mqbi::StorageResult::Enum rc =
        d_virtualStorageCatalog.confirm(msgGUID, appKey);
    if (mqbi::StorageResult::e_SUCCESS != rc) {
        return rc;  // RETURN
    }

    Item& item = *it->second;
    BSLS_ASSERT_SAFE(!item.d_array.empty());
    BSLS_ASSERT_SAFE(item.d_pendingConfirms < item.d_refCount);

    if (1 == item.d_refCount - item.d_pendingConfirms) {
        // Last app to confirm.  Skip recording this CONFIRM, an optimization
        // of journal file usage: the caller follows with 'remove' and that
        // DELETION stands for this CONFIRM too.
        return mqbi::StorageResult::e_ZERO_REFERENCES;  // RETURN
    }

    // 'processConfirmRecord' decrements 'd_pendingConfirms', and on a
    // single-node cluster it runs inside 'writeConfirmRecord' below:
    // 'PartitionRaft::propose' reaches quorum on its own, so 'dispatchOutput'
    // applies the entry before the write returns.  Increment before the write,
    // and decrement if the write fails.
    ++item.d_pendingConfirms;

    DataStoreRecordHandle     handle;
    const ConfirmReason::Enum reason = onReject ? ConfirmReason::e_REJECTED
                                                : ConfirmReason::e_CONFIRMED;
    const int writeResult            = d_store_p->writeConfirmRecord(&handle,
                                                          msgGUID,
                                                          d_queueKey,
                                                          appKey,
                                                          timestamp,
                                                          reason);
    if (0 != writeResult) {
        // If 'appKey' isn't null, we have already removed 'msgGUID' from the
        // virtual storage of 'appKey'.  This is ok, because if above 'write'
        // has failed, its game over for this node anyways.

        --item.d_pendingConfirms;
        return mqbi::StorageResult::e_WRITE_FAILURE;  // RETURN
    }

    if (!d_store_p->isRaft()) {
        // Legacy has no commit point: the record is in the journal and on its
        // way to the replicas, so it applies now.  Same writer, just a
        // different moment -- a Raft partition gets here from
        // 'FileStore::onRecordCommittedPrimary' instead.
        processConfirmRecord(msgGUID, appKey, reason, handle, true);
    }

    return mqbi::StorageResult::e_NON_ZERO_REFERENCES;
}

mqbi::StorageResult::Enum
FileBackedStorage::releaseRef(const bmqt::MessageGUID& guid, bool asPrimary)
{
    RecordHandleMapIter it = d_handles.find(guid);
    if (it == d_handles.end()) {
        return mqbi::StorageResult::e_GUID_NOT_FOUND;
    }

    if (0 == it->second->d_refCount) {
        // Outstanding refCount for this message is already zero.

        return mqbi::StorageResult::e_INVALID_OPERATION;
    }

    const RecordHandlesArray& handles = it->second->d_array;
    BSLS_ASSERT_SAFE(!handles.empty());

    if (0 == --it->second->d_refCount) {
        if (asPrimary) {
            // This appKey was the last outstanding client for this message.
            // Message can now be deleted.

            // Mark before the write: on a single-node cluster the record
            // commits inside it and erases this entry, after which 'it' is
            // dangling.  The mark keeps the TTL sweep from writing a second
            // DELETION while this one is in flight.
            const bool isRaft = d_store_p->isRaft();
            if (isRaft) {
                it->second->d_deletionProposedLeaseId =
                    d_store_p->writeHeadLeaseId();
            }

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

            // The message is removed when the DELETION commits, by
            // 'FileStore::onRecordCommittedPrimary' calling
            // 'processDeletionRecord'.  On a single-node cluster that is
            // inside the write above, so 'it' and 'handles' may already be
            // erased here.  Legacy has no commit, so it removes now.
            if (!isRaft) {
                removeMessage(it, 0);

                d_virtualStorageCatalog.stats()
                    ->onEvent<mqbstat::QueueStatsDomain::EventType::
                                  e_UPDATE_HISTORY>(d_handles.historySize());
            }
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

    const RecordHandlesArray& handles = it->second->d_array;
    BSLS_ASSERT_SAFE(!handles.empty());

    int msgLen = static_cast<int>(d_store_p->getMessageLenRaw(handles[0]));
    const bool isRaft = d_store_p->isRaft();

    // Mark before the write: on a single-node cluster the record commits
    // inside it and erases this entry, after which 'it' is dangling.  The mark
    // keeps the TTL sweep from writing a second DELETION while this one is in
    // flight.
    if (isRaft) {
        it->second->d_deletionProposedLeaseId = d_store_p->writeHeadLeaseId();
    }

    int rc = d_store_p->writeDeletionRecord(
        msgGUID,
        d_queueKey,
        DeletionRecordFlag::e_NONE,
        bdlt::EpochUtil::convertToTimeT64(bdlt::CurrentTime::utc()));

    if (0 != rc) {
        if (isRaft) {
            it->second->d_deletionProposedLeaseId = 0;
        }
        return mqbi::StorageResult::e_WRITE_FAILURE;  // RETURN
    }

    if (msgSize) {
        *msgSize = msgLen;
    }

    if (isRaft) {
        // Propose only.  'processDeletionRecord' removes the message when the
        // record commits, so a truncation finds nothing to undo -- neither a
        // handle into a record it erased nor a message already gone.
        return mqbi::StorageResult::e_SUCCESS;  // RETURN
    }

    d_virtualStorageCatalog.remove(msgGUID);

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
        VirtualStorageCatalog::DataStreamIterator first;

        rc = d_virtualStorageCatalog.firstMessage(&first, appKey);
        if (mqbi::StorageResult::e_SUCCESS == rc) {
            rc = writeAppPurgeRecord(timestamp, appKey, first);

            if (mqbi::StorageResult::e_SUCCESS == rc && !d_store_p->isRaft()) {
                // Legacy has no commit point, so the App is purged now.  On
                // Raft this is propose only: the purge applies when the
                // QueueOp commits, through the same writer a replica uses
                // ('applyCommittedQueueOp' -> 'purge'), so one the log later
                // truncates leaves the storage untouched.
                d_virtualStorageCatalog.purge(appKey, true);
            }
        }
        else if (mqbi::StorageResult::e_GUID_NOT_FOUND == rc) {
            // Nothing for this App to purge.
            rc = mqbi::StorageResult::e_SUCCESS;
        }

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
            if (d_store_p->isRaft()) {
                // Propose only.  The purge applies when the QueueOp commits,
                // through the same writer a replica uses
                // ('applyCommittedQueueOp' -> 'purge'), so a purge the log
                // later truncates leaves the storage untouched.  Applying it
                // here as well would purge twice, and -- since this node
                // writes messages into the storage at propose time -- would
                // also purge messages whose log position is after the purge.
                // 'onPurgeComplete' likewise runs from the apply, out of the
                // apply loop: it proposes 'e_ROLLOVER'.
                return mqbi::StorageResult::e_SUCCESS;  // RETURN
            }

            purgeQueue();
            d_store_p->onPurgeComplete();

            d_isEmpty.storeRelaxed(1);

            return mqbi::StorageResult::e_SUCCESS;  // RETURN
        }
    }

    d_virtualStorageCatalog.stats()
        ->onEvent<mqbstat::QueueStatsDomain::EventType::e_UPDATE_HISTORY>(
            d_handles.historySize());

    return rc;
}

bool FileBackedStorage::removeVirtualStorage(const mqbu::StorageKey& appKey,
                                             bool                    asPrimary)
{
    BSLS_ASSERT_SAFE(!appKey.isNull());

    // 'asPrimary' governs only whether the walk writes a DELETION for each
    // message it drops to a zero refCount.  The records that authorize the
    // removal are written by 'writeAppRemoval', on the propose side.
    const mqbi::StorageResult::Enum rc =
        d_virtualStorageCatalog.removeVirtualStorage(appKey, asPrimary);

    if (d_handles.empty()) {
        d_isEmpty.storeRelaxed(1);
    }

    return mqbi::StorageResult::e_SUCCESS == rc;
}

int FileBackedStorage::writeAppRemoval(const mqbu::StorageKey& appKey)
{
    BSLS_ASSERT_SAFE(!appKey.isNull());

    const bsls::Types::Uint64 timestamp = bdlt::EpochUtil::convertToTimeT64(
        bdlt::CurrentTime::utc());

    VirtualStorageCatalog::DataStreamIterator first;

    mqbi::StorageResult::Enum rc =
        d_virtualStorageCatalog.firstMessage(&first, appKey);
    if (mqbi::StorageResult::e_APPKEY_NOT_FOUND == rc) {
        return rc;  // RETURN
    }

    if (mqbi::StorageResult::e_SUCCESS == rc) {
        rc = writeAppPurgeRecord(timestamp, appKey, first);
        if (mqbi::StorageResult::e_SUCCESS != rc) {
            return rc;  // RETURN
        }
    }
    // else, this App can see no message; only the DELETION is needed

    rc = writeAppDeletionRecord(timestamp, appKey);

    return mqbi::StorageResult::e_SUCCESS == rc ? 0 : rc;
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
    const RecordHandlesArray& handles = itRecord->second->d_array;
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

int FileBackedStorage::gcExpiredMessages(const bdlt::Datetime& currentTimeUtc,
                                         bsls::Types::Uint64 secondsFromEpoch,
                                         int                 limit)
{
    // Executed by QUEUE dispatcher thread
    BSLS_ASSERT_SAFE(d_store_p);

    if (!d_store_p->isFileSetAvailable()) {
        return 0;
    }
    bsls::Types::Uint64 latestMsgTimestampEpoch = 0;

    int                numMsgsDeleted     = 0;
    int                numMsgsUnreceipted = 0;
    bsls::Types::Int64 now                 = bmqu::Time::highResolutionTimer();
    bsls::Types::Int64 deduplicationTimeNs = 0;
    if (queue() && queue()->domain()) {
        deduplicationTimeNs =
            queue()->domain()->config()->deduplicationTimeMs() *
            bdlt::TimeUnitRatio::k_NANOSECONDS_PER_MILLISECOND;
    }

    for (RecordHandleMapIter next = d_handles.begin(), cit;
         next != d_handles.end();) {
        if (0 == limit--) {
            // Will never be triggered if provided `limit` is negative
            break;  // BREAK
        }
        cit = next++;

        if (cit->second->wasDeletionProposed(d_store_p->writeHeadLeaseId())) {
            // A DELETION for this message is already in the log, waiting to
            // commit.  Sweeping again before it does would write a second one.
            continue;  // CONTINUE
        }

        const RecordHandlesArray& handles = cit->second->d_array;
        BSLS_ASSERT_SAFE(!handles.empty());

        const DataStoreRecordHandle& handle       = handles[0];
        DeletionRecordFlag::Enum     deletionFlag = DeletionRecordFlag::e_NONE;

        latestMsgTimestampEpoch = handle.timestamp();
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

        // Read and mark before the write: on a single-node cluster the record
        // commits inside it and 'processDeletionRecord' erases this entry,
        // after which 'cit' is no longer live.
        const bmqt::MessageGUID guid   = cit->first;
        const bool              isRaft = d_store_p->isRaft();

        if (isRaft) {
            cit->second->d_deletionProposedLeaseId =
                d_store_p->writeHeadLeaseId();
        }

        int rc = d_store_p->writeDeletionRecord(guid,
                                                d_queueKey,
                                                deletionFlag,
                                                secondsFromEpoch);
        if (0 != rc) {
            BMQTSK_ALARMLOG_ALARM("FILE_IO")
                << "Partition [" << partitionId() << "]"
                << " failed to write DELETION record for "
                << "GUID: " << guid << ", for queue '" << d_queueUri
                << "', queueKey '" << d_queueKey << "' while attempting to GC "
                << "the message due to TTL/ACK expiration, rc: " << rc
                << BMQTSK_ALARMLOG_END;
            // Nothing was written, so this message is still to be swept.
            cit->second->d_deletionProposedLeaseId = 0;

            // Do NOT remove the expired record without replicating Deletion.
            return numMsgsDeleted;  // RETURN
        }

        // The message is removed when the DELETION commits, by
        // 'FileStore::onRecordCommittedPrimary' calling
        // 'processDeletionRecord'.  Legacy has no commit, so it removes now.
        if (!isRaft) {
            removeMessage(cit, now);
        }

        ++numMsgsDeleted;
    }

    if (numMsgsDeleted > 0) {
        if (numMsgsDeleted > numMsgsUnreceipted) {
            d_virtualStorageCatalog.stats()
                ->onEvent<mqbstat::QueueStatsDomain::EventType::e_GC_MESSAGE>(
                    numMsgsDeleted - numMsgsUnreceipted);
        }
        if (numMsgsUnreceipted) {
            d_virtualStorageCatalog.stats()
                ->onEvent<
                    mqbstat::QueueStatsDomain::EventType::e_NO_SC_MESSAGE>(
                    numMsgsUnreceipted);
        }
        d_virtualStorageCatalog.stats()
            ->onEvent<mqbstat::QueueStatsDomain::EventType::e_UPDATE_HISTORY>(
                d_handles.historySize());

        BALL_LOG_INFO << d_store_p->description() << "For storage for queue ["
                      << queueUri() << "] and queueKey [" << queueKey()
                      << "] configured with TTL value of [" << d_ttlSeconds
                      << "] seconds, garbage-collected [" << numMsgsDeleted
                      << "] messages due to TTL expiration. "
                      << "Timestamp (UTC) of the latest encountered message: "
                      << bdlt::EpochUtil::convertFromTimeT64(
                             latestMsgTimestampEpoch)
                      << " (Epoch: " << latestMsgTimestampEpoch
                      << "). Current time (UTC): " << currentTimeUtc
                      << " (Epoch: " << secondsFromEpoch << ")."
                      << " Num messages remaining in the storage: "
                      << numMessages(mqbu::StorageKey::k_NULL_KEY)
                      << ". Storage type: "
                      << (isPersistent() ? "persistent." : "in-memory.");
    }

    if (d_handles.empty()) {
        d_isEmpty.storeRelaxed(1);
    }

    return numMsgsDeleted;
}

void FileBackedStorage::gcHistory(bsls::Types::Int64 now)
{
    const int rc = d_handles.gc(now, k_GC_HISTORY_BATCH_SIZE);
    if (0 != rc) {
        d_virtualStorageCatalog.stats()
            ->onEvent<mqbstat::QueueStatsDomain::EventType::e_UPDATE_HISTORY>(
                d_handles.historySize());
    }
}

void FileBackedStorage::processMessageRecord(
    const bmqt::MessageGUID&     guid,
    unsigned int                 msgLen,
    unsigned int                 refCount,
    const DataStoreRecordHandle& handle,
    bool                         isOwn)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(RecordType::e_MESSAGE == handle.type());

    RecordHandleMapIter it = d_handles.find(guid);
    if (d_handles.end() == it) {
        bsl::shared_ptr<Item> item(bsl::allocate_shared<Item>(d_allocator_p));

        InsertRc irc = d_handles.insert(bsl::make_pair(guid, item),
                                        bmqu::Time::highResolutionTimer());
        irc.first->second->d_array.push_back(handle);
        irc.first->second->d_refCount = refCount;

        bsl::shared_ptr<mqbi::DataStreamMessage> dataStreamMessage =
            d_virtualStorageCatalog.createDataStreamMessage(
                msgLen,
                refCount +
                    static_cast<unsigned int>(d_autoConfirmHandles.size()));

        if (!d_autoConfirmHandles.empty()) {
            if (!d_currentlyAutoConfirming.isUnset()) {
                if (d_currentlyAutoConfirming == guid) {
                    d_virtualStorageCatalog.setup(dataStreamMessage.get());

                    // Move auto confirms to the data record
                    for (AutoConfirmHandles::const_iterator cit =
                             d_autoConfirmHandles.begin();
                         cit != d_autoConfirmHandles.end();
                         ++cit) {
                        irc.first->second->d_array.push_back(
                            cit->d_confirmRecordHandle);
                        d_virtualStorageCatalog.autoConfirm(
                            dataStreamMessage.get(),
                            cit->d_appKey);
                    }
                }
                else {
                    removeAutoConfirmHandles();
                }
            }
            d_autoConfirmHandles.clear();
        }
        d_currentlyAutoConfirming = bmqt::MessageGUID();

        d_virtualStorageCatalog.insert(guid, dataStreamMessage);

        // Update the messages & bytes monitors, and the stats.  A record this
        // node did not write reserved nothing, so it is charged outright.  One
        // it did write was accounted for in 'put': on Raft as a reservation,
        // which 'commit' now converts, and on legacy as a charge already
        // taken by 'commitUnreserved'.
        if (!isOwn) {
            d_capacityMeter.forceCommit(1, msgLen);
        }
        else if (d_store_p->isRaft()) {
            d_capacityMeter.commit(1, msgLen);
        }

        d_virtualStorageCatalog.stats()
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
    const DataStoreRecordHandle& handle,
    bool                         isOwn)
{
    BSLS_ASSERT_SAFE(RecordType::e_CONFIRM == handle.type());

    if (reason == ConfirmReason::e_AUTO_CONFIRMED) {
        if (d_currentlyAutoConfirming != guid) {
            if (!d_currentlyAutoConfirming.isUnset()) {
                removeAutoConfirmHandles();
            }
            d_currentlyAutoConfirming = guid;
        }

        d_autoConfirmHandles.emplace_back(appKey, handle);
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

    if (0 == it->second->d_refCount) {
        // Outstanding refCount for this message is already zero at this node.
        BMQTSK_ALARMLOG_ALARM("REPLICATION")
            << "Partition [" << partitionId() << "]"
            << "' received CONFIRM record for GUID '" << guid
            << "' for queue '" << queueUri() << "', queueKey '" << queueKey()
            << "' for which refCount is already zero. Ignoring this message."
            << BMQTSK_ALARMLOG_END;
        return;  // RETURN
    }

    RecordHandlesArray& handles = it->second->d_array;
    BSLS_ASSERT_SAFE(!handles.empty());
    BSLS_ASSERT_SAFE(RecordType::e_MESSAGE == handles[0].type());

    handles.push_back(handle);
    --it->second->d_refCount;  // Update outstanding refCount

    if (isOwn) {
        BSLS_ASSERT_SAFE(0 < it->second->d_pendingConfirms);
        --it->second->d_pendingConfirms;

        // 'confirm' moved the app view when it wrote this record.
        return;  // RETURN
    }

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

    // 'silentCapacity' is true here: a replica may be a storage-only node,
    // where 'configure' never ran and the domain limits are not the correct
    // ones, so every removal would log a 'low watermark reached' WARN.
    removeMessage(it, 0);

    d_virtualStorageCatalog.stats()
        ->onEvent<mqbstat::QueueStatsDomain::EventType::e_UPDATE_HISTORY>(
            d_handles.historySize());
}

void FileBackedStorage::removeMessage(RecordHandleMapIter it,
                                      bsls::Types::Int64  now)
{
    // The apply half of a DELETION record: takes the message out of every
    // container holding it, writing nothing.  Note that 'appKey' should be
    // null, but we don't assert it here.

    // TBD: check that outstanding refCount maintained by self is zero?

    const bmqt::MessageGUID&  guid    = it->first;
    const RecordHandlesArray& handles = it->second->d_array;
    const unsigned int        msgLen = d_store_p->getMessageLenRaw(handles[0]);

    if (queue()) {
        queue()->queueEngine()->beforeMessageRemoved(guid);
    }
    d_virtualStorageCatalog.stats()
        ->onEvent<mqbstat::QueueStatsDomain::EventType::e_DEL_MESSAGE>(msgLen);

    // Delete 'guid' from all virtual storages, if any.
    // Note that we call `gc`, not `remove`, because we want to update
    // message/byte counters.  We don't replicate the last confirm and
    // REPLICA needs to find appId that was implicitly confirmed and update it.
    d_virtualStorageCatalog.gc(guid);

    d_capacityMeter.remove(1, msgLen, !d_store_p->isLeader());

    // Delete all existing handles.

    for (unsigned int i = 0; i < handles.size(); ++i) {
        d_store_p->removeRecordRaw(handles[i]);
    }

    // Finally erase entry from 'd_handles' now that all records for the GUID
    // have been deleted.  A caller that already holds the current time passes
    // it, which skips history for an entry whose retention has elapsed rather
    // than leaving that to the map's own 'gc'; zero leaves it to 'gc'.

    d_handles.erase(it, now);

    if (d_handles.empty()) {
        d_isEmpty.storeRelaxed(1);
    }
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
    bsl::string appId;

    if (appKey.isNull()) {
        purgeQueue();

        // The propose path used to do this inline; on a Raft partition the
        // whole-queue purge applies here instead.
        d_isEmpty.storeRelaxed(d_handles.empty() ? 1 : 0);

        appId = bmqp::ProtocolUtil::k_NULL_APP_ID;
    }
    else {
        purgeApp(appKey);

        const bool rc = d_virtualStorageCatalog.hasVirtualStorage(appKey,
                                                                  &appId);
        BSLS_ASSERT_SAFE(rc);
        static_cast<void>(rc);
    }

    if (queue()) {
        queue()->queueEngine()->afterQueuePurged(appId, appKey);
    }
}

void FileBackedStorage::selectForAutoConfirming(
    BSLA_MAYBE_UNUSED const bmqt::MessageGUID& msgGUID)
{
    // 'put' writes the records for 'd_autoConfirmApps' under its own
    // 'msgGUID', and 'd_currentlyAutoConfirming' names the message whose
    // records are staged, which is a commit-side question.  So the propose
    // side needs nothing but the app list.
    d_autoConfirmApps.clear();
}

void FileBackedStorage::autoConfirm(const mqbu::StorageKey& appKey)
{
    BSLS_ASSERT_SAFE(!appKey.isNull());

    d_autoConfirmApps.emplace_back(appKey);
}

void FileBackedStorage::setPrimary()
{
    d_virtualStorageCatalog.stats()
        ->onEvent<mqbstat::QueueStatsDomain::EventType::e_CHANGE_ROLE>(
            mqbstat::QueueStatsDomain::Role::e_PRIMARY);
}

void FileBackedStorage::calibrate()
{
    // use this event as another trigger to clear orphan confirms
    removeAutoConfirmHandles();

    d_virtualStorageCatalog.calibrate();
}

void FileBackedStorage::removeAutoConfirmHandles()
{
    for (AutoConfirmHandles::const_iterator it = d_autoConfirmHandles.begin();
         it != d_autoConfirmHandles.end();
         ++it) {
        d_store_p->removeRecordRaw(it->d_confirmRecordHandle);
    }
    d_autoConfirmHandles.clear();

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
