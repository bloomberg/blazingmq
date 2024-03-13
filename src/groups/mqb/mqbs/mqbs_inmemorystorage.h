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

// MWC
#include <mwcc_orderedhashmapwithhistory.h>

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
namespace mwcma {
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
    typedef mwcc::OrderedHashMapWithHistory<
        bmqt::MessageGUID,
        Item,
        bslh::Hash<bmqt::MessageGUIDHashAlgo> >
        ItemsMap;

    typedef ItemsMap::iterator ItemsMapIter;

    typedef ItemsMap::const_iterator ItemsMapConstIter;

    typedef mqbi::Storage::StorageKeys StorageKeys;

  public:
    // CLASS METHODS

    /// Factory method to create a new `InMemoryStorage` in the specified
    /// `out`, using the specified `allocator`.
    static void factoryMethod(bslma::ManagedPtr<mqbi::Storage>* out,
                              bslma::Allocator*                 allocator);

  public:
    // TYPES
    typedef mqbi::Storage::AppIdKeyPair AppIdKeyPair;

    typedef mqbi::Storage::AppIdKeyPairs AppIdKeyPairs;

    typedef ReplicatedStorage::RecordHandles RecordHandles;

  private:
    // DATA
    bslma::Allocator* d_allocator_p;

    mqbi::Queue* d_queue_p;

    mqbu::StorageKey d_key;

    bmqt::Uri d_uri;

    int d_partitionId;

    mqbconfm::Storage d_config;

    mqbconfm::Limits d_limits;

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

    bsl::string d_emptyAppId;
    // This field is unused, but needs to
    // be a member variable so that
    // 'appId()' routine can return a ref.

    mqbu::StorageKey d_nullAppKey;
    // This field is unused, but needs to
    // be a member variable so that
    // 'appKey()' routine can return a ref.

    bsls::AtomicInt d_isEmpty;
    // Flag indicating if storage is empty.
    // This flag can be checked from any
    // thread.

    bmqp::RdaInfo d_defaultRdaInfo;
    // Use in all 'put' operations.

    bmqt::MessageGUID d_currentlyAutoConfirming;
    // Message being evaluated and possibly auto confirmed.

    unsigned int d_numAutoConfirms;
    // Current number of auto confirmed Apps for 'd_currentlyAutoConfirming'.

  private:
    // NOT IMPLEMENTED
    InMemoryStorage(const InMemoryStorage&) BSLS_KEYWORD_DELETED;

