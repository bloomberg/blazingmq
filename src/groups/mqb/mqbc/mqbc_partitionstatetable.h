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

        /// Self is replica, waiting for either success or failure
        /// PrimaryStateResponse.  It **must** reject the ReplicaStateRequest
        /// to prevent the primary from healing self twice in a row, leading
        /// to duplicate work.
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
        /// Can *only* be generated by the Cluster FSM, currently only possible
        /// via `mqbc::ClusterStateManager::markOrphan`.
        e_RST_UNKNOWN = 0,

        /// Role detection as primary to begin healing.  Can *only* be
        /// generated by the Cluster FSM as part of CSL commit.
        e_DETECT_SELF_PRIMARY = 1,

        /// Role detection as replica to begin healing.  Can *only* be
        /// generated by the Cluster FSM as part of CSL commit.
        e_DETECT_SELF_REPLICA = 2,

        /// Generated by the reapply mechanism (watchdog timeout, error
        /// recovery) to retry primary healing.
        e_REAPPLY_SELF_PRIMARY = 3,

        /// Generated by the reapply mechanism (watchdog timeout, error
        /// recovery) to retry replica healing.
        e_REAPPLY_SELF_REPLICA = 4,

        e_REPLICA_STATE_RQST           = 5,
        e_REPLICA_STATE_RSPN           = 6,
        e_FAIL_REPLICA_STATE_RSPN      = 7,
        e_PRIMARY_STATE_RQST           = 8,
        e_PRIMARY_STATE_RSPN           = 9,
        e_FAIL_PRIMARY_STATE_RSPN      = 10,
        e_REPLICA_DATA_RQST_PULL       = 11,
        e_REPLICA_DATA_RQST_PUSH       = 12,
        e_REPLICA_DATA_RQST_DROP       = 13,
        e_REPLICA_DATA_RSPN_PULL       = 14,
        e_REPLICA_DATA_RSPN_PUSH       = 15,
        e_REPLICA_DATA_RSPN_DROP       = 16,
        e_FAIL_REPLICA_DATA_RSPN_PULL  = 17,
        e_CRASH_REPLICA_DATA_RSPN_PULL = 18,
        e_FAIL_REPLICA_DATA_RSPN_PUSH  = 19,
        e_FAIL_REPLICA_DATA_RSPN_DROP  = 20,
        e_DONE_SENDING_DATA_CHUNKS     = 21,
        e_ERROR_SENDING_DATA_CHUNKS    = 22,
        e_DONE_RECEIVING_DATA_CHUNKS   = 23,
        e_ERROR_RECEIVING_DATA_CHUNKS  = 24,
        e_RECOVERY_DATA                = 25,
        e_LIVE_DATA                    = 26,
        e_QUORUM_REPLICA_DATA_RSPN     = 27,
        e_ISSUE_LIVESTREAM             = 28,
        e_QUORUM_REPLICA_SEQ           = 29,
        e_SELF_HIGHEST_SEQ             = 30,
        e_REPLICA_HIGHEST_SEQ          = 31,

        /// Watchdog triggered due to timeout.
        e_WATCHDOG = 32,

        /// Can *only* be generated by the Cluster FSM.
        e_STOP_NODE = 33,

        /// Self replica detects that primary has irreconcilable data.
        e_IRRECONCILABLE_DATA = 34,

        /// Upon receipt of a failure ReplicaDataResponsePull, where the error
        /// code indicates that self primary has irreconcilable data with the
        /// up-to-date replica.
        e_IRRECONCILABLE_REPLICA_DATA_RSPN_PULL = 35,

        /// **NOT A VALID STATE**.  This is an artificial enum value to
        /// represent the number of states.
        e_NUM_EVENTS = 36
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

// FORWARD DECLARATIONS
class PartitionFSMEventData;

// ================================
// class PartitionStateTableActions
// ================================

/// This class defines the actions in the partition state table.
class PartitionStateTableActions {
  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBC.PARTITIONSTATETABLEACTIONS");

  public:
    // TYPES
    typedef void (PartitionStateTableActions::*ActionFunctor)(
        PartitionStateTableEvent::Enum eventType,
        const PartitionFSMEventData&   eventData);

  public:
    virtual ~PartitionStateTableActions();

    virtual void do_none(PartitionStateTableEvent::Enum eventType,
                         const PartitionFSMEventData&   eventData);

    virtual void do_startWatchdog(PartitionStateTableEvent::Enum eventType,
                                  const PartitionFSMEventData& eventData) = 0;

    virtual void do_stopWatchdog(PartitionStateTableEvent::Enum eventType,
                                 const PartitionFSMEventData&   eventData) = 0;

    virtual void
    do_openRecoveryFileSet(PartitionStateTableEvent::Enum eventType,
                           const PartitionFSMEventData&   eventData) = 0;

    virtual void
    do_closeRecoveryFileSet(PartitionStateTableEvent::Enum eventType,
                            const PartitionFSMEventData&   eventData) = 0;

    virtual void do_storeSelfSeq(PartitionStateTableEvent::Enum eventType,
                                 const PartitionFSMEventData&   eventData) = 0;

    virtual void
    do_storePrimarySeq(PartitionStateTableEvent::Enum eventType,
                       const PartitionFSMEventData&   eventData) = 0;

    virtual void
    do_storeReplicaSeq(PartitionStateTableEvent::Enum eventType,
                       const PartitionFSMEventData&   eventData) = 0;

    virtual void
    do_replicaStateRequest(PartitionStateTableEvent::Enum eventType,
                           const PartitionFSMEventData&   eventData) = 0;

    virtual void
    do_replicaStateResponse(PartitionStateTableEvent::Enum eventType,
                            const PartitionFSMEventData&   eventData) = 0;

    virtual void
    do_failureReplicaStateResponse(PartitionStateTableEvent::Enum eventType,
                                   const PartitionFSMEventData& eventData) = 0;

    virtual void do_logFailureReplicaStateResponse(
        PartitionStateTableEvent::Enum eventType,
        const PartitionFSMEventData&   eventData) = 0;

