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
// m_bmqstoragetool::CslSearchShortResult: handles short result output.
// m_bmqstoragetool::CslSearchDetailResult: handles detail result output.
// m_bmqstoragetool::CslSearchResultDecorator: base decorator for search result
//   decorators.
// m_bmqstoragetool::CslSearchSequenceNumberDecorator: provides
//   decorator to handle sequence numbers.
// m_bmqstoragetool::CslSearchOffsetDecorator: provides decorator to handle
//   offsets.
// m_bmqstoragetool::CslSummaryResult: handles summary result
//   output.
//
//@DESCRIPTION: 'CslSearchResult' interface and implementation classes provide
// a logic of CSL search and output results.

// bmqstoragetool
#include <m_bmqstoragetool_compositesequencenumber.h>
#include <m_bmqstoragetool_parameters.h>
#include <m_bmqstoragetool_printer.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>

// MQB
#include <mqbc_clusterstateledgerprotocol.h>
#include <mqbsi_ledger.h>

// BDE
#include <bsl_map.h>

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
        /// Counter of snapshot records.
        bsls::Types::Uint64 d_snapshotCount;
        /// Counter of update records.
        bsls::Types::Uint64 d_updateCount;
        /// Counter of commit records.
        bsls::Types::Uint64 d_commitCount;
        // Counter of ack records.
        bsls::Types::Uint64 d_ackCount;

        // CREATORS
        /// Create a 'RecordCount' object with all counters set to 0.
        RecordCount();
    };

    // CREATORS

    /// Destructor
    virtual ~CslSearchResult();

    // MANIPULATORS

    /// Process record with the specified `header`, `record` and
    /// `recordId`.
    virtual bool processRecord(const mqbc::ClusterStateRecordHeader& header,
                               const bmqp_ctrlmsg::ClusterMessage&   record,
                               const mqbsi::LedgerRecordId& recordId) = 0;

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

    /// Reference to output stream.
    bsl::ostream& d_ostream;
    /// Record types to process
    const Parameters::ProcessCslRecordTypes& d_processCslRecordTypes;
    /// Record counters
    RecordCount d_recordCount;
    /// Allocator used inside the class.
    bslma::Allocator* d_allocator_p;

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
    /// `recordId`.
    bool
    processRecord(const mqbc::ClusterStateRecordHeader& header,
                  const bmqp_ctrlmsg::ClusterMessage&   record,
                  const mqbsi::LedgerRecordId& recordId) BSLS_KEYWORD_OVERRIDE;

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

    /// Reference to output stream.
    bsl::ostream& d_ostream;
    /// Record types to process
    const Parameters::ProcessCslRecordTypes& d_processCslRecordTypes;
    /// Record counters
    RecordCount d_recordCount;
    /// Allocator used inside the class.
    bslma::Allocator* d_allocator_p;

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
    /// `recordId`.
    bool
    processRecord(const mqbc::ClusterStateRecordHeader& header,
                  const bmqp_ctrlmsg::ClusterMessage&   record,
                  const mqbsi::LedgerRecordId& recordId) BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS

    /// Output result of a search.
    void outputResult() const BSLS_KEYWORD_OVERRIDE;
};

// ==============================
// class CslSearchResultDecorator
// ==============================

/// This class provides a base decorator that handles
/// given `component`.
class CslSearchResultDecorator : public CslSearchResult {
  protected:
    /// Pointer to object that is decorated.
    bsl::shared_ptr<CslSearchResult> d_searchResult;
    /// Pointer to allocator that is used inside the class.
    bslma::Allocator* d_allocator_p;

  public:
    // CREATORS

    /// Constructor using the specified `component` and `allocator`.
    CslSearchResultDecorator(const bsl::shared_ptr<CslSearchResult>& component,
                             bslma::Allocator* allocator);

    // MANIPULATORS

    /// Process record with the specified `header`, `record` and
    /// `recordId`.
    bool
    processRecord(const mqbc::ClusterStateRecordHeader& header,
                  const bmqp_ctrlmsg::ClusterMessage&   record,
                  const mqbsi::LedgerRecordId& recordId) BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS

    /// Output result of a search.
    void outputResult() const BSLS_KEYWORD_OVERRIDE;
};

// ======================================
// class CslSearchSequenceNumberDecorator
// ======================================

