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

// bmqstoragetool
#include <m_bmqstoragetool_cslsearchresult.h>
#include <m_bmqstoragetool_parameters.h>
#include <m_bmqstoragetool_recordprinter.h>

// BMQ
#include <bmqu_alignedprinter.h>
#include <bmqu_memoutstream.h>

// BDE
#include <bslim_printer.h>

namespace BloombergLP {
namespace m_bmqstoragetool {

namespace {
// Helper to print CSL record header
void printClusterStateRecordHeader(
    bsl::ostream&                         ostream,
    const mqbc::ClusterStateRecordHeader& header,
    int                                   level          = 0,
    int                                   spacesPerLevel = -1)
{
    BSLS_ASSERT(ostream.good());

    bslim::Printer printer(&ostream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("recordType", header.recordType());
    printer.printAttribute("electorTerm", header.electorTerm());
    printer.printAttribute("sequenceNumber", header.sequenceNumber());
    printer.printAttribute("timestamp", header.timestamp());
    printer.end();
}

// Helper to update record counters
void updateRecordCount(CslSearchResult::RecordCount*      recordCount_p,
                       mqbc::ClusterStateRecordType::Enum recordType)
{
    switch (recordType) {
    case mqbc::ClusterStateRecordType::e_SNAPSHOT:
        recordCount_p->d_snapshotCount++;
        break;
    case mqbc::ClusterStateRecordType::e_UPDATE:
        recordCount_p->d_updateCount++;
        break;
    case mqbc::ClusterStateRecordType::e_COMMIT:
        recordCount_p->d_commitCount++;
        break;
    case mqbc::ClusterStateRecordType::e_ACK:
        recordCount_p->d_ackCount++;
        break;
    default: BSLS_ASSERT(false && "Unknown record type");
    }
}

// Helper to print summary of search result
void outputFooter(
    bsl::ostream&                            ostream,
    const Parameters::ProcessCslRecordTypes& processCslRecordTypes,
    const CslSearchResult::RecordCount&      recordCount)
{
    BSLS_ASSERT(ostream.good());

    if (processCslRecordTypes.d_snapshot) {
        recordCount.d_snapshotCount > 0
            ? (ostream << recordCount.d_snapshotCount << " snapshot")
            : ostream << "No snapshot";
        ostream << " record(s) found.\n";
    }
    if (processCslRecordTypes.d_update) {
        recordCount.d_updateCount > 0
            ? (ostream << recordCount.d_updateCount << " update")
            : ostream << "No update";
        ostream << " record(s) found.\n";
    }
    if (processCslRecordTypes.d_commit) {
        recordCount.d_commitCount > 0
            ? (ostream << recordCount.d_commitCount << " commit")
            : ostream << "No commit";
        ostream << " record(s) found.\n";
    }
    if (processCslRecordTypes.d_ack) {
        recordCount.d_ackCount > 0
            ? (ostream << recordCount.d_ackCount << " ack")
            : ostream << "No ack";
        ostream << " record(s) found.\n";
    }
}

}  // close unnamed namespace

// =====================
// class CslSearchResult
// =====================

CslSearchResult::~CslSearchResult()
{
    // NOTHING
}

// ==================
// struct RecordCount
// ==================

CslSearchResult::RecordCount::RecordCount()
: d_snapshotCount(0)
, d_updateCount(0)
, d_commitCount(0)
, d_ackCount(0)
{
    // NOTHING
}

// ==========================
// class CslSearchShortResult
// ==========================

CslSearchShortResult::CslSearchShortResult(
    bsl::ostream&                            ostream,
    const Parameters::ProcessCslRecordTypes& processCslRecordTypes,
    bslma::Allocator*                        allocator)
: d_ostream(ostream)
, d_processCslRecordTypes(processCslRecordTypes)
, d_recordCount()
, d_allocator_p(allocator)
{
    // NOTHING
}

bool CslSearchShortResult::processRecord(
    const mqbc::ClusterStateRecordHeader& header,
    BSLS_ANNOTATION_UNUSED const bmqp_ctrlmsg::ClusterMessage& record,
    const mqbsi::LedgerRecordId&                               recordId)
{
    printClusterStateRecordHeader(d_ostream, header);
    d_ostream << recordId << '\n';

    updateRecordCount(&d_recordCount, header.recordType());

    return false;
}

void CslSearchShortResult::outputResult() const
{
    // Print summary counters
    outputFooter(d_ostream, d_processCslRecordTypes, d_recordCount);
}

// ===========================
// class CslSearchDetailResult
// ===========================

CslSearchDetailResult::CslSearchDetailResult(
    bsl::ostream&                            ostream,
    const Parameters::ProcessCslRecordTypes& processCslRecordTypes,
    bslma::Allocator*                        allocator)
: d_ostream(ostream)
, d_processCslRecordTypes(processCslRecordTypes)
, d_recordCount()
, d_allocator_p(allocator)
{
    // NOTHING
}

bool CslSearchDetailResult::processRecord(
    const mqbc::ClusterStateRecordHeader& header,
    const bmqp_ctrlmsg::ClusterMessage&   record,
    const mqbsi::LedgerRecordId&          recordId)
{
    RecordPrinter::printRecord(d_ostream,
                               record,
                               header,
                               recordId,
                               d_allocator_p);

    updateRecordCount(&d_recordCount, header.recordType());

    return false;
}

void CslSearchDetailResult::outputResult() const
{
    // Print summary counters
    outputFooter(d_ostream, d_processCslRecordTypes, d_recordCount);
}

// ==============================
// class CslSearchResultDecorator
// ==============================

CslSearchResultDecorator::CslSearchResultDecorator(
    const bsl::shared_ptr<CslSearchResult>& component,
    bslma::Allocator*                       allocator)
: d_searchResult(component)
, d_allocator_p(allocator)
{
    // NOTHING
}

bool CslSearchResultDecorator::processRecord(
    const mqbc::ClusterStateRecordHeader& header,
    const bmqp_ctrlmsg::ClusterMessage&   record,
    const mqbsi::LedgerRecordId&          recordId)
{
    return d_searchResult->processRecord(header, record, recordId);
}

void CslSearchResultDecorator::outputResult() const
{
    d_searchResult->outputResult();
}

// ======================================
// class CslSearchSequenceNumberDecorator
// ======================================

CslSearchSequenceNumberDecorator::CslSearchSequenceNumberDecorator(
    const bsl::shared_ptr<CslSearchResult>&     component,
    const bsl::vector<CompositeSequenceNumber>& seqNums,
    bsl::ostream&                               ostream,
    bslma::Allocator*                           allocator)
: CslSearchResultDecorator(component, allocator)
, d_seqNums(seqNums, allocator)
, d_ostream(ostream)
{
    // NOTHING
}

bool CslSearchSequenceNumberDecorator::processRecord(
    const mqbc::ClusterStateRecordHeader& header,
    const bmqp_ctrlmsg::ClusterMessage&   record,
    const mqbsi::LedgerRecordId&          recordId)
{
    CompositeSequenceNumber seqNum(header.electorTerm(),
                                   header.sequenceNumber());
    bsl::vector<CompositeSequenceNumber>::const_iterator it =
        bsl::find(d_seqNums.cbegin(), d_seqNums.cend(), seqNum);
    if (it != d_seqNums.cend()) {
        CslSearchResultDecorator::processRecord(header, record, recordId);
        // Remove processed sequence number.
        d_seqNums.erase(it);
    }

    // return true (stop search) if d_seqNums is empty.
    return d_seqNums.empty();
}

void CslSearchSequenceNumberDecorator::outputResult() const
{
    CslSearchResultDecorator::outputResult();

    // Print not found sequence numbers
    if (!d_seqNums.empty()) {
        d_ostream << '\n'
                  << "The following " << d_seqNums.size()
                  << " sequence number(s) not found:" << '\n';
        bsl::vector<CompositeSequenceNumber>::const_iterator it =
            d_seqNums.cbegin();
        for (; it != d_seqNums.cend(); ++it) {
            d_ostream << *it << '\n';
        }
    }
}

// ==============================
// class CslSearchOffsetDecorator
// ==============================

CslSearchOffsetDecorator::CslSearchOffsetDecorator(
    const bsl::shared_ptr<CslSearchResult>& component,
    const bsl::vector<bsls::Types::Int64>&  offsets,
    bsl::ostream&                           ostream,
    bslma::Allocator*                       allocator)
: CslSearchResultDecorator(component, allocator)
, d_offsets(offsets, allocator)
, d_ostream(ostream)
{
    // NOTHING
}

bool CslSearchOffsetDecorator::processRecord(
    const mqbc::ClusterStateRecordHeader& header,
    const bmqp_ctrlmsg::ClusterMessage&   record,
    const mqbsi::LedgerRecordId&          recordId)
{
    bsl::vector<bsls::Types::Int64>::const_iterator it =
        bsl::find(d_offsets.cbegin(), d_offsets.cend(), recordId.offset());
    if (it != d_offsets.cend()) {
        CslSearchResultDecorator::processRecord(header, record, recordId);
        // Remove processed offset.
        d_offsets.erase(it);
    }

    // return true (stop search) if d_offsets is empty.
    return d_offsets.empty();
}

void CslSearchOffsetDecorator::outputResult() const
{
    CslSearchResultDecorator::outputResult();

    // Print not found offsets
    if (!d_offsets.empty()) {
        d_ostream << '\n'
                  << "The following " << d_offsets.size()
                  << " offset(s) not found:" << '\n';
        bsl::vector<bsls::Types::Int64>::const_iterator it =
            d_offsets.cbegin();
        for (; it != d_offsets.cend(); ++it) {
            d_ostream << *it << '\n';
        }
    }
}

// ======================
// class CslSummaryResult
// ======================

CslSummaryResult::CslSummaryResult(
    bsl::ostream&                            ostream,
    const Parameters::ProcessCslRecordTypes& processCslRecordTypes,
    const QueueMap&                          queueMap,
    bslma::Allocator*                        allocator)
: d_ostream(ostream)
, d_processCslRecordTypes(processCslRecordTypes)
, d_recordCount()
, d_updateChoiceCount(allocator)
, d_queueMap(queueMap)
, d_allocator_p(allocator)
{
    // NOTHING
}

bool CslSummaryResult::processRecord(
    const mqbc::ClusterStateRecordHeader& header,
    const bmqp_ctrlmsg::ClusterMessage&   record,
    BSLS_ANNOTATION_UNUSED const mqbsi::LedgerRecordId& recordId)
{
    updateRecordCount(&d_recordCount, header.recordType());

    if (header.recordType() == mqbc::ClusterStateRecordType::e_UPDATE) {
        ++d_updateChoiceCount[record.choice().selectionId()];
    }

    return false;
}

void CslSummaryResult::outputResult() const
{
    if (d_processCslRecordTypes.d_snapshot) {
        d_recordCount.d_snapshotCount > 0
            ? (d_ostream << '\n'
                         << d_recordCount.d_snapshotCount << " snapshot")
            : d_ostream << "\nNo snapshot";
        d_ostream << " record(s) found.\n";
    }
    if (d_processCslRecordTypes.d_update) {
        if (d_recordCount.d_updateCount > 0) {
            d_ostream << '\n'
                      << d_recordCount.d_updateCount
                      << " update record(s) found, including:" << '\n';
            bsl::vector<const char*>           fields(d_allocator_p);
            bmqp_ctrlmsg::ClusterMessageChoice clusterMessageChoice(
                d_allocator_p);
            for (UpdateChoiceMap::const_iterator it =
                     d_updateChoiceCount.begin();
                 it != d_updateChoiceCount.end();
                 ++it) {
                clusterMessageChoice.makeSelection(it->first);
                fields.push_back(clusterMessageChoice.selectionName());
            }
            bmqu::AlignedPrinter printer(d_ostream, &fields);
            for (UpdateChoiceMap::const_iterator it =
                     d_updateChoiceCount.begin();
                 it != d_updateChoiceCount.end();
                 ++it) {
                printer << it->second;
            }
        }
        else {
            d_ostream << "\nNo update record(s) found." << '\n';
        }
    }
    if (d_processCslRecordTypes.d_commit) {
        d_recordCount.d_commitCount > 0
            ? (d_ostream << '\n'
                         << d_recordCount.d_commitCount << " commit")
            : d_ostream << "\nNo commit";
        d_ostream << " record(s) found.\n";
    }
    if (d_processCslRecordTypes.d_ack) {
        d_recordCount.d_ackCount > 0
            ? (d_ostream << '\n'
                         << d_recordCount.d_ackCount << " ack")
            : d_ostream << "\nNo ack";
        d_ostream << " record(s) found.\n";
    }

    // Print queues info
    const bsl::vector<bmqp_ctrlmsg::QueueInfo>& queueInfos =
        d_queueMap.queueInfos();
    if (!queueInfos.empty()) {
        d_ostream << '\n' << queueInfos.size() << " Queues found:" << '\n';
        bsl::vector<bmqp_ctrlmsg::QueueInfo>::const_iterator it =
            queueInfos.cbegin();
        for (; it != queueInfos.cend(); ++it) {
            d_ostream << *it << '\n';
        }
    }
}

}  // close package namespace
}  // close enterprise namespace
