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

// mqbs_inmemorystorage.h                                             -*-C++-*-
#ifndef INCLUDED_MQBS_INMEMORYSTORAGE
#define INCLUDED_MQBS_INMEMORYSTORAGE

//@PURPOSE: Provide BlazingMQ storage implementation storing data in memory.
//
//@CLASSES:
//  mqbs::InMemoryStorage:         BlazingMQ storage in memory.
//  mqbs::InMemoryStorageIterator: Iterator over in-memory storage.
//
//@DESCRIPTION: 'mqbs::InMemoryStorage' provide an implementation of the
// 'mqbi::Storage' protocol that stores all data and associated metadata in
// memory.  'mqbs::InMemoryStorageIterator' provides an iterator implementation
// of 'mqbi::StorageIterator' protocol and can be used to iterate over messages
// stored in the in-memory storage.

// MQB

#include <mqbconfm_messages.h>
#include <mqbi_storage.h>
#include <mqbs_replicatedstorage.h>
#include <mqbs_virtualstoragecatalog.h>
#include <mqbu_capacitymeter.h>
#include <mqbu_storagekey.h>

// BMQ
#include <bmqt_messageguid.h>
#include <bmqt_uri.h>

#include <bmqc_orderedhashmapwithhistory.h>

// BDE
#include <ball_log.h>
#include <bdlbb_blob.h>
#include <bsl_map.h>
#include <bsl_memory.h>
#include <bsl_ostream.h>
#include <bsl_string.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
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

// FORWARD DECLARATION
class InMemoryStorageIterator;

// ==========================
// class InMemoryStorage_Item
// ==========================

/// This class provides a VST which represents an item stored in the
/// in-memory storage.  This class is an implementation detail of
/// `mqbs::InMemoryStorage`, and should not be used outside this component.
class InMemoryStorage_Item {
  private:
    // DATA
    bsl::shared_ptr<bdlbb::Blob> d_appData;

    bsl::shared_ptr<bdlbb::Blob> d_options;

    mqbi::StorageMessageAttributes d_attributes;

  public:
    // CREATORS
    InMemoryStorage_Item();

    InMemoryStorage_Item(const bsl::shared_ptr<bdlbb::Blob>&   appData,
                         const bsl::shared_ptr<bdlbb::Blob>&   options,
                         const mqbi::StorageMessageAttributes& attributes);

    // MANIPULATORS
    InMemoryStorage_Item&
    setAppData(const bsl::shared_ptr<bdlbb::Blob>& value);
    InMemoryStorage_Item&
    setOptions(const bsl::shared_ptr<bdlbb::Blob>& value);
    InMemoryStorage_Item&
    setAttributes(const mqbi::StorageMessageAttributes& value);
    mqbi::StorageMessageAttributes& attributes();

    void reset();

    // ACCESSORS
    const bsl::shared_ptr<bdlbb::Blob>&   appData() const;
    const bsl::shared_ptr<bdlbb::Blob>&   options() const;
    const mqbi::StorageMessageAttributes& attributes() const;
};

// =====================
// class InMemoryStorage
// =====================

class InMemoryStorage BSLS_KEYWORD_FINAL : public ReplicatedStorage {
    // TBD

  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBS.INMEMORYSTORAGE");

    // FRIENDS
    friend class InMemoryStorageIterator;

    // PRIVATE TYPES
    typedef InMemoryStorage_Item Item;

    /// Must be a container in which iteration order is same as insertion
    /// order.
    typedef bmqc::OrderedHashMapWithHistory<
        bmqt::MessageGUID,
        Item,
        bslh::Hash<bmqt::MessageGUIDHashAlgo> >
        ItemsMap;

    typedef ItemsMap::iterator ItemsMapIter;

    typedef ItemsMap::const_iterator ItemsMapConstIter;

    typedef mqbi::Storage::StorageKeys StorageKeys;

    struct AutoConfirm {
        const mqbu::StorageKey d_appKey;

        AutoConfirm(const mqbu::StorageKey& appKey);

        const mqbu::StorageKey& appKey();
    };

    typedef bsl::list<AutoConfirm> AutoConfirms;

  public:
    // CLASS METHODS

    /// Factory method to create a new `InMemoryStorage` in the specified
    /// `out`, using the specified `allocator`.
    static void factoryMethod(bslma::ManagedPtr<mqbi::Storage>* out,
                              bslma::Allocator*                 allocator);

  public:
    // TYPES
    typedef mqbi::Storage::AppInfo AppInfo;

