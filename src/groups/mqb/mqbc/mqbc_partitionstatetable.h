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

// mqbc_partitionstatetable.h                                         -*-C++-*-
#ifndef INCLUDED_MQBC_PARTITIONSTATETABLE
#define INCLUDED_MQBC_PARTITIONSTATETABLE

/// @file mqbc_partitionstatetable.h
///
/// @brief Provide a state table for the Partition FSM.
///
/// @bbref{mqbc::PartitionStateTable} is a state table for the Partition FSM.
///
/// Thread Safety                            {#mqbc_partitionstatetable_thread}
/// =============
///
/// The @bbref{mqbc::PartitionStateTable} object is not thread safe.

// MQB
#include <mqbu_statetable.h>

// BDE
#include <ball_log.h>
#include <bsl_iostream.h>
#include <bsl_string.h>

namespace BloombergLP {
namespace mqbc {

// ===============================
// struct PartitionStateTableState
// ===============================

/// This struct defines the type of state in the cluster state table.
struct PartitionStateTableState {
    // TYPES

    /// Enumeration used to distinguish among different type of state.
    enum Enum {
        /// The primary is unknown.
        e_UNKNOWN = 0,

        /// Self is in primary healing stage 1, which collects sequence
        /// numbers from replicas through state requests/responses to determine
        /// the most up-to-date node.
        e_PRIMARY_HEALING_STG1 = 1,

        /// Self is in primary healing stage 2, which synchronizes self with
        /// the most up-to-date node if self is behind, then sychronizes data
        /// with all replicas.
        e_PRIMARY_HEALING_STG2 = 2,

        /// Self is healed primary, which guarantees that a quorum of nodes
        /// have synchronized their storage data.
        e_PRIMARY_HEALED = 3,

        /// Self is replica, waiting for either success of failure
        /// PrimaryStateResponse.  It **must** reject the ReplicaStateRequest
        /// to prevent primary from healing us twice in a row, leading to
        /// duplicate work.
        e_REPLICA_WAITING = 4,

        /// Self is healing replica, following instructions from primary to
        /// synchronize its storage data.
        e_REPLICA_HEALING = 5,

        /// Self is healed replica, which guarantees that self has synchronized
        /// its storage data with the primary.
        e_REPLICA_HEALED = 6,

        /// Self is stopping.  Self could be either primary or replica, or
        /// replica might be unknown.
        e_STOPPED = 7,

        /// **NOT A VALID STATE**.  This is an artificial enum value to
        /// represent the number of states.
        e_NUM_STATES = 8
    };

    // CLASS METHODS

    /// Write the string representation of the specified enumeration `value` to
    /// the specified output `stream`, and return a reference to `stream`.
    /// Optionally specify an initial indentation `level`, whose absolute value
    /// is incremented recursively for nested objects.  If `level` is
    /// specified, optionally specify `spacesPerLevel`, whose absolute value
    /// indicates the number of spaces per indentation level for this and all
    /// of its nested objects.  If `level` is negative, suppress indentation of
    /// the first line.  If `spacesPerLevel` is negative, format the entire
    /// output on one line, suppressing all but the initial indentation (as
    /// governed by `level`).  See `toAscii` for what constitutes the string
    /// representation of a @bbref{PartitionStateTableState::Enum} value.
    static bsl::ostream& print(bsl::ostream&                  stream,
                               PartitionStateTableState::Enum value,
                               int                            level = 0,
                               int spacesPerLevel                   = 4);

    /// Return the non-modifiable string representation corresponding to the
    /// specified enumeration `value`, if it exists, and a unique (error)
    /// string otherwise.  The string representation of `value` matches its
    /// corresponding enumerator name with the `e_` prefix eluded.  Note
    /// that specifying a `value` that does not match any of the enumerators
    /// will result in a string representation that is distinct from any of
    /// those corresponding to the enumerators, but is otherwise
    /// unspecified.
    static const char* toAscii(PartitionStateTableState::Enum value);
};

// FREE OPERATORS

/// Format the specified `value` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream&                  stream,
                         PartitionStateTableState::Enum value);

// ===============================
// struct PartitionStateTableEvent
// ===============================

/// This struct defines the type of event in the cluster state table.
struct PartitionStateTableEvent {
    // TYPES

