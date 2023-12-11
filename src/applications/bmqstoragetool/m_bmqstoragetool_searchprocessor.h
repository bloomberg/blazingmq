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
#include <m_bmqstoragetool_commandprocessor.h>

// MQB
#include <mqbs_datafileiterator.h>
#include <mqbs_filestoreprotocol.h>
#include <mqbs_journalfileiterator.h>
#include <mqbs_mappedfiledescriptor.h>
// #include <mqbu_storagekey.h>

// BDE
#include <bsls_keyword.h>

namespace BloombergLP {
namespace m_bmqstoragetool {

// =====================
// class SearchProcessor
// =====================

class SearchProcessor : public CommandProcessor {
  private:
    // DATA
    bslma::Allocator* d_allocator_p;

    // MANIPULATORS

  public:
    // CREATORS
    explicit SearchProcessor(const bsl::shared_ptr<Parameters>& params,
                             bslma::Allocator*                  allocator);

    // MANIPULATORS
    void process(bsl::ostream& ostream) BSLS_KEYWORD_OVERRIDE;
};

}  // close package namespace
}  // close enterprise namespace
