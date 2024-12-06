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

// mqbsl_ledger.t.cpp                                                 -*-C++-*-
#include <mqbsl_ledger.h>

// MQB
#include <mqbmock_logidgenerator.h>
#include <mqbsi_ledger.h>
#include <mqbsi_log.h>
#include <mqbsl_memorymappedondisklog.h>
#include <mqbu_storagekey.h>

// BDE
#include <bdlbb_blob.h>
#include <bdlbb_blobutil.h>
#include <bdlbb_pooledblobbufferfactory.h>
#include <bdlf_bind.h>
#include <bdls_pathutil.h>
#include <bsl_cstring.h>  // for memcmp
#include <bsl_memory.h>
#include <bsl_string.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bsls_annotation.h>
#include <bsls_assert.h>
#include <bsls_types.h>

// SYS
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// TEST DRIVER
#include <bmqsys_time.h>
#include <bmqtst_testhelper.h>
#include <bmqu_tempdirectory.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------

namespace {

// CONSTANTS
const bsls::Types::Int64 k_LOG_MAX_SIZE          = 64 * 1024 * 1024;
const char*              k_DEFAULT_LOG_PREFIX    = "BMQ_TEST_LOG_";
const char*              k_DUMMY_LOG_MESSAGE     = "This is dummy message.";
const int                k_DUMMY_LOG_MESSAGE_LEN = 22;

const char* const k_ENTRIES[]   = {"ax001",
                                   "ax002",
                                   "ax003",
                                   "ax004",
                                   "ax005",
                                   "ax006",
                                   "ax007",
                                   "ax008",
                                   "ax009",
                                   "ax010"};
const int         k_NUM_ENTRIES = 10;
const int         k_ENTRY_LEN   = 5;

const char* const k_EXTRA_ENTRY_1   = "bxx001";
const char* const k_EXTRA_ENTRY_2   = "bxx002";
const char* const k_EXTRA_ENTRY_3   = "bxx003";
const int         k_EXTRA_ENTRY_LEN = 6;

const char* const k_LONG_ENTRY          = "xxxxxxxxxxHELLO_WORLDxxxxxxxxxx";
const char* const k_LONG_ENTRY_MEAT     = "HELLO_WORLD";
const int         k_LONG_ENTRY_OFFSET   = 10;
const int         k_LONG_ENTRY_LEN      = 11;
const int         k_LONG_ENTRY_FULL_LEN = 31;

// ALIASES
typedef mqbsi::LedgerOpResult LedgerOpResult;
typedef mqbsi::LedgerRecordId LedgerRecordId;

// FUNCTIONS
int extractLogIdCallback(mqbu::StorageKey*                      logId,
                         const bsl::string&                     logPath,
                         bsl::shared_ptr<mqbsi::LogIdGenerator> logIdGenerator)
{
    bsl::string logName;
    int         rc = bdls::PathUtil::getLeaf(&logName, logPath);
    BSLS_ASSERT_OPT(rc == 0);

    PVV("Extracting log id from '" << logPath << "'");

    // Open file
    int fd = ::open(logPath.c_str(),
                    O_RDONLY,
                    S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    BSLS_ASSERT_OPT(fd >= 0);

    // Read the file to extract logId
    char logIdStr[mqbu::StorageKey::e_KEY_LENGTH_BINARY];
    rc = ::read(fd, logIdStr, mqbu::StorageKey::e_KEY_LENGTH_BINARY);
    BSLS_ASSERT_OPT(rc == mqbu::StorageKey::e_KEY_LENGTH_BINARY);

    logId->fromBinary(logIdStr);
    logIdGenerator->registerLogId(*logId);

    return 0;
}

int validateLogCallback(
    mqbsi::Log::Offset*          offset,
    BSLS_ANNOTATION_UNUSED const bsl::shared_ptr<mqbsi::Log>& log)
{
    *offset = mqbu::StorageKey::e_KEY_LENGTH_BINARY + k_DUMMY_LOG_MESSAGE_LEN;
    return 0;
}

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
  public:
    // CLASS DATA
    static const int k_OLD_LOG_LEN;

  private:
    // DATA
    bsl::shared_ptr<mqbsi::LogIdGenerator> d_logIdGenerator_sp;
    bsl::shared_ptr<mqbsi::LogFactory>     d_logFactory_sp;
    mqbsi::LedgerConfig                    d_config;
    bslma::ManagedPtr<mqbsi::Ledger>       d_ledger_mp;
    bmqu::TempDirectory                    d_tempDir;
    bdlbb::PooledBlobBufferFactory         d_bufferFactory;
    bdlbb::PooledBlobBufferFactory         d_miniBufferFactory;
    bslma::Allocator*                      d_allocator_p;

  public:
    // CREATORS
    Tester(bool               keepOldLogs = true,
           bsls::Types::Int64 maxLogSize  = k_LOG_MAX_SIZE,
           bslma::Allocator*  allocator = bmqtst::TestHelperUtil::allocator())
    : d_config(allocator)
    , d_ledger_mp(0)
    , d_tempDir(allocator)
    , d_bufferFactory(k_LONG_ENTRY_LEN * 2, allocator)
    , d_miniBufferFactory(k_ENTRY_LEN, allocator)
    , d_allocator_p(allocator)
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
            .setKeepOldLogs(keepOldLogs)
            .setLogFactory(d_logFactory_sp)
            .setExtractLogIdCallback(
                bdlf::BindUtil::bind(&extractLogIdCallback,
                                     bdlf::PlaceHolders::_1,  // logId
                                     bdlf::PlaceHolders::_2,  // logPath
                                     d_logIdGenerator_sp))
            .setValidateLogCallback(&validateLogCallback)
            .setRolloverCallback(&onRolloverCallback)
            .setCleanupCallback(&cleanupCallback);

        // Create and open the ledger
        d_ledger_mp.load(new (*allocator) mqbsl::Ledger(d_config, allocator),
                         allocator);
    }

    // MANIPULATORS
    void generateOldLogs(int numLogs)
    {
        for (int i = 0; i < numLogs; ++i) {
            bsl::string      logName;
            mqbu::StorageKey logId;
            d_logIdGenerator_sp->generateLogId(&logName, &logId);

            // Create and open a new log
            const bsl::string& logFullPath = d_tempDir.path() + "/" + logName;
            PVV("Generating log file: " << logFullPath);
            mqbsi::LogConfig logConfig(k_LOG_MAX_SIZE,
                                       logId,
                                       logFullPath,
                                       false,
                                       false,
                                       d_allocator_p);

            bslma::ManagedPtr<mqbsi::Log> log = d_logFactory_sp->create(
                logConfig);

            int rc = log->open(mqbsi::Log::e_CREATE_IF_MISSING);
            BSLS_ASSERT_OPT(rc == 0);

            // Write the logId to the log
            rc = log->write(log->logConfig().logId().data(),
                            0,
                            mqbu::StorageKey::e_KEY_LENGTH_BINARY);
            BSLS_ASSERT_OPT(rc == 0);

            // Write a dummy message to the log
            rc = log->write(k_DUMMY_LOG_MESSAGE, 0, k_DUMMY_LOG_MESSAGE_LEN);
            BSLS_ASSERT_OPT(rc == mqbu::StorageKey::e_KEY_LENGTH_BINARY);

            rc = log->close();
            BSLS_ASSERT_OPT(rc == 0);
        }
    }

    /// Return a new ledger with the specified `keepOldLogs` flags.
    bslma::ManagedPtr<mqbsi::Ledger> createNewLedger(bool keepOldLogs)
    {
        bool oldFlag = d_config.keepOldLogs();
        d_config.setKeepOldLogs(keepOldLogs);
        bslma::ManagedPtr<mqbsi::Ledger> ledger(
            new (*d_allocator_p) mqbsl::Ledger(d_config, d_allocator_p),
            d_allocator_p);
        d_config.setKeepOldLogs(oldFlag);

        return ledger;
    }

    // ACCESSORS
    mqbsi::Ledger* ledger() const { return d_ledger_mp.get(); }

    bdlbb::BlobBufferFactory* bufferFactory() { return &d_bufferFactory; }

    bdlbb::BlobBufferFactory* miniBufferFactory()
    {
        return &d_miniBufferFactory;
    }
};

const int Tester::k_OLD_LOG_LEN = mqbu::StorageKey::e_KEY_LENGTH_BINARY +
                                  k_DUMMY_LOG_MESSAGE_LEN;

}  // close anonymous namespace

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
//   Basic functionality
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("BREATHING TEST");

    Tester         tester;
    mqbsi::Ledger* ledger = tester.ledger();
    BMQTST_ASSERT_EQ(ledger->isOpened(), false);

    BMQTST_ASSERT_EQ(ledger->open(mqbsi::Ledger::e_CREATE_IF_MISSING),
                     LedgerOpResult::e_SUCCESS);
    BMQTST_ASSERT_EQ(ledger->isOpened(), true);
    BMQTST_ASSERT_EQ(ledger->supportsAliasing(), true);
    BMQTST_ASSERT_EQ(ledger->numLogs(), 1U);
    BMQTST_ASSERT_EQ(ledger->logs().size(), 1U);

    const mqbu::StorageKey& logId = ledger->currentLog()->logConfig().logId();
    BMQTST_ASSERT_EQ(ledger->outstandingNumBytes(), 0);
    BMQTST_ASSERT_EQ(ledger->outstandingNumBytes(logId), 0);
    BMQTST_ASSERT_EQ(ledger->totalNumBytes(), 0);
    BMQTST_ASSERT_EQ(ledger->totalNumBytes(logId), 0);

    BMQTST_ASSERT_EQ(ledger->flush(), LedgerOpResult::e_SUCCESS);
    BMQTST_ASSERT_EQ(ledger->close(), LedgerOpResult::e_SUCCESS);
    BMQTST_ASSERT_EQ(ledger->isOpened(), false);
}

static void test2_openMultipleLogs()
// ------------------------------------------------------------------------
// OPEN MULTIPLE LOGS
//
// Concerns:
//   Ensure that when multiple logs exist at the location, the ledger
//   recognizes them upon opening.  Here, the 'keepOldLogs' flag is set to
//   true, so all logs should be kept.
//
// Testing:
//   open()
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("OPEN MULTIPLE LOGS");

    Tester tester;
    tester.generateOldLogs(3);

    mqbsi::Ledger* ledger = tester.ledger();
    BMQTST_ASSERT_EQ(ledger->open(mqbsi::Ledger::e_READ_ONLY),
                     LedgerOpResult::e_SUCCESS);
    BMQTST_ASSERT_EQ(ledger->supportsAliasing(), true);
    BMQTST_ASSERT_EQ(ledger->numLogs(), 3U);
    BMQTST_ASSERT_EQ(ledger->logs().size(), 3U);
    BMQTST_ASSERT_EQ(ledger->outstandingNumBytes(), Tester::k_OLD_LOG_LEN * 3);
    BMQTST_ASSERT_EQ(ledger->totalNumBytes(), Tester::k_OLD_LOG_LEN * 3);

    for (mqbsl::Ledger::Logs::const_iterator cit = ledger->logs().cbegin();
         cit != ledger->logs().cend();
         ++cit) {
        const mqbu::StorageKey& logId = (*cit)->logConfig().logId();
        BMQTST_ASSERT_EQ(ledger->outstandingNumBytes(logId),
                         Tester::k_OLD_LOG_LEN);
        BMQTST_ASSERT_EQ(ledger->totalNumBytes(logId), Tester::k_OLD_LOG_LEN);
    }

    BSLS_ASSERT_OPT(ledger->close() == LedgerOpResult::e_SUCCESS);
}

