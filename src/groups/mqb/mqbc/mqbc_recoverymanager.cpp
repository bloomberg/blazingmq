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
#include <mqbc_recoverymanager.h>

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

// MWC
#include <mwctsk_alarmlog.h>
#include <mwcu_blob.h>
#include <mwcu_blobobjectproxy.h>
#include <mwcu_memoutstream.h>

// BDE
#include <bdlb_scopeexit.h>
#include <bdlf_bind.h>
#include <bdls_filesystemutil.h>
#include <bsl_algorithm.h>

namespace BloombergLP {
namespace mqbc {

namespace {

const unsigned int k_REQUESTED_JOURNAL_SPACE =
    3 * mqbs::FileStoreProtocol::k_JOURNAL_RECORD_SIZE;
// Above, 3 == 1 journal record being written +
//             1 journal sync point if rolling over +
//             1 journal sync point if self needs to issue another sync point
//             in 'setPrimary' with old values

}  // close unnamed namespace

void RecoveryManager::ChunkDeleter::operator()(
    BSLS_ANNOTATION_UNUSED const void* ptr) const
{
    // executed by *ANY* thread
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_mfd_sp.get());

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
    d_liveDataSource_p     = 0;
    d_recoveryDataSource_p = 0;
    d_expectChunks         = false;
    d_recoveryRequestId    = -1;
    d_beginSeqNum.reset();
    d_endSeqNum.reset();
    d_currSeqNum.reset();
    mqbs::FileSystemUtil::close(&d_mappedJournalFd);
    d_journalFilePosition = 0;
    mqbs::FileSystemUtil::close(&d_mappedDataFd);
    d_dataFilePosition = 0;
    d_recoveryFileSet.reset();
    d_bufferedEvents.clear();
}

// ---------------------
// class RecoveryManager
// ---------------------

// CREATORS
RecoveryManager::RecoveryManager(
    const mqbcfg::ClusterDefinition& clusterConfig,
    mqbc::ClusterData*               clusterData,
    const mqbs::DataStoreConfig&     dataStoreConfig,
    bslma::Allocator*                allocator)
: d_allocator_p(allocator)
, d_clusterConfig(clusterConfig)
, d_dataStoreConfig(dataStoreConfig)
, d_clusterData_p(clusterData)
, d_receiveDataContextVec(allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(clusterData);
    BSLS_ASSERT_SAFE(allocator);

    d_receiveDataContextVec.resize(
        clusterConfig.partitionConfig().numPartitions());
}

RecoveryManager::~RecoveryManager()
{
    // NOTHING
}

// MANIPULATORS
int RecoveryManager::start(
    BSLS_ANNOTATION_UNUSED bsl::ostream& errorDescription)
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
    // executed by the *STORAGE (QUEUE) DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(partitionId >= 0 &&
                     partitionId <
                         d_clusterConfig.partitionConfig().numPartitions());

    ReceiveDataContext& receiveDataCtx = d_receiveDataContextVec[partitionId];
    mwcu::MemOutStream  errorDesc;
    int                 rc = -1;
    if (receiveDataCtx.d_mappedJournalFd.isValid()) {
        rc = mqbs::FileSystemUtil::truncate(
            &receiveDataCtx.d_mappedJournalFd,
            receiveDataCtx.d_journalFilePosition,
            errorDesc);
        if (rc != 0) {
            MWCTSK_ALARMLOG_ALARM("FILE_IO")
                << d_clusterData_p->identity().description()
                << " PartitionId [" << partitionId << "]: "
                << "Failed to truncate journal file ["
                << receiveDataCtx.d_recoveryFileSet.journalFile()
                << "], rc: " << rc << ", error: " << errorDesc.str()
                << MWCTSK_ALARMLOG_END;
        }

        rc = mqbs::FileSystemUtil::close(&receiveDataCtx.d_mappedJournalFd);
        if (rc != 0) {
            MWCTSK_ALARMLOG_ALARM("FILE_IO")
                << d_clusterData_p->identity().description()
                << " PartitionId [" << partitionId << "]: "
                << "Failed to close journal file ["
                << receiveDataCtx.d_recoveryFileSet.journalFile()
                << "], rc: " << rc << MWCTSK_ALARMLOG_END;
        }
    }
    rc = mqbs::FileSystemUtil::move(
        receiveDataCtx.d_recoveryFileSet.journalFile(),
        d_dataStoreConfig.archiveLocation());
    if (0 != rc) {
        MWCTSK_ALARMLOG_ALARM("FILE_IO")
            << d_clusterData_p->identity().description() << " PartitionId ["
            << partitionId << "]: "
            << "Failed to move file ["
            << receiveDataCtx.d_recoveryFileSet.journalFile() << "] "
            << "to location [" << d_dataStoreConfig.archiveLocation()
            << "] rc: " << rc << MWCTSK_ALARMLOG_END;
    }

    if (receiveDataCtx.d_mappedDataFd.isValid()) {
        rc = mqbs::FileSystemUtil::truncate(&receiveDataCtx.d_mappedDataFd,
                                            receiveDataCtx.d_dataFilePosition,
                                            errorDesc);
        if (rc != 0) {
            MWCTSK_ALARMLOG_ALARM("FILE_IO")
                << d_clusterData_p->identity().description()
                << " PartitionId [" << partitionId << "]: "
                << "Failed to truncate data file ["
                << receiveDataCtx.d_recoveryFileSet.dataFile()
                << "], rc: " << rc << ", error: " << errorDesc.str()
                << MWCTSK_ALARMLOG_END;
        }

        rc = mqbs::FileSystemUtil::close(&receiveDataCtx.d_mappedDataFd);
        if (rc != 0) {
            MWCTSK_ALARMLOG_ALARM("FILE_IO")
                << d_clusterData_p->identity().description()
                << " PartitionId [" << partitionId << "]: "
                << "Failed to close data file ["
                << receiveDataCtx.d_recoveryFileSet.dataFile()
                << "], rc: " << rc << MWCTSK_ALARMLOG_END;
        }
    }
    rc = mqbs::FileSystemUtil::move(
        receiveDataCtx.d_recoveryFileSet.dataFile(),
        d_dataStoreConfig.archiveLocation());
    if (0 != rc) {
        MWCTSK_ALARMLOG_ALARM("FILE_IO")
            << d_clusterData_p->identity().description() << " PartitionId ["
            << partitionId << "]: "
            << "Failed to move file ["
            << receiveDataCtx.d_recoveryFileSet.dataFile() << "] "
            << "to location [" << d_dataStoreConfig.archiveLocation()
            << "] rc: " << rc << MWCTSK_ALARMLOG_END;
    }
}

