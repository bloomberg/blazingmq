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

#ifndef INCLUDED_MQBC_STORAGEUTIL
#define INCLUDED_MQBC_STORAGEUTIL

/// @file mqbc_storageutil.h
///
/// @brief Provide generic utilities used for storage operations.
///
/// @bbref{mqbc::StorageUtil} provides generic utilities.

// MQB
#include <mqbi_storage.h>
#include <mqbi_storagemanager.h>
#include <mqbs_datastore.h>
#include <mqbs_filestore.h>
#include <mqbs_storageutil.h>
#include <mqbu_storagekey.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_event.h>
#include <bmqp_storagemessageiterator.h>
#include <bmqt_uri.h>

// BDE
#include <ball_log.h>
#include <bsl_functional.h>
#include <bsl_map.h>
#include <bsl_memory.h>
#include <bsl_ostream.h>
#include <bsl_string.h>
#include <bsl_unordered_map.h>
#include <bsl_unordered_set.h>
#include <bsl_vector.h>
#include <bslma_allocator.h>
#include <bslmt_latch.h>
#include <bsls_assert.h>
#include <bsls_atomic.h>
#include <bsls_types.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace bdlmt {
class FixedThreadPool;
}
namespace bmqma {
class CountingAllocatorStore;
}
namespace bslmt {
class Mutex;
}
namespace mqbc {
class ClusterData;
class ClusterState;
}
namespace mqbcfg {
class ClusterDefinition;
class PartitionConfig;
}
namespace mqbi {
class Cluster;
class Dispatcher;
class Domain;
class DomainFactory;
class Queue;
class StorageProvider;
}
namespace mqbs {
class ReplicatedStorage;
}
namespace mqbcmd {
class ClusterStorageSummary;
class PurgeQueueResult;
class ReplicationCommand;
class ReplicationResult;
class StorageCommand;
class StorageQueueInfo;
class StorageResult;
}
namespace mqbconfm {
class StorageDefinition;
}
namespace mqbnet {
class ClusterNode;
}

namespace mqbc {

// FORWARD DECLARATION
class StorageMonitor;

// ==================
// struct StorageUtil
// ==================

/// Generic utilities for storage related operations.
struct StorageUtil {
  public:
    // PUBLIC CONSTANTS

    /// Constant used to represent an invalid partition id.
    static const unsigned int k_INVALID_PARTITION_ID =
        static_cast<unsigned int>(mqbi::Storage::k_INVALID_PARTITION_ID);

  private:
    // CLASS SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBC.STORAGEUTIL");

  private:
    // TYPES
    typedef mqbi::StorageManager::AppInfos AppInfos;

    typedef mqbi::StorageManager::AppIds         AppIds;
    typedef mqbi::StorageManager::AppIdsIter     AppIdsIter;
    typedef mqbi::StorageManager::AppIdsInsertRc AppIdsInsertRc;

    typedef mqbi::StorageManager::StorageSp StorageSp;

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
    typedef DomainMap::iterator                  DomainMapIter;

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
    static int registerQueueDispatched(mqbs::RecordStore*       rs,
                                       mqbs::ReplicatedStorage* storage,
                                       const AppInfos&          appIdKeyPairs);

    /// THREAD: This method is called from the Queue's dispatcher thread.
    static void updateQueuePrimaryDispatched(mqbs::ReplicatedStorage* storage,
                                             mqbs::RecordStore*       rs,
                                             const AppInfos& appIdKeyPairs,
                                             bool            isFanout);

    /// StorageManager's storages lock must be locked before calling this
    /// method.
    ///
    /// THREAD: Executed by the Queue's dispatcher thread.
    static int updateQueuePrimaryRaw(mqbs::ReplicatedStorage* storage,
                                     mqbs::RecordStore*       rs,
                                     const AppInfos&          addedIdKeyPairs,
                                     const AppInfos& removedIdKeyPairs,
                                     bool            isFanout);

    static int addVirtualStoragesInternal(mqbs::ReplicatedStorage* storage,
                                          const AppInfos&  appIdKeyPairs,
                                          bsl::string_view description,
                                          bool             isFanout);

