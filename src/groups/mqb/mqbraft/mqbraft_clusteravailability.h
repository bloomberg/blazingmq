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

// mqbraft_clusteravailability.h -*-C++-*-
#ifndef INCLUDED_MQBRAFT_CLUSTERAVAILABILITY
#define INCLUDED_MQBRAFT_CLUSTERAVAILABILITY

//@PURPOSE: Decide when a Raft cluster node and its partitions become active.
//
//@CLASSES:
//  mqbraft::ClusterAvailability: availability decisions of a Raft node.
//
//@DESCRIPTION: This component records, from events:
//
//: o the CSL leader and the CSL term ('ClusterStateRaft');
//: o whether a 'LeaderAdvisory' or 'partitionPrimaryAdvisory' of the current
//:   CSL term has been applied, which makes the CSL state caught up
//:   ('ClusterStateRaft');
//: o for each partition, the primary and the primary's leaseId (the
//:   partition's Raft term) ('ClusterOrchestrator');
//: o for each partition, the leaseId the last committed CSL advisory
//:   confirmed ('LeaderAdvisory' or 'partitionPrimaryAdvisory'), reported by
//:   'ClusterStateRaft';
//: o for each partition, whether self has applied the deferred sync point of
//:   the primary's leaseId ('ClusterOrchestrator');
//: o for each partition, whether self, after losing primaryship, has
//:   converted its queues to remote ('ClusterOrchestrator').
//
// A partition event ('onPartitionPrimary', 'onSyncPointApplied',
// 'onQueuesConvertedToRemote') returns the 'Action' for that partition.  After
// a CSL event, the caller asks 'needsAdvisory', then 'evaluatePartition' for
// each partition, then 'becomeAvailable'.
//
/// Threading
///---------
// Used only by the cluster dispatcher thread.

// BMQ
#include <bmqp_ctrlmsg_messages.h>

// BDE
#include <ball_log.h>
#include <bsl_vector.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_types.h>

namespace BloombergLP {
namespace mqbraft {

// =========================
// class ClusterAvailability
// =========================

/// Availability decisions of a Raft cluster node.
class ClusterAvailability {
  public:
    // TYPES
    typedef bsl::vector<bmqp_ctrlmsg::PartitionPrimaryInfo>
        PartitionPrimaryInfos;

    /// What self has to do for a partition.
    struct Action {
        enum Enum {
            /// Nothing.
            e_NONE,

            /// Self, as the primary, asks for the deferred sync point of the
            /// primary's leaseId.
            e_REQUEST_SYNC_POINT,

            /// Mark the partition `E_ACTIVE`.
            e_ACTIVATE,

            /// Mark the partition `E_ACTIVE`, then transition self to
            /// `E_AVAILABLE`: every partition is active.  Only a partition
            /// event returns this action.
            e_ACTIVATE_AND_BECOME_AVAILABLE
        };

        /// Return the name of the specified `value`.
        static const char* toAscii(Enum value);
    };

  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBRAFT.CLUSTERAVAILABILITY");

    // PRIVATE TYPES
    struct PartitionState {
        /// LeaseId of the primary; 0 if the partition has never had a
        /// primary.
        unsigned int d_leaseId;

        /// `true` if the partition has a primary.
        bool d_hasPrimary;

        /// `true` if self is the primary.
        bool d_isSelfPrimary;

        /// `true` once `evaluatePartition` has returned an `e_ACTIVATE`
        /// action for the current primary.
        bool d_isActive;

        /// Highest leaseId a committed CSL advisory confirmed; 0 if none.
        unsigned int d_confirmedLeaseId;

        /// Primary node of `d_confirmedLeaseId`.
        int d_confirmedNodeId;

        /// LeaseId recorded by the last `needsAdvisory` that returned `true`;
        /// 0 if none, or if self is not the CSL leader.
        unsigned int d_advisoryLeaseId;

        /// LeaseId of the deferred sync point self asked for or applied; 0
        /// if none.
        unsigned int d_syncPointLeaseId;

        /// `true` if self has applied the sync point of `d_syncPointLeaseId`,
        /// and `false` if self has only asked for that sync point.
        bool d_isSyncPointApplied;

