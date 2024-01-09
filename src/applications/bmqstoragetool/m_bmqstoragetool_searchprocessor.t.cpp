// Copyright 2023 Bloomberg Finance L.P.
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
#include <m_bmqstoragetool_searchprocessor.h>

// BMQ
#include <bmqt_messageguid.h>

// MQB
#include <mqbs_filestoreprotocol.h>
#include <mqbs_mappedfiledescriptor.h>
#include <mqbs_memoryblock.h>
#include <mqbs_offsetptr.h>
#include <mqbu_messageguidutil.h>

// BDE
#include <bsl_iostream.h>
#include <bsl_limits.h>
#include <bsl_list.h>
#include <bsl_utility.h>
#include <bslma_default.h>
#include <bsls_alignedbuffer.h>

// TEST DRIVER
#include <mwctst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace m_bmqstoragetool;
using namespace bsl;
using namespace mqbs;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------

namespace {

typedef bsls::AlignedBuffer<FileStoreProtocol::k_JOURNAL_RECORD_SIZE>
    RecordBufferType;

typedef bsl::pair<RecordType::Enum, RecordBufferType> NodeType;

typedef bsl::list<NodeType> RecordsListType;

void addJournalRecords(MemoryBlock*         block,
                       FileHeader*          fileHeader,
                       bsls::Types::Uint64* lastRecordPos,
                       bsls::Types::Uint64* lastSyncPtPos,
                       RecordsListType*     records,
                       unsigned int         numRecords)
{
    bsls::Types::Uint64 currPos = 0;

    OffsetPtr<FileHeader> fh(*block, currPos);
    new (fh.get()) FileHeader();
    *fileHeader = *fh;
    currPos += sizeof(FileHeader);

    OffsetPtr<JournalFileHeader> jfh(*block, currPos);
    new (jfh.get()) JournalFileHeader();  // Default values are ok
    currPos += sizeof(JournalFileHeader);

    for (unsigned int i = 1; i <= numRecords; ++i) {
        *lastRecordPos = currPos;

        unsigned int remainder = i % 5;
        if (0 == remainder) {
            bmqt::MessageGUID g;
            mqbu::MessageGUIDUtil::generateGUID(&g);
            OffsetPtr<MessageRecord> rec(*block, currPos);
            new (rec.get()) MessageRecord();
            rec->header()
                .setPrimaryLeaseId(100)
                .setSequenceNumber(i)
                .setTimestamp(i);
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
            OffsetPtr<ConfirmRecord> rec(*block, currPos);
            new (rec.get()) ConfirmRecord();
            rec->header()
                .setPrimaryLeaseId(100)
                .setSequenceNumber(i)
                .setTimestamp(i);
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
            OffsetPtr<DeletionRecord> rec(*block, currPos);
            new (rec.get()) DeletionRecord();
            rec->header()
                .setPrimaryLeaseId(100)
                .setSequenceNumber(i)
                .setTimestamp(i);
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
            OffsetPtr<QueueOpRecord> rec(*block, currPos);
            new (rec.get()) QueueOpRecord();
            rec->header()
                .setPrimaryLeaseId(100)
                .setSequenceNumber(i)
                .setTimestamp(i);
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
            // JournalOpRec.SyncPoint
            *lastSyncPtPos = currPos;
            OffsetPtr<JournalOpRecord> rec(*block, currPos);
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
                .setTimestamp(i);
            RecordBufferType buf;
            bsl::memcpy(buf.buffer(),
                        rec.get(),
                        FileStoreProtocol::k_JOURNAL_RECORD_SIZE);
            records->push_back(bsl::make_pair(RecordType::e_JOURNAL_OP, buf));
        }

        currPos += FileStoreProtocol::k_JOURNAL_RECORD_SIZE;
    }
}

bsl::vector<bmqt::MessageGUID>
addJournalRecordsWithOutstandingAndConfirmedMessages(
    MemoryBlock*         block,
    FileHeader*          fileHeader,
    bsls::Types::Uint64* lastRecordPos,
    bsls::Types::Uint64* lastSyncPtPos,
    RecordsListType*     records,
    unsigned int         numRecords,
    bool                 expectOutstandingResult)
{
    bsls::Types::Uint64 currPos = 0;

    OffsetPtr<FileHeader> fh(*block, currPos);
    new (fh.get()) FileHeader();
    *fileHeader = *fh;
    currPos += sizeof(FileHeader);

    OffsetPtr<JournalFileHeader> jfh(*block, currPos);
    new (jfh.get()) JournalFileHeader();  // Default values are ok
    currPos += sizeof(JournalFileHeader);

    bool                           outstandingFlag = false;
    bmqt::MessageGUID              lastMessageGUID;
    bsl::vector<bmqt::MessageGUID> expectedGUIDs;

    for (unsigned int i = 1; i <= numRecords; ++i) {
        *lastRecordPos = currPos;

        unsigned int remainder = i % 3;
        if (1 == remainder) {
            // bmqt::MessageGUID g;
            mqbu::MessageGUIDUtil::generateGUID(&lastMessageGUID);
            OffsetPtr<MessageRecord> rec(*block, currPos);
            new (rec.get()) MessageRecord();
            rec->header().setPrimaryLeaseId(100).setSequenceNumber(i);
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
            OffsetPtr<ConfirmRecord> rec(*block, currPos);
            new (rec.get()) ConfirmRecord();
            rec->header().setPrimaryLeaseId(100).setSequenceNumber(i);
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
                    expectedGUIDs.push_back(lastMessageGUID);
            }
            else {
                g = lastMessageGUID;
                if (!expectOutstandingResult)
                    expectedGUIDs.push_back(lastMessageGUID);
            }
            outstandingFlag = !outstandingFlag;
            OffsetPtr<DeletionRecord> rec(*block, currPos);
            new (rec.get()) DeletionRecord();
            rec->header().setPrimaryLeaseId(100).setSequenceNumber(i);
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

        currPos += FileStoreProtocol::k_JOURNAL_RECORD_SIZE;
    }

    return expectedGUIDs;
}

// Generate sequence of MessageRecord, ConfirmRecord and DeleteRecord records.
// MessageRecord and ConfirmRecord records have the same GUID.
// DeleteRecord records even records have the same GUID as MessageRecord, odd
// ones - not the same.
bsl::vector<bmqt::MessageGUID> addJournalRecordsWithPartiallyConfirmedMessages(
    MemoryBlock*         block,
    FileHeader*          fileHeader,
    bsls::Types::Uint64* lastRecordPos,
    bsls::Types::Uint64* lastSyncPtPos,
    RecordsListType*     records,
    unsigned int         numRecords)
{
    bsls::Types::Uint64 currPos = 0;

    OffsetPtr<FileHeader> fh(*block, currPos);
    new (fh.get()) FileHeader();
    *fileHeader = *fh;
    currPos += sizeof(FileHeader);

    OffsetPtr<JournalFileHeader> jfh(*block, currPos);
    new (jfh.get()) JournalFileHeader();  // Default values are ok
    currPos += sizeof(JournalFileHeader);

    bool                           partialyConfirmedFlag = false;
    bmqt::MessageGUID              lastMessageGUID;
    bsl::vector<bmqt::MessageGUID> expectedGUIDs;

    for (unsigned int i = 1; i <= numRecords; ++i) {
        *lastRecordPos = currPos;

        unsigned int remainder = i % 3;
        if (1 == remainder) {
            // bmqt::MessageGUID g;
            mqbu::MessageGUIDUtil::generateGUID(&lastMessageGUID);
            OffsetPtr<MessageRecord> rec(*block, currPos);
            new (rec.get()) MessageRecord();
            rec->header().setPrimaryLeaseId(100).setSequenceNumber(i);
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
            OffsetPtr<ConfirmRecord> rec(*block, currPos);
            new (rec.get()) ConfirmRecord();
            rec->header().setPrimaryLeaseId(100).setSequenceNumber(i);
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
                expectedGUIDs.push_back(lastMessageGUID);
            }
            else {
                g = lastMessageGUID;
            }
            partialyConfirmedFlag = !partialyConfirmedFlag;
            OffsetPtr<DeletionRecord> rec(*block, currPos);
            new (rec.get()) DeletionRecord();
            rec->header().setPrimaryLeaseId(100).setSequenceNumber(i);
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

        currPos += FileStoreProtocol::k_JOURNAL_RECORD_SIZE;
    }

    return expectedGUIDs;
}

// Generate sequence of MessageRecord, ConfirmRecord and DeleteRecord records.
// MessageRecord ConfirmRecord and DeletionRecord records have the same GUID.
// queueKey1 is used for even records, queueKey1 for odd ones.
// Returns GUIDs with queueKey1.
bsl::vector<bmqt::MessageGUID>
addJournalRecordsWithTwoQueueKeys(MemoryBlock*         block,
                                  FileHeader*          fileHeader,
                                  bsls::Types::Uint64* lastRecordPos,
                                  bsls::Types::Uint64* lastSyncPtPos,
                                  RecordsListType*     records,
                                  unsigned int         numRecords,
                                  const char*          queueKey1,
                                  const char*          queueKey2)
{
    bsls::Types::Uint64 currPos = 0;

    OffsetPtr<FileHeader> fh(*block, currPos);
    new (fh.get()) FileHeader();
    *fileHeader = *fh;
    currPos += sizeof(FileHeader);

    OffsetPtr<JournalFileHeader> jfh(*block, currPos);
    new (jfh.get()) JournalFileHeader();  // Default values are ok
    currPos += sizeof(JournalFileHeader);

    bool                           partialyConfirmedFlag = false;
    bmqt::MessageGUID              lastMessageGUID;
    bsl::vector<bmqt::MessageGUID> expectedGUIDs;

    for (unsigned int i = 1; i <= numRecords; ++i) {
        *lastRecordPos = currPos;

        const char* queueKey = (i % 2 != 0) ? queueKey1 : queueKey2;

        unsigned int remainder = i % 3;
        if (1 == remainder) {
            mqbu::MessageGUIDUtil::generateGUID(&lastMessageGUID);
            if (queueKey == queueKey1) {
                expectedGUIDs.push_back(lastMessageGUID);
            }
            OffsetPtr<MessageRecord> rec(*block, currPos);
            new (rec.get()) MessageRecord();
            rec->header().setPrimaryLeaseId(100).setSequenceNumber(i);
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
            OffsetPtr<ConfirmRecord> rec(*block, currPos);
            new (rec.get()) ConfirmRecord();
            rec->header().setPrimaryLeaseId(100).setSequenceNumber(i);
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
            OffsetPtr<DeletionRecord> rec(*block, currPos);
            new (rec.get()) DeletionRecord();
            rec->header().setPrimaryLeaseId(100).setSequenceNumber(i);
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

        currPos += FileStoreProtocol::k_JOURNAL_RECORD_SIZE;
    }

    return expectedGUIDs;
}

bsls::Types::Uint64 memoryBufferSize(size_t numRecords)
{
    return sizeof(FileHeader) + sizeof(JournalFileHeader) +
           numRecords * FileStoreProtocol::k_JOURNAL_RECORD_SIZE;
}

void outputGuidString(bsl::ostream&            ostream,
                      const bmqt::MessageGUID& messageGUID,
                      const bool               addNewLine = true)
{
    char buf[bmqt::MessageGUID::e_SIZE_HEX];
    messageGUID.toHex(buf);
    ostream.write(buf, bmqt::MessageGUID::e_SIZE_HEX);
    if (addNewLine)
        ostream << bsl::endl;
}

JournalFileIterator& createJournalFileIterator(unsigned int     numRecords,
                                               MemoryBlock&     block,
                                               RecordsListType* records)
{
    bsls::Types::Uint64 totalSize = memoryBufferSize(numRecords);

    // char*       p = static_cast<char*>(s_allocator_p->allocate(totalSize));
    // MemoryBlock block(p, totalSize);
    FileHeader          fileHeader;
    bsls::Types::Uint64 lastRecordPos = 0;
    bsls::Types::Uint64 lastSyncPtPos = 0;

    // RecordsListType records(s_allocator_p);

    addJournalRecords(&block,
                      &fileHeader,
                      &lastRecordPos,
                      &lastSyncPtPos,
                      records,
                      numRecords);

    // Create JournalFileIterator
    MappedFileDescriptor mfd;
    mfd.setFd(-1);  // invalid fd will suffice.
    mfd.setBlock(block);
    mfd.setFileSize(totalSize);
    JournalFileIterator it(&mfd, fileHeader, false);
    return it;
}

bsl::vector<bmqt::MessageGUID>
addJournalRecordsWithConfirmedMessagesWithDifferentOrder(
    MemoryBlock*               block,
    FileHeader*                fileHeader,
    bsls::Types::Uint64*       lastRecordPos,
    bsls::Types::Uint64*       lastSyncPtPos,
    RecordsListType*           records,
    unsigned int               numRecords,
    bsl::vector<unsigned int>& messageOffsets)
{
    bsls::Types::Uint64 currPos = 0;

    OffsetPtr<FileHeader> fh(*block, currPos);
    new (fh.get()) FileHeader();
    *fileHeader = *fh;
    currPos += sizeof(FileHeader);

    OffsetPtr<JournalFileHeader> jfh(*block, currPos);
    new (jfh.get()) JournalFileHeader();  // Default values are ok
    currPos += sizeof(JournalFileHeader);

    // bmqt::MessageGUID              lastMessageGUID;
    bsl::vector<bmqt::MessageGUID> expectedGUIDs;

    ASSERT(numRecords == messageOffsets.size());
    ASSERT(numRecords >= 3);

    // Create messages
    for (unsigned int i = 0; i < numRecords; ++i) {
        *lastRecordPos = currPos;

        bmqt::MessageGUID g;
        mqbu::MessageGUIDUtil::generateGUID(&g);
        expectedGUIDs.push_back(g);
        OffsetPtr<MessageRecord> rec(*block, currPos);
        new (rec.get()) MessageRecord();
        rec->header().setPrimaryLeaseId(100).setSequenceNumber(i + 1);
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

        currPos += FileStoreProtocol::k_JOURNAL_RECORD_SIZE;
    }

    // Create delete messages, and replace GUIDS for 2nd and 3rd message
    for (unsigned int i = 0; i < numRecords; ++i) {
        *lastRecordPos = currPos;

        // DelRec
        bmqt::MessageGUID g;
        // Change GUIDs order for 2nd and 3rd deletion records
        if (i == 1) {
            g = expectedGUIDs.at(2);
        }
        else if (i == 2) {
            g = expectedGUIDs.at(1);
        }
        else {
            g = expectedGUIDs.at(i);
        }

        OffsetPtr<DeletionRecord> rec(*block, currPos);
        new (rec.get()) DeletionRecord();
        rec->header().setPrimaryLeaseId(100).setSequenceNumber(i + 1);
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

        currPos += FileStoreProtocol::k_JOURNAL_RECORD_SIZE;
    }

    return expectedGUIDs;
}

struct DataMessage {
    int         d_line;
    const char* d_appData_p;
    const char* d_options_p;
};

static char* addDataRecords(bslma::Allocator*          ta,
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

}  // close unnamed namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_breathingTest()
// ------------------------------------------------------------------------
// BREATHING TEST
//
// Concerns:
//   Exercise the basic functionality of the tool - output all message GUIDs
//   found in journal file.
//
// Testing:
//   Basic functionality
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("BREATHING TEST");

    // Simulate journal file
    unsigned int numRecords = 15;

    bsls::Types::Uint64 totalSize =
        sizeof(FileHeader) + sizeof(JournalFileHeader) +
        numRecords * FileStoreProtocol::k_JOURNAL_RECORD_SIZE;

    char*       p = static_cast<char*>(s_allocator_p->allocate(totalSize));
    MemoryBlock block(p, totalSize);
    FileHeader  fileHeader;
    bsls::Types::Uint64 lastRecordPos = 0;
    bsls::Types::Uint64 lastSyncPtPos = 0;

    RecordsListType records(s_allocator_p);

    addJournalRecords(&block,
                      &fileHeader,
                      &lastRecordPos,
                      &lastSyncPtPos,
                      &records,
                      numRecords);

    // Create JournalFileIterator
    MappedFileDescriptor mfd;
    mfd.setFd(-1);  // invalid fd will suffice.
    mfd.setBlock(block);
    mfd.setFileSize(totalSize);
    JournalFileIterator it(&mfd, fileHeader, false);

    // char*       p =
    // static_cast<char*>(s_allocator_p->allocate(memoryBufferSize(numRecords)));
    // MemoryBlock block(p, memoryBufferSize(numRecords));
    // RecordsListType records(s_allocator_p);
    // JournalFileIterator it = createJournalFileIterator(numRecords, block,
    // &records);
    CommandLineArguments        arguments;
    bsl::unique_ptr<Parameters> params =
        bsl::make_unique<Parameters>(arguments, s_allocator_p);
    params->journalFile()->setIterator(&it);

    auto searchProcessor = SearchProcessor(bsl::move(params), s_allocator_p);

    bsl::ostringstream resultStream(s_allocator_p);
    searchProcessor.process(resultStream);

    // Prepare expected output with list of message GUIDs in Journal file
    bsl::ostringstream                  expectedStream(s_allocator_p);
    bsl::list<NodeType>::const_iterator recordIter         = records.begin();
    bsl::size_t                         foundMessagesCount = 0;
    while (recordIter++ != records.end()) {
        RecordType::Enum rtype = recordIter->first;
        if (rtype == RecordType::e_MESSAGE) {
            const MessageRecord& msg = *reinterpret_cast<const MessageRecord*>(
                recordIter->second.buffer());
            char buf[bmqt::MessageGUID::e_SIZE_HEX];
            msg.messageGUID().toHex(buf);
            expectedStream.write(buf, bmqt::MessageGUID::e_SIZE_HEX);
            expectedStream << bsl::endl;
            foundMessagesCount++;
        }
    }
    expectedStream << foundMessagesCount << " message GUID(s) found."
                   << bsl::endl;
    float outstandingRatio = float(foundMessagesCount) / foundMessagesCount *
                             100.0;
    expectedStream << "Outstanding ratio: " << outstandingRatio << "% ("
                   << foundMessagesCount << "/" << foundMessagesCount << ")"
                   << bsl::endl;

    ASSERT_EQ(resultStream.str(), expectedStream.str());

    s_allocator_p->deallocate(p);

    // bsl::string
    // journalFile("/home/aivanov71/projects/pr/blazingmq/build/blazingmq/src/applications/bmqbrkr/localBMQ/storage/local/bmq_0.20231121_091839.bmq_journal",
    // s_allocator_p); auto sp = SearchProcessor(journalFile, s_allocator_p);
    // sp.process(bsl::cout);
    // ASSERT(sp.getJournalFileIter().isValid());
}

static void test2_searchGuidTest()
// ------------------------------------------------------------------------
// SEARCH GUID TEST
//
// Concerns:
//   Search messages by GUIDs in journal file and output GUIDs.
//
// Testing:
//   SearchProcessor::process()
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("SEARCH GUID");