    /// Enumeration used to distinguish among different type of event.
    enum Enum {
        /// This event can *only* be generated by the Cluster FSM, currently
        /// only possible via `mqbc::ClusterStateManager::markOrphan`.
        e_RST_UNKNOWN = 0,

        /// This event can *only* be generated by the Cluster FSM or self.
        e_DETECT_SELF_PRIMARY = 1,

        /// This event can *only* be generated by the Cluster FSM or self.
        e_DETECT_SELF_REPLICA          = 2,
        e_REPLICA_STATE_RQST           = 3,
        e_REPLICA_STATE_RSPN           = 4,
        e_FAIL_REPLICA_STATE_RSPN      = 5,
        e_PRIMARY_STATE_RQST           = 6,
        e_PRIMARY_STATE_RSPN           = 7,
        e_FAIL_PRIMARY_STATE_RSPN      = 8,
        e_REPLICA_DATA_RQST_PULL       = 9,
        e_REPLICA_DATA_RQST_PUSH       = 10,
        e_REPLICA_DATA_RQST_DROP       = 11,
        e_REPLICA_DATA_RSPN_PULL       = 12,
        e_REPLICA_DATA_RSPN_PUSH       = 13,
        e_REPLICA_DATA_RSPN_DROP       = 14,
        e_FAIL_REPLICA_DATA_RSPN_PULL  = 15,
        e_CRASH_REPLICA_DATA_RSPN_PULL = 16,
        e_FAIL_REPLICA_DATA_RSPN_PUSH  = 17,
        e_FAIL_REPLICA_DATA_RSPN_DROP  = 18,
        e_DONE_SENDING_DATA_CHUNKS     = 19,
        e_ERROR_SENDING_DATA_CHUNKS    = 20,
        e_DONE_RECEIVING_DATA_CHUNKS   = 21,
        e_ERROR_RECEIVING_DATA_CHUNKS  = 22,
        e_RECOVERY_DATA                = 23,
        e_LIVE_DATA                    = 24,
        e_QUORUM_REPLICA_DATA_RSPN     = 25,
        e_ISSUE_LIVESTREAM             = 26,
        e_QUORUM_REPLICA_SEQ           = 27,
        e_SELF_HIGHEST_SEQ             = 28,
        e_REPLICA_HIGHEST_SEQ          = 29,
        e_WATCH_DOG                    = 30,

        /// This event can *only* be generated by the Cluster FSM.
        e_STOP_NODE  = 31,
        e_NUM_EVENTS = 32
    };

    // CLASS METHODS

    /// Write the string representation of the specified enumeration `value` to
    /// the specified output `stream`, and return a reference to `stream`.
    /// Optionally specify an initial indentation `level`, whose absolute value
    /// ieventremented recursively for nested objects.  If `level` is
    /// specified, optionally specify `spacesPerLevel`, whose absolute value
    /// indicates the number of spaces per indentation level for this and all
    /// of its nested objects.  If `level` is negative, suppress indentation of
    /// the first line.  If `spacesPerLevel` is negative, format the entire
    /// output on one line, suppressing all but the initial indentation (as
    /// governed by `level`).  See `toAscii` for what constitutes the string
    /// representation of a @bbref{PartitionStateTableEvent::Enum} value.
    static bsl::ostream& print(bsl::ostream&                  stream,
                               PartitionStateTableEvent::Enum value,
                               int                            level = 0,
                               int spacesPerLevel                   = 4);

    /// Return the non-modifiable string representation corresponding to the
    /// specified enumeration `value`, if it exists, and a unique (error)
    /// string otherwise.  The string representation of `value` matches its
    /// corresponding enumerator name with the `e_` prefix elided.  Note
    /// that specifying a `value` that does not match any of the enumerators
    /// will result in a string representation that is distinct from any of
    /// those corresponding to the enumerators, but is otherwise
    /// unspecified.
    static const char* toAscii(PartitionStateTableEvent::Enum value);
};

// FREE OPERATORS

/// Format the specified `value` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream&                  stream,
                         PartitionStateTableEvent::Enum value);

// ================================
// class PartitionStateTableActions
// ================================

/// This class defines the actions in the partition state table.
template <typename ARGS>
class PartitionStateTableActions {
  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBC.PARTITIONSTATETABLEACTIONS");

