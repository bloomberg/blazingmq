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

// mqbraft_partitionraftlog.cpp -*-C++-*-
#include <mqbraft_partitionraftlog.h>

// MQB
#include <mqbs_filestore.h>
#include <mqbstat_statmonitorsnapshotrecorder.h>

// BMQ
#include <bmqtsk_alarmlog.h>

// BDE
#include <ball_log.h>
#include <bdlbb_blob.h>
#include <bsl_algorithm.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace mqbraft {

namespace {

/// Backstop cap on the number of writes buffered during a single
/// proposed-but-not-yet-committed `e_ROLLOVER` window.  A committed rollover
/// normally drains the buffer promptly; this bound simply protects against
/// unbounded memory growth if commit is pathologically delayed, at which
/// point excess writes are NACK-ed as in phase 1.
const bsl::size_t k_MAX_PENDING_WRITES = 4096;

/// `append` failure code: the journal is no longer where the offset anchor
/// says it is, so nothing was written.
const int rc_BAD_OFFSET_ANCHOR = -100;

}  // close unnamed namespace

// ======================
// class PartitionRaftLog
// ======================

// CREATORS
PartitionRaftLog::PartitionRaftLog(mqbs::FileStore*  fileStore,
                                   bslma::Allocator* allocator)
: d_fileStore_p(fileStore)
, d_index(allocator)
, d_snapshotIndex(0)
, d_snapshotTerm(0)
, d_allocator_p(bslma::Default::allocator(allocator))
, d_pendingWrites(d_allocator_p)
, d_cache(allocator)
, d_frontIndex(1)
, d_firstRecordIndex(0)
, d_firstRecordOffset(0)
, d_cacheBase(0)
, d_cacheTerm(0)
, d_cacheBytes(0)
, d_appendedCount(0)
{
    BSLS_ASSERT_SAFE(fileStore);
}

PartitionRaftLog::~PartitionRaftLog()
{
}

// MANIPULATORS
int PartitionRaftLog::open()
{
    // Recovery only appends to 'd_index'.  A reopen (InstallSnapshot replaces
    // the file set) must not keep entries anchored to the discarded files.
    d_index.clear();
    clearCache();

    int rc = d_fileStore_p->openForRaft(&d_index);
    if (rc != 0) {
        return rc;
    }

    const bmqp_ctrlmsg::PartitionSequenceNumber& snapshotPSN =
        d_fileStore_p->firstSyncPointAfterRolloverSeqNum();
    d_snapshotIndex = snapshotPSN.sequenceNumber();
    d_snapshotTerm  = snapshotPSN.primaryLeaseId();

    d_frontIndex = d_snapshotIndex + 1;

    // Anchor the offset arithmetic on the first record of this file set.
    d_firstRecordIndex  = d_snapshotIndex + 1;
    d_firstRecordOffset = d_index.empty() ? d_fileStore_p->journalPosition()
                                          : d_index.front().d_journalOffset;

    BALL_LOG_INFO << "PartitionRaftLog::open: recovered " << d_index.size()
                  << " entries, snapshotIndex=" << d_snapshotIndex
                  << ", snapshotTerm=" << d_snapshotTerm
                  << ", lastIndex=" << lastIndex()
                  << ", lastTerm=" << lastTerm()
                  << ", firstRecordIndex=" << d_firstRecordIndex
                  << ", firstRecordOffset=" << d_firstRecordOffset
                  << ", journalPosition=" << d_fileStore_p->journalPosition()
                  << ", frontOffset="
                  << (d_index.empty() ? 0 : d_index.front().d_journalOffset)
                  << ", backOffset="
                  << (d_index.empty() ? 0 : d_index.back().d_journalOffset);

    d_appendedCount = 0;
    return 0;
}

void PartitionRaftLog::setPendingWrite(const bsl::shared_ptr<PendingWrite>& pw)
{
    d_pendingWrites.push_back(pw);
}

int PartitionRaftLog::bufferPendingWrite(
    const bsl::shared_ptr<PendingWrite>& pw,
    bsls::Types::Uint64                  term)
{
    BSLS_ASSERT_SAFE(pw);
    enum { rc_SUCCESS = 0, rc_ROLLOVER_PENDING = -1 };

    // A rollover is in flight ('e_ROLLOVER' proposed but not yet committed):
    // the write must not append after 'e_ROLLOVER', so hold it here to be
    // drained into the new file set once the rollover commits.
    if (d_pendingWrites.size() >= k_MAX_PENDING_WRITES) {
        // Backstop: buffer is full, fall back to NACK-ing the write.
        BALL_LOG_WARN << "PartitionRaftLog: pending-write buffer full ("
                      << k_MAX_PENDING_WRITES
                      << ") during rollover window; NACK-ing write.";
        return rc_ROLLOVER_PENDING;  // RETURN
    }

    // While 'e_ROLLOVER' is pending, 'lastIndex()' is fixed at the
    // 'e_ROLLOVER' index (N); a write buffered at position 'k' (== count of
    // writes already buffered, i.e. excluding the still-appended-but-
    // uncommitted 'e_ROLLOVER' itself) will receive log index 'N + 1 + k'
    // when drained in FIFO order.
    const bsls::Types::Uint64 futureIndex = lastIndex() + 1 +
                                            (d_pendingWrites.size() -
                                             d_appendedCount);

    // Reserve a placeholder 'd_records' entry keyed by the future
    // '(term, index)' so callers get a valid handle now; the physical offsets
    // are patched in when this write drains.  Record types that produce no
    // handle (message-deletion) are skipped -- their write method ignores the
    // handle, matching the normal path.
    const bool producesHandle = pw->d_recordType !=
                                mqbs::RecordType::e_DELETION;
    if (producesHandle) {
        d_fileStore_p->reservePendingRecord(pw.get(),
                                            static_cast<unsigned int>(term),
                                            futureIndex,
                                            pw->d_recordType);
    }

    d_pendingWrites.push_back(pw);

    return rc_SUCCESS;
}

