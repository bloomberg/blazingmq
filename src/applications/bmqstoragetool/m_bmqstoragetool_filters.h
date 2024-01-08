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

// m_bmqstoragetool_filters.h                                       -*-C++-*-
#ifndef INCLUDED_M_BMQSTORAGETOOL_FILTERS
#define INCLUDED_M_BMQSTORAGETOOL_FILTERS

#include <m_bmqstoragetool_parameters.h>

// MQB
#include <mqbu_storagekey.h>

namespace BloombergLP {
namespace m_bmqstoragetool {

// =====================
// class Filters
// =====================

class Filters {
  private:
    // DATA
    bsl::vector<mqbu::StorageKey> d_queueKeys;
    const bsls::Types::Int64      d_timestampGt;
    const bsls::Types::Int64      d_timestampLt;

  public:
    // CREATORS
    explicit Filters(const bsl::vector<bsl::string>& queueHexKeys,
                     const bsl::vector<bsl::string>& queueURIS,
                     const QueueMap&                 queueMap,
                     const bsls::Types::Int64        timestampGt,
                     const bsls::Types::Int64        timestampLt,
                     bsl::ostream&                   ostream,
                     bslma::Allocator*               allocator);

    // MANIPULATORS
    bool apply(const mqbs::MessageRecord& record);
    // Apply filters and return true if filter matched, false otherwise.
};

}  // close package namespace
}  // close enterprise namespace

#endif
