// Copyright 2025 Bloomberg Finance L.P.
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
#include "m_bmqstoragetool_parameters.h"
#include <m_bmqstoragetool_commandprocessorfactory.h>
#include <m_bmqstoragetool_compositesequencenumber.h>
#include <m_bmqstoragetool_cslfileprocessor.h>
#include <m_bmqstoragetool_cslprinter.h>
#include <m_bmqstoragetool_filemanagermock.h>
#include <m_bmqstoragetool_printermock.h>
#include <m_bmqstoragetool_searchresultfactory.h>

// MQB
#include <mqbc_clusterstateledgerprotocol.h>
#include <mqbc_clusterstateledgerutil.h>
#include <mqbmock_logidgenerator.h>
#include <mqbsi_ledger.h>
#include <mqbu_storagekey.h>

// BMQ
#include <bmqp_crc32c.h>
#include <bmqp_ctrlmsg_messages.h>

// BDE
#include <bdlbb_pooledblobbufferfactory.h>

// TEST DRIVER
#include <bmqsys_time.h>
#include <bmqtst_testhelper.h>
#include <bmqu_tempdirectory.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;
using namespace m_bmqstoragetool;
using namespace bmqp_ctrlmsg;
using ::testing::_;
using ::testing::Eq;
using ::testing::Field;
using ::testing::Invoke;
using ::testing::Property;
using ::testing::Sequence;

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

    bdlbb::Blob record(bufferFactory, bmqtst::TestHelperUtil::allocator());
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