static void test3_openMultipleLogsNoKeep()
// ------------------------------------------------------------------------
// OPEN MULTIPLE LOGS NO KEEP
//
// Concerns:
//   Ensure that when multiple logs exist at the location, the ledger
//   recognizes them upon opening.  Here, the 'keepOldLogs' flag is set to
//   false, so only the latest log should be kept, and any operation on an
//   obsolete log (via an obsolete 'LedgerRecordId') must fail.
//
// Testing:
//   open()
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("OPEN MULTIPLE LOGS NO KEEP");

    Tester tester(false);
    tester.generateOldLogs(3);

    mqbsi::Ledger* ledger = tester.ledger();
    BMQTST_ASSERT_EQ(ledger->open(mqbsi::Ledger::e_READ_ONLY),
                     LedgerOpResult::e_SUCCESS);
    BMQTST_ASSERT_EQ(ledger->supportsAliasing(), true);
    BMQTST_ASSERT_EQ(ledger->numLogs(), 1U);
    BMQTST_ASSERT_EQ(ledger->logs().size(), 1U);

    const mqbu::StorageKey& logId = ledger->currentLog()->logConfig().logId();
    BMQTST_ASSERT_EQ(ledger->outstandingNumBytes(), Tester::k_OLD_LOG_LEN);
    BMQTST_ASSERT_EQ(ledger->outstandingNumBytes(logId),
                     Tester::k_OLD_LOG_LEN);
    BMQTST_ASSERT_EQ(ledger->totalNumBytes(), Tester::k_OLD_LOG_LEN);
    BMQTST_ASSERT_EQ(ledger->totalNumBytes(logId), Tester::k_OLD_LOG_LEN);

    BSLS_ASSERT_OPT(ledger->close() == LedgerOpResult::e_SUCCESS);

    // Extract an obsolete logId.
    bslma::ManagedPtr<mqbsi::Ledger> keepLogsLedger = tester.createNewLedger(
        true);
    BSLS_ASSERT_OPT(keepLogsLedger->open(mqbsi::Ledger::e_READ_ONLY) ==
                    LedgerOpResult::e_SUCCESS);

    const mqbu::StorageKey& obsoleteLogId =
        keepLogsLedger->logs()[0]->logConfig().logId();
    BSLS_ASSERT_OPT(obsoleteLogId != logId);
    BSLS_ASSERT_OPT(keepLogsLedger->close() == LedgerOpResult::e_SUCCESS);

    // Ensure that any opetation on an obsolete log must fail.
    BSLS_ASSERT_OPT(ledger->open(mqbsi::Ledger::e_READ_ONLY) ==
                    LedgerOpResult::e_SUCCESS);

    LedgerRecordId obsoleteRecordId(obsoleteLogId, 0);
    char           entry[Tester::k_OLD_LOG_LEN];
    bdlbb::Blob    blobEntry(tester.bufferFactory(),
                          bmqtst::TestHelperUtil::allocator());
    BMQTST_ASSERT_EQ(ledger->updateOutstandingNumBytes(obsoleteLogId, 100),
                     LedgerOpResult::e_LOG_NOT_FOUND);
    BMQTST_ASSERT_EQ(ledger->setOutstandingNumBytes(obsoleteLogId, 100),
                     LedgerOpResult::e_LOG_NOT_FOUND);
    BMQTST_ASSERT_EQ(
        ledger->readRecord(entry, Tester::k_OLD_LOG_LEN, obsoleteRecordId),
        LedgerOpResult::e_LOG_NOT_FOUND);
    BMQTST_ASSERT_EQ(ledger->readRecord(&blobEntry,
                                        Tester::k_OLD_LOG_LEN,
                                        obsoleteRecordId),
                     LedgerOpResult::e_LOG_NOT_FOUND);
    BMQTST_ASSERT_EQ(ledger->aliasRecord(reinterpret_cast<void**>(&entry),
                                         Tester::k_OLD_LOG_LEN,
                                         obsoleteRecordId),
                     LedgerOpResult::e_LOG_NOT_FOUND);
    BMQTST_ASSERT_EQ(ledger->aliasRecord(&blobEntry,
                                         Tester::k_OLD_LOG_LEN,
                                         obsoleteRecordId),
                     LedgerOpResult::e_LOG_NOT_FOUND);
    BMQTST_ASSERT_OPT_FAIL(ledger->outstandingNumBytes(obsoleteLogId));
    BMQTST_ASSERT_OPT_FAIL(ledger->totalNumBytes(obsoleteLogId));

    BSLS_ASSERT_OPT(ledger->close() == LedgerOpResult::e_SUCCESS);
}

static void test4_updateOutstandingNumBytes()
// ------------------------------------------------------------------------
// UPDATE OUTSTANDING NUM BYTES
//
// Concerns:
//   Verify that 'updateOutstandingNumBytes' works as intended.
//
// Testing:
//   updateOutstandingNumBytes(...)
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("UPDATE OUTSTANDING NUM BYTES");

    Tester tester;
    tester.generateOldLogs(2);

    mqbsi::Ledger* ledger = tester.ledger();
    BSLS_ASSERT_OPT(ledger->open(mqbsi::Ledger::e_READ_ONLY) ==
                    LedgerOpResult::e_SUCCESS);

    const mqbu::StorageKey& logId1 = ledger->logs()[0]->logConfig().logId();
    const mqbu::StorageKey& logId2 = ledger->logs()[1]->logConfig().logId();
    BSLS_ASSERT_OPT(ledger->outstandingNumBytes() ==
                    Tester::k_OLD_LOG_LEN * 2);
    BSLS_ASSERT_OPT(ledger->outstandingNumBytes(logId1) ==
                    Tester::k_OLD_LOG_LEN);
    BSLS_ASSERT_OPT(ledger->outstandingNumBytes(logId2) ==
                    Tester::k_OLD_LOG_LEN);

    ledger->updateOutstandingNumBytes(logId1, 200);
    BMQTST_ASSERT_EQ(ledger->outstandingNumBytes(),
                     200 + Tester::k_OLD_LOG_LEN * 2);
    BMQTST_ASSERT_EQ(ledger->outstandingNumBytes(logId1),
                     200 + Tester::k_OLD_LOG_LEN);
    BMQTST_ASSERT_EQ(ledger->outstandingNumBytes(logId2),
                     Tester::k_OLD_LOG_LEN);

    ledger->updateOutstandingNumBytes(logId2, 1000);
    BMQTST_ASSERT_EQ(ledger->outstandingNumBytes(),
                     1200 + Tester::k_OLD_LOG_LEN * 2);
    BMQTST_ASSERT_EQ(ledger->outstandingNumBytes(logId1),
                     200 + Tester::k_OLD_LOG_LEN);
    BMQTST_ASSERT_EQ(ledger->outstandingNumBytes(logId2),
                     1000 + Tester::k_OLD_LOG_LEN);

    ledger->updateOutstandingNumBytes(logId2, -700);
    BMQTST_ASSERT_EQ(ledger->outstandingNumBytes(),
                     500 + Tester::k_OLD_LOG_LEN * 2);
    BMQTST_ASSERT_EQ(ledger->outstandingNumBytes(logId1),
                     200 + Tester::k_OLD_LOG_LEN);
    BMQTST_ASSERT_EQ(ledger->outstandingNumBytes(logId2),
                     300 + Tester::k_OLD_LOG_LEN);

    // Close and re-open the ledger. 'outstandingNumBytes' must be
    // re-calibrated.
    BSLS_ASSERT_OPT(ledger->close() == LedgerOpResult::e_SUCCESS);
    BSLS_ASSERT_OPT(ledger->open(mqbsi::Ledger::e_READ_ONLY) ==
                    LedgerOpResult::e_SUCCESS);
    BMQTST_ASSERT_EQ(ledger->outstandingNumBytes(), Tester::k_OLD_LOG_LEN * 2);
    BMQTST_ASSERT_EQ(ledger->outstandingNumBytes(logId1),
                     Tester::k_OLD_LOG_LEN);
    BMQTST_ASSERT_EQ(ledger->outstandingNumBytes(logId2),
                     Tester::k_OLD_LOG_LEN);

    BSLS_ASSERT_OPT(ledger->close() == LedgerOpResult::e_SUCCESS);
}

static void test5_setOutstandingNumBytes()
// ------------------------------------------------------------------------
// SET OUTSTANDING NUM BYTES
//
// Concerns:
//   Verify that 'setOutstandingNumBytes' works as intended.
//
// Testing:
//   setOutstandingNumBytes(...)
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("SET OUTSTANDING NUM BYTES");

    Tester tester;
    tester.generateOldLogs(2);

    mqbsi::Ledger* ledger = tester.ledger();
    BSLS_ASSERT_OPT(ledger->open(mqbsi::Ledger::e_READ_ONLY) ==
                    LedgerOpResult::e_SUCCESS);

    const mqbu::StorageKey& logId1 = ledger->logs()[0]->logConfig().logId();
    const mqbu::StorageKey& logId2 = ledger->logs()[1]->logConfig().logId();
    BSLS_ASSERT_OPT(ledger->outstandingNumBytes() ==
                    Tester::k_OLD_LOG_LEN * 2);
    BSLS_ASSERT_OPT(ledger->outstandingNumBytes(logId1) ==
                    Tester::k_OLD_LOG_LEN);
    BSLS_ASSERT_OPT(ledger->outstandingNumBytes(logId2) ==
                    Tester::k_OLD_LOG_LEN);

    ledger->setOutstandingNumBytes(logId1, 500);
    BMQTST_ASSERT_EQ(ledger->outstandingNumBytes(),
                     500 + Tester::k_OLD_LOG_LEN);
    BMQTST_ASSERT_EQ(ledger->outstandingNumBytes(logId1), 500);
    BMQTST_ASSERT_EQ(ledger->outstandingNumBytes(logId2),
                     Tester::k_OLD_LOG_LEN);

    ledger->setOutstandingNumBytes(logId2, 2000);
    BMQTST_ASSERT_EQ(ledger->outstandingNumBytes(), 2500);
    BMQTST_ASSERT_EQ(ledger->outstandingNumBytes(logId1), 500);
    BMQTST_ASSERT_EQ(ledger->outstandingNumBytes(logId2), 2000);

    ledger->setOutstandingNumBytes(logId2, 666);
    BMQTST_ASSERT_EQ(ledger->outstandingNumBytes(), 1166);
    BMQTST_ASSERT_EQ(ledger->outstandingNumBytes(logId1), 500);
    BMQTST_ASSERT_EQ(ledger->outstandingNumBytes(logId2), 666);

    // Close and re-open the ledger. 'outstandingNumBytes' must be
    // re-calibrated.
    BSLS_ASSERT_OPT(ledger->close() == LedgerOpResult::e_SUCCESS);
    BSLS_ASSERT_OPT(ledger->open(mqbsi::Ledger::e_READ_ONLY) ==
                    LedgerOpResult::e_SUCCESS);
    BMQTST_ASSERT_EQ(ledger->outstandingNumBytes(), Tester::k_OLD_LOG_LEN * 2);
    BMQTST_ASSERT_EQ(ledger->outstandingNumBytes(logId1),
                     Tester::k_OLD_LOG_LEN);
    BMQTST_ASSERT_EQ(ledger->outstandingNumBytes(logId2),
                     Tester::k_OLD_LOG_LEN);

    BSLS_ASSERT_OPT(ledger->close() == LedgerOpResult::e_SUCCESS);
}

