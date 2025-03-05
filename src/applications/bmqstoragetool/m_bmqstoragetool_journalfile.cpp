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

// bmqstoragetool
#include <m_bmqstoragetool_journalfile.h>

// MQB
#include <mqbs_memoryblock.h>
#include <mqbs_offsetptr.h>
#include <mqbu_messageguidutil.h>

namespace BloombergLP {

namespace m_bmqstoragetool {

// CONVENIENCE
using namespace mqbs;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------

// =================
// class JournalFile
// =================

void JournalFile::createFileHeader()
{
    d_currPos = 0;

    OffsetPtr<FileHeader> fh(d_block, d_currPos);
    new (fh.get()) FileHeader();
    d_fileHeader = *fh;
    d_currPos += sizeof(FileHeader);

    OffsetPtr<JournalFileHeader> jfh(d_block, d_currPos);
    new (jfh.get()) JournalFileHeader();  // Default values are ok
    d_currPos += sizeof(JournalFileHeader);
}

JournalFile::JournalFile(size_t numRecords, bslma::Allocator* allocator)
: d_numRecords(numRecords)
, d_mfd()
, d_buffer(allocator)
, d_block()
, d_currPos(0)
, d_timestampIncrement(100)
, d_fileHeader()
, d_iterator()
{
    bsls::Types::Uint64 totalSize =
        sizeof(FileHeader) + sizeof(JournalFileHeader) +
        numRecords * FileStoreProtocol::k_JOURNAL_RECORD_SIZE;

    d_buffer.resize(totalSize);
    d_block.setBase(d_buffer.data());
    d_block.setSize(totalSize);

    createFileHeader();

    d_mfd.setFd(-1);  // invalid fd will suffice.
    d_mfd.setBlock(d_block);
    d_mfd.setFileSize(totalSize);
}

JournalFile::~JournalFile()
{
    // NOTHING
}

JournalFile::RecordBufferType
JournalFile::makeQueueOpRecord(unsigned int        primaryLeaseId,
                               bsls::Types::Uint64 sequenceNumber)
{
    // QueueOpRec
    OffsetPtr<QueueOpRecord> rec(d_block, d_currPos);
    new (rec.get()) QueueOpRecord();
    rec->header()
        .setPrimaryLeaseId(primaryLeaseId)
        .setSequenceNumber(sequenceNumber)
        .setTimestamp(sequenceNumber * d_timestampIncrement);
    rec->setFlags(3)
        .setQueueKey(mqbu::StorageKey(mqbu::StorageKey::BinaryRepresentation(),
                                      "abcde"))
        .setAppKey(mqbu::StorageKey(mqbu::StorageKey::BinaryRepresentation(),
                                    "appid"))
        .setType(QueueOpType::e_PURGE)
        .setMagic(RecordHeader::k_MAGIC);

    RecordBufferType buf;
    bsl::memcpy(buf.buffer(),
                rec.get(),
                FileStoreProtocol::k_JOURNAL_RECORD_SIZE);
    return buf;
}

JournalFile::RecordBufferType
JournalFile::makeJournalOpRecord(unsigned int        primaryLeaseId,
                                 bsls::Types::Uint64 sequenceNumber)
{
    OffsetPtr<JournalOpRecord> rec(d_block, d_currPos);
    new (rec.get()) JournalOpRecord(JournalOpType::e_SYNCPOINT,
                                    SyncPointType::e_REGULAR,
                                    1234567,  // seqNum
                                    25,       // leaderTerm
                                    121,      // leaderNodeId
                                    8800,     // dataFilePosition
                                    100,      // qlistFilePosition
                                    RecordHeader::k_MAGIC);

    rec->header()
        .setPrimaryLeaseId(primaryLeaseId)
        .setSequenceNumber(sequenceNumber)
        .setTimestamp(sequenceNumber * d_timestampIncrement);
    RecordBufferType buf;
    bsl::memcpy(buf.buffer(),
                rec.get(),
                FileStoreProtocol::k_JOURNAL_RECORD_SIZE);
    return buf;
}

void JournalFile::addAllTypesRecords(RecordsListType* records)
{
    // PRECONDITIONS
    BSLS_ASSERT(records);

    for (unsigned int i = 1; i <= d_numRecords; ++i) {
        unsigned int remainder = i % 5;
        if (0 == remainder) {
            bmqt::MessageGUID guid;
            mqbu::MessageGUIDUtil::generateGUID(&guid);
            OffsetPtr<MessageRecord> rec(d_block, d_currPos);
            new (rec.get()) MessageRecord();
            rec->header()
                .setPrimaryLeaseId(100)
                .setSequenceNumber(i)
                .setTimestamp(i * d_timestampIncrement);
            rec->setRefCount(i % FileStoreProtocol::k_MAX_MSG_REF_COUNT_HARD)
                .setQueueKey(
                    mqbu::StorageKey(mqbu::StorageKey::BinaryRepresentation(),
                                     "abcde"))
                .setFileKey(
                    mqbu::StorageKey(mqbu::StorageKey::BinaryRepresentation(),
                                     "12345"))
                .setMessageOffsetDwords(i)
                .setMessageGUID(guid)
                .setCrc32c(i)
                .setCompressionAlgorithmType(
                    bmqt::CompressionAlgorithmType::e_NONE)
                .setMagic(RecordHeader::k_MAGIC);

            RecordBufferType buf;
            bsl::memcpy(buf.buffer(),
                        rec.get(),
                        FileStoreProtocol::k_JOURNAL_RECORD_SIZE);
            records->push_back(bsl::make_pair(RecordType::e_MESSAGE, buf));
        }
        else if (1 == remainder) {
            // ConfRec
            bmqt::MessageGUID guid;
            mqbu::MessageGUIDUtil::generateGUID(&guid);
            OffsetPtr<ConfirmRecord> rec(d_block, d_currPos);
            new (rec.get()) ConfirmRecord();
            rec->header()
                .setPrimaryLeaseId(100)
                .setSequenceNumber(i)
                .setTimestamp(i * d_timestampIncrement);
            rec->setReason(ConfirmReason::e_REJECTED)
                .setQueueKey(
                    mqbu::StorageKey(mqbu::StorageKey::BinaryRepresentation(),
                                     "abcde"))
                .setAppKey(
                    mqbu::StorageKey(mqbu::StorageKey::BinaryRepresentation(),
                                     "appid"))
                .setMessageGUID(guid)
                .setMagic(RecordHeader::k_MAGIC);

            RecordBufferType buf;
            bsl::memcpy(buf.buffer(),
                        rec.get(),
                        FileStoreProtocol::k_JOURNAL_RECORD_SIZE);
            records->push_back(bsl::make_pair(RecordType::e_CONFIRM, buf));
        }
        else if (2 == remainder) {
            // DelRec
            bmqt::MessageGUID guid;
            mqbu::MessageGUIDUtil::generateGUID(&guid);
            OffsetPtr<DeletionRecord> rec(d_block, d_currPos);
            new (rec.get()) DeletionRecord();
            rec->header()
                .setPrimaryLeaseId(100)
                .setSequenceNumber(i)
                .setTimestamp(i * d_timestampIncrement);
            rec->setDeletionRecordFlag(DeletionRecordFlag::e_IMPLICIT_CONFIRM)
                .setQueueKey(
                    mqbu::StorageKey(mqbu::StorageKey::BinaryRepresentation(),
                                     "abcde"))
                .setMessageGUID(guid)
                .setMagic(RecordHeader::k_MAGIC);

            RecordBufferType buf;
            bsl::memcpy(buf.buffer(),
                        rec.get(),
                        FileStoreProtocol::k_JOURNAL_RECORD_SIZE);
            records->push_back(bsl::make_pair(RecordType::e_DELETION, buf));
        }
        else if (3 == remainder) {
            records->push_back(bsl::make_pair(RecordType::e_QUEUE_OP,
                                              makeQueueOpRecord(100, i)));
        }
        else {
            records->push_back(bsl::make_pair(RecordType::e_JOURNAL_OP,
                                              makeJournalOpRecord(100, i)));
        }

        d_currPos += FileStoreProtocol::k_JOURNAL_RECORD_SIZE;
    }
}

void JournalFile::addJournalRecordsWithOutstandingAndConfirmedMessages(
    RecordsListType*                records,
    bsl::vector<bmqt::MessageGUID>* expectedGUIDs,
    bool                            expectOutstandingResult)
{
    // PRECONDITIONS
    BSLS_ASSERT(records);
    BSLS_ASSERT(expectedGUIDs);

    bool              outstandingFlag = false;
    bmqt::MessageGUID lastMessageGUID;

    for (unsigned int i = 1; i <= d_numRecords; ++i) {
        unsigned int remainder = i % 3;
        if (1 == remainder) {
            mqbu::MessageGUIDUtil::generateGUID(&lastMessageGUID);
            OffsetPtr<MessageRecord> rec(d_block, d_currPos);
            new (rec.get()) MessageRecord();
            rec->header()
                .setPrimaryLeaseId(100)
                .setSequenceNumber(i)
                .setTimestamp(i * d_timestampIncrement);
            rec->setRefCount(i % FileStoreProtocol::k_MAX_MSG_REF_COUNT_HARD)
                .setQueueKey(
                    mqbu::StorageKey(mqbu::StorageKey::BinaryRepresentation(),
                                     "abcde"))
                .setFileKey(
                    mqbu::StorageKey(mqbu::StorageKey::BinaryRepresentation(),
                                     "12345"))
                .setMessageOffsetDwords(i)
                .setMessageGUID(lastMessageGUID)
                .setCrc32c(i)
                .setCompressionAlgorithmType(
                    bmqt::CompressionAlgorithmType::e_NONE)
                .setMagic(RecordHeader::k_MAGIC);

            RecordBufferType buf;
            bsl::memcpy(buf.buffer(),
                        rec.get(),
                        FileStoreProtocol::k_JOURNAL_RECORD_SIZE);
            records->push_back(bsl::make_pair(RecordType::e_MESSAGE, buf));
        }
        else if (2 == remainder) {
            // ConfRec
            bmqt::MessageGUID        guid = lastMessageGUID;
            OffsetPtr<ConfirmRecord> rec(d_block, d_currPos);
            new (rec.get()) ConfirmRecord();
            rec->header()
                .setPrimaryLeaseId(100)
                .setSequenceNumber(i)
                .setTimestamp(i * d_timestampIncrement);
            rec->setReason(ConfirmReason::e_REJECTED)
                .setQueueKey(
                    mqbu::StorageKey(mqbu::StorageKey::BinaryRepresentation(),
                                     "abcde"))
                .setAppKey(
                    mqbu::StorageKey(mqbu::StorageKey::BinaryRepresentation(),
                                     "appid"))
                .setMessageGUID(guid)
                .setMagic(RecordHeader::k_MAGIC);

            RecordBufferType buf;
            bsl::memcpy(buf.buffer(),
                        rec.get(),
                        FileStoreProtocol::k_JOURNAL_RECORD_SIZE);
            records->push_back(bsl::make_pair(RecordType::e_CONFIRM, buf));
        }
        else {
            // DelRec
            bmqt::MessageGUID guid;
            if (outstandingFlag) {
                mqbu::MessageGUIDUtil::generateGUID(&guid);
                if (expectOutstandingResult)
                    expectedGUIDs->push_back(lastMessageGUID);
            }
            else {
                guid = lastMessageGUID;
                if (!expectOutstandingResult)
                    expectedGUIDs->push_back(lastMessageGUID);
            }
            outstandingFlag = !outstandingFlag;
            OffsetPtr<DeletionRecord> rec(d_block, d_currPos);
            new (rec.get()) DeletionRecord();
            rec->header()
                .setPrimaryLeaseId(100)
                .setSequenceNumber(i)
                .setTimestamp(i * d_timestampIncrement);
            rec->setDeletionRecordFlag(DeletionRecordFlag::e_IMPLICIT_CONFIRM)
                .setQueueKey(
                    mqbu::StorageKey(mqbu::StorageKey::BinaryRepresentation(),
                                     "abcde"))
                .setMessageGUID(guid)
                .setMagic(RecordHeader::k_MAGIC);

            RecordBufferType buf;
            bsl::memcpy(buf.buffer(),
                        rec.get(),
                        FileStoreProtocol::k_JOURNAL_RECORD_SIZE);
            records->push_back(bsl::make_pair(RecordType::e_DELETION, buf));
        }

        d_currPos += FileStoreProtocol::k_JOURNAL_RECORD_SIZE;
    }
}

void JournalFile::addJournalRecordsWithPartiallyConfirmedMessages(
    RecordsListType*                records,
    bsl::vector<bmqt::MessageGUID>* expectedGUIDs)
{
    // PRECONDITIONS
    BSLS_ASSERT(records);
    BSLS_ASSERT(expectedGUIDs);

    bool              partialyConfirmedFlag = false;
    bmqt::MessageGUID lastMessageGUID;

    for (unsigned int i = 1; i <= d_numRecords; ++i) {
        unsigned int remainder = i % 3;

        if (1 == remainder) {
            mqbu::MessageGUIDUtil::generateGUID(&lastMessageGUID);
            OffsetPtr<MessageRecord> rec(d_block, d_currPos);
            new (rec.get()) MessageRecord();
            rec->header()
                .setPrimaryLeaseId(100)
                .setSequenceNumber(i)
                .setTimestamp(i * d_timestampIncrement);
            rec->setRefCount(i % FileStoreProtocol::k_MAX_MSG_REF_COUNT_HARD)
                .setQueueKey(
                    mqbu::StorageKey(mqbu::StorageKey::BinaryRepresentation(),
                                     "abcde"))
                .setFileKey(
                    mqbu::StorageKey(mqbu::StorageKey::BinaryRepresentation(),
                                     "12345"))
                .setMessageOffsetDwords(i)
                .setMessageGUID(lastMessageGUID)
                .setCrc32c(i)
                .setCompressionAlgorithmType(
                    bmqt::CompressionAlgorithmType::e_NONE)
                .setMagic(RecordHeader::k_MAGIC);

            RecordBufferType buf;
            bsl::memcpy(buf.buffer(),
                        rec.get(),
                        FileStoreProtocol::k_JOURNAL_RECORD_SIZE);
            records->push_back(bsl::make_pair(RecordType::e_MESSAGE, buf));
        }
        else if (2 == remainder) {
            // ConfRec
            bmqt::MessageGUID        guid = lastMessageGUID;
            OffsetPtr<ConfirmRecord> rec(d_block, d_currPos);
            new (rec.get()) ConfirmRecord();
            rec->header()
                .setPrimaryLeaseId(100)
                .setSequenceNumber(i)
                .setTimestamp(i * d_timestampIncrement);
            rec->setReason(ConfirmReason::e_REJECTED)
                .setQueueKey(
                    mqbu::StorageKey(mqbu::StorageKey::BinaryRepresentation(),
                                     "abcde"))
                .setAppKey(
                    mqbu::StorageKey(mqbu::StorageKey::BinaryRepresentation(),
                                     "appid"))
                .setMessageGUID(guid)
                .setMagic(RecordHeader::k_MAGIC);

            RecordBufferType buf;
            bsl::memcpy(buf.buffer(),
                        rec.get(),
                        FileStoreProtocol::k_JOURNAL_RECORD_SIZE);
            records->push_back(bsl::make_pair(RecordType::e_CONFIRM, buf));
        }
        else {
            // DelRec
            bmqt::MessageGUID guid;
            if (partialyConfirmedFlag) {
                mqbu::MessageGUIDUtil::generateGUID(&guid);
                expectedGUIDs->push_back(lastMessageGUID);
            }
            else {
                guid = lastMessageGUID;
            }
            partialyConfirmedFlag = !partialyConfirmedFlag;
            OffsetPtr<DeletionRecord> rec(d_block, d_currPos);
            new (rec.get()) DeletionRecord();
            rec->header()
                .setPrimaryLeaseId(100)
                .setSequenceNumber(i)
                .setTimestamp(i * d_timestampIncrement);
            rec->setDeletionRecordFlag(DeletionRecordFlag::e_IMPLICIT_CONFIRM)
                .setQueueKey(
                    mqbu::StorageKey(mqbu::StorageKey::BinaryRepresentation(),
                                     "abcde"))
                .setMessageGUID(guid)
                .setMagic(RecordHeader::k_MAGIC);

            RecordBufferType buf;
            bsl::memcpy(buf.buffer(),
                        rec.get(),
                        FileStoreProtocol::k_JOURNAL_RECORD_SIZE);
            records->push_back(bsl::make_pair(RecordType::e_DELETION, buf));
        }

        d_currPos += FileStoreProtocol::k_JOURNAL_RECORD_SIZE;
    }
}

void JournalFile::addJournalRecordsWithTwoQueueKeys(
    RecordsListType* records,
    GuidVectorType*  expectedGUIDs,
    const char*      queueKey1,
    const char*      queueKey2,
    bool             captureAllGUIDs)
{
    bmqt::MessageGUID lastMessageGUID;

    for (unsigned int i = 1; i <= d_numRecords; ++i) {
        const char* queueKey = (i % 2 != 0) ? queueKey1 : queueKey2;

        unsigned int remainder = i % 3;
        if (1 == remainder) {
            mqbu::MessageGUIDUtil::generateGUID(&lastMessageGUID);
            if (captureAllGUIDs || queueKey == queueKey1) {
                expectedGUIDs->push_back(lastMessageGUID);
            }
            OffsetPtr<MessageRecord> rec(d_block, d_currPos);
            new (rec.get()) MessageRecord();
            rec->header()
                .setPrimaryLeaseId(100)
                .setSequenceNumber(i)
                .setTimestamp(i * d_timestampIncrement);
            rec->setRefCount(i % FileStoreProtocol::k_MAX_MSG_REF_COUNT_HARD)
                .setQueueKey(
                    mqbu::StorageKey(mqbu::StorageKey::HexRepresentation(),
                                     queueKey))
                .setFileKey(
                    mqbu::StorageKey(mqbu::StorageKey::BinaryRepresentation(),
                                     "12345"))
                .setMessageOffsetDwords(i)
                .setMessageGUID(lastMessageGUID)
                .setCrc32c(i)
                .setCompressionAlgorithmType(
                    bmqt::CompressionAlgorithmType::e_NONE)
                .setMagic(RecordHeader::k_MAGIC);

            RecordBufferType buf;
            bsl::memcpy(buf.buffer(),
                        rec.get(),
                        FileStoreProtocol::k_JOURNAL_RECORD_SIZE);
            records->push_back(bsl::make_pair(RecordType::e_MESSAGE, buf));
        }
        else if (2 == remainder) {
            // ConfRec
            bmqt::MessageGUID        guid = lastMessageGUID;
            OffsetPtr<ConfirmRecord> rec(d_block, d_currPos);
            new (rec.get()) ConfirmRecord();
            rec->header()
                .setPrimaryLeaseId(100)
                .setSequenceNumber(i)
                .setTimestamp(i * d_timestampIncrement);
            rec->setReason(ConfirmReason::e_REJECTED)
                .setQueueKey(
                    mqbu::StorageKey(mqbu::StorageKey::HexRepresentation(),
                                     queueKey))
                .setAppKey(
                    mqbu::StorageKey(mqbu::StorageKey::BinaryRepresentation(),
                                     "appid"))
                .setMessageGUID(guid)
                .setMagic(RecordHeader::k_MAGIC);

            RecordBufferType buf;
            bsl::memcpy(buf.buffer(),
                        rec.get(),
                        FileStoreProtocol::k_JOURNAL_RECORD_SIZE);
            records->push_back(bsl::make_pair(RecordType::e_CONFIRM, buf));
        }
        else {
            // DelRec
            bmqt::MessageGUID         guid = lastMessageGUID;
            OffsetPtr<DeletionRecord> rec(d_block, d_currPos);
            new (rec.get()) DeletionRecord();
            rec->header()
                .setPrimaryLeaseId(100)
                .setSequenceNumber(i)
                .setTimestamp(i * d_timestampIncrement);
            rec->setDeletionRecordFlag(DeletionRecordFlag::e_IMPLICIT_CONFIRM)
                .setQueueKey(
                    mqbu::StorageKey(mqbu::StorageKey::HexRepresentation(),
                                     queueKey))
                .setMessageGUID(guid)
                .setMagic(RecordHeader::k_MAGIC);

            RecordBufferType buf;
            bsl::memcpy(buf.buffer(),
                        rec.get(),
                        FileStoreProtocol::k_JOURNAL_RECORD_SIZE);
            records->push_back(bsl::make_pair(RecordType::e_DELETION, buf));
        }

        d_currPos += FileStoreProtocol::k_JOURNAL_RECORD_SIZE;
    }
}

void JournalFile::addJournalRecordsWithConfirmedMessagesWithDifferentOrder(
    RecordsListType*           records,
    GuidVectorType*            expectedGUIDs,
    size_t                     numMessages,
    bsl::vector<unsigned int>& messageOffsets)
{
    // PRECONDITIONS
    BSLS_ASSERT(numMessages == messageOffsets.size());
    BSLS_ASSERT(numMessages >= 3);
    BSLS_ASSERT(d_numRecords == numMessages * 2);

    // Create messages
    for (unsigned int i = 0; i < numMessages; ++i) {
        bmqt::MessageGUID guid;
        mqbu::MessageGUIDUtil::generateGUID(&guid);
        expectedGUIDs->push_back(guid);
        OffsetPtr<MessageRecord> rec(d_block, d_currPos);
        new (rec.get()) MessageRecord();
        rec->header()
            .setPrimaryLeaseId(100)
            .setSequenceNumber(i + 1)
            .setTimestamp(i * d_timestampIncrement);
        rec->setRefCount(i % FileStoreProtocol::k_MAX_MSG_REF_COUNT_HARD)
            .setQueueKey(
                mqbu::StorageKey(mqbu::StorageKey::BinaryRepresentation(),
                                 "abcde"))
            .setFileKey(
                mqbu::StorageKey(mqbu::StorageKey::BinaryRepresentation(),
                                 "12345"))
            .setMessageOffsetDwords(messageOffsets.at(i))
            .setMessageGUID(guid)
            .setCrc32c(i)
            .setCompressionAlgorithmType(
                bmqt::CompressionAlgorithmType::e_NONE)
            .setMagic(RecordHeader::k_MAGIC);

        RecordBufferType buf;
        bsl::memcpy(buf.buffer(),
                    rec.get(),
                    FileStoreProtocol::k_JOURNAL_RECORD_SIZE);
        records->push_back(bsl::make_pair(RecordType::e_MESSAGE, buf));

        d_currPos += FileStoreProtocol::k_JOURNAL_RECORD_SIZE;
    }

    // Create delete messages, and replace GUIDS for 2nd and 3rd message
    for (unsigned int i = 0; i < numMessages; ++i) {
        // DelRec
        bmqt::MessageGUID guid;
        // Change GUIDs order for 2nd and 3rd deletion records
        if (i == 1) {
            guid = expectedGUIDs->at(2);
        }
        else if (i == 2) {
            guid = expectedGUIDs->at(1);
        }
        else {
            guid = expectedGUIDs->at(i);
        }

        OffsetPtr<DeletionRecord> rec(d_block, d_currPos);
        new (rec.get()) DeletionRecord();
        rec->header()
            .setPrimaryLeaseId(100)
            .setSequenceNumber(i + 1)
            .setTimestamp(i * d_timestampIncrement);
        rec->setDeletionRecordFlag(DeletionRecordFlag::e_IMPLICIT_CONFIRM)
            .setQueueKey(
                mqbu::StorageKey(mqbu::StorageKey::BinaryRepresentation(),
                                 "abcde"))
            .setMessageGUID(guid)
            .setMagic(RecordHeader::k_MAGIC);

        RecordBufferType buf;
        bsl::memcpy(buf.buffer(),
                    rec.get(),
                    FileStoreProtocol::k_JOURNAL_RECORD_SIZE);
        records->push_back(bsl::make_pair(RecordType::e_DELETION, buf));

        d_currPos += FileStoreProtocol::k_JOURNAL_RECORD_SIZE;
    }
}

void JournalFile::addMultipleTypesRecordsWithMultipleLeaseId(
    RecordsListType* records,
    size_t           numRecordsWithSameLeaseId)
{
    // PRECONDITIONS
    BSLS_ASSERT(records);

    unsigned int        leaseId   = 0;
    bsls::Types::Uint64 seqNumber = 1;

    for (unsigned int i = 1; i <= d_numRecords; ++i) {
        unsigned int remainder = i % 4;
        if (i % numRecordsWithSameLeaseId == 1) {
            leaseId++;
            seqNumber = 1;
        }
        if (0 == remainder) {
            bmqt::MessageGUID guid;
            mqbu::MessageGUIDUtil::generateGUID(&guid);
            OffsetPtr<MessageRecord> rec(d_block, d_currPos);
            new (rec.get()) MessageRecord();
            rec->header()
                .setPrimaryLeaseId(leaseId)
                .setSequenceNumber(seqNumber++)
                .setTimestamp(i * d_timestampIncrement);
            rec->setRefCount(i % FileStoreProtocol::k_MAX_MSG_REF_COUNT_HARD)
                .setQueueKey(
                    mqbu::StorageKey(mqbu::StorageKey::BinaryRepresentation(),
                                     "abcde"))
                .setFileKey(
                    mqbu::StorageKey(mqbu::StorageKey::BinaryRepresentation(),
                                     "12345"))
                .setMessageOffsetDwords(i)
                .setMessageGUID(guid)
                .setCrc32c(i)
                .setCompressionAlgorithmType(
                    bmqt::CompressionAlgorithmType::e_NONE)
                .setMagic(RecordHeader::k_MAGIC);

            RecordBufferType buf;
            bsl::memcpy(buf.buffer(),
                        rec.get(),
                        FileStoreProtocol::k_JOURNAL_RECORD_SIZE);
            records->push_back(bsl::make_pair(RecordType::e_MESSAGE, buf));
        }
        else if (1 == remainder) {
            // ConfRec
            bmqt::MessageGUID guid;
            mqbu::MessageGUIDUtil::generateGUID(&guid);
            OffsetPtr<ConfirmRecord> rec(d_block, d_currPos);
            new (rec.get()) ConfirmRecord();
            rec->header()
                .setPrimaryLeaseId(leaseId)
                .setSequenceNumber(seqNumber++)
                .setTimestamp(i * d_timestampIncrement);
            rec->setReason(ConfirmReason::e_REJECTED)
                .setQueueKey(
                    mqbu::StorageKey(mqbu::StorageKey::BinaryRepresentation(),
                                     "abcde"))
                .setAppKey(
                    mqbu::StorageKey(mqbu::StorageKey::BinaryRepresentation(),
                                     "appid"))
                .setMessageGUID(guid)
                .setMagic(RecordHeader::k_MAGIC);

            RecordBufferType buf;
            bsl::memcpy(buf.buffer(),
                        rec.get(),
                        FileStoreProtocol::k_JOURNAL_RECORD_SIZE);
            records->push_back(bsl::make_pair(RecordType::e_CONFIRM, buf));
        }
        else if (2 == remainder) {
            // DelRec
            bmqt::MessageGUID guid;
            mqbu::MessageGUIDUtil::generateGUID(&guid);
            OffsetPtr<DeletionRecord> rec(d_block, d_currPos);
            new (rec.get()) DeletionRecord();
            rec->header()
                .setPrimaryLeaseId(leaseId)
                .setSequenceNumber(seqNumber++)
                .setTimestamp(i * d_timestampIncrement);
            rec->setDeletionRecordFlag(DeletionRecordFlag::e_IMPLICIT_CONFIRM)
                .setQueueKey(
                    mqbu::StorageKey(mqbu::StorageKey::BinaryRepresentation(),
                                     "abcde"))
                .setMessageGUID(guid)
                .setMagic(RecordHeader::k_MAGIC);

            RecordBufferType buf;
            bsl::memcpy(buf.buffer(),
                        rec.get(),
                        FileStoreProtocol::k_JOURNAL_RECORD_SIZE);
            records->push_back(bsl::make_pair(RecordType::e_DELETION, buf));
        }
        else {
            // QueueOpRec
            records->push_back(
                bsl::make_pair(RecordType::e_QUEUE_OP,
                               makeQueueOpRecord(leaseId, seqNumber++)));
        }

        d_currPos += FileStoreProtocol::k_JOURNAL_RECORD_SIZE;
    }
}

}  // close package namespace

}  // close enterprise namespace
