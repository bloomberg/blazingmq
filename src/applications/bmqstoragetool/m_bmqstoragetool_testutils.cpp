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
#include <m_bmqstoragetool_testutils.h>

namespace BloombergLP {

namespace m_bmqstoragetool {

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------

namespace TestUtils {

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

JournalFile::JournalFile(const size_t numRecords, bslma::Allocator* allocator)
: d_numRecords(numRecords)
, d_timestampIncrement(100)
, d_allocator_p(allocator)
{
    bsls::Types::Uint64 totalSize =
        sizeof(FileHeader) + sizeof(JournalFileHeader) +
        numRecords * FileStoreProtocol::k_JOURNAL_RECORD_SIZE;

    d_mem_p = static_cast<char*>(d_allocator_p->allocate(totalSize));
    d_block.setBase(d_mem_p);
    d_block.setSize(totalSize);

    createFileHeader();

    d_mfd.setFd(-1);  // invalid fd will suffice.
    d_mfd.setBlock(d_block);
    d_mfd.setFileSize(totalSize);
}

JournalFile::~JournalFile()
{
    d_allocator_p->deallocate(d_mem_p);
}

void JournalFile::addAllTypesRecords(RecordsListType* records)
{
    for (unsigned int i = 1; i <= d_numRecords; ++i) {
        unsigned int remainder = i % 5;
        if (0 == remainder) {
            bmqt::MessageGUID g;
            mqbu::MessageGUIDUtil::generateGUID(&g);
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
                .setMessageGUID(g)
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
            bmqt::MessageGUID g;
            mqbu::MessageGUIDUtil::generateGUID(&g);
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
                .setMessageGUID(g)
                .setMagic(RecordHeader::k_MAGIC);

            RecordBufferType buf;
            bsl::memcpy(buf.buffer(),
                        rec.get(),
                        FileStoreProtocol::k_JOURNAL_RECORD_SIZE);
            records->push_back(bsl::make_pair(RecordType::e_CONFIRM, buf));
        }
        else if (2 == remainder) {
            // DelRec
            bmqt::MessageGUID g;
            mqbu::MessageGUIDUtil::generateGUID(&g);
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
                .setMessageGUID(g)
                .setMagic(RecordHeader::k_MAGIC);

            RecordBufferType buf;
            bsl::memcpy(buf.buffer(),
                        rec.get(),
                        FileStoreProtocol::k_JOURNAL_RECORD_SIZE);
            records->push_back(bsl::make_pair(RecordType::e_DELETION, buf));
        }
        else if (3 == remainder) {
            // QueueOpRec
            OffsetPtr<QueueOpRecord> rec(d_block, d_currPos);
            new (rec.get()) QueueOpRecord();
            rec->header()
                .setPrimaryLeaseId(100)
                .setSequenceNumber(i)
                .setTimestamp(i * d_timestampIncrement);
            rec->setFlags(3)
                .setQueueKey(
                    mqbu::StorageKey(mqbu::StorageKey::BinaryRepresentation(),
                                     "abcde"))
                .setAppKey(
                    mqbu::StorageKey(mqbu::StorageKey::BinaryRepresentation(),
                                     "appid"))
                .setType(QueueOpType::e_PURGE)
                .setMagic(RecordHeader::k_MAGIC);

            RecordBufferType buf;
            bsl::memcpy(buf.buffer(),
                        rec.get(),
                        FileStoreProtocol::k_JOURNAL_RECORD_SIZE);
            records->push_back(bsl::make_pair(RecordType::e_QUEUE_OP, buf));
        }
        else {
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
                .setPrimaryLeaseId(100)
                .setSequenceNumber(i)
                .setTimestamp(i * d_timestampIncrement);
            RecordBufferType buf;
            bsl::memcpy(buf.buffer(),
                        rec.get(),
                        FileStoreProtocol::k_JOURNAL_RECORD_SIZE);
            records->push_back(bsl::make_pair(RecordType::e_JOURNAL_OP, buf));
        }

        d_currPos += FileStoreProtocol::k_JOURNAL_RECORD_SIZE;
    }
}

void JournalFile::addJournalRecordsWithOutstandingAndConfirmedMessages(
    RecordsListType*                records,
    bsl::vector<bmqt::MessageGUID>* expectedGUIDs,
    bool                            expectOutstandingResult)
{
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
            bmqt::MessageGUID        g = lastMessageGUID;
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
                .setMessageGUID(g)
                .setMagic(RecordHeader::k_MAGIC);

            RecordBufferType buf;
            bsl::memcpy(buf.buffer(),
                        rec.get(),
                        FileStoreProtocol::k_JOURNAL_RECORD_SIZE);
            records->push_back(bsl::make_pair(RecordType::e_CONFIRM, buf));
        }
        else {
            // DelRec
            bmqt::MessageGUID g;
            if (outstandingFlag) {
                mqbu::MessageGUIDUtil::generateGUID(&g);
                if (expectOutstandingResult)
                    expectedGUIDs->push_back(lastMessageGUID);
            }
            else {
                g = lastMessageGUID;
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
                .setMessageGUID(g)
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
    bool              partialyConfirmedFlag = false;
    bmqt::MessageGUID lastMessageGUID;

    for (unsigned int i = 1; i <= d_numRecords; ++i) {
        unsigned int remainder = i % 3;

        if (1 == remainder) {
            // bmqt::MessageGUID g;
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
            bmqt::MessageGUID        g = lastMessageGUID;
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
                .setMessageGUID(g)
                .setMagic(RecordHeader::k_MAGIC);

            RecordBufferType buf;
            bsl::memcpy(buf.buffer(),
                        rec.get(),
                        FileStoreProtocol::k_JOURNAL_RECORD_SIZE);
            records->push_back(bsl::make_pair(RecordType::e_CONFIRM, buf));
        }
        else {
            // DelRec
            bmqt::MessageGUID g;
            if (partialyConfirmedFlag) {
                mqbu::MessageGUIDUtil::generateGUID(&g);
                expectedGUIDs->push_back(lastMessageGUID);
            }
            else {
                g = lastMessageGUID;
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
                .setMessageGUID(g)
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
            bmqt::MessageGUID        g = lastMessageGUID;
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
                .setMessageGUID(g)
                .setMagic(RecordHeader::k_MAGIC);

            RecordBufferType buf;
            bsl::memcpy(buf.buffer(),
                        rec.get(),
                        FileStoreProtocol::k_JOURNAL_RECORD_SIZE);
            records->push_back(bsl::make_pair(RecordType::e_CONFIRM, buf));
        }
        else {
            // DelRec
            bmqt::MessageGUID         g = lastMessageGUID;
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
                .setMessageGUID(g)
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
    ASSERT(numMessages == messageOffsets.size());
    ASSERT(numMessages >= 3);
    ASSERT(d_numRecords == numMessages * 2);

    // Create messages
    for (unsigned int i = 0; i < numMessages; ++i) {
        bmqt::MessageGUID g;
        mqbu::MessageGUIDUtil::generateGUID(&g);
        expectedGUIDs->push_back(g);
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
            .setMessageGUID(g)
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
        bmqt::MessageGUID g;
        // Change GUIDs order for 2nd and 3rd deletion records
        if (i == 1) {
            g = expectedGUIDs->at(2);
        }
        else if (i == 2) {
            g = expectedGUIDs->at(1);
        }
        else {
            g = expectedGUIDs->at(i);
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
            .setMessageGUID(g)
            .setMagic(RecordHeader::k_MAGIC);

        RecordBufferType buf;
        bsl::memcpy(buf.buffer(),
                    rec.get(),
                    FileStoreProtocol::k_JOURNAL_RECORD_SIZE);
        records->push_back(bsl::make_pair(RecordType::e_DELETION, buf));

        d_currPos += FileStoreProtocol::k_JOURNAL_RECORD_SIZE;
    }
}

char* addDataRecords(bslma::Allocator*          ta,
                     MappedFileDescriptor*      mfd,
                     FileHeader*                fileHeader,
                     const DataMessage*         messages,
                     const unsigned int         numMessages,
                     bsl::vector<unsigned int>& messageOffsets)
{
    bsls::Types::Uint64 currPos = 0;
    const unsigned int  dhSize  = sizeof(DataHeader);
    unsigned int totalSize      = sizeof(FileHeader) + sizeof(DataFileHeader);

    // Have to compute the 'totalSize' we need for the 'MemoryBlock' based on
    // the padding that we need for each record.

    for (unsigned int i = 0; i < numMessages; i++) {
        unsigned int optionsLen = bsl::strlen(messages[i].d_options_p);
        BSLS_ASSERT_OPT(0 == optionsLen % bmqp::Protocol::k_WORD_SIZE);

        unsigned int appDataLen     = bsl::strlen(messages[i].d_appData_p);
        int          appDataPadding = 0;
        bmqp::ProtocolUtil::calcNumDwordsAndPadding(&appDataPadding,
                                                    appDataLen + optionsLen +
                                                        dhSize);

        totalSize += dhSize + appDataLen + appDataPadding + optionsLen;
    }

    // Allocate the memory now.
    char* p = static_cast<char*>(ta->allocate(totalSize));

    // Create the 'MemoryBlock'
    MemoryBlock block(p, totalSize);

    // Set the MFD
    mfd->setFd(-1);
    mfd->setBlock(block);
    mfd->setFileSize(totalSize);

    // Add the entries to the block.
    OffsetPtr<FileHeader> fh(block, currPos);
    new (fh.get()) FileHeader();
    fh->setHeaderWords(sizeof(FileHeader) / bmqp::Protocol::k_WORD_SIZE);
    fh->setMagic1(FileHeader::k_MAGIC1);
    fh->setMagic2(FileHeader::k_MAGIC2);
    currPos += sizeof(FileHeader);

    OffsetPtr<DataFileHeader> dfh(block, currPos);
    new (dfh.get()) DataFileHeader();
    dfh->setHeaderWords(sizeof(DataFileHeader) / bmqp::Protocol::k_WORD_SIZE);
    currPos += sizeof(DataFileHeader);

    for (unsigned int i = 0; i < numMessages; i++) {
        messageOffsets.push_back(currPos / bmqp::Protocol::k_DWORD_SIZE);

        OffsetPtr<DataHeader> dh(block, currPos);
        new (dh.get()) DataHeader();

        unsigned int optionsLen = bsl::strlen(messages[i].d_options_p);
        dh->setOptionsWords(optionsLen / bmqp::Protocol::k_WORD_SIZE);
        currPos += sizeof(DataHeader);

        char* destination = reinterpret_cast<char*>(block.base() + currPos);
        bsl::memcpy(destination, messages[i].d_options_p, optionsLen);
        currPos += optionsLen;
        destination += optionsLen;

        unsigned int appDataLen = bsl::strlen(messages[i].d_appData_p);
        int          appDataPad = 0;
        bmqp::ProtocolUtil::calcNumDwordsAndPadding(&appDataPad,
                                                    appDataLen + optionsLen +
                                                        dhSize);

        bsl::memcpy(destination, messages[i].d_appData_p, appDataLen);
        currPos += appDataLen;
        destination += appDataLen;
        bmqp::ProtocolUtil::appendPaddingDwordRaw(destination, appDataPad);
        currPos += appDataPad;

        unsigned int messageOffset = dh->headerWords() +
                                     ((appDataLen + appDataPad + optionsLen) /
                                      bmqp::Protocol::k_WORD_SIZE);
        dh->setMessageWords(messageOffset);
    }

    *fileHeader = *fh;

    return p;
}

void outputGuidString(std::ostream&            ostream,
                      const bmqt::MessageGUID& messageGUID,
                      const bool               addNewLine)
{
    ostream << messageGUID;
    if (addNewLine)
        ostream << bsl::endl;
}

}  // close TestUtils namespace

}  // close package namespace

}  // close enterprise namespace
