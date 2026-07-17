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
// A single-entry cache avoids re-reading the most recently appended entry
// from the mmap'd files.  The cache is populated by 'append()' and served by
// 'entries()'.  The caller must invoke 'clearCache()' after processing the
// output to release the blob.
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
  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBRAFT.PARTITIONRAFTLOG");

    // TYPES
    typedef mqbs::RecoveryRecordInfo      EntryInfo;
    typedef mqbs::FileStore::PendingWrite PendingWrite;

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
    bsl::deque<bsl::shared_ptr<PendingWrite> > d_pendingWrites;

    /// Single-entry cache: the most recently appended write on the primary
    /// path, retained (a cheap pooled shared_ptr copy) so `entries()` can
    /// serve its `d_entryBlob` to peers without re-reading mmap.  Null when
    /// empty (e.g. on the replica path, or after `clearCache`).
    bsl::shared_ptr<PendingWrite> d_cached;

    /// Count of appended writes at the front of `d_pendingWrites`.  Writes in
    /// [0, d_appendedCount) have been formatted and appended to the log,
    /// waiting for commit/application and removal.  Writes in
    /// [d_appendedCount, end) are buffered (not yet appended, e.g. during a
    /// rollover window).
    size_t d_appendedCount;

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

    /// Append a new log entry with the specified 'term'.  If 'auxiliary' is
    /// null (replica path), 'data' is a combined blob passed to
    /// 'FileStore::writeFormattedRecord()' unchanged.  If 'auxiliary' is
    /// set (primary path), 'data' is the 60-byte journal record and
    /// 'auxiliary' is the data/qlist payload; each is written to the
    /// corresponding mmap file directly without building an intermediate
    /// combined blob.
    int append(bsls::Types::Uint64                 term,
               const bsl::shared_ptr<bdlbb::Blob>& data,
               bsls::Types::Uint64 id = 0) BSLS_KEYWORD_OVERRIDE;

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
    /// `d_attributes_p`->`d_ownedAttributes` self-pointers) stay valid.
    void takePendingWrites(bsl::deque<bsl::shared_ptr<PendingWrite> >* out);

    /// Drop all buffered writes without replaying them, erasing each reserved
    /// placeholder record.  Called on leadership loss, and on shutdown: in
    /// both cases these writes were never acknowledged or committed, so
    /// discarding them is safe.
    void dropPendingWrites();

    /// If the specified `handle` was reserved by a pending write currently in
    /// the buffer, invalidate its handle so application becomes a no-op.
    /// Otherwise the caller is responsible for removing it.  O(1) via position
    /// computation.
    void
    invalidatePendingWriteHandle(const mqbs::DataStoreRecordHandle& handle);

    /// Release the single-entry cache.  Must be called after
    /// processOutput() completes.
    void clearCache();

    /// Perform a Raft-driven file rollover for the committed `e_ROLLOVER`
    /// entry at the specified `rolloverIndex`.  Because window writes NACK in
    /// phase 1, `e_ROLLOVER` is the last log entry when it commits, so there
    /// is no uncommitted tail.  Orchestrates the `FileStore` rollover pieces
    /// directly: prepare a new file set, compact every live record with
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

    /// Reset the log to the specified snapshot state.  Called after
    /// receiving and applying an InstallSnapshot from the leader.
    void
    applySnapshot(bsls::Types::Uint64 lastIncludedIndex,
                  bsls::Types::Uint64 lastIncludedTerm) BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS
    bsls::Types::Uint64 lastIndex() const BSLS_KEYWORD_OVERRIDE;

    bsls::Types::Uint64 lastTerm() const BSLS_KEYWORD_OVERRIDE;

    bsls::Types::Uint64
    term(bsls::Types::Uint64 index) const BSLS_KEYWORD_OVERRIDE;

    /// Load into the specified 'out' the log entries in the half-open range
    /// '[lo, hi)'.  For entries matching the single-entry cache, the cached
    /// blob is returned directly.  For other entries, zero-copy aliased
    /// blobs are created from the mmap'd journal+data files via
    /// 'FileStore::readRecord()'.  Return 0 on success, non-zero on error.
    int entries(bsls::Types::Uint64    lo,
                bsls::Types::Uint64    hi,
                bsl::vector<LogEntry>* out) const BSLS_KEYWORD_OVERRIDE;

    bsls::Types::Uint64 snapshotIndex() const BSLS_KEYWORD_OVERRIDE;

    bsls::Types::Uint64 snapshotTerm() const BSLS_KEYWORD_OVERRIDE;

    bool isRollover(bsls::Types::Uint64 index) const;

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