int RecoveryManager::setExpectedDataChunkRange(
    int                                          partitionId,
    mqbnet::ClusterNode*                         source,
    const bmqp_ctrlmsg::PartitionSequenceNumber& beginSeqNum,
    const bmqp_ctrlmsg::PartitionSequenceNumber& endSeqNum,
    int                                          requestId)
{
    // executed by the *STORAGE (QUEUE) DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(partitionId >= 0 &&
                     partitionId <
                         d_clusterConfig.partitionConfig().numPartitions());
    BSLS_ASSERT_SAFE(source);
    BSLS_ASSERT_SAFE(beginSeqNum < endSeqNum);

    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS               = 0,
        rc_OPEN_FILE_SET_FAILURE = -1,
        rc_INVALID_FILE_SET      = -2
    };

    ReceiveDataContext& receiveDataCtx = d_receiveDataContextVec[partitionId];

    if (receiveDataCtx.d_expectChunks) {
        BALL_LOG_ERROR << d_clusterData_p->identity().description()
                       << " PartitionId [" << partitionId << "]: "
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

    BALL_LOG_INFO_BLOCK
    {
        BALL_LOG_OUTPUT_STREAM << d_clusterData_p->identity().description()
                               << " PartitionId [" << partitionId << "]: "
                               << "Got notification to expect data chunks "
                               << "of range " << beginSeqNum << " to "
                               << endSeqNum << " from "
                               << source->nodeDescription() << ".";
        if (requestId != -1) {
            BALL_LOG_OUTPUT_STREAM << " Recovery requestId is " << requestId
                                   << ".";
        }
    }

    if (receiveDataCtx.d_mappedJournalFd.isValid() &&
        receiveDataCtx.d_mappedDataFd.isValid()) {
        return rc_SUCCESS;  // RETURN
    }

    // Open journal and data files, if not already open.
    mwcu::MemOutStream errorDesc;
    int                rc = mqbs::FileStoreUtil::openFileSetWriteMode(
        errorDesc,
        receiveDataCtx.d_recoveryFileSet,
        d_dataStoreConfig.hasPreallocate(),
        false,  // deleteOnFailure
        receiveDataCtx.d_mappedJournalFd.isValid()
                           ? 0
                           : &receiveDataCtx.d_mappedJournalFd,
        receiveDataCtx.d_mappedDataFd.isValid()
                           ? 0
                           : &receiveDataCtx.d_mappedDataFd,
        0,  // qlistFd
        d_dataStoreConfig.hasPrefaultPages());
    if (rc != 0) {
        BALL_LOG_ERROR << d_clusterData_p->identity().description()
                       << " PartitionId [" << partitionId
                       << "]: file set: " << receiveDataCtx.d_recoveryFileSet
                       << " failed to open. Reason: " << errorDesc.str()
                       << ", rc: " << rc;
        return rc_OPEN_FILE_SET_FAILURE;  // RETURN
    }

    rc = mqbs::FileStoreUtil::validateFileSet(receiveDataCtx.d_mappedJournalFd,
                                              receiveDataCtx.d_mappedDataFd,
                                              mqbs::MappedFileDescriptor());

    if (rc != 0) {
        BALL_LOG_ERROR << d_clusterData_p->identity().description()
                       << "PartitionId [" << partitionId
                       << "]: file set: " << receiveDataCtx.d_recoveryFileSet
                       << " validation failed, rc: " << rc;
        mqbs::FileSystemUtil::close(&receiveDataCtx.d_mappedJournalFd);
        mqbs::FileSystemUtil::close(&receiveDataCtx.d_mappedDataFd);

        return rc_INVALID_FILE_SET;  // RETURN
    }

    return rc_SUCCESS;
}

