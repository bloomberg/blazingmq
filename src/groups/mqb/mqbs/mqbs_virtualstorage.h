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

// mqbs_virtualstorage.h                                              -*-C++-*-
#ifndef INCLUDED_MQBS_VIRTUALSTORAGE
#define INCLUDED_MQBS_VIRTUALSTORAGE

//@PURPOSE: Provide a mechanism to add per-client state to a BlazingMQ storage.
//
//@CLASSES:
//  mqbs::VirtualStorage: Mechanism to add per-client state to a BlazingMQ
//  storage.
//
//@DESCRIPTION: 'mqbs::VirtualStorage' provides a mechanism to add per-client
// state to an underlying BlazingMQ storage.
//
/// Warning
///-------
// An instance of this component is backed by a "real" underlying storage.
// There are two requirements on the underlying storage:
//: o It must out-live all virtual storage based off of it.
//: o A message *must* *not* be deleted from the underlying storage as long as
//:   it is being referenced by a virtual storage.

// MQB

#include <mqbi_storage.h>
#include <mqbs_datastore.h>
#include <mqbs_filestoreprotocol.h>
#include <mqbu_storagekey.h>

// BMQ
#include <bmqt_messageguid.h>

// MWC
#include <mwcc_orderedhashmap.h>

// BDE
#include <bdlbb_blob.h>
#include <bsl_memory.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_annotation.h>
#include <bsls_assert.h>
#include <bsls_types.h>

namespace BloombergLP {

namespace mqbs {

// FORWARD DECLARATION
class VirtualStorageIterator;

// ====================
// class VirtualStorage
// ====================

class VirtualStorage : public mqbi::Storage {
    // TBD

  private:
    // FRIENDS
    friend class VirtualStorageIterator;

    // PRIVATE TYPES
    struct MessageContext {
        int                   d_size;
        mutable bmqp::RdaInfo d_rdaInfo;
        unsigned int          d_subscriptionId;

        MessageContext(int                  size,
                       const bmqp::RdaInfo& rdaInfo,
                       unsigned int         subScriptionId);
    };

    /// msgGUID -> MessageContext
    /// Must be a container in which iteration order is same as insertion
    /// order.
    typedef mwcc::OrderedHashMap<bmqt::MessageGUID,
                                 MessageContext,
                                 bslh::Hash<bmqt::MessageGUIDHashAlgo> >
        GuidList;

    typedef GuidList::iterator GuidListIter;

    typedef mqbi::Storage::StorageKeys StorageKeys;

  private:
    // DATA
    bslma::Allocator* d_allocator_p;

    mqbi::Storage* d_storage_p;
    // underlying 'real' storage.  Held.

    bsl::string d_appId;
    // AppId of the consumer client which this
    // instance represents.

    mqbu::StorageKey d_appKey;
    // Storage key of the associated 'appId'.

    GuidList d_guids;
    // List of guids that are part of this storage.

    bsls::Types::Int64 d_totalBytes;
    // Total size (in bytes) of all the messages that
    // it holds.

    bmqt::MessageGUID d_autoConfirm;
    // This App should not 'put' this guid because it is auto confirmed.

