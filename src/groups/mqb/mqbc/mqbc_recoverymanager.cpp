// Copyright 2021-2023 Bloomberg Finance L.P.
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

// mqbc_recoverymanager.cpp                                           -*-C++-*-
#include <ball_log.h>
#include <bsls_assert.h>
#include <mqbc_recoverymanager.h>
#include <mqbs_qlistfileiterator.h>

#include <mqbscm_version.h>
// IMPLEMENTATION NOTES:
//
// Note that this component is likely to behave just as a helper to
// PartitionFSM as managed and implemented as part of mqbc::StorageManager.
// There are scenarios where recovery is likely to fail or not completed due
// to loss of communication between nodes.
// In these corner cases, rather than explicitly having logic of retrying,
// mqbc::RecoveryManager depends on PartitionFSM inside mqbc::StorageManager to
// reset the state of the node to UNKNOWN and try to heal the storage and
// perform the logic in mqbc::RecoveryManager from the beginning.

// MQB
#include <mqbc_clusterutil.h>
#include <mqbc_recoveryutil.h>
#include <mqbnet_cluster.h>
#include <mqbs_datafileiterator.h>
#include <mqbs_fileset.h>
#include <mqbs_filestoreprotocol.h>
#include <mqbs_filestoreprotocolutil.h>
#include <mqbs_filestoreutil.h>
#include <mqbs_filesystemutil.h>
#include <mqbs_journalfileiterator.h>
#include <mqbs_offsetptr.h>
#include <mqbs_storageutil.h>
#include <mqbu_exit.h>

// BMQ
#include <bmqp_event.h>
#include <bmqp_recoveryeventbuilder.h>
#include <bmqp_storageeventbuilder.h>
#include <bmqp_storagemessageiterator.h>
#include <bmqt_resultcode.h>

#include <bmqtsk_alarmlog.h>
#include <bmqu_blob.h>
#include <bmqu_blobobjectproxy.h>
#include <bmqu_memoutstream.h>

// BDE
#include <bdlb_scopeexit.h>
#include <bdlf_bind.h>
#include <bdls_filesystemutil.h>
#include <bsl_algorithm.h>

namespace BloombergLP {
namespace mqbc {

void RecoveryManager::ChunkDeleter::operator()(
    BSLA_UNUSED const void* ptr) const
{
    // executed by *ANY* thread
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_mfd_sp);
    BSLS_ASSERT_SAFE(d_counter_sp);

    if (0 != --(*d_counter_sp)) {
        return;  // RETURN
    }

    const int rc = mqbs::FileSystemUtil::close(d_mfd_sp.get());
    if (rc != 0) {
        BALL_LOG_WARN << "Failure while closing mapped file descriptor"
                      << ", rc: " << rc;
        return;  // RETURN
    }

    d_mfd_sp->reset();
}

// ------------------------
// class ReceiveDataContext
// ------------------------

// MANIPULATORS
void RecoveryManager::ReceiveDataContext::reset()
{
    d_recoveryDataSource_p = 0;
    d_expectChunks         = false;
    d_recoveryRequestId    = -1;
    d_beginSeqNum.reset();
    d_endSeqNum.reset();
    d_currSeqNum.reset();
}

// ---------------------
// class RecoveryManager
// ---------------------

// CREATORS
RecoveryManager::RecoveryManager(
    const mqbcfg::ClusterDefinition& clusterConfig,
    mqbc::ClusterData&               clusterData,
    const mqbs::DataStoreConfig&     dataStoreConfig,
    bslma::Allocator*                allocator)
: d_allocator_p(allocator)
, d_qListAware(clusterConfig.clusterAttributes().doesFSMwriteQLIST())
, d_blobSpPool_p(&clusterData.blobSpPool())
, d_clusterConfig(clusterConfig)
, d_dataStoreConfig(dataStoreConfig)
, d_clusterData(clusterData)
, d_recoveryContextVec(allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(allocator);
    BSLS_ASSERT_SAFE(d_clusterConfig.clusterAttributes().isCSLModeEnabled() &&
                     d_clusterConfig.clusterAttributes().isFSMWorkflow());

    d_recoveryContextVec.resize(
        clusterConfig.partitionConfig().numPartitions());
}

RecoveryManager::~RecoveryManager()
{
    // NOTHING
}

// MANIPULATORS
int RecoveryManager::start(BSLA_UNUSED bsl::ostream& errorDescription)
{
    // NOTHING
    return 0;
}

void RecoveryManager::stop()
{
    // NOTHING
}

void RecoveryManager::deprecateFileSet(int partitionId)
{
    // executed by the *QUEUE DISPATCHER* thread associated with 'partitionId'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(partitionId >= 0 &&
                     partitionId <
                         d_clusterConfig.partitionConfig().numPartitions());

    RecoveryContext&   recoveryCtx = d_recoveryContextVec[partitionId];
    bmqu::MemOutStream errorDesc;
    int                rc = -1;
    if (recoveryCtx.d_mappedJournalFd.isValid()) {
        rc = mqbs::FileSystemUtil::truncate(&recoveryCtx.d_mappedJournalFd,
                                            recoveryCtx.d_journalFilePosition,
                                            errorDesc);
        if (rc != 0) {
            BMQTSK_ALARMLOG_ALARM("FILE_IO")
                << d_clusterData.identity().description() << " Partition ["
                << partitionId << "]: " << "Failed to truncate journal file ["
                << recoveryCtx.d_recoveryFileSet.journalFile()
                << "], rc: " << rc << ", error: " << errorDesc.str()
                << BMQTSK_ALARMLOG_END;
            errorDesc.reset();
        }

        rc = mqbs::FileSystemUtil::flush(
            recoveryCtx.d_mappedJournalFd.mapping(),
            recoveryCtx.d_journalFilePosition,
            errorDesc);
        if (rc != 0) {
            BMQTSK_ALARMLOG_ALARM("FILE_IO")
                << d_clusterData.identity().description() << " Partition ["
                << partitionId << "]: " << "Failed to flush journal file ["
                << recoveryCtx.d_recoveryFileSet.journalFile()
                << "], rc: " << rc << ", error: " << errorDesc.str()
                << BMQTSK_ALARMLOG_END;
            errorDesc.reset();
        }

        rc = mqbs::FileSystemUtil::close(&recoveryCtx.d_mappedJournalFd);
        if (rc != 0) {
            BMQTSK_ALARMLOG_ALARM("FILE_IO")
                << d_clusterData.identity().description() << " Partition ["
                << partitionId << "]: " << "Failed to close journal file ["
                << recoveryCtx.d_recoveryFileSet.journalFile()
                << "], rc: " << rc << BMQTSK_ALARMLOG_END;
        }
    }
    rc = mqbs::FileSystemUtil::move(
        recoveryCtx.d_recoveryFileSet.journalFile(),
        d_dataStoreConfig.archiveLocation());
    if (0 != rc) {
        BMQTSK_ALARMLOG_ALARM("FILE_IO")
            << d_clusterData.identity().description() << " Partition ["
            << partitionId << "]: " << "Failed to move file ["
            << recoveryCtx.d_recoveryFileSet.journalFile() << "] "
            << "to location [" << d_dataStoreConfig.archiveLocation()
            << "] rc: " << rc << BMQTSK_ALARMLOG_END;
    }
    recoveryCtx.d_journalFilePosition = 0;

    if (recoveryCtx.d_mappedDataFd.isValid()) {
        rc = mqbs::FileSystemUtil::truncate(&recoveryCtx.d_mappedDataFd,
                                            recoveryCtx.d_dataFilePosition,
                                            errorDesc);
        if (rc != 0) {
            BMQTSK_ALARMLOG_ALARM("FILE_IO")
                << d_clusterData.identity().description() << " Partition ["
                << partitionId << "]: " << "Failed to truncate data file ["
                << recoveryCtx.d_recoveryFileSet.dataFile() << "], rc: " << rc
                << ", error: " << errorDesc.str() << BMQTSK_ALARMLOG_END;
            errorDesc.reset();
        }

        rc = mqbs::FileSystemUtil::flush(recoveryCtx.d_mappedDataFd.mapping(),
                                         recoveryCtx.d_dataFilePosition,
                                         errorDesc);
        if (rc != 0) {
            BMQTSK_ALARMLOG_ALARM("FILE_IO")
                << d_clusterData.identity().description() << " Partition ["
                << partitionId << "]: " << "Failed to flush data file ["
                << recoveryCtx.d_recoveryFileSet.dataFile() << "], rc: " << rc
                << ", error: " << errorDesc.str() << BMQTSK_ALARMLOG_END;
            errorDesc.reset();
        }

        rc = mqbs::FileSystemUtil::close(&recoveryCtx.d_mappedDataFd);
        if (rc != 0) {
            BMQTSK_ALARMLOG_ALARM("FILE_IO")
                << d_clusterData.identity().description() << " Partition ["
                << partitionId << "]: " << "Failed to close data file ["
                << recoveryCtx.d_recoveryFileSet.dataFile() << "], rc: " << rc
                << BMQTSK_ALARMLOG_END;
        }
    }
    rc = mqbs::FileSystemUtil::move(recoveryCtx.d_recoveryFileSet.dataFile(),
                                    d_dataStoreConfig.archiveLocation());
    if (0 != rc) {
        BMQTSK_ALARMLOG_ALARM("FILE_IO")
            << d_clusterData.identity().description() << " Partition ["
            << partitionId << "]: " << "Failed to move file ["
            << recoveryCtx.d_recoveryFileSet.dataFile() << "] "
            << "to location [" << d_dataStoreConfig.archiveLocation()
            << "] rc: " << rc << BMQTSK_ALARMLOG_END;
    }
    recoveryCtx.d_dataFilePosition = 0;

