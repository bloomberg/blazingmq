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

#ifndef INCLUDED_M_BMQSTORAGETOOL_CSLRECORDPRINTER_H
#define INCLUDED_M_BMQSTORAGETOOL_CSLRECORDPRINTER_H

//@PURPOSE: Provide utilities for printing CSL file records for different
// formats.
//
//@CLASSES:
// - m_bmqstoragetool::CslRecordCount: VST representing CSL record counters.
// - m_bmqstoragetool::CslRecordPrinter: CSL Printer for different formats.
//
//@DESCRIPTION: utilities for printing CSL file records for different formats.

// bmqstoragetool
#include <m_bmqstoragetool_parameters.h>

// BMQ
#include <mqbc_clusterstateledgerprotocol.h>
#include <mqbsi_ledger.h>

// BDE
#include <bdlt_epochutil.h>
#include <bsl_ostream.h>
#include <bsl_unordered_map.h>
#include <bsl_vector.h>
#include <bslma_allocator.h>

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

    /// Counter of ack records.
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

// ======================
// class CslRecordPrinter
// ======================

template <typename PRINTER_TYPE>
class CslRecordPrinter {
  private:
    bsl::ostream&                   d_ostream;
    bsl::vector<const char*>        d_fields;
    bslma::ManagedPtr<PRINTER_TYPE> d_printer_mp;
    bslma::Allocator*               d_allocator_p;

  public:
    // CREATORS
    explicit CslRecordPrinter(bsl::ostream&     stream,
                              bslma::Allocator* allocator = 0);

    // PUBLIC METHODS

    /// Print cluster state ledger (CSL) record details.
    void printRecordDetails(const bsl::string&                    recStr,
                            const mqbc::ClusterStateRecordHeader& header,
                            const mqbsi::LedgerRecordId&          recId);

    /// Print cluster state ledger (CSL) records summary.
    void printRecordsSummary(const CslRecordCount&     recordCount,
                             const CslUpdateChoiceMap& updateChoiceMap);
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ======================
// class CslRecordPrinter
// ======================

template <typename PRINTER_TYPE>
CslRecordPrinter<PRINTER_TYPE>::CslRecordPrinter(std::ostream&     stream,
                                                 bslma::Allocator* allocator)
: d_ostream(stream)
, d_fields(allocator)
, d_printer_mp()
, d_allocator_p(allocator)
{
    // NOTHING
}

template <typename PRINTER_TYPE>
void CslRecordPrinter<PRINTER_TYPE>::printRecordDetails(
    const bsl::string&                    recStr,
    const mqbc::ClusterStateRecordHeader& header,
    const mqbsi::LedgerRecordId&          recId)
{
    d_fields.clear();
    d_fields.reserve(10);  // max number of fields
    d_fields.push_back("RecordType");
    d_fields.push_back("Offset");
    d_fields.push_back("LogId");
    d_fields.push_back("ElectorTerm");
    d_fields.push_back("SequenceNumber");
    d_fields.push_back("HeaderWords");
    d_fields.push_back("LeaderAdvisoryWords");
    d_fields.push_back("Timestamp");
    d_fields.push_back("Epoch");
    if (!recStr.empty()) {
        d_fields.push_back("Record");
    }
    // It's ok to pass a vector by pointer and push elements after that as
    // we've reserved it's capacity in advance. Hense, no reallocations will
    // happen and the pointer won't get invalidated.
    d_printer_mp.load(new (*d_allocator_p) PRINTER_TYPE(d_ostream, &d_fields),
                      d_allocator_p);

    *d_printer_mp << header.recordType() << recId.offset() << recId.logId()
                  << header.electorTerm() << header.sequenceNumber()
                  << header.headerWords() << header.leaderAdvisoryWords();

    const bsls::Types::Uint64 epochValue = header.timestamp();
    bdlt::Datetime            datetime;
    const int rc = bdlt::EpochUtil::convertFromTimeT64(&datetime, epochValue);
    if (0 != rc) {
        *d_printer_mp << 0;
    }
    else {
        *d_printer_mp << datetime;
    }
    *d_printer_mp << epochValue;

    if (!recStr.empty()) {
        // Print record string.
        *d_printer_mp << recStr;
    }

    d_printer_mp.reset();
}

template <typename PRINTER_TYPE>
void CslRecordPrinter<PRINTER_TYPE>::printRecordsSummary(
    const CslRecordCount&     recordCount,
    const CslUpdateChoiceMap& updateChoiceMap)
{
    d_fields.clear();
    d_fields.push_back("SnapshotRecords");
    d_fields.push_back("UpdateRecords");
    if (recordCount.d_updateCount > 0) {
        bmqp_ctrlmsg::ClusterMessageChoice clusterMessageChoice(d_allocator_p);
        for (CslUpdateChoiceMap::const_iterator it = updateChoiceMap.begin();
             it != updateChoiceMap.end();
             ++it) {
            clusterMessageChoice.makeSelection(it->first);
            d_fields.push_back(clusterMessageChoice.selectionName());
        }
    }
    d_fields.push_back("CommitRecords");
    d_fields.push_back("AckRecords");

    // It's ok to pass a vector by pointer and push elements after that as
    // we've reserved it's capacity in advance. Hense, no reallocations will
    // happen and the pointer won't get invalidated.
    d_printer_mp.load(new (*d_allocator_p) PRINTER_TYPE(d_ostream, &d_fields),
                      d_allocator_p);

    *d_printer_mp << recordCount.d_snapshotCount << recordCount.d_updateCount;
    if (recordCount.d_updateCount > 0) {
        for (CslUpdateChoiceMap::const_iterator it = updateChoiceMap.begin();
             it != updateChoiceMap.end();
             ++it) {
            *d_printer_mp << it->second;
        }
    }
    *d_printer_mp << recordCount.d_commitCount << recordCount.d_ackCount;

    d_printer_mp.reset();
}

}  // close package namespace

}  // close enterprise namespace

#endif  // INCLUDED_M_BMQSTORAGETOOL_CSLRECORDPRINTER_H
