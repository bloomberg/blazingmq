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

// mqbc_storagemanager.h                                              -*-C++-*-
#ifndef INCLUDED_MQBC_STORAGEMANAGER
#define INCLUDED_MQBC_STORAGEMANAGER

//@PURPOSE: Provide an implementation for storage manager, in charge of BMQ
//          storage.
//
//@CLASSES:
//  mqbc::StorageManager:         Provide a storage manager which wraps over
//                                all operations to be performed on storage
//                                partitions. Every operation including and not
//                                limited to change in partition node ownership
//                                and syncing partition info should be routed
//                                via this component.
//  mqbc::StorageManagerIterator: Iterator for iterating over the storages of
//                                partitions being managed by storage manager.
//
//@DESCRIPTION: The 'mqbc::StorageManager' class is a concrete implementation
//              of the 'mqbi::StorageManager' base protocol to manage lifecycle
//              and data for all the partitions being managed by this node as
//              allocated by the mqbi::ClusterStateManager component.
//
/// Thread Safety
///-------------
// Thread safe.

// MQB
#include <mqbc_clusterdata.h>
#include <mqbc_clusterstate.h>
#include <mqbc_partitionfsm.h>
#include <mqbc_partitionfsmobserver.h>
#include <mqbc_partitionstatetable.h>
#include <mqbc_storageutil.h>
#include <mqbcfg_messages.h>
#include <mqbi_dispatcher.h>
#include <mqbi_storagemanager.h>
#include <mqbs_datastore.h>
#include <mqbs_filestore.h>
#include <mqbu_storagekey.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>
#include <bmqt_uri.h>

// BDE
#include <ball_log.h>
#include <bdlmt_eventscheduler.h>
#include <bsl_functional.h>
#include <bsl_memory.h>
#include <bsl_new.h>
#include <bsl_ostream.h>
#include <bsl_string.h>
#include <bsl_vector.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bslmt_latch.h>
#include <bslmt_mutex.h>
#include <bsls_atomic.h>
#include <bsls_keyword.h>
#include <bsls_timeinterval.h>
#include <bsls_types.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace mqbi {
class Cluster;
}
namespace mqbnet {
class ClusterNode;
}
namespace mqbs {
class ReplicatedStorage;
}
namespace bdlbb {
class BlobBufferFactory;
}

