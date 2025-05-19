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

/// @file mqbc_storageutil.h
///
/// @brief Provide generic utilities used for storage operations.
///
/// @bbref{mqbc::StorageUtil} provides generic utilities.

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
#include <mqbs_storageutil.h>
#include <mqbu_storagekey.h>

// BMQ
#include <bmqma_countingallocatorstore.h>
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_event.h>
#include <bmqp_storagemessageiterator.h>
#include <bmqt_uri.h>

// BDE
#include <ball_log.h>
#include <bdlb_nullablevalue.h>
#include <bdlmt_eventscheduler.h>
#include <bdlmt_fixedthreadpool.h>
#include <bsl_algorithm.h>
#include <bsl_functional.h>
#include <bsl_map.h>
#include <bsl_memory.h>
#include <bsl_numeric.h>
#include <bsl_string.h>
#include <bsl_unordered_set.h>
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
    typedef mqbi::StorageManager::AppInfos AppInfos;

    typedef mqbi::StorageManager::AppIds         AppIds;
    typedef mqbi::StorageManager::AppIdsIter     AppIdsIter;
    typedef mqbi::StorageManager::AppIdsInsertRc AppIdsInsertRc;

    typedef mqbi::StorageManager::StorageSp             StorageSp;
    typedef mqbi::StorageManager::StorageSpMap          StorageSpMap;
    typedef mqbi::StorageManager::StorageSpMapVec       StorageSpMapVec;
    typedef mqbi::StorageManager::StorageSpMapIter      StorageSpMapIter;
    typedef mqbi::StorageManager::StorageSpMapConstIter StorageSpMapConstIter;

    typedef mqbi::StorageManager::PartitionPrimaryStatusCb
        PartitionPrimaryStatusCb;

    typedef mqbs::DataStoreConfig::QueueKeyInfoMap QueueKeyInfoMap;
    typedef mqbs::DataStoreConfig::QueueKeyInfoMapConstIter
        QueueKeyInfoMapConstIter;

    typedef mqbs::DataStoreConfig::QueueCreationCb   QueueCreationCb;
    typedef mqbs::DataStoreConfig::QueueDeletionCb   QueueDeletionCb;
    typedef mqbs::DataStoreConfig::RecoveredQueuesCb RecoveredQueuesCb;

    typedef mqbi::StorageManager_PartitionInfo PartitionInfo;

    typedef bsl::function<void(int, bslmt::Latch*)> ShutdownCb;

    typedef mqbs::FileStore::StorageList StorageList;
    typedef bsl::vector<StorageList>     StorageLists;
    typedef StorageLists::const_iterator StorageListsConstIter;

    typedef mqbs::FileStore::StorageFilters StorageFilters;

    /// @note It is important that we use an associative container where
    ///       iterators don't get invalidated, because `onDomain` routine
    ///       implementation depends on this assumption.
    typedef bsl::map<bsl::string, mqbi::Domain*> DomainMap;
    typedef DomainMap::iterator DomainMapIter;

    typedef mqbs::StorageUtil::DomainQueueMessagesCountMap
        DomainQueueMessagesCountMap;

  public:
    // TYPES
    typedef bsl::shared_ptr<mqbs::FileStore> FileStoreSp;
    typedef bsl::vector<FileStoreSp>         FileStores;

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

    typedef bsl::vector<DomainQueueMessagesCountMap>
        DomainQueueMessagesCountMaps;

  private:
    // PRIVATE FUNCTIONS

    /// Load into the specified `result` the list of elements present in
    /// `baseSet` which are not present in `subtractionSet`.  If the specified
    /// `findConflicts` is `true`, detect appKey mismatch between the same
    /// appId in the `baseSet` and `subtractionSet` and return `false` if
    /// appKey values do not match.  Otherwise, return `true`.
    static bool loadDifference(mqbi::Storage::AppInfos*       result,
                               const mqbi::Storage::AppInfos& baseSet,
                               const mqbi::Storage::AppInfos& subtractionSet,
                               bool                           findConflicts);

    /// Load into the specified `result` the list of elements present in
    /// `baseSet` which are not present in `subtractionSet`.
    static void
    loadDifference(bsl::vector<bsl::string>*              result,
                   const bsl::unordered_set<bsl::string>& baseSet,
                   const bsl::unordered_set<bsl::string>& subtractionSet);

    /// Load into the specified `addedAppInfos` and
    /// `removedAppInfos` the appId/key pairs which have been added and
    /// removed respectively for the specified `existingAppInfos` based on the
    /// specified `newAppInfos`.  Return true if there are any added or removed
    /// appId/key pairs, false otherwise.
    ///
    /// THREAD: Executed by the queue dispatcher thread.
    static bool loadUpdatedAppInfos(AppInfos*       addedAppInfos,
                                    AppInfos*       removedAppInfos,
                                    const AppInfos& existingAppInfos,
                                    const AppInfos& newAppInfos);

    /// THREAD: Executed by the Queue's dispatcher thread.
    static void registerQueueDispatched(mqbs::FileStore*         fs,
                                        mqbs::ReplicatedStorage* storage,
                                        const bsl::string& clusterDescription,
                                        int                partitionId,
                                        const AppInfos&    appIdKeyPairs);

    /// THREAD: This method is called from the Queue's dispatcher thread.
    static void
    updateQueuePrimaryDispatched(mqbs::ReplicatedStorage* storage,
                                 bslmt::Mutex*            storagesLock,
                                 mqbs::FileStore*         fs,
                                 const bsl::string&       clusterDescription,
                                 int                      partitionId,
                                 const AppInfos&          appIdKeyPairs,
                                 bool                     isFanout);

    /// StorageManager's storages lock must be locked before calling this
    /// method.
    ///
    /// THREAD: Executed by the Queue's dispatcher thread.
    static int updateQueuePrimaryRaw(mqbs::ReplicatedStorage* storage,
                                     mqbs::FileStore*         fs,
                                     const bsl::string& clusterDescription,
                                     int                partitionId,
                                     const AppInfos&    addedIdKeyPairs,
                                     const AppInfos&    removedIdKeyPairs,
                                     bool               isFanout);

    static int
    addVirtualStoragesInternal(mqbs::ReplicatedStorage* storage,
                               const AppInfos&          appIdKeyPairs,
                               const bsl::string&       clusterDescription,
                               int                      partitionId,
                               bool                     isFanout);

    static int removeVirtualStorageInternal(mqbs::ReplicatedStorage* storage,
                                            const mqbu::StorageKey&  appKey,
                                            int  partitionId,
                                            bool asPrimary);

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

    static void
    purgeDomainDispatched(bsl::vector<bsl::vector<mqbcmd::PurgeQueueResult> >*
                                             purgedQueuesResultsVec,
                          bslmt::Latch*      latch,
                          int                partitionId,
                          StorageSpMapVec*   storageMapVec,
                          bslmt::Mutex*      storagesLock,
                          const FileStores*  fileStores,
                          const bsl::string& domainName);
    /// Execute the domain purge command for the specified `domainName` within
    /// the specified `partitionId`.  The specified `storageMapVec` contains
    /// mutable storages to search for domain's queues, while the specified
    /// `storagesLock` controls thread-safe access to this container.  The
    /// specified `latch` used to notify the calling thread that this operation
    /// has finished.  The specified `purgedQueuesResultsVec` is used to store
    /// execution results.  The specified `fileStores` contains FileStore
    /// objects used to verify correctness and thread-safety of calling this
    /// method.
    ///
    /// NOTE: designed to be called for all `partitionId`s in parallel by
    ///       `executeForEachPartition`.
    ///
    /// THREAD: Executed by the Queue's dispatcher thread for the specified
    ///         `partitionId`.

    static void
    purgeQueueDispatched(mqbcmd::PurgeQueueResult* purgedQueueResult,
                         bslmt::Semaphore*         purgeFinishedSemaphore,
                         mqbi::Storage*            storage,
                         const mqbs::FileStore*    fileStore,
                         const bsl::string&        appId);
    /// Execute the queue purge command for the specified `storage` with
    /// the specified `appId`.  The optionally specified
    /// `purgeFinishedSemaphore` used to notify the calling thread that this
    /// operation has finished. The specified `purgedQueuesResult` is used to
    /// store execution result. The specified `fileStore` contains FileStore
    /// object used to verify correctness and thread-safety of calling this
    /// method.
    ///
    /// THREAD: Executed by the Queue's dispatcher thread for the specified
    ///         `fileStore`.

    /// Execute the specified `job` for each partition in the specified
    /// `fileStores`.  Each partition will receive its partitionId and a
    /// latch along with the `job`.  Each partition *must* call
    /// `latch->arrive()` after it has finished executing the `job`.
    ///
    /// THREAD: Executed by the cluster-dispatcher thread.
    static void executeForEachPartitions(const PerPartitionFunctor& job,
                                         const FileStores& fileStores);

    /// For each partition which has the current node as the primary,
    /// Execute the specified `job` in the specified `fileStores`.
    /// Each partition will receive its partitionId and a latch
    /// along with the `job`.  Each valid partition *must* call
    /// `latch->arrive()` after it has finished executing the `job`.
    ///
    /// THREAD: Executed by the cluster-dispatcher thread.
    static void executeForValidPartitions(const PerPartitionFunctor& job,
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
    /// present in `newEntries` but not in `existingEntries`.  Similarly, load
    /// into the specified `removedEntries` the list of entries which are
    /// present in `existingEntries` but not in `newEntries`.  Return `false`
    /// if appKey values do not match for the same appId in the `newEntries`
    /// and the `existingEntries`.  Otherwise, return `true`.
    static bool
    loadAddedAndRemovedEntries(mqbi::Storage::AppInfos*       addedEntries,
                               mqbi::Storage::AppInfos*       removedEntries,
                               const mqbi::Storage::AppInfos& existingEntries,
                               const mqbi::Storage::AppInfos& newEntries);

    /// Load into the specified `addedEntries` the list of entries which are
    /// present in `newEntries` but not in `existingEntries`.  Similarly, load
    /// into the specified `removedEntries` the list of entries which are
    /// present in `existingEntries` but not in `newEntries`.
    static void loadAddedAndRemovedEntries(
        bsl::vector<bsl::string>*              addedEntries,
        bsl::vector<bsl::string>*              removedEntries,
        const bsl::unordered_set<bsl::string>& existingEntries,
        const bsl::unordered_set<bsl::string>& newEntries);

    /// Return true if the queue having specified `uri` has no messages in the
    /// specified `storageMap`, false in any other case.  If the optionally
    /// specified `storagesLock` is specified, lock it.  Behavior is undefined
    /// unless this routine is invoked from cluster dispatcher thread.
    static bool isStorageEmpty(bslmt::Mutex*       storagesLock,
                               const StorageSpMap& storageMap,
                               const bmqt::Uri&    uri);

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
    ///
    /// THREAD: Executed by the cluster dispatcher thread.
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

    /// Validate that every storage message in the specified 'event' have
    /// the same specified 'partitionId', and that the specified event
    /// 'source' is the specified 'primary' with the specified 'status'
    /// which needs to be ACTIVE.  Use the specified 'clusterData' to help
    /// with validation.  The specified 'skipAlarm' flag determines whether
    /// to skip alarming if 'source' is not active primary.  Use the
    /// specified 'isFSMWorkflow' flag to help with validation.  Return true
    /// if valid, false otherwise.
    ///
    /// THREAD: Executed by the Queue's dispatcher thread for the specified
    ///         `partitionId` or by the cluster dispatcher thread.
    static bool validateStorageEvent(const bmqp::Event&         event,
                                     int                        partitionId,
                                     const mqbnet::ClusterNode* source,
                                     const mqbnet::ClusterNode* primary,
                                     bmqp_ctrlmsg::PrimaryStatus::Value status,
                                     const bsl::string& clusterDescription,
                                     bool               skipAlarm,
                                     bool               isFSMWorkflow);

    /// Validate that every partition sync message in the specified `event`
    /// have the same specified `partitionId`, and that ether self or the
    /// specified event `source` is the primary node in the specified
    /// `partitionInfo`.  Use the specified `clusterData` and `isFSMWorkflow`
    /// flag to help with validation.  Return true if valid, false
    /// otherwise.
    ///
    /// THREAD: Executed by the Queue's dispatcher thread for the specified
    ///         `partitionId` or by the cluster dispatcher thread.
    static bool
    validatePartitionSyncEvent(const bmqp::Event&         event,
                               int                        partitionId,
                               const mqbnet::ClusterNode* source,
                               const PartitionInfo&       partitionInfo,
                               const mqbc::ClusterData&   clusterData,
                               bool                       isFSMWorkflow);

    /// Assign Queue dispatcher threads from the specified 'threadPool' of
    /// the specified 'clusterData', specified 'cluster' using the specified
    /// 'dispatcher' where the configuration is used from the specified
    /// 'config' to the specified 'fileStores' using the specified
    /// 'blobSpPool' where memory is allocated using the specified
    /// 'allocators', any errors are captured using the specified
    /// 'errorDescription' and there are specified 'recoveredQueuesCb',
    /// optionally specified 'queueCreationCb' and 'queueDeletionCb'.
    ///
    /// THREAD: Executed by the cluster *DISPATCHER* thread.
    static int assignPartitionDispatcherThreads(
        bdlmt::FixedThreadPool*                     threadPool,
        mqbc::ClusterData*                          clusterData,
        const mqbi::Cluster&                        cluster,
        mqbi::Dispatcher*                           dispatcher,
        const mqbcfg::PartitionConfig&              config,
        FileStores*                                 fileStores,
        BlobSpPool*                                 blobSpPool,
        bmqma::CountingAllocatorStore*              allocators,
        bsl::ostream&                               errorDescription,
        int                                         replicationFactor,
        const RecoveredQueuesCb&                    recoveredQueuesCb,
        const bdlb::NullableValue<QueueCreationCb>& queueCreationCb =
            bdlb::NullableValue<QueueCreationCb>(),
        const bdlb::NullableValue<QueueDeletionCb>& queueDeletionCb =
            bdlb::NullableValue<QueueDeletionCb>());

    /// Clear the specified `primary` of the specified `partitionId` from
    /// the specified `fs` and `partitionInfo`, using the specified
    /// `clusterDescription`.  Behavior is undefined unless the specified
    /// `partitionId` is in range and the specified `primary` is not null.
    ///
    /// THREAD: Executed by the queue dispatcher thread associated with
    ///         'partitionId'.
    static void clearPrimaryForPartition(mqbs::FileStore*   fs,
                                         PartitionInfo*     partitionInfo,
                                         const bsl::string& clusterDescription,
                                         int                partitionId,
                                         mqbnet::ClusterNode* primary);

    /// Find the minimum required disk space using the specified `config`.
    static bsls::Types::Uint64
    findMinReqDiskSpace(const mqbcfg::PartitionConfig& config);

    /// Transition self to active primary of the specified `partitionId` and
    /// load this info into the specified `partitionInfo`.  Then, broadcast
    /// a primary status advisory to peers using the specified
    /// `clusterData`.
    ///
    /// THREAD: Executed by the queue dispatcher thread associated with
    ///         'partitionId'.
    static void transitionToActivePrimary(PartitionInfo*     partitionInfo,
                                          mqbc::ClusterData* clusterData,
                                          int                partitionId);

    /// Callback executed after primary sync for the specified 'partitionId'
    /// is complete with the specified 'status'.  Use the specified 'fs',
    /// 'pinfo', 'clusterData' and 'partitionPrimaryStatusCb'.
    ///
    /// THREAD: Executed by the queue dispatcher thread associated with
    ///         'partitionId'.
    static void onPartitionPrimarySync(
        mqbs::FileStore*                fs,
        PartitionInfo*                  pinfo,
        mqbc::ClusterData*              clusterData,
        const PartitionPrimaryStatusCb& partitionPrimaryStatusCb,
        int                             partitionId,
        int                             status);

    /// Callback executed when the partition having the specified
    /// 'partitionId' has performed recovery and recovered file-backed
    /// queues and their virtual storages in the specified
    /// 'queueKeyInfoMap'.
    ///
    /// THREAD: Executed by the dispatcher thread of the partition.
    static void
    recoveredQueuesCb(StorageSpMap*                storageMap,
                      bslmt::Mutex*                storagesLock,
                      mqbs::FileStore*             fs,
                      mqbi::DomainFactory*         domainFactory,
                      bslmt::Mutex*                unrecognizedDomainsLock,
                      DomainQueueMessagesCountMap* unrecognizedDomains,
                      const bsl::string&           clusterDescription,
                      int                          partitionId,
                      const QueueKeyInfoMap&       queueKeyInfoMap,
                      bool                         isCSLMode);

    /// Print statistics regarding the specified 'unrecognizedDomains',
    /// protected by the specified 'unrecognizedDomainsLock', encountered
    /// during recovery of the specified 'clusterDescription', if any.
    static void dumpUnknownRecoveredDomains(
        const bsl::string&                  clusterDescription,
        bslmt::Mutex*                       unrecognizedDomainsLock,
        const DomainQueueMessagesCountMaps& unrecognizedDomains);

    /// GC the queues of the specified 'unrecognizedDomains', protected by the
    /// specified 'unrecognizedDomainsLock', from the specified 'fileStores',
    /// if any.
    static void gcUnrecognizedDomainQueues(
        FileStores*                         fileStores,
        bslmt::Mutex*                       unrecognizedDomainsLock,
        const DomainQueueMessagesCountMaps& unrecognizedDomains);

    /// Stop all the underlying partitions by scheduling execution of the
    /// specified `shutdownCb` for each FileStore in the specified
    /// `fileStores`.  Use the specified `clusterDescription` for logging.
    ///
    /// THREAD: Executed by cluster *DISPATCHER* thread.
    static void stop(FileStores*        fileStores,
                     const bsl::string& clusterDescription,
                     const ShutdownCb&  shutdownCb);

    /// Shutdown the underlying partition associated with the specified
    /// `partitionId` from the specified `fileStores` by using the specified
    /// `latch`, and the specified `clusterConfig`. The cluster information
    /// is printed using the specified `clusterDescription`.
    ///
    /// THREAD: Executed by *QUEUE_DISPATCHER* thread with the specified
    ///         `partitionId`.
    static void shutdown(int                              partitionId,
                         bslmt::Latch*                    latch,
                         FileStores*                      fileStores,
                         const bsl::string&               clusterDescription,
                         const mqbcfg::ClusterDefinition& clusterConfig);

    /// Return a unique appKey for the specified `appId` for a queue, and
    /// load the appKey into the specified `appKeys`.  This routine can be
    /// invoked by any thread.
    static mqbu::StorageKey
    generateAppKey(bsl::unordered_set<mqbu::StorageKey>* appKeys,
                   const bsl::string&                    appId);

    /// Register a queue with the specified `uri`, `queueKey` and
    /// `partitionId`, having the specified `appIdKeyPairs`, and belonging to
    /// the specified `domain`.  Load into the specified `storage` the
    /// associated queue storage created.
    ///
    /// THREAD: Executed by the Client's dispatcher thread.
    static void registerQueue(const mqbi::Cluster*           cluster,
                              mqbi::Dispatcher*              dispatcher,
                              StorageSpMap*                  storageMap,
                              bslmt::Mutex*                  storagesLock,
                              mqbs::FileStore*               fs,
                              bmqma::CountingAllocatorStore* allocators,
                              const bmqt::Uri&               uri,
                              const mqbu::StorageKey&        queueKey,
                              const bsl::string& clusterDescription,
                              int                partitionId,
                              const AppInfos&    appIdKeyPairs,
                              mqbi::Domain*      domain);

    /// THREAD: Executed by the Queue's dispatcher thread.
    static void unregisterQueueDispatched(mqbs::FileStore*     fs,
                                          StorageSpMap*        storageMap,
                                          bslmt::Mutex*        storagesLock,
                                          const ClusterData*   clusterData,
                                          int                  partitionId,
                                          const PartitionInfo& pinfo,
                                          const bmqt::Uri&     uri);

    /// Configure the fanout queue having specified `uri` and `queueKey`,
    /// assigned to the specified `partitionId` to have the specified
    /// `addedIdKeyPairs` appId/appKey pairs added and `removedIdKeyPairs`
    /// appId/appKey pairs removed.  Return zero on success, and non-zero
    /// value otherwise.  Behavior is undefined unless this function is
    /// invoked at the primary node.  Behavior is also undefined unless the
    /// queue is configured in fanout mode.
    ///
    /// THREAD: Executed by the Queue's dispatcher thread.
    static int updateQueuePrimary(StorageSpMap*           storageMap,
                                  bslmt::Mutex*           storagesLock,
                                  mqbs::FileStore*        fs,
                                  const bsl::string&      clusterDescription,
                                  const bmqt::Uri&        uri,
                                  const mqbu::StorageKey& queueKey,
                                  int                     partitionId,
                                  const AppInfos&         addedIdKeyPairs,
                                  const AppInfos&         removedIdKeyPairs);

    static void
    registerQueueReplicaDispatched(int*                 status,
                                   StorageSpMap*        storageMap,
                                   bslmt::Mutex*        storagesLock,
                                   mqbs::FileStore*     fs,
                                   mqbi::DomainFactory* domainFactory,
                                   bmqma::CountingAllocatorStore* allocators,
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
                                 mqbi::DomainFactory*    domainFactory,
                                 const bsl::string&      clusterDescription,
                                 int                     partitionId,
                                 const bmqt::Uri&        uri,
                                 const mqbu::StorageKey& queueKey,
                                 const AppInfos&         addedIdKeyPairs,
                                 mqbi::Domain*           domain = 0,
                                 bool allowDuplicate            = false);

    /// Executed by queue-dispatcher thread with the specified
    /// `processorId`.
    static void setQueueDispatched(StorageSpMap*      storageMap,
                                   bslmt::Mutex*      storagesLock,
                                   const bsl::string& clusterDescription,
                                   int                partitionId,
                                   const bmqt::Uri&   uri,
                                   mqbi::Queue*       queue);

    static int makeStorage(bsl::ostream&                   errorDescription,
                           bsl::shared_ptr<mqbi::Storage>* out,
                           StorageSpMap*                   storageMap,
                           bslmt::Mutex*                   storagesLock,
                           const bmqt::Uri&                uri,
                           const mqbu::StorageKey&         queueKey,
                           int                             partitionId,
                           const bsls::Types::Int64        messageTtl,
                           const int                       maxDeliveryAttempts,
                           const mqbconfm::StorageDefinition& storageDef);

    /// THREAD: Executed by the queue dispatcher thread associated with
    ///         'partitionId'.
    static void processPrimaryStatusAdvisoryDispatched(
        mqbs::FileStore*                           fs,
        PartitionInfo*                             pinfo,
        const bmqp_ctrlmsg::PrimaryStatusAdvisory& advisory,
        const bsl::string&                         clusterDescription,
        mqbnet::ClusterNode*                       source,
        bool                                       isFSMWorkflow);

    /// THREAD: Executed by the queue dispatcher thread associated with
    ///         'partitionId'.
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
    //
    /// THREAD: Executed by the cluster-dispatcher thread.
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

    /// THREAD: Executed by the Queue's dispatcher thread for the partitionId
    ///         of the specified `fs`.
    static void forceIssueAdvisoryAndSyncPt(mqbc::ClusterData*   clusterData,
                                            mqbs::FileStore*     fs,
                                            mqbnet::ClusterNode* destination,
                                            const PartitionInfo& pinfo);

    /// Purge the queues on a given domain.
    static void purgeQueueOnDomain(mqbcmd::StorageResult* result,
                                   const bsl::string&     domainName,
                                   FileStores*            fileStores,
                                   StorageSpMapVec*       storageMapVec,
                                   bslmt::Mutex*          storagesLock);
};

template <>
unsigned int StorageUtil::extractPartitionId<true>(const bmqp::Event& event);

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ------------------
// struct StorageUtil
// ------------------

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
