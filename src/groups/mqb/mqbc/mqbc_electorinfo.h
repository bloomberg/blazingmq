// Copyright 2019-2023 Bloomberg Finance L.P.
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

// mqbc_electorinfo.h                                                 -*-C++-*-
#ifndef INCLUDED_MQBC_ELECTORINFO
#define INCLUDED_MQBC_ELECTORINFO

//@PURPOSE: Provide a VST for elector information.
//
//@CLASSES:
//  mqbc::ElectorInfoLeaderStatus: Status of a leader node in a cluster
//  mqbc::ElectorInfo            : VST for elector information
//  mqbc::ElectorInfoObserver    : Interface for an observer of elector info
//
//@DESCRIPTION: 'mqbc::ElectorInfo' is a value-semantic type representing the
// elector information in a cluster.  Important changes in elector information
// can be notified to observers, implementing the 'mqbc::ElectorInfoObserver'
// interface.
//
/// Thread Safety
///-------------
// The 'mqbc::ElectorInfo' object is not thread safe and should always be
// manipulated from the associated cluster's dispatcher thread.

// MQB

#include <mqbc_clusterfsm.h>
#include <mqbi_cluster.h>
#include <mqbnet_elector.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>

// BDE
#include <ball_log.h>
#include <bdlmt_eventscheduler.h>
#include <bsl_ostream.h>
#include <bsl_unordered_set.h>
#include <bsls_keyword.h>
#include <bsls_types.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace mqbnet {
class ClusterNode;
}

namespace mqbc {

// ==============================
// struct ElectorInfoLeaderStatus
// ==============================

/// This struct defines various status of a leader node in a cluster.
struct ElectorInfoLeaderStatus {
    // TYPES
    enum Enum { e_UNDEFINED = 0, e_PASSIVE = 1, e_ACTIVE = 2 };

    // CLASS METHODS

    /// Write the string representation of the specified enumeration `value`
    /// to the specified output `stream`, and return a reference to
    /// `stream`.  Optionally specify an initial indentation `level`, whose
    /// absolute value is incremented recursively for nested objects.  If
    /// `level` is specified, optionally specify `spacesPerLevel`, whose
    /// absolute value indicates the number of spaces per indentation level
    /// for this and all of its nested objects.  If `level` is negative,
    /// suppress indentation of the first line.  If `spacesPerLevel` is
    /// negative, format the entire output on one line, suppressing all but
    /// the initial indentation (as governed by `level`).  See `toAscii` for
    /// what constitutes the string representation of a
    /// `ElectorInfoLeaderStatus::Enum` value.
    static bsl::ostream& print(bsl::ostream&                 stream,
                               ElectorInfoLeaderStatus::Enum value,
                               int                           level = 0,
                               int spacesPerLevel                  = 4);

    /// Return the non-modifiable string representation corresponding to the
    /// specified enumeration `value`, if it exists, and a unique (error)
    /// string otherwise.  The string representation of `value` matches its
    /// corresponding enumerator name with the "e_" prefix eluded.  For
    /// example:
    /// ```
    /// bsl::cout << ElectorInfoLeaderStatus::toAscii(
    ///                                 ElectorInfoLeaderStatus::e_ACTIVE);
    /// ```
    /// will print the following on standard output:
    /// ```
    /// ACTIVE
    /// ```
    /// Note that specifying a `value` that does not match any of the
    /// enumerators will result in a string representation that is distinct
    /// from any of those corresponding to the enumerators, but is otherwise
    /// unspecified.
    static const char* toAscii(ElectorInfoLeaderStatus::Enum value);
};

// FREE OPERATORS

/// Format the specified `value` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream&                 stream,
                         ElectorInfoLeaderStatus::Enum value);

// =========================
// class ElectorInfoObserver
// =========================

/// This interface exposes notifications of events happening on the elector
/// information state.
///
/// NOTE: This is purposely not a pure interface, each method has a default
///       void implementation, so that clients only need to implement the
///       ones they care about.
class ElectorInfoObserver {
  public:
    // CREATORS

    /// Destructor
    virtual ~ElectorInfoObserver();