    if (recoveryCtx.d_mappedQlistFd.isValid()) {
        rc = mqbs::FileSystemUtil::truncate(&recoveryCtx.d_mappedQlistFd,
                                            recoveryCtx.d_qlistFilePosition,
                                            errorDesc);
        if (rc != 0) {
            BMQTSK_ALARMLOG_ALARM("FILE_IO")
                << d_clusterData.identity().description() << " Partition ["
                << partitionId << "]: " << "Failed to truncate QList file ["
                << recoveryCtx.d_recoveryFileSet.qlistFile() << "], rc: " << rc
                << ", error: " << errorDesc.str() << BMQTSK_ALARMLOG_END;
            errorDesc.reset();
        }

        rc = mqbs::FileSystemUtil::flush(recoveryCtx.d_mappedQlistFd.mapping(),
                                         recoveryCtx.d_qlistFilePosition,
                                         errorDesc);
        if (rc != 0) {
            BMQTSK_ALARMLOG_ALARM("FILE_IO")
                << d_clusterData.identity().description() << " Partition ["
                << partitionId << "]: " << "Failed to flush QList file ["
                << recoveryCtx.d_recoveryFileSet.qlistFile() << "], rc: " << rc
                << ", error: " << errorDesc.str() << BMQTSK_ALARMLOG_END;
            errorDesc.reset();
        }

        rc = mqbs::FileSystemUtil::close(&recoveryCtx.d_mappedQlistFd);
        if (rc != 0) {
            BMQTSK_ALARMLOG_ALARM("FILE_IO")
                << d_clusterData.identity().description() << " Partition ["
                << partitionId << "]: " << "Failed to close QList file ["
                << recoveryCtx.d_recoveryFileSet.qlistFile() << "], rc: " << rc
                << BMQTSK_ALARMLOG_END;
        }
    }
    rc = mqbs::FileSystemUtil::move(recoveryCtx.d_recoveryFileSet.qlistFile(),
                                    d_dataStoreConfig.archiveLocation());
    if (0 != rc) {
        BMQTSK_ALARMLOG_ALARM("FILE_IO")
            << d_clusterData.identity().description() << " Partition ["
            << partitionId << "]: " << "Failed to move file ["
            << recoveryCtx.d_recoveryFileSet.qlistFile() << "] "
            << "to location [" << d_dataStoreConfig.archiveLocation()
            << "] rc: " << rc << BMQTSK_ALARMLOG_END;
    }
    recoveryCtx.d_qlistFilePosition = 0;
}

void RecoveryManager::setExpectedDataChunkRange(
    int                                          partitionId,
    const mqbs::FileStore&                       fs,
    mqbnet::ClusterNode*                         source,
    const bmqp_ctrlmsg::PartitionSequenceNumber& beginSeqNum,
    const bmqp_ctrlmsg::PartitionSequenceNumber& endSeqNum,
    int                                          requestId)
{
    // executed by the *QUEUE DISPATCHER* thread associated with 'partitionId'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(fs.inDispatcherThread());
    BSLS_ASSERT_SAFE(partitionId >= 0 &&
                     partitionId <
                         d_clusterConfig.partitionConfig().numPartitions());
    BSLS_ASSERT_SAFE(source);
    BSLS_ASSERT_SAFE(beginSeqNum < endSeqNum);

    RecoveryContext&    recoveryCtx    = d_recoveryContextVec[partitionId];
    ReceiveDataContext& receiveDataCtx = recoveryCtx.d_receiveDataContext;
    if (receiveDataCtx.d_expectChunks) {
        BALL_LOG_ERROR << d_clusterData.identity().description()
                       << " Partition [" << partitionId << "]: "
                       << "Got notification to expect chunks when self is "
                       << "already expecting chunks.  Self's view: "
                       << "recovery requestId = "
                       << receiveDataCtx.d_recoveryRequestId
                       << "; beginSeqNum = " << receiveDataCtx.d_beginSeqNum
                       << "; endSeqNum = " << receiveDataCtx.d_endSeqNum
                       << "; currSeqNum = " << receiveDataCtx.d_currSeqNum
                       << ".  Please review Partition FSM logic.";
    }

    receiveDataCtx.d_recoveryDataSource_p = source;
    receiveDataCtx.d_expectChunks         = true;
    receiveDataCtx.d_recoveryRequestId    = requestId;
    receiveDataCtx.d_beginSeqNum          = beginSeqNum;
    receiveDataCtx.d_endSeqNum            = endSeqNum;
    receiveDataCtx.d_currSeqNum           = beginSeqNum;
    if (fs.isOpen()) {
        BSLS_ASSERT_SAFE(receiveDataCtx.d_currSeqNum.primaryLeaseId() ==
                         fs.primaryLeaseId());
        BSLS_ASSERT_SAFE(receiveDataCtx.d_currSeqNum.sequenceNumber() ==
                         fs.sequenceNumber());
    }
    else {
        BSLS_ASSERT_SAFE(recoveryCtx.d_mappedJournalFd.isValid() &&
                         recoveryCtx.d_mappedDataFd.isValid());
        if (d_qListAware) {
            BSLS_ASSERT_SAFE(recoveryCtx.d_mappedQlistFd.isValid());
        }
    }

    BALL_LOG_INFO_BLOCK
    {
        BALL_LOG_OUTPUT_STREAM
            << d_clusterData.identity().description() << " Partition ["
            << partitionId
            << "]: " << "Got notification to expect data chunks "
            << "of range " << beginSeqNum << " to " << endSeqNum << " from "
            << source->nodeDescription() << ".";
        if (requestId != -1) {
            BALL_LOG_OUTPUT_STREAM << " Recovery requestId is " << requestId
                                   << ".";
        }
    }
}

void RecoveryManager::resetReceiveDataCtx(int partitionId)
{
    // executed by the *QUEUE DISPATCHER* thread associated with 'partitionId'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(partitionId >= 0 &&
                     partitionId <
                         d_clusterConfig.partitionConfig().numPartitions());

    d_recoveryContextVec[partitionId].d_receiveDataContext.reset();
}

