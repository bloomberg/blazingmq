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
#include "m_bmqstoragetool_compositesequencenumber.h"
#include "m_bmqstoragetool_parameters.h"
#include <m_bmqstoragetool_commandprocessorfactory.h>
#include <m_bmqstoragetool_cslfileprocessor.h>
#include <m_bmqstoragetool_filemanagermock.h>

// MQB
#include <mqbc_clusterstateledgerprotocol.h>
#include <mqbc_clusterstateledgerutil.h>
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
#include <bmqp_protocolutil.h>
#include <bmqu_memoutstream.h>

// BDE
#include <bdlbb_pooledblobbufferfactory.h>
#include <bdlf_bind.h>
#include <bdlf_placeholder.h>
#include <bsl_memory.h>
#include <bsl_string.h>
#include <bsl_utility.h>  // bsl::pair, bsl::make_pair
#include <bsl_vector.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bsls_assert.h>
#include <bsls_types.h>

// TEST DRIVER
#include <bmqsys_time.h>
#include <bmqtst_testhelper.h>
#include <bmqu_tempdirectory.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;
using namespace m_bmqstoragetool;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {

// CONSTANTS
const bsls::Types::Int64 k_LOG_MAX_SIZE       = 64 * 1024 * 1024;
const char*              k_DEFAULT_LOG_PREFIX = "BMQ_TEST_LOG_";

// TYPES
struct AdvisoryType {
    enum Enum {
        e_LEADER             = 0,
        e_PARTITION_PRIMARY  = 1,
        e_QUEUE_ASSIGNMENT   = 2,
        e_QUEUE_UNASSIGNMENT = 3,
        e_QUEUE_UNASSIGNED   = 4,
        e_COMMIT             = 5,
        e_ACK                = 6
    };
};

/// Pair of (clusterMessage, recordLength) for a record
typedef bsl::pair<bmqp_ctrlmsg::ClusterMessage, int> RecordInfo;

// FUNCTIONS

/// Load into the specified `message` an artificial cluster message of the
/// specified `advisoryType` having the specified `sequenceNumber`, `queueUri`
/// and `queueKey`.  Return the appropriate cluster state record type for the
/// message.
mqbc::ClusterStateRecordType::Enum
createClusterMessage(bmqp_ctrlmsg::ClusterMessage*              message,
                     AdvisoryType::Enum                         advisoryType,
                     const bmqp_ctrlmsg::LeaderMessageSequence& sequenceNumber,
                     const bsl::string&                         queueUri,
                     const bsl::string&                         queueKey)
{
    switch (advisoryType) {
    case AdvisoryType::e_LEADER: {
        bmqp_ctrlmsg::PartitionPrimaryInfo pinfo;
        pinfo.primaryNodeId()  = 2;
        pinfo.partitionId()    = 2U;
        pinfo.primaryLeaseId() = 1U;

        bmqp_ctrlmsg::QueueInfo qinfo;
        qinfo.partitionId() = 2U;
        qinfo.uri()         = queueUri;

        const mqbu::StorageKey key(mqbu::StorageKey::HexRepresentation(),
                                   queueKey.c_str());
        key.loadBinary(&qinfo.key());

        bmqp_ctrlmsg::LeaderAdvisory advisory;
        advisory.sequenceNumber() = sequenceNumber;
        advisory.partitions().push_back(pinfo);
        advisory.queues().push_back(qinfo);

        message->choice().makeLeaderAdvisory(advisory);

        return mqbc::ClusterStateRecordType::e_SNAPSHOT;
    }
    case AdvisoryType::e_PARTITION_PRIMARY: {
        bmqp_ctrlmsg::PartitionPrimaryInfo pinfo;
        pinfo.primaryNodeId()  = 1;
        pinfo.partitionId()    = 1U;
        pinfo.primaryLeaseId() = 1U;

        bmqp_ctrlmsg::PartitionPrimaryAdvisory advisory;
        advisory.sequenceNumber() = sequenceNumber;
        advisory.partitions().push_back(pinfo);

        message->choice().makePartitionPrimaryAdvisory(advisory);

        return mqbc::ClusterStateRecordType::e_UPDATE;
    }
    case AdvisoryType::e_QUEUE_ASSIGNMENT: {
        bmqp_ctrlmsg::QueueInfo qinfo;
        qinfo.partitionId() = 1U;
        qinfo.uri()         = queueUri;

        const mqbu::StorageKey key(mqbu::StorageKey::HexRepresentation(),
                                   queueKey.c_str());
        key.loadBinary(&qinfo.key());

        bmqp_ctrlmsg::QueueAssignmentAdvisory advisory;
        advisory.sequenceNumber() = sequenceNumber;
        advisory.queues().push_back(qinfo);

        message->choice().makeQueueAssignmentAdvisory(advisory);

        return mqbc::ClusterStateRecordType::e_UPDATE;
    }
    case AdvisoryType::e_QUEUE_UNASSIGNMENT: {
        bmqp_ctrlmsg::QueueInfo qinfo;
        qinfo.partitionId() = 1U;
        qinfo.uri()         = queueUri;

        const mqbu::StorageKey key(mqbu::StorageKey::HexRepresentation(),
                                   queueKey.c_str());
        key.loadBinary(&qinfo.key());

        bmqp_ctrlmsg::QueueUnAssignmentAdvisory advisory;
        advisory.primaryNodeId()  = 1;
        advisory.partitionId()    = 1U;
        advisory.primaryLeaseId() = 1U;
        advisory.queues().push_back(qinfo);

        message->choice().makeQueueUnAssignmentAdvisory(advisory);

        return mqbc::ClusterStateRecordType::e_UPDATE;
    }
    case AdvisoryType::e_QUEUE_UNASSIGNED: {
        bmqp_ctrlmsg::QueueInfo qinfo;
        qinfo.partitionId() = 1U;
        qinfo.uri()         = queueUri;

        const mqbu::StorageKey key(mqbu::StorageKey::HexRepresentation(),
                                   queueKey.c_str());
        key.loadBinary(&qinfo.key());

        bmqp_ctrlmsg::QueueUnassignedAdvisory advisory;
        advisory.sequenceNumber() = sequenceNumber;
        advisory.primaryNodeId()  = 1;
        advisory.partitionId()    = 1U;
        advisory.primaryLeaseId() = 1U;
        advisory.queues().push_back(qinfo);

        message->choice().makeQueueUnassignedAdvisory(advisory);

        return mqbc::ClusterStateRecordType::e_UPDATE;
    }
    case AdvisoryType::e_COMMIT: {
        bmqp_ctrlmsg::LeaderAdvisoryCommit commit;
        commit.sequenceNumber()                           = sequenceNumber;
        commit.sequenceNumberCommitted().electorTerm()    = 2U;
        commit.sequenceNumberCommitted().sequenceNumber() = 99U;

        message->choice().makeLeaderAdvisoryCommit(commit);

        return mqbc::ClusterStateRecordType::e_COMMIT;
    }
    case AdvisoryType::e_ACK: {
        bmqp_ctrlmsg::LeaderAdvisoryAck ack;
        ack.sequenceNumberAcked().electorTerm()    = 3U;
        ack.sequenceNumberAcked().sequenceNumber() = 88U;

        message->choice().makeLeaderAdvisoryAck(ack);

        return mqbc::ClusterStateRecordType::e_ACK;
    }
    }

    BSLS_ASSERT_OPT(false && "Unreachable by design.");
    return mqbc::ClusterStateRecordType::e_UNDEFINED;
}

int extractLogIdCallback(mqbu::StorageKey*                      logId,
                         const bsl::string&                     logPath,
                         bsl::shared_ptr<mqbsi::LogIdGenerator> logIdGenerator)
{
    int rc = mqbc::ClusterStateLedgerUtil::extractLogId(logId, logPath);
    BSLS_ASSERT_OPT(rc == 0);

    logIdGenerator->registerLogId(*logId);

    return 0;
}

int cleanupCallback(BSLS_ANNOTATION_UNUSED const bsl::string& logPath)
{
    return 0;
}

/// Write a record with the specified `sequenceNumber`, `timeStamp`,
/// `queueUri`, `queueKey`  and `advisoryType` to the specified `ledger` using
/// the specified `bufferFactory`.  Load into the specified `recordInfos` the
/// cluster message being written and the record length.
void writeRecord(bsl::vector<RecordInfo>*                   recordInfos,
                 const bmqp_ctrlmsg::LeaderMessageSequence& sequenceNumber,
                 bsls::Types::Uint64                        timeStamp,
                 const bsl::string&                         queueUri,
                 const bsl::string&                         queueKey,
                 AdvisoryType::Enum                         advisoryType,
                 mqbsi::Ledger*                             ledger,
                 bdlbb::BlobBufferFactory*                  bufferFactory)
{
    bmqp_ctrlmsg::ClusterMessage             msg;
    const mqbc::ClusterStateRecordType::Enum recordType = createClusterMessage(
        &msg,
        advisoryType,
        sequenceNumber,
        queueUri,
        queueKey);

    bdlbb::Blob record(bufferFactory, s_allocator_p);
    int         rc = mqbc::ClusterStateLedgerUtil::appendRecord(&record,
                                                        msg,
                                                        sequenceNumber,
                                                        timeStamp,
                                                        recordType);
    BSLS_ASSERT_OPT(rc == 0);

    mqbsi::LedgerRecordId recordId;
    rc = ledger->writeRecord(&recordId,
                             record,
                             bmqu::BlobPosition(),
                             record.length());
    BSLS_ASSERT_OPT(rc == 0);

    recordInfos->push_back(bsl::make_pair(msg, record.length()));
}

// CLASSES
// =============
// struct Tester
// =============

struct Tester {
  private:
    // DATA
    bsl::shared_ptr<mqbsi::LogIdGenerator> d_logIdGenerator_sp;
    bsl::shared_ptr<mqbsi::LogFactory>     d_logFactory_sp;
    bmqu::TempDirectory                    d_tempDir;
    mqbsi::LedgerConfig                    d_config;
    bslma::ManagedPtr<mqbsi::Ledger>       d_ledger_mp;
    bdlbb::PooledBlobBufferFactory         d_bufferFactory;

  public:
    // CREATORS
    Tester(bsls::Types::Int64 maxLogSize = k_LOG_MAX_SIZE,
           bslma::Allocator*  allocator  = s_allocator_p)
    : d_logIdGenerator_sp(0)
    , d_logFactory_sp(0)
    , d_tempDir(allocator)
    , d_config(allocator)
    , d_ledger_mp(0)
    , d_bufferFactory(1024, allocator)
    {
        // Instantiate ledger config
        d_logIdGenerator_sp.load(
            new (*allocator)
                mqbmock::LogIdGenerator(k_DEFAULT_LOG_PREFIX, allocator),
            allocator);

        d_logFactory_sp.load(
            new (*allocator) mqbsl::MemoryMappedOnDiskLogFactory(allocator),
            allocator);

        bsl::string filePattern(bsl::string(k_DEFAULT_LOG_PREFIX) + "[0-9]*" +
                                    ".bmq",
                                allocator);

        d_config.setLocation(d_tempDir.path())
            .setPattern(filePattern)
            .setMaxLogSize(maxLogSize)
            .setReserveOnDisk(false)
            .setPrefaultPages(false)
            .setLogIdGenerator(d_logIdGenerator_sp)
            .setKeepOldLogs(true)
            .setLogFactory(d_logFactory_sp)
            .setExtractLogIdCallback(
                bdlf::BindUtil::bind(&extractLogIdCallback,
                                     bdlf::PlaceHolders::_1,  // logId
                                     bdlf::PlaceHolders::_2,  // logPath
                                     d_logIdGenerator_sp))
            .setValidateLogCallback(mqbc::ClusterStateLedgerUtil::validateLog)
            .setRolloverCallback(
                bdlf::BindUtil::bind(&Tester::onRolloverCallback,
                                     this,
                                     bdlf::PlaceHolders::_1,   // oldLogId
                                     bdlf::PlaceHolders::_2))  // newLogId
            .setCleanupCallback(&cleanupCallback);

        // Create and open the ledger
        d_ledger_mp.load(new (*allocator) mqbsl::Ledger(d_config, allocator),
                         allocator);

        BSLS_ASSERT_OPT(d_ledger_mp->open(mqbsi::Log::e_CREATE_IF_MISSING) ==
                        mqbsi::LedgerOpResult::e_SUCCESS);
        BSLS_ASSERT_OPT(d_ledger_mp->totalNumBytes() ==
                        sizeof(mqbc::ClusterStateFileHeader));
    }

    ~Tester()
    {
        BSLS_ASSERT_OPT(d_ledger_mp->close() ==
                        mqbsi::LedgerOpResult::e_SUCCESS);
    }

    // MANIPULATORS
    int
    onRolloverCallback(BSLS_ANNOTATION_UNUSED const mqbu::StorageKey& oldLogId,
                       const mqbu::StorageKey&                        newLogId)
    {
        int rc = mqbc::ClusterStateLedgerUtil::writeFileHeader(
            d_ledger_mp.get(),
            newLogId);
        BSLS_ASSERT_OPT(rc == mqbc::ClusterStateLedgerUtilRc::e_SUCCESS);

        return 0;
    }

    // ACCESSORS
    mqbsi::Ledger* ledger() const { return d_ledger_mp.get(); }

    bdlbb::BlobBufferFactory* bufferFactory() { return &d_bufferFactory; }
};

}  // close anonymous namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_breathingTest()
// ------------------------------------------------------------------------
// BREATHING TEST
//
// Concerns:
//   Exercise basic functionality, i.e. process a ledger containing
//   multiple records and output short records info.
//
// Testing:
//   Basic functionality.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("BREATHING TEST");

    Tester tester;

    struct Test {
        int                                d_line;
        bsls::Types::Uint64                d_electorTerm;
        bsls::Types::Uint64                d_sequenceNumber;
        bsls::Types::Uint64                d_timeStamp;
        bsl::string                        d_queueUri;
        bsl::string                        d_queueKey;
        AdvisoryType::Enum                 d_advisoryType;
        mqbc::ClusterStateRecordType::Enum d_recordType;
    } k_DATA[] = {{L_,
                   2U,
                   1U,
                   33333U,
                   "bmq://bmq.random.y/q2",
                   "54321",
                   AdvisoryType::e_LEADER,
                   mqbc::ClusterStateRecordType::e_SNAPSHOT},
                  {L_,
                   3U,
                   1U,
                   123456U,
                   "bmq://bmq.random.y/q2",
                   "54321",
                   AdvisoryType::e_PARTITION_PRIMARY,
                   mqbc::ClusterStateRecordType::e_UPDATE},
                  {L_,
                   3U,
                   2U,
                   123567U,
                   "bmq://bmq.random.y/q2",
                   "54321",
                   AdvisoryType::e_QUEUE_ASSIGNMENT,
                   mqbc::ClusterStateRecordType::e_UPDATE},
                  {L_,
                   3U,
                   3U,
                   123567U,
                   "bmq://bmq.random.y/q2",
                   "54321",
                   AdvisoryType::e_QUEUE_UNASSIGNMENT,
                   mqbc::ClusterStateRecordType::e_UPDATE},
                  {L_,
                   3U,
                   3U,
                   123567U,
                   "bmq://bmq.random.y/q2",
                   "54321",
                   AdvisoryType::e_QUEUE_UNASSIGNED,
                   mqbc::ClusterStateRecordType::e_UPDATE},
                  {L_,
                   3U,
                   4U,
                   123678U,
                   "bmq://bmq.random.y/q2",
                   "54321",
                   AdvisoryType::e_COMMIT,
                   mqbc::ClusterStateRecordType::e_COMMIT},
                  {L_,
                   3U,
                   5U,
                   33333U,
                   "bmq://bmq.random.y/q2",
                   "54321",
                   AdvisoryType::e_LEADER,
                   mqbc::ClusterStateRecordType::e_SNAPSHOT},
                  {L_,
                   3U,
                   6U,
                   123678U,
                   "bmq://bmq.random.y/q2",
                   "54321",
                   AdvisoryType::e_ACK,
                   mqbc::ClusterStateRecordType::e_ACK}};

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    // Write a record of each advisory type
    bsl::vector<RecordInfo> recordInfos(s_allocator_p);
    recordInfos.reserve(k_NUM_DATA);
    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];

        bmqp_ctrlmsg::LeaderMessageSequence lms;
        lms.electorTerm()    = test.d_electorTerm;
        lms.sequenceNumber() = test.d_sequenceNumber;
        writeRecord(&recordInfos,
                    lms,
                    test.d_timeStamp,
                    test.d_queueUri,
                    test.d_queueKey,
                    test.d_advisoryType,
                    tester.ledger(),
                    tester.bufferFactory());
    }

    mqbc::IncoreClusterStateLedgerIterator incoreCslIt(tester.ledger());
    incoreCslIt.next();
    ASSERT(incoreCslIt.isValid());

    // Prepare parameters
    Parameters params(CommandLineArguments(s_allocator_p), s_allocator_p);
    params.d_cslMode                          = true;
    params.d_processCslRecordTypes.d_snapshot = true;
    params.d_processCslRecordTypes.d_update   = true;
    params.d_processCslRecordTypes.d_commit   = true;
    params.d_processCslRecordTypes.d_ack      = true;

    // Prepare file manager
    bslma::ManagedPtr<FileManager> fileManager(
        new (*s_allocator_p) FileManagerMock(&incoreCslIt),
        s_allocator_p);

    // Run search
    bmqu::MemOutStream                  resultStream(s_allocator_p);
    bslma::ManagedPtr<CommandProcessor> searchProcessor =
        CommandProcessorFactory::createCommandProcessor(&params,
                                                        fileManager,
                                                        resultStream,
                                                        s_allocator_p);
    searchProcessor->process();

    // Prepare expected output with list of snapshot records in CSL file
    mqbc::IncoreClusterStateLedgerIterator standardCslIt(tester.ledger());
    standardCslIt.next();
    bsl::size_t        snapshotCount = 0;
    bsl::size_t        commitCount   = 0;
    bsl::size_t        updateCount   = 0;
    bsl::size_t        ackCount      = 0;
    bmqu::MemOutStream expectedStream(s_allocator_p);
    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];
        expectedStream << "[ recordType = " << test.d_recordType
                       << " electorTerm = " << test.d_electorTerm
                       << " sequenceNumber = " << test.d_sequenceNumber
                       << " timestamp = " << test.d_timeStamp << " ]";
        expectedStream << standardCslIt.currRecordId() << '\n';
        if (test.d_recordType == mqbc::ClusterStateRecordType::e_SNAPSHOT) {
            snapshotCount++;
        }
        else if (test.d_recordType == mqbc::ClusterStateRecordType::e_UPDATE) {
            updateCount++;
        }
        else if (test.d_recordType == mqbc::ClusterStateRecordType::e_COMMIT) {
            commitCount++;
        }
        else if (test.d_recordType == mqbc::ClusterStateRecordType::e_ACK) {
            ackCount++;
        }
        standardCslIt.next();
    }
    expectedStream << snapshotCount << " snapshot record(s) found.\n";
    expectedStream << updateCount << " update record(s) found.\n";
    expectedStream << commitCount << " commit record(s) found.\n";
    expectedStream << ackCount << " ack record(s) found.\n";

    ASSERT_EQ(resultStream.str(), expectedStream.str());
}

