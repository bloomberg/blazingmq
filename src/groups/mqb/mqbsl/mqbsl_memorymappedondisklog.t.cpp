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

// mqbsl_memorymappedondisklog.t.cpp                                  -*-C++-*-
#include <mqbsl_memorymappedondisklog.h>

// MQB
#include <mqbsi_log.h>
#include <mqbsl_ondisklog.h>
#include <mqbu_storagekey.h>

// BDE
#include <bdlbb_blob.h>
#include <bdlbb_blobutil.h>
#include <bdlbb_pooledblobbufferfactory.h>
#include <bsl_cstring.h>  // for memcmp
#include <bsls_types.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>
#include <bmqu_tempdirectory.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

//=============================================================================
//                             TEST PLAN
//-----------------------------------------------------------------------------
// - breathingTest
// - fileNotExist
// - updateOutstandingNumBytes
// - setOutstandingNumBytes
// - writeRaw
// - writeBlob
// - writeBlobSection
// - readRaw
// - readBlob
// - aliasRaw
// - aliasBlob
// - seek
//-----------------------------------------------------------------------------

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------

namespace {

// CONSTANTS
const bsls::Types::Int64 k_LOG_MAX_SIZE = 2048;
const char               k_LOG_ID[]     = "DEADFACE42";
const mqbu::StorageKey   k_LOG_KEY(mqbu::StorageKey::HexRepresentation(),
                                 k_LOG_ID);

const char* const k_ENTRIES[]    = {"ax001",
                                    "ax002",
                                    "ax003",
                                    "ax004",
                                    "ax005",
                                    "ax006",
                                    "ax007",
                                    "ax008",
                                    "ax009",
                                    "ax010"};
const int         k_NUM_ENTRIES  = 10;
const int         k_ENTRY_LENGTH = 5;

const char* const k_LONG_ENTRY             = "xxxxxxxxxxHELLO_WORLDxxxxxxxxxx";
const char* const k_LONG_ENTRY_MEAT        = "HELLO_WORLD";
const int         k_LONG_ENTRY_OFFSET      = 10;
const int         k_LONG_ENTRY_LENGTH      = 11;
const int         k_LONG_ENTRY_FULL_LENGTH = 31;

const char* const k_LONG_ENTRY2             = "xxxxBMQ_ROCKSxxxxxxxx";
const char* const k_LONG_ENTRY2_MEAT        = "BMQ_ROCKS";
const int         k_LONG_ENTRY2_OFFSET      = 4;
const int         k_LONG_ENTRY2_LENGTH      = 9;
const int         k_LONG_ENTRY2_FULL_LENGTH = 21;

// ALIASES
typedef mqbsl::MemoryMappedOnDiskLog MemoryMappedOnDiskLog;
typedef mqbsi::Log                   Log;
typedef mqbsi::Log::Offset           Offset;
typedef mqbsi::LogOpResult           LogOpResult;

// STATICS
static bdlbb::PooledBlobBufferFactory* g_bufferFactory_p     = 0;
static bdlbb::PooledBlobBufferFactory* g_miniBufferFactory_p = 0;

// CLASSES
// =============
// struct Tester
// =============

struct Tester {
  private:
    // DATA
    bmqu::TempDirectory    d_tempDirectory;
    const mqbsi::LogConfig d_config;
    MemoryMappedOnDiskLog  d_log;

  public:
    // CREATORS
    Tester(bsls::Types::Int64 logMaxSize = k_LOG_MAX_SIZE,
           bslma::Allocator*  allocator  = bmqtst::TestHelperUtil::allocator())
    : d_tempDirectory(allocator)
    , d_config(logMaxSize,
               k_LOG_KEY,
               d_tempDirectory.path() + "/test_log.bmq",
               true,   // reserveOnDisk
               false,  // prefaultPages
               allocator)
    , d_log(d_config)
    {
        // NOTHING
    }

    const mqbsi::LogConfig& config() { return d_config; }

    MemoryMappedOnDiskLog& log() { return d_log; }
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
//   Exercise the basic functionality of the component.
//
// Testing:
//   Basic functionality
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("BREATHING TEST");

    Tester                 tester;
    MemoryMappedOnDiskLog& log = tester.log();
    BMQTST_ASSERT_EQ(log.isOpened(), false);

    BMQTST_ASSERT_EQ(log.open(Log::e_CREATE_IF_MISSING),
                     LogOpResult::e_SUCCESS);
    BMQTST_ASSERT_EQ(log.isOpened(), true);
    BMQTST_ASSERT_EQ(log.totalNumBytes(), 0);
    BMQTST_ASSERT_EQ(log.outstandingNumBytes(), 0);
    BMQTST_ASSERT_EQ(log.currentOffset(), static_cast<Offset>(0));
    BMQTST_ASSERT_EQ(log.logConfig(), tester.config());
    BMQTST_ASSERT_EQ(log.supportsAliasing(), true);
    BMQTST_ASSERT_EQ(log.config(), tester.config());
    BMQTST_ASSERT_EQ(log.flush(), LogOpResult::e_SUCCESS);

    BMQTST_ASSERT_EQ(log.close(), LogOpResult::e_SUCCESS);
    BMQTST_ASSERT_EQ(log.isOpened(), false);
}

static void test2_fileNotExist()
// ------------------------------------------------------------------------
// FILE NOT EXIST
//
// Concerns:
//   Verify that opening the log without the CREATE_IF_MISSING flag fails
//   if the file does not exist.
//
// Testing:
//   open(...)
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("FILE NOT EXIST");

    Tester                 tester;
    MemoryMappedOnDiskLog& log = tester.log();

    BMQTST_ASSERT_EQ(log.open(Log::e_READ_ONLY),
                     LogOpResult::e_FILE_NOT_EXIST);
    BMQTST_ASSERT_EQ(log.open(0), LogOpResult::e_FILE_NOT_EXIST);
}

