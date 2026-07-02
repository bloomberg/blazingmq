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
};

}  // close package namespace
}  // close enterprise namespace

#endif
