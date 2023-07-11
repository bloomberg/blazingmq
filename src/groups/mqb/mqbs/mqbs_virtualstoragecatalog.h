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

// BMQ
#include <bmqt_messageguid.h>

// BDE
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

/// A catalog of virtual storages associated with a queue.
class VirtualStorageCatalog {
  private:
    // PRIVATE TYPES
    typedef bsl::shared_ptr<VirtualStorage> VirtualStorageSp;

    /// appKey -> virtualStorage
    typedef bsl::unordered_map<mqbu::StorageKey, VirtualStorageSp>
        VirtualStorages;

    typedef VirtualStorages::iterator VirtualStoragesIter;

    typedef VirtualStorages::const_iterator VirtualStoragesConstIter;

  private:
    // DATA
    mqbi::Storage* d_storage_p;  // Physical storage underlying all
                                 // virtual storages known to this
                                 // object

    VirtualStorages d_virtualStorages;
    // Map of appKey to corresponding
    // virtual storage

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
    // TYPES
    typedef mqbi::Storage::AppIdKeyPair AppIdKeyPair;

    typedef mqbi::Storage::AppIdKeyPairs AppIdKeyPairs;

    // CREATORS

    /// Create an instance of virtual storage catalog with the specified
    /// `allocator`.
    VirtualStorageCatalog(mqbi::Storage* storage, bslma::Allocator* allocator);

    /// Destructor
    ~VirtualStorageCatalog();

    // MANIPULATORS

    /// Save the message having the specified `msgGUID`, `msgSize` and
    /// `rdaInfo` to the virtual storage associated with the specified
    /// `appKey`.  Note that if `appKey` is null, the message will be added
    /// to all virtual storages maintained by this instance.
    mqbi::StorageResult::Enum put(const bmqt::MessageGUID& msgGUID,
                                  int                      msgSize,
                                  const bmqp::RdaInfo&     rdaInfo,
                                  unsigned int             subScriptionId,
                                  const mqbu::StorageKey&  appKey);

    /// Get an iterator for items stored in the virtual storage identified
    /// by the specified `appKey`.  Iterator will point to point to the
    /// oldest item, if any, or to the end of the collection if empty.  Note
    /// that if `appKey` is null, an iterator over the underlying physical
    /// storage will be returned.  Also note that because `Storage` and
    /// `StorageIterator` are interfaces, the implementation of this method
    /// will allocate, so it's recommended to keep the iterator.
    /// TBD: Is the behavior undefined if `appKey` is null?
    bslma::ManagedPtr<mqbi::StorageIterator>
    getIterator(const mqbu::StorageKey& appKey);

    /// Load into the the specified `out` an iterator for items stored in
    /// the virtual storage identified by the specified `appKey`, initially
    /// pointing to the item associated with the specified `msgGUID`.
    /// Return zero on success, and a non-zero code if `msgGUID` was not
    /// found in the storage.  Note that if `appKey` is null, an iterator
    /// over the underlying physical storage will be returned.  Also note
    /// that because `Storage` and `StorageIterator` are interfaces, the
    /// implementation of this method will allocate, so it's recommended to
    /// keep the iterator.
    /// TBD: Is the behavior undefined if `appKey` is null?
    mqbi::StorageResult::Enum
    getIterator(bslma::ManagedPtr<mqbi::StorageIterator>* out,
                const mqbu::StorageKey&                   appKey,
                const bmqt::MessageGUID&                  msgGUID);

    /// Remove the message having the specified `msgGUID` from the storage
    /// for the client identified by the specified `appKey`.  If `appKey` is
    /// null, then remove the message from the storages for all clients.
    /// Return 0 on success, or a non-zero return code if the `msgGUID` was
    /// not found or the `appKey` is invalid.
    mqbi::StorageResult::Enum remove(const bmqt::MessageGUID& msgGUID,
                                     const mqbu::StorageKey&  appKey);

