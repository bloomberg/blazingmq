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
#include <bdls_filesystemutil.h>
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

/// Build a base `e_SNAPSHOT` CSL record blob, as a rollover writes and a
/// leader ships, with the specified 'term' and 'seqNum'.
bsl::shared_ptr<bdlbb::Blob>
makeSnapshotRecord(bsls::Types::Uint64       term,
                   bsls::Types::Uint64       seqNum,
                   bdlbb::BlobBufferFactory* factory,
                   bslma::Allocator*         allocator)
{
    bmqp_ctrlmsg::ClusterMessage        clusterMessage(allocator);
    bmqp_ctrlmsg::LeaderMessageSequence lms;
    lms.electorTerm()    = term;
    lms.sequenceNumber() = seqNum;

    bmqp_ctrlmsg::LeaderAdvisory& advisory =
        clusterMessage.choice().makeLeaderAdvisory();
    advisory.sequenceNumber() = lms;

    bsl::shared_ptr<bdlbb::Blob> record =
        bsl::make_shared<bdlbb::Blob>(factory, allocator);
    const int rc = mqbc::ClusterStateLedgerUtil::appendRecord(
        record.get(),
        clusterMessage,
        lms,
        0,
        mqbc::ClusterStateRecordType::e_SNAPSHOT,
        allocator);
    BSLS_ASSERT_OPT(rc == 0);
    (void)rc;

    return record;
}

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
    int rc = mqbc::ClusterStateLedgerUtil::appendRecord(
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

/// Build a legacy `e_COMMIT` CSL record blob with the specified `term` and
/// `seqNum`, committing the record at `term` and the specified
/// `committedSeqNum`.
bsl::shared_ptr<bdlbb::Blob>
makeCommitRecord(bsls::Types::Uint64       term,
                 bsls::Types::Uint64       seqNum,
                 bsls::Types::Uint64       committedSeqNum,
                 bdlbb::BlobBufferFactory* factory,
                 bslma::Allocator*         allocator)
{
    bmqp_ctrlmsg::ClusterMessage        clusterMessage(allocator);
    bmqp_ctrlmsg::LeaderMessageSequence lms;
    lms.electorTerm()    = term;
    lms.sequenceNumber() = seqNum;

    bmqp_ctrlmsg::LeaderAdvisoryCommit& commit =
        clusterMessage.choice().makeLeaderAdvisoryCommit();
    commit.sequenceNumber()                           = lms;
    commit.sequenceNumberCommitted().electorTerm()    = term;
    commit.sequenceNumberCommitted().sequenceNumber() = committedSeqNum;

    bsl::shared_ptr<bdlbb::Blob> record =
        bsl::make_shared<bdlbb::Blob>(factory, allocator);
    int rc = mqbc::ClusterStateLedgerUtil::appendRecord(
        record.get(),
        clusterMessage,
        lms,
        0,
        mqbc::ClusterStateRecordType::e_COMMIT,
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
    bmqu::TempDirectory              d_tempDir;
    bsl::string                      d_logPath;
    mqbsi::LogConfig                 d_logConfig;
    bsl::shared_ptr<mqbsi::Log>      d_log_sp;
    bdlbb::PooledBlobBufferFactory   d_bufferFactory;
    bmqp::BlobPoolUtil::BlobSpPoolSp d_blobSpPool_sp;
    CslRaftLog                       d_cslRaftLog;
    bslma::Allocator*                d_allocator_p;

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
    , d_blobSpPool_sp(
          bmqp::BlobPoolUtil::createBlobPool(&d_bufferFactory, allocator))
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

    CslRaftLog&       raftLog() { return d_cslRaftLog; }
    bslma::Allocator* allocator() { return d_allocator_p; }
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
    tester.raftLog().entries(1, 4, &entries, 0, 0);
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
    bdlbb::PooledBlobBufferFactory factory(256, alloc);
    bmqp::BlobPoolUtil::BlobSpPoolSp poolSp =
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
            bsl::shared_ptr<bdlbb::Blob> rec =
                makeRecord(i, i, &factory, alloc);
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

static void test6_installSnapshotMatchesOpen()
// INSTALL SNAPSHOT MATCHES OPEN
//
// 'installSnapshot' must leave the log in the state 'open()' derives from
// the file it just wrote, or this node's indexes change when it restarts.
{
    bmqtst::TestHelper::printTestName("INSTALL SNAPSHOT MATCHES OPEN");

    bslma::Allocator*              alloc = bmqtst::TestHelperUtil::allocator();
    bmqu::TempDirectory            tempDir(alloc);
    bdlbb::PooledBlobBufferFactory factory(256, alloc);
    bmqp::BlobPoolUtil::BlobSpPoolSp poolSp =
        bmqp::BlobPoolUtil::createBlobPool(&factory, alloc);

    // The record a leader ships: the entry at index 18, term 1.
    const bsls::Types::Uint64 k_RECORD_INDEX = 18;
    const bsls::Types::Uint64 k_RECORD_TERM  = 1;

    const bsl::string oldPath = tempDir.path() + "/csl_old.bmq";
    const bsl::string newPath = tempDir.path() + "/csl_new.bmq";

    mqbsi::LogConfig oldConfig(k_LOG_MAX_SIZE,
                               k_LOG_KEY,
                               oldPath,
                               true,
                               false,
                               alloc);
    mqbsi::LogConfig newConfig(k_LOG_MAX_SIZE,
                               k_LOG_KEY,
                               newPath,
                               true,
                               false,
                               alloc);

    bsls::Types::Uint64 installedSnapshotIndex = 0;
    bsls::Types::Uint64 installedLastIndex     = 0;

    // Phase 1: install the snapshot over a log holding a couple of entries.
    {
        bsl::shared_ptr<mqbsl::MemoryMappedOnDiskLog> oldLog(
            new (*alloc) mqbsl::MemoryMappedOnDiskLog(oldConfig),
            alloc);
        BMQTST_ASSERT_EQ(oldLog->open(mqbsi::Log::e_CREATE_IF_MISSING), 0);

        mqbc::ClusterStateFileHeader fileHeader;
        oldLog->write(&fileHeader,
                      0,
                      static_cast<int>(sizeof(mqbc::ClusterStateFileHeader)));

        CslRaftLog raftLog(oldLog, poolSp.get(), alloc);
        BMQTST_ASSERT_EQ(raftLog.open(), 0);
        for (bsls::Types::Uint64 i = 1; i <= 2; ++i) {
            raftLog.append(1, makeRecord(1, i, &factory, alloc));
        }
        BMQTST_ASSERT_EQ(raftLog.lastIndex(), 2ULL);

        bsl::shared_ptr<mqbsl::MemoryMappedOnDiskLog> newLog(
            new (*alloc) mqbsl::MemoryMappedOnDiskLog(newConfig),
            alloc);
        BMQTST_ASSERT_EQ(newLog->open(mqbsi::Log::e_CREATE_IF_MISSING), 0);

        bsl::shared_ptr<mqbsi::Log> replaced;
        BMQTST_ASSERT_EQ(
            raftLog.installSnapshot(&replaced,
                                    newLog,
                                    k_LOG_KEY,
                                    makeSnapshotRecord(k_RECORD_TERM,
                                                       k_RECORD_INDEX,
                                                       &factory,
                                                       alloc),
                                    k_RECORD_INDEX,
                                    k_RECORD_TERM),
            0);
        replaced->close();

        installedSnapshotIndex = raftLog.snapshotIndex();
        installedLastIndex     = raftLog.lastIndex();

        // The snapshot record is the entry at its own index, and the index
        // below it is answerable -- that is the 'prevLogIndex' the leader
        // sends next.
        BMQTST_ASSERT_EQ(installedSnapshotIndex, k_RECORD_INDEX - 1);
        BMQTST_ASSERT_EQ(installedLastIndex, k_RECORD_INDEX);
        BMQTST_ASSERT_EQ(raftLog.term(k_RECORD_INDEX - 1), k_RECORD_TERM);
        BMQTST_ASSERT_EQ(raftLog.term(k_RECORD_INDEX), k_RECORD_TERM);

        newLog->close();
    }

    // Phase 2: reopen the file that was just written; it must describe the
    // same log.
    {
        bsl::shared_ptr<mqbsl::MemoryMappedOnDiskLog> log(
            new (*alloc) mqbsl::MemoryMappedOnDiskLog(newConfig),
            alloc);
        BMQTST_ASSERT_EQ(log->open(0), 0);

        CslRaftLog raftLog(log, poolSp.get(), alloc);
        BMQTST_ASSERT_EQ(raftLog.open(), 0);

        BMQTST_ASSERT_EQ(raftLog.snapshotIndex(), installedSnapshotIndex);
        BMQTST_ASSERT_EQ(raftLog.lastIndex(), installedLastIndex);
        BMQTST_ASSERT_EQ(raftLog.term(k_RECORD_INDEX - 1), k_RECORD_TERM);
        BMQTST_ASSERT_EQ(raftLog.term(k_RECORD_INDEX), k_RECORD_TERM);
    }
}

static void test7_rolloverMatchesOpen()
// ROLLOVER MATCHES OPEN
//
// 'rollover' must leave the log in the state 'open()' derives from the file
// it just wrote, so a leader describes its log the same way before and after
// a restart.
{
    bmqtst::TestHelper::printTestName("ROLLOVER MATCHES OPEN");

    bslma::Allocator*              alloc = bmqtst::TestHelperUtil::allocator();
    bmqu::TempDirectory            tempDir(alloc);
    bdlbb::PooledBlobBufferFactory factory(256, alloc);
    bmqp::BlobPoolUtil::BlobSpPoolSp poolSp =
        bmqp::BlobPoolUtil::createBlobPool(&factory, alloc);

    // Compact at 3 of 5 entries, leaving 4 and 5 as the uncommitted tail.
    const bsls::Types::Uint64 k_COMPACT_INDEX = 3;
    const bsls::Types::Uint64 k_COMPACT_TERM  = 1;
    const bsls::Types::Uint64 k_LAST_INDEX    = 5;

    const bsl::string oldPath = tempDir.path() + "/csl_ro_old.bmq";
    const bsl::string newPath = tempDir.path() + "/csl_ro_new.bmq";

    mqbsi::LogConfig oldConfig(k_LOG_MAX_SIZE,
                               k_LOG_KEY,
                               oldPath,
                               true,
                               false,
                               alloc);
    mqbsi::LogConfig newConfig(k_LOG_MAX_SIZE,
                               k_LOG_KEY,
                               newPath,
                               true,
                               false,
                               alloc);

    bsls::Types::Uint64 rolledSnapshotIndex = 0;
    bsls::Types::Uint64 rolledLastIndex     = 0;

    // Phase 1: roll over.
    {
        bsl::shared_ptr<mqbsl::MemoryMappedOnDiskLog> oldLog(
            new (*alloc) mqbsl::MemoryMappedOnDiskLog(oldConfig),
            alloc);
        BMQTST_ASSERT_EQ(oldLog->open(mqbsi::Log::e_CREATE_IF_MISSING), 0);

        mqbc::ClusterStateFileHeader fileHeader;
        oldLog->write(&fileHeader,
                      0,
                      static_cast<int>(sizeof(mqbc::ClusterStateFileHeader)));

        CslRaftLog raftLog(oldLog, poolSp.get(), alloc);
        BMQTST_ASSERT_EQ(raftLog.open(), 0);
        for (bsls::Types::Uint64 i = 1; i <= k_LAST_INDEX; ++i) {
            raftLog.append(k_COMPACT_TERM,
                           makeRecord(k_COMPACT_TERM, i, &factory, alloc));
        }
        BMQTST_ASSERT_EQ(raftLog.lastIndex(), k_LAST_INDEX);

        bsl::shared_ptr<mqbsl::MemoryMappedOnDiskLog> newLog(
            new (*alloc) mqbsl::MemoryMappedOnDiskLog(newConfig),
            alloc);
        BMQTST_ASSERT_EQ(newLog->open(mqbsi::Log::e_CREATE_IF_MISSING), 0);

        bsl::shared_ptr<mqbsi::Log> replaced;
        BMQTST_ASSERT_EQ(raftLog.rollover(&replaced,
                                          newLog,
                                          k_LOG_KEY,
                                          makeSnapshotRecord(k_COMPACT_TERM,
                                                             k_COMPACT_INDEX,
                                                             &factory,
                                                             alloc),
                                          k_COMPACT_INDEX,
                                          k_COMPACT_TERM),
                         0);
        replaced->close();

        rolledSnapshotIndex = raftLog.snapshotIndex();
        rolledLastIndex     = raftLog.lastIndex();

        // The tail survives at its original indexes, and the index below the
        // snapshot record still reports a term.
        BMQTST_ASSERT_EQ(rolledSnapshotIndex, k_COMPACT_INDEX - 1);
        BMQTST_ASSERT_EQ(rolledLastIndex, k_LAST_INDEX);
        BMQTST_ASSERT_EQ(raftLog.term(k_COMPACT_INDEX - 1), k_COMPACT_TERM);
        BMQTST_ASSERT_EQ(raftLog.term(k_LAST_INDEX), k_COMPACT_TERM);

        newLog->close();
    }

    // Phase 2: reopen the file that was just written.
    {
        bsl::shared_ptr<mqbsl::MemoryMappedOnDiskLog> log(
            new (*alloc) mqbsl::MemoryMappedOnDiskLog(newConfig),
            alloc);
        BMQTST_ASSERT_EQ(log->open(0), 0);

        CslRaftLog raftLog(log, poolSp.get(), alloc);
        BMQTST_ASSERT_EQ(raftLog.open(), 0);

        BMQTST_ASSERT_EQ(raftLog.snapshotIndex(), rolledSnapshotIndex);
        BMQTST_ASSERT_EQ(raftLog.lastIndex(), rolledLastIndex);
        BMQTST_ASSERT_EQ(raftLog.term(k_COMPACT_INDEX - 1), k_COMPACT_TERM);
        BMQTST_ASSERT_EQ(raftLog.term(k_LAST_INDEX), k_COMPACT_TERM);
    }
}

static void test8_openAfterCrashRestart()
// OPEN AFTER CRASH RESTART
//
// 'open()' must leave the log positioned at the end of the records it
// indexed.  A clean 'close()' truncates the file to what was written, so
// reopening it lands there by accident; a process killed mid-run leaves
// the file at the size 'open()' grew it to, and then the write position
// recovered from the file size is the end of the FILE, not the end of the
// records.  Everything derived from it -- 'canAppend', 'bytesAbove', the
// offset the next 'append' writes at -- is then wrong.
{
    bmqtst::TestHelper::printTestName("OPEN AFTER CRASH RESTART");

    bslma::Allocator*              alloc = bmqtst::TestHelperUtil::allocator();
    bmqu::TempDirectory            tempDir(alloc);
    bdlbb::PooledBlobBufferFactory factory(256, alloc);
    bmqp::BlobPoolUtil::BlobSpPoolSp poolSp =
        bmqp::BlobPoolUtil::createBlobPool(&factory, alloc);

    bsl::string      logPath = tempDir.path() + "/csl_crash.bmq";
    mqbsi::LogConfig logConfig(k_LOG_MAX_SIZE,
                               k_LOG_KEY,
                               logPath,
                               false,  // reserveOnDisk
                               false,
                               alloc);

    const bsls::Types::Uint64 k_NUM_RECORDS = 3;

    mqbsi::Log::Offset offsetBeforeCrash = 0;

    // Phase 1: populate, then die without closing.  'close()' is what
    // truncates the file back to the written length, so skipping it is what
    // a 'kill -9' leaves behind: a file still grown to 'maxSize'.
    {
        bsl::shared_ptr<mqbsl::MemoryMappedOnDiskLog> log(
            new (*alloc) mqbsl::MemoryMappedOnDiskLog(logConfig),
            alloc);
        BMQTST_ASSERT_EQ(log->open(mqbsi::Log::e_CREATE_IF_MISSING), 0);

        mqbc::ClusterStateFileHeader fileHeader;
        log->write(&fileHeader,
                   0,
                   static_cast<int>(sizeof(mqbc::ClusterStateFileHeader)));

        CslRaftLog raftLog(log, poolSp.get(), alloc);
        BMQTST_ASSERT_EQ(raftLog.open(), 0);

        for (bsls::Types::Uint64 i = 1; i <= k_NUM_RECORDS; ++i) {
            bsl::shared_ptr<bdlbb::Blob> rec =
                makeRecord(i, i, &factory, alloc);
            BMQTST_ASSERT_EQ(raftLog.append(i, rec), 0);
        }

        offsetBeforeCrash = log->currentOffset();

        // No 'close()' and no 'raftLog.close()': the process is gone.
    }

    // The file is left at its grown size, above what was written.
    BMQTST_ASSERT_EQ(bdls::FilesystemUtil::getFileSize(logPath),
                     k_LOG_MAX_SIZE);
    BMQTST_ASSERT_LT(offsetBeforeCrash, k_LOG_MAX_SIZE);

    // Phase 2: restart on that file.
    {
        bsl::shared_ptr<mqbsl::MemoryMappedOnDiskLog> log(
            new (*alloc) mqbsl::MemoryMappedOnDiskLog(logConfig),
            alloc);
        BMQTST_ASSERT_EQ(log->open(mqbsi::Log::e_CREATE_IF_MISSING), 0);

        CslRaftLog raftLog(log, poolSp.get(), alloc);
        BMQTST_ASSERT_EQ(raftLog.open(), 0);

        // The index is rebuilt from the records -- this part already worked.
        BMQTST_ASSERT_EQ(raftLog.lastIndex(), k_NUM_RECORDS);
        BMQTST_ASSERT_EQ(raftLog.term(1), 1ULL);
        BMQTST_ASSERT_EQ(raftLog.term(2), 2ULL);
        BMQTST_ASSERT_EQ(raftLog.term(3), 3ULL);

        // The write position must be where the records end, not where the
        // file does.
        BMQTST_ASSERT_EQ(log->currentOffset(), offsetBeforeCrash);
        BMQTST_ASSERT_EQ(log->outstandingNumBytes(), offsetBeforeCrash);

        // 'bytesAbove' measures the uncommitted tail a rollover has to carry
        // forward; off the file size it reports nearly the whole file.
        BMQTST_ASSERT_EQ(
            raftLog.bytesAbove(0),
            static_cast<bsls::Types::Uint64>(
                offsetBeforeCrash - sizeof(mqbc::ClusterStateFileHeader)));

        // Replication can proceed: room is judged against the records, and
        // the next entry lands after them.
        bsl::shared_ptr<bdlbb::Blob> rec = makeRecord(4, 4, &factory, alloc);
        BMQTST_ASSERT(raftLog.canAppend(rec->length()));
        BMQTST_ASSERT_EQ(raftLog.append(4, rec), 0);
        BMQTST_ASSERT_EQ(raftLog.lastIndex(), k_NUM_RECORDS + 1);

        log->close();
    }

    // Phase 3: the entry appended after the restart is durable and indexed
    // where the pre-crash ones are.
    {
        bsl::shared_ptr<mqbsl::MemoryMappedOnDiskLog> log(
            new (*alloc) mqbsl::MemoryMappedOnDiskLog(logConfig),
            alloc);
        BMQTST_ASSERT_EQ(log->open(0), 0);

        CslRaftLog raftLog(log, poolSp.get(), alloc);
        BMQTST_ASSERT_EQ(raftLog.open(), 0);

        BMQTST_ASSERT_EQ(raftLog.lastIndex(), k_NUM_RECORDS + 1);
        BMQTST_ASSERT_EQ(raftLog.term(4), 4ULL);

        bsl::vector<LogEntry> entries(alloc);
        raftLog.entries(1, k_NUM_RECORDS + 2, &entries, 0, 0);
        BMQTST_ASSERT_EQ(entries.size(),
                         static_cast<size_t>(k_NUM_RECORDS + 1));

        log->close();
    }
}

static void test9_appendPastTruncationPoint()
// APPEND PAST TRUNCATION POINT
//
// A follower truncates its log and then appends the leader's entries.  The
// file stays at its mapped size, so those entries may run past the old end
// of data.  After a restart without 'close()', 'open()' must stop at the
// entries written after the truncation: the removed ones read as zero.
{
    bmqtst::TestHelper::printTestName("APPEND PAST TRUNCATION POINT");

    bslma::Allocator*              alloc = bmqtst::TestHelperUtil::allocator();
    bmqu::TempDirectory            tempDir(alloc);
    bdlbb::PooledBlobBufferFactory factory(256, alloc);
    bmqp::BlobPoolUtil::BlobSpPoolSp poolSp =
        bmqp::BlobPoolUtil::createBlobPool(&factory, alloc);

    bsl::string      logPath = tempDir.path() + "/csl_truncate.bmq";
    mqbsi::LogConfig logConfig(k_LOG_MAX_SIZE,
                               k_LOG_KEY,
                               logPath,
                               false,  // reserveOnDisk
                               false,
                               alloc);

    // Enough records to cover several pages.
    const bsls::Types::Uint64 k_NUM_RECORDS = 1000;
    const bsls::Types::Uint64 k_KEEP        = 1;
    const bsls::Types::Uint64 k_REAPPEND    = 10;

    {
        bsl::shared_ptr<mqbsl::MemoryMappedOnDiskLog> log(
            new (*alloc) mqbsl::MemoryMappedOnDiskLog(logConfig),
            alloc);
        BMQTST_ASSERT_EQ(log->open(mqbsi::Log::e_CREATE_IF_MISSING), 0);

        mqbc::ClusterStateFileHeader fileHeader;
        log->write(&fileHeader,
                   0,
                   static_cast<int>(sizeof(mqbc::ClusterStateFileHeader)));

        CslRaftLog raftLog(log, poolSp.get(), alloc);
        BMQTST_ASSERT_EQ(raftLog.open(), 0);

        for (bsls::Types::Uint64 i = 1; i <= k_NUM_RECORDS; ++i) {
            BMQTST_ASSERT_EQ(raftLog.append(1,
                                            makeRecord(1, i, &factory, alloc)),
                             0);
        }

        BMQTST_ASSERT_EQ(raftLog.truncateFrom(k_KEEP + 1), 0);
        BMQTST_ASSERT_EQ(raftLog.lastIndex(), k_KEEP);

        // The file keeps its mapped size.
        BMQTST_ASSERT_EQ(bdls::FilesystemUtil::getFileSize(logPath),
                         k_LOG_MAX_SIZE);

        // Run past the truncation point by more than a page.
        for (bsls::Types::Uint64 i = k_KEEP + 1; i <= k_NUM_RECORDS; ++i) {
            BMQTST_ASSERT_EQ(raftLog.append(2,
                                            makeRecord(2, i, &factory, alloc)),
                             0);
        }
        BMQTST_ASSERT_EQ(raftLog.lastIndex(), k_NUM_RECORDS);

        // Remove most of them again and append a few, so the tail past the
        // new end held records.
        BMQTST_ASSERT_EQ(raftLog.truncateFrom(k_KEEP + 1), 0);
        for (bsls::Types::Uint64 i = 1; i <= k_REAPPEND; ++i) {
            BMQTST_ASSERT_EQ(
                raftLog.append(3, makeRecord(3, k_KEEP + i, &factory, alloc)),
                0);
        }

        // No 'close()': the process is gone.
    }

    {
        bsl::shared_ptr<mqbsl::MemoryMappedOnDiskLog> log(
            new (*alloc) mqbsl::MemoryMappedOnDiskLog(logConfig),
            alloc);
        BMQTST_ASSERT_EQ(log->open(mqbsi::Log::e_CREATE_IF_MISSING), 0);

        CslRaftLog raftLog(log, poolSp.get(), alloc);
        BMQTST_ASSERT_EQ(raftLog.open(), 0);

        BMQTST_ASSERT_EQ(raftLog.lastIndex(), k_KEEP + k_REAPPEND);
        BMQTST_ASSERT_EQ(raftLog.term(k_KEEP), 1ULL);
        BMQTST_ASSERT_EQ(raftLog.term(k_KEEP + k_REAPPEND), 3ULL);

        log->close();
    }
}

static void test10_openLegacyFile()
// OPEN LEGACY FILE
//
// A legacy CSL file holds every record type, numbered by sequence number
// within a term, with gaps before each 'leaderAdvisory'.  'open()' starts at
// the latest 'e_SNAPSHOT', makes each record the entry at its sequence
// number, and ends the log at the first record whose sequence number is not
// the next one.
{
    bmqtst::TestHelper::printTestName("OPEN LEGACY FILE");

    bslma::Allocator*              alloc = bmqtst::TestHelperUtil::allocator();
    bmqu::TempDirectory            tempDir(alloc);
    bdlbb::PooledBlobBufferFactory factory(256, alloc);
    bmqp::BlobPoolUtil::BlobSpPoolSp poolSp =
        bmqp::BlobPoolUtil::createBlobPool(&factory, alloc);

    bsl::string      logPath = tempDir.path() + "/csl_legacy.bmq";
    mqbsi::LogConfig logConfig(k_LOG_MAX_SIZE,
                               k_LOG_KEY,
                               logPath,
                               false,  // reserveOnDisk
                               false,
                               alloc);

    mqbsi::Log::Offset endOfLastEntry = 0;
    {
        bsl::shared_ptr<mqbsl::MemoryMappedOnDiskLog> log(
            new (*alloc) mqbsl::MemoryMappedOnDiskLog(logConfig),
            alloc);
        BMQTST_ASSERT_EQ(log->open(mqbsi::Log::e_CREATE_IF_MISSING), 0);

        mqbc::ClusterStateFileHeader fileHeader;
        log->write(&fileHeader,
                   0,
                   static_cast<int>(sizeof(mqbc::ClusterStateFileHeader)));

        bsl::vector<bsl::shared_ptr<bdlbb::Blob> > records(alloc);
        records.push_back(makeSnapshotRecord(1, 1, &factory, alloc));
        records.push_back(makeCommitRecord(1, 2, 1, &factory, alloc));
        records.push_back(makeRecord(1, 3, &factory, alloc));
        records.push_back(makeCommitRecord(1, 4, 3, &factory, alloc));
        // Seq 5 to 8 missed; brought up to date by the next advisory.
        records.push_back(makeSnapshotRecord(1, 9, &factory, alloc));
        records.push_back(makeCommitRecord(1, 10, 9, &factory, alloc));
        records.push_back(makeRecord(1, 11, &factory, alloc));

        for (size_t i = 0; i < records.size(); ++i) {
            log->write(*records[i],
                       bmqu::BlobPosition(),
                       records[i]->length());
        }
        endOfLastEntry = log->currentOffset();

        // Not the next sequence number: the log ends before it.
        bsl::shared_ptr<bdlbb::Blob> stale =
            makeRecord(2, 500, &factory, alloc);
        log->write(*stale, bmqu::BlobPosition(), stale->length());

        log->close();
    }

    {
        bsl::shared_ptr<mqbsl::MemoryMappedOnDiskLog> log(
            new (*alloc) mqbsl::MemoryMappedOnDiskLog(logConfig),
            alloc);
        BMQTST_ASSERT_EQ(log->open(0), 0);

        CslRaftLog raftLog(log, poolSp.get(), alloc);
        BMQTST_ASSERT_EQ(raftLog.open(), 0);

        BMQTST_ASSERT_EQ(raftLog.snapshotIndex(), 8ULL);
        BMQTST_ASSERT_EQ(raftLog.snapshotTerm(), 1ULL);
        BMQTST_ASSERT_EQ(raftLog.lastIndex(), 11ULL);
        BMQTST_ASSERT_EQ(raftLog.lastTerm(), 1ULL);
        BMQTST_ASSERT_EQ(log->currentOffset(), endOfLastEntry);

        bsl::vector<LogEntry> entries(alloc);
        raftLog.entries(9, 12, &entries, 0, 0);
        BMQTST_ASSERT_EQ(entries.size(), 3u);

        bmqp_ctrlmsg::ClusterMessage message(alloc);
        BMQTST_ASSERT_EQ(mqbc::ClusterStateLedgerUtil::loadClusterMessage(
                             &message,
                             *entries[0].d_data),
                         0);
        BMQTST_ASSERT(message.choice().isLeaderAdvisoryValue());
        BMQTST_ASSERT_EQ(mqbc::ClusterStateLedgerUtil::loadClusterMessage(
                             &message,
                             *entries[1].d_data),
                         0);
        BMQTST_ASSERT(message.choice().isLeaderAdvisoryCommitValue());

        log->close();
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
    case 10: test10_openLegacyFile(); break;
    case 9: test9_appendPastTruncationPoint(); break;
    case 8: test8_openAfterCrashRestart(); break;
    case 7: test7_rolloverMatchesOpen(); break;
    case 6: test6_installSnapshotMatchesOpen(); break;
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