    // Simulate journal file
    unsigned int numRecords = 15;

    bsls::Types::Uint64 totalSize =
        sizeof(FileHeader) + sizeof(JournalFileHeader) +
        numRecords * FileStoreProtocol::k_JOURNAL_RECORD_SIZE;

    char*       p = static_cast<char*>(s_allocator_p->allocate(totalSize));
    MemoryBlock block(p, totalSize);
    FileHeader  fileHeader;
    bsls::Types::Uint64 lastRecordPos = 0;
    bsls::Types::Uint64 lastSyncPtPos = 0;

    RecordsListType records(s_allocator_p);

    addJournalRecords(&block,
                      &fileHeader,
                      &lastRecordPos,
                      &lastSyncPtPos,
                      &records,
                      numRecords);

    // Create JournalFileIterator
    MappedFileDescriptor mfd;
    mfd.setFd(-1);  // invalid fd will suffice.
    mfd.setBlock(block);
    mfd.setFileSize(totalSize);
    JournalFileIterator it(&mfd, fileHeader, false);

    // Get list of message GUIDs for searching
    CommandLineArguments      arguments;
    bsl::vector<bsl::string>& searchGuids = arguments.d_guid;

    bsl::list<NodeType>::const_iterator recordIter = records.begin();
    bsl::size_t                         msgCnt     = 0;
    while (recordIter++ != records.end()) {
        RecordType::Enum rtype = recordIter->first;
        if (rtype == RecordType::e_MESSAGE) {
            if (msgCnt++ % 2 != 0)
                continue;  // Skip odd messages for test purposes
            const MessageRecord& msg = *reinterpret_cast<const MessageRecord*>(
                recordIter->second.buffer());
            char buf[bmqt::MessageGUID::e_SIZE_HEX];
            msg.messageGUID().toHex(buf);
            bsl::string guid(buf, s_allocator_p);
            arguments.d_guid.push_back(guid);
        }
    }
    bsl::unique_ptr<Parameters> params =
        bsl::make_unique<Parameters>(arguments, s_allocator_p);
    params->journalFile()->setIterator(&it);

