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

// mqbc_clusterstateledgerutil.t.cpp                                  -*-C++-*-
#include <mqbc_clusterstateledgerutil.h>

// MQB
#include <mqbc_clusterstateledgerprotocol.h>
#include <mqbmock_logidgenerator.h>
#include <mqbsi_ledger.h>
#include <mqbsi_log.h>
#include <mqbsl_ledger.h>
#include <mqbsl_memorymappedondisklog.h>
#include <mqbu_storagekey.h>

// BMQ
#include <bmqp_crc32c.h>
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_protocol.h>

#include <bmqsys_time.h>
#include <bmqu_tempdirectory.h>

// BDE
#include <bdlbb_blob.h>
#include <bdlbb_blobutil.h>
#include <bdlbb_pooledblobbufferfactory.h>
#include <bsl_memory.h>
#include <bsl_string.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bsls_annotation.h>
#include <bsls_assert.h>
#include <bsls_keyword.h>
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

// CONSTANTS
const char* k_DEFAULT_LOG_PREFIX = "BMQ_TEST_LOG_";

// TYPES
struct AdvisoryType {
    enum Enum {
        e_PARTITION_PRIMARY = 0,
        e_QUEUE_ASSIGNMENT  = 1,
        e_COMMIT            = 2
    };
};

// FUNCTIONS
int cleanupCallback(BSLS_ANNOTATION_UNUSED const bsl::string& logPath)
{
    return 0;
}

int onRolloverCallback(BSLS_ANNOTATION_UNUSED const mqbu::StorageKey& oldLogId,
                       BSLS_ANNOTATION_UNUSED const mqbu::StorageKey& newLogId)
{
    return 0;
}

// CLASSES
// =============
// struct Tester
// =============

struct Tester {
  private:
    // DATA
    mqbsi::LedgerConfig              d_config;
    bslma::ManagedPtr<mqbsi::Ledger> d_ledger_mp;
    bmqu::TempDirectory              d_tempDir;
    bdlbb::PooledBlobBufferFactory   d_bufferFactory;

  public:
    // PUBLIC CONSTANTS
    static const int k_LEADER_NODE_ID = 1;

  public:
    // CREATORS
    Tester(bslma::Allocator* allocator = s_allocator_p)
    : d_config(allocator)
    , d_ledger_mp(0)
    , d_tempDir(allocator)
    , d_bufferFactory(1024, allocator)
    {
        // Instantiate ledger config
        bsl::shared_ptr<mqbsi::LogIdGenerator> logIdGenerator(
            new (*allocator)
                mqbmock::LogIdGenerator(k_DEFAULT_LOG_PREFIX, allocator),
            allocator);

        bsl::shared_ptr<mqbsi::LogFactory> logFactory(
            new (*allocator) mqbsl::MemoryMappedOnDiskLogFactory(allocator),
            allocator);

        bsl::string filePattern(bsl::string(k_DEFAULT_LOG_PREFIX) + "[0-9]*" +
                                    ".bmq",
                                allocator);

        d_config.setLocation(d_tempDir.path())
            .setPattern(filePattern)
            .setMaxLogSize(64 * 1024 * 1024)
            .setReserveOnDisk(false)
            .setPrefaultPages(false)
            .setLogIdGenerator(logIdGenerator)
            .setLogFactory(logFactory)
            .setExtractLogIdCallback(
                &mqbc::ClusterStateLedgerUtil::extractLogId)
            .setValidateLogCallback(&mqbc::ClusterStateLedgerUtil::validateLog)
            .setRolloverCallback(&onRolloverCallback)
            .setCleanupCallback(&cleanupCallback);

        // Create and open the ledger
        d_ledger_mp.load(new (*allocator) mqbsl::Ledger(d_config, allocator),
                         allocator);
        BSLS_ASSERT_OPT(
            d_ledger_mp->open(mqbsi::Ledger::e_CREATE_IF_MISSING) == 0);
    }

