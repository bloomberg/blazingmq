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

// mqbraft_partitionraftlog.h -*-C++-*-
#ifndef INCLUDED_MQBRAFT_PARTITIONRAFTLOG
#define INCLUDED_MQBRAFT_PARTITIONRAFTLOG

//@PURPOSE: Provide a RaftLog adapter backed by a partition's journal and data
// files.
//
//@CLASSES:
//  mqbraft::PartitionRaftLog: RaftLog implementation over journal+data files
//
//@DESCRIPTION: This component implements the 'mqbraft::RaftLog' interface
// using a partition's journal and data files as the underlying storage.
// Journal records already carry 'primaryLeaseId' and 'sequenceNumber' which
// map directly to Raft '(term, index)'.  All journal record types (MESSAGE,
// CONFIRM, DELETE, JOURNAL_OP) are Raft log entries.
//
// The 'append()' method delegates physical writes to 'mqbs::FileStore' via
// its 'writeFormattedRecord()' method.  This is the same code path for both
// leader and follower — the blob passed to 'append()' always contains a
// fully-formed entry.
//
// A bounded cache of recently appended entry blobs avoids re-reading them
// from the mmap'd files.  It is populated by 'append()' and served by
// 'entries()', and trims itself; the log invalidates it on truncation and
// rollover, so callers do not manage it.
//
/// Threading
///----------
// This component is NOT thread-safe.  All operations must run on the
// partition's dispatcher thread.

// MQB
#include <mqbraft_raftnode.h>
#include <mqbs_filestore.h>

// BDE
#include <ball_log.h>
#include <bdlbb_blob.h>
#include <bsl_deque.h>
#include <bsl_memory.h>
#include <bsl_vector.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_types.h>

namespace BloombergLP {

namespace mqbs {
class FileStore;
}

namespace mqbraft {

// ======================
// class PartitionRaftLog
// ======================

class PartitionRaftLog : public RaftLog {
  public:
    // TYPES

    /// FIFO of writes on the primary path: appended and awaiting commit, or
    /// -- during a rollover window -- buffered for the drain that follows it.
    typedef bsl::deque<bsl::shared_ptr<mqbs::FileStore::PendingWrite> >
        PendingWrites;

  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBRAFT.PARTITIONRAFTLOG");

    // TYPES
    typedef mqbs::RecoveryRecordInfo      EntryInfo;
    typedef mqbs::FileStore::PendingWrite PendingWrite;
    typedef bsl::shared_ptr<bdlbb::Blob>  EntryBlobSp;

    // PRIVATE CLASS DATA

    /// Bounds on `d_cache`.  Sized to hold more than a peer can fall behind
    /// between acks, so a round of replication is served from memory.
    static const size_t              k_MAX_CACHED_ENTRIES = 4096;
    static const bsls::Types::Uint64 k_MAX_CACHED_BYTES   = 4 * 1024 * 1024;

    // DATA
    mqbs::FileStore*      d_fileStore_p;
    bsl::deque<EntryInfo> d_index;
    bsls::Types::Uint64   d_snapshotIndex;
    bsls::Types::Uint64   d_snapshotTerm;
    bslma::Allocator*     d_allocator_p;

    /// `lastIndex()` as of `open()`, i.e. the last index recovered from local
    /// disk before this process started applying live commits.  Entries at or
    /// below this watermark were already materialized into `ReplicatedStorage`
    /// by `StorageUtil::recoveredQueuesCb` (including its refcount "ghost"
    /// adjustment for apps added after a message).
    /// `applyCommittedEntryAsReplica` uses it to avoid re-applying them a
    /// second time.
    bsls::Types::Uint64 d_recoveredLastIndex;

    /// FIFO of writes to append.  During normal operation this holds at most
    /// one entry (enqueued by `setPendingWrite`, consumed by `append`). During
    /// a rollover window it holds every write buffered by `setPendingWrite`,
    /// drained once the `e_ROLLOVER` commits (see `takePendingWrites`).
    PendingWrites d_pendingWrites;

