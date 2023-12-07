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

namespace BloombergLP {
namespace m_bmqstoragetool {

// =====================
// class SearchResult
// =====================

SearchResult::SearchResult(bsl::ostream&     ostream,
                           bool              withDetails,
                           bslma::Allocator* allocator)
: d_ostream(ostream)
, d_withDetails(withDetails)
, d_foundMessagesCount()
, d_totalMessagesCount()
, d_allocator_p(bslma::Default::allocator(allocator))
{
    // NOTHING
}

bool SearchResult::processMessageRecord(const mqbs::MessageRecord& record)
{
    d_totalMessagesCount++;
    d_foundMessagesCount++;

    return false;
}

bool SearchResult::processConfirmRecord(const mqbs::ConfirmRecord& record)
{
    return false;
}

bool SearchResult::processDeletionRecord(const mqbs::DeletionRecord& record)
{
    return false;
}

void SearchResult::outputResult()
{
    if (!d_withDetails) {
        outputFooter();
    }
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
    d_foundMessagesCount > 0
        ? (d_ostream << d_foundMessagesCount << d_foundGuidCaption)
        : d_ostream << d_notFoundGuidCaption;
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
                                 bslma::Allocator* allocator)
: SearchResult(ostream, withDetails, allocator)
{
    // NOTHING
}

bool SearchAllResult::processMessageRecord(const mqbs::MessageRecord& record)
{
    if (!d_withDetails) {
        outputGuidString(record.messageGUID());
    }

    return SearchResult::processMessageRecord(record);
}

// =====================
// class SearchGuidResult
// =====================

SearchGuidResult::SearchGuidResult(bsl::ostream&                   ostream,
                                   bool                            withDetails,
                                   const bsl::vector<bsl::string>& guids,
                                   bslma::Allocator*               allocator)
: SearchResult(ostream, withDetails, allocator)
, d_guidsMap(allocator)
{
    // Build MessageGUID->StrGUID Map
    for (const auto& guidStr : guids) {
        bmqt::MessageGUID guid;
        d_guidsMap[guid.fromHex(guidStr.c_str())] = guidStr;
    }
}

bool SearchGuidResult::processMessageRecord(const mqbs::MessageRecord& record)
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
                                                 bslma::Allocator* allocator)
: SearchResult(ostream, withDetails, allocator)
, d_outstandingGUIDS(allocator)
{
    // NOTHING
}

bool SearchOutstandingResult::processMessageRecord(
    const mqbs::MessageRecord& record)
{
    if (!d_withDetails) {
        d_outstandingGUIDS.push_back(record.messageGUID());
    }

    return SearchResult::processMessageRecord(record);
}

bool SearchOutstandingResult::processDeletionRecord(
    const mqbs::DeletionRecord& record)
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
                                             bslma::Allocator* allocator)
: SearchResult(ostream, withDetails, allocator)
, d_messageGUIDS(allocator)
{
    // NOTHING
}

bool SearchConfirmedResult::processMessageRecord(
    const mqbs::MessageRecord& record)
{
    if (!d_withDetails) {
        d_messageGUIDS.push_back(record.messageGUID());
    }
    d_totalMessagesCount++;

    return false;
}

bool SearchConfirmedResult::processDeletionRecord(
    const mqbs::DeletionRecord& record)
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
    bslma::Allocator* allocator)
: SearchResult(ostream, withDetails, allocator)
, d_partiallyConfirmedGUIDS(allocator)
{
    // NOTHING
}

bool SearchPartiallyConfirmedResult::processMessageRecord(
    const mqbs::MessageRecord& record)
{
    if (!d_withDetails) {
        d_partiallyConfirmedGUIDS[record.messageGUID()] = 0;
    }
    d_totalMessagesCount++;

    return false;
}

bool SearchPartiallyConfirmedResult::processConfirmRecord(
    const mqbs::ConfirmRecord& record)
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
    const mqbs::DeletionRecord& record)
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
