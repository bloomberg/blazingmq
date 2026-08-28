// Copyright 2026 Bloomberg Finance L.P.
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

#include <mqbs_filestoreutil.h>

// MQB
#include <mqbi_storage.h>
#include <mqbs_filestoreprotocol.h>
#include <mqbs_mappedfiledescriptor.h>
#include <mqbs_memoryblock.h>
#include <mqbs_offsetptr.h>
#include <mqbu_storagekey.h>

// BMQ
#include <bmqp_protocolutil.h>
#include <bmqu_blob.h>

// BDE
#include <bdlbb_blob.h>
#include <bdlbb_blobutil.h>
#include <bdlbb_pooledblobbufferfactory.h>
#include <bsl_cstring.h>
#include <bsl_string.h>
#include <bsl_vector.h>
#include <bsls_types.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------

namespace {

/// Size of the buffers backing the fabricated JOURNAL and QLIST files, at
/// least as large as any declared `fileSize` so that a test shrinking the
/// latter leaves slack for an unchecked write to land in.
const int k_FILE_BUFFER_SIZE = 4096;

/// Write at the specified `buffer` a QLIST queue record for the specified
/// `uri` holding the specified `appIds`, and return the length (in bytes)
/// of the record.  The behavior is undefined unless `buffer` has room for the
/// record.
unsigned int buildQueueRecord(char*                           buffer,
                              const bslstl::StringRef&        uri,
                              const bsl::vector<bsl::string>& appIds)
{
    bslma::Allocator* alloc = bmqtst::TestHelperUtil::allocator();

    int                uriPadding = 0;
    const unsigned int paddedUriLen =
        bmqp::Protocol::k_WORD_SIZE *
        bmqp::ProtocolUtil::calcNumWordsAndPadding(&uriPadding, uri.length());

    unsigned int recordLen = sizeof(mqbs::QueueRecordHeader) + paddedUriLen +
                             mqbs::FileStoreProtocol::k_HASH_LENGTH;

    bsl::vector<int>          appIdPaddings(alloc);
    bsl::vector<unsigned int> paddedAppIdLens(alloc);
    for (size_t i = 0; i < appIds.size(); ++i) {
        int                padding = 0;
        const unsigned int paddedLen =
            bmqp::Protocol::k_WORD_SIZE *
            bmqp::ProtocolUtil::calcNumWordsAndPadding(&padding,
                                                       appIds[i].length());
        appIdPaddings.push_back(padding);
        paddedAppIdLens.push_back(paddedLen);

        recordLen += sizeof(mqbs::AppIdHeader) + paddedLen +
                     mqbs::FileStoreProtocol::k_HASH_LENGTH;
    }
    recordLen += sizeof(unsigned int);  // Magic word

    bsl::memset(buffer, 0, recordLen);

    // QueueRecordHeader
    mqbs::MemoryBlock                        recordBlock(buffer, recordLen);
    mqbs::OffsetPtr<mqbs::QueueRecordHeader> qrh(recordBlock, 0);
    new (qrh.get()) mqbs::QueueRecordHeader();
    qrh->setQueueUriLengthWords(paddedUriLen / bmqp::Protocol::k_WORD_SIZE)
        .setNumAppIds(appIds.size())
        .setQueueRecordWords(recordLen / bmqp::Protocol::k_WORD_SIZE);

    unsigned int offset = sizeof(mqbs::QueueRecordHeader);

    // Padded QueueUri, followed by its (unused by the parser) hash.
    bsl::memcpy(buffer + offset, uri.data(), uri.length());
    offset += uri.length();
    bmqp::ProtocolUtil::appendPaddingRaw(buffer + offset, uriPadding);
    offset += uriPadding;
    offset += mqbs::FileStoreProtocol::k_HASH_LENGTH;

    // AppIdHeader, padded AppId and AppKey for each appId.
    for (size_t i = 0; i < appIds.size(); ++i) {
        mqbs::MemoryBlock                  appIdBlock(buffer + offset,
                                     sizeof(mqbs::AppIdHeader));
        mqbs::OffsetPtr<mqbs::AppIdHeader> aih(appIdBlock, 0);
        new (aih.get()) mqbs::AppIdHeader();
        aih->setAppIdLengthWords(paddedAppIdLens[i] /
                                 bmqp::Protocol::k_WORD_SIZE);
        offset += sizeof(mqbs::AppIdHeader);

        bsl::memcpy(buffer + offset, appIds[i].c_str(), appIds[i].length());
        offset += appIds[i].length();
        bmqp::ProtocolUtil::appendPaddingRaw(buffer + offset,
                                             appIdPaddings[i]);
        offset += appIdPaddings[i];

        const mqbu::StorageKey appKey(mqbu::StorageKey::BinaryRepresentation(),
                                      "abcde");
        bsl::memcpy(buffer + offset,
                    appKey.data(),
                    mqbu::StorageKey::e_KEY_LENGTH_BINARY);
        offset += mqbs::FileStoreProtocol::k_HASH_LENGTH;
    }

    // Magic word
    mqbs::MemoryBlock magicBlock(buffer + offset, sizeof(unsigned int));
    mqbs::OffsetPtr<bdlb::BigEndianUint32> magic(magicBlock, 0);
    *magic = mqbs::QueueRecordHeader::k_MAGIC;
    offset += sizeof(unsigned int);

    BSLS_ASSERT_OPT(offset == recordLen);

    return recordLen;
}

/// Write at the specified `buffer` a QueueOp journal record of type
/// `e_CREATION` for a QLIST record located at the specified `qlistOffset`,
/// and return the length (in bytes) of the record.  The behavior is
/// undefined unless `buffer` has room for a journal record.
unsigned int buildQueueOpRecord(char* buffer, bsls::Types::Uint64 qlistOffset)
{
    bsl::memset(buffer, 0, mqbs::FileStoreProtocol::k_JOURNAL_RECORD_SIZE);

    mqbs::MemoryBlock recordBlock(
        buffer,
        mqbs::FileStoreProtocol::k_JOURNAL_RECORD_SIZE);
    mqbs::OffsetPtr<mqbs::QueueOpRecord> rec(recordBlock, 0);
    new (rec.get()) mqbs::QueueOpRecord();

    rec->header()
        .setType(mqbs::RecordType::e_QUEUE_OP)
        .setPrimaryLeaseId(1U)
        .setSequenceNumber(1U);
    rec->setType(mqbs::QueueOpType::e_CREATION)
        .setQueueKey(mqbu::StorageKey(mqbu::StorageKey::BinaryRepresentation(),
                                      "12345"))
        .setQueueUriRecordOffsetWords(qlistOffset /
                                      bmqp::Protocol::k_WORD_SIZE)
        .setMagic(mqbs::RecordHeader::k_MAGIC);

    return mqbs::FileStoreProtocol::k_JOURNAL_RECORD_SIZE;
}

/// Scaffolding for a single `writeQueueCreationRecordImpl` invocation: a
/// fabricated JOURNAL and QLIST mapped file pair, and the replication event
/// holding a QueueOp journal record immediately followed by its QLIST queue
/// record.
class Tester {
  private:
    // DATA
    bslma::Allocator* d_allocator_p;

