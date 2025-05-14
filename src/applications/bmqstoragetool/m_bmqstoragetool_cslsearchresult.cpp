// Copyright 2014-2025 Bloomberg Finance L.P.
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

// Helper to update record counters
void updateRecordCount(CslRecordCount*                    recordCount_p,
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

}  // close unnamed namespace

// =====================
// class CslSearchResult
// =====================

CslSearchResult::~CslSearchResult()
{
    // NOTHING
}

// ==========================
// class CslSearchShortResult
// ==========================

CslSearchShortResult::CslSearchShortResult(
    const bsl::shared_ptr<CslPrinter>&       printer,
    const Parameters::ProcessCslRecordTypes& processCslRecordTypes,
    bslma::Allocator*                        allocator)
: d_printer(printer)
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
    printer()->printShortResult(header, recordId);

    updateRecordCount(&d_recordCount, header.recordType());

    return false;
}

void CslSearchShortResult::outputResult() const
{
    // Print summary counters
    printer()->printFooter(d_recordCount, d_processCslRecordTypes);
}

const bsl::shared_ptr<CslPrinter>& CslSearchShortResult::printer() const
{
    return d_printer;
}

// ===========================
// class CslSearchDetailResult
// ===========================

CslSearchDetailResult::CslSearchDetailResult(
    const bsl::shared_ptr<CslPrinter>&       printer,
    const Parameters::ProcessCslRecordTypes& processCslRecordTypes,
    bslma::Allocator*                        allocator)
: d_printer(printer)
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
    printer()->printDetailResult(record, header, recordId);

    updateRecordCount(&d_recordCount, header.recordType());

    return false;
}

void CslSearchDetailResult::outputResult() const
{
    // Print summary counters
    d_printer->printFooter(d_recordCount, d_processCslRecordTypes);
}

const bsl::shared_ptr<CslPrinter>& CslSearchDetailResult::printer() const
{
    return d_printer;
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

const bsl::shared_ptr<CslPrinter>& CslSearchResultDecorator::printer() const
{
    return d_searchResult->printer();
}

// ======================================
// class CslSearchSequenceNumberDecorator
// ======================================

CslSearchSequenceNumberDecorator::CslSearchSequenceNumberDecorator(
    const bsl::shared_ptr<CslSearchResult>&     component,
    const bsl::vector<CompositeSequenceNumber>& seqNums,
    bslma::Allocator*                           allocator)
: CslSearchResultDecorator(component, allocator)
, d_seqNums(seqNums, allocator)
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

    // Print non found sequence numbers
    if (!d_seqNums.empty()) {
        printer()->printCompositesNotFound(d_seqNums);
    }
}

// ==============================
// class CslSearchOffsetDecorator
// ==============================

CslSearchOffsetDecorator::CslSearchOffsetDecorator(
    const bsl::shared_ptr<CslSearchResult>& component,
    const bsl::vector<bsls::Types::Int64>&  offsets,
    bslma::Allocator*                       allocator)
: CslSearchResultDecorator(component, allocator)
, d_offsets(offsets, allocator)
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

    // Print non found offsets
    if (!d_offsets.empty()) {
        printer()->printOffsetsNotFound(d_offsets);
    }
}

// ======================
// class CslSummaryResult
// ======================

CslSummaryResult::CslSummaryResult(
    const bsl::shared_ptr<CslPrinter>&       printer,
    const Parameters::ProcessCslRecordTypes& processCslRecordTypes,
    const QueueMap&                          queueMap,
    unsigned int                             cslSummaryQueuesLimit,
    bslma::Allocator*                        allocator)
: d_printer(printer)
, d_processCslRecordTypes(processCslRecordTypes)
, d_recordCount()
, d_updateChoiceCount(allocator)
, d_queueMap(queueMap)
, d_cslSummaryQueuesLimit(cslSummaryQueuesLimit)
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
    printer()->printSummaryResult(d_recordCount,
                                  d_updateChoiceCount,
                                  d_queueMap,
                                  d_processCslRecordTypes,
                                  d_cslSummaryQueuesLimit);
}

const bsl::shared_ptr<CslPrinter>& CslSummaryResult::printer() const
{
    return d_printer;
}

}  // close package namespace
}  // close enterprise namespace