    // MANIPULATORS

    /// Callback invoked when the cluster's leader changes to the specified
    /// `node` with the specified `status`.  Note that null is a valid value
    /// for the `node`, and it implies that the cluster has transitioned to
    /// a state of no leader, and in this case, `status` will be
    /// `UNDEFINED`.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    virtual void onClusterLeader(mqbnet::ClusterNode*          node,
                                 ElectorInfoLeaderStatus::Enum status);
};

// =================
// class ElectorInfo
// =================

/// This class provides a VST representing the elector information of a
/// cluster.
class ElectorInfo : public ClusterFSMObserver {
  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBC.ELECTORINFO");

  public:
    // TYPES
    typedef bdlmt::EventScheduler::EventHandle SchedulerEventHandle;

    typedef bsl::unordered_set<ElectorInfoObserver*> ObserversSet;
    // A set of ElectorInfo observers.

  private:
    // DATA

    // from mqbblp::ClusterState
    mqbnet::ElectorState::Enum d_electorState;
    // Elector's state

    mqbnet::ClusterNode* d_leaderNode_p;
    // Current leader, as notified by
    // the elector, or null if no
    // leader.

    ElectorInfoLeaderStatus::Enum d_leaderStatus;
    // Status of current leader.

    bmqp_ctrlmsg::LeaderMessageSequence d_leaderMessageSequence;
    // Sequence number of the last
    // message received from the leader
    // (if follower), or published (if
    // leader).

    SchedulerEventHandle d_leaderSyncEventHandle;
    // Handle to a one-time event
    // scheduled by leader before
    // initiating leader sync with
    // AVAILABLE followers

    const mqbi::Cluster* d_cluster_p;
    // Constant pointer to the cluster
    // associated with this elector
    // information (held not owned)

    ObserversSet d_observers;
    // Observers of this object.

  public:
    // CREATORS

    /// Default constructor
    ElectorInfo(mqbi::Cluster* cluster);

    // MANIPULATORS

    /// Invoked when the Cluster FSM transitions to `LDR_HEALED` state.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    void onHealedLeader() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    /// Register the specified `observer` to be notified of elector changes.
    /// Return a reference offerring modifiable access to this object.
    ///
    /// THREAD: This method should only be called from the associated
    /// cluster's dispatcher thread.
    ElectorInfo& registerObserver(ElectorInfoObserver* observer);

    /// Unregister the specified `observer` to be notified of elector
    /// changes.  Return a reference offerring modifiable access to this
    /// object.
    ///
    /// THREAD: This method should only be called from the associated
    /// cluster's dispatcher thread.
    ElectorInfo& unregisterObserver(ElectorInfoObserver* observer);

    ElectorInfo& setElectorState(mqbnet::ElectorState::Enum value);
    ElectorInfo& setElectorTerm(bsls::Types::Uint64 value);
    ElectorInfo& setLeaderNode(mqbnet::ClusterNode* value);
    ElectorInfo& setLeaderStatus(ElectorInfoLeaderStatus::Enum value);

    /// Set the corresponding member to the specified `value` and return a
    /// reference offering modifiable access to this object.
    ElectorInfo&
    setLeaderMessageSequence(const bmqp_ctrlmsg::LeaderMessageSequence& value);

    /// Update the elector status to represent the specified `state` with
    /// the associated specified `term`.  Indicate that the specified `node`
    /// is the leader with the specified `status`, or a null pointer if
    /// there are no leader.  If the leader has changed, this will notify
    /// all active observers by invoking `onClusterLeader()` on each of
    /// them, with the `node` as parameter.
    ElectorInfo& setElectorInfo(mqbnet::ElectorState::Enum    state,
                                bsls::Types::Uint64           term,
                                mqbnet::ClusterNode*          node,
                                ElectorInfoLeaderStatus::Enum status);

    /// Bump up the leader sequence number and populate to the optionally
    /// specified 'sequence'.
    void nextLeaderMessageSequence(
        bmqp_ctrlmsg::LeaderMessageSequence* sequence = 0);

    /// Invoked when self has transitioned to ACTIVE leader.
    void onSelfActiveLeader();

