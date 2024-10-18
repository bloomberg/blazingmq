// Copyright 2015-2023 Bloomberg Finance L.P.
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

// mqbs_filestore.cpp                                                 -*-C++-*-
#include <mqbs_filestore.h>

#include <mqbscm_version.h>
// MQB
#include <mqbcmd_humanprinter.h>
#include <mqbcmd_messages.h>
#include <mqbi_domain.h>
#include <mqbi_queue.h>
#include <mqbi_queueengine.h>
#include <mqbs_datafileiterator.h>
#include <mqbs_filebackedstorage.h>
#include <mqbs_filestoreprintutil.h>
#include <mqbs_filestoreprotocolutil.h>
#include <mqbs_filestoreset.h>
#include <mqbs_filestoreutil.h>
#include <mqbs_filesystemutil.h>
#include <mqbs_inmemorystorage.h>
#include <mqbs_journalfileiterator.h>
#include <mqbs_memoryblock.h>
#include <mqbs_offsetptr.h>
#include <mqbs_qlistfileiterator.h>
#include <mqbs_replicatedstorage.h>
#include <mqbs_storageutil.h>
#include <mqbstat_clusterstats.h>
#include <mqbu_exit.h>

// BMQ
#include <bmqp_crc32c.h>
#include <bmqp_event.h>
#include <bmqp_protocol.h>
#include <bmqp_protocolutil.h>
#include <bmqp_storagemessageiterator.h>
#include <bmqt_resultcode.h>

// MWC
#include <mwcsys_statmonitorsnapshotrecorder.h>
#include <mwcsys_time.h>
#include <mwctsk_alarmlog.h>
#include <mwcu_blobobjectproxy.h>
#include <mwcu_memoutstream.h>
#include <mwcu_outstreamformatsaver.h>
#include <mwcu_printutil.h>

// BDE
#include <bdlb_bigendian.h>
#include <bdlb_print.h>
#include <bdlb_string.h>
#include <bdlbb_blobutil.h>
#include <bdlf_bind.h>
#include <bdlf_placeholder.h>
#include <bdlma_localsequentialallocator.h>
#include <bdls_filesystemutil.h>
#include <bdlt_currenttime.h>
#include <bdlt_datetime.h>
#include <bdlt_epochutil.h>
#include <bdlt_timeunitratio.h>
#include <bsl_algorithm.h>
#include <bsl_c_errno.h>
#include <bsl_cstddef.h>
#include <bsl_cstring.h>
#include <bsl_iomanip.h>
#include <bsl_iostream.h>
#include <bsl_map.h>
#include <bsl_unordered_set.h>
#include <bsl_utility.h>
#include <bslim_printer.h>
#include <bsls_annotation.h>
#include <bsls_timeinterval.h>

// SYS
#include <unistd.h>

namespace BloombergLP {
namespace mqbs {

namespace {

typedef bsl::pair<unsigned int, bsls::Types::Uint64> MessageByteCounter;

/// Soft limit, or threshold, percentage of space associated with
/// outstanding data in a partition, used as a threshold to generate an
/// alarm
const bsls::Types::Uint64 k_SPACE_USED_PERCENT_SOFT = 60;

/// Interval, in seconds, to perform a check of available space in the
/// partition.
const double k_PARTITION_AVAILABLESPACE_SECS = 20;

const int k_NAGLE_PACKET_COUNT = 100;

const int k_KEY_LEN = FileStoreProtocol::k_KEY_LENGTH;

const unsigned int k_REQUESTED_JOURNAL_SPACE =
    3 * FileStoreProtocol::k_JOURNAL_RECORD_SIZE;
// Above, 3 == 1 journal record being written +
//             1 journal sync point if rolling over +
//             1 journal sync point if self needs to issue another sync point
//             in 'setActivePrimary' with old values

/// Return a rounded (down) percentage value (range [0-100]) representing
/// the space in use on a file with the specified `capacity`, currently
/// having the specified `inUse` bytes used.
bsls::Types::Uint64 computePercentage(bsls::Types::Uint64 inUse,
                                      bsls::Types::Uint64 capacity)
{
    bsls::Types::Uint64 percent = 0;
    if (inUse <= capacity) {
        percent = (inUse * 100) / capacity;
    }

    return percent;
}

/// Print to the specified `out` a capture of used space in the partition
/// file represented by the specified `prefix` and having the specified
/// `inUse`, `capacity`, and `inUsePercent` metrics.
void printSpaceInUse(bsl::ostream&            out,
                     const bslstl::StringRef& prefix,
                     bsls::Types::Uint64      inUse,
                     bsls::Types::Uint64      capacity,
                     bsls::Types::Uint64      inUsePercent)
{
    out << prefix << ": "
        << mwcu::PrintUtil::prettyNumber(
               static_cast<bsls::Types::Int64>(inUse))
        << " / "
        << mwcu::PrintUtil::prettyNumber(
               static_cast<bsls::Types::Int64>(capacity))
        << ", [" << inUsePercent << "%]";
}

/// Print the specified top `numQueues` contributing queues to the specified
/// `event`, triggered by the specified `fileType` of the partition having
/// the specified `storageMap`, to the specified `os`.  Optionally specify
/// an initial indentation `level`, whose absolute value is incremented
/// recursively for nested objects. If `level` is specified, optionally
/// specify `spacesPerLevel`, whose absolute value indicates the number of
/// spaces per indentation level for this and all of its nested objects. If
/// `level` is negative, suppress indentation of the first line. If
/// `spacesPerLevel` is negative, format the entire output on one line,
/// suppressing all but the initial indentation (as governed by `level`).
void printTopContributingQueues(
    bsl::ostream&                             os,
    unsigned int                              numQueues,
    const bslstl::StringRef&                  event,
    FileType::Enum                            fileType,
    const StorageCollectionUtil::StoragesMap& storageMap,
    int                                       level          = 0,
    int                                       spacesPerLevel = 4)
{
    // No top contributing queues for QList files
    if (FileType::e_QLIST == fileType) {
        return;  // RETURN
    }

    bdlb::Print::newlineAndIndent(os, level, spacesPerLevel);
    os << "Top " << numQueues << " queues contributing to the " << event
       << ": ";

    StorageCollectionUtil::StorageList storages;
    StorageCollectionUtil::loadStorages(&storages, storageMap);
    if (FileType::e_JOURNAL == fileType) {
        // For journal files, these will be the queues with the top 'numQueues'
        // highest number of messages. Queues with 0 messages will be ignored.
        StorageCollectionUtil::filterStorages(
            &storages,
            StorageCollectionUtilFilterFactory::byMessageCount(1));
        StorageCollectionUtil::sortStorages(
            &storages,
            StorageCollectionUtilSortMetric::e_MESSAGE_COUNT);
    }
    else if (FileType::e_DATA == fileType) {
        // For data files, these will be the queues with the top 'numQueues'
        // highest number of bytes. Queues with 0 bytes will be ignored.
        StorageCollectionUtil::filterStorages(
            &storages,
            StorageCollectionUtilFilterFactory::byByteCount(1));
        StorageCollectionUtil::sortStorages(
            &storages,
            StorageCollectionUtilSortMetric::e_BYTE_COUNT);
    }

    mqbcmd::Result result;
    FileStorePrintUtil::loadQueuesStatus(storages,
                                         &result.makeStorageContent(),
                                         numQueues);
    mqbcmd::HumanPrinter::print(os, result, level + 1, spacesPerLevel);

    os << "\n";
}

bool compareByByte(const bsl::pair<mqbu::StorageKey, MessageByteCounter>& lhs,
                   const bsl::pair<mqbu::StorageKey, MessageByteCounter>& rhs)
{
    return lhs.second.second > rhs.second.second;
}

}  // close unnamed namespace

// -------------------------------------
// struct FileStore_AliasedBufferDeleter
// -------------------------------------

// CREATORS
FileStore_AliasedBufferDeleter::FileStore_AliasedBufferDeleter()
: d_fileSet_p(0)
{
    // NOTHING
}

// MANIPULATORS
void FileStore_AliasedBufferDeleter::setFileSet(FileSet* fileSet)
{
    d_fileSet_p = fileSet;
    ++d_fileSet_p->d_aliasedBlobBufferCount;
}

void FileStore_AliasedBufferDeleter::reset()
{
    // executed by *ANY* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_fileSet_p);
    BSLS_ASSERT_SAFE(0 < d_fileSet_p->d_aliasedBlobBufferCount);

    if (0 == --(d_fileSet_p->d_aliasedBlobBufferCount)) {
        d_fileSet_p->d_store_p->gc(d_fileSet_p);
    }

    d_fileSet_p = 0;
}

// ---------------
// class FileStore
// ---------------

// PRIVATE MANIPULATORS

void FileStore::cancelUnreceipted(const DataStoreRecordKey& recordKey)
{
    Unreceipted::const_iterator it = d_unreceipted.find(recordKey);
    // end of of Receipt range

    if (it == d_unreceipted.end()) {
        // ignore
        return;  // RETURN
    }
    StorageMapIter sit = d_storages.find(it->second.d_queueKey);
    if (sit != d_storages.end()) {
        BSLS_ASSERT_SAFE(sit->second->queue());

        sit->second->queue()->onRemoval(it->second.d_guid,
                                        it->second.d_qH,
                                        bmqt::AckResult::e_UNKNOWN);
    }
    // else the queue and its storage are gone; ignore the receipt
    d_unreceipted.erase(it);
}

int FileStore::openInNonRecoveryMode()
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(inDispatcherThread());

    FileSetSp fileSetSp;
    int       rc = create(&fileSetSp);
    if (0 == rc) {
        d_fileSets.insert(d_fileSets.begin(), fileSetSp);
    }

    // If error, already logged by 'create'
    return rc;
}

int FileStore::openInRecoveryMode(bsl::ostream&          errorDescription,
                                  const QueueKeyInfoMap& queueKeyInfoMap)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(inDispatcherThread());

    enum {
        rc_NO_FILES_TO_RECOVER = 1  // Reserved rc
        ,
        rc_SUCCESS                             = 0,
        rc_RECOVERY_FILE_SET_RETRIEVAL_FAILURE = -1,
        rc_FILE_ITERATOR_FAILURE               = -2,
        rc_FILE_TRUNCATION_FAILURE             = -3,
        rc_FILE_ITERATOR_RELOAD_FAILURE        = -4,
        rc_RECOVERY_FAILURE                    = -5,
        rc_INVALID_SYNC_PT                     = -6,
        rc_CONFIGURATION_ERROR                 = -7,
        rc_PARTITION_FULL                      = -8,
        rc_OPEN_FAILURE                        = -9,
        rc_SYNC_POINT_FAILURE                  = -10
    };

    const bool needQList = !d_isFSMWorkflow;

    MappedFileDescriptor journalFd;
    MappedFileDescriptor dataFd;
    MappedFileDescriptor qlistFd;
    FileStoreSet         recoveryFileSet;

    // Max number of file sets that we want to be inspected.
    //
    // TBD: add reason for '2'.

    const int k_MAX_NUM_FILE_SETS_TO_CHECK = 2;

    bsls::Types::Uint64 journalFilePos;
    bsls::Types::Uint64 dataFilePos;

    int rc = FileStoreUtil::openRecoveryFileSet(errorDescription,
                                                &journalFd,
                                                &dataFd,
                                                &recoveryFileSet,
                                                &journalFilePos,
                                                &dataFilePos,
                                                d_config.partitionId(),
                                                k_MAX_NUM_FILE_SETS_TO_CHECK,
                                                d_config,
                                                true,  // readOnly
                                                d_isFSMWorkflow,
                                                needQList ? &qlistFd : 0);

    if (1 == rc) {
        // Special 'rc' implying no file sets present.
        return rc_NO_FILES_TO_RECOVER;  // RETURN
    }

    if (0 != rc) {
        return 100 * rc + rc_RECOVERY_FILE_SET_RETRIEVAL_FAILURE;  // RETURN
    }

    BALL_LOG_INFO << partitionDesc()
                  << "File set opened for recovery: " << recoveryFileSet;

    // Open iterators on three files.

    JournalFileIterator jit;
    QlistFileIterator   qit;
    DataFileIterator    dit;
    rc = FileStoreUtil::loadIterators(errorDescription,
                                      &jit,
                                      &dit,
                                      &qit,
                                      journalFd,
                                      dataFd,
                                      qlistFd,
                                      recoveryFileSet,
                                      needQList);
    if (0 != rc) {
        FileSystemUtil::close(&journalFd);
        FileSystemUtil::close(&dataFd);
        if (needQList) {
            FileSystemUtil::close(&qlistFd);
        }

        return 100 * rc + rc_FILE_ITERATOR_FAILURE;  // RETURN
    }

    // Print last sync point in the journal, if available.

    if (0 != jit.lastSyncPointPosition()) {
        const JournalOpRecord& lsp = jit.lastSyncPoint();
        BSLS_ASSERT_SAFE(JournalOpType::e_SYNCPOINT == lsp.type());

        mwcu::MemOutStream out;
        out << partitionDesc() << " Last sync point details: "
            << "SyncPoint sub-type: " << lsp.syncPointType()
            << ", PrimaryNodeId: " << lsp.primaryNodeId()
            << ", PrimaryLeaseId (in SyncPt): " << lsp.primaryLeaseId()
            << ", SequenceNumber (in SyncPt): " << lsp.sequenceNum()
            << ", PrimaryLeaseId (in RecordHeader): "
            << lsp.header().primaryLeaseId()
            << ", SequenceNumber (in RecordHeader): "
            << lsp.header().sequenceNumber()
            << ", SyncPoint offset in journal: " << jit.lastSyncPointPosition()
            << ", DataFileOffset: "
            << (static_cast<bsls::Types::Uint64>(lsp.dataFileOffsetDwords()) *
                bmqp::Protocol::k_DWORD_SIZE);
        if (!d_isFSMWorkflow) {
            out << ", QlistFileOffset: "
                << (static_cast<bsls::Types::Uint64>(
                        lsp.qlistFileOffsetWords()) *
                    bmqp::Protocol::k_WORD_SIZE);
        }
        out << ", Timestamp (epoch): " << lsp.header().timestamp()
            << ", Last sync point is the last record: " << bsl::boolalpha
            << (jit.lastSyncPointPosition() == jit.lastRecordPosition());

        if (jit.lastSyncPointPosition() != jit.lastRecordPosition()) {
            OffsetPtr<const RecordHeader> lastRecHeader(
                jit.mappedFileDescriptor()->block(),
                jit.lastRecordPosition());
            BSLS_ASSERT_SAFE(RecordType::e_UNDEFINED != lastRecHeader->type());
            out << ", last record type: " << lastRecHeader->type();
        }

        bdlt::Datetime datetime;
        rc = bdlt::EpochUtil::convertFromTimeT64(&datetime,
                                                 lsp.header().timestamp());
        if (0 == rc) {
            out << ", Timestamp (datetime): " << datetime;
        }
        BALL_LOG_INFO << out.str();
    }

    bool                appendSyncPoint    = false;
    unsigned int        primaryLeaseIdCurr = d_primaryLeaseId;
    bsls::Types::Uint64 sequenceNumcurr    = d_sequenceNum;

    if (d_isFSMWorkflow) {
        // In FSM workflow, we always point to last record regardless of sync
        // points.

        if (0 == jit.firstRecordPosition()) {
            // Looks like the journal has only BlazingMQ file header and
            // JournalFile header.  If other records exist, they are corrupt.

            BSLS_ASSERT_SAFE(0 == jit.lastRecordPosition());
            BSLS_ASSERT_SAFE(0 == jit.lastSyncPointPosition());

            d_sequenceNum    = 0;
            d_primaryLeaseId = 0;
        }
        else {
            BSLS_ASSERT_SAFE(0 != jit.lastRecordPosition());

            const RecordHeader& recHeader = jit.lastRecordHeader();
            d_primaryLeaseId              = recHeader.primaryLeaseId();
            d_sequenceNum                 = recHeader.sequenceNumber();
        }
    }
    else {
        if (1 < clusterSize()) {
            // There are 3 scenarios as far as last SyncPt in the JOURNAL is
            // concerned in a multi-node cluster:
            // 1) Last SyncPt *is* the last record in the JOURNAL.  All good in
            //    this case; simply retrieve primaryLeaseId and sequenceNum
            //    from the SyncPt.
            // 2) Last SyncPt is not the last record.  Truncate the JOURNAL to
            //    the last SyncPt, and also truncate the QLIST and DATA files
            //    to the offsets which appear in the last SyncPt.
            // 3) No SyncPt exists in the JOURNAL.  Truncate all 3 files to
            //    just contain their respective headers.

            // (2) and (3) scenarios can occur even after this node has
            // performed recovery, in the case when there was no primary and
            // this node synced up with one of the AVAILABLE nodes, which just
            // sent whatever it had.  Since a primary will eventually come up,
            // this partition at this node will eventually get synced up.

            // Lastly, we truncate the files in (2) and (3) to the last SyncPt
            // so that its easy to initialize primaryLeaseId and seqNum from
            // the last SyncPt.  If we don't do that, we'd have to calculate
            // their values.

            bool                needTruncation = false;
            bsls::Types::Uint64 journalOffset  = 0;
            bsls::Types::Uint64 qlistOffset    = 0;
            bsls::Types::Uint64 dataOffset     = 0;

            if (0 == jit.lastSyncPointPosition()) {
                // Scenario (3) from above.

                needTruncation   = true;
                d_primaryLeaseId = 0;
                d_sequenceNum    = 0;

                journalOffset = (FileStoreProtocolUtil::bmqHeader(journalFd)
                                     .headerWords() +
                                 jit.header().headerWords()) *
                                bmqp::Protocol::k_WORD_SIZE;

                if (needQList) {
                    qlistOffset = (FileStoreProtocolUtil::bmqHeader(qlistFd)
                                       .headerWords() +
                                   qit.header().headerWords()) *
                                  bmqp::Protocol::k_WORD_SIZE;
                }

                dataOffset =
                    (FileStoreProtocolUtil::bmqHeader(dataFd).headerWords() +
                     dit.header().headerWords()) *
                    bmqp::Protocol::k_WORD_SIZE;

                BALL_LOG_WARN << partitionDesc()
                              << "No sync point found in journal, in a multi "
                              << "node cluster.  Files will be truncated upto "
                              << "their respective headers.";
            }
            else if (jit.lastSyncPointPosition() != jit.lastRecordPosition()) {
                // Scenario (2) from above.

                needTruncation = true;

                const JournalOpRecord& lsp = jit.lastSyncPoint();

                // (LeaseId, SeqNum) must be extracted from last SyncPt's
                // record header, not from the SyncPt itself.  The two sequence
                // numbers can differ in case last SyncPt was issued by new
                // primary on behalf of the old one, and we always use the
                // (leaseId, seqNum) present in the RecordHeader.
                d_primaryLeaseId = lsp.header().primaryLeaseId();
                d_sequenceNum    = lsp.header().sequenceNumber();

                journalOffset = jit.lastSyncPointPosition() +
                                FileStoreProtocol::k_JOURNAL_RECORD_SIZE;

                if (needQList) {
                    qlistOffset = static_cast<bsls::Types::Uint64>(
                                      lsp.qlistFileOffsetWords()) *
                                  bmqp::Protocol::k_WORD_SIZE;
                }

                dataOffset = static_cast<bsls::Types::Uint64>(
                                 lsp.dataFileOffsetDwords()) *
                             bmqp::Protocol::k_DWORD_SIZE;

                BALL_LOG_WARN
                    << partitionDesc()
                    << "JOURNAL's last record is not SyncPt record,"
                    << " in a multi-node cluster.  Last record "
                    << "offset: " << jit.lastRecordPosition()
                    << ", SyncPt offset: " << jit.lastSyncPointPosition()
                    << ".";
            }
            else {
                // Scenario (1) from above.  Initialize primary leaseId and
                // sequence numbers from last SyncPt's record header.

                needTruncation             = false;
                const JournalOpRecord& lsp = jit.lastSyncPoint();
                d_primaryLeaseId           = lsp.header().primaryLeaseId();
                d_sequenceNum              = lsp.header().sequenceNumber();
            }

            if (needTruncation) {
                BALL_LOG_WARN << partitionDesc()
                              << "Truncating partition JOURNAL, QLIST and DATA"
                              << " files at: " << journalOffset << ", "
                              << qlistOffset << " and " << dataOffset
                              << " offsets respectively.";

                // Since the 3 files have been opened in read mode, we can't
                // use FileStoreUtil::truncate, which uses ftruncate() syscall
                // under the hood, because ftruncate() fails with EINVAL if
                // file is opened in read mode.  So, we just call truncate()
                // syscall, which works in all cases.
                rc = ::truncate(recoveryFileSet.journalFile().c_str(),
                                journalOffset);
                if (0 != rc) {
                    BALL_LOG_ERROR << partitionDesc()
                                   << "Failed to truncate JOURNAL file at "
                                   << "offset " << journalOffset
                                   << ", rc: " << rc << ", errno: [" << errno
                                   << " [" << bsl::strerror(errno) << "].";
                    FileSystemUtil::close(&journalFd);
                    if (needQList) {
                        FileSystemUtil::close(&qlistFd);
                    }
                    FileSystemUtil::close(&dataFd);

                    return rc_FILE_TRUNCATION_FAILURE;  // RETURN
                }

                if (needQList) {
                    rc = ::truncate(recoveryFileSet.qlistFile().c_str(),
                                    qlistOffset);
                    if (0 != rc) {
                        BALL_LOG_ERROR << partitionDesc()
                                       << "Failed to truncate QLIST file "
                                       << "at offset " << qlistOffset
                                       << ", rc: " << rc << ", errno: ["
                                       << errno << " [" << bsl::strerror(errno)
                                       << "].";
                        FileSystemUtil::close(&journalFd);
                        FileSystemUtil::close(&qlistFd);
                        FileSystemUtil::close(&dataFd);

                        return rc_FILE_TRUNCATION_FAILURE;  // RETURN
                    }
                }

                rc = ::truncate(recoveryFileSet.dataFile().c_str(),
                                dataOffset);
                if (0 != rc) {
                    BALL_LOG_ERROR << partitionDesc()
                                   << "Failed to truncate DATA file at offset "
                                   << dataOffset << ", rc: " << rc
                                   << ", errno: [" << errno << " ["
                                   << bsl::strerror(errno) << "].";
                    FileSystemUtil::close(&journalFd);
                    if (needQList) {
                        FileSystemUtil::close(&qlistFd);
                    }
                    FileSystemUtil::close(&dataFd);

                    return rc_FILE_TRUNCATION_FAILURE;  // RETURN
                }

                // Files have been truncated.  We need to update relevant field
                // in 'journalFd', 'qlistFd' and 'dataFd' to the new file size,
                // and also need to load the iterators again as they may now be
                // pointing to invalid file blocks.
                BALL_LOG_INFO << partitionDesc()
                              << "Adjusting file sizes and reloading iterators"
                              << " due to above truncation.";

                journalFd.setFileSize(journalOffset);
                if (needQList) {
                    qlistFd.setFileSize(qlistOffset);
                }
                dataFd.setFileSize(dataOffset);

                jit.clear();
                qit.clear();
                dit.clear();

                rc = FileStoreUtil::loadIterators(errorDescription,
                                                  &jit,
                                                  &dit,
                                                  &qit,
                                                  journalFd,
                                                  dataFd,
                                                  qlistFd,
                                                  recoveryFileSet,
                                                  needQList);
                if (0 != rc) {
                    FileSystemUtil::close(&journalFd);
                    if (needQList) {
                        FileSystemUtil::close(&qlistFd);
                    }
                    FileSystemUtil::close(&dataFd);

                    return 100 * rc +
                           rc_FILE_ITERATOR_RELOAD_FAILURE;  // RETURN
                }
            }
        }
        else {
            // Single node cluster.  Note that journal can end with a record of
            // type different than sync point.

            if (0 == jit.firstRecordPosition()) {
                // Looks like the journal has only BlazingMQ file header and
                // JournalFile header.  If other records exist, they are
                // corrupt.

                BSLS_ASSERT_SAFE(0 == jit.lastRecordPosition());
                BSLS_ASSERT_SAFE(0 == jit.lastSyncPointPosition());

                d_sequenceNum    = 0;
                d_primaryLeaseId = 0;
            }
            else if (jit.lastRecordPosition() == jit.lastSyncPointPosition()) {
                // Last record is a sync point.  Extract leaseId and seqNum
                // from it's header.

                const JournalOpRecord& lsp = jit.lastSyncPoint();
                d_primaryLeaseId           = lsp.header().primaryLeaseId();
                d_sequenceNum              = lsp.header().sequenceNumber();
            }
            else {
                // Last record is not a sync point.  This is ok for a single
                // node cluster; we will explicitly append a SyncPt.  Extract
                // leaseId and seqNum from last record's header.

                BSLS_ASSERT_SAFE(0 != jit.lastRecordPosition());

                appendSyncPoint               = true;
                const RecordHeader& recHeader = jit.lastRecordHeader();
                d_primaryLeaseId              = recHeader.primaryLeaseId();
                d_sequenceNum                 = recHeader.sequenceNumber();
            }
        }
    }

    BALL_LOG_INFO << partitionDesc() << "Retrieved primaryLeaseId and sequence"
                  << " number: (" << d_primaryLeaseId << ", " << d_sequenceNum
                  << ").";

    // Create file set.
    FileSetSp fileSetSp;
    fileSetSp.createInplace(d_allocator_p, this, d_allocator_p);
    d_fileSets.insert(d_fileSets.begin(), fileSetSp);

    fileSetSp->d_dataFile         = dataFd;
    fileSetSp->d_dataFileName     = recoveryFileSet.dataFile();
    fileSetSp->d_dataFilePosition = bdls::FilesystemUtil::getFileSize(
        recoveryFileSet.dataFile());
    fileSetSp->d_journalFile         = journalFd;
    fileSetSp->d_journalFileName     = recoveryFileSet.journalFile();
    fileSetSp->d_journalFilePosition = bdls::FilesystemUtil::getFileSize(
        recoveryFileSet.journalFile());

    if (needQList) {
        fileSetSp->d_qlistFile         = qlistFd;
        fileSetSp->d_qlistFileName     = recoveryFileSet.qlistFile();
        fileSetSp->d_qlistFilePosition = bdls::FilesystemUtil::getFileSize(
            recoveryFileSet.qlistFile());
    }

    fileSetSp->d_outstandingBytesJournal +=
        FileStoreProtocolUtil::bmqHeader(journalFd).headerWords() *
        bmqp::Protocol::k_WORD_SIZE;
    fileSetSp->d_outstandingBytesJournal += jit.header().headerWords() *
                                            bmqp::Protocol::k_WORD_SIZE;

    fileSetSp->d_outstandingBytesData +=
        FileStoreProtocolUtil::bmqHeader(dataFd).headerWords() *
        bmqp::Protocol::k_WORD_SIZE;
    fileSetSp->d_outstandingBytesData += dit.header().headerWords() *
                                         bmqp::Protocol::k_WORD_SIZE;

    if (needQList) {
        fileSetSp->d_outstandingBytesQlist +=
            FileStoreProtocolUtil::bmqHeader(qlistFd).headerWords() *
            bmqp::Protocol::k_WORD_SIZE;
        fileSetSp->d_outstandingBytesQlist += qit.header().headerWords() *
                                              bmqp::Protocol::k_WORD_SIZE;
    }

    // Offsets where files should be written from (in other words, the offset
    // of the *end* of last valid record in each file).

    QueueKeyInfoMap* queueKeyInfoMap_p = 0;
    QueueKeyInfoMap  queueKeyInfoMapEmpty;
    if (d_isFSMWorkflow) {
        queueKeyInfoMap_p = const_cast<QueueKeyInfoMap*>(&queueKeyInfoMap);
    }
    else {
        queueKeyInfoMap_p = &queueKeyInfoMapEmpty;
    }

    bsls::Types::Uint64 journalFileOffset = 0;
    bsls::Types::Uint64 qlistFileOffset   = 0;
    bsls::Types::Uint64 dataFileOffset    = 0;

    BALL_LOG_INFO << partitionDesc()
                  << "Attempting to recover messages from the local storage.";

    // jit, qit & dit may get invalidated after the call below.

    rc = recoverMessages(queueKeyInfoMap_p,
                         &journalFileOffset,
                         &qlistFileOffset,
                         &dataFileOffset,
                         &jit,
                         &qit,
                         &dit);
    if (0 != rc) {
        BALL_LOG_ERROR << partitionDesc() << "Failed to recover messages from"
                       << " storage, rc: " << rc;
        return 100 * rc + rc_RECOVERY_FAILURE;  // RETURN
    }

    BALL_LOG_INFO << partitionDesc()
                  << "Finished recovering messages from local storage. End "
                  << "offsets of JOURNAL, QLIST and DATA files respectively: "
                  << journalFileOffset << ", " << qlistFileOffset << ", "
                  << dataFileOffset;

    // Offsets populated above must not be greater than the file size.
    BSLS_ASSERT_SAFE(journalFileOffset <= journalFd.fileSize());
    if (needQList) {
        BSLS_ASSERT_SAFE(qlistFileOffset <= qlistFd.fileSize());
    }
    BSLS_ASSERT_SAFE(dataFileOffset <= dataFd.fileSize());

    // Ensure that 'd_syncPoints' data structure must be in sync with what has
    // been recovered from the storage.  Since last record in a 1-node cluster
    // need not be a SyncPt, this validation is performed only for multi-node
    // cluster.

    if (1 < clusterSize() && 0 < d_primaryLeaseId) {
        // In a multi-node cluster, retrieved primaryLeaseId is valid.

        if (0 == d_sequenceNum) {
            BALL_LOG_ERROR << partitionDesc() << "Invalid sequence number ["
                           << d_sequenceNum << "] while primary leaseId is "
                           << "valid [" << d_primaryLeaseId << "].";
            return rc_INVALID_SYNC_PT;  // RETURN
        }

        if (!d_isFSMWorkflow) {
            if (d_syncPoints.empty()) {
                BALL_LOG_ERROR << partitionDesc()
                               << "Internal list of SyncPts is empty, while "
                               << "retrieved (leaseId, seqNum) are valid: ("
                               << d_primaryLeaseId << ", " << d_sequenceNum
                               << ").";
                return rc_INVALID_SYNC_PT;  // RETURN
            }

            const bmqp_ctrlmsg::SyncPoint& lastSp =
                d_syncPoints.back().syncPoint();
            if (lastSp.primaryLeaseId() > d_primaryLeaseId) {
                BALL_LOG_ERROR
                    << partitionDesc() << "Invalid leaseId in the last "
                    << "in-memory SyncPt: " << lastSp
                    << ". Retrieved (leaseId, seqNum): (" << d_primaryLeaseId
                    << ", " << d_sequenceNum << ").";
                return rc_INVALID_SYNC_PT;  // RETURN
            }

            if (lastSp.primaryLeaseId() == d_primaryLeaseId &&
                lastSp.sequenceNum() != d_sequenceNum) {
                BALL_LOG_ERROR
                    << partitionDesc() << "Invalid seqNum in the last "
                    << "in-memory SyncPt: " << lastSp
                    << ". Retrieved (leaseId, seqNum): (" << d_primaryLeaseId
                    << ", " << d_sequenceNum << ").";
                return rc_INVALID_SYNC_PT;  // RETURN
            }
        }
    }

    // Ensure that the maximum file size for the partition is equal or greater
    // than the offsets obtained above.

    if (dataFileOffset > d_config.maxDataFileSize() ||
        (qlistFileOffset > d_config.maxQlistFileSize() && needQList)) {
        BALL_LOG_ERROR << partitionDesc() << "Maximum file sizes specified in "
                       << "configuration: (" << d_config.maxJournalFileSize()
                       << ", " << d_config.maxDataFileSize() << ", "
                       << d_config.maxQlistFileSize() << ") is smaller than "
                       << "file sizes on disk for recovery: ("
                       << journalFileOffset << ", " << dataFileOffset << ", "
                       << qlistFileOffset << ") for JOURNAL, DATA and QLIST "
                       << "files respectively.";
        return rc_CONFIGURATION_ERROR;  // RETURN
    }

    // See if journal is full.  If it is, the only way is to update the config
    // to bump the journal file size.

    if (d_config.maxJournalFileSize() <
        (journalFileOffset + 2 * FileStoreProtocol::k_JOURNAL_RECORD_SIZE)) {
        BALL_LOG_ERROR << partitionDesc() << "Journal does not have enough "
                       << "space. Journal offset: " << journalFileOffset
                       << ", max journal file size per config: "
                       << d_config.maxJournalFileSize()
                       << ", additional space (in bytes) required in journal: "
                       << (2 * FileStoreProtocol::k_JOURNAL_RECORD_SIZE);
        return rc_PARTITION_FULL;  // RETURN
    }

    // Check if DATA and QLIST files are full.  This is not a show stopper,
    // as we may still get some deletions which may make space in those files.

    if (dataFileOffset == d_config.maxDataFileSize() ||
        (qlistFileOffset == d_config.maxQlistFileSize() && needQList)) {
        BALL_LOG_ERROR << partitionDesc() << "Maximum file sizes specified in "
                       << "configuration: (" << d_config.maxDataFileSize()
                       << ", " << d_config.maxQlistFileSize()
                       << ") is equal to at least one file size on disk for "
                       << "recovery: (" << dataFileOffset << ", "
                       << qlistFileOffset
                       << ") for DATA and QLIST files respectively.";
    }

    // Close file set as it is in read mode.
    close(*fileSetSp,
          false);  // flush

    // Re-open in write mode, grow & map (do not delete on failure).
    mwcu::MemOutStream errorDesc;
    recoveryFileSet.setJournalFileSize(d_config.maxJournalFileSize())
        .setDataFileSize(d_config.maxDataFileSize());
    if (needQList) {
        recoveryFileSet.setQlistFileSize(d_config.maxQlistFileSize());
    }
    rc = FileStoreUtil::openFileSetWriteMode(
        errorDesc,
        recoveryFileSet,
        d_config.hasPreallocate(),
        false,  // don't delete on error
        &fileSetSp->d_journalFile,
        &fileSetSp->d_dataFile,
        needQList ? &fileSetSp->d_qlistFile : 0,
        d_config.hasPrefaultPages());

    if (0 != rc) {
        BALL_LOG_ERROR << partitionDesc() << "Failed to open file set in write"
                       << " mode during recovery. Reason: " << errorDesc.str();
        return rc * 100 + rc_OPEN_FAILURE;  // RETURN
    }

    // Update file positions to point to the offsets retrieved in
    // 'recoverMessages' call above.

    fileSetSp->d_journalFilePosition = journalFileOffset;
    fileSetSp->d_dataFilePosition    = dataFileOffset;
    if (needQList) {
        fileSetSp->d_qlistFilePosition = qlistFileOffset;
    }

    // Check if we need to write a sync point in 1-node cluster.  It is
    // important to set the file positions (done above) before this.

    if (1 == clusterSize() && appendSyncPoint) {
        BSLS_ASSERT_SAFE(!d_isFSMWorkflow);

        BALL_LOG_INFO << partitionDesc() << "Appending a SyncPt with "
                      << "(primaryLeaseId, sequenceNum): (" << d_primaryLeaseId
                      << ", " << (d_sequenceNum + 1)
                      << ") since journal does not end with a SyncPt for this "
                      << "partition belonging to a 1-node cluster.";

        BSLS_ASSERT_SAFE(0 == fileSetSp->d_dataFilePosition %
                                  bmqp::Protocol::k_DWORD_SIZE);
        if (needQList) {
            BSLS_ASSERT_SAFE(0 == fileSetSp->d_qlistFilePosition %
                                      bmqp::Protocol::k_WORD_SIZE);
        }

        // A little back in this routine, we made sure that journal has space
        // for at least 2 records.  Now we can safely right a 'regular' SyncPt.
        // If the journal has space for exactly 2 records, when self node is
        // chosen as the primary, it will right a 'rollover' SyncPt as its
        // first work item, and we should be good then.

        bmqp_ctrlmsg::SyncPoint syncPoint;
        syncPoint.primaryLeaseId()       = d_primaryLeaseId;
        syncPoint.sequenceNum()          = ++d_sequenceNum;
        syncPoint.dataFileOffsetDwords() = fileSetSp->d_dataFilePosition /
                                           bmqp::Protocol::k_DWORD_SIZE;
        if (needQList) {
            syncPoint.qlistFileOffsetWords() = fileSetSp->d_qlistFilePosition /
                                               bmqp::Protocol::k_WORD_SIZE;
        }

        rc = issueSyncPointInternal(SyncPointType::e_REGULAR,
                                    true,
                                    &syncPoint);
        if (0 != rc) {
            BALL_LOG_ERROR << partitionDesc() << "Failed to issue above sync "
                           << "point, rc: " << rc;
            return 100 * rc + rc_SYNC_POINT_FAILURE;  // RETURN
        }
    }