  public:
    // TYPES
    typedef void (PartitionStateTableActions<ARGS>::*ActionFunctor)(
        ARGS& args);

  public:
    virtual ~PartitionStateTableActions();

    virtual void do_none(ARGS& args);

    virtual void do_startWatchDog(ARGS& args) = 0;

    virtual void do_stopWatchDog(ARGS& args) = 0;

    virtual void do_openRecoveryFileSet(ARGS& args) = 0;

    virtual void do_closeRecoveryFileSet(ARGS& args) = 0;

    virtual void do_storeSelfSeq(ARGS& args) = 0;

    virtual void do_storePrimarySeq(ARGS& args) = 0;

    virtual void do_storeReplicaSeq(ARGS& args) = 0;

    virtual void do_replicaStateRequest(ARGS& args) = 0;

    virtual void do_replicaStateResponse(ARGS& args) = 0;

    virtual void do_failureReplicaStateResponse(ARGS& args) = 0;

    virtual void do_logFailureReplicaStateResponse(ARGS& args) = 0;

    virtual void do_logFailurePrimaryStateResponse(ARGS& args) = 0;

    virtual void do_logUnexpectedPrimaryStateResponse(ARGS& args) = 0;

    virtual void do_logUnexpectedFailurePrimaryStateResponse(ARGS& args) = 0;

    virtual void do_primaryStateRequest(ARGS& args) = 0;

    virtual void do_primaryStateResponse(ARGS& args) = 0;

    virtual void do_failurePrimaryStateResponse(ARGS& args) = 0;

    virtual void do_replicaDataRequestPush(ARGS& args) = 0;

    virtual void do_replicaDataResponsePush(ARGS& args) = 0;

    virtual void do_replicaDataRequestDrop(ARGS& args) = 0;

    virtual void do_replicaDataResponseDrop(ARGS& args) = 0;

    virtual void do_replicaDataRequestPull(ARGS& args) = 0;

    virtual void do_replicaDataResponsePull(ARGS& args) = 0;

    virtual void do_failureReplicaDataResponsePull(ARGS& args) = 0;

    virtual void do_failureReplicaDataResponsePush(ARGS& args) = 0;

    virtual void do_bufferLiveData(ARGS& args) = 0;

    virtual void do_processBufferedLiveData(ARGS& args) = 0;

    virtual void do_clearBufferedLiveData(ARGS& args) = 0;

    virtual void do_processBufferedPrimaryStatusAdvisories(ARGS& args) = 0;

    virtual void do_processLiveData(ARGS& args) = 0;

    virtual void do_setPrimary(ARGS& args) = 0;

    virtual void do_cleanupMetadata(ARGS& args) = 0;

    virtual void do_startSendDataChunks(ARGS& args) = 0;

    virtual void do_setExpectedDataChunkRange(ARGS& args) = 0;

    virtual void do_resetReceiveDataCtx(ARGS& args) = 0;

    virtual void do_attemptOpenStorage(ARGS& args) = 0;

    virtual void do_updateStorage(ARGS& args) = 0;

    virtual void do_removeStorage(ARGS& args) = 0;

    virtual void do_incrementNumRplcaDataRspn(ARGS& args) = 0;

    virtual void do_checkQuorumRplcaDataRspn(ARGS& args) = 0;

    virtual void do_reapplyEvent(ARGS& args) = 0;

    virtual void do_checkQuorumSeq(ARGS& args) = 0;

    virtual void do_findHighestSeq(ARGS& args) = 0;

    virtual void do_flagFailedReplicaSeq(ARGS& args) = 0;

    virtual void do_transitionToActivePrimary(ARGS& args) = 0;

    virtual void do_reapplyDetectSelfPrimary(ARGS& args) = 0;

    virtual void do_reapplyDetectSelfReplica(ARGS& args) = 0;

    virtual void do_unsupportedPrimaryDowngrade(ARGS& args) = 0;

    /// Execute a compile-time chain of action functions sequentially,
    /// starting from `args.d_chainResumeIndex`.  If any action sets
    /// `args.d_isFreezeRequested`, the resume index is saved and execution
    /// stops.
    template <ActionFunctor... Fns>
    void executeChain(ARGS& args);
};

