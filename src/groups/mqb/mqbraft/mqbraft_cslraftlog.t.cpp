// Copyright 2025-2026 Bloomberg Finance L.P.
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

// mqbraft_cslraftlog.t.cpp -*-C++-*-
#include <mqbraft_cslraftlog.h>

// MQB
#include <mqbc_clusterstateledgerprotocol.h>
#include <mqbc_clusterstateledgerutil.h>
#include <mqbsl_memorymappedondisklog.h>

// BMQ
#include <bmqp_blobpoolutil.h>

#include <bmqu_tempdirectory.h>

// BDE
#include <bdlbb_blob.h>
#include <bdlbb_pooledblobbufferfactory.h>
#include <bsl_iostream.h>
#include <bsl_memory.h>
#include <bslma_testallocator.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

using namespace BloombergLP;
using namespace BloombergLP::mqbraft;
using namespace bsl;

// ============================================================================
//                            TEST HELPERS
// ----------------------------------------------------------------------------
namespace {

const bsls::Types::Int64 k_LOG_MAX_SIZE = 1024 * 1024;  // 1 MB

const mqbu::StorageKey k_LOG_KEY(mqbu::StorageKey::BinaryRepresentation(),
                                 "12345");

/// Build a CSL record blob for a PartitionPrimaryAdvisory with the
/// specified 'term' and 'seqNum'.
bsl::shared_ptr<bdlbb::Blob> makeRecord(bsls::Types::Uint64       term,
                                         bsls::Types::Uint64       seqNum,
                                         bdlbb::BlobBufferFactory* factory,
                                         bslma::Allocator*         allocator)
{
    bmqp_ctrlmsg::ClusterMessage        clusterMessage(allocator);
    bmqp_ctrlmsg::LeaderMessageSequence lms;
    lms.electorTerm()    = term;
    lms.sequenceNumber() = seqNum;

    bmqp_ctrlmsg::PartitionPrimaryAdvisory& advisory =
        clusterMessage.choice().makePartitionPrimaryAdvisory();
    advisory.sequenceNumber() = lms;

    bsl::shared_ptr<bdlbb::Blob> record =
        bsl::make_shared<bdlbb::Blob>(factory, allocator);
    int         rc = mqbc::ClusterStateLedgerUtil::appendRecord(
        record.get(),
        clusterMessage,
        lms,
        0,
        mqbc::ClusterStateRecordType::e_UPDATE,
        allocator);
    BSLS_ASSERT_OPT(rc == 0);
    (void)rc;

    return record;
}

/// Helper class that creates a temp directory, opens a
/// MemoryMappedOnDiskLog, writes a CSL file header, and provides a
/// CslRaftLog for testing.
class Tester {
  private:
    bmqu::TempDirectory            d_tempDir;
    bsl::string                    d_logPath;
    mqbsi::LogConfig               d_logConfig;
    bsl::shared_ptr<mqbsi::Log>    d_log_sp;
    bdlbb::PooledBlobBufferFactory       d_bufferFactory;
    bmqp::BlobPoolUtil::BlobSpPoolSp     d_blobSpPool_sp;
    CslRaftLog                           d_cslRaftLog;
    bslma::Allocator*              d_allocator_p;

    // NOT IMPLEMENTED
    Tester(const Tester&);
    Tester& operator=(const Tester&);

    static bsl::string makePath(const bsl::string& dir,
                                bslma::Allocator*  allocator)
    {
        bsl::string path(dir, allocator);
        path.append("/csl_raft.bmq");
        return path;
    }

  public:
    explicit Tester(
        bslma::Allocator* allocator = bmqtst::TestHelperUtil::allocator())
    : d_tempDir(allocator)
    , d_logPath(makePath(d_tempDir.path(), allocator))
    , d_logConfig(k_LOG_MAX_SIZE, k_LOG_KEY, d_logPath, true, false, allocator)
    , d_log_sp(bsl::allocate_shared<mqbsl::MemoryMappedOnDiskLog>(allocator,
                                                                  d_logConfig,
                                                                  allocator))
    , d_bufferFactory(256, allocator)
    , d_blobSpPool_sp(bmqp::BlobPoolUtil::createBlobPool(&d_bufferFactory,
                                                          allocator))
    , d_cslRaftLog(d_log_sp, d_blobSpPool_sp.get(), allocator)
    , d_allocator_p(allocator)
    {
    }

