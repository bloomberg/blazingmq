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

// mqbblp_clusterorchestrator.h                                       -*-C++-*-
#ifndef INCLUDED_MQBBLP_CLUSTERORCHESTRATOR
#define INCLUDED_MQBBLP_CLUSTERORCHESTRATOR

//@PURPOSE: Provide a mechanism to orchestrate the state and health of cluster.
//
//@CLASSES:
//  mqbblp::ClusterOrchestrator: Mechanism to orchestrate a cluster.
//
//@DESCRIPTION: 'mqbblp::ClusterOrchestrator' is a mechanism to orchestrate the
// state and health of a cluster.
//
/// Thread Safety
///-------------
// The 'mqbblp::ClusterOrchestrator' object is not thread safe and should
// always be manipulated from the associated cluster's dispatcher thread,
// unless explicitly documented in a method's contract.

// MQB

#include <mqbblp_clusterqueuehelper.h>
#include <mqbc_clusterdata.h>
#include <mqbc_clusternodesession.h>
#include <mqbc_clusterstate.h>
#include <mqbcfg_messages.h>
#include <mqbi_clusterstatemanager.h>
#include <mqbi_dispatcher.h>
#include <mqbnet_elector.h>

// MWC
#include <mwcma_countingallocatorstore.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_event.h>

// BDE
#include <ball_log.h>
#include <bdld_manageddatum.h>
#include <bdlmt_eventscheduler.h>
#include <bsl_iostream.h>
#include <bslma_managedptr.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_keyword.h>
#include <bsls_types.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace mqbc {
class ClusterState;
}
namespace mqbcmd {
class ClusterStateCommand;
}
namespace mqbcmd {
class ClusterResult;
}
namespace mqbi {
class StorageManager;
}
namespace mqbnet {
class ClusterNode;
}

namespace mqbblp {

// =========================
// class ClusterOrchestrator
// =========================

class ClusterOrchestrator {
  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBBLP.CLUSTERORCHESTRATOR");

  private:
    // PRIVATE TYPES
    typedef bslma::ManagedPtr<mqbnet::Elector> ElectorMp;

    typedef bslma::ManagedPtr<mqbi::ClusterStateManager> ClusterStateManagerMp;

    typedef mqbc::ClusterMembership::ClusterNodeSessionMapIter
        ClusterNodeSessionMapIter;

    typedef bslma::ManagedPtr<bdld::ManagedDatum> ManagedDatumMp;

    typedef mqbc::ClusterNodeSession::QueueHandleMap QueueHandleMap;

    typedef mqbc::ClusterNodeSession::QueueHandleMapIter QueueHandleMapIter;

    typedef mqbc::ClusterNodeSession::QueueHandleMapConstIter
        QueueHandleMapConstIter;

    typedef bdlmt::EventScheduler::RecurringEventHandle RecurringEventHandle;

    typedef mqbc::ClusterStateQueueInfo::AppIdInfos AppIdInfos;

  private:
    // DATA
    bslma::Allocator* d_allocator_p;
    // Allocator to use

    mwcma::CountingAllocatorStore d_allocators;
    // Allocator store to spawn new
    // allocators for sub-components

    bool d_isStarted;

    bool d_wasAvailableAdvisorySent;
    // Whether an available advisory was sent
    // to peer nodes.  Per the contract of
    // node startup sequence, we will only
    // emit this advisory *once* in the
    // lifetime of this instance.

    mqbcfg::ClusterDefinition& d_clusterConfig;
    // Cluster configuration to use

    mqbi::Cluster* d_cluster_p;

    mqbc::ClusterData* d_clusterData_p;
    // Transient cluster data

    ClusterStateManagerMp d_stateManager_mp;
    // Cluster's state manager

    ClusterQueueHelper d_queueHelper;
    // Cluster's queue helper

    ElectorMp d_elector_mp;

    mqbi::StorageManager* d_storageManager_p;

    RecurringEventHandle d_consumptionMonitorEventHandle;

  private:
    // NOT IMPLEMENTED
    ClusterOrchestrator(const ClusterOrchestrator&);             // = delete;
    ClusterOrchestrator& operator=(const ClusterOrchestrator&);  // = delete;

  private:
    // PRIVATE MANIPULATORS