static void test3_updateOutstandingNumBytes()
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

    Tester                 tester;
    MemoryMappedOnDiskLog& log = tester.log();
    BSLS_ASSERT_OPT(log.open(Log::e_CREATE_IF_MISSING) ==
                    LogOpResult::e_SUCCESS);
    BSLS_ASSERT_OPT(log.outstandingNumBytes() == 0);

    log.updateOutstandingNumBytes(200);
    BMQTST_ASSERT_EQ(log.outstandingNumBytes(), 200);
    log.updateOutstandingNumBytes(1000);
    BMQTST_ASSERT_EQ(log.outstandingNumBytes(), 1200);
    log.updateOutstandingNumBytes(-700);
    BMQTST_ASSERT_EQ(log.outstandingNumBytes(), 500);

    // Close and re-open the log. 'outstandingNumBytes' should be re-calibrated
    // to 0.
    BSLS_ASSERT_OPT(log.close() == LogOpResult::e_SUCCESS);
    BSLS_ASSERT_OPT(log.open(Log::e_READ_ONLY) == LogOpResult::e_SUCCESS);
    BMQTST_ASSERT_EQ(log.outstandingNumBytes(), 0);

    BSLS_ASSERT_OPT(log.close() == LogOpResult::e_SUCCESS);
}

static void test4_setOutstandingNumBytes()
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

    Tester                 tester;
    MemoryMappedOnDiskLog& log = tester.log();
    BSLS_ASSERT_OPT(log.open(Log::e_CREATE_IF_MISSING) ==
                    LogOpResult::e_SUCCESS);
    BSLS_ASSERT_OPT(log.outstandingNumBytes() == 0);

    log.setOutstandingNumBytes(500);
    BMQTST_ASSERT_EQ(log.outstandingNumBytes(), 500);
    log.setOutstandingNumBytes(2000);
    BMQTST_ASSERT_EQ(log.outstandingNumBytes(), 2000);
    log.setOutstandingNumBytes(666);
    BMQTST_ASSERT_EQ(log.outstandingNumBytes(), 666);

    // Close and re-open the log. 'outstandingNumBytes' should be re-calibrated
    // to 0.
    BSLS_ASSERT_OPT(log.close() == LogOpResult::e_SUCCESS);
    BSLS_ASSERT_OPT(log.open(Log::e_READ_ONLY) == LogOpResult::e_SUCCESS);
    BMQTST_ASSERT_EQ(log.outstandingNumBytes(), 0);

    BSLS_ASSERT_OPT(log.close() == LogOpResult::e_SUCCESS);
}

static void test5_writeRaw()
// ------------------------------------------------------------------------
// WRITE RAW
//
// Concerns:
//   Verify that 'write' works as intended when dealing with void*, and
//   give an example of updating `outstandingNumBytes` when using 'write'.
//
// Testing:
//   write(const void *entry, int offset, int length)
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("WRITE RAW");

    const bsls::Types::Int64 logMaxSize = k_NUM_ENTRIES * k_ENTRY_LENGTH +
                                          k_LONG_ENTRY_LENGTH + 10;
    Tester                 tester(logMaxSize);
    MemoryMappedOnDiskLog& log = tester.log();
    BSLS_ASSERT_OPT(log.open(Log::e_CREATE_IF_MISSING) ==
                    LogOpResult::e_SUCCESS);

    // 1. Write a list of entries
    for (int i = 0; i < k_NUM_ENTRIES; ++i) {
        BMQTST_ASSERT_EQ(log.write(k_ENTRIES[i], 0, k_ENTRY_LENGTH),
                         static_cast<Offset>(i * k_ENTRY_LENGTH));
        BMQTST_ASSERT_EQ(log.totalNumBytes(), (i + 1) * k_ENTRY_LENGTH);
        BMQTST_ASSERT_EQ(log.outstandingNumBytes(), (i + 1) * k_ENTRY_LENGTH);
        BMQTST_ASSERT_EQ(log.currentOffset(),
                         static_cast<Offset>((i + 1) * k_ENTRY_LENGTH));
    }

    // 2. Set `outstandingNumBytes` to zero, indicating that all entries are no
    //    longer outstanding
    log.setOutstandingNumBytes(0);
    BSLS_ASSERT_OPT(log.outstandingNumBytes() == 0);

    // 3. Write a long entry
    bsls::Types::Int64 currNumBytes = log.totalNumBytes();
    BMQTST_ASSERT_EQ(
        log.write(k_LONG_ENTRY, k_LONG_ENTRY_OFFSET, k_LONG_ENTRY_LENGTH),
        static_cast<Offset>(currNumBytes));

    currNumBytes += k_LONG_ENTRY_LENGTH;
    BMQTST_ASSERT_EQ(log.totalNumBytes(), currNumBytes);
    BMQTST_ASSERT_EQ(log.outstandingNumBytes(), k_LONG_ENTRY_LENGTH);
    BMQTST_ASSERT_EQ(log.currentOffset(), static_cast<Offset>(currNumBytes));

    // 4. Write another long entry. This should fail due to exceeding max size.
    BMQTST_ASSERT_EQ(
        log.write(k_LONG_ENTRY, k_LONG_ENTRY_OFFSET, k_LONG_ENTRY_LENGTH),
        LogOpResult::e_REACHED_END_OF_LOG);

    // 5. Close and re-open the log.  'currentOffset', 'totalNumBytes' and
    //    'outstandingNumBytes' must be re-calibrated.
    log.setOutstandingNumBytes(0);
    BSLS_ASSERT_OPT(log.flush() == LogOpResult::e_SUCCESS);
    BSLS_ASSERT_OPT(log.close() == LogOpResult::e_SUCCESS);

    BSLS_ASSERT_OPT(log.open(Log::e_READ_ONLY) == LogOpResult::e_SUCCESS);
    BMQTST_ASSERT_EQ(log.currentOffset(), currNumBytes);
    BMQTST_ASSERT_EQ(log.totalNumBytes(), currNumBytes);
    BMQTST_ASSERT_EQ(log.outstandingNumBytes(), currNumBytes);

    BSLS_ASSERT_OPT(log.close() == LogOpResult::e_SUCCESS);
}

