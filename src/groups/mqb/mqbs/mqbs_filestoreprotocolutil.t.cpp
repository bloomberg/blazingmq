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

// mqbs_filestoreprotocolutil.t.cpp                                   -*-C++-*-
#include <mqbs_filestoreprotocolutil.h>

// MQB
#include <mqbs_mappedfiledescriptor.h>
#include <mqbs_memoryblock.h>
#include <mqbs_offsetptr.h>

// BDE
#include <bdlbb_pooledblobbufferfactory.h>
#include <bdlf_bind.h>
#include <bsl_string.h>
#include <bsl_unordered_map.h>
#include <bslma_default.h>
#include <bslmt_barrier.h>
#include <bslmt_threadgroup.h>
#include <bsls_timeutil.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// BDE
#include <bsl_limits.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_hasBmqHeader()
// ------------------------------------------------------------------------
// Testing:
//   static int hasBmqHeader(const MemoryBlock& block);
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("HAS BlazingMQ HEADER");

    using namespace mqbs;

    {
        // Empty block
        MemoryBlock          block;
        MappedFileDescriptor mfd;
        mfd.setBlock(block);
        ASSERT_EQ(FileStoreProtocolUtil::hasBmqHeader(mfd), -1);
    }

    {
        // Sufficient minimum bytes for header but magic mismatch
        char* p = static_cast<char*>(
            s_allocator_p->allocate(FileHeader::k_MIN_HEADER_SIZE + 3));
        bsl::memset(p, 0, FileHeader::k_MIN_HEADER_SIZE + 3);
        MemoryBlock          block(p, FileHeader::k_MIN_HEADER_SIZE + 3);
        MappedFileDescriptor mfd;
        mfd.setBlock(block);
        mfd.setFileSize(FileHeader::k_MIN_HEADER_SIZE + 3);
        ASSERT_EQ(-2, FileStoreProtocolUtil::hasBmqHeader(mfd));
        s_allocator_p->deallocate(p);
    }

    {
        // Sufficient bytes as per sizeof(FileHeader) but not
        // header.headerWords()

        char* p = static_cast<char*>(
            s_allocator_p->allocate(sizeof(FileHeader)));
        MemoryBlock          block(p, sizeof(FileHeader));
        MappedFileDescriptor mfd;
        mfd.setBlock(block);
        mfd.setFileSize(sizeof(FileHeader));

        OffsetPtr<FileHeader> fh(block, 0);
        new (fh.get()) FileHeader();

        // Update 'headerWords' field with a value > block.size()
        fh->setHeaderWords(block.size() / 4 + 10);

        ASSERT_EQ(-4, FileStoreProtocolUtil::hasBmqHeader(mfd));
        s_allocator_p->deallocate(p);
    }

    {
        // All good
        char* p = static_cast<char*>(
            s_allocator_p->allocate(sizeof(FileHeader) + 100));
        MemoryBlock          block(p, sizeof(FileHeader) + 100);
        MappedFileDescriptor mfd;
        mfd.setBlock(block);
        mfd.setFileSize(sizeof(FileHeader) + 100);

        OffsetPtr<FileHeader> fh(block, 0);
        new (fh.get()) FileHeader();
        fh->setFileType(FileType::e_JOURNAL);
        ASSERT_EQ(0, FileStoreProtocolUtil::hasBmqHeader(mfd));
        s_allocator_p->deallocate(p);
    }
}