  private:
    // NOT IMPLEMENTED
    VirtualStorage(const VirtualStorage&);             // = delete
    VirtualStorage& operator=(const VirtualStorage&);  // = delete

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(VirtualStorage, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create an instance of virtual storage backed by the specified real
    /// `storage`, and having the specified `appId` and `appKey`, and use
    /// the specified `allocator` for any memory allocations.  Behavior is
    /// undefined unless `storage` is non-null, `appId` is non-empty and
    /// `appKey` is non-null.  Note that the specified real `storage` must
    /// outlive this virtual storage instance.
    VirtualStorage(mqbi::Storage*          storage,
                   const bsl::string&      appId,
                   const mqbu::StorageKey& appKey,
                   bslma::Allocator*       allocator);

    /// Destructor.
    ~VirtualStorage() BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS

    /// Return the URI of the queue this storage is associated with.
    const bmqt::Uri& queueUri() const BSLS_KEYWORD_OVERRIDE;

    /// Return the storage key associated with this instance.
    const mqbu::StorageKey& queueKey() const BSLS_KEYWORD_OVERRIDE;

    /// Return the appId associated with this storage instance.  Note that
    /// the returned string is always non-empty.
    const bsl::string& appId() const BSLS_KEYWORD_OVERRIDE;

    /// Return the app key, if any, associated with this storage instance.
    /// Note that the returned key is always non-null.
    const mqbu::StorageKey& appKey() const BSLS_KEYWORD_OVERRIDE;

    /// Return the current configuration used by this storage. The behavior
    /// is undefined unless `configure` was successfully called.
    const mqbconfm::Storage& config() const BSLS_KEYWORD_OVERRIDE;

    /// Return the partitionId associated with this storage.
    int partitionId() const BSLS_KEYWORD_OVERRIDE;

    /// Return true if storage is backed by a persistent data store,
    /// otherwise return false.
    bool isPersistent() const BSLS_KEYWORD_OVERRIDE;

    bsls::Types::Int64
    numMessages(const mqbu::StorageKey& appKey) const BSLS_KEYWORD_OVERRIDE;
    // Return the number of messages in the storage.

    bsls::Types::Int64
    numBytes(const mqbu::StorageKey& appKey) const BSLS_KEYWORD_OVERRIDE;
    // Return the number of bytes in the storage.

    /// Behavior is undefined if this method is ever invoked.  This method
    /// needs to be implemented as its part of base protocol.
    bool isEmpty() const BSLS_KEYWORD_OVERRIDE;

    bool
    hasMessage(const bmqt::MessageGUID& msgGUID) const BSLS_KEYWORD_OVERRIDE;
    // Return true if this storage has message with the specified
    // 'msgGUID', false otherwise.

    /// Behavior is undefined if this method is ever invoked.  This method
    /// needs to be implemented as its part of base protocol.
    int numVirtualStorages() const BSLS_KEYWORD_OVERRIDE;

    /// Behavior is undefined if this method is ever invoked.  This method
    /// needs to be implemented as its part of base protocol.
    bool hasVirtualStorage(const mqbu::StorageKey& appKey,
                           bsl::string* appId = 0) const BSLS_KEYWORD_OVERRIDE;

    /// Behavior is undefined if this method is ever invoked.  This method
    /// needs to be implemented as its part of base protocol.
    bool hasVirtualStorage(const bsl::string& appId,
                           mqbu::StorageKey*  appKey = 0) const
        BSLS_KEYWORD_OVERRIDE;

    /// Return `true` if there was Replication Receipt for the specified
    /// `msgGUID`.
    bool
    hasReceipt(const bmqt::MessageGUID& msgGUID) const BSLS_KEYWORD_OVERRIDE;

    /// Behavior is undefined if this method is ever invoked.  This method
    /// needs to be implemented as its part of base protocol.
    void loadVirtualStorageDetails(AppIdKeyPairs* buffer) const
        BSLS_KEYWORD_OVERRIDE;

    /// Behavior is undefined if these methods are ever invoked.  These methods
    /// need to be implemented as their part of the base protocol.
    unsigned int numAutoConfirms() const BSLS_KEYWORD_OVERRIDE;

    /// Store in the specified `msgSize` the size, in bytes, of the message
    /// having the specified `msgGUID` if found and return success, or
    /// return a non-zero return code and leave `msgSize` untouched if no
    /// message with `msgGUID` were found.
    mqbi::StorageResult::Enum getMessageSize(
        int*                     msgSize,
        const bmqt::MessageGUID& msgGUID) const BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    /// Behavior is undefined if this method is ever invoked.  This method
    /// needs to be implemented as its part of base protocol.
    mqbu::CapacityMeter* capacityMeter() BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS

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
                  const int maxDeliveryAttempts) BSLS_KEYWORD_OVERRIDE;

    /// Behavior is undefined if this method is ever invoked.  This method
    /// needs to be implemented as its part of base protocol.
    void setQueue(mqbi::Queue* queue) BSLS_KEYWORD_OVERRIDE;

    /// Behavior is undefined if this method is ever invoked.  This method
    /// needs to be implemented as its part of base protocol.
    mqbi::Queue* queue() BSLS_KEYWORD_OVERRIDE;

    /// Close this storage.
    void close() BSLS_KEYWORD_OVERRIDE;

    /// Save the message having the specified `msgGUID`, `msgSize`, and
    /// `rdaInfo` into this virtual storage. Return 0 on success or an
    /// non-zero error code on failure.
    mqbi::StorageResult::Enum put(const bmqt::MessageGUID& msgGUID,
                                  int                      msgSize,
                                  const bmqp::RdaInfo&     rdaInfo,
                                  unsigned int             subScriptionId);

    /// Behavior is undefined if this method is ever invoked.  This method
    /// needs to be implemented as its part of base protocol. Please call
    /// put(const bmqt::MessageGUID& msgGUID, const int msgSize) instead.
    mqbi::StorageResult::Enum
    put(mqbi::StorageMessageAttributes*     attributes,
        const bmqt::MessageGUID&            msgGUID,
        const bsl::shared_ptr<bdlbb::Blob>& appData,
        const bsl::shared_ptr<bdlbb::Blob>& options,
        const StorageKeys& storageKeys = StorageKeys()) BSLS_KEYWORD_OVERRIDE;

