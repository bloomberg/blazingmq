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

// mqbs_journalfileiterator.t.cpp                                     -*-C++-*-
#include <mqbs_journalfileiterator.h>

// MQB
#include <mqbs_filestoreprotocol.h>
#include <mqbs_mappedfiledescriptor.h>
#include <mqbs_memoryblock.h>
#include <mqbs_offsetptr.h>
#include <mqbu_messageguidutil.h>

// BMQ
#include <bmqt_messageguid.h>

// BDE
#include <bsl_iostream.h>
#include <bsl_limits.h>
#include <bsl_list.h>
#include <bsl_utility.h>
#include <bslma_default.h>
#include <bsls_alignedbuffer.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
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

/// Return 0 if `lhs` is equal to `rhs`, non-zero loggable value otherwise.
int areEqual(const MessageRecord& lhs, const MessageRecord& rhs)
{
    if (lhs.refCount() != rhs.refCount()) {
        return -1;  // RETURN
    }

    if (lhs.queueKey() != rhs.queueKey()) {
        return -2;  // RETURN
    }

    if (lhs.fileKey() != rhs.fileKey()) {
        return -3;  // RETURN
    }

    if (lhs.messageOffsetDwords() != rhs.messageOffsetDwords()) {
        return -4;  // RETURN
    }

    if (lhs.messageGUID() != rhs.messageGUID()) {
        return -6;  // RETURN
    }

    if (lhs.crc32c() != rhs.crc32c()) {
        return -7;  // RETURN
    }

    if (lhs.magic() != rhs.magic()) {
        return -8;  // RETURN
    }

    return 0;
}

/// Return 0 if `lhs` is equal to `rhs`, non-zero loggable value otherwise.
int areEqual(const ConfirmRecord& lhs, const ConfirmRecord& rhs)
{
    if (lhs.queueKey() != rhs.queueKey()) {
        return -1;  // RETURN
    }

    if (lhs.appKey() != rhs.appKey()) {
        return -2;  // RETURN
    }

    if (lhs.messageGUID() != rhs.messageGUID()) {
        return -4;  // RETURN
    }

    if (lhs.magic() != rhs.magic()) {
        return -5;  // RETURN
    }

    return 0;
}

/// Return 0 if `lhs` is equal to `rhs`, non-zero loggable value otherwise.
int areEqual(const DeletionRecord& lhs, const DeletionRecord& rhs)
{
    if (lhs.queueKey() != rhs.queueKey()) {
        return -1;  // RETURN
    }

    if (lhs.deletionRecordFlag() != rhs.deletionRecordFlag()) {
        return -2;  // RETURN
    }

    if (lhs.messageGUID() != rhs.messageGUID()) {
        return -4;  // RETURN
    }

    if (lhs.magic() != rhs.magic()) {
        return -5;  // RETURN
    }

    return 0;
}

/// Return 0 if `lhs` is equal to `rhs`, non-zero loggable value otherwise.
int areEqual(const QueueOpRecord& lhs, const QueueOpRecord& rhs)
{
    if (lhs.flags() != rhs.flags()) {
        return -1;  // RETURN
    }

    if (lhs.queueKey() != rhs.queueKey()) {
        return -2;  // RETURN
    }

    if (lhs.appKey() != rhs.appKey()) {
        return -3;  // RETURN
    }

    if (lhs.type() != rhs.type()) {
        return -5;  // RETURN
    }

    if (lhs.magic() != rhs.magic()) {
        return -7;  // RETURN
    }

    return 0;
}

/// Return 0 if `lhs` is equal to `rhs`, non-zero loggable value otherwise.
int areEqual(const JournalOpRecord& lhs, const JournalOpRecord& rhs)
{
    if (lhs.flags() != rhs.flags()) {
        return -1;  // RETURN
    }

    if (lhs.type() != rhs.type()) {
        return -2;  // RETURN
    }

    if (lhs.sequenceNum() != rhs.sequenceNum()) {
        return -4;  // RETURN
    }

    if (lhs.primaryNodeId() != rhs.primaryNodeId()) {
        return -5;  // RETURN
    }

    if (lhs.primaryLeaseId() != rhs.primaryLeaseId()) {
        return -6;  // RETURN
    }

    if (lhs.magic() != rhs.magic()) {
        return -7;  // RETURN
    }

    return 0;
}

/// Assert if the record currently pointed by iterator 'it' isn't equal to the
/// record stored in 'node'.
void assertEqual(const JournalFileIterator& it, const NodeType& node)
{
    const unsigned int i     = it.recordIndex();
    RecordType::Enum   rtype = it.recordType();
    ASSERT_EQ_D(i, rtype, node.first);

    switch (rtype) {
    case RecordType::e_MESSAGE: {
        const MessageRecord& m1 = *reinterpret_cast<const MessageRecord*>(
            node.second.buffer());
        const MessageRecord& m2 = it.asMessageRecord();

        ASSERT_EQ_D(i, 0, areEqual(m1, m2));
    } break;
    case RecordType::e_CONFIRM: {
        const ConfirmRecord& m1 = *reinterpret_cast<const ConfirmRecord*>(
            node.second.buffer());
        const ConfirmRecord& m2 = it.asConfirmRecord();

        ASSERT_EQ_D(i, 0, areEqual(m1, m2));
    } break;
    case RecordType::e_DELETION: {
        const DeletionRecord& m1 = *reinterpret_cast<const DeletionRecord*>(
            node.second.buffer());
        const DeletionRecord& m2 = it.asDeletionRecord();

        ASSERT_EQ_D(i, 0, areEqual(m1, m2));
    } break;
    case RecordType::e_QUEUE_OP: {
        const QueueOpRecord& m1 = *reinterpret_cast<const QueueOpRecord*>(
            node.second.buffer());
        const QueueOpRecord& m2 = it.asQueueOpRecord();

        ASSERT_EQ_D(i, 0, areEqual(m1, m2));
    } break;
    case RecordType::e_JOURNAL_OP: {
        const JournalOpRecord& m1 = *reinterpret_cast<const JournalOpRecord*>(
            node.second.buffer());

        const JournalOpRecord& m2 = it.asJournalOpRecord();

        ASSERT_EQ_D(i, 0, areEqual(m1, m2));
    } break;
    case RecordType::e_UNDEFINED:
    default: ASSERT_EQ_D(i, 100, 101);  // will fail
    }
}