namespace mqbc {

// FORWARD DECLARATION
class PartitionFSMObserver;
class RecoveryManager;
class StorageManagerIterator;

// ====================
// class StorageManager
// ====================

/// Storage Manager, in charge of all the partitions.
class StorageManager
: public mqbi::StorageManager,
  public PartitionStateTableActions<PartitionFSM::PartitionFSMArgsSp>,
  public PartitionFSMObserver {
  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBC.STORAGEMANAGER");

    // FRIENDS
    friend class StorageManagerIterator;

  private:
    // PRIVATE TYPES
    typedef StorageUtil::FileStores FileStores;

    typedef mqbi::StorageManager_PartitionInfo PartitionInfo;
    typedef bsl::vector<PartitionInfo>         PartitionInfoVec;

    typedef bsl::vector<bsl::shared_ptr<PartitionFSM> > PartitionFSMVec;

    typedef bsl::function<void(int)> RecoveryStatusCb;

    typedef ClusterData::MultiRequestManagerType       MultiRequestManagerType;
    typedef MultiRequestManagerType::RequestContextSp  RequestContextSp;
    typedef MultiRequestManagerType::NodeResponsePairs NodeResponsePairs;
    typedef MultiRequestManagerType::NodeResponsePairsConstIter
        NodeResponsePairsCIter;

    typedef ClusterData::RequestManagerType RequestManagerType;

    typedef bdlmt::EventScheduler::RecurringEventHandle RecurringEventHandle;

    typedef bsl::vector<bdlmt::EventSchedulerEventHandle> EventHandles;

    typedef bsl::vector<PartitionFSMEventData>                 EventData;
    typedef bsl::vector<PartitionFSMEventData>::const_iterator EventDataCIter;

    typedef PartitionFSMEventData::PartitionSeqNumDataRange
        PartitionSeqNumDataRange;

    typedef bslma::ManagedPtr<RecoveryManager> RecoveryManagerMp;

    typedef bsl::vector<mqbnet::ClusterNode*> ClusterNodeVec;
    typedef ClusterNodeVec::const_iterator    ClusterNodeVecCIter;

    typedef mqbs::DataStore::QueueKeyInfoMap QueueKeyInfoMap;

    typedef ClusterState::DomainStatesCIter      DomainStatesCIter;
    typedef ClusterState::UriToQueueInfoMapCIter UriToQueueInfoMapCIter;

    typedef ClusterStateQueueInfo::AppIdInfosCIter AppIdInfosCIter;

    typedef mqbi::StorageManager::StorageSpMapVec StorageSpMapVec;

    typedef bsl::vector<AppKeys>       AppKeysVec;
    typedef AppKeysVec::iterator       AppKeysVecIter;
    typedef AppKeysVec::const_iterator AppKeysVecConstIter;

  public:
    // TYPES
    typedef PartitionFSM::PartitionFSMArgsSp PartitionFSMArgsSp;

    /// Pool of shared pointers to Blobs
    typedef StorageUtil::BlobSpPool BlobSpPool;

    typedef mqbc::StorageUtil::NodeToSeqNumMap NodeToSeqNumMap;
    typedef NodeToSeqNumMap::const_iterator    NodeToSeqNumMapCIter;
    typedef bsl::vector<NodeToSeqNumMap>       NodeToSeqNumMapPartitionVec;

  private:
    // DATA
    bslma::Allocator* d_allocator_p;
    // Allocator to use

    mwcma::CountingAllocatorStore d_allocators;
    // Allocator store to spawn new
    // allocators for sub-components

    bsls::AtomicBool d_isStarted;
    // Whether this StorageMgr has started

    EventHandles d_watchDogEventHandles;
    // List of event handles for the watch
    // dog, indexed by 'partitionId'

    bsls::TimeInterval d_watchDogTimeoutInterval;
    // Timeout interval for the watch dog

    bool d_lowDiskspaceWarning;
    // Flag to denote if a low disk space
    // warning was issued.  This flag is
    // used *only* for logging purposes
    // (see 'storageMonitorCb' impl)

    BlobSpPool* d_blobSpPool_p;
    // SharedObjectPool of blobs to use

    mqbi::DomainFactory* d_domainFactory_p;
    // Domain factory to use

    mqbi::Dispatcher* d_dispatcher_p;
    // Dispatcher to use

    mqbi::Cluster* d_cluster_p;
    // Associated cluster object

    ClusterData* d_clusterData_p;
    // Associated non-persistent cluster
    // data for this node

    const mqbc::ClusterState& d_clusterState;
    // Associated persistent cluster data
    // for this node

    const mqbcfg::ClusterDefinition& d_clusterConfig;
    // Cluster config to use

    FileStores d_fileStores;
    // List of all partitions, indexed by
    // 'partitionId'

    bdlmt::FixedThreadPool d_miscWorkThreadPool;
    // Thread pool used for any standalone
    // work that can be offloaded to
    // non-partition-dispatcher threads.
    // It is used by the partitions owned
    // by this object.

    RecoveryStatusCb d_recoveryStatusCb;

    mutable bslmt::Mutex d_storagesLock;
    // Mutex to protect access to
    // 'd_storages' and its elements.  See
    // comments for 'd_storages' and
    // 'd_appKeysLock' variables as well.

    StorageSpMapVec d_storages;
    // Vector of (CanonicalQueueUri ->
    // ReplicatedStorage) maps.  Vector is
    // indexed by partitionId.  The maps
    // contains *both* in-memory and
    // file-backed storages.  Note that
    // 'd_storagesLock' must be held while
    // accessing this container and any of
    // its elements (URI->Storage maps),
    // because they are accessed from
    // partitions' dispatcher threads, as
    // well as cluster dispatcher thread.

    bslmt::Mutex d_appKeysLock;
    // Mutex to protect access to
    // 'd_appKeysVec' and its elements.
    // Note that when acquiring this lock,
    // *if* 'd_storagesLock' needs to be
    // acquired as well, 'd_storagesLock'
    // must be acquired before
    // 'd_appKeysLock' in order to respect
    // hierarchy of locks and avoid
    // deadlock.  Also note that it is not
    // necessary to acquire one when
    // acquiring the other.

    AppKeysVec d_appKeysVec;
    // Vector of set of AppKeys which are
    // currently active.  Vector is indexed
    // on the partitionId, and the inner
    // set contains a unique list of
    // appKeys of virtual storages of the
    // physical storages assigned to that
    // partition.  Contains appKeys for
    // *both* in-memory as well as
    // file-backed 'physical' storages.
    // Note that the corresponding virtual
    // storages are owned by the physical
    // storage.  Also note that
    // 'd_appKeysLock' must be held while
    // accessing this container and any of
    // its elements, because they are
    // accessed from partitions' dispatcher
    // threads as well as cluster
    // dispatcher threads.

    PartitionInfoVec d_partitionInfoVec;
    // Vector of 'PartitionInfo' indexed by
    // partitionId

    PartitionFSMVec d_partitionFSMVec;
    // Vector of 'PartitionFSM' indexed by
    // partitionId

    PartitionFSMObserver* d_partitionFSMObserver_p;
    // Partition FSM observer provided at
    // ctor

    bsls::AtomicInt d_numPartitionsRecoveredFully;
    // Number of partitions whose recovery
    // has been fully completed.  This
    // variable needs to be atomic because
    // it's touched from the dispatcher
    // threads of all partitions.

    bsl::vector<bsls::Types::Int64> d_recoveryStartTimes;
    // Vector of partition recovery start
    // times indexed by partitionId.

    NodeToSeqNumMapPartitionVec d_nodeToSeqNumMapPartitionVec;
    // Vector of 'NodeToSeqNumMap' indexed
    // by partitionId. Note, that each
    // element of the vector should only be
    // accessed in corresponding thread
    // attached to the partitionId.
    // Currently, false sharing is not much
    // of a performance bottleneck since
    // update to elements of this vector is
    // not a highly frequent operation.

    unsigned int d_seqNumQuorum;
    // Quorum config to use for Sequence
    // numbers being collected by self if
    // primary while getting the latest
    // view of the partitions owned by self

    bsl::vector<unsigned int> d_numReplicaDataResponsesReceivedVec;
    // Vector of number of replica data
    // responses received, indexed by
    // partitionId.

    QueueKeyInfoMap d_queueKeyInfoMap;
    // Mapping from queue key to queue
    // info, populated from cluster state
    // at startup.  This is used to
    // validate against FileStore on-disk
    // content when recovering messages.

    bsls::Types::Uint64 d_minimumRequiredDiskSpace;
    // The bare minimum space required for
    // storage manager to be able to
    // successfully load all partitions.

    RecurringEventHandle d_storageMonitorEventHandle;
    // Handle for recurring events for
    // monitoring storage.

    RecurringEventHandle d_gcMessagesEventHandle;
    // Handle for recurring events for
    // GC'ing expired messages.

    RecoveryManagerMp d_recoveryManager_mp;
    // Recovery manager.

    int d_replicationFactor;
    // Replication factor used to configure
    // FileStores.

  private:
    // NOT IMPLEMENTED
    StorageManager(const StorageManager&);             // = delete;
    StorageManager& operator=(const StorageManager&);  // = delete;

  private:
    // PRIVATE MANIPULATORS

    /// Return the dispatcher of the associated cluster.
    mqbi::Dispatcher* dispatcher();

    /// Callback to start the recovery for the specified `partitionId`.
    ///
    /// THREAD: This method is invoked in the associated Queue dispatcher
    ///         thread for the specified 'partitionId.
    void startRecoveryCb(int partitionId);

    /// Gracefully shut down the partition associated with the specified
    /// `partitionId` and arrive on the specified `latch` when
    /// shut down is complete.
    ///
    /// THREAD: Executed by the dispatcher thread associated with the
    ///         specified `partitionId`.
    void shutdownCb(int partitionId, bslmt::Latch* latch);

    /// Process the watch dog trigger event for the specified `partitionId`,
    /// indicating unhealthiness in the Partition FSM.
    ///
    /// THREAD: Executed by the scheduler thread.
    void onWatchDog(int partitionId);

    /// Process the watch dog trigger event for the specified `partitionId`,
    /// indicating unhealthiness in the Partition FSM.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    void onWatchDogDispatched(int partitionId);

    /// Callback to generate an event for the associated PartitionFSM after
    /// done sending data chunks of the specified `range` related to the
    /// specified `requestId` for the specified `partitionId` to the
    /// specified `destination`, resulting in the specified `status`.
    ///
    /// THREAD: This method is invoked in the associated Queue dispatcher
    ///         thread for the specified 'partitionId.
    void onPartitionDoneSendDataChunksCb(int partitionId,
                                         int requestId,
                                         const PartitionSeqNumDataRange& range,
                                         mqbnet::ClusterNode* destination,
                                         int                  status);

    /// Callback invoked when the recovery for the specified `paritionId` is
    /// complete.
    ///
    /// THREAD: This method is invoked in the associated Queue dispatcher
    ///         thread for the specified 'partitionId.
    void onPartitionRecovery(int partitionId);

    /// Dispatch the event to *QUEUE DISPATCHER* thread associated with
    /// the partitionId as per the specified `eventDataVec` with the
    /// specified `event` using the specified `fs`.
    void dispatchEventToPartition(mqbs::FileStore*          fs,
                                  PartitionFSM::Event::Enum event,
                                  const EventData&          eventDataVec);

    /// Process the PrimaryStateResponse contained in the specified
    /// `context` from the specified `responder`.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    void processPrimaryStateResponseDispatched(
        const RequestManagerType::RequestSp& context,
        mqbnet::ClusterNode*                 responder);