    /// Get an iterator for items stored in the virtual storage identified
    /// by the specified `appKey`.  Iterator will point to point to the
    /// oldest item, if any, or to the end of the collection if empty.  Note
    /// that if `appKey` is null, an iterator over the underlying physical
    /// storage will be returned.  Also note that because `Storage` and
    /// `StorageIterator` are interfaces, the implementation of this method
    /// will allocate, so it's recommended to keep the iterator.
    bslma::ManagedPtr<mqbi::StorageIterator>
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
    mqbi::StorageResult::Enum
    getIterator(bslma::ManagedPtr<mqbi::StorageIterator>* out,
                const mqbu::StorageKey&                   appKey,
                const bmqt::MessageGUID& msgGUID) BSLS_KEYWORD_OVERRIDE;

    /// Remove from the storage the message having the specified `msgGUID`
    /// and store it's size, in bytes, in the optionally specified `msgSize`
    /// if the `msgGUID` was found.  Return 0 on success, or a non-zero
    /// return code if the `msgGUID` was not found.  The optionally
    /// specified `clearAll` is ignored.
    mqbi::StorageResult::Enum
    remove(const bmqt::MessageGUID& msgGUID,
           int*                     msgSize  = 0,
           bool                     clearAll = false) BSLS_KEYWORD_OVERRIDE;

    /// Behavior is undefined if this method is ever invoked.  This method
    /// needs to be implemented as its part of base protocol.
    mqbi::StorageResult::Enum
    releaseRef(const bmqt::MessageGUID& msgGUID,
               const mqbu::StorageKey&  appKey,
               bsls::Types::Int64       timestamp,
               bool onReject = false) BSLS_KEYWORD_OVERRIDE;

    /// Remove all entries from this storage.  Specified `appKey` is unused.
    /// This routine always returns success.
    mqbi::StorageResult::Enum
    removeAll(const mqbu::StorageKey& appKey) BSLS_KEYWORD_OVERRIDE;

    /// Behavior is undefined if this method is ever invoked.  This method
    /// needs to be implemented as its part of base protocol.
    void dispatcherFlush(bool storage, bool queues) BSLS_KEYWORD_OVERRIDE;

    /// Attempt to garbage-collect messages for which TTL has expired, and
    /// return the number of messages garbage-collected.  Populate the
    /// specified `latestGcMsgTimestampEpoch` with the timestamp, as seconds
    /// from epoch, of the latest message garbage-collected due to TTL
    /// expiration, and the specified `configuredTtlValue` with the TTL
    /// value (in seconds) with which this storage instance is configured.
    virtual int gcExpiredMessages(
        bsls::Types::Uint64* latestGcMsgTimestampEpoch,
        bsls::Types::Int64*  configuredTtlValue,
        bsls::Types::Uint64  secondsFromEpoch) BSLS_KEYWORD_OVERRIDE;

    /// Garbage-collect those messages from the deduplication history which
    /// have expired the deduplication window.  Return `true`, if there are
    /// expired items unprocessed because of the batch limit.
    virtual bool gcHistory() BSLS_KEYWORD_OVERRIDE;

    /// Behavior is undefined if this method is ever invoked.  This method
    /// needs to be implemented as its part of base protocol.
    int
    addVirtualStorage(bsl::ostream&           errorDescription,
                      const bsl::string&      appId,
                      const mqbu::StorageKey& appKey) BSLS_KEYWORD_OVERRIDE;

    /// Behavior is undefined if this method is ever invoked.  This method
    /// needs to be implemented as its part of base protocol.
    bool
    removeVirtualStorage(const mqbu::StorageKey& appKey) BSLS_KEYWORD_OVERRIDE;

    /// Behavior is undefined if these methods are ever invoked.  These methods
    /// need to be implemented as their part of the base protocol.
    void selectForAutoConfirming(const bmqt::MessageGUID& msgGUID)
        BSLS_KEYWORD_OVERRIDE;
    mqbi::StorageResult::Enum
    autoConfirm(const mqbu::StorageKey& appKey,
                bsls::Types::Uint64     timestamp) BSLS_KEYWORD_OVERRIDE;

    /// Ignore the specified 'msgGUID' in the subsequent 'put' call because the
    /// App has auto confirmed it.
    void autoConfirm(const bmqt::MessageGUID& msgGUID);
};

// ============================
// class VirtualStorageIterator
// ============================

class VirtualStorageIterator : public mqbi::StorageIterator {
    // TBD

  private:
    // DATA
    VirtualStorage* d_virtualStorage_p;

    VirtualStorage::GuidList::const_iterator d_iterator;

    mutable mqbi::StorageMessageAttributes d_attributes;

