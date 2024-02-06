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
#include <m_bmqstoragetool_commandprocessorfactory.h>
#include <m_bmqstoragetool_journalfileprocessor.h>

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

// GMOCK
#include <gmock/gmock.h>

// TEST DRIVER
#include <mwctst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace m_bmqstoragetool;
using namespace bsl;
using namespace mqbs;
using namespace ::testing;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------

namespace {

typedef bsls::AlignedBuffer<FileStoreProtocol::k_JOURNAL_RECORD_SIZE>
    RecordBufferType;

typedef bsl::pair<RecordType::Enum, RecordBufferType> NodeType;

typedef bsl::list<NodeType> RecordsListType;

class JournalFile {
  private:
    size_t               d_numRecords;
    MappedFileDescriptor d_mfd;
    char*                d_mem_p;
    MemoryBlock          d_block;
    bsls::Types::Uint64  d_currPos;
    FileHeader           d_fileHeader;
    JournalFileIterator  d_iterator;
    bslma::Allocator*    d_allocator_p;

    // MANIPULATORS

    void createFileHeader()
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

  public:
    // CREATORS
    JournalFile(const size_t numRecords, bslma::Allocator* allocator)
    : d_numRecords(numRecords)
    , d_allocator_p(allocator)
    {
        bsls::Types::Uint64 totalSize =
            sizeof(FileHeader) + sizeof(JournalFileHeader) +
            numRecords * FileStoreProtocol::k_JOURNAL_RECORD_SIZE;

        d_mem_p = static_cast<char*>(s_allocator_p->allocate(totalSize));
        d_block.setBase(d_mem_p);
        d_block.setSize(totalSize);

        createFileHeader();

        d_mfd.setFd(-1);  // invalid fd will suffice.
        d_mfd.setBlock(d_block);
        d_mfd.setFileSize(totalSize);
    };

    ~JournalFile() { d_allocator_p->deallocate(d_mem_p); };

    // ACCESSORS

    const MappedFileDescriptor& mappedFileDescriptor() const { return d_mfd; }

    const FileHeader& fileHeader() const { return d_fileHeader; }

    // MANIPULATORS

