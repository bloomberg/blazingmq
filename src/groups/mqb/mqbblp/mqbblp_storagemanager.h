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

#ifndef INCLUDED_MQBBLP_STORAGEMANAGER
#define INCLUDED_MQBBLP_STORAGEMANAGER

/// @file mqbblp_storagemanager.h
///
/// @brief Provide a storage manager, in charge of BlazingMQ storage.
///
/// @todo Document component
///
/// Thread Safety                               {#mqbblp_storagemanager_thread}
/// =============
///
/// Thread safe.

// MQB
#include <mqbc_clusterdata.h>
#include <mqbc_clusterstate.h>
#include <mqbc_storageutil.h>
#include <mqbcfg_messages.h>
#include <mqbconfm_messages.h>
#include <mqbi_dispatcher.h>
#include <mqbi_domain.h>
#include <mqbi_storagemanager.h>
#include <mqbnet_multirequestmanager.h>
#include <mqbs_filestore.h>
#include <mqbs_filestoreprotocol.h>
#include <mqbu_storagekey.h>

// BMQ
#include <bmqma_countingallocatorstore.h>
#include <bmqt_uri.h>

// BDE
#include <ball_log.h>
#include <bdlbb_blob.h>
#include <bdlcc_objectpool.h>
#include <bdlcc_sharedobjectpool.h>
#include <bdlmt_eventscheduler.h>
#include <bdlmt_fixedthreadpool.h>
#include <bsl_functional.h>
#include <bsl_memory.h>
#include <bsl_ostream.h>
#include <bsl_string.h>
#include <bsl_vector.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bslmt_latch.h>
#include <bslmt_mutex.h>
#include <bsls_assert.h>
#include <bsls_atomic.h>
#include <bsls_types.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace bdlmt {
class FixedThreadPool;
}
namespace bslmt {
class Latch;
}
namespace mqbcmd {
class ClusterStorageSummary;
}
namespace mqbcmd {
class StorageCommand;
}
namespace mqbcmd {
class StorageQueueInfo;
}
namespace mqbcmd {
class StorageResult;
}
namespace mqbi {
class Cluster;
}
namespace mqbi {
class Queue;
}
namespace mqbnet {
class Cluster;
}
namespace mqbnet {
class ClusterNode;
}
namespace mqbs {
class ReplicatedStorage;
}
namespace mqbs {
class VirtualStorage;
}

