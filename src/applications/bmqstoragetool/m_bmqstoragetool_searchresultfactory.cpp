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
#include <m_bmqstoragetool_searchresultfactory.h>

namespace BloombergLP {
namespace m_bmqstoragetool {

// =========================
// class SearchResultFactory
// =========================

bsl::shared_ptr<SearchResult> SearchResultFactory::createSearchResult(
    const Parameters*                   params,
    const bsl::shared_ptr<FileManager>& fileManager,
    bsl::ostream&                       ostream,
    bslma::Allocator*                   allocator)
{
    // Create payload dumper
    bsl::shared_ptr<PayloadDumper> payloadDumper;
    if (params->d_dumpPayload)
        payloadDumper.reset(new (*allocator)
                                PayloadDumper(ostream,
                                              fileManager->dataFileIterator(),
                                              params->d_dumpLimit),
                            allocator);

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
    if (details) {
        searchResult.reset(new (*allocator)
                               SearchDetailResult(ostream,
                                                  params->d_queueMap,
                                                  payloadDumper,
                                                  allocator,
                                                  printImmediately,
                                                  eraseDeleted,
                                                  cleanUnprinted),
                           allocator);
    }
    else {
        searchResult.reset(new (*allocator) SearchShortResult(ostream,
                                                              payloadDumper,
                                                              allocator,
                                                              printImmediately,
                                                              eraseDeleted,
                                                              printOnDelete),
                           allocator);
    }

    // Create Decorator for specific search
    if (!params->d_guid.empty()) {
        // Search GUIDs
        searchResult.reset(new (*allocator)
                               SearchGuidDecorator(searchResult,
                                                   params->d_guid,
                                                   ostream,
                                                   params->d_details,
                                                   allocator),
                           allocator);
    }
    else if (params->d_summary) {
        // Summary
        searchResult.reset(new (*allocator) SummaryProcessor(
                               ostream,
                               fileManager->journalFileIterator(),
                               fileManager->dataFileIterator(),
                               allocator),
                           allocator);
    }
    else if (params->d_outstanding || params->d_confirmed) {
        // Search outstanding or confirmed
        searchResult.reset(
            new (*allocator)
                SearchOutstandingDecorator(searchResult, ostream, allocator),
            allocator);
    }
    else if (params->d_partiallyConfirmed) {
        // Search partially confirmed
        searchResult.reset(new (*allocator)
                               SearchPartiallyConfirmedDecorator(searchResult,
                                                                 ostream,
                                                                 allocator),
                           allocator);
    }
    else {
        // Drefault: search all
        searchResult.reset(new (*allocator) SearchAllDecorator(searchResult),
                           allocator);
    }

    // Add TimestampDecorator if 'timestampLt' is given.
    if (params->d_timestampLt > 0) {
        searchResult.reset(new (*allocator) SearchResultTimestampDecorator(
                               searchResult,
                               params->d_timestampLt),
                           allocator);
    }

    BSLS_ASSERT(searchResult);

    return searchResult;
}

}  // close package namespace
}  // close enterprise namespace