    /// Process the PrimaryStateResponse contained in the specified
    /// `context` from the specified `responder`.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread or scheduler thread.
    void
    processPrimaryStateResponse(const RequestManagerType::RequestSp& context,
                                mqbnet::ClusterNode* responder);

    /// Process the ReplicaStateResponse contained in the specified
    /// `requestContext`.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    void processReplicaStateResponseDispatched(
        const RequestContextSp& requestContext);

    /// Process the ReplicaStateResponse contained in the specified
    /// `requestContext`.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread or scheduler thread.
    void processReplicaStateResponse(const RequestContextSp& requestContext);

    /// Process the ReplicaDataResponse contained in the specified
    /// `requestContext` from the specified `responder`.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread.
    void processReplicaDataResponseDispatched(
        const RequestManagerType::RequestSp& context,
        mqbnet::ClusterNode*                 responder);

    /// Process the ReplicaDataResponse contained in the specified
    /// `requestContext` from the specified `responder`.
    ///
    /// THREAD: This method is invoked in the associated cluster's
    ///         dispatcher thread or scheduler thread.
    void
    processReplicaDataResponse(const RequestManagerType::RequestSp& context,
                               mqbnet::ClusterNode*                 responder);

    /// THREAD: Executed by the dispatcher thread for the specified
    ///         `partitionId`.
    void processShutdownEventDispatched(int partitionId);

    /// Explicitly call `flush` on all FileStores to enforce their GC.
    void forceFlushFileStores();

    //   (virtual: mqbc::PartitionStateTableActions)
    virtual void
    do_startWatchDog(const PartitionFSMArgsSp& args) BSLS_KEYWORD_OVERRIDE;

    virtual void
    do_stopWatchDog(const PartitionFSMArgsSp& args) BSLS_KEYWORD_OVERRIDE;

    virtual void do_populateQueueKeyInfoMap(const PartitionFSMArgsSp& args)
        BSLS_KEYWORD_OVERRIDE;

    virtual void
    do_storeSelfSeq(const PartitionFSMArgsSp& args) BSLS_KEYWORD_OVERRIDE;

    virtual void
    do_storePrimarySeq(const PartitionFSMArgsSp& args) BSLS_KEYWORD_OVERRIDE;

    virtual void
    do_storeReplicaSeq(const PartitionFSMArgsSp& args) BSLS_KEYWORD_OVERRIDE;

    virtual void do_storePartitionInfo(const PartitionFSMArgsSp& args)
        BSLS_KEYWORD_OVERRIDE;

    virtual void do_clearPartitionInfo(const PartitionFSMArgsSp& args)
        BSLS_KEYWORD_OVERRIDE;

    virtual void do_replicaStateRequest(const PartitionFSMArgsSp& args)
        BSLS_KEYWORD_OVERRIDE;

    virtual void do_replicaStateResponse(const PartitionFSMArgsSp& args)
        BSLS_KEYWORD_OVERRIDE;

    virtual void do_failureReplicaStateResponse(const PartitionFSMArgsSp& args)
        BSLS_KEYWORD_OVERRIDE;

    virtual void do_logFailureReplicaStateResponse(
        const PartitionFSMArgsSp& args) BSLS_KEYWORD_OVERRIDE;

    virtual void do_logFailurePrimaryStateResponse(
        const PartitionFSMArgsSp& args) BSLS_KEYWORD_OVERRIDE;

    virtual void do_primaryStateRequest(const PartitionFSMArgsSp& args)
        BSLS_KEYWORD_OVERRIDE;

    virtual void do_primaryStateResponse(const PartitionFSMArgsSp& args)
        BSLS_KEYWORD_OVERRIDE;

    virtual void do_failurePrimaryStateResponse(const PartitionFSMArgsSp& args)
        BSLS_KEYWORD_OVERRIDE;

    virtual void do_replicaDataRequestPush(const PartitionFSMArgsSp& args)
        BSLS_KEYWORD_OVERRIDE;

    virtual void do_replicaDataResponsePush(const PartitionFSMArgsSp& args)
        BSLS_KEYWORD_OVERRIDE;

    virtual void do_replicaDataRequestDrop(const PartitionFSMArgsSp& args)
        BSLS_KEYWORD_OVERRIDE;

    virtual void do_replicaDataRequestPull(const PartitionFSMArgsSp& args)
        BSLS_KEYWORD_OVERRIDE;

    virtual void do_replicaDataResponsePull(const PartitionFSMArgsSp& args)
        BSLS_KEYWORD_OVERRIDE;

    virtual void do_failureReplicaDataResponsePull(
        const PartitionFSMArgsSp& args) BSLS_KEYWORD_OVERRIDE;

    virtual void do_failureReplicaDataResponsePush(
        const PartitionFSMArgsSp& args) BSLS_KEYWORD_OVERRIDE;

    virtual void
    do_bufferLiveData(const PartitionFSMArgsSp& args) BSLS_KEYWORD_OVERRIDE;

    virtual void do_processBufferedLiveData(const PartitionFSMArgsSp& args)
        BSLS_KEYWORD_OVERRIDE;

    virtual void
    do_processLiveData(const PartitionFSMArgsSp& args) BSLS_KEYWORD_OVERRIDE;

    virtual void
    do_processPut(const PartitionFSMArgsSp& args) BSLS_KEYWORD_OVERRIDE;

    virtual void
    do_nackPut(const PartitionFSMArgsSp& args) BSLS_KEYWORD_OVERRIDE;

    virtual void
    do_cleanupSeqnums(const PartitionFSMArgsSp& args) BSLS_KEYWORD_OVERRIDE;

    virtual void do_startSendDataChunks(const PartitionFSMArgsSp& args)
        BSLS_KEYWORD_OVERRIDE;

    virtual void do_setExpectedDataChunkRange(const PartitionFSMArgsSp& args)
        BSLS_KEYWORD_OVERRIDE;

    virtual void do_resetReceiveDataCtx(const PartitionFSMArgsSp& args)
        BSLS_KEYWORD_OVERRIDE;

    virtual void
    do_openStorage(const PartitionFSMArgsSp& args) BSLS_KEYWORD_OVERRIDE;

    virtual void
    do_updateStorage(const PartitionFSMArgsSp& args) BSLS_KEYWORD_OVERRIDE;

    virtual void
    do_removeStorage(const PartitionFSMArgsSp& args) BSLS_KEYWORD_OVERRIDE;

    virtual void do_checkQuorumRplcaDataRspn(const PartitionFSMArgsSp& args)
        BSLS_KEYWORD_OVERRIDE;

    virtual void do_clearRplcaDataRspnCnt(const PartitionFSMArgsSp& args)
        BSLS_KEYWORD_OVERRIDE;

    virtual void
    do_reapplyEvent(const PartitionFSMArgsSp& args) BSLS_KEYWORD_OVERRIDE;

    virtual void
    do_checkQuorumSeq(const PartitionFSMArgsSp& args) BSLS_KEYWORD_OVERRIDE;

    virtual void
    do_findHighestSeq(const PartitionFSMArgsSp& args) BSLS_KEYWORD_OVERRIDE;

    virtual void do_flagFailedReplicaSeq(const PartitionFSMArgsSp& args)
        BSLS_KEYWORD_OVERRIDE;

    virtual void do_reapplyDetectSelfPrimary(const PartitionFSMArgsSp& args)
        BSLS_KEYWORD_OVERRIDE;

    virtual void do_reapplyDetectSelfReplica(const PartitionFSMArgsSp& args)
        BSLS_KEYWORD_OVERRIDE;

  private:
    // PRIVATE ACCESSORS
    bool isLocalCluster() const;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(StorageManager, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a new `StorageManager` using the specified `clusterConfig`
    /// which is associated with the specified `cluster` which uses the
    /// non-persistent data in the specified `clusterData` and the
    /// persistent data in the specified `clusterState`, using the
    /// specified `domainFactory`, `domainFactory`, `dispatcher`,
    /// `watchDogTimeoutDuration`, `recoveryStatusCb` and `allocator`.
    StorageManager(const mqbcfg::ClusterDefinition& clusterConfig,
                   mqbi::Cluster*                   cluster,
                   mqbc::ClusterData*               clusterData,
                   const mqbc::ClusterState&        clusterState,
                   mqbi::DomainFactory*             domainFactory,
                   PartitionFSMObserver*            fsmObserver,
                   mqbi::Dispatcher*                dispatcher,
                   bsls::Types::Int64               watchDogTimeoutDuration,
                   const RecoveryStatusCb&          recoveryStatusCb,
                   bslma::Allocator*                allocator);

    /// Destroy this instance. Behavior is undefined unless this instance is
    /// stopped.
    ~StorageManager() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS
    //   (virtual: mqbc::PartitionFSMObserver)
    void onTransitionToPrimaryHealed(int partitionId,
                                     PartitionStateTableState::Enum oldState)
        BSLS_KEYWORD_OVERRIDE;

    /// Called by PartitionFSM when corresponding state transition from the
    /// specified `oldState` happens for the specified `paritionId`.
    ///
    /// THREAD: Executed by the dispatcher thread for the specified
    ///         `partitionId`.
    void onTransitionToReplicaHealed(
        int                                  partitionId,
        mqbc::PartitionStateTableState::Enum oldState) BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    /// Start this storage manager.  Return 0 on success, or a non-zero rc
    /// otherwise, populating the specified `errorDescription` with a
    /// description of the error.
    ///
    /// THREAD: Executed by the cluster's dispatcher thread.
    virtual int start(bsl::ostream& errorDescription) BSLS_KEYWORD_OVERRIDE;

    /// Stop this storage manager.
    ///
    /// THREAD: Executed by the cluster's dispatcher thread.
    virtual void stop() BSLS_KEYWORD_OVERRIDE;

    /// Register a queue with the specified `uri`, `queueKey` and
    /// `partitionId`, having the spcified `appIdKeyPairs`, and belonging to
    /// the specified `domain`.  Load into the specified `storage` the
    /// associated queue storage created.
    ///
    /// THREAD: Executed by the Client's dispatcher thread.
    virtual void registerQueue(const bmqt::Uri&        uri,
                               const mqbu::StorageKey& queueKey,
                               int                     partitionId,
                               const AppIdKeyPairs&    appIdKeyPairs,
                               mqbi::Domain* domain) BSLS_KEYWORD_OVERRIDE;

    /// Synchronously unregister the queue with the specified `uri` from the
    /// specified `partitionId`.  Behavior is undefined unless this routine
    /// is invoked from the cluster dispatcher thread.
    ///
    /// THREAD: Executed by the Client's dispatcher thread.
    virtual void unregisterQueue(const bmqt::Uri& uri,
                                 int partitionId) BSLS_KEYWORD_OVERRIDE;

    /// Configure the fanout queue having specified `uri` and `queueKey`,
    /// assigned to the specified `partitionId` to have the specified
    /// `addedIdKeyPairs` appId/appKey pairs added and `removedIdKeyPairs`
    /// appId/appKey pairs removed.  Return zero on success, and non-zero
    /// value otherwise.  Behavior is undefined unless this function is
    /// invoked at the primary node.  Behavior is also undefined unless the
    /// queue is configured in fanout mode.
    ///
    /// THREAD: Executed by the Queue's dispatcher thread.
    virtual int
    updateQueue(const bmqt::Uri&        uri,
                const mqbu::StorageKey& queueKey,
                int                     partitionId,
                const AppIdKeyPairs&    addedIdKeyPairs,
                const AppIdKeyPairs& removedIdKeyPairs) BSLS_KEYWORD_OVERRIDE;

    virtual void
    registerQueueReplica(int                     partitionId,
                         const bmqt::Uri&        uri,
                         const mqbu::StorageKey& queueKey,
                         mqbi::Domain*           domain = 0,
                         bool allowDuplicate = false) BSLS_KEYWORD_OVERRIDE;

    virtual void unregisterQueueReplica(int                     partitionId,
                                        const bmqt::Uri&        uri,
                                        const mqbu::StorageKey& queueKey,
                                        const mqbu::StorageKey& appKey)
        BSLS_KEYWORD_OVERRIDE;

    virtual void
    updateQueueReplica(int                     partitionId,
                       const bmqt::Uri&        uri,
                       const mqbu::StorageKey& queueKey,
                       const AppIdKeyPairs&    appIdKeyPairs,
                       mqbi::Domain*           domain = 0,
                       bool allowDuplicate = false) BSLS_KEYWORD_OVERRIDE;

    /// Return a unique appKey for the specified `appId` for a queue
    /// assigned to the specified `partitionId`.  This routine can be
    /// invoked by any thread.
    virtual mqbu::StorageKey
    generateAppKey(const bsl::string& appId,
                   int                partitionId) BSLS_KEYWORD_OVERRIDE;

    /// Set the queue instance associated with the file-backed storage for
    /// the specified `uri` mapped to the specified `partitionId` to the
    /// specified `queue` value.  Note that this method *does* *not*
    /// synchronize on the queue-dispatcher thread.
    virtual void setQueue(mqbi::Queue*     queue,
                          const bmqt::Uri& uri,
                          int              partitionId) BSLS_KEYWORD_OVERRIDE;

    /// Set the queue instance associated with the file-backed storage for
    /// the specified `uri` mapped to the specified `partitionId` to the
    /// specified `queue` value.  Behavior is undefined unless `queue` is
    /// non-null or unless this routine is invoked from the dispatcher
    /// thread associated with the `partitionId`.
    virtual void setQueueRaw(mqbi::Queue*     queue,
                             const bmqt::Uri& uri,
                             int partitionId) BSLS_KEYWORD_OVERRIDE;

    /// Executed in cluster dispatcher thread.  Behavior is undefined unless
    /// the specified `partitionId` is in range and the specified
    /// `primaryNode` is not null.
    virtual void
    setPrimaryForPartition(int                  partitionId,
                           mqbnet::ClusterNode* primaryNode,
                           unsigned int primaryLeaseId) BSLS_KEYWORD_OVERRIDE;

    /// Executed in cluster dispatcher thread.  Behavior is undefined unless
    /// the specified `partitionId` is in range and the specified
    /// `primaryNode` is not null.
    virtual void clearPrimaryForPartition(int                  partitionId,
                                          mqbnet::ClusterNode* primary)
        BSLS_KEYWORD_OVERRIDE;

    /// Apply DETECT_SelfPrimary event to PartitionFSM using the specified
    /// `partitionId`, `primaryNode`, `primaryLeaseId`.
    virtual void
    processPrimaryDetect(int                  partitionId,
                         mqbnet::ClusterNode* primaryNode,
                         unsigned int primaryLeaseId) BSLS_KEYWORD_OVERRIDE;

    /// Apply DETECT_SelfReplica event to StorageFSM using the specified
    /// `partitionId`, `primaryNode` and `primaryLeaseId`.
    virtual void
    processReplicaDetect(int                  partitionId,
                         mqbnet::ClusterNode* primaryNode,
                         unsigned int primaryLeaseId) BSLS_KEYWORD_OVERRIDE;

    /// Process primary state request received from the specified `source`
    /// with the specified `message`.
    virtual void processPrimaryStateRequest(
        const bmqp_ctrlmsg::ControlMessage& message,
        mqbnet::ClusterNode*                source) BSLS_KEYWORD_OVERRIDE;

    /// Process replica state request received from the specified `source`
    /// with the specified `message`.
    virtual void processReplicaStateRequest(
        const bmqp_ctrlmsg::ControlMessage& message,
        mqbnet::ClusterNode*                source) BSLS_KEYWORD_OVERRIDE;

    /// Process replica data request received from the specified `source`
    /// with the specified `message`.
    virtual void processReplicaDataRequestPull(
        const bmqp_ctrlmsg::ControlMessage& message,
        mqbnet::ClusterNode*                source) BSLS_KEYWORD_OVERRIDE;

    /// Process replica data request received from the specified `source`
    /// with the specified `message`.
    virtual void processReplicaDataRequestPush(
        const bmqp_ctrlmsg::ControlMessage& message,
        mqbnet::ClusterNode*                source) BSLS_KEYWORD_OVERRIDE;

    /// Process replica data request received from the specified `source`
    /// with the specified `message`.
    virtual void processReplicaDataRequestDrop(
        const bmqp_ctrlmsg::ControlMessage& message,
        mqbnet::ClusterNode*                source) BSLS_KEYWORD_OVERRIDE;

    virtual int makeStorage(bsl::ostream&                     errorDescription,
                            bslma::ManagedPtr<mqbi::Storage>* out,
                            const bmqt::Uri&                  uri,
                            const mqbu::StorageKey&           queueKey,
                            int                               partitionId,
                            const bsls::Types::Int64          messageTtl,
                            const int maxDeliveryAttempts,
                            const mqbconfm::StorageDefinition& storageDef)
        BSLS_KEYWORD_OVERRIDE;

    /// Executed in cluster dispatcher thread.
    virtual void processStorageEvent(const mqbi::DispatcherStorageEvent& event)
        BSLS_KEYWORD_OVERRIDE;

    /// Executed by any thread.
    virtual void processStorageSyncRequest(
        const bmqp_ctrlmsg::ControlMessage& message,
        mqbnet::ClusterNode*                source) BSLS_KEYWORD_OVERRIDE;

    /// Executed by any thread.
    virtual void processPartitionSyncStateRequest(
        const bmqp_ctrlmsg::ControlMessage& message,
        mqbnet::ClusterNode*                source) BSLS_KEYWORD_OVERRIDE;

    /// Executed by any thread.
    virtual void processPartitionSyncDataRequest(
        const bmqp_ctrlmsg::ControlMessage& message,
        mqbnet::ClusterNode*                source) BSLS_KEYWORD_OVERRIDE;

    /// Executed by any thread.
    virtual void processPartitionSyncDataRequestStatus(
        const bmqp_ctrlmsg::ControlMessage& message,
        mqbnet::ClusterNode*                source) BSLS_KEYWORD_OVERRIDE;

    /// Executed in cluster dispatcher thread.
    virtual void processRecoveryEvent(
        const mqbi::DispatcherRecoveryEvent& event) BSLS_KEYWORD_OVERRIDE;

    /// Executed in cluster dispatcher thread.
    virtual void processReceiptEvent(const mqbi::DispatcherReceiptEvent& event)
        BSLS_KEYWORD_OVERRIDE;

    /// Executed by any thread.
    virtual void processPrimaryStatusAdvisory(
        const bmqp_ctrlmsg::PrimaryStatusAdvisory& advisory,
        mqbnet::ClusterNode* source) BSLS_KEYWORD_OVERRIDE;

    /// Executed by any thread.
    virtual void processReplicaStatusAdvisory(
        int                             partitionId,
        mqbnet::ClusterNode*            source,
        bmqp_ctrlmsg::NodeStatus::Value status) BSLS_KEYWORD_OVERRIDE;

    /// Executed by any thread.
    virtual void processShutdownEvent() BSLS_KEYWORD_OVERRIDE;

    /// Invoke the specified `functor` with each queue associated to the
    /// partition identified by the specified `partitionId` if that
    /// partition has been successfully opened.  The behavior is undefined
    /// unless invoked from the queue thread corresponding to `partitionId`.
    virtual void
    applyForEachQueue(int                 partitionId,
                      const QueueFunctor& functor) const BSLS_KEYWORD_OVERRIDE;

    /// Process the specified `command`, and load the result to the
    /// specified `result`.  Return 0 if the command was successfully
    /// processed, or a non-zero value otherwise.  This function can be
    /// invoked from any thread, and will block until the potentially
    /// asynchronous operation is complete.
    virtual int processCommand(mqbcmd::StorageResult*        result,
                               const mqbcmd::StorageCommand& command)
        BSLS_KEYWORD_OVERRIDE;

    /// GC the queues from unrecognized domains, if any.
    virtual void gcUnrecognizedDomainQueues() BSLS_KEYWORD_OVERRIDE;

    /// Return partition corresponding to the specified `partitionId`.  The
    /// behavior is undefined if `partitionId` does not represent a valid
    /// partition id. Note, this modifiable reference to partition is only
    /// meant to be used for unit testing purposes.
    mqbs::FileStore& fileStore(int partitionId);

    // ACCESSORS

    /// Return the processor handle in charge of the specified
    /// `partitionId`.  The behavior is undefined if `partitionId` does not
    /// represent a valid partition id.
    virtual mqbi::Dispatcher::ProcessorHandle
    processorForPartition(int partitionId) const BSLS_KEYWORD_OVERRIDE;

    /// Return true if the queue having specified `uri` and assigned to the
    /// specified `partitionId` has no messages, false in any other case.
    /// Behavior is undefined unless this routine is invoked from cluster
    /// dispatcher thread.
    virtual bool isStorageEmpty(const bmqt::Uri& uri,
                                int partitionId) const BSLS_KEYWORD_OVERRIDE;

    /// Return the blob buffer factory to use.
    virtual bdlbb::BlobBufferFactory*
    blobBufferFactory() const BSLS_KEYWORD_OVERRIDE;

    /// Return partition corresponding to the specified `partitionId`.  The
    /// behavior is undefined if `partitionId` does not represent a valid
    /// partition id.
    virtual const mqbs::FileStore&
    fileStore(int partitionId) const BSLS_KEYWORD_OVERRIDE;

    /// Return a StorageManagerIterator for the specified `partitionId`.
    virtual bslma::ManagedPtr<mqbi::StorageManagerIterator>
    getIterator(int partitionId) const BSLS_KEYWORD_OVERRIDE;

    /// Return the health state of the specified `partitionId`.
    PartitionFSM::State::Enum partitionHealthState(int partitionId) const;

    /// Return the mapping from nodes in the cluster to their sequence
    /// numbers for the specified `partitionId`.
    const NodeToSeqNumMap& nodeToSeqNumMap(int partitionId) const;
};

