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

// mqbc_clusterstatetable.h                                           -*-C++-*-
#ifndef INCLUDED_MQBC_CLUSTERSTATETABLE
#define INCLUDED_MQBC_CLUSTERSTATETABLE

/// @file mqbc_clusterstatetable.h
///
/// @brief Provide a state table for the Cluster FSM.
///
/// @bbref{mqbc::ClusterStateTable} is a state table for the Cluster FSM.
///
/// Thread Safety                              {#mqbc_clusterstatetable_thread}
/// =============
///
/// The @bbref{mqbc::ClusterStateTable} object is not thread safe.

// MQB
#include <mqbu_statetable.h>

// BDE
#include <bsl_iostream.h>
#include <bsl_string.h>
#include <bsl_utility.h>
#include <bsls_annotation.h>

namespace BloombergLP {
namespace mqbc {

// =============================
// struct ClusterStateTableState
// =============================

/// This struct defines the type of state in the cluster state table.
struct ClusterStateTableState {
    // TYPES

    /// Enumeration used to distinguish among different type of state.
    enum Enum {
        e_UNKNOWN          = 0,
        e_FOL_HEALING      = 1,
        e_LDR_HEALING_STG1 = 2,
        e_LDR_HEALING_STG2 = 3,
        e_FOL_HEALED       = 4,
        e_LDR_HEALED       = 5,
        e_STOPPING         = 6,
        e_STOPPED          = 7,
        e_NUM_STATES       = 8
    };

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
    /// @bbref{ClusterStateTableState::Enum} value.
    static bsl::ostream& print(bsl::ostream&                stream,
                               ClusterStateTableState::Enum value,
                               int                          level = 0,
                               int spacesPerLevel                 = 4);

    /// Return the non-modifiable string representation corresponding to the
    /// specified enumeration `value`, if it exists, and a unique (error)
    /// string otherwise.  The string representation of `value` matches its
    /// corresponding enumerator name with the `e_` prefix elided.  Note
    /// that specifying a `value` that does not match any of the enumerators
    /// will result in a string representation that is distinct from any of
    /// those corresponding to the enumerators, but is otherwise
    /// unspecified.
    static const char* toAscii(ClusterStateTableState::Enum value);

    /// Return true and fills the specified `out` with the enum value
    /// corresponding to the specified `str`, if valid, or return false and
    /// leave `out` untouched if `str` doesn't correspond to any value of
    /// the enum.
    static bool fromAscii(ClusterStateTableState::Enum* out,
                          const bslstl::StringRef&      str);
};

// FREE OPERATORS

/// Format the specified `value` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream&                stream,
                         ClusterStateTableState::Enum value);

// =============================
// struct ClusterStateTableEvent
// =============================

/// This struct defines the type of event in the cluster state table.
struct ClusterStateTableEvent {
    // TYPES

    /// Enumeration used to distinguish among different type of event.
    enum Enum {
        e_SLCT_LDR               = 0,
        e_SLCT_FOL               = 1,
        e_FOL_LSN_RQST           = 2,
        e_FOL_LSN_RSPN           = 3,
        e_QUORUM_LSN             = 4,
        e_LOST_QUORUM_LSN        = 5,
        e_SELF_HIGHEST_LSN       = 6,
        e_FOL_HIGHEST_LSN        = 7,
        e_FAIL_FOL_LSN_RSPN      = 8,
        e_FOL_CSL_RQST           = 9,
        e_FOL_CSL_RSPN           = 10,
        e_FAIL_FOL_CSL_RSPN      = 11,
        e_CRASH_FOL_CSL          = 12,
        e_STOP_NODE              = 13,
        e_STOP_SUCCESS           = 14,
        e_REGISTRATION_RQST      = 15,
        e_REGISTRATION_RSPN      = 16,
        e_FAIL_REGISTRATION_RSPN = 17,
        e_RST_UNKNOWN            = 18,
        e_CSL_CMT_SUCCESS        = 19,
        e_CSL_CMT_FAIL           = 20,
        e_WATCH_DOG              = 21,
        e_NUM_EVENTS             = 22
    };

    // CLASS METHODS

