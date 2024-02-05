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

// m_bmqstoragetool_searchresult.cpp -*-C++-*-

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

// Helpers to print journal and data files meta
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

void printJournalFileMeta(bsl::ostream&              ostream,
                          mqbs::JournalFileIterator* journalFile_p)
{
    if (!journalFile_p || !journalFile_p->isValid()) {
        return;
    }

    ostream << "\nDetails of journal file: \n";
    ostream << journalFile_p->mappedFileDescriptor();
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
        int rc = bdlt::EpochUtil::convertFromTimeT64(&datetime, epochValue);
        if (0 != rc) {
            printer << 0;
        }
        else {
            printer << datetime;
        }
        printer << epochValue;
    }

    bsls::Types::Uint64 syncPointPos = journalFile_p->lastSyncPointPosition();

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

void outputGuidString(bsl::ostream&            ostream,
                      const bmqt::MessageGUID& messageGUID,
                      const bool               addNewLine = true)
{
    ostream << messageGUID;

    if (addNewLine)
        ostream << '\n';
}

void outputOutstandingRatio(bsl::ostream& ostream,
                            std::size_t   totalMessagesCount,
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

void outputFooter(bsl::ostream& ostream, std::size_t foundMessagesCount)
{
    const char* captionForFound    = " message GUID(s) found.";
    const char* captionForNotFound = "No message GUID found.";
    foundMessagesCount > 0 ? (ostream << foundMessagesCount << captionForFound)
                           : ostream << captionForNotFound;
    ostream << '\n';
}

}  // close unnamed namespace

// ==================
// class SearchResult
// ==================

SearchResult::SearchResult(bsl::ostream&           ostream,
                           bool                    withDetails,
                           bool                    dumpPayload,
                           unsigned int            dumpLimit,
                           mqbs::DataFileIterator* dataFile_p,
                           const QueueMap&         queueMap,
                           Filters&                filters,
                           bslma::Allocator*       allocator)
: d_ostream(ostream)
, d_withDetails(withDetails)
, d_dumpPayload(dumpPayload)
, d_dumpLimit(dumpLimit)
, d_dataFile_p(dataFile_p)
, d_queueMap(queueMap)
, d_filters(filters)
, d_totalMessagesCount(0)
, d_foundMessagesCount(0)
, d_deletedMessagesCount(0)
, d_allocator_p(bslma::Default::allocator(allocator))
, d_messagesDetails(allocator)
// , dataRecordOffsetMap(allocator)
{
    // NOTHING
}

bool SearchResult::hasCache() const
{
    return !d_messagesDetails.empty();
}

void SearchResult::addMessageDetails(const mqbs::MessageRecord& record,
                                     bsls::Types::Uint64        recordIndex,
                                     bsls::Types::Uint64        recordOffset)
{
    d_messagesDetails.emplace(
        record.messageGUID(),
        MessageDetails(record, recordIndex, recordOffset, d_allocator_p));
    d_messageIndexToGuidMap.emplace(recordIndex, record.messageGUID());
}

void SearchResult::deleteMessageDetails(MessagesDetails::iterator iterator)
{
    // Erase record from both maps
    d_messagesDetails.erase(iterator);
    d_messageIndexToGuidMap.erase(iterator->second.messageRecordIndex());
}

void SearchResult::deleteMessageDetails(const bmqt::MessageGUID& messageGUID,
                                        bsls::Types::Uint64      recordIndex)
{
    // Erase record from both maps
    d_messagesDetails.erase(messageGUID);
    d_messageIndexToGuidMap.erase(recordIndex);
}

bool SearchResult::processMessageRecord(const mqbs::MessageRecord& record,
                                        bsls::Types::Uint64        recordIndex,
                                        bsls::Types::Uint64 recordOffset)
{
    // Apply filters
    bool filterPassed = d_filters.apply(record);

    if (filterPassed) {
        // Store record details for further output
        addMessageDetails(record, recordIndex, recordOffset);
        d_totalMessagesCount++;
    }

    return filterPassed;
}

