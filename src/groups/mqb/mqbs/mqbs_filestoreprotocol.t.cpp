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

// mqbs_filestoreprotocol.t.cpp                                       -*-C++-*-
#include <mqbs_filestoreprotocol.h>

// BMQ
#include <bmqt_messageguid.h>

// MWC
#include <mwcu_memoutstream.h>

// BDE
#include <bsl_ios.h>
#include <bsl_iostream.h>
#include <bsl_limits.h>
#include <bslma_default.h>

// TEST DRIVER
#include <mwctst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------

const int k_INT_MAX = bsl::numeric_limits<int>::max();

const unsigned int k_UNSIGNED_INT_MAX =
    bsl::numeric_limits<unsigned int>::max();

const bsls::Types::Uint64 k_UINT64_MAX =
    bsl::numeric_limits<bsls::Types::Uint64>::max();

const unsigned char k_UNSIGNED_CHAR_MIN =
    bsl::numeric_limits<unsigned char>::min();

const bsls::Types::Uint64 k_33_BITS_MASK = 0x1FFFFFFFF;

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_breathingTest()
// ------------------------------------------------------------------------
// BREATHING TEST
//
// Concerns:
//   Exercise the basic functionality of the component.
//
// Testing:
//   Basic functionality of protocol structs
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("BREATHING TEST");

    using namespace mqbs;
    typedef FileStoreProtocol FSP;

    {
        // --------------------------
        // FileHeader Breathing Test
        // --------------------------
        PV("FileHeader");

        // Create default Fileheader
        FileHeader          fh;
        const unsigned char headerWords = sizeof(fh) / 4;

        ASSERT_EQ(fh.magic1(), FileHeader::k_MAGIC1);
        ASSERT_EQ(fh.magic2(), FileHeader::k_MAGIC2);
        ASSERT_EQ(fh.protocolVersion(), FileStoreProtocol::k_VERSION);
        ASSERT_EQ(fh.headerWords(), headerWords);
        ASSERT_EQ(fh.bitness(), Bitness::e_64);
        ASSERT_EQ(fh.fileType(), FileType::e_UNDEFINED);
        ASSERT_EQ(fh.partitionId(), 0);

        // Create FileHeader, set fields, assert fields
        FileHeader fh2;
        fh2.setMagic1(0xdeadbeef)
            .setMagic2(0xcafebabe)
            .setProtocolVersion(2)
            .setHeaderWords(10)
            .setBitness(Bitness::e_32)
            .setFileType(FileType::e_JOURNAL)
            .setPartitionId(2);

        ASSERT_EQ(fh2.magic1(), 0xdeadbeef);
        ASSERT_EQ(fh2.magic2(), 0xcafebabe);
        ASSERT_EQ(fh2.protocolVersion(), 2);
        ASSERT_EQ(fh2.headerWords(), 10);
        ASSERT_EQ(fh2.bitness(), Bitness::e_32);
        ASSERT_EQ(fh2.fileType(), FileType::e_JOURNAL);
        ASSERT_EQ(fh2.partitionId(), 2);
    }

    {
        // --------------------------
        // DataFileHeader Breathing Test
        // --------------------------
        PV("DateFileHeader");

        // Create default DataFileHeader
        DataFileHeader      fh;
        const unsigned char numWords = sizeof(fh) / 4;

        ASSERT_EQ(fh.headerWords(), numWords);
        ASSERT_EQ(mqbu::StorageKey::k_NULL_KEY, fh.fileKey());

        // Create DataFileHeader, set fields, assert fields
        mqbu::StorageKey key(mqbu::StorageKey::BinaryRepresentation(),
                             "12345");
        DataFileHeader   fh2;
        fh2.setHeaderWords(8).setFileKey(key);

        ASSERT_EQ(fh2.headerWords(), 8);
        ASSERT_EQ(key, fh2.fileKey());
    }

    {
        // --------------------------------
        // JournalFileHeader Breathing Test
        // --------------------------------
        PV("JournalFileHeader");

        // Create default JournalFileHeader
        JournalFileHeader   fh;
        const unsigned char numWords = sizeof(fh) / 4;
        const unsigned char recWords = FSP::k_JOURNAL_RECORD_SIZE / 4;

        ASSERT_EQ(fh.headerWords(), numWords);
        ASSERT_EQ(fh.recordWords(), recWords);
        ASSERT_EQ(fh.firstSyncPointOffsetWords(), 0ULL);

        // Create JournalFileHeader, set fields, assert fields
        JournalFileHeader fh2;
        fh2.setHeaderWords(8).setRecordWords(12).setFirstSyncPointOffsetWords(
            k_UINT64_MAX);

        ASSERT_EQ(fh2.headerWords(), 8U);
        ASSERT_EQ(fh2.recordWords(), 12U);
        ASSERT_EQ(fh2.firstSyncPointOffsetWords(), k_UINT64_MAX);
    }

    {
        // ------------------------------
        // QlistFileHeader Breathing Test
        // ------------------------------
        PV("QlistFileHeader");

        // Create default QlistFileHeader
        QlistFileHeader     fh;
        const unsigned char numWords = sizeof(fh) / 4;

        ASSERT_EQ(fh.headerWords(), numWords);

        // Create QlistFileHeader, set fields, assert fields
        QlistFileHeader fh2;
        fh2.setHeaderWords(8);

        ASSERT_EQ(fh2.headerWords(), 8U);
    }

    {
        // -------------------------
        // DataHeader Breathing Test
        // -------------------------
        PV("DataHeader");

        // Create default DataHeader
        DataHeader dh;
        const int  numWords = sizeof(dh) / 4;

        ASSERT_EQ(dh.headerWords(), numWords);
        ASSERT_EQ(dh.messageWords(), numWords);
        ASSERT_EQ(dh.optionsWords(), 0);
        ASSERT_EQ(dh.flags(), 0);

        // Create DataHeader, set fields, assert fields
        DataHeader dh2;
        dh2.setHeaderWords(5)
            .setMessageWords(123)
            .setOptionsWords(42)
            .setFlags(29);

        ASSERT_EQ(dh2.headerWords(), 5);
        ASSERT_EQ(dh2.messageWords(), 123);
        ASSERT_EQ(dh2.optionsWords(), 42);
        ASSERT_EQ(dh2.flags(), 29);
    }

    {
        // --------------------------------
        // QueueRecordHeader Breathing Test
        // --------------------------------
        PV("QueueRecordHeader");

        // Create default QueueRecordHeader
        QueueRecordHeader  fh;
        const unsigned int numWords = sizeof(fh) / 4;

        ASSERT_EQ(fh.queueUriLengthWords(), 0U);
        ASSERT_EQ(fh.numAppIds(), 0U);
        ASSERT_EQ(fh.headerWords(), numWords);
        ASSERT_EQ(fh.queueRecordWords(), numWords);

        // Create QueueRecordHeader, set fields, assert fields
        QueueRecordHeader fh2;
        fh2.setQueueUriLengthWords(5)
            .setNumAppIds(10)
            .setHeaderWords(7)
            .setQueueRecordWords(20);

        ASSERT_EQ(fh2.queueUriLengthWords(), 5U);
        ASSERT_EQ(fh2.numAppIds(), 10U);
        ASSERT_EQ(fh2.headerWords(), 7U);
        ASSERT_EQ(fh2.queueRecordWords(), 20U);
    }

    {
        // ---------------------------
        // RecordHeader Breathing Test
        // ---------------------------
        PV("RecordHeader");

        // Create default RecordHeader
        RecordHeader fh;

        ASSERT_EQ(fh.type(), RecordType::e_UNDEFINED);
        ASSERT_EQ(fh.flags(), 0U);
        ASSERT_EQ(fh.timestamp(), 0ULL);
        ASSERT_EQ(fh.primaryLeaseId(), 0U);
        ASSERT_EQ(fh.sequenceNumber(), 0ULL);

        // Create RecordHeader, set fields, assert fields
        RecordHeader fh2;
        fh2.setType(RecordType::e_JOURNAL_OP)
            .setFlags(123U)
            .setPrimaryLeaseId(k_UNSIGNED_INT_MAX)
            .setSequenceNumber(0xFFFFFFFFFFFF)
            .setTimestamp(k_UINT64_MAX);

        ASSERT_EQ(fh2.type(), RecordType::e_JOURNAL_OP);
        ASSERT_EQ(fh2.flags(), 123U);
        ASSERT_EQ(fh2.timestamp(), k_UINT64_MAX);
        ASSERT_EQ(fh2.primaryLeaseId(), k_UNSIGNED_INT_MAX);
        ASSERT_EQ(fh2.sequenceNumber(),
                  static_cast<bsls::Types::Uint64>(0xFFFFFFFFFFFF));
    }

    {
        // ----------------------------
        // MessageRecord Breathing Test
        // ----------------------------
        PV("MessageRecord");

        // Create default MessageRecord
        MessageRecord fh;

        ASSERT_EQ(fh.refCount(), 0U);
        ASSERT_EQ(fh.queueKey(), mqbu::StorageKey::k_NULL_KEY);
        ASSERT_EQ(fh.fileKey(), mqbu::StorageKey::k_NULL_KEY);
        ASSERT_EQ(fh.messageOffsetDwords(), 0U);
        ASSERT_EQ(fh.header().primaryLeaseId(), 0U);
        ASSERT_EQ(fh.header().sequenceNumber(), 0ULL);
        ASSERT_EQ(fh.messageGUID(), bmqt::MessageGUID());
        ASSERT_EQ(fh.crc32c(), 0U);
        ASSERT_EQ(fh.compressionAlgorithmType(),
                  bmqt::CompressionAlgorithmType::e_NONE);
        ASSERT_EQ(fh.magic(), 0U);

        // Create MessageRecord, set fields, assert fields
        MessageRecord fh2;
        fh2.setRefCount(1000)
            .setQueueKey(
                mqbu::StorageKey(mqbu::StorageKey::BinaryRepresentation(),
                                 "abcde"))
            .setFileKey(
                mqbu::StorageKey(mqbu::StorageKey::BinaryRepresentation(),
                                 "12345"))
            .setMessageOffsetDwords(1000000)
            .setMessageGUID(bmqt::MessageGUID())
            .setCrc32c(987654321)
            .setCompressionAlgorithmType(
                bmqt::CompressionAlgorithmType::e_ZLIB)
            .setMagic(0xdeadbeef);
        ASSERT_EQ(fh2.refCount(), 1000U);
        ASSERT(fh2.queueKey() ==
               mqbu::StorageKey(mqbu::StorageKey::BinaryRepresentation(),
                                "abcde"));
        ASSERT(fh2.fileKey() ==
               mqbu::StorageKey(mqbu::StorageKey::BinaryRepresentation(),
                                "12345"));
        ASSERT_EQ(fh2.messageOffsetDwords(), 1000000U);
        ASSERT_EQ(fh2.messageGUID(), bmqt::MessageGUID());
        ASSERT_EQ(fh2.crc32c(), 987654321U);
        ASSERT_EQ(fh2.compressionAlgorithmType(),
                  bmqt::CompressionAlgorithmType::e_ZLIB);
        ASSERT_EQ(fh2.magic(), 0xdeadbeef);
    }

    {
        // ----------------------------
        // ConfirmRecord Breathing Test
        // ----------------------------
        PV("ConfirmRecord");

        // Create default ConfirmRecord
        ConfirmRecord fh;

        ASSERT_EQ(fh.reason(), ConfirmReason::e_CONFIRMED);
        ASSERT_EQ(fh.queueKey(), mqbu::StorageKey::k_NULL_KEY);
        ASSERT_EQ(fh.appKey(), mqbu::StorageKey::k_NULL_KEY);
        ASSERT_EQ(fh.messageGUID(), bmqt::MessageGUID());
        ASSERT_EQ(fh.magic(), 0U);

        // Create MessageRecord, set fields, assert fields
        ConfirmRecord fh2;
        fh2.setReason(ConfirmReason::e_REJECTED)
            .setQueueKey(
                mqbu::StorageKey(mqbu::StorageKey::BinaryRepresentation(),
                                 "abcde"))
            .setAppKey(
                mqbu::StorageKey(mqbu::StorageKey::BinaryRepresentation(),
                                 "12345"))
            .setMessageGUID(bmqt::MessageGUID())
            .setMagic(0xdeadbeef);

        ASSERT_EQ(fh2.reason(), ConfirmReason::e_REJECTED);
        ASSERT_EQ(
            0,
            bsl::memcmp(fh2.queueKey().data(), "abcde", FSP::k_KEY_LENGTH));
        ASSERT_EQ(
            0,
            bsl::memcmp(fh2.appKey().data(), "12345", FSP::k_KEY_LENGTH));
        ASSERT_EQ(fh2.messageGUID(), bmqt::MessageGUID());
        ASSERT_EQ(fh2.magic(), 0xdeadbeef);
    }

    {
        // ----------------------------
        // DeletionRecord Breathing Test
        // ----------------------------
        PV("DeletionRecord");

        // Create default DeletionRecord
        DeletionRecord fh;

        ASSERT_EQ(fh.deletionRecordFlag(), 0);
        ASSERT_EQ(fh.queueKey(), mqbu::StorageKey::k_NULL_KEY);
        ASSERT_EQ(fh.messageGUID(), bmqt::MessageGUID());
        ASSERT_EQ(fh.magic(), 0U);

        // Create DeletionRecord, set fields, assert fields
        DeletionRecord fh2;
        fh2.setDeletionRecordFlag(DeletionRecordFlag::e_TTL_EXPIRATION)
            .setQueueKey(
                mqbu::StorageKey(mqbu::StorageKey::BinaryRepresentation(),
                                 "abcde"))
            .setMessageGUID(bmqt::MessageGUID())
            .setMagic(0xdeadbeef);

        ASSERT_EQ(fh2.deletionRecordFlag(),
                  DeletionRecordFlag::e_TTL_EXPIRATION);
        ASSERT_EQ(
            0,
            bsl::memcmp(fh2.queueKey().data(), "abcde", FSP::k_KEY_LENGTH));
        ASSERT_EQ(fh2.messageGUID(), bmqt::MessageGUID());
        ASSERT_EQ(fh2.magic(), 0xdeadbeef);
    }

    {
        // ----------------------------
        // QueueOpRecord Breathing Test
        // ----------------------------
        PV("QueueOpRecord");

        // Create default QueueOpRecord
        QueueOpRecord fh;

        ASSERT_EQ(fh.flags(), 0U);
        ASSERT_EQ(fh.queueKey(), mqbu::StorageKey::k_NULL_KEY);
        ASSERT_EQ(fh.appKey(), mqbu::StorageKey::k_NULL_KEY);
        ASSERT_EQ(fh.type(), QueueOpType::e_UNDEFINED);
        ASSERT_EQ(fh.magic(), 0U);

        // Create QueueOpRecord, set fields, assert fields
        QueueOpRecord fh2;
        fh2.setFlags(1000)
            .setQueueKey(
                mqbu::StorageKey(mqbu::StorageKey::BinaryRepresentation(),
                                 "abcde"))
            .setAppKey(
                mqbu::StorageKey(mqbu::StorageKey::BinaryRepresentation(),
                                 "12345"))
            .setType(QueueOpType::e_PURGE)
            .setMagic(0xdeadbeef);

        ASSERT_EQ(fh2.flags(), 1000U);
        ASSERT_EQ(
            0,
            bsl::memcmp(fh2.queueKey().data(), "abcde", FSP::k_KEY_LENGTH));
        ASSERT_EQ(
            0,
            bsl::memcmp(fh2.appKey().data(), "12345", FSP::k_KEY_LENGTH));
        ASSERT_EQ(fh2.type(), QueueOpType::e_PURGE);
        ASSERT_EQ(fh2.magic(), 0xdeadbeef);
    }

    {
        // ------------------------------
        // JournalOpRecord Breathing Test
        // ------------------------------
        PV("JournalOpRecord");

        // Create default JournalOpRecord
        JournalOpRecord fh;

        ASSERT_EQ(fh.flags(), 0U);
        ASSERT_EQ(fh.type(), JournalOpType::e_UNDEFINED);
        ASSERT_EQ(fh.sequenceNum(), 0ULL);
        ASSERT_EQ(fh.primaryNodeId(), 0);
        ASSERT_EQ(fh.primaryLeaseId(), 0U);
        ASSERT_EQ(fh.magic(), 0U);

        // Create JournalOpRecord, set fields, assert fields
        JournalOpRecord fh2;
        fh2.setFlags(1000)
            .setType(JournalOpType::e_SYNCPOINT)
            .setSequenceNum(k_33_BITS_MASK)
            .setPrimaryNodeId(k_INT_MAX)
            .setPrimaryLeaseId(k_UNSIGNED_INT_MAX)
            .setMagic(0xdeadbeef);

        ASSERT_EQ(fh2.flags(), 1000U);
        ASSERT_EQ(fh2.type(), JournalOpType::e_SYNCPOINT);
        ASSERT_EQ(fh2.sequenceNum(), k_33_BITS_MASK);
        ASSERT_EQ(fh2.primaryNodeId(), k_INT_MAX);
        ASSERT_EQ(fh2.primaryLeaseId(), k_UNSIGNED_INT_MAX);
        ASSERT_EQ(fh2.magic(), 0xdeadbeef);
    }
}