/// This class provides decorator to handle search of given composite sequence
/// numbers.
class CslSearchSequenceNumberDecorator : public CslSearchResultDecorator {
  private:
    // PRIVATE DATA

    /// List of composite sequence numbers to search for.
    bsl::vector<CompositeSequenceNumber> d_seqNums;
    /// Reference to output stream.
    bsl::ostream& d_ostream;

  public:
    // CREATORS

    /// Constructor using the specified `component`, `seqNums`, `ostream` and
    /// `allocator`.
    CslSearchSequenceNumberDecorator(
        const bsl::shared_ptr<CslSearchResult>&     component,
        const bsl::vector<CompositeSequenceNumber>& seqNums,
        bsl::ostream&                               ostream,
        bslma::Allocator*                           allocator);

    // MANIPULATORS

    /// Process record with the specified `header`, `record` and
    /// `recordId`.
    bool
    processRecord(const mqbc::ClusterStateRecordHeader& header,
                  const bmqp_ctrlmsg::ClusterMessage&   record,
                  const mqbsi::LedgerRecordId& recordId) BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS

    /// Output result of a search.
    void outputResult() const BSLS_KEYWORD_OVERRIDE;
};

// ==============================
// class CslSearchOffsetDecorator
// ==============================

/// This class provides decorator to handle search of given offsets.
class CslSearchOffsetDecorator : public CslSearchResultDecorator {
  private:
    // PRIVATE DATA

    /// List of offsets to search for.
    bsl::vector<bsls::Types::Int64> d_offsets;
    /// Reference to output stream.
    bsl::ostream& d_ostream;

  public:
    // CREATORS

    /// Constructor using the specified `component`, `seqNums`, `ostream` and
    /// `allocator`.
    CslSearchOffsetDecorator(const bsl::shared_ptr<CslSearchResult>& component,
                             const bsl::vector<bsls::Types::Int64>&  offsets,
                             bsl::ostream&                           ostream,
                             bslma::Allocator* allocator);

    // MANIPULATORS

    /// Process record with the specified `header`, `record` and
    /// `recordId`.
    bool
    processRecord(const mqbc::ClusterStateRecordHeader& header,
                  const bmqp_ctrlmsg::ClusterMessage&   record,
                  const mqbsi::LedgerRecordId& recordId) BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS

    /// Output result of a search.
    void outputResult() const BSLS_KEYWORD_OVERRIDE;
};

// ======================
// class CslSummaryResult
// ======================

/// This class provides logic to process summary of CSL file.
class CslSummaryResult : public CslSearchResult {
  private:
    // PRIVATE TYPES

    /// Map of found update record choices.
    typedef bsl::map<int, bsls::Types::Uint64> UpdateChoiceMap;

    // PRIVATE DATA

    /// Reference to output stream.
    bsl::ostream& d_ostream;

    /// CSL Record types to process.
    const Parameters::ProcessCslRecordTypes& d_processCslRecordTypes;
    /// Record counters
    RecordCount d_recordCount;
    /// Map of found update record choices.
    UpdateChoiceMap d_updateChoiceCount;
    /// Reference to 'QueueMap' instance.
    const QueueMap& d_queueMap;
    /// Limit number of queues to display
    unsigned int d_cslSummaryQueuesLimit;

    /// Pointer to allocator that is used inside the class.
    bslma::Allocator* d_allocator_p;

  public:
    // CREATORS

    /// Constructor using the specified `ostream`, `processCslRecordTypes` and
    /// `allocator`.
    explicit CslSummaryResult(
        bsl::ostream&                            ostream,
        const Parameters::ProcessCslRecordTypes& processCslRecordTypes,
        const QueueMap&                          d_queueMap,
        unsigned int                             cslSummaryQueuesLimit,
        bslma::Allocator*                        allocator);

    // MANIPULATORS

    /// Process record with the specified `header`, `record` and
    /// `recordId`.
    bool
    processRecord(const mqbc::ClusterStateRecordHeader& header,
                  const bmqp_ctrlmsg::ClusterMessage&   record,
                  const mqbsi::LedgerRecordId& recordId) BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS

    /// Output result of a search.
    void outputResult() const BSLS_KEYWORD_OVERRIDE;
};

}  // close package namespace
}  // close enterprise namespace

#endif
