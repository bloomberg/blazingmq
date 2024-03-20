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

#ifndef INCLUDED_M_BMQSTORAGETOOL_FILTERS
#define INCLUDED_M_BMQSTORAGETOOL_FILTERS

//@PURPOSE: Provide filters for search engine.
//
//@CLASSES:
//  m_bmqstoragetool::Filters: Filters for search engine.
//
//@DESCRIPTION: 'Filters' provides filters for search engine.

// bmqstoragetool
#include <m_bmqstoragetool_parameters.h>

// MQB
#include <mqbu_storagekey.h>

// BDE
#include <bsl_unordered_set.h>

namespace BloombergLP {
namespace m_bmqstoragetool {

// =============
// class Filters
// =============

class Filters {
  private:
    // DATA
    bsl::unordered_set<mqbu::StorageKey> d_queueKeys;
    const bsls::Types::Int64             d_timestampGt;
    const bsls::Types::Int64             d_timestampLt;

  public:
    // CREATORS

    /// Constructor using the specified arguments.
    explicit Filters(const bsl::vector<bsl::string>& queueKeys,
                     const bsl::vector<bsl::string>& queueUris,
                     const QueueMap&                 queueMap,
                     const bsls::Types::Int64        timestampGt,
                     const bsls::Types::Int64        timestampLt,
                     bsl::ostream&                   ostream,
                     bslma::Allocator*               allocator);

    // MANIPULATORS

    /// Apply filters at specified 'record' and return true if all filters
    /// are matched, false otherwise.
    bool apply(const mqbs::MessageRecord& record);
};

}  // close package namespace
}  // close enterprise namespace

#endif
