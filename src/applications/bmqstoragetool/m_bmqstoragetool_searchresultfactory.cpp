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
#include <bsl_memory.h>
#include <m_bmqstoragetool_parameters.h>
#include <m_bmqstoragetool_printer.h>
#include <m_bmqstoragetool_searchresultfactory.h>

namespace BloombergLP {
namespace m_bmqstoragetool {

// =========================
// class SearchResultFactory
// =========================

bsl::shared_ptr<SearchResult> SearchResultFactory::createSearchResult(
    const Parameters*                     params,
    const bslma::ManagedPtr<FileManager>& fileManager,
    const bsl::shared_ptr<Printer>&       printer,
    bslma::ManagedPtr<PayloadDumper>&     payloadDumper,
    bslma::Allocator*                     allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT(params);

    // Set up processing flags
    const bool details    = params->d_details;
    const bool exactMatch = !params->d_seqNum.empty() ||
                            !params->d_offset.empty();
    // Print data immediately as soon as it is completed to save memory, except
    // the foollowing cases, where data should be kept
    const bool printImmediately = !(params->d_outstanding ||
                                    params->d_partiallyConfirmed ||
                                    (params->d_confirmed && !details));
    // Always erase stored data when 'deletion' record received
    const bool eraseDeleted = true;
    // Print data immediately on 'deletion' record receiving for specific case
    const bool printOnDelete = params->d_confirmed;
    // Clean unprinted/unerased data for specific case
    const bool cleanUnprinted = params->d_confirmed;

    // Create searchResult implementation in the following order:
    // summary, exact match, detail or short result.
    bsl::shared_ptr<SearchResult> searchResult;
    if (params->d_summary) {
        searchResult = bsl::allocate_shared<SummaryProcessor>(
            allocator,
            printer,
            fileManager->journalFileIterator(),
            fileManager->dataFileIterator(),
            params->d_processRecordTypes,
            params->d_queueMap,
            params->d_minRecordsPerQueue);
    }
    else if (exactMatch) {
        searchResult = bsl::allocate_shared<SearchExactMatchResult>(
            allocator,
            printer,
            params->d_processRecordTypes,
            details,
            params->d_queueMap,
            payloadDumper);
    }
    else if (details) {
        searchResult = bsl::allocate_shared<SearchDetailResult>(
            allocator,
            printer,
            params->d_processRecordTypes,
            params->d_queueMap,
            payloadDumper,
            printImmediately,
            eraseDeleted,
            cleanUnprinted);
    }
    else {
        searchResult = bsl::allocate_shared<SearchShortResult>(
            allocator,
            printer,
            params->d_processRecordTypes,
            payloadDumper,
            printImmediately,
            eraseDeleted,
            printOnDelete);
    }

    if (!params->d_summary) {
        // Create Decorator for specific search
        if (!params->d_guid.empty()) {
            // Search GUIDs
            searchResult = bsl::allocate_shared<SearchGuidDecorator>(
                allocator,
                searchResult,
                params->d_guid,
                details);
        }
        else if (!params->d_seqNum.empty()) {
            // Search composite sequence numbers
            searchResult = bsl::allocate_shared<SearchSequenceNumberDecorator>(
                allocator,
                searchResult,
                params->d_seqNum,
                details);
        }
        else if (!params->d_offset.empty()) {
            // Search offsets
            searchResult = bsl::allocate_shared<SearchOffsetDecorator>(
                allocator,
                searchResult,
                params->d_offset,
                details);
        }
        else if (params->d_outstanding || params->d_confirmed) {
            // Search outstanding or confirmed
            searchResult = bsl::allocate_shared<SearchOutstandingDecorator>(
                allocator,
                searchResult);
        }
        else if (params->d_partiallyConfirmed) {
            // Search partially confirmed
            BSLS_ASSERT(!exactMatch);
            searchResult =
                bsl::allocate_shared<SearchPartiallyConfirmedDecorator>(
                    allocator,
                    searchResult);
        }
        else {
            // Drefault: search all
            searchResult = bsl::allocate_shared<SearchAllDecorator>(
                allocator,
                searchResult);
        }
    }

    // Add TimestampDecorator if 'timestampLt' is given.
    if (params->d_range.d_timestampLt) {
        searchResult = bsl::allocate_shared<SearchResultTimestampDecorator>(
            allocator,
            searchResult,
            params->d_range.d_timestampLt.value());
    }
    else if (params->d_range.d_offsetLt) {
        // Add OffsetDecorator if 'offsetLt' is given.
        searchResult = bsl::allocate_shared<SearchResultOffsetDecorator>(
            allocator,
            searchResult,
            params->d_range.d_offsetLt.value());
    }
    else if (params->d_range.d_seqNumLt) {
        // Add SequenceNumberDecorator if 'seqNumLt' is given.
        searchResult =
            bsl::allocate_shared<SearchResultSequenceNumberDecorator>(
                allocator,
                searchResult,
                params->d_range.d_seqNumLt.value());
    }

    BSLS_ASSERT(searchResult);

    return searchResult;
}

bsl::shared_ptr<CslSearchResult> SearchResultFactory::createCslSearchResult(
    const Parameters*                  params,
    const bsl::shared_ptr<CslPrinter>& printer,
    bslma::Allocator*                  allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT(params);

    // Create CslSearchResult implementation
    bsl::shared_ptr<CslSearchResult> cslSearchResult;
    if (params->d_details) {
        cslSearchResult = bsl::allocate_shared<CslSearchDetailResult>(
            allocator,
            printer,
            params->d_processCslRecordTypes);
    }
    else if (params->d_summary) {
        cslSearchResult = bsl::allocate_shared<CslSummaryResult>(
            allocator,
            printer,
            params->d_processCslRecordTypes,
            params->d_queueMap,
            params->d_cslSummaryQueuesLimit);
    }
    else {
        cslSearchResult = bsl::allocate_shared<CslSearchShortResult>(
            allocator,
            printer,
            params->d_processCslRecordTypes);
    }

    if (!params->d_seqNum.empty()) {
        // Search composite sequence numbers
        cslSearchResult =
            bsl::allocate_shared<CslSearchSequenceNumberDecorator>(
                allocator,
                cslSearchResult,
                params->d_seqNum);
    }
    else if (!params->d_offset.empty()) {
        // Search offsets
        cslSearchResult = bsl::allocate_shared<CslSearchOffsetDecorator>(
            allocator,
            cslSearchResult,
            params->d_offset);
    }

    BSLS_ASSERT(cslSearchResult);

    return cslSearchResult;
}

}  // close package namespace
}  // close enterprise namespace