    /// Entry blobs held in memory, oldest first: the entry at index
    /// `d_cacheBase + i` is `d_cache[i]`.  Two feeds, one per write path.  As
    /// primary, `applyCommittedEntryAsPrimary` hands over the blob of the
    /// `PendingWrite` it is about to drop, so the window picks up where
    /// `d_pendingWrites` leaves off and serves a retransmit to a peer that has
    /// not acked what a quorum already committed.  As replica, `append` keeps
    /// the blob it was sent, so the commit that follows applies it instead of
    /// reading back what it just wrote.  Bounded by `k_MAX_CACHED_ENTRIES` and
    /// `k_MAX_CACHED_BYTES`, and trimmed from the front, the end being what is
    /// read.  The blobs alias what they came from -- the active file set, or
    /// the received event -- rather than copying it, so the bound is on how
    /// much of that is pinned; see `clearCache` for what that costs at close.
    bsl::deque<EntryBlobSp> d_cache;

    /// Log index of `d_index.front()`, or of the next append while `d_index`
    /// is empty.  Starts at `d_snapshotIndex + 1` and rises above it as
    /// applied entries are dropped from the front.
    bsls::Types::Uint64 d_frontIndex;

    /// Log index and journal offset of the first record of the current file
    /// set.  Journal records are a fixed `k_JOURNAL_RECORD_SIZE` apart, so the
    /// offset of any index follows from these two without storing it per
    /// record: see `journalOffsetAt`.  Reset by `open` and by `rollover`.
    bsls::Types::Uint64 d_firstRecordIndex;
    bsls::Types::Uint64 d_firstRecordOffset;

    /// Index of `d_cache[0]`; meaningless while `d_cache` is empty.
    bsls::Types::Uint64 d_cacheBase;

    /// Term of every entry in `d_cache`; meaningless while it is empty.  One
    /// value covers the window because an entry of a different term restarts
    /// it, and serving a stale term would corrupt a peer's `prevLogTerm`
    /// check.
    bsls::Types::Uint64 d_cacheTerm;

    /// Total length of the blobs in `d_cache`.
    bsls::Types::Uint64 d_cacheBytes;

    /// Count of appended writes at the front of `d_pendingWrites`.  Writes in
    /// [0, d_appendedCount) have been formatted and appended to the log,
    /// waiting for commit/application and removal.  Writes in
    /// [d_appendedCount, end) are buffered (not yet appended, e.g. during a
    /// rollover window).
    size_t d_appendedCount;

    /// Removals held back during a rollover window.
    bsl::vector<mqbs::DataStoreRecordHandle> d_deferredRemovals;

    // PRIVATE MANIPULATORS

    /// Retain the specified `blob` and `term` as the entry at the specified
    /// `index`, and drop the oldest cached entries until the cache is back
    /// within its bounds.  A non-contiguous `index` restarts the window.
    void cacheEntry(bsls::Types::Uint64 index,
                    bsls::Types::Uint64 term,
                    const EntryBlobSp&  blob);

    /// Drop cached entries at or above the specified `index`.
    void dropCacheFrom(bsls::Types::Uint64 index);

    /// Drop cached entries below the specified `index`.
    void dropCacheThrough(bsls::Types::Uint64 index);

    /// Drop entries at or below the specified applied `index` from the front
    /// of `d_index`.  Only the truncation anchors and the record handle live
    /// there, and a committed entry is never truncated, so once applied it
    /// needs neither.
    void trimFrontThrough(bsls::Types::Uint64 index);

    /// Stop tracking the appended writes at or above the specified `index`,
    /// whose log entries `truncateFrom` is about to erase.  The buffered ones
    /// must already be gone (see `dropBufferedWrites`).
    void dropAppendedWritesFrom(bsls::Types::Uint64 index);

    /// Return the journal offset the anchor predicts for the specified
    /// `index`, without requiring it to be in range.
    bsls::Types::Uint64 expectedJournalOffset(bsls::Types::Uint64 index) const;

    /// Return 0 if the record for the specified `index` landed at the
    /// specified `actualOffset`.  Otherwise log the anchor, mark the partition
    /// unavailable and return non-zero: from here on every derived offset
    /// would read a neighbouring record.
    int verifyJournalOffset(bsls::Types::Uint64 index,
                            bsls::Types::Uint64 actualOffset);

    // NOT IMPLEMENTED
    PartitionRaftLog(const PartitionRaftLog&);
    PartitionRaftLog& operator=(const PartitionRaftLog&);

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(PartitionRaftLog, bslma::UsesBslmaAllocator)

    // CREATORS
    PartitionRaftLog(mqbs::FileStore*  fileStore,
                     bslma::Allocator* allocator = 0);

    ~PartitionRaftLog() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    /// Open the FileStore, recover journal records into `d_index`, and
    /// reverse to forward order.  Return 0 on success.
    int open();

