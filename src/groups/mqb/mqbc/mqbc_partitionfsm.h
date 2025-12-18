// Copyright 2020-2023 Bloomberg Finance L.P.
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

// mqbc_partitionfsm.h                                                -*-C++-*-
#ifndef INCLUDED_MQBC_PARTITIONFSM
#define INCLUDED_MQBC_PARTITIONFSM

/// @file mqbc_partitionfsm.h
///
/// @brief Provide a finite state machine for controlling partition state.
///
/// @bbref{mqbc::PartitionFSM} is a finite state machine for controlling
/// partition state.
///
/// Thread Safety                                   {#mqbc_partitionfsm_thread}
/// =============
///
/// The @bbref{mqbc::PartitionFSM} object is not thread safe.

// MQB
#include <mqbc_partitionfsmobserver.h>
#include <mqbc_partitionstatetable.h>
#include <mqbs_datastore.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>

// BDE
#include <ball_log.h>
#include <bdlbb_blob.h>
#include <bsl_memory.h>
#include <bsl_queue.h>
#include <bsl_unordered_set.h>
#include <bsl_utility.h>
#include <bsl_vector.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace mqbnet {
class ClusterNode;
}

namespace mqbc {

// ===========================
// class PartitionFSMEventData
// ===========================

/// This class defines VST for data of a Partition FSM event.
class PartitionFSMEventData {
  public:
    // TYPES
    typedef bsl::pair<bmqp_ctrlmsg::PartitionSequenceNumber,
                      bmqp_ctrlmsg::PartitionSequenceNumber>
        PartitionSeqNumDataRange;

  private:
    // DATA

    /// The source cluster node from which this event was originated from.
    mqbnet::ClusterNode* d_source_p;

    /// Associated request Id.
    int d_requestId;

    /// Associated partition Id.
    int d_partitionId;

    /// Increment count (for counting number of replica data responses).
    int d_incrementCount;

    /// The primary for the associated partitionId.
    mqbnet::ClusterNode* d_primary_p;

    /// The primary lease id for the associated partitionId.
    unsigned int d_primaryLeaseId;

    /// Partition sequence number as sent by the `d_source_p` node for the
    /// associated partitionId.
    bmqp_ctrlmsg::PartitionSequenceNumber d_partitionSequenceNumber;

    /// Sequence number of the first sync point after rollover as sent by
    /// the `d_source_p` node for the associated partitionId.
    bmqp_ctrlmsg::PartitionSequenceNumber
        d_firstSyncPointAfterRolloverSequenceNumber;

    /// The node which has the highest sequence number for the associated
    /// partitionId.
    mqbnet::ClusterNode* d_highestSeqNumNode;

    /// Partition Sequence number range as sent and expected when the data
    /// exchange takes place.
    PartitionSeqNumDataRange d_partitionSeqNumDataRange;

    /// The StorageEvent received, consisting of data chunks.
    bsl::shared_ptr<bdlbb::Blob> d_storageEvent;

  public:
    // CREATORS

    /// Create a default instance
    PartitionFSMEventData();

    /// Create an instance using the specified `source` with the specified
    /// `requestId`, specified `partitionId` and specified 'incrementCount'.
    /// Optionally specify the `primary` and `primaryLeaseId`. If applicable
    /// the associated data sequence number is an optionally specified
    /// `seqNum` and optionally specified `firstSyncPointAfterRollloverSeqNum`.
    /// There are also optionally specified `highestSeqNumNode`,
    /// optionally specified `seqNumDataRange`.
    PartitionFSMEventData(mqbnet::ClusterNode* source,
                          int                  requestId,
                          int                  partitionId,
                          int                  incrementCount,
                          mqbnet::ClusterNode* primary        = 0,
                          unsigned int         primaryLeaseId = 0,
                          const bmqp_ctrlmsg::PartitionSequenceNumber& seqNum =
                              bmqp_ctrlmsg::PartitionSequenceNumber(),
                          const bmqp_ctrlmsg::PartitionSequenceNumber&
                              firstSyncPointAfterRollloverSeqNum =
                                  bmqp_ctrlmsg::PartitionSequenceNumber(),
                          mqbnet::ClusterNode* highestSeqNumNode = 0,
                          const PartitionSeqNumDataRange& seqNumDataRange =
                              PartitionSeqNumDataRange());