/// Impl. of the "WRITE RECORD RAW" test, using the specified `ledger`.
static void writeRecordRawImpl(mqbsi::Ledger* ledger)
{
    BSLS_ASSERT_OPT(ledger->open(mqbsi::Ledger::e_CREATE_IF_MISSING) ==
                    LedgerOpResult::e_SUCCESS);

    // Old log w.r.t. rollover
    const mqbu::StorageKey& oldLogId =
        ledger->currentLog()->logConfig().logId();

    bsls::Types::Int64 oldLogNumBytes = Tester::k_OLD_LOG_LEN;
    BSLS_ASSERT_OPT(ledger->totalNumBytes(oldLogId) == oldLogNumBytes);
    BSLS_ASSERT_OPT(ledger->outstandingNumBytes(oldLogId) == oldLogNumBytes);
    BSLS_ASSERT_OPT(ledger->totalNumBytes() ==
                    oldLogNumBytes + Tester::k_OLD_LOG_LEN);
    BSLS_ASSERT_OPT(ledger->outstandingNumBytes() ==
                    oldLogNumBytes + Tester::k_OLD_LOG_LEN);

    // 1. Write a list of records.
    LedgerRecordId recordId;
    for (int i = 0; i < k_NUM_ENTRIES; ++i) {
        BMQTST_ASSERT_EQ(
            ledger->writeRecord(&recordId, k_ENTRIES[i], 0, k_ENTRY_LEN),
            LedgerOpResult::e_SUCCESS);
        BMQTST_ASSERT_EQ(recordId.logId(), oldLogId);
        BMQTST_ASSERT_EQ(recordId.offset(), oldLogNumBytes);

        oldLogNumBytes += k_ENTRY_LEN;
        BMQTST_ASSERT_EQ(ledger->totalNumBytes(oldLogId), oldLogNumBytes);
        BMQTST_ASSERT_EQ(ledger->outstandingNumBytes(oldLogId),
                         oldLogNumBytes);
        BMQTST_ASSERT_EQ(ledger->totalNumBytes(),
                         oldLogNumBytes + Tester::k_OLD_LOG_LEN);
        BMQTST_ASSERT_EQ(ledger->outstandingNumBytes(),
                         oldLogNumBytes + Tester::k_OLD_LOG_LEN);
    }

    // 2. Set `outstandingNumBytes` to zero, indicating that all entries are no
    //    longer outstanding.
    ledger->setOutstandingNumBytes(oldLogId, 0);
    BSLS_ASSERT_OPT(ledger->outstandingNumBytes(oldLogId) == 0);
    BSLS_ASSERT_OPT(ledger->outstandingNumBytes() == Tester::k_OLD_LOG_LEN);

    // 3. Write a long record.
    BMQTST_ASSERT_EQ(ledger->writeRecord(&recordId,
                                         k_LONG_ENTRY,
                                         k_LONG_ENTRY_OFFSET,
                                         k_LONG_ENTRY_LEN),
                     LedgerOpResult::e_SUCCESS);
    BMQTST_ASSERT_EQ(recordId.logId(), oldLogId);
    BMQTST_ASSERT_EQ(recordId.offset(), oldLogNumBytes);

    oldLogNumBytes += k_LONG_ENTRY_LEN;
    BMQTST_ASSERT_EQ(ledger->totalNumBytes(oldLogId), oldLogNumBytes);
    BMQTST_ASSERT_EQ(ledger->outstandingNumBytes(oldLogId), k_LONG_ENTRY_LEN);
    BMQTST_ASSERT_EQ(ledger->totalNumBytes(),
                     oldLogNumBytes + Tester::k_OLD_LOG_LEN);
    BMQTST_ASSERT_EQ(ledger->outstandingNumBytes(),
                     k_LONG_ENTRY_LEN + Tester::k_OLD_LOG_LEN);

    // 4. Write a record that would trigger rollover and would be written to a
    //    new log.
    BMQTST_ASSERT_EQ(
        ledger->writeRecord(&recordId, k_EXTRA_ENTRY_1, 0, k_EXTRA_ENTRY_LEN),
        LedgerOpResult::e_SUCCESS);

    // Rollover should be triggered here.
    BMQTST_ASSERT_EQ(ledger->numLogs(), 3U);
    bsls::Types::Int64 newLogNumBytes = k_EXTRA_ENTRY_LEN;

    const mqbu::StorageKey& newLogId =
        ledger->currentLog()->logConfig().logId();
    PVV("Old log ID: " << oldLogId);
    PVV("New log ID: " << newLogId);
    BMQTST_ASSERT_NE(newLogId, oldLogId);
    BMQTST_ASSERT_EQ(recordId.logId(), newLogId);
    BMQTST_ASSERT_EQ(recordId.offset(), 0);

    BMQTST_ASSERT_EQ(ledger->totalNumBytes(newLogId), newLogNumBytes);
    BMQTST_ASSERT_EQ(ledger->outstandingNumBytes(newLogId), newLogNumBytes);
    BMQTST_ASSERT_EQ(ledger->totalNumBytes(oldLogId), oldLogNumBytes);
    BMQTST_ASSERT_EQ(ledger->outstandingNumBytes(oldLogId), k_LONG_ENTRY_LEN);
    BMQTST_ASSERT_EQ(ledger->totalNumBytes(),
                     newLogNumBytes + oldLogNumBytes + Tester::k_OLD_LOG_LEN);
    BMQTST_ASSERT_EQ(ledger->outstandingNumBytes(),
                     newLogNumBytes + k_LONG_ENTRY_LEN +
                         Tester::k_OLD_LOG_LEN);

    // 5. Write one more record.
    BMQTST_ASSERT_EQ(
        ledger->writeRecord(&recordId, k_EXTRA_ENTRY_2, 0, k_EXTRA_ENTRY_LEN),
        LedgerOpResult::e_SUCCESS);
    BMQTST_ASSERT_EQ(recordId.logId(), newLogId);
    BMQTST_ASSERT_EQ(recordId.offset(), k_EXTRA_ENTRY_LEN);

    newLogNumBytes += k_EXTRA_ENTRY_LEN;
    BMQTST_ASSERT_EQ(ledger->totalNumBytes(newLogId), newLogNumBytes);
    BMQTST_ASSERT_EQ(ledger->outstandingNumBytes(newLogId), newLogNumBytes);
    BMQTST_ASSERT_EQ(ledger->totalNumBytes(oldLogId), oldLogNumBytes);
    BMQTST_ASSERT_EQ(ledger->outstandingNumBytes(oldLogId), k_LONG_ENTRY_LEN);
    BMQTST_ASSERT_EQ(ledger->totalNumBytes(),
                     newLogNumBytes + oldLogNumBytes + Tester::k_OLD_LOG_LEN);
    BMQTST_ASSERT_EQ(ledger->outstandingNumBytes(),
                     newLogNumBytes + k_LONG_ENTRY_LEN +
                         Tester::k_OLD_LOG_LEN);

    // 6. Close and re-open the ledger.  'totalNumBytes' and
    //    'outstandingNumBytes' must be re-calibrated.
    ledger->setOutstandingNumBytes(oldLogId, 0);
    ledger->setOutstandingNumBytes(newLogId, 0);
    BSLS_ASSERT_OPT(ledger->flush() == LedgerOpResult::e_SUCCESS);
    BSLS_ASSERT_OPT(ledger->close() == LedgerOpResult::e_SUCCESS);

    BSLS_ASSERT_OPT(ledger->open(mqbsi::Ledger::e_READ_ONLY) ==
                    LedgerOpResult::e_SUCCESS);
    BMQTST_ASSERT_EQ(ledger->totalNumBytes(newLogId), newLogNumBytes);
    BMQTST_ASSERT_EQ(ledger->outstandingNumBytes(newLogId), newLogNumBytes);
    BMQTST_ASSERT_EQ(ledger->totalNumBytes(oldLogId), oldLogNumBytes);
    BMQTST_ASSERT_EQ(ledger->outstandingNumBytes(oldLogId), oldLogNumBytes);
    BMQTST_ASSERT_EQ(ledger->totalNumBytes(),
                     newLogNumBytes + oldLogNumBytes + Tester::k_OLD_LOG_LEN);
    BMQTST_ASSERT_EQ(ledger->outstandingNumBytes(),
                     newLogNumBytes + oldLogNumBytes + Tester::k_OLD_LOG_LEN);

    BSLS_ASSERT_OPT(ledger->close() == LedgerOpResult::e_SUCCESS);
}

static void test6_writeRecordRaw()
// ------------------------------------------------------------------------
// WRITE RECORD RAW
//
// Concerns:
//   Verify that 'writeRecord' works as intended when dealing with void*,
//   even when rollover is involved.  Also, give an example of updating
//   `outstandingNumBytes` when using 'write'.
//
// Testing:
//   writeRecord(LedgerRecordId *recordId,
//               const void     *record,
//               int             offset,
//               int             length)
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("WRITE RECORD RAW");

    Tester tester(true,
                  Tester::k_OLD_LOG_LEN + k_ENTRY_LEN * k_NUM_ENTRIES +
                      k_LONG_ENTRY_LEN);
    tester.generateOldLogs(2);

    mqbsi::Ledger* ledger = tester.ledger();
    writeRecordRawImpl(ledger);
}

