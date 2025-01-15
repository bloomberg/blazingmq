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
#include <m_bmqstoragetool_searchresult.h>

// MQB
#include <mqbs_filestoreprotocolprinter.h>
#include <mqbs_filestoreprotocolutil.h>

// BMQ
#include <bmqu_alignedprinter.h>
#include <bmqu_memoutstream.h>

// BDE
#include <bsl_cmath.h>

namespace BloombergLP {
namespace m_bmqstoragetool {

namespace {

// Helper to calculate outstanding ratio
OutstandingPrintBundle
calculateOutstandingRatio(bsl::size_t totalMessagesCount,
                          bsl::size_t deletedMessagesCount)
{
    BSLS_ASSERT_SAFE(totalMessagesCount > 0);
    bsl::size_t outstandingMessages = totalMessagesCount -
                                      deletedMessagesCount;
    return OutstandingPrintBundle(
        totalMessagesCount,
        outstandingMessages,
        static_cast<int>(bsl::floor(float(outstandingMessages) /
                                        float(totalMessagesCount) * 100.0f +
                                    0.5f)));
}

}  // close unnamed namespace

// ==================
// class SearchResult
// ==================

SearchResult::~SearchResult()
{
    // NOTHING
}

// ===========================
// class SearchResultDecorator
// ===========================

SearchResultDecorator::SearchResultDecorator(
    const bsl::shared_ptr<SearchResult>& component,
    bslma::Allocator*                    allocator)
: d_searchResult(component)
, d_allocator_p(allocator)
{
}

bool SearchResultDecorator::processMessageRecord(
    const mqbs::MessageRecord& record,
    bsls::Types::Uint64        recordIndex,
    bsls::Types::Uint64        recordOffset)
{
    return d_searchResult->processMessageRecord(record,
                                                recordIndex,
                                                recordOffset);
}

bool SearchResultDecorator::processConfirmRecord(
    const mqbs::ConfirmRecord& record,
    bsls::Types::Uint64        recordIndex,
    bsls::Types::Uint64        recordOffset)
{
    return d_searchResult->processConfirmRecord(record,
                                                recordIndex,
                                                recordOffset);
}

bool SearchResultDecorator::processDeletionRecord(
    const mqbs::DeletionRecord& record,
    bsls::Types::Uint64         recordIndex,
    bsls::Types::Uint64         recordOffset)
{
    return d_searchResult->processDeletionRecord(record,
                                                 recordIndex,
                                                 recordOffset);
}

void SearchResultDecorator::outputResult()
{
    d_searchResult->outputResult();
}

void SearchResultDecorator::outputResult(const GuidsList& guidFilter)
{
    d_searchResult->outputResult(guidFilter);
}

const bsl::shared_ptr<Printer>& SearchResultDecorator::printer()
{
    return d_searchResult->printer();
}

// ====================================
// class SearchResultTimestampDecorator
// ====================================

bool SearchResultTimestampDecorator::stop(bsls::Types::Uint64 timestamp) const
{
    return timestamp >= d_timestampLt && !SearchResultDecorator::hasCache();
}

SearchResultTimestampDecorator::SearchResultTimestampDecorator(
    const bsl::shared_ptr<SearchResult>& component,
    bsls::Types::Uint64                  timestampLt,
    bslma::Allocator*                    allocator)
: SearchResultDecorator(component, allocator)
, d_timestampLt(timestampLt)
{
}

bool SearchResultTimestampDecorator::processMessageRecord(
    const mqbs::MessageRecord& record,
    bsls::Types::Uint64        recordIndex,
    bsls::Types::Uint64        recordOffset)
{
    return SearchResultDecorator::processMessageRecord(record,
                                                       recordIndex,
                                                       recordOffset) ||
           stop(record.header().timestamp());
}

bool SearchResultTimestampDecorator::processConfirmRecord(
    const mqbs::ConfirmRecord& record,
    bsls::Types::Uint64        recordIndex,
    bsls::Types::Uint64        recordOffset)
{
    return SearchResultDecorator::processConfirmRecord(record,
                                                       recordIndex,
                                                       recordOffset) ||
           stop(record.header().timestamp());
}

bool SearchResultTimestampDecorator::processDeletionRecord(
    const mqbs::DeletionRecord& record,
    bsls::Types::Uint64         recordIndex,
    bsls::Types::Uint64         recordOffset)
{
    return SearchResultDecorator::processDeletionRecord(record,
                                                        recordIndex,
                                                        recordOffset) ||
           stop(record.header().timestamp());
}

// =======================
// class SearchShortResult
// =======================

SearchShortResult::SearchShortResult(
    const bsl::shared_ptr<Printer>&   printer,
    bslma::ManagedPtr<PayloadDumper>& payloadDumper,
    bool                              printImmediately,
    bool                              eraseDeleted,
    bool                              printOnDelete,
    bslma::Allocator*                 allocator)
: d_printer(printer)
, d_payloadDumper(payloadDumper)
, d_printImmediately(printImmediately)
, d_eraseDeleted(eraseDeleted)
, d_printOnDelete(printOnDelete)
, d_printedMessagesCount(0)
, d_guidMap(allocator)
, d_guidList(allocator)
, d_allocator_p(allocator)
{
    // NOTHING
}

bool SearchShortResult::processMessageRecord(
    const mqbs::MessageRecord& record,
    BSLS_ANNOTATION_UNUSED bsls::Types::Uint64 recordIndex,
    BSLS_ANNOTATION_UNUSED bsls::Types::Uint64 recordOffset)
{
    GuidData guidData = bsl::make_pair(record.messageGUID(),
                                       record.messageOffsetDwords());

    if (d_printImmediately) {
        outputGuidData(guidData);
    }
    else {
        d_guidMap[record.messageGUID()] = d_guidList.insert(d_guidList.cend(),
                                                            guidData);
    }

    return false;
}

bool SearchShortResult::processConfirmRecord(
    BSLS_ANNOTATION_UNUSED const mqbs::ConfirmRecord& record,
    BSLS_ANNOTATION_UNUSED bsls::Types::Uint64 recordIndex,
    BSLS_ANNOTATION_UNUSED bsls::Types::Uint64 recordOffset)
{
    return false;
}

bool SearchShortResult::processDeletionRecord(
    const mqbs::DeletionRecord& record,
    BSLS_ANNOTATION_UNUSED bsls::Types::Uint64 recordIndex,
    BSLS_ANNOTATION_UNUSED bsls::Types::Uint64 recordOffset)
{
    if (!d_printImmediately && (d_printOnDelete || d_eraseDeleted)) {
        GuidDataMap::iterator it = d_guidMap.find(record.messageGUID());
        if (it != d_guidMap.end()) {
            if (d_printOnDelete) {
                outputGuidData(*it->second);
            }
            if (d_eraseDeleted) {
                d_guidList.erase(it->second);
                d_guidMap.erase(it);
            }
        }
    }
    return false;
}

void SearchShortResult::outputResult()
{
    if (!d_printOnDelete) {
        // Print results that were not printed on Delete record processing
        bsl::list<GuidData>::const_iterator it = d_guidList.cbegin();
        for (; it != d_guidList.cend(); ++it) {
            outputGuidData(*it);
        }
    }

    d_printer->printFooter(d_printedMessagesCount);
}

void SearchShortResult::outputResult(const GuidsList& guidFilter)
{
    // Print only Guids from `guidFilter`
    if (!d_printOnDelete) {
        GuidsList::const_iterator it = guidFilter.cbegin();
        for (; it != guidFilter.cend(); ++it) {
            GuidDataMap::const_iterator gIt = d_guidMap.find(*it);
            if (gIt != d_guidMap.end()) {
                outputGuidData(*gIt->second);
            }
            else {
                // this should not happen
                d_printer->printGuidNotFound(*it);
            }
        }
    }

    d_printer->printFooter(d_printedMessagesCount);
}

void SearchShortResult::outputGuidData(const GuidData& guidData)
{
    d_printer->printGuid(guidData.first);
    if (d_payloadDumper)
        d_payloadDumper->outputPayload(guidData.second);

    d_printedMessagesCount++;
}

bool SearchShortResult::hasCache() const
{
    return !d_guidList.empty();
}

const bsl::shared_ptr<Printer>& SearchShortResult::printer()
{
    return d_printer;
}

// ========================
// class SearchDetailResult
// ========================

SearchDetailResult::SearchDetailResult(
    const QueueMap&                   queueMap,
    const bsl::shared_ptr<Printer>&   printer,
    bslma::ManagedPtr<PayloadDumper>& payloadDumper,
    bool                              printImmediately,
    bool                              eraseDeleted,
    bool                              cleanUnprinted,
    bslma::Allocator*                 allocator)
: d_queueMap(queueMap)
, d_printer(printer)
, d_payloadDumper(payloadDumper)
, d_printImmediately(printImmediately)
, d_eraseDeleted(eraseDeleted)
, d_cleanUnprinted(cleanUnprinted)
, d_printedMessagesCount(0)
, d_messageDetailsList(allocator)
, d_messageDetailsMap(allocator)
, d_allocator_p(allocator)
{
    // NOTHING
}

bool SearchDetailResult::processMessageRecord(
    const mqbs::MessageRecord& record,
    bsls::Types::Uint64        recordIndex,
    bsls::Types::Uint64        recordOffset)
{
    // Store record details for further output
    addMessageDetails(record, recordIndex, recordOffset);

    return false;
}

bool SearchDetailResult::processConfirmRecord(
    const mqbs::ConfirmRecord& record,
    bsls::Types::Uint64        recordIndex,
    bsls::Types::Uint64        recordOffset)
{
    // Store record details for further output
    DetailsMap::iterator it = d_messageDetailsMap.find(record.messageGUID());
    if (it != d_messageDetailsMap.end()) {
        it->second->addConfirmRecord(record, recordIndex, recordOffset);
    }

    return false;
}

bool SearchDetailResult::processDeletionRecord(
    const mqbs::DeletionRecord& record,
    bsls::Types::Uint64         recordIndex,
    bsls::Types::Uint64         recordOffset)
{
    DetailsMap::iterator it = d_messageDetailsMap.find(record.messageGUID());
    if (it != d_messageDetailsMap.end()) {
        if (d_printImmediately) {
            // Print message details immediately
            it->second->addDeleteRecord(record, recordIndex, recordOffset);
            outputMessageDetails(*it->second);
        }
        if (d_eraseDeleted) {
            // Delete record if it is not needed anymore
            deleteMessageDetails(it);
        }
    }

    return false;
}

void SearchDetailResult::outputResult()
{
    if (!d_cleanUnprinted) {
        DetailsList::const_iterator it = d_messageDetailsList.cbegin();
        for (; it != d_messageDetailsList.cend(); ++it) {
            outputMessageDetails(*it);
        }
    }

    d_printer->printFooter(d_printedMessagesCount);
}

void SearchDetailResult::outputResult(const GuidsList& guidFilter)
{
    // Print only Guids from `guidFilter`
    if (!d_cleanUnprinted) {
        GuidsList::const_iterator it = guidFilter.cbegin();
        for (; it != guidFilter.cend(); ++it) {
            DetailsMap::const_iterator dIt = d_messageDetailsMap.find(*it);
            if (dIt != d_messageDetailsMap.end()) {
                outputMessageDetails(*dIt->second);
            }
            else {
                // this should not happen
                d_printer->printGuidNotFound(*it);
            }
        }
    }

    d_printer->printFooter(d_printedMessagesCount);
}

void SearchDetailResult::addMessageDetails(const mqbs::MessageRecord& record,
                                           bsls::Types::Uint64 recordIndex,
                                           bsls::Types::Uint64 recordOffset)
{
    d_messageDetailsMap.emplace(
        record.messageGUID(),
        d_messageDetailsList.insert(d_messageDetailsList.cend(),
                                    MessageDetails(record,
                                                   recordIndex,
                                                   recordOffset,
                                                   d_queueMap,
                                                   d_allocator_p)));
}

void SearchDetailResult::deleteMessageDetails(DetailsMap::iterator iterator)
{
    // Erase record from both containers
    d_messageDetailsList.erase(iterator->second);
    d_messageDetailsMap.erase(iterator);
}

void SearchDetailResult::outputMessageDetails(
    const MessageDetails& messageDetails)
{
    d_printer->printMessage(messageDetails);
    if (d_payloadDumper)
        d_payloadDumper->outputPayload(messageDetails.dataRecordOffset());

    d_printedMessagesCount++;
}

bool SearchDetailResult::hasCache() const
{
    return !d_messageDetailsMap.empty();
}

const bsl::shared_ptr<Printer>& SearchDetailResult::printer()
{
    return d_printer;
}

// ========================
// class SearchAllDecorator
// ========================

SearchAllDecorator::SearchAllDecorator(
    const bsl::shared_ptr<SearchResult>& component,
    bslma::Allocator*                    allocator)
: SearchResultDecorator(component, allocator)
{
    // NOTHING
}

bool SearchAllDecorator::processMessageRecord(
    const mqbs::MessageRecord& record,
    bsls::Types::Uint64        recordIndex,
    bsls::Types::Uint64        recordOffset)
{
    SearchResultDecorator::processMessageRecord(record,
                                                recordIndex,
                                                recordOffset);
    return false;
}

// ================================
// class SearchOutstandingDecorator
// ================================
SearchOutstandingDecorator::SearchOutstandingDecorator(
    const bsl::shared_ptr<SearchResult>& component,
    bslma::Allocator*                    allocator)
: SearchResultDecorator(component, allocator)
, d_foundMessagesCount(0)
, d_deletedMessagesCount(0)
, d_guids(allocator)
{
    // NOTHING
}

bool SearchOutstandingDecorator::processMessageRecord(
    const mqbs::MessageRecord& record,
    bsls::Types::Uint64        recordIndex,
    bsls::Types::Uint64        recordOffset)
{
    SearchResultDecorator::processMessageRecord(record,
                                                recordIndex,
                                                recordOffset);
    d_guids.insert(record.messageGUID());
    d_foundMessagesCount++;
    return false;
}

bool SearchOutstandingDecorator::processDeletionRecord(
    const mqbs::DeletionRecord& record,
    bsls::Types::Uint64         recordIndex,
    bsls::Types::Uint64         recordOffset)
{
    SearchResultDecorator::processDeletionRecord(record,
                                                 recordIndex,
                                                 recordOffset);
    bsl::unordered_set<bmqt::MessageGUID>::iterator it = d_guids.find(
        record.messageGUID());
    if (it != d_guids.end()) {
        d_guids.erase(it);
        d_deletedMessagesCount++;
    }

    return false;
}

void SearchOutstandingDecorator::outputResult()
{
    SearchResultDecorator::outputResult();
    if (d_foundMessagesCount > 0) {
        printer()->printOutstandingRatio(
            calculateOutstandingRatio(d_foundMessagesCount,
                                      d_deletedMessagesCount));
    }
}

// =======================================
// class SearchPartiallyConfirmedDecorator
// =======================================

SearchPartiallyConfirmedDecorator::SearchPartiallyConfirmedDecorator(
    const bsl::shared_ptr<SearchResult>& component,
    bslma::Allocator*                    allocator)
: SearchResultDecorator(component, allocator)
, d_foundMessagesCount(0)
, d_deletedMessagesCount(0)
, d_guidsList(allocator)
, d_notConfirmedGuids(allocator)
, d_partiallyConfirmedGuids(allocator)
{
    // NOTHING
}

bool SearchPartiallyConfirmedDecorator::processMessageRecord(
    const mqbs::MessageRecord& record,
    bsls::Types::Uint64        recordIndex,
    bsls::Types::Uint64        recordOffset)
{
    SearchResultDecorator::processMessageRecord(record,
                                                recordIndex,
                                                recordOffset);
    d_notConfirmedGuids.emplace(record.messageGUID(),
                                d_guidsList.insert(d_guidsList.cend(),
                                                   record.messageGUID()));
    d_foundMessagesCount++;
    return false;
}

bool SearchPartiallyConfirmedDecorator::processConfirmRecord(
    const mqbs::ConfirmRecord& record,
    bsls::Types::Uint64        recordIndex,
    bsls::Types::Uint64        recordOffset)
{
    SearchResultDecorator::processConfirmRecord(record,
                                                recordIndex,
                                                recordOffset);
    GuidsMap::iterator it = d_notConfirmedGuids.find(record.messageGUID());
    if (it != d_notConfirmedGuids.end()) {
        // Message is partially confirmed, move it to the dedicated map.
        d_partiallyConfirmedGuids.emplace(*it);
        d_notConfirmedGuids.erase(it);
    }

    return false;
}

bool SearchPartiallyConfirmedDecorator::processDeletionRecord(
    const mqbs::DeletionRecord& record,
    bsls::Types::Uint64         recordIndex,
    bsls::Types::Uint64         recordOffset)
{
    SearchResultDecorator::processDeletionRecord(record,
                                                 recordIndex,
                                                 recordOffset);
    GuidsMap::iterator it = d_partiallyConfirmedGuids.find(
        record.messageGUID());
    if (it != d_partiallyConfirmedGuids.end()) {
        // Message is confirmed, remove it.
        d_guidsList.erase(it->second);
        d_partiallyConfirmedGuids.erase(it);
        d_deletedMessagesCount++;
    }

    return false;
}

void SearchPartiallyConfirmedDecorator::outputResult()
{
    GuidsMap::const_iterator it = d_notConfirmedGuids.cbegin();
    for (; it != d_notConfirmedGuids.cend(); ++it) {
        d_guidsList.erase(it->second);
    }
    SearchResultDecorator::outputResult(d_guidsList);

    if (d_foundMessagesCount > 0) {
        printer()->printOutstandingRatio(
            calculateOutstandingRatio(d_foundMessagesCount,
                                      d_deletedMessagesCount));
    }
}

// =========================
// class SearchGuidDecorator
// =========================
SearchGuidDecorator::SearchGuidDecorator(
    const bsl::shared_ptr<SearchResult>& component,
    const bsl::vector<bsl::string>&      guids,
    bool                                 withDetails,
    bslma::Allocator*                    allocator)
: SearchResultDecorator(component, allocator)
, d_withDetails(withDetails)
, d_guidsMap(allocator)
, d_guids(allocator)
{
    // Build MessageGUID->StrGUID Map
    bsl::vector<bsl::string>::const_iterator it = guids.cbegin();
    for (; it != guids.cend(); ++it) {
        bmqt::MessageGUID guid;
        guid.fromHex(it->c_str());
        d_guidsMap.emplace(guid, d_guids.insert(d_guids.cend(), guid));
    }
}

bool SearchGuidDecorator::processMessageRecord(
    const mqbs::MessageRecord& record,
    bsls::Types::Uint64        recordIndex,
    bsls::Types::Uint64        recordOffset)
{
    GuidsMap::iterator it = d_guidsMap.find(record.messageGUID());
    if (it != d_guidsMap.end()) {
        SearchResultDecorator::processMessageRecord(record,
                                                    recordIndex,
                                                    recordOffset);
        // Remove processed GUID from map.
        d_guids.erase(it->second);
        d_guidsMap.erase(it);
    }

    // return true (stop search) if no detail is needed and map is empty.
    return (!d_withDetails && d_guidsMap.empty());
}

bool SearchGuidDecorator::processDeletionRecord(
    const mqbs::DeletionRecord& record,
    bsls::Types::Uint64         recordIndex,
    bsls::Types::Uint64         recordOffset)
{
    SearchResultDecorator::processDeletionRecord(record,
                                                 recordIndex,
                                                 recordOffset);
    // return true (stop search) when details needed and search is done (map is
    // empty).
    return (d_withDetails && d_guidsMap.empty());
}

void SearchGuidDecorator::outputResult()
{
    SearchResultDecorator::outputResult();
    printer()->printGuidsNotFound(d_guids);
}

// ======================
// class SummaryProcessor
// ======================

SummaryProcessor::SummaryProcessor(const bsl::shared_ptr<Printer>& printer,
                                   mqbs::JournalFileIterator* journalFile_p,
                                   mqbs::DataFileIterator*    dataFile_p,
                                   bslma::Allocator*          allocator)
: d_printer(printer)
, d_journalFile_p(journalFile_p)
, d_dataFile_p(dataFile_p)
, d_foundMessagesCount(0)
, d_deletedMessagesCount(0)
, d_notConfirmedGuids(allocator)
, d_partiallyConfirmedGuids(allocator)
, d_allocator_p(allocator)
{
    // NOTHING
}

bool SummaryProcessor::processMessageRecord(
    const mqbs::MessageRecord& record,
    BSLS_ANNOTATION_UNUSED bsls::Types::Uint64 recordIndex,
    BSLS_ANNOTATION_UNUSED bsls::Types::Uint64 recordOffset)
{
    d_notConfirmedGuids.emplace(record.messageGUID());
    d_foundMessagesCount++;

    return false;
}

bool SummaryProcessor::processConfirmRecord(
    const mqbs::ConfirmRecord& record,
    BSLS_ANNOTATION_UNUSED bsls::Types::Uint64 recordIndex,
    BSLS_ANNOTATION_UNUSED bsls::Types::Uint64 recordOffset)
{
    GuidsSet::iterator it = d_notConfirmedGuids.find(record.messageGUID());
    if (it != d_notConfirmedGuids.end()) {
        // Message is partially confirmed, move it to the dedeicated set.
        d_partiallyConfirmedGuids.emplace(*it);
        d_notConfirmedGuids.erase(it);
    }

    return false;
}

bool SummaryProcessor::processDeletionRecord(
    const mqbs::DeletionRecord& record,
    BSLS_ANNOTATION_UNUSED bsls::Types::Uint64 recordIndex,
    BSLS_ANNOTATION_UNUSED bsls::Types::Uint64 recordOffset)
{
    GuidsSet::iterator it = d_partiallyConfirmedGuids.find(
        record.messageGUID());
    if (it != d_partiallyConfirmedGuids.end()) {
        // Message is confirmed, remove it.
        d_partiallyConfirmedGuids.erase(it);
        d_deletedMessagesCount++;
    }

    return false;
}

void SummaryProcessor::outputResult()
{
    printer()->printFooter(d_foundMessagesCount);

    if (d_foundMessagesCount == 0) {
        return;  // RETURN
    }

    printer()->printSummary(d_foundMessagesCount,
                            d_deletedMessagesCount,
                            d_partiallyConfirmedGuids.size());

    printer()->printOutstandingRatio(
        calculateOutstandingRatio(d_foundMessagesCount,
                                  d_deletedMessagesCount));

    // Print meta data of opened files
    printer()->printJournalFileMeta(d_journalFile_p, d_allocator_p);
    printer()->printDataFileMeta(d_dataFile_p);
}

void SummaryProcessor::outputResult(
    BSLS_ANNOTATION_UNUSED const GuidsList& guidFilter)
{
    outputResult();
}

const bsl::shared_ptr<Printer>& SummaryProcessor::printer()
{
    return d_printer;
}

}  // close package namespace
}  // close enterprise namespace
