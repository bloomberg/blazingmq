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

#include <mwcu_memoutstream.h>

namespace BloombergLP {
namespace m_bmqstoragetool {

// =====================
// class SearchResult
// =====================

SearchResult::SearchResult(
    bsl::ostream&                                    ostream,
    bool                                             withDetails,
    bool                                             dumpPayload,
    unsigned int                                     dumpLimit,
    Parameters::FileHandler<mqbs::DataFileIterator>* dataFile_p,
    QueueMap&                                        queueMap,
    Filters&                                         filters,
    bslma::Allocator*                                allocator)
: d_ostream(ostream)
, d_withDetails(withDetails)
, d_dumpPayload(dumpPayload)
, d_dumpLimit(dumpLimit)
, d_dataFile_p(dataFile_p)
, d_queueMap(queueMap)
, d_filters(filters)
, d_foundMessagesCount(0)
, d_totalMessagesCount(0)
, d_deletedMessagesCount(0)
, d_allocator_p(bslma::Default::allocator(allocator))
, d_messagesDetails(allocator)
, dataRecordOffsetMap(allocator)
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
                                d_totalMessagesCount * 100.0)
                  << "% (" << outstandingMessages << "/"
                  << d_totalMessagesCount << ")" << '\n';
    }
}

void SearchResult::outputPayload(bsls::Types::Uint64 messageOffsetDwords)
{
    auto recordOffset = messageOffsetDwords * bmqp::Protocol::k_DWORD_SIZE;
    auto it           = d_dataFile_p->iterator();

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

SearchAllResult::SearchAllResult(
    bsl::ostream&                                    ostream,
    bool                                             withDetails,
    bool                                             dumpPayload,
    unsigned int                                     dumpLimit,
    Parameters::FileHandler<mqbs::DataFileIterator>* dataFile_p,
    QueueMap&                                        queueMap,
    Filters&                                         filters,
    bslma::Allocator*                                allocator)
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

void SearchAllResult::outputResult(bool outputRatio)
{
    SearchResult::outputResult(false);
}

// =====================
// class SearchGuidResult
// =====================

SearchGuidResult::SearchGuidResult(
    bsl::ostream&                                    ostream,
    bool                                             withDetails,
    bool                                             dumpPayload,
    unsigned int                                     dumpLimit,
    Parameters::FileHandler<mqbs::DataFileIterator>* dataFile_p,
    QueueMap&                                        queueMap,
    const bsl::vector<bsl::string>&                  guids,
    Filters&                                         filters,
    bslma::Allocator*                                allocator)
: SearchResult(ostream,
               withDetails,
               dumpPayload,
               dumpLimit,
               dataFile_p,
               queueMap,
               filters,
               allocator)
, d_guidsMap(allocator)
{
    // Build MessageGUID->StrGUID Map
    for (const auto& guidStr : guids) {
        bmqt::MessageGUID guid;
        d_guidsMap[guid.fromHex(guidStr.c_str())] = guidStr;
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
            d_ostream << it->second << '\n';
            if (d_dumpPayload) {
                outputPayload(record.messageOffsetDwords());
            }
        }
        // Remove processed GUID from map.
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

void SearchGuidResult::outputResult(bool outputRatio)
{
    SearchResult::outputResult(false);
    // Print non found GUIDs
    if (auto nonFoundCount = d_guidsMap.size(); nonFoundCount > 0) {
        d_ostream << '\n'
                  << "The following " << nonFoundCount
                  << " GUID(s) not found:" << '\n';
        for (const auto& item : d_guidsMap) {
            d_ostream << item.second << '\n';
        }
    }
}

// =============================
// class SearchOutstandingResult
// =============================

SearchOutstandingResult::SearchOutstandingResult(
    bsl::ostream&                                    ostream,
    bool                                             withDetails,
    bool                                             dumpPayload,
    unsigned int                                     dumpLimit,
    Parameters::FileHandler<mqbs::DataFileIterator>* dataFile_p,
    QueueMap&                                        queueMap,
    Filters&                                         filters,
    bslma::Allocator*                                allocator)
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
    bsls::Types::Uint64         recordIndex,
    bsls::Types::Uint64         recordOffset)
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
    bsl::ostream&                                    ostream,
    bool                                             withDetails,
    bool                                             dumpPayload,
    unsigned int                                     dumpLimit,
    Parameters::FileHandler<mqbs::DataFileIterator>* dataFile_p,
    QueueMap&                                        queueMap,
    Filters&                                         filters,
    bslma::Allocator*                                allocator)
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

void SearchConfirmedResult::outputResult(bool outputRatio)
{
    outputFooter();
    outputOutstandingRatio();
}

// ====================================
// class SearchPartiallyConfirmedResult
// ====================================

SearchPartiallyConfirmedResult::SearchPartiallyConfirmedResult(
    bsl::ostream&                                    ostream,
    bool                                             withDetails,
    bool                                             dumpPayload,
    unsigned int                                     dumpLimit,
    Parameters::FileHandler<mqbs::DataFileIterator>* dataFile_p,
    QueueMap&                                        queueMap,
    Filters&                                         filters,
    bslma::Allocator*                                allocator)
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
    bsls::Types::Uint64         recordIndex,
    bsls::Types::Uint64         recordOffset)
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

void SearchPartiallyConfirmedResult::outputResult(bool outputRatio)
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

// ====================================
// class SearchSummaryResult
// ====================================

SearchSummaryResult::SearchSummaryResult(
    bsl::ostream&                                    ostream,
    Parameters::FileHandler<mqbs::DataFileIterator>* dataFile_p,
    QueueMap&                                        queueMap,
    Filters&                                         filters,
    bslma::Allocator*                                allocator)
: SearchResult(ostream,
               false,
               false,
               0,
               dataFile_p,
               queueMap,
               filters,
               allocator)
, d_partiallyConfirmedGUIDS(allocator)
{
    // NOTHING
}

bool SearchSummaryResult::processMessageRecord(
    const mqbs::MessageRecord& record,
    bsls::Types::Uint64        recordIndex,
    bsls::Types::Uint64        recordOffset)
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
    SearchResult::processConfirmRecord(record, recordIndex, recordOffset);
    if (auto it = d_partiallyConfirmedGUIDS.find(record.messageGUID());
        it != d_partiallyConfirmedGUIDS.end()) {
        // Message is partially confirmed, increase counter.
        it->second++;
    }

    return false;
}

bool SearchSummaryResult::processDeletionRecord(
    const mqbs::DeletionRecord& record,
    bsls::Types::Uint64         recordIndex,
    bsls::Types::Uint64         recordOffset)
{
    if (auto it = d_partiallyConfirmedGUIDS.find(record.messageGUID());
        it != d_partiallyConfirmedGUIDS.end()) {
        // Message is confirmed, remove it.
        d_partiallyConfirmedGUIDS.erase(it);
        d_deletedMessagesCount++;
    }

    return false;
}

void SearchSummaryResult::outputResult(bool outputRatio)
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
}

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

}  // close package namespace
}  // close enterprise namespace
