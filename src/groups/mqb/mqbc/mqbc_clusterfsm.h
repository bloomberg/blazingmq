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

// mqbc_clusterfsm.h                                                  -*-C++-*-
#ifndef INCLUDED_MQBC_CLUSTERFSM
#define INCLUDED_MQBC_CLUSTERFSM

/// @file mqbc_clusterfsm.h
///
/// @brief Provide a finite state machine for controlling cluster state.
///
/// @bbref{mqbc::ClusterFSM} is a finite state machine for controlling cluster
/// state.
///
/// Thread Safety                                     {#mqbc_clusterfsm_thread}
/// =============
///
/// The @bbref{mqbc::ClusterFSM} object is not thread safe.

// MQB
#include <mqbc_clusterstatetable.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>

// BDE
#include <ball_log.h>
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

// ========================
// class ClusterFSMObserver
// ========================

/// This interface defines an observer of the Cluster FSM.
class ClusterFSMObserver {
  public:
    // CREATORS

    /// Destructor
    virtual ~ClusterFSMObserver();

    // MANIPULATORS

    /// Invoked when the Cluster FSM transitions to `UNKNOWN` state.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    virtual void onUnknown();

    /// Invoked when the Cluster FSM transitions to `LDR_HEALED` state.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    virtual void onHealedLeader();

    /// Invoked when the Cluster FSM transitions to `FOL_HEALED` state.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    virtual void onHealedFollower();

    /// Invoked when the Cluster FSM transitions to `STOPPING` state.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    virtual void onStopping();
};

// =============================
// class ClusterFSMEventMetadata
// =============================

/// This class defines a VST for the metadata of a Cluster FSM event.
class ClusterFSMEventMetadata {
  public:
    // TYPES
    struct InputMessage;

    typedef bsl::vector<InputMessage>     InputMessages;
    typedef InputMessages::iterator       InputMessagesIter;
    typedef InputMessages::const_iterator InputMessagesCIter;

    /// This struct defines an input cluster message which triggers the
    /// Cluster FSM event.
    struct InputMessage {
      private:
        // DATA

        /// Source of the message.
        mqbnet::ClusterNode* d_source;

        /// Id of the associated request, if any.
        int d_requestId;

        /// Associated LSN.
        bmqp_ctrlmsg::LeaderMessageSequence d_leaderSequenceNumber;

      public:
        // CREATORS

        /// Create a default instance.
        InputMessage();

        // MANIPULATORS

        /// Set this object's source to the specified `value`.
        InputMessage& setSource(mqbnet::ClusterNode* value);

        /// Set this object's request id to the specified `value`.
        InputMessage& setRequestId(int value);

        /// Set this object's LSN to the specified `value`.
        InputMessage& setLeaderSequenceNumber(
            const bmqp_ctrlmsg::LeaderMessageSequence& value);

        // ACCESSORS
        mqbnet::ClusterNode* source() const;
        int                  requestId() const;

        /// Return the value of the corresponding member of this object.
        const bmqp_ctrlmsg::LeaderMessageSequence&
        leaderSequenceNumber() const;
    };

  private:
    // DATA

    /// Input cluster messages which triggers this Cluster FSM event.
    InputMessages d_messages;

    /// Node with the highest LSN.
    mqbnet::ClusterNode* d_highestLSNNode;

    /// Follower node who crashed, if any.
    mqbnet::ClusterNode* d_crashedFollowerNode;