    ~Tester() { BSLS_ASSERT_OPT(d_ledger_mp->close() == 0); }

    static void
    createClusterMessage(bmqp_ctrlmsg::ClusterMessage* message,
                         AdvisoryType::Enum            advisoryType,
                         const bmqp_ctrlmsg::LeaderMessageSequence& lms)
    {
        switch (advisoryType) {
        case AdvisoryType::e_PARTITION_PRIMARY: {
            bmqp_ctrlmsg::PartitionPrimaryInfo pinfo;
            pinfo.primaryNodeId()  = Tester::k_LEADER_NODE_ID;
            pinfo.partitionId()    = 1U;
            pinfo.primaryLeaseId() = 1U;

            bmqp_ctrlmsg::PartitionPrimaryAdvisory advisory;
            advisory.sequenceNumber() = lms;
            advisory.partitions().push_back(pinfo);

            message->choice().makePartitionPrimaryAdvisory(advisory);
        } break;
        case AdvisoryType::e_QUEUE_ASSIGNMENT: {
            bmqp_ctrlmsg::QueueInfo qinfo;
            qinfo.partitionId() = 1U;
            qinfo.uri()         = "bmq://bmq.random.x/q1";

            mqbu::StorageKey key(mqbu::StorageKey::BinaryRepresentation(),
                                 "12345");
            key.loadBinary(&qinfo.key());

            bmqp_ctrlmsg::QueueAssignmentAdvisory advisory;
            advisory.sequenceNumber() = lms;
            advisory.queues().push_back(qinfo);

            message->choice().makeQueueAssignmentAdvisory(advisory);
        } break;
        case AdvisoryType::e_COMMIT: {
            bmqp_ctrlmsg::LeaderAdvisoryCommit commit;
            commit.sequenceNumber()                           = lms;
            commit.sequenceNumberCommitted().electorTerm()    = 2U;
            commit.sequenceNumberCommitted().sequenceNumber() = 99U;

            message->choice().makeLeaderAdvisoryCommit(commit);
        } break;
        }
    }

    mqbsi::Ledger* ledger() const { return d_ledger_mp.get(); }

    bdlbb::BlobBufferFactory* bufferFactory() { return &d_bufferFactory; }
};

}  // close anonymous namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_validateFileHeader()
// ------------------------------------------------------------------------
// VALIDATE FILE HEADER
//
// Concerns:
//   Ensure proper behavior of 'validateFileHeader' method.
//
// Testing:
//   validateFileHeader(...)
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("VALIDATE FILE HEADER");

    mqbc::ClusterStateFileHeader header;
    mqbu::StorageKey key(mqbu::StorageKey::BinaryRepresentation(), "12345");
    header.setProtocolVersion(mqbc::ClusterStateLedgerProtocol::k_VERSION)
        .setHeaderWords(mqbc::ClusterStateFileHeader::k_HEADER_NUM_WORDS)
        .setFileKey(key);

    ASSERT_EQ(mqbc::ClusterStateLedgerUtil::validateFileHeader(header), 0);
    ASSERT_EQ(mqbc::ClusterStateLedgerUtil::validateFileHeader(header, key),
              0);

    // Test invalid protocol version
    header.setProtocolVersion(mqbc::ClusterStateLedgerProtocol::k_VERSION - 1);
    ASSERT_EQ(mqbc::ClusterStateLedgerUtil::validateFileHeader(header),
              mqbc::ClusterStateLedgerUtilRc::e_INVALID_PROTOCOL_VERSION);
    header.setProtocolVersion(mqbc::ClusterStateLedgerProtocol::k_VERSION);

    // Test invalid header words
    header.setHeaderWords(0);
    ASSERT_EQ(mqbc::ClusterStateLedgerUtil::validateFileHeader(header),
              mqbc::ClusterStateLedgerUtilRc::e_INVALID_HEADER_WORDS);
    header.setHeaderWords(mqbc::ClusterStateFileHeader::k_HEADER_NUM_WORDS);

    // Test invalid log id
    header.setFileKey(mqbu::StorageKey());
    ASSERT_EQ(mqbc::ClusterStateLedgerUtil::validateFileHeader(header),
              mqbc::ClusterStateLedgerUtilRc::e_INVALID_LOG_ID);
    header.setFileKey(key);

    // Test unexpected log id
    mqbu::StorageKey unexpectedKey(mqbu::StorageKey::BinaryRepresentation(),
                                   "23456");
    header.setFileKey(unexpectedKey);
    ASSERT_EQ(mqbc::ClusterStateLedgerUtil::validateFileHeader(header, key),
              mqbc::ClusterStateLedgerUtilRc::e_INVALID_LOG_ID);
    header.setFileKey(key);
}