// ============================
// class StorageManagerIterator
// ============================

/// Provide thread safe iteration through all the storages of a partition in
/// the storage manager.  The order of the iteration is implementation
/// defined.  An iterator is *valid* if it is associated with a storage in
/// the manager, otherwise it is *invalid*.  Thread-safe iteration is
/// provided by locking the manager during the iterator's construction and
/// unlocking it at the iterator's destruction.  This guarantees that during
/// the life time of an iterator, the manager can't be modified.
class StorageManagerIterator : public mqbi::StorageManagerIterator {
  private:
    // PRIVATE TYPES
    typedef StorageManager::StorageSpMap          StorageSpMap;
    typedef StorageManager::StorageSpMapConstIter StorageMapConstIter;

  private:
    // DATA
    const StorageManager* d_manager_p;

    const StorageSpMap* d_map_p;

    StorageMapConstIter d_iterator;

  private:
    // NOT IMPLEMENTED
    StorageManagerIterator(const StorageManagerIterator&) BSLS_KEYWORD_DELETED;
    StorageManagerIterator&
    operator=(const StorageManagerIterator&) BSLS_KEYWORD_DELETED;

  public:
    // CREATORS

    /// Create an iterator for the specified `partitionId` in the specified
    /// storage `manager` and associated it with the first storage of the
    /// `partitionId`.  If the `manager` is empty then the iterator is
    /// initialized to be invalid.  The `manager` is locked for the duration
    /// of iterator's life time.  The behavior is undefined unless
    /// `partitionId` is valid and `manager` is not null.
    StorageManagerIterator(int partitionId, const StorageManager* manager);