void RecoveryManager::resetReceiveDataCtx(int partitionId)
{
    // executed by the *STORAGE (QUEUE) DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(partitionId >= 0 &&
                     partitionId <
                         d_clusterConfig.partitionConfig().numPartitions());

    d_receiveDataContextVec[partitionId].reset();
}

int RecoveryManager::processSendDataChunks(
    int                                          partitionId,
    mqbnet::ClusterNode*                         destination,
    const bmqp_ctrlmsg::PartitionSequenceNumber& beginSeqNum,
    const bmqp_ctrlmsg::PartitionSequenceNumber& endSeqNum,
    const mqbs::FileStore&                       fs,
    PartitionDoneSendDataChunksCb                doneDataChunksCb)
{
    // executed by the *STORAGE (QUEUE) DISPATCHER* thread

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

    rc = RecoveryUtil::loadFileDescriptors(mappedJournalFd.get(),
                                           mappedDataFd.get(),
                                           fileSet);

    if (rc != 0) {
        return rc * 10 + rc_LOAD_FD_FAILURE;  // RETURN
    }

    bsl::shared_ptr<bsls::AtomicInt> journalChunkDeleterCounter =
        bsl::make_shared<bsls::AtomicInt>(0);
    bsl::shared_ptr<bsls::AtomicInt> dataChunkDeleterCounter =
        bsl::make_shared<bsls::AtomicInt>(0);

    // Scope Exit Guards to unmap the fds incase of errors.
    bdlb::ScopeExit<ChunkDeleter> guardJournalFd(
        ChunkDeleter(mappedJournalFd, journalChunkDeleterCounter));
    bdlb::ScopeExit<ChunkDeleter> guardDataFd(
        ChunkDeleter(mappedDataFd, dataChunkDeleterCounter));

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
                                      d_clusterData_p->bufferFactory(),
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

        rc = RecoveryUtil::incrementCurrentSeqNum(&currentSeqNum,
                                                  &journalRecordBase,
                                                  *mappedJournalFd,
                                                  endSeqNum,
                                                  partitionId,
                                                  *destination,
                                                  *d_clusterData_p,
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
                                           true);  // fsmWorkflow

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
            BSLS_ASSERT_SAFE(bmqp::StorageMessageType::e_DATA ==
                             storageMsgType);

            bsl::shared_ptr<char> payloadRecordSp(
                payloadRecordBase,
                ChunkDeleter(mappedDataFd, dataChunkDeleterCounter));

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
        BALL_LOG_WARN << d_clusterData_p->identity().description()
                      << " PartitionId [" << partitionId
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

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << " Partition [" << partitionId << "]: sent data chunks "
                  << "from " << beginSeqNum << " to " << endSeqNum
                  << " to node: " << destination->nodeDescription() << ".";

    return rc_SUCCESS;
}