void addRecords(MemoryBlock*         block,
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
            rec->header().setPrimaryLeaseId(100).setSequenceNumber(i);
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
        else if (2 == remainder) {
            // DelRec
            bmqt::MessageGUID g;
            mqbu::MessageGUIDUtil::generateGUID(&g);
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
        else if (3 == remainder) {
            // QueueOpRec
            OffsetPtr<QueueOpRecord> rec(*block, currPos);
            new (rec.get()) QueueOpRecord();
            rec->header().setPrimaryLeaseId(100).setSequenceNumber(i);
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

            rec->header().setPrimaryLeaseId(100).setSequenceNumber(i);
            RecordBufferType buf;
            bsl::memcpy(buf.buffer(),
                        rec.get(),
                        FileStoreProtocol::k_JOURNAL_RECORD_SIZE);
            records->push_back(bsl::make_pair(RecordType::e_JOURNAL_OP, buf));
        }

        currPos += FileStoreProtocol::k_JOURNAL_RECORD_SIZE;
    }
}

}  // close unnamed namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_breathingTest()
// ------------------------------------------------------------------------
// BREATHING TEST
//
//  Concerns:
//    Exercise the basic functionality of the component.
//
//  Testing:
//    Basic functionality.
// --------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("BREATHING TEST");

    {
        PV("Default object");

        JournalFileIterator it;
        ASSERT_EQ(false, it.isValid());
        ASSERT_EQ(-1, it.nextRecord());
    }

    {
        PV("Empty file -- forwared iteration");

        MappedFileDescriptor mfd;
        mfd.setMappingSize(1);

        FileHeader          fh;
        JournalFileIterator it(&mfd, fh, false);
        ASSERT_EQ(false, it.isValid());
        ASSERT_EQ(-1, it.nextRecord());
    }

    {
        PV("Empty file -- backward iteration");

        MappedFileDescriptor mfd;
        mfd.setMappingSize(1);

        FileHeader          fh;
        JournalFileIterator it(&mfd, fh, true);
        ASSERT_EQ(false, it.isValid());
        ASSERT_EQ(-1, it.nextRecord());
    }
}

static void test2_forwardIteration()
// ------------------------------------------------------------------------
// FORWARD ITERATION
//
// Testing:
//   Forward iteration with non-zero journal records.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("FORWARD ITERATION");

    unsigned int numRecords = 5000;

    bsls::Types::Uint64 totalSize =
        sizeof(FileHeader) + sizeof(JournalFileHeader) +
        numRecords * FileStoreProtocol::k_JOURNAL_RECORD_SIZE;

    char* p = static_cast<char*>(
        bmqtst::TestHelperUtil::allocator()->allocate(totalSize));
    MemoryBlock block(p, totalSize);
    FileHeader  fileHeader;
    bsls::Types::Uint64 lastRecordPos = 0;
    bsls::Types::Uint64 lastSyncPtPos = 0;

    RecordsListType records(bmqtst::TestHelperUtil::allocator());

    addRecords(&block,
               &fileHeader,
               &lastRecordPos,
               &lastSyncPtPos,
               &records,
               numRecords);

    // Create iterator
    MappedFileDescriptor mfd;
    mfd.setFd(-1);  // invalid fd will suffice.
    mfd.setBlock(block);
    mfd.setFileSize(totalSize);

    JournalFileIterator it(&mfd, fileHeader, false);

    ASSERT_EQ(true, it.isValid());
    ASSERT_EQ(&mfd, it.mappedFileDescriptor());
    ASSERT_EQ(false, it.isReverseMode());
    ASSERT_EQ(lastRecordPos, it.lastRecordPosition());
    ASSERT_EQ(lastSyncPtPos, it.lastSyncPointPosition());

    bsl::list<NodeType>::const_iterator recordIter = records.begin();
    unsigned int                        i          = 0;
    int                                 rc         = 0;
    while ((rc = it.nextRecord()) == 1) {
        ASSERT_EQ_D(i, false, recordIter == records.end());
        ASSERT_EQ_D(i, it.recordIndex(), i);

        assertEqual(it, *recordIter);

        ++i;
        ++recordIter;
    }

    ASSERT_EQ(i, records.size());
    ASSERT_EQ(false, it.isValid());

    bmqtst::TestHelperUtil::allocator()->deallocate(p);
}

