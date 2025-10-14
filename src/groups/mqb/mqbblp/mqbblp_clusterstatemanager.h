// Copyright 2015-2023 Bloomberg Finance L.P.
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

// mqbblp_clusterstatemanager.h                                       -*-C++-*-
#ifndef INCLUDED_MQBBLP_CLUSTERSTATEMANAGER
#define INCLUDED_MQBBLP_CLUSTERSTATEMANAGER

/// @file mqbblp_clusterstatemanager.h
///
/// @brief Provide a mechanism to manage the state of a cluster.
///
/// @bbref{mqbblp::ClusterStateManager} is a mechanism to manage the state of a
/// cluster.
///
/// Thread Safety                          {#mqbblp_clusterstatemanager_thread}
/// =============
///
/// The @bbref{mqbblp::ClusterStateManager} object is not thread safe and
/// should always be manipulated from the associated cluster's dispatcher
/// thread, unless explicitly documented in a method's contract.

// MQB
#include <mqbc_clusterdata.h>
#include <mqbc_clustermembership.h>
#include <mqbc_clusterstate.h>
#include <mqbc_clusterstateledger.h>
#include <mqbc_clusterutil.h>
#include <mqbc_electorinfo.h>
#include <mqbcfg_messages.h>
#include <mqbi_clusterstatemanager.h>
#include <mqbi_dispatcher.h>
#include <mqbnet_multirequestmanager.h>
#include <mqbu_storagekey.h>

// BMQ
#include <bmqma_countingallocatorstore.h>
#include <bmqp_ctrlmsg_messages.h>

// BDE
#include <ball_log.h>
#include <bsl_iostream.h>
#include <bsl_string.h>
#include <bsl_utility.h>
#include <bsl_vector.h>
#include <bslma_managedptr.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_keyword.h>
#include <bsls_types.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace mqbi {
class Cluster;
}
namespace mqbi {
class Domain;
}
namespace mqbi {
class StorageManager;
}
namespace mqbnet {
class ClusterNode;
}

namespace mqbblp {
// =========================
// class ClusterStateManager
// =========================

class ClusterStateManager BSLS_KEYWORD_FINAL
: public mqbc::ClusterStateObserver,
  public mqbi::ClusterStateManager,
  public mqbc::ElectorInfoObserver {
  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBBLP.CLUSTERSTATEMANAGER");

  public:
    // TYPES
    typedef bslma::ManagedPtr<mqbc::ClusterStateLedger> ClusterStateLedgerMp;

  private:
    // PRIVATE TYPES
    typedef mqbc::ClusterMembership::ClusterNodeSessionMapIter
        ClusterNodeSessionMapIter;

    typedef mqbc::ClusterData::RequestManagerType RequestManagerType;

    typedef mqbc::ClusterData::MultiRequestManagerType MultiRequestManagerType;

    typedef MultiRequestManagerType::RequestContextSp RequestContextSp;

    typedef MultiRequestManagerType::NodeResponsePairs NodeResponsePairs;

    typedef MultiRequestManagerType::NodeResponsePairsConstIter
        NodeResponsePairsConstIter;

    typedef bsl::pair<bmqp_ctrlmsg::ControlMessage, mqbnet::ClusterNode*>
        QueueAdvisoryAndSource;

    typedef bsl::vector<QueueAdvisoryAndSource> QueueAdvisories;

    typedef mqbc::ClusterStateQueueInfo::AppInfo  AppInfo;
    typedef mqbc::ClusterStateQueueInfo::AppInfos AppInfos;

    typedef mqbc::ClusterState::UriToQueueInfoMap      UriToQueueInfoMap;
    typedef mqbc::ClusterState::UriToQueueInfoMapCIter UriToQueueInfoMapCIter;
    typedef mqbc::ClusterState::DomainStatesCIter      DomainStatesCIter;

  private:
    // DATA

    /// Allocator to use.
    bslma::Allocator* d_allocator_p;

    /// Allocator store to spawn new allocators for sub-components.
    bmqma::CountingAllocatorStore d_allocators;

    bool d_isStarted;

    /// Cluster configuration to use.
    const mqbcfg::ClusterDefinition& d_clusterConfig;

    mqbi::Cluster* d_cluster_p;

    /// Transient cluster data.
    mqbc::ClusterData* d_clusterData_p;

    /// Cluster's state.
    mqbc::ClusterState* d_state_p;

    /// Underlying cluster state ledger.
    ///
    /// @note At this time, it's notified of cluster state events, but does not
    ///       serve as the "single source of truth".
    ClusterStateLedgerMp d_clusterStateLedger_mp;

    mqbi::StorageManager* d_storageManager_p;

    AfterPartitionPrimaryAssignmentCb d_afterPartitionPrimaryAssignmentCb;

  private:
    // NOT IMPLEMENTED
    ClusterStateManager(const ClusterStateManager&);             // = delete;
    ClusterStateManager& operator=(const ClusterStateManager&);  // = delete;

  private:
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

    /// Invoked when self has transitioned to ACTIVE leader.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    void onSelfActiveLeader();

    /// Callback provided to the scheduler to be invoked when a newly
    /// elected (ie, passive) leader node starts a sync with followers
    /// before transitioning to active leader.
    ///
    /// THREAD: This method is invoked in the scheduler's dispatcher thread.
    void leaderSyncCb();

    /// Process the leader-sync-state-query response contained in the
    /// specified `requestContext`.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    void onLeaderSyncStateQueryResponse(const RequestContextSp& requestContext,
                                        bsls::Types::Uint64     term);