    /// Process a committed entry on the primary.  Strong consistency
    /// receipt processing (stub).
    void applyCommittedEntryAsPrimary(bsls::Types::Uint64 index);

    /// Process a committed entry at the specified `index` on a replica.  A
    /// no-op if `index` is at or below `d_recoveredLastIndex` (already
    /// materialized by `StorageUtil::recoveredQueuesCb` at `open()` time).
    /// Otherwise read the record type from the specified `data` blob, look up
    /// the handle in `d_index`, and delegate to
    /// `FileStore::onRecordCommittedReplica`.
    void applyCommittedEntryAsReplica(bsls::Types::Uint64 index,
                                      const bdlbb::Blob&  data);

    /// Append a new log entry with the specified 'term'.  A null 'data' is
    /// the primary path: the entry is formatted straight into the mmap files
    /// from the next unappended `PendingWrite`.  Otherwise 'data' is the
    /// combined blob from AppendEntries, passed to
    /// 'FileStore::writeFormattedRecord()' unchanged.
    int append(bsls::Types::Uint64                 term,
               const bsl::shared_ptr<bdlbb::Blob>& data) BSLS_KEYWORD_OVERRIDE;

    /// Truncate the log from the specified 'index' (inclusive) to the end.
    /// Remove entries from the in-memory index, truncate the journal and
    /// data files, and invalidate the cache.  Return 0 on success, non-zero
    /// on error.
    int truncateFrom(bsls::Types::Uint64 index) BSLS_KEYWORD_OVERRIDE;

    /// Enqueue the specified `pw` as the next write for `append()` to consume
    /// from the front (matching by id, calling the corresponding
    /// `FileStore::format*Record`).  Called by `PartitionRaft` when no
    /// rollover is in flight; the behavior is undefined unless
    /// `!isBuffering()`.
    void setPendingWrite(const bsl::shared_ptr<PendingWrite>& pw);

    /// Buffer the specified `pw` for replay after the in-flight `e_ROLLOVER`
    /// commits, instead of appending it now (which would place it after
    /// `e_ROLLOVER`, breaking the invariant that nothing follows a pending
    /// rollover).  Reserve a placeholder record for its future log index using
    /// the specified `term` and set `pw->d_handle` (except for record types
    /// that produce no handle, e.g. message-deletion), and deep-copy any
    /// borrowed attributes into the buffered entry.  Return 0 on success, or a
    /// non-zero backstop code if the buffer is at capacity
    /// (`k_MAX_PENDING_WRITES`).  The behavior is undefined unless
    /// `isBuffering()`.
    int bufferPendingWrite(const bsl::shared_ptr<PendingWrite>& pw,
                           bsls::Types::Uint64                  term);

    /// Hand ownership of the buffered writes to the specified `out` for
    /// drain-replay, leaving the log's buffer empty.  Uses `swap` so the
    /// buffered shared_ptrs (and thus the pooled objects and their
    /// `d_attributes_p`->`d_ownedAttributes` self-pointers) stay valid.  The
    /// behavior is undefined unless no pending write has been appended: the
    /// caller re-proposes every one it is handed.
    void takePendingWrites(PendingWrites* out);

    /// Stop tracking all pending writes without replaying them, erasing the
    /// reserved placeholder record of each one not yet appended.  Called on
    /// shutdown, where nothing more will commit.  The records of the
    /// already-appended ones are left in place, since their log entries
    /// survive on disk.
    void dropPendingWrites();

    /// Stop tracking the writes that were buffered for a rollover drain that
    /// will not happen, erasing the reserved placeholder record of each.
    /// Called on leadership loss: their placeholders belong to no log entry,
    /// so nothing else would reclaim them.  The appended writes are kept --
    /// their entries may still commit under the new leader, and
    /// `isOwnAppendedEntry` is what routes such a commit back through the
    /// primary apply path.
    void dropBufferedWrites();

    /// Return the journal offset of the record at the specified `index`,
    /// computed from the file set's anchor.  The behavior is undefined unless
    /// `snapshotIndex() < index <= lastIndex()`.
    bsls::Types::Uint64 journalOffsetAt(bsls::Types::Uint64 index) const;

    /// Drop every cached entry.  The cached blobs alias the active file set,
    /// so the caller must invoke this before closing it: an alias that
    /// outlives the close is released later by a deferred `FileStore::gc`, by
    /// which time the dispatcher may be stopped.  `truncateFrom` and
    /// `rollover` do this for themselves.
    void clearCache();

