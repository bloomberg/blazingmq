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

// MWC
#include <mwcu_alignedprinter.h>
#include <mwcu_memoutstream.h>

namespace BloombergLP {
namespace m_bmqstoragetool {

namespace {

// Helper to print data file meta data
void printDataFileMeta(bsl::ostream&           ostream,
                       mqbs::DataFileIterator* dataFile_p)
{
    if (!dataFile_p || !dataFile_p->isValid()) {
        return;
    }
    ostream << "\nDetails of data file: \n"
            << *dataFile_p->mappedFileDescriptor() << " "
            << dataFile_p->header();
}

// Helper to print journal file meta data
void printJournalFileMeta(bsl::ostream&              ostream,
                          mqbs::JournalFileIterator* journalFile_p)
{
    if (!journalFile_p || !journalFile_p->isValid()) {
        return;
    }

    ostream << "\nDetails of journal file: \n";
    ostream << "File descriptor: " << journalFile_p->mappedFileDescriptor()
            << '\n';
    mqbs::FileStoreProtocolPrinter::printHeader(
        ostream,
        journalFile_p->header(),
        *journalFile_p->mappedFileDescriptor());

    // Print journal-specific fields
    ostream << "Journal SyncPoint:\n";
    bsl::vector<const char*> fields;
    fields.push_back("Last Valid Record Offset");
    fields.push_back("Record Type");
    fields.push_back("Record Timestamp");
    fields.push_back("Record Epoch");
    fields.push_back("Last Valid SyncPoint Offset");
    fields.push_back("SyncPoint Timestamp");
    fields.push_back("SyncPoint Epoch");
    fields.push_back("SyncPoint SeqNum");
    fields.push_back("SyncPoint Primary NodeId");
    fields.push_back("SyncPoint Primary LeaseId");
    fields.push_back("SyncPoint DataFileOffset (DWORDS)");
    fields.push_back("SyncPoint QlistFileOffset (WORDS)");

    mwcu::AlignedPrinter printer(ostream, &fields);
    bsls::Types::Uint64  lastRecPos = journalFile_p->lastRecordPosition();
    printer << lastRecPos;
    if (0 == lastRecPos) {
        // No valid record
        printer << "** NA **"
                << "** NA **";
    }
    else {
        mqbs::OffsetPtr<const mqbs::RecordHeader> recHeader(
            journalFile_p->mappedFileDescriptor()->block(),
            lastRecPos);
        printer << recHeader->type();
        bdlt::Datetime      datetime;
        bsls::Types::Uint64 epochValue = recHeader->timestamp();
        const int           rc = bdlt::EpochUtil::convertFromTimeT64(&datetime,
                                                           epochValue);
        if (0 != rc) {
            printer << 0;
        }
        else {
            printer << datetime;
        }
        printer << epochValue;
    }

    const bsls::Types::Uint64 syncPointPos =
        journalFile_p->lastSyncPointPosition();

    printer << syncPointPos;
    if (0 == syncPointPos) {
        // No valid syncPoint
        printer << "** NA **"
                << "** NA **"
                << "** NA **"
                << "** NA **"
                << "** NA **"
                << "** NA **";
    }
    else {
        const mqbs::JournalOpRecord& syncPt = journalFile_p->lastSyncPoint();

        BSLS_ASSERT_OPT(mqbs::JournalOpType::e_SYNCPOINT == syncPt.type());

        bsls::Types::Uint64 epochValue = syncPt.header().timestamp();
        bdlt::Datetime      datetime;
        int rc = bdlt::EpochUtil::convertFromTimeT64(&datetime, epochValue);
        if (0 != rc) {
            printer << 0;
        }
        else {
            printer << datetime;
        }
        printer << epochValue;

        printer << syncPt.sequenceNum() << syncPt.primaryNodeId()
                << syncPt.primaryLeaseId() << syncPt.dataFileOffsetDwords()
                << syncPt.qlistFileOffsetWords();
    }
}

// Helper to print message GUID as a string
void outputGuidString(bsl::ostream&            ostream,
                      const bmqt::MessageGUID& messageGUID,
                      const bool               addNewLine = true)
{
    ostream << messageGUID;

    if (addNewLine)
        ostream << '\n';
}

// Helper to calculate and print outstanding ratio
void outputOutstandingRatio(bsl::ostream& ostream,
                            bsl::size_t   totalMessagesCount,
                            bsl::size_t   deletedMessagesCount)
{
    if (totalMessagesCount > 0) {
        bsl::size_t outstandingMessages = totalMessagesCount -
                                          deletedMessagesCount;
        ostream << "Outstanding ratio: "
                << bsl::round(float(outstandingMessages) /
                              float(totalMessagesCount) * 100.0)
                << "% (" << outstandingMessages << "/" << totalMessagesCount
                << ")" << '\n';
    }
}

// Helper to print summary of search result
void outputFooter(bsl::ostream& ostream, bsl::size_t foundMessagesCount)
{
    const char* captionForFound    = " message GUID(s) found.";
    const char* captionForNotFound = "No message GUID found.";
    foundMessagesCount > 0 ? (ostream << foundMessagesCount << captionForFound)
                           : ostream << captionForNotFound;
    ostream << '\n';
}

}  // close unnamed namespace

// ===========================
// class SearchResultDecorator
// ===========================

SearchResultDecorator::SearchResultDecorator(
    const bsl::shared_ptr<SearchResult> component,
    bslma::Allocator*                   allocator)
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

void SearchResultDecorator::outputResult(
    const bsl::unordered_set<bmqt::MessageGUID>& guidFilter)
{
    d_searchResult->outputResult(guidFilter);
}

// ====================================
// class SearchResultTimestampDecorator
// ====================================

bool SearchResultTimestampDecorator::stop(bsls::Types::Uint64 timestamp) const
{
    return timestamp >= d_timestampLt && !SearchResultDecorator::hasCache();
}

SearchResultTimestampDecorator::SearchResultTimestampDecorator(
    const bsl::shared_ptr<SearchResult> component,
    bsls::Types::Uint64                 timestampLt,
    bslma::Allocator*                   allocator)
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
    bsl::ostream&                     ostream,
    bslma::ManagedPtr<PayloadDumper>& payloadDumper,
    bslma::Allocator*                 allocator,
    const bool                        printImmediately,
    const bool                        eraseDeleted,
    const bool                        printOnDelete)
: d_ostream(ostream)
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
        if (auto it = d_guidMap.find(record.messageGUID());
            it != d_guidMap.end()) {
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
        for (const auto& guidData : d_guidList) {
            outputGuidData(guidData);
        }
    }