    /// Process the leader-sync-data-query response from the specified
    /// `responder` contained in the specified `context`.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    void
    onLeaderSyncDataQueryResponse(const RequestManagerType::RequestSp& context,
                                  const mqbnet::ClusterNode* responder);

    // PRIVATE MANIPULATORS
    //   (virtual: mqbc::ElectorInfoObserver)

    /// Callback invoked when the cluster's leader changes to the specified
    /// `node` with the specified `status`.  Note that null is a valid value
    /// for the `node`, and it implies that the cluster has transitioned to
    /// a state of no leader, and in this case, `status` will be
    /// `UNDEFINED`.
    void onClusterLeader(mqbnet::ClusterNode*                node,
                         mqbc::ElectorInfoLeaderStatus::Enum status)
        BSLS_KEYWORD_OVERRIDE;

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
    void onPartitionPrimaryAssignment(
        int                                partitionId,
        mqbnet::ClusterNode*               primary,
        unsigned int                       leaseId,
        bmqp_ctrlmsg::PrimaryStatus::Value status,
        mqbnet::ClusterNode*               oldPrimary,
        unsigned int                       oldLeaseId) BSLS_KEYWORD_OVERRIDE;

  private:
    // PRIVATE ACCESSORS

    /// Return true if this is a local cluster.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    bool isLocal() const;

    /// Return the dispatcher of the associated cluster.
    const mqbi::Dispatcher* dispatcher() const;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(ClusterStateManager,
                                   bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create an instance with the specified `clusterConfig`, `cluster`,
    /// `clusterData`, `clusterState`, `clusterStateLedger` and `allocator`.
    ClusterStateManager(const mqbcfg::ClusterDefinition& clusterConfig,
                        mqbi::Cluster*                   cluster,
                        mqbc::ClusterData*               clusterData,
                        mqbc::ClusterState*              clusterState,
                        ClusterStateLedgerMp             clusterStateLedger,
                        bslma::Allocator*                allocator);

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
    int start(bsl::ostream& errorDescription) BSLS_KEYWORD_OVERRIDE;

    /// Stop this instance.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    void stop() BSLS_KEYWORD_OVERRIDE;

    /// Set the storage manager to the specified `value`.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    void setStorageManager(mqbi::StorageManager* value) BSLS_KEYWORD_OVERRIDE;

    /// Set the after partition primary assignment callback to the specified
    /// `value`.
    void setAfterPartitionPrimaryAssignmentCb(
        const AfterPartitionPrimaryAssignmentCb& value) BSLS_KEYWORD_OVERRIDE;

    /// Set the primary for the specified `partitionId` to be the specified
    /// `primary` with the specified `leaseId`.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    void setPrimary(int                  partitionId,
                    unsigned int         leaseId,
                    mqbnet::ClusterNode* primary) BSLS_KEYWORD_OVERRIDE;