static void test6_writeBlob()
// ------------------------------------------------------------------------
// WRITE BLOB
//
// Concerns:
//   Verify that 'write' works as intended when dealing with blobs, and
//   give an example of updating `outstandingNumBytes` when using 'write'.
//
// Testing:
//   write(const bdlbb::Blob&        entry,
//         const bmqu::BlobPosition& offset,
//         int                       length)
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("WRITE BLOB");

    const bsls::Types::Int64 logMaxSize = k_NUM_ENTRIES * k_ENTRY_LENGTH +
                                          k_LONG_ENTRY_LENGTH + 10;
    Tester                 tester(logMaxSize);
    MemoryMappedOnDiskLog& log = tester.log();
    BSLS_ASSERT_OPT(log.open(Log::e_CREATE_IF_MISSING) ==
                    LogOpResult::e_SUCCESS);

    // 1. Write a list of entries
    bdlbb::Blob blob(g_miniBufferFactory_p,
                     bmqtst::TestHelperUtil::allocator());
    for (int i = 0; i < k_NUM_ENTRIES; ++i) {
        bdlbb::BlobUtil::append(&blob, k_ENTRIES[i], k_ENTRY_LENGTH);

        bmqu::BlobPosition pos(i, 0);
        BMQTST_ASSERT_EQ(log.write(blob, pos, k_ENTRY_LENGTH),
                         static_cast<Offset>(i * k_ENTRY_LENGTH));
        BMQTST_ASSERT_EQ(log.totalNumBytes(), (i + 1) * k_ENTRY_LENGTH);
        BMQTST_ASSERT_EQ(log.outstandingNumBytes(), (i + 1) * k_ENTRY_LENGTH);
        BMQTST_ASSERT_EQ(log.currentOffset(),
                         static_cast<Offset>((i + 1) * k_ENTRY_LENGTH));
    }
    blob.removeAll();

    // 2. Set `outstandingNumBytes` to zero, indicating that all entries are no
    //    longer outstanding
    log.setOutstandingNumBytes(0);
    BSLS_ASSERT_OPT(log.outstandingNumBytes() == 0);

    // 3. Write a long entry
    bsls::Types::Int64 currNumBytes = log.totalNumBytes();

    bdlbb::Blob blob2(g_bufferFactory_p, bmqtst::TestHelperUtil::allocator());
    bdlbb::BlobUtil::append(&blob2, k_LONG_ENTRY, k_LONG_ENTRY_FULL_LENGTH);
    BMQTST_ASSERT_EQ(log.write(blob2,
                               bmqu::BlobPosition(0, k_LONG_ENTRY_OFFSET),
                               k_LONG_ENTRY_LENGTH),
                     static_cast<Offset>(currNumBytes));
    currNumBytes += k_LONG_ENTRY_LENGTH;
    BMQTST_ASSERT_EQ(log.totalNumBytes(), currNumBytes);
    BMQTST_ASSERT_EQ(log.outstandingNumBytes(), k_LONG_ENTRY_LENGTH);
    BMQTST_ASSERT_EQ(log.currentOffset(), static_cast<Offset>(currNumBytes));

    // 4. Write another long entry. This should fail due to exceeding max size.
    BMQTST_ASSERT_EQ(log.write(blob2,
                               bmqu::BlobPosition(0, k_LONG_ENTRY_OFFSET),
                               k_LONG_ENTRY_LENGTH),
                     LogOpResult::e_REACHED_END_OF_LOG);

    // 5. Close and re-open the log.  'currentOffset', 'totalNumBytes' and
    //    'outstandingNumBytes' must be re-calibrated.
    log.setOutstandingNumBytes(0);
    BSLS_ASSERT_OPT(log.flush() == LogOpResult::e_SUCCESS);
    BSLS_ASSERT_OPT(log.close() == LogOpResult::e_SUCCESS);

    BSLS_ASSERT_OPT(log.open(Log::e_READ_ONLY) == LogOpResult::e_SUCCESS);
    BMQTST_ASSERT_EQ(log.currentOffset(), currNumBytes);
    BMQTST_ASSERT_EQ(log.totalNumBytes(), currNumBytes);
    BMQTST_ASSERT_EQ(log.outstandingNumBytes(), currNumBytes);

    BSLS_ASSERT_OPT(log.close() == LogOpResult::e_SUCCESS);
}

static void test7_writeBlobSection()
// ------------------------------------------------------------------------
// WRITE BLOB SECTION
//
// Concerns:
//   Verify that 'write' works as intended when dealing with blob sections,
//   , and give an example of updating `outstandingNumBytes` when using
//   'write'.
//
// Testing:
//   write(const bdlbb::Blob& entry, const bmqu::BlobSection& section)
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("WRITE BLOB SECTION");

    const bsls::Types::Int64 logMaxSize = k_NUM_ENTRIES * k_ENTRY_LENGTH +
                                          k_LONG_ENTRY_LENGTH + 10;
    Tester                 tester(logMaxSize);
    MemoryMappedOnDiskLog& log = tester.log();
    BSLS_ASSERT_OPT(log.open(Log::e_CREATE_IF_MISSING) ==
                    LogOpResult::e_SUCCESS);

    // 1. Write a list of entries
    bdlbb::Blob blob(g_miniBufferFactory_p,
                     bmqtst::TestHelperUtil::allocator());
    for (int i = 0; i < k_NUM_ENTRIES; ++i) {
        bdlbb::BlobUtil::append(&blob, k_ENTRIES[i], k_ENTRY_LENGTH);

        bmqu::BlobPosition start(i, 0);
        bmqu::BlobPosition end(i + 1, 0);
        bmqu::BlobSection  section(start, end);
        BMQTST_ASSERT_EQ(log.write(blob, section),
                         static_cast<Offset>(i * k_ENTRY_LENGTH));
        BMQTST_ASSERT_EQ(log.totalNumBytes(), (i + 1) * k_ENTRY_LENGTH);
        BMQTST_ASSERT_EQ(log.outstandingNumBytes(), (i + 1) * k_ENTRY_LENGTH);
        BMQTST_ASSERT_EQ(log.currentOffset(),
                         static_cast<Offset>((i + 1) * k_ENTRY_LENGTH));
    }
    blob.removeAll();

    // 2. Set `outstandingNumBytes` to zero, indicating that all entries are no
    //    longer outstanding
    log.setOutstandingNumBytes(0);
    BSLS_ASSERT_OPT(log.outstandingNumBytes() == 0);

    // 3. Write a long entry
    bsls::Types::Int64 currNumBytes = log.totalNumBytes();

    bdlbb::Blob blob2(g_bufferFactory_p, bmqtst::TestHelperUtil::allocator());
    bdlbb::BlobUtil::append(&blob2, k_LONG_ENTRY, k_LONG_ENTRY_FULL_LENGTH);

    bmqu::BlobPosition start(0, k_LONG_ENTRY_OFFSET);
    bmqu::BlobPosition end(0, k_LONG_ENTRY_OFFSET + k_LONG_ENTRY_LENGTH);
    bmqu::BlobSection  section(start, end);
    BMQTST_ASSERT_EQ(log.write(blob2, section),
                     static_cast<Offset>(currNumBytes));
    currNumBytes += k_LONG_ENTRY_LENGTH;
    BMQTST_ASSERT_EQ(log.totalNumBytes(), currNumBytes);
    BMQTST_ASSERT_EQ(log.outstandingNumBytes(), k_LONG_ENTRY_LENGTH);
    BMQTST_ASSERT_EQ(log.currentOffset(), static_cast<Offset>(currNumBytes));

    // 4. Write another long entry. This should fail due to exceeding max size.
    BMQTST_ASSERT_EQ(log.write(blob2, section),
                     LogOpResult::e_REACHED_END_OF_LOG);

    // 5. Close and re-open the log.  'currentOffset', 'totalNumBytes' and
    //    'outstandingNumBytes' must be re-calibrated.
    log.setOutstandingNumBytes(0);
    BSLS_ASSERT_OPT(log.flush() == LogOpResult::e_SUCCESS);
    BSLS_ASSERT_OPT(log.close() == LogOpResult::e_SUCCESS);

    BSLS_ASSERT_OPT(log.open(Log::e_READ_ONLY) == LogOpResult::e_SUCCESS);
    BMQTST_ASSERT_EQ(log.currentOffset(), currNumBytes);
    BMQTST_ASSERT_EQ(log.totalNumBytes(), currNumBytes);
    BMQTST_ASSERT_EQ(log.outstandingNumBytes(), currNumBytes);

    BSLS_ASSERT_OPT(log.close() == LogOpResult::e_SUCCESS);
}