static void test2_validateRecordHeader()
// ------------------------------------------------------------------------
// VALIDATE RECORD HEADER
//
// Concerns:
//   Ensure proper behavior of 'validateRecordHeader' method.
//
// Testing:
//   validateRecordHeader(...)
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("VALIDATE RECORD HEADER");

    mqbc::ClusterStateRecordHeader header;
    header.setHeaderWords(mqbc::ClusterStateRecordHeader::k_HEADER_NUM_WORDS)
        .setRecordType(mqbc::ClusterStateRecordType::e_SNAPSHOT)
        .setLeaderAdvisoryWords(17)
        .setElectorTerm(3)
        .setSequenceNumber(100)
        .setTimestamp(123456U);

    ASSERT_EQ(mqbc::ClusterStateLedgerUtil::validateRecordHeader(header), 0);

    // Test invalid header words
    header.setHeaderWords(0);
    ASSERT_EQ(mqbc::ClusterStateLedgerUtil::validateRecordHeader(header),
              mqbc::ClusterStateLedgerUtilRc::e_INVALID_HEADER_WORDS);
    header.setHeaderWords(mqbc::ClusterStateRecordHeader::k_HEADER_NUM_WORDS);

    // Test invalid record type
    header.setRecordType(mqbc::ClusterStateRecordType::e_UNDEFINED);
    ASSERT_EQ(mqbc::ClusterStateLedgerUtil::validateRecordHeader(header),
              mqbc::ClusterStateLedgerUtilRc::e_INVALID_RECORD_TYPE);
    header.setRecordType(mqbc::ClusterStateRecordType::e_SNAPSHOT);

    // Test invalid leader advisory words
    header.setLeaderAdvisoryWords(0);
    ASSERT_EQ(mqbc::ClusterStateLedgerUtil::validateRecordHeader(header),
              mqbc::ClusterStateLedgerUtilRc::e_INVALID_LEADER_ADVISORY_WORDS);
    header.setLeaderAdvisoryWords(17);
}

static void test3_extractLogId()
// ------------------------------------------------------------------------
// EXTRACT LOG ID
//
// Concerns:
//   Ensure proper behavior of 'extractLogId' method.
//
// Testing:
//   extractLogId(...)
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("EXTRACT LOG ID");

    Tester                  tester;
    mqbsi::Ledger*          ledger = tester.ledger();
    const mqbu::StorageKey& logId  = ledger->currentLog()->logConfig().logId();

    int rc = mqbc::ClusterStateLedgerUtil::writeFileHeader(ledger, logId);
    BSLS_ASSERT_OPT(rc == 0);

    mqbu::StorageKey extractedLogId;
    rc = mqbc::ClusterStateLedgerUtil::extractLogId(
        &extractedLogId,
        ledger->currentLog()->logConfig().location());
    ASSERT_EQ(rc, 0);

    ASSERT_EQ(extractedLogId, logId);
}

