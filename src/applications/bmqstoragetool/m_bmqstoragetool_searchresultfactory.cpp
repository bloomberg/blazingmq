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

    bslma::Allocator* alloc = bslma::Default::allocator(allocator);

    // Set up processing flags
    const bool details = params->d_details;
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

    // Create searchResult implementation
    bsl::shared_ptr<SearchResult> searchResult;
    if (params->d_summary) {
        searchResult.reset(
            new (*alloc) SummaryProcessor(printer,
                                          fileManager->journalFileIterator(),
                                          fileManager->dataFileIterator(),
                                          params->d_processRecordTypes,
                                          params->d_queueMap,
                                          params->d_minRecordsPerQueue,
                                          alloc),
            alloc);
    }
    else if (details) {
        bsl::cout << "create SearchDetailResult\n";
        searchResult.reset(new (*alloc)
                               SearchDetailResult(printer,
                                                  params->d_processRecordTypes,
                                                  params->d_queueMap,
                                                  payloadDumper,
                                                  printImmediately,
                                                  eraseDeleted,
                                                  cleanUnprinted,
                                                  alloc),
                           alloc);
    }
    else {
        searchResult.reset(new (*alloc)
                               SearchShortResult(printer,
                                                 params->d_processRecordTypes,
                                                 payloadDumper,
                                                 printImmediately,
                                                 eraseDeleted,
                                                 printOnDelete,
                                                 alloc),
                           alloc);
    }

    if (!params->d_summary) {
        // Create Decorator for specific search
        if (!params->d_guid.empty()) {
            // Search GUIDs
            searchResult.reset(new (*alloc) SearchGuidDecorator(searchResult,
                                                                params->d_guid,
                                                                details,
                                                                alloc),
                               alloc);
        }
        else if (!params->d_seqNum.empty()) {
            // Search offsets
            searchResult.reset(
                new (*alloc) SearchSequenceNumberDecorator(searchResult,
                                                           params->d_seqNum,
                                                           details,
                                                           alloc),
                alloc);
        }
        else if (!params->d_offset.empty()) {
            // Search composite sequence numbers
            searchResult.reset(new (*alloc)
                                   SearchOffsetDecorator(searchResult,
                                                         params->d_offset,
                                                         details,
                                                         alloc),
                               alloc);
        }
        else if (params->d_outstanding || params->d_confirmed) {
            // Search outstanding or confirmed
            searchResult.reset(
                new (*alloc) SearchOutstandingDecorator(searchResult, alloc),
                alloc);
        }
        else if (params->d_partiallyConfirmed) {
            // Search partially confirmed
            searchResult.reset(
                new (*alloc)
                    SearchPartiallyConfirmedDecorator(searchResult, alloc),
                alloc);
        }
        else {
            // Drefault: search all
            searchResult.reset(new (*alloc)
                                   SearchAllDecorator(searchResult, alloc),
                               alloc);
        }
    }

    // Add TimestampDecorator if 'timestampLt' is given.
    if (params->d_range.d_timestampLt) {
        searchResult.reset(new (*alloc) SearchResultTimestampDecorator(
                               searchResult,
                               params->d_range.d_timestampLt.value(),
                               alloc),
                           alloc);
    }
    else if (params->d_range.d_offsetLt) {
        // Add OffsetDecorator if 'offsetLt' is given.
        searchResult.reset(new (*alloc) SearchResultOffsetDecorator(
                               searchResult,
                               params->d_range.d_offsetLt.value(),
                               alloc),
                           alloc);
    }
    else if (params->d_range.d_seqNumLt) {
        // Add SequenceNumberDecorator if 'seqNumLt' is given.
        searchResult.reset(new (*alloc) SearchResultSequenceNumberDecorator(
                               searchResult,
                               params->d_range.d_seqNumLt.value(),
                               alloc),
                           alloc);
    }

    BSLS_ASSERT(searchResult);

    return searchResult;
}

}  // close package namespace
}  // close enterprise namespace