bool SearchResult::processConfirmRecord(const mqbs::ConfirmRecord& record,
                                        bsls::Types::Uint64        recordIndex,
                                        bsls::Types::Uint64 recordOffset)
{
    if (d_withDetails) {
        // Store record details for further output
        if (auto it = d_messagesDetails.find(record.messageGUID());
            it != d_messagesDetails.end()) {
            it->second.addConfirmRecord(record, recordIndex, recordOffset);
        }
    }
    return false;
}

bool SearchResult::processDeletionRecord(const mqbs::DeletionRecord& record,
                                         bsls::Types::Uint64 recordIndex,
                                         bsls::Types::Uint64 recordOffset)
{
    if (auto it = d_messagesDetails.find(record.messageGUID());
        it != d_messagesDetails.end()) {
        // Print message details immediately and delete record to save space.
        if (d_withDetails) {
            it->second.addDeleteRecord(record, recordIndex, recordOffset);
            it->second.print(d_ostream, d_queueMap);
        }
        else {
            outputGuidString(it->first);
        }
        if (d_dumpPayload) {
            auto dataRecordOffset = it->second.dataRecordOffset();
            outputPayload(dataRecordOffset);
        }
        deleteMessageDetails(it);
        d_deletedMessagesCount++;
    }
    return false;
}

void SearchResult::outputResult(bool outputRatio)
{
    for (const auto& item : d_messageIndexToGuidMap) {
        auto messageDetails = d_messagesDetails.at(item.second);
        if (d_withDetails) {
            messageDetails.print(d_ostream, d_queueMap);
        }
        else {
            outputGuidString(item.second);
        }
        if (d_dumpPayload) {
            outputPayload(messageDetails.dataRecordOffset());
        }
    }

    outputFooter();
    if (outputRatio)
        outputOutstandingRatio();
}

void SearchResult::outputGuidString(const bmqt::MessageGUID& messageGUID,
                                    const bool               addNewLine)
{
    d_ostream << messageGUID;

    if (addNewLine)
        d_ostream << '\n';
}

void SearchResult::outputFooter()
{
    const char* captionForFound    = d_withDetails ? " message(s) found."
                                                   : " message GUID(s) found.";
    const char* captionForNotFound = d_withDetails ? "No message found."
                                                   : "No message GUID found.";
    d_foundMessagesCount > 0
        ? (d_ostream << d_foundMessagesCount << captionForFound)
        : d_ostream << captionForNotFound;
    d_ostream << '\n';
}

void SearchResult::outputOutstandingRatio()
{
    if (d_totalMessagesCount > 0) {
        bsl::size_t outstandingMessages = d_totalMessagesCount -
                                          d_deletedMessagesCount;
        d_ostream << "Outstanding ratio: "
                  << bsl::round(float(outstandingMessages) /
                                float(d_totalMessagesCount) * 100.0)
                  << "% (" << outstandingMessages << "/"
                  << d_totalMessagesCount << ")" << '\n';
    }
}