static void test4_validateLog()
// ------------------------------------------------------------------------
// VALIDATE LOG
//
// Concerns:
//   Ensure proper behavior of 'validateLog' method.
//
// Testing:
//   validateLog(...)
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("VALIDATE LOG");

    Tester         tester;
    mqbsi::Ledger* ledger = tester.ledger();

    // 1. Write a file header
    int rc = mqbc::ClusterStateLedgerUtil::writeFileHeader(
        ledger,
        ledger->currentLog()->logConfig().logId());
    BSLS_ASSERT_OPT(rc == 0);

    // 2. Write a record of each type (Snapshot, Update, Commit)

    // Snapshot record
    bmqp_ctrlmsg::LeaderMessageSequence lms1;
    lms1.electorTerm()    = 3U;
    lms1.sequenceNumber() = 7U;

    bmqp_ctrlmsg::ClusterMessage msg1;
    Tester::createClusterMessage(&msg1,
                                 AdvisoryType::e_PARTITION_PRIMARY,
                                 lms1);

    bdlbb::Blob record1(tester.bufferFactory(), s_allocator_p);
    rc = mqbc::ClusterStateLedgerUtil::appendRecord(
        &record1,
        msg1,
        lms1,
        123456U,
        mqbc::ClusterStateRecordType::e_SNAPSHOT);
    BSLS_ASSERT_OPT(rc == 0);

    mqbsi::LedgerRecordId recordId1;
    rc = ledger->writeRecord(&recordId1,
                             record1,
                             bmqu::BlobPosition(),
                             record1.length());
    BSLS_ASSERT_OPT(rc == 0);

    // Update record
    bmqp_ctrlmsg::LeaderMessageSequence lms2;
    lms2.electorTerm()    = 3U;
    lms2.sequenceNumber() = 8U;

    bmqp_ctrlmsg::ClusterMessage msg2;
    Tester::createClusterMessage(&msg2,
                                 AdvisoryType::e_QUEUE_ASSIGNMENT,
                                 lms2);

    bdlbb::Blob record2(tester.bufferFactory(), s_allocator_p);
    rc = mqbc::ClusterStateLedgerUtil::appendRecord(
        &record2,
        msg2,
        lms2,
        123567U,
        mqbc::ClusterStateRecordType::e_UPDATE);
    BSLS_ASSERT_OPT(rc == 0);

    mqbsi::LedgerRecordId recordId2;
    rc = ledger->writeRecord(&recordId2,
                             record2,
                             bmqu::BlobPosition(),
                             record2.length());
    BSLS_ASSERT_OPT(rc == 0);

    // Commit record
    bmqp_ctrlmsg::LeaderMessageSequence lms3;
    lms3.electorTerm()    = 3U;
    lms3.sequenceNumber() = 9U;

    bmqp_ctrlmsg::ClusterMessage msg3;
    Tester::createClusterMessage(&msg3, AdvisoryType::e_COMMIT, lms3);

    bdlbb::Blob record3(tester.bufferFactory(), s_allocator_p);
    rc = mqbc::ClusterStateLedgerUtil::appendRecord(
        &record3,
        msg3,
        lms3,
        123678U,
        mqbc::ClusterStateRecordType::e_COMMIT);
    BSLS_ASSERT_OPT(rc == 0);

    mqbsi::LedgerRecordId recordId3;
    rc = ledger->writeRecord(&recordId3,
                             record3,
                             bmqu::BlobPosition(),
                             record3.length());
    BSLS_ASSERT_OPT(rc == 0);

    // 3. Try validateLog()
    mqbsi::Log::Offset offset = 0;
    ASSERT_EQ(mqbc::ClusterStateLedgerUtil::validateLog(&offset,
                                                        ledger->currentLog()),
              0);
    PVV("offset: " << offset);

    // 4. Write an invalid record
    bdlbb::Blob record4(tester.bufferFactory(), s_allocator_p);
    mqbc::ClusterStateRecordHeader recordHeader;
    recordHeader
        .setHeaderWords(mqbc::ClusterStateRecordHeader::k_HEADER_NUM_WORDS)
        .setLeaderAdvisoryWords(400)
        .setRecordType(mqbc::ClusterStateRecordType::e_UPDATE);
    bdlbb::BlobUtil::append(&record4,
                            reinterpret_cast<const char*>(&recordHeader),
                            sizeof(mqbc::ClusterStateRecordHeader));

    mqbsi::LedgerRecordId recordId4;
    rc = ledger->writeRecord(&recordId4,
                             record4,
                             bmqu::BlobPosition(),
                             record4.length());
    BSLS_ASSERT_OPT(rc == 0);

    // 5. Ensure that validateLog() fails now (size of record is beyond length
    // of the log)
    offset = 0;
    ASSERT_NE(mqbc::ClusterStateLedgerUtil::validateLog(&offset,
                                                        ledger->currentLog()),
              0);
}

