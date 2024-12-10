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

// mqbs_filebackedstorage.h                                           -*-C++-*-
#ifndef INCLUDED_MQBS_FILEBACKEDSTORAGE
#define INCLUDED_MQBS_FILEBACKEDSTORAGE

//@PURPOSE: Provide a BlazingMQ storage backed by a file on disk.
//
//@CLASSES:
//  mqbs::FileBackedStorage:         BlazingMQ storage backed by a file on disk
//  mqbs::FileBackedStorageIterator: Iterator over a file-backed storage.
//
//@DESCRIPTION: 'mqbs::FileBackedStorage' provides a BlazingMQ storage backed
// by a file on disk; which can be iterated over with an
// 'mqbs::FileBackedStorageIterator'.

// MQB

#include <mqbconfm_messages.h>
#include <mqbi_storage.h>
#include <mqbs_datastore.h>
#include <mqbs_filestoreprotocol.h>
#include <mqbs_replicatedstorage.h>
#include <mqbs_virtualstoragecatalog.h>
#include <mqbu_capacitymeter.h>
#include <mqbu_storagekey.h>

// BMQ
#include <bmqp_protocol.h>
#include <bmqp_schemalearner.h>
#include <bmqt_messageguid.h>
#include <bmqt_uri.h>

#include <bmqc_array.h>
#include <bmqc_orderedhashmapwithhistory.h>

// BDE
#include <ball_log.h>
#include <bdlbb_blob.h>
#include <bsl_cstddef.h>
#include <bsl_map.h>
#include <bsl_memory.h>
#include <bsl_ostream.h>
#include <bsl_string.h>
#include <bsl_utility.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_assert.h>
#include <bsls_atomic.h>
#include <bsls_keyword.h>
#include <bsls_performancehint.h>
#include <bsls_types.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace mqbi {
class Queue;
}
namespace bmqma {
class CountingAllocatorStore;
}

namespace mqbs {

// =======================
// class FileBackedStorage
// =======================

/// Provide a BlazingMQ storage backed by a file on disk.
class FileBackedStorage BSLS_KEYWORD_FINAL : public ReplicatedStorage {
  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBS.FILEBACKEDSTORAGE");

    // FRIENDS
    friend class FileBackedStorageIterator;

    // PRIVATE CONSTANTS

    // The most probable number of records for each guid for priority queue.
    // Currently, the value is 2: one data record + one deletion record.
    // With last confirm optimization, we don't write a last confirm, and don't
    // count it here.
    // For fanout queues, the expected number of records is more than this:
    // one data record + (number of appIDs - 1) confirms + one deletion record,
    // where -1 due to last confirm optimization.
    static const size_t k_MOST_LIKELY_NUM_RECORDS = 2;

    // PRIVATE TYPES
    typedef bmqc::Array<DataStoreRecordHandle, k_MOST_LIKELY_NUM_RECORDS>
        RecordHandlesArray;

    struct Item {
        RecordHandlesArray d_array;
        unsigned int       d_refCount;  // Outstanding reference count

        void reset();
    };

    struct AutoConfirm {
        // Transient state tracking auto-confirm status for the current
        // message being replicated or put.

        const mqbu::StorageKey      d_appKey;
        const DataStoreRecordHandle d_confirmRecordHandle;

        AutoConfirm(const mqbu::StorageKey&      appKey,
                    const DataStoreRecordHandle& confirmRecordHandle);

        const mqbu::StorageKey&      appKey();
        const DataStoreRecordHandle& confirmRecordHandle();
    };

    typedef bsl::list<AutoConfirm> AutoConfirms;

  public:
    // TYPES
    typedef mqbi::Storage::AppInfos AppInfos;

    typedef ReplicatedStorage::RecordHandles RecordHandles;

  private:
    // PRIVATE TYPES

    /// Must be a container in which iteration order is same as insertion
    /// order.
    typedef bmqc::OrderedHashMapWithHistory<
        bmqt::MessageGUID,
        Item,
        bslh::Hash<bmqt::MessageGUIDHashAlgo> >
        RecordHandleMap;