    /// If the specified `handle` was reserved by a pending write currently in
    /// the buffer, invalidate its handle so application becomes a no-op.
    /// Otherwise the caller is responsible for removing it.  O(1) via position
    /// computation.
    void
    invalidatePendingWriteHandle(const mqbs::DataStoreRecordHandle& handle);

    /// Hold back removal of the record referenced by the specified `handle`
    /// until `e_ROLLOVER` is applied and return `true`; return `false` if the
    /// caller must remove it now.  The compaction copies `d_records`, so a
    /// record appended before `e_ROLLOVER` must survive it, as on replicas.
    bool deferRemoval(const mqbs::DataStoreRecordHandle& handle);

    /// Apply the removals held back by `deferRemoval`.
    void flushDeferredRemovals();

    /// Perform a Raft-driven file rollover for the committed `e_ROLLOVER`
    /// entry at the specified `rolloverIndex`.  Because window writes NACK in
    /// phase 1, `e_ROLLOVER` is the last log entry when it commits, so
    /// nothing uncommitted follows it.  Orchestrates the `FileStore` rollover
    /// pieces directly: prepare a new file set, compact every live record with
    /// sequence number at most `rolloverIndex` into it
    /// (`writeRolledOverRecords`; the `e_ROLLOVER` journal-op is naturally
    /// excluded), write marker (i) (`writeFirstSyncPointAfterRollover`,
    /// stamped with the `e_ROLLOVER` record's own timestamp) and finalize
    /// (`finalizeRolloverFileSet`).  Then empty `d_index` and advance the
    /// snapshot boundary to `rolloverIndex`.
    void rollover(bsls::Types::Uint64 rolloverIndex);

    /// Process a committed entry at the specified `index` on a replica.
    /// For application records delegate to
    /// `FileStore::onRecordCommittedReplica`.  JOURNAL_OP records are
    /// skipped.
    void onRecordCommitted(bsls::Types::Uint64 index);

    // ACCESSORS
    bsls::Types::Uint64 lastIndex() const BSLS_KEYWORD_OVERRIDE;

    bsls::Types::Uint64 lastTerm() const BSLS_KEYWORD_OVERRIDE;

    bsls::Types::Uint64
    term(bsls::Types::Uint64 index) const BSLS_KEYWORD_OVERRIDE;

    /// Append to the specified 'out' the log entries in the half-open range
    /// '[lo, hi)'.  For entries matching the single-entry cache, the cached
    /// blob is returned directly.  For other entries, zero-copy aliased
    /// blobs are created from the mmap'd journal+data files via
    /// 'FileStore::readRecord()'.  See 'RaftLog::entries'.
    void entries(bsls::Types::Uint64    lo,
                 bsls::Types::Uint64    hi,
                 bsl::vector<LogEntry>* out,
                 bsls::Types::Uint64    maxCount,
                 bsls::Types::Uint64    maxBytes,
                 bool                   forApply) const BSLS_KEYWORD_OVERRIDE;

    bsls::Types::Uint64 snapshotIndex() const BSLS_KEYWORD_OVERRIDE;

    bsls::Types::Uint64 snapshotTerm() const BSLS_KEYWORD_OVERRIDE;

    /// Return `true` if the entry at the specified `index` is an `e_ROLLOVER`
    /// sync point.  Answered from `d_index`, so the behavior is undefined
    /// unless `index` is one it still holds: at or above `d_frontIndex`,
    /// which excludes every entry already applied and trimmed.
    bool isRollover(bsls::Types::Uint64 index) const;

    /// Return `true` if the entry at the specified `index` was appended by
    /// this node through the primary write path and is still awaiting its
    /// commit.  Such an entry already has its record, its handle and its
    /// storage-side effects in place from propose time, so it must be applied
    /// as primary even if leadership has since moved: the replica path would
    /// redo that work, re-inserting a handle to a record that propose-time
    /// deletion or receipt GC has already erased.
    bool isOwnAppendedEntry(bsls::Types::Uint64 index) const;

    /// Return `true` if the log holds an `e_ROLLOVER` entry above the
    /// specified `commitIndex` (i.e. an uncommitted rollover, whether
    /// self-proposed or inherited from a prior leader).  A new leader uses
    /// this after appending its become-leader sync point to detect an
    /// inherited rollover it must carry to commit, so it can buffer writes
    /// until that rollover completes.
    bool hasUncommittedRollover(bsls::Types::Uint64 commitIndex) const;
};

}  // close package namespace
}  // close enterprise namespace

#endif
