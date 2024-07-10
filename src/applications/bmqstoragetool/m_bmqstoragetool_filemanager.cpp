// Copyright 2014-2023 Bloomberg Finance L.P.
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

#include "m_bmqstoragetool_filemanager.h"

// BMQ
#include <bmqp_crc32c.h>

// MQB
#include <mqbc_clusterstateledgerutil.h>
#include <mqbc_incoreclusterstateledgeriterator.h>
#include <mqbmock_logidgenerator.h>
#include <mqbs_filestoreprotocolutil.h>
#include <mqbsl_ledger.h>
#include <mqbsl_memorymappedondisklog.h>

// MWC
#include <mwcu_memoutstream.h>

// BDE
#include <bdls_filesystemutil.h>
#include <bdls_pathutil.h>

namespace BloombergLP {
namespace m_bmqstoragetool {

namespace {

// Ledger config stubs
int onRolloverCallback(BSLS_ANNOTATION_UNUSED const mqbu::StorageKey& oldLogId,
                       BSLS_ANNOTATION_UNUSED const mqbu::StorageKey& newLogId)
{
    return 0;  // RETURN
}

int cleanupCallback(BSLS_ANNOTATION_UNUSED const bsl::string& logPath)
{
    return 0;  // RETURN
}

void closeLedger(mqbsl::Ledger* ledger)
{
    const int rc = ledger->close();
    BSLS_ASSERT(rc == 0);
    (void)rc;  // Compiler happiness
}

}  // close unnamed namespace

// ==============================
// class FileManager::FileHandler
// ==============================

FileManager::~FileManager()
{
    // NOTHING
}

// =====================
// class FileManagerImpl
// =====================

// CREATORS

FileManagerImpl::FileManagerImpl(const bsl::string& journalFile,
                                 const bsl::string& dataFile,
                                 bslma::Allocator*  allocator)
: d_journalFile(journalFile, allocator)
, d_dataFile(dataFile, allocator)
{
    mwcu::MemOutStream ss(allocator);
    if ((!d_journalFile.path().empty() && !d_journalFile.resetIterator(ss)) ||
        (!d_dataFile.path().empty() && !d_dataFile.resetIterator(ss))) {
        throw bsl::runtime_error(ss.str());  // THROW
    }
}

// MANIPULATORS

mqbs::JournalFileIterator* FileManagerImpl::journalFileIterator()
{
    return d_journalFile.iterator();
}

mqbs::DataFileIterator* FileManagerImpl::dataFileIterator()
{
    return d_dataFile.iterator();
}

// PUBLIC FUNCTIONS

QueueMap FileManagerImpl::buildQueueMap(const bsl::string& cslFile,
                                        bslma::Allocator*  allocator)
{
    bslma::Allocator* alloc = bslma::Default::allocator(allocator);

    using namespace bmqp_ctrlmsg;
    typedef bsl::vector<QueueInfo>     QueueInfos;
    typedef QueueInfos::const_iterator QueueInfosIt;
    if (cslFile.empty()) {
        throw bsl::runtime_error("empty CSL file path");  // THROW
    }
    QueueMap d_queueMap(alloc);

    // Required for ledger operations
    bmqp::Crc32c::initialize();

    // Instantiate ledger config
    mqbsi::LedgerConfig                    ledgerConfig(alloc);
    bsl::shared_ptr<mqbsi::LogIdGenerator> logIdGenerator(
        new (*alloc) mqbmock::LogIdGenerator("bmq_csl_", alloc),
        alloc);
    bsl::shared_ptr<mqbsi::LogFactory> logFactory(
        new (*alloc) mqbsl::MemoryMappedOnDiskLogFactory(alloc),
        alloc);
    bdls::FilesystemUtil::Offset fileSize = bdls::FilesystemUtil::getFileSize(
        cslFile.c_str());
    bsl::string pattern(alloc);
    bsl::string location(alloc);
    BSLS_ASSERT(bdls::PathUtil::getBasename(&pattern, cslFile) == 0);
    BSLS_ASSERT(bdls::PathUtil::getDirname(&location, cslFile) == 0);
    ledgerConfig.setLocation(location)
        .setPattern(pattern)
        .setMaxLogSize(fileSize)
        .setReserveOnDisk(false)
        .setPrefaultPages(false)
        .setLogIdGenerator(logIdGenerator)
        .setLogFactory(logFactory)
        .setExtractLogIdCallback(&mqbc::ClusterStateLedgerUtil::extractLogId)
        .setRolloverCallback(onRolloverCallback)
        .setCleanupCallback(cleanupCallback)
        .setValidateLogCallback(mqbc::ClusterStateLedgerUtil::validateLog);

    // Create and open the ledger
    mqbsl::Ledger ledger(ledgerConfig, alloc);
    BSLS_ASSERT(ledger.open(mqbsi::Ledger::e_READ_ONLY) == 0);
    // Set guard to close the ledger
    bdlb::ScopeExitAny guard(bdlf::BindUtil::bind(closeLedger, &ledger));

    // Iterate through each record in the ledger to find the last snapshot
    // record
    mqbc::IncoreClusterStateLedgerIterator cslIt(&ledger);
    mqbc::IncoreClusterStateLedgerIterator lastSnapshotIt(&ledger);
    while (true) {
        if (cslIt.next() != 0) {
            // End iterator reached or CSL file is corrupted or incomplete
            if (!lastSnapshotIt.isValid()) {
                throw bsl::runtime_error(
                    "No Snapshot found in csl file");  // THROW
            }
            break;  // BREAK
        }

        if (cslIt.header().recordType() ==
            mqbc::ClusterStateRecordType::e_SNAPSHOT) {
            // Save snapshot iterator
            lastSnapshotIt = cslIt;
        }
    }

    // Process last snapshot
    ClusterMessage clusterMessage;
    lastSnapshotIt.loadClusterMessage(&clusterMessage);
    BSLS_ASSERT(clusterMessage.choice().selectionId() ==
                ClusterMessageChoice::SELECTION_ID_LEADER_ADVISORY);

    // Get queue info from snapshot (leaderAdvisory) record
    LeaderAdvisory& leaderAdvisory = clusterMessage.choice().leaderAdvisory();
    QueueInfos&     queuesInfo     = leaderAdvisory.queues();
    {
        // Fill queue map
        QueueInfosIt it = queuesInfo.cbegin();
        for (; it != queuesInfo.cend(); ++it) {
            d_queueMap.insert(*it);
        }
    }

    // Iterate from last snapshot to get updates
    while (true) {
        if (lastSnapshotIt.next() != 0) {
            // End iterator reached or CSL file is corrupted or incomplete
            break;  // BREAK
        }

        if (lastSnapshotIt.header().recordType() ==
            mqbc::ClusterStateRecordType::e_UPDATE) {
            lastSnapshotIt.loadClusterMessage(&clusterMessage);
            // Process queueAssignmentAdvisory record
            if (clusterMessage.choice().selectionId() ==
                ClusterMessageChoice::SELECTION_ID_QUEUE_ASSIGNMENT_ADVISORY) {
                QueueAssignmentAdvisory& queueAdvisory =
                    clusterMessage.choice().queueAssignmentAdvisory();
                const QueueInfos& updateQueuesInfo = queueAdvisory.queues();
                QueueInfosIt      it               = updateQueuesInfo.cbegin();
                for (; it != updateQueuesInfo.cend(); ++it) {
                    d_queueMap.insert(*it);
                }
            }
            else if (clusterMessage.choice().selectionId() ==
                     // Process queueUpdateAdvisory record
                     ClusterMessageChoice::
                         SELECTION_ID_QUEUE_UPDATE_ADVISORY) {
                QueueUpdateAdvisory queueUpdateAdvisory =
                    clusterMessage.choice().queueUpdateAdvisory();
                const bsl::vector<QueueInfoUpdate>& queueInfoUpdates =
                    queueUpdateAdvisory.queueUpdates();
                bsl::vector<QueueInfoUpdate>::const_iterator it =
                    queueInfoUpdates.cbegin();
                for (; it != queueInfoUpdates.cend(); ++it) {
                    d_queueMap.update(*it);
                }
            }
        }
    }

    return d_queueMap;
}

// ==================================
// class FileManagerImpl::FileHandler
// ==================================

template <typename ITER>
bool FileManagerImpl::FileHandler<ITER>::resetIterator(
    bsl::ostream& errorDescription)
{
    // 1) Open
    mwcu::MemOutStream errorDesc;
    int                rc = mqbs::FileSystemUtil::open(
        &d_mfd,
        d_path.c_str(),
        bdls::FilesystemUtil::getFileSize(d_path),
        true,  // read only
        errorDesc);
    if (0 != rc) {
        errorDescription << "Failed to open file [" << d_path << "] rc: " << rc
                         << ", error: " << errorDesc.str() << "\n";
        return false;  // RETURN
    }

    // 2) Basic sanity check
    rc = mqbs::FileStoreProtocolUtil::hasBmqHeader(d_mfd);
    if (0 != rc) {
        errorDescription << "Missing BlazingMQ header from file [" << d_path
                         << "] rc: " << rc << "\n";
        mqbs::FileSystemUtil::close(&d_mfd);
        return false;  // RETURN
    }

    // 3) Load iterator and check
    rc = d_iter.reset(&d_mfd, mqbs::FileStoreProtocolUtil::bmqHeader(d_mfd));
    if (0 != rc) {
        errorDescription << "Failed to create iterator for file [" << d_path
                         << "] rc: " << rc << "\n";
        mqbs::FileSystemUtil::close(&d_mfd);
        return false;  // RETURN
    }

    BSLS_ASSERT_OPT(d_iter.isValid());
    return true;  // RETURN
}

}  // close package namespace
}  // close enterprise namespace
