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

// mqbs_storageprintutil.cpp                                          -*-C++-*-
#include <mqbs_storageprintutil.h>

#include <mqbscm_version.h>
// MQB
#include <mqbcmd_messages.h>
#include <mqbi_queueengine.h>
#include <mqbs_filestore.h>
#include <mqbs_replicatedstorage.h>
#include <mqbs_storageutil.h>
#include <mqbu_storagekey.h>

#include <bmqsys_time.h>
#include <bmqu_memoutstream.h>
#include <bmqu_printutil.h>

// BMQ
#include <bmqp_protocolutil.h>

// BDE
#include <bdlma_localsequentialallocator.h>
#include <bdlt_datetime.h>
#include <bdlt_datetimetz.h>
#include <bsl_algorithm.h>
#include <bslmt_lockguard.h>
#include <bslmt_mutex.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace mqbs {

// -----------------------
// struct StoragePrintUtil
// -----------------------

// CLASS METHODS
int StoragePrintUtil::listMessage(mqbcmd::Message*             message,
                                  const mqbi::Storage*         storage,
                                  const mqbi::StorageIterator& storageIter)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(storage);
    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS = 0  // Success
        ,
        rc_ITER_AT_END = -1  // Iterator is pointing at the end of the queue
    };

    if (storageIter.atEnd()) {
        return rc_ITER_AT_END;  // RETURN
    }

    bdlt::Datetime arrivalDatetime;
    StorageUtil::loadArrivalTime(&arrivalDatetime, storageIter.attributes());
    int msgSize = -1;
    storage->getMessageSize(&msgSize, storageIter.guid());

    bmqu::MemOutStream guid;
    guid << storageIter.guid();
    message->guid()             = guid.str();
    message->arrivalTimestamp() = bdlt::DatetimeTz(arrivalDatetime, 0);
    message->sizeBytes()        = msgSize;

    return rc_SUCCESS;
}

int StoragePrintUtil::listMessages(mqbcmd::QueueContents* queueContents,
                                   const bsl::string&     appId,
                                   bsls::Types::Int64     offset,
                                   bsls::Types::Int64     count,
                                   mqbi::Storage*         storage)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(storage);

    mqbu::StorageKey appKey;

    if (appId.empty() || appId == bmqp::ProtocolUtil::k_DEFAULT_APP_ID) {
        appKey = mqbu::StorageKey::k_NULL_KEY;
    }
    else {
        bool hasTheStorage = storage->hasVirtualStorage(appId, &appKey);
        BSLS_ASSERT_SAFE(hasTheStorage);
        (void)hasTheStorage;
    }

    const bsls::Types::Int64 numMessages        = storage->numMessages(appKey);
    bslma::ManagedPtr<mqbi::StorageIterator> it = storage->getIterator(appKey);

    if (offset < 0) {
        offset = -offset;
        offset = bsl::max(0LL, numMessages - offset);
    }

    if (count == 0LL) {
        count = numMessages - offset;
    }
    else if (count < 0LL) {
        count  = -count;
        offset = bsl::max(0LL, offset - count);
    }

    queueContents->offset()             = offset;
    queueContents->totalQueueMessages() = numMessages;
    bsls::Types::Int64 i;

    for (i = 0; i < offset && !it->atEnd(); ++i, it->advance()) {
        // NOTHING
    }

    bsls::Types::Int64 listed = 0;

    queueContents->messages().reserve(count);
    for (; listed < count && !it->atEnd(); ++i, ++listed, it->advance()) {
        queueContents->messages().resize(queueContents->messages().size() + 1);
        mqbcmd::Message&            message = queueContents->messages().back();
        BSLA_MAYBE_UNUSED const int rc = listMessage(&message, storage, *it);
        BSLS_ASSERT_SAFE(rc == 0);
    }

    return 0;
}

void StoragePrintUtil::printRecoveredStorages(
    bsl::ostream&            out,
    bslmt::Mutex*            storagesLock,
    const StorageSpMap&      storageMap,
    int                      partitionId,
    const bsl::string&       clusterDescription,
    const bsls::Types::Int64 recoveryStartTime)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(storagesLock);
    BSLS_ASSERT_SAFE(0 <= partitionId);

    // Needed to protect access to `storageMap` and its elements.
    bslmt::LockGuard<bslmt::Mutex> guard(storagesLock);  // LOCK

    out << clusterDescription << ": Partition [" << partitionId
        << "]: Number of recovered storages: " << storageMap.size()
        << ". Time taken for recovery: "
        << bmqu::PrintUtil::prettyTimeInterval(
               (bmqsys::Time::highResolutionTimer() - recoveryStartTime))
        << ". Summary: \n(format: [QueueUri] [QueueKey] "
        << "[Num Msgs] [Num Virtual Storages] "
        << "[Virtual Storages Details])";

    StorageSpMapConstIter cit = storageMap.begin();
    while (cit != storageMap.end()) {
        const mqbs::ReplicatedStorage* rs    = cit->second.get();
        const size_t                   numVS = rs->numVirtualStorages();

        out << "\n  [" << cit->first << "] [" << rs->queueKey() << "] ["
            << rs->numMessages(mqbu::StorageKey::k_NULL_KEY) << "] [" << numVS
            << "]";

        if (numVS) {
            out << " [";
        }

        AppInfos appIdKeyPairs;
        rs->loadVirtualStorageDetails(&appIdKeyPairs);
        BSLS_ASSERT_SAFE(numVS == appIdKeyPairs.size());

        for (AppInfos::const_iterator vit = appIdKeyPairs.cbegin();
             vit != appIdKeyPairs.cend();
             ++vit) {
            BSLS_ASSERT_SAFE(rs->hasVirtualStorage(vit->second));

            out << " {'" << vit->first << "' (" << vit->second
                << "): " << rs->numMessages(vit->second) << "}";
        }

        if (numVS) {
            out << " ]";
        }

        ++cit;
    }
}

bool StoragePrintUtil::printStorageRecoveryCompletion(
    bsl::ostream&      out,
    const FileStores&  fileStores,
    const bsl::string& clusterDescription)
{
    bool success = true;
    out << clusterDescription
        << ": Recovery for all partitions is complete. Summary:";
    for (unsigned int i = 0; i < fileStores.size(); ++i) {
        const bool  isOpen    = fileStores[i]->isOpen();
        const char* isOpenStr = isOpen ? "opened" : "closed";
        out << "\nPartition [" << i << "] status: " << isOpenStr;
        success = success && isOpen;
    }

    return success;
}

}  // close package namespace
}  // close enterprise namespace
