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

// BMQ
#include <bmqu_memoutstream.h>

// BDE
#include <bdls_filesystemutil.h>
#include <bdls_pathutil.h>
#include <bsl_memory.h>
#include <bsl_ostream.h>
#include <bsl_stdexcept.h>
#include <bsl_vector.h>
#include <bsla_annotations.h>

namespace BloombergLP {
namespace m_bmqstoragetool {

namespace {

// Ledger config stubs
int onRolloverCallback(BSLA_UNUSED const mqbu::StorageKey& oldLogId,
                       BSLA_UNUSED const mqbu::StorageKey& newLogId)
{
    return 0;  // RETURN
}

int cleanupCallback(BSLA_UNUSED const bsl::string& logPath)
{
    return 0;  // RETURN
}

// Iterate through each record in the cluster state ledger to find the last
// snapshot record and set it to the specified `lastSnapshotIt_p`. Return false
// if snapshot record is not found, true otherwise.
bool findLastSnapshot(bsl::ostream&                           errorDescription,
                      mqbc::IncoreClusterStateLedgerIterator* lastSnapshotIt_p,
                      mqbsl::Ledger*                          ledger_p)
{
    mqbc::IncoreClusterStateLedgerIterator cslIt(ledger_p);

    while (true) {
        const int rc = cslIt.next();
        if (rc == 1 || rc < 0) {
            // End iterator reached or error occured.
            if (rc < 0) {
                errorDescription
                    << "CSL file either corrupted or incomplete at offset="
                    << cslIt.currRecordId().offset() << ". rc=" << rc << '\n';
            }
            if (!lastSnapshotIt_p->isValid()) {
                errorDescription << "No Snapshot record found in csl file\n";
                return false;  // RETURN
            }
            break;  // BREAK
        }

        if (cslIt.header().recordType() ==
            mqbc::ClusterStateRecordType::e_SNAPSHOT) {
            // Save snapshot iterator
            lastSnapshotIt_p->copy(cslIt);
        }
    }

    return true;
}

}  // close unnamed namespace

// =================
// class FileManager
// =================

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
                                 const bsl::string& cslFile,
                                 bool               cslFromBegin,
                                 bslma::Allocator*  allocator)
: d_journalFile(journalFile, allocator)
, d_dataFile(dataFile, allocator)
, d_cslFile(cslFile, cslFromBegin, allocator)
{
    bmqu::MemOutStream ss(allocator);
    if ((!d_journalFile.path().empty() && !d_journalFile.resetIterator(ss)) ||
        (!d_dataFile.path().empty() && !d_dataFile.resetIterator(ss)) ||
        (!d_cslFile.path().empty() && !d_cslFile.resetIterator(ss))) {
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

mqbc::IncoreClusterStateLedgerIterator* FileManagerImpl::cslFileIterator()
{
    return d_cslFile.iterator();
}

// ACCESSORS

void FileManagerImpl::fillQueueMapFromCslFile(QueueMap* queueMap_p) const
{
    return d_cslFile.fillQueueMap(queueMap_p);
}

// ==================================
// class FileManagerImpl::FileHandler
// ==================================

template <typename ITER>
bool FileManagerImpl::FileHandler<ITER>::resetIterator(
    bsl::ostream& errorDescription)
{
    // 1) Open
    bmqu::MemOutStream errorDesc;
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
    return true;
}

// =====================================
// class FileManagerImpl::CslFileHandler
// =====================================

FileManagerImpl::CslFileHandler::CslFileHandler(const bsl::string& path,
                                                bool              cslFromBegin,
                                                bslma::Allocator* allocator)
: d_path(path)
, d_ledger_p()
, d_cslFromBegin(cslFromBegin)
, d_allocator(allocator)
{
    // Required for ledger operations
    bmqp::Crc32c::initialize();
}

FileManagerImpl::CslFileHandler::~CslFileHandler()
{
    if (d_ledger_p) {
        BSLA_MAYBE_UNUSED const int rc = d_ledger_p->close();
        BSLS_ASSERT(rc == 0);
    }
}

bool FileManagerImpl::CslFileHandler::resetIterator(
    bsl::ostream& errorDescription)
{
    BSLS_ASSERT(!d_path.empty());

    // Create ledger config
    mqbsi::LedgerConfig                    ledgerConfig(d_allocator);
    bsl::shared_ptr<mqbsi::LogIdGenerator> logIdGenerator(
        new (*d_allocator) mqbmock::LogIdGenerator("bmq_csl_", d_allocator),
        d_allocator);
    bsl::shared_ptr<mqbsi::LogFactory> logFactory(
        new (*d_allocator) mqbsl::MemoryMappedOnDiskLogFactory(d_allocator),
        d_allocator);
    bdls::FilesystemUtil::Offset fileSize = bdls::FilesystemUtil::getFileSize(
        d_path.c_str());

    bsl::string pattern(d_allocator);
    bsl::string location(d_allocator);
    int         rc = bdls::PathUtil::getBasename(&pattern, d_path);
    if (rc != 0) {
        errorDescription << "bdls::PathUtil::getBasename() failed with error: "
                         << rc << '\n';
        return false;  // RETURN
    }
    rc = bdls::PathUtil::getDirname(&location, d_path);
    if (rc != 0) {
        errorDescription << "bdls::PathUtil::getDirname() failed with error: "
                         << rc << '\n';
        return false;  // RETURN
    }

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
    d_ledger_p.load(new (*d_allocator)
                        mqbsl::Ledger(ledgerConfig, d_allocator),
                    d_allocator);
    rc = d_ledger_p->open(mqbsi::Ledger::e_READ_ONLY);
    if (rc != 0) {
        errorDescription << "Open ledger failed with error: " << rc << '\n';
        return false;  // RETURN
    }

    d_iter_p.load(new (*d_allocator)
                      mqbc::IncoreClusterStateLedgerIterator(d_ledger_p.get()),
                  d_allocator);

    if (d_cslFromBegin) {
        // Move iterator to the first record.
        rc = d_iter_p->next();
        if (rc != 0) {
            errorDescription << "CSL file either empty or corrupted: rc="
                             << rc;
            return false;  // RETURN
        }
    }
    else {
        // Move iterator to the last snapshot.
        mqbc::IncoreClusterStateLedgerIterator lastSnapshotIt(
            d_ledger_p.get());
        if (!findLastSnapshot(errorDescription,
                              &lastSnapshotIt,
                              d_ledger_p.get())) {
            return false;  // RETURN
        }
        d_iter_p->copy(lastSnapshotIt);
    }

    return true;
}

mqbc::IncoreClusterStateLedgerIterator*
FileManagerImpl::CslFileHandler::iterator()
{
    return d_iter_p.get();
}

const bsl::string& FileManagerImpl::CslFileHandler::path() const
{
    return d_path;
}

void FileManagerImpl::CslFileHandler::fillQueueMap(QueueMap* queueMap_p) const
{
    if (!d_ledger_p) {
        // Return if CSL file is not present.
        return;  // RETURN
    }

    mqbc::IncoreClusterStateLedgerIterator lastSnapshotIt(d_ledger_p.get());
    if (d_cslFromBegin) {
        // Move iterator to the last snapshot.
        bmqu::MemOutStream errorDescr(d_allocator);
        if (!findLastSnapshot(errorDescr, &lastSnapshotIt, d_ledger_p.get())) {
            throw bsl::runtime_error(errorDescr.str());  // THROW
        }
    }
    else {
        lastSnapshotIt.copy(*d_iter_p);
    }

    using namespace bmqp_ctrlmsg;
    typedef bsl::vector<QueueInfo>     QueueInfos;
    typedef QueueInfos::const_iterator QueueInfosIt;

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
            queueMap_p->insert(*it);
        }
    }

    // Iterate from last snapshot to get updates
    while (true) {
        const int rc = lastSnapshotIt.next();
        if (rc == 1 || rc < 0) {
            // End iterator reached or error occured.
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
                    queueMap_p->insert(*it);
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
                    queueMap_p->update(*it);
                }
            }
        }
    }
}

}  // close package namespace
}  // close enterprise namespace
