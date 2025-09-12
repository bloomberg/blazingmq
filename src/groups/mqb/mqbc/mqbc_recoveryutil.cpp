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

// mqbc_recoveryutil.cpp                                              -*-C++-*-
#include <mqbc_recoveryutil.h>

#include <mqbscm_version.h>
// MQB
#include <mqbc_clusterutil.h>
#include <mqbs_datafileiterator.h>
#include <mqbs_filestoreprotocolutil.h>
#include <mqbs_filestoreset.h>
#include <mqbs_filestoreutil.h>
#include <mqbs_filesystemutil.h>
#include <mqbs_offsetptr.h>

#include <bmqu_memoutstream.h>

// BDE
#include <bsl_string.h>
#include <bsla_annotations.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace mqbc {

// ===================
// struct RecoveryUtil
// ===================
int RecoveryUtil::loadFileDescriptors(mqbs::MappedFileDescriptor* journalFd,
                                      mqbs::MappedFileDescriptor* dataFd,
                                      const mqbs::FileStoreSet&   fileSet,
                                      mqbs::MappedFileDescriptor* qlistFd)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(journalFd);
    BSLS_ASSERT_SAFE(dataFd);

    enum {
        rc_SUCCESS            = 0,
        rc_FAIL_OPEN_FILE_SET = -1,
        rc_INVALID_FILE_SET   = -2
    };

    bmqu::MemOutStream errorDesc;
    int                rc = mqbs::FileStoreUtil::openFileSetReadMode(errorDesc,
                                                      fileSet,
                                                      journalFd,
                                                      dataFd,
                                                      qlistFd);
    if (rc != 0) {
        BALL_LOG_WARN << "File set: " << fileSet
                      << " failed to open. Reason: " << errorDesc.str()
                      << ", rc: " << rc;
        return rc * 10 + rc_FAIL_OPEN_FILE_SET;  // RETURN
    }

    rc = mqbs::FileStoreUtil::validateFileSet(
        *journalFd,
        *dataFd,
        qlistFd ? *qlistFd : mqbs::MappedFileDescriptor());
    if (rc != 0) {
        // Close this set
        BALL_LOG_ERROR << "File set: " << fileSet
                       << " validation failed, rc: " << rc;
        mqbs::FileSystemUtil::close(journalFd);
        mqbs::FileSystemUtil::close(dataFd);
        if (qlistFd) {
            mqbs::FileSystemUtil::close(qlistFd);
        }
        return rc * 10 + rc_INVALID_FILE_SET;  // RETURN
    }

    return rc_SUCCESS;
}

void RecoveryUtil::validateArgs(
    BSLA_MAYBE_UNUSED const bmqp_ctrlmsg::PartitionSequenceNumber& beginSeqNum,
    BSLA_MAYBE_UNUSED const bmqp_ctrlmsg::PartitionSequenceNumber& endSeqNum,
    BSLA_MAYBE_UNUSED mqbnet::ClusterNode* destination)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(beginSeqNum < endSeqNum);
    BSLS_ASSERT_SAFE(0 < endSeqNum.primaryLeaseId());
    BSLS_ASSERT_SAFE(0 < endSeqNum.sequenceNumber());  // TBD: Is this ok?
    BSLS_ASSERT_SAFE(destination);
}

int RecoveryUtil::bootstrapCurrentSeqNum(
    bmqp_ctrlmsg::PartitionSequenceNumber*       currentSeqNum,
    mqbs::JournalFileIterator&                   journalIt,
    const bmqp_ctrlmsg::PartitionSequenceNumber& beginSeqNum)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(currentSeqNum);

    // Skip JOURNAL records until the record after 'beginSeqNum' is reached.
    // This assumes initial 'journalIt.nextRecord()' call has been done.
    int hadFound = false;

    do {
        const mqbs::RecordHeader& recordHeader = journalIt.recordHeader();
        currentSeqNum->primaryLeaseId()        = recordHeader.primaryLeaseId();
        currentSeqNum->sequenceNumber()        = recordHeader.sequenceNumber();

        if (beginSeqNum.primaryLeaseId() == 0 &&
            beginSeqNum.sequenceNumber() == 0) {
            // This is a special case.  There is never such record in journals,
            // so we stop at the very beginning
            return 0;  // RETURN
        }
        if (hadFound) {
            return 0;  // RETURN
        }
        if (*currentSeqNum == beginSeqNum) {
            // Need to iterate once more to the next record
            hadFound = true;
        }
        else if (*currentSeqNum > beginSeqNum) {
            // Past the 'beginSeqNum'.
            return -1;  // RETURN
        }
    } while (1 == journalIt.nextRecord());

    // We reached the end of JOURNAL, but couldn't reach the beginning
    // sequence number 'beginSeqNum'.
    return -1;
}