    int open()
    {
        int rc = d_log_sp->open(mqbsi::Log::e_CREATE_IF_MISSING);
        if (rc != 0) {
            return rc;
        }

        // Write CSL file header
        mqbc::ClusterStateFileHeader fileHeader;
        mqbsi::Log::Offset           offset = d_log_sp->write(
            &fileHeader,
            0,
            sizeof(mqbc::ClusterStateFileHeader));
        if (offset < 0) {
            return static_cast<int>(offset);
        }

        return d_cslRaftLog.open();
    }

    bsl::shared_ptr<bdlbb::Blob> makeUpdateRecord(bsls::Types::Uint64 term,
                                                   bsls::Types::Uint64 seqNum)
    {
        return makeRecord(term, seqNum, &d_bufferFactory, d_allocator_p);
    }

    CslRaftLog&                     raftLog() { return d_cslRaftLog; }
    bsl::shared_ptr<mqbsi::Log>&    log() { return d_log_sp; }
    bdlbb::PooledBlobBufferFactory& factory()  { return d_bufferFactory; }
    CslRaftLog::BlobSpPool*         blobPool() { return d_blobSpPool_sp.get(); }
    bslma::Allocator*               allocator() { return d_allocator_p; }
};

}  // close unnamed namespace

// ============================================================================
//                                    TESTS
// ============================================================================

static void test1_breathingTest()
{
    bmqtst::TestHelper::printTestName("BREATHING TEST");

    Tester tester;
    int    rc = tester.open();
    BMQTST_ASSERT_EQ(rc, 0);

    BMQTST_ASSERT_EQ(tester.raftLog().lastIndex(), 0ULL);
    BMQTST_ASSERT_EQ(tester.raftLog().lastTerm(), 0ULL);
    BMQTST_ASSERT_EQ(tester.raftLog().snapshotIndex(), 0ULL);
    BMQTST_ASSERT_EQ(tester.raftLog().snapshotTerm(), 0ULL);
}

static void test2_appendAndReadBack()
{
    bmqtst::TestHelper::printTestName("APPEND AND READ BACK");

    Tester tester;
    int    rc = tester.open();
    BMQTST_ASSERT_EQ(rc, 0);

    // Append 3 entries with different terms
    bsl::shared_ptr<bdlbb::Blob> rec1 = tester.makeUpdateRecord(1, 1);
    bsl::shared_ptr<bdlbb::Blob> rec2 = tester.makeUpdateRecord(1, 2);
    bsl::shared_ptr<bdlbb::Blob> rec3 = tester.makeUpdateRecord(2, 3);

    BMQTST_ASSERT_EQ(tester.raftLog().append(1, rec1), 0);
    BMQTST_ASSERT_EQ(tester.raftLog().append(1, rec2), 0);
    BMQTST_ASSERT_EQ(tester.raftLog().append(2, rec3), 0);

    BMQTST_ASSERT_EQ(tester.raftLog().lastIndex(), 3ULL);
    BMQTST_ASSERT_EQ(tester.raftLog().lastTerm(), 2ULL);
    BMQTST_ASSERT_EQ(tester.raftLog().term(1), 1ULL);
    BMQTST_ASSERT_EQ(tester.raftLog().term(2), 1ULL);
    BMQTST_ASSERT_EQ(tester.raftLog().term(3), 2ULL);

    // Read entries back
    bsl::vector<LogEntry> entries(tester.allocator());
    rc = tester.raftLog().entries(1, 4, &entries);
    BMQTST_ASSERT_EQ(rc, 0);
    BMQTST_ASSERT_EQ(entries.size(), 3u);
    BMQTST_ASSERT_EQ(entries[0].d_term, 1ULL);
    BMQTST_ASSERT_EQ(entries[1].d_term, 1ULL);
    BMQTST_ASSERT_EQ(entries[2].d_term, 2ULL);
}

static void test3_truncate()
{
    bmqtst::TestHelper::printTestName("TRUNCATE");

    Tester tester;
    int    rc = tester.open();
    BMQTST_ASSERT_EQ(rc, 0);

    // Append 5 entries
    for (bsls::Types::Uint64 i = 1; i <= 5; ++i) {
        bsl::shared_ptr<bdlbb::Blob> rec = tester.makeUpdateRecord(1, i);
        BMQTST_ASSERT_EQ(tester.raftLog().append(1, rec), 0);
    }
    BMQTST_ASSERT_EQ(tester.raftLog().lastIndex(), 5ULL);

    // Truncate from index 3
    rc = tester.raftLog().truncateFrom(3);
    BMQTST_ASSERT_EQ(rc, 0);
    BMQTST_ASSERT_EQ(tester.raftLog().lastIndex(), 2ULL);
    BMQTST_ASSERT_EQ(tester.raftLog().term(1), 1ULL);
    BMQTST_ASSERT_EQ(tester.raftLog().term(2), 1ULL);
    BMQTST_ASSERT_EQ(tester.raftLog().term(3), 0ULL);  // out of range
}