    /// Write the string representation of the specified enumeration `value`
    /// to the specified output `stream`, and return a reference to
    /// `stream`.  Optionally specify an initial indentation `level`, whose
    /// absolute value ieventremented recursively for nested objects.  If
    /// `level` is specified, optionally specify `spacesPerLevel`, whose
    /// absolute value indicates the number of spaces per indentation level
    /// for this and all of its nested objects.  If `level` is negative,
    /// suppress indentation of the first line.  If `spacesPerLevel` is
    /// negative, format the entire output on one line, suppressing all but
    /// the initial indentation (as governed by `level`).  See `toAscii` for
    /// what constitutes the string representation of a
    /// @bbref{ClusterStateTableEvent::Enum} value.
    static bsl::ostream& print(bsl::ostream&                stream,
                               ClusterStateTableEvent::Enum value,
                               int                          level = 0,
                               int spacesPerLevel                 = 4);

    /// Return the non-modifiable string representation corresponding to the
    /// specified enumeration `value`, if it exists, and a unique (error)
    /// string otherwise.  The string representation of `value` matches its
    /// corresponding enumerator name with the `e_` prefix elided.  Note
    /// that specifying a `value` that does not match any of the enumerators
    /// will result in a string representation that is distinct from any of
    /// those corresponding to the enumerators, but is otherwise
    /// unspecified.
    static const char* toAscii(ClusterStateTableEvent::Enum value);

    /// Return true and fills the specified `out` with the enum value
    /// corresponding to the specified `str`, if valid, or return false and
    /// leave `out` untouched if `str` doesn't correspond to any value of
    /// the enum.
    static bool fromAscii(ClusterStateTableEvent::Enum* out,
                          const bslstl::StringRef&      str);
};

// FREE OPERATORS

/// Format the specified `value` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream&                stream,
                         ClusterStateTableEvent::Enum value);

// ==============================
// class ClusterStateTableActions
// ==============================

/// This class defines the actions in the cluster state table.
template <typename ARGS>
class ClusterStateTableActions {
  public:
    // TYPES
    typedef void (ClusterStateTableActions<ARGS>::*ActionFunctor)(
        const ARGS& x);

  public:
    /// Virtual destructor.
    virtual ~ClusterStateTableActions();

    virtual void do_none(const ARGS& args);

    virtual void do_abort(const ARGS& args) = 0;

    virtual void do_startWatchDog(const ARGS& args) = 0;

    virtual void do_stopWatchDog(const ARGS& args) = 0;

    virtual void do_triggerWatchDog(const ARGS& args) = 0;

    virtual void do_applyCSLSelf(const ARGS& args) = 0;

    virtual void do_initializeQueueKeyInfoMap(const ARGS& args) = 0;

    virtual void do_sendFollowerLSNRequests(const ARGS& args) = 0;

    virtual void do_sendFollowerLSNResponse(const ARGS& args) = 0;

    virtual void do_sendFailureFollowerLSNResponse(const ARGS& args) = 0;

    virtual void do_findHighestLSN(const ARGS& args) = 0;

    virtual void do_sendFollowerClusterStateRequest(const ARGS& args) = 0;

    virtual void do_sendFollowerClusterStateResponse(const ARGS& args) = 0;

    virtual void
    do_sendFailureFollowerClusterStateResponse(const ARGS& args) = 0;

    virtual void do_storeSelfLSN(const ARGS& args) = 0;

    virtual void do_storeFollowerLSNs(const ARGS& args) = 0;

    virtual void do_removeFollowerLSN(const ARGS& args) = 0;

    virtual void do_checkLSNQuorum(const ARGS& args) = 0;

    virtual void do_sendRegistrationRequest(const ARGS& args) = 0;

    virtual void do_sendRegistrationResponse(const ARGS& args) = 0;

    virtual void do_sendFailureRegistrationResponse(const ARGS& args) = 0;

    virtual void do_logStaleFollowerLSNResponse(const ARGS& args) = 0;

    virtual void do_logStaleFollowerClusterStateResponse(const ARGS& args) = 0;

    virtual void do_logErrorLeaderNotHealed(const ARGS& args) = 0;

