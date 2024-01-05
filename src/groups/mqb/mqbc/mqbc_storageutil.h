// Copyright 2021-2023 Bloomberg Finance L.P.
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

// mqbc_storageutil.h                                                 -*-C++-*-
#ifndef INCLUDED_MQBC_STORAGEUTIL
#define INCLUDED_MQBC_STORAGEUTIL

//@PURPOSE: Provide generic utilities used for storage operations.
//
//@CLASSES:
//  mqbc::StorageUtil: Generic utilities for storage related operations.
//
//@DESCRIPTION: 'mqbc::StorageUtil' provides generic utilities.

// MQB

#include <mqbc_clusterdata.h>
#include <mqbc_clusterstate.h>
#include <mqbcfg_messages.h>
#include <mqbcmd_messages.h>
#include <mqbconfm_messages.h>
#include <mqbi_cluster.h>
#include <mqbi_dispatcher.h>
#include <mqbi_domain.h>
#include <mqbi_queue.h>
#include <mqbi_storage.h>
#include <mqbi_storagemanager.h>
#include <mqbs_datastore.h>
#include <mqbs_filestore.h>
#include <mqbs_replicatedstorage.h>
#include <mqbu_storagekey.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_event.h>
#include <bmqp_storagemessageiterator.h>
#include <bmqt_uri.h>

// MWC
#include <mwcma_countingallocatorstore.h>

// BDE
#include <ball_log.h>
#include <bdlb_nullablevalue.h>
#include <bdlmt_eventscheduler.h>
#include <bdlmt_fixedthreadpool.h>
#include <bsl_algorithm.h>
#include <bsl_functional.h>
#include <bsl_memory.h>
#include <bsl_numeric.h>
#include <bsl_string.h>
#include <bsl_unordered_map.h>
#include <bsl_vector.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bslmt_latch.h>
#include <bsls_assert.h>
#include <bsls_atomic.h>
#include <bsls_types.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace bslmt {
class Mutex;
}
namespace mqbnet {
class ClusterNode;
}

namespace mqbc {

namespace {
/// Constant used to represent an invalid partition id.
const unsigned int k_INVALID_PARTITION_ID =
    bsl::numeric_limits<unsigned int>::max();
}  // close unnamed namespace

// ==================
// struct StorageUtil
// ==================

/// Generic utilities for storage related operations.
struct StorageUtil {
  private:
    // CLASS SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBC.STORAGEUTIL");

  private:
    // TYPES
    typedef mqbi::StorageManager::AppIdKeyPair  AppIdKeyPair;
    typedef mqbi::StorageManager::AppIdKeyPairs AppIdKeyPairs;
    typedef AppIdKeyPairs::const_iterator       AppIdKeyPairsCIter;

    typedef mqbi::StorageManager::AppKeys         AppKeys;
    typedef mqbi::StorageManager::AppKeysInsertRc AppKeysInsertRc;

    typedef mqbi::StorageManager::StorageSp             StorageSp;
    typedef mqbi::StorageManager::StorageSpMap          StorageSpMap;
    typedef mqbi::StorageManager::StorageSpMapVec       StorageSpMapVec;
    typedef mqbi::StorageManager::StorageSpMapIter      StorageSpMapIter;
    typedef mqbi::StorageManager::StorageSpMapConstIter StorageSpMapConstIter;

    typedef mqbs::DataStoreConfig::QueueKeyInfoMap QueueKeyInfoMap;

    typedef mqbs::DataStoreConfig::QueueCreationCb   QueueCreationCb;
    typedef mqbs::DataStoreConfig::QueueDeletionCb   QueueDeletionCb;
    typedef mqbs::DataStoreConfig::RecoveredQueuesCb RecoveredQueuesCb;

    typedef mqbi::StorageManager_PartitionInfo PartitionInfo;

    typedef bsl::function<void(int, bslmt::Latch*)> ShutdownCb;

    typedef mqbs::FileStore::StorageList StorageList;
    typedef bsl::vector<StorageList>     StorageLists;
    typedef StorageLists::const_iterator StorageListsConstIter;

