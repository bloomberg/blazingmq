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

namespace {

using namespace bmqp_ctrlmsg;
typedef bsl::vector<QueueInfo> QueueInfos;

// Helper method to check if queue key from QueueInfo matches keys in
// queueKeys.
bool isQueueKeyMatch(const QueueInfos&                           queuesInfo,
                     const bsl::unordered_set<mqbu::StorageKey>& queueKeys)
{
    bool queueKeyMatch = false;

    QueueInfos::const_iterator it = queuesInfo.cbegin();
    for (; it != queuesInfo.cend(); ++it) {
        mqbu::StorageKey key(mqbu::StorageKey::BinaryRepresentation(),
                             it->key().data());
        // Match by queueKey
        if (queueKeys.find(key) != queueKeys.end()) {
            queueKeyMatch = true;
        }
    }

    return queueKeyMatch;
}

// Helper method to apply range filter
bool applyRangeFilter(const Parameters::Range&       range,
                      const bsls::Types::Uint64      timestamp,
                      const bsls::Types::Uint64      offset,
                      const CompositeSequenceNumber& compositeSequenceNumber,
                      bool*                          highBoundReached_p)
{
    bsls::Types::Uint64 value, valueGt, valueLt;
    switch (range.d_type) {
    case Parameters::Range::e_TIMESTAMP:
        value   = timestamp;
        valueGt = range.d_timestampGt;
        valueLt = range.d_timestampLt;
        break;
    case Parameters::Range::e_OFFSET:
        value   = offset;
        valueGt = range.d_offsetGt;
        valueLt = range.d_offsetLt;
        break;
    case Parameters::Range::e_SEQUENCE_NUM: {
        const bool greaterOrEqualToHigherBound = range.d_seqNumLt.isSet() &&
                                                 range.d_seqNumLt <=
                                                     compositeSequenceNumber;
        if (highBoundReached_p && greaterOrEqualToHigherBound) {
            *highBoundReached_p = true;
        }

        return !((range.d_seqNumGt.isSet() &&
                  compositeSequenceNumber <= range.d_seqNumGt) ||
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

}  // close unnamed namespace

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
    return applyRangeFilter(
        d_range,
        recordHeader.timestamp(),
        recordOffset,
        CompositeSequenceNumber(recordHeader.primaryLeaseId(),
                                recordHeader.sequenceNumber()),
        highBoundReached_p);
}

bool Filters::apply(const mqbc::ClusterStateRecordHeader& recordHeader,
                    const bmqp_ctrlmsg::ClusterMessage&   record,
                    bsls::Types::Uint64                   recordOffset,
                    bool* highBoundReached_p) const
{
    // Apply `queue key` filter
    if (!d_queueKeys.empty()) {
        using namespace bmqp_ctrlmsg;
        bool queueKeyMatch = false;

        if (recordHeader.recordType() ==
            mqbc::ClusterStateRecordType::e_SNAPSHOT) {
            const LeaderAdvisory& leaderAdvisory =
                record.choice().leaderAdvisory();
            queueKeyMatch = isQueueKeyMatch(leaderAdvisory.queues(),
                                            d_queueKeys);
        }
        else if (recordHeader.recordType() ==
                     mqbc::ClusterStateRecordType::e_UPDATE) {
            if (record.choice().selectionId() ==
                     ClusterMessageChoice::
                         SELECTION_ID_QUEUE_ASSIGNMENT_ADVISORY) {
                const QueueAssignmentAdvisory& queueAdvisory =
                    record.choice().queueAssignmentAdvisory();
                queueKeyMatch = isQueueKeyMatch(queueAdvisory.queues(),
                                                d_queueKeys);
            }
        }

        if (!queueKeyMatch) {
            // Not matched
            return false;  // RETURN
        }
    }

    // Apply `range` filter
    return applyRangeFilter(
        d_range,
        recordHeader.timestamp(),
        recordOffset,
        CompositeSequenceNumber(recordHeader.electorTerm(),
                                recordHeader.sequenceNumber()),
        highBoundReached_p);
}

}  // close package namespace
}  // close enterprise namespace