/// Create a 'CommandProcessor' instance with the specified arguments
bslma::ManagedPtr<CommandProcessor>
createCommandProcessor(const Parameters*                  params,
                       const bsl::shared_ptr<CslPrinter>& printer,
                       bslma::ManagedPtr<FileManager>&    fileManager,
                       bsl::ostream&                      ostream,
                       bslma::Allocator*                  allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT(params);

    bslma::Allocator* alloc = bslma::Default::allocator(allocator);

    // Create searchResult for given 'params'.
    bsl::shared_ptr<CslSearchResult> searchResult =
        SearchResultFactory::createCslSearchResult(params, printer, alloc);
    // Create commandProcessor.
    bslma::ManagedPtr<CommandProcessor> commandProcessor(
        new (*alloc) CslFileProcessor(params,
                                      fileManager,
                                      searchResult,
                                      ostream,
                                      alloc),
        alloc);
    return commandProcessor;
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
           bslma::Allocator*  allocator  = bmqtst::TestHelperUtil::allocator())
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
//   CslFileProcessor::process()
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
                   "1111111111",
                   AdvisoryType::e_LEADER,
                   mqbc::ClusterStateRecordType::e_SNAPSHOT},
                  {L_,
                   3U,
                   1U,
                   123456U,
                   "bmq://bmq.random.y/q2",
                   "1111111111",
                   AdvisoryType::e_PARTITION_PRIMARY,
                   mqbc::ClusterStateRecordType::e_UPDATE},
                  {L_,
                   3U,
                   2U,
                   123567U,
                   "bmq://bmq.random.y/q2",
                   "1111111111",
                   AdvisoryType::e_QUEUE_ASSIGNMENT,
                   mqbc::ClusterStateRecordType::e_UPDATE},
                  {L_,
                   3U,
                   3U,
                   123567U,
                   "bmq://bmq.random.y/q2",
                   "1111111111",
                   AdvisoryType::e_QUEUE_UNASSIGNMENT,
                   mqbc::ClusterStateRecordType::e_UPDATE},
                  {L_,
                   3U,
                   3U,
                   123567U,
                   "bmq://bmq.random.y/q2",
                   "1111111111",
                   AdvisoryType::e_QUEUE_UNASSIGNED,
                   mqbc::ClusterStateRecordType::e_UPDATE},
                  {L_,
                   3U,
                   4U,
                   123678U,
                   "bmq://bmq.random.y/q2",
                   "1111111111",
                   AdvisoryType::e_COMMIT,
                   mqbc::ClusterStateRecordType::e_COMMIT},
                  {L_,
                   3U,
                   5U,
                   33333U,
                   "bmq://bmq.random.y/q2",
                   "1111111111",
                   AdvisoryType::e_LEADER,
                   mqbc::ClusterStateRecordType::e_SNAPSHOT},
                  {L_,
                   3U,
                   6U,
                   123678U,
                   "bmq://bmq.random.y/q2",
                   "1111111111",
                   AdvisoryType::e_ACK,
                   mqbc::ClusterStateRecordType::e_ACK}};

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    // Write records to CSL
    bsl::vector<RecordInfo> recordInfos(bmqtst::TestHelperUtil::allocator());
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
    Parameters params(
        CommandLineArguments(bmqtst::TestHelperUtil::allocator()),
        bmqtst::TestHelperUtil::allocator());
    params.d_cslMode                          = true;
    params.d_processCslRecordTypes.d_snapshot = true;
    params.d_processCslRecordTypes.d_update   = true;
    params.d_processCslRecordTypes.d_commit   = true;
    params.d_processCslRecordTypes.d_ack      = true;

    // Prepare file manager
    bslma::ManagedPtr<FileManager> fileManager(
        new (*bmqtst::TestHelperUtil::allocator())
            FileManagerMock(&incoreCslIt),
        bmqtst::TestHelperUtil::allocator());

    // Create printer mock
    bsl::shared_ptr<CslPrinterMock> printer(
        new (*bmqtst::TestHelperUtil::allocator()) CslPrinterMock(),
        bmqtst::TestHelperUtil::allocator());

    // Create command processor
    bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
    bslma::ManagedPtr<CommandProcessor> searchProcessor =
        createCommandProcessor(&params,
                               printer,
                               fileManager,
                               resultStream,
                               bmqtst::TestHelperUtil::allocator());

    // Prepare expected output with list of snapshot records in CSL file
    mqbc::IncoreClusterStateLedgerIterator standardCslIt(tester.ledger());
    CslRecordCount                         recordCount;
    Sequence                               s;
    while (standardCslIt.next() == 0) {
        // standardCslIt.header() has no `operator==` defined, so skip it by
        // using `_`
        EXPECT_CALL(*printer,
                    printShortResult(_, standardCslIt.currRecordId()))
            .InSequence(s);

        if (standardCslIt.header().recordType() ==
            mqbc::ClusterStateRecordType::e_SNAPSHOT) {
            recordCount.d_snapshotCount++;
        }
        else if (standardCslIt.header().recordType() ==
                 mqbc::ClusterStateRecordType::e_UPDATE) {
            recordCount.d_updateCount++;
        }
        else if (standardCslIt.header().recordType() ==
                 mqbc::ClusterStateRecordType::e_COMMIT) {
            recordCount.d_commitCount++;
        }
        else if (standardCslIt.header().recordType() ==
                 mqbc::ClusterStateRecordType::e_ACK) {
            recordCount.d_ackCount++;
        }
    }

    EXPECT_CALL(*printer,
                printFooter(recordCount, params.d_processCslRecordTypes))
        .InSequence(s);

    // Run search
    searchProcessor->process();
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
                   "1111111111",
                   AdvisoryType::e_LEADER,
                   mqbc::ClusterStateRecordType::e_SNAPSHOT},
                  {L_,
                   3U,
                   1U,
                   123456U,
                   "bmq://bmq.random.y/q2",
                   "1111111111",
                   AdvisoryType::e_PARTITION_PRIMARY,
                   mqbc::ClusterStateRecordType::e_UPDATE},
                  {L_,
                   3U,
                   2U,
                   123567U,
                   "bmq://bmq.random.y/q2",
                   "1111111111",
                   AdvisoryType::e_QUEUE_ASSIGNMENT,
                   mqbc::ClusterStateRecordType::e_UPDATE},
                  {L_,
                   3U,
                   3U,
                   123567U,
                   "bmq://bmq.random.y/q2",
                   "1111111111",
                   AdvisoryType::e_QUEUE_UNASSIGNED,
                   mqbc::ClusterStateRecordType::e_UPDATE},
                  {L_,
                   3U,
                   4U,
                   123678U,
                   "bmq://bmq.random.y/q2",
                   "1111111111",
                   AdvisoryType::e_COMMIT,
                   mqbc::ClusterStateRecordType::e_COMMIT},
                  {L_,
                   3U,
                   5U,
                   33333U,
                   "bmq://bmq.random.y/q2",
                   "1111111111",
                   AdvisoryType::e_LEADER,
                   mqbc::ClusterStateRecordType::e_SNAPSHOT},
                  {L_,
                   3U,
                   6U,
                   123678U,
                   "bmq://bmq.random.y/q2",
                   "1111111111",
                   AdvisoryType::e_ACK,
                   mqbc::ClusterStateRecordType::e_ACK}};

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    // Write records to CSL
    bsl::vector<RecordInfo> recordInfos(bmqtst::TestHelperUtil::allocator());
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
    Parameters params(
        CommandLineArguments(bmqtst::TestHelperUtil::allocator()),
        bmqtst::TestHelperUtil::allocator());
    params.d_cslMode                          = true;
    params.d_processCslRecordTypes.d_snapshot = true;
    params.d_processCslRecordTypes.d_commit   = true;

    // Prepare file manager
    bslma::ManagedPtr<FileManager> fileManager(
        new (*bmqtst::TestHelperUtil::allocator())
            FileManagerMock(&incoreCslIt),
        bmqtst::TestHelperUtil::allocator());

    // Create printer mock
    bsl::shared_ptr<CslPrinterMock> printer(
        new (*bmqtst::TestHelperUtil::allocator()) CslPrinterMock(),
        bmqtst::TestHelperUtil::allocator());

    // Create command processor
    bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
    bslma::ManagedPtr<CommandProcessor> searchProcessor =
        createCommandProcessor(&params,
                               printer,
                               fileManager,
                               resultStream,
                               bmqtst::TestHelperUtil::allocator());

    // Prepare expected output
    mqbc::IncoreClusterStateLedgerIterator standardCslIt(tester.ledger());
    CslRecordCount                         recordCount;
    Sequence                               s;
    while (standardCslIt.next() == 0) {
        if (standardCslIt.header().recordType() ==
            mqbc::ClusterStateRecordType::e_SNAPSHOT) {
            // standardCslIt.header() has no `operator==` defined, so skip it
            // by using `_`
            EXPECT_CALL(*printer,
                        printShortResult(_, standardCslIt.currRecordId()))
                .InSequence(s);
            recordCount.d_snapshotCount++;
        }
        else if (standardCslIt.header().recordType() ==
                 mqbc::ClusterStateRecordType::e_COMMIT) {
            // standardCslIt.header() has no `operator==` defined, so skip it
            // by using `_`
            EXPECT_CALL(*printer,
                        printShortResult(_, standardCslIt.currRecordId()))
                .InSequence(s);
            recordCount.d_commitCount++;
        }
    }

    EXPECT_CALL(*printer,
                printFooter(recordCount, params.d_processCslRecordTypes))
        .InSequence(s);

    // Run search
    searchProcessor->process();
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

    // Write records to CSL
    bsl::vector<RecordInfo> recordInfos(bmqtst::TestHelperUtil::allocator());
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
    Parameters params(
        CommandLineArguments(bmqtst::TestHelperUtil::allocator()),
        bmqtst::TestHelperUtil::allocator());
    params.d_cslMode                          = true;
    params.d_processCslRecordTypes.d_snapshot = true;
    params.d_processCslRecordTypes.d_commit   = true;
    params.d_processCslRecordTypes.d_update   = true;
    params.d_queueKey.push_back("2222222222");
    params.d_queueKey.push_back("3333333333");

    // Prepare file manager
    bslma::ManagedPtr<FileManager> fileManager(
        new (*bmqtst::TestHelperUtil::allocator())
            FileManagerMock(&incoreCslIt),
        bmqtst::TestHelperUtil::allocator());

    // Create printer mock
    bsl::shared_ptr<CslPrinterMock> printer(
        new (*bmqtst::TestHelperUtil::allocator()) CslPrinterMock(),
        bmqtst::TestHelperUtil::allocator());

    // Create command processor
    bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
    bslma::ManagedPtr<CommandProcessor> searchProcessor =
        createCommandProcessor(&params,
                               printer,
                               fileManager,
                               resultStream,
                               bmqtst::TestHelperUtil::allocator());

    // Prepare expected output
    mqbc::IncoreClusterStateLedgerIterator standardCslIt(tester.ledger());
    Sequence                               s;
    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        standardCslIt.next();

        const Test& test = k_DATA[idx];
        if (test.d_queueKey == "2222222222" ||
            test.d_queueKey == "3333333333") {
            // standardCslIt.header() has no `operator==` defined, so skip it
            // by using `_`
            EXPECT_CALL(*printer,
                        printShortResult(_, standardCslIt.currRecordId()))
                .InSequence(s);
        }
    }

    CslRecordCount recordCount;
    recordCount.d_snapshotCount = 1;
    recordCount.d_updateCount   = 3;
    EXPECT_CALL(*printer,
                printFooter(recordCount, params.d_processCslRecordTypes))
        .InSequence(s);

    // Run search
    searchProcessor->process();
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

    // Write records to CSL
    bsl::vector<RecordInfo> recordInfos(bmqtst::TestHelperUtil::allocator());
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
    Parameters params(
        CommandLineArguments(bmqtst::TestHelperUtil::allocator()),
        bmqtst::TestHelperUtil::allocator());
    params.d_cslMode                          = true;
    params.d_processCslRecordTypes.d_snapshot = true;
    params.d_range.d_timestampGt              = 100002U;
    params.d_range.d_timestampLt              = 100005U;

    // Prepare file manager
    bslma::ManagedPtr<FileManager> fileManager(
        new (*bmqtst::TestHelperUtil::allocator())
            FileManagerMock(&incoreCslIt),
        bmqtst::TestHelperUtil::allocator());

    // Create printer mock
    bsl::shared_ptr<CslPrinterMock> printer(
        new (*bmqtst::TestHelperUtil::allocator()) CslPrinterMock(),
        bmqtst::TestHelperUtil::allocator());

    // Create command processor
    bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
    bslma::ManagedPtr<CommandProcessor> searchProcessor =
        createCommandProcessor(&params,
                               printer,
                               fileManager,
                               resultStream,
                               bmqtst::TestHelperUtil::allocator());

    // Prepare expected output with list of snapshot records in CSL file
    mqbc::IncoreClusterStateLedgerIterator standardCslIt(tester.ledger());
    Sequence                               s;
    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        standardCslIt.next();

        const Test& test = k_DATA[idx];
        if (test.d_timeStamp > params.d_range.d_timestampGt &&
            test.d_timeStamp < params.d_range.d_timestampLt) {
            // standardCslIt.header() has no `operator==` defined, so skip it
            // by using `_`
            EXPECT_CALL(*printer,
                        printShortResult(_, standardCslIt.currRecordId()))
                .InSequence(s);
        }
    }

    CslRecordCount recordCount;
    recordCount.d_snapshotCount = 2;
    EXPECT_CALL(*printer,
                printFooter(recordCount, params.d_processCslRecordTypes))
        .InSequence(s);

    // Run search
    searchProcessor->process();
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

    // Write records to CSL
    bsl::vector<RecordInfo> recordInfos(bmqtst::TestHelperUtil::allocator());
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
    Parameters params(
        CommandLineArguments(bmqtst::TestHelperUtil::allocator()),
        bmqtst::TestHelperUtil::allocator());
    params.d_cslMode                          = true;
    params.d_processCslRecordTypes.d_snapshot = true;
    params.d_range.d_offsetGt                 = 100U;
    params.d_range.d_offsetLt                 = 300U;

    // Prepare file manager
    bslma::ManagedPtr<FileManager> fileManager(
        new (*bmqtst::TestHelperUtil::allocator())
            FileManagerMock(&incoreCslIt),
        bmqtst::TestHelperUtil::allocator());

    // Create printer mock
    bsl::shared_ptr<CslPrinterMock> printer(
        new (*bmqtst::TestHelperUtil::allocator()) CslPrinterMock(),
        bmqtst::TestHelperUtil::allocator());

    bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
    bslma::ManagedPtr<CommandProcessor> searchProcessor =
        createCommandProcessor(&params,
                               printer,
                               fileManager,
                               resultStream,
                               bmqtst::TestHelperUtil::allocator());

    // Prepare expected output with list of snapshot records in CSL file
    mqbc::IncoreClusterStateLedgerIterator standardCslIt(tester.ledger());
    Sequence                               s;
    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        standardCslIt.next();

        bsls::Types::Uint64 recOffset = static_cast<bsls::Types::Uint64>(
            standardCslIt.currRecordId().offset());
        if (recOffset > params.d_range.d_offsetGt &&
            recOffset < params.d_range.d_offsetLt) {
            // standardCslIt.header() has no `operator==` defined, so skip it
            // by using `_`
            EXPECT_CALL(*printer,
                        printShortResult(_, standardCslIt.currRecordId()))
                .InSequence(s);
        }
    }

    CslRecordCount recordCount;
    recordCount.d_snapshotCount = 2;
    EXPECT_CALL(*printer,
                printFooter(recordCount, params.d_processCslRecordTypes))
        .InSequence(s);

    // Run search
    searchProcessor->process();
}