    bsl::vector<char> d_journalBuffer;

    bsl::vector<char> d_qlistBuffer;

    mqbs::MappedFileDescriptor d_journal;

    mqbs::MappedFileDescriptor d_qlist;

    bdlbb::PooledBlobBufferFactory d_bufferFactory;

    bdlbb::Blob d_event;

    unsigned int d_queueRecordLen;

  public:
    // CREATORS

    /// Create a tester whose event carries a well-formed QLIST record
    /// holding the specified `appIds`.
    Tester(const bsl::vector<bsl::string>& appIds, bslma::Allocator* allocator)
    : d_allocator_p(allocator)
    , d_journalBuffer(k_FILE_BUFFER_SIZE, '\0', allocator)
    , d_qlistBuffer(k_FILE_BUFFER_SIZE, '\0', allocator)
    , d_journal()
    , d_qlist()
    , d_bufferFactory(1024, allocator)
    , d_event(&d_bufferFactory, allocator)
    , d_queueRecordLen(0)
    {
        d_journal.setFd(1)
            .setFileSize(k_FILE_BUFFER_SIZE)
            .setBlock(
                mqbs::MemoryBlock(&d_journalBuffer[0], k_FILE_BUFFER_SIZE))
            .setMapping(&d_journalBuffer[0])
            .setMappingSize(k_FILE_BUFFER_SIZE);

        d_qlist.setFd(2)
            .setFileSize(k_FILE_BUFFER_SIZE)
            .setBlock(mqbs::MemoryBlock(&d_qlistBuffer[0], k_FILE_BUFFER_SIZE))
            .setMapping(&d_qlistBuffer[0])
            .setMappingSize(k_FILE_BUFFER_SIZE);

        const char k_URI[] = "bmq://bmq.test.persistent.priority/testq123";
        bsl::vector<char>  raw(k_FILE_BUFFER_SIZE, '\0', allocator);
        const unsigned int journalRecLen = buildQueueOpRecord(&raw[0], 0);
        d_queueRecordLen = buildQueueRecord(&raw[0] + journalRecLen,
                                            k_URI,
                                            appIds);

        bdlbb::BlobUtil::append(&d_event,
                                &raw[0],
                                journalRecLen + d_queueRecordLen);
    }