void PartitionRaftLog::takePendingWrites(PendingWrites* out)
{
    BSLS_ASSERT_SAFE(out);

    // Every write handed over must still be unappended, since the caller
    // re-proposes each one: an appended write here would be appended a second
    // time.  Both rollover shapes leave nothing appended by the time the drain
    // runs -- 'rollover' pops the 'e_ROLLOVER' this node proposed itself, and
    // an inherited one can only commit under a current-term entry above it,
    // which the apply loop pops before the drain.
    BSLS_ASSERT_SAFE(0 == d_appendedCount);

    // 'swap' is O(1) and hands over the shared_ptrs, so the pooled objects
    // stay valid.
    out->swap(d_pendingWrites);
    d_appendedCount = 0;
}

bsls::Types::Uint64 PartitionRaftLog::writeHeadIndex() const
{
    // The buffered writes are those past 'd_appendedCount'; each takes the
    // next index above the log when the rollover drains them.
    return lastIndex() + (d_pendingWrites.size() - d_appendedCount);
}

void PartitionRaftLog::releaseBufferedRecords()
{
    for (size_t i = d_appendedCount; i < d_pendingWrites.size(); ++i) {
        const bsl::shared_ptr<PendingWrite>& sp = d_pendingWrites[i];

        if (sp->d_handle.isValid()) {
            d_fileStore_p->dropPendingRecord(sp->d_handle);
            sp->d_handle = mqbs::DataStoreRecordHandle();
        }
    }
}

void PartitionRaftLog::dropWritesFrom(bsls::Types::Uint64 index,
                                      HeldWrites*         out)
{
    const size_t mark     = out ? out->size() : 0;
    const size_t before   = d_pendingWrites.size();
    size_t       buffered = 0;

    while (!d_pendingWrites.empty()) {
        const bsl::shared_ptr<PendingWrite>& sp = d_pendingWrites.back();

        // The buffered ones go whole: they were held for a rollover that will
        // not drain now, and their reserved indices are all above the log.
        const bool isBuffered = d_pendingWrites.size() > d_appendedCount;

        if (!isBuffered && sp->d_sequenceNumber < index) {
            break;  // BREAK
        }

        if (isBuffered) {
            // A placeholder belongs to no log entry and carries offset 0, so
            // nothing else would reclaim it.  An appended write's record is a
            // real entry, erased by the truncation that erased the entry.
            if (sp->d_handle.isValid()) {
                d_fileStore_p->dropPendingRecord(sp->d_handle);
            }
            ++buffered;
        }

        // Whatever propose set aside is given back here, or by the caller
        // that takes the write on instead, so that it happens once.
        if (out) {
            // 'd_handle' indexes a record that is gone, and 'd_entryBlob'
            // aliases a mapping the truncation rolled back.  A re-post reads
            // neither, and holding the alias would keep the file set
            // referenced past its close.
            sp->d_handle = mqbs::DataStoreRecordHandle();
            sp->d_entryBlob.reset();

            out->push_back(sp);
        }
        else {
            d_fileStore_p->undoPropose(*sp);
        }

        d_pendingWrites.pop_back();
        if (d_appendedCount > d_pendingWrites.size()) {
            d_appendedCount = d_pendingWrites.size();
        }
    }

    if (before == d_pendingWrites.size()) {
        return;  // RETURN
    }

    // Popped newest-first; hand them over in the order the producers sent
    // them.  Only this call's range, since 'out' accumulates across calls.
    if (out) {
        bsl::reverse(out->begin() + mark, out->end());
    }

    BALL_LOG_INFO << "PartitionRaftLog: removing "
                  << (before - d_pendingWrites.size()) << " write(s) -- "
                  << buffered << " buffered, "
                  << (before - d_pendingWrites.size() - buffered)
                  << " appended at or above index " << index << "; "
                  << (out ? "kept by the caller" : "discarded");
}

void PartitionRaftLog::invalidatePendingWriteHandle(
    const mqbs::DataStoreRecordHandle& handle)
{
    if (d_pendingWrites.empty()) {
        return;  // RETURN (not in buffer)
    }

    bsls::Types::Uint64 firstSeqNum = d_pendingWrites[0]->d_sequenceNumber;
    if (handle.sequenceNum() < firstSeqNum) {
        return;  // RETURN (already removed or never buffered)
    }

    if (handle.sequenceNum() >= firstSeqNum + d_pendingWrites.size()) {
        // Past the buffer: a record the new leader appended after this node's
        // own writes, which outlive its leadership until they commit.
        return;  // RETURN
    }

    bsls::Types::Uint64 pos        = handle.sequenceNum() - firstSeqNum;
    d_pendingWrites[pos]->d_handle = mqbs::DataStoreRecordHandle();
}