    if (needQList) {
        BALL_LOG_INFO << partitionDesc()
                      << "JOURNAL, QLIST and DATA files will be"
                      << " written to at these offsets respectively: "
                      << journalFileOffset << ", " << qlistFileOffset << ", "
                      << dataFileOffset;
    }
    else {
        BALL_LOG_INFO << partitionDesc() << "JOURNAL and DATA files will be"
                      << " written to at these offsets respectively: "
                      << journalFileOffset << ", " << dataFileOffset;
    }

    // Hand over recovered queues to the storage manager *after* files have
    // been successfully opened.
    BSLS_ASSERT_SAFE(d_config.recoveredQueuesCb());
    d_config.recoveredQueuesCb()(d_config.partitionId(), *queueKeyInfoMap_p);

    if (d_isFSMWorkflow) {
        if (primaryLeaseIdCurr > d_primaryLeaseId ||
            (primaryLeaseIdCurr == d_primaryLeaseId &&
             sequenceNumcurr > d_sequenceNum)) {
            BALL_LOG_INFO << partitionDesc() << "Current partitionSeqeNum: ("
                          << primaryLeaseIdCurr << ", " << sequenceNumcurr
                          << ") is higher than storage-retrieved "
                          << "partitionSeqeNum: (" << d_primaryLeaseId << ", "
                          << d_sequenceNum << ").  This is possible if self "
                          << "replica has received primary status advisory "
                          << "before opening FileStore.  Thus, we keep current"
                          << " partitionSeqeNum.";

            d_primaryLeaseId = primaryLeaseIdCurr;
            d_sequenceNum    = sequenceNumcurr;
        }
    }

    return rc_SUCCESS;
}

int FileStore::recoverMessages(QueueKeyInfoMap*     queueKeyInfoMap,
                               bsls::Types::Uint64* journalOffset,
                               bsls::Types::Uint64* qlistOffset,
                               bsls::Types::Uint64* dataOffset,
                               JournalFileIterator* jit,
                               QlistFileIterator*   qit,
                               DataFileIterator*    dit)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(queueKeyInfoMap);
    if (!d_isFSMWorkflow) {
        BSLS_ASSERT_SAFE(queueKeyInfoMap->empty());
    }
    BSLS_ASSERT_SAFE(journalOffset);
    BSLS_ASSERT_SAFE(dataOffset);
    BSLS_ASSERT_SAFE(jit);
    BSLS_ASSERT_SAFE(dit);
    BSLS_ASSERT_SAFE(jit->isReverseMode());
    BSLS_ASSERT_SAFE(0 < d_fileSets.size());
    BSLS_ASSERT_SAFE(d_fileSets[0].get());

    const bool needQList = !d_isFSMWorkflow;
    if (needQList) {
        BSLS_ASSERT_SAFE(qlistOffset);
        BSLS_ASSERT_SAFE(qit);
    }

    enum {
        rc_SUCCESS                  = 0,
        rc_INVALID_JOURNAL_RECORD   = -1,
        rc_INVALID_PRIMARY_LEASE_ID = -2,
        rc_INVALID_SEQ_NUMBER       = -3,
        rc_INVALID_QUEUE_OP_RECORD  = -4,
        rc_NULL_QUEUE_KEY           = -5,
        rc_INVALID_QLIST_OFFSET     = -6,
        rc_DUPLICATE_QUEUE_KEY      = -7,
        rc_UNEXPECTED_QUEUE_OP_TYPE = -8,
        rc_INVALID_QUEUE_KEY        = -9,
        rc_QUEUE_URI_MISMATCH       = -10,
        rc_INVALID_DATA_OFFSET      = -11,
        rc_INVALID_SYNC_PT_SUB_TYPE = -12,
        rc_INVALID_QLIST_RECORD     = -13,
        rc_INVALID_DELETION_RECORD  = -14,
        rc_INVALID_CONFIRM_RECORD   = -15,
        rc_INVALID_MESSAGE_RECORD   = -16,
        rc_INVALID_DATA_RECORD      = -17,
        rc_INVALID_PARTITION_ID     = -18
    };

    FileSet*                    activeFileSet = d_fileSets[0].get();
    const MappedFileDescriptor* dataFd        = dit->mappedFileDescriptor();
    const MappedFileDescriptor* qlistFd       = needQList
                                                    ? qit->mappedFileDescriptor()
                                                    : 0;

    FileStoreUtil::setFileHeaderOffsets(journalOffset,
                                        dataOffset,
                                        *jit,
                                        *dit,
                                        needQList,
                                        qlistOffset,
                                        *qit);

    // Create scratch data structure to keep track of deleted queueKeys and
    // appKeys with their deletion record offsets.

    typedef bsl::unordered_map<mqbu::StorageKey,
                               bsls::Types::Uint64,
                               bslh::Hash<mqbu::StorageKeyHashAlgo> >
                                               StorageKeysOffsets;
    typedef StorageKeysOffsets::iterator       StorageKeysOffsetsIter;
    typedef StorageKeysOffsets::const_iterator StorageKeysOffsetsConstIter;
    typedef bsl::pair<StorageKeysOffsetsIter, bool> StorageKeysOffsetsInsertRc;

    typedef bsl::unordered_set<mqbu::StorageKey,
                               bslh::Hash<mqbu::StorageKeyHashAlgo> >
        StorageKeys;

    StorageKeys         queueOpAdditionQueueKeys;
    StorageKeysOffsets  deletedQueueKeysOffsets;
    StorageKeysOffsets  deletedAppKeysOffsets;
    bsls::Types::Uint64 firstSyncPtOffset = 0;

    // Make a copy of JOURNAL file iterator as we will be making 2 passes over
    // it.  Both passes will be in reverse iteration.  First pass will retrieve
    // the list of non-deleted queues in a scratch data structure, and second
    // pass will retrieve outstanding records for all those non-deleted queues.
    // The in-memory 'd_records' structure will be updated only in the second
    // pass.  DATA and QLIST files are not read during the 1st pass.

    JournalFileIterator journalIt(*jit);
    BSLS_ASSERT_SAFE(journalIt.isReverseMode());

    // First pass.
    int rc = 0;
    while ((rc = journalIt.nextRecord()) == 1) {
        const RecordHeader& recHeader = journalIt.recordHeader();
        RecordType::Enum    rt        = recHeader.type();
        if (rt == RecordType::e_UNDEFINED) {
            BALL_LOG_ERROR << partitionDesc() << "Encountered invalid JOURNAL "
                           << "record while reverse-iterating JOURNAL during "
                           << "recovery. Record offset: "
                           << journalIt.recordOffset()
                           << ", record index: " << journalIt.recordIndex();
            return rc_INVALID_JOURNAL_RECORD;  // RETURN
        }

        if (recHeader.primaryLeaseId() == 0 ||
            recHeader.sequenceNumber() == 0) {
            BALL_LOG_ERROR << partitionDesc() << "Encountered invalid leaseId/"
                           << "sequenceNumber while reverse-iterating JOURNAL "
                           << "during recovery. LeaseId: "
                           << recHeader.primaryLeaseId()
                           << ", sequence num: " << recHeader.sequenceNumber()
                           << ". Record offset: " << journalIt.recordOffset()
                           << ", record index: " << journalIt.recordIndex();
            return rc_INVALID_JOURNAL_RECORD;  // RETURN
        }

        if (RecordType::e_JOURNAL_OP == rt) {
            firstSyncPtOffset = journalIt.recordOffset();
            continue;  // CONTINUE
        }

        if (RecordType::e_QUEUE_OP != rt) {
            continue;  // CONTINUE
        }

        const QueueOpRecord&    rec         = journalIt.asQueueOpRecord();
        QueueOpType::Enum       queueOpType = rec.type();
        const mqbu::StorageKey& queueKey    = rec.queueKey();
        const mqbu::StorageKey& appKey      = rec.appKey();

        if (QueueOpType::e_UNDEFINED == queueOpType) {
            BALL_LOG_ERROR << partitionDesc() << "Encountered a QueueOp record"
                           << " with invalid sub-type during 1st pass reverse "
                           << "iteration of the journal during recovery. "
                           << "Record offset: " << journalIt.recordOffset()
                           << ", record index: " << journalIt.recordIndex();
            return rc_INVALID_QUEUE_OP_RECORD;  // RETURN
        }

        if (queueKey.isNull()) {
            BALL_LOG_ERROR << partitionDesc() << "Encountered a QueueOp record"
                           << " of sub-type [" << queueOpType
                           << "] with null queueKey during 1st pass reverse "
                           << "iteration of the journal during recovery. "
                           << "Record offset: " << journalIt.recordOffset()
                           << ", record index: " << journalIt.recordIndex();
            return rc_NULL_QUEUE_KEY;  // RETURN
        }

        if (QueueOpType::e_DELETION == queueOpType) {
            if (d_isFSMWorkflow) {
                if (appKey.isNull() && queueKeyInfoMap->end() !=
                                           queueKeyInfoMap->find(queueKey)) {
                    BALL_LOG_ERROR
                        << partitionDesc()
                        << "Encountered a QueueOp.DELETION record "
                        << "for queueKey [" << queueKey
                        << "] for the entire queue during 1st pass"
                        << " reverse iteration, but the queueKey "
                        << "is alive in cluster state.  Record "
                        << "offset: " << journalIt.recordOffset()
                        << ", record index: " << journalIt.recordIndex();

                    return rc_INVALID_DELETION_RECORD;  // RETURN
                }
            }
            else {
                if (queueKeyInfoMap->end() !=
                    queueKeyInfoMap->find(queueKey)) {
                    // A QueueOpRecord.CREATION/ADDITION for this queueKey was
                    // encountered earlier in the backward iteration of this
                    // journal.  This means that a queue was created with this
                    // queue key after a (same or different) queue with the
                    // same queue key was deleted.  We do try to enforce
                    // uniqueness of queue keys but it may not be possible all
                    // the time (what we do guarantee is that only one queue
                    // key will be "live" at any given time).  So we just warn.
                    // We do, however, keep track of this deletion record for
                    // this queueKey.  Same scenario is applicable to AppKeys
                    // as well, and its handled the same way via
                    // 'deletedAppKeysOffsets'.

                    BALL_LOG_WARN
                        << partitionDesc() << "Encountered a "
                        << "QueueOp.DELETION record for queueKey [" << queueKey
                        << "], appKey [" << appKey
                        << "] at offset: " << journalIt.recordOffset()
                        << ", for which a QueueOp.CREATION record "
                        << "was encountered earlier.";
                }
            }

            if (appKey.isNull()) {
                StorageKeysOffsetsInsertRc irc =
                    deletedQueueKeysOffsets.insert(
                        bsl::make_pair(queueKey, journalIt.recordOffset()));

                if (!irc.second) {
                    const OffsetPtr<const QueueOpRecord> originalRec(
                        MemoryBlock(
                            journalIt.mappedFileDescriptor()->mapping(),
                            journalIt.mappedFileDescriptor()->fileSize()),
                        irc.first->second);

                    BALL_LOG_WARN
                        << partitionDesc() << "Encountered a "
                        << "QueueOp.DELETION record for queueKey [" << queueKey
                        << "] & no appKey, at offset: "
                        << journalIt.recordOffset()
                        << ", for which a QueueOp.DELETION record "
                        << "was encountered before in backward "
                        << "iteration at offset: " << irc.first->second
                        << ". Current record timestamp: "
                        << bdlt::EpochUtil::convertFromTimeT64(
                               rec.header().timestamp())
                        << ", original record timestamp: "
                        << bdlt::EpochUtil::convertFromTimeT64(
                               originalRec->header().timestamp())
                        << ". Ignoring this record.";
                }
            }
            else {
                StorageKeysOffsetsInsertRc irc = deletedAppKeysOffsets.insert(
                    bsl::make_pair(appKey, journalIt.recordOffset()));

                if (!irc.second) {
                    const OffsetPtr<const QueueOpRecord> originalRec(
                        MemoryBlock(
                            journalIt.mappedFileDescriptor()->mapping(),
                            journalIt.mappedFileDescriptor()->fileSize()),
                        irc.first->second);

                    BALL_LOG_WARN
                        << partitionDesc() << "Encountered a "
                        << "QueueOp.DELETION record for queueKey [" << queueKey
                        << "], appKey [" << appKey
                        << "], at offset: " << journalIt.recordOffset()
                        << ", but a QueueOp.DELETION record for "
                        << "same appKey was encountered before in "
                        << "backward iteration at offset: "
                        << irc.first->second << ". Current record timestamp: "
                        << bdlt::EpochUtil::convertFromTimeT64(
                               rec.header().timestamp())
                        << ", original record timestamp: "
                        << bdlt::EpochUtil::convertFromTimeT64(
                               originalRec->header().timestamp())
                        << ". Ignoring this record for the appKey.";
                }
            }
        }
        else if (QueueOpType::e_PURGE == queueOpType) {
            // No need to keep track of purge records in first pass.  We are
            // interested only in retrieving list of 'alive' queues and appIds
            // in the 1st pass, and a purge record doesn't affect the lifecycle
            // of either of them.

            continue;  // CONTINUE
        }
        else if (QueueOpType::e_ADDITION == queueOpType) {
            // No need to keep track of appIds/appKeys in the 1st pass.  It
            // will be done in 2nd pass.  Just perform some basic validation.

            if (needQList && 0 == rec.queueUriRecordOffsetWords()) {
                // Invalid record.

                BALL_LOG_ERROR
                    << partitionDesc() << "Encountered a QueueOp.ADDITION "
                    << "record with invalid QLIST file offset field during 1st"
                    << " pass reverse iteration of the journal during "
                    << "recovery. Record offset: " << journalIt.recordOffset()
                    << ", record index: " << journalIt.recordIndex();

                return rc_INVALID_QLIST_OFFSET;  // RETURN
            }

            if (1 == deletedQueueKeysOffsets.count(queueKey)) {
                // Queue has been deleted.

                continue;  // CONTINUE
            }

            if (d_isFSMWorkflow) {
                if (queueKeyInfoMap->end() ==
                    queueKeyInfoMap->find(queueKey)) {
                    BALL_LOG_ERROR
                        << partitionDesc()
                        << "Encountered a QueueOp.ADDITION record "
                        << "for queueKey [" << queueKey
                        << "] during 1st pass reverse iteration, "
                        << "but the queueKey is not present in "
                        << "cluster state.  Record offset: "
                        << journalIt.recordOffset()
                        << ", record index: " << journalIt.recordIndex();

                    return rc_INVALID_QUEUE_KEY;  // RETURN
                }
            }
            else if (queueKeyInfoMap->end() !=
                     queueKeyInfoMap->find(queueKey)) {
                // This means that we encountered a QueueOp.CREATION record for
                // this queueKey, without a QueueOp.DELETION record b/w the
                // two.  In other words, two queues with same queueKey (which
                // may or may not have same URI) are alive at the same time.

                MWCTSK_ALARMLOG_ALARM("RECOVERY")
                    << partitionDesc()
                    << "Encountered a QueueOp.ADDITION record for queueKey ["
                    << queueKey << "] for which a QueueOp.CREATION record "
                    << "was seen earlier during first pass backward iteration."
                    << " Record offset: " << journalIt.recordOffset()
                    << ", record index: " << journalIt.recordIndex()
                    << MWCTSK_ALARMLOG_END;
                return rc_DUPLICATE_QUEUE_KEY;  // RETURN
            }

            // AppKey specified in the QueueOp.ADDITION record is supposed to
            // be null, because the AppId/AppKey combo goes in the
            // corresponding QLIST file.  We could check that here if desired.

            continue;  // CONTINUE
        }
        else if (QueueOpType::e_CREATION == queueOpType) {
            if (needQList && 0 == rec.queueUriRecordOffsetWords()) {
                // Invalid record.

                BALL_LOG_ERROR
                    << partitionDesc() << "Encountered a QueueOp.CREATION "
                    << "record with invalid QLIST file offset field during 1st"
                    << " pass reverse iteration of the journal during "
                    << "recovery. Record offset: " << journalIt.recordOffset()
                    << ", record index: " << journalIt.recordIndex();

                return rc_INVALID_QLIST_OFFSET;  // RETURN
            }

            if (1 == deletedQueueKeysOffsets.count(queueKey)) {
                // Queue has been deleted.  No need to keep track of this
                // queue record.

                continue;  // CONTINUE
            }

            if (d_isFSMWorkflow) {
                if (queueKeyInfoMap->end() ==
                    queueKeyInfoMap->find(queueKey)) {
                    BALL_LOG_ERROR
                        << partitionDesc()
                        << "Encountered a QueueOp.CREATION record "
                        << "for queueKey [" << queueKey
                        << "] during 1st pass reverse iteration, "
                        << "but the queueKey is not present in "
                        << "cluster state.  Record offset: "
                        << journalIt.recordOffset()
                        << ", record index: " << journalIt.recordIndex();

                    return rc_INVALID_QUEUE_KEY;  // RETURN
                }
            }
            else {
                // Update 'queueKeyInfoMap' with correct queueKey, but empty
                // queue info.  Queue info (uri, appIds/appKeys) will be
                // updated in second pass.  Note that for a given queue key,
                // QueueOp.CREATION record must be unique.  Also note that
                // appIds/appKeys are retrieved in the 2nd pass (same for
                // 'QueueOpType::e_ADDITION' as well).

                QueueKeyInfoMapInsertRc insertRc = queueKeyInfoMap->insert(
                    bsl::make_pair(queueKey, DataStoreConfigQueueInfo()));
                insertRc.first->second.setPartitionId(d_config.partitionId());

                if (false == insertRc.second) {
                    // This means that we encountered a second QueueOp.CREATION
                    // record without a QueueOp.DELETION record b/w the two.
                    // In other words, two queues with same queue key (which
                    // may or may not have same URI) are alive at the same
                    // time.

                    MWCTSK_ALARMLOG_ALARM("RECOVERY")
                        << partitionDesc()
                        << "Encountered a second QueueOp.CREATION record for "
                        << "queueKey [" << queueKey << "] during 1st pass "
                        << "reverse iteration. Record offset: "
                        << journalIt.recordOffset()
                        << ", record index: " << journalIt.recordIndex()
                        << MWCTSK_ALARMLOG_END;
                    return rc_DUPLICATE_QUEUE_KEY;  // RETURN
                }
            }

            BALL_LOG_INFO
                << partitionDesc()
                << "Retrieved a QueueOp.CREATION record for queueKey ["
                << queueKey << "] during 1st pass reverse iteration. "
                << "Record offset: " << journalIt.recordOffset()
                << ",  record index: " << journalIt.recordIndex() << ".";
        }
        else {
            MWCTSK_ALARMLOG_ALARM("RECOVERY")
                << partitionDesc()
                << "Encountered a QueueOp record with unexpected QueueOpType: "
                << queueOpType << ", for queueKey [" << queueKey
                << "] during first pass backward iteration. "
                << "Record offset: " << journalIt.recordOffset()
                << ", record index: " << journalIt.recordIndex()
                << MWCTSK_ALARMLOG_END;
            return rc_UNEXPECTED_QUEUE_OP_TYPE;  // RETURN
        }
    }

    BALL_LOG_INFO << partitionDesc() << "Completed first pass over the journal"
                  << " with rc: " << rc
                  << ". Offset of 1st SyncPt: " << firstSyncPtOffset << ".";

    typedef bsl::unordered_set<bmqt::MessageGUID,
                               bslh::Hash<bmqt::MessageGUIDHashAlgo> >
                                  Guids;
    typedef Guids::const_iterator GuidsCIter;

    Guids        deletedGuids;
    StorageKeys  purgedQueueKeys;
    StorageKeys  purgedAppKeys;
    bool         isLastJournalRecord = true;  // ie, first in iteration
    bool         isLastMessageRecord = true;  // ie, first in iteration
    bool         isLastQlistRecord   = true;  // ie, first in iteration
    unsigned int primaryLeaseId      = d_primaryLeaseId;

    // `+1` so that checks in first iteration in the second pass work
    // correctly.
    bsls::Types::Uint64 sequenceNum = d_sequenceNum + 1;

    // Second pass.
    while (1 == (rc = jit->nextRecord())) {
        const RecordHeader& recHeader = jit->recordHeader();
        RecordType::Enum    rt        = recHeader.type();
        BSLS_ASSERT_SAFE(RecordType::e_UNDEFINED != rt);
        BSLS_ASSERT_SAFE(0 != recHeader.primaryLeaseId());
        BSLS_ASSERT_SAFE(0 != recHeader.sequenceNumber());

        // Validate (leaseId, seqNum) in the RecordHeader. Note that leaseId in
        // the RecordHeader can be smaller than 'primaryLeaseId'.

        if (recHeader.primaryLeaseId() > primaryLeaseId) {
            BALL_LOG_ERROR
                << partitionDesc()
                << "Encountered a record during backward journal iteration "
                << "with invalid leaseId: " << recHeader.primaryLeaseId()
                << ". It cannot be greater than " << primaryLeaseId
                << ". Record offset: " << jit->recordOffset()
                << ", record index: " << jit->recordIndex();

            return rc_INVALID_PRIMARY_LEASE_ID;  // RETURN
        }

        if (recHeader.primaryLeaseId() == primaryLeaseId) {
            bool invalidSeqNum = false;

            if (jit->recordOffset() >= firstSyncPtOffset) {
                // Not a rolled-over record.

                if (recHeader.sequenceNumber() != (sequenceNum - 1)) {
                    invalidSeqNum = true;
                }
            }
            else {
                // Sequence numbers of rolled over records may not increment by
                // 1, but they should still be monotonically increasing.

                if (recHeader.sequenceNumber() > (sequenceNum - 1)) {
                    invalidSeqNum = true;
                }
            }

            if (invalidSeqNum) {
                BALL_LOG_ERROR
                    << partitionDesc()
                    << "Encountered a record during backward journal iteration"
                    << " with invalid seqnum: " << recHeader.sequenceNumber()
                    << ". Expected seqNum: " << (sequenceNum - 1)
                    << " (or smaller). PrimaryLeaseId: "
                    << recHeader.primaryLeaseId()
                    << ". Record offset: " << jit->recordOffset()
                    << ", record index: " << jit->recordIndex()
                    << ". Rolled-over record: " << bsl::boolalpha
                    << (jit->recordOffset() < firstSyncPtOffset) << ".";

                return rc_INVALID_SEQ_NUMBER;  // RETURN
            }
        }

        primaryLeaseId = recHeader.primaryLeaseId();
        sequenceNum    = recHeader.sequenceNumber();

        // Update journal file offset only if its the last record in the
        // journal (ie, first record in iteration since we are iterating
        // backwards).

        if (isLastJournalRecord) {
            isLastJournalRecord = false;
            *journalOffset      = jit->recordOffset() +
                             (jit->header().recordWords() *
                              bmqp::Protocol::k_WORD_SIZE);
        }

        if (RecordType::e_JOURNAL_OP == rt) {
            const JournalOpRecord& rec = jit->asJournalOpRecord();
            // Perform basic sanity check for as many fields as possible.

            if (SyncPointType::e_UNDEFINED == rec.syncPointType()) {
                BALL_LOG_ERROR
                    << partitionDesc()
                    << "Encountered a SyncPt during backward journal iteration"
                    << " with invalid sub-type. Record offset: "
                    << jit->recordOffset()
                    << ", record index: " << jit->recordIndex();

                return rc_INVALID_SYNC_PT_SUB_TYPE;  // RETURN
            }

            if (0 == rec.dataFileOffsetDwords()) {
                BALL_LOG_ERROR
                    << partitionDesc()
                    << "Encountered a SyncPt during backward journal iteration"
                    << " with invalid DATA file offset field. Record offset: "
                    << jit->recordOffset()
                    << ", record index: " << jit->recordIndex();

                return rc_INVALID_DATA_OFFSET;  // RETURN
            }

            if (dataFd->fileSize() <
                (static_cast<bsls::Types::Uint64>(rec.dataFileOffsetDwords()) *
                 bmqp::Protocol::k_DWORD_SIZE)) {
                BALL_LOG_ERROR
                    << partitionDesc()
                    << "Encountered a SyncPt during backward journal iteration"
                    << " with DATA file offset field ["
                    << (static_cast<bsls::Types::Uint64>(
                            rec.dataFileOffsetDwords()) *
                        bmqp::Protocol::k_DWORD_SIZE)
                    << "], which is greater than DATA file size ["
                    << dataFd->fileSize()
                    << "]. Record offset: " << jit->recordOffset()
                    << ", record index: " << jit->recordIndex();

                return rc_INVALID_DATA_OFFSET;  // RETURN
            }

            if (needQList && 0 == rec.qlistFileOffsetWords()) {
                BALL_LOG_ERROR
                    << partitionDesc()
                    << "Encountered a SyncPt during backward journal iteration"
                    << " with invalid QLIST file offset field. Record offset: "
                    << jit->recordOffset()
                    << ", record index: " << jit->recordIndex();

                return rc_INVALID_QLIST_OFFSET;  // RETURN
            }

            if (needQList &&
                qlistFd->fileSize() < (static_cast<bsls::Types::Uint64>(
                                           rec.qlistFileOffsetWords()) *
                                       bmqp::Protocol::k_WORD_SIZE)) {
                BALL_LOG_ERROR
                    << partitionDesc()
                    << "Encountered a SyncPt during backward journal iteration"
                    << " with QLIST file offset field ["
                    << (static_cast<bsls::Types::Uint64>(
                            rec.qlistFileOffsetWords()) *
                        bmqp::Protocol::k_WORD_SIZE)
                    << "], which is greater than QLIST file size ["
                    << qlistFd->fileSize()
                    << "]. Record offset: " << jit->recordOffset()
                    << ", record index: " << jit->recordIndex();

                return rc_INVALID_QLIST_OFFSET;  // RETURN
            }

            if (0 == rec.primaryLeaseId()) {
                BALL_LOG_ERROR
                    << partitionDesc()
                    << "Encountered a SyncPt during backward journal iteration"
                    << " with zero primaryLeaseId, current primaryLeaseId: "
                    << primaryLeaseId
                    << ". Record offset: " << jit->recordOffset()
                    << ", record index: " << jit->recordIndex();

                return rc_INVALID_PRIMARY_LEASE_ID;  // RETURN
            }

            if (0 == rec.sequenceNum()) {
                BALL_LOG_ERROR
                    << partitionDesc()
                    << "Encountered a SyncPt during backward journal iteration"
                    << " with zero sequenceNum, current sequenceNum: "
                    << sequenceNum
                    << ". Record offset: " << jit->recordOffset()
                    << ", record index: " << jit->recordIndex();

                return rc_INVALID_SEQ_NUMBER;  // RETURN
            }

            // Ensure that leaseId/seqNum in the SyncPt are within expected
            // bound.  Note that leaseId appearing in the SyncPt can be smaller
            // than the one appearing in its RecordHeader.  This can occur if
            // that SyncPt was issued by new primary on behalf of the old one
            // upon being chosen as the primary.

            if (rec.primaryLeaseId() > primaryLeaseId) {
                MWCTSK_ALARMLOG_ALARM("RECOVERY")
                    << partitionDesc()
                    << "Encountered a SyncPt during backward journal "
                    << "iteration with higher primaryLeaseId: "
                    << rec.primaryLeaseId()
                    << ", current primaryLeaseId: " << primaryLeaseId
                    << ". Record offset: " << jit->recordOffset()
                    << ", record index: " << jit->recordIndex()
                    << MWCTSK_ALARMLOG_END;

                return rc_INVALID_PRIMARY_LEASE_ID;  // RETURN
            }

            if (rec.primaryLeaseId() == primaryLeaseId) {
                if (rec.sequenceNum() != sequenceNum) {
                    MWCTSK_ALARMLOG_ALARM("RECOVERY")
                        << partitionDesc()
                        << "Encountered a Syncpt during backward journal "
                        << "iteration with incorrect sequence number: "
                        << rec.sequenceNum()
                        << ", expected sequence number: " << sequenceNum
                        << ". Record offset: " << jit->recordOffset()
                        << ", record index: " << jit->recordIndex()
                        << MWCTSK_ALARMLOG_END;
                    return rc_INVALID_SEQ_NUMBER;  // RETURN
                }
            }

            // Keep track of sync point encountered in the journal.

            bmqp_ctrlmsg::SyncPointOffsetPair spoPair;
            bmqp_ctrlmsg::SyncPoint&          syncPoint = spoPair.syncPoint();
            syncPoint.primaryLeaseId()                  = rec.primaryLeaseId();
            syncPoint.sequenceNum()                     = rec.sequenceNum();
            syncPoint.dataFileOffsetDwords() = rec.dataFileOffsetDwords();
            if (needQList) {
                syncPoint.qlistFileOffsetWords() = rec.qlistFileOffsetWords();
            }
            spoPair.offset() = jit->recordOffset();

            d_syncPoints.push_front(spoPair);

            // No need to update outstanding journal bytes, since SyncPts are
            // not rolled over.
        }
        else if (RecordType::e_QUEUE_OP == rt) {
            const QueueOpRecord&    rec         = jit->asQueueOpRecord();
            QueueOpType::Enum       queueOpType = rec.type();
            const mqbu::StorageKey& queueKey    = rec.queueKey();
            const mqbu::StorageKey& appKey      = rec.appKey();

            BSLS_ASSERT_SAFE(QueueOpType::e_UNDEFINED != queueOpType);

            if (QueueOpType::e_DELETION == queueOpType) {
                if (1 == deletedQueueKeysOffsets.count(queueKey)) {
                    // Entire QueueKey has been deleted.  No need to insert
                    // QueueOpRecord.DELETION in 'd_records'.

                    continue;  // CONTINUE
                }

                // QueueKey has not been deleted, but since this is a deletion
                // queue op record, a specific appKey of the queue must have
                // been deleted, which means appKey must be non-null, and must
                // appear in the 'deleted appKeys' list.

                BSLS_ASSERT_SAFE(!appKey.isNull());
                BSLS_ASSERT_SAFE(1 == deletedAppKeysOffsets.count(appKey));

                // Need to keep track of this record (AppKey deletion record)
                // in 'd_records'.

                DataStoreRecordKey key(sequenceNum, primaryLeaseId);
                DataStoreRecord    record(RecordType::e_QUEUE_OP,
                                       jit->recordOffset());
                d_records.rinsert(bsl::make_pair(key, record));

                // Update outstanding JOURNAL bytes.

                activeFileSet->d_outstandingBytesJournal +=
                    FileStoreProtocol::k_JOURNAL_RECORD_SIZE;
            }
            else if (QueueOpType::e_PURGE == queueOpType) {
                StorageKeysOffsetsConstIter queueIt =
                    deletedQueueKeysOffsets.find(queueKey);

                if (queueIt != deletedQueueKeysOffsets.end()) {
                    BSLS_ASSERT_SAFE(jit->recordOffset() != queueIt->second);
                    if (jit->recordOffset() < queueIt->second) {
                        // This record appears before the QueueOp.DELETION
                        // record for this queueKey so should be ignored.

                        continue;  // CONTINUE
                    }
                }

                if (0 == queueKeyInfoMap->count(queueKey)) {
                    if (d_isFSMWorkflow) {
                        BALL_LOG_ERROR
                            << partitionDesc()
                            << "Encountered a QueueOp.PURGE record for "
                            << "queueKey [" << queueKey
                            << "], offset: " << jit->recordOffset()
                            << ", index: " << jit->recordIndex()
                            << ", but the queueKey is not present in cluster "
                            << "state.";
                        return rc_INVALID_QUEUE_KEY;  // RETURN
                    }
                    else {
                        MWCTSK_ALARMLOG_ALARM("RECOVERY")
                            << partitionDesc()
                            << "Encountered a QueueOp.PURGE record for "
                            << "queueKey [" << queueKey
                            << "], offset: " << jit->recordOffset()
                            << ", index: " << jit->recordIndex()
                            << ", for which a QueueOp.CREATION record was not "
                            << "seen in first pass." << MWCTSK_ALARMLOG_END;
                        return rc_INVALID_QUEUE_KEY;  // RETURN
                    }
                }

                if (!appKey.isNull()) {
                    // Specific appKey is purged.
                    StorageKeysOffsetsConstIter appKeyIt =
                        deletedAppKeysOffsets.find(appKey);

                    if (appKeyIt != deletedAppKeysOffsets.end()) {
                        BSLS_ASSERT_SAFE(jit->recordOffset() !=
                                         appKeyIt->second);
                        if (jit->recordOffset() < appKeyIt->second) {
                            // This record appears before the QueueOp.DELETION
                            // record for this appKey so should be ignored.

                            continue;  // CONTINUE
                        }
                    }
                }
                else {
                    // Entire queue is purged.
                    purgedQueueKeys.insert(queueKey);
                }

                DataStoreRecordKey key(sequenceNum, primaryLeaseId);
                DataStoreRecord    record(RecordType::e_QUEUE_OP,
                                       jit->recordOffset());
                d_records.rinsert(bsl::make_pair(key, record));

                // Update outstanding JOURNAL bytes.
                activeFileSet->d_outstandingBytesJournal +=
                    FileStoreProtocol::k_JOURNAL_RECORD_SIZE;
            }
            else {
                BSLS_ASSERT_SAFE(QueueOpType::e_CREATION == queueOpType ||
                                 QueueOpType::e_ADDITION == queueOpType);

                bsls::Types::Uint64 queueUriRecOffset = 0;
                unsigned int        queueRecLength    = 0;
                unsigned int        queueRecHeaderLen = 0;
                unsigned int        paddedUriLen      = 0;
                unsigned int        numAppIds         = 0;
                if (needQList) {
                    BSLS_ASSERT_SAFE(0 != rec.queueUriRecordOffsetWords());
                    // Checked in 1st iteration.

                    queueUriRecOffset = static_cast<bsls::Types::Uint64>(
                                            rec.queueUriRecordOffsetWords()) *
                                        bmqp::Protocol::k_WORD_SIZE;

                    if (queueUriRecOffset > qlistFd->fileSize()) {
                        BALL_LOG_ERROR
                            << partitionDesc()
                            << "Encountered a QueueOp record of type ["
                            << queueOpType
                            << "], with QLIST file offset field ["
                            << queueUriRecOffset
                            << "] greater than QLIST file size ["
                            << qlistFd->fileSize()
                            << "] during backward journal iteration. Record "
                            << "offset: " << jit->recordOffset()
                            << ", record index: " << jit->recordIndex();

                        return rc_INVALID_QLIST_OFFSET;  // RETURN
                    }

                    // 'queueUriRecOffset' == offset of mqbs::QueueRecordHeader

                    OffsetPtr<const QueueRecordHeader> queueRecHeader(
                        qlistFd->block(),
                        queueUriRecOffset);

                    if (0 == queueRecHeader->headerWords()) {
                        BALL_LOG_ERROR
                            << partitionDesc()
                            << "For QueueOp record of type [" << queueOpType
                            << "], the record present in QLIST file has "
                            << "invalid 'headerWords' field in the header. "
                            << "Journal record offset: " << jit->recordOffset()
                            << ", journal record index: " << jit->recordIndex()
                            << ". QLIST record offset: " << queueUriRecOffset
                            << ".";

                        return rc_INVALID_QLIST_RECORD;  // RETURN
                    }

                    queueRecHeaderLen = queueRecHeader->headerWords() *
                                        bmqp::Protocol::k_WORD_SIZE;

                    if (qlistFd->fileSize() <
                        (queueUriRecOffset + queueRecHeaderLen)) {
                        BALL_LOG_ERROR
                            << partitionDesc()
                            << "For QueueOp record of type [" << queueOpType
                            << "], the record present in QLIST file has "
                            << "invalid header size [" << queueRecHeaderLen
                            << "]. Journal record offset: "
                            << jit->recordOffset()
                            << ", journal record index: " << jit->recordIndex()
                            << ". QLIST record offset: " << queueUriRecOffset
                            << ". QLIST file size: " << qlistFd->fileSize()
                            << ".";
                        return rc_INVALID_QLIST_RECORD;  // RETURN
                    }

                    if (0 == queueRecHeader->queueUriLengthWords()) {
                        BALL_LOG_ERROR
                            << partitionDesc()
                            << "For QueueOp record of type [" << queueOpType
                            << "], the record present in QLIST file has "
                            << "invalid 'queueUriLengthWords' field in the "
                            << "header. Journal record offset: "
                            << jit->recordOffset()
                            << ", journal record index: " << jit->recordIndex()
                            << ". QLIST record offset: " << queueUriRecOffset
                            << ".";
                        return rc_INVALID_QLIST_RECORD;  // RETURN
                    }

                    paddedUriLen = queueRecHeader->queueUriLengthWords() *
                                   bmqp::Protocol::k_WORD_SIZE;

                    if (qlistFd->fileSize() <
                        (queueUriRecOffset + paddedUriLen)) {
                        BALL_LOG_ERROR
                            << partitionDesc()
                            << "For QueueOp record of type [" << queueOpType
                            << "], the record present in QLIST file has "
                            << "invalid 'queueUriLengthWords' field in the "
                            << "header. Journal record offset: "
                            << jit->recordOffset()
                            << ", journal record index: " << jit->recordIndex()
                            << ". QLIST record offset: " << queueUriRecOffset
                            << ". QLIST file size: " << qlistFd->fileSize()
                            << ".";
                        return rc_INVALID_QLIST_RECORD;  // RETURN
                    }

                    queueRecLength = queueRecHeader->queueRecordWords() *
                                     bmqp::Protocol::k_WORD_SIZE;

                    if (0 == queueRecLength) {
                        BALL_LOG_ERROR
                            << partitionDesc()
                            << "For QueueOp record of type [" << queueOpType
                            << "], the record present in QLIST file has "
                            << "invalid 'queueRecordWords' field in the "
                            << "header. Journal record offset: "
                            << jit->recordOffset()
                            << ", journal record index: " << jit->recordIndex()
                            << ". QLIST record offset: " << queueUriRecOffset
                            << ".";
                        return rc_INVALID_QLIST_RECORD;  // RETURN
                    }

                    if (qlistFd->fileSize() <
                        (queueUriRecOffset + queueRecLength)) {
                        BALL_LOG_ERROR
                            << partitionDesc()
                            << "For QueueOp record of type [" << queueOpType
                            << "], the record present in QLIST file has "
                            << "invalid 'queueRecLength' field in the header. "
                            << "Journal record offset: " << jit->recordOffset()
                            << ", journal record index: " << jit->recordIndex()
                            << ". QLIST record offset: " << queueUriRecOffset
                            << ". QLIST file size: " << qlistFd->fileSize()
                            << ".";
                        return rc_INVALID_QLIST_RECORD;  // RETURN
                    }

                    numAppIds = queueRecHeader->numAppIds();

                    // Only QueueOp.CREATION & QueueOp.ADDITION records have
                    // corresponding record in the QLIST file.

                    if (isLastQlistRecord) {
                        isLastQlistRecord = false;
                        *qlistOffset      = queueUriRecOffset + queueRecLength;
                    }
                }

                StorageKeysOffsetsConstIter queueIt =
                    deletedQueueKeysOffsets.find(queueKey);
                if (queueIt != deletedQueueKeysOffsets.end()) {
                    BSLS_ASSERT_SAFE(jit->recordOffset() != queueIt->second);
                    if (jit->recordOffset() < queueIt->second) {
                        // This QueueOp.CREATION/ADDITION record appears before
                        // the QueueOpRecord.DELETION for the same queueKey,
                        // and thus should be ignored.

                        continue;  // CONTINUE
                    }
                }

                QueueKeyInfoMap::iterator iter = queueKeyInfoMap->find(
                    queueKey);
                if (!d_isFSMWorkflow &&
                    QueueOpType::e_ADDITION == queueOpType &&
                    iter == queueKeyInfoMap->end()) {
                    MWCTSK_ALARMLOG_ALARM("RECOVERY")
                        << partitionDesc()
                        << "Encountered a QueueOp.ADDITION record for queueKey"
                        << " [" << queueKey
                        << "], offset: " << jit->recordOffset()
                        << ", index: " << jit->recordIndex()
                        << ", for which a "
                        << "QueueOp.CREATION record was not seen in first "
                        << "pass." << MWCTSK_ALARMLOG_END;
                    return rc_INVALID_QUEUE_KEY;  // RETURN
                }

                BSLS_ASSERT_SAFE(iter != queueKeyInfoMap->end());
                // If its QueueOp.CREATION record, 'queueKeyInfoMap' must
                // contain an entry for 'queueKey', courtesy 1st pass.

                DataStoreConfigQueueInfo& qinfo = iter->second;

                if (needQList) {
                    // Retrieve QueueUri from QueueUriRecord.

                    const char* uriBegin = qlistFd->block().base() +
                                           queueUriRecOffset +
                                           queueRecHeaderLen;
                    char lastByte = uriBegin[paddedUriLen - 1];

                    if (paddedUriLen <= static_cast<unsigned int>(lastByte)) {
                        BALL_LOG_ERROR
                            << partitionDesc()
                            << "For QueueOp record of type [" << queueOpType
                            << "], the record present in QLIST file has "
                            << "invalid padding byte for 'QueueUri' field. "
                            << "Journal record offset: " << jit->recordOffset()
                            << ", journal record index: " << jit->recordIndex()
                            << ". QLIST record offset: " << queueUriRecOffset
                            << ". QLIST file size: " << qlistFd->fileSize()
                            << ". Padded URI length: " << paddedUriLen
                            << ", last byte value (as int): "
                            << static_cast<unsigned int>(lastByte) << ".";
                        return rc_INVALID_QLIST_RECORD;  // RETURN
                    }

                    bslstl::StringRef uri(uriBegin,
                                          paddedUriLen -
                                              uriBegin[paddedUriLen - 1]);

                    // Update 'qinfo'.  If queue was in fanout mode *and* a
                    // QueueOp.ADDITION record was encountered earlier in the
                    // backward iteration during 2nd pass, 'qinfo' will already
                    // contain the uri field.

                    if (!qinfo.canonicalQueueUri().empty() &&
                        qinfo.canonicalQueueUri() != uri) {
                        // Must have seen a QueueOp.ADDITION record earlier but
                        // uri *must* match.

                        MWCTSK_ALARMLOG_ALARM("RECOVERY")
                            << partitionDesc() << "Encountered a QueueOp ["
                            << queueOpType << "] record for queueKey ["
                            << queueKey << "], offset: " << jit->recordOffset()
                            << ", index: " << jit->recordIndex()
                            << ", with QueueUri mismatch. Expected URI ["
                            << qinfo.canonicalQueueUri()
                            << "], recovered URI [" << uri << "]."
                            << MWCTSK_ALARMLOG_END;
                        return rc_QUEUE_URI_MISMATCH;  // RETURN
                    }
                    else {
                        qinfo.setCanonicalQueueUri(uri);
                    }

                    BALL_LOG_INFO << partitionDesc() << "Recovered QueueOp ["
                                  << queueOpType << "] record for queue ["
                                  << qinfo.canonicalQueueUri()
                                  << "] with queue key [" << queueKey << "].";

                    // Retrieve AppId/AppKey pairs and add those pairs to
                    // 'qinfo' for which the appKey does not appear in
                    // 'deletedAppKeysOffsets'.

                    unsigned int appIdsAreaLen =
                        queueRecLength - queueRecHeaderLen - paddedUriLen -
                        FileStoreProtocol::k_HASH_LENGTH -
                        sizeof(unsigned int);  // Magic

                    MemoryBlock appIdsBlock(
                        qlistFd->block().base() + queueUriRecOffset +
                            queueRecHeaderLen + paddedUriLen +
                            FileStoreProtocol::k_HASH_LENGTH,
                        appIdsAreaLen);

                    AppIdKeyPairs appIdKeyPairs;
                    FileStoreProtocolUtil::loadAppIdKeyPairs(&appIdKeyPairs,
                                                             appIdsBlock,
                                                             numAppIds);

                    for (size_t n = 0; n < appIdKeyPairs.size(); ++n) {
                        const AppIdKeyPair& p = appIdKeyPairs[n];
                        if (0 == deletedAppKeysOffsets.count(p.second)) {
                            // This appKey is not deleted.  Add it to the list
                            // of 'alive' appId/appKey pairs for this queue.
                            // Note that we don't check for appId/appKey
                            // uniqueness here.  That check is done in
                            // StorageMgr because we have recovered all
                            // appId/appKey pairs by that time.

                            qinfo.addAppIdKeyPair(p);

                            BALL_LOG_INFO
                                << partitionDesc()
                                << "Recovered appId/appKey pair ['" << p.first
                                << "' (" << p.second << ")] in QueueOp ["
                                << queueOpType << "] record for queue ["
                                << qinfo.canonicalQueueUri()
                                << "] with queue key [" << queueKey << "].";
                        }
                    }
                }
                else if (qinfo.partitionId() != d_config.partitionId()) {
                    BALL_LOG_ERROR
                        << partitionDesc() << "Encountered a QueueOp ["
                        << queueOpType << "] record for queueKey [" << queueKey
                        << "], offset: " << jit->recordOffset()
                        << ", index: " << jit->recordIndex()
                        << ", in the FileStore of Partition ["
                        << d_config.partitionId() << "], but the cluster "
                        << "state indicates that the queue belongs to "
                        << "Partition [" << qinfo.partitionId() << "]";
                    return rc_INVALID_PARTITION_ID;  // RETURN
                }

                // Update 'd_records'.

                DataStoreRecordKey key(sequenceNum, primaryLeaseId);
                DataStoreRecord    record(RecordType::e_QUEUE_OP,
                                       jit->recordOffset(),
                                       queueRecLength);

                d_records.rinsert(bsl::make_pair(key, record));

                // Update outstanding JOURNAL and QLIST bytes.

                activeFileSet->d_outstandingBytesJournal +=
                    FileStoreProtocol::k_JOURNAL_RECORD_SIZE;
                if (needQList) {
                    activeFileSet->d_outstandingBytesQlist +=
                        record.d_dataOrQlistRecordPaddedLen;
                }
            }
        }
        else if (RecordType::e_DELETION == rt) {
            const DeletionRecord& rec = jit->asDeletionRecord();

            if (rec.messageGUID().isUnset()) {
                BALL_LOG_ERROR
                    << partitionDesc()
                    << "Encountered a DELETION record with unset guid. "
                    << "QueueKey [" << rec.queueKey() << "]. Journal record "
                    << "offset: " << jit->recordOffset()
                    << ", journal record index: " << jit->recordIndex() << ".";

                return rc_INVALID_DELETION_RECORD;  // RETURN
            }

            if (rec.queueKey().isNull()) {
                BALL_LOG_ERROR
                    << partitionDesc()
                    << "Encountered a DELETION record with null queueKey. "
                    << "GUID [" << rec.messageGUID() << "]. Journal record "
                    << "offset: " << jit->recordOffset()
                    << ", journal record index: " << jit->recordIndex() << ".";

                return rc_INVALID_DELETION_RECORD;  // RETURN
            }

            StorageKeysOffsetsConstIter queueIt = deletedQueueKeysOffsets.find(
                rec.queueKey());
            if (queueIt != deletedQueueKeysOffsets.end()) {
                BSLS_ASSERT_SAFE(jit->recordOffset() != queueIt->second);
                if (jit->recordOffset() < queueIt->second) {
                    // This DELETION record appears before
                    // QueueOpRecord.DELETION and thus should be ignored.

                    continue;  // CONTINUE
                }
            }

            if (1 == purgedQueueKeys.count(rec.queueKey())) {
                // Queue has been purged.  No need to keep track of deleted
                // guid.
                continue;  // CONTINUE
            }

            if (0 == queueKeyInfoMap->count(rec.queueKey())) {
                if (d_isFSMWorkflow) {
                    BALL_LOG_ERROR
                        << partitionDesc()
                        << "Encountered a DELETION record for "
                        << "queueKey [" << rec.queueKey()
                        << "], offset: " << jit->recordOffset()
                        << ", index: " << jit->recordIndex()
                        << ", but the queueKey is not present in cluster "
                        << "state.";
                    return rc_INVALID_QUEUE_KEY;  // RETURN
                }
                else {
                    MWCTSK_ALARMLOG_ALARM("RECOVERY")
                        << partitionDesc()
                        << "Encountered a DELETION record for "
                        << "queueKey [" << rec.queueKey()
                        << "], offset: " << jit->recordOffset()
                        << ", index: " << jit->recordIndex()
                        << ", for which a QueueOp.CREATION record was not "
                        << "seen in first pass." << MWCTSK_ALARMLOG_END;
                    return rc_INVALID_QUEUE_KEY;  // RETURN
                }
            }

            deletedGuids.insert(rec.messageGUID());

            // Not need to insert the deleted record in 'd_records'
        }
        else if (RecordType::e_CONFIRM == rt) {
            const ConfirmRecord& rec = jit->asConfirmRecord();

            if (rec.messageGUID().isUnset()) {
                BALL_LOG_ERROR
                    << partitionDesc()
                    << "Encountered a CONFIRM record with unset guid. "
                    << "QueueKey [" << rec.queueKey() << "]. Journal record "
                    << "offset: " << jit->recordOffset()
                    << ", journal record index: " << jit->recordIndex() << ".";

                return rc_INVALID_CONFIRM_RECORD;  // RETURN
            }

            if (rec.queueKey().isNull()) {
                BALL_LOG_ERROR
                    << partitionDesc()
                    << "Encountered a CONFIRM record with null queueKey. "
                    << "GUID [" << rec.messageGUID() << "]. Journal record "
                    << "offset: " << jit->recordOffset()
                    << ", journal record index: " << jit->recordIndex() << ".";

                return rc_INVALID_CONFIRM_RECORD;  // RETURN
            }

            StorageKeysOffsetsConstIter queueIt = deletedQueueKeysOffsets.find(
                rec.queueKey());
            if (queueIt != deletedQueueKeysOffsets.end()) {
                BSLS_ASSERT_SAFE(jit->recordOffset() != queueIt->second);
                if (jit->recordOffset() < queueIt->second) {
                    // This CONFIRM record appears before
                    // QueueOpRecord.DELETION and thus should be ignored.

                    continue;  // CONTINUE
                }
            }

            if (1 == purgedQueueKeys.count(rec.queueKey())) {
                // Queue has been purge.  No need to keep track of confirmed
                // guid.
                continue;  // CONTINUE
            }

            if (deletedGuids.find(rec.messageGUID()) != deletedGuids.end()) {
                // guid has already been deleted
                continue;  // CONTINUE
            }

            if (0 == queueKeyInfoMap->count(rec.queueKey())) {
                if (d_isFSMWorkflow) {
                    BALL_LOG_ERROR
                        << partitionDesc()
                        << "Encountered a CONFIRM record for queueKey ["
                        << rec.queueKey()
                        << "], offset: " << jit->recordOffset()
                        << ", index: " << jit->recordIndex()
                        << ", but the queueKey is not "
                        << "present in cluster state.";
                    return rc_INVALID_QUEUE_KEY;  // RETURN
                }
                else {
                    MWCTSK_ALARMLOG_ALARM("RECOVERY")
                        << partitionDesc()
                        << "Encountered a CONFIRM record for queueKey ["
                        << rec.queueKey()
                        << "], offset: " << jit->recordOffset()
                        << ", index: " << jit->recordIndex()
                        << ", for which a "
                        << "QueueOp.CREATION record was not seen in first "
                        << "pass." << MWCTSK_ALARMLOG_END;
                    return rc_INVALID_QUEUE_KEY;  // RETURN
                }
            }

            DataStoreRecordKey key(sequenceNum, primaryLeaseId);
            DataStoreRecord record(RecordType::e_CONFIRM, jit->recordOffset());
            d_records.rinsert(bsl::make_pair(key, record));

            // Update outstanding JOURNAL bytes.
            activeFileSet->d_outstandingBytesJournal +=
                FileStoreProtocol::k_JOURNAL_RECORD_SIZE;
        }
        else if (RecordType::e_MESSAGE == rt) {
            // Note that DATA file is touched only if it has been determined
            // that this message needs to be recovered.  This can speed up
            // recovery at startup.

            const MessageRecord& rec = jit->asMessageRecord();

            if (rec.messageGUID().isUnset()) {
                BALL_LOG_ERROR
                    << partitionDesc()
                    << "Encountered a MESSAGE record with unset guid. "
                    << "QueueKey [" << rec.queueKey() << "]. Journal record "
                    << "offset: " << jit->recordOffset()
                    << ", journal record index: " << jit->recordIndex() << ".";

                return rc_INVALID_MESSAGE_RECORD;  // RETURN
            }

            if (rec.queueKey().isNull()) {
                BALL_LOG_ERROR
                    << partitionDesc()
                    << "Encountered a MESSAGE record with null queueKey. "
                    << "GUID [" << rec.messageGUID() << "]. Journal record "
                    << "offset: " << jit->recordOffset()
                    << ", journal record index: " << jit->recordIndex() << ".";

                return rc_INVALID_MESSAGE_RECORD;  // RETURN
            }

            bsls::Types::Uint64 dataHeaderOffset =
                static_cast<bsls::Types::Uint64>(rec.messageOffsetDwords()) *
                bmqp::Protocol::k_DWORD_SIZE;

            if (0 == dataHeaderOffset) {
                BALL_LOG_ERROR
                    << partitionDesc()
                    << "Encountered a MESSAGE record with GUID ["
                    << rec.messageGUID() << "], queueKey [" << rec.queueKey()
                    << "], but invalid DATA file offset field. Journal record "
                    << "offset: " << jit->recordOffset()
                    << ", journal record index: " << jit->recordIndex() << ".";

                return rc_INVALID_DATA_OFFSET;  // RETURN
            }

            if (dataHeaderOffset > dataFd->fileSize()) {
                BALL_LOG_ERROR
                    << partitionDesc()
                    << "Encountered a MESSAGE record with GUID ["
                    << rec.messageGUID() << "], queueKey [" << rec.queueKey()
                    << "], but out-of-bound DATA file offset field: "
                    << dataHeaderOffset
                    << ", DATA file size: " << dataFd->fileSize()
                    << ". Journal record offset: " << jit->recordOffset()
                    << ", journal record index: " << jit->recordIndex() << ".";

                return rc_INVALID_DATA_OFFSET;  // RETURN
            }

            // Update 'dataOffset' if its the last message record (ie, first in
            // the iteration since we are iterating backwards).

            if (isLastMessageRecord) {
                OffsetPtr<const DataHeader> dataHeader(dataFd->block(),
                                                       dataHeaderOffset);
                const unsigned int totalLen = dataHeader->messageWords() *
                                              bmqp::Protocol::k_WORD_SIZE;

                *dataOffset         = dataHeaderOffset + totalLen;
                isLastMessageRecord = false;
            }

            StorageKeysOffsetsConstIter queueIt = deletedQueueKeysOffsets.find(
                rec.queueKey());
            if (queueIt != deletedQueueKeysOffsets.end()) {
                BSLS_ASSERT_SAFE(jit->recordOffset() != queueIt->second);
                if (jit->recordOffset() < queueIt->second) {
                    // This MESSAGE record appears before
                    // QueueOpRecord.DELETION and thus should be ignored.

                    continue;  // CONTINUE
                }
            }

            if (1 == purgedQueueKeys.count(rec.queueKey())) {
                // Queue has been purged.  No need to keep track of this
                // message.

                continue;  // CONTINUE
            }

            GuidsCIter delGuidIter = deletedGuids.find(rec.messageGUID());
            if (delGuidIter != deletedGuids.end()) {
                // Message has been deleted, no need to recover it.  We also
                // remove the guid from 'deletedGuids' container so as it keep
                // a check on that container's size and growth.

                deletedGuids.erase(delGuidIter);
                continue;  // CONTINUE
            }

            // The message needs to be recovered as it is an outstanding one.
            // Check various fields in the message header etc as well as the
            // CRC32C.

            OffsetPtr<const DataHeader> dataHeader(dataFd->block(),
                                                   dataHeaderOffset);

            const unsigned int headerSize = dataHeader->headerWords() *
                                            bmqp::Protocol::k_WORD_SIZE;
            const unsigned int optionsSize = dataHeader->optionsWords() *
                                             bmqp::Protocol::k_WORD_SIZE;
            const unsigned int totalLen = dataHeader->messageWords() *
                                          bmqp::Protocol::k_WORD_SIZE;

            if (0 == headerSize) {
                BALL_LOG_ERROR
                    << partitionDesc() << "MESSAGE record with GUID ["
                    << rec.messageGUID() << "], queueKey [" << rec.queueKey()
                    << "] at journal offset: " << jit->recordOffset()
                    << ", the DATA record present at offset: "
                    << dataHeaderOffset << " in the DATA file has invalid "
                    << "'headerWords' field in its 'DataHeader'.";

                return rc_INVALID_DATA_RECORD;  // RETURN
            }

            if (0 == totalLen) {
                BALL_LOG_ERROR
                    << partitionDesc() << "MESSAGE record with GUID ["
                    << rec.messageGUID() << "], queueKey [" << rec.queueKey()
                    << "] at journal offset: " << jit->recordOffset()
                    << ", the DATA record present at offset: "
                    << dataHeaderOffset
                    << " in the DATA file has invalid 'messageWords' field in "
                    << "its 'DataHeader'.";

                return rc_INVALID_DATA_RECORD;  // RETURN
            }

            if ((headerSize + optionsSize) >= totalLen) {
                BALL_LOG_ERROR
                    << partitionDesc() << "MESSAGE record with GUID ["
                    << rec.messageGUID() << "], queueKey [" << rec.queueKey()
                    << "] at journal offset: " << jit->recordOffset()
                    << ", the DATA record present at offset: "
                    << dataHeaderOffset
                    << " in the DATA file has invalid 'headerWords', "
                    << "optionsWords and/or 'messageWords' fields in its "
                    << "'DataHeader'. headerSize: " << headerSize
                    << ", optionsSize: " << optionsSize
                    << ", total length: " << totalLen << ".";

                return rc_INVALID_DATA_RECORD;  // RETURN
            }

            const char* begin    = dataFd->block().base() + dataHeaderOffset;
            const int   lastByte = static_cast<int>(begin[totalLen - 1]);

            if (lastByte < 1 || lastByte > bmqp::Protocol::k_DWORD_SIZE) {
                BALL_LOG_ERROR
                    << partitionDesc() << "MESSAGE record with GUID ["
                    << rec.messageGUID() << "], queueKey [" << rec.queueKey()
                    << "] at journal offset: " << jit->recordOffset()
                    << ", the DATA record present at offset: "
                    << dataHeaderOffset
                    << " in the DATA file has invalid padding: " << lastByte
                    << ".";

                return rc_INVALID_DATA_RECORD;  // RETURN
            }

            if (totalLen < (headerSize + optionsSize + lastByte)) {
                BALL_LOG_ERROR
                    << partitionDesc() << "MESSAGE record with GUID ["
                    << rec.messageGUID() << "], queueKey [" << rec.queueKey()
                    << "] at journal offset: " << jit->recordOffset()
                    << ", the DATA record present at offset: "
                    << dataHeaderOffset
                    << " in the DATA file has invalid messageWords/headerWords"
                    << "/optionsWords/padding fields -- " << totalLen << "/"
                    << headerSize << "/" << optionsSize << "/" << lastByte
                    << ".";

                return rc_INVALID_DATA_RECORD;  // RETURN
            }

            if (0 == queueKeyInfoMap->count(rec.queueKey())) {
                if (d_isFSMWorkflow) {
                    BALL_LOG_ERROR
                        << partitionDesc()
                        << "Encountered a MESSAGE record for queueKey ["
                        << rec.queueKey()
                        << "], offset: " << jit->recordOffset()
                        << ", index: " << jit->recordIndex()
                        << ", but the queueKey is not "
                        << "present in cluster state.";
                    return rc_INVALID_QUEUE_KEY;  // RETURN
                }
                else {
                    MWCTSK_ALARMLOG_ALARM("RECOVERY")
                        << partitionDesc()
                        << "Encountered a MESSAGE record for queueKey ["
                        << rec.queueKey()
                        << "], offset: " << jit->recordOffset()
                        << ", index: " << jit->recordIndex()
                        << ", for which a "
                        << "QueueOp.CREATION record was not seen in first "
                        << "pass." << MWCTSK_ALARMLOG_END;
                    return rc_INVALID_QUEUE_KEY;  // RETURN
                }
            }

            bsls::Types::Uint64 appDataOffset = dataHeaderOffset + headerSize +
                                                optionsSize;
            unsigned int appDataLen = totalLen - headerSize - optionsSize -
                                      lastByte;

            if (!d_ignoreCrc32c) {
                // Check CRC32C
                unsigned int checksum = bmqp::Crc32c::calculate(
                    dataFd->block().base() + appDataOffset,
                    appDataLen);

                if (rec.crc32c() != checksum) {
                    MWCTSK_ALARMLOG_ALARM("RECOVERY")
                        << partitionDesc()
                        << "Recovery: CRC mismatch for guid ["
                        << rec.messageGUID() << "] for queueKey ["
                        << rec.queueKey() << "] in journal file ["
                        << activeFileSet->d_journalFileName
                        << "], offset: " << jit->recordOffset()
                        << ", index: " << jit->recordIndex()
                        << ". CRC32-C in JOURNAL record: " << rec.crc32c()
                        << ". CRC32-C of payload in DATA file: " << checksum
                        << ". Payload offset in DATA file: " << appDataOffset
                        << MWCTSK_ALARMLOG_END;
                    continue;  // CONTINUE
                }
            }

            DataStoreRecordKey key(sequenceNum, primaryLeaseId);
            DataStoreRecord record(RecordType::e_MESSAGE, jit->recordOffset());
            record.d_messageOffset              = dataHeaderOffset;
            record.d_appDataUnpaddedLen         = appDataLen;
            record.d_dataOrQlistRecordPaddedLen = totalLen;
            record.d_hasReceipt                 = true;
            record.d_arrivalTimestamp           = recHeader.timestamp();
            record.d_messagePropertiesInfo      = bmqp::MessagePropertiesInfo(
                *dataHeader);

            // Update in-memory record mapping.
            d_records.rinsert(bsl::make_pair(key, record));

            // Update outstanding JOURNAL and DATA bytes.

            activeFileSet->d_outstandingBytesJournal +=
                FileStoreProtocol::k_JOURNAL_RECORD_SIZE;
            activeFileSet->d_outstandingBytesData += totalLen;
        }
    }

    BALL_LOG_INFO << partitionDesc() << "Completed second pass over the "
                  << "journal with rc: " << rc;

    return rc_SUCCESS;
}

