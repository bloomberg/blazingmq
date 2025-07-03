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

// mqbs_filestoreprintutil.h                                          -*-C++-*-
#ifndef INCLUDED_MQBS_FILESTOREPRINTUTIL
#define INCLUDED_MQBS_FILESTOREPRINTUTIL

//@PURPOSE: Provide utilities for printing for BlazingMQ file store.
//
//@CLASSES:
//  mqbs::FileStorePrintUtil: Printing utilities for BlazingMQ file store.
//
//@SEE ALSO: mqbs::FileStore
//
//@DESCRIPTION: 'mqbs::FileStoreUtil' provides utilities for printing for a
//  BlazingMQ file store.

// MQB
#include <mqbnet_cluster.h>
#include <mqbs_fileset.h>
#include <mqbs_replicatedstorage.h>
#include <mqbs_storagecollectionutil.h>

// BDE
#include <bsl_memory.h>
#include <bsl_vector.h>
#include <bsls_types.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace mqbcmd {
class FileStoreSummary;
}
namespace mqbcmd {
class StorageContent;
}
namespace mqbcmd {
class StorageQueueInfo;
}

namespace mqbs {

// =========================
// struct FileStorePrintUtil
// =========================

/// This component provides utilities for printing for a BlazingMQ file
/// store.
struct FileStorePrintUtil {
  private:
    // PRIVATE TYPES
    typedef bsl::vector<bsl::shared_ptr<FileSet> > FileSets;

    typedef StorageCollectionUtil::StorageList          StorageList;
    typedef StorageCollectionUtil::StorageListConstIter StorageListConstIter;

    typedef StorageCollectionUtil::StoragesMap StoragesMap;

  public:
    // CLASS METHODS

    /// Load the summary of the specified `fileStore` having the specified
    /// `primaryNode`, `primaryLeaseId`, `sequenceNum`,
    /// `numOutstandingRecords`, `fileSets` and `storageMap` into the
    /// specified `summary` object.
    static void loadSummary(mqbcmd::FileStoreSummary*  summary,
                            const mqbnet::ClusterNode* primaryNode,
                            unsigned int               primaryLeaseId,
                            bsls::Types::Uint64        sequenceNum,
                            size_t                     numOutstandingRecords,
                            size_t                     numUnreceiptedMessages,
                            int                        naglePacketCount,
                            const FileSets&            fileSets,
                            const StoragesMap&         storageMap);

    /// Load the status of the queue stored in the specified `storage` to
    /// the specified `queueInfo` object.
    static void loadQueueStatus(mqbcmd::StorageQueueInfo* queueInfo,
                                const ReplicatedStorage*  storage);

    /// Load the status of the queues stored in the specified `storages` to
    /// the specified `storageContent` internal list.  The optionally
    /// specified `maxNumQueues` indicates the maximum number of queues to
    /// load before terminating.
    static void loadQueuesStatus(const StorageList&      storages,
                                 mqbcmd::StorageContent* storageContent,
                                 unsigned int maxNumQueues = INT_MAX);
};

}  // close package namespace
}  // close enterprise namespace

#endif