int RecoveryManager::processSendDataChunks(
    int                                          partitionId,
    mqbnet::ClusterNode*                         destination,
    const bmqp_ctrlmsg::PartitionSequenceNumber& beginSeqNum,
    const bmqp_ctrlmsg::PartitionSequenceNumber& endSeqNum,
    const mqbs::FileStore&                       fs,
    PartitionDoneSendDataChunksCb                doneDataChunksCb)
{
    // executed by the *QUEUE DISPATCHER* thread associated with 'partitionId'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(fs.inDispatcherThread());
    BSLS_ASSERT_SAFE(fs.isOpen());

    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS                  = 0,
        rc_LOAD_FD_FAILURE          = -1,
        rc_JOURNAL_ITERATOR_FAILURE = -2,
        rc_INVALID_SEQUENCE_NUMBER  = -3,
        rc_BUILDER_FAILURE          = -4,
        rc_WRITE_FAILURE            = -5,
        rc_INCOMPLETE_REPLAY        = -6
    };

    int rc = rc_SUCCESS;

    bdlb::ScopeExitAny guardDoneDataChunks(
        bdlf::BindUtil::bind(doneDataChunksCb, partitionId, destination, rc));

    if (beginSeqNum == endSeqNum) {
        return rc_SUCCESS;  // RETURN
    }

    mqbs::FileStoreSet fileSet;

    fs.loadCurrentFiles(&fileSet);

    bsl::shared_ptr<mqbs::MappedFileDescriptor> mappedJournalFd =
        bsl::make_shared<mqbs::MappedFileDescriptor>();
    bsl::shared_ptr<mqbs::MappedFileDescriptor> mappedDataFd =
        bsl::make_shared<mqbs::MappedFileDescriptor>();
    // Only used if d_qlistAware == true
    bsl::shared_ptr<mqbs::MappedFileDescriptor> mappedQlistFd =
        bsl::make_shared<mqbs::MappedFileDescriptor>();

    rc = RecoveryUtil::loadFileDescriptors(mappedJournalFd.get(),
                                           mappedDataFd.get(),
                                           fileSet,
                                           d_qListAware ? mappedQlistFd.get()
                                                        : 0);
    if (rc != 0) {
        return rc * 10 + rc_LOAD_FD_FAILURE;  // RETURN
    }

    bsl::shared_ptr<bsls::AtomicInt> journalChunkDeleterCounter =
        bsl::make_shared<bsls::AtomicInt>(0);
    bsl::shared_ptr<bsls::AtomicInt> dataChunkDeleterCounter =
        bsl::make_shared<bsls::AtomicInt>(0);
    // Only used if d_qlistAware == true
    bsl::shared_ptr<bsls::AtomicInt> qlistChunkDeleterCounter =
        bsl::make_shared<bsls::AtomicInt>(0);

    // Scope Exit Guards to unmap the fds incase of errors.
    bdlb::ScopeExit<ChunkDeleter> guardJournalFd(
        ChunkDeleter(mappedJournalFd, journalChunkDeleterCounter));
    bdlb::ScopeExit<ChunkDeleter> guardDataFd(
        ChunkDeleter(mappedDataFd, dataChunkDeleterCounter));
    // Only used if d_qListAware == true
    bsl::optional<bdlb::ScopeExit<ChunkDeleter> > guardQlistFd;
    if (d_qListAware) {
        guardQlistFd.emplace(
            ChunkDeleter(mappedQlistFd, qlistChunkDeleterCounter));
    }

    RecoveryUtil::validateArgs(beginSeqNum, endSeqNum, destination);

    mqbs::JournalFileIterator journalIt;
    rc = journalIt.reset(
        mappedJournalFd.get(),
        mqbs::FileStoreProtocolUtil::bmqHeader(*mappedJournalFd.get()));
    if (0 != rc) {
        return rc * 10 + rc_JOURNAL_ITERATOR_FAILURE;  // RETURN
    }

    bmqp_ctrlmsg::PartitionSequenceNumber currentSeqNum;
    rc = RecoveryUtil::bootstrapCurrentSeqNum(&currentSeqNum,
                                              journalIt,
                                              beginSeqNum);
    if (rc != 0) {
        return rc * 10 + rc_INVALID_SEQUENCE_NUMBER;  // RETURN
    }

    bmqp::StorageEventBuilder builder(mqbs::FileStoreProtocol::k_VERSION,
                                      bmqp::EventType::e_PARTITION_SYNC,
                                      d_blobSpPool_p,
                                      d_allocator_p);

    // Note that partition has to be replayed from the record *after*
    // 'beginSeqNum'.  So move forward by one record in the JOURNAL.
    while (currentSeqNum < endSeqNum) {
        char* journalRecordBase = 0;
        int journalRecordLen = mqbs::FileStoreProtocol::k_JOURNAL_RECORD_SIZE;
        char*                          payloadRecordBase = 0;
        int                            payloadRecordLen  = 0;
        bmqp::StorageMessageType::Enum storageMsgType =
            bmqp::StorageMessageType::e_UNDEFINED;

        rc = RecoveryUtil::incrementCurrentSeqNum(
            &currentSeqNum,
            &journalRecordBase,
            *mappedJournalFd,
            endSeqNum,
            partitionId,
            *destination,
            d_clusterData.identity().description(),
            journalIt);
        if (rc == 1) {
            break;
        }
        else if (rc < 0) {
            return rc * 10 + rc_JOURNAL_ITERATOR_FAILURE;  // RETURN
        }

        RecoveryUtil::processJournalRecord(&storageMsgType,
                                           &payloadRecordBase,
                                           &payloadRecordLen,
                                           journalIt,
                                           *mappedDataFd,
                                           d_qListAware,
                                           *mappedQlistFd);

        BSLS_ASSERT_SAFE(bmqp::StorageMessageType::e_UNDEFINED !=
                         storageMsgType);
        BSLS_ASSERT_SAFE(0 != journalRecordBase);
        BSLS_ASSERT_SAFE(0 != journalRecordLen);

        bsl::shared_ptr<char> journalRecordSp(
            journalRecordBase,
            ChunkDeleter(mappedJournalFd, journalChunkDeleterCounter));

        bdlbb::BlobBuffer journalRecordBlobBuffer(journalRecordSp,
                                                  journalRecordLen);

        bmqt::EventBuilderResult::Enum builderRc;
        if (0 != payloadRecordBase) {
            BSLS_ASSERT_SAFE(0 != payloadRecordLen);
            if (d_qListAware) {
                BSLS_ASSERT_SAFE(
                    bmqp::StorageMessageType::e_DATA == storageMsgType ||
                    bmqp::StorageMessageType::e_QLIST == storageMsgType);
            }
            else {
                BSLS_ASSERT_SAFE(bmqp::StorageMessageType::e_DATA ==
                                 storageMsgType);
            }

            bsl::shared_ptr<char> payloadRecordSp(
                payloadRecordBase,
                bmqp::StorageMessageType::e_DATA == storageMsgType
                    ? ChunkDeleter(mappedDataFd, dataChunkDeleterCounter)
                    : ChunkDeleter(mappedQlistFd, qlistChunkDeleterCounter));

            bdlbb::BlobBuffer payloadRecordBlobBuffer(payloadRecordSp,
                                                      payloadRecordLen);

            builderRc = builder.packMessage(storageMsgType,
                                            partitionId,
                                            0,  // flags
                                            journalIt.recordOffset() /
                                                bmqp::Protocol::k_WORD_SIZE,
                                            journalRecordBlobBuffer,
                                            payloadRecordBlobBuffer);
        }
        else {
            builderRc = builder.packMessage(storageMsgType,
                                            partitionId,
                                            0,  // flags
                                            journalIt.recordOffset() /
                                                bmqp::Protocol::k_WORD_SIZE,
                                            journalRecordBlobBuffer);
        }

        if (bmqt::EventBuilderResult::e_SUCCESS != builderRc) {
            return rc_BUILDER_FAILURE + 10 * static_cast<int>(builderRc);
            // RETURN
        }

        if (d_clusterConfig.partitionConfig()
                .syncConfig()
                .partitionSyncEventSize() <= builder.eventSize()) {
            const bmqt::GenericResult::Enum writeRc = destination->write(
                builder.blob(),
                bmqp::EventType::e_PARTITION_SYNC);

            if (bmqt::GenericResult::e_SUCCESS != writeRc) {
                return static_cast<int>(writeRc) * 10 + rc_WRITE_FAILURE;
                // RETURN
            }

            builder.reset();
        }
    }

    if (currentSeqNum != endSeqNum) {
        BALL_LOG_WARN << d_clusterData.identity().description()
                      << " Partition [" << partitionId
                      << "]: incomplete replay of partition. Sequence number "
                      << "of last record sent: " << currentSeqNum
                      << ", was supposed to send up to: " << endSeqNum
                      << ". Peer: " << destination->nodeDescription() << ".";
        return rc_INCOMPLETE_REPLAY;  // RETURN
    }

    if (0 < builder.messageCount()) {
        const bmqt::GenericResult::Enum writeRc = destination->write(
            builder.blob(),
            bmqp::EventType::e_PARTITION_SYNC);

        if (bmqt::GenericResult::e_SUCCESS != writeRc) {
            return static_cast<int>(writeRc) * 10 +
                   rc_WRITE_FAILURE;  // RETURN
        }
    }

    BALL_LOG_INFO << d_clusterData.identity().description() << " Partition ["
                  << partitionId << "]: " << "Sent data chunks from "
                  << beginSeqNum << " to " << endSeqNum
                  << " to node: " << destination->nodeDescription() << ".";

    return rc_SUCCESS;
}