    auto searchProcessor = SearchProcessor(bsl::move(params), s_allocator_p);

    bsl::ostringstream resultStream(s_allocator_p);
    searchProcessor.process(resultStream);

    // Prepare expected output
    bsl::ostringstream expectedStream(s_allocator_p);
    for (auto& guid : searchGuids) {
        expectedStream << guid << bsl::endl;
    }
    expectedStream << searchGuids.size() << " message GUID(s) found."
                   << bsl::endl;

    ASSERT_EQ(resultStream.str(), expectedStream.str());

    s_allocator_p->deallocate(p);
}

static void test3_searchNonExistingGuidTest()
// ------------------------------------------------------------------------
// SEARCH NON EXISTING GUID TEST
//
// Concerns:
//   Search messages by non existing GUIDs in journal file and output result.
//
// Testing:
//   SearchProcessor::process()
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("SEARCH NON EXISTING GUID");

    // Simulate journal file
    unsigned int numRecords = 15;

    bsls::Types::Uint64 totalSize =
        sizeof(FileHeader) + sizeof(JournalFileHeader) +
        numRecords * FileStoreProtocol::k_JOURNAL_RECORD_SIZE;

    char*       p = static_cast<char*>(s_allocator_p->allocate(totalSize));
    MemoryBlock block(p, totalSize);
    FileHeader  fileHeader;
    bsls::Types::Uint64 lastRecordPos = 0;
    bsls::Types::Uint64 lastSyncPtPos = 0;

    RecordsListType records(s_allocator_p);

    addJournalRecords(&block,
                      &fileHeader,
                      &lastRecordPos,
                      &lastSyncPtPos,
                      &records,
                      numRecords);

    // Create JournalFileIterator
    MappedFileDescriptor mfd;
    mfd.setFd(-1);  // invalid fd will suffice.
    mfd.setBlock(block);
    mfd.setFileSize(totalSize);
    JournalFileIterator it(&mfd, fileHeader, false);

    // Get list of message GUIDs for searching
    CommandLineArguments      arguments;
    bsl::vector<bsl::string>& searchGuids = arguments.d_guid;

    bmqt::MessageGUID guid;
    mqbu::MessageGUIDUtil::generateGUID(&guid);
    char buf[bmqt::MessageGUID::e_SIZE_HEX];
    guid.toHex(buf);
    bsl::string guidStr(buf, s_allocator_p);
    searchGuids.push_back(guidStr);
    mqbu::MessageGUIDUtil::generateGUID(&guid);
    guid.toHex(buf);
    guidStr = buf;
    searchGuids.push_back(guidStr);

    bsl::unique_ptr<Parameters> params =
        bsl::make_unique<Parameters>(arguments, s_allocator_p);
    params->journalFile()->setIterator(&it);

    auto searchProcessor = SearchProcessor(bsl::move(params), s_allocator_p);

    bsl::ostringstream resultStream(s_allocator_p);
    searchProcessor.process(resultStream);

    // Prepare expected output
    bsl::ostringstream expectedStream(s_allocator_p);
    expectedStream << "No message GUID found." << bsl::endl;

    expectedStream << bsl::endl
                   << "The following 2 GUID(s) not found:" << bsl::endl;
    //  TODO: fix sporadic fail due to order
    expectedStream << searchGuids[1] << bsl::endl
                   << searchGuids[0] << bsl::endl;
    ;

    ASSERT_EQ(resultStream.str(), expectedStream.str());

    s_allocator_p->deallocate(p);
}