int PartitionRaftLog::append(bsls::Types::Uint64                 term,
                             const bsl::shared_ptr<bdlbb::Blob>& data)
{
    // Both write paths append at the journal's current position, so the anchor
    // can be checked before anything is written.
    if (0 != verifyJournalOffset(lastIndex() + 1,
                                 d_fileStore_p->journalPosition())) {
        return rc_BAD_OFFSET_ANCHOR;  // RETURN
    }

    if (!data) {
        // Primary path: format directly in mmap via PendingWrite.
        BSLS_ASSERT_SAFE(!d_pendingWrites.empty());
        BSLS_ASSERT_SAFE(d_appendedCount < d_pendingWrites.size());
        PendingWrite& pw = *d_pendingWrites[d_appendedCount];

        pw.d_primaryLeaseId = term;
        pw.d_sequenceNumber = lastIndex() + 1;

        int rc = -1;
        if (pw.d_recordType == mqbs::RecordType::e_MESSAGE) {
            rc = d_fileStore_p->formatMessageRecord(&pw);
        }
        else if (pw.d_recordType == mqbs::RecordType::e_CONFIRM) {
            rc = d_fileStore_p->formatConfirmRecord(&pw);
        }
        else if (pw.d_recordType == mqbs::RecordType::e_DELETION) {
            rc = d_fileStore_p->formatDeletionRecord(&pw);
        }
        else if (pw.d_recordType == mqbs::RecordType::e_QUEUE_OP) {
            if (pw.d_queueOpType == mqbs::QueueOpType::e_CREATION) {
                rc = d_fileStore_p->formatQueueCreationRecord(&pw);
            }
            else if (pw.d_queueOpType == mqbs::QueueOpType::e_PURGE) {
                rc = d_fileStore_p->formatQueuePurgeRecord(&pw);
            }
            else {
                BSLS_ASSERT_SAFE(pw.d_queueOpType ==
                                 mqbs::QueueOpType::e_DELETION);
                rc = d_fileStore_p->formatQueueDeletionRecord(&pw);
            }
        }
        else {
            BSLS_ASSERT_SAFE(pw.d_recordType ==
                             mqbs::RecordType::e_JOURNAL_OP);
            rc = d_fileStore_p->formatSyncPointRecord(&pw);
        }
        if (rc != 0) {
            // The FileStore refused the record (e.g. the active file set is
            // out of journal space or has been marked unavailable).  The entry
            // is NOT appended (lastIndex unchanged), so it can never commit;
            // log loudly rather than fail silently.
            BALL_LOG_ERROR << "PartitionRaftLog: failed to format primary "
                           << "record (recordType=" << pw.d_recordType
                           << ", syncPointType=" << pw.d_syncPointType
                           << ") at index " << pw.d_sequenceNumber
                           << ", rc=" << rc;
            return rc;
        }

        d_index.push_back(EntryInfo(term,
                                    pw.d_journalOffset,
                                    pw.d_dataOffset,
                                    pw.d_qlistOffset,
                                    pw.d_recordType,
                                    pw.d_handle,
                                    pw.d_syncPointType));

        d_appendedCount++;
        return 0;
    }

    // Replica path: combined blob from AppendEntries.
    BSLS_ASSERT_SAFE(data && data->length() > 0);

    EntryInfo info;
    int       rc = d_fileStore_p->writeFormattedRecord(*data, &info);
    if (rc != 0) {
        // Replica could not write the replicated record to its file set (out
        // of space / unavailable).  The entry is not appended, so this replica
        // will diverge from the leader; surface it rather than stalling
        // mutely.
        BALL_LOG_ERROR
            << "PartitionRaftLog: failed to write replicated record "
            << "at index " << (lastIndex() + 1) << ", rc=" << rc;
        return rc;
    }

    // 'writeFormattedRecord' fills the physical metadata; the Raft layer owns
    // the primary lease id (term), which 'term()' reads back for the log
    // consistency check.  Without this the entry reads as term 0 and every
    // subsequent AppendEntries fails the prevLogTerm check and truncates.
    info.d_primaryLeaseId = term;
    d_index.push_back(info);

    // Cached, so the commit that follows applies these bytes rather than
    // reading back what was just written.  'data' is self-consistent: its
    // offsets describe the leader's files and the payload it carries, as the
    // record written above describes this node's.  Neither reader needs this
    // node's -- apply locates the record through its handle, and a peer served
    // this entry frames it by the fields it arrives with.  A term change or a
    // gap restarts the window, dropping a prior leader stint.
    cacheEntry(lastIndex(), term, data);

    // The physical rollover for an appended 'e_ROLLOVER' now happens via the
    // apply hook ('applyCommittedEntryAsReplica') when it commits, not eagerly
    // at append time.

    return 0;
}

