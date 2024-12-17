// Copyright 2014-2023 Bloomberg Finance L.P.
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

#ifndef INCLUDED_M_BMQSTORAGETOOL_CSLSEARCHRESULT
#define INCLUDED_M_BMQSTORAGETOOL_CSLSEARCHRESULT

//@PURPOSE: Provide a logic of search and output results.
//
//@CLASSES:
// m_bmqstoragetool::CslSearchResult: an interface for cluster state ledger
// (CSL) search processors.
//
//@DESCRIPTION: 'CslSearchResult' interface and implementation classes provide
// a logic of CSL search and output results.

// bmqstoragetool
#include <m_bmqstoragetool_compositesequencenumber.h>
#include <m_bmqstoragetool_parameters.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>

// MQB
#include <mqbc_clusterstateledgerprotocol.h>
#include <mqbsi_ledger.h>

namespace BloombergLP {
namespace m_bmqstoragetool {

// =====================
// class CslSearchResult
// =====================

/// This class provides an interface for CSL search processors.
class CslSearchResult {
  public:
    // PUBLIC TYPES

    // ==================
    // struct RecordCount
    // ==================

    /// VST representing record counters.
    struct RecordCount {
        bsls::Types::Uint64 d_snapshotCount;
        // Counter of  snapshot records.
        bsls::Types::Uint64 d_updateCount;
        // Counter of  update records.
        bsls::Types::Uint64 d_commitCount;
        // Counter of  commit records.
        bsls::Types::Uint64 d_ackCount;
        // Counter of ack records.

        // CREATORS
        RecordCount();
    };

    // CREATORS

    /// Destructor
    virtual ~CslSearchResult();

    // MANIPULATORS

    /// Process record with the specified `header`, `record` and
    /// `currRecordId`.
    virtual bool processRecord(const mqbc::ClusterStateRecordHeader& header,
                               const bmqp_ctrlmsg::ClusterMessage&   record,
                               const mqbsi::LedgerRecordId& currRecordId) = 0;

    // ACCESSORS

    /// Output result of a search.
    virtual void outputResult() const = 0;
};

// ==========================
// class CslSearchShortResult
// ==========================

/// This class provides logic to handle and output short result.
class CslSearchShortResult : public CslSearchResult {
  private:
    // PRIVATE DATA

    bsl::ostream& d_ostream;
    // Reference to output stream.
    Parameters::ProcessCslRecordTypes d_processCslRecordTypes;
    // Record types to process
    RecordCount d_recordCount;
    // Record counters
    bslma::Allocator* d_allocator_p;
    // Allocator used inside the class.

  public:
    // CREATORS

    /// Constructor using the specified `ostream`, `processCslRecordTypes`
    /// and `allocator`.
    explicit CslSearchShortResult(
        bsl::ostream&                            ostream,
        const Parameters::ProcessCslRecordTypes& processCslRecordTypes,
        bslma::Allocator*                        allocator = 0);

    // MANIPULATORS

    /// Process record with the specified `header`, `record` and
    /// `currRecordId`.
    bool processRecord(const mqbc::ClusterStateRecordHeader& header,
                       const bmqp_ctrlmsg::ClusterMessage&   record,
                       const mqbsi::LedgerRecordId&          currRecordId)
        BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS

    /// Output result of a search.
    void outputResult() const BSLS_KEYWORD_OVERRIDE;
};

// ===========================
// class CslSearchDetailResult
// ===========================

/// This class provides logic to handle and output detail result.
class CslSearchDetailResult : public CslSearchResult {
  private:
    // PRIVATE DATA

    bsl::ostream& d_ostream;
    // Reference to output stream.
    Parameters::ProcessCslRecordTypes d_processCslRecordTypes;
    // Record types to process
    RecordCount d_recordCount;
    // Record counters
    bslma::Allocator* d_allocator_p;
    // Allocator used inside the class.

  public:
    // CREATORS

    /// Constructor using the specified `ostream`, `processCslRecordTypes`
    /// and `allocator`.
    explicit CslSearchDetailResult(
        bsl::ostream&                            ostream,
        const Parameters::ProcessCslRecordTypes& processCslRecordTypes,
        bslma::Allocator*                        allocator = 0);

    // MANIPULATORS

    /// Process record with the specified `header`, `record` and
    /// `currRecordId`.
    bool processRecord(const mqbc::ClusterStateRecordHeader& header,
                       const bmqp_ctrlmsg::ClusterMessage&   record,
                       const mqbsi::LedgerRecordId&          currRecordId)
        BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS

    /// Output result of a search.
    void outputResult() const BSLS_KEYWORD_OVERRIDE;
};

}  // close package namespace
}  // close enterprise namespace

#endif
