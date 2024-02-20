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

// ==================================
// class FileManagerReal::FileHandler
// ==================================

namespace BloombergLP {
namespace m_bmqstoragetool {

mqbs::JournalFileIterator* FileManagerReal::journalFileIterator()
{
    return d_journalFile.iterator();
}

mqbs::DataFileIterator* FileManagerReal::dataFileIterator()
{
    return d_dataFile.iterator();
}

// MANIPULATORS
QueueMap FileManagerReal::buildQueueMap(const bsl::string& cslFile,
                                        bslma::Allocator*  allocator)
{
    if (cslFile.empty()) {
        throw bsl::runtime_error("empty CSL file path");
    }
    QueueMap d_queueMap(allocator);

    // Required for ledger operations
    bmqp::Crc32c::initialize();

    // Ledger config stubs
    auto onRolloverCallback = [](const mqbu::StorageKey& oldLogId,
                                 const mqbu::StorageKey& newLogId) {
        return 0;
    };
    auto cleanupCallback = [](const bsl::string& logPath) {
        return 0;
    };

    // Instantiate ledger config
    mqbsi::LedgerConfig                    ledgerConfig(allocator);
    bsl::shared_ptr<mqbsi::LogIdGenerator> logIdGenerator(
        new (*allocator) mqbmock::LogIdGenerator("bmq_csl_", allocator),
        allocator);
    bsl::shared_ptr<mqbsi::LogFactory> logFactory(
        new (*allocator) mqbsl::MemoryMappedOnDiskLogFactory(allocator),
        allocator);
    auto        fileSize = bdls::FilesystemUtil::getFileSize(cslFile.c_str());
    bsl::string pattern(allocator);
    bsl::string location(allocator);
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
    mqbsl::Ledger ledger(ledgerConfig, allocator);
    BSLS_ASSERT(ledger.open(mqbsi::Ledger::e_READ_ONLY) == 0);
    // Set guard to close the ledger
    auto closeLedger = [](mqbsl::Ledger* ledger) {
        BSLS_ASSERT(ledger->close() == 0);
    };
    bdlb::ScopeExitAny guard(bdlf::BindUtil::bind(closeLedger, &ledger));

    // Iterate through each record in the ledger to find the last snapshot
    // record
    mqbc::IncoreClusterStateLedgerIterator cslIt(&ledger);
    mqbc::IncoreClusterStateLedgerIterator lastSnapshotIt(&ledger);
    while (true) {
        int rc = cslIt.next();
        if (rc != 0) {
            // End iterator reached or CSL file is corrupted or incomplete
            if (!lastSnapshotIt.isValid()) {
                throw bsl::runtime_error("No Snapshot found in csl file");
            }
            break;
        }

        if (cslIt.header().recordType() ==
            mqbc::ClusterStateRecordType::Enum::e_SNAPSHOT) {
            // Save snapshot iterator
            lastSnapshotIt = cslIt;
        }
    }

    // Process last snapshot
    bmqp_ctrlmsg::ClusterMessage clusterMessage;
    lastSnapshotIt.loadClusterMessage(&clusterMessage);
    BSLS_ASSERT(
        clusterMessage.choice().selectionId() ==
        bmqp_ctrlmsg::ClusterMessageChoice::SELECTION_ID_LEADER_ADVISORY);

    // Get queue info from snapshot (leaderAdvisory) record
    auto leaderAdvisory = clusterMessage.choice().leaderAdvisory();
    auto queuesInfo     = leaderAdvisory.queues();
    // Fill queue map
    bsl::for_each(queuesInfo.begin(),
                  queuesInfo.end(),
                  [&](const bmqp_ctrlmsg::QueueInfo& queueInfo) {
                      d_queueMap.insert(queueInfo);
                  });

    // Iterate from last snapshot to get updates
    while (true) {
        int rc = lastSnapshotIt.next();
        if (rc != 0) {
            // End iterator reached or CSL file is corrupted or incomplete
            break;
        }

        if (lastSnapshotIt.header().recordType() ==
            mqbc::ClusterStateRecordType::Enum::e_UPDATE) {
            lastSnapshotIt.loadClusterMessage(&clusterMessage);
            // Process queueAssignmentAdvisory record
            if (clusterMessage.choice().selectionId() ==
                bmqp_ctrlmsg::ClusterMessageChoice::
                    SELECTION_ID_QUEUE_ASSIGNMENT_ADVISORY) {
                auto queueAdvisory =
                    clusterMessage.choice().queueAssignmentAdvisory();
                auto updateQueuesInfo = queueAdvisory.queues();
                bsl::for_each(updateQueuesInfo.begin(),
                              updateQueuesInfo.end(),
                              [&](const bmqp_ctrlmsg::QueueInfo& queueInfo) {
                                  d_queueMap.insert(queueInfo);
                              });
            }
            else if (clusterMessage.choice().selectionId() ==
                     // Process queueUpdateAdvisory record
                     bmqp_ctrlmsg::ClusterMessageChoice::
                         SELECTION_ID_QUEUE_UPDATE_ADVISORY) {
                auto queueUpdateAdvisory =
                    clusterMessage.choice().queueUpdateAdvisory();
                auto queueInfoUpdates = queueUpdateAdvisory.queueUpdates();
                bsl::for_each(
                    queueInfoUpdates.begin(),
                    queueInfoUpdates.end(),
                    [&](const bmqp_ctrlmsg::QueueInfoUpdate& queueInfoUpdate) {
                        d_queueMap.update(queueInfoUpdate);
                    });
            }
        }
    }

    return d_queueMap;
}

FileManagerReal::FileManagerReal(const bsl::string& journalFile,
                                 const bsl::string& dataFile,
                                 bslma::Allocator*  allocator)
: d_journalFile(journalFile, allocator)
, d_dataFile(dataFile, allocator)
{
    mwcu::MemOutStream ss(allocator);
    if ((!d_journalFile.path().empty() && !d_journalFile.resetIterator(ss)) ||
        (!d_dataFile.path().empty() && !d_dataFile.resetIterator(ss))) {
        throw bsl::runtime_error(ss.str());
    }
}

template <typename ITER>
bool FileManagerReal::FileHandler<ITER>::resetIterator(
    std::ostream& errorDescription)
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
