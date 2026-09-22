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

    /// FIFO of writes accepted on the primary path but not yet carried by a
    /// log entry: the one `setProposal` enqueued for the next `append`,
    /// and -- during a rollover window -- those buffered for the drain that
    /// follows it.  A write leaves here the moment `append` gives it an
    /// entry; from then on the entry owns it.
    typedef bsl::deque<bsl::shared_ptr<mqbs::Proposal> > Proposals;

    /// Writes taken off the primary path that no log entry will ever carry,
    /// kept for their producers rather than undone.
    /// The same container as `Proposals`, under the name of the list it is
    /// taken from.  Kept distinct in the interface because the two lists
    /// are: only `d_bufferedProposals` reserves log indices, and
    /// `proposalHeadIndex` counts on nothing else being parked there.
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

    /// The write `setProposal` enqueued for the next `append`, which
    /// consumes it.  Set and cleared within a single `propose`, and kept out
    /// of `d_bufferedProposals` so that nothing else can be appended in its
    /// place.
    bsl::shared_ptr<Proposal> d_currentProposal;

    /// Writes buffered by `bufferProposal` while an `e_ROLLOVER` is in
    /// flight, drained in order once it commits (see `takeBufferedProposals`).
    /// Each takes the next index above the log when it drains, derived from
    /// this queue's size, so nothing else may be parked here.
    Proposals d_bufferedProposals;

    /// Writes no log entry carries any more -- a truncation erased theirs, or
    /// they never got one -- kept for their producers rather than undone.
    /// The owner takes them with `takeHeldProposals` and re-posts each one
    /// whose producer is still attached.
    HeldProposals d_heldProposals;

    /// Entry blobs held in memory, oldest first: the entry at index
    /// `d_cacheBase + i` is `d_cache[i]`.  Two feeds, one per write path.  For
    /// an entry of this node's own, `applyCommittedEntry` hands over the blob
    /// of the write it is about to drop, so the window picks up where the
    /// entries leave off and serves a retransmit to a peer that has not acked
    /// what a quorum already committed.  As replica, `append` keeps the blob
    /// it was sent, so the commit that follows applies it instead of reading
    /// back what it just wrote.  Bounded by `k_MAX_CACHED_ENTRIES` and
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

    /// Drop the front entry of `d_index`.  Only the truncation anchors and
    /// the record handle live there, and a committed entry is never
    /// truncated, so once applied it needs neither.  The behavior is
    /// undefined unless `d_index` is non-empty.
    void popFront();

    /// Take the specified `pw` off the primary path: clear the handle into a
    /// record that is gone and the blob aliasing a mapping that is rolled
    /// back, then hold it for its producer if the specified `hold` is true,
    /// or give back what propose set aside if it is false.
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

    /// Open the FileStore, recover journal records into `d_index`, and
    /// reverse to forward order.  Return 0 on success.
    int open();

    /// Enqueue the specified `pw` for the next `append`.
    void setProposal(const bsl::shared_ptr<Proposal>& pw);

    /// Hold the specified `pw` until the in-flight `e_ROLLOVER` commits and
    /// it can be drained into the new file set.  Return 0 on success,
    /// non-zero if the buffer is full.
    int bufferProposal(const bsl::shared_ptr<Proposal>& pw);

    /// Load into the specified `out` the buffered writes, emptying the queue.
    void takeBufferedProposals(Proposals* out);

    /// Return the index the most recently accepted write will occupy: the
    /// last appended one's, or -- during a rollover window -- the reserved
    /// index of the last buffered one, which `bufferProposal` derives the
    /// same way.
    bsls::Types::Uint64 proposalHeadIndex() const;

    /// Stop tracking proposals this node accepted on the primary path: the
    /// one awaiting `append`, all buffered while rollover was pending, and
    /// the ones in the log at or above the specified `index`.  Hold them
    /// for re-posting if the specified `hold` is true, otherwise undo their
    /// impact (capacity, confirm tracking).  Reset the proposals' record
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
    /// oldest first, emptying the list.  Whoever takes them owes each one
    /// either a re-post or the capacity behind it.
    void takeHeldProposals(HeldProposals* out);

    /// Drop every cached entry.  The blobs alias the active file set, so this
    /// runs before anything that replaces or closes it.
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
    /// Such an entry already has its record, its handle and its storage-side
    /// effects in place from propose time, and only the write itself holds
    /// what apply still needs -- the producer to answer, the capacity to
    /// charge -- so it goes to `FileStore::onRecordCommittedPrimary` even if
    /// leadership has since moved.  Every other entry, replicated or
    /// recovered, goes to `FileStore::onRecordCommittedReplica`, which reads
    /// `data`.  The entry answers which, so a truncation that erased this
    /// node's entry and the write with it cannot leave the replacement
    /// looking like its own.
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
    /// '[lo, hi)'.  For entries matching the single-entry cache, the cached
    /// blob is returned directly.  For other entries, zero-copy aliased
    /// blobs are created from the mmap'd journal+data files via
    /// 'FileStore::readRecord()'.  See 'RaftLog::entries'.
    void entries(bsls::Types::Uint64    lo,
                 bsls::Types::Uint64    hi,
                 bsl::vector<LogEntry>* out,
                 bsls::Types::Uint64    maxCount,
                 bsls::Types::Uint64 maxBytes) const BSLS_KEYWORD_OVERRIDE;

    bsls::Types::Uint64 snapshotIndex() const BSLS_KEYWORD_OVERRIDE;

    bsls::Types::Uint64 snapshotTerm() const BSLS_KEYWORD_OVERRIDE;

    /// Return `true` if the entry at the specified `index` is an `e_ROLLOVER`
    /// sync point.  Answered from `d_index`, so the behavior is undefined
    /// unless `index` is one it still holds: at or above `d_frontIndex`,
    /// which excludes every entry already applied and trimmed.
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