    /// Destroy this iterator and unlock the storage manager associated with
    /// it.
    ~StorageManagerIterator() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    /// Advance this iterator to refer to the next storage of the associated
    /// partition; if there is no next storage in the associated partition,
    /// then this iterator becomes *invalid*.  The behavior is undefined
    /// unless this iterator is valid.  Note that the order of the iteration
    /// is not specified.
    void operator++() BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS

    /// Return non-zero if the iterator is *valid*, and 0 otherwise.
    operator const void*() const BSLS_KEYWORD_OVERRIDE;

    /// Return a reference offering non-modifiable access to the queue URI
    /// being pointed by this iterator.  The behavior is undefined unless
    /// the iterator is *valid*.
    const bmqt::Uri& uri() const BSLS_KEYWORD_OVERRIDE;

    /// Return a reference offering non-modifiable access to the storage
    /// being pointed by this iterator.  The behavior is undefined unless
    /// the iterator is *valid*. Note that since iterator is not a first
    /// class object, its okay to pass a raw pointer.
    const mqbs::ReplicatedStorage* storage() const BSLS_KEYWORD_OVERRIDE;
};

// ============================================================================
//                            INLINE DEFINITIONS
// ============================================================================

// ----------------------------
// class StorageManagerIterator
// ----------------------------