    static int removeVirtualStorageInternal(mqbs::ReplicatedStorage* storage,
                                            const mqbu::StorageKey&  appKey,
                                            bool asPrimary);

    static void createQueueStorageAsPrimary(mqbs::RecordStore*      rs,
                                            const bmqt::Uri&        uri,
                                            const mqbu::StorageKey& queueKey,
                                            const AppInfos& appIdKeyPairs,
                                            mqbi::Domain*   domain);

    static bsl::shared_ptr<mqbs::ReplicatedStorage>
    createQueueStorageImpl(mqbs::RecordStore*      rs,
                           const bmqt::Uri&        uri,
                           const mqbu::StorageKey& queueKey,
                           const AppInfos&         appIdKeyPairs,
                           mqbi::Domain*           domain);

    /// Execute the specified `job` on the dispatcher thread of the specified
    /// `partitionId` of the specified `provider`.
    ///
    /// THREAD: Executed by any thread.
    static void executeOnPartition(mqbi::StorageProvider* provider,
                                   int                    partitionId,
                                   const mqbi::Dispatcher::VoidFunction& job);

    /// Block until everything `executeOnPartition` has enqueued so far on the
    /// specified `partitionId` of the specified `provider` has run.
    ///
    /// THREAD: Executed by any thread other than `partitionId`s dispatcher
    ///         one, which would deadlock.
    static void synchronizePartition(mqbi::StorageProvider* provider,
                                     int                    partitionId);

    /// Load the list of queue storages on the partition of the specified
    /// `provider` having the specified `partitionId` into the corresponding
    /// element of the specified `storageLists`, and arrive on the specified
    /// `latch` upon completion.  All queues for which any filter in the
    /// specified `filters` returns false will be filtered out from the list.
    /// The size of `storageLists` must be equal to the number of partitions
    /// in the storage manager before calling this function.
    ///
    /// THREAD: Executed by the Queue's dispatcher thread for the specified
    ///         `partitionId`.
    static void getStoragesDispatched(StorageLists*          storageLists,
                                      bslmt::Latch*          latch,
                                      mqbi::StorageProvider* provider,
                                      int                    partitionId,
                                      const StorageFilters&  filters);

    /// Load the status of the queue storages of the specified `provider`
    /// belonging to the specified `domainName` into the specified `storages`.
    /// The specified `storageMonitor` supplies the partition count.
    ///
    /// THREAD: Executed by the thread issuing the command.
    static void loadStorages(bsl::vector<mqbcmd::StorageQueueInfo>* storages,
                             const bsl::string&                     domainName,
                             mqbi::StorageProvider*                 provider,
                             StorageMonitor* storageMonitor);

    /// Load the summary of the partition of the specified `provider` having
    /// the specified `partitionId` and `partitionLocation` into the specified
    /// `result` object.  The specified `storageMonitor` supplies the
    /// partition count.
    ///
    /// THREAD: Executed by the thread issuing the command.
    static void
    loadPartitionStorageSummary(mqbcmd::StorageResult*   result,
                                mqbi::StorageProvider*   provider,
                                StorageMonitor*          storageMonitor,
                                int                      partitionId,
                                const bslstl::StringRef& partitionLocation);

    /// Roll the partition of the specified `provider` having the specified
    /// `partitionId` over, or every partition if it is negative, and wait for
    /// the rollover to take effect -- on a Raft partition it is only proposed,
    /// and the file set is swapped when it commits.  The specified
    /// `storageMonitor` supplies the partition count.  Use the specified
    /// `allocator` for memory allocations.  Store the result into the
    /// specified `result` object.
    ///
    /// THREAD: Executed by the thread issuing the command.
    static void doRollover(mqbcmd::StorageResult* result,
                           mqbi::StorageProvider* provider,
                           StorageMonitor*        storageMonitor,
                           int                    partitionId,
                           bslma::Allocator*      allocator);