    // MANIPULATORS

    /// Return the header of the QLIST record carried by the event, so that
    /// a test can corrupt one of its fields.
    mqbs::QueueRecordHeader& queueRecordHeader()
    {
        bmqu::BlobPosition pos;
        const int          rc = bmqu::BlobUtil::findOffsetSafe(
            &pos,
            d_event,
            bmqu::BlobPosition(0, 0),
            mqbs::FileStoreProtocol::k_JOURNAL_RECORD_SIZE);
        BSLS_ASSERT_OPT(0 == rc);

        return *reinterpret_cast<mqbs::QueueRecordHeader*>(
            d_event.buffer(pos.buffer()).data() + pos.byte());
    }

    /// Return a modifiable reference to the byte at the specified `offset`
    /// within the QLIST record carried by the event.
    char& queueRecordByte(unsigned int offset)
    {
        return *(reinterpret_cast<char*>(&queueRecordHeader()) + offset);
    }

    /// Declare the QLIST mapped file to be of the specified `size` bytes,
    /// leaving its (larger) backing buffer untouched.
    void setQlistFileSize(bsls::Types::Uint64 size)
    {
        d_qlist.setFileSize(size);
    }

    /// Invoke `writeQueueCreationRecordImpl` on the event and return its
    /// result code.
    int write()
    {
        bsls::Types::Uint64     journalPos   = 0;
        bsls::Types::Uint64     qlistFilePos = 0;
        mqbi::Storage::AppInfos appIdKeyPairs(d_allocator_p);

        return mqbs::FileStoreUtil::writeQueueCreationRecordImpl(
            &journalPos,
            &qlistFilePos,
            &appIdKeyPairs,
            1,  // partitionId
            d_event,
            bmqu::BlobPosition(0, 0),
            d_journal,
            true,  // qListAware
            d_qlist);
    }

    // ACCESSORS

    /// Return the length (in bytes) of the QLIST record carried by the
    /// event.
    unsigned int queueRecordLen() const { return d_queueRecordLen; }

