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

// mqbs_replicatedstorage.h                                           -*-C++-*-
#ifndef INCLUDED_MQBS_REPLICATEDSTORAGE
#define INCLUDED_MQBS_REPLICATEDSTORAGE

//@PURPOSE: Provide an interface for a replicated storage
//
//@CLASSES:
//  mqbs::ReplicatedStorage: Interface for the replicated storage
//
//@DESCRIPTION: 'mqbs::ReplicatedStorage' provides a pure protocol for storage
// in a clustered environment, where a given storage is replicated on several
// nodes in a cluster.
//
/// Thread Safety
///-------------
// Components implementing the 'mqbs::ReplicatedStorage' interface are *NOT*
// required to be thread safe.

// MQB

#include <mqbi_storage.h>
#include <mqbs_datastore.h>
#include <mqbu_storagekey.h>

// BMQ
#include <bmqt_messageguid.h>

namespace BloombergLP {
namespace mqbs {

// =======================
// class ReplicatedStorage
// =======================

class ReplicatedStorage : public mqbi::Storage {
    // TBD

  public:
    // TYPES
    typedef bsl::vector<DataStoreRecordHandle> RecordHandles;

  public:
    // CREATORS

    /// Destructor
    ~ReplicatedStorage() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    /// Process the MESSAGE record having the specified `guid`, `msgLen` and
    /// `refCount`, and use the specified `handle` to retrieve the message
    /// from the underlying persistent store.  Note that this routine is
    /// supposed to be invoked at replica nodes, and the record will not be
    /// replicated to peer nodes.
    virtual void processMessageRecord(const bmqt::MessageGUID&     guid,
                                      unsigned int                 msgLen,
                                      unsigned int                 refCount,
                                      const DataStoreRecordHandle& handle) = 0;

    /// Process the CONFIRM record having the specified `guid` and `appKey`
    /// and use the specified `handle` to retrieve the confirm record from
    /// the underlying persistent store.  Note that `appKey` can be null.
    /// Also note that this routine is supposed to be invoked at replica
    /// nodes, and the record will not be replicated to peer nodes.
    virtual void processConfirmRecord(const bmqt::MessageGUID&     guid,
                                      const mqbu::StorageKey&      appKey,
                                      ConfirmReason::Enum          reason,
                                      const DataStoreRecordHandle& handle) = 0;

    /// Process the DELETION having the specified `guid`.  Note that this
    /// routine is supposed to be invoked at replica nodes, and the record
    /// will not be replicated to peer nodes.
    virtual void processDeletionRecord(const bmqt::MessageGUID& guid) = 0;

    /// Add the specified `handle` which represents a QUEUEOP record for the
    /// queue associated with this storage.
    virtual void
    addQueueOpRecordHandle(const DataStoreRecordHandle& handle) = 0;

    /// Purge the virtual storage associated with the specified `appKey`.
    /// If `appKey` is null, purge the physical as well as all virtual
    /// storages.  Note that this routine is supposed to be invoked at
    /// replica nodes, and the record will not be replicated to peer nodes.
    virtual void purge(const mqbu::StorageKey& appKey) = 0;

    /// Return a non-modifiable list of handles of all QUEUEOP records
    /// associated with this storage.
    virtual const RecordHandles& queueOpRecordHandles() const = 0;

    // Return 'true' if the storage is of the strong consistency
    virtual bool isStrongConsistency() const = 0;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

}  // close package namespace
}  // close enterprise namespace

#endif