static void test5_validateLog_invalidCrc32c()
// ------------------------------------------------------------------------
// VALIDATE LOG - INVALID CRC32-C
//
// Concerns:
//   Ensure 'validateLog' picks up on incorrect CRC32-C in a record.
//
// Testing:
//   validateLog(...)
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("VALIDATE LOG - INVALID CRC32-C");

    Tester         tester;
    mqbsi::Ledger* ledger = tester.ledger();

    // 1. Write a file header
    int rc = mqbc::ClusterStateLedgerUtil::writeFileHeader(
        ledger,
        ledger->currentLog()->logConfig().logId());
    BSLS_ASSERT_OPT(rc == 0);

    // 2. Write a correct update record
    // Update record
    bmqp_ctrlmsg::LeaderMessageSequence lms1;
    lms1.electorTerm()    = 3U;
    lms1.sequenceNumber() = 8U;

    bmqp_ctrlmsg::ClusterMessage msg1;
    Tester::createClusterMessage(&msg1,
                                 AdvisoryType::e_QUEUE_ASSIGNMENT,
                                 lms1);

    bdlbb::Blob record1(tester.bufferFactory(), s_allocator_p);
    rc = mqbc::ClusterStateLedgerUtil::appendRecord(
        &record1,
        msg1,
        lms1,
        123567U,
        mqbc::ClusterStateRecordType::e_UPDATE);
    BSLS_ASSERT_OPT(rc == 0);

    mqbsi::LedgerRecordId recordId1;
    rc = ledger->writeRecord(&recordId1,
                             record1,
                             bmqu::BlobPosition(),
                             record1.length());
    BSLS_ASSERT_OPT(rc == 0);

    mqbsi::Log::Offset offset = 0;
    ASSERT_EQ(mqbc::ClusterStateLedgerUtil::validateLog(&offset,
                                                        ledger->currentLog()),
              0);

    // 3. Write an update record with incorrect CRC32-C
    // Update record
    bmqp_ctrlmsg::LeaderMessageSequence lms2;
    lms2.electorTerm()    = 3U;
    lms2.sequenceNumber() = 8U;

    bmqp_ctrlmsg::ClusterMessage msg2;
    Tester::createClusterMessage(&msg2,
                                 AdvisoryType::e_QUEUE_ASSIGNMENT,
                                 lms2);

    bdlbb::Blob record2(tester.bufferFactory(), s_allocator_p);
    rc = mqbc::ClusterStateLedgerUtil::appendRecord(
        &record2,
        msg2,
        lms2,
        123567U,
        mqbc::ClusterStateRecordType::e_UPDATE);
    BSLS_ASSERT_OPT(rc == 0);

    // Inject incorrect CRC32-C
    bdlb::BigEndianUint32 crc32c;
    crc32c = 111111;
    record2.setLength(record2.length() - sizeof(bdlb::BigEndianUint32));
    bdlbb::BlobUtil::append(&record2,
                            reinterpret_cast<const char*>(&crc32c),
                            sizeof(bdlb::BigEndianUint32));

    mqbsi::LedgerRecordId recordId2;
    rc = ledger->writeRecord(&recordId2,
                             record2,
                             bmqu::BlobPosition(),
                             record2.length());
    BSLS_ASSERT_OPT(rc == 0);

    // Validation should fail
    offset = 0;
    ASSERT_NE(mqbc::ClusterStateLedgerUtil::validateLog(&offset,
                                                        ledger->currentLog()),
              0);
}

