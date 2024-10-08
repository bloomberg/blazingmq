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

// mqbc_clusterutil.h                                                 -*-C++-*-
#ifndef INCLUDED_MQBC_CLUSTERUTIL
#define INCLUDED_MQBC_CLUSTERUTIL

//@PURPOSE: Provide generic utilities for a cluster.
//
//@CLASSES:
//  mqbc::ClusterUtil: Generic utilities for a cluster.
//
//@DESCRIPTION: 'mqbc::ClusterUtil' provides generic utilities for a cluster.
//
/// Thread Safety
///-------------
/// This component is designed to be executed only by the cluster *DISPATCHER*
/// thread.

// MQB

#include <mqbc_clusterdata.h>
#include <mqbc_clusterstate.h>
#include <mqbc_clusterstateledger.h>
#include <mqbc_clusterstateledgeriterator.h>
#include <mqbcfg_messages.h>
#include <mqbconfm_messages.h>
#include <mqbi_clusterstatemanager.h>
#include <mqbi_dispatcher.h>
#include <mqbu_storagekey.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_protocolutil.h>
#include <bmqp_puteventbuilder.h>
#include <bmqt_resultcode.h>
#include <bmqt_uri.h>

// BDE
#include <bdlbb_blob.h>
#include <bdlf_bind.h>
#include <bsl_iostream.h>
#include <bsl_memory.h>
#include <bsl_string.h>
#include <bsl_unordered_map.h>
#include <bsl_vector.h>
#include <bslma_allocator.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace mqbi {
class Domain;
}
namespace mqbi {
class StorageManager;
}
namespace mqbnet {
class ClusterNode;
}

namespace mqbc {

// FORWARD DECLARATION
class ClusterNodeSession;

// ==================
// struct ClusterUtil
// ==================

/// Generic utilities for a cluster.
struct ClusterUtil {
  private:
    // CLASS SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBC.CLUSTERUTIL");

  public:
    // TYPES
    typedef ClusterStateQueueInfo::AppIdInfo       AppIdInfo;
    typedef ClusterStateQueueInfo::AppIdInfos      AppIdInfos;
    typedef ClusterStateQueueInfo::AppIdInfosCIter AppIdInfosCIter;

    typedef mqbc::ClusterState::QueueInfoSp      QueueInfoSp;
    typedef ClusterState::UriToQueueInfoMap      UriToQueueInfoMap;
    typedef ClusterState::UriToQueueInfoMapCIter UriToQueueInfoMapCIter;
    typedef ClusterState::DomainStates           DomainStates;
    typedef ClusterState::DomainStatesCIter      DomainStatesCIter;

    typedef ClusterMembership::ClusterNodeSessionMapConstIter
        ClusterNodeSessionMapConstIter;

    typedef mqbi::ClusterStateManager::QueueAssignmentResult
        QueueAssignmentResult;

    typedef mqbi::ClusterStateManager::QueueAssigningCb QueueAssigningCb;

    /// Map of NodeSession -> number of new partitions to assign to it
    typedef bsl::unordered_map<ClusterNodeSession*, unsigned int>
                                                NumNewPartitionsMap;
    typedef NumNewPartitionsMap::const_iterator NumNewPartitionsMapCIter;

  private:
    // PRIVATE TYPES
    typedef ClusterState::UriToQueueInfoMapIter UriToQueueInfoMapIter;
    typedef ClusterState::DomainStatesIter      DomainStatesIter;

  public:
    // FUNCTIONS

    /// Generate a nack with the specified `status` for a PUT message having
    /// the specified `putHeader` from the specified `source`.  The nack is
    /// replied to the `source`.
    static void generateNack(bmqt::AckResult::Enum               status,
                             const bmqp::PutHeader&              putHeader,
                             mqbi::DispatcherClient*             source,
                             mqbi::Dispatcher*                   dispatcher,
                             const bsl::shared_ptr<bdlbb::Blob>& appData,
                             const bsl::shared_ptr<bdlbb::Blob>& options);

    /// Return true if the specified `syncPoint` is valid, false otherwise.
    static bool isValid(const bmqp_ctrlmsg::SyncPoint& syncPoint);