static void test8_readRaw()
// ------------------------------------------------------------------------
// READ RAW
//
// Concerns:
//   Verify that 'read' works as intended when dealing with void*.
//
// Testing:
//   read(void *entry, int length, Offset offset)
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("READ RAW");

    Tester                 tester;
    MemoryMappedOnDiskLog& log = tester.log();
    BSLS_ASSERT_OPT(log.open(Log::e_CREATE_IF_MISSING) ==
                    LogOpResult::e_SUCCESS);

    // 1. Write a list of entries
    for (int i = 0; i < k_NUM_ENTRIES; ++i) {
        BSLS_ASSERT_OPT(log.write(k_ENTRIES[i], 0, k_ENTRY_LENGTH) ==
                        static_cast<Offset>(i * k_ENTRY_LENGTH));
    }

    // 2. Read each entry in the list of entries
    char entry[k_LONG_ENTRY_LENGTH];
    for (int i = 0; i < k_NUM_ENTRIES; ++i) {
        BMQTST_ASSERT_EQ(log.read(static_cast<void*>(entry),
                                  k_ENTRY_LENGTH,
                                  i * k_ENTRY_LENGTH),
                         LogOpResult::e_SUCCESS);
        BMQTST_ASSERT_EQ(bsl::memcmp(entry, k_ENTRIES[i], k_ENTRY_LENGTH), 0);
    }

    // 3. Close and re-open the log
    BSLS_ASSERT_OPT(log.close() == LogOpResult::e_SUCCESS);
    BSLS_ASSERT_OPT(log.open(Log::e_CREATE_IF_MISSING) ==
                    LogOpResult::e_SUCCESS);

    // 4. Write a long entry
    bdlbb::Blob blob(g_bufferFactory_p, bmqtst::TestHelperUtil::allocator());
    bdlbb::BlobUtil::append(&blob, k_LONG_ENTRY, k_LONG_ENTRY_FULL_LENGTH);
    BSLS_ASSERT_OPT(log.write(blob,
                              bmqu::BlobPosition(0, k_LONG_ENTRY_OFFSET),
                              k_LONG_ENTRY_LENGTH) ==
                    static_cast<Offset>(k_NUM_ENTRIES * k_ENTRY_LENGTH));

    // 5. Re-read the list of entries, then read the long entry
    for (int i = 0; i < k_NUM_ENTRIES; ++i) {
        BMQTST_ASSERT_EQ(log.read(static_cast<void*>(entry),
                                  k_ENTRY_LENGTH,
                                  i * k_ENTRY_LENGTH),
                         LogOpResult::e_SUCCESS);
        BMQTST_ASSERT_EQ(bsl::memcmp(entry, k_ENTRIES[i], k_ENTRY_LENGTH), 0);
    }

    BMQTST_ASSERT_EQ(log.read(static_cast<void*>(entry),
                              k_LONG_ENTRY_LENGTH,
                              k_NUM_ENTRIES * k_ENTRY_LENGTH),
                     LogOpResult::e_SUCCESS);
    BMQTST_ASSERT_EQ(
        bsl::memcmp(entry, k_LONG_ENTRY_MEAT, k_LONG_ENTRY_LENGTH),
        0);

    // 6. Write another long entry
    const Offset currOffset = static_cast<Offset>(
        k_NUM_ENTRIES * k_ENTRY_LENGTH + k_LONG_ENTRY_LENGTH);

    bdlbb::Blob blob2(g_bufferFactory_p, bmqtst::TestHelperUtil::allocator());
    bdlbb::BlobUtil::append(&blob2, k_LONG_ENTRY2, k_LONG_ENTRY2_FULL_LENGTH);

    bmqu::BlobPosition start(0, k_LONG_ENTRY2_OFFSET);
    bmqu::BlobPosition end(0, k_LONG_ENTRY2_OFFSET + k_LONG_ENTRY2_LENGTH);
    bmqu::BlobSection  section(start, end);
    BSLS_ASSERT_OPT(log.write(blob2, section) == currOffset);

    // 7. Read the other long entry
    BMQTST_ASSERT_EQ(
        log.read(static_cast<void*>(entry), k_LONG_ENTRY2_LENGTH, currOffset),
        LogOpResult::e_SUCCESS);
    BMQTST_ASSERT_EQ(memcmp(entry, k_LONG_ENTRY2_MEAT, k_LONG_ENTRY2_LENGTH),
                     0);

    // 8. Read beyond the last record offset should fail
    BMQTST_ASSERT_EQ(log.read(static_cast<void*>(entry), k_ENTRY_LENGTH, 9999),
                     LogOpResult::e_OFFSET_OUT_OF_RANGE);

    // 9. Read beyond the length of the log should fail
    BMQTST_ASSERT_EQ(log.read(static_cast<void*>(entry), 9999, 0),
                     LogOpResult::e_REACHED_END_OF_LOG);

    BSLS_ASSERT_OPT(log.flush() == LogOpResult::e_SUCCESS);
    BSLS_ASSERT_OPT(log.close() == LogOpResult::e_SUCCESS);
}