static void test7_writeRecordBlob()
// ------------------------------------------------------------------------
// WRITE RECORD BLOB
//
// Concerns:
//   Verify that 'writeRecord' works as intended when dealing with blobs,
//   even when rollover is involved.  Also, give an example of updating
//   `outstandingNumBytes` when using 'write'.
//
// Testing:
//   writeRecord(LedgerRecordId            *recordId,
//               const bdlbb::Blob&          record,
//               const bmqu::BlobPosition&  offset,
//               int                        length)
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("WRITE RECORD BLOB");

    Tester tester(true,
                  Tester::k_OLD_LOG_LEN + k_ENTRY_LEN * k_NUM_ENTRIES +
                      k_LONG_ENTRY_LEN);
    tester.generateOldLogs(2);

    mqbsi::Ledger* ledger = tester.ledger();
    BSLS_ASSERT_OPT(ledger->open(mqbsi::Ledger::e_CREATE_IF_MISSING) ==
                    LedgerOpResult::e_SUCCESS);

    // Old log w.r.t. rollover
    const mqbu::StorageKey& oldLogId =
        ledger->currentLog()->logConfig().logId();

    bsls::Types::Int64 oldLogNumBytes = Tester::k_OLD_LOG_LEN;
    BSLS_ASSERT_OPT(ledger->totalNumBytes(oldLogId) == oldLogNumBytes);
    BSLS_ASSERT_OPT(ledger->outstandingNumBytes(oldLogId) == oldLogNumBytes);
    BSLS_ASSERT_OPT(ledger->totalNumBytes() ==
                    oldLogNumBytes + Tester::k_OLD_LOG_LEN);
    BSLS_ASSERT_OPT(ledger->outstandingNumBytes() ==
                    oldLogNumBytes + Tester::k_OLD_LOG_LEN);

    // 1. Write a list of records.
    bdlbb::Blob    blob(tester.miniBufferFactory(),
                     bmqtst::TestHelperUtil::allocator());
    LedgerRecordId recordId;
    for (int i = 0; i < k_NUM_ENTRIES; ++i) {
        bdlbb::BlobUtil::append(&blob, k_ENTRIES[i], k_ENTRY_LEN);

        bmqu::BlobPosition pos(i, 0);
        BMQTST_ASSERT_EQ(
            ledger->writeRecord(&recordId, blob, pos, k_ENTRY_LEN),
            LedgerOpResult::e_SUCCESS);
        BMQTST_ASSERT_EQ(recordId.logId(), oldLogId);
        BMQTST_ASSERT_EQ(recordId.offset(), oldLogNumBytes);

        oldLogNumBytes += k_ENTRY_LEN;
        BMQTST_ASSERT_EQ(ledger->totalNumBytes(oldLogId), oldLogNumBytes);
        BMQTST_ASSERT_EQ(ledger->outstandingNumBytes(oldLogId),
                         oldLogNumBytes);
        BMQTST_ASSERT_EQ(ledger->totalNumBytes(),
                         oldLogNumBytes + Tester::k_OLD_LOG_LEN);
        BMQTST_ASSERT_EQ(ledger->outstandingNumBytes(),
                         oldLogNumBytes + Tester::k_OLD_LOG_LEN);
    }

    // 2. Set `outstandingNumBytes` to zero, indicating that all entries are no
    //    longer outstanding.
    ledger->setOutstandingNumBytes(oldLogId, 0);
    BSLS_ASSERT_OPT(ledger->outstandingNumBytes(oldLogId) == 0);
    BSLS_ASSERT_OPT(ledger->outstandingNumBytes() == Tester::k_OLD_LOG_LEN);

    // 3. Write a long record.
    bdlbb::Blob blob2(tester.bufferFactory(),
                      bmqtst::TestHelperUtil::allocator());
    bdlbb::BlobUtil::append(&blob2, k_LONG_ENTRY, k_LONG_ENTRY_FULL_LEN);
    BMQTST_ASSERT_EQ(
        ledger->writeRecord(&recordId,
                            blob2,
                            bmqu::BlobPosition(0, k_LONG_ENTRY_OFFSET),
                            k_LONG_ENTRY_LEN),
        LedgerOpResult::e_SUCCESS);
    BMQTST_ASSERT_EQ(recordId.logId(), oldLogId);
    BMQTST_ASSERT_EQ(recordId.offset(), oldLogNumBytes);

    oldLogNumBytes += k_LONG_ENTRY_LEN;
    BMQTST_ASSERT_EQ(ledger->totalNumBytes(oldLogId), oldLogNumBytes);
    BMQTST_ASSERT_EQ(ledger->outstandingNumBytes(oldLogId), k_LONG_ENTRY_LEN);
    BMQTST_ASSERT_EQ(ledger->totalNumBytes(),
                     oldLogNumBytes + Tester::k_OLD_LOG_LEN);
    BMQTST_ASSERT_EQ(ledger->outstandingNumBytes(),
                     k_LONG_ENTRY_LEN + Tester::k_OLD_LOG_LEN);

    // 4. Write a record that would trigger rollover and would be written to a
    //    new log.
    bdlbb::Blob blob3(tester.bufferFactory(),
                      bmqtst::TestHelperUtil::allocator());
    bdlbb::BlobUtil::append(&blob3, k_EXTRA_ENTRY_1, k_EXTRA_ENTRY_LEN);
    BMQTST_ASSERT_EQ(ledger->writeRecord(&recordId,
                                         blob3,
                                         bmqu::BlobPosition(),
                                         k_EXTRA_ENTRY_LEN),
                     LedgerOpResult::e_SUCCESS);

    // Rollover should be triggered here.
    BMQTST_ASSERT_EQ(ledger->numLogs(), 3U);
    bsls::Types::Int64 newLogNumBytes = k_EXTRA_ENTRY_LEN;

    const mqbu::StorageKey& newLogId =
        ledger->currentLog()->logConfig().logId();
    PVV("Old log ID: " << oldLogId);
    PVV("New log ID: " << newLogId);
    BMQTST_ASSERT_NE(newLogId, oldLogId);
    BMQTST_ASSERT_EQ(recordId.logId(), newLogId);
    BMQTST_ASSERT_EQ(recordId.offset(), 0);

    BMQTST_ASSERT_EQ(ledger->totalNumBytes(newLogId), newLogNumBytes);
    BMQTST_ASSERT_EQ(ledger->outstandingNumBytes(newLogId), newLogNumBytes);
    BMQTST_ASSERT_EQ(ledger->totalNumBytes(oldLogId), oldLogNumBytes);
    BMQTST_ASSERT_EQ(ledger->outstandingNumBytes(oldLogId), k_LONG_ENTRY_LEN);
    BMQTST_ASSERT_EQ(ledger->totalNumBytes(),
                     newLogNumBytes + oldLogNumBytes + Tester::k_OLD_LOG_LEN);
    BMQTST_ASSERT_EQ(ledger->outstandingNumBytes(),
                     newLogNumBytes + k_LONG_ENTRY_LEN +
                         Tester::k_OLD_LOG_LEN);

    // 5. Write one more record.
    bdlbb::Blob blob4(tester.bufferFactory(),
                      bmqtst::TestHelperUtil::allocator());
    bdlbb::BlobUtil::append(&blob4, k_EXTRA_ENTRY_2, k_EXTRA_ENTRY_LEN);
    BMQTST_ASSERT_EQ(ledger->writeRecord(&recordId,
                                         blob4,
                                         bmqu::BlobPosition(),
                                         k_EXTRA_ENTRY_LEN),
                     LedgerOpResult::e_SUCCESS);
    BMQTST_ASSERT_EQ(recordId.logId(), newLogId);
    BMQTST_ASSERT_EQ(recordId.offset(), k_EXTRA_ENTRY_LEN);

    newLogNumBytes += k_EXTRA_ENTRY_LEN;
    BMQTST_ASSERT_EQ(ledger->totalNumBytes(newLogId), newLogNumBytes);
    BMQTST_ASSERT_EQ(ledger->outstandingNumBytes(newLogId), newLogNumBytes);
    BMQTST_ASSERT_EQ(ledger->totalNumBytes(oldLogId), oldLogNumBytes);
    BMQTST_ASSERT_EQ(ledger->outstandingNumBytes(oldLogId), k_LONG_ENTRY_LEN);
    BMQTST_ASSERT_EQ(ledger->totalNumBytes(),
                     newLogNumBytes + oldLogNumBytes + Tester::k_OLD_LOG_LEN);
    BMQTST_ASSERT_EQ(ledger->outstandingNumBytes(),
                     newLogNumBytes + k_LONG_ENTRY_LEN +
                         Tester::k_OLD_LOG_LEN);

    // 6. Close and re-open the ledger.  'totalNumBytes' and
    //    'outstandingNumBytes' must be re-calibrated.
    ledger->setOutstandingNumBytes(oldLogId, 0);
    ledger->setOutstandingNumBytes(newLogId, 0);
    BSLS_ASSERT_OPT(ledger->flush() == LedgerOpResult::e_SUCCESS);
    BSLS_ASSERT_OPT(ledger->close() == LedgerOpResult::e_SUCCESS);

    BSLS_ASSERT_OPT(ledger->open(mqbsi::Ledger::e_READ_ONLY) ==
                    LedgerOpResult::e_SUCCESS);
    BMQTST_ASSERT_EQ(ledger->totalNumBytes(newLogId), newLogNumBytes);
    BMQTST_ASSERT_EQ(ledger->outstandingNumBytes(newLogId), newLogNumBytes);
    BMQTST_ASSERT_EQ(ledger->totalNumBytes(oldLogId), oldLogNumBytes);
    BMQTST_ASSERT_EQ(ledger->outstandingNumBytes(oldLogId), oldLogNumBytes);
    BMQTST_ASSERT_EQ(ledger->totalNumBytes(),
                     newLogNumBytes + oldLogNumBytes + Tester::k_OLD_LOG_LEN);
    BMQTST_ASSERT_EQ(ledger->outstandingNumBytes(),
                     newLogNumBytes + oldLogNumBytes + Tester::k_OLD_LOG_LEN);

    BSLS_ASSERT_OPT(ledger->close() == LedgerOpResult::e_SUCCESS);
}

