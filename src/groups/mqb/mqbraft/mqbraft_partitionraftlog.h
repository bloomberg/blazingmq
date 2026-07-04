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
    mqbs::FileStore*       d_fileStore_p;
    bsl::vector<EntryInfo> d_index;
    bsls::Types::Uint64    d_snapshotIndex;
    bsls::Types::Uint64    d_snapshotTerm;
    bslma::Allocator*      d_allocator_p;

    struct CachedEntry {
        bsl::shared_ptr<bdlbb::Blob> d_blob;
        mqbs::DataStoreRecordHandle  d_handle;
        bsls::Types::Uint64          d_index;
        bool                         d_valid;

        CachedEntry()
        : d_blob()
        , d_handle()
        , d_index(0)
        , d_valid(false)
        {
        }
    };

    PendingWrite d_pendingWrite;
    CachedEntry  d_cache;

    // Single-scalar resend slot for the 'e_ROLLOVER' entry.  Because
    // 'e_ROLLOVER' is deliberately excluded from the new file set during a
    // rollover, it cannot be served from mmap by 'entries()' afterwards; its
    // blob is kept here instead.  At most one rollover is ever outstanding
    // (enforced by 'd_journalFileAvailable' staying false until commit), so a
    // single slot suffices.  'd_pendingRolloverIndex' is 0 when empty.
    bsl::shared_ptr<bdlbb::Blob> d_pendingRolloverBlob;
    bsls::Types::Uint64          d_pendingRolloverIndex;

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

    /// Process a committed entry on a replica.  Read the record type from
    /// the specified `data` blob, look up the handle in `d_index`, and
    /// delegate to `FileStore::onRecordCommittedReplica`.
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

    /// Set the pending write for the primary path.  Called by
    /// 'PartitionRaft' before 'propose()'.  'append()' calls
    /// 'FileStore::formatMessageRecord' with this when the id matches.
    void setPendingWrite(const PendingWrite& pw);

    /// Release the single-entry cache.  Must be called after
    /// processOutput() completes.
    void clearCache();

    /// Perform a Raft-driven file rollover triggered by the `e_ROLLOVER`
    /// entry just appended at `lastIndex()`.  Orchestrates the `FileStore`
    /// rollover pieces directly: prepare a new file set, copy the committed
    /// prefix `[snapshotIndex+1 .. commitIndex]` (`writeRolledOverRecords`),
    /// replay the uncommitted tail `[commitIndex+1 .. lastIndex()]` in index
    /// order (`writeRolledOverRecord` for normal records, verbatim copy for
    /// journal-ops, skip for the triggering `e_ROLLOVER`), write marker (i)
    /// (`writeFirstSyncPointAfterRollover`) and finalize
    /// (`finalizeRolloverFileSet`).  Then rebuild `d_index` with the surviving
    /// tail's new offsets, advance the snapshot boundary to the specified
    /// `commitIndex`, and stash `e_ROLLOVER`'s blob in the single-scalar
    /// resend slot (it is deliberately excluded from the new file).  Marker
    /// (i) is stamped with the specified `timestamp`.
    void rollover(bsls::Types::Uint64 commitIndex,
                  bsls::Types::Uint64 timestamp);

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

    /// Return the DataStoreRecordHandle from the last successful append.
    const mqbs::DataStoreRecordHandle& cachedHandle() const;
};

}  // close package namespace
}  // close enterprise namespace

#endif