static void test4_searchExistingAndNonExistingGuidTest()
// ------------------------------------------------------------------------
// SEARCH EXISTING AND NON EXISTING GUID TEST
//
// Concerns:
//   Search messages by existing and non existing GUIDs in journal file and
//   output result.
//
// Testing:
//   SearchProcessor::process()
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("SEARCH EXISTING AND NON EXISTING GUID");

    // Simulate journal file
    unsigned int numRecords = 15;

    bsls::Types::Uint64 totalSize =
        sizeof(FileHeader) + sizeof(JournalFileHeader) +
        numRecords * FileStoreProtocol::k_JOURNAL_RECORD_SIZE;

    char*       p = static_cast<char*>(s_allocator_p->allocate(totalSize));
    MemoryBlock block(p, totalSize);
    FileHeader  fileHeader;
    bsls::Types::Uint64 lastRecordPos = 0;
    bsls::Types::Uint64 lastSyncPtPos = 0;

    RecordsListType records(s_allocator_p);

    addJournalRecords(&block,
                      &fileHeader,
                      &lastRecordPos,
                      &lastSyncPtPos,
                      &records,
                      numRecords);

    // Create JournalFileIterator
    MappedFileDescriptor mfd;
    mfd.setFd(-1);  // invalid fd will suffice.
    mfd.setBlock(block);
    mfd.setFileSize(totalSize);
    JournalFileIterator it(&mfd, fileHeader, false);

    // Get list of message GUIDs for searching
    CommandLineArguments      arguments;
    bsl::vector<bsl::string>& searchGuids = arguments.d_guid;

    // Get two existing message GUIDs
    bsl::list<NodeType>::const_iterator recordIter = records.begin();
    bsl::size_t                         msgCnt     = 0;
    while (recordIter++ != records.end()) {
        RecordType::Enum rtype = recordIter->first;
        if (rtype == RecordType::e_MESSAGE) {
            if (msgCnt++ == 2)
                break;  // Take two GUIDs
            const MessageRecord& msg = *reinterpret_cast<const MessageRecord*>(
                recordIter->second.buffer());
            char buf[bmqt::MessageGUID::e_SIZE_HEX];
            msg.messageGUID().toHex(buf);
            bsl::string guid(buf, s_allocator_p);
            searchGuids.push_back(guid);
        }
    }

    // Get two non existing message GUIDs
    bmqt::MessageGUID guid;
    mqbu::MessageGUIDUtil::generateGUID(&guid);
    char buf[bmqt::MessageGUID::e_SIZE_HEX];
    guid.toHex(buf);
    bsl::string guidStr(buf, s_allocator_p);
    searchGuids.push_back(guidStr);
    mqbu::MessageGUIDUtil::generateGUID(&guid);
    guid.toHex(buf);
    guidStr = buf;
    searchGuids.push_back(guidStr);

    bsl::unique_ptr<Parameters> params =
        bsl::make_unique<Parameters>(arguments, s_allocator_p);
    params->journalFile()->setIterator(&it);

    auto searchProcessor = SearchProcessor(bsl::move(params), s_allocator_p);

    bsl::ostringstream resultStream(s_allocator_p);
    searchProcessor.process(resultStream);

    // Prepare expected output
    bsl::ostringstream expectedStream(s_allocator_p);
    expectedStream << searchGuids[0] << bsl::endl
                   << searchGuids[1] << bsl::endl;
    expectedStream << "2 message GUID(s) found." << bsl::endl;
    expectedStream << bsl::endl
                   << "The following 2 GUID(s) not found:" << bsl::endl;
    // TODO: fix sporadic search due to order in map
    expectedStream << searchGuids[2] << bsl::endl
                   << searchGuids[3] << bsl::endl;

    ASSERT_EQ(resultStream.str(), expectedStream.str());

    s_allocator_p->deallocate(p);
}