static void test6_searchRecordsBySeqNumberRangeTest()
// ------------------------------------------------------------------------
// SEARCH RECORDS BY SEQUENCE NUMBER RANGE TEST
//
// Concerns:
//   Search records by sequence number range in CSL
//   file and output short result.
//
// Testing:
//   CslFileProcessor::process()
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName(
        "SEARCH RECORDS BY SEQUENCE NUMBER RANGE TEST");

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
                   1U,
                   1U,
                   100001U,
                   "bmq://bmq.random.y/q1",
                   "1111111111",
                   AdvisoryType::e_LEADER,
                   mqbc::ClusterStateRecordType::e_SNAPSHOT},
                  {L_,
                   1U,
                   2U,
                   100002U,
                   "bmq://bmq.random.y/q1",
                   "1111111111",
                   AdvisoryType::e_LEADER,
                   mqbc::ClusterStateRecordType::e_SNAPSHOT},
                  {L_,
                   2U,
                   1U,
                   100003U,
                   "bmq://bmq.random.y/q1",
                   "1111111111",
                   AdvisoryType::e_LEADER,
                   mqbc::ClusterStateRecordType::e_SNAPSHOT},
                  {L_,
                   2U,
                   2U,
                   100004U,
                   "bmq://bmq.random.y/q1",
                   "1111111111",
                   AdvisoryType::e_LEADER,
                   mqbc::ClusterStateRecordType::e_SNAPSHOT},
                  {L_,
                   2U,
                   3U,
                   100005U,
                   "bmq://bmq.random.y/q1",
                   "1111111111",
                   AdvisoryType::e_LEADER,
                   mqbc::ClusterStateRecordType::e_SNAPSHOT}};

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    // Write records to CSL
    bsl::vector<RecordInfo> recordInfos(bmqtst::TestHelperUtil::allocator());
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
    Parameters params(
        CommandLineArguments(bmqtst::TestHelperUtil::allocator()),
        bmqtst::TestHelperUtil::allocator());
    params.d_cslMode                          = true;
    params.d_processCslRecordTypes.d_snapshot = true;
    params.d_range.d_seqNumGt = CompositeSequenceNumber(1U, 2U);
    params.d_range.d_seqNumLt = CompositeSequenceNumber(2U, 3U);

    // Prepare file manager
    bslma::ManagedPtr<FileManager> fileManager(
        new (*bmqtst::TestHelperUtil::allocator())
            FileManagerMock(&incoreCslIt),
        bmqtst::TestHelperUtil::allocator());

    // Create printer mock
    bsl::shared_ptr<CslPrinterMock> printer(
        new (*bmqtst::TestHelperUtil::allocator()) CslPrinterMock(),
        bmqtst::TestHelperUtil::allocator());

    // Create command processor
    bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
    bslma::ManagedPtr<CommandProcessor> searchProcessor =
        createCommandProcessor(&params,
                               printer,
                               fileManager,
                               resultStream,
                               bmqtst::TestHelperUtil::allocator());

    // Prepare expected output with list of snapshot records in CSL file
    mqbc::IncoreClusterStateLedgerIterator standardCslIt(tester.ledger());
    Sequence                               s;
    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        standardCslIt.next();

        const Test&             test = k_DATA[idx];
        CompositeSequenceNumber recSeqNum(test.d_electorTerm,
                                          test.d_sequenceNumber);
        if (params.d_range.d_seqNumGt < recSeqNum &&
            recSeqNum < params.d_range.d_seqNumLt) {
            // standardCslIt.header() has no `operator==` defined, so skip it
            // by using `_`
            EXPECT_CALL(*printer,
                        printShortResult(_, standardCslIt.currRecordId()))
                .InSequence(s);
        }
    }

    CslRecordCount recordCount;
    recordCount.d_snapshotCount = 2;
    EXPECT_CALL(*printer,
                printFooter(recordCount, params.d_processCslRecordTypes))
        .InSequence(s);

    // Run search
    searchProcessor->process();
}