static void test8_writeRecordBlobSection()
// ------------------------------------------------------------------------
// WRITE RECORD BLOB SECTION
//
// Concerns:
//   Verify that 'writeRecord' works as intended when dealing with blob
//   sections, even when rollover is involved.  Also, give an example of
//   updating `outstandingNumBytes` when using 'write'.
//
// Testing:
//   writeRecord(LedgerRecordId           *recordId,
//               const bdlbb::Blob&         record,
//               const bmqu::BlobSection&  section)
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("WRITE RECORD BLOB SECTION");

    Tester tester(true,
                  Tester::k_OLD_LOG_LEN + k_ENTRY_LEN * k_NUM_ENTRIES +
                      k_LONG_ENTRY_LEN);
    tester.generateOldLogs(2);

    mqbsi::Ledger* ledger = tester.ledger();
    BSLS_ASSERT_OPT(ledger->open(mqbsi::Ledger::e_CREATE_IF_MISSING) ==
                    LedgerOpResult::e_SUCCESS);

    // Old log w.r.t. rollover
    const mqbu::StorageKey& oldLogId =
        ledger->currentLog()->logConfig().logId();

    bsls::Types::Int64 oldLogNumBytes = Tester::k_OLD_LOG_LEN;
    BSLS_ASSERT_OPT(ledger->totalNumBytes(oldLogId) == oldLogNumBytes);
    BSLS_ASSERT_OPT(ledger->outstandingNumBytes(oldLogId) == oldLogNumBytes);
    BSLS_ASSERT_OPT(ledger->totalNumBytes() ==
                    oldLogNumBytes + Tester::k_OLD_LOG_LEN);
    BSLS_ASSERT_OPT(ledger->outstandingNumBytes() ==
                    oldLogNumBytes + Tester::k_OLD_LOG_LEN);

    // 1. Write a list of records.
    bdlbb::Blob    blob(tester.miniBufferFactory(),
                     bmqtst::TestHelperUtil::allocator());
    LedgerRecordId recordId;
    for (int i = 0; i < k_NUM_ENTRIES; ++i) {
        bdlbb::BlobUtil::append(&blob, k_ENTRIES[i], k_ENTRY_LEN);

        bmqu::BlobPosition start(i, 0);
        bmqu::BlobPosition end(i + 1, 0);
        bmqu::BlobSection  section(start, end);
        BMQTST_ASSERT_EQ(ledger->writeRecord(&recordId, blob, section),
                         LedgerOpResult::e_SUCCESS);
        BMQTST_ASSERT_EQ(recordId.logId(), oldLogId);
        BMQTST_ASSERT_EQ(recordId.offset(), oldLogNumBytes);

        oldLogNumBytes += k_ENTRY_LEN;
        BMQTST_ASSERT_EQ(ledger->totalNumBytes(oldLogId), oldLogNumBytes);
        BMQTST_ASSERT_EQ(ledger->outstandingNumBytes(oldLogId),
                         oldLogNumBytes);
        BMQTST_ASSERT_EQ(ledger->totalNumBytes(),
                         oldLogNumBytes + Tester::k_OLD_LOG_LEN);
        BMQTST_ASSERT_EQ(ledger->outstandingNumBytes(),
                         oldLogNumBytes + Tester::k_OLD_LOG_LEN);
    }

    // 2. Set `outstandingNumBytes` to zero, indicating that all entries are no
    //    longer outstanding.
    ledger->setOutstandingNumBytes(oldLogId, 0);
    BSLS_ASSERT_OPT(ledger->outstandingNumBytes(oldLogId) == 0);
    BSLS_ASSERT_OPT(ledger->outstandingNumBytes() == Tester::k_OLD_LOG_LEN);

    // 3. Write a long record.
    bdlbb::Blob blob2(tester.bufferFactory(),
                      bmqtst::TestHelperUtil::allocator());
    bdlbb::BlobUtil::append(&blob2, k_LONG_ENTRY, k_LONG_ENTRY_FULL_LEN);

    bmqu::BlobPosition start2(0, k_LONG_ENTRY_OFFSET);
    bmqu::BlobPosition end2(0, k_LONG_ENTRY_OFFSET + k_LONG_ENTRY_LEN);
    bmqu::BlobSection  section2(start2, end2);
    BMQTST_ASSERT_EQ(ledger->writeRecord(&recordId, blob2, section2),
                     LedgerOpResult::e_SUCCESS);
    BMQTST_ASSERT_EQ(recordId.logId(), oldLogId);
    BMQTST_ASSERT_EQ(recordId.offset(), oldLogNumBytes);

    oldLogNumBytes += k_LONG_ENTRY_LEN;
    BMQTST_ASSERT_EQ(ledger->totalNumBytes(oldLogId), oldLogNumBytes);
    BMQTST_ASSERT_EQ(ledger->outstandingNumBytes(oldLogId), k_LONG_ENTRY_LEN);
    BMQTST_ASSERT_EQ(ledger->totalNumBytes(),
                     oldLogNumBytes + Tester::k_OLD_LOG_LEN);
    BMQTST_ASSERT_EQ(ledger->outstandingNumBytes(),
                     k_LONG_ENTRY_LEN + Tester::k_OLD_LOG_LEN);

    // 4. Write a record that would trigger rollover and would be written to a
    //    new log.
    bdlbb::Blob blob3(tester.bufferFactory(),
                      bmqtst::TestHelperUtil::allocator());
    bdlbb::BlobUtil::append(&blob3, k_EXTRA_ENTRY_1, k_EXTRA_ENTRY_LEN);

    bmqu::BlobPosition start3;
    bmqu::BlobPosition end3(1, 0);
    bmqu::BlobSection  section3(start3, end3);
    BMQTST_ASSERT_EQ(ledger->writeRecord(&recordId, blob3, section3),
                     LedgerOpResult::e_SUCCESS);

    // Rollover should be triggered here.
    BMQTST_ASSERT_EQ(ledger->numLogs(), 3U);
    bsls::Types::Int64 newLogNumBytes = k_EXTRA_ENTRY_LEN;

    const mqbu::StorageKey& newLogId =
        ledger->currentLog()->logConfig().logId();
    PVV("Old log ID: " << oldLogId);
    PVV("New log ID: " << newLogId);
    BMQTST_ASSERT_NE(newLogId, oldLogId);
    BMQTST_ASSERT_EQ(recordId.logId(), newLogId);
    BMQTST_ASSERT_EQ(recordId.offset(), 0);

    BMQTST_ASSERT_EQ(ledger->totalNumBytes(newLogId), newLogNumBytes);
    BMQTST_ASSERT_EQ(ledger->outstandingNumBytes(newLogId), newLogNumBytes);
    BMQTST_ASSERT_EQ(ledger->totalNumBytes(oldLogId), oldLogNumBytes);
    BMQTST_ASSERT_EQ(ledger->outstandingNumBytes(oldLogId), k_LONG_ENTRY_LEN);
    BMQTST_ASSERT_EQ(ledger->totalNumBytes(),
                     newLogNumBytes + oldLogNumBytes + Tester::k_OLD_LOG_LEN);
    BMQTST_ASSERT_EQ(ledger->outstandingNumBytes(),
                     newLogNumBytes + k_LONG_ENTRY_LEN +
                         Tester::k_OLD_LOG_LEN);

    // 5. Write one more record.
    bdlbb::Blob blob4(tester.bufferFactory(),
                      bmqtst::TestHelperUtil::allocator());
    bdlbb::BlobUtil::append(&blob4, k_EXTRA_ENTRY_2, k_EXTRA_ENTRY_LEN);

    bmqu::BlobPosition start4;
    bmqu::BlobPosition end4(1, 0);
    bmqu::BlobSection  section4(start4, end4);
    BMQTST_ASSERT_EQ(ledger->writeRecord(&recordId, blob4, section4),
                     LedgerOpResult::e_SUCCESS);
    BMQTST_ASSERT_EQ(recordId.logId(), newLogId);
    BMQTST_ASSERT_EQ(recordId.offset(), k_EXTRA_ENTRY_LEN);

    newLogNumBytes += k_EXTRA_ENTRY_LEN;
    BMQTST_ASSERT_EQ(ledger->totalNumBytes(newLogId), newLogNumBytes);
    BMQTST_ASSERT_EQ(ledger->outstandingNumBytes(newLogId), newLogNumBytes);
    BMQTST_ASSERT_EQ(ledger->totalNumBytes(oldLogId), oldLogNumBytes);
    BMQTST_ASSERT_EQ(ledger->outstandingNumBytes(oldLogId), k_LONG_ENTRY_LEN);
    BMQTST_ASSERT_EQ(ledger->totalNumBytes(),
                     newLogNumBytes + oldLogNumBytes + Tester::k_OLD_LOG_LEN);
    BMQTST_ASSERT_EQ(ledger->outstandingNumBytes(),
                     newLogNumBytes + k_LONG_ENTRY_LEN +
                         Tester::k_OLD_LOG_LEN);

    // 6. Close and re-open the ledger.  'totalNumBytes' and
    //    'outstandingNumBytes' must be re-calibrated.
    ledger->setOutstandingNumBytes(oldLogId, 0);
    ledger->setOutstandingNumBytes(newLogId, 0);
    BSLS_ASSERT_OPT(ledger->flush() == LedgerOpResult::e_SUCCESS);
    BSLS_ASSERT_OPT(ledger->close() == LedgerOpResult::e_SUCCESS);

    BSLS_ASSERT_OPT(ledger->open(mqbsi::Ledger::e_READ_ONLY) ==
                    LedgerOpResult::e_SUCCESS);
    BMQTST_ASSERT_EQ(ledger->totalNumBytes(newLogId), newLogNumBytes);
    BMQTST_ASSERT_EQ(ledger->outstandingNumBytes(newLogId), newLogNumBytes);
    BMQTST_ASSERT_EQ(ledger->totalNumBytes(oldLogId), oldLogNumBytes);
    BMQTST_ASSERT_EQ(ledger->outstandingNumBytes(oldLogId), oldLogNumBytes);
    BMQTST_ASSERT_EQ(ledger->totalNumBytes(),
                     newLogNumBytes + oldLogNumBytes + Tester::k_OLD_LOG_LEN);
    BMQTST_ASSERT_EQ(ledger->outstandingNumBytes(),
                     newLogNumBytes + oldLogNumBytes + Tester::k_OLD_LOG_LEN);

    BSLS_ASSERT_OPT(ledger->close() == LedgerOpResult::e_SUCCESS);
}