void SearchResult::outputPayload(bsls::Types::Uint64 messageOffsetDwords)
{
    auto recordOffset = messageOffsetDwords * bmqp::Protocol::k_DWORD_SIZE;
    auto it           = d_dataFile_p;

    // Flip iterator direction depending on recordOffset
    if ((it->recordOffset() > recordOffset && !it->isReverseMode()) ||
        (it->recordOffset() < recordOffset && it->isReverseMode())) {
        it->flipDirection();
    }
    // Search record in data file
    while (it->recordOffset() != recordOffset) {
        int rc = it->nextRecord();

        if (rc != 1) {
            d_ostream << "Failed to retrieve message from DATA "
                      << "file rc: " << rc << bsl::endl;
            return;  // RETURN
        }
    }

    mwcu::MemOutStream dataHeaderOsstr;
    mwcu::MemOutStream optionsOsstr;
    mwcu::MemOutStream propsOsstr;

    dataHeaderOsstr << it->dataHeader();

    const char*  options    = 0;
    unsigned int optionsLen = 0;
    it->loadOptions(&options, &optionsLen);
    mqbs::FileStoreProtocolPrinter::printOptions(optionsOsstr,
                                                 options,
                                                 optionsLen);

    const char*  appData    = 0;
    unsigned int appDataLen = 0;
    it->loadApplicationData(&appData, &appDataLen);

    const mqbs::DataHeader& dh                = it->dataHeader();
    unsigned int            propertiesAreaLen = 0;
    if (mqbs::DataHeaderFlagUtil::isSet(
            dh.flags(),
            mqbs::DataHeaderFlags::e_MESSAGE_PROPERTIES)) {
        int rc = mqbs::FileStoreProtocolPrinter::printMessageProperties(
            &propertiesAreaLen,
            propsOsstr,
            appData,
            bmqp::MessagePropertiesInfo(dh));
        if (rc) {
            d_ostream << "Failed to retrieve message properties, rc: " << rc
                      << bsl::endl;
        }

        appDataLen -= propertiesAreaLen;
        appData += propertiesAreaLen;
    }

    // Payload
    mwcu::MemOutStream payloadOsstr;
    unsigned int minLen = d_dumpLimit > 0 ? bsl::min(appDataLen, d_dumpLimit)
                                          : appDataLen;
    payloadOsstr << "First " << minLen << " bytes of payload:" << '\n';
    bdlb::Print::hexDump(payloadOsstr, appData, minLen);
    if (minLen < appDataLen) {
        payloadOsstr << "And " << (appDataLen - minLen)
                     << " more bytes (redacted)" << '\n';
    }

    d_ostream << '\n'
              << "DataRecord index: " << it->recordIndex()
              << ", offset: " << it->recordOffset() << '\n'
              << "DataHeader: " << '\n'
              << dataHeaderOsstr.str();

    if (0 != optionsLen) {
        d_ostream << "\nOptions: " << '\n' << optionsOsstr.str();
    }

    if (0 != propertiesAreaLen) {
        d_ostream << "\nProperties: " << '\n' << propsOsstr.str();
    }

    d_ostream << "\nPayload: " << '\n' << payloadOsstr.str() << '\n';
}

// =====================
// class SearchAllResult
// =====================

SearchAllResult::SearchAllResult(bsl::ostream&           ostream,
                                 bool                    withDetails,
                                 bool                    dumpPayload,
                                 unsigned int            dumpLimit,
                                 mqbs::DataFileIterator* dataFile_p,
                                 const QueueMap&         queueMap,
                                 Filters&                filters,
                                 bslma::Allocator*       allocator)
: SearchResult(ostream,
               withDetails,
               dumpPayload,
               dumpLimit,
               dataFile_p,
               queueMap,
               filters,
               allocator)
{
    // NOTHING
}

bool SearchAllResult::processMessageRecord(const mqbs::MessageRecord& record,
                                           bsls::Types::Uint64 recordIndex,
                                           bsls::Types::Uint64 recordOffset)
{
    bool filterPassed = SearchResult::processMessageRecord(record,
                                                           recordIndex,
                                                           recordOffset);
    if (filterPassed) {
        if (!d_withDetails) {
            // Output GUID immediately and delete message details.
            outputGuidString(record.messageGUID());
            if (d_dumpPayload) {
                outputPayload(record.messageOffsetDwords());
            }
            deleteMessageDetails(record.messageGUID(), recordIndex);
        }
        d_foundMessagesCount++;
    }

    return false;
}

void SearchAllResult::outputResult(BSLS_ANNOTATION_UNUSED bool outputRatio)
{
    SearchResult::outputResult(false);
}

// ======================
// class SearchGuidResult
// ======================

SearchGuidResult::SearchGuidResult(bsl::ostream&                   ostream,
                                   bool                            withDetails,
                                   bool                            dumpPayload,
                                   unsigned int                    dumpLimit,
                                   mqbs::DataFileIterator*         dataFile_p,
                                   const QueueMap&                 queueMap,
                                   const bsl::vector<bsl::string>& guids,
                                   Filters&                        filters,
                                   bslma::Allocator*               allocator)