    typedef mqbs::FileStore::StorageFilters StorageFilters;

  public:
    // TYPES
    typedef bsl::shared_ptr<mqbs::FileStore> FileStoreSp;
    typedef bsl::vector<FileStoreSp>         FileStores;

    typedef bsl::unordered_map<mqbnet::ClusterNode*,
                               bmqp_ctrlmsg::PartitionSequenceNumber>
        NodeToSeqNumMap;

    typedef bdlcc::SharedObjectPool<
        bdlbb::Blob,
        bdlcc::ObjectPoolFunctors::DefaultCreator,
        bdlcc::ObjectPoolFunctors::RemoveAll<bdlbb::Blob> >
        BlobSpPool;

    typedef bdlmt::EventScheduler::RecurringEventHandle RecurringEventHandle;

    /// Type of the functor required by `executeForEachPartitions`.  It
    /// represents a function to be executed for each partition in the
    /// storage manager.  Each partition will receive its partitionId and a
    /// latch along with the function.  Each partition should call
    /// `latch->arrive()` after it has finished executing the function.
    typedef bsl::function<void(const int, bslmt::Latch*)> PerPartitionFunctor;

  private:
    // PRIVATE FUNCTIONS

    /// Load into the specified `result` the list of elements present in
    /// `baseSet` which are not present in `subtractionSet`.
    template <typename T>
    static void loadDifference(bsl::vector<T>*       result,
                               const bsl::vector<T>& baseSet,
                               const bsl::vector<T>& subtractionSet);

    /// Load into the specified `addedAppIdKeyPairs` and
    /// `removedAppIdKeyPairs` the appId/key pairs which have been added and
    /// removed respectively for the specified `storage` based on the
    /// specified `newAppIdKeyPairs` or `cfgAppIds`, as well as the
    /// specified `isCSLMode` mode.  If new app keys are generated, load
    /// them into the specified `appKeys`.  If the optionally specified
    /// `appKeysLock` is provided, lock it.  Return true if there are any
    /// added or removed appId/key pairs, false otherwise.
    ///
    /// THREAD: Executed by the cluster-dispatcher thread.
    static bool
    loadUpdatedAppIdKeyPairs(AppIdKeyPairs* addedAppIdKeyPairs,
                             AppIdKeyPairs* removedAppIdKeyPairs,
                             AppKeys*       appKeys,
                             bslmt::Mutex*  appKeysLock,
                             const mqbs::ReplicatedStorage&  storage,
                             const AppIdKeyPairs&            newAppIdKeyPairs,
                             const bsl::vector<bsl::string>& cfgAppIds,
                             bool                            isCSLMode);

    /// THREAD: Executed by the Queue's dispatcher thread.
    static void
    registerQueueDispatched(const mqbi::Dispatcher::ProcessorHandle& processor,
                            mqbs::FileStore*                         fs,
                            mqbs::ReplicatedStorage*                 storage,
                            const bsl::string&   clusterDescription,
                            int                  partitionId,
                            const AppIdKeyPairs& appIdKeyPairs);

    /// THREAD: This method is called from the Queue's dispatcher thread.
    static void
    updateQueueDispatched(const mqbi::Dispatcher::ProcessorHandle& processor,
                          mqbs::ReplicatedStorage*                 storage,
                          bslmt::Mutex*        storagesLock,
                          mqbs::FileStore*     fs,
                          AppKeys*             appKeys,
                          bslmt::Mutex*        appKeysLock,
                          const bsl::string&   clusterDescription,
                          int                  partitionId,
                          const AppIdKeyPairs& addedIdKeyPairs,
                          const AppIdKeyPairs& removedIdKeyPairs,
                          bool                 isFanout,
                          bool                 isCSLMode);

    /// StorageManager's storages lock must be locked before calling this
    /// method.
    ///
    /// THREAD: Executed by the Queue's dispatcher thread.
    static int updateQueueRaw(mqbs::ReplicatedStorage* storage,
                              mqbs::FileStore*         fs,
                              AppKeys*                 appKeys,
                              bslmt::Mutex*            appKeysLock,
                              const bsl::string&       clusterDescription,
                              int                      partitionId,
                              const AppIdKeyPairs&     addedIdKeyPairs,
                              const AppIdKeyPairs&     removedIdKeyPairs,
                              bool                     isFanout,
                              bool                     isCSLMode);

