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

// BDE
#include <ball_log.h>
#include <bdlbb_blob.h>
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
, d_recoveredLastIndex(0)
, d_pendingWrites(d_allocator_p)
, d_cached()
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
    int rc = d_fileStore_p->openForRaft(&d_index);
    if (rc != 0) {
        return rc;
    }

    const bmqp_ctrlmsg::PartitionSequenceNumber& snapshotPSN =
        d_fileStore_p->firstSyncPointAfterRolloverSeqNum();
    d_snapshotIndex = snapshotPSN.sequenceNumber();
    d_snapshotTerm  = snapshotPSN.primaryLeaseId();

    // Everything up to and including 'lastIndex()' at this point was just
    // recovered from local disk and already materialized into
    // 'ReplicatedStorage' by 'StorageUtil::recoveredQueuesCb'.
    // 'applyCommittedEntryAsReplica' uses this watermark to avoid replaying
    // it a second time.
    d_recoveredLastIndex = lastIndex();

    BALL_LOG_INFO << "PartitionRaftLog::open: recovered " << d_index.size()
                  << " entries, snapshotIndex=" << d_snapshotIndex
                  << ", snapshotTerm=" << d_snapshotTerm
                  << ", lastIndex=" << lastIndex()
                  << ", lastTerm=" << lastTerm();

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

    // Deep-copy the borrowed attributes into the pooled entry and repoint its
    // pointer at the owned copy: 'd_attributes_p' is only valid for the
    // duration of the synchronous write call, but the buffered entry outlives
    // it (until drain).  The object is stable (shared, never copied out of the
    // deque), so this is done once, before the push_back -- no post-push
    // repoint dance is needed.
    if (pw->d_recordType == mqbs::RecordType::e_MESSAGE) {
    }

    d_pendingWrites.push_back(pw);

    return rc_SUCCESS;
}

void PartitionRaftLog::takePendingWrites(
    bsl::deque<bsl::shared_ptr<PendingWrite> >* out)
{
    BSLS_ASSERT_SAFE(out);
    // 'swap' is O(1) and hands over the shared_ptrs, so the pooled objects
    // (and their 'd_attributes_p'->'d_ownedAttributes' self-pointers) stay
    // valid.
    out->swap(d_pendingWrites);
    d_appendedCount = 0;
}

void PartitionRaftLog::dropPendingWrites()
{
    if (d_pendingWrites.empty()) {
        return;  // RETURN
    }

    BALL_LOG_WARN << "PartitionRaftLog: dropping " << d_pendingWrites.size()
                  << " buffered write(s) (no longer leader, or shutting "
                  << "down).";

    for (bsl::deque<bsl::shared_ptr<PendingWrite> >::iterator it =
             d_pendingWrites.begin();
         it != d_pendingWrites.end();
         ++it) {
        const bsl::shared_ptr<PendingWrite>& sp = *it;
        if (sp->d_handle.isValid()) {
            d_fileStore_p->dropPendingRecord(sp->d_handle);
        }
    }

    d_pendingWrites.clear();
    d_appendedCount = 0;
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

    // Handle claims a future index beyond the buffer — bug if true.
    BSLS_ASSERT_SAFE(handle.sequenceNum() <
                     firstSeqNum + d_pendingWrites.size());

    bsls::Types::Uint64 pos        = handle.sequenceNum() - firstSeqNum;
    d_pendingWrites[pos]->d_handle = mqbs::DataStoreRecordHandle();
}