    /// Not implemented
    InMemoryStorage& operator=(const InMemoryStorage&) BSLS_KEYWORD_DELETED;

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
                    const bmqp::RdaInfo&           defaultRdaInfo,
                    bslma::Allocator*              allocator,
                    mwcma::CountingAllocatorStore* allocatorStore = 0);

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

    virtual void setQueue(mqbi::Queue* queue) BSLS_KEYWORD_OVERRIDE;

    virtual mqbi::Queue* queue() BSLS_KEYWORD_OVERRIDE;

    /// Close this storage.
    virtual void close() BSLS_KEYWORD_OVERRIDE;

    /// Get an iterator for items stored in the virtual storage identified
    /// by the specified `appKey`.  Iterator will point to point to the
    /// oldest item, if any, or to the end of the collection if empty.  Note
    /// that if `appKey` is null, an iterator over the underlying physical
    /// storage will be returned.  Also note that because `Storage` and
    /// `StorageIterator` are interfaces, the implementation of this method
    /// will allocate, so it's recommended to keep the iterator.
    virtual bslma::ManagedPtr<mqbi::StorageIterator>
    getIterator(const mqbu::StorageKey& appKey) BSLS_KEYWORD_OVERRIDE;

    /// Load into the specified `out` an iterator for items stored in
    /// the virtual storage identified by the specified `appKey`, initially
    /// pointing to the item associated with the specified `msgGUID`.
    /// Return zero on success, and a non-zero code if `msgGUID` was not
    /// found in the storage.  Note that if `appKey` is null, an iterator
    /// over the underlying physical storage will be returned.  Also note
    /// that because `Storage` and `StorageIterator` are interfaces, the
    /// implementation of this method will allocate, so it's recommended to
    /// keep the iterator.
    virtual mqbi::StorageResult::Enum
    getIterator(bslma::ManagedPtr<mqbi::StorageIterator>* out,
                const mqbu::StorageKey&                   appKey,
                const bmqt::MessageGUID& msgGUID) BSLS_KEYWORD_OVERRIDE;

    /// Save the message contained in the specified `appData`, `options` and
    /// the associated `attributes` and `msgGUID` into this storage and the
    /// associated virtual storages, if any.  The `attributes` is an in/out
    /// parameter and storage layer can populate certain fields of that
    /// struct.  Return 0 on success or an non-zero error code on failure.
    virtual mqbi::StorageResult::Enum
    put(mqbi::StorageMessageAttributes*     attributes,
        const bmqt::MessageGUID&            msgGUID,
        const bsl::shared_ptr<bdlbb::Blob>& appData,
        const bsl::shared_ptr<bdlbb::Blob>& options,
        const StorageKeys& storageKeys = StorageKeys()) BSLS_KEYWORD_OVERRIDE;

    /// Release the reference of the specified `appKey` on the message
    /// identified by the specified `msgGUID`, and record this event in the
    /// storage.  Return one of the return codes from:
    /// * **e_GUID_NOT_FOUND**      : `msgGUID` was not found
    /// * **e_ZERO_REFERENCES**     : message refCount has become zero
    /// * **e_NON_ZERO_REFERENCES** : message refCount is still not zero
    /// * **e_WRITE_FAILURE**       : failed to record this event in storage
    mqbi::StorageResult::Enum
    releaseRef(const bmqt::MessageGUID& msgGUID,
               const mqbu::StorageKey&  appKey,
               bsls::Types::Int64       timestamp,
               bool onReject = false) BSLS_KEYWORD_OVERRIDE;

    /// Remove from the storage the message having the specified `msgGUID`
    /// and store it's size, in bytes, in the optionally specified `msgSize`
    /// if the `msgGUID` was found.  Return 0 on success, or a non-zero
    /// return code if the `msgGUID` was not found.  If the optionally
    /// specified `clearAll` is true, remove the message from all virtual
    /// storages as well.
    virtual mqbi::StorageResult::Enum
    remove(const bmqt::MessageGUID& msgGUID,
           int*                     msgSize  = 0,
           bool                     clearAll = false) BSLS_KEYWORD_OVERRIDE;

    /// Remove all messages from this storage for the client identified by
    /// the specified `appKey`.  If `appKey` is null, then remove messages
    /// for all clients.  Return one of the return codes from:
    /// * **e_SUCCESS**          : `msgGUID` was not found
    /// * **e_APPKEY_NOT_FOUND** : Invalid `appKey` specified
    virtual mqbi::StorageResult::Enum
    removeAll(const mqbu::StorageKey& appKey) BSLS_KEYWORD_OVERRIDE;

    /// If the specified `storage` is `true`, flush any buffered replication
    /// messages to the peers.  If the specified `queues` is `true`, `flush`
    /// all associated queues.  Behavior is undefined unless this node is
    /// the primary for this partition.
    virtual void dispatcherFlush(bool storage,
                                 bool queues) BSLS_KEYWORD_OVERRIDE;

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

    /// Return the URI of the queue this storage is associated with.
    virtual const bmqt::Uri& queueUri() const BSLS_KEYWORD_OVERRIDE;

    /// Return the storage key associated with this instance.
    virtual const mqbu::StorageKey& queueKey() const BSLS_KEYWORD_OVERRIDE;

    /// Return the appId associated with this storage instance.  If there is
    /// not appId associated, return an empty string.
    virtual const bsl::string& appId() const BSLS_KEYWORD_OVERRIDE;

    /// Return the app key, if any, associated with this storage instance.
    /// If there is no appKey associated, return a null key.
    virtual const mqbu::StorageKey& appKey() const BSLS_KEYWORD_OVERRIDE;

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

    virtual mqbi::StorageResult::Enum getMessageSize(
        int*                     msgSize,
        const bmqt::MessageGUID& msgGUID) const BSLS_KEYWORD_OVERRIDE;
    // Store in the specified 'msgSize' the size, in bytes, of the message
    // having the specified 'msgGUID' if found and return success, or
    // return a non-zero return code and leave 'msgSize' untouched if no
    // message with 'msgGUID' were found.

    /// Return the number of virtual storages registered with this instance.
    virtual int numVirtualStorages() const BSLS_KEYWORD_OVERRIDE;

    virtual bool
    hasVirtualStorage(const mqbu::StorageKey& appKey,
                      bsl::string* appId = 0) const BSLS_KEYWORD_OVERRIDE;
    // Return true if virtual storage identified by the specified 'appKey'
    // exists, otherwise return false.  Load into the optionally specified
    // 'appId' the appId associated with 'appKey' if the virtual storage
    // exists, otherwise set it to 0.

    virtual bool hasVirtualStorage(const bsl::string& appId,
                                   mqbu::StorageKey*  appKey = 0) const
        BSLS_KEYWORD_OVERRIDE;
    // Return true if virtual storage identified by the specified 'appId'
    // exists, otherwise return false.  Load into the optionally specified
    // 'appKey' the appKey associated with 'appId' if the virtual storage
    // exists, otherwise set it to 0.

    virtual bool
    hasReceipt(const bmqt::MessageGUID& msgGUID) const BSLS_KEYWORD_OVERRIDE;
    // Return 'true' if there was Replication Receipt for the specified
    // 'msgGUID'.

    virtual void loadVirtualStorageDetails(AppIdKeyPairs* buffer) const
        BSLS_KEYWORD_OVERRIDE;
    // Load into the specified 'buffer' the list of pairs of appId and
    // appKey for all the virtual storages registered with this instance.

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

    // ACCESSORS
    //   (virtual mqbs::ReplicatedStorage)
    virtual int partitionId() const BSLS_KEYWORD_OVERRIDE;

    virtual const RecordHandles&
    queueOpRecordHandles() const BSLS_KEYWORD_OVERRIDE;

    virtual bool isStrongConsistency() const BSLS_KEYWORD_OVERRIDE;
};