    /// Get a modifiable reference to this object's leaderSyncEventHandle.
    SchedulerEventHandle* leaderSyncEventHandle();

    /// Return a pointer to the ClusterNode currently leader of the cluster,
    /// or null pointer if there are no leader.
    mqbnet::ClusterNode* leaderNode();

    // ACCESSORS

    /// Convenience method to return the nodeId of the current leader, or
    /// `mqbnet::Elector::k_INVALID_NODE_ID` if there are no leader.
    int leaderNodeId() const;

    mqbnet::ElectorState::Enum    electorState() const;
    bsls::Types::Uint64           electorTerm() const;
    mqbnet::ClusterNode*          leaderNode() const;
    ElectorInfoLeaderStatus::Enum leaderStatus() const;

    /// Return the value of the corresponding member of this object.
    const bmqp_ctrlmsg::LeaderMessageSequence& leaderMessageSequence() const;

    bool isSelfLeader() const;

    bool isSelfActiveLeader() const;

    /// Return true if there is currently a leader, *and* the leader is
    /// active, false otherwise.  Note that self node could be an active
    /// leader as well.
    bool hasActiveLeader() const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -----------------
// class ElectorInfo
// -----------------

// CREATORS
inline ElectorInfo::ElectorInfo(mqbi::Cluster* cluster)
: d_electorState(mqbnet::ElectorState::e_DORMANT)
, d_leaderNode_p(0)
, d_leaderStatus(ElectorInfoLeaderStatus::e_UNDEFINED)
, d_leaderMessageSequence()
, d_leaderSyncEventHandle()
, d_cluster_p(cluster)
, d_observers()
{
    d_leaderMessageSequence.electorTerm()    = mqbnet::Elector::k_INVALID_TERM;
    d_leaderMessageSequence.sequenceNumber() = 0;

    BALL_LOG_INFO << "Setting elector's leader sequence number to "
                  << d_leaderMessageSequence;
}

// MANIPULATORS
inline ElectorInfo&
ElectorInfo::setElectorState(mqbnet::ElectorState::Enum value)
{
    d_electorState = value;
    return *this;
}

inline ElectorInfo& ElectorInfo::setElectorTerm(bsls::Types::Uint64 value)
{
    d_leaderMessageSequence.electorTerm()    = value;
    d_leaderMessageSequence.sequenceNumber() = 0;

    BALL_LOG_INFO << "Setting elector's leader sequence number to "
                  << d_leaderMessageSequence;
    return *this;
}

inline ElectorInfo& ElectorInfo::setLeaderNode(mqbnet::ClusterNode* value)
{
    d_leaderNode_p = value;
    return *this;
}

inline ElectorInfo& ElectorInfo::setLeaderMessageSequence(
    const bmqp_ctrlmsg::LeaderMessageSequence& value)
{
    d_leaderMessageSequence = value;

    BALL_LOG_INFO << "Setting elector's leader sequence number to "
                  << d_leaderMessageSequence;
    return *this;
}

inline void ElectorInfo::nextLeaderMessageSequence(
    bmqp_ctrlmsg::LeaderMessageSequence* sequence)
{
    ++d_leaderMessageSequence.sequenceNumber();
    if (sequence) {
        *sequence = d_leaderMessageSequence;
    }

    BALL_LOG_INFO << "Bumping up elector's leader sequence number to "
                  << d_leaderMessageSequence;
}

inline ElectorInfo::SchedulerEventHandle* ElectorInfo::leaderSyncEventHandle()
{
    return &d_leaderSyncEventHandle;
}

inline mqbnet::ClusterNode* ElectorInfo::leaderNode()
{
    return d_leaderNode_p;
}

// ACCESSORS
inline int ElectorInfo::leaderNodeId() const
{
    return (d_leaderNode_p ? d_leaderNode_p->nodeId()
                           : mqbnet::Elector::k_INVALID_NODE_ID);
}

inline mqbnet::ElectorState::Enum ElectorInfo::electorState() const
{
    return d_electorState;
}

inline bsls::Types::Uint64 ElectorInfo::electorTerm() const
{
    return d_leaderMessageSequence.electorTerm();
}

inline mqbnet::ClusterNode* ElectorInfo::leaderNode() const
{
    return d_leaderNode_p;
}

inline ElectorInfoLeaderStatus::Enum ElectorInfo::leaderStatus() const
{
    return d_leaderStatus;
}

inline const bmqp_ctrlmsg::LeaderMessageSequence&
ElectorInfo::leaderMessageSequence() const
{
    return d_leaderMessageSequence;
}

inline bool ElectorInfo::isSelfLeader() const
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));

    if (d_cluster_p->isRemote()) {
        return false;  // RETURN
    }

    return d_leaderNode_p && (d_leaderNode_p->nodeId() ==
                              d_cluster_p->netCluster().selfNodeId());
}

