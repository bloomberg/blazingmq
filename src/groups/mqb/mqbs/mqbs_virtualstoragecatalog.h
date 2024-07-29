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

// mqbs_virtualstoragecatalog.h                                       -*-C++-*-
#ifndef INCLUDED_MQBS_VIRTUALSTORAGECATALOG
#define INCLUDED_MQBS_VIRTUALSTORAGECATALOG

//@PURPOSE: Provide a catalog of virtual storages associated with a queue.
//
//@CLASSES:
//  mqbs::VirtualStorageCatalog: Catalog of virtual storages
//
//@DESCRIPTION: 'mqbs::VirtualStorageCatalog' provides a collection of virtual
// storages associated with a queue.

// MQB

#include <mqbi_storage.h>
#include <mqbs_virtualstorage.h>
#include <mqbu_storagekey.h>

// MWC
#include <mwcc_twokeyhashmap.h>

// BMQ
#include <bmqt_messageguid.h>

// BDE
#include <bsl_list.h>
#include <bsl_memory.h>
#include <bsl_ostream.h>
#include <bsl_string.h>
#include <bsl_unordered_map.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>

namespace BloombergLP {

namespace mqbs {

// ===========================
// class VirtualStorageCatalog
// ===========================

// The owner of so-called Virtual Storage(s) implemented as
// 'VirtualStorage::DataStream'.  Both 'FileBasedStorage' and 'InMemoryStorage'
// own an instance of 'VirtualStorageCatalog'.  The access to which is done in
// two ways.
// 1) While the 'mgbi::Storage' does not expose a reference to this
// instance, most of the 'mgbi::Storage' calls result in calling the
// corresponding 'VirtualStorageCatalog' method.
// 2) Both 'RootQueueEngine' and 'RelayQueueEngine' access the Virtual Storage
// by an 'mqbi::StorageIterator' implemented by 'mqbs::StorageIterator'.
//
// The purpose of Virtual Storage is to keep state of (guid, App) pairs for
// delivery by QueueEngines.  'App' is identified by 'appKey' and an ordinal -
// offset in the consecutive memory ('VirtualStorage::DataStreamMessage')
// holding all Apps states ('mqbi::AppMessage') for each guid.

class VirtualStorageCatalog {
  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBS.VIRTUALSTORAGE");

  public:
    // TYPES
    typedef mqbi::Storage::AppIdKeyPair AppIdKeyPair;

    typedef mqbi::Storage::AppIdKeyPairs AppIdKeyPairs;

    typedef unsigned int Ordinal;

  private:
    // PRIVATE TYPES
    typedef bsl::shared_ptr<VirtualStorage> VirtualStorageSp;

    /// List of available ordinal values for Virtual Storages.
    typedef bsl::list<Ordinal> AvailableOrdinals;

    /// appKey -> virtualStorage
    typedef mwcc::
        TwoKeyHashMap<bsl::string, mqbu::StorageKey, VirtualStorageSp>
            VirtualStorages;

    typedef VirtualStorages::iterator VirtualStoragesIter;

    typedef VirtualStorages::const_iterator VirtualStoragesConstIter;

  public:
    // TYPES

    /// Access to the DataStream
    typedef VirtualStorage::DataStream::iterator DataStreamIterator;

  private:
    // DATA
    mqbi::Storage* d_storage_p;  // Physical storage underlying all
                                 // virtual storages known to this
                                 // object

    VirtualStorages d_virtualStorages;
    // Map of appKey to corresponding
    // virtual storage

    AvailableOrdinals d_availableOrdinals;
    // available ordinal values for Virtual Storages.

    Ordinal d_nextOrdinal;
    // Monotonically increasing value to generate new ordinal.

    VirtualStorage::DataStream d_dataStream;
    // The DataStream tracking all Apps states.

    bsls::Types::Int64 d_totalBytes;
    // Cumulative count of all bytes.

    bsls::Types::Int64 d_numMessages;
    // Cumulative count of all messages.

    mqbi::AppMessage d_defaultAppMessage;
    // The default App state

    bslma::Allocator* d_allocator_p;  // Allocator to use

  private:
    // NOT IMPLEMENTED
    VirtualStorageCatalog(const VirtualStorageCatalog&);  // = delete
    VirtualStorageCatalog&
    operator=(const VirtualStorageCatalog&);  // = delete

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(VirtualStorageCatalog,
                                   bslma::UsesBslmaAllocator)