    static int
    addVirtualStoragesInternal(mqbs::ReplicatedStorage* storage,
                               AppKeys*                 appKeys,
                               bslmt::Mutex*            appKeysLock,
                               const AppIdKeyPairs&     appIdKeyPairs,
                               const bsl::string&       clusterDescription,
                               int                      partitionId,
                               bool                     isFanout,
                               bool                     isCSLMode);

    static int removeVirtualStorageInternal(mqbs::ReplicatedStorage* storage,
                                            AppKeys*                 appKeys,
                                            bslmt::Mutex* appKeysLock,
                                            const mqbu::StorageKey& appKey,
                                            int partitionId);

    /// Load the list of queue storages on the partition from the specified
    /// `fileStores` having the specified `partitionId` into the
    /// corresponding element of the specified `storageLists`, and arrive on
    /// the specified `latch` upon completion.  All queues for which any
    /// filter in the specified `filters` returns false will be filtered out
    /// from the list.  The size of `storageLists` must be equal to the
    /// number of partitions in the storage manager before calling this
    /// function.
    ///
    /// THREAD: Executed by the Queue's dispatcher thread for the specified
    ///         `partitionId`.
    static void getStoragesDispatched(StorageLists*         storageLists,
                                      bslmt::Latch*         latch,
                                      const FileStores&     fileStores,
                                      int                   partitionId,
                                      const StorageFilters& filters);

    /// Load the status of the queue storages from the specified
    /// `fileStores` belonging to the specified `domainName` into the
    /// specified `storages`.
    ///
    /// THREAD: Executed by the cluster-dispatcher thread.
    static void loadStorages(bsl::vector<mqbcmd::StorageQueueInfo>* storages,
                             const bsl::string&                     domainName,
                             const FileStores& fileStores);

    /// Load the summary of the partition out of the specified `fileStores`
    /// having the specified `partitionId` and `partitionLocation` into the
    /// specified `result` object.
    ///
    /// THREAD: Executed by the cluster-dispatcher thread.
    static void
    loadPartitionStorageSummary(mqbcmd::StorageResult*   result,
                                FileStores*              fileStores,
                                int                      partitionId,
                                const bslstl::StringRef& partitionLocation);

    /// Load the summary of the partitions of the spcified `fileStores` at
    /// the specified `location` to the specified `result` object.
    ///
    /// THREAD: Executed by the cluster-dispatcher thread.
    static void loadStorageSummary(mqbcmd::StorageResult*  result,
                                   const FileStores&       fileStores,
                                   const bslstl::StringRef location);

    /// Load the summary of the partition out of the specified `fileStores`
    /// having the specified `partitionId` into an element of the specified
    /// `summary` object, and arrive on the specified `latch` upon
    /// completion.
    ///
    /// THREAD: Executed by the Queue's dispatcher thread for the specified
    ///         `partitionId`.
    static void
    loadStorageSummaryDispatched(mqbcmd::ClusterStorageSummary* summary,
                                 bslmt::Latch*                  latch,
                                 int                            partitionId,
                                 const FileStores&              fileStores);

    /// Execute the domain purge command for the specified `partitionId`,
    /// while selecting the appropriate FileStore from the specified
    /// `fileStores`.  The specified `domainName` is the name of a domain
    /// to purge.  Purge results are stored in the specified `purgedQueuesVec`.
    /// Use the specified `latch` upon completion to synchronize domain purge
    /// across all partitions ids.
    ///
    /// THREAD: Executed by the Queue's dispatcher thread for the specified
    ///         `partitionId`.
    static void purgeDomainDispatched(
        bsl::vector<bsl::vector<mqbcmd::PurgeQueueResult> >* purgedQueuesVec,
        bslmt::Latch*                                        latch,
        int                                                  partitionId,
        const FileStores&                                    fileStores,
        const bsl::string&                                   domainName);