: SearchResult(ostream,
               withDetails,
               dumpPayload,
               dumpLimit,
               dataFile_p,
               queueMap,
               filters,
               allocator)
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

bool SearchGuidResult::processMessageRecord(const mqbs::MessageRecord& record,
                                            bsls::Types::Uint64 recordIndex,
                                            bsls::Types::Uint64 recordOffset)
{
    if (auto it = d_guidsMap.find(record.messageGUID());
        it != d_guidsMap.end()) {
        if (d_withDetails) {
            SearchResult::processMessageRecord(record,
                                               recordIndex,
                                               recordOffset);
        }
        else {
            // Output result immediately.
            d_ostream << *it->second << '\n';
            if (d_dumpPayload) {
                outputPayload(record.messageOffsetDwords());
            }
        }
        // Remove processed GUID from map.
        d_guids.erase(it->second);
        d_guidsMap.erase(it);
        d_foundMessagesCount++;
    }
    d_totalMessagesCount++;

    // return true (stop search) if no detail is needed and map is empty.
    return (!d_withDetails && d_guidsMap.empty());
}

bool SearchGuidResult::processDeletionRecord(
    const mqbs::DeletionRecord& record,
    bsls::Types::Uint64         recordIndex,
    bsls::Types::Uint64         recordOffset)
{
    SearchResult::processDeletionRecord(record, recordIndex, recordOffset);
    // return true (stop search) when details needed and search is done (map is
    // empty).
    return (d_withDetails && d_guidsMap.empty());
}