static void test5_searchOutstandingMessagesTest()
// ------------------------------------------------------------------------
// SEARCH OUTSTANDING MESSAGES TEST
//
// Concerns:
//   Search outstanding (not deleted) messages and output GUIDs.
//
// Testing:
//   SearchProcessor::process()
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("SEARCH OUTSTANDING MESSAGES TEST");

    // Simulate journal file
    unsigned int numRecords = 15;

    bsls::Types::Uint64 totalSize =
        sizeof(FileHeader) + sizeof(JournalFileHeader) +
        numRecords * FileStoreProtocol::k_JOURNAL_RECORD_SIZE;

    char*       p = static_cast<char*>(s_allocator_p->allocate(totalSize));
    MemoryBlock block(p, totalSize);
    FileHeader  fileHeader;
    bsls::Types::Uint64 lastRecordPos = 0;
    bsls::Types::Uint64 lastSyncPtPos = 0;

    RecordsListType records(s_allocator_p);

    bsl::vector<bmqt::MessageGUID> outstandingGUIDS =
        addJournalRecordsWithOutstandingAndConfirmedMessages(&block,
                                                             &fileHeader,
                                                             &lastRecordPos,
                                                             &lastSyncPtPos,
                                                             &records,
                                                             numRecords,
                                                             true);

    // Create JournalFileIterator
    MappedFileDescriptor mfd;
    mfd.setFd(-1);  // invalid fd will suffice.
    mfd.setBlock(block);
    mfd.setFileSize(totalSize);
    JournalFileIterator it(&mfd, fileHeader, false);

    // Configure parameters to search outstanding messages
    CommandLineArguments arguments;
    arguments.d_outstanding = true;
    bsl::unique_ptr<Parameters> params =
        bsl::make_unique<Parameters>(arguments, s_allocator_p);
    params->journalFile()->setIterator(&it);

    auto searchProcessor = SearchProcessor(bsl::move(params), s_allocator_p);

    bsl::ostringstream resultStream(s_allocator_p);
    searchProcessor.process(resultStream);

    // Prepare expected output
    bsl::ostringstream expectedStream(s_allocator_p);
    for (const auto& guid : outstandingGUIDS) {
        outputGuidString(expectedStream, guid);
    }
    expectedStream << outstandingGUIDS.size() << " message GUID(s) found."
                   << bsl::endl;
    float messageCount     = numRecords / 3.0;
    float outstandingRatio = float(outstandingGUIDS.size()) / messageCount *
                             100.0;
    expectedStream << "Outstanding ratio: " << outstandingRatio << "% ("
                   << outstandingGUIDS.size() << "/" << messageCount << ")"
                   << bsl::endl;

    ASSERT_EQ(resultStream.str(), expectedStream.str());

    s_allocator_p->deallocate(p);
}

static void test6_searchConfirmedMessagesTest()
// ------------------------------------------------------------------------
// SEARCH CONFIRMED MESSAGES TEST
//
// Concerns:
//   Search confirmed (deleted) messages  in journal file and output GUIDs.
//
// Testing:
//   SearchProcessor::process()
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("SEARCH CONFIRMED MESSAGES TEST");

    // Simulate journal file
    unsigned int numRecords = 15;

    bsls::Types::Uint64 totalSize =
        sizeof(FileHeader) + sizeof(JournalFileHeader) +
        numRecords * FileStoreProtocol::k_JOURNAL_RECORD_SIZE;

    char*       p = static_cast<char*>(s_allocator_p->allocate(totalSize));
    MemoryBlock block(p, totalSize);
    FileHeader  fileHeader;
    bsls::Types::Uint64 lastRecordPos = 0;
    bsls::Types::Uint64 lastSyncPtPos = 0;

    RecordsListType records(s_allocator_p);

    bsl::vector<bmqt::MessageGUID> confirmedGUIDS =
        addJournalRecordsWithOutstandingAndConfirmedMessages(&block,
                                                             &fileHeader,
                                                             &lastRecordPos,
                                                             &lastSyncPtPos,
                                                             &records,
                                                             numRecords,
                                                             false);

    // Create JournalFileIterator
    MappedFileDescriptor mfd;
    mfd.setFd(-1);  // invalid fd will suffice.
    mfd.setBlock(block);
    mfd.setFileSize(totalSize);
    JournalFileIterator it(&mfd, fileHeader, false);

    // Configure parameters to search confirmed messages
    CommandLineArguments arguments;
    arguments.d_confirmed = true;
    bsl::unique_ptr<Parameters> params =
        bsl::make_unique<Parameters>(arguments, s_allocator_p);
    params->journalFile()->setIterator(&it);

    auto searchProcessor = SearchProcessor(bsl::move(params), s_allocator_p);

    bsl::ostringstream resultStream(s_allocator_p);
    searchProcessor.process(resultStream);

    // Prepare expected output
    bsl::ostringstream expectedStream(s_allocator_p);
    for (const auto& guid : confirmedGUIDS) {
        outputGuidString(expectedStream, guid);
    }
    expectedStream << confirmedGUIDS.size() << " message GUID(s) found."
                   << bsl::endl;
    float messageCount     = numRecords / 3.0;
    float outstandingRatio = float(messageCount - confirmedGUIDS.size()) /
                             messageCount * 100.0;
    expectedStream << "Outstanding ratio: " << outstandingRatio << "% ("
                   << (messageCount - confirmedGUIDS.size()) << "/"
                   << messageCount << ")" << bsl::endl;

    ASSERT_EQ(resultStream.str(), expectedStream.str());

    s_allocator_p->deallocate(p);
}