int RecoveryManager::processReceiveDataChunks(
    const bsl::shared_ptr<bdlbb::Blob>& blob,
    mqbnet::ClusterNode*                source,
    mqbs::FileStore*                    fs,
    int                                 partitionId)
{
    // executed by the *QUEUE DISPATCHER* thread associated with 'partitionId'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(fs && fs->inDispatcherThread());
    BSLS_ASSERT_SAFE(source);
    BSLS_ASSERT_SAFE(0 <= partitionId);

    enum RcEnum {
        // Value for the various RC error categories
        rc_LAST_DATA_CHUNK                   = 1,
        rc_SUCCESS                           = 0,
        rc_UNEXPECTED_DATA                   = -1,
        rc_INVALID_RECOVERY_PEER             = -2,
        rc_INVALID_STORAGE_HDR               = -3,
        rc_INVALID_RECORD_SEQ_NUM            = -4,
        rc_JOURNAL_OUT_OF_SYNC               = -5,
        rc_WRITE_MESSAGE_RECORD_ERROR        = -6,
        rc_WRITE_QUEUE_CREATION_RECORD_ERROR = -7,
        rc_INVALID_QUEUE_RECORD              = -8,
    };

    RecoveryContext&    recoveryCtx    = d_recoveryContextVec[partitionId];
    ReceiveDataContext& receiveDataCtx = recoveryCtx.d_receiveDataContext;
    if (!receiveDataCtx.d_expectChunks) {
        BMQTSK_ALARMLOG_ALARM("RECOVERY")
            << d_clusterData.identity().description() << " Partition ["
            << partitionId
            << "]: " << "Received partition-sync event from node "
            << source->nodeDescription()
            << ", but self is not expecting data chunks. "
            << "Ignoring this event." << BMQTSK_ALARMLOG_END;
        return rc_UNEXPECTED_DATA;  // RETURN
    }

    BSLS_ASSERT_SAFE(receiveDataCtx.d_recoveryDataSource_p);
    if (receiveDataCtx.d_recoveryDataSource_p->nodeId() != source->nodeId()) {
        BMQTSK_ALARMLOG_ALARM("RECOVERY")
            << d_clusterData.identity().description() << " Partition ["
            << partitionId
            << "]: " << "Received partition-sync event from node "
            << source->nodeDescription()
            << ", which is not identified as recovery peer node "
            << receiveDataCtx.d_recoveryDataSource_p->nodeDescription()
            << ". Ignoring this event." << BMQTSK_ALARMLOG_END;
        return rc_INVALID_RECOVERY_PEER;  // RETURN
    }

    if (fs->isOpen()) {
        BSLS_ASSERT_SAFE(receiveDataCtx.d_currSeqNum.primaryLeaseId() ==
                         fs->primaryLeaseId());
        BSLS_ASSERT_SAFE(receiveDataCtx.d_currSeqNum.sequenceNumber() ==
                         fs->sequenceNumber());

        fs->processStorageEvent(blob, true /* isPartitionSyncEvent */, source);

        receiveDataCtx.d_currSeqNum.primaryLeaseId() = fs->primaryLeaseId();
        receiveDataCtx.d_currSeqNum.sequenceNumber() = fs->sequenceNumber();

        if (receiveDataCtx.d_currSeqNum == receiveDataCtx.d_endSeqNum) {
            return rc_LAST_DATA_CHUNK;  // RETURN
        }
        else if (receiveDataCtx.d_currSeqNum > receiveDataCtx.d_endSeqNum) {
            BMQTSK_ALARMLOG_ALARM("REPLICATION")
                << d_clusterData.identity().description() << " Partition ["
                << partitionId << "]: "
                << "The last partition sync msg inside a storage event "
                << "processed by FileStore has sequenceNumber "
                << receiveDataCtx.d_currSeqNum
                << ", larger than self's expected ending sequence number of "
                << "data chunks: " << receiveDataCtx.d_endSeqNum << "."
                << BMQTSK_ALARMLOG_END;
            return rc_INVALID_RECORD_SEQ_NUM;  // RETURN
        }

        return rc_SUCCESS;  // RETURN
    }

    bmqp::Event event(blob.get(), d_allocator_p);
    BSLS_ASSERT_SAFE(event.isPartitionSyncEvent());

    bmqp::StorageMessageIterator iter;
    event.loadStorageMessageIterator(&iter);
    BSLS_ASSERT_SAFE(iter.isValid());

    while (1 == iter.next()) {
        const bmqp::StorageHeader&                header = iter.header();
        bmqu::BlobPosition                        recordPosition;
        bmqu::BlobObjectProxy<mqbs::RecordHeader> recHeader;

        bmqu::MemOutStream partitionDesc;
        partitionDesc << d_clusterData.identity().description()
                      << " Partition [" << partitionId << "]: ";

        int rc = mqbs::StorageUtil::loadRecordHeaderAndPos(
            &recHeader,
            &recordPosition,
            iter,
            blob,
            partitionDesc.str());
        if (rc != 0) {
            return rc * 10 + rc_INVALID_STORAGE_HDR;  // RETURN
        }

        // Check sequence number (only if leaseId is same).  Received leaseId
        // cannot be smaller.

        bmqp_ctrlmsg::PartitionSequenceNumber recordSeqNum;
        recordSeqNum.primaryLeaseId() = recHeader->primaryLeaseId();
        recordSeqNum.sequenceNumber() = recHeader->sequenceNumber();

        if (recordSeqNum <= receiveDataCtx.d_currSeqNum) {
            BMQTSK_ALARMLOG_ALARM("REPLICATION")
                << d_clusterData.identity().description() << " Partition ["
                << partitionId
                << "]: " << "Received partition sync msg of type "
                << header.messageType() << " with sequenceNumber "
                << recordSeqNum
                << ", smaller than or equal to self current sequence number: "
                << receiveDataCtx.d_endSeqNum
                << ". Record's journal offset (in words): "
                << header.journalOffsetWords() << ". Ignoring entire event."
                << BMQTSK_ALARMLOG_END;
            return rc_INVALID_RECORD_SEQ_NUM;  // RETURN
        }

        if (recordSeqNum > receiveDataCtx.d_endSeqNum) {
            BMQTSK_ALARMLOG_ALARM("REPLICATION")
                << d_clusterData.identity().description() << " Partition ["
                << partitionId
                << "]: " << "Received partition sync msg of type "
                << header.messageType() << " with sequenceNumber "
                << recordSeqNum
                << ", larger than self's expected ending sequence number of "
                << "data chunks: " << receiveDataCtx.d_endSeqNum
                << ". Record's journal offset (in words): "
                << header.journalOffsetWords() << ". Ignoring entire event."
                << BMQTSK_ALARMLOG_END;
            return rc_INVALID_RECORD_SEQ_NUM;  // RETURN
        }

        // Local refs for convenience.

        mqbs::MappedFileDescriptor& journal = recoveryCtx.d_mappedJournalFd;
        bsls::Types::Uint64& journalPos = recoveryCtx.d_journalFilePosition;
        BSLS_ASSERT_SAFE(journal.isValid());

        // Ensure that JOURNAL offset of source and self match.

        const bsls::Types::Uint64 sourceJournalOffset =
            static_cast<bsls::Types::Uint64>(header.journalOffsetWords()) *
            bmqp::Protocol::k_WORD_SIZE;

        if (journalPos != sourceJournalOffset) {
            // Source's and self views of the journal have diverged.

            BMQTSK_ALARMLOG_ALARM("REPLICATION")
                << d_clusterData.identity().description() << " Partition ["
                << partitionId << "]: " << "Received journal record of type ["
                << header.messageType() << "] with journal offset mismatch. "
                << "Source's journal offset: " << sourceJournalOffset
                << ", self journal offset: " << journalPos
                << ", msg sequence number (" << recHeader->primaryLeaseId()
                << ", " << recHeader->sequenceNumber()
                << "). Ignoring this message." << BMQTSK_ALARMLOG_END;
            return rc_JOURNAL_OUT_OF_SYNC;  // RETURN
        }

        if (bmqp::StorageMessageType::e_DATA == header.messageType()) {
            mqbs::MappedFileDescriptor& dataFile = recoveryCtx.d_mappedDataFd;
            bsls::Types::Uint64& dataFilePos = recoveryCtx.d_dataFilePosition;
            bsls::Types::Uint64  dataOffset  = dataFilePos;
            BSLS_ASSERT_SAFE(dataFile.isValid());
            BSLS_ASSERT_SAFE(0 == dataOffset % bmqp::Protocol::k_DWORD_SIZE);

            rc = mqbs::FileStoreUtil::writeMessageRecordImpl(&journalPos,
                                                             &dataFilePos,
                                                             *blob,
                                                             recordPosition,
                                                             journal,
                                                             dataFile,
                                                             dataOffset);
            if (0 != rc) {
                return 10 * rc + rc_WRITE_MESSAGE_RECORD_ERROR;  // RETURN
            }
        }
        else if (bmqp::StorageMessageType::e_QLIST == header.messageType()) {
            mqbs::MappedFileDescriptor& qlistFile =
                recoveryCtx.d_mappedQlistFd;
            bsls::Types::Uint64& qlistFilePos =
                recoveryCtx.d_qlistFilePosition;
            bsls::Types::Uint64 qlistOffset = qlistFilePos;
            BSLS_ASSERT_SAFE(0 == qlistOffset % bmqp::Protocol::k_WORD_SIZE);

            mqbi::Storage::AppInfos appIdKeyPairs;
            rc = mqbs::FileStoreUtil::writeQueueCreationRecordImpl(
                &journalPos,
                &qlistFilePos,
                &appIdKeyPairs,
                partitionId,
                *blob,
                recordPosition,
                journal,
                d_qListAware,
                qlistFile,
                qlistOffset);
            if (0 != rc) {
                return 10 * rc +
                       rc_WRITE_QUEUE_CREATION_RECORD_ERROR;  // RETURN
            }
        }
        else {
            // Append record to journal.  Note that here, we only assert on
            // journal file having space for 1 record.  This is because the
            // journal record being written could be a sync point record
            // with subType == SyncPointType::e_ROLLOVER, in which case,
            // journal may not have space for more than 1 record.

            BSLS_ASSERT_SAFE(
                journal.fileSize() >=
                (journalPos +
                 mqbs::FileStoreProtocol ::k_JOURNAL_RECORD_SIZE));

            // Keep track of journal record's offset.

            const bsls::Types::Uint64 recordOffset = journalPos;

            bmqu::BlobUtil::copyToRawBufferFromIndex(
                journal.block().base() + recordOffset,
                *blob,
                recordPosition.buffer(),
                recordPosition.byte(),
                mqbs::FileStoreProtocol::k_JOURNAL_RECORD_SIZE);
            journalPos += mqbs::FileStoreProtocol::k_JOURNAL_RECORD_SIZE;

            if (header.messageType() == bmqp::StorageMessageType::e_QLIST) {
                mqbs::OffsetPtr<const mqbs::QueueOpRecord> queueRec(
                    journal.block(),
                    recordOffset);
                if (mqbs::QueueOpType::e_CREATION != queueRec->type() &&
                    mqbs::QueueOpType::e_ADDITION != queueRec->type()) {
                    BALL_LOG_ERROR << d_clusterData.identity().description()
                                   << " Partition [" << partitionId
                                   << "]: " << " Unexpected QueueOpType: "
                                   << queueRec->type();
                    return rc_INVALID_QUEUE_RECORD;  // RETURN
                }
            }
        };

        receiveDataCtx.d_currSeqNum = recordSeqNum;
        if (receiveDataCtx.d_currSeqNum == receiveDataCtx.d_endSeqNum) {
            return rc_LAST_DATA_CHUNK;  // RETURN
        }
    }  // end: while loop

    return rc_SUCCESS;
}