static void test9_readBlob()
// ------------------------------------------------------------------------
// READ BLOB
//
// Concerns:
//   Verify that 'read' works as intended when dealing with blobs.
//
// Testing:
//   read(bdlbb::Blob *entry, int length, Offset offset)
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("READ BLOB");

    Tester                 tester;
    MemoryMappedOnDiskLog& log = tester.log();
    BSLS_ASSERT_OPT(log.open(Log::e_CREATE_IF_MISSING) ==
                    LogOpResult::e_SUCCESS);

    // 1. Write a list of entries
    for (int i = 0; i < k_NUM_ENTRIES; ++i) {
        BSLS_ASSERT_OPT(log.write(k_ENTRIES[i], 0, k_ENTRY_LENGTH) ==
                        static_cast<Offset>(i * k_ENTRY_LENGTH));
    }

    // 2. Read each entry in the list of entries
    bdlbb::Blob blob(g_miniBufferFactory_p,
                     bmqtst::TestHelperUtil::allocator());

    char entry[k_LONG_ENTRY_LENGTH];
    for (int i = 0; i < k_NUM_ENTRIES; ++i) {
        BMQTST_ASSERT_EQ(log.read(&blob, k_ENTRY_LENGTH, i * k_ENTRY_LENGTH),
                         LogOpResult::e_SUCCESS);
        bmqu::BlobUtil::readNBytes(entry,
                                   blob,
                                   bmqu::BlobPosition(),
                                   k_ENTRY_LENGTH);
        BMQTST_ASSERT_EQ(bsl::memcmp(entry, k_ENTRIES[i], k_ENTRY_LENGTH), 0);

        blob.removeBuffer(0);
    }

    // 3. Close and re-open the log
    BSLS_ASSERT_OPT(log.close() == LogOpResult::e_SUCCESS);
    BSLS_ASSERT_OPT(log.open(Log::e_CREATE_IF_MISSING) ==
                    LogOpResult::e_SUCCESS);

    // 4. Write a long entry
    bdlbb::Blob blob2(g_bufferFactory_p, bmqtst::TestHelperUtil::allocator());
    bdlbb::BlobUtil::append(&blob2, k_LONG_ENTRY, k_LONG_ENTRY_FULL_LENGTH);
    BSLS_ASSERT_OPT(log.write(blob2,
                              bmqu::BlobPosition(0, k_LONG_ENTRY_OFFSET),
                              k_LONG_ENTRY_LENGTH) ==
                    static_cast<Offset>(k_NUM_ENTRIES * k_ENTRY_LENGTH));

    // 5. Re-read the list of entries, then read the long entry
    for (int i = 0; i < k_NUM_ENTRIES; ++i) {
        BMQTST_ASSERT_EQ(log.read(&blob, k_ENTRY_LENGTH, i * k_ENTRY_LENGTH),
                         LogOpResult::e_SUCCESS);
        bmqu::BlobUtil::readNBytes(entry,
                                   blob,
                                   bmqu::BlobPosition(),
                                   k_ENTRY_LENGTH);
        BMQTST_ASSERT_EQ(bsl::memcmp(entry, k_ENTRIES[i], k_ENTRY_LENGTH), 0);

        blob.removeBuffer(0);
    }

    BMQTST_ASSERT_EQ(
        log.read(&blob, k_LONG_ENTRY_LENGTH, k_NUM_ENTRIES * k_ENTRY_LENGTH),
        LogOpResult::e_SUCCESS);
    bmqu::BlobUtil::readNBytes(entry,
                               blob,
                               bmqu::BlobPosition(),
                               k_LONG_ENTRY_LENGTH);
    BMQTST_ASSERT_EQ(
        bsl::memcmp(entry, k_LONG_ENTRY_MEAT, k_LONG_ENTRY_LENGTH),
        0);
    blob.removeAll();

    // 6. Write another long entry
    const Offset currOffset = static_cast<Offset>(
        k_NUM_ENTRIES * k_ENTRY_LENGTH + k_LONG_ENTRY_LENGTH);

    bdlbb::Blob blob3(g_bufferFactory_p, bmqtst::TestHelperUtil::allocator());
    bdlbb::BlobUtil::append(&blob3, k_LONG_ENTRY2, k_LONG_ENTRY2_FULL_LENGTH);

    bmqu::BlobPosition start(0, k_LONG_ENTRY2_OFFSET);
    bmqu::BlobPosition end(0, k_LONG_ENTRY2_OFFSET + k_LONG_ENTRY2_LENGTH);
    bmqu::BlobSection  section(start, end);
    BSLS_ASSERT_OPT(log.write(blob3, section) == currOffset);

    // 7. Read the other long entry
    BMQTST_ASSERT_EQ(log.read(&blob, k_LONG_ENTRY2_LENGTH, currOffset),
                     LogOpResult::e_SUCCESS);
    bmqu::BlobUtil::readNBytes(entry,
                               blob,
                               bmqu::BlobPosition(),
                               k_LONG_ENTRY2_LENGTH);
    BMQTST_ASSERT_EQ(
        bsl::memcmp(entry, k_LONG_ENTRY2_MEAT, k_LONG_ENTRY2_LENGTH),
        0);
    blob.removeAll();

    // 8. Read beyond the last record offset should fail
    BMQTST_ASSERT_EQ(log.read(&blob, k_ENTRY_LENGTH, 9999),
                     LogOpResult::e_OFFSET_OUT_OF_RANGE);

    // 9. Read beyond the length of the log should fail
    BMQTST_ASSERT_EQ(log.read(&blob, 9999, 0),
                     LogOpResult::e_REACHED_END_OF_LOG);

    BSLS_ASSERT_OPT(log.flush() == LogOpResult::e_SUCCESS);
    BSLS_ASSERT_OPT(log.close() == LogOpResult::e_SUCCESS);
}