int PartitionRaftLog::truncateFrom(bsls::Types::Uint64 index)
{
    // 'truncateFrom' is only ever invoked from 'RaftNode::handleAppendEntries'
    // when this node's own logged suffix conflicts with the (new) leader's.
    // If this node was itself leader until this very same message, it still
    // tracks the writes behind the entries erased below.  Those stay: their
    // producers are attached and no entry will ever carry them again, so the
    // owner takes them off 'dispatchOutput' and re-posts them, clearing the
    // handle and the entry blob as it does.  The writes below 'index' keep
    // both, along with their provenance: their entries survive this
    // truncation and can still commit.
    //
    // Only the buffered writes' records go here, because they must: a
    // placeholder carries offset 0 and the highest key, so 'truncateRecords'
    // would stop on it and erase nothing.
    releaseBufferedRecords();

    if (index <= d_snapshotIndex || index > lastIndex()) {
        return -1;
    }

    // Each entry caches the data- and qlist-file positions as of that entry,
    // so the first truncated entry already carries the exact offsets to roll
    // both files back to -- even when it carries no payload of its own (its
    // offsets then point at the next payload / file end).  No scan needed.
    bsls::Types::Uint64 vectorIdx     = index - d_frontIndex;
    bsls::Types::Uint64 journalOffset = d_index[vectorIdx].d_journalOffset;
    bsls::Types::Uint64 dataOffset    = d_index[vectorIdx].d_dataOffset;
    bsls::Types::Uint64 qlistOffset   = d_index[vectorIdx].d_qlistOffset;

    BALL_LOG_WARN << "Raft log truncation from index " << index
                  << " (lastIndex=" << lastIndex()
                  << ", journalOffset=" << journalOffset
                  << ", dataOffset=" << dataOffset
                  << ", qlistOffset=" << qlistOffset << "). Removing "
                  << (d_index.size() - vectorIdx) << " entries.";

    // Before the files are touched: 'truncateJournal' zeroes the removed
    // range, so a cached blob left aliasing it would read as zeros.
    dropCacheFrom(index);

    d_fileStore_p->truncateRecords(journalOffset);

    int rc = d_fileStore_p->truncateJournal(journalOffset);
    if (rc != 0) {
        return rc;
    }

    if (dataOffset > 0) {
        rc = d_fileStore_p->truncateData(dataOffset);
        if (rc != 0) {
            return rc;
        }
    }

    if (qlistOffset > 0) {
        rc = d_fileStore_p->truncateQlist(qlistOffset);
        if (rc != 0) {
            return rc;
        }
    }

    d_index.erase(d_index.begin() + vectorIdx, d_index.end());

    return 0;
}

void PartitionRaftLog::cacheEntry(bsls::Types::Uint64 index,
                                  bsls::Types::Uint64 term,
                                  const EntryBlobSp&  blob)
{
    if (!blob) {
        // Nothing to serve later; a gap would make the window non-contiguous,
        // so drop what is held and restart the window past this entry.
        clearCache();
        return;  // RETURN
    }

    if (!d_cache.empty() &&
        (index != d_cacheBase + d_cache.size() || term != d_cacheTerm)) {
        // The window has to stay contiguous, and single-term, for what
        // 'entries()' reads off it to hold.  Restart it rather than assert.
        clearCache();
    }
    if (d_cache.empty()) {
        d_cacheBase = index;
        d_cacheTerm = term;
    }

    d_cache.push_back(blob);
    d_cacheBytes += blob->length();

    while (d_cache.size() > k_MAX_CACHED_ENTRIES ||
           (d_cacheBytes > k_MAX_CACHED_BYTES && d_cache.size() > 1)) {
        d_cacheBytes -= d_cache.front()->length();
        d_cache.pop_front();
        ++d_cacheBase;
    }
}

void PartitionRaftLog::dropCacheFrom(bsls::Types::Uint64 index)
{
    if (d_cache.empty() || index >= d_cacheBase + d_cache.size()) {
        return;  // RETURN
    }
    if (index <= d_cacheBase) {
        clearCache();
        return;  // RETURN
    }

    while (d_cacheBase + d_cache.size() > index) {
        d_cacheBytes -= d_cache.back()->length();
        d_cache.pop_back();
    }
}

void PartitionRaftLog::dropCacheThrough(bsls::Types::Uint64 index)
{
    while (!d_cache.empty() && d_cacheBase < index) {
        d_cacheBytes -= d_cache.front()->length();
        d_cache.pop_front();
        ++d_cacheBase;
    }
}

bsls::Types::Uint64
PartitionRaftLog::journalOffsetAt(bsls::Types::Uint64 index) const
{
    BSLS_ASSERT_SAFE(index > d_snapshotIndex && index <= lastIndex());
    BSLS_ASSERT_SAFE(index >= d_firstRecordIndex);

    return expectedJournalOffset(index);
}

bsls::Types::Uint64
PartitionRaftLog::expectedJournalOffset(bsls::Types::Uint64 index) const
{
    return d_firstRecordOffset +
           (index - d_firstRecordIndex) *
               mqbs::FileStoreProtocol::k_JOURNAL_RECORD_SIZE;
}