static void test7_searchPartiallyConfirmedMessagesTest()
// ------------------------------------------------------------------------
// SEARCH PARTIALLY CONFIRMED MESSAGES TEST
//
// Concerns:
//   Search partially confirmed (at least one confirm) messages in journal
//   file and output GUIDs.
//
// Testing:
//   SearchProcessor::process()
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName(
        "SEARCH PARTIALLY CONFIRMED MESSAGES TEST");

    // Simulate journal file
    unsigned int numRecords = 15;

    bsls::Types::Uint64 totalSize =
        sizeof(FileHeader) + sizeof(JournalFileHeader) +
        numRecords * FileStoreProtocol::k_JOURNAL_RECORD_SIZE;

    char*       p = static_cast<char*>(s_allocator_p->allocate(totalSize));
    MemoryBlock block(p, totalSize);
    FileHeader  fileHeader;
    bsls::Types::Uint64 lastRecordPos = 0;
    bsls::Types::Uint64 lastSyncPtPos = 0;

    RecordsListType records(s_allocator_p);

    bsl::vector<bmqt::MessageGUID> partiallyConfirmedGUIDS =
        addJournalRecordsWithPartiallyConfirmedMessages(&block,
                                                        &fileHeader,
                                                        &lastRecordPos,
                                                        &lastSyncPtPos,
                                                        &records,
                                                        numRecords);

    // Create JournalFileIterator
    MappedFileDescriptor mfd;
    mfd.setFd(-1);  // invalid fd will suffice.
    mfd.setBlock(block);
    mfd.setFileSize(totalSize);
    JournalFileIterator it(&mfd, fileHeader, false);

    // Configure parameters to search outstanding messages
    CommandLineArguments arguments;
    arguments.d_partiallyConfirmed = true;
    bsl::unique_ptr<Parameters> params =
        bsl::make_unique<Parameters>(arguments, s_allocator_p);
    params->journalFile()->setIterator(&it);

    auto searchProcessor = SearchProcessor(bsl::move(params), s_allocator_p);

    bsl::ostringstream resultStream(s_allocator_p);
    searchProcessor.process(resultStream);

    // Prepare expected output
    bsl::ostringstream expectedStream(s_allocator_p);
    for (const auto& guid : partiallyConfirmedGUIDS) {
        outputGuidString(expectedStream, guid);
    }
    expectedStream << partiallyConfirmedGUIDS.size()
                   << " message GUID(s) found." << bsl::endl;
    float messageCount     = numRecords / 3.0;
    float outstandingRatio = float(partiallyConfirmedGUIDS.size()) /
                             messageCount * 100.0;
    expectedStream << "Outstanding ratio: " << outstandingRatio << "% ("
                   << partiallyConfirmedGUIDS.size() << "/" << messageCount
                   << ")" << bsl::endl;

    // TODO: fix ordering issue (sporadic fail)
    ASSERT_EQ(resultStream.str(), expectedStream.str());

    s_allocator_p->deallocate(p);
}

static void test8_searchMessagesByQueueKeyTest()
// ------------------------------------------------------------------------
// SEARCH MESSAGES BY QUEUE KEY TEST
//
// Concerns:
//   Search messages by queue key in journal
//   file and output GUIDs.
//
// Testing:
//   SearchProcessor::process()
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("SEARCH MESSAGES BY QUEUE KEY TEST");

    // Simulate journal file
    unsigned int numRecords = 15;

    bsls::Types::Uint64 totalSize =
        sizeof(FileHeader) + sizeof(JournalFileHeader) +
        numRecords * FileStoreProtocol::k_JOURNAL_RECORD_SIZE;

    char*       p = static_cast<char*>(s_allocator_p->allocate(totalSize));
    MemoryBlock block(p, totalSize);
    FileHeader  fileHeader;
    bsls::Types::Uint64 lastRecordPos = 0;
    bsls::Types::Uint64 lastSyncPtPos = 0;

    RecordsListType records(s_allocator_p);

    const char* queueKey1 = "ABCDE12345";
    const char* queueKey2 = "12345ABCDE";

    bsl::vector<bmqt::MessageGUID> queueKey1GUIDS =
        addJournalRecordsWithTwoQueueKeys(&block,
                                          &fileHeader,
                                          &lastRecordPos,
                                          &lastSyncPtPos,
                                          &records,
                                          numRecords,
                                          queueKey1,
                                          queueKey2);

    // Create JournalFileIterator
    MappedFileDescriptor mfd;
    mfd.setFd(-1);  // invalid fd will suffice.
    mfd.setBlock(block);
    mfd.setFileSize(totalSize);
    JournalFileIterator it(&mfd, fileHeader, false);

    // Configure parameters to search messages by queueKey1
    CommandLineArguments arguments;
    arguments.d_queueKey.push_back(queueKey1);
    bsl::unique_ptr<Parameters> params =
        bsl::make_unique<Parameters>(arguments, s_allocator_p);
    params->journalFile()->setIterator(&it);

    auto searchProcessor = SearchProcessor(bsl::move(params), s_allocator_p);

    bsl::ostringstream resultStream(s_allocator_p);
    searchProcessor.process(resultStream);

    // Prepare expected output
    bsl::ostringstream expectedStream(s_allocator_p);
    for (const auto& guid : queueKey1GUIDS) {
        outputGuidString(expectedStream, guid);
    }
    auto foundMessagesCount = queueKey1GUIDS.size();
    expectedStream << foundMessagesCount << " message GUID(s) found."
                   << bsl::endl;
    float outstandingRatio = float(foundMessagesCount) / foundMessagesCount *
                             100.0;
    expectedStream << "Outstanding ratio: " << outstandingRatio << "% ("
                   << foundMessagesCount << "/" << foundMessagesCount << ")"
                   << bsl::endl;

    // TODO: fix ordering issue (sporadic fail)
    ASSERT_EQ(resultStream.str(), expectedStream.str());

    s_allocator_p->deallocate(p);
}

static void test9_searchMessagesByQueueNameTest()
// ------------------------------------------------------------------------
// SEARCH MESSAGES BY QUEUE NAME TEST
//
// Concerns:
//   Search messages by queue name in journal
//   file and output GUIDs.
//
// Testing:
//   SearchProcessor::process()
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("SEARCH MESSAGES BY QUEUE NAME TEST");

    // Simulate journal file
    unsigned int numRecords = 15;

    bsls::Types::Uint64 totalSize =
        sizeof(FileHeader) + sizeof(JournalFileHeader) +
        numRecords * FileStoreProtocol::k_JOURNAL_RECORD_SIZE;

    char*       p = static_cast<char*>(s_allocator_p->allocate(totalSize));
    MemoryBlock block(p, totalSize);
    FileHeader  fileHeader;
    bsls::Types::Uint64 lastRecordPos = 0;
    bsls::Types::Uint64 lastSyncPtPos = 0;

    RecordsListType records(s_allocator_p);

    const char* queueKey1 = "ABCDE12345";
    const char* queueKey2 = "12345ABCDE";

    bsl::vector<bmqt::MessageGUID> queueKey1GUIDS =
        addJournalRecordsWithTwoQueueKeys(&block,
                                          &fileHeader,
                                          &lastRecordPos,
                                          &lastSyncPtPos,
                                          &records,
                                          numRecords,
                                          queueKey1,
                                          queueKey2);

    // Create JournalFileIterator
    MappedFileDescriptor mfd;
    mfd.setFd(-1);  // invalid fd will suffice.
    mfd.setBlock(block);
    mfd.setFileSize(totalSize);
    JournalFileIterator it(&mfd, fileHeader, false);

    // Configure parameters to search messages by 'queue1' and 'unknown' names
    CommandLineArguments arguments;
    arguments.d_queueName.push_back("queue1");
    arguments.d_queueName.push_back("unknown");
    bsl::unique_ptr<Parameters> params =
        bsl::make_unique<Parameters>(arguments, s_allocator_p);
    params->journalFile()->setIterator(&it);
    // Add uri to key mapping
    bmqp_ctrlmsg::QueueInfo queueInfo;
    queueInfo.uri() = "queue1";
    auto key        = mqbu::StorageKey(mqbu::StorageKey::HexRepresentation(),
                                queueKey1);
    for (int i = 0; i < mqbu::StorageKey::e_KEY_LENGTH_BINARY; i++) {
        queueInfo.key().push_back(key.data()[i]);
    }
    params->queueMap().insert(queueInfo);

    auto searchProcessor = SearchProcessor(bsl::move(params), s_allocator_p);

    bsl::ostringstream resultStream(s_allocator_p);
    searchProcessor.process(resultStream);

    // Prepare expected output
    bsl::ostringstream expectedStream(s_allocator_p);
    expectedStream
        << "Queue name: 'unknown' is not found in Csl file. Skipping..."
        << bsl::endl;
    for (const auto& guid : queueKey1GUIDS) {
        outputGuidString(expectedStream, guid);
    }
    auto foundMessagesCount = queueKey1GUIDS.size();
    expectedStream << foundMessagesCount << " message GUID(s) found."
                   << bsl::endl;
    float outstandingRatio = float(foundMessagesCount) / foundMessagesCount *
                             100.0;
    expectedStream << "Outstanding ratio: " << outstandingRatio << "% ("
                   << foundMessagesCount << "/" << foundMessagesCount << ")"
                   << bsl::endl;

    // TODO: fix ordering issue (sporadic fail)
    ASSERT_EQ(resultStream.str(), expectedStream.str());

    s_allocator_p->deallocate(p);
}