    virtual void do_logFailurePrimaryStateResponse(
        PartitionStateTableEvent::Enum eventType,
        const PartitionFSMEventData&   eventData) = 0;

    virtual void do_logUnexpectedPrimaryStateResponse(
        PartitionStateTableEvent::Enum eventType,
        const PartitionFSMEventData&   eventData) = 0;

    virtual void do_logUnexpectedFailurePrimaryStateResponse(
        PartitionStateTableEvent::Enum eventType,
        const PartitionFSMEventData&   eventData) = 0;

    virtual void
    do_primaryStateRequest(PartitionStateTableEvent::Enum eventType,
                           const PartitionFSMEventData&   eventData) = 0;

    virtual void
    do_primaryStateResponse(PartitionStateTableEvent::Enum eventType,
                            const PartitionFSMEventData&   eventData) = 0;

    /// This method is called by primary to drop partition storage
    /// if needed (e.g primary missed rollover).
    virtual void do_primaryRemoveStorageIfNeeded(
        PartitionStateTableEvent::Enum eventType,
        const PartitionFSMEventData&   eventData) = 0;

    /// This method is called by primary to unconditionally drop partition
    /// storage (e.g. upon receiving irreconcilable data response from
    /// replica).
    virtual void
    do_primaryRemoveStorage(PartitionStateTableEvent::Enum eventType,
                            const PartitionFSMEventData&   eventData) = 0;

    virtual void
    do_failurePrimaryStateResponse(PartitionStateTableEvent::Enum eventType,
                                   const PartitionFSMEventData& eventData) = 0;

    virtual void
    do_replicaDataResponsePush(PartitionStateTableEvent::Enum eventType,
                               const PartitionFSMEventData&   eventData) = 0;

    virtual void
    do_replicaDataRequestPull(PartitionStateTableEvent::Enum eventType,
                              const PartitionFSMEventData&   eventData) = 0;

    virtual void
    do_replicaDataResponsePull(PartitionStateTableEvent::Enum eventType,
                               const PartitionFSMEventData&   eventData) = 0;

    virtual void do_failureReplicaDataResponsePull(
        PartitionStateTableEvent::Enum eventType,
        const PartitionFSMEventData&   eventData) = 0;

    virtual void do_failureReplicaDataResponsePush(
        PartitionStateTableEvent::Enum eventType,
        const PartitionFSMEventData&   eventData) = 0;

    virtual void
    do_sendDataToReplicas(PartitionStateTableEvent::Enum eventType,
                          const PartitionFSMEventData&   eventData) = 0;

    virtual void
    do_sendDataToPrimary(PartitionStateTableEvent::Enum eventType,
                         const PartitionFSMEventData&   eventData) = 0;

    virtual void do_bufferLiveData(PartitionStateTableEvent::Enum eventType,
                                   const PartitionFSMEventData& eventData) = 0;

    virtual void
    do_processBufferedLiveData(PartitionStateTableEvent::Enum eventType,
                               const PartitionFSMEventData&   eventData) = 0;

    virtual void
    do_clearBufferedLiveData(PartitionStateTableEvent::Enum eventType,
                             const PartitionFSMEventData&   eventData) = 0;

    virtual void do_processBufferedPrimaryStatusAdvisories(
        PartitionStateTableEvent::Enum eventType,
        const PartitionFSMEventData&   eventData) = 0;

    virtual void
    do_processLiveData(PartitionStateTableEvent::Enum eventType,
                       const PartitionFSMEventData&   eventData) = 0;

    virtual void do_setPrimary(PartitionStateTableEvent::Enum eventType,
                               const PartitionFSMEventData&   eventData) = 0;

    virtual void
    do_cleanupMetadata(PartitionStateTableEvent::Enum eventType,
                       const PartitionFSMEventData&   eventData) = 0;

    virtual void do_cancelRequests(PartitionStateTableEvent::Enum eventType,
                                   const PartitionFSMEventData& eventData) = 0;

    virtual void do_clearPrimary(PartitionStateTableEvent::Enum eventType,
                                 const PartitionFSMEventData&   eventData) = 0;

    virtual void
    do_setExpectedDataChunkRange(PartitionStateTableEvent::Enum eventType,
                                 const PartitionFSMEventData&   eventData) = 0;

    virtual void
    do_resetReceiveDataCtx(PartitionStateTableEvent::Enum eventType,
                           const PartitionFSMEventData&   eventData) = 0;

    virtual void
    do_attemptOpenStorage(PartitionStateTableEvent::Enum eventType,
                          const PartitionFSMEventData&   eventData) = 0;

    virtual void do_updateStorage(PartitionStateTableEvent::Enum eventType,
                                  const PartitionFSMEventData& eventData) = 0;

    virtual void do_removeStorageAndSendReplicaDataDropResponse(
        PartitionStateTableEvent::Enum eventType,
        const PartitionFSMEventData&   eventData) = 0;

    virtual void
    do_incrementNumRplcaDataRspn(PartitionStateTableEvent::Enum eventType,
                                 const PartitionFSMEventData&   eventData) = 0;

    virtual void
    do_checkQuorumRplcaDataRspn(PartitionStateTableEvent::Enum eventType,
                                const PartitionFSMEventData&   eventData) = 0;

    virtual void do_reapplyEvent(PartitionStateTableEvent::Enum eventType,
                                 const PartitionFSMEventData&   eventData) = 0;

    virtual void do_checkQuorumSeq(PartitionStateTableEvent::Enum eventType,
                                   const PartitionFSMEventData& eventData) = 0;

    virtual void do_findHighestSeq(PartitionStateTableEvent::Enum eventType,
                                   const PartitionFSMEventData& eventData) = 0;

    virtual void
    do_flagFailedReplicaSeq(PartitionStateTableEvent::Enum eventType,
                            const PartitionFSMEventData&   eventData) = 0;

    virtual void
    do_transitionToActivePrimary(PartitionStateTableEvent::Enum eventType,
                                 const PartitionFSMEventData&   eventData) = 0;

