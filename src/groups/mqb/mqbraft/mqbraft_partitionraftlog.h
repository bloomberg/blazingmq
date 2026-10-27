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
// using a partition's journal and data files as the underlying storage.  In
// a record written in Raft mode, 'primaryLeaseId' is the Raft term and
// 'sequenceNumber' is the Raft index.  Every journal record type (MESSAGE,
// CONFIRM, DELETION, QUEUE_OP, JOURNAL_OP) is a Raft log entry.
//
// 'append()' writes an entry in one of two ways:
// 1. Leader: no blob is passed.  The 'FileStore' formats the record from the
//    'Proposal' that 'setProposal' stored ('FileStore::format*Record').
// 2. Replica: the blob from AppendEntries is written as is
//    ('FileStore::writeFormattedRecord').
//
// A bounded cache of recent entry blobs serves 'entries()' without reading
// the journal.  A replica fills the cache in 'append()', and the leader fills
// the cache when it applies its own entries.  The log clears the cache on
// truncation, rollover and 'open', so callers do not manage the cache.
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

    /// Writes, in the order they were proposed.
    typedef bsl::deque<bsl::shared_ptr<mqbs::Proposal> > Proposals;

    /// Writes that no log entry will carry, held for their producers
    /// (`takeHeldProposals`).  A separate name from `Proposals` because
    /// `proposalHeadIndex` counts only `d_bufferedProposals`.
    typedef Proposals HeldProposals;

  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBRAFT.PARTITIONRAFTLOG");

    // TYPES
    typedef mqbs::EntryInfo              EntryInfo;
    typedef mqbs::Proposal               Proposal;
    typedef bsl::shared_ptr<bdlbb::Blob> EntryBlobSp;

    // PRIVATE CLASS DATA

    /// Bounds on `d_cache`.  Sized to hold more than a peer can fall behind
    /// between acks, so a round of replication is served from memory.
    static const size_t              k_MAX_CACHED_ENTRIES = 4;       // 096;
    static const bsls::Types::Uint64 k_MAX_CACHED_BYTES = 4 * 1024;  // * 1024;

    // DATA
    mqbs::FileStore*      d_fileStore_p;
    bsl::deque<EntryInfo> d_index;
    bsls::Types::Uint64   d_snapshotIndex;
    bsls::Types::Uint64   d_snapshotTerm;
    bslma::Allocator*     d_allocator_p;

    /// The write `setProposal` stored for the next `append`, which takes the
    /// write.  Set and cleared within a single `propose`.
    bsl::shared_ptr<Proposal> d_currentProposal;

    /// Writes buffered by `bufferProposal` while an `e_ROLLOVER` is pending,
    /// drained in order once the `e_ROLLOVER` commits
    /// (`takeBufferedProposals`).  `proposalHeadIndex` counts these writes,
    /// so only buffered writes belong here.
    Proposals d_bufferedProposals;

    /// Writes that no log entry carries: a truncation removed their entries,
    /// or they never had one.  The owner takes them with `takeHeldProposals`,
    /// then re-posts or re-proposes them.
    HeldProposals d_heldProposals;

    /// Blobs of recent entries, which `entries()` serves without reading the
    /// journal.  At most `k_MAX_CACHED_ENTRIES` entries and
    /// `k_MAX_CACHED_BYTES` bytes.  The blobs alias the active file set or a
    /// received event, so `clearCache` must run before the file set is
    /// closed.
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

    // PRIVATE MANIPULATORS

    /// Retain the specified `blob` and `term` as the entry at the specified
    /// `index`, and drop the oldest cached entries until the cache is back
    /// within its bounds.  A non-contiguous `index` or a different `term`
    /// restarts the window.  A null `blob` clears the cache.
    void cacheEntry(bsls::Types::Uint64 index,
                    bsls::Types::Uint64 term,
                    const EntryBlobSp&  blob);

    /// Drop cached entries at or above the specified `index`.
    void dropCacheFrom(bsls::Types::Uint64 index);

    /// Drop cached entries below the specified `index`.
    void dropCacheThrough(bsls::Types::Uint64 index);

    /// Drop the front entry of `d_index`.  Only the truncation anchors and
    /// the record handle live there, and a committed entry is never
    /// truncated, so once applied it needs neither.  The behavior is
    /// undefined unless `d_index` is non-empty.
    void popFront();

    /// Take the specified `pw` off the primary path: clear the handle into a
    /// record that is gone and the blob aliasing a mapping that is rolled
    /// back.  Then hold the write for its producer if the specified `hold` is
    /// true, or undo what `propose` reserved (`FileStore::undoPropose`) if
    /// `hold` is false.
    void releaseProposal(const bsl::shared_ptr<Proposal>& pw, bool hold);

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

    /// Open the FileStore and recover journal records into `d_index`
    /// (`FileStore::openForRaft`).  Set the snapshot index from the first sync
    /// point after the last rollover.  Return 0 on success.
    int open();

    /// Store the specified `pw` for the next `append`.
    void setProposal(const bsl::shared_ptr<Proposal>& pw);

    /// Hold the specified `pw` until the in-flight `e_ROLLOVER` commits and
    /// it can be drained into the new file set.  Return 0 on success,
    /// non-zero if the buffer is full.
    int bufferProposal(const bsl::shared_ptr<Proposal>& pw);

    /// Load into the specified `out` the buffered writes, emptying the queue.
    void takeBufferedProposals(Proposals* out);

    /// Return `lastIndex()` plus the number of buffered writes.  The drain
    /// appends those writes at the next indices.
    bsls::Types::Uint64 proposalHeadIndex() const;

    /// Stop tracking proposals this node accepted on the primary path: the
    /// one awaiting `append`, all buffered while rollover was pending, and
    /// the ones in the log at or above the specified `index`.  Hold them
    /// for re-posting if the specified `hold` is true, otherwise undo what
    /// `propose` reserved for them (`FileStore::undoPropose`).  Reset the
    /// proposals' record
    /// handles and blobs (aliasing the active file set) -- so this runs
    /// before that file set is closed or replaced, as well as before a
    /// truncation.
    ///
    /// Those entries stay but change their state -- this node no longer
    /// owns them (as if a peer had sent them).
    void dropProposalsFrom(bsls::Types::Uint64 index, bool hold);

    /// Hold the specified `pw`, which no log entry carries, for its producer.
    void holdProposal(const bsl::shared_ptr<Proposal>& pw);

    /// Load into the specified `out` the writes held for their producers,
    /// oldest first, emptying the list.  The caller must re-post or re-propose
    /// each write, or undo what `propose` reserved for the write
    /// (`FileStore::undoPropose`).
    void takeHeldProposals(HeldProposals* out);

    /// Drop every cached entry.  The blobs alias the active file set or a
    /// received event, so this runs before anything that replaces or closes
    /// the file set.
    void clearCache();

    int append(bsls::Types::Uint64                 term,
               const bsl::shared_ptr<bdlbb::Blob>& data) BSLS_KEYWORD_OVERRIDE;

    int truncateFrom(bsls::Types::Uint64 index) BSLS_KEYWORD_OVERRIDE;

    /// Perform the physical rollover for the committed `e_ROLLOVER` entry at
    /// the specified `rolloverIndex`: compact the live records into a new file
    /// set, rewrite the entries above it, and re-anchor on the new set.
    void rollover(bsls::Types::Uint64 rolloverIndex);

    /// Apply the committed entry at the specified `index`, carrying the
    /// specified `data`, at the specified `commitTimepoint`.  Return `true`
    /// if a write of this node's produced the entry.
    ///
    /// Such an entry has its record and handle from `append`, and only the
    /// write holds what apply still needs (the producer to answer, the
    /// capacity to charge).  So the entry goes to
    /// `FileStore::onRecordCommittedPrimary`, even if leadership has moved
    /// since.  Every other entry, replicated or recovered, goes to
    /// `FileStore::onRecordCommittedReplica`, which reads `data`.  The entry
    /// itself decides which, so the entry that replaces a truncated one is
    /// never taken for this node's own.
    bool applyCommittedEntry(bsls::Types::Uint64                 index,
                             const bsl::shared_ptr<bdlbb::Blob>& data,
                             bsls::Types::Int64 commitTimepoint);

    // ACCESSORS

    /// Return the journal offset of the record for the specified `index`.
    bsls::Types::Uint64 journalOffsetAt(bsls::Types::Uint64 index) const;

    bsls::Types::Uint64 lastIndex() const BSLS_KEYWORD_OVERRIDE;

    bsls::Types::Uint64 lastTerm() const BSLS_KEYWORD_OVERRIDE;

    bsls::Types::Uint64
    term(bsls::Types::Uint64 index) const BSLS_KEYWORD_OVERRIDE;

    /// Append to the specified 'out' the log entries in the half-open range
    /// '[lo, hi)'.  For entries in `d_cache`, the cached blob is returned
    /// directly.  For other entries, zero-copy aliased
    /// blobs are created from the mmap'd journal+data files via
    /// 'FileStore::readRecord()'.  See 'RaftLog::entries'.
    void entries(bsls::Types::Uint64    lo,
                 bsls::Types::Uint64    hi,
                 bsl::vector<LogEntry>* out,
                 bsls::Types::Uint64    maxCount,
                 bsls::Types::Uint64    maxBytes) const BSLS_KEYWORD_OVERRIDE;

    bsls::Types::Uint64 snapshotIndex() const BSLS_KEYWORD_OVERRIDE;

    bsls::Types::Uint64 snapshotTerm() const BSLS_KEYWORD_OVERRIDE;

    /// Return `true` if the entry at the specified `index` is an `e_ROLLOVER`
    /// sync point.  Answered from `d_index`, so the behavior is undefined
    /// unless `index` is one it still holds: at or above `d_frontIndex`,
    /// which excludes every entry already applied and trimmed.
    bool isRollover(bsls::Types::Uint64 index) const;

    /// Return `true` if the entry at the specified `index` is an `e_REGULAR`
    /// sync point -- the type `proposeSyncPoint` writes, as opposed to the
    /// `e_ROLLOVER` one.  Same `d_index` restriction as `isRollover`.
    bool isRegularSyncPoint(bsls::Types::Uint64 index) const;

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