static void test9_readRecordRaw()
// ------------------------------------------------------------------------
// READ RECORD RAW
//
// Concerns:
//   Verify that 'readRecord' works as intended when dealing with void*,
//   even when rollover is involved.
//
// Testing:
//   readRecord(void                  *entry,
//              int                    length,
//              const LedgerRecordId&  recordId)
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("READ RECORD RAW");

    Tester tester(true,
                  Tester::k_OLD_LOG_LEN + k_ENTRY_LEN * k_NUM_ENTRIES +
                      k_LONG_ENTRY_LEN);
    tester.generateOldLogs(2);

    // Run the impl. of the "WRITE RECORD RAW" test to write some records into
    // the ledger.  As a result, the ledger should have 3 log files, where each
    // log should have these messages respectively:
    //
    // Log 1: only a dummy log message
    // Log 2: a dummy log message, followed by a list of records, followed by a
    //        long record
    // Log 3: 2 extra records, namely k_EXTRA_ENTRY_1 and k_EXTRA_ENTRY_2
    mqbsi::Ledger* ledger = tester.ledger();
    writeRecordRawImpl(ledger);
    BSLS_ASSERT_OPT(ledger->open(mqbsi::Ledger::e_READ_ONLY) ==
                    LedgerOpResult::e_SUCCESS);
    BSLS_ASSERT_OPT(ledger->numLogs() == 3U);

    const mqbu::StorageKey& logId1 = ledger->logs()[0]->logConfig().logId();
    const mqbu::StorageKey& logId2 = ledger->logs()[1]->logConfig().logId();
    const mqbu::StorageKey& logId3 = ledger->logs()[2]->logConfig().logId();
    BSLS_ASSERT_OPT(ledger->totalNumBytes(logId1) == Tester::k_OLD_LOG_LEN);
    BSLS_ASSERT_OPT(ledger->totalNumBytes(logId2) ==
                    Tester::k_OLD_LOG_LEN + k_ENTRY_LEN * k_NUM_ENTRIES +
                        k_LONG_ENTRY_LEN);
    BSLS_ASSERT_OPT(ledger->totalNumBytes(logId3) == k_EXTRA_ENTRY_LEN * 2);

    // 1. Read each record in Log 1
    char entry[k_DUMMY_LOG_MESSAGE_LEN];

    // Skip the log ID written at the beginning of the log
    LedgerRecordId recordId(logId1, mqbu::StorageKey::e_KEY_LENGTH_BINARY);
    BMQTST_ASSERT_EQ(
        ledger->readRecord(entry, k_DUMMY_LOG_MESSAGE_LEN, recordId),
        LedgerOpResult::e_SUCCESS);
    BMQTST_ASSERT_EQ(
        bsl::memcmp(entry, k_DUMMY_LOG_MESSAGE, k_DUMMY_LOG_MESSAGE_LEN),
        0);

    // 2. Read each record in Log 2
    recordId.setLogId(logId2).setOffset(mqbu::StorageKey::e_KEY_LENGTH_BINARY);
    // Again, skip the log ID written at the beginning of the log
    BMQTST_ASSERT_EQ(
        ledger->readRecord(entry, k_DUMMY_LOG_MESSAGE_LEN, recordId),
        LedgerOpResult::e_SUCCESS);
    BMQTST_ASSERT_EQ(
        bsl::memcmp(entry, k_DUMMY_LOG_MESSAGE, k_DUMMY_LOG_MESSAGE_LEN),
        0);
    recordId.setOffset(recordId.offset() + k_DUMMY_LOG_MESSAGE_LEN);

    for (int i = 0; i < k_NUM_ENTRIES; ++i) {
        BMQTST_ASSERT_EQ(ledger->readRecord(entry, k_ENTRY_LEN, recordId),
                         LedgerOpResult::e_SUCCESS);
        BMQTST_ASSERT_EQ(bsl::memcmp(entry, k_ENTRIES[i], k_ENTRY_LEN), 0);
        recordId.setOffset(recordId.offset() + k_ENTRY_LEN);
    }

    BMQTST_ASSERT_EQ(ledger->readRecord(entry, k_LONG_ENTRY_LEN, recordId),
                     LedgerOpResult::e_SUCCESS);
    BMQTST_ASSERT_EQ(bsl::memcmp(entry, k_LONG_ENTRY_MEAT, k_LONG_ENTRY_LEN),
                     0);

    // 3. Read each record in Log 3
    recordId.setLogId(logId3).setOffset(0);
    BMQTST_ASSERT_EQ(ledger->readRecord(entry, k_EXTRA_ENTRY_LEN, recordId),
                     LedgerOpResult::e_SUCCESS);
    BMQTST_ASSERT_EQ(bsl::memcmp(entry, k_EXTRA_ENTRY_1, k_EXTRA_ENTRY_LEN),
                     0);
    recordId.setOffset(recordId.offset() + k_EXTRA_ENTRY_LEN);

    BMQTST_ASSERT_EQ(ledger->readRecord(entry, k_EXTRA_ENTRY_LEN, recordId),
                     LedgerOpResult::e_SUCCESS);
    BMQTST_ASSERT_EQ(bsl::memcmp(entry, k_EXTRA_ENTRY_2, k_EXTRA_ENTRY_LEN),
                     0);

    // 4. Close and re-open the ledger
    BSLS_ASSERT_OPT(ledger->close() == LedgerOpResult::e_SUCCESS);
    BSLS_ASSERT_OPT(ledger->open(mqbsi::Ledger::e_CREATE_IF_MISSING) ==
                    LedgerOpResult::e_SUCCESS);

    // 5. Write another extra record
    LedgerRecordId extraRecordId;
    BSLS_ASSERT_OPT(ledger->writeRecord(&extraRecordId,
                                        k_EXTRA_ENTRY_3,
                                        0,
                                        k_EXTRA_ENTRY_LEN) ==
                    LedgerOpResult::e_SUCCESS);
    BSLS_ASSERT_OPT(extraRecordId.logId() == logId3);
    BSLS_ASSERT_OPT(extraRecordId.offset() == k_EXTRA_ENTRY_LEN * 2);

    // 6. Re-read the list of records from Log 2, then read the newest record
    //    in Log 3
    recordId.setLogId(logId2).setOffset(Tester::k_OLD_LOG_LEN);
    for (int i = 0; i < k_NUM_ENTRIES; ++i) {
        BMQTST_ASSERT_EQ(ledger->readRecord(entry, k_ENTRY_LEN, recordId),
                         LedgerOpResult::e_SUCCESS);
        BMQTST_ASSERT_EQ(bsl::memcmp(entry, k_ENTRIES[i], k_ENTRY_LEN), 0);
        recordId.setOffset(recordId.offset() + k_ENTRY_LEN);
    }

    BMQTST_ASSERT_EQ(
        ledger->readRecord(entry, k_EXTRA_ENTRY_LEN, extraRecordId),
        LedgerOpResult::e_SUCCESS);
    BMQTST_ASSERT_EQ(bsl::memcmp(entry, k_EXTRA_ENTRY_3, k_EXTRA_ENTRY_LEN),
                     0);

    // 7. Read from an invalid log ID should fail
    BMQTST_ASSERT_EQ(ledger->readRecord(entry,
                                        k_ENTRY_LEN,
                                        LedgerRecordId(mqbu::StorageKey(), 0)),
                     LedgerOpResult::e_LOG_NOT_FOUND);

    // 8. Read beyond the last record offset should fail
    BMQTST_ASSERT_EQ(
        ledger->readRecord(entry, k_ENTRY_LEN, LedgerRecordId(logId3, 9999)) %
            100,
        LedgerOpResult::e_RECORD_READ_FAILURE);

    // 9. Read beyond the length of a log should fail
    BMQTST_ASSERT_EQ(
        ledger->readRecord(entry, 9999, LedgerRecordId(logId1, 0)) % 100,
        LedgerOpResult::e_RECORD_READ_FAILURE);

    BSLS_ASSERT_OPT(ledger->flush() == LedgerOpResult::e_SUCCESS);
    BSLS_ASSERT_OPT(ledger->close() == LedgerOpResult::e_SUCCESS);
}

static void test10_readRecordBlob()
// ------------------------------------------------------------------------
// READ RECORD BLOB
//
// Concerns:
//   Verify that 'readRecord' works as intended when dealing with blobs,
//   even when rollover is involved.
//
// Testing:
//   readRecord(bdlbb::Blob            *entry,
//              int                    length,
//              const LedgerRecordId&  recordId)
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("READ RECORD BLOB");

    Tester tester(true,
                  Tester::k_OLD_LOG_LEN + k_ENTRY_LEN * k_NUM_ENTRIES +
                      k_LONG_ENTRY_LEN);
    tester.generateOldLogs(2);

    // Run the impl. of the "WRITE RECORD RAW" test to write some records into
    // the ledger.  As a result, the ledger should have 3 log files, where each
    // log should have these messages respectively:
    //
    // Log 1: only a dummy log message
    // Log 2: a dummy log message, followed by a list of records, followed by a
    //        long record
    // Log 3: 2 extra records, namely k_EXTRA_ENTRY_1 and k_EXTRA_ENTRY_2
    mqbsi::Ledger* ledger = tester.ledger();
    writeRecordRawImpl(ledger);
    BSLS_ASSERT_OPT(ledger->open(mqbsi::Ledger::e_READ_ONLY) ==
                    LedgerOpResult::e_SUCCESS);
    BSLS_ASSERT_OPT(ledger->numLogs() == 3U);

    const mqbu::StorageKey& logId1 = ledger->logs()[0]->logConfig().logId();
    const mqbu::StorageKey& logId2 = ledger->logs()[1]->logConfig().logId();
    const mqbu::StorageKey& logId3 = ledger->logs()[2]->logConfig().logId();
    BSLS_ASSERT_OPT(ledger->totalNumBytes(logId1) == Tester::k_OLD_LOG_LEN);
    BSLS_ASSERT_OPT(ledger->totalNumBytes(logId2) ==
                    Tester::k_OLD_LOG_LEN + k_ENTRY_LEN * k_NUM_ENTRIES +
                        k_LONG_ENTRY_LEN);
    BSLS_ASSERT_OPT(ledger->totalNumBytes(logId3) == k_EXTRA_ENTRY_LEN * 2);

    // 1. Read each record in Log 1
    bdlbb::Blob blob(tester.bufferFactory(),
                     bmqtst::TestHelperUtil::allocator());
    char        entry[k_DUMMY_LOG_MESSAGE_LEN];

    // Skip the log ID written at the beginning of the log
    LedgerRecordId recordId(logId1, mqbu::StorageKey::e_KEY_LENGTH_BINARY);
    BMQTST_ASSERT_EQ(
        ledger->readRecord(&blob, k_DUMMY_LOG_MESSAGE_LEN, recordId),
        LedgerOpResult::e_SUCCESS);
    bmqu::BlobUtil::readNBytes(entry,
                               blob,
                               bmqu::BlobPosition(),
                               k_DUMMY_LOG_MESSAGE_LEN);
    BMQTST_ASSERT_EQ(
        bsl::memcmp(entry, k_DUMMY_LOG_MESSAGE, k_DUMMY_LOG_MESSAGE_LEN),
        0);
    blob.removeBuffer(0);

    // 2. Read each record in Log 2
    recordId.setLogId(logId2).setOffset(mqbu::StorageKey::e_KEY_LENGTH_BINARY);
    // Again, skip the log ID written at the beginning of the log
    BMQTST_ASSERT_EQ(
        ledger->readRecord(&blob, k_DUMMY_LOG_MESSAGE_LEN, recordId),
        LedgerOpResult::e_SUCCESS);
    bmqu::BlobUtil::readNBytes(entry,
                               blob,
                               bmqu::BlobPosition(),
                               k_DUMMY_LOG_MESSAGE_LEN);
    BMQTST_ASSERT_EQ(
        bsl::memcmp(entry, k_DUMMY_LOG_MESSAGE, k_DUMMY_LOG_MESSAGE_LEN),
        0);
    recordId.setOffset(recordId.offset() + k_DUMMY_LOG_MESSAGE_LEN);
    blob.removeBuffer(0);

    for (int i = 0; i < k_NUM_ENTRIES; ++i) {
        BMQTST_ASSERT_EQ(ledger->readRecord(&blob, k_ENTRY_LEN, recordId),
                         LedgerOpResult::e_SUCCESS);
        bmqu::BlobUtil::readNBytes(entry,
                                   blob,
                                   bmqu::BlobPosition(),
                                   k_ENTRY_LEN);
        BMQTST_ASSERT_EQ(bsl::memcmp(entry, k_ENTRIES[i], k_ENTRY_LEN), 0);
        recordId.setOffset(recordId.offset() + k_ENTRY_LEN);
        blob.removeBuffer(0);
    }

    BMQTST_ASSERT_EQ(ledger->readRecord(&blob, k_LONG_ENTRY_LEN, recordId),
                     LedgerOpResult::e_SUCCESS);
    bmqu::BlobUtil::readNBytes(entry,
                               blob,
                               bmqu::BlobPosition(),
                               k_LONG_ENTRY_LEN);
    BMQTST_ASSERT_EQ(bsl::memcmp(entry, k_LONG_ENTRY_MEAT, k_LONG_ENTRY_LEN),
                     0);
    blob.removeBuffer(0);

    // 3. Read each record in Log 3
    recordId.setLogId(logId3).setOffset(0);
    BMQTST_ASSERT_EQ(ledger->readRecord(&blob, k_EXTRA_ENTRY_LEN, recordId),
                     LedgerOpResult::e_SUCCESS);
    bmqu::BlobUtil::readNBytes(entry,
                               blob,
                               bmqu::BlobPosition(),
                               k_EXTRA_ENTRY_LEN);
    BMQTST_ASSERT_EQ(bsl::memcmp(entry, k_EXTRA_ENTRY_1, k_EXTRA_ENTRY_LEN),
                     0);
    recordId.setOffset(recordId.offset() + k_EXTRA_ENTRY_LEN);
    blob.removeBuffer(0);

    BMQTST_ASSERT_EQ(ledger->readRecord(&blob, k_EXTRA_ENTRY_LEN, recordId),
                     LedgerOpResult::e_SUCCESS);
    bmqu::BlobUtil::readNBytes(entry,
                               blob,
                               bmqu::BlobPosition(),
                               k_EXTRA_ENTRY_LEN);
    BMQTST_ASSERT_EQ(bsl::memcmp(entry, k_EXTRA_ENTRY_2, k_EXTRA_ENTRY_LEN),
                     0);
    blob.removeBuffer(0);

    // 4. Close and re-open the ledger
    BSLS_ASSERT_OPT(ledger->close() == LedgerOpResult::e_SUCCESS);
    BSLS_ASSERT_OPT(ledger->open(mqbsi::Ledger::e_CREATE_IF_MISSING) ==
                    LedgerOpResult::e_SUCCESS);

    // 5. Write another extra record
    LedgerRecordId extraRecordId;
    BSLS_ASSERT_OPT(ledger->writeRecord(&extraRecordId,
                                        k_EXTRA_ENTRY_3,
                                        0,
                                        k_EXTRA_ENTRY_LEN) ==
                    LedgerOpResult::e_SUCCESS);
    BSLS_ASSERT_OPT(extraRecordId.logId() == logId3);
    BSLS_ASSERT_OPT(extraRecordId.offset() == k_EXTRA_ENTRY_LEN * 2);

    // 6. Re-read the list of records from Log 2, then read the newest record
    //    in Log 3
    recordId.setLogId(logId2).setOffset(Tester::k_OLD_LOG_LEN);
    for (int i = 0; i < k_NUM_ENTRIES; ++i) {
        BMQTST_ASSERT_EQ(ledger->readRecord(&blob, k_ENTRY_LEN, recordId),
                         LedgerOpResult::e_SUCCESS);
        bmqu::BlobUtil::readNBytes(entry,
                                   blob,
                                   bmqu::BlobPosition(),
                                   k_ENTRY_LEN);
        BMQTST_ASSERT_EQ(bsl::memcmp(entry, k_ENTRIES[i], k_ENTRY_LEN), 0);
        recordId.setOffset(recordId.offset() + k_ENTRY_LEN);
        blob.removeBuffer(0);
    }

    BMQTST_ASSERT_EQ(
        ledger->readRecord(&blob, k_EXTRA_ENTRY_LEN, extraRecordId),
        LedgerOpResult::e_SUCCESS);
    bmqu::BlobUtil::readNBytes(entry,
                               blob,
                               bmqu::BlobPosition(),
                               k_EXTRA_ENTRY_LEN);
    BMQTST_ASSERT_EQ(bsl::memcmp(entry, k_EXTRA_ENTRY_3, k_EXTRA_ENTRY_LEN),
                     0);
    blob.removeBuffer(0);

    // 7. Read from an invalid log ID should fail
    BMQTST_ASSERT_EQ(ledger->readRecord(&blob,
                                        k_ENTRY_LEN,
                                        LedgerRecordId(mqbu::StorageKey(), 0)),
                     LedgerOpResult::e_LOG_NOT_FOUND);

    // 8. Read beyond the last record offset should fail
    BMQTST_ASSERT_EQ(
        ledger->readRecord(&blob, k_ENTRY_LEN, LedgerRecordId(logId3, 9999)) %
            100,
        LedgerOpResult::e_RECORD_READ_FAILURE);

    // 9. Read beyond the length of a log should fail
    BMQTST_ASSERT_EQ(
        ledger->readRecord(&blob, 9999, LedgerRecordId(logId1, 0)) % 100,
        LedgerOpResult::e_RECORD_READ_FAILURE);

    BSLS_ASSERT_OPT(ledger->flush() == LedgerOpResult::e_SUCCESS);
    BSLS_ASSERT_OPT(ledger->close() == LedgerOpResult::e_SUCCESS);
}