static void test10_aliasRaw()
// ------------------------------------------------------------------------
// ALIAS RAW
//
// Concerns:
//   Verify that 'alias' works as intended when dealing with void*.
//
// Testing:
//   alias(void **entry, int length, Offset offset)
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("ALIAS RAW");

    Tester                 tester;
    MemoryMappedOnDiskLog& log = tester.log();
    BSLS_ASSERT_OPT(log.open(Log::e_CREATE_IF_MISSING) ==
                    LogOpResult::e_SUCCESS);

    // 1. Write a list of entries
    for (int i = 0; i < k_NUM_ENTRIES; ++i) {
        BSLS_ASSERT_OPT(log.write(k_ENTRIES[i], 0, k_ENTRY_LENGTH) ==
                        static_cast<Offset>(i * k_ENTRY_LENGTH));
    }

    // 2. Alias each entry in the list of entries
    char* entry;
    for (int i = 0; i < k_NUM_ENTRIES; ++i) {
        BMQTST_ASSERT_EQ(log.alias(reinterpret_cast<void**>(&entry),
                                   k_ENTRY_LENGTH,
                                   i * k_ENTRY_LENGTH),
                         LogOpResult::e_SUCCESS);
        BMQTST_ASSERT_EQ(bsl::memcmp(entry, k_ENTRIES[i], k_ENTRY_LENGTH), 0);
    }

    // 3. Close and re-open the log
    BSLS_ASSERT_OPT(log.close() == LogOpResult::e_SUCCESS);
    BSLS_ASSERT_OPT(log.open(Log::e_CREATE_IF_MISSING) ==
                    LogOpResult::e_SUCCESS);

    // 4. Write a long entry
    bdlbb::Blob blob(g_bufferFactory_p, bmqtst::TestHelperUtil::allocator());
    bdlbb::BlobUtil::append(&blob, k_LONG_ENTRY, k_LONG_ENTRY_FULL_LENGTH);
    BSLS_ASSERT_OPT(log.write(blob,
                              bmqu::BlobPosition(0, k_LONG_ENTRY_OFFSET),
                              k_LONG_ENTRY_LENGTH) ==
                    static_cast<Offset>(k_NUM_ENTRIES * k_ENTRY_LENGTH));

    // 5. Re-alias the list of entries, then alias the long entry
    for (int i = 0; i < k_NUM_ENTRIES; ++i) {
        BMQTST_ASSERT_EQ(log.alias(reinterpret_cast<void**>(&entry),
                                   k_ENTRY_LENGTH,
                                   i * k_ENTRY_LENGTH),
                         LogOpResult::e_SUCCESS);
        BMQTST_ASSERT_EQ(bsl::memcmp(entry, k_ENTRIES[i], k_ENTRY_LENGTH), 0);
    }

    BMQTST_ASSERT_EQ(log.alias(reinterpret_cast<void**>(&entry),
                               k_LONG_ENTRY_LENGTH,
                               k_NUM_ENTRIES * k_ENTRY_LENGTH),
                     LogOpResult::e_SUCCESS);
    BMQTST_ASSERT_EQ(
        bsl::memcmp(entry, k_LONG_ENTRY_MEAT, k_LONG_ENTRY_LENGTH),
        0);

    // 6. Write another long entry
    const Offset currOffset = static_cast<Offset>(
        k_NUM_ENTRIES * k_ENTRY_LENGTH + k_LONG_ENTRY_LENGTH);

    bdlbb::Blob blob2(g_bufferFactory_p, bmqtst::TestHelperUtil::allocator());
    bdlbb::BlobUtil::append(&blob2, k_LONG_ENTRY2, k_LONG_ENTRY2_FULL_LENGTH);

    bmqu::BlobPosition start(0, k_LONG_ENTRY2_OFFSET);
    bmqu::BlobPosition end(0, k_LONG_ENTRY2_OFFSET + k_LONG_ENTRY2_LENGTH);
    bmqu::BlobSection  section(start, end);
    BSLS_ASSERT_OPT(log.write(blob2, section) == currOffset);

    // 7. Alias the other entry
    BMQTST_ASSERT_EQ(log.alias(reinterpret_cast<void**>(&entry),
                               k_LONG_ENTRY2_LENGTH,
                               currOffset),
                     LogOpResult::e_SUCCESS);
    BMQTST_ASSERT_EQ(
        bsl::memcmp(entry, k_LONG_ENTRY2_MEAT, k_LONG_ENTRY2_LENGTH),
        0);

    // 8. Alias beyond the last record offset should fail
    BMQTST_ASSERT_EQ(
        log.alias(reinterpret_cast<void**>(&entry), k_ENTRY_LENGTH, 9999),
        LogOpResult::e_OFFSET_OUT_OF_RANGE);

    // 9. Alias beyond the length of the log should fail
    BMQTST_ASSERT_EQ(log.alias(reinterpret_cast<void**>(&entry), 9999, 0),
                     LogOpResult::e_REACHED_END_OF_LOG);

    BSLS_ASSERT_OPT(log.flush() == LogOpResult::e_SUCCESS);
    BSLS_ASSERT_OPT(log.close() == LogOpResult::e_SUCCESS);
}

