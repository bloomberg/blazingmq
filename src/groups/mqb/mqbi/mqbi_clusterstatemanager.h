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

#ifndef INCLUDED_MQBI_CLUSTERSTATEMANAGER
#define INCLUDED_MQBI_CLUSTERSTATEMANAGER

//@PURPOSE: Provide an interface for mechanism to manage the cluster state.
//
//@CLASSES:
//  mqbi::ClusterStateUpdater:  Minimal interface for publishing queue state
//  mqbi::ClusterStateManager: Interface for mechanism to manage cluster state
//
//@DESCRIPTION: 'mqbi::ClusterStateUpdater' is the minimal interface used by
// 'ClusterQueueHelper' to publish queue assignment, unassignment and app-id
// changes to the cluster.  Both the legacy 'ClusterStateManager' and the
// Raft-based 'ClusterStateRaft' implement this interface.
//
// 'mqbi::ClusterStateManager' inherits from 'ClusterStateUpdater' and extends
// it with healing, synchronization, and advisory processing methods that are
// only needed in legacy/FSM mode.
//
/// Thread Safety
///-------------
// The 'mqbi::ClusterStateManager' object is not thread safe and should
// always be manipulated from the associated cluster's dispatcher thread,
// unless explicitly documented in a method's contract.

// MQB

#include <mqbi_cluster.h>
#include <mqbi_dispatcher.h>
#include <mqbu_storagekey.h>

// BMQ
#include <bmqc_orderedhashmap.h>
#include <bmqp_ctrlmsg_messages.h>
#include <bmqt_uri.h>

// BDE
#include <bsl_functional.h>
#include <bsl_iostream.h>
#include <bsl_string.h>
#include <bsl_unordered_set.h>
#include <bsl_utility.h>
#include <bsl_vector.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace mqbc {
class ClusterState;
}
namespace mqbnet {
class ClusterNode;
}
namespace mqbevt {
class ClusterStateEvent;
}

namespace mqbi {

// FORWARD DECLARATION
class Domain;
class StorageManager;

// ==========================
// class ClusterStateUpdater
// ==========================

/// Minimal interface for publishing queue state changes to the cluster.
/// Used by 'ClusterQueueHelper'; implemented by both legacy
/// 'ClusterStateManager' and Raft-based 'ClusterStateRaft'.
class ClusterStateUpdater {
  public:
    // CREATORS
    virtual ~ClusterStateUpdater();

    // MANIPULATORS

    /// Assign a queue key, partition, and appIds to the queue with the
    /// specified `uri`, applying the corresponding advisory.  Return false
    /// on permanent failure, true on success or retriable failure.
    virtual bool assignQueue(const bmqt::Uri&      uri,
                             bmqp_ctrlmsg::Status* status) = 0;

    /// Unassign the queue in the specified `advisory`.
    virtual void
    unassignQueue(const bmqp_ctrlmsg::QueueUnAssignmentAdvisory& advisory) = 0;

    /// Register/unregister the specified `added`/`removed` appIds for
    /// the specified `domainName` and `uri`.  Return 0 on success.
    virtual mqbi::ClusterErrorCode::Enum
    updateAppIds(const bsl::vector<bsl::string>& added,
                 const bsl::vector<bsl::string>& removed,
                 const bsl::string&              domainName,
                 const bsl::string&              uri) = 0;
};

// =========================
// class ClusterStateManager
// =========================

/// This class provides an interface for mechanism to manage the cluster
/// state.
class ClusterStateManager : public ClusterStateUpdater {
  public:
    // TYPES

    /// Signature of a callback invoked after the specified `partitionId`
    /// gets assigned to the specified `primary` with the specified `status`.
    /// This callback is specific to legacy ClusterStateManager; Raft-based
    /// ClusterStateRaft does not support or require this callback.
    typedef bsl::function<void(int                                partitionId,
                               mqbnet::ClusterNode*               primary,
                               bmqp_ctrlmsg::PrimaryStatus::Value status)>
        AfterPartitionPrimaryAssignmentCb;

    /// Pair of (appId, appKey)
    typedef bsl::pair<bsl::string, mqbu::StorageKey>            AppInfo;
    typedef bmqc::OrderedHashMap<bsl::string, mqbu::StorageKey> AppInfos;
    typedef AppInfos::const_iterator                            AppInfosCIter;

  public:
    // CREATORS

    /// Destroy this instance.  Behavior is undefined unless this instance
    /// is stopped.
    ~ClusterStateManager() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    /// Start this instance.  Return 0 in case of success, non-zero value
    /// otherwise.  In case of failure, the specified `errorDescription`
    /// will be populated with a brief error message.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    virtual int start(bsl::ostream& errorDescription) = 0;

    /// Stop this instance.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    virtual void stop() = 0;

    /// Set the storage manager to the specified `value`.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    virtual void setStorageManager(StorageManager* value) = 0;

    /// Set the after partition primary assignment callback to the specified
    /// `value`.
    virtual void setAfterPartitionPrimaryAssignmentCb(
        const AfterPartitionPrimaryAssignmentCb& value) = 0;

    /// Set the primary for the specified `partitionId` to be the specified
    /// `primary` with the specified `leaseId`.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    virtual void setPrimary(int                  partitionId,
                            unsigned int         leaseId,
                            mqbnet::ClusterNode* primary) = 0;

