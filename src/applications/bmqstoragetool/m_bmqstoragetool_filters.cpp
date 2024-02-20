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

#include <m_bmqstoragetool_filters.h>

namespace BloombergLP {
namespace m_bmqstoragetool {

// =============
// class Filters
// =============

Filters::Filters(const bsl::vector<bsl::string>& queueHexKeys,
                 const bsl::vector<bsl::string>& queueURIS,
                 const QueueMap&                 queueMap,
                 const bsls::Types::Int64        timestampGt,
                 const bsls::Types::Int64        timestampLt,
                 bsl::ostream&                   ostream,
                 bslma::Allocator*               allocator)
: d_queueKeys(allocator)
, d_timestampGt(timestampGt)
, d_timestampLt(timestampLt)
{
    // Fill internal structures
    if (!queueHexKeys.empty()) {
        for (const auto& key : queueHexKeys) {
            d_queueKeys.insert(
                mqbu::StorageKey(mqbu::StorageKey::HexRepresentation(),
                                 key.c_str()));
        }
    }
    if (!queueURIS.empty()) {
        mqbu::StorageKey key;
        for (const auto& uri : queueURIS) {
            if (queueMap.findKeyByUri(&key, uri)) {
                d_queueKeys.insert(key);
            }
        }
    }
}

bool Filters::apply(const mqbs::MessageRecord& record)
{
    if (!d_queueKeys.empty())
        // Match by queueKey
        if (auto it = bsl::find(d_queueKeys.begin(),
                                d_queueKeys.end(),
                                record.queueKey());
            it == d_queueKeys.end()) {
            // Not matched
            return false;  // RETURN
        }
    const bsls::Types::Uint64& ts = record.header().timestamp();
    if ((d_timestampGt > 0 && ts <= d_timestampGt) ||
        (d_timestampLt > 0 && ts >= d_timestampLt)) {
        // Match by timestamp
        return false;  // RETURN
    }
    return true;
}

}  // close package namespace
}  // close enterprise namespace