int PartitionRaftLog::verifyJournalOffset(bsls::Types::Uint64 index,
                                          bsls::Types::Uint64 actualOffset)
{
    const bsls::Types::Uint64 expected = expectedJournalOffset(index);
    if (expected == actualOffset) {
        return 0;  // RETURN
    }

    // Every offset derived from here on would read a neighbouring record, so
    // stop instead of writing on: mirrors the rollover-failure behavior.
    BMQTSK_ALARMLOG_PANIC("PARTITION_OFFSET_ANCHOR")
        << d_fileStore_p->partitionDesc() << "Journal record at index "
        << index << " landed at offset " << actualOffset << ", expected "
        << expected << " [firstRecordIndex: " << d_firstRecordIndex
        << ", firstRecordOffset: " << d_firstRecordOffset
        << ", frontIndex: " << d_frontIndex
        << ", indexSize: " << d_index.size()
        << ", snapshotIndex: " << d_snapshotIndex
        << ", journalPosition: " << d_fileStore_p->journalPosition()
        << "]. Partition left unavailable." << BMQTSK_ALARMLOG_END;

    d_fileStore_p->setAvailabilityStatus(false);
    return -1;
}

void PartitionRaftLog::trimFrontThrough(bsls::Types::Uint64 index)
{
    while (!d_index.empty() && d_frontIndex <= index) {
        d_index.pop_front();
        ++d_frontIndex;
    }
}

void PartitionRaftLog::clearCache()
{
    d_cache.clear();
    d_cacheBase  = 0;
    d_cacheTerm  = 0;
    d_cacheBytes = 0;
}

