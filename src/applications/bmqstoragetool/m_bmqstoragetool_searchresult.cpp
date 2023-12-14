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

namespace BloombergLP {
namespace m_bmqstoragetool {

// =====================
// class SearchResult
// =====================

SearchResult::SearchResult(bsl::ostream&     ostream,
                           bool              withDetails,
                           Filters&          filters,
                           bslma::Allocator* allocator)
: d_ostream(ostream)
, d_withDetails(withDetails)
, d_filters(filters)
, d_foundMessagesCount()
, d_totalMessagesCount()
, d_allocator_p(bslma::Default::allocator(allocator))
, d_messagesDetails(allocator)
{
    // NOTHING
}

bool SearchResult::processMessageRecord(const mqbs::MessageRecord& record,
                                        bsls::Types::Uint64        recordIndex,
                                        bsls::Types::Uint64 recordOffset)
{
    // Apply filters
    bool filterPassed = d_filters.apply(record);

    if (filterPassed && d_withDetails) {
        // Store record details for further output
        d_messagesDetails.emplace(
            record.messageGUID(),
            MessageDetails(record, recordIndex, recordOffset, d_allocator_p));
    }

    d_totalMessagesCount++;

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
    if (d_withDetails) {
        // Print message details immediately and delete record to save space.
        if (auto it = d_messagesDetails.find(record.messageGUID());
            it != d_messagesDetails.end()) {
            it->second.addDeleteRecord(record, recordIndex, recordOffset);
            it->second.print(d_ostream);
            d_messagesDetails.erase(it);
        }
    }
    return false;
}

void SearchResult::outputResult()
{
    if (d_withDetails) {
        // Print all collected messages details
        for (auto& item : d_messagesDetails) {
            item.second.print(d_ostream);
        }
    }
    outputFooter();
}

void SearchResult::outputGuidString(const bmqt::MessageGUID& messageGUID,
                                    const bool               addNewLine)
{
    char buf[bmqt::MessageGUID::e_SIZE_HEX];
    messageGUID.toHex(buf);
    d_ostream.write(buf, bmqt::MessageGUID::e_SIZE_HEX);
    if (addNewLine)
        d_ostream << bsl::endl;
}

void SearchResult::outputFooter()
{
    const char* caption = d_withDetails ? " message(s) found."
                                        : " message GUID(s) found.";
    d_foundMessagesCount > 0 ? (d_ostream << d_foundMessagesCount << caption)
                             : d_ostream << "No message GUID found.";
    d_ostream << bsl::endl;
}

void SearchResult::outputOutstandingRatio()
{
    if (d_foundMessagesCount) {
        d_ostream << "Outstanding ratio: "
                  << float(d_foundMessagesCount) / d_totalMessagesCount * 100.0
                  << "%" << bsl::endl;
    }
}

// =====================
// class SearchAllResult
// =====================

SearchAllResult::SearchAllResult(bsl::ostream&     ostream,
                                 bool              withDetails,
                                 Filters&          filters,
                                 bslma::Allocator* allocator)
: SearchResult(ostream, withDetails, filters, allocator)
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
            // Output GUID immediately.
            outputGuidString(record.messageGUID());
        }
        d_foundMessagesCount++;
    }

    return false;
}

// =====================
// class SearchGuidResult
// =====================

SearchGuidResult::SearchGuidResult(bsl::ostream&                   ostream,
                                   bool                            withDetails,
                                   const bsl::vector<bsl::string>& guids,
                                   Filters&                        filters,
                                   bslma::Allocator*               allocator)
: SearchResult(ostream, withDetails, filters, allocator)
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
        if (!d_withDetails) {
            // Output result immediately and remove processed GUID from map.
            d_ostream << it->second << bsl::endl;
            d_guidsMap.erase(it);
        }
        d_foundMessagesCount++;
    }
    d_totalMessagesCount++;

    return d_guidsMap.empty() ? true : false;
}

// =====================
// class SearchOutstandingResult
// =====================

SearchOutstandingResult::SearchOutstandingResult(bsl::ostream&     ostream,
                                                 bool              withDetails,
                                                 Filters&          filters,
                                                 bslma::Allocator* allocator)
: SearchResult(ostream, withDetails, filters, allocator)
, d_outstandingGUIDS(allocator)
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
        if (!d_withDetails) {
            d_outstandingGUIDS.push_back(record.messageGUID());
        }
        d_foundMessagesCount++;
    }

    return false;
}