// =============================
// class InMemoryStorageIterator
// =============================

class InMemoryStorageIterator : public mqbi::StorageIterator {
    // TBD

  private:
    // PRIVATE TYPES
    typedef InMemoryStorage::ItemsMapConstIter ItemsMapConstIter;

  private:
    // DATA
    InMemoryStorage* d_storage_p;  // Storage this Iterator is associated
                                   // with

    ItemsMapConstIter d_iterator;  // Internal iterator representing the
                                   // current position

  public:
    // CREATORS

    /// Create a new InMemoryStorageIterator from the specified `storage`
    /// and pointing at the specified `initialPosition`.
    InMemoryStorageIterator(InMemoryStorage*         storage,
                            const ItemsMapConstIter& initialPosition);

    /// Destructor
    ~InMemoryStorageIterator() BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS

    /// Return a reference offering non-modifiable access to the guid
    /// associated to the item currently pointed at by this iterator.  The
    /// behavior is undefined unless `atEnd` returns `false`.
    const bmqt::MessageGUID& guid() const BSLS_KEYWORD_OVERRIDE;

    /// Return a reference offering modifiable access to the RdaInfo
    /// associated to the item currently pointed at by this iterator.  The
    /// behavior is undefined unless `atEnd` returns `false`.
    bmqp::RdaInfo& rdaInfo() const BSLS_KEYWORD_OVERRIDE;

    /// Return subscription id associated to the item currently pointed at
    /// by this iterator.
    /// The behavior is undefined unless `atEnd` returns `false`.
    unsigned int subscriptionId() const BSLS_KEYWORD_OVERRIDE;

    /// Return a reference offering non-modifiable access to the application
    /// data associated with the item currently pointed at by this iterator.
    /// The behavior is undefined unless `atEnd` returns `false`.
    const bsl::shared_ptr<bdlbb::Blob>& appData() const BSLS_KEYWORD_OVERRIDE;

