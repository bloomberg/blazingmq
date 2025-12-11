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

#include <bmqc_orderedhashmap.h>

// BDE
#include <bdlbb_blob.h>
#include <bsl_memory.h>
#include <bsla_annotations.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_assert.h>
#include <bsls_types.h>

namespace BloombergLP {

namespace mqbs {

// FORWARD DECLARATION
class VirtualStorageCatalog;

// ====================
// class VirtualStorage
// ====================

class VirtualStorage {
    // This Mechanism represents one App in a Storage (FileBased or InMemory)

  public:
    /// msgGUID -> MessageContext
    /// Must be a container in which iteration order is same as insertion
    /// order.
    typedef bmqc::OrderedHashMap<bmqt::MessageGUID,
                                 mqbi::DataStreamMessage,
                                 bslh::Hash<bmqt::MessageGUIDHashAlgo> >
        DataStream;

    typedef DataStream::iterator DataStreamIterator;

  private:
    // DATA
    bslma::Allocator* d_allocator_p;

    mqbi::Storage* d_storage_p;
    // underlying 'real' storage.  Held.

    bsl::string d_appId;
    // AppId of the App which this instance represents.

    mqbu::StorageKey d_appKey;
    // Storage key of the associated 'appId'.

    bsls::Types::Int64 d_removedBytes;
    // Cumulative count of all bytes _removed_ from this App
    // The owner 'VirtualStorageCatalog' keeps track of all bytes

    bsls::Types::Int64 d_numRemoved;
    // Cumulative count of all messages _removed_ from this App
    // The owner 'VirtualStorageCatalog' keeps track of all messages

    unsigned int d_ordinal;
    // The ordinal to locate corresponding state in 'DataStreamMessage'

  private:
    // NOT IMPLEMENTED
    VirtualStorage(const VirtualStorage&);             // = delete
    VirtualStorage& operator=(const VirtualStorage&);  // = delete

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(VirtualStorage, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create an instance of virtual storage backed by the specified real
    /// `storage`, and having the specified `appId`, and `appKey`, and use
    /// the specified `allocator` for any memory allocations.  Behavior is
    /// undefined unless `storage` is non-null, `appId` is non-empty and
    /// `appKey` is non-null.  Note that the specified real `storage` must
    /// outlive this virtual storage instance.
    /// The specified 'ordinal' uniquely identifies an offset for this instance
    /// in 'DataStreamMessage'.
    VirtualStorage(mqbi::Storage*          storage,
                   const bsl::string&      appId,
                   const mqbu::StorageKey& appKey,
                   unsigned int            ordinal,
                   bsls::Types::Int64      numMessagesSoFar,
                   bslma::Allocator*       allocator);

    /// Destructor.
    ~VirtualStorage();

    // ACCESSORS

    /// Return the appId associated with this storage instance.  Note that
    /// the returned string is always non-empty.
    const bsl::string& appId() const;

    /// Return the app key, if any, associated with this storage instance.
    /// Note that the returned key is always non-null.
    const mqbu::StorageKey& appKey() const;

    bsls::Types::Int64 numRemoved() const;
    // Return the number of messages in the storage.

    bsls::Types::Int64 removedBytes() const;
    // Return the number of bytes in the storage.

    /// Return `true` if there was Replication Receipt for the specified
    /// `msgGUID`.
    bool hasReceipt(const bmqt::MessageGUID& msgGUID) const;

    /// Return the unique offset of this instance in 'DataStreamMessage'.
    unsigned int ordinal() const;

    // MANIPULATORS

    /// Change the state of this App in the specified 'dataStreamMessage' to
    /// indicate CONFIRM.
    mqbi::StorageResult::Enum
    confirm(mqbi::DataStreamMessage* dataStreamMessage);

    /// Change the state of this App in the specified 'dataStreamMessage' to
    /// indicate removal (by a purge or unregistration).
    mqbi::StorageResult::Enum
    remove(mqbi::DataStreamMessage* dataStreamMessage);

    bool remove(mqbi::DataStreamMessage* dataStreamMessage,
                unsigned int             replacingOrdinal);

    /// Update bytes by the specified 'size' and messages counts by `1` for
    /// this App as the result of garbage-collecting the message.
    void onGC(int size);

    /// Reset bytes and messages counts as in the case of purging all Apps.
    void resetStats();

    void replaceOrdinal(unsigned int replacingOrdinal);

    void setNumRemoved(bsls::Types::Int64 numRemoved,
                       bsls::Types::Int64 bytes);
};

// =====================
// class StorageIterator
// =====================

class StorageIterator : public mqbi::StorageIterator {
    // Mechanism to provide access to both underlying real storage (FileBased
    // or InMemory) and all App states in Virtual Storage.

  private:
    // DATA
    mqbi::Storage* d_storage_p;
    // underlying 'real' storage (FileBased or InMemory).