int FileStore::create(FileSetSp* fileSetSp)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(fileSetSp);

    mwcu::MemOutStream errorDesc;
    return FileStoreUtil::create(errorDesc,
                                 fileSetSp,
                                 this,
                                 d_config.partitionId(),
                                 d_config,
                                 partitionDesc(),
                                 !d_isFSMWorkflow,
                                 d_allocator_p);
}

int FileStore::rollover(bsls::Types::Uint64 timestamp)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(0 < d_fileSets.size());

    FileSet* activeFileSet = d_fileSets[0].get();
    BSLS_ASSERT_SAFE(activeFileSet);

    BALL_LOG_INFO_BLOCK
    {
        BALL_LOG_OUTPUT_STREAM
            << partitionDesc() << "Initiating rollover for data file ["
            << activeFileSet->d_dataFileName << "], journal file ["
            << activeFileSet->d_journalFileName << "]";
        if (!d_isFSMWorkflow) {
            BALL_LOG_OUTPUT_STREAM << ", qlist file ["
                                   << activeFileSet->d_qlistFileName << "]";
        }
    }

    // Start a StatMonitorSnapshotRecorder to track system stats during
    // rollover
    mwcsys::StatMonitorSnapshotRecorder statRecorder(partitionDesc(),
                                                     d_allocator_p);

    // Create new files, add header etc.
    FileSetSp newActiveFileSetSp;
    int       rc = create(&newActiveFileSetSp);
    if (0 != rc) {
        // 'create' will log error
        return rc;  // RETURN
    }

    // Iterate over outstanding records in the active set, and copy them to the
    // rollover set.

    QueueKeyCounterMap queueKeyCounterMap;
    for (RecordIterator recordIt = d_records.begin();
         recordIt != d_records.end();
         ++recordIt) {
        writeRolledOverRecord(&(recordIt->second),
                              &queueKeyCounterMap,
                              activeFileSet,
                              newActiveFileSetSp.get());
    }

    // Print summary of rolled over queues.
    mwcu::MemOutStream outStream;
    outStream << partitionDesc() << "Queue rollover summary:"
              << "\n      QueueKey    NumMsgs   NumBytes      QueueUri";

    QueueKeyCounterList queueKeyCounters;
    queueKeyCounters.reserve(queueKeyCounterMap.size());
    for (QueueKeyCounterMapCIter queueKeyCounterCIter =
             queueKeyCounterMap.cbegin();
         queueKeyCounterCIter != queueKeyCounterMap.cend();
         ++queueKeyCounterCIter) {
        queueKeyCounters.push_back(*queueKeyCounterCIter);
    }
    bsl::sort(queueKeyCounters.begin(), queueKeyCounters.end(), compareByByte);

    for (QueueKeyCounterListCIter queueCountersCIter =
             queueKeyCounters.cbegin();
         queueCountersCIter != queueKeyCounters.cend();
         ++queueCountersCIter) {
        StorageMapConstIter sit = d_storages.find(queueCountersCIter->first);
        BSLS_ASSERT_SAFE(sit != d_storages.cend());

        outStream << "\n    [" << queueCountersCIter->first << "] "
                  << bsl::setw(8)
                  << mwcu::PrintUtil::prettyNumber(
                         static_cast<int>(queueCountersCIter->second.first))
                  << " " << bsl::setw(10)
                  << mwcu::PrintUtil::prettyBytes(
                         queueCountersCIter->second.second)
                  << " " << sit->second->queueUri();
    }
    BALL_LOG_INFO << outStream.str();

    // Local refs for convenience.

    MappedFileDescriptor& rJournalFile = newActiveFileSetSp->d_journalFile;
    bsls::Types::Uint64&  rJournalFilePos =
        newActiveFileSetSp->d_journalFilePosition;

    bmqp_ctrlmsg::SyncPointOffsetPair spoPair;
    bmqp_ctrlmsg::SyncPoint&          syncPoint = spoPair.syncPoint();

    spoPair.offset() = rJournalFilePos;
    BSLS_ASSERT_SAFE(0 == spoPair.offset() % bmqp::Protocol::k_WORD_SIZE);

    // Since we are rolling over the partition, there must be at least one sync
    // point, because rollover is always initiated by writing (if primary) or
    // receiving (if replica) a SyncPt of e_ROLLOVER sub-type.

    BSLS_ASSERT_SAFE(!d_syncPoints.empty());
    BSLS_ASSERT_SAFE(d_syncPoints.back().offset() <=
                     activeFileSet->d_journalFilePosition);

    syncPoint =
        d_syncPoints.back().syncPoint();  // Make a copy to update later

    OffsetPtr<const JournalOpRecord> journalOpRec(
        activeFileSet->d_journalFile.block(),
        d_syncPoints.back().offset());

    BSLS_ASSERT_SAFE(JournalOpType::e_SYNCPOINT == journalOpRec->type());
    BSLS_ASSERT_SAFE(syncPoint.sequenceNum() == journalOpRec->sequenceNum());
    BSLS_ASSERT_SAFE(syncPoint.primaryLeaseId() ==
                     journalOpRec->primaryLeaseId());
    BSLS_ASSERT_SAFE(syncPoint.dataFileOffsetDwords() ==
                     journalOpRec->dataFileOffsetDwords());
    if (!d_isFSMWorkflow) {
        BSLS_ASSERT_SAFE(syncPoint.qlistFileOffsetWords() ==
                         journalOpRec->qlistFileOffsetWords());
    }
    BSLS_ASSERT_SAFE(SyncPointType::e_UNDEFINED !=
                     journalOpRec->syncPointType());

    // Note that we don't use the 'dataFileOffset' of the last sync point, but
    // instead use the position of new data file.  Same argument for
    // 'qlistFileOffset'.

    syncPoint.dataFileOffsetDwords() = newActiveFileSetSp->d_dataFilePosition /
                                       bmqp::Protocol::k_DWORD_SIZE;
    if (!d_isFSMWorkflow) {
        syncPoint.qlistFileOffsetWords() =
            newActiveFileSetSp->d_qlistFilePosition /
            bmqp::Protocol::k_WORD_SIZE;
    }

    // Write the first SyncPt in the new JOURNAL.  This SyncPt will contain
    // updated DATA and QLIST file offsets, but same (leaseId, seqNum) as the
    // last SyncPt in the old JOURNAL.  Also note that the RecordHeader of this
    // first SyncPt will contain the *same* (leaseId, seqNum) as those
    // appearing in the RecordHeader of the last SyncPt in the old JOURNAL.

    // Also note that we cannot use 'd_primaryLeaseId' and 'd_sequenceNum' to
    // populate (leaseId, seqnum) fields of the RecordHeader of the first
    // SyncPt in the new JOURNAL, because in case of replicas, those 2
    // variables are updated at the end of processing the storage event, but
    // this routine is invoked *while* processing the storage event.

    OffsetPtr<JournalOpRecord> spRec(rJournalFile.block(), rJournalFilePos);
    new (spRec.get())
        JournalOpRecord(JournalOpType::e_SYNCPOINT,
                        SyncPointType::e_REGULAR,  // 'regular' sync point
                        syncPoint.sequenceNum(),
                        journalOpRec->primaryNodeId(),
                        syncPoint.primaryLeaseId(),
                        syncPoint.dataFileOffsetDwords(),
                        d_isFSMWorkflow ? 0 : syncPoint.qlistFileOffsetWords(),
                        RecordHeader::k_MAGIC);
    spRec->header()
        .setPrimaryLeaseId(journalOpRec->header().primaryLeaseId())
        .setSequenceNumber(journalOpRec->header().sequenceNumber())
        .setTimestamp(timestamp);
    rJournalFilePos += FileStoreProtocol::k_JOURNAL_RECORD_SIZE;

    // Update first sync point of JournalFileHeader of new active file set with
    // 'syncPointOffset'.  A non-zero JournalFileHeader.d_firstSyncPointOffset
    // implies that rollover was successfully finished (this may help during
    // recovery after crash) ** NOTE ** Updating
    // JournalFileHeader.d_firstSyncPointOffset must be the last operation to
    // occur in rolling over file store.

    OffsetPtr<const FileHeader>  fhJ(rJournalFile.block(), 0);
    OffsetPtr<JournalFileHeader> jfh(rJournalFile.block(),
                                     fhJ->headerWords() *
                                         bmqp::Protocol::k_WORD_SIZE);

    jfh->setFirstSyncPointOffsetWords(spoPair.offset() /
                                      bmqp::Protocol::k_WORD_SIZE);

    // Now clear the 'd_syncPoints' as the rollover is complete, and make the
    // previous newest sync point the first new sync point.

    d_syncPoints.clear();
    d_syncPoints.push_back(spoPair);

    // No need to update outstanding bytes for the journal belonging to the new
    // file set, since we don't rollover SyncPts.

    // Print some rollover-related metrics.

    mwcu::MemOutStream out;
    out << partitionDesc() << "Compaction metrics: \n"
        << "    DATA file size    (new/old): "
        << mwcu::PrintUtil::prettyBytes(newActiveFileSetSp->d_dataFilePosition)
        << "/"
        << mwcu::PrintUtil::prettyBytes(activeFileSet->d_dataFilePosition)
        << " ("
        << ((newActiveFileSetSp->d_dataFilePosition * 100) /
            activeFileSet->d_dataFilePosition)
        << "%)\n";

    out << "    JOURNAL file size (new/old): "
        << mwcu::PrintUtil::prettyBytes(
               newActiveFileSetSp->d_journalFilePosition)
        << "/"
        << mwcu::PrintUtil::prettyBytes(activeFileSet->d_journalFilePosition)
        << " ("
        << ((newActiveFileSetSp->d_journalFilePosition * 100) /
            activeFileSet->d_journalFilePosition)
        << "%)\n";

    if (!d_isFSMWorkflow) {
        out << "    QLIST file size   (new/old): "
            << mwcu::PrintUtil::prettyBytes(
                   newActiveFileSetSp->d_qlistFilePosition)
            << "/"
            << mwcu::PrintUtil::prettyBytes(activeFileSet->d_qlistFilePosition)
            << " ("
            << ((newActiveFileSetSp->d_qlistFilePosition * 100) /
                activeFileSet->d_qlistFilePosition)
            << "%)";
    }
    BALL_LOG_INFO << out.str();

    BALL_LOG_INFO_BLOCK
    {
        statRecorder.print(BALL_LOG_OUTPUT_STREAM,
                           "ROLLOVER - STEP 1 (COMPACTION)");
    }

    // Decrement the old file set's aliased blob buffer count by 1 to obtain
    // the 'real' value.  Note that count was initialized with a value of 1
    // instead of 0 when file set was created.  If count is 0 (ie, if no
    // payload blob buffers are referring to the old data file), close it out
    // too.  Old file set can be archived as well after that.

    // Irrespective of the aliased blob buffer counter, file set can be
    // truncated because nothing else will be written to the file.

    truncate(activeFileSet);
    BALL_LOG_INFO_BLOCK
    {
        statRecorder.print(BALL_LOG_OUTPUT_STREAM,
                           "ROLLOVER - STEP 2 (TRUNCATE)");
    }

    if (0 == --activeFileSet->d_aliasedBlobBufferCount) {
        BALL_LOG_INFO_BLOCK
        {
            BALL_LOG_OUTPUT_STREAM << partitionDesc()
                                   << "During rollover, closing old file set ["
                                   << activeFileSet->d_dataFileName << "], ["
                                   << activeFileSet->d_journalFileName << "]";
            if (!d_isFSMWorkflow) {
                BALL_LOG_OUTPUT_STREAM
                    << ", [" << activeFileSet->d_qlistFileName << "]";
            }
            BALL_LOG_OUTPUT_STREAM << " as it can be gc'd.";
        }

        // Garbage-collect 'activeFileSet' from 'd_fileSets'.  We know that it
        // is the first element.
        BSLS_ASSERT_SAFE(d_fileSets.begin()->get() == activeFileSet);

        // Make a copy of the target fileSetSp before erasing it from
        // 'd_fileSets'.
        bsl::shared_ptr<FileSet> activeFileSetSp(*d_fileSets.begin());
        d_fileSets.erase(d_fileSets.begin());

        rc = d_miscWorkThreadPool_p->enqueueJob(
            bdlf::BindUtil::bind(&FileStore::gcWorkerDispatched,
                                 this,
                                 activeFileSetSp));
        BSLS_ASSERT_SAFE(rc == 0);
    }
    else {
        BALL_LOG_INFO << partitionDesc() << "Rollover: number of references to"
                      << " old file set: "
                      << activeFileSet->d_aliasedBlobBufferCount;
        BSLS_ASSERT_SAFE(activeFileSet->d_dataFile.isValid());
        BSLS_ASSERT_SAFE(activeFileSet->d_journalFile.isValid());
        if (!d_isFSMWorkflow) {
            BSLS_ASSERT_SAFE(activeFileSet->d_qlistFile.isValid());
        }
    }

    // Add 'newActiveFileSetSp' as the first element of 'd_fileSets'.
    d_fileSets.insert(d_fileSets.begin(), newActiveFileSetSp);

    BALL_LOG_INFO_BLOCK
    {
        BALL_LOG_OUTPUT_STREAM << partitionDesc()
                               << "All data files and alias counts: \n";
        for (unsigned int index = 0; index < d_fileSets.size(); ++index) {
            const FileSet*     fs    = d_fileSets[index].get();
            bsls::Types::Int64 count = fs->d_aliasedBlobBufferCount;
            BALL_LOG_OUTPUT_STREAM
                << fs->d_dataFileName
                << " : alias count: " << (0 == index ? (count - 1) : count)
                << "\n";
        }
    }

    // Enqueue an event in the scheduler thread to be executed right away to
    // delete any archived files for this partition, if applicable.  Note that
    // one such recurring event is also scheduled by the StorageMgr, and
    // processed in the event scheduler's dispatcher thread.  So this event
    // should also be processed in the same event scheduler's dispatcher thread
    // to avoid any race while deleting archived files.

    d_config.scheduler()->scheduleEvent(
        mwcsys::Time::nowMonotonicClock(),
        bdlf::BindUtil::bind(&FileStore::deleteArchiveFilesCb, this));

    BALL_LOG_INFO_BLOCK
    {
        statRecorder.print(BALL_LOG_OUTPUT_STREAM, "ROLLOVER COMPLETE");
    }

    d_clusterStats_p->onPartitionEvent(
        mqbstat::ClusterStats::PartitionEventType::e_PARTITION_ROLLOVER,
        d_config.partitionId(),
        statRecorder.totalElapsed());

    return 0;
}