    /// Return true if the specified `spoPair` is valid, false otherwise.
    static bool isValid(const bmqp_ctrlmsg::SyncPointOffsetPair& spoPair);

    /// Set the specified `uri` to have the specified `pendingUnassignment`
    /// status in the specified `clusterState`.
    static void setPendingUnassignment(ClusterState*    clusterState,
                                       const bmqt::Uri& uri,
                                       bool             pendingUnassignment);

    /// Load into the specified `message` the message encoded in the
    /// specified `eventBlob` using the specified `allocator`.
    static void extractMessage(bmqp_ctrlmsg::ControlMessage* message,
                               const bdlbb::Blob&            eventBlob,
                               bslma::Allocator*             allocator);

    /// Assign an available node to each partition which is currently
    /// orphan or is assigned to a node which is not available, and load the
    /// results into the specified `partitions`, using the specified
    /// `clusterState`, `assignmentAlgo`, `clusterData` and `isCSLMode`
    /// flag.  Note that a healthy partition-node mapping is not modified.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    static void assignPartitions(
        bsl::vector<bmqp_ctrlmsg::PartitionPrimaryInfo>* partitions,
        ClusterState*                                    clusterState,
        mqbcfg::MasterAssignmentAlgorithm::Value         assignmentAlgo,
        const ClusterData&                               clusterData,
        bool                                             isCSLMode);

    /// Return the partition id to use for a new queue, taking into account
    /// current load of each partition in the specified `clusterState` and
    /// the specified `uri`.
    static int getNextPartitionId(const ClusterState& clusterState,
                                  const bmqt::Uri&    uri);

    /// Callback invoked when the specified 'partitionId' gets assigned to
    /// the specified 'primary' with the specified 'leaseId' and the
    /// specified 'status', replacing the specified 'oldPrimary' with the
    /// specified 'oldLeaseId', using the specified 'clusterData' and
    /// notifying the specified 'storageManager'.  Note that null is a valid
    /// value for the 'primary', and it implies that there is no primary for
    /// that partition.  Also note that this method will be invoked when the
    /// 'primary' or it status or both change.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    static void
    onPartitionPrimaryAssignment(ClusterData*          clusterData,
                                 mqbi::StorageManager* storageManager,
                                 int                   partitionId,
                                 mqbnet::ClusterNode*  primary,
                                 unsigned int          leaseId,
                                 bmqp_ctrlmsg::PrimaryStatus::Value status,
                                 mqbnet::ClusterNode*               oldPrimary,
                                 unsigned int oldLeaseId);

    /// Process the queue assignment in the specified `request`, received
    /// from the specified `requester`, using the specified `clusterState`,
    /// `clusterData`, `ledger`, `cluster`, `queueAssigningCb` and
    /// `allocator`.  Return the queue assignment result.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    static void
    processQueueAssignmentRequest(ClusterState*        clusterState,
                                  ClusterData*         clusterData,
                                  ClusterStateLedger*  ledger,
                                  const mqbi::Cluster* cluster,
                                  const bmqp_ctrlmsg::ControlMessage& request,
                                  mqbnet::ClusterNode*    requester,
                                  const QueueAssigningCb& queueAssigningCb,
                                  bslma::Allocator*       allocator);

    /// Populate the specified `advisory` with information describing a
    /// queue assignment of the specified `uri` living in the specified
    /// `domain`, using the specified `clusterState`, `clusterData`.  Load into
    /// the specified `key` the unique queue key generated.
    static void populateQueueAssignmentAdvisory(
        bmqp_ctrlmsg::QueueAssignmentAdvisory* advisory,
        mqbu::StorageKey*                      key,
        ClusterState*                          clusterState,
        ClusterData*                           clusterData,
        const bmqt::Uri&                       uri,
        const mqbi::Domain*                    domain);