static void test2_lastJournalRecord()
// ------------------------------------------------------------------------
// Testing:
//   lastJournalRecord()
// ------------------------------------------------------------------------
{
    using namespace mqbs;

    {
        // Journal contains FileHeader, JournalHeader and 10 records.  Last
        // record is corrupt.

        const unsigned int  numRecords = 10;
        bsls::Types::Uint64 blockSize =
            sizeof(FileHeader) + sizeof(JournalFileHeader) +
            (numRecords * FileStoreProtocol::k_JOURNAL_RECORD_SIZE);

        char* p = static_cast<char*>(s_allocator_p->allocate(blockSize));

        MemoryBlock          block(p, blockSize);
        MappedFileDescriptor mfd;
        mfd.setBlock(block);
        mfd.setFileSize(blockSize);

        bsls::Types::Uint64   currPos = 0;
        OffsetPtr<FileHeader> fh(block, currPos);
        new (fh.get()) FileHeader();
        currPos += sizeof(FileHeader);

        OffsetPtr<JournalFileHeader> jfh(block, currPos);
        new (jfh.get()) JournalFileHeader();
        currPos += sizeof(JournalFileHeader);

        for (unsigned int i = 0; i < numRecords; ++i) {
            OffsetPtr<DeletionRecord> rec(block, currPos);
            new (rec.get()) DeletionRecord();

            if (i == (numRecords - 1)) {
                // Corrupt last record
                rec->setMagic(123);
            }
            else {
                rec->setMagic(RecordHeader::k_MAGIC);
            }
            currPos += FileStoreProtocol::k_JOURNAL_RECORD_SIZE;
        }

        bsls::Types::Uint64 lastRecordPos =
            currPos - (2 * FileStoreProtocol::k_JOURNAL_RECORD_SIZE);

        ASSERT_EQ(lastRecordPos,
                  FileStoreProtocolUtil::lastJournalRecord(
                      mfd,
                      *fh,
                      *jfh,
                      0));  // last journal sync point

        s_allocator_p->deallocate(p);
    }

    {
        // Last journal sync point *is* the last valid record
        bsls::Types::Uint64 lastJournalSyncPoint = 1000;

        bsls::Types::Uint64 blockSize =
            lastJournalSyncPoint +
            (2 * FileStoreProtocol::k_JOURNAL_RECORD_SIZE) - 1;

        char* p = static_cast<char*>(s_allocator_p->allocate(blockSize));

        MemoryBlock          block(p, blockSize);
        MappedFileDescriptor mfd;
        mfd.setBlock(block);
        mfd.setFileSize(blockSize);
        FileHeader        fh;
        JournalFileHeader jfh;

        ASSERT_EQ(
            lastJournalSyncPoint,
            FileStoreProtocolUtil::lastJournalRecord(mfd,
                                                     fh,
                                                     jfh,
                                                     lastJournalSyncPoint));
        s_allocator_p->deallocate(p);
    }

    {
        // 50 records present after last journal sync point
        bsls::Types::Uint64 lastJournalSyncPoint = 1000;
        unsigned int        numRecords           = 50;

        bsls::Types::Uint64 blockSize =
            lastJournalSyncPoint +
            (numRecords + 1) * FileStoreProtocol::k_JOURNAL_RECORD_SIZE;

        bsls::Types::Uint64 currPos = lastJournalSyncPoint +
                                      FileStoreProtocol::k_JOURNAL_RECORD_SIZE;

        bsls::Types::Uint64 lastValidRecordPos =
            blockSize - FileStoreProtocol::k_JOURNAL_RECORD_SIZE;

        char* p = static_cast<char*>(s_allocator_p->allocate(blockSize));

        MemoryBlock          block(p, blockSize);
        MappedFileDescriptor mfd;
        mfd.setBlock(block);
        mfd.setFileSize(blockSize);
        FileHeader        fh;
        JournalFileHeader jfh;

        // Make all 50 records after 'lastJournalSyncPoint' valid
        for (unsigned int i = 0; i < numRecords; ++i) {
            OffsetPtr<MessageRecord> msgRec(block, currPos);
            new (msgRec.get()) MessageRecord();
            msgRec->setMagic(RecordHeader::k_MAGIC);
            currPos += FileStoreProtocol::k_JOURNAL_RECORD_SIZE;
        }

        bsls::Types::Uint64 result = FileStoreProtocolUtil::lastJournalRecord(
            mfd,
            fh,
            jfh,
            lastJournalSyncPoint);

        ASSERT_EQ(lastValidRecordPos, result);

        s_allocator_p->deallocate(p);
    }

    {
        // Journal contains FileHeader and JournalFileHeader (i.e., no
        // record or syncpt).

        bsls::Types::Uint64 blockSize = sizeof(FileHeader) +
                                        sizeof(JournalFileHeader);

        char* p = static_cast<char*>(s_allocator_p->allocate(blockSize));

        MemoryBlock          block(p, blockSize);
        MappedFileDescriptor mfd;
        mfd.setBlock(block);
        mfd.setFileSize(blockSize);

        bsls::Types::Uint64   currPos = 0;
        OffsetPtr<FileHeader> fh(block, currPos);
        new (fh.get()) FileHeader();
        currPos += sizeof(FileHeader);

        OffsetPtr<JournalFileHeader> jfh(block, currPos);
        new (jfh.get()) JournalFileHeader();
        currPos += sizeof(JournalFileHeader);

        bsls::Types::Uint64 result =
            FileStoreProtocolUtil::lastJournalRecord(mfd, *fh, *jfh, 0);
        ASSERT_EQ(0ULL, result);

        s_allocator_p->deallocate(p);
    }
}