static void test10_searchMessagesByTimestamp()
// ------------------------------------------------------------------------
// SEARCH MESSAGES BY TIMESTAMP TEST
//
// Concerns:
//   Search messages by timestamp in journal file and output GUIDs.
//
// Testing:
//   SearchProcessor::process()
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("SEARCH MESSAGES BY TIMESTAMP TEST");

    // Simulate journal file
    unsigned int numRecords = 50;

    bsls::Types::Uint64 totalSize =
        sizeof(FileHeader) + sizeof(JournalFileHeader) +
        numRecords * FileStoreProtocol::k_JOURNAL_RECORD_SIZE;

    char*       p = static_cast<char*>(s_allocator_p->allocate(totalSize));
    MemoryBlock block(p, totalSize);
    FileHeader  fileHeader;
    bsls::Types::Uint64 lastRecordPos = 0;
    bsls::Types::Uint64 lastSyncPtPos = 0;

    RecordsListType records(s_allocator_p);

    addJournalRecords(&block,
                      &fileHeader,
                      &lastRecordPos,
                      &lastSyncPtPos,
                      &records,
                      numRecords);

    // Create JournalFileIterator
    MappedFileDescriptor mfd;
    mfd.setFd(-1);  // invalid fd will suffice.
    mfd.setBlock(block);
    mfd.setFileSize(totalSize);
    JournalFileIterator it(&mfd, fileHeader, false);

    // Configure parameters to search messages by timestamps
    CommandLineArguments arguments;
    arguments.d_timestampGt = 10;
    arguments.d_timestampLt = 40;
    bsl::unique_ptr<Parameters> params =
        bsl::make_unique<Parameters>(arguments, s_allocator_p);
    params->journalFile()->setIterator(&it);

    // Get GUIDs of messages with matching timestamps and prepare expected
    // output
    bsl::ostringstream expectedStream(s_allocator_p);

    bsl::list<NodeType>::const_iterator recordIter = records.begin();
    bsl::size_t                         msgCnt     = 0;
    while (recordIter++ != records.end()) {
        RecordType::Enum rtype = recordIter->first;
        if (rtype == RecordType::e_MESSAGE) {
            const MessageRecord& msg = *reinterpret_cast<const MessageRecord*>(
                recordIter->second.buffer());
            const bsls::Types::Uint64& ts = msg.header().timestamp();
            if (ts > arguments.d_timestampGt && ts < arguments.d_timestampLt) {
                char buf[bmqt::MessageGUID::e_SIZE_HEX];
                msg.messageGUID().toHex(buf);
                bsl::string guid(buf, s_allocator_p);
                expectedStream.write(buf, bmqt::MessageGUID::e_SIZE_HEX);
                expectedStream << bsl::endl;
                msgCnt++;
            }
        }
    }
    expectedStream << msgCnt << " message GUID(s) found." << bsl::endl;
    expectedStream << "Outstanding ratio: 100% (" << msgCnt << "/" << msgCnt
                   << ")" << bsl::endl;

    auto searchProcessor = SearchProcessor(bsl::move(params), s_allocator_p);

    bsl::ostringstream resultStream(s_allocator_p);
    searchProcessor.process(resultStream);

    ASSERT_EQ(resultStream.str(), expectedStream.str());

    s_allocator_p->deallocate(p);
}

static void test11_printMessagesDetailsTest()
// ------------------------------------------------------------------------
// PRINT MESSAGE DETAILS TEST
//
// Concerns:
//   Search messages in journal file and output message details.
//
// Testing:
//   SearchProcessor::process()
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("SEARCH CONFIRMED MESSAGES TEST");

    // Simulate journal file
    unsigned int numRecords = 15;

    bsls::Types::Uint64 totalSize =
        sizeof(FileHeader) + sizeof(JournalFileHeader) +
        numRecords * FileStoreProtocol::k_JOURNAL_RECORD_SIZE;

    char*       p = static_cast<char*>(s_allocator_p->allocate(totalSize));
    MemoryBlock block(p, totalSize);
    FileHeader  fileHeader;
    bsls::Types::Uint64 lastRecordPos = 0;
    bsls::Types::Uint64 lastSyncPtPos = 0;

    RecordsListType records(s_allocator_p);

    bsl::vector<bmqt::MessageGUID> confirmedGUIDS =
        addJournalRecordsWithOutstandingAndConfirmedMessages(&block,
                                                             &fileHeader,
                                                             &lastRecordPos,
                                                             &lastSyncPtPos,
                                                             &records,
                                                             numRecords,
                                                             false);

    // Create JournalFileIterator
    MappedFileDescriptor mfd;
    mfd.setFd(-1);  // invalid fd will suffice.
    mfd.setBlock(block);
    mfd.setFileSize(totalSize);
    JournalFileIterator it(&mfd, fileHeader, false);

    // Configure parameters to print message details
    CommandLineArguments arguments;
    arguments.d_details = true;
    bsl::unique_ptr<Parameters> params =
        bsl::make_unique<Parameters>(arguments, s_allocator_p);
    params->journalFile()->setIterator(&it);

    auto searchProcessor = SearchProcessor(bsl::move(params), s_allocator_p);

    bsl::ostringstream resultStream(s_allocator_p);
    searchProcessor.process(resultStream);
    bsl::cout << resultStream.str();

    // Prepare expected output
    // bsl::ostringstream expectedStream(s_allocator_p);
    // for (const auto& guid : confirmedGUIDS) {
    //     outputGuidString(expectedStream, guid);
    // }
    // expectedStream << confirmedGUIDS.size() << " message GUID(s) found."
    //                << bsl::endl;
    // float messageCount     = numRecords / 3.0;
    // float outstandingRatio = float(confirmedGUIDS.size()) / messageCount *
    //                          100.0;
    // expectedStream << "Outstanding ratio: " << outstandingRatio << "%"
    //                << bsl::endl;

    // ASSERT_EQ(resultStream.str(), expectedStream.str());

    s_allocator_p->deallocate(p);
}