    virtual void do_logFailFollowerLSNResponses(const ARGS& args) = 0;

    virtual void do_logFailFollowerClusterStateResponse(const ARGS& args) = 0;

    virtual void do_logFailRegistrationResponse(const ARGS& args) = 0;

    virtual void do_reapplyEvent(const ARGS& args) = 0;

    virtual void do_reapplySelectLeader(const ARGS& args) = 0;

    virtual void do_reapplySelectFollower(const ARGS& args) = 0;

    virtual void do_cleanupLSNs(const ARGS& args) = 0;

    virtual void do_cancelRequests(const ARGS& args) = 0;

    void do_startWatchDog_storeSelfLSN_sendFollowerLSNRequests_checkLSNQuorum(
        const ARGS& args);

    void do_startWatchDog_sendRegistrationRequest(const ARGS& args);

    void do_stopWatchDog_cancelRequests(const ARGS& args);

    void do_stopWatchDog_cancelRequests_reapplyEvent(const ARGS& args);

    void do_stopWatchDog_initializeQueueKeyInfoMap(const ARGS& args);

    void do_stopWatchDog_cleanupLSNs_cancelRequests(const ARGS& args);

    void
    do_stopWatchDog_cleanupLSNs_cancelRequests_reapplyEvent(const ARGS& args);

    void do_cleanupLSNs_reapplyEvent(const ARGS& args);

    void do_cancelRequests_reapplySelectFollower(const ARGS& args);

    void do_cleanupLSNs_cancelRequests_reapplySelectLeader(const ARGS& args);

    void do_sendFollowerLSNResponse_logErrorLeaderNotHealed(const ARGS& args);

    void do_sendFollowerClusterStateResponse_logErrorLeaderNotHealed(
        const ARGS& args);

    void do_storeFollowerLSNs_checkLSNQuorum(const ARGS& args);

    void do_storeFollowerLSNs_checkLSNQuorum_sendRegistrationResponse(
        const ARGS& args);

    void do_storeFollowerLSNs_sendRegistrationResponse(const ARGS& args);

    void
    do_logFailFollowerClusterStateResponse_removeFollowerLSN_checkLSNQuorum(
        const ARGS& args);

    void do_removeFollowerLSN_checkLSNQuorum(const ARGS& args);

    void do_sendRegistrationResponse_applyCSLSelf(const ARGS& args);
};

// =======================
// class ClusterStateTable
// =======================