    /// Cluster state snapshot.
    bmqp_ctrlmsg::LeaderAdvisory d_clusterStateSnapshot;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(ClusterFSMEventMetadata,
                                   bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a default instance, using the optionally specified
    /// `allocator`.
    explicit ClusterFSMEventMetadata(bslma::Allocator* allocator = 0);

    /// Create a new instance of @bbref{mqbc::ClusterFSMEventMetadata} with the
    /// specified `inputMessages`, and optionally specified `highestLSNNode`,
    /// `crashedFollowerNode` and `clusterStateSnapshot`.
    explicit ClusterFSMEventMetadata(
        const InputMessages&                inputMessages,
        mqbnet::ClusterNode*                highestLSNNode      = 0,
        mqbnet::ClusterNode*                crashedFollowerNode = 0,
        const bmqp_ctrlmsg::LeaderAdvisory& clusterStateSnapshot =
            bmqp_ctrlmsg::LeaderAdvisory());

    /// Create a new instance of @bbref{mqbc::ClusterFSMEventMetadata} with the
    /// specified `highestLSNNode`, and optionally specified
    /// `crashedFollowerNode`, `clusterStateSnapshot` and `allocator`.
    explicit ClusterFSMEventMetadata(
        mqbnet::ClusterNode*                highestLSNNode,
        mqbnet::ClusterNode*                crashedFollowerNode = 0,
        const bmqp_ctrlmsg::LeaderAdvisory& clusterStateSnapshot =
            bmqp_ctrlmsg::LeaderAdvisory(),
        bslma::Allocator* allocator = 0);

    /// Create a new instance copying from the specified `rhs`.  Use the
    /// optionally specified `allocator` for memory allocations.
    ClusterFSMEventMetadata(const ClusterFSMEventMetadata& rhs,
                            bslma::Allocator*              allocator = 0);

    // ACCESSORS
    const InputMessages& inputMessages() const;
    mqbnet::ClusterNode* highestLSNNode() const;
    mqbnet::ClusterNode* crashedFollowerNode() const;

    /// Return the value of the corresponding member of this object.
    const bmqp_ctrlmsg::LeaderAdvisory& clusterStateSnapshot() const;
};

// ================
// class ClusterFSM
// ================

/// This class provides a finite state machine for controlling cluster
/// state.
class ClusterFSM {
  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBC.CLUSTERFSM");

  public:
    // TYPES
    typedef bsl::pair<ClusterStateTableEvent::Enum, ClusterFSMEventMetadata>
                                                EventWithMetadata;
    typedef bsl::queue<EventWithMetadata>       ClusterFSMArgs;
    typedef bsl::shared_ptr<ClusterFSMArgs>     ClusterFSMArgsSp;
    typedef ClusterStateTable<ClusterFSMArgsSp> StateTable;
    typedef StateTable::State                   State;
    typedef StateTable::Event                   Event;
    typedef StateTable::ActionFunctor           ActionFunctor;
    typedef StateTable::Transition              Transition;

    /// A set of ClusterFSM observers.
    typedef bsl::unordered_set<ClusterFSMObserver*> ObserversSet;
    typedef ObserversSet::iterator                  ObserversSetIter;

  private:
    // DATA

    /// State table of FSM transitions.
    StateTable d_stateTable;

    /// Current state of this FSM.
    State::Enum d_state;

    /// Actions to perform as part of FSM transitions.
    ClusterStateTableActions<ClusterFSMArgsSp>& d_actions;

    /// Observers of this object.
    ObserversSet d_observers;

  public:
    // CREATORS

    /// Create an instance with the specified `actions`.
    ClusterFSM(ClusterStateTableActions<ClusterFSMArgsSp>& actions);

    // MANIPULATORS

    /// Register the specified `observer` to be notified of cluster FSM
    /// changes.  Return a reference offerring modifiable access to this
    /// object.
    ClusterFSM& registerObserver(ClusterFSMObserver* observer);

    /// Unregister the specified `observer` to be notified of cluster FSM
    /// changes.  Return a reference offerring modifiable access to this
    /// object.
    ClusterFSM& unregisterObserver(ClusterFSMObserver* observer);

    /// While the specified `eventsQueue` is not empty, pop the event from the
    /// head of the queue and process it as an input to the FSM.  During the
    /// processing, new events might be enqueued to the end of `eventsQueue`.
    void popEventAndProcess(ClusterFSMArgsSp& eventsQueue);

    // ACCESSORS

    /// Return the current cluster state.
    State::Enum state() const;

    /// Return true if self node is the leader, false otherwise.
    bool isSelfLeader() const;

    /// Return true if self node is a follower, false otherwise.
    bool isSelfFollower() const;

