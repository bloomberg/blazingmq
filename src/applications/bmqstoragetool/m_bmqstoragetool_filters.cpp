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
#include <bsl_algorithm.h>
#include <bsl_vector.h>
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
    QueueInfos::const_iterator it = queuesInfo.cbegin();
    for (; it != queuesInfo.cend(); ++it) {
        mqbu::StorageKey key(mqbu::StorageKey::BinaryRepresentation(),
                             it->key().data());
        // Match by queueKey
        if (queueKeys.find(key) != queueKeys.end()) {
            return true;  // RETURN
        }
    }

    return false;
}

// Helper method to apply range filter
bool applyRangeFilter(const Parameters::Range& range,
                      bsls::Types::Uint64      timestamp,
                      bsls::Types::Uint64      offset,
                      bsls::Types::Uint64      primaryLeaseId,
                      bsls::Types::Uint64      sequenceNumber,
                      bool*                    highBoundReached_p)
{
    // Check timestamp range
    bool greaterOrEqualToHigherBound = range.d_timestampLt &&
                                       timestamp >= *range.d_timestampLt;
    if ((range.d_timestampGt && timestamp <= *range.d_timestampGt) ||
        greaterOrEqualToHigherBound) {
        if (highBoundReached_p && greaterOrEqualToHigherBound) {
            *highBoundReached_p = true;
        }
        // Not inside range
        return false;  // RETURN
    }

    // Check offset range
    greaterOrEqualToHigherBound = range.d_offsetLt &&
                                  offset >= *range.d_offsetLt;
    if ((range.d_offsetGt && offset <= *range.d_offsetGt) ||
        greaterOrEqualToHigherBound) {
        if (highBoundReached_p && greaterOrEqualToHigherBound) {
            *highBoundReached_p = true;
        }
        // Not inside range
        return false;  // RETURN
    }

    // Check sequence number range
    if (range.d_seqNumLt || range.d_seqNumGt) {
        const CompositeSequenceNumber seqNum(primaryLeaseId, sequenceNumber);
        greaterOrEqualToHigherBound = range.d_seqNumLt &&
                                      range.d_seqNumLt <= seqNum;
        if ((range.d_seqNumGt && seqNum <= *range.d_seqNumGt) ||
            greaterOrEqualToHigherBound) {
            if (highBoundReached_p && greaterOrEqualToHigherBound) {
                *highBoundReached_p = true;
            }
            // Not inside range
            return false;  // RETURN
        }
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
            d_queueKeys.emplace(
                mqbu::StorageKey(mqbu::StorageKey::HexRepresentation(),
                                 keyIt->c_str()));
        }
    }
    if (!queueUris.empty()) {
        bsl::vector<bsl::string>::const_iterator uriIt = queueUris.cbegin();
        for (; uriIt != queueUris.cend(); ++uriIt) {
            bsl::optional<mqbu::StorageKey> key = queueMap.findKeyByUri(
                *uriIt);
            if (key.has_value()) {
                d_queueKeys.insert(key.value());
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
    return applyRangeFilter(d_range,
                            recordHeader.timestamp(),
                            recordOffset,
                            recordHeader.primaryLeaseId(),
                            recordHeader.sequenceNumber(),
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

        mqbc::ClusterStateRecordType::Enum recordType =
            recordHeader.recordType();
        if (recordType == mqbc::ClusterStateRecordType::e_SNAPSHOT) {
            const LeaderAdvisory& leaderAdvisory =
                record.choice().leaderAdvisory();
            queueKeyMatch = isQueueKeyMatch(leaderAdvisory.queues(),
                                            d_queueKeys);
        }
        else if (recordType == mqbc::ClusterStateRecordType::e_UPDATE) {
            int selectionId = record.choice().selectionId();
            if (selectionId ==
                ClusterMessageChoice::SELECTION_ID_QUEUE_ASSIGNMENT_ADVISORY) {
                const QueueAssignmentAdvisory& queueAdvisory =
                    record.choice().queueAssignmentAdvisory();
                queueKeyMatch = isQueueKeyMatch(queueAdvisory.queues(),
                                                d_queueKeys);
            }
            else if (selectionId ==
                     ClusterMessageChoice::
                         SELECTION_ID_QUEUE_UNASSIGNED_ADVISORY) {
                const QueueUnassignedAdvisory& queueAdvisory =
                    record.choice().queueUnassignedAdvisory();
                queueKeyMatch = isQueueKeyMatch(queueAdvisory.queues(),
                                                d_queueKeys);
            }
            else if (selectionId ==
                     ClusterMessageChoice::
                         SELECTION_ID_QUEUE_UN_ASSIGNMENT_ADVISORY) {
                const QueueUnAssignmentAdvisory& queueAdvisory =
                    record.choice().queueUnAssignmentAdvisory();
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
    return applyRangeFilter(d_range,
                            recordHeader.timestamp(),
                            recordOffset,
                            recordHeader.electorTerm(),
                            recordHeader.sequenceNumber(),
                            highBoundReached_p);
}

}  // close package namespace
}  // close enterprise namespace
