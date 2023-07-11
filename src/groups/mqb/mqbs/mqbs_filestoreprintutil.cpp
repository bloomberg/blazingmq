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

// mqbs_filestoreprintutil.cpp                                        -*-C++-*-
#include <mqbs_filestoreprintutil.h>

// MQB
#include <mqbcmd_messages.h>
#include <mqbs_fileset.h>

// MWC
#include <mwcu_memoutstream.h>
#include <mwcu_outstreamformatsaver.h>
#include <mwcu_printutil.h>

// BDE
#include <bdlb_print.h>
#include <bdls_pathutil.h>
#include <bsl_string.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace mqbs {

// -------------------------
// struct FileStorePrintUtil
// -------------------------

// CLASS METHODS
void FileStorePrintUtil::loadSummary(mqbcmd::FileStoreSummary*  summary,
                                     const mqbnet::ClusterNode* primaryNode,
                                     unsigned int               primaryLeaseId,
                                     bsls::Types::Uint64        sequenceNum,
                                     size_t             numOutstandingRecords,
                                     size_t             numUnreceiptedMessages,
                                     int                naglePacketCount,
                                     const FileSets&    fileSets,
                                     const StoragesMap& storageMap)
{
    BSLS_ASSERT_SAFE(0 < fileSets.size());

    FileSet* activeFileSet = fileSets[0].get();
    BSLS_ASSERT_SAFE(activeFileSet);

    summary->primaryNodeDescription() = (primaryNode
                                             ? primaryNode->nodeDescription()
                                             : "** NONE **");
    summary->primaryLeaseId()         = primaryLeaseId;
    summary->sequenceNum()            = sequenceNum;
    summary->isAvailable() = !activeFileSet->d_fileSetRolloverPolicyAlarm;
    summary->fileSets().resize(fileSets.size());

    bsls::Types::Uint64 totalMappedSize = 0;
    for (unsigned int i = 0; i < fileSets.size(); ++i) {
        bsl::string leaf;
        int rc = bdls::PathUtil::getLeaf(&leaf, fileSets[i]->d_dataFileName);
        summary->fileSets()[i].dataFileName() =
            (0 == rc ? leaf : fileSets[i]->d_dataFileName);
        summary->fileSets()[i].aliasedBlobBufferCount() =
            fileSets[i]->d_aliasedBlobBufferCount;
        if (0 == i) {
            totalMappedSize += fileSets[i]->d_dataFilePosition;
            totalMappedSize += fileSets[i]->d_qlistFilePosition;
            totalMappedSize += fileSets[i]->d_journalFilePosition;
        }
        else {
            totalMappedSize += fileSets[i]->d_dataFilePosition;
        }
    }

    mqbcmd::ActiveFileSet& activeFileSetResult = summary->activeFileSet();
    activeFileSetResult.dataFile().positionBytes() =
        activeFileSet->d_dataFilePosition;
    activeFileSetResult.dataFile().sizeBytes() =
        activeFileSet->d_dataFile.fileSize();
    activeFileSetResult.dataFile().outstandingBytes() =
        activeFileSet->d_outstandingBytesData;

    activeFileSetResult.journalFile().positionBytes() =
        activeFileSet->d_journalFilePosition;
    activeFileSetResult.journalFile().sizeBytes() =
        activeFileSet->d_journalFile.fileSize();
    activeFileSetResult.journalFile().outstandingBytes() =
        activeFileSet->d_outstandingBytesJournal;

    activeFileSetResult.qlistFile().positionBytes() =
        activeFileSet->d_qlistFilePosition;
    activeFileSetResult.qlistFile().sizeBytes() =
        activeFileSet->d_qlistFile.fileSize();
    activeFileSetResult.qlistFile().outstandingBytes() =
        activeFileSet->d_outstandingBytesQlist;

    summary->totalMappedBytes()       = totalMappedSize;
    summary->numOutstandingRecords()  = numOutstandingRecords;
    summary->numUnreceiptedMessages() = numUnreceiptedMessages;
    summary->naglePacketCount()       = naglePacketCount;

    StorageList storages;
    StorageCollectionUtil::loadStorages(&storages, storageMap);
    StorageCollectionUtil::sortStorages(
        &storages,
        StorageCollectionUtilSortMetric::e_BYTE_COUNT);

    loadQueuesStatus(storages, &summary->storageContent());
}

void FileStorePrintUtil::loadQueueStatus(mqbcmd::StorageQueueInfo* queueInfo,
                                         const ReplicatedStorage*  storage)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(storage);

    using namespace mwcu::PrintUtil;

    mwcu::MemOutStream field;
    field << storage->queueKey();
    queueInfo->queueKey()    = field.str();
    queueInfo->partitionId() = storage->partitionId();
    queueInfo->numMessages() = storage->numMessages(
        mqbu::StorageKey::k_NULL_KEY);
    queueInfo->numBytes() = storage->numBytes(mqbu::StorageKey::k_NULL_KEY);
    field.reset();
    field << storage->queueUri();
    queueInfo->queueUri()     = field.str();
    queueInfo->isPersistent() = storage->isPersistent();
}

void FileStorePrintUtil::loadQueuesStatus(
    const StorageList&      storages,
    mqbcmd::StorageContent* storageContent,
    unsigned int            maxNumQueues)
{
    storageContent->storages().reserve(storages.size());
    for (StorageListConstIter cit = storages.begin(); cit != storages.end();
         ++cit) {
        storageContent->storages().resize(storageContent->storages().size() +
                                          1);
        mqbcmd::StorageQueueInfo& storage = storageContent->storages().back();
        loadQueueStatus(&storage, *cit);

        maxNumQueues--;
        if (maxNumQueues == 0) {
            break;  // BREAK
        }
    }
}

}  // close package namespace
}  // close enterprise namespace