static void test3_lastJournalSyncPoint()
// ------------------------------------------------------------------------
// Testing:
//   lastJournalSyncPoint()
// ------------------------------------------------------------------------
{
    using namespace mqbs;

    {
        // Journal contains FileHeader, JournalHeader and 10 records, but
        // contains no sync points (indicated by journal header)

        const unsigned int  numRecords = 10;
        bsls::Types::Uint64 blockSize =
            sizeof(FileHeader) + sizeof(JournalFileHeader) +
            numRecords * FileStoreProtocol::k_JOURNAL_RECORD_SIZE;

        char* p = static_cast<char*>(s_allocator_p->allocate(blockSize));

        MemoryBlock          block(p, blockSize);
        MappedFileDescriptor mfd;
        mfd.setBlock(block);
        mfd.setFileSize(blockSize);
        bsls::Types::Uint64   currPos = 0;
        OffsetPtr<FileHeader> fh(block, currPos);
        new (fh.get()) FileHeader();
        currPos += sizeof(FileHeader);

        OffsetPtr<JournalFileHeader> jfh(block, currPos);
        new (jfh.get()) JournalFileHeader();
        currPos += sizeof(JournalFileHeader);

        for (unsigned int i = 0; i < numRecords; ++i) {
            OffsetPtr<DeletionRecord> rec(block, currPos);
            new (rec.get()) DeletionRecord();
            rec->setMagic(RecordHeader::k_MAGIC);
            currPos += FileStoreProtocol::k_JOURNAL_RECORD_SIZE;
        }

        ASSERT_EQ(0ULL,
                  FileStoreProtocolUtil::lastJournalSyncPoint(mfd, *fh, *jfh));
        s_allocator_p->deallocate(p);
    }

    {
        // Journal contains FileHeader, JournalHeader and zero records.

        const unsigned int  numRecords = 0;
        bsls::Types::Uint64 blockSize =
            sizeof(FileHeader) + sizeof(JournalFileHeader) +
            numRecords * FileStoreProtocol::k_JOURNAL_RECORD_SIZE;

        char* p = static_cast<char*>(s_allocator_p->allocate(blockSize));

        MemoryBlock          block(p, blockSize);
        MappedFileDescriptor mfd;
        mfd.setBlock(block);
        mfd.setFileSize(blockSize);

        bsls::Types::Uint64   currPos = 0;
        OffsetPtr<FileHeader> fh(block, currPos);
        new (fh.get()) FileHeader();
        currPos += sizeof(FileHeader);

        OffsetPtr<JournalFileHeader> jfh(block, currPos);
        new (jfh.get()) JournalFileHeader();
        currPos += sizeof(JournalFileHeader);

        ASSERT_EQ(0ULL,
                  FileStoreProtocolUtil::lastJournalSyncPoint(mfd, *fh, *jfh));
        s_allocator_p->deallocate(p);
    }

    {
        // Journal contains FileHeader, JournalHeader and 10 records.  7th
        // record is sync point.

        const unsigned int  numRecords = 10;
        bsls::Types::Uint64 blockSize =
            sizeof(FileHeader) + sizeof(JournalFileHeader) +
            numRecords * FileStoreProtocol::k_JOURNAL_RECORD_SIZE;

        char* p = static_cast<char*>(s_allocator_p->allocate(blockSize));

        MemoryBlock          block(p, blockSize);
        MappedFileDescriptor mfd;
        mfd.setBlock(block);
        mfd.setFileSize(blockSize);

        bsls::Types::Uint64   currPos = 0;
        OffsetPtr<FileHeader> fh(block, currPos);
        new (fh.get()) FileHeader();
        currPos += sizeof(FileHeader);

        OffsetPtr<JournalFileHeader> jfh(block, currPos);
        new (jfh.get()) JournalFileHeader();
        currPos += sizeof(JournalFileHeader);

        bsls::Types::Uint64 syncPointPos = 0;

        for (unsigned int i = 0; i < numRecords; ++i) {
            if (6 == i) {
                // 7th record is sync point
                syncPointPos = currPos;
                OffsetPtr<JournalOpRecord> rec(block, currPos);
                new (rec.get()) JournalOpRecord(JournalOpType::e_SYNCPOINT,
                                                SyncPointType::e_REGULAR,
                                                10000,  // seqNum
                                                10,     // primaryNodeId
                                                2,      // primaryLeaseId
                                                80,     // dataFilePos
                                                10,     // qlistFilePos
                                                RecordHeader::k_MAGIC);
                rec->header()
                    .setPrimaryLeaseId(i * 10 + 1)
                    .setSequenceNumber(i * 10 + 10);
            }
            else {
                OffsetPtr<DeletionRecord> rec(block, currPos);
                new (rec.get()) DeletionRecord();
                rec->header()
                    .setPrimaryLeaseId(i * 10 + 1)
                    .setSequenceNumber(i * 10 + 10);
                rec->setMagic(RecordHeader::k_MAGIC);
            }
            currPos += FileStoreProtocol::k_JOURNAL_RECORD_SIZE;
        }

        ASSERT_EQ(syncPointPos,
                  FileStoreProtocolUtil::lastJournalSyncPoint(mfd, *fh, *jfh));
        s_allocator_p->deallocate(p);
    }

    {
        // Journal contains FileHeader, JournalHeader and 1 record.  The
        // only record present is *not* sync point

        const unsigned int  numRecords = 1;
        bsls::Types::Uint64 blockSize =
            sizeof(FileHeader) + sizeof(JournalFileHeader) +
            numRecords * FileStoreProtocol::k_JOURNAL_RECORD_SIZE;

        char* p = static_cast<char*>(s_allocator_p->allocate(blockSize));

        MemoryBlock          block(p, blockSize);
        MappedFileDescriptor mfd;
        mfd.setBlock(block);
        mfd.setFileSize(blockSize);

        bsls::Types::Uint64   currPos = 0;
        OffsetPtr<FileHeader> fh(block, currPos);
        new (fh.get()) FileHeader();
        currPos += sizeof(FileHeader);

        OffsetPtr<JournalFileHeader> jfh(block, currPos);
        new (jfh.get()) JournalFileHeader();
        currPos += sizeof(JournalFileHeader);

        for (unsigned int i = 0; i < numRecords; ++i) {
            // Can be any record except sync point
            OffsetPtr<DeletionRecord> rec(block, currPos);
            new (rec.get()) DeletionRecord();
            rec->setMagic(RecordHeader::k_MAGIC);
            currPos += FileStoreProtocol::k_JOURNAL_RECORD_SIZE;
        }

        ASSERT_EQ(0ULL,
                  FileStoreProtocolUtil::lastJournalSyncPoint(mfd, *fh, *jfh));
        s_allocator_p->deallocate(p);
    }

    {
        // Journal contains FileHeader, JournalHeader and 1 record.  The
        // only record present *is* sync point

        const unsigned int  numRecords = 1;
        bsls::Types::Uint64 blockSize =
            sizeof(FileHeader) + sizeof(JournalFileHeader) +
            numRecords * FileStoreProtocol::k_JOURNAL_RECORD_SIZE;

        char* p = static_cast<char*>(s_allocator_p->allocate(blockSize));

        MemoryBlock          block(p, blockSize);
        MappedFileDescriptor mfd;
        mfd.setBlock(block);
        mfd.setFileSize(blockSize);

        bsls::Types::Uint64   currPos = 0;
        OffsetPtr<FileHeader> fh(block, currPos);
        new (fh.get()) FileHeader();
        currPos += sizeof(FileHeader);

        OffsetPtr<JournalFileHeader> jfh(block, currPos);
        new (jfh.get()) JournalFileHeader();
        currPos += sizeof(JournalFileHeader);

        bsls::Types::Uint64 syncPointPos = 0;
        for (unsigned int i = 0; i < numRecords; ++i) {
            // Must be sync point record
            OffsetPtr<JournalOpRecord> rec(block, currPos);
            new (rec.get()) JournalOpRecord(JournalOpType::e_SYNCPOINT,
                                            SyncPointType::e_REGULAR,
                                            10000,  // seqNum
                                            10,     // leaderTerm
                                            2,      // leaderNodeId
                                            80,     // DataFilePos
                                            20,     // QlistFilePos
                                            RecordHeader::k_MAGIC);
            rec->header()
                .setPrimaryLeaseId(i * 10 + 1)
                .setSequenceNumber(i * 10 + 10);
            syncPointPos = currPos;
            currPos += FileStoreProtocol::k_JOURNAL_RECORD_SIZE;
        }

        ASSERT_EQ(syncPointPos,
                  FileStoreProtocolUtil::lastJournalSyncPoint(mfd, *fh, *jfh));
        s_allocator_p->deallocate(p);
    }
}