static void test11_aliasBlob()
// ------------------------------------------------------------------------
// ALIAS BLOB
//
// Concerns:
//   Verify that 'alias' works as intended when dealing with blobs.
//
// Testing:
//   alias(bdlbb::Blob *entry, int length, Offset offset)
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("ALIAS BLOB");

    Tester                 tester;
    MemoryMappedOnDiskLog& log = tester.log();
    BSLS_ASSERT_OPT(log.open(Log::e_CREATE_IF_MISSING) ==
                    LogOpResult::e_SUCCESS);

    // 1. Write a list of entries
    for (int i = 0; i < k_NUM_ENTRIES; ++i) {
        BSLS_ASSERT_OPT(log.write(k_ENTRIES[i], 0, k_ENTRY_LENGTH) ==
                        static_cast<Offset>(i * k_ENTRY_LENGTH));
    }

    // 2. Alias each entry in the list of entries
    bdlbb::Blob blob(g_bufferFactory_p, bmqtst::TestHelperUtil::allocator());

    char entry[k_LONG_ENTRY_LENGTH];
    for (int i = 0; i < k_NUM_ENTRIES; ++i) {
        BMQTST_ASSERT_EQ(log.alias(&blob, k_ENTRY_LENGTH, i * k_ENTRY_LENGTH),
                         LogOpResult::e_SUCCESS);
        bmqu::BlobUtil::readNBytes(entry,
                                   blob,
                                   bmqu::BlobPosition(),
                                   k_ENTRY_LENGTH);
        BMQTST_ASSERT_EQ(bsl::memcmp(entry, k_ENTRIES[i], k_ENTRY_LENGTH), 0);

        blob.removeBuffer(0);
    }

    // 3. Close and re-open the log
    BSLS_ASSERT_OPT(log.close() == LogOpResult::e_SUCCESS);
    BSLS_ASSERT_OPT(log.open(Log::e_CREATE_IF_MISSING) ==
                    LogOpResult::e_SUCCESS);

    // 4. Write a long entry
    bdlbb::Blob blob2(g_bufferFactory_p, bmqtst::TestHelperUtil::allocator());
    bdlbb::BlobUtil::append(&blob2, k_LONG_ENTRY, k_LONG_ENTRY_FULL_LENGTH);
    BSLS_ASSERT_OPT(log.write(blob2,
                              bmqu::BlobPosition(0, k_LONG_ENTRY_OFFSET),
                              k_LONG_ENTRY_LENGTH) ==
                    static_cast<Offset>(k_NUM_ENTRIES * k_ENTRY_LENGTH));

    // 5. Re-alias the list of entries, then alias the long entry
    for (int i = 0; i < k_NUM_ENTRIES; ++i) {
        BMQTST_ASSERT_EQ(log.alias(&blob, k_ENTRY_LENGTH, i * k_ENTRY_LENGTH),
                         LogOpResult::e_SUCCESS);
        bmqu::BlobUtil::readNBytes(entry,
                                   blob,
                                   bmqu::BlobPosition(),
                                   k_ENTRY_LENGTH);
        BMQTST_ASSERT_EQ(bsl::memcmp(entry, k_ENTRIES[i], k_ENTRY_LENGTH), 0);

        blob.removeBuffer(0);
    }

    BMQTST_ASSERT_EQ(
        log.alias(&blob, k_LONG_ENTRY_LENGTH, k_NUM_ENTRIES * k_ENTRY_LENGTH),
        LogOpResult::e_SUCCESS);
    bmqu::BlobUtil::readNBytes(entry,
                               blob,
                               bmqu::BlobPosition(),
                               k_LONG_ENTRY_LENGTH);
    BMQTST_ASSERT_EQ(
        bsl::memcmp(entry, k_LONG_ENTRY_MEAT, k_LONG_ENTRY_LENGTH),
        0);
    blob.removeBuffer(0);

    // 6. Write another long entry
    const Offset currOffset = static_cast<Offset>(
        k_NUM_ENTRIES * k_ENTRY_LENGTH + k_LONG_ENTRY_LENGTH);

    bdlbb::Blob blob3(g_bufferFactory_p, bmqtst::TestHelperUtil::allocator());
    bdlbb::BlobUtil::append(&blob3, k_LONG_ENTRY2, k_LONG_ENTRY2_FULL_LENGTH);

    bmqu::BlobPosition start(0, k_LONG_ENTRY2_OFFSET);
    bmqu::BlobPosition end(0, k_LONG_ENTRY2_OFFSET + k_LONG_ENTRY2_LENGTH);
    bmqu::BlobSection  section(start, end);
    BSLS_ASSERT_OPT(log.write(blob3, section) == currOffset);

    // 7. Alias the other entry
    BMQTST_ASSERT_EQ(log.alias(&blob, k_LONG_ENTRY2_LENGTH, currOffset),
                     LogOpResult::e_SUCCESS);
    bmqu::BlobUtil::readNBytes(entry,
                               blob,
                               bmqu::BlobPosition(),
                               k_LONG_ENTRY2_LENGTH);
    BMQTST_ASSERT_EQ(
        bsl::memcmp(entry, k_LONG_ENTRY2_MEAT, k_LONG_ENTRY2_LENGTH),
        0);
    blob.removeBuffer(0);

    // 8. Alias beyond the last record offset should fail
    BMQTST_ASSERT_EQ(log.alias(&blob, k_ENTRY_LENGTH, 9999),
                     LogOpResult::e_OFFSET_OUT_OF_RANGE);

    // 9. Alias beyond the length of the log should fail
    BMQTST_ASSERT_EQ(log.alias(&blob, 9999, 0),
                     LogOpResult::e_REACHED_END_OF_LOG);

    BSLS_ASSERT_OPT(log.flush() == LogOpResult::e_SUCCESS);
    BSLS_ASSERT_OPT(log.close() == LogOpResult::e_SUCCESS);
}