    /// Return true if self node is healed, false otherwise.
    bool isSelfHealed() const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -------------------------------------------
// class ClusterFSMEventMetadata::InputMessage
// -------------------------------------------

// CREATORS
inline ClusterFSMEventMetadata::InputMessage::InputMessage()
: d_source(0)
, d_requestId(-1)  // Invalid Request Id
, d_leaderSequenceNumber()
{
    // NOTHING
}

// MANIPULATORS
inline ClusterFSMEventMetadata::InputMessage&
ClusterFSMEventMetadata::InputMessage::setSource(mqbnet::ClusterNode* value)
{
    d_source = value;

    return *this;
}

inline ClusterFSMEventMetadata::InputMessage&
ClusterFSMEventMetadata::InputMessage::setRequestId(int value)
{
    d_requestId = value;

    return *this;
}

inline ClusterFSMEventMetadata::InputMessage&
ClusterFSMEventMetadata::InputMessage::setLeaderSequenceNumber(
    const bmqp_ctrlmsg::LeaderMessageSequence& value)
{
    d_leaderSequenceNumber = value;

    return *this;
}

// ACCESSORS
inline mqbnet::ClusterNode*
ClusterFSMEventMetadata::InputMessage::source() const
{
    return d_source;
}

inline int ClusterFSMEventMetadata::InputMessage::requestId() const
{
    return d_requestId;
}

inline const bmqp_ctrlmsg::LeaderMessageSequence&
ClusterFSMEventMetadata::InputMessage::leaderSequenceNumber() const
{
    return d_leaderSequenceNumber;
}

// -----------------------------
// class ClusterFSMEventMetadata
// -----------------------------

// CREATORS
inline ClusterFSMEventMetadata::ClusterFSMEventMetadata(
    bslma::Allocator* allocator)
: d_messages(allocator)
, d_highestLSNNode(0)
, d_crashedFollowerNode(0)
, d_clusterStateSnapshot()
{
    // NOTHING
}

inline ClusterFSMEventMetadata::ClusterFSMEventMetadata(
    const InputMessages&                inputMessages,
    mqbnet::ClusterNode*                highestLSNNode,
    mqbnet::ClusterNode*                crashedFollowerNode,
    const bmqp_ctrlmsg::LeaderAdvisory& clusterStateSnapshot)
: d_messages(inputMessages)
, d_highestLSNNode(highestLSNNode)
, d_crashedFollowerNode(crashedFollowerNode)
, d_clusterStateSnapshot(clusterStateSnapshot)
{
    // NOTHING
}

inline ClusterFSMEventMetadata::ClusterFSMEventMetadata(
    mqbnet::ClusterNode*                highestLSNNode,
    mqbnet::ClusterNode*                crashedFollowerNode,
    const bmqp_ctrlmsg::LeaderAdvisory& clusterStateSnapshot,
    bslma::Allocator*                   allocator)
: d_messages(allocator)
, d_highestLSNNode(highestLSNNode)
, d_crashedFollowerNode(crashedFollowerNode)
, d_clusterStateSnapshot(clusterStateSnapshot)
{
    // NOTHING
}

inline ClusterFSMEventMetadata::ClusterFSMEventMetadata(
    const ClusterFSMEventMetadata& rhs,
    bslma::Allocator*              allocator)
: d_messages(rhs.inputMessages(), allocator)
, d_highestLSNNode(rhs.highestLSNNode())
, d_crashedFollowerNode(rhs.crashedFollowerNode())
, d_clusterStateSnapshot(rhs.clusterStateSnapshot())
{
    // NOTHING
}

// ACCESSORS
inline const ClusterFSMEventMetadata::InputMessages&
ClusterFSMEventMetadata::inputMessages() const
{
    return d_messages;
}

inline mqbnet::ClusterNode* ClusterFSMEventMetadata::highestLSNNode() const
{
    return d_highestLSNNode;
}

inline mqbnet::ClusterNode*
ClusterFSMEventMetadata::crashedFollowerNode() const
{
    return d_crashedFollowerNode;
}

inline const bmqp_ctrlmsg::LeaderAdvisory&
ClusterFSMEventMetadata::clusterStateSnapshot() const
{
    return d_clusterStateSnapshot;
}

// ----------------
// class ClusterFSM
// ----------------

// CREATORS
inline ClusterFSM::ClusterFSM(
    ClusterStateTableActions<ClusterFSMArgsSp>& actions)
: d_stateTable()
, d_state(State::e_UNKNOWN)
, d_actions(actions)
, d_observers()
{
    // NOTHING
}

inline ClusterFSM::State::Enum ClusterFSM::state() const
{
    return d_state;
}

inline bool ClusterFSM::isSelfLeader() const
{
    return d_state == State::e_LDR_HEALING_STG1 ||
           d_state == State::e_LDR_HEALING_STG2 ||
           d_state == State::e_LDR_HEALED;
}

inline bool ClusterFSM::isSelfFollower() const
{
    return d_state == State::e_FOL_HEALING || d_state == State::e_FOL_HEALED;
}

inline bool ClusterFSM::isSelfHealed() const
{
    return d_state == State::e_FOL_HEALED || d_state == State::e_LDR_HEALED;
}

}  // close package namespace
}  // close enterprise namespace

#endif