    /// Return true if nothing has been written to the JOURNAL file.
    bool isJournalPristine() const
    {
        for (size_t i = 0; i < d_journalBuffer.size(); ++i) {
            if ('\0' != d_journalBuffer[i]) {
                return false;  // RETURN
            }
        }

        return true;
    }
};

}  // close unnamed namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_writeQueueCreationRecordImpl()
// ------------------------------------------------------------------------
// WRITE QUEUE CREATION RECORD IMPL
//
// Concerns:
//   A well-formed QLIST queue record is applied.
//
// Testing:
//   writeQueueCreationRecordImpl()
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("WRITE QUEUE CREATION RECORD IMPL");

    bslma::Allocator* alloc = bmqtst::TestHelperUtil::allocator();

    {
        // No appIds.
        bsl::vector<bsl::string> appIds(alloc);
        Tester                   tester(appIds, alloc);

        BMQTST_ASSERT_EQ(0, tester.write());
        BMQTST_ASSERT(!tester.isJournalPristine());
    }

    {
        // Two appIds.
        bsl::vector<bsl::string> appIds(alloc);
        appIds.emplace_back("foo");
        appIds.emplace_back("barbazqux");

        Tester tester(appIds, alloc);

        BMQTST_ASSERT_EQ(0, tester.write());
        BMQTST_ASSERT(!tester.isJournalPristine());
    }
}