static void test2_manipulators()
// ------------------------------------------------------------------------
// MANIPULATORS TEST
//
//
// Concerns:
//   Test protocol structs with edge/corner values
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("MANIPULATORS");

    using namespace mqbs;

    const struct TestData {
        int d_line;

        unsigned char d_protocolVersion;
        unsigned char d_expectedPV;

        unsigned char d_headerWords;
        unsigned char d_expectedHW;

        int d_fileType;
        int d_expectedFT;

        int d_partitionId;
        int d_expectedGID;

    } DATA[] = {
        {
            L_,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
        },
        {L_,
         k_UNSIGNED_CHAR_MIN,
         k_UNSIGNED_CHAR_MIN,
         k_UNSIGNED_CHAR_MIN,
         k_UNSIGNED_CHAR_MIN,
         k_INT_MAX,
         127,
         k_INT_MAX,
         k_INT_MAX},
    };

    const int NUM_DATA = sizeof(DATA) / sizeof(*DATA);

    for (int dataIdx = 0; dataIdx < NUM_DATA; ++dataIdx) {
        const TestData& data = DATA[dataIdx];

        // Create default FileHeader
        FileHeader fh;

        // Set fields
        fh.setMagic1(FileHeader::k_MAGIC1)
            .setMagic2(FileHeader::k_MAGIC2)
            .setProtocolVersion(data.d_protocolVersion)
            .setHeaderWords(data.d_headerWords)
            .setFileType(static_cast<FileType::Enum>(data.d_fileType))
            .setPartitionId(data.d_partitionId);

        // Check

        // Somehow the compiler doesn't like direct usage of those
        // constants in the ASSERT macro ...
        const unsigned int magic1 = FileHeader::k_MAGIC1;
        const unsigned int magic2 = FileHeader::k_MAGIC2;

        ASSERT_EQ_D(dataIdx, magic1, fh.magic1());
        ASSERT_EQ_D(dataIdx, magic2, fh.magic2());
        ASSERT_EQ_D(dataIdx, fh.protocolVersion(), data.d_expectedPV);
        ASSERT_EQ_D(dataIdx, fh.headerWords(), data.d_expectedHW);

        ASSERT_EQ_D(dataIdx,
                    fh.fileType(),
                    static_cast<FileType::Enum>(data.d_expectedFT));

        ASSERT_EQ_D(dataIdx, fh.partitionId(), data.d_expectedGID);
    }
}