static void test7_searchRecordsByOffsetTest()
// ------------------------------------------------------------------------
// SEARCH RECORDS BY OFFSET TEST
//
// Concerns:
//   Search records by еру given offset in CSL
//   file and output short result.
//
// Testing:
//   CslFileProcessor::process()
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("SEARCH RECORDS BY OFFSET TEST");

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

    // Write records to CSL
    bsl::vector<RecordInfo> recordInfos(bmqtst::TestHelperUtil::allocator());
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
    Parameters params(
        CommandLineArguments(bmqtst::TestHelperUtil::allocator()),
        bmqtst::TestHelperUtil::allocator());
    params.d_cslMode                          = true;
    params.d_processCslRecordTypes.d_snapshot = true;
    params.d_offset.push_back(132U);
    params.d_offset.push_back(256U);

    // Prepare file manager
    bslma::ManagedPtr<FileManager> fileManager(
        new (*bmqtst::TestHelperUtil::allocator())
            FileManagerMock(&incoreCslIt),
        bmqtst::TestHelperUtil::allocator());

    // Create printer mock
    bsl::shared_ptr<CslPrinterMock> printer(
        new (*bmqtst::TestHelperUtil::allocator()) CslPrinterMock(),
        bmqtst::TestHelperUtil::allocator());

    bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
    bslma::ManagedPtr<CommandProcessor> searchProcessor =
        createCommandProcessor(&params,
                               printer,
                               fileManager,
                               resultStream,
                               bmqtst::TestHelperUtil::allocator());

    // Prepare expected output with list of snapshot records in CSL file
    mqbc::IncoreClusterStateLedgerIterator standardCslIt(tester.ledger());
    Sequence                               s;
    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        standardCslIt.next();

        bsls::Types::Uint64 recOffset = static_cast<bsls::Types::Uint64>(
            standardCslIt.currRecordId().offset());
        if (recOffset == 132U || recOffset == 256U) {
            // standardCslIt.header() has no `operator==` defined, so skip it
            // by using `_`
            EXPECT_CALL(*printer,
                        printShortResult(_, standardCslIt.currRecordId()))
                .InSequence(s);
        }
    }

    CslRecordCount recordCount;
    recordCount.d_snapshotCount = 2;
    EXPECT_CALL(*printer,
                printFooter(recordCount, params.d_processCslRecordTypes))
        .InSequence(s);

    // Run search
    searchProcessor->process();
}