int RecoveryManager::createRecoveryFileSet(bsl::ostream&    errorDescription,
                                           mqbs::FileStore* fs,
                                           int              partitionId)
{
    // executed by the *QUEUE DISPATCHER* thread associated with 'partitionId'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(fs && fs->inDispatcherThread());

    bsl::shared_ptr<mqbs::FileSet> fileSetSp;

    bmqu::MemOutStream partitionDesc;
    partitionDesc << "Partition [" << partitionId
                  << "] (cluster: " << d_clusterData.cluster().name() << "): ";

    int rc = mqbs::FileStoreUtil::create(errorDescription,
                                         &fileSetSp,
                                         fs,
                                         partitionId,
                                         d_dataStoreConfig,
                                         partitionDesc.str(),
                                         d_qListAware,  // needQList
                                         d_allocator_p);
    if (rc != 0) {
        return rc;  // RETURN
    }

    RecoveryContext& recoveryCtx = d_recoveryContextVec[partitionId];
    recoveryCtx.d_recoveryFileSet.setJournalFile(fileSetSp->d_journalFileName)
        .setJournalFileSize(d_dataStoreConfig.maxJournalFileSize())
        .setDataFile(fileSetSp->d_dataFileName)
        .setDataFileSize(d_dataStoreConfig.maxDataFileSize())
        .setQlistFileSize(d_dataStoreConfig.maxQlistFileSize())
        .setQlistFile(fileSetSp->d_qlistFileName);

    mqbs::FileStoreUtil::loadCurrentFiles(&recoveryCtx.d_recoveryFileSet,
                                          *fileSetSp,
                                          d_qListAware);  // needQList

    recoveryCtx.d_mappedJournalFd     = fileSetSp->d_journalFile;
    recoveryCtx.d_journalFilePosition = fileSetSp->d_journalFilePosition;
    recoveryCtx.d_mappedDataFd        = fileSetSp->d_dataFile;
    recoveryCtx.d_dataFilePosition    = fileSetSp->d_dataFilePosition;
    BSLS_ASSERT_SAFE(recoveryCtx.d_mappedJournalFd.isValid());
    BSLS_ASSERT_SAFE(recoveryCtx.d_mappedDataFd.isValid());
    if (d_qListAware) {
        recoveryCtx.d_mappedQlistFd     = fileSetSp->d_qlistFile;
        recoveryCtx.d_qlistFilePosition = fileSetSp->d_qlistFilePosition;
        BSLS_ASSERT_SAFE(recoveryCtx.d_mappedQlistFd.isValid());
    }

    BALL_LOG_INFO_BLOCK
    {
        BALL_LOG_OUTPUT_STREAM
            << d_clusterData.identity().description() << " Partition ["
            << partitionId << "]: " << "Created recovery data file store set: "
            << recoveryCtx.d_recoveryFileSet
            << ", journal file position: " << recoveryCtx.d_journalFilePosition
            << ", data file position: " << recoveryCtx.d_dataFilePosition;
        if (d_qListAware) {
            BALL_LOG_OUTPUT_STREAM << ", qlist file position: "
                                   << fileSetSp->d_qlistFilePosition;
        }
    }

    return 0;
}