    typedef mqbi::Storage::AppInfos AppInfos;

    typedef ReplicatedStorage::RecordHandles RecordHandles;

  private:
    // DATA
    bslma::Allocator* d_allocator_p;

    mqbu::StorageKey d_key;

    bmqt::Uri d_uri;

    int d_partitionId;

    mqbconfm::Storage d_config;

    mqbu::CapacityMeter d_capacityMeter;

    ItemsMap d_items;

    VirtualStorageCatalog d_virtualStorageCatalog;

    RecordHandles d_queueOpRecordHandles;
    // List of handles to all QueueOpRecord
    // events associated with queue of this
    // storage.  Note that QueueOpRecords
    // for an in-memory storage will only
    // be used for queues in at-most-once
    // mode in a clustered setup.  First
    // handle in this vector always points
    // to QueueOpRecord of type
    // 'CREATION'. Apart from that, it will
    // contain 0 or more QueueOpRecords of
    // type 'PURGE' and 0 or 1
    // QueueOpRecord of type 'DELETION'.
    // Also note that records of type
    // 'ADDITION' are *never* present in
    // this data structure present only if
    // queue is in fanout mode.

    bsls::Types::Int64 d_ttlSeconds;

    bsls::AtomicInt d_isEmpty;
    // Flag indicating if storage is empty.
    // This flag can be checked from any
    // thread.

    bmqt::MessageGUID d_currentlyAutoConfirming;
    // Message being evaluated and possibly auto confirmed.

    AutoConfirms d_autoConfirms;
    // Current auto confirmed Apps for 'd_currentlyAutoConfirming'.

  private:
    // NOT IMPLEMENTED
    InMemoryStorage(const InMemoryStorage&) BSLS_KEYWORD_DELETED;

    /// Not implemented
    InMemoryStorage& operator=(const InMemoryStorage&) BSLS_KEYWORD_DELETED;

    // PRIVATE ACCESSORS

    /// Callback function called by `d_capacityMeter` to log appllications
    /// subscription info into the specified `stream`.
    bsl::ostream& logAppsSubscriptionInfoCb(bsl::ostream& stream) const;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(InMemoryStorage, bslma::UsesBslmaAllocator)

  public:
    // CREATORS

    /// Constructor of a new object associated to the queue having specified
    /// `uri` and using the specified `parentCapacityMeter`, and
    /// `allocator`.
    InMemoryStorage(const bmqt::Uri&               uri,
                    const mqbu::StorageKey&        queueKey,
                    int                            partitionId,
                    const mqbconfm::Domain&        config,
                    mqbu::CapacityMeter*           parentCapacityMeter,
                    bslma::Allocator*              allocator,
                    bmqma::CountingAllocatorStore* allocatorStore = 0);

    /// Destructor
    virtual ~InMemoryStorage() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS
    //   (virtual mqbi::Storage)

    /// Configure this storage using the specified `config` and `limits`.
    /// Return 0 on success, or an non-zero return code and fill in a
    /// description of the error in the specified `errorDescription`
    /// otherwise.  Note that calling `configure` on an already configured
    /// storage should fail and leave the storage untouched.
    virtual int configure(bsl::ostream&            errorDescription,
                          const mqbconfm::Storage& config,
                          const mqbconfm::Limits&  limits,
                          const bsls::Types::Int64 messageTtl,
                          const int maxDeliveryAttempts) BSLS_KEYWORD_OVERRIDE;

    /// Set the consistency level associated to this storage to the specified
    /// `value`.
    void
    setConsistency(const mqbconfm::Consistency& value) BSLS_KEYWORD_OVERRIDE;

    virtual void setQueue(mqbi::Queue* queue) BSLS_KEYWORD_OVERRIDE;

    /// Close this storage.
    virtual void close() BSLS_KEYWORD_OVERRIDE;

    /// Get an iterator for data stored in the virtual storage identified by
    /// the specified 'appKey'.
    /// If the 'appKey' is null, the returned  iterator can iterate states of
    /// all Apps; otherwise, the iterator can iterate states of the App
    /// corresponding to the 'appKey'.
    virtual bslma::ManagedPtr<mqbi::StorageIterator>
    getIterator(const mqbu::StorageKey& appKey) BSLS_KEYWORD_OVERRIDE;