    /// Return the dispatcher of the associated cluster.
    mqbi::Dispatcher* dispatcher();

    void processElectorEventDispatched(const bmqp::Event&   event,
                                       mqbnet::ClusterNode* source);

    /// Callback provided to the elector instance for notification in change
    /// of elector to the specified `state` having specified `term`, and
    /// having the specified `leaderNodeId`, with the reason for state
    /// change captured in the specified `code`.  Note that `leaderNodeId`
    /// could be invalid which indicates absence of a leader.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    void onElectorStateChange(mqbnet::ElectorState::Enum            state,
                              mqbnet::ElectorTransitionReason::Enum code,
                              int                 leaderNodeId,
                              bsls::Types::Uint64 term);

    void electorTransitionToDormant(int                 leaderNodeId,
                                    bsls::Types::Uint64 term);
    void electorTransitionToFollower(int                 leaderNodeId,
                                     bsls::Types::Uint64 term);
    void electorTransitionToCandidate(int                 leaderNodeId,
                                      bsls::Types::Uint64 term);

    /// Helper routine invoked to handle transition to the corresponding
    /// elector state with the specified `leaderNodeId` having the specified
    /// `term`.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    void electorTransitionToLeader(int leaderNodeId, bsls::Types::Uint64 term);

    /// Executed by any thread.
    void onPartitionPrimaryStatusDispatched(int          partitionId,
                                            int          status,
                                            unsigned int primaryLeaseId);

    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    void onNodeUnavailable(mqbnet::ClusterNode* node);

    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    void dropPeerQueues(mqbc::ClusterNodeSession* ns);

    void timerCb();

    void timerCbDispatched();

    /// THREAD: Executed by the dispatcher thread for the specified
    ///         `partitionId`.
    void onQueueActivityTimer(bsls::Types::Int64 timer, int partitionId);