int RecoveryManager::openRecoveryFileSet(bsl::ostream& errorDescription,
                                         int           partitionId)
{
    // executed by the *QUEUE DISPATCHER* thread associated with 'partitionId'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(partitionId >= 0 &&
                     partitionId <
                         d_clusterConfig.partitionConfig().numPartitions());

    enum RcEnum {
        // Value for the various RC error categories
        rc_NO_FILE_SETS_TO_RECOVER = 1  // Special rc, do not change
        ,
        rc_SUCCESS               = 0,
        rc_OPEN_FILE_SET_FAILURE = -1,
        rc_INVALID_FILE_SET      = -2,
        rc_FILE_ITERATOR_FAILURE = -3,
        rc_INVALID_DATA_OFFSET   = -4,
        rc_INVALID_QLIST_OFFSET  = -5,
        rc_INVALID_QLIST_RECORD  = -6
    };

    const int        k_MAX_NUM_FILE_SETS_TO_CHECK = 2;
    RecoveryContext& recoveryCtx = d_recoveryContextVec[partitionId];

    if (recoveryCtx.d_mappedJournalFd.isValid()) {
        BSLS_ASSERT_SAFE(recoveryCtx.d_mappedDataFd.isValid());
        if (d_qListAware) {
            BSLS_ASSERT_SAFE(recoveryCtx.d_mappedQlistFd.isValid());
        }

        BALL_LOG_INFO_BLOCK
        {
            BALL_LOG_OUTPUT_STREAM
                << d_clusterData.identity().description() << " Partition ["
                << partitionId << "]: "
                << "Not opening recovery file set because it's already "
                << "opened.  Current recovery file set: "
                << recoveryCtx.d_recoveryFileSet << ", journal file position: "
                << recoveryCtx.d_journalFilePosition
                << ", data file position: " << recoveryCtx.d_dataFilePosition;
            if (d_qListAware) {
                BALL_LOG_OUTPUT_STREAM << ", QList file position: "
                                       << recoveryCtx.d_qlistFilePosition;
            }
            BALL_LOG_OUTPUT_STREAM << ".";
        }
        return rc_SUCCESS;  // RETURN
    }
    BSLS_ASSERT_SAFE(!recoveryCtx.d_mappedDataFd.isValid() &&
                     !recoveryCtx.d_mappedQlistFd.isValid());

    int rc = mqbs::FileStoreUtil::openRecoveryFileSet(
        errorDescription,
        &recoveryCtx.d_mappedJournalFd,
        &recoveryCtx.d_mappedDataFd,
        &recoveryCtx.d_recoveryFileSet,
        &recoveryCtx.d_journalFilePosition,
        &recoveryCtx.d_dataFilePosition,
        partitionId,
        k_MAX_NUM_FILE_SETS_TO_CHECK,
        d_dataStoreConfig,
        false,  // readOnly
        d_qListAware ? &recoveryCtx.d_mappedQlistFd : 0,
        d_qListAware ? &recoveryCtx.d_qlistFilePosition : 0);
    if (rc == 1) {
        return rc_NO_FILE_SETS_TO_RECOVER;  // RETURN
    }
    else if (rc != 0) {
        return rc * 10 + rc_OPEN_FILE_SET_FAILURE;  // RETURN
    }

    rc = mqbs::FileStoreUtil::validateFileSet(
        recoveryCtx.d_mappedJournalFd,
        recoveryCtx.d_mappedDataFd,
        d_qListAware ? recoveryCtx.d_mappedQlistFd
                     : mqbs::MappedFileDescriptor());
    if (rc != 0) {
        errorDescription << d_clusterData.identity().description()
                         << " Partition [" << partitionId << "]: "
                         << "File set: " << recoveryCtx.d_recoveryFileSet
                         << " validation failed, rc: " << rc;
        mqbs::FileSystemUtil::close(&recoveryCtx.d_mappedJournalFd);
        mqbs::FileSystemUtil::close(&recoveryCtx.d_mappedDataFd);
        if (d_qListAware) {
            mqbs::FileSystemUtil::close(&recoveryCtx.d_mappedQlistFd);
        }

        return rc * 10 + rc_INVALID_FILE_SET;  // RETURN
    }

    // Set journal, data and QList file position to the last record of the
    // journal and data file respectively.
    mqbs::JournalFileIterator jit;
    mqbs::DataFileIterator    dit;
    mqbs::QlistFileIterator   qit;
    rc = mqbs::FileStoreUtil::loadIterators(errorDescription,
                                            recoveryCtx.d_recoveryFileSet,
                                            &jit,
                                            recoveryCtx.d_mappedJournalFd,
                                            &dit,
                                            recoveryCtx.d_mappedDataFd,
                                            d_qListAware ? &qit : 0,
                                            recoveryCtx.d_mappedQlistFd);
    if (rc != 0) {
        return 10 * rc + rc_FILE_ITERATOR_FAILURE;  // RETURN
    }

    mqbs::FileStoreUtil::setFileHeaderOffsets(
        &recoveryCtx.d_journalFilePosition,
        &recoveryCtx.d_dataFilePosition,
        jit,
        dit,
        d_qListAware,
        d_qListAware ? &recoveryCtx.d_qlistFilePosition : 0,
        d_qListAware ? qit : mqbs::QlistFileIterator());

    bool isLastJournalRecord = true;          // ie, first in iteration
    bool isLastMessageRecord = true;          // ie, first in iteration
    bool isLastQlistRecord   = d_qListAware;  // ie, first in iteration
    while (1 == (rc = jit.nextRecord())) {
        const mqbs::RecordHeader& recHeader = jit.recordHeader();
        mqbs::RecordType::Enum    rt        = recHeader.type();

        if (isLastJournalRecord) {
            isLastJournalRecord               = false;
            recoveryCtx.d_journalFilePosition = jit.recordOffset() +
                                                (jit.header().recordWords() *
                                                 bmqp::Protocol::k_WORD_SIZE);
        }

        if (mqbs::RecordType::e_MESSAGE == rt) {
            const mqbs::MessageRecord& rec = jit.asMessageRecord();

            bsls::Types::Uint64 dataHeaderOffset =
                static_cast<bsls::Types::Uint64>(rec.messageOffsetDwords()) *
                bmqp::Protocol::k_DWORD_SIZE;

            if (0 == dataHeaderOffset) {
                errorDescription
                    << d_clusterData.identity().description() << " Partition ["
                    << partitionId
                    << "]: " << "Encountered a MESSAGE record with GUID ["
                    << rec.messageGUID() << "], queueKey [" << rec.queueKey()
                    << "], but invalid DATA file offset field. Journal record "
                    << "offset: " << jit.recordOffset()
                    << ", journal record index: " << jit.recordIndex() << ".";

                return rc_INVALID_DATA_OFFSET;  // RETURN
            }

            if (dataHeaderOffset > recoveryCtx.d_mappedDataFd.fileSize()) {
                errorDescription
                    << d_clusterData.identity().description() << " Partition ["
                    << partitionId
                    << "]: " << "Encountered a MESSAGE record with GUID ["
                    << rec.messageGUID() << "], queueKey [" << rec.queueKey()
                    << "], but out-of-bound DATA file offset field: "
                    << dataHeaderOffset << ", DATA file size: "
                    << recoveryCtx.d_mappedDataFd.fileSize()
                    << ". Journal record offset: " << jit.recordOffset()
                    << ", journal record index: " << jit.recordIndex() << ".";

                return rc_INVALID_DATA_OFFSET;  // RETURN
            }

            // Update 'd_dataFilePosition' if it's the last message record (ie,
            // first in the iteration since we are iterating backwards).
            if (isLastMessageRecord) {
                mqbs::OffsetPtr<const mqbs::DataHeader> dataHeader(
                    dit.mappedFileDescriptor()->block(),
                    dataHeaderOffset);
                const unsigned int totalLen = dataHeader->messageWords() *
                                              bmqp::Protocol::k_WORD_SIZE;

                recoveryCtx.d_dataFilePosition = dataHeaderOffset + totalLen;
                isLastMessageRecord            = false;
            }
        }

        if (d_qListAware && mqbs::RecordType::e_QUEUE_OP == rt) {
            const mqbs::QueueOpRecord& rec         = jit.asQueueOpRecord();
            mqbs::QueueOpType::Enum    queueOpType = rec.type();
            BSLS_ASSERT_SAFE(mqbs::QueueOpType::e_UNDEFINED != queueOpType);

            if ((mqbs::QueueOpType::e_CREATION != queueOpType &&
                 mqbs::QueueOpType::e_ADDITION != queueOpType)) {
                continue;  // CONTINUE
            }

            bsls::Types::Uint64 queueUriRecOffset = 0;
            unsigned int        queueRecLength    = 0;
            unsigned int        queueRecHeaderLen = 0;
            unsigned int        paddedUriLen      = 0;
            unsigned int        numAppIds         = 0;
            BSLS_ASSERT_SAFE(0 != rec.queueUriRecordOffsetWords());

            queueUriRecOffset = static_cast<bsls::Types::Uint64>(
                                    rec.queueUriRecordOffsetWords()) *
                                bmqp::Protocol::k_WORD_SIZE;

            if (queueUriRecOffset > recoveryCtx.d_mappedQlistFd.fileSize()) {
                errorDescription
                    << d_clusterData.identity().description() << " Partition ["
                    << partitionId
                    << "]: " << "Encountered a QueueOp record of type ["
                    << queueOpType << "], with QLIST file offset field ["
                    << queueUriRecOffset << "] greater than QLIST file size ["
                    << recoveryCtx.d_mappedQlistFd.fileSize()
                    << "] during backward journal iteration. Record "
                    << "offset: " << jit.recordOffset()
                    << ", record index: " << jit.recordIndex()
                    << ", record sequence number: ("
                    << recHeader.primaryLeaseId() << ", "
                    << recHeader.sequenceNumber() << ")";

                return rc_INVALID_QLIST_OFFSET;  // RETURN
            }

            // 'queueUriRecOffset' == offset of mqbs::QueueRecordHeader

            mqbs::OffsetPtr<const mqbs::QueueRecordHeader> queueRecHeader(
                recoveryCtx.d_mappedQlistFd.block(),
                queueUriRecOffset);

            if (0 == queueRecHeader->headerWords()) {
                errorDescription
                    << d_clusterData.identity().description() << " Partition ["
                    << partitionId << "]: " << "For QueueOp record of type ["
                    << queueOpType
                    << "], the record present in QLIST file has "
                    << "invalid 'headerWords' field in the header. "
                    << "Journal record offset: " << jit.recordOffset()
                    << ", journal record index: " << jit.recordIndex()
                    << ". QLIST record offset: " << queueUriRecOffset
                    << ", record sequence number: ("
                    << recHeader.primaryLeaseId() << ", "
                    << recHeader.sequenceNumber() << ")";

                return rc_INVALID_QLIST_RECORD;  // RETURN
            }

            queueRecHeaderLen = queueRecHeader->headerWords() *
                                bmqp::Protocol::k_WORD_SIZE;

            if (recoveryCtx.d_mappedQlistFd.fileSize() <
                (queueUriRecOffset + queueRecHeaderLen)) {
                errorDescription
                    << d_clusterData.identity().description() << " Partition ["
                    << partitionId << "]: " << "For QueueOp record of type ["
                    << queueOpType
                    << "], the record present in QLIST file has "
                    << "invalid header size [" << queueRecHeaderLen
                    << "]. Journal record offset: " << jit.recordOffset()
                    << ", journal record index: " << jit.recordIndex()
                    << ". QLIST record offset: " << queueUriRecOffset
                    << ". QLIST file size: "
                    << recoveryCtx.d_mappedQlistFd.fileSize()
                    << ", record sequence number: ("
                    << recHeader.primaryLeaseId() << ", "
                    << recHeader.sequenceNumber() << ")";
                return rc_INVALID_QLIST_RECORD;  // RETURN
            }

            if (0 == queueRecHeader->queueUriLengthWords()) {
                errorDescription
                    << d_clusterData.identity().description() << " Partition ["
                    << partitionId << "]: " << "For QueueOp record of type ["
                    << queueOpType
                    << "], the record present in QLIST file has "
                    << "invalid 'queueUriLengthWords' field in the "
                    << "header. Journal record offset: " << jit.recordOffset()
                    << ", journal record index: " << jit.recordIndex()
                    << ". QLIST record offset: " << queueUriRecOffset
                    << ", record sequence number: ("
                    << recHeader.primaryLeaseId() << ", "
                    << recHeader.sequenceNumber() << ")";
                return rc_INVALID_QLIST_RECORD;  // RETURN
            }

            paddedUriLen = queueRecHeader->queueUriLengthWords() *
                           bmqp::Protocol::k_WORD_SIZE;

            if (recoveryCtx.d_mappedQlistFd.fileSize() <
                (queueUriRecOffset + paddedUriLen)) {
                errorDescription
                    << d_clusterData.identity().description() << " Partition ["
                    << partitionId << "]: " << "For QueueOp record of type ["
                    << queueOpType
                    << "], the record present in QLIST file has "
                    << "invalid 'queueUriLengthWords' field in the "
                    << "header. Journal record offset: " << jit.recordOffset()
                    << ", journal record index: " << jit.recordIndex()
                    << ". QLIST record offset: " << queueUriRecOffset
                    << ". QLIST file size: "
                    << recoveryCtx.d_mappedQlistFd.fileSize()
                    << ", record sequence number: ("
                    << recHeader.primaryLeaseId() << ", "
                    << recHeader.sequenceNumber() << ")";
                return rc_INVALID_QLIST_RECORD;  // RETURN
            }

            queueRecLength = queueRecHeader->queueRecordWords() *
                             bmqp::Protocol::k_WORD_SIZE;

            if (0 == queueRecLength) {
                errorDescription
                    << d_clusterData.identity().description() << " Partition ["
                    << partitionId << "]: " << "For QueueOp record of type ["
                    << queueOpType
                    << "], the record present in QLIST file has "
                    << "invalid 'queueRecordWords' field in the "
                    << "header. Journal record offset: " << jit.recordOffset()
                    << ", journal record index: " << jit.recordIndex()
                    << ". QLIST record offset: " << queueUriRecOffset
                    << ", record sequence number: ("
                    << recHeader.primaryLeaseId() << ", "
                    << recHeader.sequenceNumber() << ")";
                return rc_INVALID_QLIST_RECORD;  // RETURN
            }

            if (recoveryCtx.d_mappedQlistFd.fileSize() <
                (queueUriRecOffset + queueRecLength)) {
                errorDescription
                    << d_clusterData.identity().description() << " Partition ["
                    << partitionId << "]: " << "For QueueOp record of type ["
                    << queueOpType
                    << "], the record present in QLIST file has "
                    << "invalid 'queueRecLength' field in the header. "
                    << "Journal record offset: " << jit.recordOffset()
                    << ", journal record index: " << jit.recordIndex()
                    << ". QLIST record offset: " << queueUriRecOffset
                    << ". QLIST file size: "
                    << recoveryCtx.d_mappedQlistFd.fileSize()
                    << ", record sequence number: ("
                    << recHeader.primaryLeaseId() << ", "
                    << recHeader.sequenceNumber() << ")";
                return rc_INVALID_QLIST_RECORD;  // RETURN
            }

            numAppIds = queueRecHeader->numAppIds();

            // Only QueueOp.CREATION & QueueOp.ADDITION records have
            // corresponding record in the QLIST file.

            if (isLastQlistRecord) {
                isLastQlistRecord               = false;
                recoveryCtx.d_qlistFilePosition = queueUriRecOffset +
                                                  queueRecLength;
            }
        }

        if (!isLastJournalRecord && !isLastMessageRecord &&
            !isLastQlistRecord) {
            break;  // BREAK
        }
    }

    BALL_LOG_INFO_BLOCK
    {
        BALL_LOG_OUTPUT_STREAM
            << d_clusterData.identity().description() << " Partition ["
            << partitionId << "]: " << "Opened recovery file set: "
            << recoveryCtx.d_recoveryFileSet
            << ", journal file position: " << recoveryCtx.d_journalFilePosition
            << ", data file position: " << recoveryCtx.d_dataFilePosition;
        if (d_qListAware) {
            BALL_LOG_OUTPUT_STREAM << ", QList file position: "
                                   << recoveryCtx.d_qlistFilePosition;
        }
        BALL_LOG_OUTPUT_STREAM << ".";
    }

    return rc_SUCCESS;
}