    /// Initiate the rollover of the partition of the specified `provider`
    /// having the specified `partitionId` and arrive on the specified `latch`
    /// upon completion.  Return the error code via the specified `rc`, and
    /// load into the specified `rolloverSeqNum` the sequence number of the
    /// record the rollover wrote, or 0 if it wrote none.
    ///
    /// THREAD: Executed by the Queue's dispatcher thread for the specified
    ///         `partitionId`.
    static void doRolloverDispatched(bslmt::Latch*          latch,
                                     int*                   rc,
                                     bsls::Types::Uint64*   rolloverSeqNum,
                                     mqbi::StorageProvider* provider,
                                     int                    partitionId);

    /// Hand the leadership of the partition of the specified `provider`
    /// having the specified `partitionId` to the node whose host name is the
    /// specified `targetName`.  Store the result into the specified `result`
    /// object.
    ///
    /// THREAD: Executed by the thread issuing the command.
    static void doTransferLeadership(mqbcmd::StorageResult* result,
                                     mqbi::StorageProvider* provider,
                                     int                    partitionId,
                                     const bsl::string&     targetName);

    /// Hand the leadership of the specified `partitionId` of the specified
    /// `provider` to the node whose host name is the specified
    /// `targetHostName`, and store into the specified `result` whether the
    /// transfer was started -- not whether it succeeded, which the caller
    /// establishes by waiting for the new primary.
    ///
    /// THREAD: Executed by the Queue's dispatcher thread for the specified
    ///         `partitionId`.
    static void
    doTransferLeadershipDispatched(mqbcmd::StorageResult* result,
                                   mqbi::StorageProvider* provider,
                                   int                    partitionId,
                                   const bsl::string&     targetHostName);

    /// Load the summary of the partitions of the specified `provider` at the
    /// specified `location` to the specified `result` object.  The specified
    /// `storageMonitor` supplies the partition count.
    ///
    /// THREAD: Executed by the thread issuing the command, or by the cluster
    ///         dispatcher thread when `Cluster::loadClusterStatus` folds a
    ///         `SUMMARY` into CLUSTER STATUS.
    static void loadStorageSummary(mqbcmd::StorageResult*  result,
                                   mqbi::StorageProvider*  provider,
                                   StorageMonitor*         storageMonitor,
                                   const bslstl::StringRef location);

    /// Load the summary of the partition of the specified `provider` having
    /// the specified `partitionId` into an element of the specified `summary`
    /// object, and arrive on the specified `latch` upon completion.
    ///
    /// THREAD: Executed by the Queue's dispatcher thread for the specified
    ///         `partitionId`.
    static void
    loadStorageSummaryDispatched(mqbcmd::ClusterStorageSummary* summary,
                                 bslmt::Latch*                  latch,
                                 int                            partitionId,
                                 mqbi::StorageProvider*         provider);

    /// Execute the domain purge command for the specified `domainName` within
    /// the specified `partitionId` of the specified `provider`.  The
    /// specified `storageMonitor` supplies the domain's queues.  The
    /// specified `latch` is used to notify the calling thread that this
    /// operation has finished.  The specified `purgedQueuesResultsVec` is
    /// used to store execution results, and the specified `purgeSeqNums` the
    /// sequence number of the last purge record written per partition.  If
    /// the specified `leadersOnly` is true, report nothing at all for a
    /// partition this node does not lead, rather than the per-queue rejection
    /// `purgeQueueDispatched` produces there.
    ///
    /// NOTE: designed to be called for all `partitionId`s in parallel by
    ///       `executeForEachPartitions`.
    ///
    /// THREAD: Executed by the Queue's dispatcher thread for the specified
    ///         `partitionId`.
    static void
    purgeDomainDispatched(bsl::vector<bsl::vector<mqbcmd::PurgeQueueResult> >*
                              purgedQueuesResultsVec,
                          bsl::vector<bsls::Types::Uint64>* purgeSeqNums,
                          bslmt::Latch*                     latch,
                          int                               partitionId,
                          mqbi::StorageProvider*            provider,
                          StorageMonitor*                   storageMonitor,
                          bool                              leadersOnly,
                          const bsl::string&                domainName);