static void test11_aliasRecordRaw()
// ------------------------------------------------------------------------
// ALIAS RECORD RAW
//
// Concerns:
//   Verify that 'aliasRecord' works as intended when dealing with void*,
//   even when rollover is involved.
//
// Testing:
//   aliasRecord(void                  **entry,
//               int                     length,
//               const LedgerRecordId&   recordId)
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("ALIAS RECORD RAW");

    Tester tester(true,
                  Tester::k_OLD_LOG_LEN + k_ENTRY_LEN * k_NUM_ENTRIES +
                      k_LONG_ENTRY_LEN);
    tester.generateOldLogs(2);

    // Run the impl. of the "WRITE RECORD RAW" test to write some records into
    // the ledger.  As a result, the ledger should have 3 log files, where each
    // log should have these messages respectively:
    //
    // Log 1: only a dummy log message
    // Log 2: a dummy log message, followed by a list of records, followed by a
    //        long record
    // Log 3: 2 extra records, namely k_EXTRA_ENTRY_1 and k_EXTRA_ENTRY_2
    mqbsi::Ledger* ledger = tester.ledger();
    writeRecordRawImpl(ledger);
    BSLS_ASSERT_OPT(ledger->open(mqbsi::Ledger::e_READ_ONLY) ==
                    LedgerOpResult::e_SUCCESS);
    BSLS_ASSERT_OPT(ledger->numLogs() == 3U);

    const mqbu::StorageKey& logId1 = ledger->logs()[0]->logConfig().logId();
    const mqbu::StorageKey& logId2 = ledger->logs()[1]->logConfig().logId();
    const mqbu::StorageKey& logId3 = ledger->logs()[2]->logConfig().logId();
    BSLS_ASSERT_OPT(ledger->totalNumBytes(logId1) == Tester::k_OLD_LOG_LEN);
    BSLS_ASSERT_OPT(ledger->totalNumBytes(logId2) ==
                    Tester::k_OLD_LOG_LEN + k_ENTRY_LEN * k_NUM_ENTRIES +
                        k_LONG_ENTRY_LEN);
    BSLS_ASSERT_OPT(ledger->totalNumBytes(logId3) == k_EXTRA_ENTRY_LEN * 2);

    // 1. Read each record in Log 1
    char* entry;

    // Skip the log ID written at the beginning of the log
    LedgerRecordId recordId(logId1, mqbu::StorageKey::e_KEY_LENGTH_BINARY);
    BMQTST_ASSERT_EQ(ledger->aliasRecord(reinterpret_cast<void**>(&entry),
                                         k_DUMMY_LOG_MESSAGE_LEN,
                                         recordId),
                     LedgerOpResult::e_SUCCESS);
    BMQTST_ASSERT_EQ(
        bsl::memcmp(entry, k_DUMMY_LOG_MESSAGE, k_DUMMY_LOG_MESSAGE_LEN),
        0);

    // 2. Read each record in Log 2
    recordId.setLogId(logId2).setOffset(mqbu::StorageKey::e_KEY_LENGTH_BINARY);
    // Again, skip the log ID written at the beginning of the log
    BMQTST_ASSERT_EQ(ledger->aliasRecord(reinterpret_cast<void**>(&entry),
                                         k_DUMMY_LOG_MESSAGE_LEN,
                                         recordId),
                     LedgerOpResult::e_SUCCESS);
    BMQTST_ASSERT_EQ(
        bsl::memcmp(entry, k_DUMMY_LOG_MESSAGE, k_DUMMY_LOG_MESSAGE_LEN),
        0);
    recordId.setOffset(recordId.offset() + k_DUMMY_LOG_MESSAGE_LEN);

    for (int i = 0; i < k_NUM_ENTRIES; ++i) {
        BMQTST_ASSERT_EQ(ledger->aliasRecord(reinterpret_cast<void**>(&entry),
                                             k_ENTRY_LEN,
                                             recordId),
                         LedgerOpResult::e_SUCCESS);
        BMQTST_ASSERT_EQ(bsl::memcmp(entry, k_ENTRIES[i], k_ENTRY_LEN), 0);
        recordId.setOffset(recordId.offset() + k_ENTRY_LEN);
    }

    BMQTST_ASSERT_EQ(ledger->aliasRecord(reinterpret_cast<void**>(&entry),
                                         k_LONG_ENTRY_LEN,
                                         recordId),
                     LedgerOpResult::e_SUCCESS);
    BMQTST_ASSERT_EQ(bsl::memcmp(entry, k_LONG_ENTRY_MEAT, k_LONG_ENTRY_LEN),
                     0);

    // 3. Read each record in Log 3
    recordId.setLogId(logId3).setOffset(0);
    BMQTST_ASSERT_EQ(ledger->aliasRecord(reinterpret_cast<void**>(&entry),
                                         k_EXTRA_ENTRY_LEN,
                                         recordId),
                     LedgerOpResult::e_SUCCESS);
    BMQTST_ASSERT_EQ(bsl::memcmp(entry, k_EXTRA_ENTRY_1, k_EXTRA_ENTRY_LEN),
                     0);
    recordId.setOffset(recordId.offset() + k_EXTRA_ENTRY_LEN);

    BMQTST_ASSERT_EQ(ledger->aliasRecord(reinterpret_cast<void**>(&entry),
                                         k_EXTRA_ENTRY_LEN,
                                         recordId),
                     LedgerOpResult::e_SUCCESS);
    BMQTST_ASSERT_EQ(bsl::memcmp(entry, k_EXTRA_ENTRY_2, k_EXTRA_ENTRY_LEN),
                     0);

    // 4. Close and re-open the ledger
    BSLS_ASSERT_OPT(ledger->close() == LedgerOpResult::e_SUCCESS);
    BSLS_ASSERT_OPT(ledger->open(mqbsi::Ledger::e_CREATE_IF_MISSING) ==
                    LedgerOpResult::e_SUCCESS);

    // 5. Write another extra record
    LedgerRecordId extraRecordId;
    BSLS_ASSERT_OPT(ledger->writeRecord(&extraRecordId,
                                        k_EXTRA_ENTRY_3,
                                        0,
                                        k_EXTRA_ENTRY_LEN) ==
                    LedgerOpResult::e_SUCCESS);
    BSLS_ASSERT_OPT(extraRecordId.logId() == logId3);
    BSLS_ASSERT_OPT(extraRecordId.offset() == k_EXTRA_ENTRY_LEN * 2);

    // 6. Re-read the list of records from Log 2, then read the newest record
    //    in Log 3
    recordId.setLogId(logId2).setOffset(Tester::k_OLD_LOG_LEN);
    for (int i = 0; i < k_NUM_ENTRIES; ++i) {
        BMQTST_ASSERT_EQ(ledger->aliasRecord(reinterpret_cast<void**>(&entry),
                                             k_ENTRY_LEN,
                                             recordId),
                         LedgerOpResult::e_SUCCESS);
        BMQTST_ASSERT_EQ(bsl::memcmp(entry, k_ENTRIES[i], k_ENTRY_LEN), 0);
        recordId.setOffset(recordId.offset() + k_ENTRY_LEN);
    }

    BMQTST_ASSERT_EQ(ledger->aliasRecord(reinterpret_cast<void**>(&entry),
                                         k_EXTRA_ENTRY_LEN,
                                         extraRecordId),
                     LedgerOpResult::e_SUCCESS);
    BMQTST_ASSERT_EQ(bsl::memcmp(entry, k_EXTRA_ENTRY_3, k_EXTRA_ENTRY_LEN),
                     0);

    // 7. Read from an invalid log ID should fail
    BMQTST_ASSERT_EQ(
        ledger->aliasRecord(reinterpret_cast<void**>(&entry),
                            k_ENTRY_LEN,
                            LedgerRecordId(mqbu::StorageKey(), 0)),
        LedgerOpResult::e_LOG_NOT_FOUND);

    // 8. Read beyond the last record offset should fail
    BMQTST_ASSERT_EQ(ledger->aliasRecord(reinterpret_cast<void**>(&entry),
                                         k_ENTRY_LEN,
                                         LedgerRecordId(logId3, 9999)) %
                         100,
                     LedgerOpResult::e_RECORD_ALIAS_FAILURE);

    // 9. Read beyond the length of a log should fail
    BMQTST_ASSERT_EQ(ledger->aliasRecord(reinterpret_cast<void**>(&entry),
                                         9999,
                                         LedgerRecordId(logId1, 0)) %
                         100,
                     LedgerOpResult::e_RECORD_ALIAS_FAILURE);

    BSLS_ASSERT_OPT(ledger->flush() == LedgerOpResult::e_SUCCESS);
    BSLS_ASSERT_OPT(ledger->close() == LedgerOpResult::e_SUCCESS);
}