static void test3_printTest()
// ------------------------------------------------------------------------
// PRINT TEST
//
// Concerns:
//   Test printing various records
//
// Testing:
//   operator<<(bsl::ostream& stream, const RecordHeader& rhs)
//   operator<<(bsl::ostream& stream, const MessageRecord& rhs)
//   operator<<(bsl::ostream& stream, const ConfirmRecord& rhs)
//   operator<<(bsl::ostream& stream, const DeletionRecord& rhs)
//   operator<<(bsl::ostream& stream, const QueueOpRecord& rhs)
//   operator<<(bsl::ostream& stream, const JournalOpRecord& rhs)
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("PRINT TEST");

    using namespace mqbs;

    // Commonly used by all sub-tests below; type is NOT set
    RecordHeader rh;
    rh.setFlags(0).setPrimaryLeaseId(8).setSequenceNumber(33).setTimestamp(
        123456);

    const mqbu::StorageKey k_FILE_KEY(mqbu::StorageKey::HexRepresentation(),
                                      "DEADBEEF01");
    const mqbu::StorageKey k_QUEUE_KEY(mqbu::StorageKey::HexRepresentation(),
                                       "DEADFACE13");
    const mqbu::StorageKey k_APP_KEY(mqbu::StorageKey::HexRepresentation(),
                                     "FACEDAFACE");

    {
        // -----------------------
        // RecordHeader Print Test
        // -----------------------
        PV("RecordHeader");
        rh.setType(RecordType::e_MESSAGE);

        const char* const k_EXPECTED_OUTPUT =
            "[ type = MESSAGE flags = 0 primaryLeaseId = 8 sequenceNumber = 33"
            " timestamp = 123456 ]";

        mwcu::MemOutStream stream(s_allocator_p);
        stream << rh;
        ASSERT_EQ(stream.str(), k_EXPECTED_OUTPUT);
        stream.reset();

        PV("Bad stream test");
        stream << "INVALID";
        stream.clear(bsl::ios_base::badbit);
        stream << rh;
        ASSERT_EQ(stream.str(), "INVALID");
    }

    {
        // ------------------------
        // MessageRecord Print Test
        // ------------------------
        PV("MessageRecord");
        rh.setType(RecordType::e_MESSAGE);
        mqbs::MessageRecord msgRec;
        msgRec.header() = rh;
        msgRec.setRefCount(2)
            .setQueueKey(k_QUEUE_KEY)
            .setFileKey(k_FILE_KEY)
            .setMessageOffsetDwords(123)
            .setMessageGUID(bmqt::MessageGUID())
            .setCrc32c(2333)
            .setCompressionAlgorithmType(
                bmqt::CompressionAlgorithmType::e_ZLIB);

        const char* const k_EXPECTED_OUTPUT =
            "[ header = [ type = MESSAGE flags = 2 primaryLeaseId = 8 "
            "sequenceNumber = 33 timestamp = 123456 ] refCount = 2 queueKey = "
            "DEADFACE13 fileKey = DEADBEEF01 messageOffsetDwords = 123 "
            "messageGUID = ** UNSET ** crc32c = 2333 compressionAlgorithmType "
            "= ZLIB ]";

        mwcu::MemOutStream stream(s_allocator_p);
        stream << msgRec;
        ASSERT_EQ(stream.str(), k_EXPECTED_OUTPUT);
        stream.reset();

        PV("Bad stream test");
        stream << "INVALID";
        stream.clear(bsl::ios_base::badbit);
        stream << msgRec;
        ASSERT_EQ(stream.str(), "INVALID");
    }

    {
        // ------------------------
        // ConfirmRecord Print Test
        // ------------------------
        PV("ConfirmRecord");
        rh.setType(RecordType::e_CONFIRM);

        mqbs::ConfirmRecord confRec;
        confRec.header() = rh;
        confRec.setReason(ConfirmReason::e_CONFIRMED)
            .setQueueKey(k_QUEUE_KEY)
            .setAppKey(k_APP_KEY)
            .setMessageGUID(bmqt::MessageGUID());

        const char* const k_EXPECTED_OUTPUT =
            "[ header = [ type = CONFIRM flags = 0 primaryLeaseId = 8 "
            "sequenceNumber = 33 timestamp = 123456 ] reason = CONFIRMED "
            "queueKey = DEADFACE13 appKey = FACEDAFACE messageGUID = ** UNSET "
            "** ]";

        mwcu::MemOutStream stream(s_allocator_p);
        stream << confRec;
        ASSERT_EQ(stream.str(), k_EXPECTED_OUTPUT);
        stream.reset();

        PV("Bad stream test");
        stream << "INVALID";
        stream.clear(bsl::ios_base::badbit);
        stream << confRec;
        ASSERT_EQ(stream.str(), "INVALID");
    }

    {
        // -------------------------
        // DeletionRecord Print Test
        // -------------------------
        PV("DeletionRecord");
        rh.setType(RecordType::e_DELETION);
        mqbs::DeletionRecord delRec;
        delRec.header() = rh;
        delRec.setDeletionRecordFlag(DeletionRecordFlag::e_IMPLICIT_CONFIRM)
            .setQueueKey(k_QUEUE_KEY)
            .setMessageGUID(bmqt::MessageGUID());

        const char* const k_EXPECTED_OUTPUT =
            "[ header = [ type = DELETION flags = 1 primaryLeaseId = 8 "
            "sequenceNumber = 33 timestamp = 123456 ] deletionRecordFlag = "
            "IMPLICIT_CONFIRM queueKey = DEADFACE13 messageGUID = "
            "** UNSET ** ]";

        mwcu::MemOutStream stream(s_allocator_p);
        stream << delRec;
        ASSERT_EQ(stream.str(), k_EXPECTED_OUTPUT);
        stream.reset();

        PV("Bad stream test");
        stream << "INVALID";
        stream.clear(bsl::ios_base::badbit);
        stream << delRec;
        ASSERT_EQ(stream.str(), "INVALID");
    }

    {
        // ------------------------
        // QueueOpRecord Print Test
        // ------------------------
        PV("QueueOpRecord");
        rh.setType(RecordType::e_QUEUE_OP);
        mqbs::QueueOpRecord qOpRec;
        qOpRec.header() = rh;
        qOpRec.setFlags(0)
            .setQueueKey(k_QUEUE_KEY)
            .setAppKey(k_APP_KEY)
            .setType(QueueOpType::e_CREATION)
            .setQueueUriRecordOffsetWords(69);

        const char* const k_EXPECTED_OUTPUT =
            "[ header = [ type = QUEUE_OP flags = 0 primaryLeaseId = 8 "
            "sequenceNumber = 33 timestamp = 123456 ] flags = 0 queueKey = "
            "DEADFACE13 appKey = FACEDAFACE type = CREATION "
            "queueUriRecordOffsetWords = 69 ]";

        mwcu::MemOutStream stream(s_allocator_p);
        stream << qOpRec;
        ASSERT_EQ(stream.str(), k_EXPECTED_OUTPUT);
        stream.reset();

        PV("Bad stream test");
        stream << "INVALID";
        stream.clear(bsl::ios_base::badbit);
        stream << qOpRec;
        ASSERT_EQ(stream.str(), "INVALID");
    }

    {
        // --------------------------
        // JournalOpRecord Print Test
        // --------------------------
        PV("JournalOpRecord");
        rh.setType(RecordType::e_JOURNAL_OP);
        mqbs::JournalOpRecord jOpRec;
        jOpRec.header() = rh;
        jOpRec.setFlags(0)
            .setType(JournalOpType::e_SYNCPOINT)
            .setSyncPointType(SyncPointType::e_REGULAR)
            .setSequenceNum(9876543)
            .setPrimaryNodeId(1)
            .setPrimaryLeaseId(8)
            .setDataFileOffsetDwords(666)
            .setQlistFileOffsetWords(23);

        const char* const expectedOut =
            "[ header = [ type = JOURNAL_OP flags = 0 primaryLeaseId = 8 "
            "sequenceNumber = 33 timestamp = 123456 ] flags = 0 type = "
            "SYNCPOINT syncPointType = REGULAR sequenceNum = 9876543 "
            "primaryNodeId = 1 primaryLeaseId = 8 dataFileOffsetDwords = 666 "
            "qlistFileOffsetWords = 23 ]";

        mwcu::MemOutStream stream(s_allocator_p);
        stream << jOpRec;
        ASSERT_EQ(stream.str(), expectedOut);
        stream.reset();

        PV("Bad stream test");
        stream << "INVALID";
        stream.clear(bsl::ios_base::badbit);
        stream << jOpRec;
        ASSERT_EQ(stream.str(), "INVALID");
    }
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(mwctst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 3: test3_printTest(); break;
    case 2: test2_manipulators(); break;
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        s_testStatus = -1;
    } break;
    }

    TEST_EPILOG(mwctst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