static void test2_searchRecordsByTypeTest()
// ------------------------------------------------------------------------
// SEARCH RECORDS BY QUEUE TYPE TEST
//
// Concerns:
//   Search records by type in CSL
//   file and output short result.
//
// Testing:
//   CslFileProcessor::process()
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("SEARCH RECORDS BY TYPE TEST");

    Tester tester;

    struct Test {
        int                                d_line;
        bsls::Types::Uint64                d_electorTerm;
        bsls::Types::Uint64                d_sequenceNumber;
        bsls::Types::Uint64                d_timeStamp;
        bsl::string                        d_queueUri;
        bsl::string                        d_queueKey;
        AdvisoryType::Enum                 d_advisoryType;
        mqbc::ClusterStateRecordType::Enum d_recordType;
    } k_DATA[] = {{L_,
                   2U,
                   1U,
                   33333U,
                   "bmq://bmq.random.y/q2",
                   "54321",
                   AdvisoryType::e_LEADER,
                   mqbc::ClusterStateRecordType::e_SNAPSHOT},
                  {L_,
                   3U,
                   1U,
                   123456U,
                   "bmq://bmq.random.y/q2",
                   "54321",
                   AdvisoryType::e_PARTITION_PRIMARY,
                   mqbc::ClusterStateRecordType::e_UPDATE},
                  {L_,
                   3U,
                   2U,
                   123567U,
                   "bmq://bmq.random.y/q2",
                   "54321",
                   AdvisoryType::e_QUEUE_ASSIGNMENT,
                   mqbc::ClusterStateRecordType::e_UPDATE},
                  {L_,
                   3U,
                   3U,
                   123567U,
                   "bmq://bmq.random.y/q2",
                   "54321",
                   AdvisoryType::e_QUEUE_UNASSIGNED,
                   mqbc::ClusterStateRecordType::e_UPDATE},
                  {L_,
                   3U,
                   4U,
                   123678U,
                   "bmq://bmq.random.y/q2",
                   "54321",
                   AdvisoryType::e_COMMIT,
                   mqbc::ClusterStateRecordType::e_COMMIT},
                  {L_,
                   3U,
                   5U,
                   33333U,
                   "bmq://bmq.random.y/q2",
                   "54321",
                   AdvisoryType::e_LEADER,
                   mqbc::ClusterStateRecordType::e_SNAPSHOT},
                  {L_,
                   3U,
                   6U,
                   123678U,
                   "bmq://bmq.random.y/q2",
                   "54321",
                   AdvisoryType::e_ACK,
                   mqbc::ClusterStateRecordType::e_ACK}};

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    // Write a record of each advisory type
    bsl::vector<RecordInfo> recordInfos(s_allocator_p);
    recordInfos.reserve(k_NUM_DATA);
    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];

        bmqp_ctrlmsg::LeaderMessageSequence lms;
        lms.electorTerm()    = test.d_electorTerm;
        lms.sequenceNumber() = test.d_sequenceNumber;
        writeRecord(&recordInfos,
                    lms,
                    test.d_timeStamp,
                    test.d_queueUri,
                    test.d_queueKey,
                    test.d_advisoryType,
                    tester.ledger(),
                    tester.bufferFactory());
    }

    mqbc::IncoreClusterStateLedgerIterator incoreCslIt(tester.ledger());
    incoreCslIt.next();
    ASSERT(incoreCslIt.isValid());

    // Prepare parameters
    Parameters params(CommandLineArguments(s_allocator_p), s_allocator_p);
    params.d_cslMode                          = true;
    params.d_processCslRecordTypes.d_snapshot = true;
    params.d_processCslRecordTypes.d_commit   = true;

    // Prepare file manager
    bslma::ManagedPtr<FileManager> fileManager(
        new (*s_allocator_p) FileManagerMock(&incoreCslIt),
        s_allocator_p);

    // Run search
    bmqu::MemOutStream                  resultStream(s_allocator_p);
    bslma::ManagedPtr<CommandProcessor> searchProcessor =
        CommandProcessorFactory::createCommandProcessor(&params,
                                                        fileManager,
                                                        resultStream,
                                                        s_allocator_p);
    searchProcessor->process();

    // Prepare expected output with list of snapshot records in CSL file
    mqbc::IncoreClusterStateLedgerIterator standardCslIt(tester.ledger());
    standardCslIt.next();
    bsl::size_t        snapshotCount = 0;
    bsl::size_t        commitCount   = 0;
    bmqu::MemOutStream expectedStream(s_allocator_p);
    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];
        if (test.d_recordType == mqbc::ClusterStateRecordType::e_SNAPSHOT) {
            expectedStream << "[ recordType = " << test.d_recordType
                           << " electorTerm = " << test.d_electorTerm
                           << " sequenceNumber = " << test.d_sequenceNumber
                           << " timestamp = " << test.d_timeStamp << " ]";
            expectedStream << standardCslIt.currRecordId() << '\n';
            snapshotCount++;
        }
        else if (test.d_recordType == mqbc::ClusterStateRecordType::e_COMMIT) {
            expectedStream << "[ recordType = " << test.d_recordType
                           << " electorTerm = " << test.d_electorTerm
                           << " sequenceNumber = " << test.d_sequenceNumber
                           << " timestamp = " << test.d_timeStamp << " ]";
            expectedStream << standardCslIt.currRecordId() << '\n';
            commitCount++;
        }
        standardCslIt.next();
    }
    expectedStream << snapshotCount << " snapshot record(s) found.\n";
    expectedStream << commitCount << " commit record(s) found.\n";

    ASSERT_EQ(resultStream.str(), expectedStream.str());
}