    /// Execute the queue purge command for the specified `storage` and the
    /// specified `appId`, associated with the specified `fileStore`.  The
    /// empty `appId` means purge of all app ids for this queue.  Purge results
    /// are stored in the specified `purgedQueue`.   Use the specified
    /// `purgeFinishedSemaphore` upon completion to synchronize queue purge
    /// with the caller.
    ///
    /// THREAD: Executed by the Queue's dispatcher thread for the specified
    ///         `fileStore`.
    static void purgeQueueDispatched(mqbcmd::PurgeQueueResult* purgedQueue,
                                     bslmt::Semaphore*  purgeFinishedSemaphore,
                                     mqbs::FileStore*   fileStore,
                                     mqbi::Storage*     storage,
                                     const bsl::string* appId);

    /// Execute the specified `job` for each partition in the specified
    /// `fileStores`.  Each partition will receive its partitionId and a
    /// latch along with the `job`.  Each partition *must* call
    /// `latch->arrive()` after it has finished executing the `job`.
    ///
    /// THREAD: Executed by the cluster-dispatcher thread.
    static void executeForEachPartitions(const PerPartitionFunctor& job,
                                         const FileStores& fileStores);

    /// Process the specified `command`, and load the result to the
    /// specified `replicationResult`.  The command might modify the
    /// specified `replicationFactor` and the corresponding value in each
    /// partition of the specified `fileStores`.  Return 0 if the command
    /// was successfully processed, or a non-zero value otherwise.
    ///
    /// THREAD: Executed by the cluster-dispatcher thread.
    static int
    processReplicationCommand(mqbcmd::ReplicationResult* replicationResult,
                              int*                       replicationFactor,
                              FileStores*                fileStores,
                              const mqbcmd::ReplicationCommand& command);

  public:
    // FUNCTIONS

    /// Load into the specified `addedEntries` the list of entries which are
    /// present in `newEntries` but not in `existingEntries`.  Similary, load
    /// into the specified `removedEntries` the list of entries which are
    /// present in `existingEntries` but not in `newEntries`.
    template <typename T>
    static void
    loadAddedAndRemovedEntries(bsl::vector<T>*       addedEntries,
                               bsl::vector<T>*       removedEntries,
                               const bsl::vector<T>& existingEntries,
                               const bsl::vector<T>& newEntries);

    /// Return true if the queue having specified `uri` and assigned to the
    /// specified `partitionId` has no messages in the specified
    /// `storageMap`, false in any other case.  If the optionally specified
    /// `storagesLock` is specified, lock it.  Behavior is undefined unless
    /// this routine is invoked from cluster dispatcher thread.
    static bool isStorageEmpty(bslmt::Mutex*       storagesLock,
                               const StorageSpMap& storageMap,
                               const bmqt::Uri&    uri,
                               int                 partitionId);

    /// Callback scheduled by the Storage Manager to run every minute which
    /// monitors storage (disk space, archive clean up, etc), using the
    /// specified `lowDiskspaceWarning`, `isManagerStarted`,
    /// `minimumRequiredDiskSpace`, `clusterDescription`, and
    /// `partitionConfig`.
    ///
    /// THREAD: Executed by the scheduler's dispatcher thread.
    static void
    storageMonitorCb(bool*                          lowDiskspaceWarning,
                     const bsls::AtomicBool*        isManagerStarted,
                     bsls::Types::Uint64            minimumRequiredDiskSpace,
                     const bslstl::StringRef&       clusterDescription,
                     const mqbcfg::PartitionConfig& partitionConfig);

    /// Print Log Banner indicating Recovery Phase One for the specified
    /// `clusterDescription` and the specified `partitionId`. Use the
    /// specified `out` to stream the banner.
    static bsl::ostream&
    printRecoveryPhaseOneBanner(bsl::ostream&      out,
                                const bsl::string& clusterDescription,
                                int                partitionId);