void PartitionRaftLog::rollover(bsls::Types::Uint64 rolloverIndex)
{
    BSLS_ASSERT_SAFE(rolloverIndex > d_snapshotIndex &&
                     rolloverIndex <= lastIndex());

    // Number of 'd_index' entries up to and including 'e_ROLLOVER'.  Normally
    // 'e_ROLLOVER' is the last log entry (window writes buffer behind it), but
    // a leadership change can leave entries after it: a new leader must keep
    // the inherited, still-uncommitted 'e_ROLLOVER' and commit it via a
    // current-term entry appended above it (the become-leader sync point).
    // Leader-side rollover-pending buffering (see
    // 'PartitionRaft::proposeDeferredSyncPoint') bounds those to journal-op
    // sync points, which are rewritten into the new file below.
    const bsls::Types::Uint64 prefixCount = rolloverIndex - d_frontIndex + 1;
    BSLS_ASSERT_SAFE(prefixCount >= 1 && prefixCount <= d_index.size());

    // The 'e_ROLLOVER' log entry.  Capture its old-file journal offset (for
    // marker (i) and the timestamp) and its term (the new snapshot boundary
    // term) before 'd_index'/'d_snapshotIndex' change below.
    const EntryInfo& e = d_index[prefixCount - 1];
    BSLS_ASSERT_SAFE(e.d_recordType == mqbs::RecordType::e_JOURNAL_OP);

    const bsls::Types::Uint64 eRolloverOldOffset = e.d_journalOffset;
    const bsls::Types::Uint64 newSnapshotTerm    = e.d_primaryLeaseId;

    // Read the marker timestamp from the 'e_ROLLOVER' record itself (its
    // old-file journal offset), matching the legacy marker semantics.
    const bsls::Types::Uint64 timestamp = d_fileStore_p->journalOpTimestampAt(
        eRolloverOldOffset);

    // Track system stats across the rollover, mirroring legacy 'rolloverImpl'.
    // ('prepareRolloverFileSet' logs the "Initiating rollover" line.)
    mqbstat::StatMonitorSnapshotRecorder statRecorder(
        d_fileStore_p->partitionDesc(),
        d_allocator_p);

    // Prepare the new file set (FileStore creates the files and headers).
    mqbs::FileStore::FileSetSp newFileSetSp;
    int rc = d_fileStore_p->prepareRolloverFileSet(&newFileSetSp);
    if (0 != rc) {
        // Nothing has been mutated yet ('d_index'/'d_snapshotIndex' are still
        // consistent with the committed log), so it is safe to bail out here.
        // Matching legacy's "mark unavailable, alarm, no automated recovery"
        // rollover-failure behavior: this node is now stuck on this partition
        // (disabled for further local writes) until manually fixed and
        // restarted, while the rest of the Raft cluster continues -- 'commit'
        // + 'apply' bookkeeping does not retry a failed local apply.
        BALL_LOG_ERROR << "PartitionRaftLog: failed to prepare rollover file "
                       << "set at index " << rolloverIndex << ", rc: " << rc
                       << ". Partition left unavailable.";
        d_fileStore_p->setAvailabilityStatus(false);
        return;  // RETURN
    }
    mqbs::FileSet* newFileSet = newFileSetSp.get();

    // Compact every live record with sequence number at most 'rolloverIndex'
    // into the new file.  'e_ROLLOVER' is a journal-op and is naturally
    // excluded (journal-ops never appear in 'd_records').
    mqbs::FileStore::QueueKeyCounterMap queueKeyCounterMap;
    d_fileStore_p->writeRolledOverRecords(newFileSet,
                                          &queueKeyCounterMap,
                                          rolloverIndex);

    // Marker (i): built from 'e_ROLLOVER' at its old-file offset.
    d_fileStore_p->writeFirstSyncPointAfterRollover(newFileSet,
                                                    eRolloverOldOffset,
                                                    timestamp);

    // Rewrite the log entries above 'e_ROLLOVER' into the new file,
    // in strict index order, right after the marker: exactly where a normal
    // post-rollover append would land, so recovery re-indexes them identically
    // and a node that rolled over *before* receiving these entries (appending
    // them normally afterwards) produces a byte-identical file.
    //
    // Two independent sources leave such entries, and both must be relocated:
    //  - Leadership change: committing an inherited prior-term 'e_ROLLOVER'
    //    requires a current-term entry (the new leader's become-leader sync
    //    point) committed above it -- a journal-op.
    //  - A follower applying a batched 'appendEntries' that carried committed
    //    regular records (e.g. QueueOp app add/remove, or a message DELETION)
    //    after 'e_ROLLOVER' before this node applied the rollover.
    // Each is copied verbatim from the old (front, not-yet-archived) file set.
    // Route by handle validity, not record type: 'writeFormattedRecord' only
    // inserts a 'd_records'/handle entry for types it tracks going forward
    // (MESSAGE, QUEUE_OP, CONFIRM) -- 'e_JOURNAL_OP' and 'e_DELETION' are both
    // untracked (fixed-size, no data/qlist payload, nothing to reference
    // afterwards) and so never have one.
    for (bsls::Types::Uint64 i = prefixCount; i < d_index.size(); ++i) {
        EntryInfo& entry = d_index[i];

        if (!entry.d_handle.isValid()) {
            entry.d_journalOffset =
                d_fileStore_p->writeRolledOverUntrackedRecord(
                    newFileSet,
                    entry.d_journalOffset);
            // Neither type writes data/qlist; their truncation anchors are the
            // new file's current ends (mirrors 'formatSyncPointRecord' /
            // 'formatDeletionRecord').
            entry.d_dataOffset  = newFileSet->d_data.d_filePosition;
            entry.d_qlistOffset = newFileSet->d_qlist.d_filePosition;
        }
        else {
            // A tracked record.  Its truncation anchors are the new file's
            // current ends captured BEFORE the copy -- which are the payload
            // starts for MESSAGE (data) and QUEUE_OP CREATION/ADDITION
            // (qlist).
            entry.d_journalOffset = newFileSet->d_journal.d_filePosition;
            entry.d_dataOffset    = newFileSet->d_data.d_filePosition;
            entry.d_qlistOffset   = newFileSet->d_qlist.d_filePosition;

            d_fileStore_p->writeRolledOverRecord(entry.d_handle,
                                                 &queueKeyCounterMap,
                                                 newFileSet);
        }
    }

    // Summarize afterwards so the rewritten records are counted too.
    d_fileStore_p->logRolloverQueueSummary(queueKeyCounterMap);

    BALL_LOG_INFO_BLOCK
    {
        statRecorder.print(BALL_LOG_OUTPUT_STREAM,
                           "ROLLOVER - STEP 1 (COMPACTION)");
    }

    // Before the old file set is finalized, not after: a cached blob aliasing
    // it keeps 'numReferences()' above one, which sends
    // 'finalizeRolloverFileSet' down its deferred branch -- the old set stays
    // in 'd_fileSets' until some later 'FileStore::gc', by which time the
    // dispatcher may be stopped.  Nothing below reads the cache.
    clearCache();

    // Truncate/gc the old file set, swap the new one in, schedule archive.
    d_fileStore_p->finalizeRolloverFileSet(newFileSetSp);

    BALL_LOG_INFO_BLOCK
    {
        statRecorder.print(BALL_LOG_OUTPUT_STREAM, "ROLLOVER COMPLETE");
    }

    d_fileStore_p->onRolloverComplete(statRecorder.totalElapsed());

    // Drop the compacted prefix (up to and including 'e_ROLLOVER') and advance
    // the snapshot boundary; what was rewritten above, if any, stays in
    // 'd_index' as the live log now anchored on the new file.  The compacted
    // prefix lives in the rolled-over file and is served to lagging peers via
    // snapshot (they request 'rolloverIndex' via InstallSnapshot).
    d_index.erase(d_index.begin(), d_index.begin() + prefixCount);
    d_snapshotIndex = rolloverIndex;
    d_snapshotTerm  = newSnapshotTerm;

    // Re-anchor on the new file set; the entries above 'e_ROLLOVER' were
    // rewritten into it with new offsets by the loop above.
    d_frontIndex        = rolloverIndex + 1;
    d_firstRecordIndex  = rolloverIndex + 1;
    d_firstRecordOffset = d_index.empty() ? d_fileStore_p->journalPosition()
                                          : d_index.front().d_journalOffset;

    BALL_LOG_INFO
        << "PartitionRaftLog::rollover: re-anchored at rolloverIndex="
        << rolloverIndex << ", firstRecordIndex=" << d_firstRecordIndex
        << ", firstRecordOffset=" << d_firstRecordOffset
        << ", journalPosition=" << d_fileStore_p->journalPosition()
        << ", indexSize=" << d_index.size();

    // Pop the 'e_ROLLOVER' pending write only if *this* node proposed it (the
    // same-leader case): it is then the sole appended pending write, handled
    // here rather than by 'applyCommittedEntryAsPrimary', so removing it now
    // keeps 'takePendingWrites' (drain) from replaying it into the new file
    // set.  A node that inherited 'e_ROLLOVER' from a prior leader has no such
    // pending write; its front appended pending write, if any, sits above
    // 'rolloverIndex' (a become-leader sync point) and must survive
    // for its own 'applyCommittedEntryAsPrimary'.  On the replica path
    // 'd_pendingWrites' is empty ('d_appendedCount == 0'), so this is skipped.
    if (0 < d_appendedCount &&
        d_pendingWrites.front()->d_sequenceNumber == rolloverIndex) {
        BSLS_ASSERT_SAFE(d_pendingWrites.front()->d_syncPointType ==
                         mqbs::SyncPointType::e_ROLLOVER);

        d_pendingWrites.pop_front();
        d_appendedCount--;
    }
}