  public:
    // CREATORS

    /// Create an instance of virtual storage catalog with the specified
    /// 'defaultRdaInfo' and 'allocator'.
    VirtualStorageCatalog(mqbi::Storage* storage, bslma::Allocator* allocator);

    /// Destructor
    ~VirtualStorageCatalog();

    // MANIPULATORS

    /// If the specified 'where' is unset, return reference to the beginning of
    /// the DataStream.  Otherwise, return reference to the corresponding item
    /// in the DataStream.
    /// If item is not found, return reference to the end of the DataStream.
    DataStreamIterator
    begin(const bmqt::MessageGUID& where = bmqt::MessageGUID());

    /// Return reference to the end of the DataStream.
    DataStreamIterator end();

    /// Return reference to the item in the DataStream corresponding to the
    /// specified 'msgGUID' and allocate space for all Apps states if needed.
    DataStreamIterator get(const bmqt::MessageGUID& msgGUID);

    /// Allocate space for all Apps states in the specified 'data' if needed.
    void setup(VirtualStorage::DataStreamMessage* data);

    /// Save the message having the specified 'msgGUID' and 'msgSize' to the
    /// DataStream.  If the specified 'out' is not '0', allocate space for all
    /// Apps states and load the created object into the 'out'.
    mqbi::StorageResult::Enum put(const bmqt::MessageGUID&            msgGUID,
                                  int                                 msgSize,
                                  VirtualStorage::DataStreamMessage** out = 0);

    /// Get an iterator for items stored in the DataStream identified by the
    /// specified 'appKey'.
    /// If the 'appKey' is null, the returned  iterator can iterate states of
    /// all Apps; otherwise, the iterator can iterate states of the App
    /// corresponding to the 'appKey'.
    bslma::ManagedPtr<mqbi::StorageIterator>
    getIterator(const mqbu::StorageKey& appKey);

    /// Load into the specified 'out' an iterator for items stored in the
    /// DataStream initially pointing to the item associated with the specified
    /// 'msgGUID'.
    /// If the 'appKey' is null, the returned  iterator can iterate states of
    /// all Apps; otherwise, the iterator can iterate states of the App
    /// corresponding to the 'appKey'.
    /// Return zero on success, and a non-zero code if 'msgGUID' was not
    /// found in the storage.
    mqbi::StorageResult::Enum
    getIterator(bslma::ManagedPtr<mqbi::StorageIterator>* out,
                const mqbu::StorageKey&                   appKey,
                const bmqt::MessageGUID&                  msgGUID);

    /// Remove the message having the specified 'msgGUID' from the DataStream.
    /// Return 0 on success, or a non-zero return code if the 'msgGUID' was
    /// not found.
    mqbi::StorageResult::Enum remove(const bmqt::MessageGUID& msgGUID);

    /// Remove the message having the specified 'msgGUID' from the DataStream
    /// and update the counts of bytes and messages according to GC logic.
    /// Return 0 on success, or a non-zero return code if the 'msgGUID' was
    /// not found.
    mqbi::StorageResult::Enum gc(const bmqt::MessageGUID& msgGUID);

    /// Update the App state corresponding to the specified 'msgGUID' and the
    /// specified 'appKey' in the DataStream.
    /// Return 0 on success, or a non-zero return code if the 'msgGUID' was
    /// not found.
    /// Behavior is undefined unless there is an App with the 'appKey'.
    mqbi::StorageResult::Enum confirm(const bmqt::MessageGUID& msgGUID,
                                      const mqbu::StorageKey&  appKey);

    /// If the specified 'appKey' is null, erase the entire DataStream;
    /// otherwise, erase all states of the App corresponding to the 'appKey'.
    mqbi::StorageResult::Enum removeAll(const mqbu::StorageKey& appKey);

    /// Create, if it doesn't exist already, a virtual storage instance with
    /// the specified 'appId' and 'appKey'.  Return zero upon success and a
    /// non-zero value otherwise, and populate the specified
    /// 'errorDescription' with a brief reason in case of failure.
    /// Behavior is undefined if the 'appKey' is not valid.
    int addVirtualStorage(bsl::ostream&           errorDescription,
                          const bsl::string&      appId,
                          const mqbu::StorageKey& appKey);

