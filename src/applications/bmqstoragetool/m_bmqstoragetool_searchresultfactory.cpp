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

// =============================
// class SearchResultFactory
// =============================

bsl::shared_ptr<SearchResult>
SearchResultFactory::createSearchResult(Parameters*       params,
                                        bsl::ostream&     ostream,
                                        bslma::Allocator* allocator)
{
    // Create payload dumper
    bsl::shared_ptr<PayloadDumper> payloadDumper;
    if (params->dumpPayload())
        payloadDumper.reset(new (*allocator)
                                PayloadDumper(ostream,
                                              params->dataFileIterator(),
                                              params->dumpLimit()),
                            allocator);

    // TODO: why unique_ptr doesn't support deleter in reset()
    // bsl::unique_ptr<SearchResult> searchResult;
    bsl::shared_ptr<SearchResult> searchResult;
    if (!params->guid().empty()) {
        if (params->details()) {
            // Base: Details
            searchResult.reset(new (*allocator)
                                   SearchDetailResult(ostream,
                                                      params->queueMap(),
                                                      payloadDumper,
                                                      allocator,
                                                      true,
                                                      true,
                                                      true),
                               allocator);
        }
        else {
            // Base: Short
            searchResult.reset(new (*allocator)
                                   SearchShortResult(ostream,
                                                     payloadDumper,
                                                     allocator,
                                                     true,
                                                     true,
                                                     true),
                               allocator);
        }
        // Decorator
        searchResult.reset(new (*allocator)
                               SearchGuidDecorator(searchResult,
                                                   params->guid(),
                                                   ostream,
                                                   params->details(),
                                                   allocator),
                           allocator);
    }
    else if (params->summary()) {
        searchResult.reset(new (*allocator)
                               SummaryProcessor(ostream,
                                                params->journalFileIterator(),
                                                params->dataFileIterator(),
                                                allocator),
                           allocator);
    }
    else if (params->outstanding()) {
        if (params->details()) {
            // Base: Details
            searchResult.reset(new (*allocator)
                                   SearchDetailResult(ostream,
                                                      params->queueMap(),
                                                      payloadDumper,
                                                      allocator,
                                                      false,
                                                      true,
                                                      false),
                               allocator);
        }
        else {
            // Base: Short
            searchResult.reset(new (*allocator)
                                   SearchShortResult(ostream,
                                                     payloadDumper,
                                                     allocator,
                                                     false,
                                                     false,
                                                     true),
                               allocator);
        }
        // Decorator
        searchResult.reset(
            new (*allocator)
                SearchOutstandingDecorator(searchResult, ostream, allocator),
            allocator);
    }
    else if (params->confirmed()) {
        if (params->details()) {
            // Base: Details
            searchResult.reset(new (*allocator)
                                   SearchDetailResult(ostream,
                                                      params->queueMap(),
                                                      payloadDumper,
                                                      allocator,
                                                      true,
                                                      true,
                                                      true),
                               allocator);
        }
        else {
            // Base: Short
            searchResult.reset(new (*allocator)
                                   SearchShortResult(ostream,
                                                     payloadDumper,
                                                     allocator,
                                                     false,
                                                     true,
                                                     true),
                               allocator);
        }
        // Decorator
        searchResult.reset(
            new (*allocator)
                SearchOutstandingDecorator(searchResult, ostream, allocator),
            allocator);
    }
    else if (params->partiallyConfirmed()) {
        if (params->details()) {
            // Base: Details
            searchResult.reset(new (*allocator)
                                   SearchDetailResult(ostream,
                                                      params->queueMap(),
                                                      payloadDumper,
                                                      allocator,
                                                      false,
                                                      true,
                                                      false),
                               allocator);
        }
        else {
            // Base: Short
            searchResult.reset(new (*allocator)
                                   SearchShortResult(ostream,
                                                     payloadDumper,
                                                     allocator,
                                                     false,
                                                     false,
                                                     true),
                               allocator);
        }
        // Decorator
        searchResult.reset(new (*allocator)
                               SearchPartiallyConfirmedDecorator(searchResult,
                                                                 ostream,
                                                                 allocator),
                           allocator);
    }
    else {
        if (params->details()) {
            // Base: Details
            searchResult.reset(new (*allocator)
                                   SearchDetailResult(ostream,
                                                      params->queueMap(),
                                                      payloadDumper,
                                                      allocator),
                               allocator);
        }
        else {
            // Base: Short
            searchResult.reset(
                new (*allocator)
                    SearchShortResult(ostream, payloadDumper, allocator),
                allocator);
        }
        // Decorator
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