// =========================
// class PartitionStateTable
// =========================

/// This class is a state table for the Partition FSM.
template <typename ARGS>
class PartitionStateTable
: public mqbu::StateTable<
      PartitionStateTableState::e_NUM_STATES,
      PartitionStateTableEvent::e_NUM_EVENTS,
      typename PartitionStateTableActions<ARGS>::ActionFunctor> {
  public:
    // TYPES
    typedef PartitionStateTableState State;
    typedef PartitionStateTableEvent Event;
    typedef
        typename PartitionStateTableActions<ARGS>::ActionFunctor ActionFunctor;
    typedef mqbu::
        StateTable<State::e_NUM_STATES, Event::e_NUM_EVENTS, ActionFunctor>
                                       Table;
    typedef typename Table::Transition Transition;

  public:
    // CREATORS
    PartitionStateTable()
    : Table(&PartitionStateTableActions<ARGS>::do_none)
    {
        typedef PartitionStateTableActions<ARGS> A;

#define PST_CFG(s, e, a, n)                                                   \
    Table::configure(State::e_##s,                                            \
                     Event::e_##e,                                            \
                     Transition(State::e_##n, &A::do_##a));

#define PST_CHAIN(s, e, n, ...)                                               \
    Table::configure(State::e_##s,                                            \
                     Event::e_##e,                                            \
                     Transition(State::e_##n,                                 \
                                &A::template executeChain<__VA_ARGS__>));

        // ===== UNKNOWN =====
        PST_CHAIN(UNKNOWN,
                  DETECT_SELF_PRIMARY,
                  PRIMARY_HEALING_STG1,
                  &A::do_setPrimary,
                  &A::do_startWatchDog,
                  &A::do_openRecoveryFileSet,
                  &A::do_storeSelfSeq,
                  &A::do_replicaStateRequest,
                  &A::do_checkQuorumSeq);
        PST_CHAIN(UNKNOWN,
                  DETECT_SELF_REPLICA,
                  REPLICA_WAITING,
                  &A::do_setPrimary,
                  &A::do_startWatchDog,
                  &A::do_openRecoveryFileSet,
                  &A::do_storeSelfSeq,
                  &A::do_primaryStateRequest);
        PST_CFG(UNKNOWN,
                PRIMARY_STATE_RQST,
                failurePrimaryStateResponse,
                UNKNOWN);
        PST_CFG(UNKNOWN, STOP_NODE, none, STOPPED);

        // ===== PRIMARY_HEALING_STG1 =====
        PST_CFG(PRIMARY_HEALING_STG1,
                DETECT_SELF_REPLICA,
                unsupportedPrimaryDowngrade,
                UNKNOWN);
        PST_CFG(PRIMARY_HEALING_STG1,
                REPLICA_STATE_RQST,
                failureReplicaStateResponse,
                PRIMARY_HEALING_STG1);
        PST_CHAIN(PRIMARY_HEALING_STG1,
                  REPLICA_STATE_RSPN,
                  PRIMARY_HEALING_STG1,
                  &A::do_storeReplicaSeq,
                  &A::do_checkQuorumSeq);
        PST_CFG(PRIMARY_HEALING_STG1,
                FAIL_REPLICA_STATE_RSPN,
                logFailureReplicaStateResponse,
                PRIMARY_HEALING_STG1);
        PST_CHAIN(PRIMARY_HEALING_STG1,
                  PRIMARY_STATE_RQST,
                  PRIMARY_HEALING_STG1,
                  &A::do_storeReplicaSeq,
                  &A::do_primaryStateResponse,
                  &A::do_checkQuorumSeq);
        PST_CFG(PRIMARY_HEALING_STG1,
                QUORUM_REPLICA_SEQ,
                findHighestSeq,
                PRIMARY_HEALING_STG1);
        PST_CHAIN(PRIMARY_HEALING_STG1,
                  SELF_HIGHEST_SEQ,
                  PRIMARY_HEALING_STG2,
                  &A::do_closeRecoveryFileSet,
                  &A::do_attemptOpenStorage,
                  &A::do_replicaDataRequestPush,
                  &A::do_replicaDataRequestDrop,
                  &A::do_startSendDataChunks,
                  &A::do_incrementNumRplcaDataRspn,
                  &A::do_checkQuorumRplcaDataRspn);
        PST_CHAIN(PRIMARY_HEALING_STG1,
                  REPLICA_HIGHEST_SEQ,
                  PRIMARY_HEALING_STG2,
                  &A::do_setExpectedDataChunkRange,
                  &A::do_replicaDataRequestPull);
        PST_CHAIN(PRIMARY_HEALING_STG1,
                  RST_UNKNOWN,
                  UNKNOWN,
                  &A::do_cleanupMetadata,
                  &A::do_closeRecoveryFileSet,
                  &A::do_stopWatchDog);
        PST_CHAIN(PRIMARY_HEALING_STG1,
                  STOP_NODE,
                  STOPPED,
                  &A::do_cleanupMetadata,
                  &A::do_closeRecoveryFileSet,
                  &A::do_stopWatchDog);
        PST_CHAIN(PRIMARY_HEALING_STG1,
                  WATCH_DOG,
                  UNKNOWN,
                  &A::do_reapplyDetectSelfPrimary,
                  &A::do_cleanupMetadata,
                  &A::do_closeRecoveryFileSet);

        // ===== PRIMARY_HEALING_STG2 =====
        PST_CFG(PRIMARY_HEALING_STG2,
                DETECT_SELF_REPLICA,
                unsupportedPrimaryDowngrade,
                UNKNOWN);
        PST_CHAIN(PRIMARY_HEALING_STG2,
                  FAIL_REPLICA_DATA_RSPN_PULL,
                  PRIMARY_HEALING_STG1,
                  &A::do_resetReceiveDataCtx,
                  &A::do_flagFailedReplicaSeq,
                  &A::do_checkQuorumSeq);
        PST_CHAIN(PRIMARY_HEALING_STG2,
                  CRASH_REPLICA_DATA_RSPN_PULL,
                  PRIMARY_HEALING_STG1,
                  &A::do_resetReceiveDataCtx,
                  &A::do_flagFailedReplicaSeq,
                  &A::do_checkQuorumSeq);
        PST_CHAIN(PRIMARY_HEALING_STG2,
                  REPLICA_STATE_RSPN,
                  PRIMARY_HEALING_STG2,
                  &A::do_storeReplicaSeq,
                  &A::do_replicaDataRequestPush,
                  &A::do_replicaDataRequestDrop,
                  &A::do_startSendDataChunks);
        PST_CFG(PRIMARY_HEALING_STG2,
                FAIL_REPLICA_STATE_RSPN,
                logFailureReplicaStateResponse,
                PRIMARY_HEALING_STG2);
        PST_CHAIN(PRIMARY_HEALING_STG2,
                  PRIMARY_STATE_RQST,
                  PRIMARY_HEALING_STG2,
                  &A::do_storeReplicaSeq,
                  &A::do_primaryStateResponse,
                  &A::do_replicaDataRequestPush,
                  &A::do_replicaDataRequestDrop,
                  &A::do_startSendDataChunks);
        PST_CFG(PRIMARY_HEALING_STG2,
                RECOVERY_DATA,
                updateStorage,
                PRIMARY_HEALING_STG2);
        PST_CHAIN(PRIMARY_HEALING_STG2,
                  ERROR_RECEIVING_DATA_CHUNKS,
                  UNKNOWN,
                  &A::do_reapplyDetectSelfPrimary,
                  &A::do_cleanupMetadata,
                  &A::do_closeRecoveryFileSet);
        PST_CHAIN(PRIMARY_HEALING_STG2,
                  REPLICA_DATA_RSPN_PULL,
                  PRIMARY_HEALING_STG2,
                  &A::do_resetReceiveDataCtx,
                  &A::do_closeRecoveryFileSet,
                  &A::do_storeSelfSeq,
                  &A::do_attemptOpenStorage,
                  &A::do_replicaDataRequestPush,
                  &A::do_replicaDataRequestDrop,
                  &A::do_startSendDataChunks,
                  &A::do_incrementNumRplcaDataRspn,
                  &A::do_checkQuorumRplcaDataRspn);
        PST_CHAIN(PRIMARY_HEALING_STG2,
                  REPLICA_DATA_RSPN_PUSH,
                  PRIMARY_HEALING_STG2,
                  &A::do_incrementNumRplcaDataRspn,
                  &A::do_checkQuorumRplcaDataRspn);
        PST_CHAIN(PRIMARY_HEALING_STG2,
                  QUORUM_REPLICA_DATA_RSPN,
                  PRIMARY_HEALED,
                  &A::do_stopWatchDog,
                  &A::do_transitionToActivePrimary);
        PST_CHAIN(PRIMARY_HEALING_STG2,
                  RST_UNKNOWN,
                  UNKNOWN,
                  &A::do_cleanupMetadata,
                  &A::do_closeRecoveryFileSet,
                  &A::do_stopWatchDog);
        PST_CHAIN(PRIMARY_HEALING_STG2,
                  WATCH_DOG,
                  UNKNOWN,
                  &A::do_reapplyDetectSelfPrimary,
                  &A::do_cleanupMetadata,
                  &A::do_closeRecoveryFileSet);
        PST_CHAIN(PRIMARY_HEALING_STG2,
                  STOP_NODE,
                  STOPPED,
                  &A::do_cleanupMetadata,
                  &A::do_closeRecoveryFileSet,
                  &A::do_stopWatchDog);

        // ===== REPLICA_WAITING =====
        PST_CHAIN(REPLICA_WAITING,
                  DETECT_SELF_PRIMARY,
                  UNKNOWN,
                  &A::do_cleanupMetadata,
                  &A::do_stopWatchDog,
                  &A::do_reapplyEvent);
        PST_CHAIN(REPLICA_WAITING,
                  DETECT_SELF_REPLICA,
                  UNKNOWN,
                  &A::do_cleanupMetadata,
                  &A::do_stopWatchDog,
                  &A::do_reapplyEvent);
        PST_CFG(REPLICA_WAITING,
                REPLICA_STATE_RQST,
                failureReplicaStateResponse,
                REPLICA_WAITING);
        PST_CFG(REPLICA_WAITING,
                PRIMARY_STATE_RQST,
                failurePrimaryStateResponse,
                REPLICA_WAITING);
        PST_CFG(REPLICA_WAITING,
                PRIMARY_STATE_RSPN,
                storePrimarySeq,
                REPLICA_HEALING);
        PST_CFG(REPLICA_WAITING,
                FAIL_PRIMARY_STATE_RSPN,
                logFailurePrimaryStateResponse,
                REPLICA_HEALING);
        PST_CHAIN(REPLICA_WAITING,
                  RST_UNKNOWN,
                  UNKNOWN,
                  &A::do_cleanupMetadata,
                  &A::do_closeRecoveryFileSet,
                  &A::do_stopWatchDog);
        PST_CHAIN(REPLICA_WAITING,
                  WATCH_DOG,
                  UNKNOWN,
                  &A::do_reapplyDetectSelfReplica,
                  &A::do_cleanupMetadata,
                  &A::do_closeRecoveryFileSet);
        PST_CHAIN(REPLICA_WAITING,
                  STOP_NODE,
                  STOPPED,
                  &A::do_cleanupMetadata,
                  &A::do_closeRecoveryFileSet,
                  &A::do_stopWatchDog);

        // ===== REPLICA_HEALING =====
        PST_CHAIN(REPLICA_HEALING,
                  DETECT_SELF_PRIMARY,
                  UNKNOWN,
                  &A::do_cleanupMetadata,
                  &A::do_stopWatchDog,
                  &A::do_reapplyEvent);
        PST_CHAIN(REPLICA_HEALING,
                  DETECT_SELF_REPLICA,
                  UNKNOWN,
                  &A::do_cleanupMetadata,
                  &A::do_stopWatchDog,
                  &A::do_reapplyEvent);
        PST_CHAIN(REPLICA_HEALING,
                  REPLICA_STATE_RQST,
                  REPLICA_HEALING,
                  &A::do_storePrimarySeq,
                  &A::do_replicaStateResponse);
        PST_CFG(REPLICA_HEALING,
                PRIMARY_STATE_RQST,
                failurePrimaryStateResponse,
                REPLICA_HEALING);
        PST_CFG(REPLICA_HEALING,
                PRIMARY_STATE_RSPN,
                logUnexpectedPrimaryStateResponse,
                REPLICA_HEALING);
        PST_CFG(REPLICA_HEALING,
                FAIL_PRIMARY_STATE_RSPN,
                logUnexpectedFailurePrimaryStateResponse,
                REPLICA_HEALING);
        PST_CHAIN(REPLICA_HEALING,
                  REPLICA_DATA_RQST_PULL,
                  REPLICA_HEALING,
                  &A::do_closeRecoveryFileSet,
                  &A::do_attemptOpenStorage,
                  &A::do_startSendDataChunks);
        PST_CHAIN(REPLICA_HEALING,
                  DONE_SENDING_DATA_CHUNKS,
                  REPLICA_HEALED,
                  &A::do_replicaDataResponsePull,
                  &A::do_processBufferedLiveData,
                  &A::do_processBufferedPrimaryStatusAdvisories,
                  &A::do_stopWatchDog);
        PST_CHAIN(REPLICA_HEALING,
                  ERROR_SENDING_DATA_CHUNKS,
                  UNKNOWN,
                  &A::do_failureReplicaDataResponsePull,
                  &A::do_reapplyDetectSelfReplica,
                  &A::do_cleanupMetadata,
                  &A::do_closeRecoveryFileSet);
        PST_CHAIN(REPLICA_HEALING,
                  REPLICA_DATA_RQST_PUSH,
                  REPLICA_HEALING,
                  &A::do_setExpectedDataChunkRange,
                  &A::do_clearBufferedLiveData);
        PST_CHAIN(REPLICA_HEALING,
                  REPLICA_DATA_RQST_DROP,
                  REPLICA_HEALING,
                  &A::do_replicaDataResponseDrop,
                  &A::do_removeStorage,
                  &A::do_reapplyDetectSelfReplica);
        PST_CFG(REPLICA_HEALING,
                RECOVERY_DATA,
                updateStorage,
                REPLICA_HEALING);
        PST_CHAIN(REPLICA_HEALING,
                  DONE_RECEIVING_DATA_CHUNKS,
                  REPLICA_HEALED,
                  &A::do_replicaDataResponsePush,
                  &A::do_resetReceiveDataCtx,
                  &A::do_closeRecoveryFileSet,
                  &A::do_attemptOpenStorage,
                  &A::do_processBufferedLiveData,
                  &A::do_processBufferedPrimaryStatusAdvisories,
                  &A::do_stopWatchDog);
        PST_CHAIN(REPLICA_HEALING,
                  ERROR_RECEIVING_DATA_CHUNKS,
                  UNKNOWN,
                  &A::do_failureReplicaDataResponsePush,
                  &A::do_reapplyDetectSelfReplica,
                  &A::do_cleanupMetadata,
                  &A::do_closeRecoveryFileSet);
        PST_CFG(REPLICA_HEALING, LIVE_DATA, bufferLiveData, REPLICA_HEALING);
        PST_CHAIN(REPLICA_HEALING,
                  RST_UNKNOWN,
                  UNKNOWN,
                  &A::do_cleanupMetadata,
                  &A::do_closeRecoveryFileSet,
                  &A::do_stopWatchDog);
        PST_CHAIN(REPLICA_HEALING,
                  WATCH_DOG,
                  UNKNOWN,
                  &A::do_reapplyDetectSelfReplica,
                  &A::do_cleanupMetadata,
                  &A::do_closeRecoveryFileSet);
        PST_CHAIN(REPLICA_HEALING,
                  STOP_NODE,
                  STOPPED,
                  &A::do_cleanupMetadata,
                  &A::do_closeRecoveryFileSet,
                  &A::do_stopWatchDog);

        // ===== REPLICA_HEALED =====
        PST_CHAIN(REPLICA_HEALED,
                  DETECT_SELF_PRIMARY,
                  UNKNOWN,
                  &A::do_cleanupMetadata,
                  &A::do_reapplyEvent);
        PST_CHAIN(REPLICA_HEALED,
                  DETECT_SELF_REPLICA,
                  UNKNOWN,
                  &A::do_cleanupMetadata,
                  &A::do_reapplyEvent);
        PST_CFG(REPLICA_HEALED,
                REPLICA_STATE_RQST,
                replicaStateResponse,
                REPLICA_HEALED);
        PST_CFG(REPLICA_HEALED,
                FAIL_PRIMARY_STATE_RSPN,
                logFailurePrimaryStateResponse,
                REPLICA_HEALED);
        PST_CFG(REPLICA_HEALED,
                REPLICA_DATA_RQST_PUSH,
                replicaDataResponsePush,
                REPLICA_HEALED);
        PST_CFG(REPLICA_HEALED, LIVE_DATA, processLiveData, REPLICA_HEALED);
        PST_CHAIN(REPLICA_HEALED,
                  ISSUE_LIVESTREAM,
                  UNKNOWN,
                  &A::do_reapplyDetectSelfReplica,
                  &A::do_cleanupMetadata);
        PST_CFG(REPLICA_HEALED, RST_UNKNOWN, cleanupMetadata, UNKNOWN);
        PST_CFG(REPLICA_HEALED, STOP_NODE, cleanupMetadata, STOPPED);

        // ===== PRIMARY_HEALED =====
        PST_CFG(PRIMARY_HEALED,
                DETECT_SELF_REPLICA,
                unsupportedPrimaryDowngrade,
                UNKNOWN);
        PST_CHAIN(PRIMARY_HEALED,
                  REPLICA_STATE_RSPN,
                  PRIMARY_HEALED,
                  &A::do_storeSelfSeq,
                  &A::do_storeReplicaSeq,
                  &A::do_replicaDataRequestPush,
                  &A::do_replicaDataRequestDrop,
                  &A::do_startSendDataChunks);
        PST_CFG(PRIMARY_HEALED,
                FAIL_REPLICA_STATE_RSPN,
                logFailureReplicaStateResponse,
                PRIMARY_HEALED);
        PST_CHAIN(PRIMARY_HEALED,
                  PRIMARY_STATE_RQST,
                  PRIMARY_HEALED,
                  &A::do_storeSelfSeq,
                  &A::do_storeReplicaSeq,
                  &A::do_primaryStateResponse,
                  &A::do_replicaDataRequestPush,
                  &A::do_replicaDataRequestDrop,
                  &A::do_startSendDataChunks);
        PST_CFG(PRIMARY_HEALED, RST_UNKNOWN, cleanupMetadata, UNKNOWN);
        PST_CFG(PRIMARY_HEALED, STOP_NODE, cleanupMetadata, STOPPED);

#undef PST_CHAIN
#undef PST_CFG
    }
};

// ============================================================================
//                           INLINE DEFINITIONS
// ============================================================================

// --------------------------------
// class PartitionStateTableActions
// --------------------------------

// CREATORS
template <typename ARGS>
PartitionStateTableActions<ARGS>::~PartitionStateTableActions()
{
    // NOTHING (pure interface)
}

template <typename ARGS>
void PartitionStateTableActions<ARGS>::do_none(ARGS& args)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!args.d_data.empty());

    const int partitionId = args.d_data[0].partitionId();
    BALL_LOG_INFO << "Partition [" << partitionId << "]: NO ACTION PERFORMED.";
}