inline bool ElectorInfo::isSelfActiveLeader() const
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_cluster_p->dispatcher()->inDispatcherThread(d_cluster_p));

    if (!isSelfLeader()) {
        return false;  // RETURN
    }

    return ElectorInfoLeaderStatus::e_ACTIVE == d_leaderStatus;
}

inline bool ElectorInfo::hasActiveLeader() const
{
    return leaderNode() && ElectorInfoLeaderStatus::e_ACTIVE == leaderStatus();
}

}  // close package namespace

// FREE OPERATORS
inline bsl::ostream&
mqbc::operator<<(bsl::ostream&                       stream,
                 mqbc::ElectorInfoLeaderStatus::Enum value)
{
    return ElectorInfoLeaderStatus::print(stream, value, 0, -1);
}

namespace bmqp_ctrlmsg {

/// Return true if the specified `lhs` is smaller than the specified `rhs`,
/// false otherwise.
bool operator<(const bmqp_ctrlmsg::LeaderMessageSequence& lhs,
               const bmqp_ctrlmsg::LeaderMessageSequence& rhs);

/// Return true if the specified `lhs` is greater than the specified `rhs`,
/// false otherwise.
bool operator>(const bmqp_ctrlmsg::LeaderMessageSequence& lhs,
               const bmqp_ctrlmsg::LeaderMessageSequence& rhs);

/// Return true if the specified `lhs` is smaller than or equal to the
/// specified `rhs`, false otherwise.
bool operator<=(const bmqp_ctrlmsg::LeaderMessageSequence& lhs,
                const bmqp_ctrlmsg::LeaderMessageSequence& rhs);

/// Return true if the specified `lhs` is greater than or equal to the
/// specified `rhs`, false otherwise.
bool operator>=(const bmqp_ctrlmsg::LeaderMessageSequence& lhs,
                const bmqp_ctrlmsg::LeaderMessageSequence& rhs);

}  // close package namespace

inline bool
bmqp_ctrlmsg::operator<(const bmqp_ctrlmsg::LeaderMessageSequence& lhs,
                        const bmqp_ctrlmsg::LeaderMessageSequence& rhs)
{
    if (lhs.electorTerm() != rhs.electorTerm()) {
        return lhs.electorTerm() < rhs.electorTerm();  // RETURN
    }

    if (lhs.sequenceNumber() != rhs.sequenceNumber()) {
        return lhs.sequenceNumber() < rhs.sequenceNumber();  // RETURN
    }

    return false;
}

inline bool
bmqp_ctrlmsg::operator>(const bmqp_ctrlmsg::LeaderMessageSequence& lhs,
                        const bmqp_ctrlmsg::LeaderMessageSequence& rhs)
{
    return !((lhs == rhs) || (lhs < rhs));
}

inline bool
bmqp_ctrlmsg::operator<=(const bmqp_ctrlmsg::LeaderMessageSequence& lhs,
                         const bmqp_ctrlmsg::LeaderMessageSequence& rhs)
{
    return !(lhs > rhs);
}

inline bool
bmqp_ctrlmsg::operator>=(const bmqp_ctrlmsg::LeaderMessageSequence& lhs,
                         const bmqp_ctrlmsg::LeaderMessageSequence& rhs)
{
    return !(lhs < rhs);
}

}  // close enterprise namespace

#endif