    /// Load into the specified 'out' an iterator for data stored in the
    /// virtual storage initially pointing to the message associated with the
    /// specified 'msgGUID'.
    /// If the 'appKey' is null, the returned  iterator can iterate states of
    /// all Apps; otherwise, the iterator can iterate states of the App
    /// corresponding to the 'appKey'.
    /// Return zero on success, and a non-zero code if 'msgGUID' was not
    /// found in the storage.
    virtual mqbi::StorageResult::Enum
    getIterator(bslma::ManagedPtr<mqbi::StorageIterator>* out,
                const mqbu::StorageKey&                   appKey,
                const bmqt::MessageGUID& msgGUID) BSLS_KEYWORD_OVERRIDE;

    /// Save the message contained in the specified 'appData', 'options' and
    /// the associated 'attributes' and 'msgGUID' into this storage and the
    /// associated virtual storage.  The 'attributes' is an in/out parameter
    /// and storage layer can populate certain fields of that struct.
    /// Return 0 on success or an non-zero error code on failure.
    virtual mqbi::StorageResult::Enum
    put(mqbi::StorageMessageAttributes*     attributes,
        const bmqt::MessageGUID&            msgGUID,
        const bsl::shared_ptr<bdlbb::Blob>& appData,
        const bsl::shared_ptr<bdlbb::Blob>& options) BSLS_KEYWORD_OVERRIDE;

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
    virtual mqbi::StorageResult::Enum
    remove(const bmqt::MessageGUID& msgGUID,
           int*                     msgSize = 0) BSLS_KEYWORD_OVERRIDE;

    /// Remove all messages from this storage for the App identified by the
    /// specified 'appKey' if 'appKey' is not null.  Otherwise, remove messages
    /// for all Apps.  Record the event in the storage.
    /// Return one of the return codes from:
    /// * e_SUCCESS          : success
    /// * e_WRITE_FAILURE    : failed to record this event in storage
    /// * e_APPKEY_NOT_FOUND : Invalid 'appKey' specified
    virtual mqbi::StorageResult::Enum
    removeAll(const mqbu::StorageKey& appKey) BSLS_KEYWORD_OVERRIDE;

    /// Flush any buffered replication messages to the peers.  Behaviour is
    /// undefined unless this cluster node is the primary for this partition.
    void flushStorage() BSLS_KEYWORD_OVERRIDE;

    /// Return the resource capacity meter associated to this storage.
    virtual mqbu::CapacityMeter* capacityMeter() BSLS_KEYWORD_OVERRIDE;

    /// Attempt to garbage-collect messages for which TTL has expired, and
    /// return the number of messages garbage-collected.  Populate the
    /// specified `latestGcMsgTimestampEpoch` with the timestamp, as seconds
    /// from epoch, of the latest message garbage-collected due to TTL
    /// expiration, and the specified `configuredTtlValue` with the TTL
    /// value (in seconds) with which this storage instance is configured.
    virtual int gcExpiredMessages(bsls::Types::Uint64* latestMsgTimestampEpoch,
                                  bsls::Types::Int64*  configuredTtlValue,
                                  bsls::Types::Uint64  secondsFromEpoch)
        BSLS_KEYWORD_OVERRIDE;

    /// Garbage-collect those messages from the deduplication history which
    /// have expired the deduplication window.  Return `true`, if there are
    /// expired items unprocessed because of the batch limit.
    virtual bool gcHistory() BSLS_KEYWORD_OVERRIDE;

    virtual int
    addVirtualStorage(bsl::ostream&           errorDescription,
                      const bsl::string&      appId,
                      const mqbu::StorageKey& appKey) BSLS_KEYWORD_OVERRIDE;
    // Create, if it doesn't exist already, a virtual storage instance with
    // the specified 'appId' and 'appKey'.  Return zero upon success and a
    // non-zero value otherwise, and populate the specified
    // 'errorDescription' with a brief reason in case of failure.  Behavior
    // is undefined unless 'appId' is non-empty and 'appKey' is non-null.

    /// Remove the virtual storage identified by the specified `appKey`.
    /// Return true if a virtual storage with `appKey` was found and
    /// deleted, false if a virtual storage with `appKey` does not exist.
    /// Behavior is undefined unless `appKey` is non-null.  Note that this
    /// method will delete the virtual storage, and any reference to it will
    /// become invalid after this method returns.
    virtual bool
    removeVirtualStorage(const mqbu::StorageKey& appKey) BSLS_KEYWORD_OVERRIDE;