static void test3_forwardIterationWithZeroJournalRecords()
// ------------------------------------------------------------------------
// FORWARD ITERATION WITH ZERO JOURNAL RECORDS
//
// Testing:
//   Forward iteration with zero journal records.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("FORWARD ITERATION WITH ZERO JOURNAL"
                                      " RECORDS");

    bsls::Types::Uint64 totalSize = sizeof(FileHeader) +
                                    sizeof(JournalFileHeader);

    char* p = static_cast<char*>(
        bmqtst::TestHelperUtil::allocator()->allocate(totalSize));

    MemoryBlock         block(p, totalSize);
    FileHeader          fileHeader;
    bsls::Types::Uint64 currPos = 0;

    OffsetPtr<FileHeader> fh(block, currPos);
    new (fh.get()) FileHeader();
    currPos += sizeof(FileHeader);

    OffsetPtr<JournalFileHeader> jfh(block, currPos);
    new (jfh.get()) JournalFileHeader();
    currPos += sizeof(JournalFileHeader);

    {
        // Create iterator with values in ctor
        MappedFileDescriptor mfd;
        mfd.setFd(-1);  // invalid fd will suffice.
        mfd.setBlock(block);
        mfd.setFileSize(totalSize);

        JournalFileIterator it(&mfd, *fh, false);
        ASSERT_EQ(true, it.isValid());
        unsigned int numRecords = 0;
        while (it.nextRecord() == 1) {
            ASSERT_EQ_D(numRecords, numRecords, it.recordIndex());
            ++numRecords;
        }

        ASSERT_EQ(0U, numRecords);
    }

    {
        // The same case with advance()
        MappedFileDescriptor mfd;
        mfd.setFd(-1);  // invalid fd will suffice.
        mfd.setBlock(block);
        mfd.setFileSize(totalSize);

        JournalFileIterator it(&mfd, *fh, false);
        ASSERT_EQ(true, it.isValid());
        unsigned int numRecords = 0;
        while (it.advance(10) == 1) {
            ASSERT_EQ_D(numRecords, numRecords, it.recordIndex());
            numRecords += 10;
        }

        ASSERT_EQ(0U, numRecords);
    }

    {
        // Create iterator with values in reset()
        MappedFileDescriptor mfd;
        mfd.setFd(-1);  // invalid fd will suffice.
        mfd.setBlock(block);
        mfd.setFileSize(totalSize);

        JournalFileIterator it;
        ASSERT_EQ(0, it.reset(&mfd, *fh, false));
        ASSERT_EQ(true, it.isValid());
        unsigned int numRecords = 0;
        while (1 == it.nextRecord()) {
            ASSERT_EQ_D(numRecords, numRecords, it.recordIndex());
            ++numRecords;
        }

        ASSERT_EQ(0U, numRecords);
    }

    {
        // The same case with advance()
        MappedFileDescriptor mfd;
        mfd.setFd(-1);  // invalid fd will suffice.
        mfd.setBlock(block);
        mfd.setFileSize(totalSize);

        JournalFileIterator it;
        ASSERT_EQ(0, it.reset(&mfd, *fh, false));
        ASSERT_EQ(true, it.isValid());
        unsigned int numRecords = 0;
        while (1 == it.advance(10)) {
            ASSERT_EQ_D(numRecords, numRecords, it.recordIndex());
            numRecords += 10;
        }

        ASSERT_EQ(0U, numRecords);
    }

    bmqtst::TestHelperUtil::allocator()->deallocate(p);
}

static void test4_backwardIteration()
// ------------------------------------------------------------------------
// BACKWARD ITERATION
//
// Testing:
//   Backward iteration with non-zero journal record
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("BACKWARD ITERATION");

    const unsigned int k_NUM_RECORDS = 5000;

    bsls::Types::Uint64 totalSize =
        sizeof(FileHeader) + sizeof(JournalFileHeader) +
        k_NUM_RECORDS * FileStoreProtocol::k_JOURNAL_RECORD_SIZE;

    char* p = static_cast<char*>(
        bmqtst::TestHelperUtil::allocator()->allocate(totalSize));

    MemoryBlock         block(p, totalSize);
    FileHeader          fileHeader;
    bsls::Types::Uint64 lastRecordPos = 0;
    bsls::Types::Uint64 lastSyncPtPos = 0;
    RecordsListType     records(bmqtst::TestHelperUtil::allocator());

    addRecords(&block,
               &fileHeader,
               &lastRecordPos,
               &lastSyncPtPos,
               &records,
               k_NUM_RECORDS);

    // Create iterator
    MappedFileDescriptor mfd;
    mfd.setFd(-1);  // invalid fd will suffice.
    mfd.setBlock(block);
    mfd.setFileSize(totalSize);

    JournalFileIterator it(&mfd, fileHeader, true);  // backward iteration
    ASSERT_EQ(true, it.isValid());
    ASSERT_EQ(&mfd, it.mappedFileDescriptor());
    ASSERT_EQ(true, it.isReverseMode());
    ASSERT_EQ(lastRecordPos, it.lastRecordPosition());
    ASSERT_EQ(lastSyncPtPos, it.lastSyncPointPosition());

    bsl::list<NodeType>::const_reverse_iterator recordIter = records.rbegin();

    unsigned int i  = 0;
    int          rc = 0;
    while (1 == (rc = it.nextRecord())) {
        ASSERT_EQ_D(i, false, recordIter == records.crend());
        ASSERT_EQ_D(i, it.recordIndex(), k_NUM_RECORDS - i - 1);

        assertEqual(it, *recordIter);

        ++i;
        ++recordIter;
    }

    ASSERT_EQ(i, records.size());
    ASSERT_EQ(false, it.isValid());

    bmqtst::TestHelperUtil::allocator()->deallocate(p);
}