void SearchGuidResult::outputResult(BSLS_ANNOTATION_UNUSED bool outputRatio)
{
    SearchResult::outputResult(false);
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

// =============================
// class SearchOutstandingResult
// =============================

SearchOutstandingResult::SearchOutstandingResult(
    bsl::ostream&           ostream,
    bool                    withDetails,
    bool                    dumpPayload,
    unsigned int            dumpLimit,
    mqbs::DataFileIterator* dataFile_p,
    const QueueMap&         queueMap,
    Filters&                filters,
    bslma::Allocator*       allocator)
: SearchResult(ostream,
               withDetails,
               dumpPayload,
               dumpLimit,
               dataFile_p,
               queueMap,
               filters,
               allocator)
{
    // NOTHING
}

bool SearchOutstandingResult::processMessageRecord(
    const mqbs::MessageRecord& record,
    bsls::Types::Uint64        recordIndex,
    bsls::Types::Uint64        recordOffset)
{
    bool filterPassed = SearchResult::processMessageRecord(record,
                                                           recordIndex,
                                                           recordOffset);
    if (filterPassed) {
        d_foundMessagesCount++;
    }

    return false;
}

bool SearchOutstandingResult::processDeletionRecord(
    const mqbs::DeletionRecord& record,
    BSLS_ANNOTATION_UNUSED bsls::Types::Uint64 recordIndex,
    BSLS_ANNOTATION_UNUSED bsls::Types::Uint64 recordOffset)
{
    if (auto it = d_messagesDetails.find(record.messageGUID());
        it != d_messagesDetails.end()) {
        // Message is confirmed (not outstanding), remove it.
        deleteMessageDetails(it);
        d_foundMessagesCount--;
        d_deletedMessagesCount++;
    }

    return false;
}

// ===========================
// class SearchConfirmedResult
// ===========================

SearchConfirmedResult::SearchConfirmedResult(
    bsl::ostream&           ostream,
    bool                    withDetails,
    bool                    dumpPayload,
    unsigned int            dumpLimit,
    mqbs::DataFileIterator* dataFile_p,
    const QueueMap&         queueMap,
    Filters&                filters,
    bslma::Allocator*       allocator)
: SearchResult(ostream,
               withDetails,
               dumpPayload,
               dumpLimit,
               dataFile_p,
               queueMap,
               filters,
               allocator)
{
    // NOTHING
}

bool SearchConfirmedResult::processMessageRecord(
    const mqbs::MessageRecord& record,
    bsls::Types::Uint64        recordIndex,
    bsls::Types::Uint64        recordOffset)
{
    SearchResult::processMessageRecord(record, recordIndex, recordOffset);
    return false;
}

bool SearchConfirmedResult::processDeletionRecord(
    const mqbs::DeletionRecord& record,
    bsls::Types::Uint64         recordIndex,
    bsls::Types::Uint64         recordOffset)
{
    if (auto it = d_messagesDetails.find(record.messageGUID());
        it != d_messagesDetails.end()) {
        d_foundMessagesCount++;
    }

    SearchResult::processDeletionRecord(record, recordIndex, recordOffset);

    return false;
}

void SearchConfirmedResult::outputResult(
    BSLS_ANNOTATION_UNUSED bool outputRatio)
{
    outputFooter();
    outputOutstandingRatio();
}

// ====================================
// class SearchPartiallyConfirmedResult
// ====================================

SearchPartiallyConfirmedResult::SearchPartiallyConfirmedResult(
    bsl::ostream&           ostream,
    bool                    withDetails,
    bool                    dumpPayload,
    unsigned int            dumpLimit,
    mqbs::DataFileIterator* dataFile_p,
    const QueueMap&         queueMap,
    Filters&                filters,
    bslma::Allocator*       allocator)
: SearchResult(ostream,
               withDetails,
               dumpPayload,
               dumpLimit,
               dataFile_p,
               queueMap,
               filters,
               allocator)
, d_partiallyConfirmedGUIDS(allocator)
{
    // NOTHING
}

bool SearchPartiallyConfirmedResult::processMessageRecord(
    const mqbs::MessageRecord& record,
    bsls::Types::Uint64        recordIndex,
    bsls::Types::Uint64        recordOffset)
{
    bool filterPassed = SearchResult::processMessageRecord(record,
                                                           recordIndex,
                                                           recordOffset);
    if (filterPassed) {
        d_partiallyConfirmedGUIDS[record.messageGUID()] = 0;
    }

    return false;
}

bool SearchPartiallyConfirmedResult::processConfirmRecord(
    const mqbs::ConfirmRecord& record,
    bsls::Types::Uint64        recordIndex,
    bsls::Types::Uint64        recordOffset)
{
    SearchResult::processConfirmRecord(record, recordIndex, recordOffset);
    if (auto it = d_partiallyConfirmedGUIDS.find(record.messageGUID());
        it != d_partiallyConfirmedGUIDS.end()) {
        // Message is partially confirmed, increase counter.
        it->second++;
    }

    return false;
}

bool SearchPartiallyConfirmedResult::processDeletionRecord(
    const mqbs::DeletionRecord& record,
    BSLS_ANNOTATION_UNUSED bsls::Types::Uint64 recordIndex,
    BSLS_ANNOTATION_UNUSED bsls::Types::Uint64 recordOffset)
{
    if (auto it = d_partiallyConfirmedGUIDS.find(record.messageGUID());
        it != d_partiallyConfirmedGUIDS.end()) {
        // Message is confirmed, remove it.
        d_partiallyConfirmedGUIDS.erase(it);
        auto messageDetails = d_messagesDetails.at(record.messageGUID());
        deleteMessageDetails(record.messageGUID(),
                             messageDetails.messageRecordIndex());
        d_deletedMessagesCount++;
    }

    return false;
}

void SearchPartiallyConfirmedResult::outputResult(
    BSLS_ANNOTATION_UNUSED bool outputRatio)
{
    for (const auto& item : d_messageIndexToGuidMap) {
        auto confirmCount = d_partiallyConfirmedGUIDS.at(item.second);
        if (confirmCount > 0) {
            if (d_withDetails) {
                d_messagesDetails.at(item.second).print(d_ostream, d_queueMap);
            }
            else {
                outputGuidString(item.second);
            }
            d_foundMessagesCount++;
        }
    }

    outputFooter();
    outputOutstandingRatio();
}

// =========================
// class SearchSummaryResult
// =========================

SearchSummaryResult::SearchSummaryResult(
    bsl::ostream&              ostream,
    mqbs::JournalFileIterator* journalFile_p,
    mqbs::DataFileIterator*    dataFile_p,
    const QueueMap&            queueMap,
    Filters&                   filters,
    bslma::Allocator*          allocator)
: SearchResult(ostream,
               false,
               false,
               0,
               dataFile_p,
               queueMap,
               filters,
               allocator)
, d_partiallyConfirmedGUIDS(allocator)
, d_journalFile_p(journalFile_p)
{
    // NOTHING
}

bool SearchSummaryResult::hasCache() const
{
    return !d_partiallyConfirmedGUIDS.empty();
}

bool SearchSummaryResult::processMessageRecord(
    const mqbs::MessageRecord& record,
    BSLS_ANNOTATION_UNUSED bsls::Types::Uint64 recordIndex,
    BSLS_ANNOTATION_UNUSED bsls::Types::Uint64 recordOffset)
{
    // Apply filters
    bool filterPassed = d_filters.apply(record);

    if (filterPassed) {
        d_partiallyConfirmedGUIDS[record.messageGUID()] = 0;
        d_totalMessagesCount++;
    }

    return false;
}

bool SearchSummaryResult::processConfirmRecord(
    const mqbs::ConfirmRecord& record,
    bsls::Types::Uint64        recordIndex,
    bsls::Types::Uint64        recordOffset)
{
    if (auto it = d_partiallyConfirmedGUIDS.find(record.messageGUID());
        it != d_partiallyConfirmedGUIDS.end()) {
        // Message is partially confirmed, increase counter.
        it->second++;
    }

    return false;
}

bool SearchSummaryResult::processDeletionRecord(
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

void SearchSummaryResult::outputResult(BSLS_ANNOTATION_UNUSED bool outputRatio)
{
    // Calculate number of partially confirmed messages
    size_t partiallyConfirmedMessagesCount = 0;
    for (const auto& item : d_partiallyConfirmedGUIDS) {
        auto confirmCount = item.second;
        if (confirmCount > 0) {
            partiallyConfirmedMessagesCount++;
        }
    }

    if (d_totalMessagesCount == 0) {
        d_ostream << "No messages found." << '\n';
        return;  // RETURN
    }

    d_ostream << d_totalMessagesCount << " message(s) found." << '\n';
    d_ostream << "Number of confirmed messages: " << d_deletedMessagesCount
              << '\n';
    d_ostream << "Number of partially confirmed messages: "
              << partiallyConfirmedMessagesCount << '\n';
    d_ostream << "Number of outstanding messages: "
              << (d_totalMessagesCount - d_deletedMessagesCount) << '\n';

    outputOutstandingRatio();

    // Print meta data of opened files
    printJournalFileMeta(d_ostream, d_journalFile_p);
    printDataFileMeta(d_ostream, d_dataFile_p);
}

// ===========================
// class SearchResultDecorator
// ===========================

SearchResultDecorator::SearchResultDecorator(
    const bsl::shared_ptr<SearchResultInterface> component)
: d_searchResult(component)
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

void SearchResultDecorator::outputResult(bool outputRatio)
{
    d_searchResult->outputResult(outputRatio);
}

void SearchResultDecorator::outputResult(
    bsl::unordered_set<bmqt::MessageGUID>& guidFilter)
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
    const bsl::shared_ptr<SearchResultInterface> component,
    bsls::Types::Uint64                          timestampLt)
: SearchResultDecorator(component)
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
    bsl::ostream&                  ostream,
    bsl::shared_ptr<PayloadDumper> payloadDumper,
    bslma::Allocator*              allocator,
    const bool                     printImmediately,
    const bool                     printOnDelete,
    const bool                     eraseDeleted)
