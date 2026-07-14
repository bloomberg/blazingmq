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

// mqbraft_cslraftlog.h -*-C++-*-
#ifndef INCLUDED_MQBRAFT_CSLRAFTLOG
#define INCLUDED_MQBRAFT_CSLRAFTLOG

//@PURPOSE: Provide a RaftLog adapter backed by a CSL (Cluster State Ledger)
// file.
//
//@CLASSES:
//  mqbraft::CslRaftLog: RaftLog implementation over a single CSL file
//
//@DESCRIPTION: This component implements the 'mqbraft::RaftLog' interface
// using an 'mqbsi::Log' as the underlying storage.  CSL records already carry
// 'electorTerm' and 'sequenceNumber' which map directly to Raft '(term,
// index)'.  Only 'e_UPDATE' and 'e_SNAPSHOT' record types are treated as Raft
// log entries.
//
/// Threading
///----------
// This component is NOT thread-safe.

// MQB
#include <mqbraft_raftnode.h>
#include <mqbsi_log.h>
#include <mqbu_storagekey.h>

#include <bmqu_blob.h>

// BDE
#include <ball_log.h>
#include <bdlbb_blob.h>
#include <bmqp_blobpoolutil.h>
#include <bsl_memory.h>
#include <bsl_vector.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_types.h>

namespace BloombergLP {
namespace mqbraft {

// ================
// class CslRaftLog
// ================

class CslRaftLog : public RaftLog {
  public:
    // TYPES
    typedef bmqp::BlobPoolUtil::BlobSpPool BlobSpPool;

  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBRAFT.CSLRAFTLOG");

    // TYPES
    struct RecordInfo {
        mqbsi::Log::Offset  d_offset;
        bsls::Types::Uint64 d_term;
    };

    // DATA
    bsl::shared_ptr<mqbsi::Log> d_log_sp;
    bsl::vector<RecordInfo>     d_index;
    bsls::Types::Uint64         d_snapshotIndex;
    bsls::Types::Uint64         d_snapshotTerm;
    BlobSpPool*                 d_blobSpPool_p;
    bslma::Allocator*           d_allocator_p;

    // NOT IMPLEMENTED
    CslRaftLog(const CslRaftLog&);
    CslRaftLog& operator=(const CslRaftLog&);

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(CslRaftLog, bslma::UsesBslmaAllocator)

    // CREATORS
    CslRaftLog(const bsl::shared_ptr<mqbsi::Log>& log,
               BlobSpPool*                       blobSpPool,
               bslma::Allocator*                 allocator = 0);

    ~CslRaftLog() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    /// Open the log and build the in-memory index by scanning all records.
    /// Return 0 on success, non-zero on error.
    int open();

    /// Close the underlying log.
    int close();

    int append(bsls::Types::Uint64                  term,
               const bsl::shared_ptr<bdlbb::Blob>&  data,
               bsls::Types::Uint64                  id = 0)
        BSLS_KEYWORD_OVERRIDE;

    int truncateFrom(bsls::Types::Uint64 index) BSLS_KEYWORD_OVERRIDE;

    /// Roll over onto the specified `newLog`, which the caller must have
    /// opened (empty), identified by the specified `newLogId`.  Seed
    /// `newLog` with a file header followed by the specified
    /// `snapshotRecord` -- the base `e_SNAPSHOT` capturing the full
    /// committed cluster state at the specified `compactIndex` /
    /// `compactTerm` -- then copy the uncommitted tail (entries with index in
    /// `(compactIndex, lastIndex]`) from the current log into `newLog`.
    /// Switch to `newLog`, advance the snapshot boundary to
    /// `(compactIndex, compactTerm)`, and load the previous underlying log
    /// into the specified `oldLog` so the caller can close/archive it.
    /// Return 0 on success and a non-zero value otherwise.  The behavior is
    /// undefined unless `snapshotIndex() <= compactIndex <= lastIndex()`.
    int rollover(bsl::shared_ptr<mqbsi::Log>*        oldLog,
                 const bsl::shared_ptr<mqbsi::Log>&  newLog,
                 const mqbu::StorageKey&             newLogId,
                 const bsl::shared_ptr<bdlbb::Blob>& snapshotRecord,
                 bsls::Types::Uint64                 compactIndex,
                 bsls::Types::Uint64                 compactTerm);

    // ACCESSORS
    bsls::Types::Uint64 lastIndex() const BSLS_KEYWORD_OVERRIDE;

    bsls::Types::Uint64 lastTerm() const BSLS_KEYWORD_OVERRIDE;

    bsls::Types::Uint64
    term(bsls::Types::Uint64 index) const BSLS_KEYWORD_OVERRIDE;

    int entries(bsls::Types::Uint64    lo,
                bsls::Types::Uint64    hi,
                bsl::vector<LogEntry>* out) const BSLS_KEYWORD_OVERRIDE;

    bsls::Types::Uint64 snapshotIndex() const BSLS_KEYWORD_OVERRIDE;

    bsls::Types::Uint64 snapshotTerm() const BSLS_KEYWORD_OVERRIDE;

    void applySnapshot(bsls::Types::Uint64 lastIncludedIndex,
                       bsls::Types::Uint64 lastIncludedTerm)
        BSLS_KEYWORD_OVERRIDE;

    /// Return true if a record of the specified `recordSize` bytes can be
    /// appended to the current log without exceeding its configured maximum
    /// size, and false otherwise (i.e. a rollover is required first).
    bool canAppend(int recordSize) const;

    /// Load into the specified `out` the base `e_SNAPSHOT` record at the
    /// start of the current log -- the compacted-state snapshot that must be
    /// applied to reconstruct `ClusterState` before replaying the tail.
    /// Return 0 on success, 1 if the current log has no base snapshot (i.e.
    /// `snapshotIndex() == 0`), and a negative value on error.
    int loadSnapshotRecord(bsl::shared_ptr<bdlbb::Blob>* out) const;
};

}  // close package namespace
}  // close enterprise namespace

#endif