    virtual void
    do_reapplyDetectSelfPrimary(PartitionStateTableEvent::Enum eventType,
                                const PartitionFSMEventData&   eventData) = 0;

    virtual void
    do_reapplyDetectSelfReplica(PartitionStateTableEvent::Enum eventType,
                                const PartitionFSMEventData&   eventData) = 0;

    virtual void
    do_unsupportedPrimaryDowngrade(PartitionStateTableEvent::Enum eventType,
                                   const PartitionFSMEventData& eventData) = 0;

    void
    do_removeStorageAndSendReplicaDataDropResponse_reapplyDetectSelfReplica(
        PartitionStateTableEvent::Enum eventType,
        const PartitionFSMEventData&   eventData);

    void
    do_setPrimary_startWatchdog_openRecoveryFileSet_storeSelfSeq_replicaStateRequest_checkQuorumSeq(
        PartitionStateTableEvent::Enum eventType,
        const PartitionFSMEventData&   eventData);

    void
    do_startWatchdog_openRecoveryFileSet_storeSelfSeq_replicaStateRequest_checkQuorumSeq(
        PartitionStateTableEvent::Enum eventType,
        const PartitionFSMEventData&   eventData);

    void
    do_setPrimary_startWatchdog_openRecoveryFileSet_storeSelfSeq_primaryStateRequest(
        PartitionStateTableEvent::Enum eventType,
        const PartitionFSMEventData&   eventData);

    void do_startWatchdog_openRecoveryFileSet_storeSelfSeq_primaryStateRequest(
        PartitionStateTableEvent::Enum eventType,
        const PartitionFSMEventData&   eventData);

    void do_storeReplicaSeq_primaryStateResponse_checkQuorumSeq(
        PartitionStateTableEvent::Enum eventType,
        const PartitionFSMEventData&   eventData);

    void
    do_storeReplicaSeq_checkQuorumSeq(PartitionStateTableEvent::Enum eventType,
                                      const PartitionFSMEventData& eventData);

    void do_storePrimarySeq_replicaStateResponse(
        PartitionStateTableEvent::Enum eventType,
        const PartitionFSMEventData&   eventData);

    void
    do_cleanupMetadata_clearPrimary_closeRecoveryFileSet_stopWatchdog_cancelRequests(
        PartitionStateTableEvent::Enum eventType,
        const PartitionFSMEventData&   eventData);

    void
    do_cleanupMetadata_clearPrimary_closeRecoveryFileSet_stopWatchdog_cancelRequests_reapplyEvent(
        PartitionStateTableEvent::Enum eventType,
        const PartitionFSMEventData&   eventData);

    void do_cleanupMetadata_clearPrimary_reapplyEvent(
        PartitionStateTableEvent::Enum eventType,
        const PartitionFSMEventData&   eventData);

    void do_cleanupMetadata_closeRecoveryFileSet_cancelRequests_reapplyEvent(
        PartitionStateTableEvent::Enum eventType,
        const PartitionFSMEventData&   eventData);

    void
    do_cleanupMetadata_reapplyEvent(PartitionStateTableEvent::Enum eventType,
                                    const PartitionFSMEventData&   eventData);

    void
    do_cleanupMetadata_clearPrimary(PartitionStateTableEvent::Enum eventType,
                                    const PartitionFSMEventData&   eventData);

    void do_resetReceiveDataCtx_flagFailedReplicaSeq_checkQuorumSeq(
        PartitionStateTableEvent::Enum eventType,
        const PartitionFSMEventData&   eventData);

    void do_resetReceiveDataCtx_closeRecoveryFileSet(
        PartitionStateTableEvent::Enum eventType,
        const PartitionFSMEventData&   eventData);

    void
    do_closeRecoveryFileSet_attemptOpenStorage_storeSelfSeq_sendDataToReplicas_incrementNumRplcaDataRspn_checkQuorumRplcaDataRspn(
        PartitionStateTableEvent::Enum eventType,
        const PartitionFSMEventData&   eventData);

    void
    do_primaryRemoveStorageIfNeeded_setExpectedDataChunkRange_replicaDataRequestPull(
        PartitionStateTableEvent::Enum eventType,
        const PartitionFSMEventData&   eventData);

    void
    do_resetReceiveDataCtx_primaryRemoveStorage_setExpectedDataChunkRange_replicaDataRequestPull(
        PartitionStateTableEvent::Enum eventType,
        const PartitionFSMEventData&   eventData);

    void do_setExpectedDataChunkRange_clearBufferedLiveData(
        PartitionStateTableEvent::Enum eventType,
        const PartitionFSMEventData&   eventData);

    void
    do_resetReceiveDataCtx_closeRecoveryFileSet_attemptOpenStorage_storeSelfSeq_sendDataToReplicas_incrementNumRplcaDataRspn_checkQuorumRplcaDataRspn(
        PartitionStateTableEvent::Enum eventType,
        const PartitionFSMEventData&   eventData);

    void do_stopWatchdog_transitionToActivePrimary(
        PartitionStateTableEvent::Enum eventType,
        const PartitionFSMEventData&   eventData);

    void do_replicaStateResponse_storePrimarySeq(
        PartitionStateTableEvent::Enum eventType,
        const PartitionFSMEventData&   eventData);

    void do_failureReplicaDataResponsePull_reapplyDetectSelfReplica(
        PartitionStateTableEvent::Enum eventType,
        const PartitionFSMEventData&   eventData);

    void do_failureReplicaDataResponsePush_reapplyDetectSelfReplica(
        PartitionStateTableEvent::Enum eventType,
        const PartitionFSMEventData&   eventData);

    void
    do_replicaDataResponsePush_resetReceiveDataCtx_closeRecoveryFileSet_attemptOpenStorage_processBufferedLiveData_processBufferedPrimaryStatusAdvisories_stopWatchdog(
        PartitionStateTableEvent::Enum eventType,
        const PartitionFSMEventData&   eventData);

