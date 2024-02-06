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

// m_bmqstoragetool_searchresultfactory.h -*-C++-*-
#ifndef INCLUDED_M_BMQSTORAGETOOL_SEARCHRESULTFACTORY
#define INCLUDED_M_BMQSTORAGETOOL_SEARCHRESULTFACTORY

//@PURPOSE: Provide a factory to create corresponding search result object.
//
//@CLASSES:
//  m_bmqstoragetool::SearchResultFactory: search result factory.
//
//@DESCRIPTION: 'SearchResultFactory' provides a factory to create
// corresponding search result object.

// bmqstoragetool
#include <m_bmqstoragetool_payloaddumper.h>
#include <m_bmqstoragetool_searchresult.h>

namespace BloombergLP {
namespace m_bmqstoragetool {

// =========================
// class SearchResultFactory
// =========================

class SearchResultFactory {
  public:
    // MANIPULATORS

    /// Create command processor for specified 'params' using specified
    /// 'ostream' and 'allocator'.
    static bsl::shared_ptr<SearchResult>
    createSearchResult(Parameters*       params,
                       bsl::ostream&     ostream,
                       bslma::Allocator* allocator);
};

}  // close package namespace
}  // close enterprise namespace

#endif
