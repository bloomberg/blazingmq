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
    // 'e_ROLLOVER' index (N); a write buffered at position 'k' (== current
    // buffer size) will receive log index 'N + 1 + k' when drained in FIFO
    // order.
    const bsls::Types::Uint64 futureIndex = lastIndex() + 1 +
                                            d_pendingWrites.size();

    // Reserve a placeholder 'd_records' entry keyed by the future
    // '(term, index)' so callers get a valid handle now; the physical offsets
    // are patched in when this write drains.  Record types that produce no
    // handle (message-deletion) are skipped -- their write method ignores the
    // handle, matching the normal path.
    const bool producesHandle = pw->d_recordType !=
                                mqbs::RecordType::e_DELETION;
    if (producesHandle) {
        d_fileStore_p->reservePendingRecord(&pw->d_handle,
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
        BSLS_ASSERT_SAFE(pw->d_attributes_p);
        pw->d_ownedAttributes = *pw->d_attributes_p;
        pw->d_attributes_p    = &pw->d_ownedAttributes;
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
                  << " buffered write(s) (no longer leader).";

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
            return rc;
        }

        d_index.push_back(EntryInfo(pw.d_sequenceNumber,
                                    term,
                                    pw.d_journalOffset,
                                    pw.d_dataOffset,
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
        return rc;
    }

    // 'writeFormattedRecord' fills the physical metadata; the Raft layer owns
    // the sequence number (index) and primary lease id (term).
    info.d_sequenceNum    = lastIndex() + 1;
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

    bsls::Types::Uint64 vectorIdx     = index - d_snapshotIndex - 1;
    bsls::Types::Uint64 journalOffset = d_index[vectorIdx].d_journalOffset;
    bsls::Types::Uint64 dataOffset    = d_index[vectorIdx].d_dataOffset;

    BALL_LOG_WARN << "Raft log truncation from index " << index
                  << " (lastIndex=" << lastIndex()
                  << ", journalOffset=" << journalOffset
                  << ", dataOffset=" << dataOffset << "). Removing "
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

    // Number of 'd_index' entries up to and including 'e_ROLLOVER'.  Because
    // window writes NACK in phase 1, 'e_ROLLOVER' is the last log entry, so
    // there is no tail: 'prefixCount' must span the whole index.
    const bsls::Types::Uint64 prefixCount = rolloverIndex - d_snapshotIndex;
    BSLS_ASSERT_SAFE(prefixCount == d_index.size());

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

    // Truncate/gc the old file set, swap the new one in, schedule archive.
    d_fileStore_p->finalizeRolloverFileSet(newFileSetSp);

    // Empty 'd_index' and advance the snapshot boundary: the committed prefix
    // now lives compacted in the rolled-over file and is served to lagging
    // peers via snapshot (they request 'rolloverIndex' via InstallSnapshot).
    d_index.erase(d_index.begin(), d_index.begin() + prefixCount);
    d_snapshotIndex = rolloverIndex;
    d_snapshotTerm  = newSnapshotTerm;

    clearCache();

    // On the primary, the committed 'e_ROLLOVER' is a pending write that was
    // appended but is handled here rather than by
    // 'applyCommittedEntryAsPrimary', so it is never popped there.  Remove it
    // now to keep 'takePendingWrites' (drain) from replaying it into the new
    // file set.  It is the sole appended pending write at this point (all
    // earlier entries were applied and popped before it in commit order, and
    // window writes are buffered behind it).  On the replica path
    // 'd_pendingWrites' is empty ('d_appendedCount == 0'), so this is skipped.
    if (0 < d_appendedCount) {
        BSLS_ASSERT_SAFE(1 == d_appendedCount);
        BSLS_ASSERT_SAFE(d_pendingWrites.front()->d_syncPointType ==
                         mqbs::SyncPointType::e_ROLLOVER);

        d_pendingWrites.pop_front();
        d_appendedCount--;
    }
}

void PartitionRaftLog::applyCommittedEntryAsPrimary(bsls::Types::Uint64 index)
{
    BSLS_ASSERT_SAFE(!d_pendingWrites.empty());
    BSLS_ASSERT_SAFE(0 < d_appendedCount);
    BSLS_ASSERT_SAFE(d_pendingWrites.front()->d_sequenceNumber == index);
    (void)index;

    d_fileStore_p->onRecordCommittedPrimary(*d_pendingWrites.front());

    // Remove the committed write from the buffer (it was kept until apply).
    d_pendingWrites.pop_front();
    d_appendedCount--;
}

void PartitionRaftLog::applyCommittedEntryAsReplica(bsls::Types::Uint64 index,
                                                    const bdlbb::Blob&  data)
{
    const EntryInfo& entryInfo = d_index[index - d_snapshotIndex - 1];

    // TODO (raft): do we need entryInfo.d_sequenceNum?
    BSLS_ASSERT_SAFE(entryInfo.d_sequenceNum == index);

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

    // TODO (raft): do we need entryInfo.d_sequenceNum?
    BSLS_ASSERT_SAFE(entryInfo.d_sequenceNum == index);

    const mqbs::RecordType::Enum    recordType    = entryInfo.d_recordType;
    const mqbs::SyncPointType::Enum syncPointType = entryInfo.d_syncPointType;

    return (recordType == mqbs::RecordType::e_JOURNAL_OP &&
            syncPointType == mqbs::SyncPointType::e_ROLLOVER);
}

bool PartitionRaftLog::hasPendingWrites() const
{
    return !d_pendingWrites.empty();
}

}  // close package namespace
}  // close enterprise namespace
