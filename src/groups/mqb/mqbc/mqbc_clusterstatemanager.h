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

// mqbc_clusterstatemanager.h                                         -*-C++-*-
#ifndef INCLUDED_MQBC_CLUSTERSTATEMANAGER
#define INCLUDED_MQBC_CLUSTERSTATEMANAGER

//@PURPOSE: Provide a mechanism to manage the state of a cluster.
//
//@CLASSES:
//  mqbc::ClusterStateManager: Mechanism to manage the state of a cluster.
//
//@DESCRIPTION: 'mqbc::ClusterStateManager' is a mechanism to manage the
// state of a cluster.
//
/// Thread Safety
///-------------
// The 'mqbc::ClusterStateManager' object is not thread safe and should
// always be manipulated from the associated cluster's dispatcher thread,
// unless explicitly documented in a method's contract.

// MQB

#include <mqbc_clusterdata.h>
#include <mqbc_clusterfsm.h>
#include <mqbc_clusterstate.h>
#include <mqbc_clusterstateledger.h>
#include <mqbc_clusterstatetable.h>
#include <mqbc_electorinfo.h>
#include <mqbcfg_messages.h>
#include <mqbi_clusterstatemanager.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>

// MWC
#include <mwcma_countingallocatorstore.h>

// BDE
#include <ball_log.h>
#include <bdlmt_eventscheduler.h>
#include <bsl_unordered_map.h>
#include <bslma_managedptr.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_keyword.h>
#include <bsls_timeinterval.h>
#include <bsls_types.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace mqbi {
class Cluster;
}
namespace mqbi {
class StorageManager;
}
namespace mqbnet {
class ClusterNode;
}