    outputFooter(d_ostream, d_printedMessagesCount);
}

void SearchShortResult::outputResult(
    const bsl::unordered_set<bmqt::MessageGUID>& guidFilter)
{
    // Remove guids from list that do not match filter
    bsl::erase_if(d_guidList, [&guidFilter](const GuidData& guidData) {
        return bsl::find(guidFilter.begin(),
                         guidFilter.end(),
                         guidData.first) == guidFilter.end();
    });

    outputResult();
}

void SearchShortResult::outputGuidData(GuidData guidData)
{
    outputGuidString(d_ostream, guidData.first);
    if (d_payloadDumper)
        d_payloadDumper->outputPayload(guidData.second);

    d_printedMessagesCount++;
}

bool SearchShortResult::hasCache() const
{
    return d_guidList.size() > 0;
}

// ========================
// class SearchDetailResult
// ========================

SearchDetailResult::SearchDetailResult(
    bsl::ostream&                     ostream,
    const QueueMap&                   queueMap,
    bslma::ManagedPtr<PayloadDumper>& payloadDumper,
    bslma::Allocator*                 allocator,
    const bool                        printImmediately,
    const bool                        eraseDeleted,
    const bool                        cleanUnprinted)
: d_ostream(ostream)
, d_queueMap(queueMap)
, d_payloadDumper(payloadDumper)
, d_printImmediately(printImmediately)
, d_eraseDeleted(eraseDeleted)
, d_cleanUnprinted(cleanUnprinted)
, d_printedMessagesCount(0)
, d_messagesDetails(allocator)
, d_messageIndexToGuidMap(allocator)
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
    if (auto it = d_messagesDetails.find(record.messageGUID());
        it != d_messagesDetails.end()) {
        it->second.addConfirmRecord(record, recordIndex, recordOffset);
    }

    return false;
}