void PartitionRaftLog::applyCommittedEntryAsPrimary(
    bsls::Types::Uint64 index,
    bsls::Types::Int64  commitTimepoint)
{
    // Callers route here on 'isOwnAppendedEntry', so the front appended write
    // is this entry's.
    if (!isOwnAppendedEntry(index)) {
        return;  // RETURN
    }

    BSLS_ASSERT_SAFE(d_pendingWrites.front()->d_sequenceNumber == index);

    const bsl::shared_ptr<PendingWrite>& pw = d_pendingWrites.front();

    d_fileStore_p->onRecordCommittedPrimary(*pw, commitTimepoint);

    // A peer that has not acked this entry can still ask for it once a quorum
    // has committed it, so keep what serves it before dropping the rest.
    cacheEntry(index, pw->d_primaryLeaseId, pw->d_entryBlob);

    // Remove the committed write from the buffer (it was kept until apply).
    d_pendingWrites.pop_front();
    d_appendedCount--;

    trimFrontThrough(index);
}

void PartitionRaftLog::applyCommittedEntryAsReplica(bsls::Types::Uint64 index,
                                                    const bdlbb::Blob&  data)
{
    // Never this node's own appended entry: that would re-insert a handle to a
    // record propose time may already have erased.  Callers route those to
    // 'applyCommittedEntryAsPrimary'.
    BSLS_ASSERT_SAFE(!isOwnAppendedEntry(index));

    const EntryInfo& entryInfo = d_index[index - d_frontIndex];

    const mqbs::DataStoreRecordHandle handle = entryInfo.d_handle;

    d_fileStore_p->onRecordCommittedReplica(data, handle);

    trimFrontThrough(index);

    // What this node cached as a replica serves no retransmit -- only a leader
    // sends -- so the window keeps the entries still awaiting apply, and
    // 'index', which the next AppendEntries asks 'term' for as its
    // 'prevLogIndex'.  Nothing below.
    dropCacheThrough(index);
}

// ACCESSORS
bsls::Types::Uint64 PartitionRaftLog::lastIndex() const
{
    return d_frontIndex + d_index.size() - 1;
}

bsls::Types::Uint64 PartitionRaftLog::lastTerm() const
{
    // 'd_index' is empty once every entry has been applied, so this cannot
    // read its back; 'term' resolves the last index against whichever source
    // still holds it.
    return term(lastIndex());
}

bsls::Types::Uint64 PartitionRaftLog::term(bsls::Types::Uint64 index) const
{
    if (index == 0) {
        return 0;
    }
    if (index == d_snapshotIndex) {
        return d_snapshotTerm;
    }
    if (index <= d_snapshotIndex || index > lastIndex()) {
        return 0;
    }

    if (index >= d_frontIndex) {
        return d_index[index - d_frontIndex].d_primaryLeaseId;
    }

    // Applied entries leave 'd_index' for 'd_cache', whose window is
    // single-term.
    if (!d_cache.empty() && index >= d_cacheBase &&
        index < d_cacheBase + d_cache.size()) {
        return d_cacheTerm;
    }

    // Held by neither, so whoever wants this entry is reading it from mmap
    // anyway -- unless there is no file set to read: the store is closed for
    // the whole of an InstallSnapshot transfer, and at shutdown.  0 is what
    // this returns for any index it cannot answer for.
    if (!d_fileStore_p->isOpen()) {
        return 0;  // RETURN
    }

    return d_fileStore_p->recordTermAt(journalOffsetAt(index));
}