    /// Look the queue having the specified `uri` up in the specified
    /// `storageMonitor` and purge it as `purgeQueueDispatched` does.  Load an
    /// error into the specified `purgedQueueResult` if it is no longer
    /// registered on the specified `partitionId` of the specified `provider`:
    /// the lookup that routed this call here ran on the command's thread, and
    /// only this one -- on the same thread that registers and unregisters
    /// storages -- is authoritative.
    ///
    /// THREAD: Executed by the Queue's dispatcher thread for the specified
    ///         `partitionId`.
    static void
    findAndPurgeQueueDispatched(mqbcmd::PurgeQueueResult* purgedQueueResult,
                                bsls::Types::Uint64*      purgeSeqNum,
                                mqbi::StorageProvider*    provider,
                                StorageMonitor*           storageMonitor,
                                int                       partitionId,
                                const bmqt::Uri&          uri,
                                const bsl::string&        appId);

    /// Execute the queue purge command for the specified `storage` with the
    /// specified `appId`.  Load into the specified `purgeSeqNum` the sequence
    /// number of the purge record, or 0 if none was written; on a Raft
    /// partition that record is only proposed here, and `waitUntilApplied`
    /// pairs the sequence number with `mqbs::RecordStore::isApplied` to tell
    /// when the purge has taken effect.  The specified `purgedQueueResult` is
    /// used to store execution result.  The specified `recordStore` is the
    /// one driving `storage`s partition.
    ///
    /// THREAD: Executed by the Queue's dispatcher thread for the specified
    ///         `recordStore`.
    static void
    purgeQueueDispatched(mqbcmd::PurgeQueueResult* purgedQueueResult,
                         bsls::Types::Uint64*      purgeSeqNum,
                         const StorageSp&          storage,
                         const mqbs::RecordStore*  recordStore,
                         const bsl::string&        appId);

    /// Wait for the record at the specified `sequenceNumber` -- written on
    /// the specified `partitionId` of the specified `provider` by the
    /// specified `operation`, which names it in any error reported -- to have
    /// been applied, and return true.  Return true immediately if
    /// `sequenceNumber` is 0, meaning no record was written.  Return false,
    /// loading a reason into the specified `errorDescription`, if it has not
    /// applied by the specified `deadline` or the node stops being the
    /// primary first: a record proposed by a node that loses primaryship
    /// never takes effect there.
    ///
    /// THREAD: Executed by the thread issuing the command; must not be
    ///         `partitionId`s own dispatcher thread.
    static bool waitUntilApplied(bmqu::MemOutStream*       errorDescription,
                                 mqbi::StorageProvider*    provider,
                                 int                       partitionId,
                                 bsls::Types::Uint64       sequenceNumber,
                                 const bslstl::StringRef&  operation,
                                 const bsls::TimeInterval& deadline);

    /// Execute the specified `job` for each partition of the specified
    /// `provider`, the specified `storageMonitor` supplying their count.
    /// Each partition will receive its partitionId and a latch along with the
    /// `job`.  Each partition *must* call `latch->arrive()` after it has
    /// finished executing the `job`.
    ///
    /// THREAD: Executed by any thread other than a partition dispatcher one,
    ///         which would deadlock on the latch.
    static void executeForEachPartitions(const PerPartitionFunctor& job,
                                         mqbi::StorageProvider*     provider,
                                         StorageMonitor* storageMonitor);

    /// Process the specified `command`, and load the result to the specified
    /// `replicationResult`.  The command might modify the specified
    /// `replicationFactor` and the corresponding value in each partition of
    /// the specified `provider`, the specified `storageMonitor` supplying
    /// their count.  Return 0 if the command was successfully processed, or a
    /// non-zero value otherwise.
    ///
    /// THREAD: Executed by the thread issuing the command.
    static int
    processReplicationCommand(mqbcmd::ReplicationResult* replicationResult,
                              bsls::AtomicInt*           replicationFactor,
                              mqbi::StorageProvider*     provider,
                              StorageMonitor*            storageMonitor,
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
        bdlmt::FixedThreadPool*        threadPool,
        mqbc::ClusterData*             clusterData,
        const mqbi::Cluster&           cluster,
        mqbi::Dispatcher*              dispatcher,
        const mqbcfg::PartitionConfig& config,
        FileStores*                    fileStores,
        mqbs::StorageMonitor*          storageMonitor,
        BlobSpPool*                    blobSpPool,
        bmqma::CountingAllocatorStore* allocators,
        bsl::ostream&                  errorDescription,
        int                            replicationFactor,
        const RecoveredQueuesCb&       recoveredQueuesCb,
        const QueueCreationCb&         queueCreationCb,
        const QueueDeletionCb&         queueDeletionCb);

