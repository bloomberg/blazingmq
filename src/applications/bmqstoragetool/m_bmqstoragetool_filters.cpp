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
#include <m_bmqstoragetool_filters.h>

namespace BloombergLP {
namespace m_bmqstoragetool {

// =============
// class Filters
// =============

Filters::Filters(const bsl::vector<bsl::string>& queueKeys,
                 const bsl::vector<bsl::string>& queueUris,
                 const QueueMap&                 queueMap,
                 const Parameters::Range&        range,
                 bslma::Allocator*               allocator)
: d_queueKeys(allocator)
, d_range(range)
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

bool Filters::apply(const mqbs::RecordHeader& recordHeader,
                    bsls::Types::Uint64       recordOffset,
                    const mqbu::StorageKey&   queueKey,
                    bool*                     highBoundReached_p) const
{
    if (highBoundReached_p) {
        *highBoundReached_p = false;  // by default
    }

    // Apply `queue key` filter
    if (queueKey != mqbu::StorageKey::k_NULL_KEY && !d_queueKeys.empty()) {
        // Match by queueKey
        bsl::unordered_set<mqbu::StorageKey>::const_iterator it =
            bsl::find(d_queueKeys.cbegin(), d_queueKeys.cend(), queueKey);
        if (it == d_queueKeys.cend()) {
            // Not matched
            return false;  // RETURN
        }
    }

    // Apply `range` filter
    bsls::Types::Uint64 value, valueGt, valueLt;
    switch (d_range.d_type) {
    case Parameters::Range::e_TIMESTAMP:
        value   = recordHeader.timestamp();
        valueGt = d_range.d_timestampGt;
        valueLt = d_range.d_timestampLt;
        break;
    case Parameters::Range::e_OFFSET:
        value   = recordOffset;
        valueGt = d_range.d_offsetGt;
        valueLt = d_range.d_offsetLt;
        break;
    case Parameters::Range::e_SEQUENCE_NUM: {
        CompositeSequenceNumber seqNum(recordHeader.primaryLeaseId(),
                                       recordHeader.sequenceNumber());
        const bool greaterOrEqualToHigherBound = d_range.d_seqNumLt.isSet() &&
                                                 d_range.d_seqNumLt <= seqNum;
        if (highBoundReached_p && greaterOrEqualToHigherBound) {
            *highBoundReached_p = true;
        }

        return !(
            (d_range.d_seqNumGt.isSet() && seqNum <= d_range.d_seqNumGt) ||
            greaterOrEqualToHigherBound);  // RETURN
    } break;
    default:
        // No range filter defined
        return true;  // RETURN
    }
    const bool greaterOrEqualToHigherBound = valueLt > 0 && value >= valueLt;
    if ((valueGt > 0 && value <= valueGt) || greaterOrEqualToHigherBound) {
        if (highBoundReached_p && greaterOrEqualToHigherBound) {
            *highBoundReached_p = true;
        }
        // Not inside range
        return false;  // RETURN
    }

    return true;
}

}  // close package namespace
}  // close enterprise namespace
