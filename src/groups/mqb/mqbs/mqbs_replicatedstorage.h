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
#include <bsl_vector.h>

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
    /// from the underlying persistent store.  The record is not replicated to
    /// peer nodes.  The specified `isOwn` is true when this node wrote the
    /// record itself, in which case `put` already accounted for the message's
    /// capacity; false when the record was received or recovered, where it is
    /// charged here.
    virtual void processMessageRecord(const bmqt::MessageGUID&     guid,
                                      unsigned int                 msgLen,
                                      unsigned int                 refCount,
                                      const DataStoreRecordHandle& handle,
                                      bool                         isOwn) = 0;

    /// Process the CONFIRM record having the specified `guid`, `appKey`, and
    /// `reason`.  Use the specified `handle` to retrieve the confirm record
    /// from the underlying persistent store.  Note that `appKey` can be null.
    /// The record is not replicated to peer nodes.  The specified `isOwn` is
    /// true when this node wrote the record itself, in which case the propose
    /// side has already moved the app view and this call only completes the
    /// message-level state the journal owns.
    virtual void processConfirmRecord(const bmqt::MessageGUID&     guid,
                                      const mqbu::StorageKey&      appKey,
                                      ConfirmReason::Enum          reason,
                                      const DataStoreRecordHandle& handle,
                                      bool                         isOwn) = 0;

    /// Process the DELETION having the specified `guid`.  Note that this
    /// routine is supposed to be invoked at replica nodes, and the record
    /// will not be replicated to peer nodes.
    virtual void processDeletionRecord(const bmqt::MessageGUID& guid) = 0;

    /// Give back the capacity `put` reserved for a proposed message of the
    /// specified `msgLen`.  Invoked when the message reaches
    /// `processMessageRecord` -- which charges the capacity for real -- and
    /// when the write is dropped instead, because the log truncated it or
    /// this node lost primaryship before it committed.
    virtual void undoCapacity(unsigned int msgLen) = 0;

    /// Undo what `confirm` did when it wrote a CONFIRM record for the message
    /// having the specified `guid` and the App identified by the specified
    /// `appKey`, for a record that will never commit: the place it took in
    /// the in-flight CONFIRM count, and the move it made in that App's view.
    virtual void undoConfirm(const bmqt::MessageGUID& guid,
                             const mqbu::StorageKey&  appKey) = 0;

    /// Write the records that authorize removing the App identified by the
    /// specified `appKey`: a purge of the messages it can see, and its
    /// deletion.  Return 0 on success.  The removal itself is applied by
    /// `removeVirtualStorage`, at commit on a Raft partition.
    virtual int writeAppRemoval(const mqbu::StorageKey& appKey) = 0;

    /// Add the specified `handle` which represents a QUEUEOP record for the
    /// queue associated with this storage.
    virtual void
    addQueueOpRecordHandle(const DataStoreRecordHandle& handle) = 0;

    /// Purge the virtual storage associated with the specified `appKey`.
    /// If `appKey` is null, purge the physical as well as all virtual
    /// storages.  Note that this routine does not write a record; the caller
    /// has already written (or received) the QueueOp that authorizes it.
    virtual void purge(const mqbu::StorageKey& appKey) = 0;

    /// Notify the storage of node role set to primary
    virtual void setPrimary() = 0;

    /// Calculate offsets of all Apps (after recovery) in the data stream.
    /// An App offset is the number of messages older than the App.
    virtual void calibrate() = 0;

    // ACCESSORS

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