static void test5_backwardIterationWithZeroJournalEntries()
// ------------------------------------------------------------------------
// BACKWARD ITERATION WITH ZERO JOURNAL RECORDS
//
// Testing:
//   Backward iteration with zero journal records.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("BACKWARD ITERATION WITH ZERO JOURNAL"
                                      " RECORDS");

    bsls::Types::Uint64 totalSize = sizeof(FileHeader) +
                                    sizeof(JournalFileHeader);

    char* p = static_cast<char*>(
        bmqtst::TestHelperUtil::allocator()->allocate(totalSize));

    MemoryBlock         block(p, totalSize);
    FileHeader          fileHeader;
    bsls::Types::Uint64 currPos = 0;

    OffsetPtr<FileHeader> fh(block, currPos);
    new (fh.get()) FileHeader();
    currPos += sizeof(FileHeader);

    OffsetPtr<JournalFileHeader> jfh(block, currPos);
    new (jfh.get()) JournalFileHeader();
    currPos += sizeof(JournalFileHeader);

    {
        // Create iterator with values in ctor
        MappedFileDescriptor mfd;
        mfd.setFd(-1);  // invalid fd will suffice.
        mfd.setBlock(block);
        mfd.setFileSize(totalSize);

        JournalFileIterator it(&mfd, fileHeader, true);
        ASSERT_EQ(true, it.isValid());
        unsigned int numRecords = 0;
        while (1 == it.nextRecord()) {
            ++numRecords;
        }

        ASSERT_EQ(0U, numRecords);
    }

    {
        // The same case with advance()
        MappedFileDescriptor mfd;
        mfd.setFd(-1);  // invalid fd will suffice.
        mfd.setBlock(block);
        mfd.setFileSize(totalSize);

        JournalFileIterator it(&mfd, fileHeader, true);
        ASSERT_EQ(true, it.isValid());
        unsigned int numRecords = 0;
        while (1 == it.advance(10)) {
            numRecords += 10;
        }

        ASSERT_EQ(0U, numRecords);
    }

    {
        // Create iterator with values in reset()
        MappedFileDescriptor mfd;
        mfd.setFd(-1);  // invalid fd will suffice.
        mfd.setBlock(block);
        mfd.setFileSize(totalSize);

        JournalFileIterator it;
        ASSERT_EQ(0, it.reset(&mfd, *fh, true));
        ASSERT_EQ(true, it.isValid());
        unsigned int numRecords = 0;
        while (1 == it.nextRecord()) {
            ++numRecords;
        }

        ASSERT_EQ(0U, numRecords);
    }

    {
        // Create iterator with values in reset()
        MappedFileDescriptor mfd;
        mfd.setFd(-1);  // invalid fd will suffice.
        mfd.setBlock(block);
        mfd.setFileSize(totalSize);

        JournalFileIterator it;
        ASSERT_EQ(0, it.reset(&mfd, *fh, true));
        ASSERT_EQ(true, it.isValid());
        unsigned int numRecords = 0;
        while (1 == it.advance(10)) {
            numRecords += 10;
        }

        ASSERT_EQ(0U, numRecords);
    }

    bmqtst::TestHelperUtil::allocator()->deallocate(p);
}

