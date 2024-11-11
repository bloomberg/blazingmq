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

Filters::Filters(const bsl::vector<bsl::string>&   queueKeys,
                 const bsl::vector<bsl::string>&   queueUris,
                 const QueueMap&                   queueMap,
                 const Parameters::SearchValueType valueType,
                 const bsls::Types::Uint64         valueGt,
                 const bsls::Types::Uint64         valueLt,
                 const CompositeSequenceNumber     seqNumGt,
                 const CompositeSequenceNumber     seqNumLt,
                 bslma::Allocator*                 allocator)
: d_queueKeys(allocator)
, d_valueType(valueType)
, d_valueGt(valueGt)
, d_valueLt(valueLt)
, d_seqNumGt(seqNumGt)
, d_seqNumLt(seqNumLt)
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

bool Filters::apply(const mqbs::MessageRecord& record,
                    bsls::Types::Uint64        offset) const
{
    // Apply `queue key` filter
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

    // Apply `range` filter
    bsls::Types::Uint64 value;
    switch (d_valueType) {
    case Parameters::e_TIMESTAMP: value = record.header().timestamp(); break;
    case Parameters::e_SEQUENCE_NUM: {
        CompositeSequenceNumber seqNum(record.header().primaryLeaseId(),
                                       record.header().sequenceNumber());
        if ((!d_seqNumGt.isUnset() && seqNum <= d_seqNumGt) ||
            (!d_seqNumLt.isUnset() && d_seqNumLt <= seqNum)) {
            // Not inside range
            return false;  // RETURN
        }
    } break;
    case Parameters::e_OFFSET: value = offset; break;
    default:
        // No range filter defined
        return true;  // RETURN
    }
    if ((d_valueGt > 0 && value <= d_valueGt) ||
        (d_valueLt > 0 && value >= d_valueLt)) {
        // Not inside range
        return false;  // RETURN
    }
    return true;
}

}  // close package namespace
}  // close enterprise namespace