/// This class is a state table for the Cluster FSM.
template <typename ARGS>
class ClusterStateTable
: public mqbu::StateTable<
      ClusterStateTableState::e_NUM_STATES,
      ClusterStateTableEvent::e_NUM_EVENTS,
      typename ClusterStateTableActions<ARGS>::ActionFunctor> {
  public:
    // TYPES
    typedef ClusterStateTableState State;
    typedef ClusterStateTableEvent Event;
    typedef
        typename ClusterStateTableActions<ARGS>::ActionFunctor ActionFunctor;

    typedef mqbu::
        StateTable<State::e_NUM_STATES, Event::e_NUM_EVENTS, ActionFunctor>
                                       Table;
    typedef typename Table::Transition Transition;

  public:
    // CREATORS
    ClusterStateTable()
    : Table(&ClusterStateTableActions<ARGS>::do_none)
    {
// ClusterStateTable Config:
#define CST_CFG(s, e, a, n)                                                   \
    Table::configure(State::e_##s,                                            \
                     Event::e_##e,                                            \
                     Transition(State::e_##n,                                 \
                                &ClusterStateTableActions<ARGS>::do_##a));
        //       state             event                   action next state
        CST_CFG(
            UNKNOWN,
            SLCT_LDR,
            startWatchDog_storeSelfLSN_sendFollowerLSNRequests_checkLSNQuorum,
            LDR_HEALING_STG1);
        CST_CFG(UNKNOWN,
                SLCT_FOL,
                startWatchDog_sendRegistrationRequest,
                FOL_HEALING);
        CST_CFG(UNKNOWN,
                FOL_LSN_RQST,
                sendFailureFollowerLSNResponse,
                UNKNOWN);
        CST_CFG(UNKNOWN, FOL_LSN_RSPN, logStaleFollowerLSNResponse, UNKNOWN);
        CST_CFG(UNKNOWN,
                FOL_CSL_RQST,
                sendFailureFollowerClusterStateResponse,
                UNKNOWN);
        CST_CFG(UNKNOWN,
                FOL_CSL_RSPN,
                logStaleFollowerClusterStateResponse,
                UNKNOWN);
        CST_CFG(UNKNOWN,
                REGISTRATION_RQST,
                sendFailureRegistrationResponse,
                UNKNOWN);
        CST_CFG(UNKNOWN, STOP_NODE, none, STOPPING);
        CST_CFG(UNKNOWN, STOP_SUCCESS, abort, UNKNOWN);
        CST_CFG(FOL_HEALING,
                SLCT_LDR,
                stopWatchDog_cancelRequests_reapplyEvent,
                UNKNOWN);
        CST_CFG(FOL_HEALING,
                FOL_LSN_RQST,
                sendFollowerLSNResponse,
                FOL_HEALING);
        CST_CFG(FOL_HEALING,
                FOL_LSN_RSPN,
                logStaleFollowerLSNResponse,
                FOL_HEALING);
        CST_CFG(FOL_HEALING,
                REGISTRATION_RQST,
                sendFailureRegistrationResponse,
                FOL_HEALING);
        CST_CFG(FOL_HEALING,
                FAIL_REGISTRATION_RSPN,
                logFailRegistrationResponse,
                FOL_HEALING);
        CST_CFG(FOL_HEALING,
                FOL_CSL_RQST,
                sendFollowerClusterStateResponse,
                FOL_HEALING);
        CST_CFG(FOL_HEALING,
                CSL_CMT_SUCCESS,
                stopWatchDog_initializeQueueKeyInfoMap,
                FOL_HEALED);
        CST_CFG(FOL_HEALING, CSL_CMT_FAIL, triggerWatchDog, UNKNOWN);
        CST_CFG(FOL_HEALING,
                RST_UNKNOWN,
                stopWatchDog_cancelRequests,
                UNKNOWN);
        CST_CFG(FOL_HEALING,
                WATCH_DOG,
                cancelRequests_reapplySelectFollower,
                UNKNOWN);
        CST_CFG(FOL_HEALING, STOP_NODE, stopWatchDog_cancelRequests, STOPPING);
        CST_CFG(FOL_HEALING, STOP_SUCCESS, abort, FOL_HEALING);
        CST_CFG(LDR_HEALING_STG1,
                SLCT_FOL,
                stopWatchDog_cleanupLSNs_cancelRequests_reapplyEvent,
                UNKNOWN);
        CST_CFG(LDR_HEALING_STG1,
                FOL_LSN_RQST,
                sendFailureFollowerLSNResponse,
                LDR_HEALING_STG1);
        CST_CFG(LDR_HEALING_STG1,
                FOL_LSN_RSPN,
                storeFollowerLSNs_checkLSNQuorum,
                LDR_HEALING_STG1);
        CST_CFG(LDR_HEALING_STG1,
                FAIL_FOL_LSN_RSPN,
                logFailFollowerLSNResponses,
                LDR_HEALING_STG1);
        CST_CFG(LDR_HEALING_STG1,
                REGISTRATION_RQST,
                storeFollowerLSNs_checkLSNQuorum_sendRegistrationResponse,
                LDR_HEALING_STG1);
        CST_CFG(LDR_HEALING_STG1,
                FOL_CSL_RQST,
                sendFailureFollowerClusterStateResponse,
                LDR_HEALING_STG1);
        CST_CFG(LDR_HEALING_STG1,
                FOL_CSL_RSPN,
                logStaleFollowerClusterStateResponse,
                LDR_HEALING_STG1);
        CST_CFG(LDR_HEALING_STG1,
                QUORUM_LSN,
                findHighestLSN,
                LDR_HEALING_STG1);
        CST_CFG(LDR_HEALING_STG1,
                SELF_HIGHEST_LSN,
                applyCSLSelf,
                LDR_HEALING_STG2);
        CST_CFG(LDR_HEALING_STG1,
                FOL_HIGHEST_LSN,
                sendFollowerClusterStateRequest,
                LDR_HEALING_STG2);
        CST_CFG(LDR_HEALING_STG1,
                RST_UNKNOWN,
                stopWatchDog_cleanupLSNs_cancelRequests,
                UNKNOWN);
        CST_CFG(LDR_HEALING_STG1,
                WATCH_DOG,
                cleanupLSNs_cancelRequests_reapplySelectLeader,
                UNKNOWN);
        CST_CFG(LDR_HEALING_STG1,
                STOP_NODE,
                stopWatchDog_cleanupLSNs_cancelRequests,
                STOPPING);
        CST_CFG(LDR_HEALING_STG1, STOP_SUCCESS, abort, LDR_HEALING_STG1);
        CST_CFG(LDR_HEALING_STG2,
                SLCT_FOL,
                stopWatchDog_cleanupLSNs_cancelRequests_reapplyEvent,
                UNKNOWN);
        CST_CFG(LDR_HEALING_STG2,
                FOL_LSN_RQST,
                sendFailureFollowerLSNResponse,
                LDR_HEALING_STG2);
        CST_CFG(LDR_HEALING_STG2,
                FOL_LSN_RSPN,
                storeFollowerLSNs,
                LDR_HEALING_STG2);
        CST_CFG(LDR_HEALING_STG2,
                FOL_CSL_RQST,
                sendFailureFollowerClusterStateResponse,
                LDR_HEALING_STG2);
        CST_CFG(LDR_HEALING_STG2,
                FOL_CSL_RSPN,
                applyCSLSelf,
                LDR_HEALING_STG2);
        CST_CFG(
            LDR_HEALING_STG2,
            FAIL_FOL_CSL_RSPN,
            logFailFollowerClusterStateResponse_removeFollowerLSN_checkLSNQuorum,
            LDR_HEALING_STG2);
        CST_CFG(LDR_HEALING_STG2,
                CRASH_FOL_CSL,
                removeFollowerLSN_checkLSNQuorum,
                LDR_HEALING_STG2);
        CST_CFG(LDR_HEALING_STG2,
                QUORUM_LSN,
                findHighestLSN,
                LDR_HEALING_STG2);
        CST_CFG(LDR_HEALING_STG2,
                LOST_QUORUM_LSN,
                sendFollowerLSNRequests,
                LDR_HEALING_STG1);
        CST_CFG(LDR_HEALING_STG2,
                SELF_HIGHEST_LSN,
                applyCSLSelf,
                LDR_HEALING_STG2);
        CST_CFG(LDR_HEALING_STG2,
                FOL_HIGHEST_LSN,
                sendFollowerClusterStateRequest,
                LDR_HEALING_STG2);
        CST_CFG(LDR_HEALING_STG2,
                REGISTRATION_RQST,
                storeFollowerLSNs_sendRegistrationResponse,
                LDR_HEALING_STG2);
        CST_CFG(LDR_HEALING_STG2,
                CSL_CMT_SUCCESS,
                stopWatchDog_initializeQueueKeyInfoMap,
                LDR_HEALED);
        CST_CFG(LDR_HEALING_STG2, CSL_CMT_FAIL, triggerWatchDog, UNKNOWN);
        CST_CFG(LDR_HEALING_STG2,
                RST_UNKNOWN,
                stopWatchDog_cleanupLSNs_cancelRequests,
                UNKNOWN);
        CST_CFG(LDR_HEALING_STG2,
                WATCH_DOG,
                cleanupLSNs_cancelRequests_reapplySelectLeader,
                UNKNOWN);
        CST_CFG(LDR_HEALING_STG2,
                STOP_NODE,
                stopWatchDog_cleanupLSNs_cancelRequests,
                STOPPING);
        CST_CFG(LDR_HEALING_STG2, STOP_SUCCESS, abort, LDR_HEALING_STG2);
        CST_CFG(FOL_HEALED, SLCT_LDR, reapplyEvent, UNKNOWN);
        CST_CFG(FOL_HEALED,
                FOL_LSN_RQST,
                sendFollowerLSNResponse_logErrorLeaderNotHealed,
                FOL_HEALING);
        CST_CFG(FOL_HEALED,
                FOL_LSN_RSPN,
                logStaleFollowerLSNResponse,
                UNKNOWN);
        CST_CFG(FOL_HEALED,
                REGISTRATION_RQST,
                sendFailureRegistrationResponse,
                FOL_HEALED);
        CST_CFG(FOL_HEALED,
                FOL_CSL_RQST,
                sendFollowerClusterStateResponse_logErrorLeaderNotHealed,
                FOL_HEALING);
        CST_CFG(FOL_HEALED, RST_UNKNOWN, none, UNKNOWN);
        CST_CFG(FOL_HEALED, STOP_NODE, none, STOPPING);
        CST_CFG(FOL_HEALED, STOP_SUCCESS, abort, FOL_HEALED);
        CST_CFG(LDR_HEALED, SLCT_FOL, cleanupLSNs_reapplyEvent, UNKNOWN);
        CST_CFG(LDR_HEALED,
                FOL_LSN_RQST,
                sendFailureFollowerLSNResponse,
                LDR_HEALED);
        CST_CFG(LDR_HEALED, FOL_LSN_RSPN, applyCSLSelf, LDR_HEALED);
        CST_CFG(LDR_HEALED,
                REGISTRATION_RQST,
                sendRegistrationResponse_applyCSLSelf,
                LDR_HEALED);
        CST_CFG(LDR_HEALED,
                FOL_CSL_RQST,
                sendFailureFollowerClusterStateResponse,
                LDR_HEALED);
        CST_CFG(LDR_HEALED, RST_UNKNOWN, cleanupLSNs, UNKNOWN);
        CST_CFG(LDR_HEALED, STOP_NODE, cleanupLSNs, STOPPING);
        CST_CFG(LDR_HEALED, STOP_SUCCESS, abort, LDR_HEALED);
        CST_CFG(STOPPING, STOP_SUCCESS, none, STOPPED);
#undef CST_CFG
    }
};

// ============================================================================
//                           INLINE DEFINITIONS
// ============================================================================

// ------------------------------
// class ClusterStateTableActions
// ------------------------------

// CREATORS
template <typename ARGS>
ClusterStateTableActions<ARGS>::~ClusterStateTableActions()
{
    // NOTHING (pure interface)
}

template <typename ARGS>
void ClusterStateTableActions<ARGS>::do_none(
    BSLS_ANNOTATION_UNUSED const ARGS& args)
{
    // NOTHING
}

template <typename ARGS>
void ClusterStateTableActions<ARGS>::
    do_startWatchDog_storeSelfLSN_sendFollowerLSNRequests_checkLSNQuorum(
        const ARGS& args)
{
    do_startWatchDog(args);
    do_storeSelfLSN(args);
    do_sendFollowerLSNRequests(args);
    do_checkLSNQuorum(args);
}

template <typename ARGS>
void ClusterStateTableActions<ARGS>::do_startWatchDog_sendRegistrationRequest(
    const ARGS& args)
{
    do_startWatchDog(args);
    do_sendRegistrationRequest(args);
}

template <typename ARGS>
void ClusterStateTableActions<ARGS>::do_stopWatchDog_cancelRequests(
    const ARGS& args)
{
    do_stopWatchDog(args);
    do_cancelRequests(args);
}

template <typename ARGS>
void ClusterStateTableActions<
    ARGS>::do_stopWatchDog_cancelRequests_reapplyEvent(const ARGS& args)
{
    do_stopWatchDog(args);
    do_cancelRequests(args);
    do_reapplyEvent(args);
}

template <typename ARGS>
void ClusterStateTableActions<ARGS>::do_stopWatchDog_initializeQueueKeyInfoMap(
    const ARGS& args)
{
    do_stopWatchDog(args);
    do_initializeQueueKeyInfoMap(args);
}

template <typename ARGS>
void ClusterStateTableActions<
    ARGS>::do_stopWatchDog_cleanupLSNs_cancelRequests(const ARGS& args)
{
    do_stopWatchDog(args);
    do_cleanupLSNs(args);
    do_cancelRequests(args);
}

template <typename ARGS>
void ClusterStateTableActions<ARGS>::
    do_stopWatchDog_cleanupLSNs_cancelRequests_reapplyEvent(const ARGS& args)
{
    do_stopWatchDog(args);
    do_cleanupLSNs(args);
    do_cancelRequests(args);
    do_reapplyEvent(args);
}

template <typename ARGS>
void ClusterStateTableActions<ARGS>::do_cleanupLSNs_reapplyEvent(
    const ARGS& args)
{
    do_cleanupLSNs(args);
    do_reapplyEvent(args);
}

template <typename ARGS>
void ClusterStateTableActions<ARGS>::do_cancelRequests_reapplySelectFollower(
    const ARGS& args)
{
    do_cancelRequests(args);
    do_reapplySelectFollower(args);
}

template <typename ARGS>
void ClusterStateTableActions<
    ARGS>::do_cleanupLSNs_cancelRequests_reapplySelectLeader(const ARGS& args)
{
    do_cleanupLSNs(args);
    do_cancelRequests(args);
    do_reapplySelectLeader(args);
}

template <typename ARGS>
void ClusterStateTableActions<
    ARGS>::do_sendFollowerLSNResponse_logErrorLeaderNotHealed(const ARGS& args)
{
    do_sendFollowerLSNResponse(args);
    do_logErrorLeaderNotHealed(args);
}

template <typename ARGS>
void ClusterStateTableActions<ARGS>::
    do_sendFollowerClusterStateResponse_logErrorLeaderNotHealed(
        const ARGS& args)
{
    do_sendFollowerClusterStateResponse(args);
    do_logErrorLeaderNotHealed(args);
}

template <typename ARGS>
void ClusterStateTableActions<ARGS>::do_storeFollowerLSNs_checkLSNQuorum(
    const ARGS& args)
{
    do_storeFollowerLSNs(args);
    do_checkLSNQuorum(args);
}

template <typename ARGS>
void ClusterStateTableActions<ARGS>::
    do_storeFollowerLSNs_checkLSNQuorum_sendRegistrationResponse(
        const ARGS& args)
{
    do_storeFollowerLSNs(args);
    do_checkLSNQuorum(args);
    do_sendRegistrationResponse(args);
}

template <typename ARGS>
void ClusterStateTableActions<
    ARGS>::do_storeFollowerLSNs_sendRegistrationResponse(const ARGS& args)
{
    do_storeFollowerLSNs(args);
    do_sendRegistrationResponse(args);
}

template <typename ARGS>
void ClusterStateTableActions<ARGS>::
    do_logFailFollowerClusterStateResponse_removeFollowerLSN_checkLSNQuorum(
        const ARGS& args)
{
    do_logFailFollowerClusterStateResponse(args);
    do_removeFollowerLSN(args);
    do_checkLSNQuorum(args);
}

template <typename ARGS>
void ClusterStateTableActions<ARGS>::do_removeFollowerLSN_checkLSNQuorum(
    const ARGS& args)
{
    do_removeFollowerLSN(args);
    do_checkLSNQuorum(args);
}

template <typename ARGS>
void ClusterStateTableActions<ARGS>::do_sendRegistrationResponse_applyCSLSelf(
    const ARGS& args)
{
    do_sendRegistrationResponse(args);
    do_applyCSLSelf(args);
}

}  // close package namespace

// -----------------------------
// struct ClusterStateTableState
// -----------------------------

// FREE OPERATORS
inline bsl::ostream& mqbc::operator<<(bsl::ostream& stream,
                                      mqbc::ClusterStateTableState::Enum value)
{
    return ClusterStateTableState::print(stream, value, 0, -1);
}

// -----------------------------
// struct ClusterStateTableEvent
// -----------------------------

// FREE OPERATORS
inline bsl::ostream& mqbc::operator<<(bsl::ostream& stream,
                                      mqbc::ClusterStateTableEvent::Enum value)
{
    return ClusterStateTableEvent::print(stream, value, 0, -1);
}

}  // close enterprise namespace

#endif