        /// LeaseId of the last `onConversionToRemoteRequested`; 0 if none.
        unsigned int d_conversionLeaseId;

        /// `true` if self has converted its queues to remote for
        /// `d_conversionLeaseId`, and `false` if the conversion is only
        /// requested.
        bool d_isConversionDone;

        PartitionState();

        /// Return the action for this partition, the specified
        /// `partitionId`, and record the action as done
        /// (`ClusterAvailability::evaluatePartition`), given the specified
        /// `isCslCaughtUp` and `isSelfCslLeader`.  Return `e_ACTIVATE`, not
        /// `e_ACTIVATE_AND_BECOME_AVAILABLE`.
        Action::Enum
        evaluate(int partitionId, bool isCslCaughtUp, bool isSelfCslLeader);
    };

    // DATA
    bsl::vector<PartitionState> d_partitions;

    /// `true` if the CSL has a leader that is not stopping.
    bool d_hasCslLeader;

    /// `true` if self is the CSL leader.
    bool d_isSelfCslLeader;

    /// CSL term of the last `onCslLeader`.
    bsls::Types::Uint64 d_cslTerm;

    /// CSL term of the last current-term `LeaderAdvisory` or
    /// `partitionPrimaryAdvisory` applied, or 0.  Applying an entry of the
    /// current term applies every entry before it (Raft 5.4.2), so the CSL
    /// state is caught up for as long as the term stays the same.
    bsls::Types::Uint64 d_activeTerm;

    /// `true` once `becomeAvailable` has returned `true`.
    bool d_isAvailable;

    // NOT IMPLEMENTED
    ClusterAvailability(const ClusterAvailability&);
    ClusterAvailability& operator=(const ClusterAvailability&);

    // PRIVATE MANIPULATORS