    void do_storeReplicaSeq_primaryStateResponse_sendDataToReplicas(
        PartitionStateTableEvent::Enum eventType,
        const PartitionFSMEventData&   eventData);

    void
    do_storeSelfSeq_storeReplicaSeq_primaryStateResponse_sendDataToReplicas(
        PartitionStateTableEvent::Enum eventType,
        const PartitionFSMEventData&   eventData);

    void do_storeReplicaSeq_sendDataToReplicas(
        PartitionStateTableEvent::Enum eventType,
        const PartitionFSMEventData&   eventData);

    void do_storeSelfSeq_storeReplicaSeq_sendDataToReplicas(
        PartitionStateTableEvent::Enum eventType,
        const PartitionFSMEventData&   eventData);

    void do_incrementNumRplcaDataRspn_checkQuorumRplcaDataRspn(
        PartitionStateTableEvent::Enum eventType,
        const PartitionFSMEventData&   eventData);
};

// =========================
// class PartitionStateTable
// =========================

/// This class is a state table for the Partition FSM.
class PartitionStateTable
: public mqbu::StateTable<PartitionStateTableState::e_NUM_STATES,
                          PartitionStateTableEvent::e_NUM_EVENTS,
                          PartitionStateTableActions::ActionFunctor> {
  public:
    // TYPES
    typedef PartitionStateTableState                  State;
    typedef PartitionStateTableEvent                  Event;
    typedef PartitionStateTableActions::ActionFunctor ActionFunctor;
    typedef mqbu::
        StateTable<State::e_NUM_STATES, Event::e_NUM_EVENTS, ActionFunctor>
                              Table;
    typedef Table::Transition Transition;

  public:
    // CREATORS
    PartitionStateTable()
    : Table(&PartitionStateTableActions::do_none)
    {
#define PST_CFG(s, e, a, n)                                                   \
    Table::configure(State::e_##s,                                            \
                     Event::e_##e,                                            \
                     Transition(State::e_##n,                                 \
                                &PartitionStateTableActions::do_##a));
        //       state                 event                         action
        //       next state
        PST_CFG(
            UNKNOWN,
            DETECT_SELF_PRIMARY,
            setPrimary_startWatchdog_openRecoveryFileSet_storeSelfSeq_replicaStateRequest_checkQuorumSeq,
            PRIMARY_HEALING_STG1);
        PST_CFG(
            UNKNOWN,
            REAPPLY_SELF_PRIMARY,
            startWatchdog_openRecoveryFileSet_storeSelfSeq_replicaStateRequest_checkQuorumSeq,
            PRIMARY_HEALING_STG1);
        PST_CFG(
            UNKNOWN,
            DETECT_SELF_REPLICA,
            setPrimary_startWatchdog_openRecoveryFileSet_storeSelfSeq_primaryStateRequest,
            REPLICA_WAITING);
        PST_CFG(
            UNKNOWN,
            REAPPLY_SELF_REPLICA,
            startWatchdog_openRecoveryFileSet_storeSelfSeq_primaryStateRequest,
            REPLICA_WAITING);
        PST_CFG(UNKNOWN,
                PRIMARY_STATE_RQST,
                failurePrimaryStateResponse,
                UNKNOWN);
        PST_CFG(UNKNOWN, STOP_NODE, none, STOPPED);
        PST_CFG(PRIMARY_HEALING_STG1,
                DETECT_SELF_REPLICA,
                unsupportedPrimaryDowngrade,
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
        PST_CFG(PRIMARY_HEALING_STG1,
                QUORUM_REPLICA_SEQ,
                findHighestSeq,
                PRIMARY_HEALING_STG1);
        PST_CFG(
            PRIMARY_HEALING_STG1,
            SELF_HIGHEST_SEQ,
            closeRecoveryFileSet_attemptOpenStorage_storeSelfSeq_sendDataToReplicas_incrementNumRplcaDataRspn_checkQuorumRplcaDataRspn,
            PRIMARY_HEALING_STG2);
        PST_CFG(
            PRIMARY_HEALING_STG1,
            REPLICA_HIGHEST_SEQ,
            primaryRemoveStorageIfNeeded_setExpectedDataChunkRange_replicaDataRequestPull,
            PRIMARY_HEALING_STG2);
        PST_CFG(
            PRIMARY_HEALING_STG1,
            RST_UNKNOWN,
            cleanupMetadata_clearPrimary_closeRecoveryFileSet_stopWatchdog_cancelRequests,
            UNKNOWN);
        PST_CFG(
            PRIMARY_HEALING_STG1,
            STOP_NODE,
            cleanupMetadata_clearPrimary_closeRecoveryFileSet_stopWatchdog_cancelRequests,
            STOPPED);
        PST_CFG(
            PRIMARY_HEALING_STG1,
            REAPPLY_SELF_PRIMARY,
            cleanupMetadata_closeRecoveryFileSet_cancelRequests_reapplyEvent,
            UNKNOWN);
        PST_CFG(PRIMARY_HEALING_STG1,
                WATCHDOG,
                reapplyDetectSelfPrimary,
                PRIMARY_HEALING_STG1);
        PST_CFG(PRIMARY_HEALING_STG2,
                DETECT_SELF_REPLICA,
                unsupportedPrimaryDowngrade,
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
            IRRECONCILABLE_REPLICA_DATA_RSPN_PULL,
            resetReceiveDataCtx_primaryRemoveStorage_setExpectedDataChunkRange_replicaDataRequestPull,
            PRIMARY_HEALING_STG2);
        PST_CFG(PRIMARY_HEALING_STG2,
                REPLICA_STATE_RSPN,
                storeReplicaSeq_sendDataToReplicas,
                PRIMARY_HEALING_STG2);
        PST_CFG(PRIMARY_HEALING_STG2,
                FAIL_REPLICA_STATE_RSPN,
                logFailureReplicaStateResponse,
                PRIMARY_HEALING_STG2);
        PST_CFG(PRIMARY_HEALING_STG2,
                PRIMARY_STATE_RQST,
                storeReplicaSeq_primaryStateResponse_sendDataToReplicas,
                PRIMARY_HEALING_STG2)
        PST_CFG(PRIMARY_HEALING_STG2,
                RECOVERY_DATA,
                updateStorage,
                PRIMARY_HEALING_STG2);
        PST_CFG(
            PRIMARY_HEALING_STG2,
            REAPPLY_SELF_PRIMARY,
            cleanupMetadata_closeRecoveryFileSet_cancelRequests_reapplyEvent,
            UNKNOWN);
        PST_CFG(PRIMARY_HEALING_STG2,
                ERROR_RECEIVING_DATA_CHUNKS,
                reapplyDetectSelfPrimary,
                PRIMARY_HEALING_STG2);
        PST_CFG(
            PRIMARY_HEALING_STG2,
            REPLICA_DATA_RSPN_PULL,
            resetReceiveDataCtx_closeRecoveryFileSet_attemptOpenStorage_storeSelfSeq_sendDataToReplicas_incrementNumRplcaDataRspn_checkQuorumRplcaDataRspn,
            PRIMARY_HEALING_STG2);
        PST_CFG(PRIMARY_HEALING_STG2,
                REPLICA_DATA_RSPN_PUSH,
                incrementNumRplcaDataRspn_checkQuorumRplcaDataRspn,
                PRIMARY_HEALING_STG2);
        PST_CFG(PRIMARY_HEALING_STG2,
                QUORUM_REPLICA_DATA_RSPN,
                stopWatchdog_transitionToActivePrimary,
                PRIMARY_HEALED);
        PST_CFG(
            PRIMARY_HEALING_STG2,
            RST_UNKNOWN,
            cleanupMetadata_clearPrimary_closeRecoveryFileSet_stopWatchdog_cancelRequests,
            UNKNOWN);
        PST_CFG(PRIMARY_HEALING_STG2,
                WATCHDOG,
                reapplyDetectSelfPrimary,
                PRIMARY_HEALING_STG2);
        PST_CFG(
            PRIMARY_HEALING_STG2,
            STOP_NODE,
            cleanupMetadata_clearPrimary_closeRecoveryFileSet_stopWatchdog_cancelRequests,
            STOPPED);
        PST_CFG(
            REPLICA_WAITING,
            DETECT_SELF_PRIMARY,
            cleanupMetadata_clearPrimary_closeRecoveryFileSet_stopWatchdog_cancelRequests_reapplyEvent,
            UNKNOWN);
        PST_CFG(
            REPLICA_WAITING,
            DETECT_SELF_REPLICA,
            cleanupMetadata_clearPrimary_closeRecoveryFileSet_stopWatchdog_cancelRequests_reapplyEvent,
            UNKNOWN);
        PST_CFG(
            REPLICA_WAITING,
            REAPPLY_SELF_REPLICA,
            cleanupMetadata_closeRecoveryFileSet_cancelRequests_reapplyEvent,
            UNKNOWN);
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
        PST_CFG(
            REPLICA_WAITING,
            RST_UNKNOWN,
            cleanupMetadata_clearPrimary_closeRecoveryFileSet_stopWatchdog_cancelRequests,
            UNKNOWN);
        PST_CFG(REPLICA_WAITING,
                WATCHDOG,
                reapplyDetectSelfReplica,
                REPLICA_WAITING);
        PST_CFG(
            REPLICA_WAITING,
            STOP_NODE,
            cleanupMetadata_clearPrimary_closeRecoveryFileSet_stopWatchdog_cancelRequests,
            STOPPED);
        PST_CFG(
            REPLICA_HEALING,
            DETECT_SELF_PRIMARY,
            cleanupMetadata_clearPrimary_closeRecoveryFileSet_stopWatchdog_cancelRequests_reapplyEvent,
            UNKNOWN);
        PST_CFG(
            REPLICA_HEALING,
            DETECT_SELF_REPLICA,
            cleanupMetadata_clearPrimary_closeRecoveryFileSet_stopWatchdog_cancelRequests_reapplyEvent,
            UNKNOWN);
        PST_CFG(
            REPLICA_HEALING,
            REAPPLY_SELF_REPLICA,
            cleanupMetadata_closeRecoveryFileSet_cancelRequests_reapplyEvent,
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
                logUnexpectedPrimaryStateResponse,
                REPLICA_HEALING);
        PST_CFG(REPLICA_HEALING,
                FAIL_PRIMARY_STATE_RSPN,
                logUnexpectedFailurePrimaryStateResponse,
                REPLICA_HEALING);
        PST_CFG(REPLICA_HEALING,
                REPLICA_DATA_RQST_PULL,
                sendDataToPrimary,
                REPLICA_HEALING);
        PST_CFG(REPLICA_HEALING,
                DONE_SENDING_DATA_CHUNKS,
                replicaDataResponsePull,
                REPLICA_HEALING);
        PST_CFG(REPLICA_HEALING,
                ERROR_SENDING_DATA_CHUNKS,
                failureReplicaDataResponsePull_reapplyDetectSelfReplica,
                REPLICA_HEALING);
        PST_CFG(REPLICA_HEALING,
                IRRECONCILABLE_DATA,
                failureReplicaDataResponsePull,
                REPLICA_HEALING);
        PST_CFG(REPLICA_HEALING,
                REPLICA_DATA_RQST_PUSH,
                setExpectedDataChunkRange_clearBufferedLiveData,
                REPLICA_HEALING);
        PST_CFG(
            REPLICA_HEALING,
            REPLICA_DATA_RQST_DROP,
            removeStorageAndSendReplicaDataDropResponse_reapplyDetectSelfReplica,
            REPLICA_HEALING);
        PST_CFG(REPLICA_HEALING,
                RECOVERY_DATA,
                updateStorage,
                REPLICA_HEALING);
        PST_CFG(
            REPLICA_HEALING,
            DONE_RECEIVING_DATA_CHUNKS,
            replicaDataResponsePush_resetReceiveDataCtx_closeRecoveryFileSet_attemptOpenStorage_processBufferedLiveData_processBufferedPrimaryStatusAdvisories_stopWatchdog,
            REPLICA_HEALED);
        PST_CFG(REPLICA_HEALING,
                ERROR_RECEIVING_DATA_CHUNKS,
                failureReplicaDataResponsePush_reapplyDetectSelfReplica,
                REPLICA_HEALING);
        PST_CFG(REPLICA_HEALING, LIVE_DATA, bufferLiveData, REPLICA_HEALING);
        PST_CFG(
            REPLICA_HEALING,
            RST_UNKNOWN,
            cleanupMetadata_clearPrimary_closeRecoveryFileSet_stopWatchdog_cancelRequests,
            UNKNOWN);
        PST_CFG(REPLICA_HEALING,
                WATCHDOG,
                reapplyDetectSelfReplica,
                REPLICA_HEALING);
        PST_CFG(
            REPLICA_HEALING,
            STOP_NODE,
            cleanupMetadata_clearPrimary_closeRecoveryFileSet_stopWatchdog_cancelRequests,
            STOPPED);
        PST_CFG(REPLICA_HEALED,
                DETECT_SELF_PRIMARY,
                cleanupMetadata_clearPrimary_reapplyEvent,
                UNKNOWN);
        PST_CFG(REPLICA_HEALED,
                DETECT_SELF_REPLICA,
                cleanupMetadata_clearPrimary_reapplyEvent,
                UNKNOWN);
        PST_CFG(REPLICA_HEALED,
                REAPPLY_SELF_REPLICA,
                cleanupMetadata_reapplyEvent,
                UNKNOWN);
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
        PST_CFG(REPLICA_HEALED,
                ISSUE_LIVESTREAM,
                reapplyDetectSelfReplica,
                REPLICA_HEALED);
        PST_CFG(REPLICA_HEALED,
                RST_UNKNOWN,
                cleanupMetadata_clearPrimary,
                UNKNOWN);
        PST_CFG(REPLICA_HEALED,
                STOP_NODE,
                cleanupMetadata_clearPrimary,
                STOPPED);
        PST_CFG(PRIMARY_HEALED,
                DETECT_SELF_REPLICA,
                unsupportedPrimaryDowngrade,
                UNKNOWN);
        PST_CFG(PRIMARY_HEALED,
                REPLICA_STATE_RSPN,
                storeSelfSeq_storeReplicaSeq_sendDataToReplicas,
                PRIMARY_HEALED);
        PST_CFG(PRIMARY_HEALED,
                FAIL_REPLICA_STATE_RSPN,
                logFailureReplicaStateResponse,
                PRIMARY_HEALED);
        PST_CFG(
            PRIMARY_HEALED,
            PRIMARY_STATE_RQST,
            storeSelfSeq_storeReplicaSeq_primaryStateResponse_sendDataToReplicas,
            PRIMARY_HEALED);
        PST_CFG(PRIMARY_HEALED,
                RST_UNKNOWN,
                cleanupMetadata_clearPrimary,
                UNKNOWN);
        PST_CFG(PRIMARY_HEALED,
                STOP_NODE,
                cleanupMetadata_clearPrimary,
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

// MACROS

/// Generate a composite action implementation that calls exactly the 4
/// sub-actions named by the arguments, in order.  The function name is
/// `do_<a1>_<a2>_<a3>_<a4>`, derived from the same arguments, so the name
/// and body cannot diverge.
#define PST_COMPOSITE_4(a1, a2, a3, a4)                                       \
    inline void PartitionStateTableActions::do_##a1##_##a2##_##a3##_##a4(     \
        PartitionStateTableEvent::Enum eventType,                             \
        const PartitionFSMEventData&   eventData)                             \
    {                                                                         \
        do_##a1(eventType, eventData);                                        \
        do_##a2(eventType, eventData);                                        \
        do_##a3(eventType, eventData);                                        \
        do_##a4(eventType, eventData);                                        \
    }

/// Generate a composite action implementation that calls exactly the 5
/// sub-actions named by the arguments, in order.
#define PST_COMPOSITE_5(a1, a2, a3, a4, a5)                                   \
    inline void                                                               \
        PartitionStateTableActions::do_##a1##_##a2##_##a3##_##a4##_##a5(      \
            PartitionStateTableEvent::Enum eventType,                         \
            const PartitionFSMEventData&   eventData)                         \
    {                                                                         \
        do_##a1(eventType, eventData);                                        \
        do_##a2(eventType, eventData);                                        \
        do_##a3(eventType, eventData);                                        \
        do_##a4(eventType, eventData);                                        \
        do_##a5(eventType, eventData);                                        \
    }

inline void PartitionStateTableActions::
    do_removeStorageAndSendReplicaDataDropResponse_reapplyDetectSelfReplica(
        PartitionStateTableEvent::Enum eventType,
        const PartitionFSMEventData&   eventData)
{
    do_removeStorageAndSendReplicaDataDropResponse(eventType, eventData);
    do_reapplyDetectSelfReplica(eventType, eventData);
}

inline void PartitionStateTableActions::
    do_startWatchdog_openRecoveryFileSet_storeSelfSeq_replicaStateRequest_checkQuorumSeq(
        PartitionStateTableEvent::Enum eventType,
        const PartitionFSMEventData&   eventData)
{
    do_startWatchdog(eventType, eventData);
    do_openRecoveryFileSet(eventType, eventData);
    do_storeSelfSeq(eventType, eventData);
    do_replicaStateRequest(eventType, eventData);
    do_checkQuorumSeq(eventType, eventData);
}

inline void PartitionStateTableActions::
    do_setPrimary_startWatchdog_openRecoveryFileSet_storeSelfSeq_replicaStateRequest_checkQuorumSeq(
        PartitionStateTableEvent::Enum eventType,
        const PartitionFSMEventData&   eventData)
{
    do_setPrimary(eventType, eventData);
    do_startWatchdog(eventType, eventData);
    do_openRecoveryFileSet(eventType, eventData);
    do_storeSelfSeq(eventType, eventData);
    do_replicaStateRequest(eventType, eventData);
    do_checkQuorumSeq(eventType, eventData);
}

inline void PartitionStateTableActions::
    do_setPrimary_startWatchdog_openRecoveryFileSet_storeSelfSeq_primaryStateRequest(
        PartitionStateTableEvent::Enum eventType,
        const PartitionFSMEventData&   eventData)
{
    do_setPrimary(eventType, eventData);
    do_startWatchdog(eventType, eventData);
    do_openRecoveryFileSet(eventType, eventData);
    do_storeSelfSeq(eventType, eventData);
    do_primaryStateRequest(eventType, eventData);
}

inline void PartitionStateTableActions::
    do_startWatchdog_openRecoveryFileSet_storeSelfSeq_primaryStateRequest(
        PartitionStateTableEvent::Enum eventType,
        const PartitionFSMEventData&   eventData)
{
    do_startWatchdog(eventType, eventData);
    do_openRecoveryFileSet(eventType, eventData);
    do_storeSelfSeq(eventType, eventData);
    do_primaryStateRequest(eventType, eventData);
}

inline void PartitionStateTableActions::
    do_storeReplicaSeq_primaryStateResponse_checkQuorumSeq(
        PartitionStateTableEvent::Enum eventType,
        const PartitionFSMEventData&   eventData)
{
    do_storeReplicaSeq(eventType, eventData);
    do_primaryStateResponse(eventType, eventData);
    do_checkQuorumSeq(eventType, eventData);
}

inline void PartitionStateTableActions::do_storeReplicaSeq_checkQuorumSeq(
    PartitionStateTableEvent::Enum eventType,
    const PartitionFSMEventData&   eventData)
{
    do_storeReplicaSeq(eventType, eventData);
    do_checkQuorumSeq(eventType, eventData);
}

inline void
PartitionStateTableActions::do_storePrimarySeq_replicaStateResponse(
    PartitionStateTableEvent::Enum eventType,
    const PartitionFSMEventData&   eventData)
{
    do_storePrimarySeq(eventType, eventData);
    do_replicaStateResponse(eventType, eventData);
}

inline void PartitionStateTableActions::
    do_cleanupMetadata_clearPrimary_closeRecoveryFileSet_stopWatchdog_cancelRequests(
        PartitionStateTableEvent::Enum eventType,
        const PartitionFSMEventData&   eventData)
{
    do_cleanupMetadata(eventType, eventData);
    do_clearPrimary(eventType, eventData);
    do_closeRecoveryFileSet(eventType, eventData);
    do_stopWatchdog(eventType, eventData);
    do_cancelRequests(eventType, eventData);
}

inline void PartitionStateTableActions::
    do_cleanupMetadata_clearPrimary_closeRecoveryFileSet_stopWatchdog_cancelRequests_reapplyEvent(
        PartitionStateTableEvent::Enum eventType,
        const PartitionFSMEventData&   eventData)
{
    do_cleanupMetadata(eventType, eventData);
    do_clearPrimary(eventType, eventData);
    do_closeRecoveryFileSet(eventType, eventData);
    do_stopWatchdog(eventType, eventData);
    do_cancelRequests(eventType, eventData);
    do_reapplyEvent(eventType, eventData);
}

PST_COMPOSITE_4(cleanupMetadata,
                closeRecoveryFileSet,
                cancelRequests,
                reapplyEvent)

inline void PartitionStateTableActions::do_cleanupMetadata_reapplyEvent(
    PartitionStateTableEvent::Enum eventType,
    const PartitionFSMEventData&   eventData)
{
    do_cleanupMetadata(eventType, eventData);
    do_reapplyEvent(eventType, eventData);
}

inline void PartitionStateTableActions::do_cleanupMetadata_clearPrimary(
    PartitionStateTableEvent::Enum eventType,
    const PartitionFSMEventData&   eventData)
{
    do_cleanupMetadata(eventType, eventData);
    do_clearPrimary(eventType, eventData);
}

inline void
PartitionStateTableActions::do_cleanupMetadata_clearPrimary_reapplyEvent(
    PartitionStateTableEvent::Enum eventType,
    const PartitionFSMEventData&   eventData)
{
    do_cleanupMetadata(eventType, eventData);
    do_clearPrimary(eventType, eventData);
    do_reapplyEvent(eventType, eventData);
}

inline void PartitionStateTableActions::
    do_resetReceiveDataCtx_flagFailedReplicaSeq_checkQuorumSeq(
        PartitionStateTableEvent::Enum eventType,
        const PartitionFSMEventData&   eventData)
{
    do_resetReceiveDataCtx(eventType, eventData);
    do_flagFailedReplicaSeq(eventType, eventData);
    do_checkQuorumSeq(eventType, eventData);
}

inline void
PartitionStateTableActions::do_resetReceiveDataCtx_closeRecoveryFileSet(
    PartitionStateTableEvent::Enum eventType,
    const PartitionFSMEventData&   eventData)
{
    do_resetReceiveDataCtx(eventType, eventData);
    do_closeRecoveryFileSet(eventType, eventData);
}

inline void PartitionStateTableActions::
    do_closeRecoveryFileSet_attemptOpenStorage_storeSelfSeq_sendDataToReplicas_incrementNumRplcaDataRspn_checkQuorumRplcaDataRspn(
        PartitionStateTableEvent::Enum eventType,
        const PartitionFSMEventData&   eventData)
{
    do_closeRecoveryFileSet(eventType, eventData);
    do_attemptOpenStorage(eventType, eventData);
    do_storeSelfSeq(eventType, eventData);
    do_sendDataToReplicas(eventType, eventData);
    do_incrementNumRplcaDataRspn(eventType, eventData);
    do_checkQuorumRplcaDataRspn(eventType, eventData);
}

inline void PartitionStateTableActions::
    do_primaryRemoveStorageIfNeeded_setExpectedDataChunkRange_replicaDataRequestPull(
        PartitionStateTableEvent::Enum eventType,
        const PartitionFSMEventData&   eventData)
{
    do_primaryRemoveStorageIfNeeded(eventType, eventData);
    do_setExpectedDataChunkRange(eventType, eventData);
    do_replicaDataRequestPull(eventType, eventData);
}

inline void PartitionStateTableActions::
    do_resetReceiveDataCtx_primaryRemoveStorage_setExpectedDataChunkRange_replicaDataRequestPull(
        PartitionStateTableEvent::Enum eventType,
        const PartitionFSMEventData&   eventData)
{
    do_resetReceiveDataCtx(eventType, eventData);
    do_primaryRemoveStorage(eventType, eventData);
    do_setExpectedDataChunkRange(eventType, eventData);
    do_replicaDataRequestPull(eventType, eventData);
}

inline void
PartitionStateTableActions::do_setExpectedDataChunkRange_clearBufferedLiveData(
    PartitionStateTableEvent::Enum eventType,
    const PartitionFSMEventData&   eventData)
{
    do_setExpectedDataChunkRange(eventType, eventData);
    do_clearBufferedLiveData(eventType, eventData);
}

inline void PartitionStateTableActions::
    do_resetReceiveDataCtx_closeRecoveryFileSet_attemptOpenStorage_storeSelfSeq_sendDataToReplicas_incrementNumRplcaDataRspn_checkQuorumRplcaDataRspn(
        PartitionStateTableEvent::Enum eventType,
        const PartitionFSMEventData&   eventData)
{
    do_resetReceiveDataCtx(eventType, eventData);
    do_closeRecoveryFileSet(eventType, eventData);
    do_attemptOpenStorage(eventType, eventData);
    do_storeSelfSeq(eventType, eventData);
    do_sendDataToReplicas(eventType, eventData);
    do_incrementNumRplcaDataRspn(eventType, eventData);
    do_checkQuorumRplcaDataRspn(eventType, eventData);
}

inline void
PartitionStateTableActions::do_stopWatchdog_transitionToActivePrimary(
    PartitionStateTableEvent::Enum eventType,
    const PartitionFSMEventData&   eventData)
{
    do_stopWatchdog(eventType, eventData);
    do_transitionToActivePrimary(eventType, eventData);
}

inline void
PartitionStateTableActions::do_replicaStateResponse_storePrimarySeq(
    PartitionStateTableEvent::Enum eventType,
    const PartitionFSMEventData&   eventData)
{
    do_replicaStateResponse(eventType, eventData);
    do_storePrimarySeq(eventType, eventData);
}

inline void PartitionStateTableActions::
    do_failureReplicaDataResponsePull_reapplyDetectSelfReplica(
        PartitionStateTableEvent::Enum eventType,
        const PartitionFSMEventData&   eventData)
{
    do_failureReplicaDataResponsePull(eventType, eventData);
    do_reapplyDetectSelfReplica(eventType, eventData);
}

inline void PartitionStateTableActions::
    do_failureReplicaDataResponsePush_reapplyDetectSelfReplica(
        PartitionStateTableEvent::Enum eventType,
        const PartitionFSMEventData&   eventData)
{
    do_failureReplicaDataResponsePush(eventType, eventData);
    do_reapplyDetectSelfReplica(eventType, eventData);
}

inline void PartitionStateTableActions::
    do_replicaDataResponsePush_resetReceiveDataCtx_closeRecoveryFileSet_attemptOpenStorage_processBufferedLiveData_processBufferedPrimaryStatusAdvisories_stopWatchdog(
        PartitionStateTableEvent::Enum eventType,
        const PartitionFSMEventData&   eventData)
{
    do_replicaDataResponsePush(eventType, eventData);
    do_resetReceiveDataCtx(eventType, eventData);
    do_closeRecoveryFileSet(eventType, eventData);
    do_attemptOpenStorage(eventType, eventData);
    do_processBufferedLiveData(eventType, eventData);
    do_processBufferedPrimaryStatusAdvisories(eventType, eventData);
    do_stopWatchdog(eventType, eventData);
}

inline void PartitionStateTableActions::
    do_storeReplicaSeq_primaryStateResponse_sendDataToReplicas(
        PartitionStateTableEvent::Enum eventType,
        const PartitionFSMEventData&   eventData)
{
    do_storeReplicaSeq(eventType, eventData);
    do_primaryStateResponse(eventType, eventData);
    do_sendDataToReplicas(eventType, eventData);
}

inline void PartitionStateTableActions::
    do_storeSelfSeq_storeReplicaSeq_primaryStateResponse_sendDataToReplicas(
        PartitionStateTableEvent::Enum eventType,
        const PartitionFSMEventData&   eventData)
{
    do_storeSelfSeq(eventType, eventData);
    do_storeReplicaSeq(eventType, eventData);
    do_primaryStateResponse(eventType, eventData);
    do_sendDataToReplicas(eventType, eventData);
}

inline void PartitionStateTableActions::do_storeReplicaSeq_sendDataToReplicas(
    PartitionStateTableEvent::Enum eventType,
    const PartitionFSMEventData&   eventData)
{
    do_storeReplicaSeq(eventType, eventData);
    do_sendDataToReplicas(eventType, eventData);
}

inline void
PartitionStateTableActions::do_storeSelfSeq_storeReplicaSeq_sendDataToReplicas(
    PartitionStateTableEvent::Enum eventType,
    const PartitionFSMEventData&   eventData)
{
    do_storeSelfSeq(eventType, eventData);
    do_storeReplicaSeq(eventType, eventData);
    do_sendDataToReplicas(eventType, eventData);
}

inline void PartitionStateTableActions::
    do_incrementNumRplcaDataRspn_checkQuorumRplcaDataRspn(
        PartitionStateTableEvent::Enum eventType,
        const PartitionFSMEventData&   eventData)
{
    do_incrementNumRplcaDataRspn(eventType, eventData);
    do_checkQuorumRplcaDataRspn(eventType, eventData);
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