int RecoveryManager::processReceiveDataChunks(
    const bsl::shared_ptr<bdlbb::Blob>& blob,
    mqbnet::ClusterNode*                source,
    mqbs::FileStore*                    fs,
    int                                 partitionId)
{
    // executed by the *STORAGE (QUEUE) DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(source);
    BSLS_ASSERT_SAFE(fs);
    BSLS_ASSERT_SAFE(0 <= partitionId);

    enum RcEnum {
        // Value for the various RC error categories
        rc_LAST_DATA_CHUNK        = 1,
        rc_SUCCESS                = 0,
        rc_UNEXPECTED_DATA        = -1,
        rc_INVALID_RECOVERY_PEER  = -2,
        rc_INVALID_STORAGE_HDR    = -3,
        rc_INVALID_RECORD_SEQ_NUM = -4,
        rc_JOURNAL_OUT_OF_SYNC    = -5,
        rc_MISSING_PAYLOAD        = -6,
        rc_MISSING_PAYLOAD_HDR    = -7,
        rc_INCOMPLETE_PAYLOAD     = -8,
        rc_DATA_OFFSET_MISMATCH   = -9,
        rc_INVALID_QUEUE_RECORD   = -10
    };

    ReceiveDataContext& receiveDataCtx = d_receiveDataContextVec[partitionId];
    if (!receiveDataCtx.d_expectChunks) {
        MWCTSK_ALARMLOG_ALARM("RECOVERY")
            << d_clusterData_p->identity().description()
            << ": For PartitionId [" << partitionId
            << "], received partition-sync event from node "
            << source->nodeDescription()
            << ", but self is not expecting data chunks."
            << "Ignoring this event." << MWCTSK_ALARMLOG_END;
        return rc_UNEXPECTED_DATA;  // RETURN
    }

    BSLS_ASSERT_SAFE(receiveDataCtx.d_recoveryDataSource_p);
    if (receiveDataCtx.d_recoveryDataSource_p != source) {
        MWCTSK_ALARMLOG_ALARM("RECOVERY")
            << d_clusterData_p->identity().description()
            << ": For PartitionId [" << partitionId
            << "], received partition-sync event from node "
            << source->nodeDescription()
            << ", which is not identified as recovery peer node "
            << receiveDataCtx.d_recoveryDataSource_p->nodeDescription()
            << ". Ignoring this event." << MWCTSK_ALARMLOG_END;
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
            MWCTSK_ALARMLOG_ALARM("REPLICATION")
                << d_clusterData_p->identity().description()
                << " PartitionId [" << partitionId << "]: "
                << "The last partition sync msg inside a storage event "
                << "processed by FileStore has sequenceNumber "
                << receiveDataCtx.d_currSeqNum
                << ", larger than self's expected ending sequence number of "
                << "data chunks: " << receiveDataCtx.d_endSeqNum << "."
                << MWCTSK_ALARMLOG_END;
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
        mwcu::BlobPosition                        recordPosition;
        mwcu::BlobObjectProxy<mqbs::RecordHeader> recHeader;

        mwcu::MemOutStream partitionDesc;
        partitionDesc << d_clusterData_p->identity().description()
                      << " PartitionId [" << partitionId << "]: ";

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
            MWCTSK_ALARMLOG_ALARM("REPLICATION")
                << d_clusterData_p->identity().description()
                << " PartitionId [" << partitionId << "]: "
                << "Received partition sync msg of type "
                << header.messageType() << " with sequenceNumber "
                << recordSeqNum
                << ", smaller than or equal to self current sequence number: "
                << receiveDataCtx.d_endSeqNum
                << ". Record's journal offset (in words): "
                << header.journalOffsetWords() << ". Ignoring entire event."
                << MWCTSK_ALARMLOG_END;
            return rc_INVALID_RECORD_SEQ_NUM;  // RETURN
        }

        if (recordSeqNum > receiveDataCtx.d_endSeqNum) {
            MWCTSK_ALARMLOG_ALARM("REPLICATION")
                << d_clusterData_p->identity().description()
                << " PartitionId [" << partitionId << "]: "
                << "Received partition sync msg of type "
                << header.messageType() << " with sequenceNumber "
                << recordSeqNum
                << ", larger than self's expected ending sequence number of "
                << "data chunks: " << receiveDataCtx.d_endSeqNum
                << ". Record's journal offset (in words): "
                << header.journalOffsetWords() << ". Ignoring entire event."
                << MWCTSK_ALARMLOG_END;
            return rc_INVALID_RECORD_SEQ_NUM;  // RETURN
        }

        // Local refs for convenience.

        mqbs::MappedFileDescriptor& journal = receiveDataCtx.d_mappedJournalFd;
        bsls::Types::Uint64& journalPos = receiveDataCtx.d_journalFilePosition;
        BSLS_ASSERT_SAFE(journal.isValid());

        // Ensure that JOURNAL offset of source and self match.

        const bsls::Types::Uint64 sourceJournalOffset =
            static_cast<bsls::Types::Uint64>(header.journalOffsetWords()) *
            bmqp::Protocol::k_WORD_SIZE;

        if (journalPos != sourceJournalOffset) {
            // Source's and self views of the journal have diverged.

            MWCTSK_ALARMLOG_ALARM("REPLICATION")
                << d_clusterData_p->identity().description()
                << " PartitionId [" << partitionId << "]: "
                << "Received journal record of type [" << header.messageType()
                << "] with journal offset mismatch. "
                << "Source's journal offset: " << sourceJournalOffset
                << ", self journal offset: " << journalPos
                << ", msg sequence number (" << recHeader->primaryLeaseId()
                << ", " << recHeader->sequenceNumber()
                << "). Ignoring this message." << MWCTSK_ALARMLOG_END;
            return rc_JOURNAL_OUT_OF_SYNC;  // RETURN
        }

        if (bmqp::StorageMessageType::e_DATA == header.messageType()) {
            // Extract payload's position from blob, based on 'recordPosition'.
            // Per replication algo, a partition sync message starts with
            // journal record followed by payload.  Payload contains
            // 'mqbs::DataHeader', options (if any), properties and message,
            // and is already DWORD aligned.

            mwcu::BlobPosition payloadBeginPos;
            rc = mwcu::BlobUtil::findOffsetSafe(
                &payloadBeginPos,
                *blob,
                recordPosition,
                mqbs::FileStoreProtocol::k_JOURNAL_RECORD_SIZE);

            if (0 != rc) {
                return 10 * rc + rc_MISSING_PAYLOAD;  // RETURN
            }

            mwcu::BlobObjectProxy<mqbs::DataHeader> dataHeader(
                blob.get(),
                payloadBeginPos,
                -mqbs::DataHeader::k_MIN_HEADER_SIZE,
                true,    // read
                false);  // write
            if (!dataHeader.isSet()) {
                // Couldn't read DataHeader
                return rc_MISSING_PAYLOAD_HDR;  // RETURN
            }

            // Ensure that blob has enough data as indicated by length in
            // 'dataHeader'.
            //
            // TBD: find a cheaper way for this check.

            const int messageSize = dataHeader->messageWords() *
                                    bmqp::Protocol::k_WORD_SIZE;

            mwcu::BlobPosition payloadEndPos;
            rc = mwcu::BlobUtil::findOffsetSafe(&payloadEndPos,
                                                *blob,
                                                payloadBeginPos,
                                                messageSize);
            if (0 != rc) {
                return 10 * rc + rc_INCOMPLETE_PAYLOAD;  // RETURN
            }

            mqbs::MappedFileDescriptor& dataFile =
                receiveDataCtx.d_mappedDataFd;
            bsls::Types::Uint64& dataFilePos =
                receiveDataCtx.d_dataFilePosition;
            bsls::Types::Uint64 dataOffset = dataFilePos;

            BSLS_ASSERT_SAFE(dataFile.isValid());
            BSLS_ASSERT_SAFE(0 == dataOffset % bmqp::Protocol::k_DWORD_SIZE);
            BSLS_ASSERT_SAFE(dataFile.fileSize() >=
                             (dataFilePos + messageSize));

            // Append payload to data file.

            mwcu::BlobUtil::copyToRawBufferFromIndex(dataFile.block().base() +
                                                         dataFilePos,
                                                     *blob,
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
                *blob,
                recordPosition.buffer(),
                recordPosition.byte(),
                mqbs::FileStoreProtocol::k_JOURNAL_RECORD_SIZE);
            journalPos += mqbs::FileStoreProtocol::k_JOURNAL_RECORD_SIZE;

            mqbs::OffsetPtr<const mqbs::MessageRecord> msgRec(journal.block(),
                                                              recordOffset);

            // Check data offset in the replicated journal record sent by the
            // source vs data offset maintained by self.  A mismatch means
            // that replica's and primary's storages are no longer in sync,
            // which indicates a bug in BlazingMQ replication algorithm.
            if (dataOffset != static_cast<bsls::Types::Uint64>(
                                  msgRec->messageOffsetDwords()) *
                                  bmqp::Protocol::k_DWORD_SIZE) {
                return rc_DATA_OFFSET_MISMATCH;  // RETURN
            }
        }
        else {
            // Append record to journal.  Note that here, we only assert on
            // journal file having space for 1 record.  This is because the
            // journal record being written could be a sync point record with
            // subType == SyncPointType::e_ROLLOVER, in which case, journal may
            // not have space for more than 1 record.

            BSLS_ASSERT_SAFE(
                journal.fileSize() >=
                (journalPos +
                 mqbs::FileStoreProtocol ::k_JOURNAL_RECORD_SIZE));

            // Keep track of journal record's offset.

            const bsls::Types::Uint64 recordOffset = journalPos;

            mwcu::BlobUtil::copyToRawBufferFromIndex(
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
                    BALL_LOG_ERROR
                        << d_clusterData_p->identity().description()
                        << " PartitionId [" << partitionId << "]: "
                        << " Unexpected QueueOpType: " << queueRec->type();
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
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(fs);

    bsl::shared_ptr<mqbs::FileSet> fileSetSp;

    mwcu::MemOutStream partitionDesc;
    partitionDesc << "PartitionId [" << partitionId
                  << "] (cluster: " << d_clusterData_p->cluster()->name()
                  << "): ";

    int rc = mqbs::FileStoreUtil::create(errorDescription,
                                         &fileSetSp,
                                         fs,
                                         partitionId,
                                         d_dataStoreConfig,
                                         partitionDesc.str(),
                                         false,  // needQList
                                         d_allocator_p);
    if (rc != 0) {
        return rc;  // RETURN
    }

    ReceiveDataContext& receiveDataCtx = d_receiveDataContextVec[partitionId];
    mqbs::FileStoreUtil::loadCurrentFiles(&receiveDataCtx.d_recoveryFileSet,
                                          *fileSetSp,
                                          false);  // needQList

    receiveDataCtx.d_mappedJournalFd     = fileSetSp->d_journalFile;
    receiveDataCtx.d_journalFilePosition = fileSetSp->d_journalFilePosition;
    receiveDataCtx.d_mappedDataFd        = fileSetSp->d_dataFile;
    receiveDataCtx.d_dataFilePosition    = fileSetSp->d_dataFilePosition;

    return 0;
}

int RecoveryManager::openRecoveryFileSet(bsl::ostream& errorDescription,
                                         int           partitionId)
{
    // executed by the *STORAGE (QUEUE) DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(partitionId >= 0 &&
                     partitionId <
                         d_clusterConfig.partitionConfig().numPartitions());

    const int           k_MAX_NUM_FILE_SETS_TO_CHECK = 2;
    ReceiveDataContext& receiveDataCtx = d_receiveDataContextVec[partitionId];

    const int rc = mqbs::FileStoreUtil::openRecoveryFileSet(
        errorDescription,
        &receiveDataCtx.d_mappedJournalFd,
        &receiveDataCtx.d_recoveryFileSet,
        partitionId,
        k_MAX_NUM_FILE_SETS_TO_CHECK,
        d_dataStoreConfig,
        false,  // readOnly
        true);  // isFSMWorkflow
    if (rc != 0) {
        return rc;  // RETURN
    }

    receiveDataCtx.d_journalFilePosition = bdls::FilesystemUtil::getFileSize(
        receiveDataCtx.d_recoveryFileSet.journalFile());
    receiveDataCtx.d_dataFilePosition = bdls::FilesystemUtil::getFileSize(
        receiveDataCtx.d_recoveryFileSet.dataFile());

    return 0;
}

int RecoveryManager::recoverSeqNum(
    bmqp_ctrlmsg::PartitionSequenceNumber* seqNum,
    int                                    partitionId)
{
    // executed by the *STORAGE (QUEUE) DISPATCHER* thread

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

    mwcu::MemOutStream  errorDesc;
    ReceiveDataContext& receiveDataCtx = d_receiveDataContextVec[partitionId];
    int                 rc             = rc_UNKNOWN;

    if (!receiveDataCtx.d_mappedJournalFd.isValid()) {
        // Open journal and data files, if not already open.
        rc = mqbs::FileStoreUtil::openFileSetWriteMode(
            errorDesc,
            receiveDataCtx.d_recoveryFileSet,
            d_dataStoreConfig.hasPreallocate(),
            false,  // deleteOnFailure
            &receiveDataCtx.d_mappedJournalFd,
            0,  // dataFd
            0,  // qlistFd
            d_dataStoreConfig.hasPrefaultPages());
        if (rc != 0) {
            BALL_LOG_ERROR << d_clusterData_p->identity().description()
                           << " PartitionId [" << partitionId
                           << "]: file set: "
                           << receiveDataCtx.d_recoveryFileSet
                           << " failed to open. Reason: " << errorDesc.str()
                           << ", rc: " << rc;
            return rc_OPEN_FILE_SET_FAILURE;  // RETURN
        }

        rc = mqbs::FileStoreUtil::validateFileSet(
            receiveDataCtx.d_mappedJournalFd,
            mqbs::MappedFileDescriptor(),
            mqbs::MappedFileDescriptor());

        if (rc != 0) {
            BALL_LOG_ERROR << d_clusterData_p->identity().description()
                           << "PartitionId [" << partitionId << "]: file set: "
                           << receiveDataCtx.d_recoveryFileSet
                           << " validation failed, rc: " << rc;
            mqbs::FileSystemUtil::close(&receiveDataCtx.d_mappedJournalFd);

            return rc_INVALID_FILE_SET;  // RETURN
        }
    }

    mqbs::JournalFileIterator jit;
    rc = mqbs::FileStoreUtil::loadIterators(errorDesc,
                                            &jit,
                                            0,  // dit
                                            0,  // qit
                                            receiveDataCtx.d_mappedJournalFd,
                                            mqbs::MappedFileDescriptor(),
                                            mqbs::MappedFileDescriptor(),
                                            receiveDataCtx.d_recoveryFileSet,
                                            false,   // needQList
                                            false);  // needData

    if (rc != 0) {
        BALL_LOG_ERROR << "Error while iterating recovered files for Partition"
                       << " [" << partitionId << "] and return code " << rc
                       << " with description " << errorDesc.str();
        return 10 * rc + rc_FILE_ITERATOR_FAILURE;  // RETURN
    }

    if (jit.hasRecordSizeRemaining()) {
        const mqbs::RecordHeader& lastRecordHeader = jit.lastRecordHeader();

        BALL_LOG_INFO << "Recovered Sequence Number ["
                      << lastRecordHeader.partitionSequenceNumber()
                      << "] for Partition [" << partitionId << "] from"
                      << "journal file ["
                      << receiveDataCtx.d_recoveryFileSet.journalFile()
                      << "].";

        *seqNum = lastRecordHeader.partitionSequenceNumber();
    }

    return rc_SUCCESS;
}

void RecoveryManager::bufferStorageEvent(
    int                                 partitionId,
    const bsl::shared_ptr<bdlbb::Blob>& blob,
    mqbnet::ClusterNode*                source)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(0 <= partitionId);
    BSLS_ASSERT_SAFE(source);

    ReceiveDataContext& receiveDataCtx = d_receiveDataContextVec[partitionId];
    if (!receiveDataCtx.d_liveDataSource_p) {
        receiveDataCtx.d_liveDataSource_p = source;
    }
    else if (receiveDataCtx.d_liveDataSource_p != source) {
        BALL_LOG_ERROR << d_clusterData_p->identity().description()
                       << ": For PartitionId [" << partitionId
                       << ", storage event from node "
                       << source->nodeDescription() << "cannot be buffered, "
                       << "because it is different from the expected live "
                       << "data source node "
                       << receiveDataCtx.d_liveDataSource_p->nodeDescription()
                       << ".";

        return;  // RETURN
    }

    receiveDataCtx.d_bufferedEvents.push_back(blob);

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << ": For PartitionId [" << partitionId
                  << "], buffered a storage event from primary node "
                  << source->nodeDescription()
                  << " as self is still healing the partition.";
}