    /// Return the specified `action` of a partition, with `e_ACTIVATE`
    /// replaced by `e_ACTIVATE_AND_BECOME_AVAILABLE` if `becomeAvailable`
    /// returns `true`.
    Action::Enum maybeBecomeAvailable(Action::Enum action);

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(ClusterAvailability,
                                   bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create an object for the specified `numPartitions`, using the
    /// optionally specified `allocator`.
    explicit ClusterAvailability(int               numPartitions,
                                 bslma::Allocator* allocator = 0);

    // MANIPULATORS

    /// Record the CSL leader and the CSL term: the specified `hasLeader`
    /// tells whether the CSL has a leader, the specified `isSelfLeader`
    /// whether self is the CSL leader, and the specified `term` is the CSL
    /// term.
    void
    onCslLeader(bool hasLeader, bool isSelfLeader, bsls::Types::Uint64 term);

    /// Record that the CSL leader is stopping.  The CSL state is not caught
    /// up until the next `onCslLeader`.
    void onCslLeaderStopping();

    /// Record the leaseIds of the specified `partitions` of an applied
    /// `LeaderAdvisory` or `partitionPrimaryAdvisory`, whose CSL entry has
    /// the specified `entryTerm`, with the specified `currentTerm` as the CSL
    /// term.  Ignore an entry of another term: only a current-term entry
    /// describes the current partition primaries (Raft 5.4.2), so an entry
    /// replayed at recovery is skipped.  Keep a confirmed leaseId higher than
    /// the one in `partitions`.  This is to avoid a CSL leader behind on a
    /// partition moving the confirmed leaseId back (accepting a
    /// `QueueUnassignmentRequest` from an old primary).
    void onLeaseIdsConfirmed(const PartitionPrimaryInfos& partitions,
                             bsls::Types::Uint64          entryTerm,
                             bsls::Types::Uint64          currentTerm);

    /// Record the primary of the specified `partitionId` with the specified
    /// `leaseId`; the specified `isSelf` tells whether self is the primary.
    /// The partition is not active.  Return the action for the partition
    /// (`evaluatePartition`).
    Action::Enum
    onPartitionPrimary(int partitionId, unsigned int leaseId, bool isSelf);

    /// Record that the specified `partitionId` has no primary.  The
    /// partition is not active.
    void onPartitionNoPrimary(int partitionId);

    /// Record that self has applied the deferred sync point of the specified
    /// `leaseId` on the specified `partitionId`.  Return the action for the
    /// partition (`evaluatePartition`).
    Action::Enum onSyncPointApplied(int partitionId, unsigned int leaseId);

    /// Record that self, having lost primaryship of the specified
    /// `partitionId`, has asked to convert its queues of that partition to
    /// remote, at the specified `leaseId`.  The partition does not become
    /// active until `onQueuesConvertedToRemote` with the same `leaseId`.
    void onConversionToRemoteRequested(int partitionId, unsigned int leaseId);

    /// Record that self has converted its queues of the specified
    /// `partitionId` to remote, as asked at the specified `leaseId`.  Ignore
    /// a `leaseId` other than the one of the last
    /// `onConversionToRemoteRequested`: a later request is still pending.
    /// Return the action for the partition (`evaluatePartition`).
    Action::Enum onQueuesConvertedToRemote(int          partitionId,
                                           unsigned int leaseId);

    /// Return the action for the specified `partitionId`, and record the
    /// action as done.  An inactive partition with a primary becomes
    /// active once:
    ///
    /// 1. self has converted its queues of the partition to remote, if
    ///    asked (`onConversionToRemoteRequested`).  This is to avoid
    ///    reopening those queues on the new primary before
    ///    `ClusterQueueHelper` has the counts of their handles;
    /// 2. the CSL state is caught up.  This is to avoid serving queue
    ///    reopens against a partly applied cluster state (creating
    ///    conflicting queue and app keys);
    /// 3. the CSL has confirmed the primary's leaseId;
    /// 4. if self is the primary or the CSL leader, self has applied the
    ///    deferred sync point of that leaseId.  This is to avoid serving
    ///    reopens before self has applied the entries inherited from the
    ///    prior term.
    ///
    /// Self, as the primary, asks for the sync point once per leaseId, after
    /// 1, 2 and 3.  Return `e_ACTIVATE`, not
    /// `e_ACTIVATE_AND_BECOME_AVAILABLE`: the caller asks `becomeAvailable`
    /// after evaluating every partition.
    Action::Enum evaluatePartition(int partitionId);

    /// Return `true` the first time every partition is active, and `false`
    /// otherwise.
    bool becomeAvailable();

    /// Return `true` if self, as the CSL leader, has to propose a
    /// `partitionPrimaryAdvisory`, and record the leaseIds of that advisory.
    /// Every partition needs a primary, because the advisory lists every
    /// partition.  No primary's leaseId may be lower than the confirmed
    /// leaseId: self is behind on that partition, and the advisory would
    /// publish the lower leaseId.  Then the advisory is needed if the CSL
    /// state is not
    /// caught up (the CSL leader's `LeaderAdvisory` has not committed), or
    /// if a primary's leaseId differs from the confirmed leaseId.  Return
    /// `false` if the leaseIds equal the recorded leaseIds.  This is to
    /// avoid proposing the same advisory again while that advisory is not
    /// committed.
    bool needsAdvisory();

    // ACCESSORS

    /// Return `true` if the CSL has a leader that is not stopping, and a
    /// `LeaderAdvisory` or `partitionPrimaryAdvisory` of the current CSL term
    /// has been applied.
    bool isCslCaughtUp() const;

    /// Return the leaseId of the primary of the specified `partitionId`, or
    /// 0 if the partition has no primary.
    unsigned int leaseId(int partitionId) const;

    /// Return the highest leaseId a committed CSL advisory confirmed for the
    /// specified `partitionId`, or 0 if none.
    unsigned int confirmedLeaseId(int partitionId) const;

    /// Replace the primary node and leaseId of each of the specified
    /// `partitions` whose leaseId is lower than the confirmed leaseId with
    /// the confirmed primary node and leaseId.  This is to avoid a CSL
    /// snapshot publishing a leaseId lower than the confirmed leaseId.
    void raiseToConfirmed(PartitionPrimaryInfos* partitions) const;

    /// Return `true` if the specified `partitionId` is active.
    bool isActive(int partitionId) const;
};

}  // close package namespace
}  // close enterprise namespace

#endif
