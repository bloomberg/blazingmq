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

// m_bmqstoragetool_searchresultfactory.cpp -*-C++-*-

// bmqstoragetool
#include <m_bmqstoragetool_searchresultfactory.h>

namespace BloombergLP {
namespace m_bmqstoragetool {

// =========================
// class SearchResultFactory
// =========================

bsl::shared_ptr<SearchResult>
SearchResultFactory::createSearchResult(bsl::shared_ptr<Parameters> params,
                                        bsl::ostream&               ostream,
                                        bslma::Allocator*           allocator)
{
    // Create payload dumper
    bsl::shared_ptr<PayloadDumper> payloadDumper;
    if (params->dumpPayload())
        payloadDumper.reset(new (*allocator)
                                PayloadDumper(ostream,
                                              params->dataFileIterator(),
                                              params->dumpLimit()),
                            allocator);

    // Set up processing flags
    bool details = params->details();
    // Print data immediately as soon as it is completed to save memory, except
    // the foollowing cases, where data should be kept
    bool printImmediately = !(params->outstanding() ||
                              params->partiallyConfirmed() ||
                              (params->confirmed() && !details));
    // Always erase stored data when 'deletion' record received
    bool eraseDeleted = true;
    // Print data immediately on 'deletion' record receiving for specific case
    bool printOnDelete = params->confirmed();
    // Clean unprinted/unerased data for specific case
    bool cleanUnprinted = params->confirmed();

    // Create searchResult implementation
    bsl::shared_ptr<SearchResult> searchResult;
    if (details)
        searchResult.reset(new (*allocator)
                               SearchDetailResult(ostream,
                                                  params->queueMap(),
                                                  payloadDumper,
                                                  allocator,
                                                  printImmediately,
                                                  eraseDeleted,
                                                  cleanUnprinted),
                           allocator);
    else
        searchResult.reset(new (*allocator) SearchShortResult(ostream,
                                                              payloadDumper,
                                                              allocator,
                                                              printImmediately,
                                                              eraseDeleted,
                                                              printOnDelete),
                           allocator);

    // Create Decorator for specific search
    if (!params->guid().empty()) {
        // Search GUIDs
        searchResult.reset(new (*allocator)
                               SearchGuidDecorator(searchResult,
                                                   params->guid(),
                                                   ostream,
                                                   params->details(),
                                                   allocator),
                           allocator);
    }
    else if (params->summary()) {
        // Summary
        searchResult.reset(new (*allocator)
                               SummaryProcessor(ostream,
                                                params->journalFileIterator(),
                                                params->dataFileIterator(),
                                                allocator),
                           allocator);
    }
    else if (params->outstanding()) {
        // Search outstanding
        searchResult.reset(
            new (*allocator)
                SearchOutstandingDecorator(searchResult, ostream, allocator),
            allocator);
    }
    else if (params->confirmed()) {
        // Search confirmed
        searchResult.reset(
            new (*allocator)
                SearchOutstandingDecorator(searchResult, ostream, allocator),
            allocator);
    }
    else if (params->partiallyConfirmed()) {
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
    if (params->timestampLt() > 0) {
        searchResult.reset(new (*allocator) SearchResultTimestampDecorator(
                               searchResult,
                               params->timestampLt()),
                           allocator);
    }

    BSLS_ASSERT(searchResult);

    return searchResult;
}

}  // close package namespace
}  // close enterprise namespace