static void test4_loadAppIdKeyPairs()
// ------------------------------------------------------------------------
// Testing:
//   loadAppIdKeyPairs()
// ------------------------------------------------------------------------
{
    typedef bsl::pair<bsl::string, mqbu::StorageKey> AppIdKeyPair;
    typedef bsl::vector<AppIdKeyPair>                AppIdKeyPairs;

    {
        // No appIds.

        char*             p = static_cast<char*>(s_allocator_p->allocate(1));
        mqbs::MemoryBlock mb(p, 1);
        AppIdKeyPairs     appIdKeyPairs(s_allocator_p);

        mqbs::FileStoreProtocolUtil::loadAppIdKeyPairs(&appIdKeyPairs,
                                                       mb,
                                                       0);  // no appIds

        ASSERT_EQ(0u, appIdKeyPairs.size());

        s_allocator_p->deallocate(p);
    }

    {
        // 1 appId/appKey pair.

        const size_t appIdLen        = 13;
        const size_t paddedAppIdLen  = 16;
        const size_t numPaddingBytes = 3;
        const size_t totalSize = sizeof(mqbs::AppIdHeader) + paddedAppIdLen +
                                 mqbs::FileStoreProtocol::k_HASH_LENGTH;
        char*  p      = static_cast<char*>(s_allocator_p->allocate(totalSize));
        size_t offset = 0;

        // Append AppIdHeader.
        mqbs::AppIdHeader header;
        header.setAppIdLengthWords(paddedAppIdLen / 4);
        bsl::memcpy(p,
                    reinterpret_cast<const char*>(&header),
                    sizeof(mqbs::AppIdHeader));
        offset += sizeof(mqbs::AppIdHeader);

        // Append AppId.
        bsl::string appId(appIdLen, 'g', s_allocator_p);
        bsl::memcpy(p + offset, appId.c_str(), appIdLen);
        offset += appIdLen;

        // Append AppId padding.
        bmqp::ProtocolUtil::appendPaddingRaw(p + offset, numPaddingBytes);
        offset += numPaddingBytes;

        // Append AppKey (app hash to be precise).
        mqbu::StorageKey appKey(mqbu::StorageKey::BinaryRepresentation(),
                                "abcde");

        char appHash[mqbs::FileStoreProtocol::k_HASH_LENGTH] = {0};
        bsl::memcpy(appHash,
                    appKey.data(),
                    mqbu::StorageKey::e_KEY_LENGTH_BINARY);

        bsl::memcpy(p + offset,
                    appHash,
                    mqbs::FileStoreProtocol::k_HASH_LENGTH);

        // Test.
        mqbs::MemoryBlock mb(p, totalSize);
        AppIdKeyPairs     appIdKeyPairs(s_allocator_p);

        mqbs::FileStoreProtocolUtil::loadAppIdKeyPairs(&appIdKeyPairs,
                                                       mb,
                                                       1);  // 1 appId

        ASSERT_EQ(1U, appIdKeyPairs.size());
        ASSERT_EQ(appId, appIdKeyPairs[0].first);
        ASSERT_EQ(appKey, appIdKeyPairs[0].second);

        s_allocator_p->deallocate(p);
    }

    {
        // 6 appId/appKey pair.
        const int numAppIds = 6;
        size_t    totalSize = 0;

        bsl::vector<size_t> appIdLenVec(s_allocator_p);
        bsl::vector<size_t> paddedAppIdLenVec(s_allocator_p);
        bsl::vector<size_t> numPaddingBytesVec(s_allocator_p);

        for (int n = 0; n < numAppIds; ++n) {
            size_t appIdLen = (n + 1) * 9 + 3;
            appIdLenVec.push_back(appIdLen);  // "random" value

            int    numPaddingBytes = 0;
            size_t paddedAppIdLen  = 4 *
                                    bmqp::ProtocolUtil::calcNumWordsAndPadding(
                                        &numPaddingBytes,
                                        appIdLen);
            paddedAppIdLenVec.push_back(paddedAppIdLen);
            numPaddingBytesVec.push_back(numPaddingBytes);

            totalSize += sizeof(mqbs::AppIdHeader) + paddedAppIdLen +
                         mqbs::FileStoreProtocol::k_HASH_LENGTH;
        }

        char*  p      = static_cast<char*>(s_allocator_p->allocate(totalSize));
        size_t offset = 0;
        bsl::vector<bsl::string>      expectedAppIds(s_allocator_p);
        bsl::vector<mqbu::StorageKey> expectedAppKeys(s_allocator_p);

        for (int n = 0; n < numAppIds; ++n) {
            // Append AppIdHeader.

            mqbs::MemoryBlock block(p + offset, sizeof(mqbs::AppIdHeader));
            mqbs::OffsetPtr<mqbs::AppIdHeader> headerPtr(block, 0);

            new (headerPtr.get()) mqbs::AppIdHeader;

            headerPtr->setAppIdLengthWords(paddedAppIdLenVec[n] / 4);
            offset += sizeof(mqbs::AppIdHeader);

            // Append AppId.

            bsl::string appId(appIdLenVec[n],
                              static_cast<char>(n + 1),
                              s_allocator_p);
            bsl::memcpy(p + offset, appId.c_str(), appIdLenVec[n]);
            offset += appIdLenVec[n];
            expectedAppIds.push_back(appId);

            // Append AppId padding.
            bmqp::ProtocolUtil::appendPaddingRaw(p + offset,
                                                 numPaddingBytesVec[n]);
            offset += numPaddingBytesVec[n];

            // Append AppKey (app hash to be precise).
            char appKeyBuf[mqbu::StorageKey::e_KEY_LENGTH_BINARY] = {
                static_cast<char>(n + 1)};

            mqbu::StorageKey appKey(mqbu::StorageKey::BinaryRepresentation(),
                                    appKeyBuf);

            char appHash[mqbs::FileStoreProtocol::k_HASH_LENGTH] = {0};
            bsl::memcpy(appHash,
                        appKey.data(),
                        mqbu::StorageKey::e_KEY_LENGTH_BINARY);

            bsl::memcpy(p + offset,
                        appHash,
                        mqbs::FileStoreProtocol::k_HASH_LENGTH);
            expectedAppKeys.push_back(appKey);
            offset += mqbs::FileStoreProtocol::k_HASH_LENGTH;
        }

        // Test.
        mqbs::MemoryBlock mb(p, totalSize);
        AppIdKeyPairs     appIdKeyPairs(s_allocator_p);

        mqbs::FileStoreProtocolUtil::loadAppIdKeyPairs(&appIdKeyPairs,
                                                       mb,
                                                       numAppIds);

        ASSERT_EQ(static_cast<size_t>(numAppIds), appIdKeyPairs.size());

        for (size_t n = 0; n < appIdKeyPairs.size(); ++n) {
            ASSERT_EQ_D(n, expectedAppIds[n], appIdKeyPairs[n].first);
            ASSERT_EQ_D(n, expectedAppKeys[n], appIdKeyPairs[n].second);
        }

        s_allocator_p->deallocate(p);
    }
}