    VirtualStorageCatalog* d_owner_p;
    // The owner and creator.

    VirtualStorage::DataStreamIterator d_iterator;
    // Access to App states in Virtual Storages.

    mutable mqbi::StorageMessageAttributes d_attributes;

    mutable bsl::shared_ptr<bdlbb::Blob> d_appData_sp;
    // If this variable is empty, it is
    // assumed that attributes, message,
    // and options have not been loaded in
    // this iteration (see also
    // 'loadMessageAndAttributes' impl).

    mutable bsl::shared_ptr<bdlbb::Blob> d_options_sp;

    mutable bool d_haveReceipt;
    // Cached value.

  private:
    // NOT IMPLEMENTED
    StorageIterator(const StorageIterator&);             // = delete
    StorageIterator& operator=(const StorageIterator&);  // = delete

  private:
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

    /// Create a new VirtualStorageIterator for the specified 'storage' and
    /// 'owner' pointing at the specified 'initialPosition'.
    StorageIterator(
        mqbi::Storage*                              storage,
        VirtualStorageCatalog*                      owner,
        const VirtualStorage::DataStream::iterator& initialPosition);

    /// Destructor
    ~StorageIterator() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    /// Clear any cached data associated with this iterator, if any.
    /// The cache might be initialized within `appData`, `options` or
    /// `attributes` routines.
    /// TODO: refactor iterators to remove cached data.
    void clearCache() BSLS_KEYWORD_OVERRIDE;

    bool advance() BSLS_KEYWORD_OVERRIDE;

    /// If the specified 'where' is unset, reset the iterator to point to the
    /// to the beginning of the Virtual Storage.  Otherwise, reset the
    /// iterator to point to the item corresponding to the 'where'.  If the
    /// item is not found, reset the iterator to the end of the storage.
    void reset(const bmqt::MessageGUID& where = bmqt::MessageGUID())
        BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS

    /// Return a reference offering non-modifiable access to the guid
    /// associated to the item currently pointed at by this iterator.  The
    /// behavior is undefined unless `atEnd` returns `false`.
    const bmqt::MessageGUID& guid() const BSLS_KEYWORD_OVERRIDE;

    /// Return a reference offering non-modifiable access to the App state
    /// corresponding to the specified 'ordinal' and the item currently pointed
    /// at by this iterator.
    /// The behavior is undefined unless `atEnd` returns `false`.
    const mqbi::AppMessage&
    appMessageView(unsigned int appOrdinal) const BSLS_KEYWORD_OVERRIDE;

    /// Return a reference offering modifiable access to the App state
    /// corresponding to the specified 'ordinal' and the item currently pointed
    /// at by this iterator.
    /// The behavior is undefined unless `atEnd` returns `false`.
    mqbi::AppMessage&
    appMessageState(unsigned int appOrdinal) BSLS_KEYWORD_OVERRIDE;

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

    bool asFarAs(const bdlb::Variant<bsls::Types::Uint64, bmqt::MessageGUID>&
                     stop) const BSLS_KEYWORD_OVERRIDE;
};

// ============================
// class VirtualStorageIterator
// ============================

class VirtualStorageIterator : public StorageIterator {
    // Mechanism to provide access to both underlying real storage (FileBased
    // or InMemory) and one App states in Virtual Storage.
  private:
    // DATA
    VirtualStorage* d_virtualStorage_p;

  private:
    // NOT IMPLEMENTED
    VirtualStorageIterator(const VirtualStorageIterator&);  // = delete
    VirtualStorageIterator&
    operator=(const VirtualStorageIterator&);  // = delete

  public:
    // CREATORS

    /// Create a new VirtualStorageIterator for the specified 'storage' and
    /// 'owner' pointing at the specified 'initialPosition'.  The specified
    /// 'virtualStorage' identifies the App which states this object iterates.
    VirtualStorageIterator(
        VirtualStorage*                             virtualStorage,
        mqbi::Storage*                              storage,
        VirtualStorageCatalog*                      owner,
        const VirtualStorage::DataStream::iterator& initialPosition);

    /// Destructor
    ~VirtualStorageIterator() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    /// Advance the iterator to the next messages which has the App state as
    /// pending.
    bool advance() BSLS_KEYWORD_OVERRIDE;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// --------------------
// class VirtualStorage
// --------------------

// ACCESSORS

inline const bsl::string& VirtualStorage::appId() const
{
    return d_appId;
}

inline const mqbu::StorageKey& VirtualStorage::appKey() const
{
    return d_appKey;
}

inline bsls::Types::Int64 VirtualStorage::numRemoved() const
{
    return d_numRemoved;  // TODO
}

inline bsls::Types::Int64 VirtualStorage::removedBytes() const
{
    return d_removedBytes;
}

}  // close package namespace
}  // close enterprise namespace

#endif