bool SearchDetailResult::processDeletionRecord(
    const mqbs::DeletionRecord& record,
    bsls::Types::Uint64         recordIndex,
    bsls::Types::Uint64         recordOffset)
{
    if (auto it = d_messagesDetails.find(record.messageGUID());
        it != d_messagesDetails.end()) {
        if (d_printImmediately) {
            // Print message details immediately
            it->second.addDeleteRecord(record, recordIndex, recordOffset);
            outputMessageDetails(it->second);
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
    if (d_cleanUnprinted) {
        d_messageIndexToGuidMap.clear();
    }

    for (const auto& item : d_messageIndexToGuidMap) {
        const auto& messageDetails = d_messagesDetails.at(item.second);
        outputMessageDetails(messageDetails);
    }

    outputFooter(d_ostream, d_printedMessagesCount);
}

void SearchDetailResult::outputResult(
    const bsl::unordered_set<bmqt::MessageGUID>& guidFilter)
{
    // Remove guids from map that do not match filter
    bsl::erase_if(
        d_messageIndexToGuidMap,
        [&guidFilter](
            const bsl::pair<bsls::Types::Uint64, bmqt::MessageGUID>& pair) {
            return bsl::find(guidFilter.begin(),
                             guidFilter.end(),
                             pair.second) == guidFilter.end();
        });

    outputResult();
}

void SearchDetailResult::addMessageDetails(const mqbs::MessageRecord& record,
                                           bsls::Types::Uint64 recordIndex,
                                           bsls::Types::Uint64 recordOffset)
{
    d_messagesDetails.emplace(
        record.messageGUID(),
        MessageDetails(record, recordIndex, recordOffset, d_allocator_p));
    d_messageIndexToGuidMap.emplace(recordIndex, record.messageGUID());
}

void SearchDetailResult::deleteMessageDetails(
    MessagesDetails::iterator iterator)
{
    // Erase record from both maps
    d_messagesDetails.erase(iterator);
    d_messageIndexToGuidMap.erase(iterator->second.messageRecordIndex());
}

void SearchDetailResult::outputMessageDetails(
    const MessageDetails& messageDetails)
{
    messageDetails.print(d_ostream, d_queueMap);
    if (d_payloadDumper)
        d_payloadDumper->outputPayload(messageDetails.dataRecordOffset());

    d_printedMessagesCount++;
}

bool SearchDetailResult::hasCache() const
{
    return d_messagesDetails.size() > 0;
}

// ========================
// class SearchAllDecorator
// ========================
SearchAllDecorator::SearchAllDecorator(
    const bsl::shared_ptr<SearchResult> component,
    bslma::Allocator*                   allocator)
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
    const bsl::shared_ptr<SearchResult> component,
    bsl::ostream&                       ostream,
    bslma::Allocator*                   allocator)
: SearchResultDecorator(component, allocator)
, d_ostream(ostream)
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
    if (auto it = d_guids.find(record.messageGUID()); it != d_guids.end()) {
        d_guids.erase(it);
        d_deletedMessagesCount++;
    }

    return false;
}

void SearchOutstandingDecorator::outputResult()
{
    SearchResultDecorator::outputResult();
    outputOutstandingRatio(d_ostream,
                           d_foundMessagesCount,
                           d_deletedMessagesCount);
}

// =======================================
// class SearchPartiallyConfirmedDecorator
// =======================================
SearchPartiallyConfirmedDecorator::SearchPartiallyConfirmedDecorator(
    const bsl::shared_ptr<SearchResult> component,
    bsl::ostream&                       ostream,
    bslma::Allocator*                   allocator)
: SearchResultDecorator(component, allocator)
, d_ostream(ostream)
, d_foundMessagesCount(0)
, d_deletedMessagesCount(0)
, d_partiallyConfirmedGUIDS(allocator)
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
    // Init confirm count
    d_partiallyConfirmedGUIDS[record.messageGUID()] = 0;
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
    if (auto it = d_partiallyConfirmedGUIDS.find(record.messageGUID());
        it != d_partiallyConfirmedGUIDS.end()) {
        // Message is partially confirmed, increase counter.
        it->second++;
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
    if (auto it = d_partiallyConfirmedGUIDS.find(record.messageGUID());
        it != d_partiallyConfirmedGUIDS.end()) {
        // Message is confirmed, remove it.
        d_partiallyConfirmedGUIDS.erase(it);
        d_deletedMessagesCount++;
    }

    return false;
}

void SearchPartiallyConfirmedDecorator::outputResult()
{
    // Build a filter of partially confirmed guids
    bsl::unordered_set<bmqt::MessageGUID> guidFilter(d_allocator_p);
    for (const auto& pair : d_partiallyConfirmedGUIDS) {
        if (pair.second > 0) {
            guidFilter.insert(pair.first);
        }
    }
    SearchResultDecorator::outputResult(guidFilter);
    outputOutstandingRatio(d_ostream,
                           d_foundMessagesCount,
                           d_deletedMessagesCount);
}

// =========================
// class SearchGuidDecorator
// =========================
SearchGuidDecorator::SearchGuidDecorator(
    const bsl::shared_ptr<SearchResult> component,
    const bsl::vector<bsl::string>&     guids,
    bsl::ostream&                       ostream,
    bool                                withDetails,
    bslma::Allocator*                   allocator)