    /// Set the primary status of the specified `partitionId` to the
    /// specified `status`.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    void setPrimaryStatus(int                                partitionId,
                          bmqp_ctrlmsg::PrimaryStatus::Value status)
        BSLS_KEYWORD_OVERRIDE;

    /// Mark the specified `partitions` as orphaned partitions, due to the
    /// loss of the specified `primary`.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    void markOrphan(const bsl::vector<int>& partitions,
                    mqbnet::ClusterNode*    primary) BSLS_KEYWORD_OVERRIDE;

    /// Assign an available node to each partition which is currently
    /// orphan or is assigned to a node which is not available, and load the
    /// results into the specified `partitions`.  Note that a healthy
    /// partition-node mapping is not modified.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    void assignPartitions(bsl::vector<bmqp_ctrlmsg::PartitionPrimaryInfo>*
                              partitions) BSLS_KEYWORD_OVERRIDE;

    /// Perform the actual assignment of the queue represented by the
    /// specified `uri` for a cluster member queue, that is assign it a
    /// queue key, a partition id, and some appIds; and applying the
    /// corresponding queue assignment advisory to CSL.  Return `false` in the
    /// case of permanent failure when need to reject the assignment.  Return
    /// `true` if the assignment is successful or can be retried.
    /// This method is called only on the leader node.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    bool assignQueue(const bmqt::Uri&      uri,
                     bmqp_ctrlmsg::Status* status) BSLS_KEYWORD_OVERRIDE;

    /// Register a queue info for the queue with the specified `advisory`.
    /// If the specified `forceUpdate` flag is true, update queue info even if
    /// it is valid but different from the specified `advisory`.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    void registerQueueInfo(const bmqp_ctrlmsg::QueueInfo& advisory,
                           bool forceUpdate) BSLS_KEYWORD_OVERRIDE;

    /// Unassign the queue in the specified `advisory` by applying the
    /// advisory to the cluster state ledger owned by this object.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    void unassignQueue(const bmqp_ctrlmsg::QueueUnAssignmentAdvisory& advisory)
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
    void sendClusterState(
        bool                 sendPartitionPrimaryInfo,
        bool                 sendQueuesInfo,
        mqbnet::ClusterNode* node = 0,
        const bsl::vector<bmqp_ctrlmsg::PartitionPrimaryInfo>& partitions =
            bsl::vector<bmqp_ctrlmsg::PartitionPrimaryInfo>())
        BSLS_KEYWORD_OVERRIDE;

    /// Unregister the specified 'removed' and register the specified `added`
    /// for the specified  `domainName` and optionally specified `uri`.
    /// Return `0` on success.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    mqbi::ClusterErrorCode::Enum
    updateAppIds(const bsl::vector<bsl::string>& added,
                 const bsl::vector<bsl::string>& removed,
                 const bsl::string&              domainName,
                 const bsl::string&              uri) BSLS_KEYWORD_OVERRIDE;

    /// Invoked when a newly elected (i.e. passive) leader node initiates a
    /// sync with followers before transitioning to active leader.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    void initiateLeaderSync(bool wait) BSLS_KEYWORD_OVERRIDE;

    /// Process the specified leader-sync-state-query `message` from the
    /// specified `source`.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    void processLeaderSyncStateQuery(
        const bmqp_ctrlmsg::ControlMessage& message,
        mqbnet::ClusterNode*                source) BSLS_KEYWORD_OVERRIDE;

    /// Process the specified leader-sync-data-query `message` from the
    /// specified `source`.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    void processLeaderSyncDataQuery(
        const bmqp_ctrlmsg::ControlMessage& message,
        mqbnet::ClusterNode*                source) BSLS_KEYWORD_OVERRIDE;

    /// Process the specified follower-LSN-request `message` from the
    /// specified `source`.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    void processFollowerLSNRequest(const bmqp_ctrlmsg::ControlMessage& message,
                                   mqbnet::ClusterNode*                source)
        BSLS_KEYWORD_OVERRIDE;

    /// Process the specified follower-cluster-state-request `message` from
    /// the specified `source`.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    void processFollowerClusterStateRequest(
        const bmqp_ctrlmsg::ControlMessage& message,
        mqbnet::ClusterNode*                source) BSLS_KEYWORD_OVERRIDE;

    /// Process the specified leader-CSL-request `message` from the
    /// specified `source`.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    void processRegistrationRequest(
        const bmqp_ctrlmsg::ControlMessage& message,
        mqbnet::ClusterNode*                source) BSLS_KEYWORD_OVERRIDE;

    /// Process the specified cluster state `event`.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    void processClusterStateEvent(
        const mqbi::DispatcherClusterStateEvent& event) BSLS_KEYWORD_OVERRIDE;

    /// Process the queue assignment in the specified `request`, received
    /// from the specified `requester`.  Return the queue assignment result.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    void processQueueAssignmentRequest(
        const bmqp_ctrlmsg::ControlMessage& request,
        mqbnet::ClusterNode*                requester) BSLS_KEYWORD_OVERRIDE;

    /// Process the shutdown event.
    ///
    /// THREAD: Executed by any thread.
    void processShutdownEvent() BSLS_KEYWORD_OVERRIDE;

    /// Invoked when the specified `node` becomes unavailable.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    void onNodeUnavailable(mqbnet::ClusterNode* node) BSLS_KEYWORD_OVERRIDE;

    /// Invoked when this node is stopping.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    void onNodeStopping() BSLS_KEYWORD_OVERRIDE;

    /// Invoked when this node is stopped.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    void onNodeStopped() BSLS_KEYWORD_OVERRIDE;

    /// Set the quorum to the specified value.
    virtual void setQuorum(int quorum) BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS
    /// Return the cluster state managed by this instacne.
    const mqbc::ClusterState* clusterState() const BSLS_KEYWORD_OVERRIDE;

    /// Invoked to perform validation of CSL's contents (on disk) against
    /// the "real" cluster state.  Logs a descriptive error message if
    /// inconsistencies are detected.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    /// dispatcher thread.
    ///
    /// TBD: This is mostly temporary, used in phase I of integrating CSL.
    void validateClusterStateLedger() const BSLS_KEYWORD_OVERRIDE;

    /// Load into the specified `out` the latest ledger LSN associated with
    /// this cluster node.  Return 0 on success, and a non-zero error code
    /// on failure.  Note that this involves iteration over the entire
    /// ledger which can be an expensive operation. This is necessary to
    /// give the latest LSN from the ledger.
    int latestLedgerLSN(bmqp_ctrlmsg::LeaderMessageSequence* out) const
        BSLS_KEYWORD_OVERRIDE;
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

// PRIVATE ACCESSORS
inline bool ClusterStateManager::isLocal() const
{
    return d_clusterData_p->cluster().isLocal();
}

inline const mqbi::Dispatcher* ClusterStateManager::dispatcher() const
{
    return d_clusterData_p->dispatcherClientData().dispatcher();
}

// MANIPULATORS
//   (virtual: mqbi::ClusterStateManager)
inline void ClusterStateManager::setStorageManager(mqbi::StorageManager* value)
{
    d_storageManager_p = value;
}

inline void ClusterStateManager::setAfterPartitionPrimaryAssignmentCb(
    const AfterPartitionPrimaryAssignmentCb& value)
{
    d_afterPartitionPrimaryAssignmentCb = value;
}

// ACCESSORS
inline const mqbc::ClusterState* ClusterStateManager::clusterState() const
{
    return d_state_p;
}

}  // close package namespace
}  // close enterprise namespace

#endif