    /// If the specified 'appKey' is null, erase the entire DataStream and
    /// all Virtual Storage instances; erase all states of the App
    /// corresponding to the 'appKey' and remove the corresponding Virtual
    /// Storage instance.
    bool removeVirtualStorage(const mqbu::StorageKey& appKey);

    /// Return the Virtual Storage instance corresponding to the specified
    /// 'appKey'.
    VirtualStorage* virtualStorage(const mqbu::StorageKey& appKey);

    /// (Auto)Confirm the specified 'msgGUID' for the specified 'appKey'.
    /// Behavior is undefined unless there is an App with the 'appKey'.
    void autoConfirm(VirtualStorage::DataStreamMessage* dataStreamMessage,
                     const mqbu::StorageKey&            appKey);

    /// Set the default RDA according to the specified 'maxDeliveryAttempts'.
    void setDefaultRda(int maxDeliveryAttempts);

    // ACCESSORS

    /// Return the number of virtual storages registered with this instance.
    int numVirtualStorages() const;

    /// Return true if virtual storage identified by the specified 'appKey'
    /// exists, otherwise return false.  Load into the optionally specified
    /// 'appId' the appId associated with 'appKey' if the virtual storage
    /// exists, otherwise set it to the empty string.
    bool hasVirtualStorage(const mqbu::StorageKey& appKey,
                           bsl::string*            appId = 0) const;

    /// Return true if virtual storage identified by the specified 'appId'
    /// exists, otherwise return false.  Load into the optionally specified
    /// 'appKey' and 'ordinal' the appKey and ordinal associated with 'appId'
    /// if the virtual storage exists, otherwise set it to the null key.
    bool hasVirtualStorage(const bsl::string& appId,
                           mqbu::StorageKey*  appKey  = 0,
                           unsigned int*      ordinal = 0) const;

    /// Load into the specified 'buffer' the list of pairs of appId and
    /// appKey for all the virtual storages registered with this instance.
    void loadVirtualStorageDetails(AppIdKeyPairs* buffer) const;

    /// Return the number of messages in the virtual storage associated with
    /// the specified 'appKey'.  Behavior is undefined unless a virtual
    /// storage associated with the 'appKey' exists in this'og.
    bsls::Types::Int64 numMessages(const mqbu::StorageKey& appKey) const;

    /// Return the number of bytes in the virtual storage associated with
    /// the specified 'appKey'.  Behavior is undefined unless a virtual
    /// storage associated with the 'appKey' exists in this catalog.
    bsls::Types::Int64 numBytes(const mqbu::StorageKey& appKey) const;

    /// Return the default App state.
    const mqbi::AppMessage& defaultAppMessage() const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ---------------------------
// class VirtualStorageCatalog
// ---------------------------

inline void VirtualStorageCatalog::setDefaultRda(int maxDeliveryAttempts)
{
    if (maxDeliveryAttempts > 0) {
        d_defaultAppMessage.d_rdaInfo.setCounter(maxDeliveryAttempts);
    }
    else {
        d_defaultAppMessage.d_rdaInfo.setUnlimited();
    }
}

// ACCESSORS
inline int VirtualStorageCatalog::numVirtualStorages() const
{
    return d_virtualStorages.size();
}

inline bsls::Types::Int64
VirtualStorageCatalog::numMessages(const mqbu::StorageKey& appKey) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!appKey.isNull());

    VirtualStoragesConstIter cit = d_virtualStorages.findByKey2(appKey);
    BSLS_ASSERT_SAFE(cit != d_virtualStorages.end());
    return d_numMessages - cit->value()->numRemoved();
}

inline bsls::Types::Int64
VirtualStorageCatalog::numBytes(const mqbu::StorageKey& appKey) const
{
    VirtualStoragesConstIter cit = d_virtualStorages.findByKey2(appKey);
    BSLS_ASSERT_SAFE(cit != d_virtualStorages.end());

    return d_totalBytes - cit->value()->removedBytes();
}

inline const mqbi::AppMessage& VirtualStorageCatalog::defaultAppMessage() const
{
    return d_defaultAppMessage;
}

}  // close package namespace
}  // close enterprise namespace

#endif
