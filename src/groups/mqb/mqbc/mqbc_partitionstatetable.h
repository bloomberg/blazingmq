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
        e_UNKNOWN              = 0,
        e_PRIMARY_HEALING_STG1 = 1,
        e_PRIMARY_HEALING_STG2 = 2,
        e_REPLICA_HEALING      = 3,
        e_PRIMARY_HEALED       = 4,
        e_REPLICA_HEALED       = 5,
        e_STOPPED              = 6,
        e_NUM_STATES           = 7
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
        e_RST_UNKNOWN                   = 0,
        e_DETECT_SELF_PRIMARY           = 1,
        e_DETECT_SELF_REPLICA           = 2,
        e_REPLICA_STATE_RQST            = 3,
        e_REPLICA_STATE_RSPN            = 4,
        e_FAIL_REPLICA_STATE_RSPN       = 5,
        e_PRIMARY_STATE_RQST            = 6,
        e_PRIMARY_STATE_RSPN            = 7,
        e_FAIL_PRIMARY_STATE_RSPN       = 8,
        e_REPLICA_DATA_RQST_PULL        = 9,
        e_REPLICA_DATA_RQST_PUSH        = 10,
        e_REPLICA_DATA_RQST_DROP        = 11,
        e_REPLICA_DATA_RSPN_PULL        = 12,
        e_REPLICA_DATA_RSPN_PUSH        = 13,
        e_REPLICA_DATA_RSPN_DROP        = 14,
        e_FAIL_REPLICA_DATA_RSPN_PULL   = 15,
        e_CRASH_REPLICA_DATA_RSPN_PULL  = 16,
        e_FAIL_REPLICA_DATA_RSPN_PUSH   = 17,
        e_FAIL_REPLICA_DATA_RSPN_DROP   = 18,
        e_DONE_SENDING_DATA_CHUNKS      = 19,
        e_ERROR_SENDING_DATA_CHUNKS     = 20,
        e_DONE_RECEIVING_DATA_CHUNKS    = 21,
        e_ERROR_RECEIVING_DATA_CHUNKS   = 22,
        e_RECOVERY_DATA                 = 23,
        e_LIVE_DATA                     = 24,
        e_QUORUM_REPLICA_DATA_RSPN      = 25,
        e_ISSUE_LIVESTREAM              = 26,
        e_QUORUM_REPLICA_SEQ            = 27,
        e_SELF_HIGHEST_SEQ              = 28,
        e_REPLICA_HIGHEST_SEQ           = 29,
        e_WATCH_DOG                     = 30,
        e_STOP_NODE                     = 31,
        e_QUORUM_REPLICA_FILE_SIZES     = 32,
        e_SELF_RESIZE_STORAGE           = 33,
        e_REPLICA_RESIZE_STORAGE        = 34,
        e_REPLICA_DATA_RQST_RESIZE      = 35,
        e_REPLICA_DATA_RSPN_RESIZE      = 36,
        e_FAIL_REPLICA_DATA_RSPN_RESIZE = 37,
        e_NUM_EVENTS                    = 38
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

    virtual void do_storeSelfMaxFileSizes(const ARGS& args) = 0;

    virtual void do_storePrimaryMaxFileSizes(const ARGS& args) = 0;

    virtual void do_storeReplicaMaxFileSizes(const ARGS& args) = 0;

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

    virtual void do_replicaDataResponseDrop(const ARGS& args) = 0;

    virtual void do_replicaDataRequestPull(const ARGS& args) = 0;

    virtual void do_replicaDataResponsePull(const ARGS& args) = 0;

    virtual void do_failureReplicaDataResponsePull(const ARGS& args) = 0;

    virtual void do_failureReplicaDataResponsePush(const ARGS& args) = 0;

    virtual void do_replicaDataRequestResize(const ARGS& args) = 0;

    virtual void do_replicaDataResponseResize(const ARGS& args) = 0;

    virtual void do_bufferLiveData(const ARGS& args) = 0;

    virtual void do_processBufferedLiveData(const ARGS& args) = 0;

    virtual void do_clearBufferedLiveData(const ARGS& args) = 0;

    virtual void
    do_processBufferedPrimaryStatusAdvisories(const ARGS& args) = 0;

    virtual void do_processLiveData(const ARGS& args) = 0;

    virtual void do_cleanupMetadata(const ARGS& args) = 0;

    virtual void do_startSendDataChunks(const ARGS& args) = 0;

    virtual void do_setExpectedDataChunkRange(const ARGS& args) = 0;

    virtual void do_resetReceiveDataCtx(const ARGS& args) = 0;

    virtual void do_attemptOpenStorage(const ARGS& args) = 0;

    virtual void do_updateStorage(const ARGS& args) = 0;

    virtual void do_removeStorage(const ARGS& args) = 0;

    virtual void do_incrementNumRplcaDataRspn(const ARGS& args) = 0;

    virtual void do_checkQuorumRplcaDataRspn(const ARGS& args) = 0;

    virtual void do_reapplyEvent(const ARGS& args) = 0;

    virtual void do_checkQuorumMaxFileSizesAndSeq(const ARGS& args) = 0;

    virtual void do_findHighestSeq(const ARGS& args) = 0;

    virtual void do_findHighestFileSizes(const ARGS& args) = 0;

    virtual void do_overrideMaxFileSizes(const ARGS& args) = 0;

    virtual void do_flagFailedReplicaSeq(const ARGS& args) = 0;

    virtual void do_transitionToActivePrimary(const ARGS& args) = 0;

    virtual void do_reapplyDetectSelfPrimary(const ARGS& args) = 0;

    virtual void do_reapplyDetectSelfReplica(const ARGS& args) = 0;

    virtual void do_unsupportedPrimaryDowngrade(const ARGS& args) = 0;

    void do_replicaDataResponseDrop_removeStorage_reapplyDetectSelfReplica(
        const ARGS& args);

    void
    do_startWatchDog_storePartitionInfo_openRecoveryFileSet_storeSelfMaxFileSizes_storeSelfSeq_replicaStateRequest_checkQuorumMaxFileSizesAndSeq(
        const ARGS& args);

    void
    do_startWatchDog_storePartitionInfo_openRecoveryFileSet_storeSelfMaxFileSizes_storeSelfSeq_primaryStateRequest(
        const ARGS& args);

    void
    do_storeReplicaMaxFileSizes_storeReplicaSeq_primaryStateResponse_checkQuorumMaxFileSizesAndSeq(
        const ARGS& args);

    void
    do_storeReplicaMaxFileSizes_storeReplicaSeq_checkQuorumMaxFileSizesAndSeq(
        const ARGS& args);

    void do_storePrimaryMaxFileSizes_storePrimarySeq_replicaStateResponse(
        const ARGS& args);

    void do_storePrimaryMaxFileSizes_storePrimarySeq(const ARGS& args);

    void do_cleanupMetadata_clearPartitionInfo_reapplyEvent(const ARGS& args);

    void do_cleanupMetadata_clearPartitionInfo_stopWatchDog_reapplyEvent(
        const ARGS& args);

    void do_cleanupMetadata_clearPartitionInfo(const ARGS& args);

    void
    do_cleanupMetadata_clearPartitionInfo_closeRecoveryFileSet_stopWatchDog(
        const ARGS& args);

    void do_cleanupMetadata_closeRecoveryFileSet_reapplyDetectSelfPrimary(
        const ARGS& args);

    void
    do_resetReceiveDataCtx_flagFailedReplicaSeq_checkQuorumMaxFileSizesAndSeq(
        const ARGS& args);

    void do_resetReceiveDataCtx_closeRecoveryFileSet(const ARGS& args);

    void do_closeRecoveryFileSet_attemptOpenStorage_startSendDataChunks(
        const ARGS& args);

    void do_closeRecoveryFileSet_overrideMaxFileSizes_attemptOpenStorage(
        const ARGS& args);

    void
    do_closeRecoveryFileSet_overrideMaxFileSizes_attemptOpenStorage_replicaDataResponseResize(
        const ARGS& args);

    void
    do_closeRecoveryFileSet_attemptOpenStorage_replicaDataRequestPush_startSendDataChunks_incrementNumRplcaDataRspn_checkQuorumRplcaDataRspn(
        const ARGS& args);

    void do_setExpectedDataChunkRange_replicaDataRequestPull(const ARGS& args);

    void do_setExpectedDataChunkRange_clearBufferedLiveData(const ARGS& args);

    void
    do_resetReceiveDataCtx_storeSelfMaxFileSizes_storeSelfSeq_closeRecoveryFileSet_attemptOpenStorage_replicaDataRequestPush_replicaDataRequestDrop_startSendDataChunks_incrementNumRplcaDataRspn_checkQuorumRplcaDataRspn(
        const ARGS& args);

    void do_stopWatchDog_transitionToActivePrimary(const ARGS& args);

    void do_replicaStateResponse_storePrimaryMaxFileSizes_storePrimarySeq(
        const ARGS& args);

    void do_cleanupMetadata_reapplyDetectSelfReplica(const ARGS& args);

    void
    do_resetReceiveDataCtx_clearPartitionInfo_closeRecoveryFileSet_stopWatchDog(
        const ARGS& args);

    void do_cleanupMetadata_closeRecoveryFileSet_reapplyDetectSelfReplica(
        const ARGS& args);

    void
    do_replicaDataResponsePull_processBufferedLiveData_processBufferedPrimaryStatusAdvisories_stopWatchDog(
        const ARGS& args);

    void
    do_failureReplicaDataResponsePull_cleanupMetadata_closeRecoveryFileSet_stopWatchDog_reapplyDetectSelfReplica(
        const ARGS& args);

    void
    do_cleanupMetadata_closeRecoveryFileSet_stopWatchDog_reapplyDetectSelfPrimary(
        const ARGS& args);

    void
    do_failureReplicaDataResponsePush_cleanupMetadata_closeRecoveryFileSet_stopWatchDog_reapplyDetectSelfReplica(
        const ARGS& args);

    void
    do_replicaDataResponsePush_resetReceiveDataCtx_closeRecoveryFileSet_attemptOpenStorage_processBufferedLiveData_processBufferedPrimaryStatusAdvisories_stopWatchDog(
        const ARGS& args);

    void
    do_storeReplicaMaxFileSizes_storeReplicaSeq_primaryStateResponse_replicaDataRequestPush_replicaDataRequestDrop_startSendDataChunks(
        const ARGS& args);

    void
    do_storeSelfMaxFileSizes_storeSelfSeq_storeReplicaMaxFileSizes_storeReplicaSeq_primaryStateResponse_replicaDataRequestPush_replicaDataRequestDrop_startSendDataChunks(
        const ARGS& args);

    void
    do_storeReplicaMaxFileSizes_storeReplicaSeq_replicaDataRequestPush_replicaDataRequestDrop_startSendDataChunks(
        const ARGS& args);

    void
    do_storeSelfMaxFileSizes_storeSelfSeq_storeReplicaMaxFileSizes_storeReplicaSeq_replicaDataRequestPush_replicaDataRequestDrop_startSendDataChunks(
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
            startWatchDog_storePartitionInfo_openRecoveryFileSet_storeSelfMaxFileSizes_storeSelfSeq_replicaStateRequest_checkQuorumMaxFileSizesAndSeq,
            PRIMARY_HEALING_STG1);
        PST_CFG(
            UNKNOWN,
            DETECT_SELF_REPLICA,
            startWatchDog_storePartitionInfo_openRecoveryFileSet_storeSelfMaxFileSizes_storeSelfSeq_primaryStateRequest,
            REPLICA_HEALING);
        PST_CFG(UNKNOWN, STOP_NODE, none, STOPPED);
        PST_CFG(PRIMARY_HEALING_STG1,
                DETECT_SELF_REPLICA,
                cleanupMetadata_clearPartitionInfo_stopWatchDog_reapplyEvent,
                UNKNOWN);
        PST_CFG(PRIMARY_HEALING_STG1,
                REPLICA_STATE_RQST,
                failureReplicaStateResponse,
                PRIMARY_HEALING_STG1);
        PST_CFG(
            PRIMARY_HEALING_STG1,
            REPLICA_STATE_RSPN,
            storeReplicaMaxFileSizes_storeReplicaSeq_checkQuorumMaxFileSizesAndSeq,
            PRIMARY_HEALING_STG1);
        PST_CFG(PRIMARY_HEALING_STG1,
                FAIL_REPLICA_STATE_RSPN,
                logFailureReplicaStateResponse,
                PRIMARY_HEALING_STG1);
        PST_CFG(
            PRIMARY_HEALING_STG1,
            PRIMARY_STATE_RQST,
            storeReplicaMaxFileSizes_storeReplicaSeq_primaryStateResponse_checkQuorumMaxFileSizesAndSeq,
            PRIMARY_HEALING_STG1);
        PST_CFG(PRIMARY_HEALING_STG1,
                QUORUM_REPLICA_SEQ,
                findHighestSeq,
                PRIMARY_HEALING_STG1);
        PST_CFG(PRIMARY_HEALING_STG1,
                QUORUM_REPLICA_FILE_SIZES,
                findHighestFileSizes,
                PRIMARY_HEALING_STG1);
        PST_CFG(PRIMARY_HEALING_STG1,
                SELF_RESIZE_STORAGE,
                closeRecoveryFileSet_overrideMaxFileSizes_attemptOpenStorage,
                PRIMARY_HEALING_STG1);
        PST_CFG(PRIMARY_HEALING_STG1,
                REPLICA_RESIZE_STORAGE,
                replicaDataRequestResize,
                PRIMARY_HEALING_STG1);
        PST_CFG(
            PRIMARY_HEALING_STG1,
            SELF_HIGHEST_SEQ,
            closeRecoveryFileSet_attemptOpenStorage_replicaDataRequestPush_startSendDataChunks_incrementNumRplcaDataRspn_checkQuorumRplcaDataRspn,
            PRIMARY_HEALING_STG2);
        PST_CFG(PRIMARY_HEALING_STG1,
                REPLICA_HIGHEST_SEQ,
                setExpectedDataChunkRange_replicaDataRequestPull,
                PRIMARY_HEALING_STG2);
        PST_CFG(
            PRIMARY_HEALING_STG1,
            RST_UNKNOWN,
            cleanupMetadata_clearPartitionInfo_closeRecoveryFileSet_stopWatchDog,
            UNKNOWN);
        PST_CFG(
            PRIMARY_HEALING_STG1,
            STOP_NODE,
            cleanupMetadata_clearPartitionInfo_closeRecoveryFileSet_stopWatchDog,
            STOPPED);
        PST_CFG(PRIMARY_HEALING_STG1,
                WATCH_DOG,
                cleanupMetadata_closeRecoveryFileSet_reapplyDetectSelfPrimary,
                UNKNOWN);
        PST_CFG(PRIMARY_HEALING_STG2,
                DETECT_SELF_REPLICA,
                cleanupMetadata_clearPartitionInfo_stopWatchDog_reapplyEvent,
                UNKNOWN);
        PST_CFG(
            PRIMARY_HEALING_STG2,
            FAIL_REPLICA_DATA_RSPN_PULL,
            resetReceiveDataCtx_flagFailedReplicaSeq_checkQuorumMaxFileSizesAndSeq,
            PRIMARY_HEALING_STG1);
        PST_CFG(
            PRIMARY_HEALING_STG2,
            CRASH_REPLICA_DATA_RSPN_PULL,
            resetReceiveDataCtx_flagFailedReplicaSeq_checkQuorumMaxFileSizesAndSeq,
            PRIMARY_HEALING_STG1);
        PST_CFG(
            PRIMARY_HEALING_STG2,
            REPLICA_STATE_RSPN,
            storeReplicaMaxFileSizes_storeReplicaSeq_replicaDataRequestPush_replicaDataRequestDrop_startSendDataChunks,
            PRIMARY_HEALING_STG2);
        PST_CFG(
            PRIMARY_HEALING_STG2,
            PRIMARY_STATE_RQST,
            storeReplicaMaxFileSizes_storeReplicaSeq_primaryStateResponse_replicaDataRequestPush_replicaDataRequestDrop_startSendDataChunks,
            PRIMARY_HEALING_STG2)
        PST_CFG(PRIMARY_HEALING_STG2,
                RECOVERY_DATA,
                updateStorage,
                PRIMARY_HEALING_STG2);
        PST_CFG(
            PRIMARY_HEALING_STG2,
            ERROR_RECEIVING_DATA_CHUNKS,
            cleanupMetadata_closeRecoveryFileSet_stopWatchDog_reapplyDetectSelfPrimary,
            UNKNOWN);
        PST_CFG(
            PRIMARY_HEALING_STG2,
            REPLICA_DATA_RSPN_PULL,
            resetReceiveDataCtx_storeSelfMaxFileSizes_storeSelfSeq_closeRecoveryFileSet_attemptOpenStorage_replicaDataRequestPush_replicaDataRequestDrop_startSendDataChunks_incrementNumRplcaDataRspn_checkQuorumRplcaDataRspn,
            PRIMARY_HEALING_STG2);
        PST_CFG(PRIMARY_HEALING_STG2,
                REPLICA_DATA_RSPN_PUSH,
                incrementNumRplcaDataRspn_checkQuorumRplcaDataRspn,
                PRIMARY_HEALING_STG2);
        PST_CFG(PRIMARY_HEALING_STG2,
                QUORUM_REPLICA_DATA_RSPN,
                stopWatchDog_transitionToActivePrimary,
                PRIMARY_HEALED);
        PST_CFG(
            PRIMARY_HEALING_STG2,
            RST_UNKNOWN,
            cleanupMetadata_clearPartitionInfo_closeRecoveryFileSet_stopWatchDog,
            UNKNOWN);
        PST_CFG(PRIMARY_HEALING_STG2,
                WATCH_DOG,
                cleanupMetadata_closeRecoveryFileSet_reapplyDetectSelfPrimary,
                UNKNOWN);
        PST_CFG(
            PRIMARY_HEALING_STG2,
            STOP_NODE,
            cleanupMetadata_clearPartitionInfo_closeRecoveryFileSet_stopWatchDog,
            STOPPED);
        PST_CFG(REPLICA_HEALING,
                DETECT_SELF_PRIMARY,
                cleanupMetadata_clearPartitionInfo_stopWatchDog_reapplyEvent,
                UNKNOWN);
        PST_CFG(REPLICA_HEALING,
                DETECT_SELF_REPLICA,
                cleanupMetadata_clearPartitionInfo_stopWatchDog_reapplyEvent,
                UNKNOWN);
        PST_CFG(REPLICA_HEALING,
                REPLICA_STATE_RQST,
                storePrimaryMaxFileSizes_storePrimarySeq_replicaStateResponse,
                REPLICA_HEALING);
        PST_CFG(REPLICA_HEALING,
                PRIMARY_STATE_RQST,
                failurePrimaryStateResponse,
                REPLICA_HEALING);
        PST_CFG(REPLICA_HEALING,
                PRIMARY_STATE_RSPN,
                storePrimaryMaxFileSizes_storePrimarySeq,
                REPLICA_HEALING);
        PST_CFG(REPLICA_HEALING,
                FAIL_PRIMARY_STATE_RSPN,
                logFailurePrimaryStateResponse,
                REPLICA_HEALING);
        PST_CFG(REPLICA_HEALING,
                REPLICA_DATA_RQST_PULL,
                closeRecoveryFileSet_attemptOpenStorage_startSendDataChunks,
                REPLICA_HEALING);
        PST_CFG(
            REPLICA_HEALING,
            DONE_SENDING_DATA_CHUNKS,
            replicaDataResponsePull_processBufferedLiveData_processBufferedPrimaryStatusAdvisories_stopWatchDog,
            REPLICA_HEALED);
        PST_CFG(
            REPLICA_HEALING,
            ERROR_SENDING_DATA_CHUNKS,
            failureReplicaDataResponsePull_cleanupMetadata_closeRecoveryFileSet_stopWatchDog_reapplyDetectSelfReplica,
            UNKNOWN);
        PST_CFG(REPLICA_HEALING,
                REPLICA_DATA_RQST_PUSH,
                setExpectedDataChunkRange_clearBufferedLiveData,
                REPLICA_HEALING);
        PST_CFG(REPLICA_HEALING,
                REPLICA_DATA_RQST_DROP,
                replicaDataResponseDrop_removeStorage_reapplyDetectSelfReplica,
                REPLICA_HEALING);
        PST_CFG(
            REPLICA_HEALING,
            REPLICA_DATA_RQST_RESIZE,
            closeRecoveryFileSet_overrideMaxFileSizes_attemptOpenStorage_replicaDataResponseResize,
            REPLICA_HEALING);
        PST_CFG(REPLICA_HEALING,
                RECOVERY_DATA,
                updateStorage,
                REPLICA_HEALING);
        PST_CFG(
            REPLICA_HEALING,
            DONE_RECEIVING_DATA_CHUNKS,
            replicaDataResponsePush_resetReceiveDataCtx_closeRecoveryFileSet_attemptOpenStorage_processBufferedLiveData_processBufferedPrimaryStatusAdvisories_stopWatchDog,
            REPLICA_HEALED);
        PST_CFG(
            REPLICA_HEALING,
            ERROR_RECEIVING_DATA_CHUNKS,
            failureReplicaDataResponsePush_cleanupMetadata_closeRecoveryFileSet_stopWatchDog_reapplyDetectSelfReplica,
            UNKNOWN);
        PST_CFG(REPLICA_HEALING, LIVE_DATA, bufferLiveData, REPLICA_HEALING);
        PST_CFG(
            REPLICA_HEALING,
            RST_UNKNOWN,
            cleanupMetadata_clearPartitionInfo_closeRecoveryFileSet_stopWatchDog,
            UNKNOWN);
        PST_CFG(REPLICA_HEALING,
                WATCH_DOG,
                cleanupMetadata_closeRecoveryFileSet_reapplyDetectSelfReplica,
                UNKNOWN);
        PST_CFG(
            REPLICA_HEALING,
            STOP_NODE,
            cleanupMetadata_clearPartitionInfo_closeRecoveryFileSet_stopWatchDog,
            STOPPED);
        PST_CFG(REPLICA_HEALED,
                DETECT_SELF_PRIMARY,
                cleanupMetadata_clearPartitionInfo_reapplyEvent,
                UNKNOWN);
        PST_CFG(REPLICA_HEALED,
                DETECT_SELF_REPLICA,
                cleanupMetadata_clearPartitionInfo_reapplyEvent,
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
        PST_CFG(REPLICA_HEALED,
                ISSUE_LIVESTREAM,
                cleanupMetadata_reapplyDetectSelfReplica,
                UNKNOWN);
        PST_CFG(REPLICA_HEALED,
                RST_UNKNOWN,
                cleanupMetadata_clearPartitionInfo,
                UNKNOWN);
        PST_CFG(REPLICA_HEALED,
                STOP_NODE,
                cleanupMetadata_clearPartitionInfo,
                STOPPED);
        PST_CFG(PRIMARY_HEALED,
                DETECT_SELF_REPLICA,
                unsupportedPrimaryDowngrade,
                UNKNOWN);
        PST_CFG(
            PRIMARY_HEALED,
            REPLICA_STATE_RSPN,
            storeSelfMaxFileSizes_storeSelfSeq_storeReplicaMaxFileSizes_storeReplicaSeq_replicaDataRequestPush_replicaDataRequestDrop_startSendDataChunks,
            PRIMARY_HEALED);
        PST_CFG(
            PRIMARY_HEALED,
            PRIMARY_STATE_RQST,
            storeSelfMaxFileSizes_storeSelfSeq_storeReplicaMaxFileSizes_storeReplicaSeq_primaryStateResponse_replicaDataRequestPush_replicaDataRequestDrop_startSendDataChunks,
            PRIMARY_HEALED);
        PST_CFG(PRIMARY_HEALED,
                RST_UNKNOWN,
                cleanupMetadata_clearPartitionInfo,
                UNKNOWN);
        PST_CFG(PRIMARY_HEALED,
                STOP_NODE,
                cleanupMetadata_clearPartitionInfo,
                STOPPED);

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
    do_replicaDataResponseDrop_removeStorage_reapplyDetectSelfReplica(
        const ARGS& args)
{
    do_replicaDataResponseDrop(args);
    do_removeStorage(args);
    do_reapplyDetectSelfReplica(args);
}

template <typename ARGS>
void PartitionStateTableActions<ARGS>::
    do_startWatchDog_storePartitionInfo_openRecoveryFileSet_storeSelfMaxFileSizes_storeSelfSeq_replicaStateRequest_checkQuorumMaxFileSizesAndSeq(
        const ARGS& args)
{
    do_startWatchDog(args);
    do_storePartitionInfo(args);
    do_openRecoveryFileSet(args);
    do_storeSelfMaxFileSizes(args);
    do_storeSelfSeq(args);
    do_replicaStateRequest(args);
    do_checkQuorumMaxFileSizesAndSeq(args);
}

template <typename ARGS>
void PartitionStateTableActions<ARGS>::
    do_startWatchDog_storePartitionInfo_openRecoveryFileSet_storeSelfMaxFileSizes_storeSelfSeq_primaryStateRequest(
        const ARGS& args)
{
    do_startWatchDog(args);
    do_storePartitionInfo(args);
    do_openRecoveryFileSet(args);
    do_storeSelfMaxFileSizes(args);
    do_storeSelfSeq(args);
    do_primaryStateRequest(args);
}

template <typename ARGS>
void PartitionStateTableActions<ARGS>::
    do_storeReplicaMaxFileSizes_storeReplicaSeq_primaryStateResponse_checkQuorumMaxFileSizesAndSeq(
        const ARGS& args)
{
    do_storeReplicaMaxFileSizes(args);
    do_storeReplicaSeq(args);
    do_primaryStateResponse(args);
    do_checkQuorumMaxFileSizesAndSeq(args);
}

template <typename ARGS>
void PartitionStateTableActions<ARGS>::
    do_storeReplicaMaxFileSizes_storeReplicaSeq_checkQuorumMaxFileSizesAndSeq(
        const ARGS& args)
{
    do_storeReplicaMaxFileSizes(args);
    do_storeReplicaSeq(args);
    do_checkQuorumMaxFileSizesAndSeq(args);
}

template <typename ARGS>
void PartitionStateTableActions<ARGS>::
    do_storePrimaryMaxFileSizes_storePrimarySeq_replicaStateResponse(
        const ARGS& args)
{
    do_storePrimaryMaxFileSizes(args);
    do_storePrimarySeq(args);
    do_replicaStateResponse(args);
}

template <typename ARGS>
void PartitionStateTableActions<
    ARGS>::do_storePrimaryMaxFileSizes_storePrimarySeq(const ARGS& args)
{
    do_storePrimaryMaxFileSizes(args);
    do_storePrimarySeq(args);
}

template <typename ARGS>
void PartitionStateTableActions<
    ARGS>::do_cleanupMetadata_clearPartitionInfo_reapplyEvent(const ARGS& args)
{
    do_cleanupMetadata(args);
    do_clearPartitionInfo(args);
    do_reapplyEvent(args);
}

template <typename ARGS>
void PartitionStateTableActions<ARGS>::
    do_cleanupMetadata_clearPartitionInfo_stopWatchDog_reapplyEvent(
        const ARGS& args)
{
    do_cleanupMetadata(args);
    do_clearPartitionInfo(args);
    do_stopWatchDog(args);
    do_reapplyEvent(args);
}

template <typename ARGS>
void PartitionStateTableActions<ARGS>::do_cleanupMetadata_clearPartitionInfo(
    const ARGS& args)
{
    do_cleanupMetadata(args);
    do_clearPartitionInfo(args);
}

template <typename ARGS>
void PartitionStateTableActions<ARGS>::
    do_cleanupMetadata_clearPartitionInfo_closeRecoveryFileSet_stopWatchDog(
        const ARGS& args)
{
    do_cleanupMetadata(args);
    do_clearPartitionInfo(args);
    do_closeRecoveryFileSet(args);
    do_stopWatchDog(args);
}

template <typename ARGS>
void PartitionStateTableActions<ARGS>::
    do_cleanupMetadata_closeRecoveryFileSet_reapplyDetectSelfPrimary(
        const ARGS& args)
{
    do_cleanupMetadata(args);
    do_closeRecoveryFileSet(args);
    do_reapplyDetectSelfPrimary(args);
}

template <typename ARGS>
void PartitionStateTableActions<ARGS>::
    do_resetReceiveDataCtx_flagFailedReplicaSeq_checkQuorumMaxFileSizesAndSeq(
        const ARGS& args)
{
    do_resetReceiveDataCtx(args);
    do_flagFailedReplicaSeq(args);
    do_checkQuorumMaxFileSizesAndSeq(args);
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
    do_closeRecoveryFileSet_attemptOpenStorage_startSendDataChunks(
        const ARGS& args)
{
    do_closeRecoveryFileSet(args);
    do_attemptOpenStorage(args);
    do_startSendDataChunks(args);
}

template <typename ARGS>
void PartitionStateTableActions<ARGS>::
    do_closeRecoveryFileSet_overrideMaxFileSizes_attemptOpenStorage(
        const ARGS& args)
{
    do_closeRecoveryFileSet(args);
    do_overrideMaxFileSizes(args);
    do_attemptOpenStorage(args);
}

template <typename ARGS>
void PartitionStateTableActions<ARGS>::
    do_closeRecoveryFileSet_overrideMaxFileSizes_attemptOpenStorage_replicaDataResponseResize(
        const ARGS& args)
{
    do_closeRecoveryFileSet(args);
    do_overrideMaxFileSizes(args);
    do_attemptOpenStorage(args);
    do_replicaDataResponseResize(args);
}

template <typename ARGS>
void PartitionStateTableActions<ARGS>::
    do_closeRecoveryFileSet_attemptOpenStorage_replicaDataRequestPush_startSendDataChunks_incrementNumRplcaDataRspn_checkQuorumRplcaDataRspn(
        const ARGS& args)
{
    do_closeRecoveryFileSet(args);
    do_attemptOpenStorage(args);
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
void PartitionStateTableActions<
    ARGS>::do_setExpectedDataChunkRange_clearBufferedLiveData(const ARGS& args)
{
    do_setExpectedDataChunkRange(args);
    do_clearBufferedLiveData(args);
}

template <typename ARGS>
void PartitionStateTableActions<ARGS>::
    do_resetReceiveDataCtx_storeSelfMaxFileSizes_storeSelfSeq_closeRecoveryFileSet_attemptOpenStorage_replicaDataRequestPush_replicaDataRequestDrop_startSendDataChunks_incrementNumRplcaDataRspn_checkQuorumRplcaDataRspn(
        const ARGS& args)
{
    do_resetReceiveDataCtx(args);
    do_storeSelfMaxFileSizes(args);
    do_storeSelfSeq(args);
    do_closeRecoveryFileSet(args);
    do_attemptOpenStorage(args);
    do_replicaDataRequestPush(args);
    do_replicaDataRequestDrop(args);
    do_startSendDataChunks(args);
    do_incrementNumRplcaDataRspn(args);
    do_checkQuorumRplcaDataRspn(args);
}

template <typename ARGS>
void PartitionStateTableActions<
    ARGS>::do_stopWatchDog_transitionToActivePrimary(const ARGS& args)
{
    do_stopWatchDog(args);
    do_transitionToActivePrimary(args);
}

template <typename ARGS>
void PartitionStateTableActions<ARGS>::
    do_replicaStateResponse_storePrimaryMaxFileSizes_storePrimarySeq(
        const ARGS& args)
{
    do_replicaStateResponse(args);
    do_storePrimaryMaxFileSizes(args);
    do_storePrimarySeq(args);
}

template <typename ARGS>
void PartitionStateTableActions<
    ARGS>::do_cleanupMetadata_reapplyDetectSelfReplica(const ARGS& args)
{
    do_cleanupMetadata(args);
    do_reapplyDetectSelfReplica(args);
}

template <typename ARGS>
void PartitionStateTableActions<ARGS>::
    do_resetReceiveDataCtx_clearPartitionInfo_closeRecoveryFileSet_stopWatchDog(
        const ARGS& args)
{
    do_resetReceiveDataCtx(args);
    do_clearPartitionInfo(args);
    do_closeRecoveryFileSet(args);
    do_stopWatchDog(args);
}

template <typename ARGS>
void PartitionStateTableActions<ARGS>::
    do_cleanupMetadata_closeRecoveryFileSet_reapplyDetectSelfReplica(
        const ARGS& args)
{
    do_cleanupMetadata(args);
    do_closeRecoveryFileSet(args);
    do_reapplyDetectSelfReplica(args);
}

template <typename ARGS>
void PartitionStateTableActions<ARGS>::
    do_replicaDataResponsePull_processBufferedLiveData_processBufferedPrimaryStatusAdvisories_stopWatchDog(
        const ARGS& args)
{
    do_replicaDataResponsePull(args);
    do_processBufferedLiveData(args);
    do_processBufferedPrimaryStatusAdvisories(args);
    do_stopWatchDog(args);
}

template <typename ARGS>
void PartitionStateTableActions<ARGS>::
    do_failureReplicaDataResponsePull_cleanupMetadata_closeRecoveryFileSet_stopWatchDog_reapplyDetectSelfReplica(
        const ARGS& args)
{
    do_failureReplicaDataResponsePull(args);
    do_cleanupMetadata(args);
    do_closeRecoveryFileSet(args);
    do_stopWatchDog(args);
    do_reapplyDetectSelfReplica(args);
}

template <typename ARGS>
void PartitionStateTableActions<ARGS>::
    do_cleanupMetadata_closeRecoveryFileSet_stopWatchDog_reapplyDetectSelfPrimary(
        const ARGS& args)
{
    do_cleanupMetadata(args);
    do_closeRecoveryFileSet(args);
    do_stopWatchDog(args);
    do_reapplyDetectSelfPrimary(args);
}

template <typename ARGS>
void PartitionStateTableActions<ARGS>::
    do_failureReplicaDataResponsePush_cleanupMetadata_closeRecoveryFileSet_stopWatchDog_reapplyDetectSelfReplica(
        const ARGS& args)
{
    do_failureReplicaDataResponsePush(args);
    do_cleanupMetadata(args);
    do_closeRecoveryFileSet(args);
    do_stopWatchDog(args);
    do_reapplyDetectSelfReplica(args);
}

template <typename ARGS>
void PartitionStateTableActions<ARGS>::
    do_replicaDataResponsePush_resetReceiveDataCtx_closeRecoveryFileSet_attemptOpenStorage_processBufferedLiveData_processBufferedPrimaryStatusAdvisories_stopWatchDog(
        const ARGS& args)
{
    do_replicaDataResponsePush(args);
    do_resetReceiveDataCtx(args);
    do_closeRecoveryFileSet(args);
    do_attemptOpenStorage(args);
    do_processBufferedLiveData(args);
    do_processBufferedPrimaryStatusAdvisories(args);
    do_stopWatchDog(args);
}

template <typename ARGS>
void PartitionStateTableActions<ARGS>::
    do_storeReplicaMaxFileSizes_storeReplicaSeq_primaryStateResponse_replicaDataRequestPush_replicaDataRequestDrop_startSendDataChunks(
        const ARGS& args)
{
    do_storeReplicaMaxFileSizes(args);
    do_storeReplicaSeq(args);
    do_primaryStateResponse(args);
    do_replicaDataRequestPush(args);
    do_replicaDataRequestDrop(args);
    do_startSendDataChunks(args);
}

template <typename ARGS>
void PartitionStateTableActions<ARGS>::
    do_storeSelfMaxFileSizes_storeSelfSeq_storeReplicaMaxFileSizes_storeReplicaSeq_primaryStateResponse_replicaDataRequestPush_replicaDataRequestDrop_startSendDataChunks(
        const ARGS& args)
{
    do_storeSelfMaxFileSizes(args);
    do_storeSelfSeq(args);
    do_storeReplicaMaxFileSizes(args);
    do_storeReplicaSeq(args);
    do_primaryStateResponse(args);
    do_replicaDataRequestPush(args);
    do_replicaDataRequestDrop(args);
    do_startSendDataChunks(args);
}

template <typename ARGS>
void PartitionStateTableActions<ARGS>::
    do_storeReplicaMaxFileSizes_storeReplicaSeq_replicaDataRequestPush_replicaDataRequestDrop_startSendDataChunks(
        const ARGS& args)
{
    do_storeReplicaMaxFileSizes(args);
    do_storeReplicaSeq(args);
    do_replicaDataRequestPush(args);
    do_replicaDataRequestDrop(args);
    do_startSendDataChunks(args);
}

template <typename ARGS>
void PartitionStateTableActions<ARGS>::
    do_storeSelfMaxFileSizes_storeSelfSeq_storeReplicaMaxFileSizes_storeReplicaSeq_replicaDataRequestPush_replicaDataRequestDrop_startSendDataChunks(
        const ARGS& args)
{
    do_storeSelfMaxFileSizes(args);
    do_storeSelfSeq(args);
    do_storeReplicaMaxFileSizes(args);
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