    typedef RecordHandleMap::iterator RecordHandleMapIter;

    typedef RecordHandleMap::const_iterator RecordHandleMapConstIter;

    typedef bsl::pair<RecordHandleMapIter, bool> InsertRc;

    typedef bsl::shared_ptr<bdlbb::Blob> BlobSp;

    typedef mqbi::Storage::StorageKeys StorageKeys;

  private:
    // DATA
    bslma::Allocator* d_allocator_p;

    DataStore* d_store_p;

    mqbu::StorageKey d_queueKey;

    mqbconfm::Storage d_config;

    bmqt::Uri d_queueUri;

    VirtualStorageCatalog d_virtualStorageCatalog;

    bsls::Types::Int64 d_ttlSeconds;

    mqbu::CapacityMeter d_capacityMeter;

    RecordHandleMap d_handles;
    // Each value in the map is an
    // 'bmqc::Array' of type 'RecordHandles'.
    // First handle in this vector *always*
    // points to the message record.

    RecordHandles d_queueOpRecordHandles;
    // List of handles to all QueueOpRecord
    // events associated with queue of this
    // storage.  First handle in this vector
    // *always* points to QueueOpRecord of type
    // 'CREATION'.  Apart from that, it will
    // contain 0 or more QueueOpRecords of type
    // 'PURGE', 0 or more QueueOpRecords of
    // type 'ADDITION' and 0 or 1 QueueOpRecord
    // of type 'DELETION'.  Also note that
    // records of type 'ADDITION' could be
    // present only if queue is in fanout mode.

    bsls::AtomicInt d_isEmpty;
    // Flag indicating if storage is empty.
    // This flag can be checked from any
    // thread..

    bmqp::SchemaLearner::Context d_schemaLearnerContext;
    // Context for replicated data.

    bool d_hasReceipts;

    bmqt::MessageGUID d_currentlyAutoConfirming;
    // Message being evaluated and possibly auto confirmed.

    AutoConfirms d_autoConfirms;
    // Auto CONFIRMs waiting for 'put' or 'processMessageRecord'

  private:
    // NOT IMPLEMENTED
    FileBackedStorage(const FileBackedStorage&) BSLS_KEYWORD_DELETED;
    FileBackedStorage&
    operator=(const FileBackedStorage&) BSLS_KEYWORD_DELETED;

  private:
    // PRIVATE MANIPULATORS
    void purgeCommon(const mqbu::StorageKey& appKey);

    /// Clear the state created by 'selectForAutoConfirming'.
    void clearSelection();

    // PRIVATE ACCESSORS