    /// Validate the partition directory for the specified `config` and use
    /// the specified `errorDescription` for emitting errors.
    static int
    validatePartitionDirectory(const mqbcfg::PartitionConfig& config,
                               bsl::ostream& errorDescription);

    /// Validate the disk space required for storing partitions as per the
    /// specified `config` for the specified `clusterData` by comparing it
    /// with the specified `minDiskSpace`.
    static int validateDiskSpace(const mqbcfg::PartitionConfig& config,
                                 const mqbc::ClusterData&       clusterData,
                                 const bsls::Types::Uint64&     minDiskSpace);

    /// Extract and return the partition id from the specified `event` or
    /// `k_INVALID_PARTITION_ID` if the event is empty or non-iterable.  The
    /// behavior is undefined unless the event is of type `e_STORAGE`,
    /// `e_PARTITION_SYNC` or `e_RECOVERY` because *all* messages in those
    /// events belong to the *same* partition.
    template <bool IS_RECOVERY>
    static unsigned int extractPartitionId(const bmqp::Event& event);

    /// Validate that every storage message in the specified `event` have
    /// the same specified `partitionId`, and that the specified event
    /// `source` is the active primary node in the specified `clusterState`.
    /// Use the specified `clusterData` to help with validation.  The
    /// specified `skipAlarm` flag determines whether to skip alarming if
    /// `source` is not active primary.  Return true if valid, false
    /// otherwise.
    static bool validateStorageEvent(const bmqp::Event&         event,
                                     int                        partitionId,
                                     const mqbnet::ClusterNode* source,
                                     const mqbc::ClusterState&  clusterState,
                                     const mqbc::ClusterData&   clusterData,
                                     bool                       skipAlarm);

    /// Validate that every partition sync message in the specified `event`
    /// have the same specified `partitionId`, and that ether self or the
    /// specified event `source` is the primary node in the specified
    /// `clusterState`.  Use the specified `clusterData` and `isFSMWorkflow`
    /// flag to help with validation.  Return true if valid, false
    /// otherwise.
    static bool
    validatePartitionSyncEvent(const bmqp::Event&         event,
                               int                        partitionId,
                               const mqbnet::ClusterNode* source,
                               const mqbc::ClusterState&  clusterState,
                               const mqbc::ClusterData&   clusterData,
                               bool                       isFSMWorkflow);

    /// Assign Queue dispatcher threads from the specified `threadPool` of
    /// the specified `clusterData`, specified `cluster` using the specified
    /// `dispatcher` where the configuration is used from the specified
    /// `config` to the specified `fileStores` using the specified
    /// `blobSpPool` where memory is allocated using the specified
    /// `allocators`, any errors are captured using the specified
    /// `errorDescription` and there are optionally specified
    /// `queueCreationCb`, `queueDeletionCb` and `recoveredQueuesCb`.
    static int assignPartitionDispatcherThreads(
        bdlmt::FixedThreadPool*                     threadPool,
        mqbc::ClusterData*                          clusterData,
        const mqbi::Cluster&                        cluster,
        mqbi::Dispatcher*                           dispatcher,
        const mqbcfg::PartitionConfig&              config,
        FileStores*                                 fileStores,
        BlobSpPool*                                 blobSpPool,
        mwcma::CountingAllocatorStore*              allocators,
        bsl::ostream&                               errorDescription,
        int                                         replicationFactor,
        const bdlb::NullableValue<QueueCreationCb>& queueCreationCb =
            bdlb::NullableValue<QueueCreationCb>(),
        const bdlb::NullableValue<QueueDeletionCb>& queueDeletionCb =
            bdlb::NullableValue<QueueDeletionCb>(),
        const bdlb::NullableValue<RecoveredQueuesCb>& recoveredQueuesCb =
            bdlb::NullableValue<RecoveredQueuesCb>());

    /// Clear the specified `primary` of the specified `partitionId` from
    /// the specified `fs` and `partitionInfo`, using the specified
    /// `clusterData`.  Behavior is undefined unless the specified
    /// `partitionId` is in range and the specified `primary` is not null.
    ///
    /// THREAD: Executed by the dispatcher thread for the specified
    ///         `partitionId`.
    static void clearPrimaryForPartition(mqbs::FileStore*     fs,
                                         PartitionInfo*       partitionInfo,
                                         const ClusterData&   clusterData,
                                         int                  partitionId,
                                         mqbnet::ClusterNode* primary);