bool SearchOutstandingResult::processDeletionRecord(
    const mqbs::DeletionRecord& record,
    bsls::Types::Uint64         recordIndex,
    bsls::Types::Uint64         recordOffset)
{
    if (auto it = bsl::find(d_outstandingGUIDS.begin(),
                            d_outstandingGUIDS.end(),
                            record.messageGUID());
        it != d_outstandingGUIDS.end()) {
        // Message is confirmed (not outstanding), remove it.
        d_outstandingGUIDS.erase(it);
        d_foundMessagesCount--;
    }

    return false;
}

void SearchOutstandingResult::outputResult()
{
    if (!d_withDetails) {
        for (const auto& guid : d_outstandingGUIDS) {
            outputGuidString(guid);
        }

        outputFooter();
        outputOutstandingRatio();
    }
}

// =====================
// class SearchConfirmedResult
// =====================

SearchConfirmedResult::SearchConfirmedResult(bsl::ostream&     ostream,
                                             bool              withDetails,
                                             Filters&          filters,
                                             bslma::Allocator* allocator)
: SearchResult(ostream, withDetails, filters, allocator)
, d_messageGUIDS(allocator)
{
    // NOTHING
}

bool SearchConfirmedResult::processMessageRecord(
    const mqbs::MessageRecord& record,
    bsls::Types::Uint64        recordIndex,
    bsls::Types::Uint64        recordOffset)
{
    bool filterPassed = SearchResult::processMessageRecord(record,
                                                           recordIndex,
                                                           recordOffset);
    if (filterPassed) {
        if (!d_withDetails) {
            d_messageGUIDS.push_back(record.messageGUID());
        }
    }

    return false;
}

bool SearchConfirmedResult::processDeletionRecord(
    const mqbs::DeletionRecord& record,
    bsls::Types::Uint64         recordIndex,
    bsls::Types::Uint64         recordOffset)
{
    if (!d_withDetails) {
        if (auto it = bsl::find(d_messageGUIDS.begin(),
                                d_messageGUIDS.end(),
                                record.messageGUID());
            it != d_messageGUIDS.end()) {
            // Message is confirmed, output it immediately and remove.
            outputGuidString(record.messageGUID());
            d_foundMessagesCount++;
            d_messageGUIDS.erase(it);
        }
    }

    return false;
}

void SearchConfirmedResult::outputResult()
{
    if (!d_withDetails) {
        outputFooter();
        outputOutstandingRatio();
    }
}

// =====================
// class SearchPartiallyConfirmedResult
// =====================

SearchPartiallyConfirmedResult::SearchPartiallyConfirmedResult(
    bsl::ostream&     ostream,
    bool              withDetails,
    Filters&          filters,
    bslma::Allocator* allocator)
: SearchResult(ostream, withDetails, filters, allocator)
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
        if (!d_withDetails) {
            d_partiallyConfirmedGUIDS[record.messageGUID()] = 0;
        }
    }

    return false;
}

bool SearchPartiallyConfirmedResult::processConfirmRecord(
    const mqbs::ConfirmRecord& record,
    bsls::Types::Uint64        recordIndex,
    bsls::Types::Uint64        recordOffset)
{
    if (!d_withDetails) {
        if (auto it = d_partiallyConfirmedGUIDS.find(record.messageGUID());
            it != d_partiallyConfirmedGUIDS.end()) {
            // Message is partially confirmed, increase counter.
            it->second++;
        }
    }

    return false;
}

bool SearchPartiallyConfirmedResult::processDeletionRecord(
    const mqbs::DeletionRecord& record,
    bsls::Types::Uint64         recordIndex,
    bsls::Types::Uint64         recordOffset)
{
    if (!d_withDetails) {
        if (auto it = d_partiallyConfirmedGUIDS.find(record.messageGUID());
            it != d_partiallyConfirmedGUIDS.end()) {
            // Message is confirmed, remove.
            d_partiallyConfirmedGUIDS.erase(it);
        }
    }

    return false;
}

void SearchPartiallyConfirmedResult::outputResult()
{
    if (!d_withDetails) {
        for (const auto& item : d_partiallyConfirmedGUIDS) {
            if (item.second > 0) {
                outputGuidString(item.first);
                d_foundMessagesCount++;
            }
        }

        outputFooter();
        outputOutstandingRatio();
    }
}

}  // close package namespace
}  // close enterprise namespace
