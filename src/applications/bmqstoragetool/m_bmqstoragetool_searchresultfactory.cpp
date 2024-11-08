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
#include "m_bmqstoragetool_parameters.h"
#include <m_bmqstoragetool_searchresultfactory.h>

namespace BloombergLP {
namespace m_bmqstoragetool {

// =========================
// class SearchResultFactory
// =========================

bsl::shared_ptr<SearchResult> SearchResultFactory::createSearchResult(
    const Parameters*                     params,
    const bslma::ManagedPtr<FileManager>& fileManager,
    bsl::ostream&                         ostream,
    bslma::Allocator*                     allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT(params);

    bslma::Allocator* alloc = bslma::Default::allocator(allocator);

    // Create payload dumper
    bslma::ManagedPtr<PayloadDumper> payloadDumper;
    if (params->d_dumpPayload) {
        payloadDumper.load(new (*alloc)
                               PayloadDumper(ostream,
                                             fileManager->dataFileIterator(),
                                             params->d_dumpLimit,
                                             alloc),
                           alloc);
    }

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
        searchResult.reset(new (*alloc) SearchDetailResult(ostream,
                                                           params->d_queueMap,
                                                           payloadDumper,
                                                           printImmediately,
                                                           eraseDeleted,
                                                           cleanUnprinted,
                                                           alloc),
                           alloc);
    }
    else {
        searchResult.reset(new (*alloc) SearchShortResult(ostream,
                                                          payloadDumper,
                                                          printImmediately,
                                                          eraseDeleted,
                                                          printOnDelete,
                                                          alloc),
                           alloc);
    }

    // Create Decorator for specific search
    if (!params->d_guid.empty()) {
        // Search GUIDs
        searchResult.reset(new (*alloc) SearchGuidDecorator(searchResult,
                                                            params->d_guid,
                                                            ostream,
                                                            details,
                                                            alloc),
                           alloc);
    }
    else if (params->d_summary) {
        // Summary
        searchResult.reset(
            new (*alloc) SummaryProcessor(ostream,
                                          fileManager->journalFileIterator(),
                                          fileManager->dataFileIterator(),
                                          alloc),
            alloc);
    }
    else if (params->d_outstanding || params->d_confirmed) {
        // Search outstanding or confirmed
        searchResult.reset(
            new (*alloc)
                SearchOutstandingDecorator(searchResult, ostream, alloc),
            alloc);
    }
    else if (params->d_partiallyConfirmed) {
        // Search partially confirmed
        searchResult.reset(new (*alloc)
                               SearchPartiallyConfirmedDecorator(searchResult,
                                                                 ostream,
                                                                 alloc),
                           alloc);
    }
    else {
        // Drefault: search all
        searchResult.reset(new (*alloc)
                               SearchAllDecorator(searchResult, alloc),
                           alloc);
    }

    if (params->d_valueLt > 0) {
        // Add TimestampDecorator if 'valueLt' is given and value type is `e_TIMESTAMP`.
        if (params->d_valueType == Parameters::e_TIMESTAMP) {
            searchResult.reset(
                new (*alloc) SearchResultTimestampDecorator(searchResult,
                                                            params->d_valueLt,
                                                            alloc),
                alloc);
        } else if (params->d_valueType == Parameters::e_OFFSET) {
            // Add OffsetDecorator if 'valueLt' is given and value type is `e_OFFSET`.
            searchResult.reset(
                new (*alloc) SearchResultOffsetDecorator(searchResult,
                                                            params->d_valueLt,
                                                            alloc),
                alloc);
        }
    }

    // Add SequenceNumberDecorator if 'seqNumLt' is given value type is `e_SEQUENCE_NUM`.
    if (params->d_valueType == Parameters::e_SEQUENCE_NUM && !params->d_seqNumLt.isUnset()) {
            searchResult.reset(
                new (*alloc) SearchResultSequenceNumberDecorator(searchResult,
                                                            params->d_seqNumLt,
                                                            alloc),
                alloc);
    }

    BSLS_ASSERT(searchResult);

    return searchResult;
}

}  // close package namespace
}  // close enterprise namespace