static void test4_appendAfterTruncate()
{
    bmqtst::TestHelper::printTestName("APPEND AFTER TRUNCATE");

    Tester tester;
    int    rc = tester.open();
    BMQTST_ASSERT_EQ(rc, 0);

    // Append 3 entries with term 1
    for (bsls::Types::Uint64 i = 1; i <= 3; ++i) {
        bsl::shared_ptr<bdlbb::Blob> rec = tester.makeUpdateRecord(1, i);
        BMQTST_ASSERT_EQ(tester.raftLog().append(1, rec), 0);
    }

    // Truncate from index 2
    rc = tester.raftLog().truncateFrom(2);
    BMQTST_ASSERT_EQ(rc, 0);
    BMQTST_ASSERT_EQ(tester.raftLog().lastIndex(), 1ULL);

    // Append new entry with term 2
    bsl::shared_ptr<bdlbb::Blob> rec = tester.makeUpdateRecord(2, 2);
    BMQTST_ASSERT_EQ(tester.raftLog().append(2, rec), 0);

    BMQTST_ASSERT_EQ(tester.raftLog().lastIndex(), 2ULL);
    BMQTST_ASSERT_EQ(tester.raftLog().term(1), 1ULL);
    BMQTST_ASSERT_EQ(tester.raftLog().term(2), 2ULL);
}

static void test5_openPrePopulated()
{
    bmqtst::TestHelper::printTestName("OPEN PRE-POPULATED");

    bslma::Allocator*              alloc = bmqtst::TestHelperUtil::allocator();
    bmqu::TempDirectory            tempDir(alloc);
    bdlbb::PooledBlobBufferFactory       factory(256, alloc);
    bmqp::BlobPoolUtil::BlobSpPoolSp     poolSp =
        bmqp::BlobPoolUtil::createBlobPool(&factory, alloc);

    bsl::string      logPath = tempDir.path() + "/csl_prepop.bmq";
    mqbsi::LogConfig logConfig(k_LOG_MAX_SIZE,
                               k_LOG_KEY,
                               logPath,
                               true,
                               false,
                               alloc);

    // Phase 1: create and populate
    {
        bsl::shared_ptr<mqbsl::MemoryMappedOnDiskLog> log(
            new (*alloc) mqbsl::MemoryMappedOnDiskLog(logConfig),
            alloc);
        int rc = log->open(mqbsi::Log::e_CREATE_IF_MISSING);
        BMQTST_ASSERT_EQ(rc, 0);

        mqbc::ClusterStateFileHeader fileHeader;
        log->write(&fileHeader,
                   0,
                   static_cast<int>(sizeof(mqbc::ClusterStateFileHeader)));

        for (bsls::Types::Uint64 i = 1; i <= 3; ++i) {
            bsl::shared_ptr<bdlbb::Blob> rec = makeRecord(i, i, &factory, alloc);
            log->write(*rec, bmqu::BlobPosition(), rec->length());
        }

        log->close();
    }

    // Phase 2: reopen and verify index is rebuilt
    {
        bsl::shared_ptr<mqbsl::MemoryMappedOnDiskLog> log(
            new (*alloc) mqbsl::MemoryMappedOnDiskLog(logConfig),
            alloc);
        int rc = log->open(0);
        BMQTST_ASSERT_EQ(rc, 0);

        CslRaftLog raftLog(log, poolSp.get(), alloc);
        rc = raftLog.open();
        BMQTST_ASSERT_EQ(rc, 0);

        BMQTST_ASSERT_EQ(raftLog.lastIndex(), 3ULL);
        BMQTST_ASSERT_EQ(raftLog.term(1), 1ULL);
        BMQTST_ASSERT_EQ(raftLog.term(2), 2ULL);
        BMQTST_ASSERT_EQ(raftLog.term(3), 3ULL);
    }
}

// ============================================================================
//                                 MAIN PROGRAM
// ============================================================================

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 5: test5_openPrePopulated(); break;
    case 4: test4_appendAfterTruncate(); break;
    case 3: test3_truncate(); break;
    case 2: test2_appendAndReadBack(); break;
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_GBL_ALLOC);
}