static void test3_searchRecordsByQueueKeyTest()
// ------------------------------------------------------------------------
// SEARCH RECORDS BY QUEUE KEY TEST
//
// Concerns:
//   Search records by queue key in CSL
//   file and output short result.
//
// Testing:
//   CslFileProcessor::process()
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("SEARCH RECORDS BY QUEUE KEY TEST");

    Tester tester;

    struct Test {
        int                                d_line;
        bsls::Types::Uint64                d_electorTerm;
        bsls::Types::Uint64                d_sequenceNumber;
        bsls::Types::Uint64                d_timeStamp;
        bsl::string                        d_queueUri;
        bsl::string                        d_queueKey;
        AdvisoryType::Enum                 d_advisoryType;
        mqbc::ClusterStateRecordType::Enum d_recordType;
    } k_DATA[] = {{L_,
                   2U,
                   1U,
                   33333U,
                   "bmq://bmq.random.y/q1",
                   "1111111111",
                   AdvisoryType::e_LEADER,
                   mqbc::ClusterStateRecordType::e_SNAPSHOT},
                  {L_,
                   3U,
                   2U,
                   123567U,
                   "bmq://bmq.random.y/q2",
                   "2222222222",
                   AdvisoryType::e_QUEUE_ASSIGNMENT,
                   mqbc::ClusterStateRecordType::e_UPDATE},
                  {L_,
                   3U,
                   3U,
                   123567U,
                   "bmq://bmq.random.y/q1",
                   "1111111111",
                   AdvisoryType::e_QUEUE_UNASSIGNED,
                   mqbc::ClusterStateRecordType::e_UPDATE},
                  {L_,
                   3U,
                   2U,
                   123567U,
                   "bmq://bmq.random.y/q1",
                   "1111111111",
                   AdvisoryType::e_QUEUE_ASSIGNMENT,
                   mqbc::ClusterStateRecordType::e_UPDATE},
                  {L_,
                   3U,
                   3U,
                   123567U,
                   "bmq://bmq.random.y/q3",
                   "3333333333",
                   AdvisoryType::e_QUEUE_UNASSIGNMENT,
                   mqbc::ClusterStateRecordType::e_UPDATE},
                  {L_,
                   3U,
                   3U,
                   123567U,
                   "bmq://bmq.random.y/q3",
                   "3333333333",
                   AdvisoryType::e_QUEUE_UNASSIGNED,
                   mqbc::ClusterStateRecordType::e_UPDATE},
                  {L_,
                   3U,
                   4U,
                   123678U,
                   "bmq://bmq.random.y/q4",
                   "4444444444",
                   AdvisoryType::e_COMMIT,
                   mqbc::ClusterStateRecordType::e_COMMIT},
                  {L_,
                   3U,
                   5U,
                   33333U,
                   "bmq://bmq.random.y/q3",
                   "3333333333",
                   AdvisoryType::e_LEADER,
                   mqbc::ClusterStateRecordType::e_SNAPSHOT}};

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    // Write a record of each advisory type
    bsl::vector<RecordInfo> recordInfos(s_allocator_p);
    recordInfos.reserve(k_NUM_DATA);
    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];

        bmqp_ctrlmsg::LeaderMessageSequence lms;
        lms.electorTerm()    = test.d_electorTerm;
        lms.sequenceNumber() = test.d_sequenceNumber;
        writeRecord(&recordInfos,
                    lms,
                    test.d_timeStamp,
                    test.d_queueUri,
                    test.d_queueKey,
                    test.d_advisoryType,
                    tester.ledger(),
                    tester.bufferFactory());
    }

    mqbc::IncoreClusterStateLedgerIterator incoreCslIt(tester.ledger());
    incoreCslIt.next();
    ASSERT(incoreCslIt.isValid());

    // Prepare parameters
    Parameters params(CommandLineArguments(s_allocator_p), s_allocator_p);
    params.d_cslMode                          = true;
    params.d_processCslRecordTypes.d_snapshot = true;
    params.d_processCslRecordTypes.d_commit   = true;
    params.d_processCslRecordTypes.d_update   = true;
    params.d_queueKey.push_back("2222222222");
    params.d_queueKey.push_back("3333333333");

    // Prepare file manager
    bslma::ManagedPtr<FileManager> fileManager(
        new (*s_allocator_p) FileManagerMock(&incoreCslIt),
        s_allocator_p);

    // Run search
    bmqu::MemOutStream                  resultStream(s_allocator_p);
    bslma::ManagedPtr<CommandProcessor> searchProcessor =
        CommandProcessorFactory::createCommandProcessor(&params,
                                                        fileManager,
                                                        resultStream,
                                                        s_allocator_p);
    searchProcessor->process();

    // Prepare expected output with list of snapshot records in CSL file
    mqbc::IncoreClusterStateLedgerIterator standardCslIt(tester.ledger());
    standardCslIt.next();
    bmqu::MemOutStream expectedStream(s_allocator_p);
    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];
        if (test.d_queueKey == "2222222222" ||
            test.d_queueKey == "3333333333") {
            expectedStream << "[ recordType = " << test.d_recordType
                           << " electorTerm = " << test.d_electorTerm
                           << " sequenceNumber = " << test.d_sequenceNumber
                           << " timestamp = " << test.d_timeStamp << " ]";
            expectedStream << standardCslIt.currRecordId() << '\n';
        }
        standardCslIt.next();
    }
    expectedStream << "1 snapshot record(s) found.\n";
    expectedStream << "3 update record(s) found.\n";
    expectedStream << "No commit record(s) found.\n";

    ASSERT_EQ(resultStream.str(), expectedStream.str());
}