int FileStore::rolloverIfNeeded(FileType::Enum              fileType,
                                const MappedFileDescriptor& file,
                                const bsl::string&          fileName,
                                bsls::Types::Uint64         currentSize,
                                unsigned int                requestedSpace)
{
    FileSet* activeFileSet = d_fileSets[0].get();
    BSLS_ASSERT_SAFE(activeFileSet);

    BSLS_ASSERT_SAFE(activeFileSet->d_journalFileAvailable);
    // If JOURNAL is full, it must be marked so, and this routine should
    // not be invoked.

    enum {
        rc_SUCCESS                         = 0,
        rc_JOURNAL_ROLLOVER_POLICY_FAILURE = -1,
        rc_DATA_ROLLOVER_POLICY_FAILURE    = -2,
        rc_QLIST_ROLLOVER_POLICY_FAILURE   = -3,
        rc_SYNC_POINT_FAILURE              = -4,
        rc_ROLLOVER_FAILURE                = -5,
        rc_SYNC_POINT_FORCE_ISSUE_FAILURE  = -6
    };

    // QList file is deprecated in FSM workflow.
    if (d_isFSMWorkflow && FileType::e_QLIST == fileType) {
        return rc_SUCCESS;  // RETURN
    }

    if (!needRollover(file, currentSize, requestedSpace)) {
        return rc_SUCCESS;  // RETURN
    }

    // TBD: make the ratio configurable
    static const bsls::Types::Uint64 k_MIN_AVAILABLE_SPACE_PERCENT = 20;

    // Rollover is needed.  If JOURNAL file requested rollover, mark it as
    // unavailable.  This is needed in the scenario when this routine fails,
    // we want to disable this partition, and we use 'd_journalFileAvailable'
    // flag for that.

    if (FileType::e_JOURNAL == fileType) {
        activeFileSet->d_journalFileAvailable = false;
    }

    mwcu::MemOutStream out;
    out << partitionDesc() << "Rollover is required due to " << fileType
        << " file [" << fileName << "] reaching capacity. Current size: "
        << mwcu::PrintUtil::prettyNumber(
               static_cast<bsls::Types::Int64>(currentSize))
        << ", capacity: "
        << mwcu::PrintUtil::prettyNumber(
               static_cast<bsls::Types::Int64>(file.fileSize()))
        << ". Checking if JOURNAL" << (d_isFSMWorkflow ? "" : ", QLIST")
        << " and DATA files satisfy "
        << "rollover criteria. Minimum required available space in each file: "
        << "[" << k_MIN_AVAILABLE_SPACE_PERCENT << "%]. "
        << "Requested record length: " << requestedSpace
        << ". Outstanding bytes in each file:"
        << "\nJOURNAL: "
        << mwcu::PrintUtil::prettyNumber(static_cast<bsls::Types::Int64>(
               activeFileSet->d_outstandingBytesJournal))
        << "\nDATA: "
        << mwcu::PrintUtil::prettyNumber(static_cast<bsls::Types::Int64>(
               activeFileSet->d_outstandingBytesData));
    if (!d_isFSMWorkflow) {
        out << "\nQLIST: "
            << mwcu::PrintUtil::prettyNumber(static_cast<bsls::Types::Int64>(
                   activeFileSet->d_outstandingBytesQlist));
    }

    // All 3 files must satisfy the rollover policy before we can initiate the
    // rollover.  Note that we also add the 'requestedSpace' in the
    // outstanding bytes when calculating ratio for the 'fileType'.

    bool           canRollover            = true;
    int            rc                     = rc_SUCCESS;
    FileType::Enum cannotRolloverFileType = FileType::e_UNDEFINED;

    bsls::Types::Uint64 outstandingBytesJournal =
        activeFileSet->d_outstandingBytesJournal;
    if (FileType::e_JOURNAL == fileType) {
        outstandingBytesJournal += requestedSpace;
    }

    bsls::Types::Uint64 availableSpacePercentJournal = 0;
    if (outstandingBytesJournal <= d_config.maxJournalFileSize()) {
        availableSpacePercentJournal =
            ((d_config.maxJournalFileSize() - outstandingBytesJournal) * 100) /
            d_config.maxJournalFileSize();
    }

    if (availableSpacePercentJournal < k_MIN_AVAILABLE_SPACE_PERCENT) {
        // JOURNAL file can't be rolled over.

        canRollover            = false;
        cannotRolloverFileType = FileType::e_JOURNAL;
        rc                     = rc_JOURNAL_ROLLOVER_POLICY_FAILURE;
    }

    bsls::Types::Uint64 outstandingBytesData =
        activeFileSet->d_outstandingBytesData;
    if (FileType::e_DATA == fileType) {
        outstandingBytesData += requestedSpace;
    }

    bsls::Types::Uint64 availableSpacePercentData = 0;
    if (outstandingBytesData <= d_config.maxDataFileSize()) {
        availableSpacePercentData =
            ((d_config.maxDataFileSize() - outstandingBytesData) * 100) /
            d_config.maxDataFileSize();
    }

    if (availableSpacePercentData < k_MIN_AVAILABLE_SPACE_PERCENT) {
        // DATA file can't be rolled over.

        canRollover            = false;
        cannotRolloverFileType = FileType::e_DATA;
        rc                     = rc_DATA_ROLLOVER_POLICY_FAILURE;
    }

    bsls::Types::Uint64 availableSpacePercentQlist = 0;
    if (!d_isFSMWorkflow) {
        bsls::Types::Uint64 outstandingBytesQlist =
            activeFileSet->d_outstandingBytesQlist;
        if (FileType::e_QLIST == fileType) {
            outstandingBytesQlist += requestedSpace;
        }

        if (outstandingBytesQlist <= d_config.maxQlistFileSize()) {
            availableSpacePercentQlist =
                ((d_config.maxQlistFileSize() - outstandingBytesQlist) * 100) /
                d_config.maxQlistFileSize();
        }

        if (availableSpacePercentQlist < k_MIN_AVAILABLE_SPACE_PERCENT) {
            // QLIST file can't be rolled over.  Alarm only if we have
            // encountered this for the first time.

            canRollover            = false;
            cannotRolloverFileType = FileType::e_QLIST;
            rc                     = rc_QLIST_ROLLOVER_POLICY_FAILURE;
        }
    }

    if (!canRollover) {
        BSLS_ASSERT_SAFE(FileType::e_UNDEFINED != cannotRolloverFileType);

        if (activeFileSet->d_fileSetRolloverPolicyAlarm) {
            // Already alarmed.  No need to alarm.
            return rc;  // RETURN
        }

        unsigned int availableSpacePercent = 0;
        if (FileType::e_JOURNAL == fileType) {
            availableSpacePercent = availableSpacePercentJournal;
        }
        else if (FileType::e_QLIST == fileType) {
            BSLS_ASSERT_SAFE(!d_isFSMWorkflow);
            availableSpacePercent = availableSpacePercentQlist;
        }
        else {
            availableSpacePercent = availableSpacePercentData;
        }

        out.reset();
        out << partitionDesc() << "[" << fileType
            << "] file is full but partition cannot be "
            << "rolled over because rolled over [" << cannotRolloverFileType
            << "] file will have [" << availableSpacePercent
            << "%] available space which is lower than the configured value "
            << "of [" << k_MIN_AVAILABLE_SPACE_PERCENT << "%]. "
            << "Outstading bytes of each file:";
        bdlb::Print::newlineAndIndent(out, 1, 4);
        printSpaceInUse(
            out,
            "JOURNAL",
            activeFileSet->d_outstandingBytesJournal,
            d_config.maxJournalFileSize(),
            computePercentage(activeFileSet->d_outstandingBytesJournal,
                              d_config.maxJournalFileSize()));
        bdlb::Print::newlineAndIndent(out, 1, 4);
        printSpaceInUse(
            out,
            "DATA",
            activeFileSet->d_outstandingBytesData,
            d_config.maxDataFileSize(),
            computePercentage(activeFileSet->d_outstandingBytesData,
                              d_config.maxDataFileSize()));
        if (d_isFSMWorkflow) {
            bdlb::Print::newlineAndIndent(out, 1, 4);
            printSpaceInUse(
                out,
                "QLIST",
                activeFileSet->d_outstandingBytesQlist,
                d_config.maxQlistFileSize(),
                computePercentage(activeFileSet->d_outstandingBytesQlist,
                                  d_config.maxQlistFileSize()));
        }
        out << "\n";

        // Print the top 10 queues contributing to the rollover failure
        printTopContributingQueues(out,
                                   10,
                                   "rollover failure",
                                   cannotRolloverFileType,
                                   d_storages);

        activeFileSet->d_fileSetRolloverPolicyAlarm = true;

        MWCTSK_ALARMLOG_PANIC("PARTITION_READONLY")
            << out.str() << MWCTSK_ALARMLOG_END;

        return rc;  // RETURN
    }

    BALL_LOG_INFO << partitionDesc() << "Rollover criteria met. Initiating "
                  << "rollover.";

    activeFileSet->d_journalFileAvailable = true;
    // Set the availability flag back to true.

    // Issue sync point first.

    bmqp_ctrlmsg::SyncPoint syncPt;
    syncPt.primaryLeaseId()       = d_primaryLeaseId;
    syncPt.sequenceNum()          = ++d_sequenceNum;
    syncPt.dataFileOffsetDwords() = activeFileSet->d_dataFilePosition /
                                    bmqp::Protocol::k_DWORD_SIZE;
    if (!d_isFSMWorkflow) {
        syncPt.qlistFileOffsetWords() = activeFileSet->d_qlistFilePosition /
                                        bmqp::Protocol::k_WORD_SIZE;
    }

    rc = issueSyncPointInternal(SyncPointType::e_ROLLOVER, true, &syncPt);
    if (0 != rc) {
        return 10 * rc + rc_SYNC_POINT_FAILURE;  // RETURN
    }

    BALL_LOG_INFO << partitionDesc() << "Issued SyncPt: " << syncPt
                  << ", with journal offset: "
                  << (activeFileSet->d_journalFilePosition -
                      FileStoreProtocol::k_JOURNAL_RECORD_SIZE)
                  << ", to indicate partition rollover.";

    // Then initiate rollover.

    rc = rollover(bdlt::EpochUtil::convertToTimeT64(bdlt::CurrentTime::utc()));
    if (0 != rc) {
        return 10 * rc + rc_ROLLOVER_FAILURE;  // RETURN
    }

    // Force-issue a SyncPt after rolling over.

    rc = issueSyncPoint();
    if (0 != rc) {
        return 10 * rc + rc_SYNC_POINT_FORCE_ISSUE_FAILURE;  // RETURN
    }

    return rc_SUCCESS;
}

void FileStore::truncate(FileSet* fileSet)
{
    mwcu::MemOutStream errorDesc;

    int rc = FileSystemUtil::truncate(&fileSet->d_dataFile,
                                      fileSet->d_dataFilePosition,
                                      errorDesc);
    if (0 != rc) {
        MWCTSK_ALARMLOG_ALARM("FILE_IO")
            << "[FILE_IO] " << partitionDesc()
            << "Failed to truncate data file [" << fileSet->d_dataFileName
            << "], rc: " << rc << ", error: " << errorDesc.str()
            << MWCTSK_ALARMLOG_END;
    }

    rc = FileSystemUtil::truncate(&fileSet->d_journalFile,
                                  fileSet->d_journalFilePosition,
                                  errorDesc);
    if (0 != rc) {
        MWCTSK_ALARMLOG_ALARM("FILE_IO")
            << partitionDesc() << "Failed to truncate journal ["
            << fileSet->d_journalFileName << "], rc: " << rc
            << ", error: " << errorDesc.str() << MWCTSK_ALARMLOG_END;
    }

    if (!d_isFSMWorkflow) {
        rc = FileSystemUtil::truncate(&fileSet->d_qlistFile,
                                      fileSet->d_qlistFilePosition,
                                      errorDesc);
        if (0 != rc) {
            MWCTSK_ALARMLOG_ALARM("FILE_IO")
                << partitionDesc() << "Failed to truncate qlist file ["
                << fileSet->d_qlistFileName << "], rc: " << rc
                << ", error: " << errorDesc.str() << MWCTSK_ALARMLOG_END;
        }
    }
}

void FileStore::close(FileSet& fileSetRef, bool flush)
{
    if (flush) {
        BALL_LOG_INFO << partitionDesc() << "Flushing partition to disk.";

        mwcu::MemOutStream errorDesc;
        int rc = FileSystemUtil::flush(fileSetRef.d_dataFile.mapping(),
                                       fileSetRef.d_dataFilePosition,
                                       errorDesc);
        if (0 != rc) {
            MWCTSK_ALARMLOG_ALARM("FILE_IO")
                << partitionDesc() << "Failed to flush data file ["
                << fileSetRef.d_dataFileName << "], error: " << errorDesc.str()
                << MWCTSK_ALARMLOG_END;
            errorDesc.reset();
        }

        rc = FileSystemUtil::flush(fileSetRef.d_journalFile.mapping(),
                                   fileSetRef.d_journalFilePosition,
                                   errorDesc);
        if (0 != rc) {
            MWCTSK_ALARMLOG_ALARM("FILE_IO")
                << partitionDesc() << "Failed to sync journal file ["
                << fileSetRef.d_journalFileName
                << "], error: " << errorDesc.str() << MWCTSK_ALARMLOG_END;
            errorDesc.reset();
        }

        if (!d_isFSMWorkflow) {
            rc = FileSystemUtil::flush(fileSetRef.d_qlistFile.mapping(),
                                       fileSetRef.d_qlistFilePosition,
                                       errorDesc);
            if (0 != rc) {
                MWCTSK_ALARMLOG_ALARM("FILE_IO")
                    << partitionDesc() << "Failed to sync qlist file ["
                    << fileSetRef.d_qlistFileName
                    << "], error: " << errorDesc.str() << MWCTSK_ALARMLOG_END;
                errorDesc.reset();
            }
        }

        BALL_LOG_INFO << partitionDesc() << "Flushed partition to disk.";
    }

    int rc = FileSystemUtil::close(&fileSetRef.d_dataFile);
    if (0 != rc) {
        MWCTSK_ALARMLOG_ALARM("FILE_IO")
            << partitionDesc() << "Failed to close data file ["
            << fileSetRef.d_dataFileName << "], rc: " << rc
            << MWCTSK_ALARMLOG_END;
    }

    rc = FileSystemUtil::close(&fileSetRef.d_journalFile);
    if (0 != rc) {
        MWCTSK_ALARMLOG_ALARM("FILE_IO")
            << partitionDesc() << "Failed to close journal file ["
            << fileSetRef.d_journalFileName << "], rc: " << rc
            << MWCTSK_ALARMLOG_END;
    }

    if (!d_isFSMWorkflow) {
        rc = FileSystemUtil::close(&fileSetRef.d_qlistFile);
        if (0 != rc) {
            MWCTSK_ALARMLOG_ALARM("FILE_IO")
                << partitionDesc() << "Failed to close qlist file ["
                << fileSetRef.d_qlistFileName << "], rc: " << rc
                << MWCTSK_ALARMLOG_END;
        }
    }
}

void FileStore::archive(FileSet* fileSet)
{
    int rc = FileSystemUtil::move(fileSet->d_dataFileName,
                                  d_config.archiveLocation());
    if (0 != rc) {
        MWCTSK_ALARMLOG_ALARM("FILE_IO")
            << partitionDesc() << "Failed to move file ["
            << fileSet->d_dataFileName << "] "
            << "to location [" << d_config.archiveLocation() << "] rc: " << rc
            << MWCTSK_ALARMLOG_END;
    }

    rc = FileSystemUtil::move(fileSet->d_journalFileName,
                              d_config.archiveLocation());
    if (0 != rc) {
        MWCTSK_ALARMLOG_ALARM("FILE_IO")
            << partitionDesc() << "Failed to move file ["
            << fileSet->d_journalFileName << "] "
            << "to location [" << d_config.archiveLocation() << "] rc: " << rc
            << MWCTSK_ALARMLOG_END;
    }

    if (!d_isFSMWorkflow) {
        rc = FileSystemUtil::move(fileSet->d_qlistFileName,
                                  d_config.archiveLocation());
        if (0 != rc) {
            MWCTSK_ALARMLOG_ALARM("FILE_IO")
                << partitionDesc() << "Failed to move file ["
                << fileSet->d_qlistFileName << "] "
                << "to location [" << d_config.archiveLocation()
                << "] rc: " << rc << MWCTSK_ALARMLOG_END;
        }
    }
}

void FileStore::gc(FileSet* fileSet)
{
    // executed by *ANY* thread

    // Fast path if we are already in dispatcher thread of this file store.

    if (inDispatcherThread()) {
        gcDispatched(d_config.partitionId(), fileSet);
        return;  // RETURN
    }

    // Enqueue an event in the partition's dispatcher thread.
    execute(bdlf::BindUtil::bind(&FileStore::gcDispatched,
                                 this,
                                 d_config.partitionId(),
                                 fileSet));
}

void FileStore::gcDispatched(int partitionId, FileSet* fileSet)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(partitionId == d_config.partitionId());
    BSLS_ASSERT_SAFE(fileSet);
    BSLS_ASSERT_SAFE(0 < d_fileSets.size());
    (void)partitionId;  // Compiler happiness

    if (fileSet == d_fileSets[0].get()) {
        // This occurs when FileStore::close() has happened.

        BALL_LOG_INFO_BLOCK
        {
            BALL_LOG_OUTPUT_STREAM << partitionDesc() << "Closing file set ["
                                   << fileSet->d_dataFileName << "], ["
                                   << fileSet->d_journalFileName << "]";
            if (!d_isFSMWorkflow) {
                BALL_LOG_OUTPUT_STREAM << ", [" << fileSet->d_qlistFileName
                                       << "]";
            }
            BALL_LOG_OUTPUT_STREAM << ".";
        }

        close(*fileSet, d_flushWhenClosing);
        d_fileSets.erase(d_fileSets.begin());

        return;  // RETURN
    }

    // This occurs when FileStore::rollover() has happened.

    FileSetSp          fileSetSp;
    FileSets::iterator it;
    for (it = d_fileSets.begin(); it != d_fileSets.end(); ++it) {
        if (it->get() == fileSet) {
            fileSetSp = *it;
            break;  // BREAK
        }
    }

    BSLS_ASSERT_SAFE(fileSetSp);
    BSLS_ASSERT_SAFE(fileSet->d_dataFile.isValid());
    BSLS_ASSERT_SAFE(it != d_fileSets.begin());
    // Cannot be the 1st file set in the vector, because 1st file set is
    // the 'active' (current) one.

    // Remove 'fileSet' from 'd_fileSets' in this thread ('d_fileSets' can be
    // manipulated only from partition-dispatcher thread), and enqueue a job in
    // the worker thread to close/unmap it.  'fileSetSp' now points to that
    // file set.
    d_fileSets.erase(it);

    BSLA_MAYBE_UNUSED const int rc = d_miscWorkThreadPool_p->enqueueJob(
        bdlf::BindUtil::bind(&FileStore::gcWorkerDispatched, this, fileSetSp));
    BSLS_ASSERT_SAFE(rc == 0);
}

void FileStore::gcWorkerDispatched(const bsl::shared_ptr<FileSet>& fileSet)
{
    // executed by a *WORKER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(fileSet);

    // Files have already been truncated.  Can safely close and archive.
    BALL_LOG_INFO_BLOCK
    {
        BALL_LOG_OUTPUT_STREAM
            << partitionDesc() << "Closing and archiving file set ["
            << fileSet->d_dataFileName << "], [" << fileSet->d_journalFileName
            << "]";
        if (!d_isFSMWorkflow) {
            BALL_LOG_OUTPUT_STREAM << ", [" << fileSet->d_qlistFileName << "]";
        }
        BALL_LOG_OUTPUT_STREAM << " as it can be gc'd.";
    }

    bsls::Types::Int64 startTime = mwcsys::Time::highResolutionTimer();

    close(*fileSet, false);
    bsls::Types::Int64 closeTime = mwcsys::Time::highResolutionTimer();
    BALL_LOG_INFO << partitionDesc() << "File set closed. Time taken: "
                  << mwcu::PrintUtil::prettyTimeInterval(closeTime -
                                                         startTime);

    bsls::Types::Int64 archiveStartTime = mwcsys::Time::highResolutionTimer();
    archive(fileSet.get());
    bsls::Types::Int64 archiveEndTime = mwcsys::Time::highResolutionTimer();
    BALL_LOG_INFO << partitionDesc() << "File set archived. Time taken: "
                  << mwcu::PrintUtil::prettyTimeInterval(archiveEndTime -
                                                         archiveStartTime);
}

int FileStore::writeQueueOpRecord(DataStoreRecordHandle*  handle,
                                  const mqbu::StorageKey& queueKey,
                                  const mqbu::StorageKey& appKey,
                                  QueueOpType::Enum       queueOpFlag,
                                  bsls::Types::Uint64     timestamp)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(handle);
    BSLS_ASSERT_SAFE(QueueOpType::e_CREATION != queueOpFlag &&
                     QueueOpType::e_ADDITION != queueOpFlag);
    BSLS_ASSERT_SAFE(0 < d_fileSets.size());

    enum {
        rc_SUCCESS          = 0,
        rc_NOT_PRIMARY      = -1,
        rc_UNAVAILABLE      = -2,
        rc_ROLLOVER_FAILURE = -3,
        rc_PARTITION_FULL   = -4
    };

    FileSet* activeFileSet = d_fileSets[0].get();
    BSLS_ASSERT_SAFE(activeFileSet);

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!d_isPrimary)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        MWCTSK_ALARMLOG_ALARM("REPLICATION")
            << partitionDesc()
            << "Not the primary. Not writing & replicating queueOp record with"
            << " queueKey [" << queueKey << "]" << MWCTSK_ALARMLOG_END;
        return rc_NOT_PRIMARY;  // RETURN
    }

    if (!activeFileSet->d_journalFileAvailable) {
        return rc_UNAVAILABLE;  // RETURN
    }

    // Roll over if needed
    int rc = rolloverIfNeeded(FileType::e_JOURNAL,
                              activeFileSet->d_journalFile,
                              activeFileSet->d_journalFileName,
                              activeFileSet->d_journalFilePosition,
                              k_REQUESTED_JOURNAL_SPACE);
    if (rc != 0) {
        return 10 * rc + rc_ROLLOVER_FAILURE;  // RETURN
    }

    // Update 'activeFileSet' as it may have rolled over above.
    activeFileSet = d_fileSets[0].get();

    // Local refs for convenience.
    MappedFileDescriptor& journal    = activeFileSet->d_journalFile;
    bsls::Types::Uint64&  journalPos = activeFileSet->d_journalFilePosition;

    BSLS_ASSERT_SAFE(journal.fileSize() >=
                     (journalPos + k_REQUESTED_JOURNAL_SPACE));
    // JOURNAL has been successfully rolled over, so above assert should
    // not fire.

    // Append queueOp record to journal.
    bsls::Types::Uint64      recordOffset = journalPos;
    OffsetPtr<QueueOpRecord> qRec(journal.block(), journalPos);
    new (qRec.get()) QueueOpRecord();
    qRec->header()
        .setPrimaryLeaseId(d_primaryLeaseId)
        .setSequenceNumber(++d_sequenceNum)
        .setTimestamp(timestamp);
    qRec->setQueueKey(queueKey).setType(queueOpFlag);

    if (!appKey.isNull()) {
        qRec->setAppKey(appKey);
    }
    qRec->setMagic(RecordHeader::k_MAGIC);
    journalPos += FileStoreProtocol::k_JOURNAL_RECORD_SIZE;

    replicateAndInsertDataStoreRecord(handle,
                                      bmqp::StorageMessageType::e_QUEUE_OP,
                                      RecordType::e_QUEUE_OP,
                                      recordOffset);

    return rc_SUCCESS;
}

void FileStore::writeRolledOverRecord(DataStoreRecord*    record,
                                      QueueKeyCounterMap* queueKeyCounterMap,
                                      FileSet*            oldFileSet,
                                      FileSet*            newFileSet)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(0 != record->d_recordOffset);
    BSLS_ASSERT_SAFE(RecordType::e_UNDEFINED != record->d_recordType &&
                     RecordType::e_JOURNAL_OP != record->d_recordType);

    // Local refs for convenience
    MappedFileDescriptor& rDataFile     = newFileSet->d_dataFile;
    bsls::Types::Uint64&  rDataFilePos  = newFileSet->d_dataFilePosition;
    MappedFileDescriptor& rJournal      = newFileSet->d_journalFile;
    bsls::Types::Uint64&  rJournalPos   = newFileSet->d_journalFilePosition;
    MappedFileDescriptor& rQlistFile    = newFileSet->d_qlistFile;
    bsls::Types::Uint64&  rQlistFilePos = newFileSet->d_qlistFilePosition;

    const MappedFileDescriptor& aJournal   = oldFileSet->d_journalFile;
    const MappedFileDescriptor& aDataFile  = oldFileSet->d_dataFile;
    const MappedFileDescriptor& aQlistFile = oldFileSet->d_qlistFile;

    if (RecordType::e_MESSAGE == record->d_recordType) {
        // Its a MessageRecord, copy payload as well.
        OffsetPtr<const MessageRecord> fromRec(aJournal.block(),
                                               record->d_recordOffset);

        // Take note of offset in rolled over data file
        bsls::Types::Uint64 newDataFileOffset = rDataFilePos;
        BSLS_ASSERT_SAFE(0 ==
                         newDataFileOffset % bmqp::Protocol::k_DWORD_SIZE);

        // Copy payload (including DataHeader) to data file.  This should be
        // done *before* copying message record to journal file.

        bsls::Types::Uint64 messageOffset =
            static_cast<bsls::Types::Uint64>(fromRec->messageOffsetDwords()) *
            bmqp::Protocol::k_DWORD_SIZE;

        OffsetPtr<const DataHeader> dataHeader(aDataFile.block(),
                                               messageOffset);

        const unsigned int dataMsgSize = dataHeader->messageWords() *
                                         bmqp::Protocol::k_WORD_SIZE;

        bsl::memcpy(rDataFile.block().base() + rDataFilePos,
                    aDataFile.block().base() + messageOffset,
                    dataMsgSize);

        rDataFilePos += dataMsgSize;

        // Append MessageRecord to journal.

        OffsetPtr<MessageRecord> toRec(rJournal.block(), rJournalPos);
        new (toRec.get()) MessageRecord(*fromRec);
        toRec->setMessageOffsetDwords(newDataFileOffset /
                                      bmqp::Protocol::k_DWORD_SIZE);

        // Update offset of the message in the in-memory DataStoreRecord
        // 'record'.
        record->d_messageOffset = newDataFileOffset;

        // Increase the message and byte counter for this queue.

        QueueKeyCounterMapIter qit = queueKeyCounterMap->find(
            toRec->queueKey());

        BSLS_ASSERT_SAFE(queueKeyCounterMap->end() != qit);

        ++(qit->second.first);
        qit->second.second += dataMsgSize;

        newFileSet->d_outstandingBytesData += dataMsgSize;
    }
    else if (RecordType::e_QUEUE_OP == record->d_recordType) {
        OffsetPtr<const QueueOpRecord> fromRec(aJournal.block(),
                                               record->d_recordOffset);

        if (QueueOpType::e_CREATION == fromRec->type() ||
            QueueOpType::e_ADDITION == fromRec->type()) {
            bsls::Types::Uint64 newQlistOffset = 0;
            if (!d_isFSMWorkflow) {
                // Copy QLIST record as well.

                // Take note of offset in rolled over QLIST file.

                newQlistOffset = rQlistFilePos;
                BSLS_ASSERT_SAFE(0 ==
                                 newQlistOffset % bmqp::Protocol::k_WORD_SIZE);

                // Copy QueueUriRecord to new QLIST file.

                bsls::Types::Uint64 qlistOffset =
                    static_cast<bsls::Types::Uint64>(
                        fromRec->queueUriRecordOffsetWords()) *
                    bmqp::Protocol::k_WORD_SIZE;

                OffsetPtr<const QueueRecordHeader> queueRecHeader(
                    aQlistFile.block(),
                    qlistOffset);

                unsigned int queueRecLength =
                    queueRecHeader->queueRecordWords() *
                    bmqp::Protocol::k_WORD_SIZE;

                bsl::memcpy(rQlistFile.block().base() + rQlistFilePos,
                            aQlistFile.block().base() + qlistOffset,
                            queueRecLength);
                rQlistFilePos += queueRecLength;

                newFileSet->d_outstandingBytesQlist += queueRecLength;
            }

            // Append QueueOpRecord to journal.

            OffsetPtr<QueueOpRecord> toRec(rJournal.block(), rJournalPos);
            new (toRec.get()) QueueOpRecord(*fromRec);
            toRec->setQueueUriRecordOffsetWords(newQlistOffset /
                                                bmqp::Protocol::k_WORD_SIZE);

            if (QueueOpType::e_CREATION == fromRec->type()) {
                // Create an entry of this queueKey if its a queue creation
                // record.  No need to this it in case of addition record.

                queueKeyCounterMap->insert(
                    bsl::make_pair(toRec->queueKey(),
                                   MessageByteCounter(0, 0)));
            }
        }
        else {
            BSLS_ASSERT_SAFE(QueueOpType::e_PURGE == fromRec->type() ||
                             QueueOpType::e_DELETION == fromRec->type());
            BSLS_ASSERT_SAFE(queueKeyCounterMap->end() !=
                             queueKeyCounterMap->find(fromRec->queueKey()));
            bsl::memcpy(rJournal.block().base() + rJournalPos,
                        aJournal.block().base() + record->d_recordOffset,
                        FileStoreProtocol::k_JOURNAL_RECORD_SIZE);
        }
    }
    else {
        BSLS_ASSERT_SAFE(RecordType::e_CONFIRM == record->d_recordType ||
                         RecordType::e_DELETION == record->d_recordType);
        bsl::memcpy(rJournal.block().base() + rJournalPos,
                    aJournal.block().base() + record->d_recordOffset,
                    FileStoreProtocol::k_JOURNAL_RECORD_SIZE);
    }

    // Irrespective of the type of record, rollover journal's position is
    // bumped up, and record's offset in-memory is updated.

    record->d_recordOffset = rJournalPos;
    rJournalPos += FileStoreProtocol::k_JOURNAL_RECORD_SIZE;

    newFileSet->d_outstandingBytesJournal +=
        FileStoreProtocol::k_JOURNAL_RECORD_SIZE;
}

void FileStore::issueSyncPointCb()
{
    // executed by the *SCHEDULER* thread

    // This routine is invoked *only* by the scheduled recurring event.

    if (!d_isOpen) {
        return;  // RETURN
    }

    execute(bdlf::BindUtil::bind(&FileStore::issueSyncPointDispatched,
                                 this,
                                 d_config.partitionId()));
}

void FileStore::alarmHighwatermarkIfNeededCb()
{
    // executed by the *SCHEDULER* thread

    // This routine is invoked *only* by the scheduled recurring event
    if (!d_isOpen) {
        return;  // RETURN
    }

    execute(
        bdlf::BindUtil::bind(&FileStore::alarmHighwatermarkIfNeededDispatched,
                             this));
}

void FileStore::alarmHighwatermarkIfNeededDispatched()
{
    // executed by the *DISPATCHER* thread

    if (!d_isOpen || !d_isPrimary) {
        return;  // RETURN
    }

    FileSet* activeFileSet = d_fileSets[0].get();
    BSLS_ASSERT_SAFE(activeFileSet);

    bool           needAlarm         = false;
    FileType::Enum needAlarmFileType = FileType::e_UNDEFINED;

    const bsls::Types::Uint64 percentageInUseJournal = computePercentage(
        activeFileSet->d_outstandingBytesJournal,
        d_config.maxJournalFileSize());
    if (percentageInUseJournal >= k_SPACE_USED_PERCENT_SOFT) {
        needAlarm         = true;
        needAlarmFileType = FileType::e_JOURNAL;
    }

    const bsls::Types::Uint64 percentageInUseData = computePercentage(
        activeFileSet->d_outstandingBytesData,
        d_config.maxDataFileSize());
    if (percentageInUseData >= k_SPACE_USED_PERCENT_SOFT) {
        needAlarm         = true;
        needAlarmFileType = FileType::e_DATA;
    }

    const bsls::Types::Uint64 percentageInUseQlist =
        d_isFSMWorkflow
            ? 0
            : computePercentage(activeFileSet->d_outstandingBytesQlist,
                                d_config.maxQlistFileSize());
    if (percentageInUseQlist >= k_SPACE_USED_PERCENT_SOFT) {
        needAlarm         = true;
        needAlarmFileType = FileType::e_QLIST;
    }

    if (needAlarm) {
        BSLS_ASSERT_SAFE(FileType::e_UNDEFINED != needAlarmFileType);

        // Alarm that the high watermark (soft-limit) has been reached
        if (d_alarmSoftLimiter.requestPermission()) {
            mwcu::MemOutStream out;
            out << partitionDesc() << "Outstanding data has accumulated beyond"
                << " the threshold of " << k_SPACE_USED_PERCENT_SOFT
                << "% of partition's usage. File set status:";
            bdlb::Print::newlineAndIndent(out, 1, 4);
            printSpaceInUse(out,
                            "JOURNAL",
                            activeFileSet->d_outstandingBytesJournal,
                            d_config.maxJournalFileSize(),
                            percentageInUseJournal);
            bdlb::Print::newlineAndIndent(out, 1, 4);
            printSpaceInUse(out,
                            "DATA",
                            activeFileSet->d_outstandingBytesData,
                            d_config.maxDataFileSize(),
                            percentageInUseData);
            if (!d_isFSMWorkflow) {
                bdlb::Print::newlineAndIndent(out, 1, 4);
                printSpaceInUse(out,
                                "QLIST",
                                activeFileSet->d_outstandingBytesQlist,
                                d_config.maxQlistFileSize(),
                                percentageInUseQlist);
            }
            out << "\n";

            // Print the top 10 queues contributing to the high watermark alarm
            printTopContributingQueues(out,
                                       10,
                                       "high watermark alarm",
                                       needAlarmFileType,
                                       d_storages);

            MWCTSK_ALARMLOG_ALARM("PARTITION_AVAILABLESPACE_SOFTLIMIT")
                << out.str() << MWCTSK_ALARMLOG_END;
        }
    }
}

