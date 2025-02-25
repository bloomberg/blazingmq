// Copyright 2025 Bloomberg Finance L.P.
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

#ifndef INCLUDED_M_BMQSTORAGETOOL_CSLPRINTER_H
#define INCLUDED_M_BMQSTORAGETOOL_CSLPRINTER_H

//@PURPOSE: Provide Printer class for printing storage files.
//
//@CLASSES:
//  Printer: provides methods to print storage files.
//
//@DESCRIPTION: Interface class to print storage files.

// bmqstoragetool
#include <m_bmqstoragetool_messagedetails.h>
#include <m_bmqstoragetool_parameters.h>

// BMQ
#include <mqbc_clusterstateledgerprotocol.h>
#include <mqbsi_ledger.h>
// #include <bmqt_messageguid.h>

// BDE
#include <bsl_list.h>
#include <bsl_map.h>
#include <bslma_managedptr.h>
#include <bsl_unordered_map.h>

namespace BloombergLP {

namespace m_bmqstoragetool {

/// Map of found `update` record choices.
typedef bsl::unordered_map<int, bsls::Types::Uint64> CslUpdateChoiceMap;

// =====================
// struct CslRecordCount
// =====================

/// VST representing CSL record counters.
struct CslRecordCount {
    /// Counter of snapshot records.
    bsls::Types::Uint64 d_snapshotCount;
    /// Counter of update records.
    bsls::Types::Uint64 d_updateCount;
    /// Counter of commit records.
    bsls::Types::Uint64 d_commitCount;
    // Counter of ack records.
    bsls::Types::Uint64 d_ackCount;

    // CREATORS

    /// Create a 'CslRecordCount' object with all counters set to 0.
    CslRecordCount()
    : d_snapshotCount(0)
    , d_updateCount(0)
    , d_commitCount(0)
    , d_ackCount(0)
    {
        // NOTHING
    }

    bool operator==(const CslRecordCount& rhs) const
    {
        return d_snapshotCount == rhs.d_snapshotCount &&
               d_updateCount == rhs.d_updateCount &&
               d_commitCount == rhs.d_commitCount &&
               d_ackCount == rhs.d_ackCount;
    }
};


// ================
// class CslPrinter
// ================

class CslPrinter {
  protected:
    // PROTECTED TYPES

    /// Offsets vector.
    typedef bsl::vector<bsls::Types::Int64> OffsetsVec;
    /// Composite sequence numbers vector.
    typedef bsl::vector<CompositeSequenceNumber> CompositesVec;

  public:
    // CREATORS

    virtual ~CslPrinter() {}

    // ACCESSORS

    /// Print the result in a short form.
    virtual void printShortResult(const mqbc::ClusterStateRecordHeader& header, const mqbsi::LedgerRecordId& recordId) const = 0;

    /// Print the result in a detail form.
    virtual void printDetailResult(const bmqp_ctrlmsg::ClusterMessage& record, const mqbc::ClusterStateRecordHeader& header, const mqbsi::LedgerRecordId& recordId) const = 0;

    /// Print not found `offsets`.
    virtual void printOffsetsNotFound(const OffsetsVec& offsets) const = 0;

    /// Print not found composite sequence numbers `seqNums`.
    virtual void
    printCompositesNotFound(const CompositesVec& seqNums) const = 0;

    /// Print search summary result.
    virtual void
    printSummaryResult(const CslRecordCount& recordCount, const CslUpdateChoiceMap& updateChoiceMap, const QueueMap& queueMap, const Parameters::ProcessCslRecordTypes& processCslRecordTypes, unsigned int queuesLimit) const = 0;

    /// Print footer of thr result
    virtual void printFooter(const CslRecordCount& recordCount, const Parameters::ProcessCslRecordTypes& processCslRecordTypes) const = 0;
};

/// Create an instance of CSL printer to print data to the specified 'stream'
/// according to the specified 'mode' using the specified 'allocator'.
bsl::shared_ptr<CslPrinter> createCslPrinter(Parameters::PrintMode mode,
    std::ostream&         stream,
    bslma::Allocator*     allocator);

}  // close package namespace

}  // close enterprise namespace

#endif  // INCLUDED_M_BMQSTORAGETOOL_CSLPRINTER_H
