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

#ifndef INCLUDED_M_BMQSTORAGETOOL_CSLFILEPROCESSOR
#define INCLUDED_M_BMQSTORAGETOOL_CSLFILEPROCESSOR

//@PURPOSE: Provide engine for searching in a cluster state ledger (CSL) file.
//
//@CLASSES:
//  m_bmqstoragetool::CslFileProcessor: search engine.
//
//@DESCRIPTION: 'CslFileProcessor' provides engine for iterating a CSL
//  file and searching records in it.

// bmqstoragetool
#include <m_bmqstoragetool_commandprocessor.h>
#include <m_bmqstoragetool_cslsearchresult.h>

// BDE
#include <bsls_keyword.h>

namespace BloombergLP {
namespace m_bmqstoragetool {

// ==========================
// class CslFileProcessor
// ==========================

class CslFileProcessor : public CommandProcessor {
  private:
    // PRIVATE DATA

    const Parameters*                    d_parameters;
    const bslma::ManagedPtr<FileManager> d_fileManager;
    bsl::ostream&                        d_ostream;
    bsl::shared_ptr<CslSearchResult>     d_searchResult_p;
    bslma::Allocator*                    d_allocator_p;

  public:
    // CREATORS

    /// Constructor using the specified `params`, 'fileManager',
    /// 'searchResult_p', 'ostream' and 'allocator'.
    CslFileProcessor(const Parameters*                       params,
                     bslma::ManagedPtr<FileManager>&         fileManager,
                     const bsl::shared_ptr<CslSearchResult>& searchResult_p,
                     bsl::ostream&                           ostream,
                     bslma::Allocator*                       allocator);

    // MANIPULATORS

    /// Process the CSL file and print result.
    void process() BSLS_KEYWORD_OVERRIDE;
};

}  // close package namespace
}  // close enterprise namespace

#endif