namespace mqbc {

// =========================
// class ClusterStateManager
// =========================

/// This class provides a mechanism to manage the state of a cluster.
class ClusterStateManager
: public mqbi::ClusterStateManager,
  public ClusterStateTableActions<ClusterFSM::ClusterFSMArgsSp>,
  public mqbc::ElectorInfoObserver,
  public mqbc::ClusterStateObserver {
  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBC.CLUSTERSTATEMANAGER");

  public:
    // TYPES
    typedef bslma::ManagedPtr<mqbc::ClusterStateLedger> ClusterStateLedgerMp;

  private:
    // PRIVATE TYPES
    typedef ClusterData::RequestManagerType RequestManagerType;
    typedef RequestManagerType::RequestSp   RequestContextSp;

    typedef ClusterData::MultiRequestManagerType       MultiRequestManagerType;
    typedef MultiRequestManagerType::RequestContextSp  MultiRequestContextSp;
    typedef MultiRequestManagerType::NodeResponsePairs NodeResponsePairs;
    typedef MultiRequestManagerType::NodeResponsePairsConstIter
        NodeResponsePairsCIter;

    typedef ClusterFSMEventMetadata::InputMessage       InputMessage;
    typedef ClusterFSMEventMetadata::InputMessages      InputMessages;
    typedef ClusterFSMEventMetadata::InputMessagesCIter InputMessagesCIter;

    typedef ClusterFSM::EventWithMetadata EventWithMetadata;
    typedef ClusterFSM::ClusterFSMArgs    ClusterFSMArgs;
    typedef ClusterFSM::ClusterFSMArgsSp  ClusterFSMArgsSp;

    typedef mqbi::ClusterStateManager::AppIdInfos AppIdInfos;

  public:
    // TYPES
    typedef bsl::unordered_map<mqbnet::ClusterNode*,
                               bmqp_ctrlmsg::LeaderMessageSequence>
        NodeToLSNMap;
    // Map of (clusterNode -> LeaderSequenceNumber)
    typedef NodeToLSNMap::const_iterator NodeToLSNMapCIter;

  private:
    // DATA
    bslma::Allocator* d_allocator_p;
    // Allocator for memory allocations

    mwcma::CountingAllocatorStore d_allocators;
    // Allocator store to spawn new
    // allocators for sub-components

    bdlmt::EventSchedulerEventHandle d_watchDogEventHandle;
    // Event handle for the watch dog

    bsls::TimeInterval d_watchDogTimeoutInterval;
    // Timeout interval for the watch dog

    const mqbcfg::ClusterDefinition& d_clusterConfig;
    // Cluster configuration to use

    mqbi::Cluster* d_cluster_p;
    // The cluster which owns this object

    mqbc::ClusterData* d_clusterData_p;
    // Transient cluster data

    mqbc::ClusterState* d_state_p;
    // Cluster's state.

    mqbc::ClusterFSM d_clusterFSM;
    // Underlying finite-state machine to
    // determine state transition and
    // action upon an event.

    NodeToLSNMap d_nodeToLedgerLSNMap;
    // Map from a cluster node (including
    // self) to its view of the latest
    // ledger leader sequence number

    unsigned int d_lsnQuorum;
    // Number of LSN views gathered from
    // cluster nodes (including self)
    // required to achieve a quorum.

    ClusterStateLedgerMp d_clusterStateLedger_mp;
    // Underlying cluster state ledger.

    mqbi::StorageManager* d_storageManager_p;

    QueueAssigningCb d_queueAssigningCb;

    AfterPartitionPrimaryAssignmentCb d_afterPartitionPrimaryAssignmentCb;

  private:
    // NOT IMPLEMENTED
    ClusterStateManager(const ClusterStateManager&);             // = delete;
    ClusterStateManager& operator=(const ClusterStateManager&);  // = delete;

  private:
    // PRIVATE MANIPULATORS
    //   (virtual: mqbc::ClusterStateTableActions)
    virtual void do_abort(const ClusterFSMArgsSp& args) BSLS_KEYWORD_OVERRIDE;

    virtual void
    do_startWatchDog(const ClusterFSMArgsSp& args) BSLS_KEYWORD_OVERRIDE;

    virtual void
    do_stopWatchDog(const ClusterFSMArgsSp& args) BSLS_KEYWORD_OVERRIDE;

    virtual void
    do_triggerWatchDog(const ClusterFSMArgsSp& args) BSLS_KEYWORD_OVERRIDE;

    virtual void
    do_applyCSLSelf(const ClusterFSMArgsSp& args) BSLS_KEYWORD_OVERRIDE;

    virtual void do_sendFollowerLSNRequests(const ClusterFSMArgsSp& args)
        BSLS_KEYWORD_OVERRIDE;

    virtual void do_sendFollowerLSNResponse(const ClusterFSMArgsSp& args)
        BSLS_KEYWORD_OVERRIDE;

    virtual void do_sendFailureFollowerLSNResponse(
        const ClusterFSMArgsSp& args) BSLS_KEYWORD_OVERRIDE;

    virtual void
    do_findHighestLSN(const ClusterFSMArgsSp& x) BSLS_KEYWORD_OVERRIDE;

    virtual void do_sendFollowerClusterStateRequest(
        const ClusterFSMArgsSp& args) BSLS_KEYWORD_OVERRIDE;

    virtual void do_sendFollowerClusterStateResponse(
        const ClusterFSMArgsSp& args) BSLS_KEYWORD_OVERRIDE;

    virtual void do_sendFailureFollowerClusterStateResponse(
        const ClusterFSMArgsSp& args) BSLS_KEYWORD_OVERRIDE;

    virtual void
    do_storeSelfLSN(const ClusterFSMArgsSp& args) BSLS_KEYWORD_OVERRIDE;

    virtual void
    do_storeFollowerLSNs(const ClusterFSMArgsSp& args) BSLS_KEYWORD_OVERRIDE;

    virtual void
    do_removeFollowerLSN(const ClusterFSMArgsSp& args) BSLS_KEYWORD_OVERRIDE;

    virtual void
    do_checkLSNQuorum(const ClusterFSMArgsSp& args) BSLS_KEYWORD_OVERRIDE;

    virtual void do_sendRegistrationRequest(const ClusterFSMArgsSp& args)
        BSLS_KEYWORD_OVERRIDE;

    virtual void do_sendRegistrationResponse(const ClusterFSMArgsSp& args)
        BSLS_KEYWORD_OVERRIDE;

    virtual void do_sendFailureRegistrationResponse(
        const ClusterFSMArgsSp& args) BSLS_KEYWORD_OVERRIDE;

    virtual void do_logStaleFollowerLSNResponse(const ClusterFSMArgsSp& args)
        BSLS_KEYWORD_OVERRIDE;

    virtual void do_logStaleFollowerClusterStateResponse(
        const ClusterFSMArgsSp& args) BSLS_KEYWORD_OVERRIDE;

    virtual void do_logErrorLeaderNotHealed(const ClusterFSMArgsSp& args)
        BSLS_KEYWORD_OVERRIDE;

    virtual void do_logFailFollowerLSNResponses(const ClusterFSMArgsSp& args)
        BSLS_KEYWORD_OVERRIDE;

    virtual void do_logFailFollowerClusterStateResponse(
        const ClusterFSMArgsSp& args) BSLS_KEYWORD_OVERRIDE;

    virtual void do_logFailRegistrationResponse(const ClusterFSMArgsSp& args)
        BSLS_KEYWORD_OVERRIDE;

    virtual void
    do_reapplyEvent(const ClusterFSMArgsSp& args) BSLS_KEYWORD_OVERRIDE;

    virtual void
    do_reapplySelectLeader(const ClusterFSMArgsSp& args) BSLS_KEYWORD_OVERRIDE;

    virtual void do_reapplySelectFollower(const ClusterFSMArgsSp& args)
        BSLS_KEYWORD_OVERRIDE;

    virtual void
    do_cleanupLSNs(const ClusterFSMArgsSp& args) BSLS_KEYWORD_OVERRIDE;

    virtual void
    do_cancelRequests(const ClusterFSMArgsSp& args) BSLS_KEYWORD_OVERRIDE;

    // PRIVATE MANIPULATORS
    //   (virtual: mqbc::ClusterStateObserver)

    /// Callback invoked when the specified `partitionId` gets assigned to
    /// the specified `primary` with the specified `leaseId` and `status`,
    /// replacing the specified `oldPrimary` with the specified
    /// `oldLeaseId`.  Note that null is a valid value for the `primary`,
    /// and it implies that there is no primary for that partition.  Also
    /// note that this method will be invoked when the `primary` or the
    /// `status` or both change.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    virtual void onPartitionPrimaryAssignment(
        int                                partitionId,
        mqbnet::ClusterNode*               primary,
        unsigned int                       leaseId,
        bmqp_ctrlmsg::PrimaryStatus::Value status,
        mqbnet::ClusterNode*               oldPrimary,
        unsigned int                       oldLeaseId) BSLS_KEYWORD_OVERRIDE;