    /// Clear the primary of the specified `partitionId` from the specified
    /// `fs` and `partitionInfo`, using the specified clusterDescription`.
    /// Behavior is undefined unless the specified `partitionId` is in range.
    ///
    /// THREAD: Executed by the queue dispatcher thread associated with
    ///         'partitionId'.
    static void clearPrimaryForPartition(mqbs::FileStore*   fs,
                                         PartitionInfo*     partitionInfo,
                                         const bsl::string& clusterDescription,
                                         int                partitionId);

    /// Find the minimum required disk space using the specified `config`.
    static bsls::Types::Uint64
    findMinReqDiskSpace(const mqbcfg::PartitionConfig& config);

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
    /// 'queueKeyInfoMap'.  Records above the specified 'snapshotOffset' are
    /// left alone: under Raft those are log entries the commit-apply path has
    /// yet to apply.  Legacy recovery has no boundary and passes
    /// 'k_NO_SNAPSHOT_BOUNDARY', which applies every record.
    ///
    /// THREAD: Executed by the dispatcher thread of the partition.
    /// Value of `recoveredQueuesCb`'s `snapshotOffset` meaning "no boundary":
    /// every record is recovered state.  Distinct from an offset of 0, which
    /// under Raft means nothing has rolled over yet, so no record is.
    static const bsls::Types::Uint64 k_NO_SNAPSHOT_BOUNDARY;

    static void
    recoveredQueuesCb(mqbs::RecordStore*           recordStore,
                      mqbi::DomainFactory*         domainFactory,
                      bslmt::Mutex*                unrecognizedDomainsLock,
                      DomainQueueMessagesCountMap* unrecognizedDomains,
                      mqbc::ClusterState*          clusterState,
                      const bsl::string&           clusterDescription,
                      int                          partitionId,
                      const QueueKeyInfoMap*       queueKeyInfoMap,
                      bsls::Types::Uint64          snapshotOffset,
                      bslma::Allocator*            allocator);

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
    /// `partitionId` by closing the specified `recordStore` (may be null if
    /// the partition was never created), using the specified `latch`, and
    /// the specified `clusterConfig`. The cluster information is printed
    /// using the specified `clusterDescription`.
    ///
    /// THREAD: Executed by *QUEUE_DISPATCHER* thread with the specified
    ///         `partitionId`.
    static void shutdown(int                              partitionId,
                         bslmt::Latch*                    latch,
                         mqbs::RecordStore*               recordStore,
                         const bsl::string&               clusterDescription,
                         const mqbcfg::ClusterDefinition& clusterConfig);

    /// Lookup queue storage for the specified `uri` in the specified
    /// `storageMap` under specified `storagesLock`.  If the storage is
    /// missing, create it using the specified `appIdKeyPairs`, `domain`.
    /// Insert the created storage into Load into `storageMap`.
    ///
    /// THREAD: Executed by the Client's dispatcher thread.
    /// Register the queue having the specified 'uri', 'queueKey',
    /// 'appIdKeyPairs', and 'domain' as primary using the specified
    /// 'storageMap', 'storagesLock', and 'rs' (RecordStore).
    ///
    /// THREAD: Must be called from the partition's dispatcher thread.
    static void registerQueueAsPrimary(mqbs::RecordStore*      rs,
                                       const bmqt::Uri&        uri,
                                       const mqbu::StorageKey& queueKey,
                                       const AppInfos&         appIdKeyPairs,
                                       mqbi::Domain*           domain);