static void test6_forwardIterationOfSparseJournalFileNoRecords()
// --------------------------------------------------------------------
// FORWARD ITERATION OF SPARSE JOURNAL FILE WITH NO RECORDS
//
// Testing:
//   Forward iteration of *sparse* journal file which contains:
//      - FileHeader
//      - JournalFileHeader
//      - No records
//   This simulates broker crash when no records have been written to
//   the journal.
// --------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("FORWARD ITERATION OF SPARSE JOURNAL"
                                      " FILE WITH NO RECORDS");

    bsls::Types::Uint64 totalSize = 1024 * 1024 * 10;  // 10MB sparse file

    char* p = static_cast<char*>(
        bmqtst::TestHelperUtil::allocator()->allocate(totalSize));
    // When this block gets passed to the JournalFileIterator c'tor, it will
    // iterate over the record-set and parse the type of each. To avoid reading
    // uninitialized memory, we zero-out the buffer now.
    bsl::memset(p, 0, totalSize);

    MemoryBlock         block(p, totalSize);
    FileHeader          fileHeader;
    bsls::Types::Uint64 currPos = 0;

    OffsetPtr<FileHeader> fh(block, currPos);
    new (fh.get()) FileHeader();
    currPos += sizeof(FileHeader);

    OffsetPtr<JournalFileHeader> jfh(block, currPos);
    new (jfh.get()) JournalFileHeader();
    currPos += sizeof(JournalFileHeader);

    {
        // Create iterator with values in ctor
        MappedFileDescriptor mfd;
        mfd.setFd(-1);  // invalid fd will suffice.
        mfd.setBlock(block);
        mfd.setFileSize(totalSize);

        JournalFileIterator it(&mfd, *fh, false);
        ASSERT_EQ(true, it.isValid());
        ASSERT_EQ(0ULL, it.lastRecordPosition());

        unsigned int numRecords = 0;
        while (1 == it.nextRecord()) {
            ASSERT_EQ_D(numRecords, numRecords, it.recordIndex());
            ++numRecords;
        }

        ASSERT_EQ(0U, numRecords);
    }

    {
        // The same case with advance()
        MappedFileDescriptor mfd;
        mfd.setFd(-1);  // invalid fd will suffice.
        mfd.setBlock(block);
        mfd.setFileSize(totalSize);

        JournalFileIterator it(&mfd, *fh, false);
        ASSERT_EQ(true, it.isValid());
        ASSERT_EQ(0ULL, it.lastRecordPosition());

        unsigned int numRecords = 0;
        while (1 == it.advance(10)) {
            ASSERT_EQ_D(numRecords, numRecords, it.recordIndex());
            numRecords += 10;
        }

        ASSERT_EQ(0U, numRecords);
    }

    {
        // Create iterator with values in reset()
        MappedFileDescriptor mfd;
        mfd.setFd(-1);  // invalid fd will suffice.
        mfd.setBlock(block);
        mfd.setFileSize(totalSize);

        JournalFileIterator it;
        ASSERT_EQ(0, it.reset(&mfd, *fh, false));
        ASSERT_EQ(true, it.isValid());
        ASSERT_EQ(0ULL, it.lastRecordPosition());

        unsigned int numRecords = 0;
        while (1 == it.nextRecord()) {
            ASSERT_EQ_D(numRecords, numRecords, it.recordIndex());
            ++numRecords;
        }

        ASSERT_EQ(0U, numRecords);
    }

    {
        // The same case with advance()
        MappedFileDescriptor mfd;
        mfd.setFd(-1);  // invalid fd will suffice.
        mfd.setBlock(block);
        mfd.setFileSize(totalSize);

        JournalFileIterator it;
        ASSERT_EQ(0, it.reset(&mfd, *fh, false));
        ASSERT_EQ(true, it.isValid());
        ASSERT_EQ(0ULL, it.lastRecordPosition());

        unsigned int numRecords = 0;
        while (1 == it.advance(10)) {
            ASSERT_EQ_D(numRecords, numRecords, it.recordIndex());
            numRecords += 10;
        }

        ASSERT_EQ(0U, numRecords);
    }

    bmqtst::TestHelperUtil::allocator()->deallocate(p);
}

static void test7_backwardIterationOfSparseJournalFileNoRecords()
// ------------------------------------------------------------------------
// BACKWARD ITERATION OF SPARSE JOURNAL FILE WITH NO RECORDS
//
// Testing:
//   Backward iteration of *sparse* journal file which contains:
//      - FileHeader
//      - JournalFileHeader
//      - No records
//   This simulates broker crash when no records have been written to
//   the journal.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("BACKWARD ITERATION OF SPARSE JOURNAL"
                                      " FILE WITH NO RECORDS");

    bsls::Types::Uint64 totalSize = 1024 * 1024 * 10;  // 10MB sparse file

    char* p = static_cast<char*>(
        bmqtst::TestHelperUtil::allocator()->allocate(totalSize));
    // When this block gets passed to the JournalFileIterator c'tor, it will
    // iterate over the record-set and parse the type of each. To avoid reading
    // uninitialized memory, we zero-out the buffer now.
    bsl::memset(p, 0, totalSize);

    MemoryBlock         block(p, totalSize);
    FileHeader          fileHeader;
    bsls::Types::Uint64 currPos = 0;

    OffsetPtr<FileHeader> fh(block, currPos);
    new (fh.get()) FileHeader();
    currPos += sizeof(FileHeader);

    OffsetPtr<JournalFileHeader> jfh(block, currPos);
    new (jfh.get()) JournalFileHeader();
    currPos += sizeof(JournalFileHeader);

    {
        // Create iterator with values in ctor
        MappedFileDescriptor mfd;
        mfd.setFd(-1);  // invalid fd will suffice.
        mfd.setBlock(block);
        mfd.setFileSize(totalSize);

        JournalFileIterator it(&mfd, *fh, true);
        ASSERT_EQ(true, it.isValid());
        ASSERT_EQ(0ULL, it.lastRecordPosition());

        unsigned int numRecords = 0;
        while (1 == it.nextRecord()) {
            ++numRecords;
        }

        ASSERT_EQ(0U, numRecords);
    }

    {
        // The same case with advance()
        MappedFileDescriptor mfd;
        mfd.setFd(-1);  // invalid fd will suffice.
        mfd.setBlock(block);
        mfd.setFileSize(totalSize);

        JournalFileIterator it(&mfd, *fh, true);
        ASSERT_EQ(true, it.isValid());
        ASSERT_EQ(0ULL, it.lastRecordPosition());

        unsigned int numRecords = 0;
        while (1 == it.advance(10)) {
            numRecords += 10;
        }

        ASSERT_EQ(0U, numRecords);
    }

    {
        // Create iterator with values in reset()
        MappedFileDescriptor mfd;
        mfd.setFd(-1);  // invalid fd will suffice.
        mfd.setBlock(block);
        mfd.setFileSize(totalSize);

        JournalFileIterator it;
        ASSERT_EQ(0, it.reset(&mfd, *fh, true));
        ASSERT_EQ(true, it.isValid());
        ASSERT_EQ(0ULL, it.lastRecordPosition());

        unsigned int numRecords = 0;
        while (1 == it.nextRecord()) {
            ++numRecords;
        }

        ASSERT_EQ(0U, numRecords);
    }

    {
        // The same case with advance()
        MappedFileDescriptor mfd;
        mfd.setFd(-1);  // invalid fd will suffice.
        mfd.setBlock(block);
        mfd.setFileSize(totalSize);

        JournalFileIterator it;
        ASSERT_EQ(0, it.reset(&mfd, *fh, true));
        ASSERT_EQ(true, it.isValid());
        ASSERT_EQ(0ULL, it.lastRecordPosition());

        unsigned int numRecords = 0;
        while (1 == it.advance(10)) {
            numRecords += 10;
        }

        ASSERT_EQ(0U, numRecords);
    }

    bmqtst::TestHelperUtil::allocator()->deallocate(p);
}