    /// Create an instance of PartitionFSMEventData using the specified
    /// `source` where request id is the specified `requestId`, the
    /// partition identifier is the specified `partitionId`, the specified
    /// 'incrementCount', the associated data sequence number is the specified
    /// `seqNum`, the first sync point after rollover sequence number is the
    /// specified `firstSyncPointAfterRollloverSeqNum`.
    PartitionFSMEventData(mqbnet::ClusterNode* source,
                          int                  requestId,
                          int                  partitionId,
                          int                  incrementCount,
                          const bmqp_ctrlmsg::PartitionSequenceNumber& seqNum,
                          const bmqp_ctrlmsg::PartitionSequenceNumber&
                              firstSyncPointAfterRollloverSeqNum);

    /// Create an instance of PartitionFSMEventData using the specified
    /// `source` where request id is the specified `requestId`, partition
    /// identifier is the specified `partitionId`, the data range is associated
    /// specified `seqNumDataRange`, and the specified 'incrementCount'.
    PartitionFSMEventData(mqbnet::ClusterNode*            source,
                          int                             requestId,
                          int                             partitionId,
                          int                             incrementCount,
                          const PartitionSeqNumDataRange& seqNumDataRange);

    /// Create an instance of PartitionFSMEventData using the specified
    /// `source`, `partitionId`, 'incrementCount' and `storageEvent`.
    PartitionFSMEventData(mqbnet::ClusterNode*                source,
                          int                                 partitionId,
                          int                                 incrementCount,
                          const bsl::shared_ptr<bdlbb::Blob>& storageEvent);

    // ACCESSORS
    mqbnet::ClusterNode* source() const;
    mqbnet::ClusterNode* primary() const;
    unsigned int         primaryLeaseId() const;
    int                  requestId() const;
    int                  partitionId() const;
    int                  incrementCount() const;
    const bmqp_ctrlmsg::PartitionSequenceNumber&
    partitionSequenceNumber() const;
    const bmqp_ctrlmsg::PartitionSequenceNumber&
                         firstSyncPointAfterRolloverSequenceNumber() const;
    mqbnet::ClusterNode* highestSeqNumNode() const;
    const PartitionSeqNumDataRange& partitionSeqNumDataRange() const;

    /// Return the value of the corresponding member of this object
    const bsl::shared_ptr<bdlbb::Blob>& storageEvent() const;
};

// ==================
// class PartitionFSM
// ==================

/// This class provides a finite state machine for controlling partition
/// state.
class PartitionFSM {
  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBC.PARTITIONFSM");

  public:
    // TYPES
    typedef bsl::pair<PartitionStateTableEvent::Enum,
                      bsl::vector<PartitionFSMEventData> >
        EventWithData;

    struct PartitionFSMArgs {
      private:
        // DATA

        /// Queue containing events which need to be applied to the current
        /// state of the FSM.
        bsl::queue<EventWithData>* d_eventsQueue_p;

      public:
        // CREATORS

        /// Create an instance with the specified `queue`.
        PartitionFSMArgs(bsl::queue<EventWithData>* queue);

        // ACCESSORS

        /// Return the value of the corresponding member of this object
        bsl::queue<EventWithData>* eventsQueue();
    };

    typedef bsl::shared_ptr<PartitionFSMArgs>       PartitionFSMArgsSp;
    typedef PartitionStateTable<PartitionFSMArgsSp> StateTable;
    typedef StateTable::State                       State;
    typedef StateTable::Event                       Event;
    typedef StateTable::ActionFunctor               ActionFunctor;
    typedef StateTable::Transition                  Transition;

    /// A set of PartitionFSM observers.
    typedef bsl::unordered_set<PartitionFSMObserver*> ObserversSet;
    typedef ObserversSet::iterator                    ObserversSetIter;

  private:
    // DATA
    bslma::Allocator* d_allocator_p;

    StateTable d_stateTable;

    State::Enum d_state;

    PartitionStateTableActions<PartitionFSMArgsSp>& d_actions;