    virtual void selectForAutoConfirming(const bmqt::MessageGUID& msgGUID)
        BSLS_KEYWORD_OVERRIDE;
    virtual mqbi::StorageResult::Enum
    autoConfirm(const mqbu::StorageKey& appKey,
                bsls::Types::Uint64     timestamp) BSLS_KEYWORD_OVERRIDE;
    /// The sequence of calls is 'startAutoConfirming', then zero or more
    /// 'autoConfirm', then 'put' - all for the same specified 'msgGUID'.
    /// Any other sequence removes auto CONFIRMs.
    /// Auto-confirmed Apps do not PUSH the message.

    // ACCESSORS
    //   (virtual mqbi::Storage)

    /// Return the queue this storage is associated with.
    /// Storage exists without a queue before `setQueue`.
    virtual mqbi::Queue* queue() const BSLS_KEYWORD_OVERRIDE;

    /// Return the URI of the queue this storage is associated with.
    virtual const bmqt::Uri& queueUri() const BSLS_KEYWORD_OVERRIDE;

    /// Return the storage key associated with this instance.
    virtual const mqbu::StorageKey& queueKey() const BSLS_KEYWORD_OVERRIDE;

    /// Return the current configuration used by this storage. The behavior
    /// is undefined unless `configure` was successfully called.
    virtual const mqbconfm::Storage& config() const BSLS_KEYWORD_OVERRIDE;

    /// Return true if storage is backed by a persistent data store,
    /// otherwise return false.
    virtual bool isPersistent() const BSLS_KEYWORD_OVERRIDE;

    /// Return the number of messages in the virtual storage associated with
    /// the specified `appKey`.  If `appKey` is null, number of messages in
    /// the `physical` storage is returned.  Behavior is undefined if
    /// `appKey` is non-null but no virtual storage identified with it
    /// exists.
    virtual bsls::Types::Int64
    numMessages(const mqbu::StorageKey& appKey) const BSLS_KEYWORD_OVERRIDE;

    /// Return the number of bytes in the virtual storage associated with
    /// the specified `appKey`. If `appKey` is null, number of bytes in the
    /// `physical` storage is returned. Behavior is undefined if
    /// `appKey` is non-null but no virtual storage identified with it
    /// exists.
    virtual bsls::Types::Int64
    numBytes(const mqbu::StorageKey& appKey) const BSLS_KEYWORD_OVERRIDE;

    /// Return true if storage is empty.  This method can be invoked from
    /// any thread.
    virtual bool isEmpty() const BSLS_KEYWORD_OVERRIDE;

    /// Return true if this storage has message with the specified
    /// `msgGUID`, false otherwise.
    virtual bool
    hasMessage(const bmqt::MessageGUID& msgGUID) const BSLS_KEYWORD_OVERRIDE;

    /// Retrieve the message and its metadata having the specified `msgGUID`
    /// in the specified `appData`, `options` and `attributes` from this
    /// storage.  Return zero on success or a non-zero error code on
    /// failure.
    virtual mqbi::StorageResult::Enum
    get(bsl::shared_ptr<bdlbb::Blob>*   appData,
        bsl::shared_ptr<bdlbb::Blob>*   options,
        mqbi::StorageMessageAttributes* attributes,
        const bmqt::MessageGUID&        msgGUID) const BSLS_KEYWORD_OVERRIDE;

    /// Populate the specified `attributes` buffer with attributes of the
    /// message having the specified `msgGUID`.  Return zero on success or a
    /// non-zero error code on failure.
    virtual mqbi::StorageResult::Enum
    get(mqbi::StorageMessageAttributes* attributes,
        const bmqt::MessageGUID&        msgGUID) const BSLS_KEYWORD_OVERRIDE;

    /// Store in the specified 'msgSize' the size, in bytes, of the message
    /// having the specified 'msgGUID' if found and return success, or return a
    /// non-zero return code and leave 'msgSize' untouched if no message for
    /// the 'msgGUID' was found.
    virtual mqbi::StorageResult::Enum getMessageSize(
        int*                     msgSize,
        const bmqt::MessageGUID& msgGUID) const BSLS_KEYWORD_OVERRIDE;

    /// Return the number of virtual storages registered with this instance.
    virtual int numVirtualStorages() const BSLS_KEYWORD_OVERRIDE;

    /// Return true if virtual storage identified by the specified 'appKey'
    /// exists, otherwise return false.  Load into the optionally specified
    /// 'appId' the appId associated with 'appKey' if the virtual storage
    /// exists, otherwise set it to 0.
    virtual bool
    hasVirtualStorage(const mqbu::StorageKey& appKey,
                      bsl::string* appId = 0) const BSLS_KEYWORD_OVERRIDE;

