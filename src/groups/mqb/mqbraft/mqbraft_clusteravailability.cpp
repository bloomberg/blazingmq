// Copyright 2026 Bloomberg Finance L.P.
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

// mqbraft_clusteravailability.cpp -*-C++-*-
#include <mqbraft_clusteravailability.h>

// MQB
#include <mqbnet_cluster.h>

// BDE
#include <bsls_assert.h>

namespace BloombergLP {
namespace mqbraft {

// -----------------------------------
// struct ClusterAvailability::Action
// -----------------------------------

const char* ClusterAvailability::Action::toAscii(Enum value)
{
#define CASE(X)                                                               \
    case e_##X: return #X;

    switch (value) {
        CASE(NONE)
        CASE(REQUEST_SYNC_POINT)
        CASE(ACTIVATE)
        CASE(ACTIVATE_AND_BECOME_AVAILABLE)
    default: return "(* UNKNOWN *)";
    }

#undef CASE
}

// -------------------------------------------
// struct ClusterAvailability::PartitionState
// -------------------------------------------

ClusterAvailability::PartitionState::PartitionState()
: d_leaseId(0)
, d_hasPrimary(false)
, d_isSelfPrimary(false)
, d_isActive(false)
, d_confirmedLeaseId(0)
, d_confirmedNodeId(mqbnet::Cluster::k_INVALID_NODE_ID)
, d_advisoryLeaseId(0)
, d_syncPointLeaseId(0)
, d_isSyncPointApplied(false)
, d_conversionLeaseId(0)
, d_isConversionDone(false)
{
    // NOTHING
}

ClusterAvailability::Action::Enum
ClusterAvailability::PartitionState::evaluate(int  partitionId,
                                              bool isCslCaughtUp,
                                              bool isSelfCslLeader)
{
    if (d_isActive) {
        return Action::e_NONE;  // RETURN
    }

    const char* waitReason = 0;
    if (!d_hasPrimary || 0 == d_leaseId) {
        waitReason = "no leader";
    }
    else if (0 != d_conversionLeaseId && !d_isConversionDone) {
        waitReason = "queues not converted to remote";
    }
    else if (!isCslCaughtUp) {
        waitReason = "CSL not caught up";
    }
    else if (d_leaseId != d_confirmedLeaseId) {
        // On a node other than the CSL leader, the leaseId from Raft
        // leadership and the confirmed leaseId arrive in either order.
        waitReason = "leaseId not confirmed";
    }
    else if (d_isSelfPrimary && d_syncPointLeaseId != d_leaseId) {
        d_syncPointLeaseId   = d_leaseId;
        d_isSyncPointApplied = false;
        return Action::e_REQUEST_SYNC_POINT;  // RETURN
    }
    else if ((d_isSelfPrimary || isSelfCslLeader) &&
             (d_syncPointLeaseId != d_leaseId || !d_isSyncPointApplied)) {
        waitReason = "sync point not applied";
    }

    if (waitReason) {
        BALL_LOG_INFO << "Partition " << partitionId
                      << " not activated: " << waitReason
                      << " (primaryLeaseId=" << (d_hasPrimary ? d_leaseId : 0)
                      << ", confirmedLeaseId=" << d_confirmedLeaseId << ")";
        return Action::e_NONE;  // RETURN
    }

    d_isActive = true;

    return Action::e_ACTIVATE;
}

// -------------------------
// class ClusterAvailability
// -------------------------

// PRIVATE MANIPULATORS
ClusterAvailability::Action::Enum
ClusterAvailability::maybeBecomeAvailable(Action::Enum action)
{
    if (Action::e_ACTIVATE != action || !becomeAvailable()) {
        return action;  // RETURN
    }

    return Action::e_ACTIVATE_AND_BECOME_AVAILABLE;
}

// CREATORS
ClusterAvailability::ClusterAvailability(int               numPartitions,
                                         bslma::Allocator* allocator)
: d_partitions(numPartitions, PartitionState(), allocator)
, d_hasCslLeader(false)
, d_isSelfCslLeader(false)
, d_cslTerm(0)
, d_activeTerm(0)
, d_isAvailable(false)
{
    BSLS_ASSERT_SAFE(0 <= numPartitions);
}

// MANIPULATORS
void ClusterAvailability::onCslLeader(bool                hasLeader,
                                      bool                isSelfLeader,
                                      bsls::Types::Uint64 term)
{
    d_hasCslLeader    = hasLeader;
    d_isSelfCslLeader = isSelfLeader;
    d_cslTerm         = term;

    if (!isSelfLeader) {
        // A later leadership of self proposes the advisory again.
        for (bsl::size_t pid = 0; pid < d_partitions.size(); ++pid) {
            d_partitions[pid].d_advisoryLeaseId = 0;
        }
    }
}

void ClusterAvailability::onCslLeaderStopping()
{
    d_hasCslLeader = false;
}

void ClusterAvailability::onLeaseIdsConfirmed(
    const PartitionPrimaryInfos& partitions,
    bsls::Types::Uint64          entryTerm,
    bsls::Types::Uint64          currentTerm)
{
    if (entryTerm != currentTerm) {
        return;  // RETURN
    }

    d_activeTerm = currentTerm;

    for (PartitionPrimaryInfos::const_iterator it = partitions.cbegin();
         it != partitions.cend();
         ++it) {
        BSLS_ASSERT_SAFE(0 <= it->partitionId() &&
                         it->partitionId() <
                             static_cast<int>(d_partitions.size()));

        PartitionState& state = d_partitions[it->partitionId()];
        if (it->primaryLeaseId() > state.d_confirmedLeaseId) {
            state.d_confirmedLeaseId = it->primaryLeaseId();
            state.d_confirmedNodeId  = it->primaryNodeId();
        }
    }
}

ClusterAvailability::Action::Enum
ClusterAvailability::onPartitionPrimary(int          partitionId,
                                        unsigned int leaseId,
                                        bool         isSelf)
{
    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_partitions.size()));