static void test2_writeQueueCreationRecordImplMalformed()
// ------------------------------------------------------------------------
// WRITE QUEUE CREATION RECORD IMPL MALFORMED
//
// Concerns:
//   A QLIST queue record whose 'QueueRecordHeader' does not describe the
//   bytes that follow is rejected with a non-zero result code, does not
//   read or write outside the record, and leaves the JOURNAL untouched.
//
// Testing:
//   writeQueueCreationRecordImpl()
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName(
        "WRITE QUEUE CREATION RECORD IMPL MALFORMED");

    bslma::Allocator*              alloc = bmqtst::TestHelperUtil::allocator();
    const bsl::vector<bsl::string> noAppIds(alloc);

    {
        // 'numAppIds' greater than the number of entries present.
        Tester tester(noAppIds, alloc);
        tester.queueRecordHeader().setNumAppIds(1);

        BMQTST_ASSERT_NE(0, tester.write());
        BMQTST_ASSERT(tester.isJournalPristine());
    }

    {
        // 'queueRecordWords' too small.
        Tester tester(noAppIds, alloc);
        tester.queueRecordHeader().setQueueRecordWords(
            tester.queueRecordLen() / bmqp::Protocol::k_WORD_SIZE - 1);

        BMQTST_ASSERT_NE(0, tester.write());
        BMQTST_ASSERT(tester.isJournalPristine());
    }

    {
        // 'headerWords' below 'QueueRecordHeader::k_MIN_HEADER_SIZE'.
        Tester tester(noAppIds, alloc);
        tester.queueRecordHeader().setHeaderWords(1);

        BMQTST_ASSERT_NE(0, tester.write());
        BMQTST_ASSERT(tester.isJournalPristine());
    }

    {
        // Zero 'queueUriLengthWords'.
        Tester tester(noAppIds, alloc);
        tester.queueRecordHeader().setQueueUriLengthWords(0);

        BMQTST_ASSERT_NE(0, tester.write());
        BMQTST_ASSERT(tester.isJournalPristine());
    }

    {
        // 'queueUriLengthWords' overrunning the record.
        Tester tester(noAppIds, alloc);
        tester.queueRecordHeader().setQueueUriLengthWords(
            tester.queueRecordLen());

        BMQTST_ASSERT_NE(0, tester.write());
        BMQTST_ASSERT(tester.isJournalPristine());
    }

    {
        // A record longer than the space remaining in the QLIST file.
        Tester tester(noAppIds, alloc);
        tester.setQlistFileSize(tester.queueRecordLen() - 1);

        BMQTST_ASSERT_NE(0, tester.write());
        BMQTST_ASSERT(tester.isJournalPristine());
    }

    {
        // A URI which does not parse.
        Tester tester(noAppIds, alloc);
        tester.queueRecordByte(sizeof(mqbs::QueueRecordHeader)) = '!';

        BMQTST_ASSERT_NE(0, tester.write());
        BMQTST_ASSERT(tester.isJournalPristine());
    }

    {
        // A URI padding byte with its high bit set, which read as a signed
        // 'char' would yield an unpadded length larger than the padded one.
        Tester             tester(noAppIds, alloc);
        const unsigned int paddedUriLen =
            tester.queueRecordHeader().queueUriLengthWords() *
            bmqp::Protocol::k_WORD_SIZE;
        tester.queueRecordByte(sizeof(mqbs::QueueRecordHeader) + paddedUriLen -
                               1) = static_cast<char>(0x80);

        BMQTST_ASSERT_NE(0, tester.write());
        BMQTST_ASSERT(tester.isJournalPristine());
    }

    {
        // Trailing magic word mismatch.
        Tester tester(noAppIds, alloc);
        tester.queueRecordByte(tester.queueRecordLen() - 1) = 0;

        BMQTST_ASSERT_NE(0, tester.write());
        BMQTST_ASSERT(tester.isJournalPristine());
    }

    {
        // Zero 'appIdLengthWords' in the sole 'AppIdHeader'.
        bsl::vector<bsl::string> appIds(alloc);
        appIds.emplace_back("foo");

        Tester             tester(appIds, alloc);
        const unsigned int paddedUriLen =
            tester.queueRecordHeader().queueUriLengthWords() *
            bmqp::Protocol::k_WORD_SIZE;
        const unsigned int appIdHeaderOffset =
            sizeof(mqbs::QueueRecordHeader) + paddedUriLen +
            mqbs::FileStoreProtocol::k_HASH_LENGTH;

        mqbs::MemoryBlock appIdBlock(
            &tester.queueRecordByte(appIdHeaderOffset),
            sizeof(mqbs::AppIdHeader));
        mqbs::OffsetPtr<mqbs::AppIdHeader> aih(appIdBlock, 0);
        aih->setAppIdLengthWords(0);

        BMQTST_ASSERT_NE(0, tester.write());
        BMQTST_ASSERT(tester.isJournalPristine());
    }

    {
        // 'appIdLengthWords' overrunning the application-ID area.
        bsl::vector<bsl::string> appIds(alloc);
        appIds.emplace_back("foo");

        Tester             tester(appIds, alloc);
        const unsigned int paddedUriLen =
            tester.queueRecordHeader().queueUriLengthWords() *
            bmqp::Protocol::k_WORD_SIZE;
        const unsigned int appIdHeaderOffset =
            sizeof(mqbs::QueueRecordHeader) + paddedUriLen +
            mqbs::FileStoreProtocol::k_HASH_LENGTH;

        mqbs::MemoryBlock appIdBlock(
            &tester.queueRecordByte(appIdHeaderOffset),
            sizeof(mqbs::AppIdHeader));
        mqbs::OffsetPtr<mqbs::AppIdHeader> aih(appIdBlock, 0);
        aih->setAppIdLengthWords(tester.queueRecordLen());

        BMQTST_ASSERT_NE(0, tester.write());
        BMQTST_ASSERT(tester.isJournalPristine());
    }

    {
        // Invalid appId padding byte.
        bsl::vector<bsl::string> appIds(alloc);
        appIds.emplace_back("foo");

        Tester             tester(appIds, alloc);
        const unsigned int paddedUriLen =
            tester.queueRecordHeader().queueUriLengthWords() *
            bmqp::Protocol::k_WORD_SIZE;
        const unsigned int appIdEndOffset =
            sizeof(mqbs::QueueRecordHeader) + paddedUriLen +
            mqbs::FileStoreProtocol::k_HASH_LENGTH +
            sizeof(mqbs::AppIdHeader) + bmqp::Protocol::k_WORD_SIZE;

        tester.queueRecordByte(appIdEndOffset - 1) = 0;

        BMQTST_ASSERT_NE(0, tester.write());
        BMQTST_ASSERT(tester.isJournalPristine());
    }
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 2: test2_writeQueueCreationRecordImplMalformed(); break;
    case 1: test1_writeQueueCreationRecordImpl(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    // 'writeQueueCreationRecordImpl' builds a 'bmqt::Uri' and a log stream
    // from the default allocator, so only the global allocator is checked.
    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_GBL_ALLOC);
}