    /// Return true if virtual storage identified by the specified 'appId'
    /// exists, otherwise return false.  Load into the optionally specified
    /// 'appKey' and 'ordinal' the appKey and ordinal associated with 'appId'
    /// if the virtual storage exists, otherwise set it to 0.
    virtual bool
    hasVirtualStorage(const bsl::string& appId,
                      mqbu::StorageKey*  appKey = 0,
                      unsigned int* ordinal = 0) const BSLS_KEYWORD_OVERRIDE;

    /// Return 'true' if there was Replication Receipt for the specified
    /// 'msgGUID'.
    virtual bool
    hasReceipt(const bmqt::MessageGUID& msgGUID) const BSLS_KEYWORD_OVERRIDE;

    /// Load into the specified 'buffer' the list of pairs of appId and
    /// appKey for all the virtual storages registered with this instance.
    virtual void
    loadVirtualStorageDetails(AppInfos* buffer) const BSLS_KEYWORD_OVERRIDE;

    virtual unsigned int numAutoConfirms() const BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS
    //   (virtual mqbs::ReplicatedStorage)
    virtual void processMessageRecord(const bmqt::MessageGUID&     guid,
                                      unsigned int                 msgLen,
                                      unsigned int                 refCount,
                                      const DataStoreRecordHandle& handle)
        BSLS_KEYWORD_OVERRIDE;

    virtual void processConfirmRecord(const bmqt::MessageGUID&     guid,
                                      const mqbu::StorageKey&      appKey,
                                      ConfirmReason::Enum          reason,
                                      const DataStoreRecordHandle& handle)
        BSLS_KEYWORD_OVERRIDE;

    virtual void
    processDeletionRecord(const bmqt::MessageGUID& guid) BSLS_KEYWORD_OVERRIDE;

    virtual void addQueueOpRecordHandle(const DataStoreRecordHandle& handle)
        BSLS_KEYWORD_OVERRIDE;

    virtual void purge(const mqbu::StorageKey& appKey) BSLS_KEYWORD_OVERRIDE;

    virtual void setPrimary() BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS
    //   (virtual mqbs::ReplicatedStorage)
    virtual int partitionId() const BSLS_KEYWORD_OVERRIDE;

    virtual const RecordHandles&
    queueOpRecordHandles() const BSLS_KEYWORD_OVERRIDE;

