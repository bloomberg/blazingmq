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

namespace {

// Helpers to create base searchResult implementation
void createSearchDetailResult(bsl::shared_ptr<SearchResult>&  baseResult,
                              bsl::ostream&                   ostream,
                              const QueueMap&                 queueMap,
                              bsl::shared_ptr<PayloadDumper>& payloadDumper,
                              bslma::Allocator*               allocator,
                              bool                            printImmediately,
                              bool                            eraseDeleted,
                              bool                            cleanUnprinted)
{
    baseResult.reset(new (*allocator) SearchDetailResult(ostream,
                                                         queueMap,
                                                         payloadDumper,
                                                         allocator,
                                                         printImmediately,
                                                         eraseDeleted,
                                                         cleanUnprinted),
                     allocator);
}

void createSearchShortResult(bsl::shared_ptr<SearchResult>&  baseResult,
                             bsl::ostream&                   ostream,
                             bsl::shared_ptr<PayloadDumper>& payloadDumper,
                             bslma::Allocator*               allocator,
                             bool                            printImmediately,
                             bool                            printOnDelete,
                             bool                            eraseDeleted)
{
    baseResult.reset(new (*allocator) SearchShortResult(ostream,
                                                        payloadDumper,
                                                        allocator,
                                                        printImmediately,
                                                        printOnDelete,
                                                        eraseDeleted),
                     allocator);
}

}  // close unnamed namespace

// =============================
// class SearchResultFactory
// =============================

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

    // Create searchResult implementation
    bsl::shared_ptr<SearchResult> baseResult;
    bsl::shared_ptr<SearchResult> searchResult;
    if (!params->guid().empty()) {
        // Search GUIDs
        if (params->details())
            createSearchDetailResult(baseResult,
                                     ostream,
                                     params->queueMap(),
                                     payloadDumper,
                                     allocator,
                                     true,
                                     true,
                                     true);
        else
            createSearchShortResult(baseResult,
                                    ostream,
                                    payloadDumper,
                                    allocator,
                                    true,
                                    true,
                                    true);
        searchResult.reset(new (*allocator)
                               SearchGuidDecorator(baseResult,
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
        if (params->details())
            createSearchDetailResult(baseResult,
                                     ostream,
                                     params->queueMap(),
                                     payloadDumper,
                                     allocator,
                                     false,
                                     true,
                                     false);
        else
            createSearchShortResult(baseResult,
                                    ostream,
                                    payloadDumper,
                                    allocator,
                                    false,
                                    false,
                                    true);
        searchResult.reset(
            new (*allocator)
                SearchOutstandingDecorator(baseResult, ostream, allocator),
            allocator);
    }
    else if (params->confirmed()) {
        // Search confirmed
        if (params->details())
            createSearchDetailResult(baseResult,
                                     ostream,
                                     params->queueMap(),
                                     payloadDumper,
                                     allocator,
                                     true,
                                     true,
                                     true);
        else
            createSearchShortResult(baseResult,
                                    ostream,
                                    payloadDumper,
                                    allocator,
                                    false,
                                    true,
                                    true);
        searchResult.reset(
            new (*allocator)
                SearchOutstandingDecorator(baseResult, ostream, allocator),
            allocator);
    }
    else if (params->partiallyConfirmed()) {
        // Search partially confirmed
        if (params->details())
            createSearchDetailResult(baseResult,
                                     ostream,
                                     params->queueMap(),
                                     payloadDumper,
                                     allocator,
                                     false,
                                     true,
                                     false);
        else
            createSearchShortResult(baseResult,
                                    ostream,
                                    payloadDumper,
                                    allocator,
                                    false,
                                    false,
                                    true);
        searchResult.reset(new (*allocator)
                               SearchPartiallyConfirmedDecorator(baseResult,
                                                                 ostream,
                                                                 allocator),
                           allocator);
    }
    else {
        // Drefault: search all
        if (params->details())
            createSearchDetailResult(baseResult,
                                     ostream,
                                     params->queueMap(),
                                     payloadDumper,
                                     allocator,
                                     true,
                                     true,
                                     false);
        else
            createSearchShortResult(baseResult,
                                    ostream,
                                    payloadDumper,
                                    allocator,
                                    true,
                                    false,
                                    false);
        searchResult.reset(new (*allocator) SearchAllDecorator(baseResult),
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
