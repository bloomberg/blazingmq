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

#ifndef INCLUDED_M_BMQSTORAGETOOL_JOURNALFILEPROCESSOR
#define INCLUDED_M_BMQSTORAGETOOL_JOURNALFILEPROCESSOR

//@PURPOSE: Provide engine for searching in a journal file.
//
//@CLASSES:
//  m_bmqstoragetool::JournalFileProcessor: search engine.
//
//@DESCRIPTION: 'JournalFileProcessor' provides engine for iterating a journal
//  file and searching records in it.

// bmqstoragetool
#include <m_bmqstoragetool_commandprocessor.h>
#include <m_bmqstoragetool_searchresult.h>

// MQB
#include <mqbs_datafileiterator.h>
#include <mqbs_filestoreprotocol.h>
#include <mqbs_journalfileiterator.h>
#include <mqbs_mappedfiledescriptor.h>

// BDE
#include <bsls_keyword.h>

namespace BloombergLP {
namespace m_bmqstoragetool {

int moveToLowerBound(mqbs::JournalFileIterator* jit,
                     const Parameters::SearchValueType valueType,
                     const bsls::Types::Uint64& value);
int moveToLowerSeqNumber(mqbs::JournalFileIterator* jit,
                     const CompositeSequenceNumber& seqNumGt);
template<typename T>
int moveToLower(mqbs::JournalFileIterator* jit,
                     const Parameters::SearchValueType valueType,
                     const T& value);

// ==========================
// class JournalFileProcessor
// ==========================

class JournalFileProcessor : public CommandProcessor {
  private:
    // PRIVATE DATA
    const Parameters*                    d_parameters;
    const bslma::ManagedPtr<FileManager> d_fileManager;
    bsl::ostream&                        d_ostream;
    bsl::shared_ptr<SearchResult>        d_searchResult_p;
    bslma::Allocator*                    d_allocator_p;

    // MANIPULATORS

  public:
    // CREATORS
    /// Constructor using the specified `params`, 'fileManager',
    /// 'searchResult_p', 'ostream' and 'allocator'.
    JournalFileProcessor(const Parameters*                    params,
                         bslma::ManagedPtr<FileManager>&      fileManager,
                         const bsl::shared_ptr<SearchResult>& searchResult_p,
                         bsl::ostream&                        ostream,
                         bslma::Allocator*                    allocator);

    // MANIPULATORS
    /// Process the journal file and print result.
    void process() BSLS_KEYWORD_OVERRIDE;
};

}  // close package namespace
}  // close enterprise namespace

#endif