// CREATORS
inline StorageManagerIterator::StorageManagerIterator(
    int                   partitionId,
    const StorageManager* manager)
: d_manager_p(manager)
, d_map_p(0)
, d_iterator()
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_manager_p);
    BSLS_ASSERT_SAFE(0 <= partitionId);

    d_map_p = &(d_manager_p->d_storages[partitionId]);
    BSLS_ASSERT_SAFE(d_map_p);

    d_manager_p->d_storagesLock.lock();  // LOCK
    d_iterator = d_map_p->begin();
}

// MANIPULATORS
inline void StorageManagerIterator::operator++()
{
    ++d_iterator;
}

// ACCESSORS
inline StorageManagerIterator::operator const void*() const
{
    return (d_iterator == d_map_p->end())
               ? 0
               : const_cast<StorageManagerIterator*>(this);
}

inline const bmqt::Uri& StorageManagerIterator::uri() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(*this);
    return d_iterator->first;
}

inline const mqbs::ReplicatedStorage* StorageManagerIterator::storage() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(*this);
    return (d_iterator->second).get();
}

// --------------------
// class StorageManager
// --------------------

// PRIVATE MANIPULATORS
inline mqbi::Dispatcher* StorageManager::dispatcher()
{
    return d_clusterData_p->dispatcherClientData().dispatcher();
}

// PRIVATE ACCESSORS
inline bool StorageManager::isLocalCluster() const
{
    return d_clusterData_p->cluster()->isLocal();
}

// ACCESSORS
inline mqbi::Dispatcher::ProcessorHandle
StorageManager::processorForPartition(int partitionId) const
{
    // executed by *ANY* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_fileStores.size()));

    return d_fileStores[partitionId]->processorId();
}

inline bslma::ManagedPtr<mqbi::StorageManagerIterator>
StorageManager::getIterator(int partitionId) const
{
    bslma::ManagedPtr<mqbi::StorageManagerIterator> mp(
        new (*d_allocator_p) StorageManagerIterator(partitionId, this),
        d_allocator_p);

    return mp;
}

inline PartitionFSM::State::Enum
StorageManager::partitionHealthState(int partitionId) const
{
    return d_partitionFSMVec[partitionId]->state();
}

inline const StorageManager::NodeToSeqNumMap&
StorageManager::nodeToSeqNumMap(int partitionId) const
{
    return d_nodeToSeqNumMapPartitionVec[partitionId];
}

}  // close package namespace
}  // close enterprise namespace

#endif