void FileStore::issueSyncPointDispatched(
    BSLS_ANNOTATION_UNUSED int partitionId)
{
    // executed by the *DISPATCHER* thread

    if (!d_isOpen || !d_isPrimary) {
        return;  // RETURN
    }

    // This routine is invoked *only* by the scheduled recurring event which
    // attempts to issue a SyncPt if applicable (see 'issueSyncPointCb'), which
    // means that there must be space for at least 2 journal records.

    issueSyncPointInternal(SyncPointType::e_REGULAR,
                           false);  // ImmediateFlush flag
}

int FileStore::issueSyncPointInternal(SyncPointType::Enum type,
                                      bool                immediateFlush,
                                      const bmqp_ctrlmsg::SyncPoint* syncPoint)
{
    enum { rc_SUCCESS = 0, rc_WRITE_FAILURE = -1 };

    bmqp_ctrlmsg::SyncPoint        sp;
    const bmqp_ctrlmsg::SyncPoint* spptr = syncPoint;
    const FileSet*                 fs    = d_fileSets[0].get();

    if (!syncPoint) {
        // No SyncPt has been explicitly specified.  Check if we need to issue
        // one.

        if (!d_syncPoints.empty()) {
            const bmqp_ctrlmsg::SyncPoint& syncPt =
                d_syncPoints.back().syncPoint();
            if (syncPt.primaryLeaseId() == d_primaryLeaseId) {
                if (syncPt.sequenceNum() == d_sequenceNum) {
                    // No new message has been published.
                    return rc_SUCCESS;  // RETURN
                }

                BSLS_ASSERT_SAFE(syncPt.sequenceNum() < d_sequenceNum);
            }
            else {
                BSLS_ASSERT_SAFE(syncPt.primaryLeaseId() < d_primaryLeaseId);

                if (0 == d_sequenceNum) {
                    // New primary and no force issue requested.  Currently,
                    // this check is redundant because we always force issue a
                    // sync point when a primary is chosen (see
                    // 'setActivePrimary()'), which always bumps up
                    // 'd_sequenceNum' to 1.

                    return rc_SUCCESS;  // RETURN
                }
            }
        }

        sp.primaryLeaseId()       = d_primaryLeaseId;
        sp.sequenceNum()          = ++d_sequenceNum;
        sp.dataFileOffsetDwords() = fs->d_dataFilePosition /
                                    bmqp::Protocol::k_DWORD_SIZE;
        if (!d_isFSMWorkflow) {
            sp.qlistFileOffsetWords() = fs->d_qlistFilePosition /
                                        bmqp::Protocol::k_WORD_SIZE;
        }

        spptr = &sp;
    }

    BSLS_ASSERT_SAFE(spptr);

    if (SyncPointType::e_REGULAR == type) {
        // Since the caller has requested a 'regular' SyncPt, journal must have
        // space for at least 2 records (this 'regular' SyncPt, and the
        // following 'rollover' SyncPt).

        BSLS_ASSERT_SAFE(fs->d_journalFile.fileSize() >=
                         (fs->d_journalFilePosition +
                          2 * FileStoreProtocol::k_JOURNAL_RECORD_SIZE));
    }

    // Write to self.
    int rc = writeSyncPointRecord(*spptr, type);
    if (0 != rc) {
        MWCTSK_ALARMLOG_ALARM("FILE_IO")
            << partitionDesc() << "Failed to write sync point: " << *spptr
            << ", rc: " << rc << MWCTSK_ALARMLOG_END;

        // Don't broadcast sync point because we failed to apply it to self
        return 10 * rc + rc_WRITE_FAILURE;  // RETURN
    }

    // Retrieve sync point's offset.
    bsls::Types::Uint64 syncPointJournalOffset =
        d_fileSets[0]->d_journalFilePosition -
        FileStoreProtocol::k_JOURNAL_RECORD_SIZE;

    // Keep track of this sync point.
    bmqp_ctrlmsg::SyncPointOffsetPair spoPair;
    spoPair.syncPoint() = *spptr;
    spoPair.offset()    = syncPointJournalOffset;

    d_syncPoints.push_back(spoPair);

    // Let replicas know about it.
    replicateRecord(bmqp::StorageMessageType::e_JOURNAL_OP,
                    syncPointJournalOffset,
                    immediateFlush);

    // Report cluster's partition stats
    d_clusterStats_p->setPartitionOutstandingBytes(
        d_config.partitionId(),
        fs->d_outstandingBytesData,
        fs->d_outstandingBytesJournal);

    return rc_SUCCESS;
}

void FileStore::processReceiptEvent(unsigned int         primaryLeaseId,
                                    bsls::Types::Uint64  sequenceNumber,
                                    mqbnet::ClusterNode* source)
{
    BSLS_ASSERT_SAFE(source);

    if (!d_isPrimary || d_isStopping) {
        return;  // RETURN
    }

    const DataStoreRecordKey recordKey(sequenceNumber, primaryLeaseId);
    Unreceipted::iterator    to = d_unreceipted.find(recordKey);
    // end of of Receipt range

    if (to == d_unreceipted.end()) {
        // ignore
        return;  // RETURN
    }

    // `to` is the end of Receipt range.  Find the start of the range
    // using prior 'source' history and making sure not to count anything
    // twice.

    int                           nodeId = source->nodeId();
    NodeReceiptContexts::iterator itNode = d_nodes.find(nodeId);
    Unreceipted::iterator         from;  // start of Receipt range

    if (itNode == d_nodes.end()) {
        // no prior history about this node
        d_nodes.insert(bsl::make_pair(
            nodeId,
            NodeContext(d_config.bufferFactory(), recordKey, d_allocator_p)));
        from = d_unreceipted.begin();
    }
    else if (itNode->second.d_key < recordKey) {
        from = d_unreceipted.find(itNode->second.d_key);

        if (from == d_unreceipted.end()) {
            // we went past the last Receipt from this node
            from = d_unreceipted.begin();
        }
        else {
            ++from;  // `itNode->second` was the last Receipt
        }

        itNode->second.d_key = recordKey;
        // next range starts at (d_unreceipted.find(itNode->second) + 1)
    }
    else {
        // This Receipt is about something already Receipted.  Ignore
        return;  // RETURN
    }

    // everything in [from, to] is Receipt'ed
    bool                             isEndOfRange = false;
    mqbu::StorageKey                 lastKey;
    bsl::unordered_set<mqbi::Queue*> affectedQueues(d_allocator_p);
    mqbi::Queue*                     lastQueue = 0;

    while (from != d_unreceipted.end() && !isEndOfRange) {
        if (from->first == recordKey) {
            // This is the last in the range
            isEndOfRange = true;
        }
        if (++(from->second.d_count) >= d_replicationFactor) {
            from->second.d_handle->second.d_hasReceipt = true;
            // notify the queue

            const mqbu::StorageKey& queueKey  = from->second.d_queueKey;
            bool                    haveQueue = (queueKey == lastKey);
            if (!haveQueue) {
                StorageMapIter sit = d_storages.find(queueKey);
                if (sit != d_storages.end()) {
                    haveQueue = true;
                    lastKey   = queueKey;
                    lastQueue = sit->second->queue();
                    BSLS_ASSERT_SAFE(lastQueue);

                    affectedQueues.insert(lastQueue);
                }
                // else the queue and its storage are gone; ignore the receipt
            }
            if (haveQueue) {
                lastQueue->onReceipt(
                    from->second.d_guid,
                    from->second.d_qH,
                    from->second.d_handle->second.d_arrivalTimepoint);
            }  // else the queue is gone
            from = d_unreceipted.erase(from);
        }
        else {
            ++from;
        }
    }
    for (bsl::unordered_set<mqbi::Queue*>::iterator it =
             affectedQueues.begin();
         it != affectedQueues.end();
         ++it) {
        (*it)->onReplicatedBatch();
    }
}

int FileStore::writeMessageRecord(const bmqp::StorageHeader& header,
                                  const mqbs::RecordHeader&  recHeader,
                                  const bsl::shared_ptr<bdlbb::Blob>& event,
                                  const mwcu::BlobPosition& recordPosition)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(0 < d_fileSets.size());

    enum {
        rc_SUCCESS              = 0,
        rc_UNAVAILABLE          = -1,
        rc_JOURNAL_OUT_OF_SYNC  = -2,
        rc_MISSING_PAYLOAD      = -3,
        rc_MISSING_PAYLOAD_HDR  = -4,
        rc_INCOMPLETE_PAYLOAD   = -5,
        rc_UNKNOWN_QUEUE_KEY    = -6,
        rc_INCOMPATIBLE_STORAGE = -7,
        rc_DATA_OFFSET_MISMATCH = -8
    };

    FileSet* activeFileSet = d_fileSets[0].get();
    BSLS_ASSERT_SAFE(activeFileSet);

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            !activeFileSet->d_journalFileAvailable)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return rc_UNAVAILABLE;  // RETURN
    }

    // Local refs for convenience.
    MappedFileDescriptor& dataFile    = activeFileSet->d_dataFile;
    bsls::Types::Uint64&  dataFilePos = activeFileSet->d_dataFilePosition;
    MappedFileDescriptor& journal     = activeFileSet->d_journalFile;
    bsls::Types::Uint64&  journalPos  = activeFileSet->d_journalFilePosition;
    bsls::Types::Uint64   dataOffset  = dataFilePos;

    // Ensure that JOURNAL offset of primary and self match.  Also note that
    // DATA offsets are checked later in this routine.
    const bsls::Types::Uint64 primaryJournalOffset =
        static_cast<bsls::Types::Uint64>(header.journalOffsetWords()) *
        bmqp::Protocol::k_WORD_SIZE;

    if (journalPos != primaryJournalOffset) {
        // Primary's and self views of the journal have diverged.

        MWCTSK_ALARMLOG_ALARM("REPLICATION")
            << partitionDesc()
            << "Received DATA record with journal offset mismatch.  "
            << "Primary's journal offset: " << primaryJournalOffset
            << ", self journal offset: " << journalPos
            << ", msg sequence number (" << recHeader.primaryLeaseId() << ", "
            << recHeader.sequenceNumber() << "). Ignoring this message."
            << MWCTSK_ALARMLOG_END;
        return rc_JOURNAL_OUT_OF_SYNC;  // RETURN
    }

    // Extract payload's position from blob, based on 'recordPosition'.  Per
    // replication algo, a storage message starts with journal record followed
    // by payload.  Payload contains 'mqbs::DataHeader', options (if any),
    // properties and message, and is already DWORD aligned.

    mwcu::BlobPosition payloadBeginPos;
    int                rc = mwcu::BlobUtil::findOffsetSafe(
        &payloadBeginPos,
        *event,
        recordPosition,
        FileStoreProtocol::k_JOURNAL_RECORD_SIZE);

    if (0 != rc) {
        return 10 * rc + rc_MISSING_PAYLOAD;  // RETURN
    }

    mwcu::BlobObjectProxy<DataHeader> dataHeader(
        event.get(),
        payloadBeginPos,
        -DataHeader::k_MIN_HEADER_SIZE,
        true,    // read
        false);  // write
    if (!dataHeader.isSet()) {
        // Couldn't read DataHeader
        return rc_MISSING_PAYLOAD_HDR;  // RETURN
    }

    // Ensure that blob has enough data as indicated by length in 'dataHeader'.
    // TBD: find a cheaper way for this check.

    const int headerSize = dataHeader->headerWords() *
                           bmqp::Protocol::k_WORD_SIZE;
    const int optionsSize = dataHeader->optionsWords() *
                            bmqp::Protocol::k_WORD_SIZE;
    const int messageSize = dataHeader->messageWords() *
                            bmqp::Protocol::k_WORD_SIZE;

    mwcu::BlobPosition payloadEndPos;
    rc = mwcu::BlobUtil::findOffsetSafe(&payloadEndPos,
                                        *event,
                                        payloadBeginPos,
                                        messageSize);
    if (0 != rc) {
        return 10 * rc + rc_INCOMPLETE_PAYLOAD;  // RETURN
    }

    BSLS_ASSERT_SAFE(0 == dataOffset % bmqp::Protocol::k_DWORD_SIZE);
    BSLS_ASSERT_SAFE(dataFile.fileSize() >= (dataFilePos + messageSize));

    // Append payload to data file.

    mwcu::BlobUtil::copyToRawBufferFromIndex(dataFile.block().base() +
                                                 dataFilePos,
                                             *event,
                                             payloadBeginPos.buffer(),
                                             payloadBeginPos.byte(),
                                             messageSize);
    dataFilePos += messageSize;

    // Keep track of journal record's offset.

    bsls::Types::Uint64 recordOffset = journalPos;

    // Append message record to journal.

    BSLS_ASSERT_SAFE(journal.fileSize() >=
                     (journalPos + k_REQUESTED_JOURNAL_SPACE));

    mwcu::BlobUtil::copyToRawBufferFromIndex(
        journal.block().base() + recordOffset,
        *event,
        recordPosition.buffer(),
        recordPosition.byte(),
        FileStoreProtocol::k_JOURNAL_RECORD_SIZE);
    journalPos += FileStoreProtocol::k_JOURNAL_RECORD_SIZE;

    OffsetPtr<const MessageRecord> msgRec(journal.block(), recordOffset);

    // Check if the queueKey is known.  Ideally, this check should occur at the
    // beginning of this routine (before writing the record to file), but if
    // we attempt to read the record and the queueKey in that record (using
    // BlobObjectProxy), depending upon where the record appears in the blob,
    // we may end up copying it.  So we do this check after record has been
    // written to the file.

    StorageMapIter sit = d_storages.find(msgRec->queueKey());
    if (sit == d_storages.end()) {
        return rc_UNKNOWN_QUEUE_KEY;  // RETURN
    }

    ReplicatedStorage* rstorage = sit->second;
    BSLS_ASSERT_SAFE(rstorage);
    if (!rstorage->isPersistent()) {
        return rc_INCOMPATIBLE_STORAGE;  // RETURN
    }

    // Check data offset in the replicated journal record sent by the primary
    // vs data offset maintained by self.  A mismatch means that replica's and
    // primary's storages are no longer in sync, which indicates a bug in
    // BlazingMQ replication algorithm.
    if (dataOffset !=
        static_cast<bsls::Types::Uint64>(msgRec->messageOffsetDwords()) *
            bmqp::Protocol::k_DWORD_SIZE) {
        return rc_DATA_OFFSET_MISMATCH;  // RETURN
    }

    // For padding.
    const char lastByte =
        dataFile.block().base()[dataOffset + messageSize - 1];

    // Create in-memory record
    DataStoreRecord record(RecordType::e_MESSAGE, recordOffset);
    record.d_messageOffset      = dataOffset;
    record.d_appDataUnpaddedLen = messageSize - headerSize - optionsSize -
                                  lastByte;
    record.d_dataOrQlistRecordPaddedLen = messageSize;
    record.d_hasReceipt                 = true;
    record.d_arrivalTimestamp           = recHeader.timestamp();

    record.d_messagePropertiesInfo = bmqp::MessagePropertiesInfo(*dataHeader);

    BSLS_ASSERT_SAFE(0 < record.d_appDataUnpaddedLen);

    DataStoreRecordKey    key(recHeader.sequenceNumber(),
                           recHeader.primaryLeaseId());
    DataStoreRecordHandle handle;
    insertDataStoreRecord(&handle, key, record);

    rstorage->processMessageRecord(msgRec->messageGUID(),
                                   record.d_appDataUnpaddedLen,
                                   msgRec->refCount(),
                                   handle);

    activeFileSet->d_outstandingBytesJournal +=
        FileStoreProtocol::k_JOURNAL_RECORD_SIZE;
    activeFileSet->d_outstandingBytesData += messageSize;

    return rc_SUCCESS;
}

int FileStore::writeQueueCreationRecord(
    const bmqp::StorageHeader&          header,
    const mqbs::RecordHeader&           recHeader,
    const bsl::shared_ptr<bdlbb::Blob>& event,
    const mwcu::BlobPosition&           recordPosition)
{
    enum {
        rc_SUCCESS                     = 0,
        rc_UNAVAILABLE                 = -1,
        rc_JOURNAL_OUT_OF_SYNC         = -2,
        rc_MISSING_QUEUE_RECORD        = -3,
        rc_MISSING_QUEUE_RECORD_HEADER = -4,
        rc_INCOMPLETE_QUEUE_RECORD     = -5,
        rc_INVALID_QUEUE_RECORD        = -6,
        rc_QLIST_OFFSET_MISMATCH       = -7,
        rc_QUEUE_CREATION_FAILURE      = -8
    };

    BSLS_ASSERT_SAFE(0 < d_fileSets.size());
    FileSet* activeFileSet = d_fileSets[0].get();
    BSLS_ASSERT_SAFE(activeFileSet);

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            !activeFileSet->d_journalFileAvailable)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return rc_UNAVAILABLE;  // RETURN
    }

    // Local refs for convenience.

    MappedFileDescriptor& qlistFile    = activeFileSet->d_qlistFile;
    bsls::Types::Uint64&  qlistFilePos = activeFileSet->d_qlistFilePosition;
    MappedFileDescriptor& journal      = activeFileSet->d_journalFile;
    bsls::Types::Uint64&  journalPos   = activeFileSet->d_journalFilePosition;
    bsls::Types::Uint64   qlistOffset  = qlistFilePos;

    // Ensure that JOURNAL offset of primary and self match.  Note that QLIST
    // offset is checked later in this routine.

    const bsls::Types::Uint64 primaryJournalOffset =
        static_cast<bsls::Types::Uint64>(header.journalOffsetWords()) *
        bmqp::Protocol::k_WORD_SIZE;

    if (journalPos != primaryJournalOffset) {
        // Primary's and self views of the journal have diverged.

        MWCTSK_ALARMLOG_ALARM("REPLICATION")
            << partitionDesc()
            << "Received QLIST record with journal offset mismatch.  "
            << "Primary's journal offset: " << primaryJournalOffset
            << ", self journal offset: " << journalPos
            << ", msg sequence number (" << recHeader.primaryLeaseId() << ", "
            << recHeader.sequenceNumber() << "). Ignoring this message."
            << MWCTSK_ALARMLOG_END;
        return rc_JOURNAL_OUT_OF_SYNC;  // RETURN
    }

    mwcu::BlobObjectProxy<QueueRecordHeader> queueRecHeader;
    unsigned int                             queueRecHeaderLen = 0;
    unsigned int                             queueRecLength    = 0;
    if (!d_isFSMWorkflow) {
        // Extract queue record's position from blob, based on
        // 'recordPosition'.  Per replication algo, a storage message starts
        // with journal record followed by queue record.  Queue record already
        // contains 'mqbs::QueueRecordHeader' and is already WORD aligned.

        mwcu::BlobPosition queueRecBeginPos;
        int                rc = mwcu::BlobUtil::findOffsetSafe(
            &queueRecBeginPos,
            *event,
            recordPosition,
            FileStoreProtocol::k_JOURNAL_RECORD_SIZE);

        if (0 != rc) {
            return 10 * rc + rc_MISSING_QUEUE_RECORD;  // RETURN
        }

        queueRecHeader.reset(event.get(),
                             queueRecBeginPos,
                             -QueueRecordHeader::k_MIN_HEADER_SIZE,
                             true,    // read
                             false);  // write
        if (!queueRecHeader.isSet()) {
            // Couldn't read QueueRecordHeader
            return rc_MISSING_QUEUE_RECORD_HEADER;  // RETURN
        }

        // Ensure that blob has enough data as indicated by length in
        // 'queueRecordHeader'.

        queueRecHeaderLen = queueRecHeader->headerWords() *
                            bmqp::Protocol::k_WORD_SIZE;
        queueRecLength = queueRecHeader->queueRecordWords() *
                         bmqp::Protocol::k_WORD_SIZE;

        mwcu::BlobPosition queueRecEndPos;
        rc = mwcu::BlobUtil::findOffsetSafe(&queueRecEndPos,
                                            *event,
                                            queueRecBeginPos,
                                            queueRecLength);
        if (0 != rc) {
            return 10 * rc + rc_INCOMPLETE_QUEUE_RECORD;  // RETURN
        }

        BSLS_ASSERT_SAFE(0 == qlistOffset % bmqp::Protocol::k_WORD_SIZE);
        BSLS_ASSERT_SAFE(qlistFile.fileSize() >=
                         (qlistFilePos + queueRecLength));

        // Append payload to QLIST file.

        mwcu::BlobUtil::copyToRawBufferFromIndex(qlistFile.block().base() +
                                                     qlistFilePos,
                                                 *event,
                                                 queueRecBeginPos.buffer(),
                                                 queueRecBeginPos.byte(),
                                                 queueRecLength);
        qlistFilePos += queueRecLength;
    }

    // Keep track of journal record's offset.

    bsls::Types::Uint64 recordOffset = journalPos;

    // Append message record to journal.

    BSLS_ASSERT_SAFE(journal.fileSize() >=
                     (journalPos + k_REQUESTED_JOURNAL_SPACE));
    mwcu::BlobUtil::copyToRawBufferFromIndex(
        journal.block().base() + recordOffset,
        *event,
        recordPosition.buffer(),
        recordPosition.byte(),
        FileStoreProtocol::k_JOURNAL_RECORD_SIZE);
    journalPos += FileStoreProtocol::k_JOURNAL_RECORD_SIZE;

    OffsetPtr<const QueueOpRecord> queueRec(journal.block(), recordOffset);
    if (QueueOpType::e_CREATION != queueRec->type() &&
        QueueOpType::e_ADDITION != queueRec->type()) {
        BALL_LOG_ERROR << partitionDesc()
                       << " Unexpected QueueOpType: " << queueRec->type();
        return rc_INVALID_QUEUE_RECORD;  // RETURN
    }

    bmqt::Uri     quri;
    AppIdKeyPairs appIdKeyPairs;
    if (!d_isFSMWorkflow) {
        // Check qlist offset in the replicated journal record sent by the
        // primary vs qlist offset maintained by self.  A mismatch means that
        // replica's and primary's storages are no longer in sync, which
        // indicates a bug in BlazingMQ replication algorithm.

        if (qlistOffset != (static_cast<bsls::Types::Uint64>(
                                queueRec->queueUriRecordOffsetWords()) *
                            bmqp::Protocol::k_WORD_SIZE)) {
            return rc_QLIST_OFFSET_MISMATCH;  // RETURN
        }

        // Retrieve QueueKey & QueueUri from QueueOpRecord and QueueUriRecord
        // respectively, and notify storage manager.

        unsigned int paddedUriLen = queueRecHeader->queueUriLengthWords() *
                                    bmqp::Protocol::k_WORD_SIZE;

        BSLS_ASSERT_SAFE(0 < paddedUriLen);

        const char* uriBegin = qlistFile.block().base() + qlistOffset +
                               queueRecHeaderLen;
        bslstl::StringRef uri(uriBegin,
                              paddedUriLen - uriBegin[paddedUriLen - 1]);
        quri                        = bmqt::Uri(uri);
        unsigned int appIdsAreaSize = queueRecLength - queueRecHeaderLen -
                                      paddedUriLen -
                                      FileStoreProtocol::k_HASH_LENGTH -
                                      sizeof(unsigned int);  // Magic word
        MemoryBlock appIdsBlock(qlistFile.block().base() + qlistOffset +
                                    queueRecHeaderLen + paddedUriLen +
                                    FileStoreProtocol::k_HASH_LENGTH,
                                appIdsAreaSize);
        FileStoreProtocolUtil::loadAppIdKeyPairs(&appIdKeyPairs,
                                                 appIdsBlock,
                                                 queueRecHeader->numAppIds());
    }

    BALL_LOG_INFO_BLOCK
    {
        BALL_LOG_OUTPUT_STREAM << partitionDesc()
                               << "Received QueueCreationRecord of "
                               << "type [" << queueRec->type() << "] for "
                               << "queueKey [" << queueRec->queueKey() << "]";
        if (!d_isFSMWorkflow) {
            BALL_LOG_OUTPUT_STREAM << ", queue [" << quri << "]"
                                   << ", with [" << appIdKeyPairs.size()
                                   << "] appId/appKey pairs ";
            for (size_t n = 0; n < appIdKeyPairs.size(); ++n) {
                BALL_LOG_OUTPUT_STREAM << " [" << appIdKeyPairs[n].first
                                       << ", " << appIdKeyPairs[n].second
                                       << "]";
            }
        }
    }

    if (!d_isCSLModeEnabled) {
        int status = 0;
        BSLS_ASSERT_SAFE(d_config.queueCreationCb());
        d_config.queueCreationCb()(&status,
                                   d_config.partitionId(),
                                   quri,
                                   queueRec->queueKey(),
                                   appIdKeyPairs,
                                   QueueOpType::e_CREATION ==
                                       queueRec->type());

        if (0 != status) {
            return 10 * status + rc_QUEUE_CREATION_FAILURE;  // RETURN
        }
    }

    StorageMapIter sit = d_storages.find(queueRec->queueKey());
    if (sit == d_storages.end()) {
        if (d_isCSLModeEnabled) {
            return rc_QUEUE_CREATION_FAILURE;  // RETURN
        }
        else {
            BSLS_ASSERT_SAFE(false && "Queue storage must have been created");
        }
    }

    ReplicatedStorage* rstorage = sit->second;
    BSLS_ASSERT_SAFE(rstorage);

    // Create in-memory record.
    DataStoreRecord       record(RecordType::e_QUEUE_OP,
                           recordOffset,
                           queueRecLength);
    DataStoreRecordKey    key(recHeader.sequenceNumber(),
                           recHeader.primaryLeaseId());
    DataStoreRecordHandle handle;
    insertDataStoreRecord(&handle, key, record);

    rstorage->addQueueOpRecordHandle(handle);

    activeFileSet->d_outstandingBytesJournal +=
        FileStoreProtocol::k_JOURNAL_RECORD_SIZE;
    if (!d_isFSMWorkflow) {
        activeFileSet->d_outstandingBytesQlist += queueRecLength;
    }

    return rc_SUCCESS;
}

int FileStore::writeJournalRecord(const bmqp::StorageHeader& header,
                                  const mqbs::RecordHeader&  recHeader,
                                  const bsl::shared_ptr<bdlbb::Blob>& event,
                                  const mwcu::BlobPosition& recordPosition,
                                  bmqp::StorageMessageType::Enum messageType)
{
    enum {
        rc_SUCCESS                = 0,
        rc_UNAVAILABLE            = -1,
        rc_JOURNAL_OUT_OF_SYNC    = -2,
        rc_UNKNOWN_QUEUE_KEY      = -3,
        rc_INCOMPATIBLE_STORAGE   = -4,
        rc_UNKNOWN_APP_KEY        = -5,
        rc_INVALID_CONTENT        = -6,
        rc_DATA_FILE_OUT_OF_SYNC  = -7,
        rc_QLIST_FILE_OUT_OF_SYNC = -8,
        rc_ROLLOVER_FAILURE       = -9,
        rc_QUEUE_DELETION_ERROR   = -10
    };

    BSLS_ASSERT_SAFE(0 < d_fileSets.size());
    FileSet* activeFileSet = d_fileSets[0].get();
    BSLS_ASSERT_SAFE(activeFileSet);

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            !activeFileSet->d_journalFileAvailable)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return rc_UNAVAILABLE;  // RETURN
    }

    // Local refs for convenience.

    MappedFileDescriptor& journal    = activeFileSet->d_journalFile;
    bsls::Types::Uint64&  journalPos = activeFileSet->d_journalFilePosition;

    // Ensure that JOURNAL offset of primary and self match.

    const bsls::Types::Uint64 primaryJournalOffset =
        static_cast<bsls::Types::Uint64>(header.journalOffsetWords()) *
        bmqp::Protocol::k_WORD_SIZE;

    if (journalPos != primaryJournalOffset) {
        // Primary's and self views of the journal have diverged.

        MWCTSK_ALARMLOG_ALARM("REPLICATION")
            << partitionDesc() << "Received journal record of type ["
            << messageType
            << "] with journal offset mismatch. Primary's journal offset: "
            << primaryJournalOffset << ", self journal offset: " << journalPos
            << ", msg sequence number (" << recHeader.primaryLeaseId() << ", "
            << recHeader.sequenceNumber() << "). Ignoring this message."
            << MWCTSK_ALARMLOG_END;
        return rc_JOURNAL_OUT_OF_SYNC;  // RETURN
    }

    // Append record to journal.  Note that here, we only assert on journal
    // file having space for 1 record.  This is because the journal record
    // being written could be a sync point record with subType ==
    // SyncPointType::e_ROLLOVER, in which case, journal may not have space for
    // more than 1 record.

    BSLS_ASSERT_SAFE(journal.fileSize() >=
                     (journalPos + FileStoreProtocol::k_JOURNAL_RECORD_SIZE));

    // Keep track of journal record's offset.

    const bsls::Types::Uint64 recordOffset = journalPos;

    mwcu::BlobUtil::copyToRawBufferFromIndex(
        journal.block().base() + recordOffset,
        *event,
        recordPosition.buffer(),
        recordPosition.byte(),
        FileStoreProtocol::k_JOURNAL_RECORD_SIZE);
    journalPos += FileStoreProtocol::k_JOURNAL_RECORD_SIZE;

    // If the record is of type CONFIRM, DELETION or QUEUE_OP, check if the
    // queueKey is known.  Ideally, this check should occur at the beginning of
    // this routine (before writing the record to file), but if we attempt to
    // read the record and the queueKey in that record (using BlobObjectProxy),
    // depending upon where the record appears in the blob, we may end up
    // copying it.  So we do this check after record has been written to the
    // file.  TBD: in case queueKey is unknown, roll back data & journal files?

    const mqbu::StorageKey*   queueKey      = 0;
    const mqbu::StorageKey*   appKey        = &(mqbu::StorageKey::k_NULL_KEY);
    const bmqt::MessageGUID*  guid          = 0;
    RecordType::Enum          recordType    = RecordType::e_UNDEFINED;
    QueueOpType::Enum         queueOpType   = QueueOpType::e_UNDEFINED;
    JournalOpType::Enum       journalOpType = JournalOpType::e_UNDEFINED;
    ReplicatedStorage*        rstorage      = 0;
    mqbs::ConfirmReason::Enum confirmReason = mqbs::ConfirmReason::e_CONFIRMED;

    if (bmqp::StorageMessageType::e_CONFIRM == messageType) {
        OffsetPtr<const ConfirmRecord> confRec(journal.block(), recordOffset);
        queueKey   = &(confRec->queueKey());
        appKey     = &(confRec->appKey());
        guid       = &(confRec->messageGUID());
        recordType = RecordType::e_CONFIRM;
        activeFileSet->d_outstandingBytesJournal +=
            FileStoreProtocol::k_JOURNAL_RECORD_SIZE;
        confirmReason = confRec->reason();
    }
    else if (bmqp::StorageMessageType::e_DELETION == messageType) {
        OffsetPtr<const DeletionRecord> delRec(journal.block(), recordOffset);
        queueKey   = &(delRec->queueKey());
        guid       = &(delRec->messageGUID());
        recordType = RecordType::e_DELETION;
    }
    else if (bmqp::StorageMessageType::e_JOURNAL_OP == messageType) {
        OffsetPtr<const JournalOpRecord> jOpRec(journal.block(), recordOffset);
        journalOpType = jOpRec->type();
        recordType    = RecordType::e_JOURNAL_OP;
        // No need to update outstanding journal bytes since we don't rollover
        // JournalOpRecords (the only sub-type is SyncPt right now).
    }
    else {
        BSLS_ASSERT_SAFE(bmqp::StorageMessageType::e_QUEUE_OP == messageType);
        OffsetPtr<const QueueOpRecord> qOpRec(journal.block(), recordOffset);
        queueKey    = &(qOpRec->queueKey());
        appKey      = &(qOpRec->appKey());
        queueOpType = qOpRec->type();
        recordType  = RecordType::e_QUEUE_OP;
        activeFileSet->d_outstandingBytesJournal +=
            FileStoreProtocol::k_JOURNAL_RECORD_SIZE;
    }

    if (queueKey) {
        // It's one of CONFIRM/DELETION/QUEUE_OP records.

        BSLS_ASSERT_SAFE(!queueKey->isNull());

        if (!(d_isCSLModeEnabled && recordType == RecordType::e_QUEUE_OP &&
              queueOpType == QueueOpType::e_DELETION)) {
            // In CSL mode, the FileStore might receive the QueueDeletionRecord
            // after the queue storage has already been removed in the CSL
            // QueueUnassignment commit callback.  Therefore, we will relax the
            // checks below.

            StorageMapIter sit = d_storages.find(*queueKey);
            if (sit == d_storages.end()) {
                BALL_LOG_ERROR << partitionDesc() << " received message of "
                               << "type '" << messageType << "' for an unknown"
                               << " storage [queueKey: " << *queueKey << "]";
                return rc_UNKNOWN_QUEUE_KEY;  // RETURN
            }

            rstorage = sit->second;
            BSLS_ASSERT_SAFE(rstorage);

            if (RecordType::e_QUEUE_OP != recordType) {
                // It's one of CONFIRM/DELETION records.  Storage must be
                // file-backed.

                if (!rstorage->isPersistent()) {
                    return rc_INCOMPATIBLE_STORAGE;  // RETURN
                }
            }

            if (!appKey->isNull()) {
                if (!(d_isCSLModeEnabled &&
                      recordType == RecordType::e_QUEUE_OP &&
                      queueOpType == QueueOpType::e_PURGE)) {
                    // In CSL mode, if we are unregistering the 'appKey', then
                    // we would have purged the subqueue and removed the
                    // virtual storage for the 'appKey' before receiving this
                    // QueueOp Purge record.
                    if (!rstorage->hasVirtualStorage(*appKey)) {
                        BALL_LOG_ERROR << partitionDesc()
                                       << " storage does not have the key ["
                                       << *appKey << "]";

                        return rc_UNKNOWN_APP_KEY;  // RETURN
                    }
                }
            }
        }
    }

    if (RecordType::e_CONFIRM == recordType) {
        // Keep track of record's offset.

        BSLS_ASSERT_SAFE(guid);
        BSLS_ASSERT_SAFE(queueKey);
        BSLS_ASSERT_SAFE(rstorage);

        DataStoreRecordKey    key(recHeader.sequenceNumber(),
                               recHeader.primaryLeaseId());
        DataStoreRecord       record(recordType, recordOffset);
        DataStoreRecordHandle handle;
        insertDataStoreRecord(&handle, key, record);

        rstorage->processConfirmRecord(*guid, *appKey, confirmReason, handle);
    }
    else if (RecordType::e_DELETION == recordType) {
        // Keep track of record's offset.

        BSLS_ASSERT_SAFE(guid);
        BSLS_ASSERT_SAFE(queueKey);
        BSLS_ASSERT_SAFE(rstorage);

        rstorage->processDeletionRecord(*guid);
    }
    else if (RecordType::e_JOURNAL_OP == recordType) {
        // Take appropriate action based on JournalOpType.

        if (JournalOpType::e_UNDEFINED == journalOpType) {
            MWCTSK_ALARMLOG_ALARM("REPLICATION")
                << partitionDesc()
                << "Received JournalOp msg with invalid type: "
                << journalOpType << ", sequence number ("
                << recHeader.primaryLeaseId() << ", "
                << recHeader.sequenceNumber() << "). Ignoring this message."
                << MWCTSK_ALARMLOG_END;
            return rc_INVALID_CONTENT;  // RETURN
        }

        if (JournalOpType::e_SYNCPOINT == journalOpType) {
            OffsetPtr<const JournalOpRecord> jOpRec(journal.block(),
                                                    recordOffset);

            if (SyncPointType::e_UNDEFINED == jOpRec->syncPointType()) {
                MWCTSK_ALARMLOG_ALARM("REPLICATION")
                    << partitionDesc()
                    << "Received sync point record with invalid type: "
                    << jOpRec->syncPointType() << ", sequence number ("
                    << recHeader.primaryLeaseId() << ", "
                    << recHeader.sequenceNumber()
                    << "). Ignoring this message." << MWCTSK_ALARMLOG_END;
                return rc_INVALID_CONTENT;  // RETURN
            }

            // Note that 'jOpRec->primaryLeaseId()' may be smaller than
            // 'recHeader.primaryLeaseId()' if a new primary was chosen, and as
            // its first job, it issues a sync point with old leaseId &
            // sequenceNum.  Also note that in this case,
            // 'recHeader.sequenceNumber()' will be different from
            // 'jOpRec->sequenceNum()'.

            BSLS_ASSERT_SAFE(jOpRec->primaryLeaseId() <=
                             recHeader.primaryLeaseId());

            // Keep track of latest sync point.  This needs to occur *before*
            // potential rollover() of the file store, rollover() routine uses
            // the latest sync point.

            bmqp_ctrlmsg::SyncPointOffsetPair spoPair;
            bmqp_ctrlmsg::SyncPoint&          syncPoint = spoPair.syncPoint();
            syncPoint.primaryLeaseId()       = jOpRec->primaryLeaseId();
            syncPoint.sequenceNum()          = jOpRec->sequenceNum();
            syncPoint.dataFileOffsetDwords() = jOpRec->dataFileOffsetDwords();
            if (!d_isFSMWorkflow) {
                syncPoint.qlistFileOffsetWords() =
                    jOpRec->qlistFileOffsetWords();
            }
            spoPair.offset() = recordOffset;

            // Ensure that replica's DATA file is in sync with that of primary.

            if (activeFileSet->d_dataFilePosition !=
                (static_cast<bsls::Types::Uint64>(
                     syncPoint.dataFileOffsetDwords()) *
                 bmqp::Protocol::k_DWORD_SIZE)) {
                MWCTSK_ALARMLOG_ALARM("REPLICATION")
                    << partitionDesc()
                    << "DATA file offset mismatch. Received sync point: "
                    << syncPoint << ", offset maintained by self (DWORDS): "
                    << (activeFileSet->d_dataFilePosition /
                        bmqp::Protocol::k_DWORD_SIZE)
                    << MWCTSK_ALARMLOG_END;

                return rc_DATA_FILE_OUT_OF_SYNC;  // RETURN
            }

            if (!d_isFSMWorkflow) {
                // Ensure that replica's QLIST file is in sync with that of
                // primary.

                if (activeFileSet->d_qlistFilePosition !=
                    (static_cast<bsls::Types::Uint64>(
                         syncPoint.qlistFileOffsetWords()) *
                     bmqp::Protocol::k_WORD_SIZE)) {
                    MWCTSK_ALARMLOG_ALARM("REPLICATION")
                        << partitionDesc()
                        << "QLIST file offset mismatch. Received sync point: "
                        << syncPoint << ", offset maintained by self (WORDS): "
                        << (activeFileSet->d_qlistFilePosition /
                            bmqp::Protocol::k_WORD_SIZE)
                        << MWCTSK_ALARMLOG_END;

                    return rc_QLIST_FILE_OUT_OF_SYNC;  // RETURN
                }
            }

            d_syncPoints.push_back(spoPair);
            if (SyncPointType::e_ROLLOVER == jOpRec->syncPointType()) {
                BALL_LOG_INFO
                    << partitionDesc()
                    << "Received SyncPt indicating rollover: " << syncPoint
                    << ", at journal offset: " << recordOffset
                    << ". Initiating rollover.";

                int rc = rollover(jOpRec->header().timestamp());

                if (0 != rc) {
                    MWCTSK_ALARMLOG_ALARM("REPLICATION")
                        << partitionDesc()
                        << "Failed to rollover partition as notified by the "
                        << "primary, rc: " << rc
                        << ", sync point: " << syncPoint
                        << MWCTSK_ALARMLOG_END;
                    return 10 * rc + rc_ROLLOVER_FAILURE;  // RETURN
                }
            }

            // If self is stopping, update the flag which indicates that self
            // has received the "last" SyncPt from the primary, and self should
            // ignore all storage events from this point forward for a clean
            // shutdown.

            if (d_isStopping) {
                d_lastSyncPtReceived = true;

                BALL_LOG_INFO
                    << partitionDesc()
                    << "Received last SyncPt from the primary while shutting "
                    << "down. No further storage events will be processed by "
                    << "self. Current seqNum: (" << d_primaryLeaseId << ", "
                    << d_sequenceNum << ").";
            }
        }
    }
    else {
        // A QueueOp message.  Take appropriate action based on QueueOpType.

        BSLS_ASSERT_SAFE(RecordType::e_QUEUE_OP == recordType);
        BSLS_ASSERT_SAFE(queueKey);

        if (QueueOpType::e_UNDEFINED == queueOpType ||
            QueueOpType::e_CREATION == queueOpType ||
            QueueOpType::e_ADDITION == queueOpType) {
            // QueueCreation event is not a journal record.  Its replicated as
            // a StorageMessageType::e_QLIST message.

            MWCTSK_ALARMLOG_ALARM("REPLICATION")
                << partitionDesc()
                << "Received QueueOp message with invalid type: "
                << queueOpType << ", sequence number ("
                << recHeader.primaryLeaseId() << ", "
                << recHeader.sequenceNumber() << "). Ignoring this message."
                << MWCTSK_ALARMLOG_END;
            return rc_INVALID_CONTENT;  // RETURN
        }

        BALL_LOG_INFO << partitionDesc() << "Received QueueOpRecord of type "
                      << queueOpType << " for queue ["
                      << (rstorage ? rstorage->queueUri() : "**UNKNOWN_URI**")
                      << "], queueKey [" << *queueKey << "], appKey ["
                      << *appKey << "].";

        DataStoreRecordKey    key(recHeader.sequenceNumber(),
                               recHeader.primaryLeaseId());
        DataStoreRecord       record(recordType, recordOffset);
        DataStoreRecordHandle handle;
        insertDataStoreRecord(&handle, key, record);

        if (QueueOpType::e_PURGE == queueOpType) {
            if (appKey->isNull() || rstorage->hasVirtualStorage(*appKey)) {
                rstorage->purge(*appKey);
            }
            else {
                // In CSL mode, if we are unregistering the 'appKey', then we
                // would have purged the subqueue and removed the virtual
                // storage for the 'appKey' before receiving this QueueOp Purge
                // record.  That why we don't process this queue-purge command
                // in CSL mode.
                BSLS_ASSERT_SAFE(d_isCSLModeEnabled);
            }
            rstorage->addQueueOpRecordHandle(handle);
        }
        else {
            BSLS_ASSERT_SAFE(QueueOpType::e_DELETION == queueOpType);

            if (appKey->isNull()) {
                // Entire queue is being deleted.

                if (!d_isCSLModeEnabled) {
                    // In CSL mode, any outstanding QueueOp records are deleted
                    // upon receiving queue-unassigned advisory (see
                    // CQH::onQueueUnassigned and
                    // StorageMgr::unregisterQueueReplica).  In non-CSL mode,
                    // outstanding QueueOp records need to be deleted here.

                    const bsls::Types::Int64 numMsgs = rstorage->numMessages(
                        mqbu::StorageKey::k_NULL_KEY);
                    if (0 != numMsgs) {
                        MWCTSK_ALARMLOG_ALARM("REPLICATION")
                            << partitionDesc()
                            << "Received QueueOpRecord.DELETION for queue ["
                            << rstorage->queueUri() << "] which has ["
                            << numMsgs << "] outstanding messages."
                            << MWCTSK_ALARMLOG_END;
                        return rc_QUEUE_DELETION_ERROR;  // RETURN
                    }

                    const ReplicatedStorage::RecordHandles& recHandles =
                        rstorage->queueOpRecordHandles();

                    for (size_t idx = 0; idx < recHandles.size(); ++idx) {
                        removeRecordRaw(recHandles[idx]);
                    }
                }

                // Delete the QueueOpRecord.DELETION record written above.
                // This needs to be done in both CSL and non-CSL modes.
                removeRecordRaw(handle);
            }
            // else: a non-null appKey is specified in a QueueOpRecord.DELETION
            // record.  No need to remove any queueOpRecords.

            if (d_isCSLModeEnabled) {
                // No further logic for CSL mode.
                return rc_SUCCESS;  // RETURN
            }
            BSLS_ASSERT_SAFE(!d_isFSMWorkflow);

            // Once below callback returns, 'rstorage' will no longer be
            // valid.  So invoking this callback should be the last thing
            // to do in this 'else' snippet.

            int status = 0;
            BSLS_ASSERT_SAFE(d_config.queueDeletionCb());
            d_config.queueDeletionCb()(&status,
                                       d_config.partitionId(),
                                       rstorage->queueUri(),
                                       *queueKey,
                                       *appKey);
            if (0 != status) {
                return rc_QUEUE_DELETION_ERROR;  // RETURN
            }
        }
    }

    return rc_SUCCESS;
}