static void test4_searchRecordsByTimestampRangeTest()
// ------------------------------------------------------------------------
// SEARCH RECORDS BY TIMESTAMP RANGE TEST
//
// Concerns:
//   Search records by timestamp range in CSL
//   file and output short result.
//
// Testing:
//   CslFileProcessor::process()
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName(
        "SEARCH RECORDS BY TIMESTAMP RANGE TEST");

    Tester tester;

    struct Test {
        int                                d_line;
        bsls::Types::Uint64                d_electorTerm;
        bsls::Types::Uint64                d_sequenceNumber;
        bsls::Types::Uint64                d_timeStamp;
        bsl::string                        d_queueUri;
        bsl::string                        d_queueKey;
        AdvisoryType::Enum                 d_advisoryType;
        mqbc::ClusterStateRecordType::Enum d_recordType;
    } k_DATA[] = {{L_,
                   2U,
                   1U,
                   100001U,
                   "bmq://bmq.random.y/q1",
                   "1111111111",
                   AdvisoryType::e_LEADER,
                   mqbc::ClusterStateRecordType::e_SNAPSHOT},
                  {L_,
                   3U,
                   5U,
                   100002U,
                   "bmq://bmq.random.y/q1",
                   "1111111111",
                   AdvisoryType::e_LEADER,
                   mqbc::ClusterStateRecordType::e_SNAPSHOT},
                  {L_,
                   3U,
                   5U,
                   100003U,
                   "bmq://bmq.random.y/q1",
                   "1111111111",
                   AdvisoryType::e_LEADER,
                   mqbc::ClusterStateRecordType::e_SNAPSHOT},
                  {L_,
                   3U,
                   5U,
                   100004U,
                   "bmq://bmq.random.y/q1",
                   "1111111111",
                   AdvisoryType::e_LEADER,
                   mqbc::ClusterStateRecordType::e_SNAPSHOT},
                  {L_,
                   3U,
                   5U,
                   100005U,
                   "bmq://bmq.random.y/q1",
                   "1111111111",
                   AdvisoryType::e_LEADER,
                   mqbc::ClusterStateRecordType::e_SNAPSHOT}};

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    // Write a record of each advisory type
    bsl::vector<RecordInfo> recordInfos(s_allocator_p);
    recordInfos.reserve(k_NUM_DATA);
    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];

        bmqp_ctrlmsg::LeaderMessageSequence lms;
        lms.electorTerm()    = test.d_electorTerm;
        lms.sequenceNumber() = test.d_sequenceNumber;
        writeRecord(&recordInfos,
                    lms,
                    test.d_timeStamp,
                    test.d_queueUri,
                    test.d_queueKey,
                    test.d_advisoryType,
                    tester.ledger(),
                    tester.bufferFactory());
    }

    mqbc::IncoreClusterStateLedgerIterator incoreCslIt(tester.ledger());
    incoreCslIt.next();
    ASSERT(incoreCslIt.isValid());

    // Prepare parameters
    Parameters params(CommandLineArguments(s_allocator_p), s_allocator_p);
    params.d_cslMode                          = true;
    params.d_processCslRecordTypes.d_snapshot = true;
    params.d_range.d_type                     = Parameters::Range::e_TIMESTAMP;
    params.d_range.d_timestampGt              = 100002U;
    params.d_range.d_timestampLt              = 100005U;

    // Prepare file manager
    bslma::ManagedPtr<FileManager> fileManager(
        new (*s_allocator_p) FileManagerMock(&incoreCslIt),
        s_allocator_p);

    // Run search
    bmqu::MemOutStream                  resultStream(s_allocator_p);
    bslma::ManagedPtr<CommandProcessor> searchProcessor =
        CommandProcessorFactory::createCommandProcessor(&params,
                                                        fileManager,
                                                        resultStream,
                                                        s_allocator_p);
    searchProcessor->process();

    // Prepare expected output with list of snapshot records in CSL file
    mqbc::IncoreClusterStateLedgerIterator standardCslIt(tester.ledger());
    standardCslIt.next();
    bmqu::MemOutStream expectedStream(s_allocator_p);
    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];
        if (test.d_timeStamp > params.d_range.d_timestampGt &&
            test.d_timeStamp < params.d_range.d_timestampLt) {
            expectedStream << "[ recordType = " << test.d_recordType
                           << " electorTerm = " << test.d_electorTerm
                           << " sequenceNumber = " << test.d_sequenceNumber
                           << " timestamp = " << test.d_timeStamp << " ]";
            expectedStream << standardCslIt.currRecordId() << '\n';
        }
        standardCslIt.next();
    }
    expectedStream << "2 snapshot record(s) found.\n";

    ASSERT_EQ(resultStream.str(), expectedStream.str());
}