    PartitionState& state = d_partitions[partitionId];
    state.d_leaseId       = leaseId;
    state.d_hasPrimary    = true;
    state.d_isSelfPrimary = isSelf;
    state.d_isActive      = false;

    return maybeBecomeAvailable(
        state.evaluate(partitionId, isCslCaughtUp(), d_isSelfCslLeader));
}

void ClusterAvailability::onPartitionNoPrimary(int partitionId)
{
    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_partitions.size()));

    PartitionState& state = d_partitions[partitionId];
    state.d_hasPrimary    = false;
    state.d_isSelfPrimary = false;
    state.d_isActive      = false;
}

ClusterAvailability::Action::Enum
ClusterAvailability::onSyncPointApplied(int partitionId, unsigned int leaseId)
{
    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_partitions.size()));

    PartitionState& state      = d_partitions[partitionId];
    state.d_syncPointLeaseId   = leaseId;
    state.d_isSyncPointApplied = true;

    return maybeBecomeAvailable(
        state.evaluate(partitionId, isCslCaughtUp(), d_isSelfCslLeader));
}

void ClusterAvailability::onConversionToRemoteRequested(int partitionId,
                                                        unsigned int leaseId)
{
    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_partitions.size()));
    BSLS_ASSERT_SAFE(0 != leaseId);

    PartitionState& state     = d_partitions[partitionId];
    state.d_conversionLeaseId = leaseId;
    state.d_isConversionDone  = false;
}

ClusterAvailability::Action::Enum
ClusterAvailability::onQueuesConvertedToRemote(int          partitionId,
                                               unsigned int leaseId)
{
    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_partitions.size()));

    PartitionState& state = d_partitions[partitionId];
    if (leaseId == state.d_conversionLeaseId) {
        state.d_isConversionDone = true;
    }

    return maybeBecomeAvailable(
        state.evaluate(partitionId, isCslCaughtUp(), d_isSelfCslLeader));
}

ClusterAvailability::Action::Enum
ClusterAvailability::evaluatePartition(int partitionId)
{
    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_partitions.size()));

    return d_partitions[partitionId].evaluate(partitionId,
                                              isCslCaughtUp(),
                                              d_isSelfCslLeader);
}

bool ClusterAvailability::becomeAvailable()
{
    if (d_isAvailable) {
        return false;  // RETURN
    }

    for (bsl::size_t pid = 0; pid < d_partitions.size(); ++pid) {
        if (!d_partitions[pid].d_isActive) {
            return false;  // RETURN
        }
    }

    d_isAvailable = true;

    return true;
}

bool ClusterAvailability::needsAdvisory()
{
    if (!d_isSelfCslLeader) {
        return false;  // RETURN
    }

    bool isConfirmed = true;
    bool isRecorded  = true;
    for (bsl::size_t pid = 0; pid < d_partitions.size(); ++pid) {
        const PartitionState& state = d_partitions[pid];
        if (!state.d_hasPrimary || 0 == state.d_leaseId ||
            state.d_leaseId < state.d_confirmedLeaseId) {
            return false;  // RETURN
        }
        isConfirmed = isConfirmed &&
                      state.d_leaseId == state.d_confirmedLeaseId;
        isRecorded = isRecorded && state.d_leaseId == state.d_advisoryLeaseId;
    }

    if ((isCslCaughtUp() && isConfirmed) || isRecorded) {
        return false;  // RETURN
    }

    for (bsl::size_t pid = 0; pid < d_partitions.size(); ++pid) {
        d_partitions[pid].d_advisoryLeaseId = d_partitions[pid].d_leaseId;
    }

    return true;
}

// ACCESSORS
bool ClusterAvailability::isCslCaughtUp() const
{
    return d_hasCslLeader && 0 != d_activeTerm && d_activeTerm == d_cslTerm;
}

unsigned int ClusterAvailability::leaseId(int partitionId) const
{
    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_partitions.size()));

    const PartitionState& state = d_partitions[partitionId];
    return state.d_hasPrimary ? state.d_leaseId : 0;
}

unsigned int ClusterAvailability::confirmedLeaseId(int partitionId) const
{
    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_partitions.size()));

    return d_partitions[partitionId].d_confirmedLeaseId;
}

void ClusterAvailability::raiseToConfirmed(
    PartitionPrimaryInfos* partitions) const
{
    BSLS_ASSERT_SAFE(partitions);

    for (PartitionPrimaryInfos::iterator it = partitions->begin();
         it != partitions->end();
         ++it) {
        BSLS_ASSERT_SAFE(0 <= it->partitionId() &&
                         it->partitionId() <
                             static_cast<int>(d_partitions.size()));

        const PartitionState& state = d_partitions[it->partitionId()];
        if (it->primaryLeaseId() < state.d_confirmedLeaseId) {
            it->primaryLeaseId() = state.d_confirmedLeaseId;
            it->primaryNodeId()  = state.d_confirmedNodeId;
        }
    }
}

bool ClusterAvailability::isActive(int partitionId) const
{
    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_partitions.size()));

    return d_partitions[partitionId].d_isActive;
}

}  // close package namespace
}  // close enterprise namespace