static void test12_searchMessagesWithPayloadDumpTest()
// ------------------------------------------------------------------------
// SEARCH MESSAGES WITH PAYLOAD DUMP TEST
//
// Concerns:
//   Search confirmed message in journal file and output GUIDs and payload
//   dumps. In case of confirmed messages search, message data (including dump)
//   are output immediately when 'delete' record found. Order of 'delete'
//   records can be different than order of messages. This test simulates
//   different order of 'delete' records and checks that payload dump is output
//   correctly.
//
// Testing:
//   SearchProcessor::process()
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName(
        "SEARCH MESSAGES WITH PAYLOAD DUMP TEST");

    // Simulate data file
    const DataMessage MESSAGES[] = {
        {
            L_,
            "APP_DATA_APP_DATA_APP_DATA_1",
            ""  //"OPTIONS_OPTIONS_"  // Word aligned
        },
        {
            L_,
            "APP_DATA_APP_DATA_APP_DATA_APP_DATA_APP_DATA_2",
            ""  // OPTIONS_OPTIONS_OPTIONS_OPTIONS_"  // Word aligned
        },
        {
            L_,
            "APP_DATA_APP_DATA_APP_DATA_APP_DATA_APP_DATA_APP_DATA_3",
            ""  // OPTIONS_OPTIONS_OPTIONS_OPTIONS_OPTIONS_OPTIONS_OPTIONS_"
        },
        {
            L_,
            "APP_DATA_APP_DATA_APP_DATA_4",
            ""  //"OPTIONS_OPTIONS_"  // Word aligned
        },
    };

    const unsigned int k_NUM_MSGS = sizeof(MESSAGES) / sizeof(*MESSAGES);

    FileHeader                fileHeader;
    MappedFileDescriptor      mfdData;
    bsl::vector<unsigned int> messageOffsets;
    char*                     pd = addDataRecords(s_allocator_p,
                              &mfdData,
                              &fileHeader,
                              MESSAGES,
                              k_NUM_MSGS,
                              messageOffsets);
    ASSERT(pd != 0);
    ASSERT_GT(mfdData.fileSize(), 0ULL);
    // Create data file iterator
    DataFileIterator dataIt(&mfdData, fileHeader);

    // Simulate journal file
    unsigned int numRecords =
        k_NUM_MSGS * 2;  // k_NUM_MSGS messages + k_NUM_MSGS deletion messages

    bsls::Types::Uint64 totalSize =
        sizeof(FileHeader) + sizeof(JournalFileHeader) +
        numRecords * FileStoreProtocol::k_JOURNAL_RECORD_SIZE;

    char*       p = static_cast<char*>(s_allocator_p->allocate(totalSize));
    MemoryBlock block(p, totalSize);
    // FileHeader  fileHeader;
    bsls::Types::Uint64 lastRecordPos = 0;
    bsls::Types::Uint64 lastSyncPtPos = 0;

    RecordsListType records(s_allocator_p);

    bsl::vector<bmqt::MessageGUID> confirmedGUIDS =
        addJournalRecordsWithConfirmedMessagesWithDifferentOrder(
            &block,
            &fileHeader,
            &lastRecordPos,
            &lastSyncPtPos,
            &records,
            k_NUM_MSGS,
            messageOffsets);

    // Create JournalFileIterator
    MappedFileDescriptor mfd;
    mfd.setFd(-1);  // invalid fd will suffice.
    mfd.setBlock(block);
    mfd.setFileSize(totalSize);
    JournalFileIterator it(&mfd, fileHeader, false);

    // Configure parameters to search confirmed messages GUIDs with dumping
    // messages payload.
    CommandLineArguments arguments;
    arguments.d_confirmed   = true;
    arguments.d_dumpPayload = true;
    bsl::unique_ptr<Parameters> params =
        bsl::make_unique<Parameters>(arguments, s_allocator_p);
    params->journalFile()->setIterator(&it);
    params->dataFile()->setIterator(&dataIt);

    // Perform search
    auto searchProcessor = SearchProcessor(bsl::move(params), s_allocator_p);

    bsl::ostringstream resultStream(s_allocator_p);

    searchProcessor.process(resultStream);

    // Prepare expected data
    bsl::string              resultString             = resultStream.str();
    size_t                   startIdx                 = 0;
    bsl::vector<bsl::string> expectedPayloadSubstring = {"DATA_1",
                                                         "DATA_3",
                                                         "DATA_2",
                                                         "DATA_4"};
    // Change GUIDs order for 2nd and 3rd messages as it was done in
    // 'addJournalRecordsWithConfirmedMessagesWithDifferentOrder()'
    bsl::swap(confirmedGUIDS[1], confirmedGUIDS[2]);

    // Check that substrings are present in resultStream in correct order
    for (int i = 0; i < k_NUM_MSGS; i++) {
        // Check GUID
        bmqt::MessageGUID  guid = confirmedGUIDS.at(i);
        bsl::ostringstream ss;
        outputGuidString(ss, guid);
        bsl::string guidStr  = ss.str();
        size_t      foundIdx = resultString.find(guidStr, startIdx);

        ASSERT_D(guidStr, (foundIdx != bsl::string::npos));
        ASSERT_D(guidStr, (foundIdx >= startIdx));

        startIdx = foundIdx + guidStr.length();

        // Check payload dump substring
        auto dumpStr = expectedPayloadSubstring[i];
        foundIdx     = resultString.find(dumpStr, startIdx);

        ASSERT_D(dumpStr, (foundIdx != bsl::string::npos));
        ASSERT_D(guidStr, (foundIdx >= startIdx));
        startIdx = foundIdx + dumpStr.length();
    }

    s_allocator_p->deallocate(p);
    s_allocator_p->deallocate(pd);
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(mwctst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 1: test1_breathingTest(); break;
    case 2: test2_searchGuidTest(); break;
    case 3: test3_searchNonExistingGuidTest(); break;
    case 4: test4_searchExistingAndNonExistingGuidTest(); break;
    case 5: test5_searchOutstandingMessagesTest(); break;
    case 6: test6_searchConfirmedMessagesTest(); break;
    case 7: test7_searchPartiallyConfirmedMessagesTest(); break;
    case 8: test8_searchMessagesByQueueKeyTest(); break;
    case 9: test9_searchMessagesByQueueNameTest(); break;
    case 10: test10_searchMessagesByTimestamp(); break;
    case 11: test11_printMessagesDetailsTest(); break;
    case 12: test12_searchMessagesWithPayloadDumpTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        s_testStatus = -1;
    } break;
    }

    // TODO: consider memory usage
    // TEST_EPILOG(mwctst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
    TEST_EPILOG(mwctst::TestHelper::e_DEFAULT);
}