static void test6_writeFileHeader()
// ------------------------------------------------------------------------
// WRTIE FILE HEADER
//
// Concerns:
//   Ensure proper behavior of 'writeFileHeader' method.
//
// Testing:
//   writeFileHeader(...)
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("WRTIE FILE HEADER");

    Tester                  tester;
    mqbsi::Ledger*          ledger = tester.ledger();
    const mqbu::StorageKey& logId  = ledger->currentLog()->logConfig().logId();

    // 1. Write a file header
    int rc = mqbc::ClusterStateLedgerUtil::writeFileHeader(ledger, logId);
    ASSERT_EQ(rc, 0);

    // 2. Read file header from the ledger to verify it
    mqbsi::LedgerRecordId         recordId(logId, 0);
    mqbc::ClusterStateFileHeader* header;

    BSLS_ASSERT_OPT(ledger->supportsAliasing());
    rc = ledger->aliasRecord(reinterpret_cast<void**>(&header),
                             sizeof(mqbc::ClusterStateFileHeader),
                             recordId);
    BSLS_ASSERT_OPT(rc == 0);

    ASSERT_EQ(header->protocolVersion(),
              mqbc::ClusterStateLedgerProtocol::k_VERSION);
    ASSERT_EQ(header->headerWords(),
              mqbc::ClusterStateFileHeader::k_HEADER_NUM_WORDS);
    ASSERT_EQ(header->fileKey(), logId);
}

static void test7_appendRecord()
// ------------------------------------------------------------------------
// APPEND RECORD
//
// Concerns:
//   Ensure proper behavior of 'appendRecord' method.
//
// Testing:
//   appendRecord(...)
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("APPEND RECORD");

    Tester tester;

    // 1. Create an update record
    bmqp_ctrlmsg::LeaderMessageSequence lms;
    lms.electorTerm()    = 3U;
    lms.sequenceNumber() = 8U;

    bmqp_ctrlmsg::ClusterMessage msg;
    Tester::createClusterMessage(&msg, AdvisoryType::e_QUEUE_ASSIGNMENT, lms);

    bdlbb::Blob record(tester.bufferFactory(), s_allocator_p);
    int         rc = mqbc::ClusterStateLedgerUtil::appendRecord(
        &record,
        msg,
        lms,
        123456U,
        mqbc::ClusterStateRecordType::e_UPDATE);
    ASSERT_EQ(rc, 0);

    // 2. Verify record header
    mqbc::ClusterStateRecordHeader header;
    bdlbb::BlobUtil::copy(reinterpret_cast<char*>(&header),
                          record,
                          0,
                          sizeof(mqbc::ClusterStateRecordHeader));
    rc = mqbc::ClusterStateLedgerUtil::validateRecordHeader(header);
    BSLS_ASSERT_OPT(rc == 0);

    ASSERT_EQ(header.headerWords(),
              mqbc::ClusterStateRecordHeader::k_HEADER_NUM_WORDS);
    ASSERT_EQ(header.recordType(), mqbc::ClusterStateRecordType::e_UPDATE);
    ASSERT_EQ(header.leaderAdvisoryWords(),
              (record.length() - sizeof(mqbc::ClusterStateRecordHeader)) /
                  bmqp::Protocol::k_WORD_SIZE);
    ASSERT_EQ(header.electorTerm(), 3U);
    ASSERT_EQ(header.sequenceNumber(), 8U);
    ASSERT_EQ(header.timestamp(), 123456U);

    // 3. Verify record content
    bmqp_ctrlmsg::ClusterMessage out;
    rc = mqbc::ClusterStateLedgerUtil::loadClusterMessage(&out, record);
    ASSERT_EQ(rc, 0);

    ASSERT_EQ(out, msg);
}