    /// Callback function called by `d_capacityMeter` to log appllications
    /// subscription info into the specified `stream`.
    bsl::ostream& logAppsSubscriptionInfoCb(bsl::ostream& stream) const;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(FileBackedStorage,
                                   bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a storage instance for the queue identified by the specified
    /// `queueUri` and `queueKey`, backed by the specified `dataStore`, and
    /// using the specified `config`, `parentCapacityMeter`, and
    /// `allocator`.
    FileBackedStorage(DataStore*                     dataStore,
                      const bmqt::Uri&               queueUri,
                      const mqbu::StorageKey&        queueKey,
                      const mqbconfm::Domain&        config,
                      mqbu::CapacityMeter*           parentCapacityMeter,
                      bslma::Allocator*              allocator,
                      bmqma::CountingAllocatorStore* allocatorStore = 0);

    ~FileBackedStorage() BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS

    /// Return the current configuration used by this storage. The behavior
    /// is undefined unless `configure` was successfully called.
    const mqbconfm::Storage& config() const BSLS_KEYWORD_OVERRIDE;

    /// Return true if storage is backed by a persistent data store,
    /// otherwise return false.
    bool isPersistent() const BSLS_KEYWORD_OVERRIDE;

    /// Return the queue this storage is associated with.
    /// Storage exists without a queue before `setQueue`.
    mqbi::Queue* queue() const BSLS_KEYWORD_OVERRIDE;

    /// Return the URI of the queue this storage is associated with.
    const bmqt::Uri& queueUri() const BSLS_KEYWORD_OVERRIDE;

    /// Return the queueKey associated with this storage instance.
    const mqbu::StorageKey& queueKey() const BSLS_KEYWORD_OVERRIDE;

    /// Return the number of messages in the virtual storage associated with
    /// the specified `appKey`.  If `appKey` is null, number of messages in
    /// the `physical` storage is returned.  Behavior is undefined if
    /// `appKey` is non-null but no virtual storage identified with it
    /// exists.
    bsls::Types::Int64
    numMessages(const mqbu::StorageKey& appKey) const BSLS_KEYWORD_OVERRIDE;

    /// Return the number of bytes in the virtual storage associated with
    /// the specified `appKey`. If `appKey` is null, number of bytes in the
    /// `physical` storage is returned. Behavior is undefined if
    /// `appKey` is non-null but no virtual storage identified with it
    /// exists.
    bsls::Types::Int64
    numBytes(const mqbu::StorageKey& appKey) const BSLS_KEYWORD_OVERRIDE;

    /// Return true if storage is empty.  This method can be invoked from
    /// any thread.
    bool isEmpty() const BSLS_KEYWORD_OVERRIDE;

    /// Return true if this storage has message with the specified
    /// `msgGUID`, false otherwise.
    bool
    hasMessage(const bmqt::MessageGUID& msgGUID) const BSLS_KEYWORD_OVERRIDE;

    /// Retrieve the message and its metadata having the specified `msgGUID`
    /// in the specified `appData`, `options` and `attributes` from this
    /// storage.  Return zero on success or a non-zero error code on
    /// failure.
    mqbi::StorageResult::Enum
    get(bsl::shared_ptr<bdlbb::Blob>*   appData,
        bsl::shared_ptr<bdlbb::Blob>*   options,
        mqbi::StorageMessageAttributes* attributes,
        const bmqt::MessageGUID&        msgGUID) const BSLS_KEYWORD_OVERRIDE;

    /// Populate the specified `attributes` buffer with attributes of the
    /// message having the specified `msgGUID`.  Return zero on success or a
    /// non-zero error code on failure.
    mqbi::StorageResult::Enum
    get(mqbi::StorageMessageAttributes* attributes,
        const bmqt::MessageGUID&        msgGUID) const BSLS_KEYWORD_OVERRIDE;

    /// Return the number of virtual storages registered with this instance.
    int numVirtualStorages() const BSLS_KEYWORD_OVERRIDE;

    /// Return true if virtual storage identified by the specified 'appKey'
    /// exists, otherwise return false.  Load into the optionally specified
    /// 'appId' the appId associated with 'appKey' if the virtual storage
    /// exists, otherwise set it to 0.
    bool hasVirtualStorage(const mqbu::StorageKey& appKey,
                           bsl::string* appId = 0) const BSLS_KEYWORD_OVERRIDE;

    /// Return `true` if there was Replication Receipt for the specified
    /// `msgGUID`.
    bool
    hasReceipt(const bmqt::MessageGUID& msgGUID) const BSLS_KEYWORD_OVERRIDE;

    /// Return true if virtual storage identified by the specified 'appId'
    /// exists, otherwise return false.  Load into the optionally specified
    /// 'appKey' and 'ordinal' the appKey and ordinal associated with 'appId'
    /// if the virtual storage exists, otherwise set it to 0.
    bool
    hasVirtualStorage(const bsl::string& appId,
                      mqbu::StorageKey*  appKey = 0,
                      unsigned int* ordinal = 0) const BSLS_KEYWORD_OVERRIDE;

    /// Load into the specified 'buffer' the list of pairs of appId and appKey
    // for all the virtual storages registered with this instance.
    void
    loadVirtualStorageDetails(AppInfos* buffer) const BSLS_KEYWORD_OVERRIDE;

    /// Store in the specified 'msgSize' the size, in bytes, of the message
    /// having the specified 'msgGUID' if found and return success, or return
    /// a non-zero return code and leave 'msgSize' untouched if no message for
    /// the 'msgGUID' was found.
    mqbi::StorageResult::Enum getMessageSize(
        int*                     msgSize,
        const bmqt::MessageGUID& msgGUID) const BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    /// Configure this storage using the specified `config` and `limits`.
    /// Return 0 on success, or an non-zero return code and fill in a
    /// description of the error in the specified `errorDescription`
    /// otherwise.  Note that calling `configure` on an already configured
    /// storage should atomically reconfigure that storage with the new
    /// configuration (or fail and leave the storage untouched).
    int configure(bsl::ostream&            errorDescription,
                  const mqbconfm::Storage& config,
                  const mqbconfm::Limits&  limits,
                  const bsls::Types::Int64 messageTtl,
                  int maxDeliveryAttempts) BSLS_KEYWORD_OVERRIDE;

    /// Set the consistency level associated to this storage to the specified
    /// `value`.
    void
    setConsistency(const mqbconfm::Consistency& value) BSLS_KEYWORD_OVERRIDE;

    /// Return the resource capacity meter associated to this storage.
    mqbu::CapacityMeter* capacityMeter() BSLS_KEYWORD_OVERRIDE;

    void setQueue(mqbi::Queue* queue) BSLS_KEYWORD_OVERRIDE;

    /// Close this storage.
    void close() BSLS_KEYWORD_OVERRIDE;

    /// Save the message contained in the specified 'appData', 'options' and
    /// the associated 'attributes' and 'msgGUID' into this storage and the
    /// associated virtual storage.  The 'attributes' is an in/out parameter
    /// and storage layer can populate certain fields of that struct.
    /// Return 0 on success or an non-zero error code on failure.
    mqbi::StorageResult::Enum
    put(mqbi::StorageMessageAttributes*     attributes,
        const bmqt::MessageGUID&            msgGUID,
        const bsl::shared_ptr<bdlbb::Blob>& appData,
        const bsl::shared_ptr<bdlbb::Blob>& options) BSLS_KEYWORD_OVERRIDE;

    /// Get an iterator for data stored in the virtual storage identified by
    /// the specified 'appKey'.
    /// If the 'appKey' is null, the returned  iterator can iterate states of
    /// all Apps; otherwise, the iterator can iterate states of the App
    /// corresponding to the 'appKey'.
    bslma::ManagedPtr<mqbi::StorageIterator>
    getIterator(const mqbu::StorageKey& appKey) BSLS_KEYWORD_OVERRIDE;

    /// Load into the specified 'out' an iterator for data stored in the
    /// virtual storage initially pointing to the message associated with the
    /// specified 'msgGUID'.
    /// If the 'appKey' is null, the returned  iterator can iterate states of
    /// all Apps; otherwise, the iterator can iterate states of the App
    /// corresponding to the 'appKey'.
    /// Return zero on success, and a non-zero code if 'msgGUID' was not
    /// found in the storage.
    mqbi::StorageResult::Enum
    getIterator(bslma::ManagedPtr<mqbi::StorageIterator>* out,
                const mqbu::StorageKey&                   appKey,
                const bmqt::MessageGUID& msgGUID) BSLS_KEYWORD_OVERRIDE;

    /// Update the App state corresponding to the specified 'msgGUID' and the
    /// specified 'appKey' in the DataStream.  Decrement the reference count of
    /// the message identified by the 'msgGUID', and record the CONFIRM in the
    /// storage.
    /// Return one of the return codes from:
    /// * e_SUCCESS          : success
    /// * e_GUID_NOT_FOUND      : 'msgGUID' was not found
    /// * e_ZERO_REFERENCES     : message refCount has become zero
    /// * e_NON_ZERO_REFERENCES : message refCount is still not zero
    /// * e_WRITE_FAILURE       : failed to record this event in storage
    ///
    /// Behavior is undefined unless there is an App with the 'appKey'.
    ///
    /// On CONFIRM, the caller of 'confirm' is responsible to follow with
    /// 'remove' call.  'releaseRef' is an alternative way to remove message in
    /// one call.
    mqbi::StorageResult::Enum
    confirm(const bmqt::MessageGUID& msgGUID,
            const mqbu::StorageKey&  appKey,
            bsls::Types::Int64       timestamp,
            bool                     onReject = false) BSLS_KEYWORD_OVERRIDE;

    /// Decrement the reference count of the message identified by the
    /// 'msgGUID'.  If the resulting value is zero, delete the message data and
    /// record the event in the storage.
    /// Return one of the return codes from:
    /// * e_SUCCESS          : success
    /// * e_GUID_NOT_FOUND      : 'msgGUID' was not found
    /// * e_INVALID_OPERATION   : the value is invalid (already zero)
    /// * e_ZERO_REFERENCES     : message refCount has become zero
    /// * e_NON_ZERO_REFERENCE  : message refCount is still not zero
    ///
    /// On CONFIRM, the caller of 'confirm' is responsible to follow with
    /// 'remove' call.  'releaseRef' is an alternative way to remove message in
    /// one call.
    mqbi::StorageResult::Enum
    releaseRef(const bmqt::MessageGUID& msgGUID) BSLS_KEYWORD_OVERRIDE;

    /// Remove from the storage the message having the specified 'msgGUID'
    /// and store it's size, in bytes, in the optionally specified 'msgSize'.
    /// Record the event in the storage.
    /// Return 0 on success, or a non-zero return code if the 'msgGUID' was not
    /// found or if has failed to record this event in storage.
    ///
    /// On CONFIRM, the caller of 'confirm' is responsible to follow with
    /// 'remove' call.  'releaseRef' is an alternative way to remove message in
    /// one call.
    mqbi::StorageResult::Enum remove(const bmqt::MessageGUID& msgGUID,
                                     int* msgSize = 0) BSLS_KEYWORD_OVERRIDE;

    /// Remove all messages from this storage for the App identified by the
    /// specified 'appKey' if 'appKey' is not null.  Otherwise, remove messages
    /// for all Apps.  Record the event in the storage.
    /// Return one of the return codes from:
    /// * e_SUCCESS          : success
    /// * e_WRITE_FAILURE    : failed to record this event in storage
    /// * e_APPKEY_NOT_FOUND : Invalid 'appKey' specified
    mqbi::StorageResult::Enum
    removeAll(const mqbu::StorageKey& appKey) BSLS_KEYWORD_OVERRIDE;

    /// Flush any buffered replication messages to the peers.  Behaviour is
    /// undefined unless this cluster node is the primary for this partition.
    void flushStorage() BSLS_KEYWORD_OVERRIDE;

    /// Attempt to garbage-collect messages for which TTL has expired, and
    /// return the number of messages garbage-collected.  Populate the
    /// specified `latestMsgTimestampEpoch` with the timestamp, as seconds
    /// from epoch, of the latest message encountered in the iteration, and
    /// the specified `configuredTtlValue` with the TTL value (in seconds)
    /// with which this storage instance is configured.
    int gcExpiredMessages(bsls::Types::Uint64* latestMsgTimestampEpoch,
                          bsls::Types::Int64*  configuredTtlValue,
                          bsls::Types::Uint64  secondsFromEpoch)
        BSLS_KEYWORD_OVERRIDE;

    /// Garbage-collect those messages from the deduplication history which
    /// have expired the deduplication window.  Return `true`, if there are
    /// expired items unprocessed because of the batch limit.
    bool gcHistory() BSLS_KEYWORD_OVERRIDE;

    /// Create, if it doesn't exist already, a virtual storage instance with
    /// the specified `appId` and `appKey`.  Return zero upon success and a
    /// non-zero value otherwise, and populate the specified
    /// `errorDescription` with a brief reason in case of failure.  Behavior
    /// is undefined unless `appId` is non-empty and `appKey` is non-null.
    int
    addVirtualStorage(bsl::ostream&           errorDescription,
                      const bsl::string&      appId,
                      const mqbu::StorageKey& appKey) BSLS_KEYWORD_OVERRIDE;

    /// Remove the virtual storage identified by the specified `appKey`.
    /// Return true if a virtual storage with `appKey` was found and
    /// deleted, false if a virtual storage with `appKey` does not exist.
    /// Behavior is undefined unless `appKey` is non-null.  Note that this
    /// method will delete the virtual storage, and any reference to it will
    /// become invalid after this method returns.
    bool
    removeVirtualStorage(const mqbu::StorageKey& appKey) BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS (for mqbs::ReplicatedStorage)
    void processMessageRecord(const bmqt::MessageGUID&     guid,
                              unsigned int                 msgLen,
                              unsigned int                 refCount,
                              const DataStoreRecordHandle& handle)
        BSLS_KEYWORD_OVERRIDE;

    void processConfirmRecord(const bmqt::MessageGUID&     guid,
                              const mqbu::StorageKey&      appKey,
                              ConfirmReason::Enum          reason,
                              const DataStoreRecordHandle& handle)
        BSLS_KEYWORD_OVERRIDE;

    void
    processDeletionRecord(const bmqt::MessageGUID& guid) BSLS_KEYWORD_OVERRIDE;

    void addQueueOpRecordHandle(const DataStoreRecordHandle& handle)
        BSLS_KEYWORD_OVERRIDE;

    void purge(const mqbu::StorageKey& appKey) BSLS_KEYWORD_OVERRIDE;

    void selectForAutoConfirming(const bmqt::MessageGUID& msgGUID)
        BSLS_KEYWORD_OVERRIDE;
    mqbi::StorageResult::Enum
    autoConfirm(const mqbu::StorageKey& appKey,
                bsls::Types::Uint64     timestamp) BSLS_KEYWORD_OVERRIDE;
    /// The sequence of calls is 'startAutoConfirming', then zero or more
    /// 'autoConfirm', then 'put' - all for the same specified 'msgGUID'.
    /// 'autoConfirm' replicates ephemeral auto CONFIRM for the specified
    /// 'appKey' in persistent storage.
    /// Any other sequence removes auto CONFIRMs.
    /// Auto-confirmed Apps do not PUSH the message.

    // ACCESSORS (for mqbs::ReplicatedStorage)
    int partitionId() const BSLS_KEYWORD_OVERRIDE;

    const RecordHandles& queueOpRecordHandles() const BSLS_KEYWORD_OVERRIDE;

    bool isStrongConsistency() const BSLS_KEYWORD_OVERRIDE;

    /// Return the number of auto confirmed Apps for the current message.
    unsigned int numAutoConfirms() const BSLS_KEYWORD_OVERRIDE;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -----------------------
// FileBackedStorage::Item
// -----------------------

inline void FileBackedStorage::Item::reset()
{
    d_array.clear();
    d_refCount = 0;
}

inline FileBackedStorage::AutoConfirm::AutoConfirm(
    const mqbu::StorageKey&      appKey,
    const DataStoreRecordHandle& confirmRecordHandle)
: d_appKey(appKey)
, d_confirmRecordHandle(confirmRecordHandle)
{
    // NOTHING
}

inline const mqbu::StorageKey& FileBackedStorage::AutoConfirm::appKey()
{
    return d_appKey;
}

inline const DataStoreRecordHandle&
FileBackedStorage::AutoConfirm::confirmRecordHandle()
{
    return d_confirmRecordHandle;
}

// -----------------
// FileBackedStorage
// -----------------

// MANIPULATORS
inline mqbi::Queue* FileBackedStorage::queue() const
{
    return d_virtualStorageCatalog.queue();
}

inline int FileBackedStorage::addVirtualStorage(bsl::ostream& errorDescription,
                                                const bsl::string&      appId,
                                                const mqbu::StorageKey& appKey)
{
    return d_virtualStorageCatalog.addVirtualStorage(errorDescription,
                                                     appId,
                                                     appKey);
}

inline bool
FileBackedStorage::removeVirtualStorage(const mqbu::StorageKey& appKey)
{
    BSLS_ASSERT_SAFE(!appKey.isNull());

    return d_virtualStorageCatalog.removeVirtualStorage(appKey);
}

// ACCESSORS
inline const mqbconfm::Storage& FileBackedStorage::config() const
{
    return d_config;
}

inline bool FileBackedStorage::isPersistent() const
{
    return true;
}

inline mqbu::CapacityMeter* FileBackedStorage::capacityMeter()
{
    return &d_capacityMeter;
}

inline const bmqt::Uri& FileBackedStorage::queueUri() const
{
    return d_queueUri;
}

inline const mqbu::StorageKey& FileBackedStorage::queueKey() const
{
    return d_queueKey;
}

inline bsls::Types::Int64
FileBackedStorage::numMessages(const mqbu::StorageKey& appKey) const
{
    if (appKey.isNull()) {
        return d_capacityMeter.messages();  // RETURN
    }

    return d_virtualStorageCatalog.numMessages(appKey);
}

inline bsls::Types::Int64
FileBackedStorage::numBytes(const mqbu::StorageKey& appKey) const
{
    if (appKey.isNull()) {
        return d_capacityMeter.bytes();  // RETURN
    }

    return d_virtualStorageCatalog.numBytes(appKey);
}

inline bool FileBackedStorage::isEmpty() const
{
    // executed by *ANY* thread
    return 1 == d_isEmpty.loadRelaxed();
}

inline bool
FileBackedStorage::hasMessage(const bmqt::MessageGUID& msgGUID) const
{
    return 1 == d_handles.count(msgGUID);
}

inline mqbi::StorageResult::Enum
FileBackedStorage::getMessageSize(int*                     msgSize,
                                  const bmqt::MessageGUID& msgGUID) const
{
    RecordHandleMap::const_iterator it = d_handles.find(msgGUID);
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(it == d_handles.end())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        *msgSize = 0;
        return mqbi::StorageResult::e_GUID_NOT_FOUND;  // RETURN
    }

    const RecordHandlesArray& handles = it->second.d_array;
    BSLS_ASSERT_SAFE(!handles.empty());
    *msgSize = static_cast<int>(d_store_p->getMessageLenRaw(handles[0]));

    return mqbi::StorageResult::e_SUCCESS;
}

inline int FileBackedStorage::partitionId() const
{
    return d_store_p->config().partitionId();
}

inline const ReplicatedStorage::RecordHandles&
FileBackedStorage::queueOpRecordHandles() const
{
    return d_queueOpRecordHandles;
}

inline bool FileBackedStorage::isStrongConsistency() const
{
    return !d_hasReceipts;
}

inline int FileBackedStorage::numVirtualStorages() const
{
    return d_virtualStorageCatalog.numVirtualStorages();
}

inline bool
FileBackedStorage::hasVirtualStorage(const mqbu::StorageKey& appKey,
                                     bsl::string*            appId) const
{
    return d_virtualStorageCatalog.hasVirtualStorage(appKey, appId);
}

inline bool FileBackedStorage::hasVirtualStorage(const bsl::string& appId,
                                                 mqbu::StorageKey*  appKey,
                                                 unsigned int* ordinal) const
{
    return d_virtualStorageCatalog.hasVirtualStorage(appId, appKey, ordinal);
}

inline void
FileBackedStorage::loadVirtualStorageDetails(AppInfos* buffer) const
{
    return d_virtualStorageCatalog.loadVirtualStorageDetails(buffer);
}

inline unsigned int FileBackedStorage::numAutoConfirms() const
{
    return d_autoConfirms.size();
}

}  // close package namespace
}  // close enterprise namespace

#endif