    /// Observers of this object.
    ObserversSet d_observers;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(PartitionFSM, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create an instance with the specified `actions`, using the specified
    /// `allocator`.
    PartitionFSM(PartitionStateTableActions<PartitionFSMArgsSp>& actions,
                 bslma::Allocator*                               allocator);

    // MANIPULATORS

    /// Register the specified `observer` to be notified of partition FSM
    /// changes.  Return a reference offerring modifiable access to this
    /// object.
    PartitionFSM& registerObserver(PartitionFSMObserver* observer);

    /// Unregister the specified `observer` to be notified of partition FSM
    /// changes.  Return a reference offerring modifiable access to this
    /// object.
    PartitionFSM& unregisterObserver(PartitionFSMObserver* observer);

    /// While the specified `eventsQueue` is not empty, pop the event from the
    /// head of the queue and process it as an input to the FSM.  During the
    /// processing, new events might be enqueued to the end of `eventsQueue`.
    void popEventAndProcess(
        const bsl::shared_ptr<bsl::queue<EventWithData> >& eventsQueue);

    // ACCESSORS

    /// Return the current partition state.
    State::Enum state() const;

    /// Return true if self node is the primary, false otherwise.
    bool isSelfPrimary() const;

    /// Return true if self node is a replica, false otherwise.
    bool isSelfReplica() const;

    /// Return true if self node is healed, false otherwise.
    bool isSelfHealed() const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ---------------------------
// class PartitionFSMEventData
// ---------------------------

// CREATORS
inline PartitionFSMEventData::PartitionFSMEventData()
: d_source_p(0)
, d_requestId(-1)  // Invalid requestId
, d_partitionId(mqbi::Storage::k_INVALID_PARTITION_ID)
, d_incrementCount(1)
, d_primary_p(0)
, d_primaryLeaseId(0)  // Invalid placeholder LeaseId
, d_partitionSequenceNumber()
, d_firstSyncPointAfterRolloverSequenceNumber()
, d_highestSeqNumNode(0)
, d_partitionSeqNumDataRange()
, d_storageEvent()
{
    // NOTHING
}

inline PartitionFSMEventData::PartitionFSMEventData(
    mqbnet::ClusterNode*                         source,
    int                                          requestId,
    int                                          partitionId,
    int                                          incrementCount,
    mqbnet::ClusterNode*                         primary,
    unsigned int                                 primaryLeaseId,
    const bmqp_ctrlmsg::PartitionSequenceNumber& seqNum,
    const bmqp_ctrlmsg::PartitionSequenceNumber&
                                    firstSyncPointAfterRollloverSeqNum,
    mqbnet::ClusterNode*            highestSeqNumNode,
    const PartitionSeqNumDataRange& seqNumDataRange)
: d_source_p(source)
, d_requestId(requestId)
, d_partitionId(partitionId)
, d_incrementCount(incrementCount)
, d_primary_p(primary)
, d_primaryLeaseId(primaryLeaseId)
, d_partitionSequenceNumber(seqNum)
, d_firstSyncPointAfterRolloverSequenceNumber(
      firstSyncPointAfterRollloverSeqNum)
, d_highestSeqNumNode(highestSeqNumNode)
, d_partitionSeqNumDataRange(seqNumDataRange)
, d_storageEvent()
{
    // NOTHING
}

inline PartitionFSMEventData::PartitionFSMEventData(
    mqbnet::ClusterNode*                         source,
    int                                          requestId,
    int                                          partitionId,
    int                                          incrementCount,
    const bmqp_ctrlmsg::PartitionSequenceNumber& seqNum,
    const bmqp_ctrlmsg::PartitionSequenceNumber&
        firstSyncPointAfterRollloverSeqNum)
: d_source_p(source)
, d_requestId(requestId)
, d_partitionId(partitionId)
, d_incrementCount(incrementCount)
, d_primary_p(0)
, d_primaryLeaseId(0)  // Invalid placeholder primaryLeaseId
, d_partitionSequenceNumber(seqNum)
, d_firstSyncPointAfterRolloverSequenceNumber(
      firstSyncPointAfterRollloverSeqNum)
, d_highestSeqNumNode(0)
, d_partitionSeqNumDataRange()
, d_storageEvent()
{
    // NOTHING
}

inline PartitionFSMEventData::PartitionFSMEventData(
    mqbnet::ClusterNode*            source,
    int                             requestId,
    int                             partitionId,
    int                             incrementCount,
    const PartitionSeqNumDataRange& seqNumDataRange)
: d_source_p(source)
, d_requestId(requestId)
, d_partitionId(partitionId)
, d_incrementCount(incrementCount)
, d_primary_p(0)
, d_primaryLeaseId(0)  // Invalid placeholder primaryLeaseId
, d_partitionSequenceNumber()
, d_firstSyncPointAfterRolloverSequenceNumber()
, d_highestSeqNumNode(0)
, d_partitionSeqNumDataRange(seqNumDataRange)
, d_storageEvent()
{
    // NOTHING
}

inline PartitionFSMEventData::PartitionFSMEventData(
    mqbnet::ClusterNode*                source,
    int                                 partitionId,
    int                                 incrementCount,
    const bsl::shared_ptr<bdlbb::Blob>& storageEvent)
: d_source_p(source)
, d_requestId(-1)  // Invalid requestId
, d_partitionId(partitionId)
, d_incrementCount(incrementCount)
, d_primary_p(0)
, d_primaryLeaseId(0)  // Invalid placeholder primaryLeaseId
, d_partitionSequenceNumber()
, d_firstSyncPointAfterRolloverSequenceNumber()
, d_highestSeqNumNode(0)
, d_partitionSeqNumDataRange()
, d_storageEvent(storageEvent)
{
    // NOTHING
}

// ACCESSORS
inline mqbnet::ClusterNode* PartitionFSMEventData::source() const
{
    return d_source_p;
}

inline mqbnet::ClusterNode* PartitionFSMEventData::primary() const
{
    return d_primary_p;
}

inline unsigned int PartitionFSMEventData::primaryLeaseId() const
{
    return d_primaryLeaseId;
}

inline int PartitionFSMEventData::requestId() const
{
    return d_requestId;
}

inline int PartitionFSMEventData::partitionId() const
{
    return d_partitionId;
}

inline int PartitionFSMEventData::incrementCount() const
{
    return d_incrementCount;
}

inline const bmqp_ctrlmsg::PartitionSequenceNumber&
PartitionFSMEventData::partitionSequenceNumber() const
{
    return d_partitionSequenceNumber;
}

inline const bmqp_ctrlmsg::PartitionSequenceNumber&
PartitionFSMEventData::firstSyncPointAfterRolloverSequenceNumber() const
{
    return d_firstSyncPointAfterRolloverSequenceNumber;
}

inline mqbnet::ClusterNode* PartitionFSMEventData::highestSeqNumNode() const
{
    return d_highestSeqNumNode;
}

inline const PartitionFSMEventData::PartitionSeqNumDataRange&
PartitionFSMEventData::partitionSeqNumDataRange() const
{
    return d_partitionSeqNumDataRange;
}

inline const bsl::shared_ptr<bdlbb::Blob>&
PartitionFSMEventData::storageEvent() const
{
    return d_storageEvent;
}

// ------------------------------------
// class PartitionFSM::PartitionFSMArgs
// ------------------------------------
// CREATORS
inline PartitionFSM::PartitionFSMArgs::PartitionFSMArgs(
    bsl::queue<EventWithData>* queue)
: d_eventsQueue_p(queue)
{
    // NOTHING
}

// ACCESSORS
inline bsl::queue<PartitionFSM::EventWithData>*
PartitionFSM::PartitionFSMArgs::eventsQueue()
{
    return d_eventsQueue_p;
}

// ------------------
// class PartitionFSM
// ------------------

// CREATORS
inline PartitionFSM::PartitionFSM(
    PartitionStateTableActions<PartitionFSMArgsSp>& actions,
    bslma::Allocator*                               allocator)
: d_allocator_p(allocator)
, d_stateTable()
, d_state(State::e_UNKNOWN)
, d_actions(actions)
, d_observers(allocator)
{
    // NOTHING
}

inline PartitionFSM::State::Enum PartitionFSM::state() const
{
    return d_state;
}

inline bool PartitionFSM::isSelfPrimary() const
{
    return d_state == State::e_PRIMARY_HEALING_STG1 ||
           d_state == State::e_PRIMARY_HEALING_STG2 ||
           d_state == State::e_PRIMARY_HEALED;
}

inline bool PartitionFSM::isSelfReplica() const
{
    return d_state == State::e_REPLICA_HEALING ||
           d_state == State::e_REPLICA_HEALED;
}

inline bool PartitionFSM::isSelfHealed() const
{
    return d_state == State::e_PRIMARY_HEALED ||
           d_state == State::e_REPLICA_HEALED;
}

}  // close package namespace
}  // close enterprise namespace

#endif