int RecoveryManager::closeRecoveryFileSet(int partitionId)
{
    // executed by the *QUEUE DISPATCHER* thread associated with 'partitionId'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(partitionId >= 0 &&
                     partitionId <
                         d_clusterConfig.partitionConfig().numPartitions());

    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS                  = 0,
        rc_JOURNAL_FD_CLOSE_FAILURE = -1,
        rc_DATA_FD_CLOSE_FAILURE    = -2,
        rc_QLIST_FD_CLOSE_FAILURE   = -3
    };

    RecoveryContext& recoveryCtx = d_recoveryContextVec[partitionId];

    int                rc = rc_SUCCESS;
    bmqu::MemOutStream errorDesc;
    if (recoveryCtx.d_mappedJournalFd.isValid()) {
        rc = mqbs::FileSystemUtil::truncate(&recoveryCtx.d_mappedJournalFd,
                                            recoveryCtx.d_journalFilePosition,
                                            errorDesc);
        if (rc != 0) {
            BMQTSK_ALARMLOG_ALARM("FILE_IO")
                << d_clusterData.identity().description() << " Partition ["
                << partitionId << "]: " << "Failed to truncate journal file ["
                << recoveryCtx.d_recoveryFileSet.journalFile()
                << "], rc: " << rc << ", error: " << errorDesc.str()
                << BMQTSK_ALARMLOG_END;
            errorDesc.reset();
        }

        rc = mqbs::FileSystemUtil::flush(
            recoveryCtx.d_mappedJournalFd.mapping(),
            recoveryCtx.d_journalFilePosition,
            errorDesc);
        if (rc != 0) {
            BMQTSK_ALARMLOG_ALARM("FILE_IO")
                << d_clusterData.identity().description() << " Partition ["
                << partitionId << "]: " << "Failed to flush journal file ["
                << recoveryCtx.d_recoveryFileSet.journalFile()
                << "], rc: " << rc << ", error: " << errorDesc.str()
                << BMQTSK_ALARMLOG_END;
            errorDesc.reset();
        }

        rc = mqbs::FileSystemUtil::close(&recoveryCtx.d_mappedJournalFd);
        if (rc != 0) {
            BMQTSK_ALARMLOG_ALARM("FILE_IO")
                << d_clusterData.identity().description() << " Partition ["
                << partitionId << "]: " << "Failed to close journal file ["
                << recoveryCtx.d_recoveryFileSet.journalFile()
                << "], rc: " << rc << BMQTSK_ALARMLOG_END;
            return rc * 10 + rc_JOURNAL_FD_CLOSE_FAILURE;  // RETURN
        }

        BALL_LOG_INFO << d_clusterData.identity().description()
                      << " Partition [" << partitionId
                      << "]: " << "Closed journal file in recovery file set; "
                      << "journal file position was "
                      << recoveryCtx.d_journalFilePosition;
    }
    recoveryCtx.d_journalFilePosition = 0;

    if (recoveryCtx.d_mappedDataFd.isValid()) {
        rc = mqbs::FileSystemUtil::truncate(&recoveryCtx.d_mappedDataFd,
                                            recoveryCtx.d_dataFilePosition,
                                            errorDesc);
        if (rc != 0) {
            BMQTSK_ALARMLOG_ALARM("FILE_IO")
                << d_clusterData.identity().description() << " Partition ["
                << partitionId << "]: " << "Failed to truncate data file ["
                << recoveryCtx.d_recoveryFileSet.dataFile() << "], rc: " << rc
                << ", error: " << errorDesc.str() << BMQTSK_ALARMLOG_END;
            errorDesc.reset();
        }

        rc = mqbs::FileSystemUtil::flush(recoveryCtx.d_mappedDataFd.mapping(),
                                         recoveryCtx.d_dataFilePosition,
                                         errorDesc);
        if (rc != 0) {
            BMQTSK_ALARMLOG_ALARM("FILE_IO")
                << d_clusterData.identity().description() << " Partition ["
                << partitionId << "]: " << "Failed to flush data file ["
                << recoveryCtx.d_recoveryFileSet.dataFile() << "], rc: " << rc
                << ", error: " << errorDesc.str() << BMQTSK_ALARMLOG_END;
            errorDesc.reset();
        }

        rc = mqbs::FileSystemUtil::close(&recoveryCtx.d_mappedDataFd);
        if (rc != 0) {
            BMQTSK_ALARMLOG_ALARM("FILE_IO")
                << d_clusterData.identity().description() << " Partition ["
                << partitionId << "]: " << "Failed to close data file ["
                << recoveryCtx.d_recoveryFileSet.dataFile() << "], rc: " << rc
                << BMQTSK_ALARMLOG_END;
            return rc * 10 + rc_DATA_FD_CLOSE_FAILURE;  // RETURN
        }

        BALL_LOG_INFO << d_clusterData.identity().description()
                      << " Partition [" << partitionId
                      << "]: " << "Closed data file in recovery file set; "
                      << "data file position was "
                      << recoveryCtx.d_dataFilePosition;
    }
    recoveryCtx.d_dataFilePosition = 0;

    if (recoveryCtx.d_mappedQlistFd.isValid()) {
        rc = mqbs::FileSystemUtil::truncate(&recoveryCtx.d_mappedQlistFd,
                                            recoveryCtx.d_qlistFilePosition,
                                            errorDesc);
        if (rc != 0) {
            BMQTSK_ALARMLOG_ALARM("FILE_IO")
                << d_clusterData.identity().description() << " Partition ["
                << partitionId << "]: " << "Failed to truncate QList file ["
                << recoveryCtx.d_recoveryFileSet.qlistFile() << "], rc: " << rc
                << ", error: " << errorDesc.str() << BMQTSK_ALARMLOG_END;
            errorDesc.reset();
        }

        rc = mqbs::FileSystemUtil::flush(recoveryCtx.d_mappedQlistFd.mapping(),
                                         recoveryCtx.d_qlistFilePosition,
                                         errorDesc);
        if (rc != 0) {
            BMQTSK_ALARMLOG_ALARM("FILE_IO")
                << d_clusterData.identity().description() << " Partition ["
                << partitionId << "]: " << "Failed to flush QList file ["
                << recoveryCtx.d_recoveryFileSet.qlistFile() << "], rc: " << rc
                << ", error: " << errorDesc.str() << BMQTSK_ALARMLOG_END;
            errorDesc.reset();
        }

        rc = mqbs::FileSystemUtil::close(&recoveryCtx.d_mappedQlistFd);
        if (rc != 0) {
            BMQTSK_ALARMLOG_ALARM("FILE_IO")
                << d_clusterData.identity().description() << " Partition ["
                << partitionId << "]: " << "Failed to close QList file ["
                << recoveryCtx.d_recoveryFileSet.qlistFile() << "], rc: " << rc
                << BMQTSK_ALARMLOG_END;
            return rc * 10 + rc_QLIST_FD_CLOSE_FAILURE;  // RETURN
        }

        BALL_LOG_INFO << d_clusterData.identity().description()
                      << " Partition [" << partitionId
                      << "]: " << "Closed QList file in recovery file set; "
                      << "QList file position was "
                      << recoveryCtx.d_qlistFilePosition;
    }
    recoveryCtx.d_qlistFilePosition = 0;

    return rc_SUCCESS;
}

