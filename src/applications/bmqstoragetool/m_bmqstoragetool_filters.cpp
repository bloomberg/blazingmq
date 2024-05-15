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

Filters::Filters(const bsl::vector<bsl::string>& queueKeys,
                 const bsl::vector<bsl::string>& queueUris,
                 const QueueMap&                 queueMap,
                 const bsls::Types::Int64        timestampGt,
                 const bsls::Types::Int64        timestampLt,
                 bslma::Allocator*               allocator)
: d_queueKeys(allocator)
, d_timestampGt(static_cast<bsls::Types::Uint64>(timestampGt))
, d_timestampLt(static_cast<bsls::Types::Uint64>(timestampLt))
{
    // Fill internal structures
    if (!queueKeys.empty()) {
        bsl::vector<bsl::string>::const_iterator keyIt = queueKeys.cbegin();
        for (; keyIt != queueKeys.cend(); ++keyIt) {
            d_queueKeys.insert(
                mqbu::StorageKey(mqbu::StorageKey::HexRepresentation(),
                                 keyIt->c_str()));
        }
    }
    if (!queueUris.empty()) {
        mqbu::StorageKey                         key;
        bsl::vector<bsl::string>::const_iterator uriIt = queueUris.cbegin();
        for (; uriIt != queueUris.cend(); ++uriIt) {
            if (queueMap.findKeyByUri(&key, *uriIt)) {
                d_queueKeys.insert(key);
            }
        }
    }
}

bool Filters::apply(const mqbs::MessageRecord& record)
{
    if (!d_queueKeys.empty()) {
        // Match by queueKey
        bsl::unordered_set<mqbu::StorageKey>::const_iterator it = bsl::find(
            d_queueKeys.cbegin(),
            d_queueKeys.cend(),
            record.queueKey());
        if (it == d_queueKeys.cend()) {
            // Not matched
            return false;  // RETURN
        }
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
