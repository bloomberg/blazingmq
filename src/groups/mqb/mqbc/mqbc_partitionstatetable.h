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

//@PURPOSE: Provide a state table for the Partition FSM.
//
//@CLASSES:
//  mqbc::PartitionStateTableState:   Enum for state in the partition state
//                                    table.
//  mqbc::PartitionStateTableEvent:   Enum for event in the partition state
//                                    table.
//  mqbc::PartitionStateTableActions: Actions in the partition state table.
//  mqbc::PartitionStateTable:        State table for the Partition FSM.
//
//@DESCRIPTION: 'mqbc::PartitionStateTable' is a state table for the Partition
//              FSM.
//
/// Thread Safety
///-------------
// The 'mqbc::PartitionStateTable' object is not thread safe.

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
    enum Enum {
        // Enumeration used to distinguish among different type of state

        e_UNKNOWN              = 0,
        e_PRIMARY_HEALING_STG1 = 1,
        e_PRIMARY_HEALING_STG2 = 2,
        e_REPLICA_HEALING      = 3,
        e_PRIMARY_HEALED       = 4,
        e_REPLICA_HEALED       = 5,
        e_NUM_STATES           = 6
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
    /// `PartitionStateTableState::Enum` value.
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
    enum Enum {
        // Enumeration used to distinguish among different type of event

        e_RST_UNKNOWN                  = 0,
        e_DETECT_SELF_PRIMARY          = 1,
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
        e_PUT                          = 26,
        e_ISSUE_LIVESTREAM             = 27,
        e_QUORUM_REPLICA_SEQ           = 28,
        e_SELF_HIGHEST_SEQ             = 29,
        e_REPLICA_HIGHEST_SEQ          = 30,
        e_WATCH_DOG                    = 31,
        e_NUM_EVENTS                   = 32
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
    /// `PartitionStateTableEvent::Enum` value.
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
        const ARGS& args);

  public:
    virtual ~PartitionStateTableActions();

    virtual void do_none(const ARGS& args);

    virtual void do_startWatchDog(const ARGS& args) = 0;

    virtual void do_stopWatchDog(const ARGS& args) = 0;

    virtual void do_openRecoveryFileSet(const ARGS& args) = 0;

    virtual void do_closeRecoveryFileSet(const ARGS& args) = 0;

    virtual void do_storeSelfSeq(const ARGS& args) = 0;

    virtual void do_storePrimarySeq(const ARGS& args) = 0;

    virtual void do_storeReplicaSeq(const ARGS& args) = 0;

    virtual void do_storePartitionInfo(const ARGS& args) = 0;

    virtual void do_clearPartitionInfo(const ARGS& args) = 0;

    virtual void do_replicaStateRequest(const ARGS& args) = 0;

    virtual void do_replicaStateResponse(const ARGS& args) = 0;

    virtual void do_failureReplicaStateResponse(const ARGS& args) = 0;

    virtual void do_logFailureReplicaStateResponse(const ARGS& args) = 0;

    virtual void do_logFailurePrimaryStateResponse(const ARGS& args) = 0;

    virtual void do_primaryStateRequest(const ARGS& args) = 0;

    virtual void do_primaryStateResponse(const ARGS& args) = 0;

    virtual void do_failurePrimaryStateResponse(const ARGS& args) = 0;

    virtual void do_replicaDataRequestPush(const ARGS& args) = 0;

    virtual void do_replicaDataResponsePush(const ARGS& args) = 0;

    virtual void do_replicaDataRequestDrop(const ARGS& args) = 0;

    virtual void do_replicaDataRequestPull(const ARGS& args) = 0;

    virtual void do_replicaDataResponsePull(const ARGS& args) = 0;

    virtual void do_failureReplicaDataResponsePull(const ARGS& args) = 0;

    virtual void do_failureReplicaDataResponsePush(const ARGS& args) = 0;

    virtual void do_bufferLiveData(const ARGS& args) = 0;

    virtual void do_processBufferedLiveData(const ARGS& args) = 0;

    virtual void do_processLiveData(const ARGS& args) = 0;

    virtual void do_processPut(const ARGS& args) = 0;

    virtual void do_nackPut(const ARGS& args) = 0;

    virtual void do_cleanupSeqnums(const ARGS& args) = 0;

    virtual void do_startSendDataChunks(const ARGS& args) = 0;

    virtual void do_setExpectedDataChunkRange(const ARGS& args) = 0;

    virtual void do_resetReceiveDataCtx(const ARGS& args) = 0;

    virtual void do_openStorage(const ARGS& args) = 0;

    virtual void do_updateStorage(const ARGS& args) = 0;

    virtual void do_removeStorage(const ARGS& args) = 0;

    virtual void do_incrementNumRplcaDataRspn(const ARGS& args) = 0;

    virtual void do_checkQuorumRplcaDataRspn(const ARGS& args) = 0;

    virtual void do_clearRplcaDataRspnCnt(const ARGS& args) = 0;

    virtual void do_reapplyEvent(const ARGS& args) = 0;

    virtual void do_checkQuorumSeq(const ARGS& args) = 0;

    virtual void do_findHighestSeq(const ARGS& args) = 0;

    virtual void do_flagFailedReplicaSeq(const ARGS& args) = 0;

    virtual void do_transitionToActivePrimary(const ARGS& args) = 0;

    virtual void do_reapplyDetectSelfPrimary(const ARGS& args) = 0;

    virtual void do_reapplyDetectSelfReplica(const ARGS& args) = 0;

    void
    do_startWatchDog_storePartitionInfo_openRecoveryFileSet_storeSelfSeq_replicaStateRequest_checkQuorumSeq(
        const ARGS& args);

    void
    do_startWatchDog_storePartitionInfo_openRecoveryFileSet_storeSelfSeq_primaryStateRequest(
        const ARGS& args);

    void
    do_storeReplicaSeq_primaryStateResponse_checkQuorumSeq(const ARGS& args);

    void do_storeReplicaSeq_checkQuorumSeq(const ARGS& args);

    void do_storePrimarySeq_replicaStateResponse(const ARGS& args);

    void do_cleanupSeqnums_reapplyEvent(const ARGS& args);

    void do_cleanupSeqnums_stopWatchDog_reapplyEvent(const ARGS& args);

    void do_cleanupSeqnums_resetReceiveDataCtx_stopWatchDog_reapplyEvent(
        const ARGS& args);

    void do_cleanupSeqnums_clearPartitionInfo(const ARGS& args);

    void
    do_cleanupSeqnums_clearPartitionInfo_closeRecoveryFileSet_stopWatchDog(
        const ARGS& args);

    void do_cleanupSeqnums_reapplyDetectSelfPrimary(const ARGS& args);

    void do_resetReceiveDataCtx_flagFailedReplicaSeq_checkQuorumSeq(
        const ARGS& args);

    void do_resetReceiveDataCtx_closeRecoveryFileSet(const ARGS& args);

    void
    do_closeRecoveryFileSet_openStorage_startSendDataChunks(const ARGS& args);

    void
    do_closeRecoveryFileSet_openStorage_replicaDataRequestPush_startSendDataChunks_incrementNumRplcaDataRspn_checkQuorumRplcaDataRspn(
        const ARGS& args);

    void do_setExpectedDataChunkRange_replicaDataRequestPull(const ARGS& args);

    void
    do_storeSelfSeq_openStorage_replicaDataRequestPush_replicaDataRequestDrop_startSendDataChunks_incrementNumRplcaDataRspn_checkQuorumRplcaDataRspn(
        const ARGS& args);

    void
    do_processPut_stopWatchDog_transitionToActivePrimary(const ARGS& args);

    void
    do_clearRplcaDataRspnCnt_cleanupSeqnums_resetReceiveDataCtx_clearPartitionInfo_closeRecoveryFileSet_stopWatchDog(
        const ARGS& args);

    void
    do_clearRplcaDataRspnCnt_cleanupSeqnums_resetReceiveDataCtx_reapplyDetectSelfPrimary(
        const ARGS& args);

    void do_replicaStateResponse_storePrimarySeq(const ARGS& args);

    void do_cleanupSeqnums_reapplyDetectSelfReplica(const ARGS& args);

    void
    do_cleanupSeqnums_resetReceiveDataCtx_clearPartitionInfo_closeRecoveryFileSet_stopWatchDog(
        const ARGS& args);

    void do_cleanupSeqnums_resetReceiveDataCtx_reapplyDetectSelfReplica(
        const ARGS& args);

    void do_replicaDataResponsePull_processBufferedLiveData_stopWatchDog(
        const ARGS& args);

    void
    do_failureReplicaDataResponsePull_cleanupSeqnums_reapplyDetectSelfReplica(
        const ARGS& args);

    void do_cleanupSeqnums_resetReceiveDataCtx_reapplyDetectSelfPrimary(
        const ARGS& args);

    void
    do_failureReplicaDataResponsePush_cleanupSeqnums_resetReceiveDataCtx_reapplyDetectSelfReplica(
        const ARGS& args);

    void
    do_replicaDataResponsePush_resetReceiveDataCtx_closeRecoveryFileSet_openStorage_processBufferedLiveData_stopWatchDog(
        const ARGS& args);

    void
    do_storeReplicaSeq_primaryStateResponse_replicaDataRequestPush_replicaDataRequestDrop_startSendDataChunks(
        const ARGS& args);

    void
    do_storeSelfSeq_storeReplicaSeq_primaryStateResponse_replicaDataRequestPush_replicaDataRequestDrop_startSendDataChunks(
        const ARGS& args);

    void
    do_storeReplicaSeq_replicaDataRequestPush_replicaDataRequestDrop_startSendDataChunks(
        const ARGS& args);

    void
    do_storeSelfSeq_storeReplicaSeq_replicaDataRequestPush_replicaDataRequestDrop_startSendDataChunks(
        const ARGS& args);

    void
    do_incrementNumRplcaDataRspn_checkQuorumRplcaDataRspn(const ARGS& args);
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
#define PST_CFG(s, e, a, n)                                                   \
    Table::configure(State::e_##s,                                            \
                     Event::e_##e,                                            \
                     Transition(State::e_##n,                                 \
                                &PartitionStateTableActions<ARGS>::do_##a));
        //       state                 event                         action
        //       next state
        PST_CFG(
            UNKNOWN,
            DETECT_SELF_PRIMARY,
            startWatchDog_storePartitionInfo_openRecoveryFileSet_storeSelfSeq_replicaStateRequest_checkQuorumSeq,
            PRIMARY_HEALING_STG1);
        PST_CFG(
            UNKNOWN,
            DETECT_SELF_REPLICA,
            startWatchDog_storePartitionInfo_openRecoveryFileSet_storeSelfSeq_primaryStateRequest,
            REPLICA_HEALING);
        PST_CFG(UNKNOWN, PUT, nackPut, UNKNOWN);
        PST_CFG(PRIMARY_HEALING_STG1,
                DETECT_SELF_REPLICA,
                cleanupSeqnums_stopWatchDog_reapplyEvent,
                UNKNOWN);
        PST_CFG(PRIMARY_HEALING_STG1,
                REPLICA_STATE_RQST,
                failureReplicaStateResponse,
                PRIMARY_HEALING_STG1);
        PST_CFG(PRIMARY_HEALING_STG1,
                REPLICA_STATE_RSPN,
                storeReplicaSeq_checkQuorumSeq,
                PRIMARY_HEALING_STG1);
        PST_CFG(PRIMARY_HEALING_STG1,
                FAIL_REPLICA_STATE_RSPN,
                logFailureReplicaStateResponse,
                PRIMARY_HEALING_STG1);
        PST_CFG(PRIMARY_HEALING_STG1,
                PRIMARY_STATE_RQST,
                storeReplicaSeq_primaryStateResponse_checkQuorumSeq,
                PRIMARY_HEALING_STG1);
        PST_CFG(PRIMARY_HEALING_STG1, PUT, nackPut, PRIMARY_HEALING_STG1);
        PST_CFG(PRIMARY_HEALING_STG1,
                QUORUM_REPLICA_SEQ,
                findHighestSeq,
                PRIMARY_HEALING_STG1);
        PST_CFG(
            PRIMARY_HEALING_STG1,
            SELF_HIGHEST_SEQ,
            closeRecoveryFileSet_openStorage_replicaDataRequestPush_startSendDataChunks_incrementNumRplcaDataRspn_checkQuorumRplcaDataRspn,
            PRIMARY_HEALING_STG2);
        PST_CFG(PRIMARY_HEALING_STG1,
                REPLICA_HIGHEST_SEQ,
                setExpectedDataChunkRange_replicaDataRequestPull,
                PRIMARY_HEALING_STG2);
        PST_CFG(
            PRIMARY_HEALING_STG1,
            RST_UNKNOWN,
            cleanupSeqnums_clearPartitionInfo_closeRecoveryFileSet_stopWatchDog,
            UNKNOWN);
        PST_CFG(PRIMARY_HEALING_STG1,
                WATCH_DOG,
                cleanupSeqnums_reapplyDetectSelfPrimary,
                UNKNOWN);
        PST_CFG(PRIMARY_HEALING_STG2,
                DETECT_SELF_REPLICA,
                cleanupSeqnums_resetReceiveDataCtx_stopWatchDog_reapplyEvent,
                UNKNOWN);
        PST_CFG(PRIMARY_HEALING_STG2,
                FAIL_REPLICA_DATA_RSPN_PULL,
                resetReceiveDataCtx_flagFailedReplicaSeq_checkQuorumSeq,
                PRIMARY_HEALING_STG1);
        PST_CFG(PRIMARY_HEALING_STG2,
                CRASH_REPLICA_DATA_RSPN_PULL,
                resetReceiveDataCtx_flagFailedReplicaSeq_checkQuorumSeq,
                PRIMARY_HEALING_STG1);
        PST_CFG(
            PRIMARY_HEALING_STG2,
            REPLICA_STATE_RSPN,
            storeReplicaSeq_replicaDataRequestPush_replicaDataRequestDrop_startSendDataChunks,
            PRIMARY_HEALING_STG2);
        PST_CFG(
            PRIMARY_HEALING_STG2,
            PRIMARY_STATE_RQST,
            storeReplicaSeq_primaryStateResponse_replicaDataRequestPush_replicaDataRequestDrop_startSendDataChunks,
            PRIMARY_HEALING_STG2)
        PST_CFG(PRIMARY_HEALING_STG2,
                RECOVERY_DATA,
                updateStorage,
                PRIMARY_HEALING_STG2);
        PST_CFG(PRIMARY_HEALING_STG2,
                DONE_RECEIVING_DATA_CHUNKS,
                resetReceiveDataCtx_closeRecoveryFileSet,
                PRIMARY_HEALING_STG2);
        PST_CFG(PRIMARY_HEALING_STG2,
                ERROR_RECEIVING_DATA_CHUNKS,
                cleanupSeqnums_resetReceiveDataCtx_reapplyDetectSelfPrimary,
                UNKNOWN);
        PST_CFG(
            PRIMARY_HEALING_STG2,
            REPLICA_DATA_RSPN_PULL,
            storeSelfSeq_openStorage_replicaDataRequestPush_replicaDataRequestDrop_startSendDataChunks_incrementNumRplcaDataRspn_checkQuorumRplcaDataRspn,
            PRIMARY_HEALING_STG2);
        PST_CFG(PRIMARY_HEALING_STG2,
                REPLICA_DATA_RSPN_PUSH,
                incrementNumRplcaDataRspn_checkQuorumRplcaDataRspn,
                PRIMARY_HEALING_STG2);
        PST_CFG(PRIMARY_HEALING_STG2,
                QUORUM_REPLICA_DATA_RSPN,
                processPut_stopWatchDog_transitionToActivePrimary,
                PRIMARY_HEALED);
        PST_CFG(PRIMARY_HEALING_STG2, PUT, nackPut, PRIMARY_HEALING_STG2);
        PST_CFG(
            PRIMARY_HEALING_STG2,
            RST_UNKNOWN,
            clearRplcaDataRspnCnt_cleanupSeqnums_resetReceiveDataCtx_clearPartitionInfo_closeRecoveryFileSet_stopWatchDog,
            UNKNOWN);
        PST_CFG(
            PRIMARY_HEALING_STG2,
            WATCH_DOG,
            clearRplcaDataRspnCnt_cleanupSeqnums_resetReceiveDataCtx_reapplyDetectSelfPrimary,
            UNKNOWN);
        PST_CFG(REPLICA_HEALING,
                DETECT_SELF_PRIMARY,
                cleanupSeqnums_resetReceiveDataCtx_stopWatchDog_reapplyEvent,
                UNKNOWN);
        PST_CFG(REPLICA_HEALING,
                DETECT_SELF_REPLICA,
                cleanupSeqnums_resetReceiveDataCtx_stopWatchDog_reapplyEvent,
                UNKNOWN);
        PST_CFG(REPLICA_HEALING,
                REPLICA_STATE_RQST,
                storePrimarySeq_replicaStateResponse,
                REPLICA_HEALING);
        PST_CFG(REPLICA_HEALING,
                PRIMARY_STATE_RQST,
                failurePrimaryStateResponse,
                REPLICA_HEALING);
        PST_CFG(REPLICA_HEALING,
                PRIMARY_STATE_RSPN,
                storePrimarySeq,
                REPLICA_HEALING);
        PST_CFG(REPLICA_HEALING,
                FAIL_PRIMARY_STATE_RSPN,
                logFailurePrimaryStateResponse,
                REPLICA_HEALING);
        PST_CFG(REPLICA_HEALING,
                REPLICA_DATA_RQST_PULL,
                closeRecoveryFileSet_openStorage_startSendDataChunks,
                REPLICA_HEALING);
        PST_CFG(REPLICA_HEALING,
                DONE_SENDING_DATA_CHUNKS,
                replicaDataResponsePull_processBufferedLiveData_stopWatchDog,
                REPLICA_HEALED);
        PST_CFG(
            REPLICA_HEALING,
            ERROR_SENDING_DATA_CHUNKS,
            failureReplicaDataResponsePull_cleanupSeqnums_reapplyDetectSelfReplica,
            UNKNOWN);
        PST_CFG(REPLICA_HEALING,
                REPLICA_DATA_RQST_PUSH,
                setExpectedDataChunkRange,
                REPLICA_HEALING);
        PST_CFG(REPLICA_HEALING,
                REPLICA_DATA_RQST_DROP,
                removeStorage,
                REPLICA_HEALING);
        PST_CFG(REPLICA_HEALING,
                RECOVERY_DATA,
                updateStorage,
                REPLICA_HEALING);
        PST_CFG(
            REPLICA_HEALING,
            DONE_RECEIVING_DATA_CHUNKS,
            replicaDataResponsePush_resetReceiveDataCtx_closeRecoveryFileSet_openStorage_processBufferedLiveData_stopWatchDog,
            REPLICA_HEALED);
        PST_CFG(
            REPLICA_HEALING,
            ERROR_RECEIVING_DATA_CHUNKS,
            failureReplicaDataResponsePush_cleanupSeqnums_resetReceiveDataCtx_reapplyDetectSelfReplica,
            UNKNOWN);
        PST_CFG(REPLICA_HEALING, LIVE_DATA, bufferLiveData, REPLICA_HEALING);
        PST_CFG(REPLICA_HEALING, PUT, nackPut, REPLICA_HEALING);
        PST_CFG(
            REPLICA_HEALING,
            RST_UNKNOWN,
            cleanupSeqnums_resetReceiveDataCtx_clearPartitionInfo_closeRecoveryFileSet_stopWatchDog,
            UNKNOWN);
        PST_CFG(REPLICA_HEALING,
                WATCH_DOG,
                cleanupSeqnums_resetReceiveDataCtx_reapplyDetectSelfReplica,
                UNKNOWN);
        PST_CFG(REPLICA_HEALED,
                DETECT_SELF_PRIMARY,
                cleanupSeqnums_reapplyEvent,
                UNKNOWN);
        PST_CFG(REPLICA_HEALED,
                DETECT_SELF_REPLICA,
                cleanupSeqnums_reapplyEvent,
                UNKNOWN);
        PST_CFG(REPLICA_HEALED,
                REPLICA_STATE_RQST,
                replicaStateResponse,
                REPLICA_HEALED);
        PST_CFG(REPLICA_HEALED,
                REPLICA_DATA_RQST_PUSH,
                replicaDataResponsePush,
                REPLICA_HEALED);
        PST_CFG(REPLICA_HEALED, LIVE_DATA, processLiveData, REPLICA_HEALED);
        PST_CFG(REPLICA_HEALED, PUT, processPut, REPLICA_HEALED);
        PST_CFG(REPLICA_HEALED,
                ISSUE_LIVESTREAM,
                cleanupSeqnums_reapplyDetectSelfReplica,
                UNKNOWN);
        PST_CFG(REPLICA_HEALED,
                RST_UNKNOWN,
                cleanupSeqnums_clearPartitionInfo,
                UNKNOWN);
        PST_CFG(PRIMARY_HEALED,
                DETECT_SELF_REPLICA,
                cleanupSeqnums_reapplyEvent,
                UNKNOWN);
        PST_CFG(
            PRIMARY_HEALED,
            REPLICA_STATE_RSPN,
            storeSelfSeq_storeReplicaSeq_replicaDataRequestPush_replicaDataRequestDrop_startSendDataChunks,
            PRIMARY_HEALED);
        PST_CFG(
            PRIMARY_HEALED,
            PRIMARY_STATE_RQST,
            storeSelfSeq_storeReplicaSeq_primaryStateResponse_replicaDataRequestPush_replicaDataRequestDrop_startSendDataChunks,
            PRIMARY_HEALED);
        PST_CFG(PRIMARY_HEALED, PUT, processPut, PRIMARY_HEALED);
        PST_CFG(PRIMARY_HEALED,
                RST_UNKNOWN,
                cleanupSeqnums_clearPartitionInfo,
                UNKNOWN);

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
void PartitionStateTableActions<ARGS>::do_none(const ARGS& args)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!args->eventsQueue()->empty());
    BSLS_ASSERT_SAFE(!args->eventsQueue()->front().second.empty());

    const int partitionId =
        args->eventsQueue()->front().second[0].partitionId();

    BALL_LOG_INFO << "Partition [" << partitionId << "]: NO ACTION PERFORMED.";
}

template <typename ARGS>
void PartitionStateTableActions<ARGS>::
    do_startWatchDog_storePartitionInfo_openRecoveryFileSet_storeSelfSeq_replicaStateRequest_checkQuorumSeq(
        const ARGS& args)
{
    do_startWatchDog(args);
    do_storePartitionInfo(args);
    do_openRecoveryFileSet(args);
    do_storeSelfSeq(args);
    do_replicaStateRequest(args);
    do_checkQuorumSeq(args);
}

template <typename ARGS>
void PartitionStateTableActions<ARGS>::
    do_startWatchDog_storePartitionInfo_openRecoveryFileSet_storeSelfSeq_primaryStateRequest(
        const ARGS& args)
{
    do_startWatchDog(args);
    do_storePartitionInfo(args);
    do_openRecoveryFileSet(args);
    do_storeSelfSeq(args);
    do_primaryStateRequest(args);
}

template <typename ARGS>
void PartitionStateTableActions<ARGS>::
    do_storeReplicaSeq_primaryStateResponse_checkQuorumSeq(const ARGS& args)
{
    do_storeReplicaSeq(args);
    do_primaryStateResponse(args);
    do_checkQuorumSeq(args);
}

template <typename ARGS>
void PartitionStateTableActions<ARGS>::do_storeReplicaSeq_checkQuorumSeq(
    const ARGS& args)
{
    do_storeReplicaSeq(args);
    do_checkQuorumSeq(args);
}

template <typename ARGS>
void PartitionStateTableActions<ARGS>::do_storePrimarySeq_replicaStateResponse(
    const ARGS& args)
{
    do_storePrimarySeq(args);
    do_replicaStateResponse(args);
}

template <typename ARGS>
void PartitionStateTableActions<ARGS>::do_cleanupSeqnums_reapplyEvent(
    const ARGS& args)
{
    do_cleanupSeqnums(args);
    do_reapplyEvent(args);
}

template <typename ARGS>
void PartitionStateTableActions<
    ARGS>::do_cleanupSeqnums_stopWatchDog_reapplyEvent(const ARGS& args)
{
    do_cleanupSeqnums(args);
    do_stopWatchDog(args);
    do_reapplyEvent(args);
}

template <typename ARGS>
void PartitionStateTableActions<ARGS>::
    do_cleanupSeqnums_resetReceiveDataCtx_stopWatchDog_reapplyEvent(
        const ARGS& args)
{
    do_cleanupSeqnums(args);
    do_resetReceiveDataCtx(args);
    do_stopWatchDog(args);
    do_reapplyEvent(args);
}

template <typename ARGS>
void PartitionStateTableActions<ARGS>::do_cleanupSeqnums_clearPartitionInfo(
    const ARGS& args)
{
    do_cleanupSeqnums(args);
    do_clearPartitionInfo(args);
}

template <typename ARGS>
void PartitionStateTableActions<ARGS>::
    do_cleanupSeqnums_clearPartitionInfo_closeRecoveryFileSet_stopWatchDog(
        const ARGS& args)
{
    do_cleanupSeqnums(args);
    do_clearPartitionInfo(args);
    do_closeRecoveryFileSet(args);
    do_stopWatchDog(args);
}

template <typename ARGS>
void PartitionStateTableActions<
    ARGS>::do_cleanupSeqnums_reapplyDetectSelfPrimary(const ARGS& args)
{
    do_cleanupSeqnums(args);
    do_reapplyDetectSelfPrimary(args);
}

template <typename ARGS>
void PartitionStateTableActions<ARGS>::
    do_resetReceiveDataCtx_flagFailedReplicaSeq_checkQuorumSeq(
        const ARGS& args)
{
    do_resetReceiveDataCtx(args);
    do_flagFailedReplicaSeq(args);
    do_checkQuorumSeq(args);
}

template <typename ARGS>
void PartitionStateTableActions<
    ARGS>::do_resetReceiveDataCtx_closeRecoveryFileSet(const ARGS& args)
{
    do_resetReceiveDataCtx(args);
    do_closeRecoveryFileSet(args);
}

template <typename ARGS>
void PartitionStateTableActions<ARGS>::
    do_closeRecoveryFileSet_openStorage_startSendDataChunks(const ARGS& args)
{
    do_closeRecoveryFileSet(args);
    do_openStorage(args);
    do_startSendDataChunks(args);
}

template <typename ARGS>
void PartitionStateTableActions<ARGS>::
    do_closeRecoveryFileSet_openStorage_replicaDataRequestPush_startSendDataChunks_incrementNumRplcaDataRspn_checkQuorumRplcaDataRspn(
        const ARGS& args)
{
    do_closeRecoveryFileSet(args);
    do_openStorage(args);
    do_replicaDataRequestPush(args);
    do_startSendDataChunks(args);
    do_incrementNumRplcaDataRspn(args);
    do_checkQuorumRplcaDataRspn(args);
}

template <typename ARGS>
void PartitionStateTableActions<ARGS>::
    do_setExpectedDataChunkRange_replicaDataRequestPull(const ARGS& args)
{
    do_setExpectedDataChunkRange(args);
    do_replicaDataRequestPull(args);
}

template <typename ARGS>
void PartitionStateTableActions<ARGS>::
    do_storeSelfSeq_openStorage_replicaDataRequestPush_replicaDataRequestDrop_startSendDataChunks_incrementNumRplcaDataRspn_checkQuorumRplcaDataRspn(
        const ARGS& args)
{
    do_storeSelfSeq(args);
    do_openStorage(args);
    do_replicaDataRequestPush(args);
    do_replicaDataRequestDrop(args);
    do_startSendDataChunks(args);
    do_incrementNumRplcaDataRspn(args);
    do_checkQuorumRplcaDataRspn(args);
}

template <typename ARGS>
void PartitionStateTableActions<ARGS>::
    do_processPut_stopWatchDog_transitionToActivePrimary(const ARGS& args)
{
    do_processPut(args);
    do_stopWatchDog(args);
    do_transitionToActivePrimary(args);
}

template <typename ARGS>
void PartitionStateTableActions<ARGS>::
    do_clearRplcaDataRspnCnt_cleanupSeqnums_resetReceiveDataCtx_clearPartitionInfo_closeRecoveryFileSet_stopWatchDog(
        const ARGS& args)
{
    do_clearRplcaDataRspnCnt(args);
    do_cleanupSeqnums(args);
    do_resetReceiveDataCtx(args);
    do_clearPartitionInfo(args);
    do_closeRecoveryFileSet(args);
    do_stopWatchDog(args);
}

template <typename ARGS>
void PartitionStateTableActions<ARGS>::
    do_clearRplcaDataRspnCnt_cleanupSeqnums_resetReceiveDataCtx_reapplyDetectSelfPrimary(
        const ARGS& args)
{
    do_clearRplcaDataRspnCnt(args);
    do_cleanupSeqnums(args);
    do_resetReceiveDataCtx(args);
    do_reapplyDetectSelfPrimary(args);
}

template <typename ARGS>
void PartitionStateTableActions<ARGS>::do_replicaStateResponse_storePrimarySeq(
    const ARGS& args)
{
    do_replicaStateResponse(args);
    do_storePrimarySeq(args);
}

template <typename ARGS>
void PartitionStateTableActions<
    ARGS>::do_cleanupSeqnums_reapplyDetectSelfReplica(const ARGS& args)
{
    do_cleanupSeqnums(args);
    do_reapplyDetectSelfReplica(args);
}

template <typename ARGS>
void PartitionStateTableActions<ARGS>::
    do_cleanupSeqnums_resetReceiveDataCtx_clearPartitionInfo_closeRecoveryFileSet_stopWatchDog(
        const ARGS& args)
{
    do_cleanupSeqnums(args);
    do_resetReceiveDataCtx(args);
    do_clearPartitionInfo(args);
    do_closeRecoveryFileSet(args);
    do_stopWatchDog(args);
}

template <typename ARGS>
void PartitionStateTableActions<ARGS>::
    do_cleanupSeqnums_resetReceiveDataCtx_reapplyDetectSelfReplica(
        const ARGS& args)
{
    do_cleanupSeqnums(args);
    do_resetReceiveDataCtx(args);
    do_reapplyDetectSelfReplica(args);
}

template <typename ARGS>
void PartitionStateTableActions<ARGS>::
    do_replicaDataResponsePull_processBufferedLiveData_stopWatchDog(
        const ARGS& args)
{
    do_replicaDataResponsePull(args);
    do_processBufferedLiveData(args);
    do_stopWatchDog(args);
}

template <typename ARGS>
void PartitionStateTableActions<ARGS>::
    do_failureReplicaDataResponsePull_cleanupSeqnums_reapplyDetectSelfReplica(
        const ARGS& args)
{
    do_failureReplicaDataResponsePull(args);
    do_cleanupSeqnums(args);
    do_reapplyDetectSelfReplica(args);
}

template <typename ARGS>
void PartitionStateTableActions<ARGS>::
    do_cleanupSeqnums_resetReceiveDataCtx_reapplyDetectSelfPrimary(
        const ARGS& args)
{
    do_cleanupSeqnums(args);
    do_resetReceiveDataCtx(args);
    do_reapplyDetectSelfPrimary(args);
}

template <typename ARGS>
void PartitionStateTableActions<ARGS>::
    do_failureReplicaDataResponsePush_cleanupSeqnums_resetReceiveDataCtx_reapplyDetectSelfReplica(
        const ARGS& args)
{
    do_failureReplicaDataResponsePush(args);
    do_cleanupSeqnums(args);
    do_resetReceiveDataCtx(args);
    do_reapplyDetectSelfReplica(args);
}

template <typename ARGS>
void PartitionStateTableActions<ARGS>::
    do_replicaDataResponsePush_resetReceiveDataCtx_closeRecoveryFileSet_openStorage_processBufferedLiveData_stopWatchDog(
        const ARGS& args)
{
    do_replicaDataResponsePush(args);
    do_resetReceiveDataCtx(args);
    do_closeRecoveryFileSet(args);
    do_openStorage(args);
    do_processBufferedLiveData(args);
    do_stopWatchDog(args);
}

template <typename ARGS>
void PartitionStateTableActions<ARGS>::
    do_storeReplicaSeq_primaryStateResponse_replicaDataRequestPush_replicaDataRequestDrop_startSendDataChunks(
        const ARGS& args)
{
    do_storeReplicaSeq(args);
    do_primaryStateResponse(args);
    do_replicaDataRequestPush(args);
    do_replicaDataRequestDrop(args);
    do_startSendDataChunks(args);
}

template <typename ARGS>
void PartitionStateTableActions<ARGS>::
    do_storeSelfSeq_storeReplicaSeq_primaryStateResponse_replicaDataRequestPush_replicaDataRequestDrop_startSendDataChunks(
        const ARGS& args)
{
    do_storeSelfSeq(args);
    do_storeReplicaSeq(args);
    do_primaryStateResponse(args);
    do_replicaDataRequestPush(args);
    do_replicaDataRequestDrop(args);
    do_startSendDataChunks(args);
}

template <typename ARGS>
void PartitionStateTableActions<ARGS>::
    do_storeReplicaSeq_replicaDataRequestPush_replicaDataRequestDrop_startSendDataChunks(
        const ARGS& args)
{
    do_storeReplicaSeq(args);
    do_replicaDataRequestPush(args);
    do_replicaDataRequestDrop(args);
    do_startSendDataChunks(args);
}

template <typename ARGS>
void PartitionStateTableActions<ARGS>::
    do_storeSelfSeq_storeReplicaSeq_replicaDataRequestPush_replicaDataRequestDrop_startSendDataChunks(
        const ARGS& args)
{
    do_storeSelfSeq(args);
    do_storeReplicaSeq(args);
    do_replicaDataRequestPush(args);
    do_replicaDataRequestDrop(args);
    do_startSendDataChunks(args);
}

template <typename ARGS>
void PartitionStateTableActions<ARGS>::
    do_incrementNumRplcaDataRspn_checkQuorumRplcaDataRspn(const ARGS& args)
{
    do_incrementNumRplcaDataRspn(args);
    do_checkQuorumRplcaDataRspn(args);
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