namespace {

typedef bsl::unordered_map<bsl::string, bsl::string> Results;
typedef Results::const_iterator                      ResultsIt;
const bsl::size_t                                    MD5_DIGEST_BYTES = 16;

static void bytesFromHex(bsl::string* destination, const bsl::string& source)
// ------------------------------------------------------------------------
// Transforms Hex numbers from the specified 'source' string to bytes and
// stores them in the specified 'destination' string.  Each pair of
// characters from 'source' will be interpreted like hexadecimal number and
// stored into 'destination' as a single char.  If length of 'source' is an
// odd number, the last character will be ignored.
// ------------------------------------------------------------------------
{
    destination->clear();
    destination->reserve(source.size() / 2);
    bsl::string::const_iterator src = source.begin();
    while (src != source.end() && (src + 1) != source.end()) {
        bslstl::StringRef sub(src, src + 2);
        destination->push_back(static_cast<char>(bsl::stoi(sub, 0, 16)));
        src += 2;
    }
}

static void jobForThreadPool(const Results* testData, bslmt::Barrier* barrier)
// ------------------------------------------------------------------------
// Calculates MD5 hashes from keys of the specified 'testData' object and
// compares them with corresponding values.  The specified 'barrier' is
// used to start all the jobs simultaneously in the different threads.
// ------------------------------------------------------------------------
{
    bdlbb::PooledBlobBufferFactory factory(1024, s_allocator_p);
    bmqu::BlobPosition             startPos;

    barrier->wait();
    for (int i = 0; i < 1000; ++i) {
        for (ResultsIt r = testData->begin(); r != testData->end(); ++r) {
            const bsl::string& source = r->first;
            const bsl::string& hex    = r->second;
            bsl::string        expected(s_allocator_p);
            bytesFromHex(&expected, hex);

            bdlbb::Blob localBlob(&factory, s_allocator_p);
            localBlob.setLength(source.size());
            bsl::memcpy(localBlob.buffer(0).data(),
                        source.c_str(),
                        source.size());
            bdlde::Md5::Md5Digest buffer;

            int rc = -1;
            ASSERT_PASS(rc = mqbs::FileStoreProtocolUtil::calculateMd5Digest(
                            &buffer,
                            localBlob,
                            startPos,
                            source.size()));
            ASSERT_EQ(0, rc);
            bsl::string result(buffer.buffer(),
                               MD5_DIGEST_BYTES,
                               s_allocator_p);
            ASSERT_EQ(result, expected);
        }
    }
}

}  // close unnamed namespace