void FileStore::replicateRecord(bmqp::StorageMessageType::Enum type,
                                bsls::Types::Uint64            journalOffset,
                                bool                           immediateFlush)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(bmqp::StorageMessageType::e_QLIST != type &&
                     bmqp::StorageMessageType::e_DATA != type);
    BSLS_ASSERT_SAFE(d_fileSets.size() > 0);

    if (clusterSize() == 1) {
        return;  // RETURN
    }

    FileSet* activeFileSet = d_fileSets[0].get();
    BSLS_ASSERT_SAFE(activeFileSet);
    BSLS_ASSERT_SAFE(journalOffset != 0);

    const unsigned int journalOffsetWords = journalOffset /
                                            bmqp::Protocol::k_WORD_SIZE;

    MappedFileDescriptor&  journal = activeFileSet->d_journalFile;
    AliasedBufferDeleterSp deleterSp =
        d_aliasedBufferDeleterSpPool.getObject();

    deleterSp->setFileSet(activeFileSet);

    bsl::shared_ptr<char> journalRecordBufferSp(deleterSp,
                                                journal.mapping() +
                                                    journalOffset);
    bdlbb::BlobBuffer     journalRecordBlobBuffer(
        journalRecordBufferSp,
        FileStoreProtocol::k_JOURNAL_RECORD_SIZE);

    bmqt::EventBuilderResult::Enum buildRc;
    bool                           doRetry = false;
    do {
        buildRc = d_storageEventBuilder.packMessage(type,
                                                    d_config.partitionId(),
                                                    0,  // flags
                                                    journalOffsetWords,
                                                    journalRecordBlobBuffer);
        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
                buildRc == bmqt::EventBuilderResult::e_EVENT_TOO_BIG) &&
            !doRetry) {
            flushIfNeeded(true);

            doRetry = true;
        }
        else {
            doRetry = false;
        }
    } while (doRetry);

    if (bmqt::EventBuilderResult::e_SUCCESS != buildRc) {
        MWCTSK_ALARMLOG_ALARM("REPLICATION")
            << partitionDesc()
            << "Failed to pack storage record of type: " << type
            << ", of length " << FileStoreProtocol::k_JOURNAL_RECORD_SIZE
            << ", at JOURNAL offset: " << journalOffset << ", rc: " << buildRc
            << ". Sequence number was: (" << d_primaryLeaseId << ", "
            << d_sequenceNum << "). Current storage "
            << "event builder size: " << d_storageEventBuilder.eventSize()
            << ", message count: " << d_storageEventBuilder.messageCount()
            << "." << MWCTSK_ALARMLOG_END;
        return;  // RETURN
    }

    // Flush if the builder is 'full' or if immediate flush is requested.
    flushIfNeeded(immediateFlush);
}

void FileStore::replicateRecord(bmqp::StorageMessageType::Enum type,
                                int                            flags,
                                bsls::Types::Uint64            journalOffset,
                                bsls::Types::Uint64            dataOffset,
                                unsigned int                   totalDataLen)
{
    BSLS_ASSERT_SAFE(bmqp::StorageMessageType::e_DATA == type ||
                     bmqp::StorageMessageType::e_QLIST == type);

    if (1 == clusterSize()) {
        return;  // RETURN
    }

    BSLS_ASSERT_SAFE(0 < d_fileSets.size());
    FileSet* activeFileSet = d_fileSets[0].get();
    BSLS_ASSERT_SAFE(activeFileSet);
    BSLS_ASSERT_SAFE(0 != journalOffset);

    const unsigned int journalOffsetWords = journalOffset /
                                            bmqp::Protocol::k_WORD_SIZE;

    bmqt::EventBuilderResult::Enum buildRc;
    MappedFileDescriptor&          journal = activeFileSet->d_journalFile;
    AliasedBufferDeleterSp         deleterSp =
        d_aliasedBufferDeleterSpPool.getObject();
    deleterSp->setFileSet(activeFileSet);

    bsl::shared_ptr<char> journalRecordBufferSp(deleterSp,
                                                journal.mapping() +
                                                    journalOffset);
    bdlbb::BlobBuffer     journalRecordBlobBuffer(
        journalRecordBufferSp,
        FileStoreProtocol::k_JOURNAL_RECORD_SIZE);

    bdlbb::BlobBuffer dataBlobBuffer;
    if (!d_isFSMWorkflow || bmqp::StorageMessageType::e_DATA == type) {
        MappedFileDescriptor& mfd = bmqp::StorageMessageType::e_DATA == type
                                        ? activeFileSet->d_dataFile
                                        : activeFileSet->d_qlistFile;

        bsl::shared_ptr<char> dataBufferSp(deleterSp,  // reuse same deleter
                                           mfd.mapping() + dataOffset);
        dataBlobBuffer.reset(dataBufferSp, totalDataLen);
    }
    bool flushAndRetry = false;
    do {
        if (bmqp::StorageMessageType::e_DATA == type) {
            buildRc = d_storageEventBuilder.packMessage(
                bmqp::StorageMessageType::e_DATA,
                d_config.partitionId(),
                flags,
                journalOffsetWords,
                journalRecordBlobBuffer,
                dataBlobBuffer);
        }
        else {
            if (d_isFSMWorkflow) {
                buildRc = d_storageEventBuilder.packMessage(
                    bmqp::StorageMessageType::e_QLIST,
                    d_config.partitionId(),
                    flags,
                    journalOffsetWords,
                    journalRecordBlobBuffer);
            }
            else {
                buildRc = d_storageEventBuilder.packMessage(
                    bmqp::StorageMessageType::e_QLIST,
                    d_config.partitionId(),
                    flags,
                    journalOffsetWords,
                    journalRecordBlobBuffer,
                    dataBlobBuffer);
            }
        }
        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
                buildRc == bmqt::EventBuilderResult::e_EVENT_TOO_BIG) &&
            !flushAndRetry) {
            flushIfNeeded(true);

            flushAndRetry = true;
        }
        else {
            flushAndRetry = false;
        }

    } while (flushAndRetry);

    if (bmqt::EventBuilderResult::e_SUCCESS != buildRc) {
        MWCTSK_ALARMLOG_ALARM("REPLICATION")
            << partitionDesc()
            << "Failed to pack storage record of type: " << type
            << ", of length "
            << mwcu::PrintUtil::prettyNumber(dataBlobBuffer.size())
            << ", at JOURNAL offset: "
            << mwcu::PrintUtil::prettyNumber(
                   static_cast<bsls::Types::Int64>(journalOffset))
            << ", rc: " << buildRc << ". Sequence number was: ("
            << mwcu::PrintUtil::prettyNumber(
                   static_cast<bsls::Types::Int64>(d_primaryLeaseId))
            << ", "
            << mwcu::PrintUtil::prettyNumber(
                   static_cast<bsls::Types::Int64>(d_sequenceNum))
            << "). Current storage event builder size: "
            << mwcu::PrintUtil::prettyNumber(d_storageEventBuilder.eventSize())
            << ", message count: "
            << mwcu::PrintUtil::prettyNumber(
                   d_storageEventBuilder.messageCount())
            << "." << MWCTSK_ALARMLOG_END;
        return;  // RETURN
    }

    // Flush if the builder is 'full'.
    flushIfNeeded(false);
}

void FileStore::deleteArchiveFilesCb()
{
    // executed by the scheduler's *DISPATCHER* thread

    FileStoreUtil::deleteArchiveFiles(d_config.partitionId(),
                                      d_config.archiveLocation(),
                                      d_config.maxArchivedFileSets(),
                                      d_config.clusterName());
}

int FileStore::validateWritingRecord(const bmqt::MessageGUID& guid,
                                     const mqbu::StorageKey&  queueKey)
{
    enum {
        rc_SUCCESS     = 0,
        rc_STOPPING    = -1,
        rc_UNAVAILABLE = -2,
        rc_NOT_PRIMARY = -3
    };

    FileSet* activeFileSet = d_fileSets[0].get();
    BSLS_ASSERT_SAFE(activeFileSet);

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(d_isStopping)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return rc_STOPPING;  // RETURN
    }

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            !activeFileSet->d_journalFileAvailable)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return rc_UNAVAILABLE;  // RETURN
    }

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!d_isPrimary)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        MWCTSK_ALARMLOG_ALARM("REPLICATION")
            << partitionDesc()
            << "Not the primary. Not writing & replicating record with guid "
            << guid << " and queueKey [" << queueKey << "]"
            << MWCTSK_ALARMLOG_END;
        return rc_NOT_PRIMARY;  // RETURN
    }

    return rc_SUCCESS;
}

void FileStore::replicateAndInsertDataStoreRecord(
    DataStoreRecordHandle*         handle,
    bmqp::StorageMessageType::Enum messageType,
    RecordType::Enum               recordType,
    bsls::Types::Uint64            recordOffset)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(handle);
    BSLS_ASSERT_SAFE(d_fileSets.size() > 0);

    replicateRecord(messageType, recordOffset);

    // Insert (key, record)
    DataStoreRecordKey key(d_sequenceNum, d_primaryLeaseId);
    DataStoreRecord    record(recordType, recordOffset);
    insertDataStoreRecord(handle, key, record);

    // Obtain 'activeFileSet'
    FileSet* activeFileSet = d_fileSets[0].get();
    BSLS_ASSERT_SAFE(activeFileSet);

    // Update outstanding JOURNAL bytes.
    activeFileSet->d_outstandingBytesJournal +=
        FileStoreProtocol::k_JOURNAL_RECORD_SIZE;
}

// PRIVATE ACCESSORS
void FileStore::aliasMessage(bsl::shared_ptr<bdlbb::Blob>* appData,
                             bsl::shared_ptr<bdlbb::Blob>* options,
                             const DataStoreRecord&        record) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(0 < d_fileSets.size());

    FileSet* activeFileSet = d_fileSets[0].get();
    BSLS_ASSERT_SAFE(activeFileSet);

    OffsetPtr<const DataHeader> dataHeader(activeFileSet->d_dataFile.block(),
                                           record.d_messageOffset);
    const unsigned int          dataHdrSize = dataHeader->headerWords() *
                                     bmqp::Protocol::k_WORD_SIZE;
    const bsls::Types::Uint64 optionsOffset = record.d_messageOffset +
                                              dataHdrSize;
    const bsls::Types::Uint64 optionsSize = static_cast<bsls::Types::Uint64>(
                                                dataHeader->optionsWords()) *
                                            bmqp::Protocol::k_WORD_SIZE;
    const bsls::Types::Uint64 appDataOffset = record.d_messageOffset +
                                              dataHdrSize + optionsSize;
    AliasedBufferDeleterSp deleter = d_aliasedBufferDeleterSpPool.getObject();
    deleter->setFileSet(activeFileSet);

    if (0 != optionsSize) {
        bsl::shared_ptr<char> optionsBufferSp(
            deleter,
            activeFileSet->d_dataFile.block().base() + optionsOffset);

        bdlbb::BlobBuffer optionsBlobBuffer(optionsBufferSp, optionsSize);

        *options = d_blobSpPool_p->getObject();
        (*options)->appendDataBuffer(optionsBlobBuffer);
    }

    bsl::shared_ptr<char> appDataBufferSp(
        deleter,
        activeFileSet->d_dataFile.block().base() + appDataOffset);

    bdlbb::BlobBuffer appDataBlobBuffer(appDataBufferSp,
                                        record.d_appDataUnpaddedLen);

    *appData = d_blobSpPool_p->getObject();
    (*appData)->appendDataBuffer(appDataBlobBuffer);
}

void FileStore::flushIfNeeded(bool immediateFlush)
{
    if (immediateFlush ||
        d_storageEventBuilder.messageCount() >= d_nagglePacketCount) {
        dispatcherFlush(true, false);

        notifyQueuesOnReplicatedBatch();
    }
}

// CREATORS
FileStore::FileStore(const DataStoreConfig&  config,
                     int                     processorId,
                     mqbi::Dispatcher*       dispatcher,
                     mqbnet::Cluster*        cluster,
                     mqbstat::ClusterStats*  clusterStats,
                     BlobSpPool*             blobSpPool,
                     StateSpPool*            statePool,
                     bdlmt::FixedThreadPool* miscWorkThreadPool,
                     bool                    isCSLModeEnabled,
                     bool                    isFSMWorkflow,
                     int                     replicationFactor,
                     bslma::Allocator*       allocator)
: d_allocator_p(allocator)
, d_allocators(allocator)
, d_storageAllocatorStore(d_allocators.get("QueueStorage"))
, d_config(config)
, d_partitionDescription(allocator)
, d_dispatcherClientData()
, d_clusterStats_p(clusterStats)
, d_blobSpPool_p(blobSpPool)
, d_statePool_p(statePool)
, d_aliasedBufferDeleterSpPool(1024, d_allocators.get("AliasedBufferDeleters"))
, d_isOpen(false)
, d_isStopping(false)
, d_flushWhenClosing(false)
, d_lastSyncPtReceived(false)
, d_records(10000, d_allocators.get("OutstandingRecords"))
, d_unreceipted(d_allocators.get("UnreceiptedRecords"))
, d_replicationFactor(replicationFactor)
, d_nodes(allocator)
, d_lastRecoveredStrongConsistency()
, d_fileSets(allocator)
, d_cluster_p(cluster)
, d_miscWorkThreadPool_p(miscWorkThreadPool)
, d_syncPointEventHandle()
, d_partitionHighwatermarkEventHandle()
, d_isPrimary(false)
, d_primaryNode_p(0)
, d_primaryLeaseId(0)
, d_sequenceNum(0)
, d_syncPoints(allocator)
, d_storages(allocator)
, d_isCSLModeEnabled(isCSLModeEnabled)
, d_isFSMWorkflow(isFSMWorkflow)
, d_ignoreCrc32c(false)
, d_nagglePacketCount(k_NAGLE_PACKET_COUNT)
, d_storageEventBuilder(FileStoreProtocol::k_VERSION,
                        bmqp::EventType::e_STORAGE,
                        config.bufferFactory(),
                        allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT(allocator);
    BSLS_ASSERT(d_cluster_p);
    BSLS_ASSERT(1 <= clusterSize());

    mwcu::MemOutStream os;
    os << "Partition [" << d_config.partitionId()
       << "] (cluster: " << d_cluster_p->name() << "): ";
    d_partitionDescription.assign(os.str().data(), os.str().length());

    dispatcher->registerClient(this,
                               mqbi::DispatcherClientType::e_QUEUE,
                               processorId);

    d_alarmSoftLimiter.initialize(1, 15 * bdlt::TimeUnitRatio::k_NS_PER_M);
    // Throttling of one maximum alarm per 15 minutes
}

FileStore::~FileStore()
{
    BSLS_ASSERT(!d_isOpen && "'close()' must be called before the destructor");

    // Unregister from the dispatcher
    dispatcher()->unregisterClient(this);
}

// MANIPULATORS
int FileStore::open(const QueueKeyInfoMap& queueKeyInfoMap)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(inDispatcherThread());

    enum {
        rc_SUCCESS                   = 0,
        rc_NON_RECOVERY_MODE_FAILURE = -1,
        rc_RECOVERY_MODE_FAILURE     = -2
    };

    BALL_LOG_INFO_BLOCK
    {
        BALL_LOG_OUTPUT_STREAM << partitionDesc()
                               << "Opening file store with config: \n";
        d_config.print(BALL_LOG_OUTPUT_STREAM);
    }

    if (d_isOpen) {
        BALL_LOG_INFO << partitionDesc() << "is already open.";
        return rc_SUCCESS;  // RETURN
    }

    mwcu::MemOutStream errorDescription;
    int rc = openInRecoveryMode(errorDescription, queueKeyInfoMap);
    if (rc == 0) {
        d_isOpen = true;
    }
    else if (rc == 1) {
        // Special rc indicating that no files were found to recover messages
        // from.

        BALL_LOG_INFO << partitionDesc() << "Recovery: no files found to "
                      << "recover messages. Attempting to open in "
                      << "'non-recovery' mode.";
        rc = openInNonRecoveryMode();
        if (0 != rc) {
            BALL_LOG_ERROR << partitionDesc() << "Recovery: failed to open in "
                           << "'non-recovery' mode, rc: " << rc;
            return rc * 10 + rc_NON_RECOVERY_MODE_FAILURE;  // RETURN
        }

        d_isOpen = true;
    }
    else {
        // Error opening file store in recovery mode.
        BALL_LOG_ERROR << partitionDesc() << "Failed to open in recovery mode,"
                       << " rc:" << rc << ", reason: ["
                       << errorDescription.str() << "].";
        return rc * 10 + rc_RECOVERY_MODE_FAILURE;  // RETURN
    }

    BSLS_ASSERT_SAFE(d_isOpen);

    // Report cluster's partition stats
    d_clusterStats_p->setPartitionOutstandingBytes(
        d_config.partitionId(),
        d_fileSets[0].get()->d_outstandingBytesData,
        d_fileSets[0].get()->d_outstandingBytesJournal);

    return rc_SUCCESS;
}

void FileStore::close(bool flush)
{
    if (!d_isOpen) {
        return;  // RETURN
    }

    d_isOpen             = false;
    d_isStopping         = false;
    d_flushWhenClosing   = flush;
    d_lastSyncPtReceived = false;

    d_config.scheduler()->cancelEventAndWait(&d_syncPointEventHandle);
    d_config.scheduler()->cancelEventAndWait(
        &d_partitionHighwatermarkEventHandle);
    // Ok to ignore rc above

    BALL_LOG_INFO << partitionDesc() << "Closing partition. ";

    // Clear 'd_records' so that gc logic is invoked on all mapped data files.
    // Note that logic will be invoked in this thread.  Note that data file of
    // active file set will not be gc'd because its alias blob buffer count
    // will not go to 0 as its initialized with 1.
    d_unreceipted.clear();
    d_records.clear();

    // After mapped data files have been gc'd, there should be only 1 file set
    // remaining in 'd_fileSets' (the active one).  Truncate and close it out.
    BSLS_ASSERT_SAFE(1 == d_fileSets.size());
    FileSet* activeFileSet = d_fileSets[0].get();
    truncate(activeFileSet);

    if (0 == --activeFileSet->d_aliasedBlobBufferCount) {
        BALL_LOG_INFO_BLOCK
        {
            BALL_LOG_OUTPUT_STREAM << partitionDesc() << "Closing file set ["
                                   << activeFileSet->d_dataFileName << "], ["
                                   << activeFileSet->d_journalFileName << "]";
            if (!d_isFSMWorkflow) {
                BALL_LOG_OUTPUT_STREAM
                    << ", [" << activeFileSet->d_qlistFileName << "]";
            }
            BALL_LOG_OUTPUT_STREAM << ".";
        }

        close(*activeFileSet, flush);
        d_fileSets.erase(d_fileSets.begin());
    }
    else {
        BALL_LOG_INFO << partitionDesc() << "Closing: number of references to"
                      << " active file set: "
                      << activeFileSet->d_aliasedBlobBufferCount;
        BSLS_ASSERT_SAFE(activeFileSet->d_dataFile.isValid());
        BSLS_ASSERT_SAFE(activeFileSet->d_journalFile.isValid());
        if (!d_isFSMWorkflow) {
            BSLS_ASSERT_SAFE(activeFileSet->d_qlistFile.isValid());
        }
    }
}

void FileStore::createStorage(bsl::shared_ptr<ReplicatedStorage>* storageSp,
                              const bmqt::Uri&                    queueUri,
                              const mqbu::StorageKey&             queueKey,
                              mqbi::Domain*                       domain)
{
    // executed by the *ANY* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(storageSp);
    BSLS_ASSERT_SAFE(domain);

    const mqbconfm::StorageDefinition& storageDef = domain->config().storage();
    const mqbconfm::Storage&           storageCfg = storageDef.config();

    BSLS_ASSERT_SAFE(!storageCfg.isUndefinedValue());

    bslma::Allocator* storageAlloc = d_storageAllocatorStore.baseAllocator();
    if (storageCfg.isInMemoryValue()) {
        storageSp->reset(new (*storageAlloc)
                             InMemoryStorage(queueUri,
                                             queueKey,
                                             config().partitionId(),
                                             domain->config(),
                                             domain->capacityMeter(),
                                             storageAlloc,
                                             &d_storageAllocatorStore),
                         storageAlloc);
    }
    else if (storageCfg.isFileBackedValue()) {
        storageSp->reset(new (*storageAlloc)
                             FileBackedStorage(this,
                                               queueUri,
                                               queueKey,
                                               domain->config(),
                                               domain->capacityMeter(),
                                               storageAlloc,
                                               &d_storageAllocatorStore),
                         storageAlloc);
    }
    else {
        BSLS_ASSERT_OPT(false && "Unknown storage type");
    }
}

int FileStore::writeMessageRecord(mqbi::StorageMessageAttributes* attributes,
                                  DataStoreRecordHandle*          handle,
                                  const bmqt::MessageGUID&        guid,
                                  const bsl::shared_ptr<bdlbb::Blob>& appData,
                                  const bsl::shared_ptr<bdlbb::Blob>& options,
                                  const mqbu::StorageKey&             queueKey)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(handle);
    BSLS_ASSERT_SAFE(!queueKey.isNull());
    BSLS_ASSERT_SAFE(0 < d_fileSets.size());

    enum {
        rc_SUCCESS          = 0,
        rc_STOPPING         = -1,
        rc_UNAVAILABLE      = -2,
        rc_DATA_FILE_FULL   = -3,
        rc_NOT_PRIMARY      = -4,
        rc_ROLLOVER_FAILURE = -5,
        rc_PARTITION_FULL   = -6
    };

    FileSet* activeFileSet = d_fileSets[0].get();
    BSLS_ASSERT_SAFE(activeFileSet);

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(d_isStopping)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return rc_STOPPING;  // RETURN
    }

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            !activeFileSet->d_journalFileAvailable)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return rc_UNAVAILABLE;  // RETURN
    }

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!d_isPrimary)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        MWCTSK_ALARMLOG_ALARM("REPLICATION")
            << partitionDesc()
            << "Not the primary. Not writing & replicating message with guid "
            << guid << " and queueKey [" << queueKey << "]"
            << MWCTSK_ALARMLOG_END;
        return rc_NOT_PRIMARY;  // RETURN
    }

    int optionsSize = 0;
    if (options) {
        optionsSize = options->length();
    }

    int numBytesPadding = 0;
    bmqp::ProtocolUtil::calcNumDwordsAndPadding(
        &numBytesPadding,
        sizeof(DataHeader) + optionsSize + appData->length());

    unsigned int totalLength = sizeof(DataHeader) +
                               static_cast<unsigned int>(optionsSize) +
                               static_cast<unsigned int>(appData->length()) +
                               static_cast<unsigned int>(numBytesPadding);

    // Roll over data file if needed.
    int rc = rolloverIfNeeded(FileType::e_DATA,
                              activeFileSet->d_dataFile,
                              activeFileSet->d_dataFileName,
                              activeFileSet->d_dataFilePosition,
                              totalLength);
    if (0 != rc) {
        return 10 * rc + rc_ROLLOVER_FAILURE;  // RETURN
    }

    // Update 'activeFileSet' as it may have rolled over above.
    activeFileSet = d_fileSets[0].get();
    BSLS_ASSERT_SAFE(activeFileSet->d_dataFile.fileSize() >=
                     (activeFileSet->d_dataFilePosition + totalLength));

    // Roll over journal if needed.
    rc = rolloverIfNeeded(FileType::e_JOURNAL,
                          activeFileSet->d_journalFile,
                          activeFileSet->d_journalFileName,
                          activeFileSet->d_journalFilePosition,
                          k_REQUESTED_JOURNAL_SPACE);
    if (0 != rc) {
        return 10 * rc + rc_ROLLOVER_FAILURE;  // RETURN
    }

    // If 'd_replicationFactor' is 1, then the message need not be persisted to
    // any replicas (i.e. eventual consistency). Therefore the writing of the
    // message by this node is sufficient to set the receipt.
    if (1 == d_replicationFactor && !attributes->hasReceipt()) {
        attributes->setReceipt(true);
    }

    // Update 'activeFileSet' as it may have rolled over above.
    activeFileSet = d_fileSets[0].get();

    // Local refs for convenience.

    MappedFileDescriptor& dataFile    = activeFileSet->d_dataFile;
    bsls::Types::Uint64&  dataFilePos = activeFileSet->d_dataFilePosition;
    MappedFileDescriptor& journal     = activeFileSet->d_journalFile;
    bsls::Types::Uint64&  journalPos  = activeFileSet->d_journalFilePosition;

    BSLS_ASSERT_SAFE(journal.fileSize() >=
                     (journalPos + k_REQUESTED_JOURNAL_SPACE));

    // All good.  Take current offset in data file.
    bsls::Types::Uint64 dataOffset = dataFilePos;
    BSLS_ASSERT_SAFE(0 == dataOffset % bmqp::Protocol::k_DWORD_SIZE);

    // Append DataHeader to data file
    OffsetPtr<DataHeader> dataHeader(dataFile.block(), dataFilePos);
    new (dataHeader.get()) DataHeader();

    int dhFlags = dataHeader->flags();

    dataHeader->setMessageWords(totalLength / bmqp::Protocol::k_WORD_SIZE)
        .setOptionsWords(optionsSize / bmqp::Protocol::k_WORD_SIZE)
        .setFlags(dhFlags);
    attributes->messagePropertiesInfo().applyTo(dataHeader.get());
    dataFilePos += sizeof(DataHeader);

    // Append options, if any, to data file.
    if (options) {
        bdlbb::BlobUtil::copy(dataFile.mapping() + dataFilePos,
                              *options,
                              0,  // start offset in blob
                              options->length());
        dataFilePos += static_cast<unsigned int>(options->length());
    }

    // Append appData to data file.
    bdlbb::BlobUtil::copy(dataFile.mapping() + dataFilePos,
                          *appData,
                          0,  // start offset in blob
                          appData->length());

    dataFilePos += static_cast<unsigned int>(appData->length());

    // Append padding to data file
    bmqp::ProtocolUtil::appendPaddingDwordRaw(dataFile.mapping() + dataFilePos,
                                              numBytesPadding);
    dataFilePos += static_cast<unsigned int>(numBytesPadding);

    // Append message record to journal.
    BSLS_ASSERT_SAFE(journal.fileSize() >=
                     (journalPos + k_REQUESTED_JOURNAL_SPACE));

    // Append MessageRecord to journal
    bsls::Types::Uint64      journalOffset = journalPos;
    OffsetPtr<MessageRecord> msgRec(journal.block(), journalPos);
    new (msgRec.get()) MessageRecord();
    msgRec->header()
        .setPrimaryLeaseId(d_primaryLeaseId)
        .setSequenceNumber(++d_sequenceNum)
        .setTimestamp(attributes->arrivalTimestamp());
    msgRec->setRefCount(attributes->refCount())
        .setQueueKey(queueKey)
        .setFileKey(activeFileSet->d_dataFileKey)
        .setMessageOffsetDwords(dataOffset / bmqp::Protocol::k_DWORD_SIZE)
        .setMessageGUID(guid)
        .setCrc32c(attributes->crc32c())
        .setCompressionAlgorithmType(attributes->compressionAlgorithmType())
        .setMagic(RecordHeader::k_MAGIC);
    journalPos += FileStoreProtocol::k_JOURNAL_RECORD_SIZE;

    DataStoreRecordKey key(d_sequenceNum, d_primaryLeaseId);
    DataStoreRecord    record(RecordType::e_MESSAGE, journalOffset);
    record.d_messageOffset      = dataOffset;
    record.d_appDataUnpaddedLen = static_cast<unsigned int>(appData->length());
    record.d_dataOrQlistRecordPaddedLen = totalLength;
    record.d_messagePropertiesInfo      = attributes->messagePropertiesInfo();
    record.d_hasReceipt                 = attributes->hasReceipt();
    record.d_arrivalTimepoint           = attributes->arrivalTimepoint();
    record.d_arrivalTimestamp           = attributes->arrivalTimestamp();

    RecordIterator recordIt;
    insertDataStoreRecord(&recordIt, key, record);
    recordIteratorToHandle(handle, recordIt);

    int flags = 0;
    // If this requires Receipt
    if (!attributes->hasReceipt()) {
        d_unreceipted.insert(
            bsl::make_pair(key,
                           ReceiptContext(queueKey,
                                          guid,
                                          recordIt,
                                          1,  // receipt count
                                          attributes->queueHandle())));
        flags = bmqp::StorageHeaderFlags::e_RECEIPT_REQUESTED;
    }

    // Replicate the message.
    replicateRecord(bmqp::StorageMessageType::e_DATA,
                    flags,
                    journalOffset,
                    dataOffset,
                    totalLength);

    // Update outstanding JOURNAL and DATA bytes.
    activeFileSet->d_outstandingBytesJournal +=
        FileStoreProtocol::k_JOURNAL_RECORD_SIZE;
    activeFileSet->d_outstandingBytesData += totalLength;

    return rc_SUCCESS;
}