static void test8_searchRecordsBySeqNumberTest()
// ------------------------------------------------------------------------
// SEARCH RECORDS BY SEQUENCE NUMBER TEST
//
// Concerns:
//   Search records by the given sequence number in CSL
//   file and output short result.
//
// Testing:
//   CslFileProcessor::process()
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName(
        "SEARCH RECORDS BY SEQUENCE NUMBER TEST");

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
                   1U,
                   1U,
                   100001U,
                   "bmq://bmq.random.y/q1",
                   "1111111111",
                   AdvisoryType::e_LEADER,
                   mqbc::ClusterStateRecordType::e_SNAPSHOT},
                  {L_,
                   1U,
                   2U,
                   100002U,
                   "bmq://bmq.random.y/q1",
                   "1111111111",
                   AdvisoryType::e_LEADER,
                   mqbc::ClusterStateRecordType::e_SNAPSHOT},
                  {L_,
                   2U,
                   1U,
                   100003U,
                   "bmq://bmq.random.y/q1",
                   "1111111111",
                   AdvisoryType::e_LEADER,
                   mqbc::ClusterStateRecordType::e_SNAPSHOT},
                  {L_,
                   2U,
                   2U,
                   100004U,
                   "bmq://bmq.random.y/q1",
                   "1111111111",
                   AdvisoryType::e_LEADER,
                   mqbc::ClusterStateRecordType::e_SNAPSHOT},
                  {L_,
                   2U,
                   3U,
                   100005U,
                   "bmq://bmq.random.y/q1",
                   "1111111111",
                   AdvisoryType::e_LEADER,
                   mqbc::ClusterStateRecordType::e_SNAPSHOT}};

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    // Write records to CSL
    bsl::vector<RecordInfo> recordInfos(bmqtst::TestHelperUtil::allocator());
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
    Parameters params(
        CommandLineArguments(bmqtst::TestHelperUtil::allocator()),
        bmqtst::TestHelperUtil::allocator());
    params.d_cslMode                          = true;
    params.d_processCslRecordTypes.d_snapshot = true;
    params.d_seqNum.push_back(CompositeSequenceNumber(1U, 2U));
    params.d_seqNum.push_back(CompositeSequenceNumber(2U, 2U));

    // Prepare file manager
    bslma::ManagedPtr<FileManager> fileManager(
        new (*bmqtst::TestHelperUtil::allocator())
            FileManagerMock(&incoreCslIt),
        bmqtst::TestHelperUtil::allocator());

    // Create printer mock
    bsl::shared_ptr<CslPrinterMock> printer(
        new (*bmqtst::TestHelperUtil::allocator()) CslPrinterMock(),
        bmqtst::TestHelperUtil::allocator());

    // Create command processor
    bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
    bslma::ManagedPtr<CommandProcessor> searchProcessor =
        createCommandProcessor(&params,
                               printer,
                               fileManager,
                               resultStream,
                               bmqtst::TestHelperUtil::allocator());

    // Prepare expected output with list of snapshot records in CSL file
    mqbc::IncoreClusterStateLedgerIterator standardCslIt(tester.ledger());
    Sequence                               s;
    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        standardCslIt.next();

        const Test& test = k_DATA[idx];
        if ((test.d_electorTerm == 1U || test.d_electorTerm == 2U) &&
            test.d_sequenceNumber == 2U) {
            // standardCslIt.header() has no `operator==` defined, so skip it
            // by using `_`
            EXPECT_CALL(*printer,
                        printShortResult(_, standardCslIt.currRecordId()))
                .InSequence(s);
        }
    }

    CslRecordCount recordCount;
    recordCount.d_snapshotCount = 2;
    EXPECT_CALL(*printer,
                printFooter(recordCount, params.d_processCslRecordTypes))
        .InSequence(s);

    // Run search
    searchProcessor->process();
}