static void test12_aliasRecordBlob()
// ------------------------------------------------------------------------
// ALIAS RECORD BLOB
//
// Concerns:
//   Verify that 'aliasRecord' works as intended when dealing with blobs,
//   even when rollover is involved.
//
// Testing:
//   aliasRecord(bdlbb::Blob            *entry,
//               int                    length,
//               const LedgerRecordId&  recordId)
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("ALIAS RECORD BLOB");

    Tester tester(true,
                  Tester::k_OLD_LOG_LEN + k_ENTRY_LEN * k_NUM_ENTRIES +
                      k_LONG_ENTRY_LEN);
    tester.generateOldLogs(2);

    // Run the impl. of the "WRITE RECORD RAW" test to write some records into
    // the ledger.  As a result, the ledger should have 3 log files, where each
    // log should have these messages respectively:
    //
    // Log 1: only a dummy log message
    // Log 2: a dummy log message, followed by a list of records, followed by a
    //        long record
    // Log 3: 2 extra records, namely k_EXTRA_ENTRY_1 and k_EXTRA_ENTRY_2
    mqbsi::Ledger* ledger = tester.ledger();
    writeRecordRawImpl(ledger);
    BSLS_ASSERT_OPT(ledger->open(mqbsi::Ledger::e_READ_ONLY) ==
                    LedgerOpResult::e_SUCCESS);
    BSLS_ASSERT_OPT(ledger->numLogs() == 3U);

    const mqbu::StorageKey& logId1 = ledger->logs()[0]->logConfig().logId();
    const mqbu::StorageKey& logId2 = ledger->logs()[1]->logConfig().logId();
    const mqbu::StorageKey& logId3 = ledger->logs()[2]->logConfig().logId();
    BSLS_ASSERT_OPT(ledger->totalNumBytes(logId1) == Tester::k_OLD_LOG_LEN);
    BSLS_ASSERT_OPT(ledger->totalNumBytes(logId2) ==
                    Tester::k_OLD_LOG_LEN + k_ENTRY_LEN * k_NUM_ENTRIES +
                        k_LONG_ENTRY_LEN);
    BSLS_ASSERT_OPT(ledger->totalNumBytes(logId3) == k_EXTRA_ENTRY_LEN * 2);

    // 1. Read each record in Log 1
    bdlbb::Blob blob(tester.bufferFactory(),
                     bmqtst::TestHelperUtil::allocator());
    char        entry[k_DUMMY_LOG_MESSAGE_LEN];

    // Skip the log ID written at the beginning of the log
    LedgerRecordId recordId(logId1, mqbu::StorageKey::e_KEY_LENGTH_BINARY);
    BMQTST_ASSERT_EQ(
        ledger->aliasRecord(&blob, k_DUMMY_LOG_MESSAGE_LEN, recordId),
        LedgerOpResult::e_SUCCESS);
    bmqu::BlobUtil::readNBytes(entry,
                               blob,
                               bmqu::BlobPosition(),
                               k_DUMMY_LOG_MESSAGE_LEN);
    BMQTST_ASSERT_EQ(
        bsl::memcmp(entry, k_DUMMY_LOG_MESSAGE, k_DUMMY_LOG_MESSAGE_LEN),
        0);
    blob.removeBuffer(0);

    // 2. Read each record in Log 2
    recordId.setLogId(logId2).setOffset(mqbu::StorageKey::e_KEY_LENGTH_BINARY);
    // Again, skip the log ID written at the beginning of the log
    BMQTST_ASSERT_EQ(
        ledger->aliasRecord(&blob, k_DUMMY_LOG_MESSAGE_LEN, recordId),
        LedgerOpResult::e_SUCCESS);
    bmqu::BlobUtil::readNBytes(entry,
                               blob,
                               bmqu::BlobPosition(),
                               k_DUMMY_LOG_MESSAGE_LEN);
    BMQTST_ASSERT_EQ(
        bsl::memcmp(entry, k_DUMMY_LOG_MESSAGE, k_DUMMY_LOG_MESSAGE_LEN),
        0);
    recordId.setOffset(recordId.offset() + k_DUMMY_LOG_MESSAGE_LEN);
    blob.removeBuffer(0);

    for (int i = 0; i < k_NUM_ENTRIES; ++i) {
        BMQTST_ASSERT_EQ(ledger->aliasRecord(&blob, k_ENTRY_LEN, recordId),
                         LedgerOpResult::e_SUCCESS);
        bmqu::BlobUtil::readNBytes(entry,
                                   blob,
                                   bmqu::BlobPosition(),
                                   k_ENTRY_LEN);
        BMQTST_ASSERT_EQ(bsl::memcmp(entry, k_ENTRIES[i], k_ENTRY_LEN), 0);
        recordId.setOffset(recordId.offset() + k_ENTRY_LEN);
        blob.removeBuffer(0);
    }

    BMQTST_ASSERT_EQ(ledger->aliasRecord(&blob, k_LONG_ENTRY_LEN, recordId),
                     LedgerOpResult::e_SUCCESS);
    bmqu::BlobUtil::readNBytes(entry,
                               blob,
                               bmqu::BlobPosition(),
                               k_LONG_ENTRY_LEN);
    BMQTST_ASSERT_EQ(bsl::memcmp(entry, k_LONG_ENTRY_MEAT, k_LONG_ENTRY_LEN),
                     0);
    blob.removeBuffer(0);

    // 3. Read each record in Log 3
    recordId.setLogId(logId3).setOffset(0);
    BMQTST_ASSERT_EQ(ledger->aliasRecord(&blob, k_EXTRA_ENTRY_LEN, recordId),
                     LedgerOpResult::e_SUCCESS);
    bmqu::BlobUtil::readNBytes(entry,
                               blob,
                               bmqu::BlobPosition(),
                               k_EXTRA_ENTRY_LEN);
    BMQTST_ASSERT_EQ(bsl::memcmp(entry, k_EXTRA_ENTRY_1, k_EXTRA_ENTRY_LEN),
                     0);
    recordId.setOffset(recordId.offset() + k_EXTRA_ENTRY_LEN);
    blob.removeBuffer(0);

    BMQTST_ASSERT_EQ(ledger->aliasRecord(&blob, k_EXTRA_ENTRY_LEN, recordId),
                     LedgerOpResult::e_SUCCESS);
    bmqu::BlobUtil::readNBytes(entry,
                               blob,
                               bmqu::BlobPosition(),
                               k_EXTRA_ENTRY_LEN);
    BMQTST_ASSERT_EQ(bsl::memcmp(entry, k_EXTRA_ENTRY_2, k_EXTRA_ENTRY_LEN),
                     0);
    blob.removeBuffer(0);

    // 4. Close and re-open the ledger
    BSLS_ASSERT_OPT(ledger->close() == LedgerOpResult::e_SUCCESS);
    BSLS_ASSERT_OPT(ledger->open(mqbsi::Ledger::e_CREATE_IF_MISSING) ==
                    LedgerOpResult::e_SUCCESS);

    // 5. Write another extra record
    LedgerRecordId extraRecordId;
    BSLS_ASSERT_OPT(ledger->writeRecord(&extraRecordId,
                                        k_EXTRA_ENTRY_3,
                                        0,
                                        k_EXTRA_ENTRY_LEN) ==
                    LedgerOpResult::e_SUCCESS);
    BSLS_ASSERT_OPT(extraRecordId.logId() == logId3);
    BSLS_ASSERT_OPT(extraRecordId.offset() == k_EXTRA_ENTRY_LEN * 2);

    // 6. Re-read the list of records from Log 2, then read the newest record
    //    in Log 3
    recordId.setLogId(logId2).setOffset(Tester::k_OLD_LOG_LEN);
    for (int i = 0; i < k_NUM_ENTRIES; ++i) {
        BMQTST_ASSERT_EQ(ledger->aliasRecord(&blob, k_ENTRY_LEN, recordId),
                         LedgerOpResult::e_SUCCESS);
        bmqu::BlobUtil::readNBytes(entry,
                                   blob,
                                   bmqu::BlobPosition(),
                                   k_ENTRY_LEN);
        BMQTST_ASSERT_EQ(bsl::memcmp(entry, k_ENTRIES[i], k_ENTRY_LEN), 0);
        recordId.setOffset(recordId.offset() + k_ENTRY_LEN);
        blob.removeBuffer(0);
    }

    BMQTST_ASSERT_EQ(
        ledger->aliasRecord(&blob, k_EXTRA_ENTRY_LEN, extraRecordId),
        LedgerOpResult::e_SUCCESS);
    bmqu::BlobUtil::readNBytes(entry,
                               blob,
                               bmqu::BlobPosition(),
                               k_EXTRA_ENTRY_LEN);
    BMQTST_ASSERT_EQ(bsl::memcmp(entry, k_EXTRA_ENTRY_3, k_EXTRA_ENTRY_LEN),
                     0);
    blob.removeBuffer(0);

    // 7. Read from an invalid log ID should fail
    BMQTST_ASSERT_EQ(
        ledger->aliasRecord(&blob,
                            k_ENTRY_LEN,
                            LedgerRecordId(mqbu::StorageKey(), 0)),
        LedgerOpResult::e_LOG_NOT_FOUND);

    // 8. Read beyond the last record offset should fail
    BMQTST_ASSERT_EQ(
        ledger->aliasRecord(&blob, k_ENTRY_LEN, LedgerRecordId(logId3, 9999)) %
            100,
        LedgerOpResult::e_RECORD_ALIAS_FAILURE);

    // 9. Read beyond the length of a log should fail
    BMQTST_ASSERT_EQ(
        ledger->aliasRecord(&blob, 9999, LedgerRecordId(logId1, 0)) % 100,
        LedgerOpResult::e_RECORD_ALIAS_FAILURE);

    BSLS_ASSERT_OPT(ledger->flush() == LedgerOpResult::e_SUCCESS);
    BSLS_ASSERT_OPT(ledger->close() == LedgerOpResult::e_SUCCESS);
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    bmqsys::Time::initialize(bmqtst::TestHelperUtil::allocator());

    {
        switch (_testCase) {
        case 0:
        case 1: test1_breathingTest(); break;
        case 2: test2_openMultipleLogs(); break;
        case 3: test3_openMultipleLogsNoKeep(); break;
        case 4: test4_updateOutstandingNumBytes(); break;
        case 5: test5_setOutstandingNumBytes(); break;
        case 6: test6_writeRecordRaw(); break;
        case 7: test7_writeRecordBlob(); break;
        case 8: test8_writeRecordBlobSection(); break;
        case 9: test9_readRecordRaw(); break;
        case 10: test10_readRecordBlob(); break;
        case 11: test11_aliasRecordRaw(); break;
        case 12: test12_aliasRecordBlob(); break;
        default: {
            cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
            bmqtst::TestHelperUtil::testStatus() = -1;
        } break;
        }
    }

    bmqsys::Time::shutdown();

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_GBL_ALLOC);
}
