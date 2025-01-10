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
#include <mqbs_filestoreprotocol.h>
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
    const Parameters::Range&             d_range;

  public:
    // CREATORS

    /// Constructor using the specified arguments.
    explicit Filters(const bsl::vector<bsl::string>& queueKeys,
                     const bsl::vector<bsl::string>& queueUris,
                     const QueueMap&                 queueMap,
                     const Parameters::Range&        range,
                     bslma::Allocator*               allocator);

    // ACCESSORS

    /// Apply filters at the record with the specified `recordHeader`,
    /// `recordOffset` and `queueKey`. Return true if all filters are matched,
    /// false otherwise. If the specified `highBoundReached_p` pointer is
    /// present, pointed value is set to true if higher bound value is reached,
    /// false otherwise.
    bool apply(const mqbs::RecordHeader& recordHeader,
               bsls::Types::Uint64       recordOffset,
               const mqbu::StorageKey&   queueKey,
               bool*                     highBoundReached_p = 0) const;
};

}  // close package namespace
}  // close enterprise namespace

#endif