int FileStore::writeQueueCreationRecord(DataStoreRecordHandle*  handle,
                                        const bmqt::Uri&        queueUri,
                                        const mqbu::StorageKey& queueKey,
                                        const AppIdKeyPairs&    appIdKeyPairs,
                                        bsls::Types::Uint64     timestamp,
                                        bool                    isNewQueue)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(handle);
    BSLS_ASSERT_SAFE(!queueUri.asString().empty());
    BSLS_ASSERT_SAFE(!queueKey.isNull());
    BSLS_ASSERT_SAFE(d_fileSets.size() > 0);

    enum {
        rc_SUCCESS          = 0,
        rc_STOPPING         = -1,
        rc_UNAVAILABLE      = -2,
        rc_NOT_PRIMARY      = -3,
        rc_ROLLOVER_FAILURE = -4,
        rc_PARTITION_FULL   = -5
    };

    FileSet* activeFileSet = d_fileSets[0].get();
    BSLS_ASSERT_SAFE(activeFileSet);

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(d_isStopping)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return rc_STOPPING;  // RETURN
    }

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            !activeFileSet->d_journalFileAvailable)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return rc_UNAVAILABLE;  // RETURN
    }

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!d_isPrimary)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        MWCTSK_ALARMLOG_ALARM("REPLICATION")
            << partitionDesc()
            << "Not the primary. Not writing & replicating QueueUriRecord for "
            << "uri: " << queueUri << " and queueKey [" << queueKey << "]"
            << MWCTSK_ALARMLOG_END;
        return rc_NOT_PRIMARY;  // RETURN
    }

    int          queueUriPadding = 0;
    int          queueUriWords   = 0;
    unsigned int totalLength     = 0;
    int          rc              = 0;

    // AppIds and AppKeys.
    bsl::vector<int> appIdPaddings(appIdKeyPairs.size(), 0);
    bsl::vector<int> appIdWords(appIdKeyPairs.size(), 0);

    if (!d_isFSMWorkflow) {
        // QueueUri and QueueKey
        queueUriWords = bmqp::ProtocolUtil::calcNumWordsAndPadding(
            &queueUriPadding,
            queueUri.asString().length());

        totalLength = sizeof(QueueRecordHeader) +
                      queueUri.asString().length() + queueUriPadding +
                      FileStoreProtocol::k_HASH_LENGTH;

        for (size_t i = 0; i < appIdKeyPairs.size(); ++i) {
            const AppIdKeyPair& appIdKeyPair = appIdKeyPairs[i];
            BSLS_ASSERT_SAFE(!appIdKeyPair.first.empty());
            BSLS_ASSERT_SAFE(!appIdKeyPair.second.isNull());
            appIdWords[i] = bmqp::ProtocolUtil::calcNumWordsAndPadding(
                &appIdPaddings[i],
                appIdKeyPair.first.length());
            totalLength += sizeof(AppIdHeader) + appIdKeyPair.first.length() +
                           appIdPaddings[i] +
                           FileStoreProtocol::k_HASH_LENGTH;  // for AppKey
        }

        totalLength += sizeof(unsigned int);  // magic length

        // Roll over QLIST file if needed.

        rc = rolloverIfNeeded(FileType::e_QLIST,
                              activeFileSet->d_qlistFile,
                              activeFileSet->d_qlistFileName,
                              activeFileSet->d_qlistFilePosition,
                              totalLength);
        if (0 != rc) {
            return 10 * rc + rc_ROLLOVER_FAILURE;  // RETURN
        }
    }

    // Update 'activeFileSet' as it may have rolled over above.

    activeFileSet = d_fileSets[0].get();
    if (!d_isFSMWorkflow) {
        BSLS_ASSERT_SAFE(activeFileSet->d_qlistFile.fileSize() >=
                         (activeFileSet->d_qlistFilePosition + totalLength));
    }

    // Roll over journal if needed.

    rc = rolloverIfNeeded(FileType::e_JOURNAL,
                          activeFileSet->d_journalFile,
                          activeFileSet->d_journalFileName,
                          activeFileSet->d_journalFilePosition,
                          k_REQUESTED_JOURNAL_SPACE);
    if (0 != rc) {
        return 10 * rc + rc_ROLLOVER_FAILURE;  // RETURN
    }

    // Update 'activeFileSet' as it may have rolled over above.

    activeFileSet = d_fileSets[0].get();

    // Local refs for convenience.

    MappedFileDescriptor& journal    = activeFileSet->d_journalFile;
    bsls::Types::Uint64&  journalPos = activeFileSet->d_journalFilePosition;
    MappedFileDescriptor  nullMfd;
    MappedFileDescriptor& qlistFile    = activeFileSet->d_qlistFile;
    bsls::Types::Uint64&  qlistFilePos = activeFileSet->d_qlistFilePosition;

    BSLS_ASSERT_SAFE(journal.fileSize() >=
                     (journalPos + k_REQUESTED_JOURNAL_SPACE));

    // All good.  Take current offset in QLIST file.

    bsls::Types::Uint64 qlistOffset = d_isFSMWorkflow ? 0 : qlistFilePos;
    BSLS_ASSERT_SAFE(0 == qlistOffset % bmqp::Protocol::k_WORD_SIZE);

    if (!d_isFSMWorkflow) {
        // Append QueueRecordHeader to QLIST file.

        OffsetPtr<QueueRecordHeader> qrh(qlistFile.block(), qlistFilePos);
        new (qrh.get()) QueueRecordHeader();
        qrh->setQueueUriLengthWords(queueUriWords)
            .setNumAppIds(appIdKeyPairs.size())
            .setQueueRecordWords(totalLength / bmqp::Protocol::k_WORD_SIZE);
        qlistFilePos += sizeof(QueueRecordHeader);

        // Append QueueRecord to QLIST file.

        // 1) Append QueueUri
        OffsetPtr<char> quri(qlistFile.block(), qlistFilePos);
        bsl::memcpy(quri.get(),
                    queueUri.asString().c_str(),
                    queueUri.asString().length());
        qlistFilePos += queueUri.asString().length();

        // 2) Append padding after QueueUri
        bmqp::ProtocolUtil::appendPaddingRaw(qlistFile.mapping() +
                                                 qlistFilePos,
                                             queueUriPadding);
        qlistFilePos += queueUriPadding;

        // 3) Append QueueUriHash.  QueueUriRecord needs
        // 'mqbs::FileStoreProtocol::k_HASH_LENGTH' characters for queue uri
        // hash and appId hash but 'queueKey' is only
        // mqbu::StorageKey::e_KEY_LENGTH_BINARY' long.  So we create an array
        // of the right length, zero it out, and copy only 'queueKey' worth of
        // data.

        char queueHash[mqbs::FileStoreProtocol::k_HASH_LENGTH] = {0};
        bsl::memcpy(queueHash, queueKey.data(), k_KEY_LEN);
        OffsetPtr<char> quriHash(qlistFile.block(), qlistFilePos);
        bsl::memcpy(quriHash.get(),
                    queueHash,
                    FileStoreProtocol::k_HASH_LENGTH);
        qlistFilePos += FileStoreProtocol::k_HASH_LENGTH;

        // 3) Append AppIds and AppKeys
        for (size_t i = 0; i < appIdKeyPairs.size(); ++i) {
            // Append AppIdHeader.

            OffsetPtr<AppIdHeader> appIdHeader(qlistFile.block(),
                                               qlistFilePos);
            new (appIdHeader.get()) AppIdHeader;
            appIdHeader->setAppIdLengthWords(appIdWords[i]);
            qlistFilePos += sizeof(AppIdHeader);

            // Append AppId.

            OffsetPtr<char> appId(qlistFile.block(), qlistFilePos);
            bsl::memcpy(appId.get(),
                        appIdKeyPairs[i].first.c_str(),
                        appIdKeyPairs[i].first.length());
            qlistFilePos += appIdKeyPairs[i].first.length();

            // Append padding after AppId.

            bmqp::ProtocolUtil::appendPaddingRaw(qlistFile.mapping() +
                                                     qlistFilePos,
                                                 appIdPaddings[i]);
            qlistFilePos += appIdPaddings[i];

            // Append AppIdHash (see 'Append QueueUriHash' comments section
            // above for explanation).

            char appIdHash[mqbs::FileStoreProtocol::k_HASH_LENGTH] = {0};
            bsl::memcpy(appIdHash, appIdKeyPairs[i].second.data(), k_KEY_LEN);
            OffsetPtr<char> appHash(qlistFile.block(), qlistFilePos);
            bsl::memcpy(appHash.get(),
                        appIdHash,
                        FileStoreProtocol::k_HASH_LENGTH);
            qlistFilePos += FileStoreProtocol::k_HASH_LENGTH;
        }

        // 4) Append magic bits
        OffsetPtr<bdlb::BigEndianUint32> magic(qlistFile.block(),
                                               qlistFilePos);
        *magic = QueueRecordHeader::k_MAGIC;
        qlistFilePos += sizeof(QueueRecordHeader::k_MAGIC);
    }

    // Append QueueOpRecord to JOURNAL file.

    bsls::Types::Uint64      recordOffset = journalPos;
    OffsetPtr<QueueOpRecord> queueOpRec(journal.block(), journalPos);
    new (queueOpRec.get()) QueueOpRecord();
    queueOpRec->header()
        .setPrimaryLeaseId(d_primaryLeaseId)
        .setSequenceNumber(++d_sequenceNum)
        .setTimestamp(timestamp);
    queueOpRec->setQueueKey(queueKey).setType(
        isNewQueue ? QueueOpType::e_CREATION : QueueOpType::e_ADDITION);
    if (!d_isFSMWorkflow) {
        queueOpRec->setQueueUriRecordOffsetWords(qlistOffset /
                                                 bmqp::Protocol::k_WORD_SIZE);
    }

    // Note that we don't write any appKey to the QueueOpRecord in the journal,
    // because there could be multiple appKeys specified, and its not possible
    // to capture all of them in the journal.  We could specify the appKey if
    // 'appKeys.size() == 1', but that may lead to confusion.

    queueOpRec->setMagic(RecordHeader::k_MAGIC);
    journalPos += FileStoreProtocol::k_JOURNAL_RECORD_SIZE;

    replicateRecord(bmqp::StorageMessageType::e_QLIST,
                    0,  // flags
                    recordOffset,
                    qlistOffset,
                    totalLength);

    DataStoreRecordKey key(d_sequenceNum, d_primaryLeaseId);
    DataStoreRecord record(RecordType::e_QUEUE_OP, recordOffset, totalLength);
    insertDataStoreRecord(handle, key, record);

    // Update outstanding JOURNAL and QLIST bytes.
    activeFileSet->d_outstandingBytesJournal +=
        FileStoreProtocol::k_JOURNAL_RECORD_SIZE;
    if (!d_isFSMWorkflow) {
        activeFileSet->d_outstandingBytesQlist += totalLength;
    }

    return rc_SUCCESS;
}

int FileStore::writeQueuePurgeRecord(DataStoreRecordHandle*  handle,
                                     const mqbu::StorageKey& queueKey,
                                     const mqbu::StorageKey& appKey,
                                     bsls::Types::Uint64     timestamp)
{
    return writeQueueOpRecord(handle,
                              queueKey,
                              appKey,
                              QueueOpType::e_PURGE,
                              timestamp);
}

int FileStore::writeQueueDeletionRecord(DataStoreRecordHandle*  handle,
                                        const mqbu::StorageKey& queueKey,
                                        const mqbu::StorageKey& appKey,
                                        bsls::Types::Uint64     timestamp)
{
    return writeQueueOpRecord(handle,
                              queueKey,
                              appKey,
                              QueueOpType::e_DELETION,
                              timestamp);
}

int FileStore::writeConfirmRecord(DataStoreRecordHandle*   handle,
                                  const bmqt::MessageGUID& guid,
                                  const mqbu::StorageKey&  queueKey,
                                  const mqbu::StorageKey&  appKey,
                                  bsls::Types::Uint64      timestamp,
                                  ConfirmReason::Enum      reason)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(handle);
    BSLS_ASSERT_SAFE(!queueKey.isNull());
    BSLS_ASSERT_SAFE(0 < d_fileSets.size());

    enum {
        rc_SUCCESS            = 0,
        rc_VALIDATION_FAILURE = -1,
        rc_ROLLOVER_FAILURE   = -2
    };

    int rc = validateWritingRecord(guid, queueKey);
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(rc != 0)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return 10 * rc + rc_VALIDATION_FAILURE;  // RETURN
    }

    // Obtain 'activeFileSet'
    FileSet* activeFileSet = d_fileSets[0].get();

    // Roll over if needed
    rc = rolloverIfNeeded(FileType::e_JOURNAL,
                          activeFileSet->d_journalFile,
                          activeFileSet->d_journalFileName,
                          activeFileSet->d_journalFilePosition,
                          k_REQUESTED_JOURNAL_SPACE);
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(rc != 0)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return 10 * rc + rc_ROLLOVER_FAILURE;  // RETURN
    }

    // Update 'activeFileSet' as it may have rolled over above.
    activeFileSet = d_fileSets[0].get();

    // Local refs for convenience
    MappedFileDescriptor& journal    = activeFileSet->d_journalFile;
    bsls::Types::Uint64&  journalPos = activeFileSet->d_journalFilePosition;

    BSLS_ASSERT_SAFE(journal.fileSize() >=
                     (journalPos + k_REQUESTED_JOURNAL_SPACE));

    // Append confirm record to journal.
    bsls::Types::Uint64      recordOffset = journalPos;
    OffsetPtr<ConfirmRecord> confRec(journal.block(), journalPos);
    new (confRec.get()) ConfirmRecord();
    confRec->header()
        .setPrimaryLeaseId(d_primaryLeaseId)
        .setSequenceNumber(++d_sequenceNum)
        .setTimestamp(timestamp);
    confRec->setQueueKey(queueKey).setMessageGUID(guid);

    confRec->setReason(reason);

    if (!appKey.isNull()) {
        confRec->setAppKey(appKey);
    }
    confRec->setMagic(RecordHeader::k_MAGIC);
    journalPos += FileStoreProtocol::k_JOURNAL_RECORD_SIZE;

    replicateAndInsertDataStoreRecord(handle,
                                      bmqp::StorageMessageType::e_CONFIRM,
                                      RecordType::e_CONFIRM,
                                      recordOffset);

    return rc_SUCCESS;
}

int FileStore::writeDeletionRecord(const bmqt::MessageGUID& guid,
                                   const mqbu::StorageKey&  queueKey,
                                   DeletionRecordFlag::Enum deletionFlag,
                                   bsls::Types::Uint64      timestamp)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!queueKey.isNull());
    BSLS_ASSERT_SAFE(0 < d_fileSets.size());

    enum {
        rc_SUCCESS            = 0,
        rc_VALIDATION_FAILURE = -1,
        rc_ROLLOVER_FAILURE   = -2
    };

    int rc = validateWritingRecord(guid, queueKey);
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(rc != 0)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return 10 * rc + rc_VALIDATION_FAILURE;  // RETURN
    }

    // Obtain 'activeFileSet'
    FileSet* activeFileSet = d_fileSets[0].get();

    // Roll over if needed
    rc = rolloverIfNeeded(FileType::e_JOURNAL,
                          activeFileSet->d_journalFile,
                          activeFileSet->d_journalFileName,
                          activeFileSet->d_journalFilePosition,
                          k_REQUESTED_JOURNAL_SPACE);
    if (rc != 0) {
        return 10 * rc + rc_ROLLOVER_FAILURE;  // RETURN
    }

    // Update 'activeFileSet' as it may have rolled over above.
    activeFileSet = d_fileSets[0].get();

    // Local refs for convenience.
    MappedFileDescriptor& journal    = activeFileSet->d_journalFile;
    bsls::Types::Uint64&  journalPos = activeFileSet->d_journalFilePosition;

    BSLS_ASSERT_SAFE(journal.fileSize() >=
                     (journalPos + k_REQUESTED_JOURNAL_SPACE));

    // Append deletion record to journal.
    bsls::Types::Uint64       recordOffset = journalPos;
    OffsetPtr<DeletionRecord> delRec(journal.block(), journalPos);
    new (delRec.get()) DeletionRecord();
    delRec->header()
        .setPrimaryLeaseId(d_primaryLeaseId)
        .setSequenceNumber(++d_sequenceNum)
        .setTimestamp(timestamp);
    delRec->setDeletionRecordFlag(deletionFlag)
        .setQueueKey(queueKey)
        .setMessageGUID(guid)
        .setMagic(RecordHeader::k_MAGIC);
    journalPos += FileStoreProtocol::k_JOURNAL_RECORD_SIZE;

    replicateRecord(bmqp::StorageMessageType::e_DELETION, recordOffset);

    return rc_SUCCESS;
}

int FileStore::writeSyncPointRecord(const bmqp_ctrlmsg::SyncPoint& syncPoint,
                                    SyncPointType::Enum            type)
{
    enum { rc_SUCCESS = 0, rc_UNAVAILABLE = -1 };

    BSLS_ASSERT_SAFE(0 < d_fileSets.size());
    FileSet* activeFileSet = d_fileSets[0].get();
    BSLS_ASSERT_SAFE(activeFileSet);

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            !activeFileSet->d_journalFileAvailable)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return rc_UNAVAILABLE;  // RETURN
    }

    // Local refs for convenience.

    MappedFileDescriptor& journal    = activeFileSet->d_journalFile;
    bsls::Types::Uint64&  journalPos = activeFileSet->d_journalFilePosition;

    // Append journalOp record to journal.  Journal should already have enough
    // space for one sync point record.

    BSLS_ASSERT_SAFE(journal.fileSize() >=
                     (journalPos + FileStoreProtocol::k_JOURNAL_RECORD_SIZE));

    OffsetPtr<JournalOpRecord> journalOpRec(journal.block(), journalPos);
    new (journalOpRec.get())
        JournalOpRecord(JournalOpType::e_SYNCPOINT,
                        type,
                        syncPoint.sequenceNum(),
                        d_config.nodeId(),
                        syncPoint.primaryLeaseId(),
                        syncPoint.dataFileOffsetDwords(),
                        d_isFSMWorkflow ? 0 : syncPoint.qlistFileOffsetWords(),
                        RecordHeader::k_MAGIC);
    journalOpRec->header()
        .setPrimaryLeaseId(d_primaryLeaseId)
        .setSequenceNumber(d_sequenceNum)
        .setTimestamp(
            bdlt::EpochUtil::convertToTimeT64(bdlt::CurrentTime::utc()));
    journalPos += FileStoreProtocol::k_JOURNAL_RECORD_SIZE;

    // Don't update outstanding journal bytes because it is a SyncPt, and we
    // don't rollover SyncPts.

    return rc_SUCCESS;
}

int FileStore::removeRecord(const DataStoreRecordHandle& handle)
{
    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS = 0  // Success
        ,
        rc_INVALID_HANDLE = -1  // Invalid handle
        ,
        rc_HANDLE_NOT_FOUND = -2  // Handle not found
    };

    if (!handle.isValid()) {
        return rc_INVALID_HANDLE;  // RETURN
    }

    const RecordIterator& recordIt = *reinterpret_cast<const RecordIterator*>(
        &handle);
    if (d_records.end() == d_records.find(recordIt->first)) {
        return rc_HANDLE_NOT_FOUND;  // RETURN
    }

    removeRecordRaw(handle);
    return rc_SUCCESS;
}

void FileStore::removeRecordRaw(const DataStoreRecordHandle& handle)
{
    BSLS_ASSERT_SAFE(handle.isValid());

    const RecordIterator& recordIt = *reinterpret_cast<const RecordIterator*>(
        &handle);
    BSLS_ASSERT_SAFE(d_records.end() != d_records.find(recordIt->first));

    FileSet*               activeFileSet = d_fileSets[0].get();
    const DataStoreRecord& record        = recordIt->second;

    activeFileSet->d_outstandingBytesJournal -=
        FileStoreProtocol::k_JOURNAL_RECORD_SIZE;

    if (RecordType::e_MESSAGE == record.d_recordType) {
        activeFileSet->d_outstandingBytesData -=
            record.d_dataOrQlistRecordPaddedLen;
        cancelUnreceipted(recordIt->first);
    }
    else if (RecordType::e_QUEUE_OP == record.d_recordType) {
        if (!d_isFSMWorkflow) {
            activeFileSet->d_outstandingBytesQlist -=
                record.d_dataOrQlistRecordPaddedLen;
        }
    }

    d_records.erase(recordIt);
}

void FileStore::processStorageEvent(const bsl::shared_ptr<bdlbb::Blob>& blob,
                                    bool                 isPartitionSyncEvent,
                                    mqbnet::ClusterNode* source)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(blob);

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(d_isStopping &&
                                              d_lastSyncPtReceived)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        // Self node is stopping and has received the "last" SyncPt.  No need
        // to process any more storage events.

        return;  // RETURN
    }

    bmqp::Event                  rawEvent(blob.get(), d_allocator_p);
    bmqp::StorageMessageIterator iter;

    rawEvent.loadStorageMessageIterator(&iter);
    BSLS_ASSERT_SAFE(iter.isValid());

    if (1 != iter.next()) {
        return;  // RETURN
    }
    const unsigned int      pid         = iter.header().partitionId();
    FileStore::NodeContext* nodeContext = 0;

    do {
        const bmqp::StorageHeader& header = iter.header();
        if (pid != header.partitionId()) {
            // A storage event is sent by 'source' cluster node.  The node may
            // be primary for one or more partitions, but as per the BlazingMQ
            // replication design, *all* messages in this event will belong to
            // the *same* partition.  Any exception to this is a bug in the
            // implementation of replication.

            MWCTSK_ALARMLOG_ALARM("STORAGE")
                << partitionDesc() << ": Received storage event from node "
                << source->nodeDescription() << " with"
                << " different PartitionId: [" << pid << "] vs ["
                << header.partitionId() << "]" << ". Ignoring storage event."
                << MWCTSK_ALARMLOG_END;
            continue;  // CONTINUE
        }

        mwcu::BlobPosition                        recordPosition;
        mwcu::BlobObjectProxy<mqbs::RecordHeader> recHeader;

        int rc = mqbs::StorageUtil::loadRecordHeaderAndPos(&recHeader,
                                                           &recordPosition,
                                                           iter,
                                                           blob,
                                                           partitionDesc());
        if (rc != 0) {
            return;  // RETURN
        }

        // Check sequence number (only if leaseId is same).  Received leaseId
        // cannot be smaller; it can, however, be larger if self node just
        // recovered and is applying buffered storage events after being
        // opened, or received this storage event as part of 'partition sync'
        // step from the new (passive) primary.

        if (d_primaryLeaseId > recHeader->primaryLeaseId()) {
            MWCTSK_ALARMLOG_ALARM("REPLICATION")
                << partitionDesc() << "Received storage msg "
                << header.messageType()
                << " with smaller leaseId in RecordHeader: "
                << recHeader->primaryLeaseId()
                << ", self leaseId: " << d_primaryLeaseId
                << ". SeqNum in RecordHeader: " << recHeader->sequenceNumber()
                << ", self seqNum: " << d_sequenceNum
                << " Record's journal offset (in words): "
                << header.journalOffsetWords() << ". Ignoring entire event."
                << MWCTSK_ALARMLOG_END;
            return;  // RETURN
        }

        if (d_primaryLeaseId == recHeader->primaryLeaseId()) {
            if (recHeader->sequenceNumber() != (d_sequenceNum + 1)) {
                MWCTSK_ALARMLOG_ALARM("REPLICATION")
                    << partitionDesc() << "Received storage msg "
                    << header.messageType()
                    << " with missing sequence num. Expected: "
                    << (d_sequenceNum + 1)
                    << ", received: " << recHeader->sequenceNumber()
                    << ". PrimaryLeaseId: " << recHeader->primaryLeaseId()
                    << " Record's journal offset (in words): "
                    << header.journalOffsetWords() << ". Aborting broker."
                    << MWCTSK_ALARMLOG_END;
                mqbu::ExitUtil::terminate(
                    mqbu::ExitCode::e_STORAGE_OUT_OF_SYNC);
                // EXIT
            }
        }

        if (bmqp::StorageMessageType::e_DATA == header.messageType()) {
            rc = writeMessageRecord(header, *recHeader, blob, recordPosition);

            if (BSLS_PERFORMANCEHINT_PREDICT_LIKELY(0 == rc)) {
                if (header.flags() &
                    bmqp::StorageHeaderFlags::e_RECEIPT_REQUESTED) {
                    nodeContext = generateReceipt(nodeContext,
                                                  source,
                                                  recHeader->primaryLeaseId(),
                                                  recHeader->sequenceNumber());
                }
            }
        }
        else if (bmqp::StorageMessageType::e_QLIST == header.messageType()) {
            rc = writeQueueCreationRecord(header,
                                          *recHeader,
                                          blob,
                                          recordPosition);
        }
        else {
            rc = writeJournalRecord(header,
                                    *recHeader,
                                    blob,
                                    recordPosition,
                                    header.messageType());
        }

        // Bump up the current sequence number if record was written
        // successfully, raise alarm otherwise.

        if (BSLS_PERFORMANCEHINT_PREDICT_LIKELY(0 == rc)) {
            d_sequenceNum = recHeader->sequenceNumber();

            if (isPartitionSyncEvent) {
                // If we are processing a partition-sync event, we have to
                // bump up the leaseId to that of the message, because we don't
                // get a separate notification about leaseId (unlike in steady
                // state when StorageMgr invokes fs.setActivePrimary()).

                d_primaryLeaseId = recHeader->primaryLeaseId();

                BALL_LOG_INFO
                    << partitionDesc()
                    << "Bumped up primaryLeaseId to: " << d_primaryLeaseId
                    << " while processing a "
                    << "partition-sync storage message. "
                    << "SequenceNum: " << d_sequenceNum;
            }
        }
        else {
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
            MWCTSK_ALARMLOG_ALARM("REPLICATION")
                << partitionDesc() << "Failed to write storage msg "
                << header.messageType() << " with sequence number ("
                << recHeader->primaryLeaseId() << ", "
                << recHeader->sequenceNumber()
                << "), journal offset words: " << header.journalOffsetWords()
                << ", rc: " << rc << ". Ignoring this message."
                << MWCTSK_ALARMLOG_END;
        }
    } while (1 == iter.next());

    sendReceipt(source, nodeContext);
}

int FileStore::processRecoveryEvent(const bsl::shared_ptr<bdlbb::Blob>& blob)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(inDispatcherThread());
    BSLS_ASSERT_SAFE(blob);

    enum {
        rc_SUCCESS                  = 0,
        rc_INVALID_MSG_TYPE         = -1,
        rc_INVALID_PAYLOAD_OFFSET   = -2,
        rc_INVALID_PRIMARY_LEASE_ID = -3,
        rc_INVALID_SEQ_NUM          = -4,
        rc_WRITE_FAILURE            = -5
    };

    bmqp::Event                  rawEvent(blob.get(), d_allocator_p);
    bmqp::StorageMessageIterator iter;

    rawEvent.loadStorageMessageIterator(&iter);
    BSLS_ASSERT_SAFE(iter.isValid());

    bool hasValidLeaseIdSeqNum = false;

    if (0 < d_primaryLeaseId) {
        // File store encountered valid primaryLeaseId and sequence number in
        // 'open()'.

        if (!d_isFSMWorkflow) {
            BSLS_ASSERT_SAFE(0 < d_sequenceNum);
        }
        hasValidLeaseIdSeqNum = true;
    }

    while (1 == iter.next()) {
        const bmqp::StorageHeader& header = iter.header();

        BSLS_ASSERT_SAFE(header.partitionId() ==
                         static_cast<unsigned int>(d_config.partitionId()));

        if (bmqp::StorageMessageType::e_UNDEFINED == header.messageType()) {
            MWCTSK_ALARMLOG_ALARM("RECOVERY")
                << partitionDesc() << "Received an unexpected storage message "
                << "type: " << header.messageType()
                << " at journal offset (in words): "
                << header.journalOffsetWords() << MWCTSK_ALARMLOG_END;
            return rc_INVALID_MSG_TYPE;  // RETURN
        }

        mwcu::BlobPosition recordPosition;
        int                rc = iter.loadDataPosition(&recordPosition);
        if (0 != rc) {
            MWCTSK_ALARMLOG_ALARM("RECOVERY")
                << partitionDesc()
                << "Failed to load record position for storage msg "
                << header.messageType() << ", with journal offset (in words): "
                << header.journalOffsetWords() << ", rc: " << rc
                << MWCTSK_ALARMLOG_END;
            return rc_INVALID_PAYLOAD_OFFSET;  // RETURN
        }

        mwcu::BlobObjectProxy<mqbs::RecordHeader> recHeader(blob.get(),
                                                            recordPosition,
                                                            true,    // read
                                                            false);  // write
        if (!recHeader.isSet()) {
            MWCTSK_ALARMLOG_ALARM("RECOVERY")
                << partitionDesc()
                << "Failed to read RecordHeader for storage msg "
                << header.messageType() << ", with journal offset (in words): "
                << header.journalOffsetWords() << MWCTSK_ALARMLOG_END;
            return rc_INVALID_PAYLOAD_OFFSET;  // RETURN
        }

        if (!hasValidLeaseIdSeqNum) {
            hasValidLeaseIdSeqNum = true;
            d_primaryLeaseId      = recHeader->primaryLeaseId();
            d_sequenceNum         = recHeader->sequenceNumber();
        }
        else {
            // Validate leaseId, sequence number, etc.

            if (d_primaryLeaseId > recHeader->primaryLeaseId()) {
                MWCTSK_ALARMLOG_ALARM("RECOVERY")
                    << partitionDesc()
                    << "Encountered smaller primaryLeaseId ["
                    << recHeader->primaryLeaseId() << "] "
                    << "in buffered storage message of type: "
                    << header.messageType()
                    << ", sequence number: " << recHeader->sequenceNumber()
                    << ", self primary leaseId: " << d_primaryLeaseId
                    << ", with journal offset (words): "
                    << header.journalOffsetWords() << "."
                    << MWCTSK_ALARMLOG_END;
                return rc_INVALID_PRIMARY_LEASE_ID;  // RETURN
            }
            else if (d_primaryLeaseId < recHeader->primaryLeaseId()) {
                // LeaseId was bumped up.

                d_primaryLeaseId = recHeader->primaryLeaseId();
                d_sequenceNum    = recHeader->sequenceNumber();
            }
            else {
                if (recHeader->sequenceNumber() <= d_sequenceNum) {
                    BALL_LOG_INFO
                        << partitionDesc()
                        << "Skipping a buffered storage message of type "
                        << header.messageType()
                        << " as it has same or smaller sequence number. "
                        << "Sequence number in buffered message: "
                        << recHeader->sequenceNumber()
                        << ", self sequence number: " << d_sequenceNum << ".";
                    return rc_SUCCESS;  // RETURN
                }
                else if (recHeader->sequenceNumber() != (d_sequenceNum + 1)) {
                    MWCTSK_ALARMLOG_ALARM("REPLICATION")
                        << partitionDesc() << "Encountered buffered storage "
                        << "message " << header.messageType()
                        << " with missing sequence num. "
                        << "Expected: " << (d_sequenceNum + 1)
                        << ", received: " << recHeader->sequenceNumber()
                        << ". PrimaryLeaseId: " << recHeader->primaryLeaseId()
                        << ". Journal offset (words): "
                        << header.journalOffsetWords() << "."
                        << MWCTSK_ALARMLOG_END;
                    return rc_INVALID_SEQ_NUM;  // RETURN
                }

                ++d_sequenceNum;
            }
        }

        // 'd_sequenceNum' has been updated to appropriate value.

        if (bmqp::StorageMessageType::e_DATA == header.messageType()) {
            rc = writeMessageRecord(header, *recHeader, blob, recordPosition);
        }
        else if (bmqp::StorageMessageType::e_QLIST == header.messageType()) {
            rc = writeQueueCreationRecord(header,
                                          *recHeader,
                                          blob,
                                          recordPosition);
        }
        else {
            rc = writeJournalRecord(header,
                                    *recHeader,
                                    blob,
                                    recordPosition,
                                    header.messageType());
        }

        if (0 != rc) {
            MWCTSK_ALARMLOG_ALARM("RECOVERY")
                << partitionDesc()
                << "Failed to write buffered storage message "
                << header.messageType() << " with sequence number ("
                << recHeader->primaryLeaseId() << ", "
                << recHeader->sequenceNumber() << "), with journal "
                << "offset (words): " << header.journalOffsetWords()
                << ", rc: " << rc << MWCTSK_ALARMLOG_END;
            return 10 * rc + rc_WRITE_FAILURE;  // RETURN
        }
    }

    return rc_SUCCESS;
}

FileStore::NodeContext*
FileStore::generateReceipt(NodeContext*         nodeContext,
                           mqbnet::ClusterNode* node,
                           unsigned int         primaryLeaseId,
                           bsls::Types::Uint64  sequenceNumber)
{
    const DataStoreRecordKey key(sequenceNumber, primaryLeaseId);

    if (nodeContext == 0) {
        const int                     nodeId = node->nodeId();
        NodeReceiptContexts::iterator itNode = d_nodes.find(nodeId);

        if (itNode == d_nodes.end()) {
            // no prior history about this node
            itNode = d_nodes
                         .insert(bsl::make_pair(
                             nodeId,
                             NodeContext(d_config.bufferFactory(),
                                         key,
                                         d_allocator_p)))
                         .first;
        }
        nodeContext = &itNode->second;
    }
    else if (nodeContext->d_key < key) {
        nodeContext->d_key = key;
    }
    else {
        // outdated receipt
        return nodeContext;  // RETURN
    }

    if (nodeContext->d_state && nodeContext->d_state->tryLock()) {
        char* buffer = nodeContext->d_blob_sp->buffer(0).data();
        bmqp::ReplicationReceipt* receipt =
            reinterpret_cast<bmqp::ReplicationReceipt*>(
                buffer + sizeof(bmqp::EventHeader));

        // Overwrite Receipt sequence number
        (*receipt)
            .setPrimaryLeaseId(primaryLeaseId)
            .setSequenceNum(sequenceNumber);

        nodeContext->d_state->unlock();
    }
    else {
        bmqp::ProtocolUtil::buildReceipt(nodeContext->d_blob_sp.get(),
                                         d_config.partitionId(),
                                         primaryLeaseId,
                                         sequenceNumber);
        nodeContext->d_state = d_statePool_p->getObject();
    }

    return nodeContext;
}

void FileStore::sendReceipt(mqbnet::ClusterNode* node,
                            NodeContext*         nodeContext)
{
    if (nodeContext == 0) {
        return;  // RETURN
    }

    int rc = node->channel().writeBlob(nodeContext->d_blob_sp,
                                       bmqp::EventType::e_REPLICATION_RECEIPT,
                                       nodeContext->d_state);

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            rc != bmqt::GenericResult::e_SUCCESS)) {
        BALL_LOG_INFO << partitionDesc() << "Failed to send Receipt for "
                      << "[ primaryLeaseId: "
                      << nodeContext->d_key.d_primaryLeaseId
                      << ", sequence number: "
                      << nodeContext->d_key.d_sequenceNum << ".";

        // Ignore the error and keep the blob
    }
}

int FileStore::issueSyncPoint()
{
    enum { rc_SUCCESS = 0, rc_UNAVAILABLE = -1, rc_MISC_FAILURE = -2 };

    // This routine is invoked out-of-band (by a higher level component, or by
    // FileStore itself when self node becomes the primary for the partition,
    // when partition rolls over, etc.).  This means two things:
    // 1) It is implicit that a SyncPt should be force-issued if needed.
    // 2) There may be a need to rollover the partition because journal may be
    //    at capacity.  If this is the case, the SyncPt which will be issued
    //    will be of type 'ROLLOVER'.

    BSLS_ASSERT_SAFE(d_isPrimary);
    BSLS_ASSERT_SAFE(d_isOpen);

    FileSet* fs = d_fileSets[0].get();
    BSLS_ASSERT_SAFE(fs);

    if (!fs->d_journalFileAvailable) {
        if (!fs->d_fileSetRolloverPolicyAlarm) {
            BALL_LOG_ERROR << partitionDesc() << "Failed to write sync point "
                           << "because partition is unavailable.";
        }
        return rc_UNAVAILABLE;  // RETURN
    }

    BSLS_ASSERT_SAFE(0 ==
                     fs->d_dataFilePosition % bmqp::Protocol::k_DWORD_SIZE);
    if (d_isFSMWorkflow) {
        BSLS_ASSERT_SAFE(0 == fs->d_qlistFilePosition %
                                  bmqp::Protocol::k_WORD_SIZE);
    }

    // Check if journal is at capacity and rollover if so.  Note that there is
    // no need to explicitly issue a SyncPt after rolling over, because a
    // SyncPt is issued as part of rollover step.

    if (needRollover(fs->d_journalFile,
                     fs->d_journalFilePosition,
                     k_REQUESTED_JOURNAL_SPACE)) {
        return rolloverIfNeeded(FileType::e_JOURNAL,
                                fs->d_journalFile,
                                fs->d_journalFileName,
                                fs->d_journalFilePosition,
                                k_REQUESTED_JOURNAL_SPACE);  // RETURN
    }

    // There is enough space in the journal to issue a 'regular' SyncPt.

    bmqp_ctrlmsg::SyncPoint syncPoint;
    syncPoint.primaryLeaseId()       = d_primaryLeaseId;
    syncPoint.sequenceNum()          = ++d_sequenceNum;
    syncPoint.dataFileOffsetDwords() = fs->d_dataFilePosition /
                                       bmqp::Protocol::k_DWORD_SIZE;
    if (!d_isFSMWorkflow) {
        syncPoint.qlistFileOffsetWords() = fs->d_qlistFilePosition /
                                           bmqp::Protocol::k_WORD_SIZE;
    }

    int rc = issueSyncPointInternal(SyncPointType::e_REGULAR,
                                    true,  // ImmediateFlush
                                    &syncPoint);
    if (0 != rc) {
        return 10 * rc + rc_MISC_FAILURE;  // RETURN
    }

    return rc_SUCCESS;
}