void PartitionRaftLog::entries(bsls::Types::Uint64    lo,
                               bsls::Types::Uint64    hi,
                               bsl::vector<LogEntry>* out,
                               bsls::Types::Uint64    maxCount,
                               bsls::Types::Uint64    maxBytes,
                               bool                   forApply) const
{
    BSLS_ASSERT_SAFE(out);
    BSLS_ASSERT_SAFE(lo <= hi);
    BSLS_ASSERT_SAFE(lo > d_snapshotIndex);
    BSLS_ASSERT_SAFE(hi <= lastIndex() + 1);

    const bsl::vector<LogEntry>::size_type loaded = out->size();
    bsls::Types::Uint64                    bytes  = 0;

    // Index of the oldest write this node appended and has yet to apply; the
    // appended ones run from there through 'lastIndex()'.
    const bsls::Types::Uint64 pendingBase =
        0 < d_appendedCount ? d_pendingWrites.front()->d_sequenceNumber : 0;

    for (bsls::Types::Uint64 i = lo; i < hi; ++i) {
        if (forApply && isOwnAppendedEntry(i)) {
            // 'applyCommittedEntryAsPrimary' routes on the same predicate and
            // reads nothing from the entry but its index -- propose time
            // already did the storage-side work, and the write itself still
            // holds what apply needs.  A committed 'e_ROLLOVER' likewise takes
            // 'rollover()', which reads the log, not the entry.
            out->push_back(LogEntry(term(i), i, EntryBlobSp()));
        }
        else if (0 < d_appendedCount && i >= pendingBase &&
                 i < pendingBase + d_appendedCount &&
                 d_pendingWrites[i - pendingBase]->d_entryBlob) {
            // Appended here and not yet committed: the write still holds both
            // the blob and the term.
            const PendingWrite& pw = *d_pendingWrites[i - pendingBase];
            out->push_back(LogEntry(pw.d_primaryLeaseId, i, pw.d_entryBlob));
            bytes += pw.d_entryBlob->length();
        }
        else if (!d_cache.empty() && i >= d_cacheBase &&
                 i < d_cacheBase + d_cache.size()) {
            const EntryBlobSp& cached = d_cache[i - d_cacheBase];
            out->push_back(LogEntry(d_cacheTerm, i, cached));
            bytes += cached->length();
        }
        else {
            // Held by neither: read the record, and its term, from mmap.
            const bsls::Types::Uint64 recordOffset = journalOffsetAt(i);

            bsl::shared_ptr<bdlbb::Blob> entryBlob;
            bsls::Types::Uint64          entryTerm = 0;
            int rc = d_fileStore_p->readRecord(&entryBlob,
                                               recordOffset,
                                               &entryTerm);
            if (rc != 0) {
                // The caller sees a short range and stops there.
                BALL_LOG_ERROR << "Failed to read log entry " << i
                               << " at journal offset " << recordOffset
                               << ", rc=" << rc;
                break;  // BREAK
            }

            out->push_back(LogEntry(entryTerm, i, entryBlob));
            bytes += entryBlob->length();
        }

        // Checked after the append, so one entry is always loaded even when
        // it alone exceeds a cap -- otherwise it could never be replicated.
        if ((maxCount != 0 && out->size() - loaded >= maxCount) ||
            (maxBytes != 0 && bytes >= maxBytes)) {
            break;
        }
    }
}

bsls::Types::Uint64 PartitionRaftLog::snapshotIndex() const
{
    return d_snapshotIndex;
}

bsls::Types::Uint64 PartitionRaftLog::snapshotTerm() const
{
    return d_snapshotTerm;
}

bool PartitionRaftLog::isOwnAppendedEntry(bsls::Types::Uint64 index) const
{
    // The appended writes sit at the front, at contiguous ascending indices,
    // and are popped in commit order, so the front's sequence number is the
    // lowest index this node still owns and the range spans 'd_appendedCount'.
    // The upper bound is not 'lastIndex()': once leadership moves, the new
    // leader's entries are appended above this node's own appended writes, and
    // those are applied as replica.
    if (0 == d_appendedCount) {
        return false;  // RETURN
    }

    const bsls::Types::Uint64 base = d_pendingWrites.front()->d_sequenceNumber;

    return base <= index && index < base + d_appendedCount;
}

bool PartitionRaftLog::isRollover(bsls::Types::Uint64 index) const
{
    BSLS_ASSERT_SAFE(index >= d_frontIndex && index <= lastIndex());

    // Both fields are in the index entry, so this reads no journal.  The '&&'
    // order matters: the sync-point type is only meaningful on a journal-op.
    const EntryInfo& entry = d_index[index - d_frontIndex];

    return entry.d_recordType == mqbs::RecordType::e_JOURNAL_OP &&
           entry.d_syncPointType == mqbs::SyncPointType::e_ROLLOVER;
}

bool PartitionRaftLog::hasUncommittedRollover(
    bsls::Types::Uint64 commitIndex) const
{
    // Scan the uncommitted suffix '(commitIndex, lastIndex]' for an
    // 'e_ROLLOVER' journal-op.  A new leader must buffer purely on the
    // presence of an uncommitted 'e_ROLLOVER' -- it is a committed cluster
    // decision that will roll over regardless of this node's own rollover
    // configuration, so 'rolloverIfNeeded' cannot be relied on to re-trigger
    // it.
    BSLS_ASSERT_SAFE(commitIndex >= d_snapshotIndex);
    for (bsls::Types::Uint64 i = commitIndex + 1; i <= lastIndex(); ++i) {
        if (isRollover(i)) {
            return true;  // RETURN
        }
    }
    return false;
}

}  // close package namespace
}  // close enterprise namespace