: d_ostream(ostream)
, d_payloadDumper(payloadDumper)
, d_printImmediately(printImmediately)
, d_printOnDelete(printOnDelete)
, d_eraseDeleted(eraseDeleted)
, d_printedMessagesCount(0)
, d_guidMap(allocator)
, d_guidList(allocator)
{
    // NOTHING
}

bool SearchShortResult::processMessageRecord(
    const mqbs::MessageRecord& record,
    bsls::Types::Uint64        recordIndex,
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
    const mqbs::ConfirmRecord& record,
    BSLS_ANNOTATION_UNUSED bsls::Types::Uint64 recordIndex,
    BSLS_ANNOTATION_UNUSED bsls::Types::Uint64 recordOffset)
{
    return false;
}

bool SearchShortResult::processDeletionRecord(
    const mqbs::DeletionRecord& record,
    bsls::Types::Uint64         recordIndex,
    bsls::Types::Uint64         recordOffset)
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

void SearchShortResult::outputResult(BSLS_ANNOTATION_UNUSED bool outputRatio)
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
    bsl::unordered_set<bmqt::MessageGUID>& guidFilter)
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

// ========================
// class SearchDetailResult
// ========================

SearchDetailResult::SearchDetailResult(
    bsl::ostream&                  ostream,
    const QueueMap&                queueMap,
    bsl::shared_ptr<PayloadDumper> payloadDumper,
    bslma::Allocator*              allocator,
    const bool                     printImmediately,
    const bool                     eraseDeleted,
    const bool                     cleanUnprinted)