    mutable bsl::shared_ptr<bdlbb::Blob> d_appData_sp;
    // If this variable is empty, it is
    // assumed that attributes, message,
    // and options have not been loaded in
    // this iteration (see also
    // 'loadMessageAndAttributes' impl).

    mutable bsl::shared_ptr<bdlbb::Blob> d_options_sp;

    mutable bool d_haveReceipt;
    // Avoid reading Attributes.

  private:
    // NOT IMPLEMENTED
    VirtualStorageIterator(const VirtualStorageIterator&);  // = delete
    VirtualStorageIterator&
    operator=(const VirtualStorageIterator&);  // = delete

  private:
    // PRIVATE MANIPULATORS

    /// Clear previous state, if any.  This is required so that new state
    /// can be loaded in `appData`, `options` or `attributes` routines.
    void clear();

    // PRIVATE ACCESSORS

    /// Load the internal state of this iterator instance with the
    /// attributes and blob pointed to by the MessageGUID to which this
    /// iterator is currently pointing.  Behavior is undefined if `atEnd()`
    /// returns true or if underlying storage does not contain the
    /// MessageGUID being pointed to by this iterator.  Return `false` if
    /// data are already loaded; return `true` otherwise.
    bool loadMessageAndAttributes() const;

  public:
    // CREATORS

    /// Create a new VirtualStorageIterator from the specified `storage` and
    /// pointing at the specified `initialPosition`.
    VirtualStorageIterator(
        VirtualStorage*                                 storage,
        const VirtualStorage::GuidList::const_iterator& initialPosition);

    /// Destructor
    ~VirtualStorageIterator() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS
    bool advance() BSLS_KEYWORD_OVERRIDE;
    void reset() BSLS_KEYWORD_OVERRIDE;

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

    /// Return a reference offering non-modifiable access to the attributes
    /// associated with the message currently pointed at by this iterator.
    /// The behavior is undefined unless `atEnd` returns `false`.
    const mqbi::StorageMessageAttributes&
    attributes() const BSLS_KEYWORD_OVERRIDE;

    /// Return `true` if this iterator is currently at the end of the items'
    /// collection, and hence doesn't reference a valid item.
    bool atEnd() const BSLS_KEYWORD_OVERRIDE;

    /// Return `true` if this iterator is currently not at the end of the
    /// `items` collection and the message currently pointed at by this
    /// iterator has received replication factor Receipts.
    bool hasReceipt() const BSLS_KEYWORD_OVERRIDE;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ------------------------------------
// class VirtualStorage::MessageContext
// ------------------------------------

inline VirtualStorage::MessageContext::MessageContext(
    int                  size,
    const bmqp::RdaInfo& rdaInfo,
    unsigned int         subScriptionId)
: d_size(size)
, d_rdaInfo(rdaInfo)
, d_subscriptionId(subScriptionId)
{
    // NOTHING
}

// --------------------
// class VirtualStorage
// --------------------

// ACCESSORS
inline const bmqt::Uri& VirtualStorage::queueUri() const
{
    return d_storage_p->queueUri();
}

inline const mqbu::StorageKey& VirtualStorage::queueKey() const
{
    return d_storage_p->queueKey();
}

inline const bsl::string& VirtualStorage::appId() const
{
    return d_appId;
}

inline const mqbu::StorageKey& VirtualStorage::appKey() const
{
    return d_appKey;
}

inline const mqbconfm::Storage& VirtualStorage::config() const
{
    return d_storage_p->config();
}

inline int VirtualStorage::partitionId() const
{
    return mqbs::DataStore::k_INVALID_PARTITION_ID;
}

inline bool VirtualStorage::isPersistent() const
{
    return d_storage_p->isPersistent();
}

inline mqbu::CapacityMeter* VirtualStorage::capacityMeter()
{
    BSLS_ASSERT_OPT(false && "Should not be invoked.");
    return static_cast<mqbu::CapacityMeter*>(0);
}

inline bsls::Types::Int64 VirtualStorage::numMessages(
    BSLS_ANNOTATION_UNUSED const mqbu::StorageKey& appKey) const
{
    return d_guids.size();
}

inline bsls::Types::Int64 VirtualStorage::numBytes(
    BSLS_ANNOTATION_UNUSED const mqbu::StorageKey& appKey) const
{
    return d_totalBytes;
}

inline bool VirtualStorage::isEmpty() const
{
    // executed by *ANY* thread

    BSLS_ASSERT_OPT(false && "Should not be invoked.");
    // If needed, can be implemented in a manner similar to
    // 'mqbs::InMemoryStorage' and 'mqbs::FileBackedStorage'.

    return false;
}

inline bool VirtualStorage::hasMessage(const bmqt::MessageGUID& msgGUID) const
{
    return 1 == d_guids.count(msgGUID);
}

}  // close package namespace
}  // close enterprise namespace

#endif