    /// Process the specified `notification` from the specified `notifier`.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    void processLeaderPassiveNotification(
        const bmqp_ctrlmsg::StateNotification& notification,
        mqbnet::ClusterNode*                   notifier);

    /// Start (asynchronous) processing of StopRequest / CLOSING node
    /// advisory from the specified `ns`.  In the case of StopRequest,
    /// the specified `request` is the StopRequest.  In the advisory case,
    /// the `request` is 0.
    void processNodeStoppingNotification(
        mqbc::ClusterNodeSession*           ns,
        const bmqp_ctrlmsg::ControlMessage* request);

    // PRIVATE ACCESSORS

    /// Return true if this is a local cluster.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    bool isLocal() const;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(ClusterOrchestrator,
                                   bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create an instance with the specified `clusterConfig`, `cluster`,
    /// `clusterData`, `clusterState` and `allocator`.
    ClusterOrchestrator(mqbcfg::ClusterDefinition& clusterConfig,
                        mqbi::Cluster*             cluster,
                        mqbc::ClusterData*         clusterData,
                        mqbc::ClusterState*        clusterState,
                        bslma::Allocator*          allocator);

    /// Destroy this instance.  Behavior is undefined unless this instance
    /// is stopped.
    ~ClusterOrchestrator();

    // MANIPULATORS

    /// Start this instance.  Return 0 in case of success, non-zero value
    /// otherwise.  In case of failure, the specified `errorDescription`
    /// will be populated with a brief error message.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    int start(bsl::ostream& errorDescription);

    /// Stop this instance.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    void stop();

    /// Set the storage manager to the specified `value`.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    void setStorageManager(mqbi::StorageManager* value);

    /// Update the datum statistics for the optionally specified
    /// `nodeSession`, or all nodes if `nodeSession` is 0.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    void updateDatumStats(mqbc::ClusterNodeSession* nodeSession = 0);

    /// Transition the cluster to AVAILABLE status, and broadcast a node
    /// status advisory to peers.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    void transitionToAvailable();

    /// Process the specified node status `advisory` from the specified
    /// `source` peer node.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    void
    processNodeStatusAdvisory(const bmqp_ctrlmsg::ControlMessage& advisory,
                              mqbnet::ClusterNode*                source);

    /// Process the node state change event from the specified `node` peer
    /// node with the specified `isAvailable` flag indicating node's
    /// availability.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    void processNodeStateChangeEvent(mqbnet::ClusterNode* node,
                                     bool                 isAvailable);

    /// Process the specified elector `event` from the specified `source`.
    /// Behavior is undefined unless `event` is of type `ELECTOR`.
    ///
    /// THREAD: This method is invoked in the associated cluster's IO
    ///         thread.
    void processElectorEvent(const bmqp::Event&   event,
                             mqbnet::ClusterNode* source);

    /// Process the specified leader-sync-state-query `message` from the
    /// specified `source`.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    void
    processLeaderSyncStateQuery(const bmqp_ctrlmsg::ControlMessage& message,
                                mqbnet::ClusterNode*                source);

    /// Process the specified leader-sync-data-query `message` from the
    /// specified `source`.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    void
    processLeaderSyncDataQuery(const bmqp_ctrlmsg::ControlMessage& message,
                               mqbnet::ClusterNode*                source);

    /// Process the specified `event`.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    void
    processClusterStateEvent(const mqbi::DispatcherClusterStateEvent& event);

    /// Process any queue assignment and un-assignment advisory messages
    /// which were received while self node was starting.  Behavior is
    /// undefined unless self node has transitioned to AVAILABLE.
    void processBufferedQueueAdvisories();

    /// Process the queue assignment in the specified `request`, received
    /// from the specified `requester`.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    void
    processQueueAssignmentRequest(const bmqp_ctrlmsg::ControlMessage& request,
                                  mqbnet::ClusterNode* requester);

    /// Process the specified queue assignment advisory `message` from the
    /// specified `source`.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    void
    processQueueAssignmentAdvisory(const bmqp_ctrlmsg::ControlMessage& message,
                                   mqbnet::ClusterNode*                source);

    /// Process the queue unAssigned advisory in the specified `msg`
    /// received from the specified `source`.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    void
    processQueueUnassignedAdvisory(const bmqp_ctrlmsg::ControlMessage& msg,
                                   mqbnet::ClusterNode*                source);

    /// Process the queue unAssignment advisory in the specified `msg`
    /// received from the specified `source`.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    void
    processQueueUnAssignmentAdvisory(const bmqp_ctrlmsg::ControlMessage& msg,
                                     mqbnet::ClusterNode* source);

    /// Process the specified partition primary advisory `message` from the
    /// specified `source`.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    void processPartitionPrimaryAdvisory(
        const bmqp_ctrlmsg::ControlMessage& message,
        mqbnet::ClusterNode*                source);

    /// Process the specified partition primary advisory `message` from the
    /// specified `source`.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    void processLeaderAdvisory(const bmqp_ctrlmsg::ControlMessage& message,
                               mqbnet::ClusterNode*                source);

    /// Process the specified storage sync request `message` from the
    /// specified `source`.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    void processStorageSyncRequest(const bmqp_ctrlmsg::ControlMessage& message,
                                   mqbnet::ClusterNode*                source);

    /// Process the partition sync state request in the specified `message`
    /// from the specified `source`.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    void processPartitionSyncStateRequest(
        const bmqp_ctrlmsg::ControlMessage& message,
        mqbnet::ClusterNode*                source);

    /// Process the partition sync data request in the specified `message`
    /// from the specified `source`.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    void processPartitionSyncDataRequest(
        const bmqp_ctrlmsg::ControlMessage& message,
        mqbnet::ClusterNode*                source);

    /// Process the partition sync data request status in the specified
    /// `message` from the specified `source`.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    void processPartitionSyncDataRequestStatus(
        const bmqp_ctrlmsg::ControlMessage& message,
        mqbnet::ClusterNode*                source);

    /// Process the primary status advisory in the specified `message` from
    /// the specified `source`.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    void
    processPrimaryStatusAdvisory(const bmqp_ctrlmsg::ControlMessage& message,
                                 mqbnet::ClusterNode*                source);

    /// Process the specified `notification` (StateNotification) from the
    /// specified `notifier`.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    void
    processStateNotification(const bmqp_ctrlmsg::ControlMessage& notification,
                             mqbnet::ClusterNode*                notifier);

    /// Process the specified `request` as StopRequest received from the
    /// specified `source`.
    void processStopRequest(const bmqp_ctrlmsg::ControlMessage& request,
                            mqbnet::ClusterNode*                source);

    // Process the specified cluster state FSM 'message' from the specified
    // 'source'.
    //
    // THREAD: This method is invoked in the associated cluster's
    //         dispatcher thread.
    void
    processClusterStateFSMMessage(const bmqp_ctrlmsg::ControlMessage& message,
                                  mqbnet::ClusterNode*                source);

    // Process the specified partition FSM 'message' from the specified
    // 'source'.
    //
    // THREAD: This method is invoked in the associated cluster's
    //         dispatcher thread.
    void processPartitionMessage(const bmqp_ctrlmsg::ControlMessage& message,
                                 mqbnet::ClusterNode*                source);

    /// Invoked by `mqbblp::Cluster` when recovery has succeeded.
    ///
    /// TBD: this is mostly temporary.
    void onRecoverySuccess();

    /// Invoked to perform validation of IncoreCSL's contents (on disk)
    /// against the "real" cluster state.  Logs a descriptive error message
    /// if inconsistencies are detected.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    /// dispatcher thread.
    ///
    /// TBD: This is mostly temporary, used in phase I of integrating
    ///      IncoreCSL.
    void validateClusterStateLedger();

    /// Invoked by `mqbblp::Cluster` to register a new `appId` for `domain`.
    ///
    /// Note: As this function is dispatched from a separate thread, `appId`
    ///       is taken by value to ensure it survives the lifetime of this
    ///       function call.
    void registerAppId(bsl::string appId, const mqbi::Domain& domain);

    /// Invoked by `mqbblp::Cluster` to unregister an `appId` for `domain`.
    ///
    /// Note: As this function is dispatched from a separate thread, `appId`
    ///       is taken by value to ensure it survives the lifetime of this
    ///       function call.
    void unregisterAppId(bsl::string appId, const mqbi::Domain& domain);

    /// Register a queue info for the queue with the specified `uri`,
    /// `partitionId`, `queueKey` and `appIdInfos`.  If the specified
    /// `forceUpdate` flag is true, update queue info even if it is valid
    /// but different from the specified `queueKey` and `partitionId`.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    void registerQueueInfo(const bmqt::Uri&        uri,
                           int                     partitionId,
                           const mqbu::StorageKey& queueKey,
                           const AppIdInfos&       appIdInfos,
                           bool                    forceUpdate);

    /// Executed by any thread.
    void onPartitionPrimaryStatus(int          partitionId,
                                  int          status,
                                  unsigned int primaryLeaseId);

    /// Process the specified `command`, and write the result to the
    /// clusterResult object.  Return 0 if the command was successfully
    /// processed, or a non-zero value otherwise.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    int processCommand(const mqbcmd::ClusterStateCommand& command,
                       mqbcmd::ClusterResult*             result);

    /// Get a modifiable reference to this object's queue helper.
    ClusterQueueHelper& queueHelper();

    // ACCESSORS
    const mqbc::ClusterState* clusterState() const;

    const ClusterQueueHelper& queueHelper() const;
};

// ============================================================================
//                           INLINE DEFINITIONS
// ============================================================================

// -------------------------
// class ClusterOrchestrator
// -------------------------

// PRIVATE MANIPULATORS
inline mqbi::Dispatcher* ClusterOrchestrator::dispatcher()
{
    return d_clusterData_p->dispatcherClientData().dispatcher();
}

// PRIVATE ACCESSORS
inline bool ClusterOrchestrator::isLocal() const
{
    return d_clusterData_p->cluster()->isLocal();
}

// MANIPULATORS
inline void ClusterOrchestrator::setStorageManager(mqbi::StorageManager* value)
{
    d_storageManager_p = value;
    d_stateManager_mp->setStorageManager(value);
}

inline ClusterQueueHelper& ClusterOrchestrator::queueHelper()
{
    return d_queueHelper;
}

// ACCESSORS
inline const mqbc::ClusterState* ClusterOrchestrator::clusterState() const
{
    return d_stateManager_mp->clusterState();
}

inline const ClusterQueueHelper& ClusterOrchestrator::queueHelper() const
{
    return d_queueHelper;
}

}  // close package namespace
}  // close enterprise namespace

#endif