    virtual bool isStrongConsistency() const BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS
    bool isProxy() const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// --------------------------
// class InMemoryStorage_Item
// --------------------------

// CREATORS
inline InMemoryStorage_Item::InMemoryStorage_Item()
: d_appData()
, d_options()
, d_attributes()
{
}

inline InMemoryStorage_Item::InMemoryStorage_Item(
    const bsl::shared_ptr<bdlbb::Blob>&   appData,
    const bsl::shared_ptr<bdlbb::Blob>&   options,
    const mqbi::StorageMessageAttributes& attributes)
: d_appData(appData)
, d_options(options)
, d_attributes(attributes)
{
}

// MANIPULATORS
inline InMemoryStorage_Item&
InMemoryStorage_Item::setAppData(const bsl::shared_ptr<bdlbb::Blob>& value)
{
    d_appData = value;
    return *this;
}

inline InMemoryStorage_Item&
InMemoryStorage_Item::setOptions(const bsl::shared_ptr<bdlbb::Blob>& value)
{
    d_options = value;
    return *this;
}

inline InMemoryStorage_Item& InMemoryStorage_Item::setAttributes(
    const mqbi::StorageMessageAttributes& value)
{
    d_attributes = value;
    return *this;
}

inline mqbi::StorageMessageAttributes& InMemoryStorage_Item::attributes()
{
    return d_attributes;
}

inline void InMemoryStorage_Item::reset()
{
    d_appData.reset();
    d_options.reset();
}

// ACCESSORS
inline const bsl::shared_ptr<bdlbb::Blob>&
InMemoryStorage_Item::appData() const
{
    return d_appData;
}

inline const bsl::shared_ptr<bdlbb::Blob>&
InMemoryStorage_Item::options() const
{
    return d_options;
}

inline const mqbi::StorageMessageAttributes&
InMemoryStorage_Item::attributes() const
{
    return d_attributes;
}

inline InMemoryStorage::AutoConfirm::AutoConfirm(
    const mqbu::StorageKey& appKey)
: d_appKey(appKey)
{
    // NOTHING
}

inline const mqbu::StorageKey& InMemoryStorage::AutoConfirm::appKey()
{
    return d_appKey;
}

// ---------------------
// class InMemoryStorage
// ---------------------

// MANIPULATORS
//   (virtual mqbi::Storage)
inline mqbi::Queue* InMemoryStorage::queue() const
{
    return d_virtualStorageCatalog.queue();
}

inline int InMemoryStorage::addVirtualStorage(bsl::ostream& errorDescription,
                                              const bsl::string&      appId,
                                              const mqbu::StorageKey& appKey)
{
    return d_virtualStorageCatalog.addVirtualStorage(errorDescription,
                                                     appId,
                                                     appKey);
}

inline bool
InMemoryStorage::removeVirtualStorage(const mqbu::StorageKey& appKey)
{
    BSLS_ASSERT_SAFE(!appKey.isNull());

    return d_virtualStorageCatalog.removeVirtualStorage(appKey);
}

// ACCESSORS
//   (virtual mqbi::Storage)
inline const bmqt::Uri& InMemoryStorage::queueUri() const
{
    return d_uri;
}

inline const mqbu::StorageKey& InMemoryStorage::queueKey() const
{
    return d_key;
}

inline const mqbconfm::Storage& InMemoryStorage::config() const
{
    return d_config;
}

inline bool InMemoryStorage::isPersistent() const
{
    return false;
}

inline bsls::Types::Int64
InMemoryStorage::numMessages(const mqbu::StorageKey& appKey) const
{
    if (appKey.isNull()) {
        return d_items.size();  // RETURN
    }

    return d_virtualStorageCatalog.numMessages(appKey);
}

inline bsls::Types::Int64
InMemoryStorage::numBytes(const mqbu::StorageKey& appKey) const
{
    if (appKey.isNull()) {
        // Note that in proxy, capacity meter is disabled so this will always
        // return 0.
        return d_capacityMeter.bytes();  // RETURN
    }

    return d_virtualStorageCatalog.numBytes(appKey);
}

inline bool InMemoryStorage::isEmpty() const
{
    // executed by *ANY* thread
    return 1 == d_isEmpty.loadRelaxed();
}

inline bool InMemoryStorage::hasMessage(const bmqt::MessageGUID& msgGUID) const
{
    return 1 == d_items.count(msgGUID);
}

inline mqbi::StorageResult::Enum
InMemoryStorage::getMessageSize(int*                     msgSize,
                                const bmqt::MessageGUID& msgGUID) const
{
    ItemsMapConstIter it = d_items.find(msgGUID);
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(it == d_items.end())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        *msgSize = 0;
        return mqbi::StorageResult::e_GUID_NOT_FOUND;  // RETURN
    }

    *msgSize = it->second.appData()->length();
    return mqbi::StorageResult::e_SUCCESS;
}

inline int InMemoryStorage::numVirtualStorages() const
{
    return d_virtualStorageCatalog.numVirtualStorages();
}

inline bool InMemoryStorage::hasVirtualStorage(const mqbu::StorageKey& appKey,
                                               bsl::string* appId) const
{
    return d_virtualStorageCatalog.hasVirtualStorage(appKey, appId);
}

inline bool InMemoryStorage::hasVirtualStorage(const bsl::string& appId,
                                               mqbu::StorageKey*  appKey,
                                               unsigned int* ordinal) const
{
    return d_virtualStorageCatalog.hasVirtualStorage(appId, appKey, ordinal);
}

inline bool InMemoryStorage::hasReceipt(const bmqt::MessageGUID&) const
{
    return true;
}

inline void InMemoryStorage::loadVirtualStorageDetails(AppInfos* buffer) const

{
    return d_virtualStorageCatalog.loadVirtualStorageDetails(buffer);
}

inline unsigned int InMemoryStorage::numAutoConfirms() const
{
    return d_autoConfirms.size();
}

inline mqbu::CapacityMeter* InMemoryStorage::capacityMeter()
{
    return &d_capacityMeter;
}

// ACCESSORS
//   (virtual mqbs::ReplicatedStorage)
inline int InMemoryStorage::partitionId() const
{
    return d_partitionId;
}

// ACCESSORS
inline bool InMemoryStorage::isProxy() const
{
    return d_partitionId == mqbs::DataStore::k_INVALID_PARTITION_ID;
}

}  // close package namespace

namespace bmqc {

template <>
inline void
clean<mqbs::InMemoryStorage_Item>(mqbs::InMemoryStorage_Item& value)
{
    value.reset();
}

}

}  // close enterprise namespace

#endif