int PartitionRaftLog::append(bsls::Types::Uint64                 term,
                             const bsl::shared_ptr<bdlbb::Blob>& data,
                             bsls::Types::Uint64                 id)
{
    if (id != 0) {
        // Primary path: format directly in mmap via PendingWrite.
        BSLS_ASSERT_SAFE(!d_pendingWrites.empty());
        BSLS_ASSERT_SAFE(d_appendedCount < d_pendingWrites.size());
        PendingWrite& pw = *d_pendingWrites[d_appendedCount];
        BSLS_ASSERT_SAFE(id == pw.d_id);

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
                           << ") for id=" << id << " at index "
                           << pw.d_sequenceNumber << ", rc=" << rc;
            return rc;
        }

        d_index.push_back(EntryInfo(term,
                                    pw.d_journalOffset,
                                    pw.d_dataOffset,
                                    pw.d_qlistOffset,
                                    pw.d_recordType,
                                    pw.d_handle,
                                    pw.d_syncPointType));

        // Retain the just-appended write as the single-entry cache, so
        // 'entries()' can serve its 'd_entryBlob' to peers without re-reading
        // mmap.  It is a pooled shared_ptr, so retaining it is cheap; released
        // by 'clearCache()' back to the pool.
        d_cached = d_pendingWrites[d_appendedCount];
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

    // No cache on the replica path: 'entries()' here serves the follower apply
    // path, where commit lags append so the fetched entry is rarely the most
    // recently appended one -- it reads from mmap ('readRecord') instead.
    // Invalidate any stale cache left from a prior leader stint.
    d_cached.reset();

    // The physical rollover for an appended 'e_ROLLOVER' now happens via the
    // apply hook ('applyCommittedEntryAsReplica') when it commits, not eagerly
    // at append time.

    return 0;
}

int PartitionRaftLog::truncateFrom(bsls::Types::Uint64 index)
{
    // truncateFrom only happens after drop (leadership loss), which clears
    // d_pendingWrites and d_appendedCount. Assert this invariant.
    BSLS_ASSERT(d_pendingWrites.empty());
    BSLS_ASSERT(0 == d_appendedCount);

    if (index <= d_snapshotIndex || index > lastIndex()) {
        return -1;
    }

    // Each entry caches the data- and qlist-file positions as of that entry,
    // so the first truncated entry already carries the exact offsets to roll
    // both files back to -- even when it carries no payload of its own (its
    // offsets then point at the next payload / file end).  No scan needed.
    bsls::Types::Uint64 vectorIdx     = index - d_snapshotIndex - 1;
    bsls::Types::Uint64 journalOffset = d_index[vectorIdx].d_journalOffset;
    bsls::Types::Uint64 dataOffset    = d_index[vectorIdx].d_dataOffset;
    bsls::Types::Uint64 qlistOffset   = d_index[vectorIdx].d_qlistOffset;

    BALL_LOG_WARN << "Raft log truncation from index " << index
                  << " (lastIndex=" << lastIndex()
                  << ", journalOffset=" << journalOffset
                  << ", dataOffset=" << dataOffset
                  << ", qlistOffset=" << qlistOffset << "). Removing "
                  << (d_index.size() - vectorIdx) << " entries.";

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

    clearCache();

    return 0;
}

void PartitionRaftLog::clearCache()
{
    d_cached.reset();
}