    /// THREAD: Executed by the Queue's dispatcher thread.
    static void unregisterQueueDispatched(mqbs::RecordStore*   rs,
                                          const ClusterData*   clusterData,
                                          int                  partitionId,
                                          const PartitionInfo& pinfo,
                                          const bmqt::Uri&     uri);

    /// Configure the fanout queue having specified `uri` and `queueKey`,
    /// assigned to the specified `fs` to have the specified
    /// `addedIdKeyPairs` appId/appKey pairs added and `removedIdKeyPairs`
    /// appId/appKey pairs removed.  Return zero on success, and non-zero
    /// value otherwise.  Behavior is undefined unless this function is
    /// invoked at the primary node.  Behavior is also undefined unless the
    /// queue is configured in fanout mode.
    ///
    /// THREAD: Executed by the Queue's dispatcher thread.
    static int updateQueuePrimary(mqbs::RecordStore* rs,
                                  const bmqt::Uri&   uri,
                                  const AppInfos&    addedIdKeyPairs,
                                  const AppInfos&    removedIdKeyPairs);

    static void createQueueStorageAsReplica(mqbs::RecordStore*   rs,
                                            mqbi::DomainFactory* domainFactory,
                                            const bmqt::Uri&     uri,
                                            const mqbu::StorageKey& queueKey,
                                            const AppInfos& appIdKeyPairs,
                                            mqbi::Domain*   domain);

    static void removeQueueStorageDispatched(mqbs::RecordStore*      rs,
                                             const bmqt::Uri&        uri,
                                             const mqbu::StorageKey& queueKey,
                                             const mqbu::StorageKey& appKey);

    static void
    updateQueueStorageDispatched(mqbi::DomainFactory*    domainFactory,
                                 mqbs::RecordStore*      rs,
                                 const bmqt::Uri&        uri,
                                 const mqbu::StorageKey& queueKey,
                                 const AppInfos&         addedIdKeyPairs,
                                 mqbi::Domain*           domain = 0);

    static int configureStorage(bsl::ostream& errorDescription,
                                bsl::shared_ptr<mqbi::Storage>* out,
                                mqbs::RecordStore*              rs,
                                const bmqt::Uri&                uri,
                                const mqbu::StorageKey&         queueKey,
                                int                             partitionId,
                                const bsls::Types::Int64        messageTtl,
                                const int maxDeliveryAttempts,
                                const mqbconfm::StorageDefinition& storageDef);

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

    /// Process the specified `command` using the specified `provider`,
    /// `storageMonitor`, `domainFactory` and `partitionLocation`, and load
    /// the result to the specified `result`.  The `provider` supplies the
    /// partitions the command is dispatched to and the record stores it acts
    /// on; the cluster-wide `storageMonitor` supplies their queue storages.
    /// The command might modify the specified `replicationFactor` and the
    /// corresponding value in each partition.  Use the specified `allocator`
    /// for memory allocations.
    ///
    /// THREAD: Executed by the thread issuing the command; must not be a
    ///         partition dispatcher thread, since every branch dispatches
    ///         into those and waits.
    static void processCommand(mqbcmd::StorageResult*        result,
                               mqbi::StorageProvider*        provider,
                               StorageMonitor*               storageMonitor,
                               const mqbi::DomainFactory*    domainFactory,
                               bsls::AtomicInt*              replicationFactor,
                               const mqbcmd::StorageCommand& command,
                               const bslstl::StringRef&      partitionLocation,
                               bslma::Allocator*             allocator);

    /// THREAD: Executed by the Queue's dispatcher thread for the partitionId
    ///         of the specified `fs`.
    static void forceIssueAdvisoryAndSyncPt(mqbc::ClusterData*   clusterData,
                                            mqbs::FileStore*     fs,
                                            mqbnet::ClusterNode* destination,
                                            const PartitionInfo& pinfo);