void FileStore::setActivePrimary(mqbnet::ClusterNode* primaryNode,
                                 unsigned int         primaryLeaseId)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(inDispatcherThread());
    BSLS_ASSERT_SAFE(0 < primaryLeaseId);
    BSLS_ASSERT_SAFE(0 != primaryNode);

    // Specified leaseId must be greater than or equal to 'd_primaryLeaseId'.
    if (d_primaryLeaseId > primaryLeaseId) {
        MWCTSK_ALARMLOG_ALARM("CLUSTER")
            << partitionDesc()
            << "Invalid primary leaseId specified: " << primaryLeaseId
            << ", current self leaseId: " << d_primaryLeaseId
            << ". Specified primaryNode: " << primaryNode->nodeDescription()
            << MWCTSK_ALARMLOG_END;
        return;  // RETURN
    }

    if (primaryLeaseId > d_primaryLeaseId) {
        // Reset the sequence number only if primary leaseId has been bumped
        // up.

        d_sequenceNum = 0;
    }
    d_primaryLeaseId = primaryLeaseId;
    d_primaryNode_p  = primaryNode;

    BALL_LOG_INFO << partitionDesc() << "Primary node is now "
                  << primaryNode->nodeDescription()
                  << " with primaryLeaseId: " << d_primaryLeaseId
                  << ", and sequence number: " << d_sequenceNum << ".";

    if (primaryNode->nodeId() != d_config.nodeId()) {
        d_isPrimary = false;
        d_clusterStats_p->setNodeRoleForPartition(
            d_config.partitionId(),
            mqbstat::ClusterStats::PrimaryStatus::e_REPLICA);
        d_config.scheduler()->cancelEvent(&d_syncPointEventHandle);
        d_config.scheduler()->cancelEvent(
            &d_partitionHighwatermarkEventHandle);

        if (d_lastRecoveredStrongConsistency.d_primaryLeaseId ==
            d_primaryLeaseId) {
            BALL_LOG_INFO << partitionDesc() << "Issuing Implicit Receipt ["
                          << "primaryLeaseId = " << d_primaryLeaseId
                          << ", d_sequenceNum = " << d_sequenceNum << "].";

            FileStore::NodeContext* nodeContext = generateReceipt(
                0,
                primaryNode,
                d_lastRecoveredStrongConsistency.d_primaryLeaseId,
                d_lastRecoveredStrongConsistency.d_sequenceNum);
            sendReceipt(primaryNode, nodeContext);
        }
        return;  // RETURN
    }

    d_isPrimary = true;
    d_clusterStats_p->setNodeRoleForPartition(
        d_config.partitionId(),
        mqbstat::ClusterStats::PrimaryStatus::e_PRIMARY);

    // Schedule a sync point issue recurring event every 1 second, starting
    // after 1 second.
    d_config.scheduler()->scheduleRecurringEvent(
        &d_syncPointEventHandle,
        bsls::TimeInterval(1),  // 1 sec. TBD: make configurable
        bdlf::BindUtil::bind(&FileStore::issueSyncPointCb, this));

    // Schedule a periodic alarm highwatermark check
    d_config.scheduler()->scheduleRecurringEvent(
        &d_partitionHighwatermarkEventHandle,
        bsls::TimeInterval(k_PARTITION_AVAILABLESPACE_SECS),
        bdlf::BindUtil::bind(&FileStore::alarmHighwatermarkIfNeededCb, this));

    // New primary needs to issue a sync point with old leaseId, if previous
    // primary disappeared w/o issuing a sync point (crash, etc).  It is
    // necessary that JOURNAL record at the primary-switch boundary is always a
    // sync point with old leaseId, because when we recover messages from the
    // JOURNAL, we iterate in reverse direction, and update 'local' leaseId and
    // sequenceNum upon encountering a sync point.

    // Read the last JOURNAL record to see if its a SyncPt or not.  Note that
    // we cannot rely on 'd_records' to retrieve last record's sequenceNum &
    // leaseId, because 'd_records' could be empty or may not contain the
    // newest record as it may have been removed from it.
    FileSet* fs = d_fileSets[0].get();
    BSLS_ASSERT_SAFE(fs);

    bool                issueOldSyncPt    = false;
    RecordType::Enum    lastRecordType    = RecordType::e_UNDEFINED;
    bsls::Types::Uint64 lastRecordOffset  = 0;
    unsigned int        lastRecordLeaseId = 0;
    bsls::Types::Uint64 lastRecordSeqNum  = 0;

    if (fs->d_journalFilePosition >
        (sizeof(FileHeader) + sizeof(JournalFileHeader))) {
        // There is at least one record in the JOURNAL.  Read the last JOURNAL
        // record.

        BSLS_ASSERT_SAFE(fs->d_journalFilePosition >
                         FileStoreProtocol::k_JOURNAL_RECORD_SIZE);

        const MappedFileDescriptor&   journalFd = fs->d_journalFile;
        OffsetPtr<const RecordHeader> recHeader(
            journalFd.block(),
            fs->d_journalFilePosition -
                FileStoreProtocol::k_JOURNAL_RECORD_SIZE);

        lastRecordLeaseId = recHeader->primaryLeaseId();
        lastRecordSeqNum  = recHeader->sequenceNumber();

        BSLS_ASSERT_SAFE(RecordType::e_UNDEFINED != recHeader->type());

        if (RecordType::e_JOURNAL_OP == recHeader->type()) {
            OffsetPtr<const JournalOpRecord> jOpRec(
                journalFd.block(),
                fs->d_journalFilePosition -
                    FileStoreProtocol::k_JOURNAL_RECORD_SIZE);

            BSLS_ASSERT_SAFE(JournalOpType::e_UNDEFINED != jOpRec->type());

            if (JournalOpType::e_SYNCPOINT != jOpRec->type()) {
                // Last record is not a SyncPt.

                issueOldSyncPt   = true;
                lastRecordType   = RecordType::e_JOURNAL_OP;
                lastRecordOffset = fs->d_journalFilePosition -
                                   FileStoreProtocol::k_JOURNAL_RECORD_SIZE;
            }
            // else: last record is a SyncPt.  No need to issue a SyncPt on
            // behalf of old primary.
        }
        else {
            issueOldSyncPt   = true;
            lastRecordType   = recHeader->type();
            lastRecordOffset = fs->d_journalFilePosition -
                               FileStoreProtocol::k_JOURNAL_RECORD_SIZE;
        }
    }

    if (issueOldSyncPt) {
        BALL_LOG_INFO << partitionDesc() << "New primary (self) will "
                      << "issue a SyncPt with old (leaseId, seqNum): ("
                      << lastRecordLeaseId << ", " << (lastRecordSeqNum + 1)
                      << "), because last JOURNAL record, at offset: "
                      << lastRecordOffset << ", is of type [" << lastRecordType
                      << "], which is not a SyncPt.";

        BSLS_ASSERT_SAFE(RecordType::e_UNDEFINED != lastRecordType);
        BSLS_ASSERT_SAFE(0 != lastRecordOffset);
        BSLS_ASSERT_SAFE(0 < lastRecordLeaseId);
        BSLS_ASSERT_SAFE(0 < lastRecordSeqNum);

        // Journal must have space for 1 regular SyncPt if previous primary's
        // last record was not a SyncPt -- because primary always ensures that
        // there is enough space in the journal to write a SyncPt.  Check
        // journal size, if it doesn't have space for at least 2 SyncPts (one
        // for this, and other for the 1st SyncPt of new primary), then mark
        // journal unavailable.

        if (fs->d_journalFile.fileSize() <
            (fs->d_journalFilePosition +
             2 * FileStoreProtocol::k_JOURNAL_RECORD_SIZE)) {
            MWCTSK_ALARMLOG_ALARM("REPLICATION")
                << partitionDesc()
                << "Not enough space in journal for new primary to issue a "
                << "SyncPt on behalf of previous primary. Max journal file "
                << "size: " << fs->d_journalFile.fileSize()
                << ", current journal file offset: "
                << fs->d_journalFilePosition << ", minimum required space: "
                << 2 * FileStoreProtocol::k_JOURNAL_RECORD_SIZE
                << ". Partition will be marked unavailable."
                << MWCTSK_ALARMLOG_END;

            fs->d_fileSetRolloverPolicyAlarm = true;
            fs->d_journalFileAvailable       = false;

            return;  // RETURN
        }

        // Explicitly create a SyncPt with (leaseId, seqNum) of previous
        // primary.

        BSLS_ASSERT_SAFE(0 == fs->d_dataFilePosition %
                                  bmqp::Protocol::k_DWORD_SIZE);
        if (!d_isFSMWorkflow) {
            BSLS_ASSERT_SAFE(0 == fs->d_qlistFilePosition %
                                      bmqp::Protocol::k_WORD_SIZE);
        }
        bmqp_ctrlmsg::SyncPoint syncPoint;
        syncPoint.primaryLeaseId()       = lastRecordLeaseId;
        syncPoint.sequenceNum()          = lastRecordSeqNum + 1;
        syncPoint.dataFileOffsetDwords() = fs->d_dataFilePosition /
                                           bmqp::Protocol::k_DWORD_SIZE;
        syncPoint.qlistFileOffsetWords() =
            d_isFSMWorkflow
                ? 0
                : fs->d_qlistFilePosition / bmqp::Protocol::k_WORD_SIZE;

        // Explicitly update the sequence number, since we are passing the
        // SyncPt ourselves, and 'issueSyncPointInternal' won't increment
        // 'd_sequenceNum' in that case.

        ++d_sequenceNum;

        int rc = issueSyncPointInternal(SyncPointType::e_REGULAR,
                                        true,  // ImmediateFlush
                                        &syncPoint);
        if (0 != rc) {
            MWCTSK_ALARMLOG_ALARM("REPLICATION")
                << partitionDesc()
                << "New primary failed to issue SyncPt on behalf of previous "
                << "primary, rc: " << rc << MWCTSK_ALARMLOG_END;
            return;  // RETURN
        }

        BALL_LOG_INFO << partitionDesc()
                      << "New primary successfully issued SyncPt on behalf of "
                      << "previous primary: " << syncPoint
                      << ", at journal offset: "
                      << (fs->d_journalFilePosition -
                          FileStoreProtocol::k_JOURNAL_RECORD_SIZE)
                      << ".";
    }

    // Issue one SyncPt right away.  Note that we invoke the 'higher' level
    // 'issueSyncPoint' API because this is an out-of-band request, and a
    // rollover might be required.

    int rc = issueSyncPoint();
    if (0 != rc) {
        MWCTSK_ALARMLOG_ALARM("REPLICATION")
            << partitionDesc()
            << "New primary failed to issue SyncPt , rc: " << rc
            << MWCTSK_ALARMLOG_END;
        return;  // RETURN
    }
}

void FileStore::clearPrimary()
{
    if (!d_isOpen) {
        return;  // RETURN
    }

    if (0 == d_primaryNode_p) {
        // Nothing to do.

        return;  // RETURN
    }

    BALL_LOG_INFO << partitionDesc() << "Clearing current primary: "
                  << d_primaryNode_p->nodeDescription()
                  << ". Current (leaseId, seqnum): (" << d_primaryLeaseId
                  << ", " << d_sequenceNum << ").";
    d_primaryNode_p = 0;

    // If self has a valid leaseId and zero seqnum (ie, previous primary went
    // away after issuing active primary stats advisory, but before issuing a
    // SyncPt), update self's (leaseId, seqnum) from last record in the
    // journal.

    if (0 == d_primaryLeaseId) {
        return;  // RETURN
    }

    if (0 != d_sequenceNum) {
        return;  // RETURN
    }

    FileSet* fs = d_fileSets[0].get();
    BSLS_ASSERT_SAFE(fs);

    if (fs->d_journalFilePosition >
        (sizeof(FileHeader) + sizeof(JournalFileHeader))) {
        // There is at least one record in the JOURNAL.  Read the last JOURNAL
        // record.

        BSLS_ASSERT_SAFE(fs->d_journalFilePosition >
                         FileStoreProtocol::k_JOURNAL_RECORD_SIZE);

        const MappedFileDescriptor&   journalFd = fs->d_journalFile;
        OffsetPtr<const RecordHeader> recHeader(
            journalFd.block(),
            fs->d_journalFilePosition -
                FileStoreProtocol::k_JOURNAL_RECORD_SIZE);

        d_primaryLeaseId = recHeader->primaryLeaseId();
        d_sequenceNum    = recHeader->sequenceNumber();
        BSLS_ASSERT_SAFE(0 != d_primaryLeaseId);
        BSLS_ASSERT_SAFE(0 != d_sequenceNum);

        BALL_LOG_INFO << partitionDesc() << "Rolled back (leaseId, seqnum) to "
                      << "(" << d_primaryLeaseId << ", " << d_sequenceNum
                      << ") upon processing 'clear-primary' event, as previous"
                      << " primary went away without issuing a SyncPt.";
    }
}

void FileStore::dispatcherFlush(bool storage, bool queues)
{
    // 'LocalQueue::flush' invokes 'dispaterFlush'.
    // This means that 'dispaterFlush' will be executed more frequently on a
    // FileStore than actually applicable.  This is ok and has no side effect.

    // Note that 'RemoteQueue::dispaterFlush' will not invoke
    // 'FileStore::dispaterFlush' because only the partition's primary node
    // should invoke 'dispaterFlush' on the FileStore.

    BSLS_ASSERT_SAFE(d_isPrimary);

    if (storage) {
        if (d_storageEventBuilder.messageCount() != 0) {
            BALL_LOG_TRACE << partitionDesc() << "Flushing "
                           << d_storageEventBuilder.messageCount()
                           << " STORAGE messages.";
            const int maxChannelPendingItems = d_cluster_p->broadcast(
                d_storageEventBuilder.blob_sp());
            if (maxChannelPendingItems > 0) {
                if (d_nagglePacketCount < k_NAGLE_PACKET_COUNT) {
                    // back off
                    ++d_nagglePacketCount;
                }
            }
            else if (d_nagglePacketCount) {
                --d_nagglePacketCount;
            }
            d_storageEventBuilder.reset();
        }
    }
    if (queues && d_storageEventBuilder.messageCount() == 0) {
        // Empty 'd_storageEventBuilder' means it has been flushed and it is a
        // good time to flush queues.
        for (StorageMapIter it = d_storages.begin(); it != d_storages.end();
             ++it) {
            ReplicatedStorage* rs = it->second;
            if (rs->queue()) {
                rs->queue()->onReplicatedBatch();
            }
        }
    }
}

void FileStore::notifyQueuesOnReplicatedBatch()
{
    if (d_storageEventBuilder.messageCount() == 0) {
        // Empty 'd_storageEventBuilder' means it has been flushed and it is a
        // good time to flush queues.
        for (StorageMapIter it = d_storages.begin(); it != d_storages.end();
             ++it) {
            ReplicatedStorage* rs = it->second;
            if (rs->queue()) {
                rs->queue()->onReplicatedBatch();
            }
        }
    }
}

bool FileStore::gcExpiredMessages(const bdlt::Datetime& currentTimeUtc)
{
    if (!d_isOpen) {
        return false;  // RETURN
    }

    if (!d_isPrimary) {
        return false;  // RETURN
    }

    BSLS_ASSERT_SAFE(0 < d_fileSets.size());
    FileSet* activeFileSet = d_fileSets[0].get();
    BSLS_ASSERT_SAFE(activeFileSet);

    if (!activeFileSet->d_journalFileAvailable) {
        return false;  // RETURN
    }

    // Go over each file-backed storage registered with this partition and
    // indicate it to GC any applicable messages.

    const bsls::Types::Uint64 currentSecondsFromEpoch =
        static_cast<bsls::Types::Uint64>(
            bdlt::EpochUtil::convertToTimeT64(currentTimeUtc));
    bool haveMore    = false;
    bool needToFlush = false;

    for (StorageMapIter it = d_storages.begin(); it != d_storages.end();
         ++it) {
        ReplicatedStorage*  rs                        = it->second;
        bsls::Types::Uint64 latestMsgTimestamp        = 0;
        bsls::Types::Int64  configuredTtlValueSeconds = 0;
        int numMsgsGc = rs->gcExpiredMessages(&latestMsgTimestamp,
                                              &configuredTtlValueSeconds,
                                              currentSecondsFromEpoch);
        if (numMsgsGc <= 0) {
            // No messages GC'd or error.

            continue;  // CONTINUE
        }
        else {
            needToFlush = true;
        }

        BALL_LOG_INFO << partitionDesc() << "For storage for queue ["
                      << rs->queueUri() << "] and queueKey [" << it->first
                      << "] configured with TTL value of ["
                      << configuredTtlValueSeconds
                      << "] seconds, garbage-collected [" << numMsgsGc
                      << "] messages due to TTL expiration. "
                      << "Timestamp (UTC) of the latest encountered message: "
                      << bdlt::EpochUtil::convertFromTimeT64(
                             latestMsgTimestamp)
                      << " (Epoch: " << latestMsgTimestamp
                      << "). Current time (UTC): " << currentTimeUtc
                      << " (Epoch: " << currentSecondsFromEpoch << ")."
                      << " Num messages remaining in the storage: "
                      << rs->numMessages(mqbu::StorageKey::k_NULL_KEY)
                      << ". Storage type: "
                      << (rs->isPersistent() ? "persistent." : "in-memory.");

        if (!rs->isEmpty() &&
            (latestMsgTimestamp + configuredTtlValueSeconds) <=
                currentSecondsFromEpoch) {
            haveMore = true;
        }
    }

    if (needToFlush) {
        // Have to explicitly flush 'd_storageEventBuilder', to make sure
        // deletion records get replicated before queue unassignement.
        // If queues are idle, 'dispatcherFlush()' won't get called.
        // Internal-ticket D168465018.

        dispatcherFlush(true, false);
    }

    return haveMore;
}

bool FileStore::gcHistory()
{
    if (!d_isOpen) {
        return false;  // RETURN
    }
    bool haveMore = false;
    for (StorageMapIter it = d_storages.begin(); it != d_storages.end();
         ++it) {
        if (it->second->gcHistory()) {
            haveMore = true;
        }
    }
    return haveMore;
}

void FileStore::applyForEachQueue(const QueueFunctor& functor) const
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(inDispatcherThread());

    if (!d_isOpen) {
        return;  // RETURN
    }

    for (StorageMapConstIter it = d_storages.begin(); it != d_storages.end();
         ++it) {
        if (it->second->queue()) {
            functor(it->second->queue());
        }
    }
}

void FileStore::deprecateFileSet()
{
    if (d_isOpen) {
        BALL_LOG_ERROR << partitionDesc()
                       << "Cannot deprecate the active file set as this "
                       << "FileStore is still open.";

        return;  // RETURN
    }

    archive(d_fileSets[0].get());
}

void FileStore::forceRollover()
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(inDispatcherThread());

    if (!d_isOpen || !d_isPrimary) {
        return;  // RETURN
    }

    const int rc = rollover(
        bdlt::EpochUtil::convertToTimeT64(bdlt::CurrentTime::utc()));
    if (rc != 0) {
        BALL_LOG_ERROR << partitionDesc()
                       << "Forced partition rollover failed with rc: " << rc;
    }
}

void FileStore::registerStorage(ReplicatedStorage* storage)
{
    BSLS_ASSERT_SAFE(storage);

    BALL_LOG_INFO << "Registering storage for queue '" << storage->queueUri()
                  << "', queueKey: " << storage->queueKey();
    d_storages[storage->queueKey()] = storage;
}

void FileStore::unregisterStorage(const ReplicatedStorage* storage)
{
    BSLS_ASSERT_SAFE(storage);

    BALL_LOG_INFO << "Unregistering storage for queue '" << storage->queueUri()
                  << "', queueKey: " << storage->queueKey();
    size_t count = d_storages.erase(storage->queueKey());
    BSLS_ASSERT_SAFE(1 == count);
    static_cast<void>(count);
}

void FileStore::cancelTimersAndWait()
{
    d_config.scheduler()->cancelEventAndWait(&d_syncPointEventHandle);
    d_config.scheduler()->cancelEventAndWait(
        &d_partitionHighwatermarkEventHandle);
}

void FileStore::processShutdownEvent()
{
    BALL_LOG_INFO << partitionDesc()
                  << "Partition notified that self node is shutting down.";

    d_isStopping = true;
}

void FileStore::setAvailabilityStatus(bool enable)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(0 < d_fileSets.size());

    FileSet* activeFileSet = d_fileSets[0].get();

    if (!d_isPrimary) {
        BALL_LOG_WARN << partitionDesc() << "attempted to disable partition on"
                      << " non-primary. Partition status is unchanged. Current"
                      << " status is: "
                      << (activeFileSet->d_journalFileAvailable
                              ? "AVAILABLE"
                              : "UNAVAILABLE");
        return;  // RETURN
    }

    activeFileSet->d_journalFileAvailable = enable;

    BALL_LOG_INFO << partitionDesc() << "partition status changed to: "
                  << (enable ? "AVAILABLE" : "UNAVAILABLE");
}

void FileStore::onDispatcherEvent(const mqbi::DispatcherEvent& event)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(inDispatcherThread());

    BALL_LOG_TRACE << description() << ": processing dispatcher event '"
                   << event << "'";

    switch (event.type()) {
    case mqbi::DispatcherEventType::e_CALLBACK: {
        const mqbi::DispatcherCallbackEvent* realEvent =
            event.asCallbackEvent();
        BSLS_ASSERT_SAFE(realEvent->callback());
        realEvent->callback()(dispatcherClientData().processorHandle());
    } break;  // BREAK
    case mqbi::DispatcherEventType::e_UNDEFINED:
    case mqbi::DispatcherEventType::e_PUT:
    case mqbi::DispatcherEventType::e_ACK:
    case mqbi::DispatcherEventType::e_CONFIRM:
    case mqbi::DispatcherEventType::e_REJECT:
    case mqbi::DispatcherEventType::e_CLUSTER_STATE:
    case mqbi::DispatcherEventType::e_STORAGE:
    case mqbi::DispatcherEventType::e_RECOVERY:
    case mqbi::DispatcherEventType::e_PUSH:
    case mqbi::DispatcherEventType::e_REPLICATION_RECEIPT:
    case mqbi::DispatcherEventType::e_CONTROL_MSG:
    case mqbi::DispatcherEventType::e_DISPATCHER:

    default:
        BALL_LOG_ERROR << "Received dispatcher event of unexpected type"
                       << event.type();
        break;  // BREAK
    };
}

void FileStore::flush()
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(inDispatcherThread());

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(d_isStopping)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return;  // RETURN
    }

    const bool haveMore        = gcExpiredMessages(bdlt::CurrentTime::utc());
    const bool haveMoreHistory = gcHistory();

    // This is either Idle or k_GC_MESSAGES_INTERVAL_SECONDS timeout.
    // 'gcHistory' attempts to iterate all old items. If there are more of them
    // than the batchSize (1000), it returns 'true'.  In this case, re-enable
    // flush client to call it again next Idle time.
    // If it returns 'false', there is no immediate work.  Wait for the
    // next k_GC_MESSAGES_INTERVAL_SECONDS.

    if (haveMore || haveMoreHistory) {
        // Explicitly schedule 'flush()' instead of relying on idleness
        dispatcher()->execute(bdlf::BindUtil::bind(&FileStore::flush, this),
                              this,
                              mqbi::DispatcherEventType::e_CALLBACK);
    }
}

void FileStore::setReplicationFactor(int value)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(inDispatcherThread());

    // Set the new replication factor.
    d_replicationFactor = value;

    // No further work needed if this is replica.
    if (!d_isPrimary) {
        return;
    }

    // Build a set of unreceipted messages whose count of persisted replicas
    // meets the new threshold replication factor.
    bsl::unordered_set<mqbi::Queue*> affectedQueues(d_allocator_p);

    Unreceipted::iterator it = d_unreceipted.begin();
    mqbu::StorageKey      lastKey;
    mqbi::Queue*          lastQueue = 0;
    while (it != d_unreceipted.end()) {
        if (it->second.d_count >= d_replicationFactor) {
            it->second.d_handle->second.d_hasReceipt = true;
            // notify the queue.

            const mqbu::StorageKey& queueKey  = it->second.d_queueKey;
            bool                    haveQueue = (queueKey == lastKey);
            if (!haveQueue) {
                StorageMapIter sit = d_storages.find(queueKey);
                if (sit != d_storages.end()) {
                    haveQueue = true;
                    lastKey   = queueKey;
                    lastQueue = sit->second->queue();
                    BSLS_ASSERT_SAFE(lastQueue);

                    affectedQueues.insert(lastQueue);
                }
                // else the queue and its storage are gone; ignore the receipt
            }
            if (haveQueue) {
                lastQueue->onReceipt(
                    it->second.d_guid,
                    it->second.d_qH,
                    it->second.d_handle->second.d_arrivalTimepoint);
            }  // else the queue is gone
            it = d_unreceipted.erase(it);
        }
        else {
            // Since we have as an invariant that
            //   unreceipted[k].d_count > unreceipted[k+1].d_count,
            // We can safely break, once we find an entry whose count does not
            // meet the replication factor.
            break;
        }
    }
    for (bsl::unordered_set<mqbi::Queue*>::iterator qit =
             affectedQueues.begin();
         qit != affectedQueues.end();
         ++qit) {
        (*qit)->queueEngine()->afterNewMessage(bmqt::MessageGUID(), 0);
    }
}

void FileStore::getStorages(StorageList*          storages,
                            const StorageFilters& filters) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(storages);

    StorageCollectionUtil::loadStorages(storages, d_storages);

    for (StorageFiltersconstIter cit = filters.cbegin(); cit != filters.cend();
         ++cit) {
        StorageCollectionUtil::filterStorages(storages, *cit);
    }
}

void FileStore::loadSummary(mqbcmd::FileStore* fileStore) const
{
    // executed by *QUEUE_DISPATCHER* thread with the specified `fileStore`'s
    // partitionId

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(fileStore);

    fileStore->partitionId() = d_config.partitionId();
    if (!isOpen()) {
        fileStore->state() = mqbcmd::FileStoreState::CLOSED;
        return;  // RETURN
    }

    if (d_isStopping) {
        fileStore->state() = mqbcmd::FileStoreState::STOPPING;
        return;  // RETURN
    }

    fileStore->state() = mqbcmd::FileStoreState::OPEN;
    FileStorePrintUtil::loadSummary(&fileStore->summary(),
                                    d_primaryNode_p,
                                    d_primaryLeaseId,
                                    d_sequenceNum,
                                    d_records.size(),
                                    d_unreceipted.size(),
                                    d_nagglePacketCount,
                                    d_fileSets,
                                    d_storages);
}

// ACCESSORS
void FileStore::loadMessageRecordRaw(MessageRecord*               buffer,
                                     const DataStoreRecordHandle& handle) const
{
    BSLS_ASSERT_SAFE(handle.isValid());
    FileSet* activeFileSet = d_fileSets[0].get();
    BSLS_ASSERT_SAFE(activeFileSet);

    const RecordIterator& recordIt = *reinterpret_cast<const RecordIterator*>(
        &handle);
    const DataStoreRecord& record = recordIt->second;
    BSLS_ASSERT_SAFE(RecordType::e_MESSAGE == record.d_recordType);
    BSLS_ASSERT_SAFE(0 != record.d_recordOffset);
    BSLS_ASSERT_SAFE(0 != record.d_messageOffset);
    BSLS_ASSERT_SAFE(0 != record.d_appDataUnpaddedLen);

    OffsetPtr<const MessageRecord> rec(activeFileSet->d_journalFile.block(),
                                       record.d_recordOffset);
    *buffer = *rec;
}

void FileStore::loadConfirmRecordRaw(ConfirmRecord*               buffer,
                                     const DataStoreRecordHandle& handle) const
{
    BSLS_ASSERT_SAFE(handle.isValid());
    FileSet* activeFileSet = d_fileSets[0].get();
    BSLS_ASSERT_SAFE(activeFileSet);

    const RecordIterator& recordIt = *reinterpret_cast<const RecordIterator*>(
        &handle);
    const DataStoreRecord& record = recordIt->second;
    BSLS_ASSERT_SAFE(RecordType::e_CONFIRM == record.d_recordType);
    BSLS_ASSERT_SAFE(0 != record.d_recordOffset);
    OffsetPtr<const ConfirmRecord> rec(activeFileSet->d_journalFile.block(),
                                       record.d_recordOffset);
    *buffer = *rec;
}

void FileStore::loadDeletionRecordRaw(
    DeletionRecord*              buffer,
    const DataStoreRecordHandle& handle) const
{
    BSLS_ASSERT_SAFE(handle.isValid());
    FileSet* activeFileSet = d_fileSets[0].get();
    BSLS_ASSERT_SAFE(activeFileSet);

    const RecordIterator& recordIt = *reinterpret_cast<const RecordIterator*>(
        &handle);
    const DataStoreRecord& record = recordIt->second;
    BSLS_ASSERT_SAFE(RecordType::e_DELETION == record.d_recordType);
    BSLS_ASSERT_SAFE(0 != record.d_recordOffset);
    OffsetPtr<const DeletionRecord> rec(activeFileSet->d_journalFile.block(),
                                        record.d_recordOffset);
    *buffer = *rec;
}

void FileStore::loadQueueOpRecordRaw(QueueOpRecord*               buffer,
                                     const DataStoreRecordHandle& handle) const
{
    BSLS_ASSERT_SAFE(handle.isValid());
    FileSet* activeFileSet = d_fileSets[0].get();
    BSLS_ASSERT_SAFE(activeFileSet);

    const RecordIterator& recordIt = *reinterpret_cast<const RecordIterator*>(
        &handle);
    const DataStoreRecord& record = recordIt->second;
    BSLS_ASSERT_SAFE(RecordType::e_QUEUE_OP == record.d_recordType);
    BSLS_ASSERT_SAFE(0 != record.d_recordOffset);
    OffsetPtr<const QueueOpRecord> rec(activeFileSet->d_journalFile.block(),
                                       record.d_recordOffset);
    *buffer = *rec;
}

void FileStore::loadMessageAttributesRaw(
    mqbi::StorageMessageAttributes* buffer,
    const DataStoreRecordHandle&    handle) const
{
    BSLS_ASSERT_SAFE(handle.isValid());
    const RecordIterator& recordIt = *reinterpret_cast<const RecordIterator*>(
        &handle);

    DataStoreRecord& record = const_cast<DataStoreRecord&>(recordIt->second);
    BSLS_ASSERT_SAFE(RecordType::e_MESSAGE == record.d_recordType);
    BSLS_ASSERT_SAFE(0 != record.d_recordOffset);
    BSLS_ASSERT_SAFE(0 != record.d_messageOffset);
    BSLS_ASSERT_SAFE(0 != record.d_appDataUnpaddedLen);

    OffsetPtr<const MessageRecord> rec(d_fileSets[0]->d_journalFile.block(),
                                       record.d_recordOffset);

    *buffer = mqbi::StorageMessageAttributes(rec->header().timestamp(),
                                             rec->refCount(),
                                             record.d_messagePropertiesInfo,
                                             rec->compressionAlgorithmType(),
                                             record.d_hasReceipt,
                                             0,
                                             rec->crc32c(),
                                             record.d_arrivalTimepoint);
}

void FileStore::loadMessageRaw(bsl::shared_ptr<bdlbb::Blob>*   appData,
                               bsl::shared_ptr<bdlbb::Blob>*   options,
                               mqbi::StorageMessageAttributes* attributes,
                               const DataStoreRecordHandle&    handle) const
{
    loadMessageAttributesRaw(attributes, handle);
    const RecordIterator& recordIt = *reinterpret_cast<const RecordIterator*>(
        &handle);
    aliasMessage(appData, options, recordIt->second);
}

unsigned int
FileStore::getMessageLenRaw(const DataStoreRecordHandle& handle) const
{
    BSLS_ASSERT_SAFE(handle.isValid());
    const RecordIterator& recordIt = *reinterpret_cast<const RecordIterator*>(
        &handle);

    const DataStoreRecord& record = recordIt->second;
    BSLS_ASSERT_SAFE(RecordType::e_MESSAGE == record.d_recordType);
    BSLS_ASSERT_SAFE(0 != record.d_recordOffset);
    BSLS_ASSERT_SAFE(0 != record.d_messageOffset);
    BSLS_ASSERT_SAFE(0 != record.d_appDataUnpaddedLen);

    return record.d_appDataUnpaddedLen;
}

void FileStore::loadCurrentFiles(mqbs::FileStoreSet* fileStoreSet) const
{
    // PRECONDITIONS
    BSLS_ASSERT(fileStoreSet);
    BSLS_ASSERT(d_fileSets.size() > 0);
    BSLS_ASSERT(d_fileSets[0].get());

    FileStoreUtil::loadCurrentFiles(fileStoreSet,
                                    *d_fileSets[0],
                                    !d_isFSMWorkflow);
}

bool FileStore::hasReceipt(const DataStoreRecordHandle& handle) const
{
    BSLS_ASSERT_SAFE(handle.isValid());
    const RecordIterator& recordIt = *reinterpret_cast<const RecordIterator*>(
        &handle);

    const DataStoreRecord& record = recordIt->second;

    return record.d_hasReceipt;
}

// -----------------------
// class FileStoreIterator
// -----------------------

// MANIPULATORS
bool FileStoreIterator::next()
{
    if (d_firstInvocation) {
        d_firstInvocation = false;
        d_iterator        = d_store_p->d_records.begin();
        if (d_iterator == d_store_p->d_records.end()) {
            return false;  // RETURN
        }

        return true;  // RETURN
    }

    if (d_iterator == d_store_p->d_records.end()) {
        return false;  // RETURN
    }

    ++d_iterator;
    if (d_iterator == d_store_p->d_records.end()) {
        return false;  // RETURN
    }

    return true;
}

// ACCESSORS
void FileStoreIterator::loadMessageRecord(MessageRecord* buffer) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(RecordType::e_MESSAGE == type());

    DataStoreRecordHandle handle;
    RecordIterator& recordItRef = *reinterpret_cast<RecordIterator*>(&handle);
    recordItRef                 = d_iterator;
    d_store_p->loadMessageRecordRaw(buffer, handle);
}

void FileStoreIterator::loadConfirmRecord(ConfirmRecord* buffer) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(RecordType::e_CONFIRM == type());

    DataStoreRecordHandle handle;
    RecordIterator& recordItRef = *reinterpret_cast<RecordIterator*>(&handle);
    recordItRef                 = d_iterator;
    d_store_p->loadConfirmRecordRaw(buffer, handle);
}

void FileStoreIterator::loadDeletionRecord(DeletionRecord* buffer) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(RecordType::e_DELETION == type());

    DataStoreRecordHandle handle;
    RecordIterator& recordItRef = *reinterpret_cast<RecordIterator*>(&handle);
    recordItRef                 = d_iterator;
    d_store_p->loadDeletionRecordRaw(buffer, handle);
}

void FileStoreIterator::loadQueueOpRecord(QueueOpRecord* buffer) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(RecordType::e_QUEUE_OP == type());

    DataStoreRecordHandle handle;
    RecordIterator& recordItRef = *reinterpret_cast<RecordIterator*>(&handle);
    recordItRef                 = d_iterator;
    d_store_p->loadQueueOpRecordRaw(buffer, handle);
}

bsl::ostream& FileStoreIterator::print(bsl::ostream& stream,
                                       int           level,
                                       int           spacesPerLevel) const
{
    if (stream.bad()) {
        return stream;  // RETURN
    }

    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    switch (type()) {
    case mqbs::RecordType::e_MESSAGE: {
        mqbs::MessageRecord record;
        loadMessageRecord(&record);
        printer.printAttribute("messageRecord", record);
    } break;
    case mqbs::RecordType::e_CONFIRM: {
        mqbs::ConfirmRecord record;
        loadConfirmRecord(&record);
        printer.printAttribute("confirmRecord", record);
    } break;
    case mqbs::RecordType::e_DELETION: {
        mqbs::DeletionRecord record;
        loadDeletionRecord(&record);
        printer.printAttribute("deletionRecord", record);
    } break;
    case mqbs::RecordType::e_QUEUE_OP: {
        mqbs::QueueOpRecord record;
        loadQueueOpRecord(&record);
        printer.printAttribute("queueOpRecord", record);
    } break;
    case mqbs::RecordType::e_JOURNAL_OP:
    case mqbs::RecordType::e_UNDEFINED: {
        // we should never be here
        BSLS_ASSERT_SAFE(false && "Invalid file store record");
    }
    }
    printer.end();

    return stream;
}

}  // close package namespace
}  // close enterprise namespace