int RecoveryManager::recoverSeqNum(
    bmqp_ctrlmsg::PartitionSequenceNumber* seqNum,
    int                                    partitionId)
{
    // executed by the *QUEUE DISPATCHER* thread associated with 'partitionId'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(seqNum);
    BSLS_ASSERT_SAFE(partitionId >= 0 &&
                     partitionId <
                         d_clusterConfig.partitionConfig().numPartitions());
    enum {
        rc_SUCCESS               = 0,
        rc_UNKNOWN               = -1,
        rc_OPEN_FILE_SET_FAILURE = -2,
        rc_INVALID_FILE_SET      = -3,
        rc_FILE_ITERATOR_FAILURE = -4
    };

    bmqu::MemOutStream errorDesc;
    RecoveryContext&   recoveryCtx = d_recoveryContextVec[partitionId];
    int                rc          = rc_UNKNOWN;

    BSLS_ASSERT_SAFE(recoveryCtx.d_mappedJournalFd.isValid());

    mqbs::JournalFileIterator jit;
    rc = mqbs::FileStoreUtil::loadIterators(errorDesc,
                                            recoveryCtx.d_recoveryFileSet,
                                            &jit,
                                            recoveryCtx.d_mappedJournalFd);
    if (rc != 0) {
        BALL_LOG_ERROR << d_clusterData.identity().description()
                       << " Partition [" << partitionId << "]: "
                       << "Error while iterating recovered files, rc: " << rc
                       << ", description: " << errorDesc.str();
        return 10 * rc + rc_FILE_ITERATOR_FAILURE;  // RETURN
    }

    if (jit.hasRecordSizeRemaining()) {
        const mqbs::RecordHeader& lastRecordHeader = jit.lastRecordHeader();

        BALL_LOG_INFO << d_clusterData.identity().description()
                      << " Partition [" << partitionId
                      << "]: " << "Recovered Sequence Number "
                      << lastRecordHeader.partitionSequenceNumber()
                      << " from journal file ["
                      << recoveryCtx.d_recoveryFileSet.journalFile() << "].";

        *seqNum = lastRecordHeader.partitionSequenceNumber();
    }
    else {
        BALL_LOG_INFO << d_clusterData.identity().description()
                      << " Partition [" << partitionId << "]: "
                      << "Journal file has no record. Storing (0, 0) as self "
                      << "sequence number.";

        seqNum->reset();
    }

    return rc_SUCCESS;
}

void RecoveryManager::setLiveDataSource(mqbnet::ClusterNode* source,
                                        int                  partitionId)
{
    // executed by the *QUEUE DISPATCHER* thread associated with 'partitionId'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(source);
    BSLS_ASSERT_SAFE(0 <= partitionId);
    BSLS_ASSERT_SAFE(partitionId <
                     static_cast<int>(d_recoveryContextVec.size()));

    RecoveryContext& recoveryCtx = d_recoveryContextVec[partitionId];

    BALL_LOG_INFO << d_clusterData.identity().description() << " Partition ["
                  << partitionId << "]: " << "Setting live data source from "
                  << (recoveryCtx.d_liveDataSource_p
                          ? recoveryCtx.d_liveDataSource_p->nodeDescription()
                          : "** NULL **")
                  << " to " << source->nodeDescription() << ", while clearing "
                  << recoveryCtx.d_bufferedEvents.size()
                  << " buffered storage events.";

    recoveryCtx.d_liveDataSource_p = source;
    recoveryCtx.d_bufferedEvents.clear();
}

void RecoveryManager::bufferStorageEvent(
    int                                 partitionId,
    const bsl::shared_ptr<bdlbb::Blob>& blob,
    mqbnet::ClusterNode*                source)
{
    // executed by the *QUEUE DISPATCHER* thread associated with 'partitionId'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(0 <= partitionId);
    BSLS_ASSERT_SAFE(source);

    RecoveryContext& recoveryCtx = d_recoveryContextVec[partitionId];
    BSLS_ASSERT_SAFE(recoveryCtx.d_liveDataSource_p);
    if (recoveryCtx.d_liveDataSource_p->nodeId() != source->nodeId()) {
        BALL_LOG_ERROR << d_clusterData.identity().description()
                       << " Partition [" << partitionId
                       << "]: " << "Storage event from node "
                       << source->nodeDescription() << "cannot be buffered, "
                       << "because it is different from the expected live "
                       << "data source node "
                       << recoveryCtx.d_liveDataSource_p->nodeDescription()
                       << ".";

        return;  // RETURN
    }

    recoveryCtx.d_bufferedEvents.push_back(blob);

    BALL_LOG_INFO << d_clusterData.identity().description() << " Partition ["
                  << partitionId
                  << "]: " << "Buffered a storage event from primary node "
                  << source->nodeDescription()
                  << " as self is still healing the partition.";
}

int RecoveryManager::loadBufferedStorageEvents(
    bsl::vector<bsl::shared_ptr<bdlbb::Blob> >* out,
    const mqbnet::ClusterNode*                  source,
    int                                         partitionId)
{
    // executed by the *QUEUE DISPATCHER* thread associated with 'partitionId'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(out);
    BSLS_ASSERT_SAFE(source);
    BSLS_ASSERT_SAFE(0 <= partitionId);

    enum { rc_SUCCESS = 0, rc_UNEXPECTED_SOURCE = -1 };

    RecoveryContext& recoveryCtx = d_recoveryContextVec[partitionId];
    if (recoveryCtx.d_bufferedEvents.empty()) {
        // We did not buffer any storage event.
        return rc_SUCCESS;  // RETURN
    }
    BSLS_ASSERT_SAFE(recoveryCtx.d_liveDataSource_p);

    if (recoveryCtx.d_liveDataSource_p->nodeId() != source->nodeId()) {
        BALL_LOG_ERROR << d_clusterData.identity().description()
                       << " Partition [" << partitionId << "]: "
                       << "Cannot load buffered storage events from node "
                       << source->nodeDescription()
                       << " because it is different from the expected live "
                       << "data source node "
                       << recoveryCtx.d_liveDataSource_p->nodeDescription()
                       << ".";

        return rc_UNEXPECTED_SOURCE;  // RETURN
    }

    *out = recoveryCtx.d_bufferedEvents;

    recoveryCtx.d_liveDataSource_p = 0;
    recoveryCtx.d_bufferedEvents.clear();

    return rc_SUCCESS;
}

// ACCESSORS
void RecoveryManager::loadReplicaDataResponsePush(
    bmqp_ctrlmsg::ControlMessage* out,
    int                           partitionId) const
{
    // executed by the *QUEUE DISPATCHER* thread associated with 'partitionId'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(out);
    BSLS_ASSERT_SAFE(partitionId >= 0 &&
                     partitionId <
                         d_clusterConfig.partitionConfig().numPartitions());

    const ReceiveDataContext& receiveDataCtx =
        d_recoveryContextVec[partitionId].d_receiveDataContext;

    out->rId() = receiveDataCtx.d_recoveryRequestId;

    bmqp_ctrlmsg::ReplicaDataResponse& response =
        out->choice()
            .makeClusterMessage()
            .choice()
            .makePartitionMessage()
            .choice()
            .makeReplicaDataResponse();

    response.replicaDataType()     = bmqp_ctrlmsg::ReplicaDataType::E_PUSH;
    response.partitionId()         = partitionId;
    response.beginSequenceNumber() = receiveDataCtx.d_beginSeqNum;
    response.endSequenceNumber()   = receiveDataCtx.d_endSeqNum;
}

}  // close package namespace
}  // close enterprise namespace