static void test8_forwardIterationOfSparseJournalFileWithRecords()
// ------------------------------------------------------------------------
// FORWARD ITERATION OF SPARSE JOURNAL FILE WITH RECORDS
//
// Testing:
//   Forward iteration of *sparse* journal file which contains:
//      - FileHeader
//      - JournalFileHeader
//      - 5432 records
//   This simulates broker crash after some records have been written
//   to the journal.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("FORWARD ITERATION OF SPARSE JOURNAL"
                                      " FILE WITH RECORDS");

    const unsigned int k_NUM_RECORDS = 5432;

    bsls::Types::Uint64 totalSize =
        sizeof(FileHeader) + sizeof(JournalFileHeader) +
        k_NUM_RECORDS * FileStoreProtocol::k_JOURNAL_RECORD_SIZE +
        1024 * 1024 * 10;  // 10MB of sparse area

    char* p = static_cast<char*>(
        bmqtst::TestHelperUtil::allocator()->allocate(totalSize));
    // When this block gets passed to the JournalFileIterator c'tor, it will
    // iterate over the record-set and parse the type of each. To avoid reading
    // uninitialized memory, we zero-out the buffer now.
    bsl::memset(p, 0, totalSize);

    MemoryBlock         block(p, totalSize);
    FileHeader          fileHeader;
    bsls::Types::Uint64 lastRecordPos = 0;
    bsls::Types::Uint64 lastSyncPtPos = 0;

    RecordsListType records(bmqtst::TestHelperUtil::allocator());

    addRecords(&block,
               &fileHeader,
               &lastRecordPos,
               &lastSyncPtPos,
               &records,
               k_NUM_RECORDS);

    // Create iterator
    MappedFileDescriptor mfd;
    mfd.setFd(-1);  // invalid fd will suffice.
    mfd.setBlock(block);
    mfd.setFileSize(totalSize);

    JournalFileIterator it(&mfd, fileHeader, false);
    ASSERT_EQ(true, it.isValid());
    ASSERT_EQ(&mfd, it.mappedFileDescriptor());
    ASSERT_EQ(false, it.isReverseMode());
    ASSERT_EQ(lastRecordPos, it.lastRecordPosition());
    ASSERT_EQ(lastSyncPtPos, it.lastSyncPointPosition());

    bsl::list<NodeType>::const_iterator recordIter = records.begin();
    unsigned int                        i          = 0;
    int                                 rc         = 0;
    while (1 == (rc = it.nextRecord())) {
        ASSERT_EQ_D(i, false, recordIter == records.end());
        ASSERT_EQ_D(i, it.recordIndex(), i);

        assertEqual(it, *recordIter);

        ++i;
        ++recordIter;
    }

    ASSERT_EQ(i, records.size());
    ASSERT_EQ(false, it.isValid());

    bmqtst::TestHelperUtil::allocator()->deallocate(p);
}

static void test9_backwardIterationOfSparseJournalFileWithRecords()
// ------------------------------------------------------------------------
// BACKWARD ITERATION OF SPARSE JOURNAL FILE WITH RECORDS
//
// Testing:
//   Backward iteration of *sparse* journal file which contains:
//      - FileHeader
//      - JournalFileHeader
//      - 5432 records
//   This simulates broker crash after some records have been written
//   to the journal.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("BACKWARD ITERATION OF SPARSE JOURNAL"
                                      " FILE WITH RECORDS");

    const unsigned int k_NUM_RECORDS = 5432;

    bsls::Types::Uint64 totalSize =
        sizeof(FileHeader) + sizeof(JournalFileHeader) +
        k_NUM_RECORDS * FileStoreProtocol::k_JOURNAL_RECORD_SIZE +
        1024 * 1024 * 50;  // 50MB of sparse area

    char* p = static_cast<char*>(
        bmqtst::TestHelperUtil::allocator()->allocate(totalSize));
    // When this block gets passed to the JournalFileIterator c'tor, it will
    // iterate over the record-set and parse the type of each. To avoid reading
    // uninitialized memory, we zero-out the buffer now.
    bsl::memset(p, 0, totalSize);

    MemoryBlock         block(p, totalSize);
    FileHeader          fileHeader;
    bsls::Types::Uint64 lastRecordPos = 0;
    bsls::Types::Uint64 lastSyncPtPos = 0;

    RecordsListType records(bmqtst::TestHelperUtil::allocator());

    addRecords(&block,
               &fileHeader,
               &lastRecordPos,
               &lastSyncPtPos,
               &records,
               k_NUM_RECORDS);

    // Create iterator
    MappedFileDescriptor mfd;
    mfd.setFd(-1);  // invalid fd will suffice.
    mfd.setBlock(block);
    mfd.setFileSize(totalSize);

    JournalFileIterator it(&mfd, fileHeader, true);
    ASSERT_EQ(true, it.isValid());
    ASSERT_EQ(&mfd, it.mappedFileDescriptor());
    ASSERT_EQ(true, it.isReverseMode());
    ASSERT_EQ(lastRecordPos, it.lastRecordPosition());
    ASSERT_EQ(lastSyncPtPos, it.lastSyncPointPosition());

    bsl::list<NodeType>::const_reverse_iterator recordIter = records.rbegin();
    unsigned int                                i          = 0;
    int                                         rc         = 0;
    while (1 == (rc = it.nextRecord())) {
        ASSERT_EQ_D(i, false, recordIter == records.crend());

        ASSERT_EQ_D(i, it.recordIndex(), k_NUM_RECORDS - i - 1);

        assertEqual(it, *recordIter);

        ++i;
        ++recordIter;
    }

    ASSERT_EQ(i, records.size());
    ASSERT_EQ(false, it.isValid());

    bmqtst::TestHelperUtil::allocator()->deallocate(p);
}