    // PRIVATE MANIPULATORS

    /// Return the dispatcher of the associated cluster.
    mqbi::Dispatcher* dispatcher();

    /// Callback provided to the cluster state ledger, invoked when the
    /// specified `status` becomes available for the commit operation of the
    /// specified `advisory`.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    void onCommit(const bmqp_ctrlmsg::ControlMessage&        advisory,
                  mqbc::ClusterStateLedgerCommitStatus::Enum status);

    /// Apply the specified `event` to the Cluster FSM, along with the
    /// specified `metadata`.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    void applyFSMEvent(ClusterFSM::Event::Enum        event,
                       const ClusterFSMEventMetadata& metadata);

    /// Load into the specified `out` the cluster state snapshot contained
    /// in self's cluster state ledger.  Return 0 on success and non-zero on
    /// error.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    int loadClusterStateSnapshot(ClusterState* out);
    int loadClusterStateSnapshot(bmqp_ctrlmsg::LeaderAdvisory* out);

    /// Process the watch dog trigger event, indicating unhealthiness in the
    /// Cluster FSM.
    ///
    /// THREAD: Executed by the scheduler thread.
    void onWatchDog();

    /// Process the watch dog trigger event, indicating unhealthiness in the
    /// Cluster FSM.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    void onWatchDogDispatched();

    /// Process the follower LSN response contained in the specified
    /// `requestContext`.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    void onFollowerLSNResponse(const MultiRequestContextSp& requestContext);

    /// Process the registration response contained in the specified
    /// `requestContext`, sent by the specified `source`.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    void onRegistrationResponse(mqbnet::ClusterNode*    source,
                                const RequestContextSp& requestContext);

    /// Process the follower cluster state response contained in the
    /// specified `requestContext`, sent by the specified `source`.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    void
    onFollowerClusterStateResponse(mqbnet::ClusterNode*    source,
                                   const RequestContextSp& requestContext);

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(ClusterStateManager,
                                   bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create an instance with the specified `clusterConfig`, `cluster`,
    /// `clusterData`, `clusterState`, `clusterStateLedger`,
    /// `watchDogTimeoutDuration` and `allocator`.
    ClusterStateManager(const mqbcfg::ClusterDefinition& clusterConfig,
                        mqbi::Cluster*                   cluster,
                        mqbc::ClusterData*               clusterData,
                        mqbc::ClusterState*              clusterState,
                        ClusterStateLedgerMp             clusterStateLedger,
                        bsls::Types::Int64 watchDogTimeoutDuration,
                        bslma::Allocator*  allocator);

    /// Destroy this instance.  Behavior is undefined unless this instance
    /// is stopped.
    ~ClusterStateManager() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS
    //   (virtual: mqbi::ClusterStateManager)

    /// Start this instance.  Return 0 in case of success, non-zero value
    /// otherwise.  In case of failure, the specified `errorDescription`
    /// will be populated with a brief error message.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    virtual int start(bsl::ostream& errorDescription) BSLS_KEYWORD_OVERRIDE;

    /// Stop this instance.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    virtual void stop() BSLS_KEYWORD_OVERRIDE;

    /// Set the storage manager to the specified `value`.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    virtual void
    setStorageManager(mqbi::StorageManager* value) BSLS_KEYWORD_OVERRIDE;

    /// Set the queue assigning callback to the specified `value`.
    virtual void
    setQueueAssigningCb(const QueueAssigningCb& value) BSLS_KEYWORD_OVERRIDE;

    virtual void setQueueUnassigningCb(const QueueUnassigningCb& value)
        BSLS_KEYWORD_OVERRIDE;
    // Set the queue unassigning callback to the specified 'value'.

    /// Set the after partition primary assignment callback to the specified
    /// `value`.
    virtual void setAfterPartitionPrimaryAssignmentCb(
        const AfterPartitionPrimaryAssignmentCb& value) BSLS_KEYWORD_OVERRIDE;

    /// Set the primary for the specified `partitionId` to be the specified
    /// `primary` with the specified `leaseId`.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    virtual void
    setPrimary(int                  partitionId,
               unsigned int         leaseId,
               mqbnet::ClusterNode* primary) BSLS_KEYWORD_OVERRIDE;

    /// Set the primary status of the specified `partitionId` to the
    /// specified `status`.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    virtual void setPrimaryStatus(int partitionId,
                                  bmqp_ctrlmsg::PrimaryStatus::Value status)
        BSLS_KEYWORD_OVERRIDE;

    /// Mark the specified `partitions` as orphaned partitions, due to the
    /// loss of the specified `primary`.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    virtual void
    markOrphan(const bsl::vector<int>& partitions,
               mqbnet::ClusterNode*    primary) BSLS_KEYWORD_OVERRIDE;

    /// Assign an available node to each partition which is currently
    /// orphan or is assigned to a node which is not available, and load the
    /// results into the specified `partitions`.  Note that a healthy
    /// partition-node mapping is not modified.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    virtual void assignPartitions(
        bsl::vector<bmqp_ctrlmsg::PartitionPrimaryInfo>* partitions)
        BSLS_KEYWORD_OVERRIDE;

    /// Perform the actual assignment of the queue represented by the
    /// specified `uri` for a cluster member queue, that is assign it a
    /// queue key, a partition id, and some appIds; and applying the
    /// corresponding queue assignment advisory to CSL.  Return a value
    /// indicating whether the assignment was successful or was definitively
    /// rejected. This method is called only on the leader node.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    virtual QueueAssignmentResult::Enum
    assignQueue(const bmqt::Uri&      uri,
                bmqp_ctrlmsg::Status* status = 0) BSLS_KEYWORD_OVERRIDE;

    /// Register a queue info for the queue with the specified `uri`,
    /// `partitionId`, `queueKey` and the optionally specified `appIdInfos`.
    /// If no `appIdInfos` is specified, use the appId infos from the domain
    /// config instead.  If the specified `forceUpdate` flag is true, update
    /// queue info even if it is valid but different from the specified
    /// `queueKey` and `partitionId`.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    virtual void registerQueueInfo(const bmqt::Uri&        uri,
                                   int                     partitionId,
                                   const mqbu::StorageKey& queueKey,
                                   const AppIdInfos&       appIdInfos,
                                   bool forceUpdate) BSLS_KEYWORD_OVERRIDE;

    /// Unassign the queue in the specified `advisory` by applying the
    /// advisory to the cluster state ledger owned by this object.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    virtual void
    unassignQueue(const bmqp_ctrlmsg::QueueUnassignedAdvisory& advisory)
        BSLS_KEYWORD_OVERRIDE;

    /// Send the current cluster state to follower nodes.  If the specified
    /// `sendPartitionPrimaryInfo` is true, the specified partition-primary
    /// mapping `partitions` will be included.  If the specified
    /// `sendQueuesInfo` is true, queue-partition assignments will be
    /// included.  If the optionally specified `node` is non-null, send the
    /// cluster state to that `node` only.  Otherwise, broadcast to all
    /// followers.  Behavior is undefined unless this node is the leader,
    /// and at least one of `sendPartitionPrimaryInfo` or `sendQueuesInfo`
    /// is true.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    virtual void sendClusterState(
        bool                 sendPartitionPartitionInfo,
        bool                 sendQueuesInfo,
        mqbnet::ClusterNode* node = 0,
        const bsl::vector<bmqp_ctrlmsg::PartitionPrimaryInfo>& partitions =
            bsl::vector<bmqp_ctrlmsg::PartitionPrimaryInfo>())
        BSLS_KEYWORD_OVERRIDE;

    /// Register the specified `appId` for all queues in the specified
    /// `domain`.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    virtual void
    registerAppId(const bsl::string&  appId,
                  const mqbi::Domain* domain) BSLS_KEYWORD_OVERRIDE;

    /// Unregister the specified `appId` for all queues in the specified
    /// `domain`.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    virtual void
    unregisterAppId(const bsl::string&  appId,
                    const mqbi::Domain* domain) BSLS_KEYWORD_OVERRIDE;

    /// Invoked when a newly elected (i.e. passive) leader node initiates a
    /// sync with followers before transitioning to active leader.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    virtual void initiateLeaderSync(bool wait) BSLS_KEYWORD_OVERRIDE;

    /// Process the specified leader-sync-state-query `message` from the
    /// specified `source`.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    virtual void processLeaderSyncStateQuery(
        const bmqp_ctrlmsg::ControlMessage& message,
        mqbnet::ClusterNode*                source) BSLS_KEYWORD_OVERRIDE;

    /// Process the specified leader-sync-data-query `message` from the
    /// specified `source`.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    virtual void processLeaderSyncDataQuery(
        const bmqp_ctrlmsg::ControlMessage& message,
        mqbnet::ClusterNode*                source) BSLS_KEYWORD_OVERRIDE;

    /// Process the specified follower-LSN-request `message` from the
    /// specified `source`.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    virtual void processFollowerLSNRequest(
        const bmqp_ctrlmsg::ControlMessage& message,
        mqbnet::ClusterNode*                source) BSLS_KEYWORD_OVERRIDE;

    /// Process the specified follower-cluster-state-request `message` from
    /// the specified `source`.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    virtual void processFollowerClusterStateRequest(
        const bmqp_ctrlmsg::ControlMessage& message,
        mqbnet::ClusterNode*                source) BSLS_KEYWORD_OVERRIDE;

    /// Process the specified leader-CSL-request `message` from the
    /// specified `source`.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    virtual void processRegistrationRequest(
        const bmqp_ctrlmsg::ControlMessage& message,
        mqbnet::ClusterNode*                source) BSLS_KEYWORD_OVERRIDE;

    /// Process the specified `event`.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    virtual void processClusterStateEvent(
        const mqbi::DispatcherClusterStateEvent& event) BSLS_KEYWORD_OVERRIDE;

    /// Process any queue assignment and unassignment advisory messages
    /// which were received while self node was starting.  Behavior is
    /// undefined unless self node has transitioned to AVAILABLE.
    virtual void processBufferedQueueAdvisories() BSLS_KEYWORD_OVERRIDE;

    /// Process the queue assignment in the specified `request`, received
    /// from the specified `requester`.  Return the queue assignment result.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    virtual void processQueueAssignmentRequest(
        const bmqp_ctrlmsg::ControlMessage& request,
        mqbnet::ClusterNode*                requester) BSLS_KEYWORD_OVERRIDE;

    /// Process the specified queue assignment advisory `message` from the
    /// specified `source`.  If the specified `delayed` is true, the
    /// advisory has previously been delayed for processing.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    ///
    /// TODO_CSL: This is the current workflow which we should be able to
    /// remove after the new workflow via
    /// ClusterQueueHelper::onQueueAssigned() is stable.
    virtual void
    processQueueAssignmentAdvisory(const bmqp_ctrlmsg::ControlMessage& message,
                                   mqbnet::ClusterNode*                source,
                                   bool delayed = false) BSLS_KEYWORD_OVERRIDE;

    /// Process the queue unAssigned advisory in the specified `message`
    /// received from the specified `source`.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    ///
    /// TODO_CSL: This is the current workflow which we should be able to
    /// remove after the new workflow via
    /// ClusterQueueHelper::onQueueUnassigned() is stable.
    virtual void processQueueUnassignedAdvisory(
        const bmqp_ctrlmsg::ControlMessage& message,
        mqbnet::ClusterNode*                source) BSLS_KEYWORD_OVERRIDE;

    /// Process the queue unAssignment advisory in the specified `message`
    /// received from the specified `source`.  If the specified `delayed` is
    /// true, the advisory has previously been delayed for processing.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    ///
    /// TODO_CSL: This is the current workflow which we should be able to
    /// remove after the new workflow via
    /// ClusterQueueHelper::onQueueUnassigned() is stable.
    virtual void processQueueUnAssignmentAdvisory(
        const bmqp_ctrlmsg::ControlMessage& message,
        mqbnet::ClusterNode*                source,
        bool delayed = false) BSLS_KEYWORD_OVERRIDE;

    /// Process the specified partition primary advisory `message` from the
    /// specified `source`.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    virtual void processPartitionPrimaryAdvisory(
        const bmqp_ctrlmsg::ControlMessage& message,
        mqbnet::ClusterNode*                source) BSLS_KEYWORD_OVERRIDE;

    /// Process the specified leader advisory `message` from the specified
    /// `source`.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    virtual void
    processLeaderAdvisory(const bmqp_ctrlmsg::ControlMessage& message,
                          mqbnet::ClusterNode* source) BSLS_KEYWORD_OVERRIDE;

    /// Process the shutdown event.
    ///
    /// THREAD: Executed by any thread.
    virtual void processShutdownEvent() BSLS_KEYWORD_OVERRIDE;

    /// Invoked when the specified `node` becomes unavailable.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    virtual void
    onNodeUnavailable(mqbnet::ClusterNode* node) BSLS_KEYWORD_OVERRIDE;

    /// Invoked when this node is stopping.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    virtual void onNodeStopping() BSLS_KEYWORD_OVERRIDE;

    /// Invoked when this node is stopped.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    virtual void onNodeStopped() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS
    //   (virtual: mqbc::ElectorInfoObserver)

    /// Callback invoked when the cluster's leader changes to the specified
    /// `node` with the specified `status`.  Note that null is a valid value
    /// for the `node`, and it implies that the cluster has transitioned to
    /// a state of no leader, and in this case, `status` will be
    /// `UNDEFINED`.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    void onClusterLeader(mqbnet::ClusterNode*          node,
                         ElectorInfoLeaderStatus::Enum status)
        BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS
    //   (virtual: mqbi::ClusterStateManager)
    virtual bool isFirstLeaderAdvisory() const BSLS_KEYWORD_OVERRIDE;

    /// Return the cluster state managed by this instacne.
    virtual const mqbc::ClusterState*
    clusterState() const BSLS_KEYWORD_OVERRIDE;

    /// Invoked to perform validation of CSL's contents (on disk) against
    /// the "real" cluster state.  Logs a descriptive error message if
    /// inconsistencies are detected.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    /// dispatcher thread.
    ///
    /// TBD: This is mostly temporary, used in phase I of integrating CSL.
    virtual void validateClusterStateLedger() const BSLS_KEYWORD_OVERRIDE;

    /// Load into the specified `out` the latest ledger LSN associated with
    /// this cluster node.  Return 0 on success, and a non-zero error code
    /// on failure.  Note that this involves iteration over the entire
    /// ledger which can be an expensive operation. This is necessary to
    /// give the latest LSN from the ledger.
    virtual int latestLedgerLSN(bmqp_ctrlmsg::LeaderMessageSequence* out) const
        BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS

    /// Return the health state of the cluster.
    ClusterFSM::State::Enum healthState() const;

    /// Return the map of clusterNode -> leaderSequenceNumber.
    ///
    /// NOTE: Used for testing purposes only.
    const NodeToLSNMap& nodeToLSNMap() const;
};