    /// Find the minimum required disk space using the specified `config`.
    static bsls::Types::Uint64
    findMinReqDiskSpace(const mqbcfg::PartitionConfig& config);

    /// Clean sequence numbers from the specified `partitionInfo` and
    /// specified `nodeToSeqNumMap`. Reset the primary node in the specified
    /// `partitionInfo` to null.
    static void cleanSeqNums(PartitionInfo&   partitionInfo,
                             NodeToSeqNumMap& nodeToSeqNumMap);

    /// Transition self to active primary of the specified `partitionId` and
    /// load this info into the specified `partitionInfo`.  Then, broadcast
    /// a primary status advisory to peers using the specified
    /// `clusterData`.
    static void transitionToActivePrimary(PartitionInfo*     partitionInfo,
                                          mqbc::ClusterData* clusterData,
                                          int                partitionId);

    /// Stop all the underlying partitions using the specified `clusterData`
    /// by scheduling execution of the specified `shutdownCb` for each
    /// FileStore in the specified `fileStores`
    static void stop(ClusterData*      clusterData,
                     FileStores*       fileStores,
                     const ShutdownCb& shutdownCb);

    /// Shutdown the underlying partition associated with the specified
    /// `partitionId` from the specified `fileStores` by using the specified
    /// `latch`, and the specified `clusterConfig`. The cluster information
    /// is printed using the specified `clusterData`.
    static void shutdown(int                              partitionId,
                         bslmt::Latch*                    latch,
                         FileStores*                      fileStores,
                         ClusterData*                     clusterData,
                         const mqbcfg::ClusterDefinition& clusterConfig);

    /// Return a unique appKey for the specified `appId` for a queue, and
    /// load the appKey into the specified `appKeys` while locking the
    /// optionally specified `appKeysLock`.  This routine can be invoked by
    /// any thread.
    static mqbu::StorageKey generateAppKey(AppKeys*           appKeys,
                                           bslmt::Mutex*      appKeysLock,
                                           const bsl::string& appId);

    /// Register a queue with the specified `uri`, `queueKey` and
    /// `partitionId`, having the specified `appIdKeyPairs`, and belonging to
    /// the specified `domain`.  Load into the specified `storage` the
    /// associated queue storage created.
    ///
    /// THREAD: Executed by the Client's dispatcher thread.
    static void
    registerQueue(const mqbi::Cluster*                     cluster,
                  mqbi::Dispatcher*                        dispatcher,
                  StorageSpMap*                            storageMap,
                  bslmt::Mutex*                            storagesLock,
                  mqbs::FileStore*                         fs,
                  AppKeys*                                 appKeys,
                  bslmt::Mutex*                            appKeysLock,
                  mwcma::CountingAllocatorStore*           allocators,
                  const mqbi::Dispatcher::ProcessorHandle& processor,
                  const bmqt::Uri&                         uri,
                  const mqbu::StorageKey&                  queueKey,
                  const bsl::string&                       clusterDescription,
                  int                                      partitionId,
                  const AppIdKeyPairs&                     appIdKeyPairs,
                  mqbi::Domain*                            domain);

    /// THREAD: Executed by the Queue's dispatcher thread.
    static void unregisterQueueDispatched(
        const mqbi::Dispatcher::ProcessorHandle& processor,
        mqbs::FileStore*                         fs,
        StorageSpMap*                            storageMap,
        bslmt::Mutex*                            storagesLock,
        const ClusterData*                       clusterData,
        int                                      partitionId,
        const PartitionInfo&                     pinfo,
        const bmqt::Uri&                         uri);