    /// Remove all messages from the storage for the client identified by
    /// the specified `appKey`.  If `appKey` is null, then remove messages
    /// for all clients.  Return one of the return codes from:
    /// * **e_SUCCESS**          : `msgGUID` was not found
    /// * **e_APPKEY_NOT_FOUND** : Invalid `appKey` specified
    mqbi::StorageResult::Enum removeAll(const mqbu::StorageKey& appKey);

    /// Create, if it doesn't exist already, a virtual storage instance with
    /// the specified `appId` and `appKey`.  Return zero upon success and a
    /// non-zero value otherwise, and populate the specified
    /// `errorDescription` with a brief reason in case of failure.
    int addVirtualStorage(bsl::ostream&           errorDescription,
                          const bsl::string&      appId,
                          const mqbu::StorageKey& appKey);

    /// Remove the virtual storage identified by the specified `appKey`.
    /// Return true if a virtual storage with `appKey` was found and
    /// deleted, false if a virtual storage with `appKey` does not exist.
    /// Behavior is undefined unless `appKey` is non-null.  Note that this
    /// method will delete the virtual storage, and any reference to it will
    /// become invalid after this method returns.
    bool removeVirtualStorage(const mqbu::StorageKey& appKey);

    mqbi::Storage* virtualStorage(const mqbu::StorageKey& appKey);

    // ACCESSORS

    /// Return the number of virtual storages registered with this instance.
    int numVirtualStorages() const;

    /// Return true if virtual storage identified by the specified `appKey`
    /// exists, otherwise return false.  Load into the optionally specified
    /// `appId` the appId associated with `appKey` if the virtual storage
    /// exists, otherwise set it to the empty string.
    bool hasVirtualStorage(const mqbu::StorageKey& appKey,
                           bsl::string*            appId = 0) const;

    /// Return true if virtual storage identified by the specified `appId`
    /// exists, otherwise return false.  Load into the optionally specified
    /// `appKey` the appKey associated with `appId` if the virtual storage
    /// exists, otherwise set it to the null key.
    bool hasVirtualStorage(const bsl::string& appId,
                           mqbu::StorageKey*  appKey = 0) const;

    /// Load into the specified `buffer` the list of pairs of appId and
    /// appKey for all the virtual storages registered with this instance.
    void loadVirtualStorageDetails(AppIdKeyPairs* buffer) const;

    /// Return the number of messages in the virtual storage associated with
    /// the specified `appKey`.  Behavior is undefined unless a virtual
    /// storage associated with the `appKey` exists in this catalog.
    bsls::Types::Int64 numMessages(const mqbu::StorageKey& appKey) const;

    /// Return the number of bytes in the virtual storage associated with
    /// the specified `appKey`.  Behavior is undefined unless a virtual
    /// storage associated with the `appKey` exists in this catalog.
    bsls::Types::Int64 numBytes(const mqbu::StorageKey& appKey) const;

    /// Return true if there is a virtual storage associated with any appKey
    /// which contains the specified `msgGUID`.  Return false otherwise.
    bool hasMessage(const bmqt::MessageGUID& msgGUID) const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ---------------------------
// class VirtualStorageCatalog
// ---------------------------

// ACCESSORS
inline int VirtualStorageCatalog::numVirtualStorages() const
{
    return d_virtualStorages.size();
}

inline bsls::Types::Int64
VirtualStorageCatalog::numMessages(const mqbu::StorageKey& appKey) const
{
    VirtualStoragesConstIter cit = d_virtualStorages.find(appKey);
    BSLS_ASSERT_SAFE(cit != d_virtualStorages.end());
    return cit->second->numMessages(appKey);
}

inline bsls::Types::Int64
VirtualStorageCatalog::numBytes(const mqbu::StorageKey& appKey) const
{
    VirtualStoragesConstIter cit = d_virtualStorages.find(appKey);
    BSLS_ASSERT_SAFE(cit != d_virtualStorages.end());
    return cit->second->numBytes(appKey);
}

}  // close package namespace
}  // close enterprise namespace

#endif