void PartitionRaftLog::rollover(bsls::Types::Uint64 rolloverIndex)
{
    BSLS_ASSERT_SAFE(rolloverIndex > d_snapshotIndex &&
                     rolloverIndex <= lastIndex());

    // Number of 'd_index' entries up to and including 'e_ROLLOVER'.  Normally
    // 'e_ROLLOVER' is the last log entry (window writes buffer behind it), but
    // a leadership change can leave a tail after it: a new leader must keep
    // the inherited, still-uncommitted 'e_ROLLOVER' and commit it via a
    // current-term entry appended above it (the become-leader sync point).
    // Leader-side rollover-pending buffering (see
    // 'PartitionRaft::proposeDeferredSyncPoint') bounds that tail to
    // journal-op sync points, which are rewritten into the new file below.
    const bsls::Types::Uint64 prefixCount = rolloverIndex - d_snapshotIndex;
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

    d_fileStore_p->logRolloverQueueSummary(queueKeyCounterMap);

    // Marker (i): built from 'e_ROLLOVER' at its old-file offset.
    d_fileStore_p->writeFirstSyncPointAfterRollover(newFileSet,
                                                    eRolloverOldOffset,
                                                    timestamp);

    // Rewrite the tail -- entries appended after 'e_ROLLOVER' -- into the new
    // file, in strict index order, right after the marker (exactly where a
    // normal post-rollover append would land, so recovery re-indexes them
    // identically).  A tail exists only when a leadership change committed an
    // inherited 'e_ROLLOVER': committing a prior-term 'e_ROLLOVER' requires a
    // current-term entry (the new leader's become-leader sync point) committed
    // above it, so that entry sits above the rollover boundary here.  Leader-
    // side rollover-pending buffering (see
    // 'PartitionRaft::proposeDeferredSyncPoint') keeps client writes out of
    // this window and 'commitIndex == lastIndex' holds at rollover time, so
    // the tail is exactly the committed become-leader sync point(s) -- one per
    // intervening election, all journal-ops.  Copy each verbatim (still
    // reading the old, not-yet-archived front set) and repoint its 'EntryInfo'
    // at the new file; a journal-op produces no handle, so only the offsets
    // move.
    for (bsls::Types::Uint64 i = prefixCount; i < d_index.size(); ++i) {
        EntryInfo& tailEntry = d_index[i];
        BSLS_ASSERT_SAFE(tailEntry.d_recordType ==
                         mqbs::RecordType::e_JOURNAL_OP);

        tailEntry.d_journalOffset =
            d_fileStore_p->writeRolledOverJournalOpRecord(
                newFileSet,
                tailEntry.d_journalOffset);
        // A journal-op writes no data/qlist; its truncation anchors are the
        // new file's current ends (mirrors 'formatSyncPointRecord').
        tailEntry.d_dataOffset  = newFileSet->d_data.d_filePosition;
        tailEntry.d_qlistOffset = newFileSet->d_qlist.d_filePosition;
    }

    BALL_LOG_INFO_BLOCK
    {
        statRecorder.print(BALL_LOG_OUTPUT_STREAM,
                           "ROLLOVER - STEP 1 (COMPACTION)");
    }

    // Truncate/gc the old file set, swap the new one in, schedule archive.
    d_fileStore_p->finalizeRolloverFileSet(newFileSetSp);

    BALL_LOG_INFO_BLOCK
    {
        statRecorder.print(BALL_LOG_OUTPUT_STREAM, "ROLLOVER COMPLETE");
    }

    // Drop the compacted prefix (up to and including 'e_ROLLOVER') and advance
    // the snapshot boundary; the tail rewritten above, if any, stays in
    // 'd_index' as the live log now anchored on the new file.  The compacted
    // prefix lives in the rolled-over file and is served to lagging peers via
    // snapshot (they request 'rolloverIndex' via InstallSnapshot).
    d_index.erase(d_index.begin(), d_index.begin() + prefixCount);
    d_snapshotIndex = rolloverIndex;
    d_snapshotTerm  = newSnapshotTerm;

    clearCache();

    // Pop the 'e_ROLLOVER' pending write only if *this* node proposed it (the
    // same-leader case): it is then the sole appended pending write, handled
    // here rather than by 'applyCommittedEntryAsPrimary', so removing it now
    // keeps 'takePendingWrites' (drain) from replaying it into the new file
    // set.  A node that inherited 'e_ROLLOVER' from a prior leader has no such
    // pending write; its front appended pending write, if any, is a tail entry
    // (a become-leader sync point at 'seq > rolloverIndex') that must survive
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

void PartitionRaftLog::applyCommittedEntryAsPrimary(bsls::Types::Uint64 index)
{
    // Become-leader no-op entries are appended by RaftNode automatically but
    // have no corresponding PendingWrite. Skip them when front pending write's
    // sequence number is greater than the index being applied.
    if (d_pendingWrites.empty() || d_appendedCount == 0) {
        return;  // RETURN
    }

    if (d_pendingWrites.front()->d_sequenceNumber > index) {
        return;  // Skip no-op entry
    }

    BSLS_ASSERT_SAFE(d_pendingWrites.front()->d_sequenceNumber == index);

    d_fileStore_p->onRecordCommittedPrimary(*d_pendingWrites.front());

    // Remove the committed write from the buffer (it was kept until apply).
    d_pendingWrites.pop_front();
    d_appendedCount--;
}

void PartitionRaftLog::applyCommittedEntryAsReplica(bsls::Types::Uint64 index,
                                                    const bdlbb::Blob&  data)
{
    if (index <= d_recoveredLastIndex) {
        // Already materialized eagerly by 'StorageUtil::recoveredQueuesCb' at
        // 'open()' time (ghost-adjusted refcounts included).  Applying it
        // again here would double-process confirms/purges on this replica,
        // and a whole-queue purge could run ahead of 'd_index' entries this
        // replay hasn't reached yet, invalidating their handles.
        return;  // RETURN
    }

    const EntryInfo& entryInfo = d_index[index - d_snapshotIndex - 1];

    const mqbs::DataStoreRecordHandle handle      = entryInfo.d_handle;

    d_fileStore_p->onRecordCommittedReplica(data, handle);
}

void PartitionRaftLog::applySnapshot(bsls::Types::Uint64 lastIncludedIndex,
                                     bsls::Types::Uint64 lastIncludedTerm)
{
    d_snapshotIndex = lastIncludedIndex;
    d_snapshotTerm  = lastIncludedTerm;
    d_index.clear();
    clearCache();
}

// ACCESSORS
bsls::Types::Uint64 PartitionRaftLog::lastIndex() const
{
    return d_snapshotIndex + d_index.size();
}

bsls::Types::Uint64 PartitionRaftLog::lastTerm() const
{
    if (d_index.empty()) {
        return d_snapshotTerm;
    }
    return d_index.back().d_primaryLeaseId;
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

    bsls::Types::Uint64 vectorIdx = index - d_snapshotIndex - 1;
    return d_index[vectorIdx].d_primaryLeaseId;
}

int PartitionRaftLog::entries(bsls::Types::Uint64    lo,
                              bsls::Types::Uint64    hi,
                              bsl::vector<LogEntry>* out) const
{
    BSLS_ASSERT_SAFE(out);

    if (lo > hi || lo <= d_snapshotIndex || hi > lastIndex() + 1) {
        return -1;
    }

    out->clear();

    for (bsls::Types::Uint64 i = lo; i < hi; ++i) {
        bsls::Types::Uint64 vectorIdx = i - d_snapshotIndex - 1;
        bsls::Types::Uint64 entryTerm = d_index[vectorIdx].d_primaryLeaseId;

        if (d_cached && i == d_cached->d_sequenceNumber) {
            out->push_back(LogEntry(entryTerm, i, d_cached->d_entryBlob));
            continue;
        }

        // Cache miss: read from mmap'd files
        bsls::Types::Uint64 journalOffset = d_index[vectorIdx].d_journalOffset;
        bsl::shared_ptr<bdlbb::Blob> entryBlob;
        int rc = d_fileStore_p->readRecord(&entryBlob, journalOffset);
        if (rc != 0) {
            return rc;
        }

        out->push_back(LogEntry(entryTerm, i, entryBlob));
    }

    return 0;
}

bsls::Types::Uint64 PartitionRaftLog::snapshotIndex() const
{
    return d_snapshotIndex;
}

bsls::Types::Uint64 PartitionRaftLog::snapshotTerm() const
{
    return d_snapshotTerm;
}

bool PartitionRaftLog::isRollover(bsls::Types::Uint64 index) const
{
    const EntryInfo& entryInfo = d_index[index - d_snapshotIndex - 1];

    const mqbs::RecordType::Enum    recordType    = entryInfo.d_recordType;
    const mqbs::SyncPointType::Enum syncPointType = entryInfo.d_syncPointType;

    return (recordType == mqbs::RecordType::e_JOURNAL_OP &&
            syncPointType == mqbs::SyncPointType::e_ROLLOVER);
}

bool PartitionRaftLog::hasUncommittedRollover(
    bsls::Types::Uint64 commitIndex) const
{
    // Scan the uncommitted suffix '(commitIndex, lastIndex]' for an
    // 'e_ROLLOVER' journal-op.  A new leader must buffer purely on the
    // presence of an uncommitted 'e_ROLLOVER' -- it is a committed cluster
    // decision that will roll over regardless of this node's own rollover
    // configuration, so 'rolloverIfNeeded' cannot be relied on to re-trigger
    // it.  'commitIndex >= d_snapshotIndex' always (the snapshot boundary
    // never exceeds commit), so the first uncommitted entry maps to vector
    // index 'commitIndex - d_snapshotIndex'.
    BSLS_ASSERT_SAFE(commitIndex >= d_snapshotIndex);
    for (bsls::Types::Uint64 i = commitIndex - d_snapshotIndex;
         i < d_index.size();
         ++i) {
        if (d_index[i].d_recordType == mqbs::RecordType::e_JOURNAL_OP &&
            d_index[i].d_syncPointType == mqbs::SyncPointType::e_ROLLOVER) {
            return true;  // RETURN
        }
    }
    return false;
}

}  // close package namespace
}  // close enterprise namespace