static void test8_loadClusterMessageLedger()
// ------------------------------------------------------------------------
// LOAD CLUSTER MESSAGE LEDGER
//
// Concerns:
//   Ensure proper behavior of the ledger flavors of 'loadClusterMessage'.
//
// Testing:
//   loadClusterMessage(bmqp_ctrlmsg::ClusterMessage    *message,
//                      const mqbsi::Ledger&             ledger,
//                      const ClusterStateRecordHeader&  recordHeader,
//                      const mqbsi::LedgerRecordId&     recordId)
//   loadClusterMessage(bmqp_ctrlmsg::ClusterMessage *message,
//                      const mqbsi::Ledger&          ledger,
//                      const mqbsi::LedgerRecordId&  recordId)
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("LOAD CLUSTER MESSAGE LEDGER");

    Tester         tester;
    mqbsi::Ledger* ledger = tester.ledger();

    // 1. Create an update record
    bmqp_ctrlmsg::LeaderMessageSequence lms;
    lms.electorTerm()    = 3U;
    lms.sequenceNumber() = 8U;

    bmqp_ctrlmsg::ClusterMessage msg;
    Tester::createClusterMessage(&msg, AdvisoryType::e_QUEUE_ASSIGNMENT, lms);

    bdlbb::Blob record(tester.bufferFactory(), s_allocator_p);
    int         rc = mqbc::ClusterStateLedgerUtil::appendRecord(
        &record,
        msg,
        lms,
        123456U,
        mqbc::ClusterStateRecordType::e_UPDATE);
    BSLS_ASSERT_OPT(rc == 0);

    // 2. Write the record to ledger
    mqbsi::LedgerRecordId recordId;
    rc = ledger->writeRecord(&recordId,
                             record,
                             bmqu::BlobPosition(),
                             record.length());
    BSLS_ASSERT_OPT(rc == 0);

    // 2. Load the cluster message from the ledger and verify
    bmqp_ctrlmsg::ClusterMessage out;
    rc = mqbc::ClusterStateLedgerUtil::loadClusterMessage(&out,
                                                          *ledger,
                                                          recordId);
    ASSERT_EQ(rc, 0);

    ASSERT_EQ(out, msg);

    // 3. Load the cluster message and verify using another flavor of
    //    loadClusterMessage()
    mqbc::ClusterStateRecordHeader header;
    bdlbb::BlobUtil::copy(reinterpret_cast<char*>(&header),
                          record,
                          0,
                          sizeof(mqbc::ClusterStateRecordHeader));
    rc = mqbc::ClusterStateLedgerUtil::validateRecordHeader(header);
    BSLS_ASSERT_OPT(rc == 0);

    bmqp_ctrlmsg::ClusterMessage out2;
    rc = mqbc::ClusterStateLedgerUtil::loadClusterMessage(&out2,
                                                          *ledger,
                                                          header,
                                                          recordId);
    ASSERT_EQ(rc, 0);

    ASSERT_EQ(out2, msg);
}