: d_ostream(ostream)
, d_queueMap(queueMap)
, d_payloadDumper(payloadDumper)
, d_allocator_p(allocator)
, d_printImmediately(printImmediately)
, d_eraseDeleted(eraseDeleted)
, d_cleanUnprinted(cleanUnprinted)
, d_printedMessagesCount(0)
, d_messagesDetails(allocator)
, d_messageIndexToGuidMap(allocator)
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

void SearchDetailResult::outputResult(BSLS_ANNOTATION_UNUSED bool outputRatio)
{
    if (d_cleanUnprinted) {
        d_messageIndexToGuidMap.clear();
    }

    for (const auto& item : d_messageIndexToGuidMap) {
        auto messageDetails = d_messagesDetails.at(item.second);
        outputMessageDetails(messageDetails);
    }

    outputFooter(d_ostream, d_printedMessagesCount);
}

void SearchDetailResult::outputResult(
    bsl::unordered_set<bmqt::MessageGUID>& guidFilter)
{
    // Remove guids from map that do not match filter
    bsl::erase_if(
        d_messageIndexToGuidMap,
        [&guidFilter](
            const bsl::pair<bsls::Types::Uint64, bmqt::MessageGUID> pair) {
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

// ========================
// class SearchAllDecorator
// ========================
SearchAllDecorator::SearchAllDecorator(
    const bsl::shared_ptr<SearchResultInterface> component)
: SearchResultDecorator(component)
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
    const bsl::shared_ptr<SearchResultInterface> component,
    bsl::ostream&                                ostream,
    bslma::Allocator*                            allocator)
: SearchResultDecorator(component)
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

void SearchOutstandingDecorator::outputResult(bool outputRatio)
{
    SearchResultDecorator::outputResult(outputRatio);
    outputOutstandingRatio(d_ostream,
                           d_foundMessagesCount,
                           d_deletedMessagesCount);
}

// ================================
// class SearchPartiallyConfirmedDecorator
// ================================
SearchPartiallyConfirmedDecorator::SearchPartiallyConfirmedDecorator(
    const bsl::shared_ptr<SearchResultInterface> component,
    bsl::ostream&                                ostream,
    bslma::Allocator*                            allocator)
: SearchResultDecorator(component)
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

void SearchPartiallyConfirmedDecorator::outputResult(bool outputRatio)
{
    // Build a filter of partially confirmed guids
    bsl::unordered_set<bmqt::MessageGUID> guidFilter;
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

}  // close package namespace
}  // close enterprise namespace