    /// Purge the queues on a given domain.
    static void purgeQueueOnDomain(mqbcmd::StorageResult* result,
                                   const bsl::string&     domainName,
                                   mqbi::StorageProvider* provider,
                                   StorageMonitor*        storageMonitor);
};

// =====================
// class storageMonitor
// =====================

/// Concrete implementation of @bbref{mqbs::storageMonitor} which owns the
/// per-partition map of queue storages.
class StorageMonitor : public mqbs::StorageMonitor {
  public:
    // TYPES
    enum RegistrationState {
        /// The storage exists and carries every App asked for.
        e_REGISTERED = 0,
        /// A registration is outstanding; wait for it rather than proposing
        /// a second one.
        e_PENDING = 1,
        /// Nothing is outstanding and something is missing; the caller must
        /// propose.  Marked outstanding by the call.
        e_REQUIRED = 2
    };

    /// Map of appKey -> appId for a queue's registered apps.
    typedef mqbs::DataStoreConfigQueueInfo::AppInfos AppKeyToIdMap;

    struct StorageWithApps {
        StorageSp     d_storage_sp;
        AppKeyToIdMap d_apps;

        /// `true` between proposing this queue's registration and the storage
        /// (or the Apps) actually arriving.  On the Raft write path a
        /// QueueOp only takes effect when it commits, so without this a
        /// second `registerQueue` -- a concurrent open, or a parked one
        /// replayed by `onQueueStorageReady` -- would look at an unchanged
        /// storage and propose the very same record again.
        bool d_awaitingRegistration;

        StorageWithApps(bslma::Allocator* basicAllocator = 0)
        : d_storage_sp()
        , d_apps(basicAllocator)
        , d_awaitingRegistration(false)
        {
        }

        StorageWithApps(const StorageWithApps& other,
                        bslma::Allocator*      basicAllocator = 0)
        : d_storage_sp(other.d_storage_sp)
        , d_apps(other.d_apps, basicAllocator)
        , d_awaitingRegistration(other.d_awaitingRegistration)
        {
        }
    };
    /// Map of QueueUri -> ReplicatedStorageSp
    typedef bsl::unordered_map<bmqt::Uri, StorageWithApps> StorageSpMap;
    typedef StorageSpMap::iterator                         StorageSpMapIter;
    typedef StorageSpMap::const_iterator StorageSpMapConstIter;

    typedef bsl::vector<StorageSpMap>       StorageSpMapVec;
    typedef StorageSpMapVec::iterator       StorageSpMapVecIter;
    typedef StorageSpMapVec::const_iterator StorageSpMapVecConstIter;

  private:
    // DATA

    /// Vector of `(CanonicalQueueUri -> ReplicatedStorage)` maps.  Vector is
    /// indexed by partitionId.  The maps contains *both* in-memory and
    /// file-backed storages.  Note that `d_storageLockVec[partitionId]` must
    /// be held while accessing `d_storages[partitionId]`, because they are
    /// accessed from partitions' dispatcher threads, as well as cluster
    /// dispatcher thread.
    ///
    /// THREAD: Protected by `d_storageLockVec` (per partition).
    bsl::vector<StorageSpMap> d_storages;

    /// Vector of mutexes to protect access to `d_storages` and its elements,
    /// one per partition.  See comments for `d_storages`.
    mutable bsl::vector<bsl::shared_ptr<bslmt::Mutex> > d_storageLockVec;

    /// Cluster to notify (via `onQueueStorageReady`) whenever a queue's
    /// storage is registered/unregistered or its app set changes, so that
    /// any locally-parked queue-open can re-check readiness.  May be null in
    /// unit tests that construct a bare `storageMonitor`.
    mqbi::Cluster* d_cluster_p;

    bslma::Allocator* d_allocator_p;