static void test10_bidirectionalIteration()
// ------------------------------------------------------------------------
// BIDIRECTIONAL ITERATION
//
// Testing:
//   Bi-directional iteration.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("BIDIRECTIONAL ITERATION");

    unsigned int k_NUM_RECORDS = 5000;

    bsls::Types::Uint64 totalSize =
        sizeof(FileHeader) + sizeof(JournalFileHeader) +
        +k_NUM_RECORDS * FileStoreProtocol::k_JOURNAL_RECORD_SIZE;

    char* p = static_cast<char*>(
        bmqtst::TestHelperUtil::allocator()->allocate(totalSize));

    MemoryBlock         block(p, totalSize);
    FileHeader          fileHeader;
    bsls::Types::Uint64 lastRecordPos = 0;
    bsls::Types::Uint64 lastSyncPtPos = 0;
    RecordsListType     records(bmqtst::TestHelperUtil::allocator());

    addRecords(&block,
               &fileHeader,
               &lastRecordPos,
               &lastSyncPtPos,
               &records,
               k_NUM_RECORDS);

    // Create iterator
    MappedFileDescriptor mfd;
    mfd.setFd(-1);  // invalid fd will suffice.
    mfd.setBlock(block);
    mfd.setFileSize(totalSize);
    JournalFileIterator it(&mfd, fileHeader, false);

    ASSERT_EQ(true, it.hasRecordSizeRemaining());
    ASSERT_EQ(true, it.isValid());
    ASSERT_EQ(&mfd, it.mappedFileDescriptor());
    ASSERT_EQ(false, it.isReverseMode());
    ASSERT_EQ(lastRecordPos, it.lastRecordPosition());
    ASSERT_EQ(lastSyncPtPos, it.lastSyncPointPosition());

    bsl::list<NodeType>::const_iterator recordIter = records.begin();
    const unsigned int                  increment  = k_NUM_RECORDS / 100;

    unsigned int i                  = 0;
    unsigned int currentRecordCount = increment;
    unsigned int count              = currentRecordCount;
    int          rc                 = 0;
    bool         forward            = true;
    while (recordIter != records.end()) {
        ASSERT_EQ_D(i, true, it.hasRecordSizeRemaining());
        ASSERT_EQ_D(i, 1, (rc = it.nextRecord()));

        unsigned int expectedIndex = 0;
        // Determine the expected index based on our iteration direction.
        // At the end, we want to check that we've either reached the
        // beginning of the file or the end of that current block.

        if (forward) {
            if (i > 0 && i % currentRecordCount == 0) {
                expectedIndex = currentRecordCount;
            }
            else {
                expectedIndex = i % currentRecordCount;
            }
        }
        else {
            if (i > 0 && i % currentRecordCount == 0) {
                expectedIndex = 0;
            }
            else {
                expectedIndex = currentRecordCount - (i % currentRecordCount);
            }
        }
        ASSERT_EQ_D(i, expectedIndex, it.recordIndex());

        assertEqual(it, *recordIter);

        if (count == 0) {
            if ((forward = !forward)) {
                currentRecordCount += increment;
                if (currentRecordCount > k_NUM_RECORDS) {
                    break;
                }
            }
            count = currentRecordCount;
            it.flipDirection();
        }

        i++;
        count--;

        if (forward) {
            ++recordIter;
        }
        else {
            --recordIter;
        }
    }

    ASSERT_EQ(false, it.isReverseMode());
    ASSERT_EQ(false, it.hasRecordSizeRemaining());

    bmqtst::TestHelperUtil::allocator()->deallocate(p);
}