static void test5_searchRecordsByOffsetRangeTest()
// ------------------------------------------------------------------------
// SEARCH RECORDS BY OFFSET RANGE TEST
//
// Concerns:
//   Search records by offset range in CSL
//   file and output short result.
//
// Testing:
//   CslFileProcessor::process()
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("SEARCH RECORDS BY OFFSET RANGE TEST");

    Tester tester;

    struct Test {
        int                                d_line;
        bsls::Types::Uint64                d_electorTerm;
        bsls::Types::Uint64                d_sequenceNumber;
        bsls::Types::Uint64                d_timeStamp;
        bsl::string                        d_queueUri;
        bsl::string                        d_queueKey;
        AdvisoryType::Enum                 d_advisoryType;
        mqbc::ClusterStateRecordType::Enum d_recordType;
    } k_DATA[] = {{L_,
                   2U,
                   1U,
                   100001U,
                   "bmq://bmq.random.y/q1",
                   "1111111111",
                   AdvisoryType::e_LEADER,
                   mqbc::ClusterStateRecordType::e_SNAPSHOT},
                  {L_,
                   3U,
                   5U,
                   100002U,
                   "bmq://bmq.random.y/q1",
                   "1111111111",
                   AdvisoryType::e_LEADER,
                   mqbc::ClusterStateRecordType::e_SNAPSHOT},
                  {L_,
                   3U,
                   5U,
                   100003U,
                   "bmq://bmq.random.y/q1",
                   "1111111111",
                   AdvisoryType::e_LEADER,
                   mqbc::ClusterStateRecordType::e_SNAPSHOT},
                  {L_,
                   3U,
                   5U,
                   100004U,
                   "bmq://bmq.random.y/q1",
                   "1111111111",
                   AdvisoryType::e_LEADER,
                   mqbc::ClusterStateRecordType::e_SNAPSHOT},
                  {L_,
                   3U,
                   5U,
                   100005U,
                   "bmq://bmq.random.y/q1",
                   "1111111111",
                   AdvisoryType::e_LEADER,
                   mqbc::ClusterStateRecordType::e_SNAPSHOT}};

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    // Write a record of each advisory type
    bsl::vector<RecordInfo> recordInfos(s_allocator_p);
    recordInfos.reserve(k_NUM_DATA);
    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];

        bmqp_ctrlmsg::LeaderMessageSequence lms;
        lms.electorTerm()    = test.d_electorTerm;
        lms.sequenceNumber() = test.d_sequenceNumber;
        writeRecord(&recordInfos,
                    lms,
                    test.d_timeStamp,
                    test.d_queueUri,
                    test.d_queueKey,
                    test.d_advisoryType,
                    tester.ledger(),
                    tester.bufferFactory());
    }

    mqbc::IncoreClusterStateLedgerIterator incoreCslIt(tester.ledger());
    incoreCslIt.next();
    ASSERT(incoreCslIt.isValid());

    // Prepare parameters
    Parameters params(CommandLineArguments(s_allocator_p), s_allocator_p);
    params.d_cslMode                          = true;
    params.d_processCslRecordTypes.d_snapshot = true;
    params.d_range.d_type                     = Parameters::Range::e_OFFSET;
    params.d_range.d_offsetGt                 = 100U;
    params.d_range.d_offsetLt                 = 300U;

    // Prepare file manager
    bslma::ManagedPtr<FileManager> fileManager(
        new (*s_allocator_p) FileManagerMock(&incoreCslIt),
        s_allocator_p);

    // Run search
    bmqu::MemOutStream                  resultStream(s_allocator_p);
    bslma::ManagedPtr<CommandProcessor> searchProcessor =
        CommandProcessorFactory::createCommandProcessor(&params,
                                                        fileManager,
                                                        resultStream,
                                                        s_allocator_p);
    searchProcessor->process();

    // Prepare expected output with list of snapshot records in CSL file
    mqbc::IncoreClusterStateLedgerIterator standardCslIt(tester.ledger());
    standardCslIt.next();
    bmqu::MemOutStream expectedStream(s_allocator_p);
    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test&         test      = k_DATA[idx];
        bsls::Types::Uint64 recOffset = static_cast<bsls::Types::Uint64>(
            standardCslIt.currRecordId().offset());
        if (recOffset > params.d_range.d_offsetGt &&
            recOffset < params.d_range.d_offsetLt) {
            expectedStream << "[ recordType = " << test.d_recordType
                           << " electorTerm = " << test.d_electorTerm
                           << " sequenceNumber = " << test.d_sequenceNumber
                           << " timestamp = " << test.d_timeStamp << " ]";
            expectedStream << standardCslIt.currRecordId() << '\n';
        }
        standardCslIt.next();
    }
    expectedStream << "2 snapshot record(s) found.\n";

    ASSERT_EQ(resultStream.str(), expectedStream.str());
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
    case 1: test1_breathingTest(); break;
    case 2: test2_searchRecordsByTypeTest(); break;
    case 3: test3_searchRecordsByQueueKeyTest(); break;
    case 4: test4_searchRecordsByTimestampRangeTest(); break;
    case 5: test5_searchRecordsByOffsetRangeTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        s_testStatus = -1;
    } break;
    }

    bmqp::ProtocolUtil::shutdown();
    bmqsys::Time::shutdown();

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_GBL_ALLOC);
}