static void test12_seek()
// ------------------------------------------------------------------------
// SEEK
//
// Concerns:
//   Verify that 'seek' works as intended, and demonstrate how
//   `outstandingNumBytes` should be updated when using 'seek' and 'write'.
//
// Testing:
//   seek(...)
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("SEEK");

    Tester                 tester;
    MemoryMappedOnDiskLog& log = tester.log();
    BSLS_ASSERT_OPT(log.open(Log::e_CREATE_IF_MISSING) ==
                    LogOpResult::e_SUCCESS);
    BSLS_ASSERT_OPT(log.currentOffset() == static_cast<Offset>(0));
    // Expected values
    bsls::Types::Int64 expTotalNumBytes       = 0;
    bsls::Types::Int64 expOutstandingNumBytes = 0;
    BSLS_ASSERT_OPT(log.totalNumBytes() == expTotalNumBytes);
    BSLS_ASSERT_OPT(log.outstandingNumBytes() == expOutstandingNumBytes);

    // 1. Write a list of entries
    for (int i = 0; i < k_NUM_ENTRIES; ++i) {
        BSLS_ASSERT_OPT(log.write(k_ENTRIES[i], 0, k_ENTRY_LENGTH) ==
                        static_cast<Offset>(i * k_ENTRY_LENGTH));
    }
    BSLS_ASSERT_OPT(log.currentOffset() ==
                    static_cast<Offset>(k_NUM_ENTRIES * k_ENTRY_LENGTH));
    expTotalNumBytes += k_NUM_ENTRIES * k_ENTRY_LENGTH;
    expOutstandingNumBytes += k_NUM_ENTRIES * k_ENTRY_LENGTH;
    BSLS_ASSERT_OPT(log.totalNumBytes() == expTotalNumBytes);
    BSLS_ASSERT_OPT(log.outstandingNumBytes() == expOutstandingNumBytes);

    // 2. Seek to a position in the middle
    const Offset midpoint = static_cast<Offset>(k_NUM_ENTRIES *
                                                k_ENTRY_LENGTH / 2);
    BMQTST_ASSERT_EQ(log.seek(midpoint), LogOpResult::e_SUCCESS);
    BMQTST_ASSERT_EQ(log.currentOffset(), midpoint);

    // 3. Read the entry at that position and the next one (since they will be
    //    overwritten), then subtract `outstandingNumBytes by their total
    //    length
    char* entry = static_cast<char*>(
        bmqtst::TestHelperUtil::allocator()->allocate(k_LONG_ENTRY_LENGTH));
    BSLS_ASSERT_OPT(log.read(static_cast<void*>(entry),
                             k_ENTRY_LENGTH,
                             midpoint) == LogOpResult::e_SUCCESS);
    BSLS_ASSERT_OPT(
        bsl::memcmp(entry, k_ENTRIES[k_NUM_ENTRIES / 2], k_ENTRY_LENGTH) == 0);

    BSLS_ASSERT_OPT(log.read(static_cast<void*>(entry),
                             k_ENTRY_LENGTH,
                             midpoint + k_ENTRY_LENGTH) ==
                    LogOpResult::e_SUCCESS);
    BSLS_ASSERT_OPT(bsl::memcmp(entry,
                                k_ENTRIES[k_NUM_ENTRIES / 2 + 1],
                                k_ENTRY_LENGTH) == 0);

    log.updateOutstandingNumBytes(-2 * k_ENTRY_LENGTH);
    expOutstandingNumBytes -= 2 * k_ENTRY_LENGTH;
    BSLS_ASSERT_OPT(log.totalNumBytes() == expTotalNumBytes);
    BSLS_ASSERT_OPT(log.outstandingNumBytes() == expOutstandingNumBytes);

    // 4. Overwrite that entry with a long entry, then read the new entry
    BMQTST_ASSERT_EQ(
        log.write(k_LONG_ENTRY, k_LONG_ENTRY_OFFSET, k_LONG_ENTRY_LENGTH),
        midpoint);
    BSLS_ASSERT_OPT(log.currentOffset() == midpoint + k_LONG_ENTRY_LENGTH);
    expOutstandingNumBytes += k_LONG_ENTRY_LENGTH;
    // Expected total num bytes remains unchanged
    BMQTST_ASSERT_EQ(log.totalNumBytes(), expTotalNumBytes);
    BMQTST_ASSERT_EQ(log.outstandingNumBytes(), expOutstandingNumBytes);

    BSLS_ASSERT_OPT(log.read(static_cast<void*>(entry),
                             k_LONG_ENTRY_LENGTH,
                             midpoint) == LogOpResult::e_SUCCESS);
    BMQTST_ASSERT_EQ(
        bsl::memcmp(entry, k_LONG_ENTRY_MEAT, k_LONG_ENTRY_LENGTH),
        0);

    // 5. Seek to the end
    Offset endpoint = static_cast<Offset>(k_NUM_ENTRIES * k_ENTRY_LENGTH);
    BMQTST_ASSERT_EQ(log.seek(endpoint), LogOpResult::e_SUCCESS);
    BMQTST_ASSERT_EQ(log.currentOffset(), endpoint);

    // 6. Write an entry, then read it
    BMQTST_ASSERT_EQ(
        log.write(k_LONG_ENTRY, k_LONG_ENTRY_OFFSET, k_LONG_ENTRY_LENGTH),
        endpoint);
    endpoint += k_LONG_ENTRY_LENGTH;
    BSLS_ASSERT_OPT(log.currentOffset() == endpoint);
    expTotalNumBytes += k_LONG_ENTRY_LENGTH;
    expOutstandingNumBytes += k_LONG_ENTRY_LENGTH;
    BMQTST_ASSERT_EQ(log.totalNumBytes(), expTotalNumBytes);
    BMQTST_ASSERT_EQ(log.outstandingNumBytes(), expOutstandingNumBytes);

    BSLS_ASSERT_OPT(log.read(static_cast<void*>(entry),
                             k_LONG_ENTRY_LENGTH,
                             endpoint - k_LONG_ENTRY_LENGTH) ==
                    LogOpResult::e_SUCCESS);
    BMQTST_ASSERT_EQ(
        bsl::memcmp(entry, k_LONG_ENTRY_MEAT, k_LONG_ENTRY_LENGTH),
        0);

    // 7. Seek to the beginning, then close and re-open the log.
    //    'currentOffset' must be re-calibrated to the end of the log.
    BMQTST_ASSERT_EQ(log.seek(0), LogOpResult::e_SUCCESS);
    BSLS_ASSERT_OPT(log.flush() == LogOpResult::e_SUCCESS);
    BSLS_ASSERT_OPT(log.close() == LogOpResult::e_SUCCESS);

    BSLS_ASSERT_OPT(log.open(Log::e_READ_ONLY) == LogOpResult::e_SUCCESS);
    BMQTST_ASSERT_EQ(log.currentOffset(), endpoint);

    BSLS_ASSERT_OPT(log.close() == LogOpResult::e_SUCCESS);
    bmqtst::TestHelperUtil::allocator()->deallocate(entry);
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    {
        bdlbb::PooledBlobBufferFactory bufferFactory(
            k_LONG_ENTRY_LENGTH * 2,
            bmqtst::TestHelperUtil::allocator());
        bdlbb::PooledBlobBufferFactory miniBufferFactory(
            k_ENTRY_LENGTH,
            bmqtst::TestHelperUtil::allocator());
        g_bufferFactory_p     = &bufferFactory;
        g_miniBufferFactory_p = &miniBufferFactory;

        switch (_testCase) {
        case 0:
        case 1: test1_breathingTest(); break;
        case 2: test2_fileNotExist(); break;
        case 3: test3_updateOutstandingNumBytes(); break;
        case 4: test4_setOutstandingNumBytes(); break;
        case 5: test5_writeRaw(); break;
        case 6: test6_writeBlob(); break;
        case 7: test7_writeBlobSection(); break;
        case 8: test8_readRaw(); break;
        case 9: test9_readBlob(); break;
        case 10: test10_aliasRaw(); break;
        case 11: test11_aliasBlob(); break;
        case 12: test12_seek(); break;
        default: {
            cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
            bmqtst::TestHelperUtil::testStatus() = -1;
        } break;
        }
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_GBL_ALLOC);
}