static void test11_forwardAdvance()
// ------------------------------------------------------------------------
// FORWARD ITERATION
//
// Testing:
//   Advance.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("FORWARD ADVANCE");

    unsigned int k_NUM_RECORDS = 5001;

    bsls::Types::Uint64 totalSize =
        sizeof(FileHeader) + sizeof(JournalFileHeader) +
        +k_NUM_RECORDS * FileStoreProtocol::k_JOURNAL_RECORD_SIZE;

    char* p = static_cast<char*>(
        bmqtst::TestHelperUtil::allocator()->allocate(totalSize));

    MemoryBlock         block(p, totalSize);
    FileHeader          fileHeader;
    bsls::Types::Uint64 lastRecordPos = 0;
    bsls::Types::Uint64 lastSyncPtPos = 0;
    RecordsListType     records(bmqtst::TestHelperUtil::allocator());

    addRecords(&block,
               &fileHeader,
               &lastRecordPos,
               &lastSyncPtPos,
               &records,
               k_NUM_RECORDS);

    // Create iterator
    MappedFileDescriptor mfd;
    mfd.setFd(-1);  // invalid fd will suffice.
    mfd.setBlock(block);
    mfd.setFileSize(totalSize);
    JournalFileIterator it(&mfd, fileHeader, false);

    ASSERT_EQ(true, it.hasRecordSizeRemaining());
    ASSERT_EQ(true, it.isValid());
    ASSERT_EQ(&mfd, it.mappedFileDescriptor());
    ASSERT_EQ(false, it.isReverseMode());
    ASSERT_EQ(lastRecordPos, it.lastRecordPosition());
    ASSERT_EQ(lastSyncPtPos, it.lastSyncPointPosition());
    ASSERT_EQ(1, it.nextRecord());  // Set iterator to first record position

    bsl::list<NodeType>::const_iterator recordIter = records.cbegin();
    const unsigned int                  increment  = k_NUM_RECORDS / 100;

    unsigned int i  = 0;
    int          rc = 0;
    while (i + 1 < k_NUM_RECORDS) {
        i += increment;
        bsl::advance(recordIter, increment);
        ASSERT_EQ_D(i, true, it.hasRecordSizeRemaining());
        ASSERT_EQ_D(i, 1, (rc = it.advance(increment)));
        ASSERT_EQ_D(i, i, it.recordIndex());

        assertEqual(it, *recordIter);
    }

    // Last one
    ASSERT_EQ(false, it.isReverseMode());
    ASSERT_EQ(false, it.hasRecordSizeRemaining());
    // Not enough bytes remaining to advance
    ASSERT_NE_D(i, 1, (rc = it.advance(increment)));

    bmqtst::TestHelperUtil::allocator()->deallocate(p);
}

static void test12_backwardAdvance()
// ------------------------------------------------------------------------
// BACKWARD ADVANCE
//
// Testing:
//   Advance.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("BACKWARD ADVANCE");

    unsigned int k_NUM_RECORDS = 5001;

    bsls::Types::Uint64 totalSize =
        sizeof(FileHeader) + sizeof(JournalFileHeader) +
        +k_NUM_RECORDS * FileStoreProtocol::k_JOURNAL_RECORD_SIZE;

    char* p = static_cast<char*>(
        bmqtst::TestHelperUtil::allocator()->allocate(totalSize));

    MemoryBlock         block(p, totalSize);
    FileHeader          fileHeader;
    bsls::Types::Uint64 lastRecordPos = 0;
    bsls::Types::Uint64 lastSyncPtPos = 0;
    RecordsListType     records(bmqtst::TestHelperUtil::allocator());

    addRecords(&block,
               &fileHeader,
               &lastRecordPos,
               &lastSyncPtPos,
               &records,
               k_NUM_RECORDS);

    // Create iterator
    MappedFileDescriptor mfd;
    mfd.setFd(-1);  // invalid fd will suffice.
    mfd.setBlock(block);
    mfd.setFileSize(totalSize);
    JournalFileIterator it(&mfd, fileHeader, true);  // backward iteration

    ASSERT_EQ(true, it.hasRecordSizeRemaining());
    ASSERT_EQ(true, it.isValid());
    ASSERT_EQ(&mfd, it.mappedFileDescriptor());
    ASSERT_EQ(true, it.isReverseMode());
    ASSERT_EQ(lastRecordPos, it.lastRecordPosition());
    ASSERT_EQ(lastSyncPtPos, it.lastSyncPointPosition());
    ASSERT_EQ(1, it.nextRecord());  // Set iterator to first record position

    bsl::list<NodeType>::const_reverse_iterator recordIter = records.crbegin();
    const unsigned int increment = k_NUM_RECORDS / 100;

    unsigned int i  = k_NUM_RECORDS - 1;
    int          rc = 0;
    while (i > 1) {
        i -= increment;
        bsl::advance(recordIter, increment);
        ASSERT_EQ_D(i, true, it.hasRecordSizeRemaining());
        ASSERT_EQ_D(i, 1, (rc = it.advance(increment)));
        ASSERT_EQ_D(i, i, it.recordIndex());

        assertEqual(it, *recordIter);
    }

    // Last one
    ASSERT_EQ(true, it.isReverseMode());
    ASSERT_EQ(false, it.hasRecordSizeRemaining());
    // Not enough bytes remaining to advance
    ASSERT_NE_D(i, 1, (rc = it.advance(increment)));

    bmqtst::TestHelperUtil::allocator()->deallocate(p);
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    mqbu::MessageGUIDUtil::initialize();

    switch (_testCase) {
    case 0:
    case 12: test12_backwardAdvance(); break;
    case 11: test11_forwardAdvance(); break;
    case 10: test10_bidirectionalIteration(); break;
    case 9: test9_backwardIterationOfSparseJournalFileWithRecords(); break;
    case 8: test8_forwardIterationOfSparseJournalFileWithRecords(); break;
    case 7: test7_backwardIterationOfSparseJournalFileNoRecords(); break;
    case 6: test6_forwardIterationOfSparseJournalFileNoRecords(); break;
    case 5: test5_backwardIterationWithZeroJournalEntries(); break;
    case 4: test4_backwardIteration(); break;
    case 3: test3_forwardIterationWithZeroJournalRecords(); break;
    case 2: test2_forwardIteration(); break;
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_GBL_ALLOC);
    // NOTE: for some reason the default allcoator verification never
    // succeeds.
}