    /// Populate the specified `advisory` with information describing a
    /// queue unassignment of the specified `uri` having the specified `key`
    /// and `partitionId`, using the specified `clusterData` and and
    /// `clusterState`.
    static void populateQueueUnassignedAdvisory(
        bmqp_ctrlmsg::QueueUnassignedAdvisory* advisory,
        ClusterData*                           clusterData,
        const bmqt::Uri&                       uri,
        const mqbu::StorageKey&                key,
        int                                    partitionId,
        const ClusterState&                    clusterState);

    /// Perform the actual assignment of the queue represented by the
    /// specified `uri` for a cluster member queue, that is assign it a
    /// queue key, a partition id, and some appIds in the specified
    /// `clusterState` of the specified `cluster` having the specified
    /// `clusterData`; and applying the corresponding queue assignment
    /// adviosry to the specified `ledger`.  Return a value indicating
    /// whether the assignment was successful or was definitively rejected,
    /// and populate the optionally specified `status` with a human readable
    /// error code and string in case of failure.  Also invoke the specified
    /// `queueAssigningCb` on success.  Use the specified `allocator` for
    /// memory allocations.  This method is called only on the leader node.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    static QueueAssignmentResult::Enum
    assignQueue(ClusterState*           clusterState,
                ClusterData*            clusterData,
                ClusterStateLedger*     ledger,
                const mqbi::Cluster*    cluster,
                const bmqt::Uri&        uri,
                const QueueAssigningCb& queueAssigningCb,
                bslma::Allocator*       allocator,
                bmqp_ctrlmsg::Status*   status = 0);