// ============================================================================
//                           INLINE DEFINITIONS
// ============================================================================

// -------------------------
// class ClusterStateManager
// -------------------------

// PRIVATE MANIPULATORS
inline mqbi::Dispatcher* ClusterStateManager::dispatcher()
{
    return d_clusterData_p->dispatcherClientData().dispatcher();
}

// MANIPULATORS
//   (virtual: mqbi::ClusterStateManager)
inline void ClusterStateManager::setStorageManager(mqbi::StorageManager* value)
{
    d_storageManager_p = value;
}

inline void
ClusterStateManager::setQueueAssigningCb(const QueueAssigningCb& value)
{
    d_queueAssigningCb = value;
}

inline void ClusterStateManager::setQueueUnassigningCb(
    BSLS_ANNOTATION_UNUSED const QueueUnassigningCb& value)
{
    // Note that QueueUnassigningCb is only ever used in non-CSL mode, so we
    // can ignore it.
    //
    // NOTHING
}

inline void ClusterStateManager::setAfterPartitionPrimaryAssignmentCb(
    const AfterPartitionPrimaryAssignmentCb& value)
{
    d_afterPartitionPrimaryAssignmentCb = value;
}

// ACCESSORS
//   (virtual: mqbi::ClusterStateManager)
inline bool ClusterStateManager::isFirstLeaderAdvisory() const
{
    BSLS_ASSERT_SAFE(false && "NOT IMPLEMENTED!");

    return false;
}

inline const mqbc::ClusterState* ClusterStateManager::clusterState() const
{
    return d_state_p;
}

inline ClusterFSM::State::Enum ClusterStateManager::healthState() const
{
    return d_clusterFSM.state();
}

inline const ClusterStateManager::NodeToLSNMap&
ClusterStateManager::nodeToLSNMap() const
{
    return d_nodeToLedgerLSNMap;
}

}  // close package namespace
}  // close enterprise namespace

#endif