    /// Return a reference offering non-modifiable access to the options
    /// associated with the item currently pointed at by this iterator.  The
    /// behavior is undefined unless `atEnd` returns `false`.
    const bsl::shared_ptr<bdlbb::Blob>& options() const BSLS_KEYWORD_OVERRIDE;

    const mqbi::StorageMessageAttributes&
    attributes() const BSLS_KEYWORD_OVERRIDE;
    // Return a reference offering non-modifiable access to the attributes
    // associated with the message currently pointed at by this iterator.
    // The behavior is undefined unless 'atEnd' returns 'false'.

    /// Return `true` if this iterator is currently at the end of the items'
    /// collection, and hence doesn't reference a valid item.
    bool atEnd() const BSLS_KEYWORD_OVERRIDE;

    /// Return `true` if this iterator is currently not at the end of the
    /// `items` collection and the message currently pointed at by this
    /// iterator has received replication factor Receipts.
    bool hasReceipt() const BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    /// Advance the iterator to the next item.  The behavior is undefined
    /// unless `atEnd` returns `false`.  Return `true` if the iterator then
    /// points to a valid item, or `false` if it now is at the end of the
    /// items' collection.
    bool advance() BSLS_KEYWORD_OVERRIDE;

    /// Reset the iterator to point to first item, if any, in the underlying
    /// storage.
    void reset() BSLS_KEYWORD_OVERRIDE;
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

// ---------------------
// class InMemoryStorage
// ---------------------

// MANIPULATORS
//   (virtual mqbi::Storage)
inline mqbi::Queue* InMemoryStorage::queue()
{
    return d_queue_p;
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

inline const bsl::string& InMemoryStorage::appId() const
{
    return d_emptyAppId;
}

inline const mqbu::StorageKey& InMemoryStorage::appKey() const
{
    return d_nullAppKey;
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
                                               mqbu::StorageKey*  appKey) const
{
    return d_virtualStorageCatalog.hasVirtualStorage(appId, appKey);
}

inline bool InMemoryStorage::hasReceipt(const bmqt::MessageGUID&) const
{
    return true;
}

inline void
InMemoryStorage::loadVirtualStorageDetails(AppIdKeyPairs* buffer) const

{
    return d_virtualStorageCatalog.loadVirtualStorageDetails(buffer);
}

inline unsigned int InMemoryStorage::numAutoConfirms() const
{
    return d_numAutoConfirms;
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

// -----------------------------
// class InMemoryStorageIterator
// -----------------------------

// ACCESSORS
inline const bmqt::MessageGUID& InMemoryStorageIterator::guid() const
{
    return d_iterator->first;
}

inline bmqp::RdaInfo& InMemoryStorageIterator::rdaInfo() const
{
    static bmqp::RdaInfo dummy;
    return dummy;
}

inline unsigned int InMemoryStorageIterator::subscriptionId() const
{
    return bmqp::Protocol::k_DEFAULT_SUBSCRIPTION_ID;
}

inline const bsl::shared_ptr<bdlbb::Blob>&
InMemoryStorageIterator::appData() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!atEnd());

    return d_iterator->second.appData();
}

inline const bsl::shared_ptr<bdlbb::Blob>&
InMemoryStorageIterator::options() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!atEnd());

    return d_iterator->second.options();
}

inline const mqbi::StorageMessageAttributes&
InMemoryStorageIterator::attributes() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!atEnd());

    return d_iterator->second.attributes();
}

inline bool InMemoryStorageIterator::atEnd() const
{
    return d_iterator == d_storage_p->d_items.end();
}

inline bool InMemoryStorageIterator::hasReceipt() const
{
    return !atEnd();
}

// MANIPULATORS
inline bool InMemoryStorageIterator::advance()
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!atEnd());

    ++d_iterator;
    return !atEnd();
}

inline void InMemoryStorageIterator::reset()
{
    d_iterator = d_storage_p->d_items.begin();
}

}  // close package namespace

namespace mwcc {

template <>
inline void
clean<mqbs::InMemoryStorage_Item>(mqbs::InMemoryStorage_Item& value)
{
    value.reset();
}

}

}  // close enterprise namespace

#endif