    /// Register a queue info for the queue with the specified `uri`,
    /// `partitionId`, `queueKey` and the optionally specified `appIdInfos`
    /// to the specified `clusterState` of the specified `cluster`.  Also
    /// invoke the specified `queueAssigningCb` on success.  If no
    /// `appIdInfos` is specified, use the appId infos from the domain
    /// config instead.  If the specified `forceUpdate` flag is true, update
    /// queue info even if it is valid but different from the specified
    /// `queueKey` and `partitionId`.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    static void registerQueueInfo(ClusterState*           clusterState,
                                  const mqbi::Cluster*    cluster,
                                  const bmqt::Uri&        uri,
                                  int                     partitionId,
                                  const mqbu::StorageKey& queueKey,
                                  const AppIdInfos&       appIdInfos,
                                  const QueueAssigningCb& queueAssigningCb,
                                  bool                    forceUpdate);

    /// Generate appKeys based on the appIds in the specified `domainConfig`
    /// and populate them into the specified `appIdInfos`.
    static void populateAppIdInfos(AppIdInfos*                appIdInfos,
                                   const mqbconfm::QueueMode& domainConfig);
    static void
    populateAppIdInfos(bsl::vector<bmqp_ctrlmsg::AppIdInfo>* appIdInfos,
                       const mqbconfm::QueueMode&            domainConfig);

    /// Register the specified `appId` for all queues in the specified
    /// `domain`, using the specified `clusterData` and `clusterState`.
    /// Apply the corresponding queue update advisory to the specified
    /// `ledger`.  Use the specified `allocator` for memory allocations.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    static void registerAppId(ClusterData*        clusterData,
                              ClusterStateLedger* ledger,
                              const ClusterState& clusterState,
                              const bsl::string&  appId,
                              const mqbi::Domain* domain,
                              bslma::Allocator*   allocator);

    /// Unregister the specified `appId` for all queues in the specified
    /// `domain`, using the specified `clusterData` and `clusterState`.
    /// Apply the corresponding queue update advisory to the specified
    /// `ledger`.  Use the specified `allocator` for memory allocations.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    static void unregisterAppId(ClusterData*        clusterData,
                                ClusterStateLedger* ledger,
                                const ClusterState& clusterState,
                                const bsl::string&  appId,
                                const mqbi::Domain* domain,
                                bslma::Allocator*   allocator);

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
    static void sendClusterState(
        ClusterData*          clusterData,
        ClusterStateLedger*   ledger,
        mqbi::StorageManager* storageManager,
        const ClusterState&   clusterState,
        bool                  sendPartitionPrimaryInfo,
        bool                  sendQueuesInfo,
        mqbnet::ClusterNode*  node = 0,
        const bsl::vector<bmqp_ctrlmsg::PartitionPrimaryInfo>& partitions =
            bsl::vector<bmqp_ctrlmsg::PartitionPrimaryInfo>());

    /// Append to the specified `out` a newly created cluster node
    /// definition with the specified `name`, `dataCenter`, `port` and `id`.
    /// Use the optionally specified `allocator` for memory allocations.
    static void appendClusterNode(bsl::vector<mqbcfg::ClusterNode>* out,
                                  const bslstl::StringRef&          name,
                                  const bslstl::StringRef&          dataCenter,
                                  int                               port,
                                  int                               id,
                                  bslma::Allocator* allocator = 0);

    /// Apply the specified `message` to the specified `state` using the
    /// specified `clusterData`.
    static void apply(ClusterState*                       clusterState,
                      const bmqp_ctrlmsg::ClusterMessage& clusterMessage,
                      const ClusterData&                  clusterData);

    /// Compare the specified `state` against the specified `reference`
    /// state and return 0 if they compare equal, otherwise return a
    /// non-zero error code and populate the specified `errorDescription`
    /// with descriptive information about the inconsistencies found.
    static int validateState(bsl::ostream&       errorDescription,
                             const ClusterState& state,
                             const ClusterState& reference);

    /// Invoked to perform validation of the contents (on disk) of the
    /// specified `ledger` against the specified "real" `clusterState` of
    /// the specified `cluster`, using the specified `clusterData`.  Use the
    /// specified `allocator` for memory allocations.  Logs a descriptive
    /// error message if inconsistencies are detected.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    /// dispatcher thread.
    ///
    /// TBD: This is mostly temporary, used during the integrating of CSL.
    static void validateClusterStateLedger(mqbi::Cluster*            cluster,
                                           const ClusterStateLedger& ledger,
                                           const ClusterState& clusterState,
                                           const ClusterData&  clusterData,
                                           bslma::Allocator*   allocator);

    /// Load the cluster state pointed to by the specified `iterator` into
    /// the specified `state` using the specified `clusterData`.  Return 0
    /// on success, or a non-zero error code on error.  Use the optionally
    /// specified `allocator` for memory allocations.
    static int load(ClusterState*               state,
                    ClusterStateLedgerIterator* iterator,
                    const ClusterData&          clusterData,
                    bslma::Allocator*           allocator = 0);

    /// Load the partitions info of the specified `state` into the
    /// specified `out`.
    static void
    loadPartitionsInfo(bsl::vector<bmqp_ctrlmsg::PartitionPrimaryInfo>* out,
                       const ClusterState&                              state);

    /// Load in the specified `out` the queues info of the specified
    /// `state`.  If the specified `includeAppIds` is true, then the appId
    /// info for the queues will be loaded as well.
    static void loadQueuesInfo(bsl::vector<bmqp_ctrlmsg::QueueInfo>* out,
                               const ClusterState&                   state,
                               bool includeAppIds);

    /// Load into the specified `out` the list of peer nodes using the
    /// specified `clusterData`.
    ///
    /// THREAD: Executed by the cluster *DISPATCHER* thread or the
    //          *QUEUE_DISPATCHER* thread.
    static void loadPeerNodes(bsl::vector<mqbnet::ClusterNode*>* out,
                              const ClusterData&                 clusterData);

    /// Load into the specified `out` the latest LSN stored in the specified
    /// `ledger`, using the specified `clusterData`.  Return 0 on success,
    /// and a non-zero error code on failure.  Note that this involves
    /// iteration over the entire ledger which can be an expensive operation.
    /// static
    static int latestLedgerLSN(bmqp_ctrlmsg::LeaderMessageSequence* out,
                               const ClusterStateLedger&            ledger,
                               const ClusterData& clusterData);
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ------------------
// struct ClusterUtil
// ------------------
inline void
ClusterUtil::generateNack(bmqt::AckResult::Enum               status,
                          const bmqp::PutHeader&              putHeader,
                          mqbi::DispatcherClient*             source,
                          mqbi::Dispatcher*                   dispatcher,
                          const bsl::shared_ptr<bdlbb::Blob>& appData,
                          const bsl::shared_ptr<bdlbb::Blob>& options)
{
    BSLS_ASSERT_SAFE(status != bmqt::AckResult::e_SUCCESS);

    bmqp::AckMessage ackMessage(bmqp::ProtocolUtil::ackResultToCode(status),
                                putHeader.correlationId(),
                                putHeader.messageGUID(),
                                putHeader.queueId());

    mqbi::DispatcherEvent* ev = dispatcher->getEvent(source);

    if (appData) {
        (*ev)
            .makeAckEvent()
            .setAckMessage(ackMessage)
            .setBlob(appData)
            .setOptions(options);
    }
    else {
        BSLS_ASSERT_SAFE(!options);
        (*ev).makeAckEvent().setAckMessage(ackMessage);
    }

    dispatcher->dispatchEvent(ev, source);
}