int RecoveryManager::loadBufferedStorageEvents(
    bsl::vector<bsl::shared_ptr<bdlbb::Blob> >* out,
    const mqbnet::ClusterNode*                  source,
    int                                         partitionId)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(out);
    BSLS_ASSERT_SAFE(source);
    BSLS_ASSERT_SAFE(0 <= partitionId);

    enum { rc_SUCCESS = 0, rc_UNEXPECTED_SOURCE = -1 };

    ReceiveDataContext& receiveDataCtx = d_receiveDataContextVec[partitionId];
    if (receiveDataCtx.d_bufferedEvents.empty()) {
        // We did not buffer any storage event.
        return rc_SUCCESS;  // RETURN
    }
    BSLS_ASSERT_SAFE(receiveDataCtx.d_liveDataSource_p);

    if (receiveDataCtx.d_liveDataSource_p != source) {
        BALL_LOG_ERROR << d_clusterData_p->identity().description()
                       << ": For PartitionId [" << partitionId
                       << ", storage event from node "
                       << source->nodeDescription() << "cannot be buffered, "
                       << "because it is different from the expected live "
                       << "data source node "
                       << receiveDataCtx.d_liveDataSource_p->nodeDescription()
                       << ".";

        return rc_UNEXPECTED_SOURCE;  // RETURN
    }

    *out = receiveDataCtx.d_bufferedEvents;

    receiveDataCtx.d_liveDataSource_p = 0;
    receiveDataCtx.d_bufferedEvents.clear();

    return rc_SUCCESS;
}

// ACCESSORS
void RecoveryManager::loadReplicaDataResponsePush(
    bmqp_ctrlmsg::ControlMessage* out,
    int                           partitionId) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(out);
    BSLS_ASSERT_SAFE(partitionId >= 0 &&
                     partitionId <
                         d_clusterConfig.partitionConfig().numPartitions());

    const ReceiveDataContext& receiveDataCtx =
        d_receiveDataContextVec[partitionId];

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