    /// Configure the fanout queue having specified `uri` and `queueKey`,
    /// assigned to the specified `partitionId` to have the specified
    /// `addedIdKeyPairs` appId/appKey pairs added and `removedIdKeyPairs`
    /// appId/appKey pairs removed.  Return zero on success, and non-zero
    /// value otherwise.  Behavior is undefined unless this function is
    /// invoked at the primary node.  Behavior is also undefined unless the
    /// queue is configured in fanout mode.
    ///
    /// THREAD: Executed by the Queue's dispatcher thread.
    static int updateQueue(StorageSpMap*           storageMap,
                           bslmt::Mutex*           storagesLock,
                           mqbs::FileStore*        fs,
                           AppKeys*                appKeys,
                           bslmt::Mutex*           appKeysLock,
                           const bsl::string&      clusterDescription,
                           const bmqt::Uri&        uri,
                           const mqbu::StorageKey& queueKey,
                           int                     partitionId,
                           const AppIdKeyPairs&    addedIdKeyPairs,
                           const AppIdKeyPairs&    removedIdKeyPairs,
                           bool                    isCSLMode);

    static void
    registerQueueReplicaDispatched(int*                 status,
                                   StorageSpMap*        storageMap,
                                   bslmt::Mutex*        storagesLock,
                                   mqbs::FileStore*     fs,
                                   mqbi::DomainFactory* domainFactory,
                                   mwcma::CountingAllocatorStore* allocators,
                                   const bsl::string&      clusterDescription,
                                   int                     partitionId,
                                   const bmqt::Uri&        uri,
                                   const mqbu::StorageKey& queueKey,
                                   mqbi::Domain*           domain = 0,
                                   bool allowDuplicate            = false);

    static void
    unregisterQueueReplicaDispatched(int*               status,
                                     StorageSpMap*      storageMap,
                                     bslmt::Mutex*      storagesLock,
                                     mqbs::FileStore*   fs,
                                     AppKeys*           appKeys,
                                     bslmt::Mutex*      appKeysLock,
                                     const bsl::string& clusterDescription,
                                     int                partitionId,
                                     const bmqt::Uri&   uri,
                                     const mqbu::StorageKey& queueKey,
                                     const mqbu::StorageKey& appKey,
                                     bool                    isCSLMode);

    static void
    updateQueueReplicaDispatched(int*                    status,
                                 StorageSpMap*           storageMap,
                                 bslmt::Mutex*           storagesLock,
                                 AppKeys*                appKeys,
                                 bslmt::Mutex*           appKeysLock,
                                 mqbi::DomainFactory*    domainFactory,
                                 const bsl::string&      clusterDescription,
                                 int                     partitionId,
                                 const bmqt::Uri&        uri,
                                 const mqbu::StorageKey& queueKey,
                                 const AppIdKeyPairs&    addedIdKeyPairs,
                                 bool                    isCSLMode,
                                 mqbi::Domain*           domain = 0,
                                 bool allowDuplicate            = false);

    /// Executed by queue-dispatcher thread with the specified
    /// `processorId`.
    static void
    setQueueDispatched(StorageSpMap*                            storageMap,
                       bslmt::Mutex*                            storagesLock,
                       const mqbi::Dispatcher::ProcessorHandle& processor,
                       const bsl::string& clusterDescription,
                       int                partitionId,
                       const bmqt::Uri&   uri,
                       mqbi::Queue*       queue);

    static int makeStorage(bsl::ostream&                     errorDescription,
                           bslma::ManagedPtr<mqbi::Storage>* out,
                           StorageSpMap*                     storageMap,
                           bslmt::Mutex*                     storagesLock,
                           const bmqt::Uri&                  uri,
                           const mqbu::StorageKey&           queueKey,
                           int                               partitionId,
                           const bsls::Types::Int64          messageTtl,
                           const int maxDeliveryAttempts,
                           const mqbconfm::StorageDefinition& storageDef);

    static void processPrimaryStatusAdvisoryDispatched(
        mqbs::FileStore*                           fs,
        PartitionInfo*                             pinfo,
        const bmqp_ctrlmsg::PrimaryStatusAdvisory& advisory,
        const bsl::string&                         clusterDescription,
        mqbnet::ClusterNode*                       source);