static void test5_calculateMd5Digest()
// ------------------------------------------------------------------------
// Testing:
//   calculateMd5Digest()
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("CALCULATE MD5 DIGEST");

    bsl::string data("12345678901234567890", s_allocator_p);
    bsl::string md5("fd85e62d9beb45428771ec688418b271", s_allocator_p);
    bsl::string md5b(s_allocator_p);
    bytesFromHex(&md5b, md5);

    bmqu::BlobPosition             startPos;
    bdlbb::PooledBlobBufferFactory myFactory(1024, s_allocator_p);
    bdlbb::Blob                    blob(&myFactory, s_allocator_p);
    blob.setLength(data.size());
    bsl::memcpy(blob.buffer(0).data(), data.c_str(), data.size());

    {
        // Empty buffer.
        ASSERT_FAIL(
            mqbs::FileStoreProtocolUtil::calculateMd5Digest(0,
                                                            blob,
                                                            startPos,
                                                            data.size()));
    }

    {
        // Empty blob.
        bdlde::Md5::Md5Digest buffer;

        int rc = mqbs::FileStoreProtocolUtil::calculateMd5Digest(
            &buffer,
            bdlbb::Blob(&myFactory, s_allocator_p),
            startPos,
            data.size());
        ASSERT_NE(0, rc);
    }

    {
        // startPos out of blob.
        bdlde::Md5::Md5Digest buffer;
        ASSERT_FAIL(mqbs::FileStoreProtocolUtil::calculateMd5Digest(
            &buffer,
            blob,
            bmqu::BlobPosition(0, data.size() + 1),
            data.size()));
        ASSERT_FAIL(mqbs::FileStoreProtocolUtil::calculateMd5Digest(
            &buffer,
            blob,
            bmqu::BlobPosition(1, 1),
            data.size()));
    }

    {
        // Zero Length.
        bdlde::Md5::Md5Digest buffer;
        ASSERT_FAIL(mqbs::FileStoreProtocolUtil::calculateMd5Digest(&buffer,
                                                                    blob,
                                                                    startPos,
                                                                    0));
    }

    {
        // More than blob Length.
        bdlde::Md5::Md5Digest buffer;
        bool                  failed = false;
        try {
            mqbs::FileStoreProtocolUtil::calculateMd5Digest(&buffer,
                                                            blob,
                                                            startPos,
                                                            data.size() + 1);
        }
        catch (const BloombergLP::bsls::AssertTestException&) {
            failed = true;
        }
        ASSERT_EQ(true, failed);
    }

    {
        // Correct MD5
        bdlde::Md5::Md5Digest buffer;

        int rc = mqbs::FileStoreProtocolUtil::calculateMd5Digest(&buffer,
                                                                 blob,
                                                                 startPos,
                                                                 data.size());
        ASSERT_EQ(0, rc);
        bsl::string result(buffer.buffer(), MD5_DIGEST_BYTES, s_allocator_p);
        ASSERT_EQ(result, md5b);
    }

    {
        // Correct MD5 in few buffers
        const bsl::size_t bufferSize   = data.size() / 3;
        int               expected_num = data.size() / bufferSize;
        if (data.size() % bufferSize > 0) {
            ++expected_num;
        }
        bdlbb::PooledBlobBufferFactory myLittleFactory(bufferSize,
                                                       s_allocator_p);
        bdlbb::Blob chunkedBlob(&myLittleFactory, s_allocator_p);
        chunkedBlob.setLength(data.size());
        const int num = chunkedBlob.numBuffers();

        ASSERT_EQ(expected_num, num);
        for (int index = 0; index < num; ++index) {
            bsl::memcpy(chunkedBlob.buffer(index).data(),
                        data.c_str() + index * bufferSize,
                        bsl::min(bufferSize,
                                 data.size() - index * bufferSize));
        }

        bdlde::Md5::Md5Digest buffer;

        int rc = mqbs::FileStoreProtocolUtil::calculateMd5Digest(&buffer,
                                                                 chunkedBlob,
                                                                 startPos,
                                                                 data.size());
        ASSERT_EQ(0, rc);
        bsl::string result(buffer.buffer(), MD5_DIGEST_BYTES, s_allocator_p);
        ASSERT_EQ(result, md5b);
    }

    {
        // Concurrency
        Results correctMd5s(s_allocator_p);
        correctMd5s.emplace(bsl::string("12345678901234567890", s_allocator_p),
                            bsl::string("fd85e62d9beb45428771ec688418b271",
                                        s_allocator_p));
        correctMd5s.emplace(bsl::string("15646546656965165468", s_allocator_p),
                            bsl::string("858d54d19e406ddf8442ece8340b41b9",
                                        s_allocator_p));
        correctMd5s.emplace(bsl::string("87849898267587856598", s_allocator_p),
                            bsl::string("dca72b4fe4319fdd40ae2153dac62c51",
                                        s_allocator_p));
        correctMd5s.emplace(bsl::string("77777777777777777777", s_allocator_p),
                            bsl::string("11f733c0934d3ec1977bf99eceaecdbb",
                                        s_allocator_p));
        correctMd5s.emplace(bsl::string("asdfdasfg456456d4489", s_allocator_p),
                            bsl::string("a5e05fd2340789b7ecf2c469e9e6fec7",
                                        s_allocator_p));
        const bsl::size_t numThreads = correctMd5s.size();

        bslmt::ThreadGroup threadGroup(s_allocator_p);
        bslmt::Barrier     barrier(numThreads + 1);

        for (bsl::size_t i = 0; i < numThreads; ++i) {
            int rc = threadGroup.addThread(
                bdlf::BindUtil::bindS(s_allocator_p,
                                      &jobForThreadPool,
                                      &correctMd5s,
                                      &barrier));
            ASSERT_EQ(0, rc);
        }
        barrier.wait();
        threadGroup.joinAll();
    }
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    // One time app initialization.
    bsls::TimeUtil::initialize();

    switch (_testCase) {
    case 0:
    case 5: test5_calculateMd5Digest(); break;
    case 4: test4_loadAppIdKeyPairs(); break;
    case 3: test3_lastJournalSyncPoint(); break;
    case 2: test2_lastJournalRecord(); break;
    case 1: test1_hasBmqHeader(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        s_testStatus = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_ALLOC);
}