    void addAllTypesRecords(RecordsListType* records)
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
                    .setTimestamp(i);
                rec->setRefCount(i %
                                 FileStoreProtocol::k_MAX_MSG_REF_COUNT_HARD)
                    .setQueueKey(mqbu::StorageKey(
                        mqbu::StorageKey::BinaryRepresentation(),
                        "abcde"))
                    .setFileKey(mqbu::StorageKey(
                        mqbu::StorageKey::BinaryRepresentation(),
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
                    .setTimestamp(i);
                rec->setReason(ConfirmReason::e_REJECTED)
                    .setQueueKey(mqbu::StorageKey(
                        mqbu::StorageKey::BinaryRepresentation(),
                        "abcde"))
                    .setAppKey(mqbu::StorageKey(
                        mqbu::StorageKey::BinaryRepresentation(),
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
                    .setTimestamp(i);
                rec->setDeletionRecordFlag(
                       DeletionRecordFlag::e_IMPLICIT_CONFIRM)
                    .setQueueKey(mqbu::StorageKey(
                        mqbu::StorageKey::BinaryRepresentation(),
                        "abcde"))
                    .setMessageGUID(g)
                    .setMagic(RecordHeader::k_MAGIC);

                RecordBufferType buf;
                bsl::memcpy(buf.buffer(),
                            rec.get(),
                            FileStoreProtocol::k_JOURNAL_RECORD_SIZE);
                records->push_back(
                    bsl::make_pair(RecordType::e_DELETION, buf));
            }
            else if (3 == remainder) {
                // QueueOpRec
                OffsetPtr<QueueOpRecord> rec(d_block, d_currPos);
                new (rec.get()) QueueOpRecord();
                rec->header()
                    .setPrimaryLeaseId(100)
                    .setSequenceNumber(i)
                    .setTimestamp(i);
                rec->setFlags(3)
                    .setQueueKey(mqbu::StorageKey(
                        mqbu::StorageKey::BinaryRepresentation(),
                        "abcde"))
                    .setAppKey(mqbu::StorageKey(
                        mqbu::StorageKey::BinaryRepresentation(),
                        "appid"))
                    .setType(QueueOpType::e_PURGE)
                    .setMagic(RecordHeader::k_MAGIC);

                RecordBufferType buf;
                bsl::memcpy(buf.buffer(),
                            rec.get(),
                            FileStoreProtocol::k_JOURNAL_RECORD_SIZE);
                records->push_back(
                    bsl::make_pair(RecordType::e_QUEUE_OP, buf));
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
                    .setTimestamp(i);
                RecordBufferType buf;
                bsl::memcpy(buf.buffer(),
                            rec.get(),
                            FileStoreProtocol::k_JOURNAL_RECORD_SIZE);
                records->push_back(
                    bsl::make_pair(RecordType::e_JOURNAL_OP, buf));
            }

            d_currPos += FileStoreProtocol::k_JOURNAL_RECORD_SIZE;
        }
    }

    bsl::vector<bmqt::MessageGUID>
    addJournalRecordsWithOutstandingAndConfirmedMessages(
        RecordsListType* records,
        bool             expectOutstandingResult)
    {
        bool                           outstandingFlag = false;
        bmqt::MessageGUID              lastMessageGUID;
        bsl::vector<bmqt::MessageGUID> expectedGUIDs;

        for (unsigned int i = 1; i <= d_numRecords; ++i) {
            unsigned int remainder = i % 3;
            if (1 == remainder) {
                mqbu::MessageGUIDUtil::generateGUID(&lastMessageGUID);
                OffsetPtr<MessageRecord> rec(d_block, d_currPos);
                new (rec.get()) MessageRecord();
                rec->header().setPrimaryLeaseId(100).setSequenceNumber(i);
                rec->setRefCount(i %
                                 FileStoreProtocol::k_MAX_MSG_REF_COUNT_HARD)
                    .setQueueKey(mqbu::StorageKey(
                        mqbu::StorageKey::BinaryRepresentation(),
                        "abcde"))
                    .setFileKey(mqbu::StorageKey(
                        mqbu::StorageKey::BinaryRepresentation(),
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
                rec->header().setPrimaryLeaseId(100).setSequenceNumber(i);
                rec->setReason(ConfirmReason::e_REJECTED)
                    .setQueueKey(mqbu::StorageKey(
                        mqbu::StorageKey::BinaryRepresentation(),
                        "abcde"))
                    .setAppKey(mqbu::StorageKey(
                        mqbu::StorageKey::BinaryRepresentation(),
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
                OffsetPtr<DeletionRecord> rec(d_block, d_currPos);
                new (rec.get()) DeletionRecord();
                rec->header().setPrimaryLeaseId(100).setSequenceNumber(i);
                rec->setDeletionRecordFlag(
                       DeletionRecordFlag::e_IMPLICIT_CONFIRM)
                    .setQueueKey(mqbu::StorageKey(
                        mqbu::StorageKey::BinaryRepresentation(),
                        "abcde"))
                    .setMessageGUID(g)
                    .setMagic(RecordHeader::k_MAGIC);

                RecordBufferType buf;
                bsl::memcpy(buf.buffer(),
                            rec.get(),
                            FileStoreProtocol::k_JOURNAL_RECORD_SIZE);
                records->push_back(
                    bsl::make_pair(RecordType::e_DELETION, buf));
            }

            d_currPos += FileStoreProtocol::k_JOURNAL_RECORD_SIZE;
        }

        return expectedGUIDs;
    }

    // Generate sequence of MessageRecord, ConfirmRecord and DeleteRecord
    // records. MessageRecord and ConfirmRecord records have the same GUID.
    // DeleteRecord records even records have the same GUID as MessageRecord,
    // odd ones - not the same.
    bsl::vector<bmqt::MessageGUID>
    addJournalRecordsWithPartiallyConfirmedMessages(RecordsListType* records)
    {
        bool                           partialyConfirmedFlag = false;
        bmqt::MessageGUID              lastMessageGUID;
        bsl::vector<bmqt::MessageGUID> expectedGUIDs;

        for (unsigned int i = 1; i <= d_numRecords; ++i) {
            unsigned int remainder = i % 3;

            if (1 == remainder) {
                // bmqt::MessageGUID g;
                mqbu::MessageGUIDUtil::generateGUID(&lastMessageGUID);
                OffsetPtr<MessageRecord> rec(d_block, d_currPos);
                new (rec.get()) MessageRecord();
                rec->header().setPrimaryLeaseId(100).setSequenceNumber(i);
                rec->setRefCount(i %
                                 FileStoreProtocol::k_MAX_MSG_REF_COUNT_HARD)
                    .setQueueKey(mqbu::StorageKey(
                        mqbu::StorageKey::BinaryRepresentation(),
                        "abcde"))
                    .setFileKey(mqbu::StorageKey(
                        mqbu::StorageKey::BinaryRepresentation(),
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
                rec->header().setPrimaryLeaseId(100).setSequenceNumber(i);
                rec->setReason(ConfirmReason::e_REJECTED)
                    .setQueueKey(mqbu::StorageKey(
                        mqbu::StorageKey::BinaryRepresentation(),
                        "abcde"))
                    .setAppKey(mqbu::StorageKey(
                        mqbu::StorageKey::BinaryRepresentation(),
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
                OffsetPtr<DeletionRecord> rec(d_block, d_currPos);
                new (rec.get()) DeletionRecord();
                rec->header().setPrimaryLeaseId(100).setSequenceNumber(i);
                rec->setDeletionRecordFlag(
                       DeletionRecordFlag::e_IMPLICIT_CONFIRM)
                    .setQueueKey(mqbu::StorageKey(
                        mqbu::StorageKey::BinaryRepresentation(),
                        "abcde"))
                    .setMessageGUID(g)
                    .setMagic(RecordHeader::k_MAGIC);

                RecordBufferType buf;
                bsl::memcpy(buf.buffer(),
                            rec.get(),
                            FileStoreProtocol::k_JOURNAL_RECORD_SIZE);
                records->push_back(
                    bsl::make_pair(RecordType::e_DELETION, buf));
            }

            d_currPos += FileStoreProtocol::k_JOURNAL_RECORD_SIZE;
        }

        return expectedGUIDs;
    }

    // Generate sequence of MessageRecord, ConfirmRecord and DeleteRecord
    // records. MessageRecord ConfirmRecord and DeletionRecord records have the
    // same GUID. queueKey1 is used for even records, queueKey1 for odd ones.
    // Returns GUIDs with queueKey1.
    bsl::vector<bmqt::MessageGUID>
    addJournalRecordsWithTwoQueueKeys(RecordsListType* records,
                                      const char*      queueKey1,
                                      const char*      queueKey2)
    {
        bmqt::MessageGUID              lastMessageGUID;
        bsl::vector<bmqt::MessageGUID> expectedGUIDs;

        for (unsigned int i = 1; i <= d_numRecords; ++i) {
            const char* queueKey = (i % 2 != 0) ? queueKey1 : queueKey2;

            unsigned int remainder = i % 3;
            if (1 == remainder) {
                mqbu::MessageGUIDUtil::generateGUID(&lastMessageGUID);
                if (queueKey == queueKey1) {
                    expectedGUIDs.push_back(lastMessageGUID);
                }
                OffsetPtr<MessageRecord> rec(d_block, d_currPos);
                new (rec.get()) MessageRecord();
                rec->header().setPrimaryLeaseId(100).setSequenceNumber(i);
                rec->setRefCount(i %
                                 FileStoreProtocol::k_MAX_MSG_REF_COUNT_HARD)
                    .setQueueKey(
                        mqbu::StorageKey(mqbu::StorageKey::HexRepresentation(),
                                         queueKey))
                    .setFileKey(mqbu::StorageKey(
                        mqbu::StorageKey::BinaryRepresentation(),
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
                rec->header().setPrimaryLeaseId(100).setSequenceNumber(i);
                rec->setReason(ConfirmReason::e_REJECTED)
                    .setQueueKey(
                        mqbu::StorageKey(mqbu::StorageKey::HexRepresentation(),
                                         queueKey))
                    .setAppKey(mqbu::StorageKey(
                        mqbu::StorageKey::BinaryRepresentation(),
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
                rec->header().setPrimaryLeaseId(100).setSequenceNumber(i);
                rec->setDeletionRecordFlag(
                       DeletionRecordFlag::e_IMPLICIT_CONFIRM)
                    .setQueueKey(
                        mqbu::StorageKey(mqbu::StorageKey::HexRepresentation(),
                                         queueKey))
                    .setMessageGUID(g)
                    .setMagic(RecordHeader::k_MAGIC);

                RecordBufferType buf;
                bsl::memcpy(buf.buffer(),
                            rec.get(),
                            FileStoreProtocol::k_JOURNAL_RECORD_SIZE);
                records->push_back(
                    bsl::make_pair(RecordType::e_DELETION, buf));
            }

            d_currPos += FileStoreProtocol::k_JOURNAL_RECORD_SIZE;
        }

        return expectedGUIDs;
    }

    bsl::vector<bmqt::MessageGUID>
    addJournalRecordsWithConfirmedMessagesWithDifferentOrder(
        RecordsListType*           records,
        size_t                     numMessages,
        bsl::vector<unsigned int>& messageOffsets)
    {
        bsl::vector<bmqt::MessageGUID> expectedGUIDs;

        ASSERT(numMessages == messageOffsets.size());
        ASSERT(numMessages >= 3);
        ASSERT(d_numRecords == numMessages * 2);

        // Create messages
        for (unsigned int i = 0; i < numMessages; ++i) {
            bmqt::MessageGUID g;
            mqbu::MessageGUIDUtil::generateGUID(&g);
            expectedGUIDs.push_back(g);
            OffsetPtr<MessageRecord> rec(d_block, d_currPos);
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

            d_currPos += FileStoreProtocol::k_JOURNAL_RECORD_SIZE;
        }

        // Create delete messages, and replace GUIDS for 2nd and 3rd message
        for (unsigned int i = 0; i < numMessages; ++i) {
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

            OffsetPtr<DeletionRecord> rec(d_block, d_currPos);
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

            d_currPos += FileStoreProtocol::k_JOURNAL_RECORD_SIZE;
        }

        return expectedGUIDs;
    }
};

class ParametersMock : public Parameters {
    mqbs::JournalFileIterator d_journalFileIt;

  public:
    // CREATORS
    explicit ParametersMock(const JournalFile& journalFile,
                            bslma::Allocator*  allocator = 0)
    : d_journalFileIt(&journalFile.mappedFileDescriptor(),
                      journalFile.fileHeader(),
                      false)
    {
    }

    // MANIPULATORS
    mqbs::JournalFileIterator* journalFileIterator() BSLS_KEYWORD_OVERRIDE
    {
        return &d_journalFileIt;
    };
    MOCK_METHOD0(dataFileIterator, mqbs::DataFileIterator*());

    // ACCESSORS
    MOCK_CONST_METHOD0(timestampGt, bsls::Types::Int64());
    MOCK_CONST_METHOD0(timestampLt, bsls::Types::Int64());
    MOCK_CONST_METHOD0(guid, bsl::vector<bsl::string>());
    MOCK_CONST_METHOD0(queueKey, bsl::vector<bsl::string>());
    MOCK_CONST_METHOD0(queueName, bsl::vector<bsl::string>());
    MOCK_CONST_METHOD0(dumpLimit, unsigned int());
    MOCK_CONST_METHOD0(details, bool());
    MOCK_CONST_METHOD0(dumpPayload, bool());
    MOCK_CONST_METHOD0(summary, bool());
    MOCK_CONST_METHOD0(outstanding, bool());
    MOCK_CONST_METHOD0(confirmed, bool());
    MOCK_CONST_METHOD0(partiallyConfirmed, bool());
    MOCK_CONST_METHOD0(queueMap, const QueueMap&());

    // MEMBER FUNCTIONS
    MOCK_CONST_METHOD1(print, void(bsl::ostream&));
};

void mockParametersDefault(ParametersMock& params)
{
    // Prepare parameters
    EXPECT_CALL(params, dataFileIterator()).WillRepeatedly(Return(nullptr));
    EXPECT_CALL(params, timestampGt()).WillRepeatedly(Return(0));
    EXPECT_CALL(params, timestampLt()).WillRepeatedly(Return(0));
    EXPECT_CALL(params, guid())
        .WillRepeatedly(Return(bsl::vector<bsl::string>(s_allocator_p)));
    EXPECT_CALL(params, queueKey())
        .WillRepeatedly(Return(bsl::vector<bsl::string>(s_allocator_p)));
    EXPECT_CALL(params, queueName())
        .WillRepeatedly(Return(bsl::vector<bsl::string>(s_allocator_p)));
    EXPECT_CALL(params, dumpLimit()).WillRepeatedly(Return(0));
    EXPECT_CALL(params, details()).WillRepeatedly(Return(false));
    EXPECT_CALL(params, dumpPayload()).WillRepeatedly(Return(false));
    EXPECT_CALL(params, summary()).WillRepeatedly(Return(false));
    EXPECT_CALL(params, outstanding()).WillRepeatedly(Return(false));
    EXPECT_CALL(params, confirmed()).WillRepeatedly(Return(false));
    EXPECT_CALL(params, partiallyConfirmed()).WillRepeatedly(Return(false));
    static QueueMap qm(s_allocator_p);
    EXPECT_CALL(params, queueMap()).WillRepeatedly(ReturnPointee(&qm));
    Mock::AllowLeak(&params);
}

void outputGuidString(bsl::ostream&            ostream,
                      const bmqt::MessageGUID& messageGUID,
                      const bool               addNewLine = true)
{
    ostream << messageGUID;
    if (addNewLine)
        ostream << bsl::endl;
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
    size_t          numRecords = 15;
    RecordsListType records(s_allocator_p);
    JournalFile     journalFile(numRecords, s_allocator_p);
    journalFile.addAllTypesRecords(&records);

    // Prepare parameters
    bsl::unique_ptr<ParametersMock> params =
        bsl::make_unique<ParametersMock>(journalFile, s_allocator_p);
    mockParametersDefault(*params);

    // Run search
    bsl::ostringstream resultStream(s_allocator_p);
    auto searchProcessor = CommandProcessorFactory::createCommandProcessor(
        bsl::move(params),
        resultStream,
        s_allocator_p);
    searchProcessor->process();

    // Prepare expected output with list of message GUIDs in Journal file
    bsl::ostringstream                  expectedStream(s_allocator_p);
    bsl::list<NodeType>::const_iterator recordIter         = records.begin();
    bsl::size_t                         foundMessagesCount = 0;
    while (recordIter++ != records.end()) {
        RecordType::Enum rtype = recordIter->first;
        if (rtype == RecordType::e_MESSAGE) {
            const MessageRecord& msg = *reinterpret_cast<const MessageRecord*>(
                recordIter->second.buffer());
            outputGuidString(expectedStream, msg.messageGUID());
            foundMessagesCount++;
        }
    }
    expectedStream << foundMessagesCount << " message GUID(s) found."
                   << bsl::endl;

    ASSERT_EQ(resultStream.str(), expectedStream.str());
}

static void test2_searchGuidTest()
// ------------------------------------------------------------------------
// SEARCH GUID TEST
//
// Concerns:
//   Search messages by GUIDs in journal file and output GUIDs.
//
// Testing:
//   JournalFileProcessor::process()
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("SEARCH GUID");

    // Simulate journal file
    size_t          numRecords = 15;
    RecordsListType records(s_allocator_p);
    JournalFile     journalFile(numRecords, s_allocator_p);
    journalFile.addAllTypesRecords(&records);

    // Get list of message GUIDs for searching
    bsl::vector<bsl::string> searchGuids;

    bsl::list<NodeType>::const_iterator recordIter = records.begin();
    bsl::size_t                         msgCnt     = 0;
    while (recordIter++ != records.end()) {
        RecordType::Enum rtype = recordIter->first;
        if (rtype == RecordType::e_MESSAGE) {
            if (msgCnt++ % 2 != 0)
                continue;  // Skip odd messages for test purposes
            const MessageRecord& msg = *reinterpret_cast<const MessageRecord*>(
                recordIter->second.buffer());
            bsl::ostringstream ss(s_allocator_p);
            ss << msg.messageGUID();
            searchGuids.push_back(ss.str());
        }
    }
    bsl::unique_ptr<ParametersMock> params =
        bsl::make_unique<ParametersMock>(journalFile, s_allocator_p);
    mockParametersDefault(*params);
    EXPECT_CALL(*params, guid()).WillRepeatedly(ReturnPointee(&searchGuids));

    // Run search
    bsl::ostringstream resultStream(s_allocator_p);
    auto searchProcessor = CommandProcessorFactory::createCommandProcessor(
        bsl::move(params),
        resultStream,
        s_allocator_p);
    searchProcessor->process();

    // Prepare expected output
    bsl::ostringstream expectedStream(s_allocator_p);
    for (auto& guid : searchGuids) {
        expectedStream << guid << bsl::endl;
    }
    expectedStream << searchGuids.size() << " message GUID(s) found."
                   << bsl::endl;

    ASSERT_EQ(resultStream.str(), expectedStream.str());
}

static void test3_searchNonExistingGuidTest()
// ------------------------------------------------------------------------
// SEARCH NON EXISTING GUID TEST
//
// Concerns:
//   Search messages by non existing GUIDs in journal file and output result.
//
// Testing:
//   JournalFileProcessor::process()
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("SEARCH NON EXISTING GUID");

    // Simulate journal file
    size_t          numRecords = 15;
    RecordsListType records(s_allocator_p);
    JournalFile     journalFile(numRecords, s_allocator_p);
    journalFile.addAllTypesRecords(&records);

    // Get list of message GUIDs for searching
    bsl::vector<bsl::string> searchGuids;
    bmqt::MessageGUID        guid;
    for (int i = 0; i < 2; ++i) {
        mqbu::MessageGUIDUtil::generateGUID(&guid);
        bsl::ostringstream ss(s_allocator_p);
        ss << guid;
        searchGuids.push_back(ss.str());
    }

    bsl::unique_ptr<ParametersMock> params =
        bsl::make_unique<ParametersMock>(journalFile, s_allocator_p);
    mockParametersDefault(*params);
    EXPECT_CALL(*params, guid()).WillRepeatedly(ReturnPointee(&searchGuids));

    // Run search
    bsl::ostringstream resultStream(s_allocator_p);
    auto searchProcessor = CommandProcessorFactory::createCommandProcessor(
        bsl::move(params),
        resultStream,
        s_allocator_p);
    searchProcessor->process();

    // Prepare expected output
    bsl::ostringstream expectedStream(s_allocator_p);
    expectedStream << "No message GUID found." << bsl::endl;

    expectedStream << bsl::endl
                   << "The following 2 GUID(s) not found:" << bsl::endl;
    expectedStream << searchGuids[0] << bsl::endl
                   << searchGuids[1] << bsl::endl;

    ASSERT_EQ(resultStream.str(), expectedStream.str());
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
//   JournalFileProcessor::process()
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("SEARCH EXISTING AND NON EXISTING GUID");

    // Simulate journal file
    size_t          numRecords = 15;
    RecordsListType records(s_allocator_p);
    JournalFile     journalFile(numRecords, s_allocator_p);
    journalFile.addAllTypesRecords(&records);

    // Get list of message GUIDs for searching
    bsl::vector<bsl::string> searchGuids;

    // Get two existing message GUIDs
    bsl::list<NodeType>::const_iterator recordIter = records.begin();
    size_t                              msgCnt     = 0;
    while (recordIter++ != records.end()) {
        RecordType::Enum rtype = recordIter->first;
        if (rtype == RecordType::e_MESSAGE) {
            if (msgCnt++ == 2)
                break;  // Take two GUIDs
            const MessageRecord& msg = *reinterpret_cast<const MessageRecord*>(
                recordIter->second.buffer());
            bsl::ostringstream ss(s_allocator_p);
            ss << msg.messageGUID();
            searchGuids.push_back(ss.str());
        }
    }

    // Get two non existing message GUIDs
    bmqt::MessageGUID guid;
    for (int i = 0; i < 2; ++i) {
        mqbu::MessageGUIDUtil::generateGUID(&guid);
        bsl::ostringstream ss(s_allocator_p);
        ss << guid;
        searchGuids.push_back(ss.str());
    }

    bsl::unique_ptr<ParametersMock> params =
        bsl::make_unique<ParametersMock>(journalFile, s_allocator_p);
    mockParametersDefault(*params);
    EXPECT_CALL(*params, guid()).WillRepeatedly(ReturnPointee(&searchGuids));

    // Run search
    bsl::ostringstream resultStream(s_allocator_p);
    auto searchProcessor = CommandProcessorFactory::createCommandProcessor(
        bsl::move(params),
        resultStream,
        s_allocator_p);
    searchProcessor->process();

    // Prepare expected output
    bsl::ostringstream expectedStream(s_allocator_p);
    expectedStream << searchGuids[0] << bsl::endl
                   << searchGuids[1] << bsl::endl;
    expectedStream << "2 message GUID(s) found." << bsl::endl;
    expectedStream << bsl::endl
                   << "The following 2 GUID(s) not found:" << bsl::endl;
    expectedStream << searchGuids[2] << bsl::endl
                   << searchGuids[3] << bsl::endl;

    ASSERT_EQ(resultStream.str(), expectedStream.str());
}

static void test5_searchOutstandingMessagesTest()
// ------------------------------------------------------------------------
// SEARCH OUTSTANDING MESSAGES TEST
//
// Concerns:
//   Search outstanding (not deleted) messages and output GUIDs.
//
// Testing:
//   JournalFileProcessor::process()
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("SEARCH OUTSTANDING MESSAGES TEST");

    // Simulate journal file
    size_t                         numRecords = 15;
    RecordsListType                records(s_allocator_p);
    JournalFile                    journalFile(numRecords, s_allocator_p);
    bsl::vector<bmqt::MessageGUID> outstandingGUIDS =
        journalFile.addJournalRecordsWithOutstandingAndConfirmedMessages(
            &records,
            true);

    // Configure parameters to search outstanding messages
    bsl::unique_ptr<ParametersMock> params =
        bsl::make_unique<ParametersMock>(journalFile, s_allocator_p);
    mockParametersDefault(*params);
    EXPECT_CALL(*params, outstanding()).WillRepeatedly(Return(true));

    // Run search
    bsl::ostringstream resultStream(s_allocator_p);
    auto searchProcessor = CommandProcessorFactory::createCommandProcessor(
        bsl::move(params),
        resultStream,
        s_allocator_p);
    searchProcessor->process();

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
}

static void test6_searchConfirmedMessagesTest()
// ------------------------------------------------------------------------
// SEARCH CONFIRMED MESSAGES TEST
//
// Concerns:
//   Search confirmed (deleted) messages  in journal file and output GUIDs.
//
// Testing:
//   JournalFileProcessor::process()
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("SEARCH CONFIRMED MESSAGES TEST");

    // Simulate journal file
    size_t                         numRecords = 15;
    RecordsListType                records(s_allocator_p);
    JournalFile                    journalFile(numRecords, s_allocator_p);
    bsl::vector<bmqt::MessageGUID> confirmedGUIDS =
        journalFile.addJournalRecordsWithOutstandingAndConfirmedMessages(
            &records,
            false);

    // Configure parameters to search confirmed messages
    bsl::unique_ptr<ParametersMock> params =
        bsl::make_unique<ParametersMock>(journalFile, s_allocator_p);
    mockParametersDefault(*params);
    EXPECT_CALL(*params, confirmed()).WillRepeatedly(Return(true));

    // Run search
    bsl::ostringstream resultStream(s_allocator_p);
    auto searchProcessor = CommandProcessorFactory::createCommandProcessor(
        bsl::move(params),
        resultStream,
        s_allocator_p);
    searchProcessor->process();

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
//   JournalFileProcessor::process()
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName(
        "SEARCH PARTIALLY CONFIRMED MESSAGES TEST");

    // Simulate journal file
    // numRecords must be multiple 3 plus one to cover all combinations
    // (confirmed, deleted, not confirmed)
    size_t                         numRecords = 16;
    RecordsListType                records(s_allocator_p);
    JournalFile                    journalFile(numRecords, s_allocator_p);
    bsl::vector<bmqt::MessageGUID> partiallyConfirmedGUIDS =
        journalFile.addJournalRecordsWithPartiallyConfirmedMessages(&records);

    // Configure parameters to search partially confirmed messages
    bsl::unique_ptr<ParametersMock> params =
        bsl::make_unique<ParametersMock>(journalFile, s_allocator_p);
    mockParametersDefault(*params);
    EXPECT_CALL(*params, partiallyConfirmed()).WillRepeatedly(Return(true));

    // Run search
    bsl::ostringstream resultStream(s_allocator_p);
    auto searchProcessor = CommandProcessorFactory::createCommandProcessor(
        bsl::move(params),
        resultStream,
        s_allocator_p);
    searchProcessor->process();

    // Prepare expected output
    bsl::ostringstream expectedStream(s_allocator_p);
    for (const auto& guid : partiallyConfirmedGUIDS) {
        outputGuidString(expectedStream, guid);
    }
    expectedStream << partiallyConfirmedGUIDS.size()
                   << " message GUID(s) found." << bsl::endl;
    float messageCount     = ceil(numRecords / 3.0);
    float outstandingRatio = float(partiallyConfirmedGUIDS.size() + 1) /
                             messageCount * 100.0;
    expectedStream << "Outstanding ratio: " << outstandingRatio << "% ("
                   << partiallyConfirmedGUIDS.size() + 1 << "/" << messageCount
                   << ")" << bsl::endl;

    ASSERT_EQ(resultStream.str(), expectedStream.str());
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
//   JournalFileProcessor::process()
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("SEARCH MESSAGES BY QUEUE KEY TEST");

    // Simulate journal file
    size_t                         numRecords = 15;
    RecordsListType                records(s_allocator_p);
    JournalFile                    journalFile(numRecords, s_allocator_p);
    const char*                    queueKey1 = "ABCDE12345";
    const char*                    queueKey2 = "12345ABCDE";
    bsl::vector<bmqt::MessageGUID> queueKey1GUIDS =
        journalFile.addJournalRecordsWithTwoQueueKeys(&records,
                                                      queueKey1,
                                                      queueKey2);

    // Configure parameters to search messages by queueKey1
    bsl::unique_ptr<ParametersMock> params =
        bsl::make_unique<ParametersMock>(journalFile, s_allocator_p);
    mockParametersDefault(*params);
    bsl::vector<bsl::string> queueKeys(1, queueKey1, s_allocator_p);
    EXPECT_CALL(*params, queueKey()).WillRepeatedly(ReturnPointee(&queueKeys));

    // Run search
    bsl::ostringstream resultStream(s_allocator_p);
    auto searchProcessor = CommandProcessorFactory::createCommandProcessor(
        bsl::move(params),
        resultStream,
        s_allocator_p);
    searchProcessor->process();

    // Prepare expected output
    bsl::ostringstream expectedStream(s_allocator_p);
    for (const auto& guid : queueKey1GUIDS) {
        outputGuidString(expectedStream, guid);
    }
    auto foundMessagesCount = queueKey1GUIDS.size();
    expectedStream << foundMessagesCount << " message GUID(s) found."
                   << bsl::endl;

    ASSERT_EQ(resultStream.str(), expectedStream.str());
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
//   JournalFileProcessor::process()
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("SEARCH MESSAGES BY QUEUE NAME TEST");

    // Simulate journal file
    size_t                         numRecords = 15;
    RecordsListType                records(s_allocator_p);
    JournalFile                    journalFile(numRecords, s_allocator_p);
    const char*                    queueKey1 = "ABCDE12345";
    const char*                    queueKey2 = "12345ABCDE";
    bsl::vector<bmqt::MessageGUID> queueKey1GUIDS =
        journalFile.addJournalRecordsWithTwoQueueKeys(&records,
                                                      queueKey1,
                                                      queueKey2);

    // Configure parameters to search messages by 'queue1' and 'unknown' names
    bmqp_ctrlmsg::QueueInfo queueInfo(s_allocator_p);
    queueInfo.uri() = "queue1";
    auto key        = mqbu::StorageKey(mqbu::StorageKey::HexRepresentation(),
                                queueKey1);
    for (int i = 0; i < mqbu::StorageKey::e_KEY_LENGTH_BINARY; i++) {
        queueInfo.key().push_back(key.data()[i]);
    }
    QueueMap qMap(s_allocator_p);
    qMap.insert(queueInfo);
    bsl::unique_ptr<ParametersMock> params =
        bsl::make_unique<ParametersMock>(journalFile, s_allocator_p);
    mockParametersDefault(*params);
    bsl::vector<bsl::string> queueNames(s_allocator_p);
    queueNames.push_back("queue1");
    EXPECT_CALL(*params, queueName())
        .WillRepeatedly(ReturnPointee(&queueNames));
    EXPECT_CALL(*params, queueMap()).WillRepeatedly(ReturnPointee(&qMap));

    // Run search
    bsl::ostringstream resultStream(s_allocator_p);
    auto searchProcessor = CommandProcessorFactory::createCommandProcessor(
        bsl::move(params),
        resultStream,
        s_allocator_p);
    searchProcessor->process();

    // Prepare expected output
    bsl::ostringstream expectedStream(s_allocator_p);
    for (const auto& guid : queueKey1GUIDS) {
        outputGuidString(expectedStream, guid);
    }
    auto foundMessagesCount = queueKey1GUIDS.size();
    expectedStream << foundMessagesCount << " message GUID(s) found."
                   << bsl::endl;

    ASSERT_EQ(resultStream.str(), expectedStream.str());
}

static void test10_searchMessagesByTimestamp()
// ------------------------------------------------------------------------
// SEARCH MESSAGES BY TIMESTAMP TEST
//
// Concerns:
//   Search messages by timestamp in journal file and output GUIDs.
//
// Testing:
//   JournalFileProcessor::process()
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("SEARCH MESSAGES BY TIMESTAMP TEST");

    // Simulate journal file
    size_t          numRecords = 50;
    RecordsListType records(s_allocator_p);
    JournalFile     journalFile(numRecords, s_allocator_p);
    journalFile.addAllTypesRecords(&records);

    // Configure parameters to search messages by timestamps
    bsl::unique_ptr<ParametersMock> params =
        bsl::make_unique<ParametersMock>(journalFile, s_allocator_p);
    mockParametersDefault(*params);
    bsl::vector<bsl::string> queueNames(s_allocator_p);
    queueNames.push_back("queue1");
    queueNames.push_back("queue");
    EXPECT_CALL(*params, timestampGt()).WillRepeatedly(Return(10));
    EXPECT_CALL(*params, timestampLt()).WillRepeatedly(Return(40));

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
            if (ts > 10 && ts < 40) {
                outputGuidString(expectedStream, msg.messageGUID());
                msgCnt++;
            }
        }
    }
    expectedStream << msgCnt << " message GUID(s) found." << bsl::endl;

    // Run search
    bsl::ostringstream resultStream(s_allocator_p);
    auto searchProcessor = CommandProcessorFactory::createCommandProcessor(
        bsl::move(params),
        resultStream,
        s_allocator_p);
    searchProcessor->process();

    ASSERT_EQ(resultStream.str(), expectedStream.str());
}

static void test11_printMessagesDetailsTest()
// ------------------------------------------------------------------------
// PRINT MESSAGE DETAILS TEST
//
// Concerns:
//   Search messages in journal file and output message details.
//
// Testing:
//   JournalFileProcessor::process()
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("PRINT MESSAGE DETAILS TEST");

    // Simulate journal file
    size_t                         numRecords = 15;
    RecordsListType                records(s_allocator_p);
    JournalFile                    journalFile(numRecords, s_allocator_p);
    bsl::vector<bmqt::MessageGUID> confirmedGUIDS =
        journalFile.addJournalRecordsWithOutstandingAndConfirmedMessages(
            &records,
            false);

    // Configure parameters to print message details
    bsl::unique_ptr<ParametersMock> params =
        bsl::make_unique<ParametersMock>(journalFile, s_allocator_p);
    mockParametersDefault(*params);
    EXPECT_CALL(*params, details()).WillRepeatedly(Return(true));

    // Run search
    bsl::ostringstream resultStream(s_allocator_p);
    auto searchProcessor = CommandProcessorFactory::createCommandProcessor(
        bsl::move(params),
        resultStream,
        s_allocator_p);
    searchProcessor->process();

    // Check that substrings are present in resultStream in correct order
    auto        resultString         = resultStream.str();
    size_t      startIdx             = 0;
    const char* messageRecordCaption = "MESSAGE Record";
    const char* confirmRecordCaption = "CONFIRM Record";
    const char* deleteRecordCaption  = "DELETE Record";
    for (int i = 0; i < confirmedGUIDS.size(); i++) {
        // Check Message type
        size_t foundIdx = resultString.find(messageRecordCaption, startIdx);
        ASSERT_D(messageRecordCaption, (foundIdx != bsl::string::npos));
        ASSERT_D(messageRecordCaption, (foundIdx >= startIdx));
        startIdx = foundIdx + bsl::strlen(messageRecordCaption);

        // Check GUID
        bsl::ostringstream ss;
        outputGuidString(ss, confirmedGUIDS.at(i));
        bsl::string guidStr = ss.str();
        foundIdx            = resultString.find(guidStr, startIdx);
        ASSERT_D(guidStr, (foundIdx != bsl::string::npos));
        ASSERT_D(guidStr, (foundIdx >= startIdx));
        startIdx = foundIdx + guidStr.length();

        // Check Confirm type
        foundIdx = resultString.find(confirmRecordCaption, startIdx);
        ASSERT_D(confirmRecordCaption, (foundIdx != bsl::string::npos));
        ASSERT_D(confirmRecordCaption, (foundIdx >= startIdx));
        startIdx = foundIdx + bsl::strlen(messageRecordCaption);

        // Check Delete type
        foundIdx = resultString.find(deleteRecordCaption, startIdx);
        ASSERT_D(deleteRecordCaption, (foundIdx != bsl::string::npos));
        ASSERT_D(deleteRecordCaption, (foundIdx >= startIdx));
        startIdx = foundIdx + bsl::strlen(messageRecordCaption);
    }
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
//   JournalFileProcessor::process()
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
        k_NUM_MSGS * 2;  // k_NUM_MSGS records + k_NUM_MSGS deletion records

    RecordsListType                records(s_allocator_p);
    JournalFile                    journalFile(numRecords, s_allocator_p);
    bsl::vector<bmqt::MessageGUID> confirmedGUIDS =
        journalFile.addJournalRecordsWithConfirmedMessagesWithDifferentOrder(
            &records,
            k_NUM_MSGS,
            messageOffsets);

    // Configure parameters to search confirmed messages GUIDs with dumping
    // messages payload.
    bsl::unique_ptr<ParametersMock> params =
        bsl::make_unique<ParametersMock>(journalFile, s_allocator_p);
    mockParametersDefault(*params);
    EXPECT_CALL(*params, confirmed()).WillRepeatedly(Return(true));
    EXPECT_CALL(*params, dumpPayload()).WillRepeatedly(Return(true));
    EXPECT_CALL(*params, dataFileIterator()).WillRepeatedly(Return(&dataIt));

    // Run search
    bsl::ostringstream resultStream(s_allocator_p);
    auto searchProcessor = CommandProcessorFactory::createCommandProcessor(
        bsl::move(params),
        resultStream,
        s_allocator_p);
    searchProcessor->process();

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

    s_allocator_p->deallocate(pd);
}

static void test13_summaryTest()
// ------------------------------------------------------------------------
// OUTPUT SUMMARY TEST
//
// Concerns:
//   Search messages in journal file and output summary.
//
// Testing:
//   JournalFileProcessor::process()
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("OUTPUT SUMMARY TEST");

    // Simulate journal file
    size_t                         numRecords = 15;
    RecordsListType                records(s_allocator_p);
    JournalFile                    journalFile(numRecords, s_allocator_p);
    bsl::vector<bmqt::MessageGUID> partiallyConfirmedGUIDS =
        journalFile.addJournalRecordsWithPartiallyConfirmedMessages(&records);

    // Configure parameters to output summary
    bsl::unique_ptr<ParametersMock> params =
        bsl::make_unique<ParametersMock>(journalFile, s_allocator_p);
    mockParametersDefault(*params);
    EXPECT_CALL(*params, summary()).WillRepeatedly(Return(true));

    // Run search
    bsl::ostringstream resultStream(s_allocator_p);
    auto searchProcessor = CommandProcessorFactory::createCommandProcessor(
        bsl::move(params),
        resultStream,
        s_allocator_p);
    searchProcessor->process();

    // Prepare expected output
    bsl::ostringstream expectedStream(s_allocator_p);
    expectedStream
        << "5 message(s) found.\nNumber of confirmed messages: 3\nNumber of "
           "partially confirmed messages: 2\n"
           "Number of outstanding messages: 2\nOutstanding ratio: 40% (2/5)\n";

    ASSERT(resultStream.str().starts_with(expectedStream.str()));
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
    case 13: test13_summaryTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        s_testStatus = -1;
    } break;
    }

    // TODO: consider memory usage
    // TEST_EPILOG(mwctst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
    TEST_EPILOG(mwctst::TestHelper::e_DEFAULT);
}