inline bool ClusterUtil::isValid(const bmqp_ctrlmsg::SyncPoint& syncPoint)
{
    return syncPoint.primaryLeaseId() >= 1 && syncPoint.sequenceNum() >= 1;
}

inline bool
ClusterUtil::isValid(const bmqp_ctrlmsg::SyncPointOffsetPair& spoPair)
{
    if (!isValid(spoPair.syncPoint())) {
        return false;  // RETURN
    }

    return 0 != spoPair.offset();
}

}  // close package namespace

namespace bmqp_ctrlmsg {

// FREE OPERATORS

/// Return true if the specified `lhs` is smaller than the specified `rhs`,
/// false otherwise.
bool operator<(const PartitionSequenceNumber& lhs,
               const PartitionSequenceNumber& rhs);

/// Return true if the specified `lhs` is smaller than or equal to the
/// specified `rhs`, false otherwise.
bool operator<=(const PartitionSequenceNumber& lhs,
                const PartitionSequenceNumber& rhs);

/// Return true if the specified `lhs` is greater than the specified `rhs`,
/// false otherwise.
bool operator>(const PartitionSequenceNumber& lhs,
               const PartitionSequenceNumber& rhs);

/// Return true if the specified `lhs` is greater than or equal to the
/// specified `rhs`, false otherwise.
bool operator>=(const PartitionSequenceNumber& lhs,
                const PartitionSequenceNumber& rhs);

/// Format the specified `value` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream&                  stream,
                         const PartitionSequenceNumber& value);

/// Return true if the specified `lhs` is smaller than the specified `rhs`,
/// false otherwise.
bool operator<(const bmqp_ctrlmsg::SyncPoint& lhs,
               const bmqp_ctrlmsg::SyncPoint& rhs);

/// Return true if the specified `lhs` is smaller than or equal to the
/// specified `rhs`, false otherwise.
bool operator<=(const bmqp_ctrlmsg::SyncPoint& lhs,
                const bmqp_ctrlmsg::SyncPoint& rhs);

/// Return true if the specified `lhs` is greater than the specified `rhs`,
/// false otherwise.
bool operator>(const bmqp_ctrlmsg::SyncPoint& lhs,
               const bmqp_ctrlmsg::SyncPoint& rhs);

/// Return true if the specified `lhs` is smaller than the specified `rhs`,
/// false otherwise.
bool operator<(const bmqp_ctrlmsg::SyncPointOffsetPair& lhs,
               const bmqp_ctrlmsg::SyncPointOffsetPair& rhs);

/// Return true if the specified `lhs` is smaller than or equal to the
/// specified `rhs`, false otherwise.
bool operator<=(const bmqp_ctrlmsg::SyncPointOffsetPair& lhs,
                const bmqp_ctrlmsg::SyncPointOffsetPair& rhs);

/// Return true if the specified `lhs` is greater than the specified `rhs`,
/// false otherwise.
bool operator>(const bmqp_ctrlmsg::SyncPointOffsetPair& lhs,
               const bmqp_ctrlmsg::SyncPointOffsetPair& rhs);

}  // close package namespace