    /// Set the primary status of the specified `partitionId` to the
    /// specified `status`.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    virtual void
    setPrimaryStatus(int                                partitionId,
                     bmqp_ctrlmsg::PrimaryStatus::Value status) = 0;

    /// Mark the specified `partitions` as orphaned partitions, due to the
    /// loss of the specified `primary`.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    virtual void markOrphan(const bsl::vector<int>& partitions,
                            mqbnet::ClusterNode*    primary) = 0;

    /// Assign an available node to each partition which is currently
    /// orphan or is assigned to a node which is not available, and load the
    /// results into the specified `partitions`.  Note that a healthy
    /// partition-node mapping is not modified.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    virtual void assignPartitions(
        bsl::vector<bmqp_ctrlmsg::PartitionPrimaryInfo>* partitions) = 0;

    /// Register a queue info for the queue with the specified `advisory`.
    /// If the specified `forceUpdate` flag is true, update queue info even if
    /// it is valid but different from the specified `advisory`.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    virtual void registerQueueInfo(const bmqp_ctrlmsg::QueueInfo& advisory,
                                   bool forceUpdate) = 0;

    /// Send the current cluster state to follower nodes.  If the specified
    /// `sendPartitionPrimaryInfo` is true, the specified partition-primary
    /// mapping `partitions` will be included.  If the specified
    /// `sendQueuesInfo` is true, queue-partition assignments will be
    /// included.  If the optionally specified `node` is non-null, send the
    /// cluster state to that `node` only.  Otherwise, broadcast to all
    /// followers.  Behavior is undefined unless this node is the leader,
    /// and at least one of `sendPartitionPrimaryInfo` or `sendQueuesInfo` is
    /// true.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    virtual void sendClusterState(
        bool                 sendPartitionPrimaryInfo,
        bool                 sendQueuesInfo,
        mqbnet::ClusterNode* node = 0,
        const bsl::vector<bmqp_ctrlmsg::PartitionPrimaryInfo>& partitions =
            bsl::vector<bmqp_ctrlmsg::PartitionPrimaryInfo>()) = 0;

    /// Invoked when a newly elected (i.e. passive) leader node initiates a
    /// sync with followers before transitioning to active leader.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    virtual void initiateLeaderSync(bool wait) = 0;

    /// Process the specified leader-sync-state-query `message` from the
    /// specified `source`.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    virtual void
    processLeaderSyncStateQuery(const bmqp_ctrlmsg::ControlMessage& message,
                                mqbnet::ClusterNode* source) = 0;

    /// Process the specified leader-sync-data-query `message` from the
    /// specified `source`.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    virtual void
    processLeaderSyncDataQuery(const bmqp_ctrlmsg::ControlMessage& message,
                               mqbnet::ClusterNode*                source) = 0;

    /// Process the specified follower-LSN-request `message` from the
    /// specified `source`.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    virtual void
    processFollowerLSNRequest(const bmqp_ctrlmsg::ControlMessage& message,
                              mqbnet::ClusterNode*                source) = 0;

    /// Process the specified follower-cluster-state-request `message` from
    /// the specified `source`.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    virtual void processFollowerClusterStateRequest(
        const bmqp_ctrlmsg::ControlMessage& message,
        mqbnet::ClusterNode*                source) = 0;

    /// Process the specified registration-request `message` from the
    /// specified `source`.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    virtual void
    processRegistrationRequest(const bmqp_ctrlmsg::ControlMessage& message,
                               mqbnet::ClusterNode*                source) = 0;

    /// Process the specified cluster state `event`.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    virtual void
    processClusterStateEvent(const mqbevt::ClusterStateEvent& event) = 0;

    /// Process the queue assignment in the specified `request`, received
    /// from the specified `requester`.  Return the queue assignment result.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    virtual void
    processQueueAssignmentRequest(const bmqp_ctrlmsg::ControlMessage& request,
                                  mqbnet::ClusterNode* requester) = 0;

    /// Process the shutdown event.
    ///
    /// THREAD: Executed by any thread.
    virtual void processShutdownEvent() = 0;

    /// Invoked when the specified `node` becomes unavailable.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    virtual void onNodeUnavailable(mqbnet::ClusterNode* node) = 0;

    /// Invoked when this node is stopping.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    virtual void onNodeStopping() = 0;

    /// Invoked when this node is stopped.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    virtual void onNodeStopped() = 0;

    // ACCESSORS
    /// Return the cluster state managed by this instacne.
    virtual const mqbc::ClusterState* clusterState() const = 0;

    /// Invoked to perform validation of CSL's contents (on disk) against
    /// the "real" cluster state.  Logs a descriptive error message if
    /// inconsistencies are detected.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    /// dispatcher thread.
    ///
    /// TBD: This is mostly temporary, used in phase I of integrating CSL.
    virtual void validateClusterStateLedger() const = 0;

    /// Load into the specified `out` the latest ledger LSN associated with
    /// this cluster node.  Return 0 on success, and a non-zero error code
    /// on failure.  Note that this involves iteration over the entire
    /// ledger which can be an expensive operation. This is necessary to
    /// give the latest LSN from the ledger.
    virtual int
    latestLedgerLSN(bmqp_ctrlmsg::LeaderMessageSequence* out) const = 0;
};

}  // close package namespace
}  // close enterprise namespace

#endif