int RecoveryUtil::incrementCurrentSeqNum(
    bmqp_ctrlmsg::PartitionSequenceNumber* currentSeqNum,
    mqbs::JournalFileIterator&             journalIt)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(currentSeqNum);

    enum {
        rc_END_OF_JOURNAL           = 1,
        rc_SUCCESS                  = 0,
        rc_JOURNAL_ITERATOR_FAILURE = -1
    };

    const int rc = journalIt.nextRecord();
    if (rc == 0) {
        return rc_END_OF_JOURNAL;  // RETURN
    }
    else if (rc < 0) {
        return rc * 10 + rc_JOURNAL_ITERATOR_FAILURE;  // RETURN
    }
    BSLS_ASSERT_SAFE(rc == 1);  // Has next record

    BSLS_ASSERT_SAFE(mqbs::RecordType::e_UNDEFINED != journalIt.recordType());

    const mqbs::RecordHeader& recHeader = journalIt.recordHeader();

    currentSeqNum->primaryLeaseId() = recHeader.primaryLeaseId();
    currentSeqNum->sequenceNumber() = recHeader.sequenceNumber();

    return rc_SUCCESS;
}

void RecoveryUtil::processJournalRecord(
    bmqp::StorageMessageType::Enum*   storageMsgType,
    char**                            payloadRecordBase,
    int*                              payloadRecordLen,
    const mqbs::JournalFileIterator&  journalIt,
    const mqbs::MappedFileDescriptor& dataFd,
    bool                              qlistAware,
    const mqbs::MappedFileDescriptor& qlistFd)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(storageMsgType);
    BSLS_ASSERT_SAFE(payloadRecordBase);
    BSLS_ASSERT_SAFE(payloadRecordLen);
    BSLS_ASSERT_SAFE(qlistAware == qlistFd.isValid());

    if (mqbs::RecordType::e_JOURNAL_OP == journalIt.recordType()) {
        *storageMsgType = bmqp::StorageMessageType::e_JOURNAL_OP;
    }
    else if (mqbs::RecordType::e_QUEUE_OP == journalIt.recordType()) {
        const mqbs::QueueOpRecord& rec = journalIt.asQueueOpRecord();

        BSLS_ASSERT_SAFE(mqbs::QueueOpType::e_UNDEFINED != rec.type());

        // We dont fetch queue related records from qlistFd.
        if (!qlistAware) {
            if (mqbs::QueueOpType::e_CREATION == rec.type() ||
                mqbs::QueueOpType::e_ADDITION == rec.type()) {
                *storageMsgType = bmqp::StorageMessageType::e_QLIST;
            }
            else {
                *storageMsgType = bmqp::StorageMessageType::e_QUEUE_OP;
            }
            return;  // RETURN
        }

        if (mqbs::QueueOpType::e_CREATION == rec.type() ||
            mqbs::QueueOpType::e_ADDITION == rec.type()) {
            bsls::Types::Uint64 qlistOffset =
                static_cast<bsls::Types::Uint64>(
                    rec.queueUriRecordOffsetWords()) *
                bmqp::Protocol::k_WORD_SIZE;

            *payloadRecordBase = qlistFd.block().base() + qlistOffset;

            mqbs::OffsetPtr<const mqbs::QueueRecordHeader> qrHeader(
                qlistFd.block(),
                qlistOffset);
            *payloadRecordLen = qrHeader->queueRecordWords() *
                                bmqp::Protocol::k_WORD_SIZE;
            *storageMsgType = bmqp::StorageMessageType::e_QLIST;
        }
        else {
            *storageMsgType = bmqp::StorageMessageType::e_QUEUE_OP;
        }
    }
    else if (mqbs::RecordType::e_MESSAGE == journalIt.recordType()) {
        const mqbs::MessageRecord& rec = journalIt.asMessageRecord();

        bsls::Types::Uint64 dataOffset = static_cast<bsls::Types::Uint64>(
                                             rec.messageOffsetDwords()) *
                                         bmqp::Protocol::k_DWORD_SIZE;

        *payloadRecordBase = dataFd.block().base() + dataOffset;

        mqbs::OffsetPtr<const mqbs::DataHeader> dataHeader(dataFd.block(),
                                                           dataOffset);

        *payloadRecordLen = dataHeader->messageWords() *
                            bmqp::Protocol::k_WORD_SIZE;
        *storageMsgType = bmqp::StorageMessageType::e_DATA;
    }
    else if (mqbs::RecordType::e_CONFIRM == journalIt.recordType()) {
        *storageMsgType = bmqp::StorageMessageType::e_CONFIRM;
    }
    else {
        BSLS_ASSERT_SAFE(mqbs::RecordType::e_DELETION ==
                         journalIt.recordType());
        *storageMsgType = bmqp::StorageMessageType::e_DELETION;
    }
}

}  // close package namespace
}  // close enterprise namespace