    // NOT IMPLEMENTED
    StorageMonitor(const StorageMonitor&);
    StorageMonitor& operator=(const StorageMonitor&);

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(StorageMonitor, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a `storageMonitor` notifying the optionally specified
    /// `cluster` (may be null, e.g. in unit tests) of queue storage/app
    /// availability changes.
    explicit StorageMonitor(mqbi::Cluster*    cluster   = 0,
                            bslma::Allocator* allocator = 0);

    ~StorageMonitor() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    void resize(int numPartitions);

    void
    onStorageRegistered(int              partitionId,
                        const bmqt::Uri& uri,
                        const StorageSp& storageSp,
                        const mqbs::DataStoreConfigQueueInfo::AppInfos& apps)
        BSLS_KEYWORD_OVERRIDE;

    void onStorageRegistered(int                            partitionId,
                             const bmqt::Uri&               uri,
                             const StorageSp&               storageSp,
                             const mqbi::Storage::AppInfos& apps)
        BSLS_KEYWORD_OVERRIDE;

    void onStorageAppsAdded(int                            partitionId,
                            const bmqt::Uri&               uri,
                            const mqbi::Storage::AppInfos& apps)
        BSLS_KEYWORD_OVERRIDE;

    void
    onStorageAppRemoved(int                     partitionId,
                        const bmqt::Uri&        uri,
                        const mqbu::StorageKey& appKey) BSLS_KEYWORD_OVERRIDE;

    void onStorageUnregistered(int              partitionId,
                               const bmqt::Uri& uri) BSLS_KEYWORD_OVERRIDE;

    /// Clear all storages and tracked apps for the specified `partitionId`.
    /// For use during teardown (e.g. a manager's destructor releasing storage
    /// shared_ptrs before its `FileStore`s are destroyed), when the owning
    /// cluster may already be stopped/partially destroyed.
    ///
    /// Only for teardown: a queue co-owns the storage it was configured with,
    /// so dropping these while a queue is live leaves it on an object that
    /// whatever replaces them does not write to.  To empty a partition's
    /// storages while keeping the objects -- what a snapshot install needs --
    /// see `mqbs::FileStore::clearStorages`.
    void releaseStorages(int partitionId);

    void onRecovered(int partitionId) BSLS_KEYWORD_OVERRIDE;

    /// Return whether the queue having the specified `uri` and `queueKey` on
    /// the specified `partitionId` still needs registering with the specified
    /// `appIdKeyPairs` for the specified `domain`.  `e_REQUIRED` marks it
    /// outstanding, creating a placeholder if the storage does not exist yet.
    RegistrationState
    registerQueue(const bmqt::Uri&               uri,
                  const mqbu::StorageKey&        queueKey,
                  int                            partitionId,
                  const mqbi::Storage::AppInfos& appIdKeyPairs,
                  mqbi::Domain*                  domain);

    // ACCESSORS

    /// Return the number of partitions this monitor covers.  Every partition
    /// has an entry, empty if no queue is registered on it.
    int numPartitions() const;

    StorageSp find(const bmqt::Uri& uri) BSLS_KEYWORD_OVERRIDE;

    void loadAllStorages(bsl::vector<StorageSp>* result,
                         int partitionId) BSLS_KEYWORD_OVERRIDE;

    bool isStorageEmpty(const bmqt::Uri& uri,
                        int partitionId) const BSLS_KEYWORD_OVERRIDE;

    /// Return true if the queue having the specified `uri` and assigned to
    /// the specified `partitionId` has a registered storage.  Safe to call
    /// from any thread (in particular, the cluster dispatcher thread) --
    /// unlike querying `ReplicatedStorage`/`find(uri)`'s result directly.
    bool hasStorage(const bmqt::Uri& uri, int partitionId) const;

    /// Load into the specified `out` the set of appIds currently registered
    /// on the storage for the queue having the specified `uri` and assigned
    /// to the specified `partitionId`, and return true.  Return false
    /// (leaving `out` unchanged) if no storage for `uri` exists on
    /// `partitionId`.  Safe to call from any thread.
    bool loadAppIds(bsl::unordered_set<bsl::string>* out,
                    const bmqt::Uri&                 uri,
                    int                              partitionId) const;

    /// Return false: the legacy storage path.  `PartitionRaftManager`
    /// overrides this to return true.
    bool isRaft() const BSLS_KEYWORD_OVERRIDE;
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

// --------------------
// class StorageMonitor
// --------------------

// ACCESSORS
inline int StorageMonitor::numPartitions() const
{
    return static_cast<int>(d_storages.size());
}

}  // close package namespace
}  // close enterprise namespace

#endif