namespace mqbblp {

// FORWARD DECLARATION
class RecoveryManager;

// ====================
// class StorageManager
// ====================

/// Storage Manager, in charge of all the partitions.
class StorageManager BSLS_KEYWORD_FINAL : public mqbi::StorageManager,
                                          public mqbc::StorageMonitor {
  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBBLP.STORAGEMANAGER");

  public:
    // TYPES

    /// Pool of shared pointers to Blobs
    typedef bdlcc::SharedObjectPool<
        bdlbb::Blob,
        bdlcc::ObjectPoolFunctors::DefaultCreator,
        bdlcc::ObjectPoolFunctors::RemoveAll<bdlbb::Blob> >
        BlobSpPool;

    typedef bsl::vector<mqbnet::ClusterNode*> ClusterNodes;

  private:
    // PRIVATE CONSTANTS

    /// For brevity.
    static const int k_KEY_LEN = mqbs::FileStoreProtocol::k_KEY_LENGTH;

  private:
    // PRIVATE TYPES

    /// Disambiguate `StorageSp`, which is inherited from both
    /// `mqbi::StorageManager` and `mqbc::storageMonitor`.
    typedef mqbi::StorageManager::StorageSp StorageSp;

    typedef bdlmt::EventScheduler::RecurringEventHandle RecurringEventHandle;

    typedef mqbc::StorageUtil::FileStores FileStores;

    typedef bslma::ManagedPtr<RecoveryManager> RecoveryManagerMp;

    typedef bsl::vector<unsigned int> PrimaryLeaseIds;

    typedef mqbi::StorageManager_PartitionInfo PartitionInfo;
    typedef bsl::vector<PartitionInfo>         PartitionInfoVec;

    typedef bsl::function<void(int, const PrimaryLeaseIds&)> RecoveryStatusCb;

    typedef bsl::shared_ptr<bdlbb::Blob> BlobSp;

    typedef mqbs::DataStoreConfig::QueueKeyInfoMap QueueKeyInfoMap;

    typedef mqbc::ClusterStatePartitionInfo ClusterStatePartitionInfo;

    typedef mqbc::ClusterData::RequestManagerType RequestManagerType;

    typedef mqbc::ClusterData::MultiRequestManagerType MultiRequestManagerType;

    typedef MultiRequestManagerType::RequestContextSp RequestContextSp;

    typedef MultiRequestManagerType::NodeResponsePair  NodeResponsePair;
    typedef MultiRequestManagerType::NodeResponsePairs NodeResponsePairs;
    typedef MultiRequestManagerType::NodeResponsePairsIter
        NodeResponsePairsIter;
    typedef MultiRequestManagerType::NodeResponsePairsConstIter
        NodeResponsePairsConstIter;

    typedef mqbc::StorageUtil::DomainQueueMessagesCountMaps
        DomainQueueMessagesCountMaps;

  private:
    // DATA

    /// Allocator to use.
    bslma::Allocator* d_allocator_p;

    /// Allocatro store to spawn new allocators for sub-components.
    bmqma::CountingAllocatorStore d_allocators;

    bsls::AtomicBool d_isStarted;

    /// Flag to denote if a low disk space warning was issued.  This flag is
    /// used *only* for logging purposes (see `storageMonitorCb` impl).
    bool d_lowDiskspaceWarning;

    /// Mutex to protect access to `d_unrecognizedDomains` and its elements.
    bslmt::Mutex d_unrecognizedDomainsLock;

    /// List of `DomainQueueMessagesMap`, indexed by `partitionId`.
    ///
    /// Each `DomainQueueMessagesMap` is a map of `unrecognized domain name ->
    /// queue messages info` found during storage recovery, either due to
    /// genuine domain migration or misconfiguration.
    DomainQueueMessagesCountMaps d_unrecognizedDomains;

    /// `SharedObjectPool` of blobs to use.
    BlobSpPool* d_blobSpPool_p;

    /// Domain factory to use.
    mqbi::DomainFactory* d_domainFactory_p;

    /// Dispatcher to use.
    mqbi::Dispatcher* d_dispatcher_p;

    /// Cluster config to use.
    const mqbcfg::ClusterDefinition& d_clusterConfig;

    mqbi::Cluster* d_cluster_p;

    mqbc::ClusterData* d_clusterData_p;

    mqbc::ClusterState* d_clusterState_p;

    /// Recovery manager.  Empty if this is a local cluster.
    RecoveryManagerMp d_recoveryManager_mp;

    /// List of all partitions, indexed by `partitionId`.
    FileStores d_fileStores;

    /// Thread pool used for any standalone work that can be offloaded to
    /// non-partition-dispatcher therads.  It is used by the partitions owned
    /// by this object.
    bdlmt::FixedThreadPool* d_miscWorkThreadPool_p;

    /// Number of partitions whose recovery has been fully completed by the
    /// recovery manager.  This variable needs to be atomic because it's
    /// touched from the dispatcher threads of all partitions.
    bsls::AtomicInt d_numPartitionsRecoveredFully;

    /// Number of partitions which have completed recovery of file-backed
    /// queues and their virtual storages.  This variable needs to be atomic
    /// because it's touched from the dispatched threads of all partitions.
    bsls::AtomicInt d_numPartitionsRecoveredQueues;

    RecoveryStatusCb d_recoveryStatusCb;

    PartitionPrimaryStatusCb d_partitionPrimaryStatusCb;

    RecurringEventHandle d_storageMonitorEventHandle;

    RecurringEventHandle d_gcMessagesEventHandle;

    /// `PrimaryLeaseIds` recovered from each partition.  Each element in this
    /// vector represents the recovered primaryLeaseId for the partition at
    /// that index.  Each element in this vector is modified once in
    /// `onPartitionRecovery` in the dispatcher therad associated with the
    /// partition (i.e., index of the element).  This variable is not used once
    /// this storage manager has notified the upper layer of its recovery
    /// status via `d_recoveryStatusCb`.
    PrimaryLeaseIds d_recoveredPrimaryLeaseIds;

    /// Vector of `PartitionInfo` indexed by partitionId.  An element in this
    /// container represents the *latest* primaryNode, leaseId, and status for
    /// the partitionId at that element's index.
    PartitionInfoVec d_partitionInfoVec;

    bsls::Types::Uint64 d_minimumRequiredDiskSpace;

    /// Replication factor used to configure `FileStores`.
    int d_replicationFactor;

  private:
    /// Not implemented.
    StorageManager(const StorageManager&) BSLS_KEYWORD_DELETED;

    /// Not implemented.
    StorageManager& operator=(const StorageManager&) BSLS_KEYWORD_DELETED;

  private:
    // PRIVATE MANIPULATORS

    /// Perform recovery of partition with specified `partitionId` assigned
    /// to the dispatcher thread identified by the specified `processorId`.
    void startRecoveryCb(int partitionId);

    /// Callback executed by `mqbblp::RecoveryManager` after recovery for
    /// the specified `partitionId` is complete with the specified `status`,
    /// having the specified `recoveryEvents` and `recoveryPeer`.  Executed
    /// by the dispatcher thread associated with `partitionId`.
    void onPartitionRecovery(int                        partitionId,
                             int                        status,
                             const bsl::vector<BlobSp>& recoveryEvents,
                             mqbnet::ClusterNode*       recoveryPeer,
                             bsls::Types::Int64         recoveryStartTime);

    /// Callback executed by `mqbblp::RecoveryManager` after primary sync
    /// for the specified `partitionId` is complete with the specified
    /// `status`.
    ///
    /// THREAD: Executed by the dispatcher thread associated with
    ///         'partitionId'.
    void onPartitionPrimarySync(int partitionId, int status);

    /// Gracefully shut down the partition associated with the specified
    /// `partitionId` assigned to the dispatcher thread identified by the
    /// specified `processorId`, and arrive on the specified `latch` when
    /// shut down is complete..  Executed by the dispatcher thread
    /// associated with `processorId`.
    void shutdownCb(int partitionId, bslmt::Latch* latch);

    // Replicas create/update/delete storage upon Replication events
    // (queueCreationCb/queueDeletionCb).
    void queueCreationCb(int                     partitionId,
                         const bmqt::Uri&        uri,
                         const mqbu::StorageKey& queueKey,
                         const AppInfos&         appIdKeyPairs,
                         bool                    isNewQueue);

    void queueDeletionCb(int                     partitionId,
                         const bmqt::Uri&        uri,
                         const mqbu::StorageKey& queueKey,
                         const mqbu::StorageKey& appKey);

    /// Callback executed when the partition having the specified
    /// `partitionId` has performed recovery and recovered file-backed
    /// queues and their virtual storages in the specified
    /// `queueKeyInfoMap`.
    ///
    /// THREAD: Executed by the dispatcher thread of the partition.
    void recoveredQueuesCb(int                    partitionId,
                           const QueueKeyInfoMap* queueKeyInfoMap);

    /// Executed by queue/file-store dispatcher thread.
    void setPrimaryForPartitionDispatched(int                  partitionId,
                                          mqbnet::ClusterNode* primaryNode,
                                          unsigned int         primaryLeaseId,
                                          const ClusterNodes&  peers);

    /// Executed by queue/file-store dispatcher thread.
    void clearPrimaryForPartitionDispatched(int                  partitionId,
                                            mqbnet::ClusterNode* primary);

    /// Executed by queue/file-store dispatcher thread.
    void
    processStorageEventDispatched(int partitionId,
                                  const bsl::shared_ptr<bdlbb::Blob>& blob,
                                  mqbnet::ClusterNode*                source);

    /// Executed in cluster dispatcher thread.
    void processPartitionSyncEvent(const mqbevt::StorageEvent& event);

    /// Executed by queue/file-store dispatcher thread.
    void processPartitionSyncEventDispatched(
        int                                 partitionId,
        const bsl::shared_ptr<bdlbb::Blob>& blob,
        mqbnet::ClusterNode*                source);

    /// Executed by queue/file-store dispatcher thread.
    void processStorageSyncRequestDispatched(
        int                                 partitionId,
        const bmqp_ctrlmsg::ControlMessage& message,
        mqbnet::ClusterNode*                source);

    /// Executed by queue/file-store dispatcher thread.
    void processPartitionSyncStateRequestDispatched(
        int                                 partitionId,
        const bmqp_ctrlmsg::ControlMessage& message,
        mqbnet::ClusterNode*                source);

    /// Executed by queue/file-store dispatcher thread.
    void processPartitionSyncDataRequestDispatched(
        int                                 partitionId,
        const bmqp_ctrlmsg::ControlMessage& message,
        mqbnet::ClusterNode*                source);

    /// Executed by queue/file-store dispatcher thread.
    void processPartitionSyncDataRequestStatusDispatched(
        int                                 partitionId,
        const bmqp_ctrlmsg::ControlMessage& message,
        mqbnet::ClusterNode*                source);

    void processShutdownEventDispatched(int partitionId);

    /// Explicitly call `flush` on all FileStores to enforce their GC.
    void forceFlushFileStores();

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(StorageManager, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a new `StorageManager` using the specified `allocator` and
    /// suplying to it the specified `blobBufferFactory`.
    StorageManager(const mqbcfg::ClusterDefinition& clusterConfig,
                   mqbi::Cluster*                   cluster,
                   mqbc::ClusterData*               clusterData,
                   mqbc::ClusterState*              clusterState,
                   const RecoveryStatusCb&          recoveryStatusCb,
                   const PartitionPrimaryStatusCb&  partitionPrimaryStatusCb,
                   mqbi::DomainFactory*             domainFactory,
                   mqbi::Dispatcher*                dispatcher,
                   bdlmt::FixedThreadPool*          threadPool,
                   bslma::Allocator*                allocator);

    /// Destructor
    ~StorageManager() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    /// Start this storage manager.  Return 0 on success, or a non-zero rc
    /// otherwise, populating the specified `errorDescription` with a
    /// description of the error.
    ///
    /// THREAD: Executed by the cluster's dispatcher thread.
    int start(bsl::ostream& errorDescription) BSLS_KEYWORD_OVERRIDE;

    /// Stop this storage manager.
    void stop() BSLS_KEYWORD_OVERRIDE;

    /// Initialize the queue key info map based on information in the specified
    /// `clusterState`.  Note that this method should only be called once;
    /// subsequent calls will be ignored.
    void initializeQueueKeyInfoMap(const mqbc::ClusterState& clusterState)
        BSLS_KEYWORD_OVERRIDE;

    /// Register a queue with the specified `uri`, `queueKey` and
    /// `partitionId`, having the specified `appIdKeyPairs`, and belonging
    /// to the specified `domain`.  Load into the specified `storage` the
    /// associated queue storage created.
    ///
    /// THREAD: Executed by the Client's dispatcher thread.
    void registerQueue(const bmqt::Uri&        uri,
                       const mqbu::StorageKey& queueKey,
                       int                     partitionId,
                       const AppInfos&         appIdKeyPairs,
                       mqbi::Domain*           domain) BSLS_KEYWORD_OVERRIDE;

    /// Synchronously unregister the queue with the specified `uri` from the
    /// specified `partitionId`.  Behavior is undefined unless this routine
    /// is invoked from the cluster dispatcher thread.
    ///
    /// THREAD: Executed by the Client's dispatcher thread.
    void unregisterQueue(const bmqt::Uri& uri,
                         int              partitionId) BSLS_KEYWORD_OVERRIDE;

    /// Configure the fanout queue having specified `uri` and `queueKey`,
    /// assigned to the specified `partitionId` to have the specified
    /// `addedIdKeyPairs` appId/appKey pairs added and `removedIdKeyPairs`
    /// appId/appKey pairs removed.  Return zero on success, and non-zero
    /// value otherwise.  Behavior is undefined unless this function is
    /// invoked at the primary node.  Behavior is also undefined unless the
    /// queue is configured in fanout mode.
    ///
    /// THREAD: Executed by the Queue's dispatcher thread.
    int updateQueuePrimary(const bmqt::Uri& uri,
                           int              partitionId,
                           const AppInfos&  addedIdKeyPairs,
                           const AppInfos&  removedIdKeyPairs)
        BSLS_KEYWORD_OVERRIDE;

    /// Behavior is undefined unless the specified 'partitionId' is in range
    /// and the specified 'primaryNode' is not null.
    ///
    /// THREAD: Executed in cluster dispatcher thread.
    void
    setPrimaryForPartition(int                  partitionId,
                           mqbnet::ClusterNode* primaryNode,
                           unsigned int primaryLeaseId) BSLS_KEYWORD_OVERRIDE;

    /// Behavior is undefined unless the specified 'partitionId' is in range
    /// and the specified 'primaryNode' is not null.
    ///
    /// THREAD: Executed in cluster dispatcher thread.
    void clearPrimaryForPartition(int                  partitionId,
                                  mqbnet::ClusterNode* primary)
        BSLS_KEYWORD_OVERRIDE;

    /// Set the primary status of the specified 'partitionId' to the specified
    /// 'value'.
    ///
    /// THREAD: Executed in cluster dispatcher thread.
    void setPrimaryStatusForPartition(int partitionId,
                                      bmqp_ctrlmsg::PrimaryStatus::Value value)
        BSLS_KEYWORD_OVERRIDE;

    /// Stop all Partition FSMs.
    ///
    /// THREAD: Executed in cluster dispatcher thread.
    void stopPFSMs() BSLS_KEYWORD_OVERRIDE;

    /// Apply `RST_UNKNOWN` event to the Partition FSM for the specified
    /// `partitionId`.
    ///
    /// THREAD: Executed in cluster dispatcher thread.
    void detectPrimaryLossInPFSM(int partitionId) BSLS_KEYWORD_OVERRIDE;

    /// Apply DETECT_SelfPrimary event to Partition FSM using the specified
    /// `partitionId`, `primaryNode`, `primaryLeaseId`.
    ///
    /// THREAD: Executed in cluster dispatcher thread.
    void
    detectSelfPrimaryInPFSM(int                  partitionId,
                            mqbnet::ClusterNode* primaryNode,
                            unsigned int primaryLeaseId) BSLS_KEYWORD_OVERRIDE;

    /// Apply DETECT_SelfReplica event to Partition FSM using the specified
    /// `partitionId`, `primaryNode` and `primaryLeaseId`.
    ///
    /// THREAD: Executed in cluster dispatcher thread.
    void
    detectSelfReplicaInPFSM(int                  partitionId,
                            mqbnet::ClusterNode* primaryNode,
                            unsigned int primaryLeaseId) BSLS_KEYWORD_OVERRIDE;

    /// Process primary state request received from the specified `source`
    /// with the specified `message`.
    void processPrimaryStateRequest(
        const bmqp_ctrlmsg::ControlMessage& message,
        mqbnet::ClusterNode*                source) BSLS_KEYWORD_OVERRIDE;

    void processReplicaStateRequest(
        const bmqp_ctrlmsg::ControlMessage& message,
        mqbnet::ClusterNode*                source) BSLS_KEYWORD_OVERRIDE;

    /// Process replica data request received from the specified `source`
    /// with the specified `message`.
    void processReplicaDataRequest(const bmqp_ctrlmsg::ControlMessage& message,
                                   mqbnet::ClusterNode*                source)
        BSLS_KEYWORD_OVERRIDE;

    int configureStorage(bsl::ostream&                   errorDescription,
                         bsl::shared_ptr<mqbi::Storage>* out,
                         const bmqt::Uri&                uri,
                         const mqbu::StorageKey&         queueKey,
                         int                             partitionId,
                         const bsls::Types::Int64        messageTtl,
                         const int                       maxDeliveryAttempts,
                         const mqbconfm::StorageDefinition& storageDef)
        BSLS_KEYWORD_OVERRIDE;

    /// Executed in cluster dispatcher thread.
    void processStorageEvent(const mqbevt::StorageEvent& event)
        BSLS_KEYWORD_OVERRIDE;

    /// Executed by any thread.
    void processStorageSyncRequest(const bmqp_ctrlmsg::ControlMessage& message,
                                   mqbnet::ClusterNode*                source)
        BSLS_KEYWORD_OVERRIDE;

    /// Executed by any thread.
    void processPartitionSyncStateRequest(
        const bmqp_ctrlmsg::ControlMessage& message,
        mqbnet::ClusterNode*                source) BSLS_KEYWORD_OVERRIDE;

    /// Executed by any thread.
    void processPartitionSyncDataRequest(
        const bmqp_ctrlmsg::ControlMessage& message,
        mqbnet::ClusterNode*                source) BSLS_KEYWORD_OVERRIDE;

    /// Executed by any thread.
    void processPartitionSyncDataRequestStatus(
        const bmqp_ctrlmsg::ControlMessage& message,
        mqbnet::ClusterNode*                source) BSLS_KEYWORD_OVERRIDE;

    /// Executed in cluster dispatcher thread.
    void processRecoveryEvent(const mqbevt::RecoveryEvent& event)
        BSLS_KEYWORD_OVERRIDE;

    /// Executed in IO thread.
    void
    processReceiptEvent(const bmqp::Event&   event,
                        mqbnet::ClusterNode* source) BSLS_KEYWORD_OVERRIDE;

    /// Executed by any thread.
    void processPrimaryStatusAdvisory(
        const bmqp_ctrlmsg::PrimaryStatusAdvisory& advisory,
        mqbnet::ClusterNode* source) BSLS_KEYWORD_OVERRIDE;

    /// Executed by any thread.
    void processReplicaStatusAdvisory(int                  partitionId,
                                      mqbnet::ClusterNode* source,
                                      bmqp_ctrlmsg::NodeStatus::Value status)
        BSLS_KEYWORD_OVERRIDE;

    /// Executed by any thread.
    void processShutdownEvent() BSLS_KEYWORD_OVERRIDE;

    /// Process the specified `command`, and load the result to the
    /// specified `result`.  This function can be invoked from any thread,
    /// and will block until the potentially asynchronous operation is
    /// complete.
    void processCommand(mqbcmd::StorageResult*        result,
                        const mqbcmd::StorageCommand& command)
        BSLS_KEYWORD_OVERRIDE;

    /// GC the queues from unrecognized domains, if any.
    void gcUnrecognizedDomainQueues() BSLS_KEYWORD_OVERRIDE;

    /// Purge the queues on a given domain.
    int
    purgeQueueOnDomain(mqbcmd::StorageResult* result,
                       const bsl::string& domainName) BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS

    /// Return the processor handle in charge of the specified
    /// `partitionId`.  The behavior is undefined if `partitionId` does not
    /// represent a valid partition id.
    mqbi::Dispatcher::ProcessorHandle
    processorForPartition(int partitionId) const BSLS_KEYWORD_OVERRIDE;

    /// Return true if the queue having specified `uri` and assigned to the
    /// specified `partitionId` has no messages, false in any other case.
    /// Behavior is undefined unless this routine is invoked from cluster
    /// dispatcher thread.
    bool isStorageEmpty(const bmqt::Uri& uri,
                        int partitionId) const BSLS_KEYWORD_OVERRIDE;

    /// Return true if the queue having the specified `uri` and assigned to
    /// the specified `partitionId` has a registered storage *and*, if the
    /// specified `appId` is non-empty, that `appId` is registered on it.
    bool hasStorage(const bmqt::Uri&   uri,
                    const bsl::string& appId,
                    int partitionId) const BSLS_KEYWORD_OVERRIDE;

    /// Return partition corresponding to the specified `partitionId`.  The
    /// behavior is undefined if `partitionId` does not represent a valid
    /// partition id.
    mqbs::FileStore& fileStore(int partitionId) const BSLS_KEYWORD_OVERRIDE;

    /// Load into the specified `result` all the storages of the specified
    /// `partitionId`.
    void loadAllStorages(bsl::vector<StorageSp>* result,
                         int partitionId) BSLS_KEYWORD_OVERRIDE;
};

// --------------------
// class StorageManager
// --------------------

// PUBLIC ACCESSORS
inline mqbi::Dispatcher::ProcessorHandle
StorageManager::processorForPartition(int partitionId) const
{
    // executed by *ANY* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_fileStores.size()));

    return d_fileStores[partitionId]->processorId();
}

inline mqbs::FileStore& StorageManager::fileStore(int partitionId) const
{
    // executed by *ANY* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_fileStores.size()));

    return *d_fileStores[partitionId].get();
}

inline void StorageManager::loadAllStorages(bsl::vector<StorageSp>* result,
                                            int partitionId)
{
    mqbc::StorageMonitor::loadAllStorages(result, partitionId);
}

}  // close package namespace
}  // close enterprise namespace

#endif