template <typename ARGS>
template <typename PartitionStateTableActions<ARGS>::ActionFunctor... Fns>
void PartitionStateTableActions<ARGS>::executeChain(ARGS& args)
{
    static BSLS_KEYWORD_CONSTEXPR ActionFunctor chain[] = {Fns...};
    for (size_t i = args.d_chainResumeIndex; i < sizeof...(Fns); ++i) {
        (this->*chain[i])(args);
        if (args.d_isFreezeRequested) {
            args.d_chainResumeIndex = i + 1;
            return;  // RETURN
        }
    }
}

}  // close package namespace

// -------------------------------
// struct PartitionStateTableState
// -------------------------------

// FREE OPERATORS
inline bsl::ostream&
mqbc::operator<<(bsl::ostream&                        stream,
                 mqbc::PartitionStateTableState::Enum value)
{
    return mqbc::PartitionStateTableState::print(stream, value, 0, -1);
}

// -------------------------------
// struct PartitionStateTableEvent
// -------------------------------

// FREE OPERATORS
inline bsl::ostream&
mqbc::operator<<(bsl::ostream&                        stream,
                 mqbc::PartitionStateTableEvent::Enum value)
{
    return mqbc::PartitionStateTableEvent::print(stream, value, 0, -1);
}

}  // close enterprise namespace

#endif