static void test9_loadClusterMessageBlob()
// ------------------------------------------------------------------------
// LOAD CLUSTER MESSAGE BLOB
//
// Concerns:
//   Ensure proper behavior of the blob flavors of 'loadClusterMessage'.
//
// Testing:
//   loadClusterMessage(bmqp_ctrlmsg::ClusterMessage    *message,
//                      const ClusterStateRecordHeader&  recordHeader,
//                      const bdlbb::Blob&               record,
//                      int                              offset)
//   loadClusterMessage(bmqp_ctrlmsg::ClusterMessage *message,
//                      const bdlbb::Blob&            record,
//                      int                           offset)
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("LOAD CLUSTER MESSAGE BLOB");

    Tester tester;

    // 1. Create an update record
    bmqp_ctrlmsg::LeaderMessageSequence lms;
    lms.electorTerm()    = 3U;
    lms.sequenceNumber() = 8U;

    bmqp_ctrlmsg::ClusterMessage msg;
    Tester::createClusterMessage(&msg, AdvisoryType::e_QUEUE_ASSIGNMENT, lms);

    bdlbb::Blob record(tester.bufferFactory(), s_allocator_p);
    int         rc = mqbc::ClusterStateLedgerUtil::appendRecord(
        &record,
        msg,
        lms,
        123456U,
        mqbc::ClusterStateRecordType::e_UPDATE);
    BSLS_ASSERT_OPT(rc == 0);

    // 2. Prepend an empty buffer to force the record to start at an offset
    const int k_EMPTY_BUFFER_LEN = 1024;

    bdlbb::BlobBuffer blobBuffer;
    tester.bufferFactory()->allocate(&blobBuffer);
    record.prependDataBuffer(blobBuffer);

    // 3. Load the cluster message from the record and verify
    bmqp_ctrlmsg::ClusterMessage out;
    rc = mqbc::ClusterStateLedgerUtil::loadClusterMessage(&out,
                                                          record,
                                                          k_EMPTY_BUFFER_LEN);
    ASSERT_EQ(rc, 0);

    ASSERT_EQ(out, msg);

    // 4. Load the cluster message and verify using another flavor of
    //    loadClusterMessage()
    mqbc::ClusterStateRecordHeader header;
    bdlbb::BlobUtil::copy(reinterpret_cast<char*>(&header),
                          record,
                          k_EMPTY_BUFFER_LEN,
                          sizeof(mqbc::ClusterStateRecordHeader));
    rc = mqbc::ClusterStateLedgerUtil::validateRecordHeader(header);
    BSLS_ASSERT_OPT(rc == 0);

    bmqp_ctrlmsg::ClusterMessage out2;
    rc = mqbc::ClusterStateLedgerUtil::loadClusterMessage(&out2,
                                                          header,
                                                          record,
                                                          k_EMPTY_BUFFER_LEN);
    ASSERT_EQ(rc, 0);

    ASSERT_EQ(out2, msg);
}

static void test10_recordSize()
// ------------------------------------------------------------------------
// RECORD SIZE
//
// Concerns:
//   Ensure proper behavior of 'recordSize' method.
//
// Testing:
//   recordSize(...)
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("RECORD SIZE");

    mqbc::ClusterStateRecordHeader header;
    header.setHeaderWords(mqbc::ClusterStateRecordHeader::k_HEADER_NUM_WORDS)
        .setRecordType(mqbc::ClusterStateRecordType::e_SNAPSHOT)
        .setLeaderAdvisoryWords(17)
        .setElectorTerm(3)
        .setSequenceNumber(100)
        .setTimestamp(123456U);

    bsls::Types::Int64 expectedRecordSize =
        (mqbc::ClusterStateRecordHeader::k_HEADER_NUM_WORDS + 17) *
        bmqp::Protocol::k_WORD_SIZE;
    ASSERT_EQ(mqbc::ClusterStateLedgerUtil::recordSize(header),
              expectedRecordSize);
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    bmqsys::Time::initialize(s_allocator_p);
    bmqp::ProtocolUtil::initialize(s_allocator_p);
    bmqp::Crc32c::initialize();

    switch (_testCase) {
    case 0:
    case 1: test1_validateFileHeader(); break;
    case 2: test2_validateRecordHeader(); break;
    case 3: test3_extractLogId(); break;
    case 4: test4_validateLog(); break;
    case 5: test5_validateLog_invalidCrc32c(); break;
    case 6: test6_writeFileHeader(); break;
    case 7: test7_appendRecord(); break;
    case 8: test8_loadClusterMessageLedger(); break;
    case 9: test9_loadClusterMessageBlob(); break;
    case 10: test10_recordSize(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        s_testStatus = -1;
    } break;
    }

    bmqp::ProtocolUtil::shutdown();
    bmqsys::Time::shutdown();

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_GBL_ALLOC);
}
