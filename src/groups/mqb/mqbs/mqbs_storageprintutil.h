// Copyright 2018-2023 Bloomberg Finance L.P.
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

// mqbs_storageprintutil.h                                            -*-C++-*-
#ifndef INCLUDED_MQBS_STORAGEPRINTUTIL
#define INCLUDED_MQBS_STORAGEPRINTUTIL

//@PURPOSE: Provide utilities for printing for BlazingMQ storage.
//
//@CLASSES:
//  mqbs::StoragePrintUtil: Printing utilities for BlazingMQ storage.
//
//@DESCRIPTION: 'mqbs::StoragePrintUtil' provides utilities for printing
//  various information about a BlazingMQ Storage.

// MQB

#include <mqbi_storage.h>
#include <mqbi_storagemanager.h>

// BDE
#include <bsl_memory.h>
#include <bsl_ostream.h>
#include <bsl_string.h>
#include <bsl_vector.h>
#include <bsls_types.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace bslmt {
class Mutex;
}
namespace mqbcmd {
class Message;
}
namespace mqbcmd {
class QueueContents;
}
namespace mqbi {
class Storage;
}
namespace mqbs {
class FileStore;
}

namespace mqbs {

// =======================
// struct StoragePrintUtil
// =======================

/// This `struct` provides utilities for printing for BlazingMQ storage.
struct StoragePrintUtil {
  private:
    // PRIVATE TYPES
    typedef mqbi::StorageManager::AppInfos AppInfos;

  public:
    // TYPES
    typedef mqbi::StorageManager::StorageSpMap          StorageSpMap;
    typedef mqbi::StorageManager::StorageSpMapConstIter StorageSpMapConstIter;

    typedef bsl::shared_ptr<mqbs::FileStore> FileStoreSp;
    typedef bsl::vector<FileStoreSp>         FileStores;

  public:
    // CLASS METHODS

    /// Populate the attributes of the specified `message` object from the
    /// message pointed at the specified `storageIter` from the specified
    /// `storage`.  Return 0 on success or a non-zero value on error.
    static int listMessage(mqbcmd::Message*             message,
                           const mqbi::Storage*         storage,
                           const mqbi::StorageIterator& storageIter);

    /// Add to the specified `queueContents` up to the specified `count` of
    /// messages queued in the specified `storage`, for the specified
    /// `appId`, starting at the specified `offset`.  If `offset` is
    /// negative, it is relative to the position just past the *last*
    /// message.  If `count` is negative, add `-count` messages *preceding*
    /// the specified starting position.  If `count` is zero, add all the
    /// messages starting at and after the specified position.  Return zero
    /// if successful or a non-zero value on error. Executed in the
    /// dispatcher thread associated with the queue associated with the
    /// `storage`.
    static int listMessages(mqbcmd::QueueContents* queueContents,
                            const bsl::string&     appId,
                            bsls::Types::Int64     offset,
                            bsls::Types::Int64     count,
                            mqbi::Storage*         storage);

    /// Print to the specified `out` a summary of the recovered storages of
    /// the specified `storageMap` belonging to the specified `partitionId`,
    /// locking the specified `storagesLock` and using the specified
    /// `clusterDescription` and `recoveryStartTime`.
    static void
    printRecoveredStorages(bsl::ostream&            out,
                           bslmt::Mutex*            storagesLock,
                           const StorageSpMap&      storageMap,
                           int                      partitionId,
                           const bsl::string&       clusterDescription,
                           const bsls::Types::Int64 recoveryStartTime);

    /// Print to the specified `out` a summary message upon storage recovery
    /// completion using the specified `fileStores` and
    /// `clusterDescription`.  Return true if all partitions are opened
    /// successfully, false otherwise.
    static bool
    printStorageRecoveryCompletion(bsl::ostream&      out,
                                   const FileStores&  fileStores,
                                   const bsl::string& clusterDescription);
};

}  // close package namespace
}  // close enterprise namespace

#endif