: SearchResultDecorator(component, allocator)
, d_ostream(ostream)
, d_withDetails(withDetails)
, d_guidsMap(allocator)
, d_guids(allocator)
{
    // Build MessageGUID->StrGUID Map
    for (const auto& guidStr : guids) {
        bmqt::MessageGUID guid;
        d_guidsMap[guid.fromHex(guidStr.c_str())] =
            d_guids.insert(d_guids.cend(), guidStr);
    }
}

bool SearchGuidDecorator::processMessageRecord(
    const mqbs::MessageRecord& record,
    bsls::Types::Uint64        recordIndex,
    bsls::Types::Uint64        recordOffset)
{
    if (auto it = d_guidsMap.find(record.messageGUID());
        it != d_guidsMap.end()) {
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
    // Print non found GUIDs
    if (!d_guids.empty()) {
        d_ostream << '\n'
                  << "The following " << d_guids.size()
                  << " GUID(s) not found:" << '\n';
        for (const auto& guid : d_guids) {
            d_ostream << guid << '\n';
        }
    }
}

// ======================
// class SummaryProcessor
// ======================

SummaryProcessor::SummaryProcessor(bsl::ostream&              ostream,
                                   mqbs::JournalFileIterator* journalFile_p,
                                   mqbs::DataFileIterator*    dataFile_p,
                                   bslma::Allocator*          allocator)
: d_ostream(ostream)
, d_journalFile_p(journalFile_p)
, d_dataFile_p(dataFile_p)
, d_foundMessagesCount(0)
, d_deletedMessagesCount(0)
, d_partiallyConfirmedGUIDS(allocator)
{
    // NOTHING
}

bool SummaryProcessor::processMessageRecord(
    const mqbs::MessageRecord& record,
    BSLS_ANNOTATION_UNUSED bsls::Types::Uint64 recordIndex,
    BSLS_ANNOTATION_UNUSED bsls::Types::Uint64 recordOffset)
{
    d_partiallyConfirmedGUIDS[record.messageGUID()] = 0;
    d_foundMessagesCount++;

    return false;
}

bool SummaryProcessor::processConfirmRecord(
    const mqbs::ConfirmRecord& record,
    BSLS_ANNOTATION_UNUSED bsls::Types::Uint64 recordIndex,
    BSLS_ANNOTATION_UNUSED bsls::Types::Uint64 recordOffset)
{
    if (auto it = d_partiallyConfirmedGUIDS.find(record.messageGUID());
        it != d_partiallyConfirmedGUIDS.end()) {
        // Message is partially confirmed, increase counter.
        it->second++;
    }

    return false;
}

bool SummaryProcessor::processDeletionRecord(
    const mqbs::DeletionRecord& record,
    BSLS_ANNOTATION_UNUSED bsls::Types::Uint64 recordIndex,
    BSLS_ANNOTATION_UNUSED bsls::Types::Uint64 recordOffset)
{
    if (auto it = d_partiallyConfirmedGUIDS.find(record.messageGUID());
        it != d_partiallyConfirmedGUIDS.end()) {
        // Message is confirmed, remove it.
        d_partiallyConfirmedGUIDS.erase(it);
        d_deletedMessagesCount++;
    }

    return false;
}

void SummaryProcessor::outputResult()
{
    // Calculate number of partially confirmed messages
    size_t partiallyConfirmedMessagesCount = 0;
    for (const auto& item : d_partiallyConfirmedGUIDS) {
        auto confirmCount = item.second;
        if (confirmCount > 0) {
            partiallyConfirmedMessagesCount++;
        }
    }

    if (d_foundMessagesCount == 0) {
        d_ostream << "No messages found." << '\n';
        return;  // RETURN
    }

    d_ostream << d_foundMessagesCount << " message(s) found." << '\n';
    d_ostream << "Number of confirmed messages: " << d_deletedMessagesCount
              << '\n';
    d_ostream << "Number of partially confirmed messages: "
              << partiallyConfirmedMessagesCount << '\n';
    d_ostream << "Number of outstanding messages: "
              << (d_foundMessagesCount - d_deletedMessagesCount) << '\n';

    outputOutstandingRatio(d_ostream,
                           d_foundMessagesCount,
                           d_deletedMessagesCount);

    // Print meta data of opened files
    printJournalFileMeta(d_ostream, d_journalFile_p);
    printDataFileMeta(d_ostream, d_dataFile_p);
}

void SummaryProcessor::outputResult(
    BSLS_ANNOTATION_UNUSED const bsl::unordered_set<bmqt::MessageGUID>&
                                 guidFilter)
{
    outputResult();
}

}  // close package namespace
}  // close enterprise namespace