// FREE OPERATORS
inline bool
bmqp_ctrlmsg::operator<(const bmqp_ctrlmsg::PartitionSequenceNumber& lhs,
                        const bmqp_ctrlmsg::PartitionSequenceNumber& rhs)
{
    if (lhs.primaryLeaseId() != rhs.primaryLeaseId()) {
        return lhs.primaryLeaseId() < rhs.primaryLeaseId();  // RETURN
    }

    if (lhs.sequenceNumber() != rhs.sequenceNumber()) {
        return lhs.sequenceNumber() < rhs.sequenceNumber();  // RETURN
    }

    return false;
}

inline bool
bmqp_ctrlmsg::operator<=(const bmqp_ctrlmsg::PartitionSequenceNumber& lhs,
                         const bmqp_ctrlmsg::PartitionSequenceNumber& rhs)
{
    return (lhs < rhs) || (lhs == rhs);
}

inline bool
bmqp_ctrlmsg::operator>(const bmqp_ctrlmsg::PartitionSequenceNumber& lhs,
                        const bmqp_ctrlmsg::PartitionSequenceNumber& rhs)
{
    return !(lhs <= rhs);
}

inline bool bmqp_ctrlmsg::operator>=(const PartitionSequenceNumber& lhs,
                                     const PartitionSequenceNumber& rhs)
{
    return !(lhs < rhs);
}

inline bool bmqp_ctrlmsg::operator<(const bmqp_ctrlmsg::SyncPoint& lhs,
                                    const bmqp_ctrlmsg::SyncPoint& rhs)
{
    // Primary leaseId should be compared first, followed by sequenceNum,
    // remaining 'offset' fields can be compared in any order after that.

    if (lhs.primaryLeaseId() != rhs.primaryLeaseId()) {
        return lhs.primaryLeaseId() < rhs.primaryLeaseId();  // RETURN
    }

    if (lhs.sequenceNum() != rhs.sequenceNum()) {
        return lhs.sequenceNum() < rhs.sequenceNum();  // RETURN
    }

    if (lhs.dataFileOffsetDwords() != rhs.dataFileOffsetDwords()) {
        return lhs.dataFileOffsetDwords() < rhs.dataFileOffsetDwords();
        // RETURN
    }

    if (lhs.qlistFileOffsetWords() != rhs.qlistFileOffsetWords()) {
        return lhs.qlistFileOffsetWords() < rhs.qlistFileOffsetWords();
        // RETURN
    }

    return false;
}

inline bool bmqp_ctrlmsg::operator<=(const bmqp_ctrlmsg::SyncPoint& lhs,
                                     const bmqp_ctrlmsg::SyncPoint& rhs)
{
    if (lhs == rhs) {
        return true;  // RETURN
    }

    return lhs < rhs;
}

inline bool bmqp_ctrlmsg::operator>(const bmqp_ctrlmsg::SyncPoint& lhs,
                                    const bmqp_ctrlmsg::SyncPoint& rhs)
{
    return !(lhs <= rhs);
}

inline bool
bmqp_ctrlmsg::operator<(const bmqp_ctrlmsg::SyncPointOffsetPair& lhs,
                        const bmqp_ctrlmsg::SyncPointOffsetPair& rhs)
{
    if (lhs.syncPoint() != rhs.syncPoint()) {
        return lhs.syncPoint() < rhs.syncPoint();  // RETURN
    }

    if (lhs.offset() != rhs.offset()) {
        return lhs.offset() < rhs.offset();  // RETURN
    }

    return false;
}

inline bool
bmqp_ctrlmsg::operator<=(const bmqp_ctrlmsg::SyncPointOffsetPair& lhs,
                         const bmqp_ctrlmsg::SyncPointOffsetPair& rhs)
{
    if (lhs == rhs) {
        return true;  // RETURN
    }

    return lhs < rhs;
}

inline bool
bmqp_ctrlmsg::operator>(const bmqp_ctrlmsg::SyncPointOffsetPair& lhs,
                        const bmqp_ctrlmsg::SyncPointOffsetPair& rhs)
{
    return !(lhs <= rhs);
}

}  // close enterprise namespace

#endif