    static void processReplicaStatusAdvisoryDispatched(
        mqbc::ClusterData*              clusterData,
        mqbs::FileStore*                fs,
        int                             partitionId,
        const PartitionInfo&            pinfo,
        mqbnet::ClusterNode*            source,
        bmqp_ctrlmsg::NodeStatus::Value status);

    static void processShutdownEventDispatched(ClusterData*     clusterData,
                                               PartitionInfo*   pinfo,
                                               mqbs::FileStore* fs,
                                               int              partitionId);

    /// Explicitly call `flush` on the specified `fileStores` to enforce
    /// their GC.
    static void forceFlushFileStores(FileStores* fileStores);

    /// Process the specified `command` using the specified `fileStores`,
    /// `domainFactory` and `partitionLocation`, and load the result to the
    /// specified `result`.  The command might modify the specified
    /// `replicationFactor` and the corresponding value in each partition of
    /// the specified `fileStores`.  The specified `storageMapVec` might be
    /// used to find a storage for a specific queue, the specified
    /// `storagesLock` is used to access this container safely.  Use the
    /// specified `allocator` for memory allocations.  Return 0 if the command
    /// was successfully processed, or a non-zero value otherwise.  This
    /// function can be invoked from any thread, and will block until the
    /// potentially asynchronous operation is complete.
    static int processCommand(mqbcmd::StorageResult*        result,
                              FileStores*                   fileStores,
                              StorageSpMapVec*              storageMapVec,
                              bslmt::Mutex*                 storagesLock,
                              const mqbi::DomainFactory*    domainFactory,
                              int*                          replicationFactor,
                              const mqbcmd::StorageCommand& command,
                              const bslstl::StringRef&      partitionLocation,
                              bslma::Allocator*             allocator);

    static void onDomain(const bmqp_ctrlmsg::Status& status,
                         mqbi::Domain*               domain,
                         mqbi::Domain**              out,
                         bslmt::Latch*               latch,
                         const bsl::string&          clusterDescription,
                         const bsl::string&          domainName,
                         int                         partitionId);

    static void forceIssueAdvisoryAndSyncPt(mqbc::ClusterData*   clusterData,
                                            mqbs::FileStore*     fs,
                                            mqbnet::ClusterNode* destination,
                                            const PartitionInfo& pinfo);
};

template <>
unsigned int StorageUtil::extractPartitionId<true>(const bmqp::Event& event);

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ------------------
// struct StorageUtil
// ------------------

template <typename T>
void StorageUtil::loadDifference(bsl::vector<T>*       result,
                                 const bsl::vector<T>& baseSet,
                                 const bsl::vector<T>& subtractionSet)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(result);

    typedef typename bsl::vector<T>::const_iterator CIter;

    for (CIter it = baseSet.begin(); it != baseSet.end(); ++it) {
        if (subtractionSet.end() ==
            bsl::find(subtractionSet.begin(), subtractionSet.end(), *it)) {
            result->push_back(*it);
        }
    }
}

template <typename T>
void StorageUtil::loadAddedAndRemovedEntries(
    bsl::vector<T>*       addedEntries,
    bsl::vector<T>*       removedEntries,
    const bsl::vector<T>& existingEntries,
    const bsl::vector<T>& newEntries)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(addedEntries);
    BSLS_ASSERT_SAFE(removedEntries);

    // Find newly added entries.
    loadDifference(addedEntries, newEntries, existingEntries);

    // Find removed entries.
    loadDifference(removedEntries, existingEntries, newEntries);
}

template <bool IS_RECOVERY>
unsigned int StorageUtil::extractPartitionId(const bmqp::Event& event)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(event.isStorageEvent() || event.isPartitionSyncEvent());

    bmqp::StorageMessageIterator iter;
    event.loadStorageMessageIterator(&iter);
    BSLS_ASSERT_SAFE(iter.isValid());

    const int rc = iter.next();
    if (rc != 1) {
        return k_INVALID_PARTITION_ID;  // RETURN
    }

    return iter.header().partitionId();
}

}  // close package namespace
}  // close enterprise namespace

#endif
