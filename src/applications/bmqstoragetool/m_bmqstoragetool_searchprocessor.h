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

// m_bmqstoragetool_searchprocessor.h -*-C++-*-
#ifndef INCLUDED_M_BMQSTORAGETOOL_SEARCHPROCESSOR
#define INCLUDED_M_BMQSTORAGETOOL_SEARCHPROCESSOR

// bmqstoragetool
#include <m_bmqstoragetool_commandprocessor.h>

// MQB
#include <mqbs_datafileiterator.h>
#include <mqbs_filestoreprotocol.h>
#include <mqbs_journalfileiterator.h>
#include <mqbs_mappedfiledescriptor.h>

// BDE
#include <bsls_keyword.h>

namespace BloombergLP {
namespace m_bmqstoragetool {

// =====================
// class SearchProcessor
// =====================

int moveToLowerBound(mqbs::JournalFileIterator* it,
                     const bsls::Types::Uint64& timestamp);

class SearchProcessor : public CommandProcessor {
  private:
    // DATA
    bslma::Allocator* d_allocator_p;

    // MANIPULATORS

  public:
    // CREATORS
    explicit SearchProcessor(bsl::unique_ptr<Parameters> params,
                             bslma::Allocator*           allocator);

    // MANIPULATORS
    void process(bsl::ostream& ostream) BSLS_KEYWORD_OVERRIDE;
};

}  // close package namespace
}  // close enterprise namespace

#endif