static void test9_summaryTest()
// ------------------------------------------------------------------------
// SUMMARY TEST
//
// Concerns:
//   Exercise summary functionality, i.e. process a ledger containing
//   multiple records and output summary info.
//
// Testing:
//   CslFileProcessor::process()
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("SUMMARY TEST");

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
                   "1111111111",
                   AdvisoryType::e_LEADER,
                   mqbc::ClusterStateRecordType::e_SNAPSHOT},
                  {L_,
                   3U,
                   1U,
                   123456U,
                   "bmq://bmq.random.y/q2",
                   "1111111111",
                   AdvisoryType::e_PARTITION_PRIMARY,
                   mqbc::ClusterStateRecordType::e_UPDATE},
                  {L_,
                   3U,
                   2U,
                   123567U,
                   "bmq://bmq.random.y/q2",
                   "1111111111",
                   AdvisoryType::e_QUEUE_ASSIGNMENT,
                   mqbc::ClusterStateRecordType::e_UPDATE},
                  {L_,
                   3U,
                   3U,
                   123567U,
                   "bmq://bmq.random.y/q2",
                   "1111111111",
                   AdvisoryType::e_QUEUE_UNASSIGNMENT,
                   mqbc::ClusterStateRecordType::e_UPDATE},
                  {L_,
                   3U,
                   3U,
                   123567U,
                   "bmq://bmq.random.y/q2",
                   "1111111111",
                   AdvisoryType::e_QUEUE_UNASSIGNED,
                   mqbc::ClusterStateRecordType::e_UPDATE},
                  {L_,
                   3U,
                   4U,
                   123678U,
                   "bmq://bmq.random.y/q2",
                   "1111111111",
                   AdvisoryType::e_COMMIT,
                   mqbc::ClusterStateRecordType::e_COMMIT},
                  {L_,
                   3U,
                   5U,
                   33333U,
                   "bmq://bmq.random.y/q2",
                   "1111111111",
                   AdvisoryType::e_LEADER,
                   mqbc::ClusterStateRecordType::e_SNAPSHOT},
                  {L_,
                   3U,
                   6U,
                   123678U,
                   "bmq://bmq.random.y/q2",
                   "1111111111",
                   AdvisoryType::e_ACK,
                   mqbc::ClusterStateRecordType::e_ACK}};

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    // Write records to CSL
    bsl::vector<RecordInfo> recordInfos(bmqtst::TestHelperUtil::allocator());
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
    Parameters params(
        CommandLineArguments(bmqtst::TestHelperUtil::allocator()),
        bmqtst::TestHelperUtil::allocator());
    params.d_cslMode                          = true;
    params.d_processCslRecordTypes.d_snapshot = true;
    params.d_processCslRecordTypes.d_update   = true;
    params.d_processCslRecordTypes.d_commit   = true;
    params.d_processCslRecordTypes.d_ack      = true;
    params.d_summary                          = true;
    params.d_cslSummaryQueuesLimit            = 123;

    // Prepare file manager
    bslma::ManagedPtr<FileManager> fileManager(
        new (*bmqtst::TestHelperUtil::allocator())
            FileManagerMock(&incoreCslIt),
        bmqtst::TestHelperUtil::allocator());

    // Create printer mock
    bsl::shared_ptr<CslPrinterMock> printer(
        new (*bmqtst::TestHelperUtil::allocator()) CslPrinterMock(),
        bmqtst::TestHelperUtil::allocator());

    // Create command processor
    bmqu::MemOutStream resultStream(bmqtst::TestHelperUtil::allocator());
    bslma::ManagedPtr<CommandProcessor> searchProcessor =
        createCommandProcessor(&params,
                               printer,
                               fileManager,
                               resultStream,
                               bmqtst::TestHelperUtil::allocator());

    // Prepare expected output with list of snapshot records in CSL file
    mqbc::IncoreClusterStateLedgerIterator standardCslIt(tester.ledger());
    CslRecordCount                         recordCount;
    CslUpdateChoiceMap                     updateChoiceMap;
    ClusterMessage                         clusterMessage;
    Sequence                               s;
    while (standardCslIt.next() == 0) {
        if (standardCslIt.header().recordType() ==
            mqbc::ClusterStateRecordType::e_SNAPSHOT) {
            recordCount.d_snapshotCount++;
        }
        else if (standardCslIt.header().recordType() ==
                 mqbc::ClusterStateRecordType::e_UPDATE) {
            recordCount.d_updateCount++;
            standardCslIt.loadClusterMessage(&clusterMessage);
            ++updateChoiceMap[clusterMessage.choice().selectionId()];
        }
        else if (standardCslIt.header().recordType() ==
                 mqbc::ClusterStateRecordType::e_COMMIT) {
            recordCount.d_commitCount++;
        }
        else if (standardCslIt.header().recordType() ==
                 mqbc::ClusterStateRecordType::e_ACK) {
            recordCount.d_ackCount++;
        }
    }

    EXPECT_CALL(*printer,
                printSummaryResult(recordCount,
                                   updateChoiceMap,
                                   _,
                                   params.d_processCslRecordTypes,
                                   params.d_cslSummaryQueuesLimit))
        .InSequence(s);

    // Run search
    searchProcessor->process();
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    bmqsys::Time::initialize(bmqtst::TestHelperUtil::allocator());
    bmqp::ProtocolUtil::initialize(bmqtst::TestHelperUtil::allocator());
    bmqp::Crc32c::initialize();

    switch (_testCase) {
    case 0:
    case 1: test1_breathingTest(); break;
    case 2: test2_searchRecordsByTypeTest(); break;
    case 3: test3_searchRecordsByQueueKeyTest(); break;
    case 4: test4_searchRecordsByTimestampRangeTest(); break;
    case 5: test5_searchRecordsByOffsetRangeTest(); break;
    case 6: test6_searchRecordsBySeqNumberRangeTest(); break;
    case 7: test7_searchRecordsByOffsetTest(); break;
    case 8: test8_searchRecordsBySeqNumberTest(); break;
    case 9: test9_summaryTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    bmqp::ProtocolUtil::shutdown();
    bmqsys::Time::shutdown();

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_GBL_ALLOC);
}
