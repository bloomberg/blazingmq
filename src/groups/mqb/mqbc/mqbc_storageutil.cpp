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

// mqbc_storageutil.cpp                                               -*-C++-*-
#include <mqbc_storageutil.h>

#include <mqbscm_version.h>
// MQB
#include <mqbi_queueengine.h>
#include <mqbnet_cluster.h>
#include <mqbs_datastore.h>
#include <mqbs_filestoreprotocol.h>
#include <mqbs_filestoreutil.h>
#include <mqbs_filesystemutil.h>
#include <mqbs_storagecollectionutil.h>
#include <mqbstat_clusterstats.h>
#include <mqbu_exit.h>

// BMQ
#include <bmqp_protocolutil.h>
#include <bmqp_recoverymessageiterator.h>
#include <bmqt_messageguid.h>

#include <bmqtsk_alarmlog.h>
#include <bmqu_memoutstream.h>
#include <bmqu_printutil.h>
#include <bmqu_throttledaction.h>

// BDE
#include <bdlb_print.h>
#include <bdlb_scopeexit.h>
#include <bdlb_stringrefutil.h>
#include <bdlf_bind.h>
#include <bdlf_memfn.h>
#include <bdlma_localsequentialallocator.h>
#include <bdls_filesystemutil.h>
#include <bdlt_currenttime.h>
#include <bdlt_epochutil.h>
#include <bsl_unordered_map.h>
#include <bslma_allocator.h>
#include <bslmt_lockguard.h>
#include <bslmt_mutex.h>
#include <bsls_annotation.h>
#include <bsls_types.h>

namespace BloombergLP {
namespace mqbc {

namespace {

/// Unary predicate used in certain `find` algorithms to match an element
/// having the specified AppId.
class AppIdMatcher {
  private:
    // TYPES
    typedef bsl::pair<bsl::string, mqbu::StorageKey> AppIdKeyPair;

    // DATA
    const bsl::string& d_expectedAppId;

  public:
    // CREATORS
    AppIdMatcher(const bsl::string& expectedAppId)
    : d_expectedAppId(expectedAppId)
    {
    }

    // ACCESSORS
    bool operator()(const AppIdKeyPair& appIdKeyPair) const
    {
        return d_expectedAppId == appIdKeyPair.first;
    }
};

/// Post on the optionally specified `semaphore`.
void optionalSemaphorePost(bslmt::Semaphore* semaphore)
{
    if (semaphore) {
        semaphore->post();
    }
}

}  // close unnamed namespace

// ------------------
// struct StorageUtil
// ------------------

// PRIVATE FUNCTIONS
bool StorageUtil::loadUpdatedAppIdKeyPairs(
    AppIdKeyPairs*                  addedAppIdKeyPairs,
    AppIdKeyPairs*                  removedAppIdKeyPairs,
    AppKeys*                        appKeys,
    bslmt::Mutex*                   appKeysLock,
    const mqbs::ReplicatedStorage&  storage,
    const AppIdKeyPairs&            newAppIdKeyPairs,
    const bsl::vector<bsl::string>& cfgAppIds,
    bool                            isCSLMode)
{
    // executed by the *CLUSTER DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(addedAppIdKeyPairs);
    BSLS_ASSERT_SAFE(removedAppIdKeyPairs);
    if (isCSLMode) {
        BSLS_ASSERT_SAFE(cfgAppIds.empty());
    }
    else {
        BSLS_ASSERT_SAFE(newAppIdKeyPairs.empty());
    }

    // This function is invoked by 'StorageManager::registerQueue' if the queue
    // with specified 'storage' is in fanout mode, in order to add or remove
    // any appIds which are not part of the currently configured appIds.
    // Here's the scenario: A broker is up and running and has a fanout queue
    // with 3 appIds: A, B and C.  Clients and broker shut down.  A new domain
    // config for the queue is deployed which removes appIds B and C, and adds
    // appId D.  So now, the effective appIds are A and D.  Now broker is
    // started, it recovers appIds A, B and C and creates 3 virtual storages,
    // one for each appId.  Then queue is opened and eventually,
    // 'registerQueue' is invoked.  At this time, broker needs to remove appIds
    // B and C, and add D.  This routine takes care of that, by retrieving the
    // list of newly added and removed appIds, and then invoking 'updateQueue'
    // in the appropriate thread.

    AppIdKeyPairs existingAppIdKeyPairs;
    storage.loadVirtualStorageDetails(&existingAppIdKeyPairs);

    if (isCSLMode) {
        loadAddedAndRemovedEntries(addedAppIdKeyPairs,
                                   removedAppIdKeyPairs,
                                   existingAppIdKeyPairs,
                                   newAppIdKeyPairs);

        if (addedAppIdKeyPairs->empty() && removedAppIdKeyPairs->empty()) {
            // No appIds to add or remove.
            return false;  // RETURN
        }
    }
    else {
        bsl::vector<bsl::string> existingAppIds;
        for (size_t i = 0; i < existingAppIdKeyPairs.size(); ++i) {
            existingAppIds.push_back(existingAppIdKeyPairs[i].first);
        }

        bsl::vector<bsl::string> addedAppIds;
        bsl::vector<bsl::string> removedAppIds;
        loadAddedAndRemovedEntries(&addedAppIds,
                                   &removedAppIds,
                                   existingAppIds,
                                   cfgAppIds);

        if (addedAppIds.empty() && removedAppIds.empty()) {
            // No appIds to add or remove.
            return false;  // RETURN
        }

        // Generate unique appKeys for the added appIds, and populate
        // 'addedAppIdKeyPairs'.
        for (size_t i = 0; i < addedAppIds.size(); ++i) {
            mqbu::StorageKey appKey = generateAppKey(appKeys,
                                                     appKeysLock,
                                                     addedAppIds[i]);
            addedAppIdKeyPairs->push_back(
                bsl::make_pair(addedAppIds[i], appKey));
        }

        // Populate 'removedAppIdKeyPairs'.
        for (size_t i = 0; i < removedAppIds.size(); ++i) {
            AppIdKeyPairsCIter it = bsl::find_if(
                existingAppIdKeyPairs.begin(),
                existingAppIdKeyPairs.end(),
                AppIdMatcher(removedAppIds[i]));
            BSLS_ASSERT_SAFE(it != existingAppIdKeyPairs.end());
            removedAppIdKeyPairs->push_back(
                bsl::make_pair(it->first, it->second));
        }
    }

    return true;
}

void StorageUtil::registerQueueDispatched(
    BSLS_ANNOTATION_UNUSED const mqbi::Dispatcher::ProcessorHandle& processor,
    mqbs::FileStore*                                                fs,
    mqbs::ReplicatedStorage*                                        storage,
    const bsl::string&   clusterDescription,
    int                  partitionId,
    const AppIdKeyPairs& appIdKeyPairs)
{
    // executed by *QUEUE_DISPATCHER* thread with the specified 'partitionId'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(fs->inDispatcherThread());
    BSLS_ASSERT_SAFE(fs);
    BSLS_ASSERT_SAFE(storage);
    BSLS_ASSERT_SAFE(0 <= partitionId);

    // Irrespective of the type of 'storage' (in-memory vs file-backed), the
    // appIds/VirtualStorages which were supposed to be added or removed
    // to/from 'storage' have already been added/removed (ie, virtual storages
    // have been created/removed).

    // Register storage with the partition.
    bsls::Types::Uint64 timestamp = bdlt::EpochUtil::convertToTimeT64(
        bdlt::CurrentTime::utc());
    mqbs::DataStoreRecordHandle handle;

    // TODO_CSL Do not write this record when we logically delete the QLIST
    // file
    int rc = fs->writeQueueCreationRecord(&handle,
                                          storage->queueUri(),
                                          storage->queueKey(),
                                          appIdKeyPairs,
                                          timestamp,
                                          true);  // Is new storage?
    if (0 != rc) {
        BMQTSK_ALARMLOG_ALARM("FILE_IO")
            << clusterDescription << ": Partition [" << partitionId
            << "] failed to write QueueCreationRecord for queue ["
            << storage->queueUri() << "] queueKey [" << storage->queueKey()
            << "], rc: " << rc << BMQTSK_ALARMLOG_END;
        return;  // RETURN
    }

    storage->addQueueOpRecordHandle(handle);
    fs->registerStorage(storage);

    // Flush the partition.  This routine ('registerQueue[Dispatched]') is
    // invoked only at the primary (we can assert that using
    // 'd_partitionInfoVec'), when a LocalQueue is being created.  If this
    // storage belongs to the first instance of LocalQueue mapped to this
    // partition, we want to make sure that queue creation record written to
    // the partition above is sent to the replicas as soon as possible.

    fs->dispatcherFlush(true, false);

    BALL_LOG_INFO << clusterDescription << ": Partition [" << partitionId
                  << "] registered [" << storage->queueUri() << "], queueKey ["
                  << storage->queueKey() << "] with the storage as primary.";
}

void StorageUtil::updateQueuePrimaryDispatched(
    BSLS_ANNOTATION_UNUSED const mqbi::Dispatcher::ProcessorHandle& processor,
    mqbs::ReplicatedStorage*                                        storage,
    bslmt::Mutex*        storagesLock,
    mqbs::FileStore*     fs,
    AppKeys*             appKeys,
    bslmt::Mutex*        appKeysLock,
    const bsl::string&   clusterDescription,
    int                  partitionId,
    const AppIdKeyPairs& addedIdKeyPairs,
    const AppIdKeyPairs& removedIdKeyPairs,
    bool                 isFanout,
    bool                 isCSLMode)
{
    // executed by *QUEUE_DISPATCHER* thread with the specified 'partitionId'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(0 <= partitionId);
    BSLS_ASSERT_SAFE(fs);
    BSLS_ASSERT_SAFE(fs->inDispatcherThread());
    BSLS_ASSERT_SAFE(storage);

    bslmt::LockGuard<bslmt::Mutex> guard(storagesLock);  // LOCK

    // Simply forward to 'updateQueuePrimaryRaw'.
    updateQueuePrimaryRaw(storage,
                          fs,
                          appKeys,
                          appKeysLock,
                          clusterDescription,
                          partitionId,
                          addedIdKeyPairs,
                          removedIdKeyPairs,
                          isFanout,
                          isCSLMode);
}

int StorageUtil::updateQueuePrimaryRaw(mqbs::ReplicatedStorage* storage,
                                       mqbs::FileStore*         fs,
                                       AppKeys*                 appKeys,
                                       bslmt::Mutex*            appKeysLock,
                                       const bsl::string&   clusterDescription,
                                       int                  partitionId,
                                       const AppIdKeyPairs& addedIdKeyPairs,
                                       const AppIdKeyPairs& removedIdKeyPairs,
                                       bool                 isFanout,
                                       bool                 isCSLMode)
{
    // executed by *QUEUE_DISPATCHER* thread with the specified 'partitionId'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(0 <= partitionId);
    BSLS_ASSERT_SAFE(fs);
    BSLS_ASSERT_SAFE(fs->inDispatcherThread());
    BSLS_ASSERT_SAFE(storage);
    BSLS_ASSERT_SAFE(appKeys);

    int                 rc        = 0;
    bsls::Types::Uint64 timestamp = bdlt::EpochUtil::convertToTimeT64(
        bdlt::CurrentTime::utc());

    if (!addedIdKeyPairs.empty()) {
        // Write QueueCreation record to data store for added appIds.
        //
        // TODO_CSL Do not write this record when we logically delete the QLIST
        // file

        mqbs::DataStoreRecordHandle handle;
        rc = fs->writeQueueCreationRecord(&handle,
                                          storage->queueUri(),
                                          storage->queueKey(),
                                          addedIdKeyPairs,
                                          timestamp,
                                          false);  // is new queue?
        if (0 != rc) {
            BMQTSK_ALARMLOG_ALARM("FILE_IO")
                << clusterDescription << ": Partition [" << partitionId
                << "] failed to write QueueCreationRecord for new appIds "
                << "for queue [" << storage->queueUri() << "] queueKey ["
                << storage->queueKey() << "], rc: " << rc
                << BMQTSK_ALARMLOG_END;
            return rc;  // RETURN
        }

        storage->addQueueOpRecordHandle(handle);

        rc = addVirtualStoragesInternal(storage,
                                        appKeys,
                                        appKeysLock,
                                        addedIdKeyPairs,
                                        clusterDescription,
                                        partitionId,
                                        isFanout,
                                        isCSLMode);
        if (0 != rc) {
            return rc;  // RETURN
        }

        BALL_LOG_INFO_BLOCK
        {
            bmqu::Printer<AppIdKeyPairs> printer(&addedIdKeyPairs);

            BALL_LOG_OUTPUT_STREAM
                << clusterDescription << ": Partition [" << partitionId
                << "] For an already registered queue [" << storage->queueUri()
                << "], queueKey [" << storage->queueKey() << "], added ["
                << addedIdKeyPairs.size() << "] new appId/appKey "
                << "pairs:" << printer;
        }
    }

    if (!removedIdKeyPairs.empty()) {
        for (AppIdKeyPairsCIter cit = removedIdKeyPairs.begin();
             cit != removedIdKeyPairs.end();
             ++cit) {
            // Write QueueDeletionRecord to data store for removed appIds.
            //
            // TODO_CSL Do not write this record when we logically delete the
            // QLIST file

            mqbs::DataStoreRecordHandle handle;
            rc = fs->writeQueueDeletionRecord(&handle,
                                              storage->queueKey(),
                                              cit->second,
                                              timestamp);
            if (0 != rc) {
                BMQTSK_ALARMLOG_ALARM("FILE_IO")
                    << clusterDescription << ": Partition [" << partitionId
                    << "] failed to write QueueDeletionRecord for queue ["
                    << storage->queueUri() << "], queueKey ["
                    << storage->queueKey() << "], appId [" << cit->first
                    << "], appKey [" << cit->second << "], rc: " << rc
                    << BMQTSK_ALARMLOG_END;
                return rc;  // RETURN
            }

            storage->addQueueOpRecordHandle(handle);

            rc = removeVirtualStorageInternal(storage,
                                              appKeys,
                                              appKeysLock,
                                              cit->second,
                                              partitionId);
            if (0 != rc) {
                BALL_LOG_ERROR
                    << clusterDescription << " Partition [" << partitionId
                    << "]: Failed to remove virtual storage for " << "appKey ["
                    << cit->second << "], appId [" << cit->first
                    << "], for queue [" << storage->queueUri()
                    << "], queueKey [" << storage->queueKey()
                    << "], rc: " << rc << ".";
                return rc;  // RETURN
            }
        }

        BALL_LOG_INFO_BLOCK
        {
            bmqu::Printer<AppIdKeyPairs> printer(&removedIdKeyPairs);

            BALL_LOG_OUTPUT_STREAM
                << clusterDescription << ": Partition [" << partitionId
                << "] For an already registered queue [" << storage->queueUri()
                << "], queueKey [" << storage->queueKey() << "], removed ["
                << removedIdKeyPairs.size() << "] existing appId/appKey "
                << "pairs:" << printer;
        }
    }

    // Flush the partition for records written above to reach replicas right
    // away.
    fs->dispatcherFlush(true, false);

    bmqu::Printer<AppIdKeyPairs> printer1(&addedIdKeyPairs);
    bmqu::Printer<AppIdKeyPairs> printer2(&removedIdKeyPairs);
    BALL_LOG_INFO << clusterDescription << ": Partition [" << partitionId
                  << "] updated [" << storage->queueUri() << "], queueKey ["
                  << storage->queueKey() << "] with the storage as primary: "
                  << "addedIdKeyPairs:" << printer1
                  << ", removedIdKeyPairs:" << printer2;

    return 0;
}

int StorageUtil::addVirtualStoragesInternal(
    mqbs::ReplicatedStorage* storage,
    AppKeys*                 appKeys,
    bslmt::Mutex*            appKeysLock,
    const AppIdKeyPairs&     appIdKeyPairs,
    const bsl::string&       clusterDescription,
    int                      partitionId,
    bool                     isFanout,
    bool                     isCSLMode)
{
    // executed by *QUEUE_DISPATCHER* thread with the specified 'partitionId'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(0 <= partitionId);
    BSLS_ASSERT_SAFE(storage);
    BSLS_ASSERT_SAFE(appKeys);

    enum {
        rc_SUCCESS                          = 0,
        rc_APP_KEY_COLLISION                = -1,
        rc_VIRTUAL_STORAGE_CREATION_FAILURE = -2
    };

    bslmt::LockGuard<bslmt::Mutex> appLockGuard(appKeysLock);  // LOCK

    int                rc = -1;
    bmqu::MemOutStream errorDesc;
    if (isFanout) {
        // Register appKeys with 'appKeys' and then with the underlying
        // physical 'storage'.

        for (AppIdKeyPairsCIter cit = appIdKeyPairs.begin();
             cit != appIdKeyPairs.end();
             ++cit) {
            AppKeysInsertRc irc = appKeys->insert(cit->second);
            if (!irc.second && isCSLMode) {
                BALL_LOG_WARN << clusterDescription << " Partition ["
                              << partitionId << "]: AppKey [" << cit->second
                              << "] already exists, while attempting to add "
                              << "appId [" << cit->first << "], for queue ["
                              << storage->queueUri() << "], queueKey ["
                              << storage->queueKey() << "].";

                return rc_APP_KEY_COLLISION;  // RETURN
            }

            if (0 != (rc = storage->addVirtualStorage(errorDesc,
                                                      cit->first,
                                                      cit->second))) {
                BALL_LOG_WARN
                    << clusterDescription << " Partition [" << partitionId
                    << "]: " << "Failed to add virtual storage for AppKey ["
                    << cit->second << "], appId [" << cit->first
                    << "], for queue [" << storage->queueUri()
                    << "], queueKey [" << storage->queueKey() << "]. Reason: ["
                    << errorDesc.str() << "], rc: " << rc << ".";

                return rc_VIRTUAL_STORAGE_CREATION_FAILURE;  // RETURN
            }
        }
    }
    else {
        rc = storage->addVirtualStorage(errorDesc,
                                        bmqp::ProtocolUtil::k_DEFAULT_APP_ID,
                                        mqbi::QueueEngine::k_DEFAULT_APP_KEY);
        // Unlike fanout queue above, we don't care about the returned value
        // for priority queue, since there is only one appId (default) which
        // could be added more than once in the startup sequence.  Its better
        // to ignore the return value instead of raising a useless warning.
    }

    return rc_SUCCESS;
}

int StorageUtil::removeVirtualStorageInternal(mqbs::ReplicatedStorage* storage,
                                              AppKeys*                 appKeys,
                                              bslmt::Mutex* appKeysLock,
                                              const mqbu::StorageKey& appKey,
                                              int partitionId)
{
    // executed by *QUEUE_DISPATCHER* thread with the specified 'partitionId'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(0 <= partitionId);
    BSLS_ASSERT_SAFE(storage);
    BSLS_ASSERT_SAFE(appKeys);
    BSLS_ASSERT_SAFE(!appKey.isNull());
    // We should never have to remove a default appKey (for non-fanout queues)
    BSLS_ASSERT_SAFE(appKey != mqbi::QueueEngine::k_DEFAULT_APP_KEY);

    enum {
        rc_SUCCESS                        = 0,
        rc_VIRTUAL_STORAGE_DOES_NOT_EXIST = -1,
        rc_APP_KEY_NOT_PRESENT            = -2
    };

    bslmt::LockGuard<bslmt::Mutex> appLockGuard(appKeysLock);  // LOCK

    bool existed = storage->removeVirtualStorage(appKey);
    if (!existed) {
        return rc_VIRTUAL_STORAGE_DOES_NOT_EXIST;  // RETURN
    }

    if (1 != appKeys->erase(appKey)) {
        // This appKey is not present in the global data structure of appKeys.
        // This really means that this node is out of sync with the cluster
        // state.

        return rc_APP_KEY_NOT_PRESENT;  // RETURN
    }

    return rc_SUCCESS;
}

void StorageUtil::getStoragesDispatched(StorageLists*         storageLists,
                                        bslmt::Latch*         latch,
                                        const FileStores&     fileStores,
                                        int                   partitionId,
                                        const StorageFilters& filters)
{
    // executed by *QUEUE_DISPATCHER* thread with the specified 'partitionId'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(storageLists);
    BSLS_ASSERT_SAFE(latch);
    BSLS_ASSERT_SAFE(0 <= partitionId);
    BSLS_ASSERT_SAFE(fileStores.size() >
                     static_cast<unsigned int>(partitionId));
    BSLS_ASSERT_SAFE(storageLists->size() == fileStores.size());

    const mqbs::FileStore& fs =
        *fileStores[static_cast<unsigned int>(partitionId)];
    BSLS_ASSERT_SAFE(fs.inDispatcherThread());

    fs.getStorages(&((*storageLists)[partitionId]), filters);

    latch->arrive();
}

void StorageUtil::loadStorages(bsl::vector<mqbcmd::StorageQueueInfo>* storages,
                               const bsl::string& domainName,
                               const FileStores&  fileStores)
{
    // executed by cluster *DISPATCHER* thread
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(storages);

    StorageLists   storageLists;
    StorageFilters filters;
    filters.push_back(
        mqbs::StorageCollectionUtilFilterFactory::byDomain(domainName));
    filters.push_back(
        mqbs::StorageCollectionUtilFilterFactory::byMessageCount(1));

    storageLists.resize(fileStores.size());
    executeForEachPartitions(
        bdlf::BindUtil::bind(&getStoragesDispatched,
                             &storageLists,
                             bdlf::PlaceHolders::_2,  // latch
                             fileStores,
                             bdlf::PlaceHolders::_1,  // partitionId
                             filters),
        fileStores);

    // Merge vector of vectors into a single vector
    StorageList storageList;
    for (StorageListsConstIter cit = storageLists.cbegin();
         cit != storageLists.cend();
         ++cit) {
        bsl::copy(cit->cbegin(), cit->cend(), bsl::back_inserter(storageList));
    }

    mqbs::StorageCollectionUtil::sortStorages(
        &storageList,
        mqbs::StorageCollectionUtilSortMetric::e_BYTE_COUNT);

    storages->reserve(storageList.size());
    for (StorageList::const_iterator cit = storageList.begin();
         cit != storageList.end();
         ++cit) {
        storages->resize(storages->size() + 1);
        mqbcmd::StorageQueueInfo& storage = storages->back();

        bmqu::MemOutStream os;
        os << (*cit)->queueKey();
        storage.queueKey()    = os.str();
        storage.partitionId() = (*cit)->partitionId();
        storage.numMessages() = (*cit)->numMessages(
            mqbu::StorageKey::k_NULL_KEY);
        storage.numBytes() = (*cit)->numBytes(mqbu::StorageKey::k_NULL_KEY);
        storage.queueUri() = (*cit)->queueUri().asString();
        storage.isPersistent() = (*cit)->isPersistent();
    }
}

void StorageUtil::loadPartitionStorageSummary(
    mqbcmd::StorageResult*   result,
    FileStores*              fileStores,
    int                      partitionId,
    const bslstl::StringRef& partitionLocation)
{
    // executed by cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(result);
    BSLS_ASSERT_SAFE(fileStores);

    mqbcmd::ClusterStorageSummary& summary =
        result->makeClusterStorageSummary();
    summary.clusterFileStoreLocation() = partitionLocation;
    summary.fileStores().resize(fileStores->size());

    bslmt::Latch latch(1);
    fileStores->at(partitionId)
        ->execute(bdlf::BindUtil::bind(&loadStorageSummaryDispatched,
                                       &summary,
                                       &latch,
                                       partitionId,
                                       *fileStores));
    // Wait
    latch.wait();

    // As we loaded information about only one partition (with 'partitionId'),
    // the 'summary.fileStores()' in general contains incomplete information
    // about all the partitions.  So we make sure that only meaningful
    // information will stay in the end.
    bsl::swap(summary.fileStores()[0], summary.fileStores()[partitionId]);
    summary.fileStores().resize(1);
}

void StorageUtil::loadStorageSummary(mqbcmd::StorageResult*  result,
                                     const FileStores&       fileStores,
                                     const bslstl::StringRef location)
{
    // executed by cluster *DISPATCHER* thread

    // This command needs to forward the 'SUMMARY' command to all partitions,
    // wait for all of them to finish executing it, and then aggregate the
    // output.

    mqbcmd::ClusterStorageSummary& summary =
        result->makeClusterStorageSummary();

    summary.clusterFileStoreLocation() = location;
    summary.fileStores().resize(fileStores.size());

    executeForEachPartitions(
        bdlf::BindUtil::bind(&loadStorageSummaryDispatched,
                             &summary,
                             bdlf::PlaceHolders::_2,  // latch
                             bdlf::PlaceHolders::_1,  // partitionId
                             fileStores),
        fileStores);
}

void StorageUtil::loadStorageSummaryDispatched(
    mqbcmd::ClusterStorageSummary* summary,
    bslmt::Latch*                  latch,
    int                            partitionId,
    const FileStores&              fileStores)
{
    // executed by *QUEUE_DISPATCHER* thread with the specified 'partitionId'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(summary);
    BSLS_ASSERT_SAFE(latch);
    BSLS_ASSERT_SAFE(0 <= partitionId);
    BSLS_ASSERT_SAFE(fileStores.size() >
                     static_cast<unsigned int>(partitionId));
    BSLS_ASSERT_SAFE(fileStores[partitionId]->inDispatcherThread());

    fileStores[partitionId]->loadSummary(&summary->fileStores()[partitionId]);

    latch->arrive();
}

void StorageUtil::executeForEachPartitions(const PerPartitionFunctor& job,
                                           const FileStores& fileStores)
{
    // executed by cluster *DISPATCHER* thread

    bslmt::Latch latch(fileStores.size());

    for (unsigned int i = 0; i < fileStores.size(); ++i) {
        fileStores[i]->execute(bdlf::BindUtil::bind(job, i, &latch));
    }

    // Wait
    latch.wait();
}

int StorageUtil::processReplicationCommand(
    mqbcmd::ReplicationResult*        replicationResult,
    int*                              replicationFactor,
    FileStores*                       fileStores,
    const mqbcmd::ReplicationCommand& command)
{
    // executed by cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(replicationResult);
    BSLS_ASSERT_SAFE(replicationFactor);
    BSLS_ASSERT_SAFE(fileStores);

    if (command.isSetTunableValue()) {
        const mqbcmd::SetTunable& tunable = command.setTunable();
        if (bdlb::StringRefUtil::areEqualCaseless(tunable.name(), "QUORUM")) {
            if (!tunable.value().isTheIntegerValue() ||
                tunable.value().theInteger() < 0) {
                bmqu::MemOutStream output;
                output << "The QUORUM tunable must be a non-negative integer, "
                          "but instead the following was specified: "
                       << tunable.value();
                replicationResult->makeError();
                replicationResult->error().message() = output.str();
                return -1;  // RETURN
            }

            mqbcmd::TunableConfirmation& tunableConfirmation =
                replicationResult->makeTunableConfirmation();
            tunableConfirmation.name() = "Quorum";
            tunableConfirmation.oldValue().makeTheInteger(*replicationFactor);
            *replicationFactor = tunable.value().theInteger();

            for (FileStores::iterator it = fileStores->begin();
                 it != fileStores->end();
                 ++it) {
                (*it)->execute(bdlf::BindUtil::bind(
                    &mqbs::FileStore::setReplicationFactor,
                    *it,
                    tunable.value().theInteger()));  // partitionId
            }
            tunableConfirmation.newValue().makeTheInteger(*replicationFactor);
            return 0;  // RETURN
        }

        bmqu::MemOutStream output;
        output << "Unknown tunable name '" << tunable.name() << "'";
        replicationResult->makeError();
        replicationResult->error().message() = output.str();
        return -1;  // RETURN
    }
    else if (command.isGetTunableValue()) {
        const bsl::string& tunable = command.getTunable().name();
        if (bdlb::StringRefUtil::areEqualCaseless(tunable, "QUORUM")) {
            mqbcmd::Tunable& tunableObj = replicationResult->makeTunable();
            tunableObj.name()           = "Quorum";
            tunableObj.value().makeTheInteger(*replicationFactor);
            return 0;  // RETURN
        }

        bmqu::MemOutStream output;
        output << "Unsupported tunable '" << tunable << "': Issue the "
               << "LIST_TUNABLES command for the list of supported tunables.";
        replicationResult->makeError();
        replicationResult->error().message() = output.str();
        return -1;  // RETURN
    }
    else if (command.isListTunablesValue()) {
        mqbcmd::Tunables& tunables = replicationResult->makeTunables();
        tunables.tunables().resize(tunables.tunables().size() + 1);
        mqbcmd::Tunable& tunable = tunables.tunables().back();
        tunable.name()           = "QUORUM";
        tunable.value().makeTheInteger(*replicationFactor);
        tunable.description() = "non-negative integer count of the number of"
                                " peers required to persist each message";
        return 0;  // RETURN
    }

    bmqu::MemOutStream output;
    output << "Unknown command '" << command << "'";
    replicationResult->makeError();
    replicationResult->error().message() = output.str();
    return -1;
}

// FUNCTIONS
bool StorageUtil::isStorageEmpty(bslmt::Mutex*       storagesLock,
                                 const StorageSpMap& storageMap,
                                 const bmqt::Uri&    uri,
                                 int                 partitionId)
{
    // executed by the *CLUSTER DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(uri.isValid());
    BSLS_ASSERT_SAFE(0 <= partitionId);

    bslmt::LockGuard<bslmt::Mutex> guard(storagesLock);  // LOCK

    StorageSpMapConstIter cit = storageMap.find(uri);
    if (cit == storageMap.end()) {
        return true;  // RETURN
    }

    BSLS_ASSERT_SAFE(cit->second);

    return cit->second->isEmpty();
}

void StorageUtil::storageMonitorCb(
    bool*                          lowDiskspaceWarning,
    const bsls::AtomicBool*        isManagerStarted,
    bsls::Types::Uint64            minimumRequiredDiskSpace,
    const bslstl::StringRef&       clusterDescription,
    const mqbcfg::PartitionConfig& partitionConfig)
{
    // executed by the scheduler's *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(lowDiskspaceWarning);
    BSLS_ASSERT_SAFE(isManagerStarted);

    if (!*isManagerStarted) {
        return;  // RETURN
    }

    // Delete archived files.

    mqbs::FileStoreUtil::deleteArchiveFiles(partitionConfig,
                                            clusterDescription);

    // Check available diskspace.

    bsls::Types::Int64 availableSpace = 0;
    bsls::Types::Int64 totalSpace     = 0;
    bmqu::MemOutStream errorDesc;
    const bsl::string& clusterFileStoreLocation = partitionConfig.location();
    int                rc = mqbs::FileSystemUtil::loadFileSystemSpace(
        errorDesc,
        &availableSpace,
        &totalSpace,
        clusterFileStoreLocation.c_str());
    if (0 != rc) {
        BMQTSK_ALARMLOG_ALARM("FILE_IO")
            << clusterDescription
            << ": Failed to retrieve available space on file system where "
            << "storage files reside: [" << clusterFileStoreLocation
            << "]. This is not fatal but broker may not be able to handle "
            << "disk-space issues gracefully. Reason: " << errorDesc.str()
            << ", rc: " << rc << "." << BMQTSK_ALARMLOG_END;
        return;  // RETURN
    }

    if (static_cast<bsls::Types::Uint64>(availableSpace) <
        minimumRequiredDiskSpace) {
        *lowDiskspaceWarning = true;

        BMQU_THROTTLEDACTION_THROTTLE(
            bmqu::ThrottledActionParams(5 * 1000 * 60, 1),
            // 1 log per 5min interval
            BALL_LOG_INFO << "[INSUFFICIENT_DISK_SPACE] " << clusterDescription
                          << ": Not enough disk space on file system ["
                          << clusterFileStoreLocation << "]. Required: "
                          << bmqu::PrintUtil::prettyBytes(
                                 minimumRequiredDiskSpace)
                          << ", available: "
                          << bmqu::PrintUtil::prettyBytes(availableSpace););
    }
    else {
        if (*lowDiskspaceWarning) {
            // Print trace displaying disk space.
            BALL_LOG_INFO << clusterDescription
                          << ": Disk space on file system ["
                          << clusterFileStoreLocation
                          << "] has gone back to normal. " << "Required: "
                          << bmqu::PrintUtil::prettyBytes(
                                 minimumRequiredDiskSpace)
                          << ", available: "
                          << bmqu::PrintUtil::prettyBytes(availableSpace);
        }

        *lowDiskspaceWarning = false;
    }
}

bsl::ostream&
StorageUtil::printRecoveryPhaseOneBanner(bsl::ostream&      out,
                                         const bsl::string& clusterDescription,
                                         int                partitionId)
{
    const int level          = 0;
    const int spacesPerLevel = 4;

    bmqu::MemOutStream header;
    header << "RECOVERY PHASE 1: " << clusterDescription << " Partition ["
           << partitionId << "]";

    bdlb::Print::newlineAndIndent(out, level + 1, spacesPerLevel);
    out << bsl::string(header.length(), '-');
    bdlb::Print::newlineAndIndent(out, level + 1, spacesPerLevel);
    out << header.str();
    bdlb::Print::newlineAndIndent(out, level + 1, spacesPerLevel);
    out << bsl::string(header.length(), '-');

    return out;
}

int StorageUtil::validatePartitionDirectory(
    const mqbcfg::PartitionConfig& config,
    bsl::ostream&                  errorDescription)
{
    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS                        = 0,
        rc_PARTITION_LOCATION_NONEXISTENT = -1
    };

    // Ensure partition directory exist
    const bsl::string& clusterFileStoreLocation = config.location();

    if (!bdls::FilesystemUtil::isDirectory(clusterFileStoreLocation, true)) {
        errorDescription << "Cluster's partition location ('"
                         << clusterFileStoreLocation << "') doesn't exist !";
        return rc_PARTITION_LOCATION_NONEXISTENT;  // RETURN
    }
    // Ensure partition's archive directory exist
    const bsl::string& clusterFileStoreArchiveLocation =
        config.archiveLocation();

    if (!bdls::FilesystemUtil::isDirectory(clusterFileStoreArchiveLocation,
                                           true)) {
        errorDescription << "Cluster's archive partition location ('"
                         << clusterFileStoreArchiveLocation
                         << "') doesn't exist !";
        return rc_PARTITION_LOCATION_NONEXISTENT;  // RETURN
    }

    return rc_SUCCESS;
}

int StorageUtil::validateDiskSpace(const mqbcfg::PartitionConfig& config,
                                   const mqbc::ClusterData&       clusterData,
                                   const bsls::Types::Uint64&     minDiskSpace)
{
    // executed by the *CLUSTER DISPATCHER* thread

    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS               = 0,
        rc_NOT_ENOUGH_DISK_SPACE = -1
    };

    // Print file-system names for cluster storage location.
    const bsl::string& clusterFileStoreLocation = config.location();
    bsl::string        fsname;
    mqbs::FileSystemUtil::loadFileSystemName(&fsname,
                                             clusterFileStoreLocation.c_str());

    BALL_LOG_INFO << clusterData.identity().description()
                  << ": file system type for cluster's storage location ["
                  << clusterFileStoreLocation << "] is [" << fsname << "].";

    // Raise low disk space warning, if applicable.
    bsls::Types::Int64 availableSpace = 0;
    bsls::Types::Int64 totalSpace     = 0;
    bmqu::MemOutStream errorDesc;
    int                rc = mqbs::FileSystemUtil::loadFileSystemSpace(
        errorDesc,
        &availableSpace,
        &totalSpace,
        clusterFileStoreLocation.c_str());

    if (0 != rc) {
        BALL_LOG_WARN << "Failed to retrieve total and available space on "
                      << "file system where storage files for cluster ["
                      << clusterData.identity().name() << "] reside: ["
                      << clusterFileStoreLocation << "], rc: " << rc
                      << ", reason: " << errorDesc.str()
                      << ". This is not a fatal issue but broker may not be "
                      << "able to handle disk-space issues gracefully.";
    }
    else {
        BALL_LOG_INFO << clusterData.identity().description()
                      << ": file system for cluster's storage location ["
                      << clusterFileStoreLocation << "] has total space: "
                      << bmqu::PrintUtil::prettyBytes(totalSpace)
                      << ", and available space: "
                      << bmqu::PrintUtil::prettyBytes(availableSpace) << ".";

        // Available space on disk must be at least 2 times the minimum space
        // required.
        if (static_cast<bsls::Types::Uint64>(availableSpace) < minDiskSpace) {
            BALL_LOG_INFO
                << "[INSUFFICIENT_DISK_SPACE] "
                << clusterData.identity().description()
                << ":(Not enough disk space on file system where storage files"
                << " reside [" << clusterFileStoreLocation << "]. Required: "
                << bmqu::PrintUtil::prettyBytes(minDiskSpace)
                << ", available: "
                << bmqu::PrintUtil::prettyBytes(availableSpace);

            // TBD: Should return error but most dev and some prod boxes are
            //      low on disk space.  So just raise an alarm for now and move
            //      on.

            // return rc_NOT_ENOUGH_DISK_SPACE;                       // RETURN
        }
    }
    return rc_SUCCESS;
}

template <>
unsigned int StorageUtil::extractPartitionId<true>(const bmqp::Event& event)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(event.isRecoveryEvent());

    bmqp::RecoveryMessageIterator iter;
    event.loadRecoveryMessageIterator(&iter);
    BSLS_ASSERT_SAFE(iter.isValid());

    const int rc = iter.next();
    if (rc != 1) {
        return k_INVALID_PARTITION_ID;  // RETURN
    }

    return iter.header().partitionId();
}

bool StorageUtil::validateStorageEvent(
    const bmqp::Event&                 event,
    int                                partitionId,
    const mqbnet::ClusterNode*         source,
    const mqbnet::ClusterNode*         primary,
    bmqp_ctrlmsg::PrimaryStatus::Value status,
    const bsl::string&                 clusterDescription,
    bool                               skipAlarm,
    bool                               isFSMWorkflow)
{
    // executed by *QUEUE_DISPATCHER* thread associated with 'partitionId' or
    // by the *CLUSTER DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(event.isStorageEvent());
    BSLS_ASSERT_SAFE(source);

    // Ensure that this node still perceives 'source' node as the primary of
    // 'partitionId'.
    if (0 == primary) {
        if (skipAlarm) {
            return false;  // RETURN
        }

        BMQTSK_ALARMLOG_ALARM("STORAGE")
            << clusterDescription << ": Received storage "
            << "event from node " << source->nodeDescription() << " for "
            << "Partition [" << partitionId << "] which has no primary as "
            << "perceived by this node. Ignoring entire storage event."
            << BMQTSK_ALARMLOG_END;
        return false;  // RETURN
    }

    if (source != primary) {
        if (skipAlarm) {
            return false;  // RETURN
        }

        BMQTSK_ALARMLOG_ALARM("STORAGE")
            << clusterDescription << ": Received storage "
            << "event from node " << source->nodeDescription() << " for "
            << "Partition [" << partitionId << "] which has different "
            << "primary as perceived by this node: "
            << primary->nodeDescription() << " Ignoring entire "
            << "storage event." << BMQTSK_ALARMLOG_END;
        return false;  // RETURN
    }

    // Ensure that primary is perceived as active.
    if (!isFSMWorkflow && bmqp_ctrlmsg::PrimaryStatus::E_ACTIVE != status) {
        if (skipAlarm) {
            return false;  // RETURN
        }

        BMQTSK_ALARMLOG_ALARM("STORAGE")
            << clusterDescription << ": Received storage "
            << "event for Partition [" << partitionId
            << "] from: " << source->nodeDescription()
            << ", which is perceived as "
            << "non-active primary. Primary status: " << status
            << ". Ignoring entire storage event." << BMQTSK_ALARMLOG_END;
        return false;  // RETURN
    }

    return true;
}

bool StorageUtil::validatePartitionSyncEvent(
    const bmqp::Event&         event,
    int                        partitionId,
    const mqbnet::ClusterNode* source,
    const PartitionInfo&       partitionInfo,
    const mqbc::ClusterData&   clusterData,
    bool                       isFSMWorkflow)
{
    // executed by *QUEUE_DISPATCHER* thread associated with 'partitionId' or
    // by the *CLUSTER DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(event.isPartitionSyncEvent());
    BSLS_ASSERT_SAFE(source);

    // Check that either self is primary or 'source' is perceived as primary
    // for the partition.
    if (partitionInfo.primary() != clusterData.membership().selfNode() &&
        partitionInfo.primary() != source) {
        BALL_LOG_ERROR << clusterData.identity().description()
                       << " Partition [" << partitionId
                       << "]: Received partition-sync event from peer: "
                       << source->nodeDescription()
                       << " but neither self nor peer is primary. Perceived"
                       << " primary: "
                       << (partitionInfo.primary()
                               ? partitionInfo.primary()->nodeDescription()
                               : "** none **");
        return false;  // RETURN
    }

    if (!isFSMWorkflow && bmqp_ctrlmsg::PrimaryStatus::E_PASSIVE !=
                              partitionInfo.primaryStatus()) {
        // Either self or source is primary.  Whichever is the primary, it must
        // be perceived as a passive one.

        BALL_LOG_ERROR << clusterData.identity().description()
                       << " Partition [" << partitionId
                       << "]: Received partition-sync event from: "
                       << source->nodeDescription()
                       << " but primary status is: "
                       << partitionInfo.primaryStatus()
                       << ", perceived primary: "
                       << (partitionInfo.primary()
                               ? partitionInfo.primary()->nodeDescription()
                               : "** none **");
        return false;  // RETURN
    }

    // Note that wire format of a bmqp::EventType::e_PARTITION_SYNC event is
    // *exactly* same as that of bmqp::EventType::e_STORAGE event.  Only
    // difference is the eventType, which is part of bmqp::EventHeader.
    bmqp::StorageMessageIterator iter;
    event.loadStorageMessageIterator(&iter);
    BSLS_ASSERT_SAFE(iter.isValid());

    // Iterate over each message and validate a few things.
    while (1 == iter.next()) {
        const bmqp::StorageHeader& header = iter.header();
        if (static_cast<unsigned int>(partitionId) != header.partitionId()) {
            // As per the BlazingMQ partition-sync algorithm, *all* messages in
            // this event will belong to the *same* partition.  Any exception
            // to this is a bug in the implementation, and thus, if it occurs,
            // we reject the *entire* partition-sync event.

            BMQTSK_ALARMLOG_ALARM("STORAGE")
                << clusterData.identity().description()
                << ": Received partition-sync event from node with "
                << source->nodeDescription()
                << " with different partitionIds: " << partitionId << "vs "
                << header.partitionId() << ". Ignoring this entire event."
                << BMQTSK_ALARMLOG_END;
            return false;  // RETURN
        }
    }

    return true;
}

int StorageUtil::assignPartitionDispatcherThreads(
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
    const bdlb::NullableValue<QueueCreationCb>& queueCreationCb,
    const bdlb::NullableValue<QueueDeletionCb>& queueDeletionCb)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher->inDispatcherThread(&cluster));

    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS                   = 0,
        rc_THREAD_POOL_START_FAILURE = -1
    };

    // Start the misc work thread pool
    int rc = threadPool->start();
    if (rc != 0) {
        errorDescription << clusterData->identity().description()
                         << ": Failed to start miscellaneous worker thread "
                         << "pool.";
        return rc_THREAD_POOL_START_FAILURE;  // RETURN
    }

    // Assign a queue dispatcher thread to each partition in round-robin
    // manner.
    const int numProcessors = dispatcher->numProcessors(
        mqbi::DispatcherClientType::e_QUEUE);
    BSLS_ASSERT_SAFE(numProcessors > 0);

    for (int i = 0; i < config.numPartitions(); ++i) {
        int                   processorId = i % numProcessors;
        mqbs::DataStoreConfig dsCfg;
        dsCfg.setScheduler(&clusterData->scheduler())
            .setBufferFactory(&clusterData->bufferFactory())
            .setPreallocate(config.preallocate())
            .setPrefaultPages(config.prefaultPages())
            .setLocation(config.location())
            .setArchiveLocation(config.archiveLocation())
            .setNodeId(clusterData->membership().selfNode()->nodeId())
            .setPartitionId(i)
            .setMaxDataFileSize(config.maxDataFileSize())
            .setMaxJournalFileSize(config.maxJournalFileSize())
            .setMaxQlistFileSize(config.maxQlistFileSize())
            .setMaxArchivedFileSets(config.maxArchivedFileSets())
            .setRecoveredQueuesCb(recoveredQueuesCb);

        if (!queueCreationCb.isNull()) {
            dsCfg.setQueueCreationCb(queueCreationCb.value());
        }

        if (!queueDeletionCb.isNull()) {
            dsCfg.setQueueDeletionCb(queueDeletionCb.value());
        }

        // Get named allocator from associated bmqma::CountingAllocatorStore
        bslma::Allocator* fileStoreAllocator = allocators->get(
            bsl::string("Partition") + bsl::to_string(i));

        bsl::shared_ptr<mqbs::FileStore> fsSp(
            new (*fileStoreAllocator)
                mqbs::FileStore(dsCfg,
                                processorId,
                                dispatcher,
                                clusterData->membership().netCluster(),
                                &clusterData->stats(),
                                blobSpPool,
                                &clusterData->stateSpPool(),
                                threadPool,
                                cluster.isCSLModeEnabled(),
                                cluster.isFSMWorkflow(),
                                replicationFactor,
                                fileStoreAllocator),
            fileStoreAllocator);

        (*fileStores)[i] = fsSp;
    }

    return rc_SUCCESS;
}

void StorageUtil::clearPrimaryForPartition(
    mqbs::FileStore*   fs,
    PartitionInfo*     partitionInfo,
    const bsl::string& clusterDescription,

    int                  partitionId,
    mqbnet::ClusterNode* primary)
{
    // executed by *QUEUE_DISPATCHER* thread associated with 'partitionId'

    // PRECONDITION
    BSLS_ASSERT_SAFE(fs && fs->inDispatcherThread());
    BSLS_ASSERT_SAFE(partitionInfo);
    BSLS_ASSERT_SAFE(0 <= partitionId);

    if (0 == partitionInfo->primary()) {
        // Already notified.

        return;  // RETURN
    }

    if (primary != partitionInfo->primary()) {
        return;  // RETURN
    }

    BALL_LOG_INFO << clusterDescription << " Partition [" << partitionId
                  << "]: processing 'clear-primary' event. Current primary: "
                  << partitionInfo->primary()->nodeDescription()
                  << ", current leaseId: " << partitionInfo->primaryLeaseId()
                  << ", current primary status: "
                  << partitionInfo->primaryStatus() << ".";

    partitionInfo->setPrimary(0);
    partitionInfo->setPrimaryStatus(bmqp_ctrlmsg::PrimaryStatus::E_PASSIVE);
    fs->clearPrimary();
}

bsls::Types::Uint64
StorageUtil::findMinReqDiskSpace(const mqbcfg::PartitionConfig& config)
{
    bsls::Types::Uint64 partitionSize = config.maxDataFileSize() +
                                        config.maxJournalFileSize() +
                                        config.maxQlistFileSize();

    bsls::Types::Uint64 minimumRequiredDiskSpace = partitionSize *
                                                   config.numPartitions();

    // Make it 2.5 times since during roll over, we might be close to 2 times
    // the size.
    minimumRequiredDiskSpace = 2 * minimumRequiredDiskSpace +
                               (minimumRequiredDiskSpace / 2);

    return minimumRequiredDiskSpace;
}

void StorageUtil::transitionToActivePrimary(PartitionInfo*     partitionInfo,
                                            mqbc::ClusterData* clusterData,
                                            int                partitionId)
{
    // executed by *QUEUE_DISPATCHER* thread associated with 'partitionId'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(partitionInfo);
    BSLS_ASSERT_SAFE(clusterData);

    partitionInfo->setPrimaryStatus(bmqp_ctrlmsg::PrimaryStatus::E_ACTIVE);

    bmqp_ctrlmsg::ControlMessage         controlMsg;
    bmqp_ctrlmsg::PrimaryStatusAdvisory& primaryAdv =
        controlMsg.choice()
            .makeClusterMessage()
            .choice()
            .makePrimaryStatusAdvisory();
    primaryAdv.partitionId()    = partitionId;
    primaryAdv.primaryLeaseId() = partitionInfo->primaryLeaseId();
    primaryAdv.status()         = bmqp_ctrlmsg::PrimaryStatus::E_ACTIVE;

    clusterData->messageTransmitter().broadcastMessageSafe(controlMsg);
}

void StorageUtil::onPartitionPrimarySync(
    mqbs::FileStore*                fs,
    PartitionInfo*                  pinfo,
    mqbc::ClusterData*              clusterData,
    const PartitionPrimaryStatusCb& partitionPrimaryStatusCb,
    int                             partitionId,
    int                             status)
{
    // executed by *QUEUE_DISPATCHER* thread associated with 'partitionId'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(fs && fs->inDispatcherThread());
    BSLS_ASSERT_SAFE(fs->isOpen());
    BSLS_ASSERT_SAFE(pinfo);
    BSLS_ASSERT_SAFE(clusterData);
    BSLS_ASSERT_SAFE(0 <= partitionId);

    if (clusterData->cluster().isStopping()) {
        BALL_LOG_WARN << clusterData->identity().description()
                      << ": Cluster is stopping; skipping partition primary "
                      << "sync notification.";
        return;  // RETURN
    }

    if (pinfo->primary() != clusterData->membership().selfNode()) {
        // Looks like a new primary was assigned while self node was performing
        // partition-primary-sync (after being chosen as the primary).  This
        // could occur if this node failed to transition to active primary in
        // the stipulated time, and the leader force-chose a new primary.  In
        // such scenario, this node should initiate recovery or partition-sync
        // with the new primary, *if* the new primary has transitioned to
        // active.  If new node hasn't yet transitioned to active primary, this
        // node should wait for that.  This should probably be checked in
        // StorageMgr.processStorageEvent(). Currently all of this is not
        // handled.

        BMQTSK_ALARMLOG_ALARM("STORAGE")
            << clusterData->identity().description() << " Partition ["
            << partitionId << "]: new primary ("
            << pinfo->primary()->nodeDescription() << ") with leaseId "
            << pinfo->primaryLeaseId()
            << " chosen while self node was undergoing partition-primary sync."
            << " This scenario is not handled currently."
            << BMQTSK_ALARMLOG_END;

        // No need to inform via 'partitionPrimaryStatusCb' though, since this
        // node is now an old primary.
        return;  // RETURN
    }

    if (0 != status) {
        BMQTSK_ALARMLOG_ALARM("STORAGE")
            << clusterData->identity().description() << " Partition ["
            << partitionId << "]: node failed to sync "
            << "after being chosen as primary, with status: " << status
            << BMQTSK_ALARMLOG_END;

        partitionPrimaryStatusCb(partitionId, status, pinfo->primaryLeaseId());
        return;  // RETURN

        // TBD: the leader should have a timeout period for a node to declare
        //      itself as active primary, once it's chosen as primary for a
        //      partition.  If chosen node doesn't transition to active primary
        //      during that time, leader should choose a new node as primary.
    }

    // Broadcast self as active primary of this partition.  This must be done
    // before invoking 'FileStore::setActivePrimary'.
    transitionToActivePrimary(pinfo, clusterData, partitionId);

    partitionPrimaryStatusCb(partitionId, status, pinfo->primaryLeaseId());

    // Safe to inform partition now.  Note that partition will issue a sync
    // point with old leaseId (if applicable) and another with new leaseId
    // immediately.
    fs->setActivePrimary(pinfo->primary(), pinfo->primaryLeaseId());
}

void StorageUtil::recoveredQueuesCb(
    StorageSpMap*                storageMap,
    bslmt::Mutex*                storagesLock,
    mqbs::FileStore*             fs,
    AppKeys*                     appKeys,
    bslmt::Mutex*                appKeysLock,
    mqbi::DomainFactory*         domainFactory,
    bslmt::Mutex*                unrecognizedDomainsLock,
    DomainQueueMessagesCountMap* unrecognizedDomains,
    const bsl::string&           clusterDescription,
    int                          partitionId,
    const QueueKeyInfoMap&       queueKeyInfoMap,
    bool                         isCSLMode)
{
    // executed by *QUEUE_DISPATCHER* thread associated with 'partitionId'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(storageMap);
    BSLS_ASSERT_SAFE(fs);
    BSLS_ASSERT_SAFE(appKeys);
    BSLS_ASSERT_SAFE(unrecognizedDomainsLock);
    BSLS_ASSERT_SAFE(unrecognizedDomains && unrecognizedDomains->empty());
    BSLS_ASSERT_SAFE(0 <= partitionId);
    BSLS_ASSERT_SAFE(fs->inDispatcherThread());

    BALL_LOG_INFO << clusterDescription << " Partition [" << partitionId
                  << "]: "
                  << "Recovered [" << queueKeyInfoMap.size() << "] queues";

    if (domainFactory == 0) {
        BALL_LOG_ERROR << clusterDescription << " Partition [" << partitionId
                       << "]: "
                       << "Aborting 'recoveredQueuesCb' due to missing Domain "
                       << "Manager";

        return;  // RETURN
    }

    bslmt::LockGuard<bslmt::Mutex> guard(storagesLock);  // LOCK

    // Create scratch data structures.

    DomainMap domainMap;

    typedef bsl::unordered_map<mqbu::StorageKey, mqbs::ReplicatedStorage*>
                                         QueueKeyStorageMap;
    typedef QueueKeyStorageMap::iterator QueueKeyStorageMapIter;
    QueueKeyStorageMap                   queueKeyStorageMap;

    // In 1st pass over 'queueKeyInfoMap', create a unique list of domains
    // encountered, for which we will need to create a concrete instance of
    // 'mqbi::Domain'.  Additionally, also ensure that if a fanout queue has
    // appId/appKey pairs associated with it, they are unique.  We don't have
    // a global list of AppIds (in fact, we can't have that, because AppIds can
    // clash), so we check uniqueness of AppIds only for a given queue.  But we
    // do have a global unique list of AppKeys ('appKeys'), so we check each
    // queue's AppKey against that list.

    for (QueueKeyInfoMapConstIter qit = queueKeyInfoMap.begin();
         qit != queueKeyInfoMap.end();
         ++qit) {
        const mqbs::DataStoreConfigQueueInfo& qinfo = qit->second;
        bmqt::Uri                             uri(qinfo.canonicalQueueUri());
        if (!uri.isValid()) {
            BMQTSK_ALARMLOG_ALARM("RECOVERY")
                << clusterDescription << ": Partition [" << partitionId
                << "]: encountered invalid CanonicalQueueUri [" << uri << "]."
                << BMQTSK_ALARMLOG_END;
            mqbu::ExitUtil::terminate(mqbu::ExitCode::e_RECOVERY_FAILURE);
            // EXIT
        }

        if (qinfo.appIdKeyPairs().size() != 1 ||
            qinfo.appIdKeyPairs()[0].first !=
                bmqp::ProtocolUtil::k_DEFAULT_APP_ID) {
            // This ia a fanout queue
            AppIds appIds;

            for (size_t n = 0; n < qinfo.appIdKeyPairs().size(); ++n) {
                const AppIdKeyPair& p = qinfo.appIdKeyPairs()[n];

                AppIdsInsertRc appIdsIrc = appIds.insert(p.first);
                if (false == appIdsIrc.second) {
                    // Duplicate AppId.

                    BMQTSK_ALARMLOG_ALARM("RECOVERY")
                        << clusterDescription << ": Partition [" << partitionId
                        << "]: "
                        << "encountered a duplicate AppId while processing "
                        << "recovered queue [" << uri << "], " << "queueKey ["
                        << qit->first << "]. AppId [" << *(appIdsIrc.first)
                        << "]. AppKey [" << p.second << "]."
                        << BMQTSK_ALARMLOG_END;
                    mqbu::ExitUtil::terminate(
                        mqbu::ExitCode::e_RECOVERY_FAILURE);
                    // EXIT
                }

                bslmt::LockGuard<bslmt::Mutex> appLockGuard(appKeysLock);
                // LOCK

                AppKeysInsertRc appKeysIrc = appKeys->insert(p.second);
                if (!appKeysIrc.second && !isCSLMode) {
                    // Duplicate AppKey.  Error in non-csl mode.  In CSL mode,
                    // this can occur because this queue (and thus its appIds &
                    // appKeys) could be assigned at node startup, via
                    // CQH::onQueueAssigned ->
                    // StorageMgr::register/UpdateQueueReplica.
                    BMQTSK_ALARMLOG_ALARM("RECOVERY")
                        << clusterDescription << ": Partition [" << partitionId
                        << "]: "
                        << "encountered a duplicate AppKey while processing "
                        << "recovered queue [" << uri << "], queueKey ["
                        << qit->first << "]. AppKey [" << *(appKeysIrc.first)
                        << "]. AppId [" << p.first << "]."
                        << BMQTSK_ALARMLOG_END;
                    mqbu::ExitUtil::terminate(
                        mqbu::ExitCode::e_RECOVERY_FAILURE);
                    // EXIT
                }
            }
        }

        // Domain (key) may already exist but we don't care.
        domainMap.insert(bsl::make_pair(uri.qualifiedDomain(),
                                        static_cast<mqbi::Domain*>(0)));
    }

    // Print the unique list of retrieved domain names (useful for debugging
    // purposes).
    bmqu::MemOutStream os;
    os << clusterDescription << ": Partition [" << partitionId
       << "]: " << "retrieved "
       << bmqu::PrintUtil::prettyNumber(
              static_cast<bsls::Types::Int64>(queueKeyInfoMap.size()))
       << " queues belonging to "
       << bmqu::PrintUtil::prettyNumber(
              static_cast<bsls::Types::Int64>(domainMap.size()))
       << " domains.";

    for (DomainMapIter it = domainMap.begin(); it != domainMap.end(); ++it) {
        os << "\n  " << it->first;
    }

    BALL_LOG_INFO << os.str();

    // For each domain name in the 'domainMap', request the domain factory to
    // create the corresponding domain object.  Obtaining a domain object is
    // async, but once we issue all domain creation requests, we block until
    // all of them complete.  This keeps things manageable in this routine,
    // which itself is executed by each partition.  Blocking is not ideal, but
    // the broker is starting at this point, so its ok to do so.  Note that we
    // first issue all domain creation requests, and then block, instead of
    // issuing and blocking on one request at a time.
    bslmt::Latch latch(domainMap.size());

    for (DomainMapIter dit = domainMap.begin(); dit != domainMap.end();
         ++dit) {
        BALL_LOG_INFO << clusterDescription << ": Partition [" << partitionId
                      << "]: requesting domain for [" << dit->first << "].";

        domainFactory->createDomain(
            dit->first,
            bdlf::BindUtil::bind(&onDomain,
                                 bdlf::PlaceHolders::_1,  // status
                                 bdlf::PlaceHolders::_2,  // domain*
                                 &(dit->second),
                                 &latch,
                                 clusterDescription,
                                 dit->first,
                                 partitionId));
    }

    BALL_LOG_INFO << clusterDescription << ": Partition [" << partitionId
                  << "]: about to wait for [" << domainMap.size()
                  << "] domains to be created.";
    latch.wait();

    BALL_LOG_INFO << clusterDescription << ": Partition [" << partitionId
                  << "]: domain creation step complete. Checking if all "
                  << "domains were created successfully.";

    {
        bslmt::LockGuard<bslmt::Mutex> unrecognizedDomainsLockGuard(
            unrecognizedDomainsLock);  // LOCK

        for (DomainMapIter dit = domainMap.begin(); dit != domainMap.end();
             ++dit) {
            if (dit->second == 0 ||
                !dit->second->cluster()->isClusterMember()) {
                // Two scenarios:
                // 1. Failed to create domain for this domain name.
                // 2. Domain is associated with a proxy cluster.
                //
                // Will add it to the map of unrecognized domain names for
                // further investigation.
                unrecognizedDomains->insert(bsl::make_pair(
                    dit->first,
                    mqbs::StorageUtil::QueueMessagesCountMap()));
            }
        }
    }

    // All domains have been created.  Now make 2nd pass over 'queueKeyUriMap'
    // and create file-backed storages for each recovered queue.

    for (QueueKeyInfoMapConstIter qit = queueKeyInfoMap.begin();
         qit != queueKeyInfoMap.end();
         ++qit) {
        const mqbu::StorageKey&               queueKey = qit->first;
        const mqbs::DataStoreConfigQueueInfo& qinfo    = qit->second;
        const AppIdKeyPairs& appIdKeyPairs             = qinfo.appIdKeyPairs();
        const bmqt::Uri      queueUri(qinfo.canonicalQueueUri());
        BSLS_ASSERT_SAFE(queueUri.isValid());

        // Ensure queueKey uniqueness.

        QueueKeyStorageMapIter it = queueKeyStorageMap.find(queueKey);
        if (it != queueKeyStorageMap.end()) {
            // Encountered the queueKey again.  This is an error.

            const mqbs::ReplicatedStorage* rs = it->second;
            BMQTSK_ALARMLOG_ALARM("RECOVERY")
                << clusterDescription << ": Partition [" << partitionId
                << "]: encountered queueKey [" << queueKey
                << "] again, for uri [" << queueUri
                << "]. Uri associated with original queueKey: " << "["
                << rs->queueUri() << "]." << BMQTSK_ALARMLOG_END;
            mqbu::ExitUtil::terminate(mqbu::ExitCode::e_RECOVERY_FAILURE);
            // EXIT
        }

        // Ensure queueURI uniqueness, for this partition only though.

        StorageSpMapConstIter spmapIt = storageMap->find(queueUri);
        if (storageMap->end() != spmapIt) {
            // Already created ReplicatedStorage for this queueURI.
            if (isCSLMode) {
                // This can happen in CSL mode so we will just log it after
                // verifying that the queueKey and appIds are matching.

                const StorageSp& rstorage = spmapIt->second;
                BSLS_ASSERT_SAFE(queueKey == rstorage->queueKey());
                BSLS_ASSERT_SAFE(partitionId == rstorage->partitionId());

                for (AppIdKeyPairsCIter ait = appIdKeyPairs.begin();
                     ait != appIdKeyPairs.end();
                     ++ait) {
                    BSLA_MAYBE_UNUSED const bsl::string& appId = ait->first;
                    BSLA_MAYBE_UNUSED const mqbu::StorageKey& appKey =
                        ait->second;

                    BSLS_ASSERT_SAFE(!appKey.isNull());
                    BSLS_ASSERT_SAFE(!appId.empty());

                    mqbu::StorageKey existingAppKey;
                    BSLS_ASSERT_SAFE(
                        rstorage->hasVirtualStorage(appId, &existingAppKey));
                    BSLS_ASSERT_SAFE(appKey == existingAppKey);
                }

                {
                    bslmt::LockGuard<bslmt::Mutex>
                        unrecognizedDomainsLockGuard(
                            unrecognizedDomainsLock);  // LOCK

                    BSLS_ASSERT_SAFE(unrecognizedDomains->find(
                                         queueUri.qualifiedDomain()) ==
                                     unrecognizedDomains->end());
                }

                BALL_LOG_INFO << clusterDescription << ": Partition ["
                              << partitionId << "]: encountered queueUri ["
                              << queueUri << "] again. QueueKey of this uri ["
                              << queueKey << "].";

                queueKeyStorageMap.insert(
                    bsl::make_pair(queueKey, rstorage.get()));

                continue;  // CONTINUE
            }
            else {
                // This is an error in non-CSL mode.

                const StorageSp& rstorage = spmapIt->second;
                BMQTSK_ALARMLOG_ALARM("RECOVERY")
                    << clusterDescription << ": Partition [" << partitionId
                    << "]: encountered queueUri [" << queueUri
                    << "] again. QueueKey of this uri [" << queueKey
                    << "]. Details of original queueUri:: Partition ["
                    << rstorage->partitionId() << "], queueKey ["
                    << rstorage->queueKey() << "]." << BMQTSK_ALARMLOG_END;
                mqbu::ExitUtil::terminate(mqbu::ExitCode::e_RECOVERY_FAILURE);
                // EXIT
            }
        }

        // If domain name is unrecognized, do not create storage.
        const bslstl::StringRef& domainName = queueUri.qualifiedDomain();
        {
            bslmt::LockGuard<bslmt::Mutex> unrecognizedDomainsLockGuard(
                unrecognizedDomainsLock);  // LOCK

            DomainQueueMessagesCountMap::iterator iter =
                unrecognizedDomains->find(domainName);
            if (iter != unrecognizedDomains->end()) {
                iter->second.insert(bsl::make_pair(queueUri, 0));
                continue;  // CONTINUE
            }
        }

        DomainMapIter dit = domainMap.find(domainName);
        BSLS_ASSERT_SAFE(dit != domainMap.end());
        BSLS_ASSERT_SAFE(0 != dit->second);
        mqbi::Domain* domain = dit->second;
        BSLS_ASSERT_SAFE(domain->cluster());

        BALL_LOG_INFO << clusterDescription << " Partition [" << partitionId
                      << "] creating storage for queue [" << queueUri
                      << "], queueKey [" << queueKey << "].";

        // Update 'storageMap'.
        const mqbconfm::Domain&            domainCfg  = domain->config();
        const mqbconfm::StorageDefinition& storageDef = domainCfg.storage();

        if (domainCfg.mode().isUndefinedValue()) {
            BMQTSK_ALARMLOG_ALARM("RECOVERY")
                << clusterDescription << ": Partition [" << partitionId
                << "]: Domain for queue [" << queueUri << "], queueKey ["
                << queueKey << "] has invalid queue mode. Aborting broker."
                << BMQTSK_ALARMLOG_END;
            mqbu::ExitUtil::terminate(mqbu::ExitCode::e_RECOVERY_FAILURE);
            // EXIT
        }

        if (storageDef.config().isUndefinedValue()) {
            BMQTSK_ALARMLOG_ALARM("RECOVERY")
                << clusterDescription << ": Partition [" << partitionId
                << "]: Domain for queue [" << queueUri << "], queueKey ["
                << queueKey << "] has invalid storage config. Aborting broker."
                << BMQTSK_ALARMLOG_END;
            mqbu::ExitUtil::terminate(mqbu::ExitCode::e_RECOVERY_FAILURE);
            // EXIT
        }

        if (domain->cluster()->isClusterMember() &&
            !domain->cluster()->isLocal() &&
            (storageDef.config().isInMemoryValue() !=
             domainCfg.mode().isBroadcastValue())) {
            // In-memory storage without broadcast mode, as well as broadcast
            // mode without in-memory storage are incompatible config in a
            // clustered setup.

            BMQTSK_ALARMLOG_ALARM("RECOVERY")
                << clusterDescription << ": Partition [" << partitionId
                << "]: Queue [" << queueUri << "], queueKey [" << queueKey
                << "] is clustered, is setup with incompatible "
                << "config. In-memory storage: " << bsl::boolalpha
                << storageDef.config().isInMemoryValue()
                << ", broadcast mode: " << bsl::boolalpha
                << domainCfg.mode().isBroadcastValue() << ". Aboring broker."
                << BMQTSK_ALARMLOG_END;
            mqbu::ExitUtil::terminate(mqbu::ExitCode::e_RECOVERY_FAILURE);
            // EXIT
        }

        StorageSp rs_sp;
        fs->createStorage(&rs_sp, queueUri, queueKey, domain);
        BSLS_ASSERT_SAFE(rs_sp);

        BSLS_ASSERT_SAFE(storageMap->end() == storageMap->find(queueUri));
        storageMap->insert(bsl::make_pair(queueUri, rs_sp));
        fs->registerStorage(rs_sp.get());
        queueKeyStorageMap.insert(bsl::make_pair(queueKey, rs_sp.get()));

        // Create and add virtual storages, if any.
        bmqu::MemOutStream errorDesc;
        int                rc;
        if (domain->config().mode().isFanoutValue()) {
            for (AppIdKeyPairsCIter ait = appIdKeyPairs.begin();
                 ait != appIdKeyPairs.end();
                 ++ait) {
                const bsl::string&      appId  = ait->first;
                const mqbu::StorageKey& appKey = ait->second;

                BSLS_ASSERT_SAFE(!appKey.isNull());
                BSLS_ASSERT_SAFE(!appId.empty());

                if (0 != (rc = rs_sp->addVirtualStorage(errorDesc,
                                                        appId,
                                                        appKey))) {
                    // TBD: does this mean storage is corrupt? Should we abort?

                    BMQTSK_ALARMLOG_ALARM("RECOVERY")
                        << clusterDescription << ": Partition [" << partitionId
                        << "]: failed to create virtual storage with appId ["
                        << appId << "], appKey [" << appKey
                        << "] for queueUri [" << queueUri << "], queueKey ["
                        << queueKey << "]. Reason: [" << errorDesc.str()
                        << "], rc: " << rc << "." << BMQTSK_ALARMLOG_END;
                    continue;  // CONTINUE
                }

                BALL_LOG_INFO
                    << clusterDescription << " Partition [" << partitionId
                    << "]: Created virtual storage with appId [" << appId
                    << "], appKey [" << appKey << "] for queueUri ["
                    << queueUri << "], queueKey [" << queueKey << "].";
            }
        }
        else {
            // Fanout and non-fanout Queue Engines are converging.  Like Fanout
            // Queue Engine, non-fanout ones operate on appId which is
            // '__default' in non-fanout case.  For this reason, add a
            // VirtualStorage to the storage - just like in the Fanout case,
            // except it is hardcoded '__default' appId.
            //
            // In CSL mode, the default appId would have been covered in
            // 'appIdKeyPairs' above, so no need to explicitly add it here.

            rc = rs_sp->addVirtualStorage(
                errorDesc,
                bmqp::ProtocolUtil::k_DEFAULT_APP_ID,
                mqbi::QueueEngine::k_DEFAULT_APP_KEY);

            if (rc) {
                // TBD: does this mean storage is corrupt? Should we abort?

                BALL_LOG_WARN
                    << clusterDescription << ": Partition [" << partitionId
                    << "]: failed to create default virtual storage"
                    << " for queueUri [" << queueUri << "], queueKey ["
                    << queueKey << "]. Reason: [" << errorDesc.str()
                    << "], rc: " << rc << ".";
            }

            BALL_LOG_INFO << clusterDescription << " Partition ["
                          << partitionId
                          << "]: Created default virtual storage "
                          << " for queueUri [" << queueUri << "], queueKey ["
                          << queueKey << "].";
        }
    }

    // Iterate over FS[partitionId] using FS-Iterator, and update file-backed
    // storages.  Note that any virtual storages associated with each
    // file-backed storage will also be populated at this time.

    BALL_LOG_INFO << clusterDescription << ": Partition [" << partitionId
                  << "], total number of "
                  << "records found during recovery: " << fs->numRecords();

    typedef bsl::vector<mqbs::DataStoreRecordHandle> DataStoreRecordHandles;
    typedef DataStoreRecordHandles::iterator DataStoreRecordHandlesIter;

    mqbs::FileStoreIterator fsIt(fs);
    DataStoreRecordHandles  recordsToPurge;

    bsls::Types::Uint64 lastStrongConsistencySequenceNum    = 0;
    unsigned int        lastStrongConsistencyPrimaryLeaseId = 0;

    while (fsIt.next()) {
        mqbu::StorageKey          appKey;
        mqbu::StorageKey          queueKey;
        bmqt::MessageGUID         guid;
        mqbs::QueueOpType::Enum   queueOpType = mqbs::QueueOpType::e_UNDEFINED;
        unsigned int              refCount    = 0;
        mqbs::ConfirmReason::Enum confirmReason =
            mqbs::ConfirmReason::e_CONFIRMED;

        if (mqbs::RecordType::e_MESSAGE == fsIt.type()) {
            mqbs::MessageRecord msgRec;
            fsIt.loadMessageRecord(&msgRec);
            queueKey = msgRec.queueKey();
            guid     = msgRec.messageGUID();
            refCount = msgRec.refCount();
        }
        else if (mqbs::RecordType::e_CONFIRM == fsIt.type()) {
            mqbs::ConfirmRecord confRec;
            fsIt.loadConfirmRecord(&confRec);
            queueKey      = confRec.queueKey();
            appKey        = confRec.appKey();
            guid          = confRec.messageGUID();
            confirmReason = confRec.reason();
        }
        else if (mqbs::RecordType::e_QUEUE_OP == fsIt.type()) {
            // TODO_CSL When we logically delete the QLIST file, will we still
            // load QueueOp records here?
            mqbs::QueueOpRecord qOpRec;
            fsIt.loadQueueOpRecord(&qOpRec);
            queueKey    = qOpRec.queueKey();
            appKey      = qOpRec.appKey();
            queueOpType = qOpRec.type();
        }
        else {
            continue;  // CONTINUE
        }

        BSLS_ASSERT_SAFE(!queueKey.isNull());

        QueueKeyStorageMapIter storageMapIt = queueKeyStorageMap.find(
            queueKey);

        const mqbs::DataStoreRecordHandle& handle = fsIt.handle();

        // If queue is either not recovered or belongs to an unrecognized
        // domain.
        if (storageMapIt == queueKeyStorageMap.end()) {
            QueueKeyInfoMapConstIter infoMapCit = queueKeyInfoMap.find(
                queueKey);
            // If queue is recovered, implying that it belongs to an
            // unrecognized domain.
            if (infoMapCit != queueKeyInfoMap.cend()) {
                const bmqt::Uri uri(infoMapCit->second.canonicalQueueUri());

                DomainQueueMessagesCountMap::iterator domIt =
                    unrecognizedDomains->find(uri.qualifiedDomain());
                BSLS_ASSERT_SAFE(domIt != unrecognizedDomains->end());

                mqbs::StorageUtil::QueueMessagesCountMap::iterator countMapIt =
                    domIt->second.find(uri);
                BSLS_ASSERT_SAFE(countMapIt != domIt->second.end());
                ++countMapIt->second;
                recordsToPurge.push_back(handle);
                continue;  // CONTINUE
            }

            // If queue is not recovered
            BMQTSK_ALARMLOG_ALARM("STORAGE")
                << clusterDescription << ": Partition [" << partitionId
                << "], dropping record " << "because queue key '" << queueKey
                << "' not found in the "
                << "list of recovered queues, record: " << fsIt
                << BMQTSK_ALARMLOG_END;
            continue;  // CONTINUE
        }

        mqbs::ReplicatedStorage* rs = storageMapIt->second;
        BSLS_ASSERT_SAFE(rs);

        const bool isStrongConsistency = rs->isStrongConsistency();

        if (mqbs::RecordType::e_QUEUE_OP != fsIt.type()) {
            // It's one of MESSAGE/CONFIRM/DELETION records, which means it
            // must be a file-backed storage.

            if (!rs->isPersistent()) {
                BMQTSK_ALARMLOG_ALARM("STORAGE")
                    << clusterDescription << ": Partition [" << partitionId
                    << "]: For queue [" << rs->queueUri() << "] queueKey ["
                    << queueKey
                    << "] which is configured with in-memory storage, "
                    << " encountered a record of incompatible type ["
                    << fsIt.type() << "] during recovery at startup. "
                    << "Skipping this record." << BMQTSK_ALARMLOG_END;
                continue;  // CONTINUE
            }
        }

        // TODO_CSL When we logically delete the QLIST file, will we still
        // do this here?
        if (mqbs::RecordType::e_QUEUE_OP == fsIt.type()) {
            BSLS_ASSERT_SAFE(guid.isUnset());
            BSLS_ASSERT_SAFE(mqbs::QueueOpType::e_UNDEFINED != queueOpType);

            rs->addQueueOpRecordHandle(handle);

            if (mqbs::QueueOpType::e_PURGE == queueOpType &&
                !appKey.isNull()) {
                // It's a 'PURGE' QueueOp record for a specific appKey.  Purge
                // the virtual storage corresponding to that appKey.  Note that
                // 'purge' needs to be invoked only if 'appKey' is non-null,
                // because in other case (ie, 'appKey' being null), the
                // recovery phase would have already ignored all messages
                // belonging to the queue.  If it is desired to invoke 'purge'
                // unconditionally here, 'purge' will need to be passed a flag
                // (something like 'inRecovery=true'), based on which storage
                // implementation may or may not invoke certain business logic.

                rs->purge(appKey);
            }

            // TBD: check if adding 'else if' clauses for 'ADDITION',
            // 'DELETION' and 'CREATION' queueOpTypes to handle
            // addition/removal of virtual storages from 'rs' is better than
            // the current logic of adding virtual storages.  Note that in the
            // current logic, virtual storages may not be removed at recovery
            // even if indicated by the storage record.
        }
        else if (mqbs::RecordType::e_MESSAGE == fsIt.type()) {
            BSLS_ASSERT_SAFE(false == guid.isUnset());
            rs->processMessageRecord(guid,
                                     fs->getMessageLenRaw(handle),
                                     refCount,
                                     handle);
            if (isStrongConsistency) {
                lastStrongConsistencySequenceNum    = handle.sequenceNum();
                lastStrongConsistencyPrimaryLeaseId = handle.primaryLeaseId();
            }
        }
        else if (mqbs::RecordType::e_CONFIRM == fsIt.type()) {
            BSLS_ASSERT_SAFE(false == guid.isUnset());
            // If appKey is non-null, ensure that the 'rs' is already aware of
            // the appKey.  Note that this check should not be done for the
            // QueueOp record because a QueueOp record may contain a new AppKey
            // (dynamic appId registration...).

            if (!appKey.isNull() && !rs->hasVirtualStorage(appKey)) {
                BMQTSK_ALARMLOG_ALARM("STORAGE")
                    << clusterDescription << ": Partition [" << partitionId
                    << "], appKey [" << appKey << "] specified in "
                    << fsIt.type() << " record, with guid [" << guid
                    << "] not found in the list of virtual "
                    << "storages associated with file-backed "
                    << "storage for queue [" << rs->queueUri()
                    << "], queueKey [" << rs->queueKey()
                    << "]. Dropping this record." << BMQTSK_ALARMLOG_END;
                continue;  // CONTINUE
            }
            rs->processConfirmRecord(guid, appKey, confirmReason, handle);
        }
        else {
            BSLS_ASSERT(false);
        }
    }

    // Purge all records that have been invalidated
    for (DataStoreRecordHandlesIter it = recordsToPurge.begin();
         it != recordsToPurge.end();
         ++it) {
        fs->removeRecordRaw(*it);
    }

    fs->setLastStrongConsistency(lastStrongConsistencyPrimaryLeaseId,
                                 lastStrongConsistencySequenceNum);
}

void StorageUtil::dumpUnknownRecoveredDomains(
    const bsl::string&                  clusterDescription,
    bslmt::Mutex*                       unrecognizedDomainsLock,
    const DomainQueueMessagesCountMaps& unrecognizedDomains)
{
    // executed by *QUEUE_DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(unrecognizedDomainsLock);

    DomainQueueMessagesCountMap unrecognizedDomainsFlat;

    // 1. Collapse 'unrecognizedDomains' from a list of maps to a single map.
    {
        bslmt::LockGuard<bslmt::Mutex> unrecognizedDomainsLockGuard(
            unrecognizedDomainsLock);  // LOCK

        // Since 'unrecognizedDomains' has been resized to the number of
        // partitions upon construction, we need to check whether each map in
        // this vector is empty to verify emptiness.
        if (static_cast<size_t>(
                bsl::count_if(unrecognizedDomains.cbegin(),
                              unrecognizedDomains.cend(),
                              bdlf::MemFnUtil::memFn(
                                  &DomainQueueMessagesCountMap::empty))) ==
            unrecognizedDomains.size()) {
            return;  // RETURN
        }

        // All partitions have gone through 'recoveredQueuesCb', but we have
        // encountered some unrecognized domains.  We will print a warning in
        // the log with statistics about them, allowing BlazingMQ developers to
        // investigate.

        for (DomainQueueMessagesCountMaps::const_iterator cit =
                 unrecognizedDomains.cbegin();
             cit != unrecognizedDomains.cend();
             ++cit) {
            mqbs::StorageUtil::mergeDomainQueueMessagesCountMap(
                &unrecognizedDomainsFlat,
                *cit);
        }
    }

    // 2. Print statistics using the collapsed map, in sorted order of domain
    //    name.
    typedef bsl::vector<bsl::string> MapKeys;

    MapKeys keys;
    keys.reserve(unrecognizedDomainsFlat.size());
    for (mqbs::StorageUtil::DomainQueueMessagesCountMap::const_iterator cit =
             unrecognizedDomainsFlat.cbegin();
         cit != unrecognizedDomainsFlat.cend();
         ++cit) {
        keys.push_back(cit->first);
    }
    bsl::sort(keys.begin(), keys.end());

    bmqu::MemOutStream out;
    const int          level = 0;

    bdlb::Print::newlineAndIndent(out, level);
    out << "Unrecognized domains found while recovering '"
        << clusterDescription << "'";
    for (MapKeys::const_iterator cit = keys.cbegin(); cit != keys.cend();
         ++cit) {
        bdlb::Print::newlineAndIndent(out, level + 1);
        out << *cit << ":";

        // Sort the queues by number of messages
        typedef bsl::vector<mqbs::StorageUtil::QueueMessagesCount>
            QueueMessagesList;

        const mqbs::StorageUtil::QueueMessagesCountMap& queueMessages =
            unrecognizedDomainsFlat[*cit];
        QueueMessagesList queueMessagesList;
        for (mqbs::StorageUtil::QueueMessagesCountMap::const_iterator qmcit =
                 queueMessages.cbegin();
             qmcit != queueMessages.cend();
             ++qmcit) {
            queueMessagesList.push_back(*qmcit);
        }
        bsl::sort(queueMessagesList.begin(),
                  queueMessagesList.end(),
                  mqbs::StorageUtil::queueMessagesCountComparator);

        for (QueueMessagesList::const_iterator qmlcit =
                 queueMessagesList.cbegin();
             qmlcit != queueMessagesList.cend();
             ++qmlcit) {
            bdlb::Print::newlineAndIndent(out, level + 2);
            out << qmlcit->first << ": "
                << bmqu::PrintUtil::prettyNumber(qmlcit->second) << " msgs";
        }
    }

    BALL_LOG_ERROR << out.str();
}

void StorageUtil::gcUnrecognizedDomainQueues(
    FileStores*                         fileStores,
    bslmt::Mutex*                       unrecognizedDomainsLock,
    const DomainQueueMessagesCountMaps& unrecognizedDomains)
{
    // executed by cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(fileStores);
    BSLS_ASSERT_SAFE(unrecognizedDomainsLock);
    BSLS_ASSERT_SAFE(unrecognizedDomains.size() == fileStores->size());

    bsl::vector<bool> bitset(unrecognizedDomains.size(), false);
    // Did not use bsl::bitset because we do not know the required size at
    // compile time.

    {
        bslmt::LockGuard<bslmt::Mutex> unrecognizedDomainsLockGuard(
            unrecognizedDomainsLock);  // LOCK

        for (size_t i = 0; i < unrecognizedDomains.size(); ++i) {
            if (!unrecognizedDomains[i].empty()) {
                bitset[i] = true;
            }
        }
    }

    for (size_t i = 0; i < bitset.size(); ++i) {
        if (bitset[i]) {
            // Unrecognized domains are found in Partition 'i'. We initiate a
            // forced rollover to ensure that queues from those domains are
            // GC'd.

            mqbs::FileStore* fs = fileStores->at(i).get();
            BSLS_ASSERT_SAFE(fs);
            BSLS_ASSERT_SAFE(fs->isOpen());

            fs->execute(
                bdlf::BindUtil::bind(&mqbs::FileStore::forceRollover, fs));
        }
    }
}

void StorageUtil::stop(FileStores*        fileStores,
                       const bsl::string& clusterDescription,
                       const ShutdownCb&  shutdownCb)
{
    // executed by cluster *DISPATCHER* thread

    // Note that we won't delete any objects until dispatcher has stopped.  The
    // storages have already been closed in BBQueue.close.

    // Enqueue event to close all FileStores.

    BALL_LOG_INFO << clusterDescription
                  << ": Enqueuing event to close FileStores.";

    bslmt::Latch       latch(fileStores->size());
    bsls::Types::Int64 shutdownStartTime = bmqsys::Time::highResolutionTimer();
    for (unsigned int i = 0; i < fileStores->size(); ++i) {
        const bsl::shared_ptr<mqbs::FileStore>& fs = (*fileStores)[i];
        // 'fs' could be null because partition might not have been created at
        // broker start up (incorrect location, other errors)
        if (fs) {
            fs->execute(
                bdlf::BindUtil::bind(&shutdownCb,
                                     static_cast<int>(i),  // partitionId
                                     &latch));
        }
        else {
            latch.arrive();
        }
    }

    BALL_LOG_INFO << clusterDescription
                  << ": About to wait for partition shutdown to complete.";
    latch.wait();
    bsls::Types::Int64 shutdownEndTime = bmqsys::Time::highResolutionTimer();

    BALL_LOG_INFO
        << clusterDescription
        << ": Shutdown complete for all partitions. Total time spent in "
        << "shutdown: "
        << bmqu::PrintUtil::prettyTimeInterval(shutdownEndTime -
                                               shutdownStartTime)
        << " (" << (shutdownEndTime - shutdownStartTime) << " nanoseconds)";
}

void StorageUtil::shutdown(int                              partitionId,
                           bslmt::Latch*                    latch,
                           FileStores*                      fileStores,
                           const bsl::string&               clusterDescription,
                           const mqbcfg::ClusterDefinition& clusterConfig)
{
    // executed by *QUEUE_DISPATCHER* thread with the specified 'partitionId'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(fileStores->size()));

    // TBD: print a shutdown summary for all file-backed & virtual storages
    //      since storage manager has entire list.

    const bsl::shared_ptr<mqbs::FileStore>& fs = (*fileStores)[partitionId];
    // 'fs' could be null because partition might not have been created at
    // broker start up (incorrect location, other errors)
    if (fs) {
        BSLS_ASSERT_SAFE(fs->inDispatcherThread());

        BALL_LOG_INFO << clusterDescription << ": Closing Partition ["
                      << partitionId << "].";

        fs->close(clusterConfig.partitionConfig().flushAtShutdown());

        BALL_LOG_INFO << clusterDescription << ": Partition [" << partitionId
                      << "] closed.";
    }

    latch->arrive();
}

mqbu::StorageKey StorageUtil::generateAppKey(AppKeys*           appKeys,
                                             bslmt::Mutex*      appKeysLock,
                                             const bsl::string& appId)
{
    // executed by *QUEUE_DISPATCHER* thread or by *CLUSTER DISPATCHER* thread.

    bslmt::LockGuard<bslmt::Mutex> guard(appKeysLock);  // LOCK

    mqbu::StorageKey appKey;
    mqbs::StorageUtil::generateStorageKey(&appKey, appKeys, appId);
    BSLS_ASSERT_SAFE(!appKey.isNull());
    BSLS_ASSERT_SAFE(appKeys->end() != appKeys->find(appKey));

    return appKey;
}

void StorageUtil::registerQueue(
    const mqbi::Cluster*                     cluster,
    mqbi::Dispatcher*                        dispatcher,
    StorageSpMap*                            storageMap,
    bslmt::Mutex*                            storagesLock,
    mqbs::FileStore*                         fs,
    AppKeys*                                 appKeys,
    bslmt::Mutex*                            appKeysLock,
    bmqma::CountingAllocatorStore*           allocators,
    const mqbi::Dispatcher::ProcessorHandle& processor,
    const bmqt::Uri&                         uri,
    const mqbu::StorageKey&                  queueKey,
    const bsl::string&                       clusterDescription,
    int                                      partitionId,
    const AppIdKeyPairs&                     appIdKeyPairs,
    mqbi::Domain*                            domain)
{
    // executed by the *CLUSTER DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(cluster);
    BSLS_ASSERT_SAFE(dispatcher);
    BSLS_ASSERT_SAFE(dispatcher->inDispatcherThread(cluster));
    BSLS_ASSERT_SAFE(storageMap);
    BSLS_ASSERT_SAFE(fs);
    BSLS_ASSERT_SAFE(appKeys);
    BSLS_ASSERT_SAFE(allocators);
    BSLS_ASSERT_SAFE(uri.isValid());
    BSLS_ASSERT_SAFE(
        0 <= partitionId &&
        partitionId <
            cluster->clusterConfig()->partitionConfig().numPartitions());
    BSLS_ASSERT_SAFE(domain);
    if (!cluster->isCSLModeEnabled()) {
        BSLS_ASSERT_SAFE(appIdKeyPairs.empty());
    }

    // StorageMgr is either aware of the queue (the 'uri') or it isn't.  If it
    // is already aware, either this queue was registered earlier or it was
    // seen during recovery (if it's a file-backed storage), which means that
    // nothing needs to be written to QLIST file, and no in-memory structures
    // need to be updated.  There is one exception to this if queue is
    // configured in fanout mode -- we need to ensure that all appIds present
    // in config are registered with the in-mem/file-backed storage.  If not,
    // they should be registered.  This could occur if a node was started,
    // recovered a queue and its old appIds from the storage and created
    // associated storage and virtual storages, but queue's config was changed
    // and deployed before the node started (some appIds were added or removed
    // or both).  We need to make sure that these appIds are handled correctly.
    // The logic to get a list of added and/or removed appId/key pairs is
    // handled by invoking 'loadUpdatedAppIdKeyPairs' in this function if queue
    // is in fanout mode.

    // If StorageMgr is not aware of the queue, then its a simpler process --
    // simply register it and its associated appIds, if any.

    const mqbconfm::StorageDefinition& storageDef = domain->config().storage();
    const mqbconfm::QueueMode&         queueMode  = domain->config().mode();

    if (queueMode.isUndefinedValue()) {
        BMQTSK_ALARMLOG_ALARM("STORAGE")
            << "Partition [" << partitionId
            << "] Invalid queue-mode in the domain configuration while "
            << "attempting to register queue '" << uri << "', queueKey '"
            << queueKey << "'." << BMQTSK_ALARMLOG_END;
        return;  // RETURN
    }

    if (!cluster->isLocal()) {
        // Its a multi-node cluster.  Queue mode and storage type should be
        // compatible.  This sanity check should be done at the time of queue
        // creation, and StorageMgr should assume that queue is correctly
        // configured.
        if (storageDef.config().isInMemoryValue() !=
            queueMode.isBroadcastValue()) {
            BMQTSK_ALARMLOG_ALARM("STORAGE")
                << "Partition [" << partitionId << "] Incompatible "
                << "queue mode (" << queueMode.selectionName() << ") "
                << "and storage type (" << storageDef.config().selectionName()
                << ") while attempting to register clustered queue '" << uri
                << "', queueKey '" << queueKey << "'." << BMQTSK_ALARMLOG_END;
            return;  // RETURN
        }
    }

    BSLS_ASSERT_SAFE(storageDef.config().isInMemoryValue() ||
                     storageDef.config().isFileBackedValue());

    BALL_LOG_INFO << clusterDescription << " Partition [" << partitionId
                  << "]: Registering queue '" << uri << "', queueKey: '"
                  << queueKey << "' to storage.";

    bslmt::LockGuard<bslmt::Mutex> guard(storagesLock);  // LOCK

    StorageSpMapIter it = storageMap->find(uri);
    if (storageMap->end() != it) {
        const StorageSp& storageSp = it->second;

        BSLS_ASSERT_SAFE(storageSp->partitionId() == partitionId);
        BSLS_ASSERT_SAFE(storageSp->queueKey() == queueKey);
        BSLS_ASSERT_SAFE(storageSp);

        if (storageDef.config().isInMemoryValue()) {
            BSLS_ASSERT_SAFE(!storageSp->isPersistent());
        }
        else {
            BSLS_ASSERT_SAFE(storageSp->isPersistent());
        }

        if (queueMode.isFanoutValue()) {
            // Queue in fanout mode.  Need to check for any appIds which need
            // to be added or removed (see comments at the beginning of this
            // routine for explanation).

            AppIdKeyPairs addedAppIdKeyPairs, removedAppIdKeyPairs;

            bool hasUpdate = false;
            if (cluster->isCSLModeEnabled()) {
                // In CSL mode, queue assignment procedure is split into queue
                // assignment and queue update, so we simply remove all appIds
                // here, and re-add them during queue update phase.
                hasUpdate = loadUpdatedAppIdKeyPairs(
                    &addedAppIdKeyPairs,
                    &removedAppIdKeyPairs,
                    appKeys,
                    appKeysLock,
                    *storageSp.get(),
                    appIdKeyPairs,
                    bsl::vector<bsl::string>(),
                    true);  // isCSLMode
            }
            else {
                hasUpdate = loadUpdatedAppIdKeyPairs(
                    &addedAppIdKeyPairs,
                    &removedAppIdKeyPairs,
                    appKeys,
                    appKeysLock,
                    *storageSp.get(),
                    AppIdKeyPairs(),
                    queueMode.fanout().appIDs(),
                    false);  // isCSLMode
            }
            if (!hasUpdate) {
                // No update needed for AppId/Key pairs.
                return;  // RETURN
            }

            // Some AppId/Key pairs need to be updated.  Invoke
            // 'updateQueuePrimaryDispatched' in the right thread to carry out
            // the addition/removal of those pairs.

            mqbi::DispatcherEvent* queueEvent = dispatcher->getEvent(
                mqbi::DispatcherClientType::e_QUEUE);

            (*queueEvent)
                .makeDispatcherEvent()
                .setCallback(bdlf::BindUtil::bind(
                    updateQueuePrimaryDispatched,
                    bdlf::PlaceHolders::_1,  // processor
                    storageSp.get(),
                    storagesLock,
                    fs,
                    appKeys,
                    appKeysLock,
                    clusterDescription,
                    partitionId,
                    addedAppIdKeyPairs,
                    removedAppIdKeyPairs,
                    domain->config().mode().isFanoutValue(),
                    cluster->isCSLModeEnabled()));

            dispatcher->dispatchEvent(queueEvent,
                                      mqbi::DispatcherClientType::e_QUEUE,
                                      processor);

            // Wait for 'updateQueuePrimaryDispatched' operation to complete.
            // We need to wait because 'updateQueuePrimaryDispatched' creates
            // virtual storages corresponding to 'addedAppIdKeyPairs' (if any),
            // and the caller of 'registerQueue' expects these virtual storages
            // to be created this routine or its caller returns.  Before
            // waiting, release the 'storagesLock' guard and unlock it to avoid
            // any deadlock b/w cluster and partition dispatcher threads.

            guard.release()->unlock();

            dispatcher->synchronize(mqbi::DispatcherClientType::e_QUEUE,
                                    processor);
        }

        return;  // RETURN
    }

    // We are here means that StorageMgr is not aware of the queue.  Create an
    // appropriate storage and insert it in 'storageMap'.  It is important to
    // do this in the thread in which this method is invoked, because the
    // caller method expects the storage for the queue to be created.  Creating
    // the storage and updating 'storageMap' here now means that there is a
    // case where FileStore.writeQueueCreationRecord() in
    // registerQueueDispatched() may fail which will leave 'storageMap' and
    // QLIST file out of sync, but if we cannot write to file, it's game over
    // anyways, and broker will be taken down.

    StorageSp storageSp;
    fs->createStorage(&storageSp, uri, queueKey, domain);
    BSLS_ASSERT_SAFE(storageSp);

    BSLS_ASSERT_SAFE(storageMap->end() == storageMap->find(uri));
    storageMap->insert(bsl::make_pair(uri, storageSp));

    bmqu::MemOutStream errorDesc;
    int                rc = 0;
    AppIdKeyPairs      appIdKeyPairsToUse;
    if (queueMode.isFanoutValue()) {
        if (cluster->isCSLModeEnabled()) {
            for (AppIdKeyPairsCIter citer = appIdKeyPairs.begin();
                 citer != appIdKeyPairs.end();
                 ++citer) {
                rc = storageSp->addVirtualStorage(errorDesc,
                                                  citer->first,
                                                  citer->second);
            }

            appIdKeyPairsToUse = appIdKeyPairs;
        }
        else {
            // If fanout queue, generate unique appKeys for the configured
            // appIds, and create virtual storages for them.  Caller expects
            // virtual storages to be created when 'registerQueue' returns.

            const mqbconfm::QueueModeFanout& fanoutMode = queueMode.fanout();
            const bsl::vector<bsl::string>&  cfgAppIds  = fanoutMode.appIDs();

            typedef bsl::vector<bsl::string>::const_iterator ConstIter;

            for (ConstIter citer = cfgAppIds.begin(); citer != cfgAppIds.end();
                 ++citer) {
                mqbu::StorageKey appKey = generateAppKey(appKeys,
                                                         appKeysLock,
                                                         *citer);

                rc = storageSp->addVirtualStorage(errorDesc, *citer, appKey);

                appIdKeyPairsToUse.push_back(bsl::make_pair(*citer, appKey));
            }
        }
    }
    else {
        // Fanout and non-fanout Queue Engines are converging.  Like Fanout
        // Queue Engine, non-fanout ones operate on appId which is
        // '__default' in non-fanout case.  For this reason, add a
        // VirtualStorage to the storage - just like in the Fanout case,
        // except it is harcoded '__default' appId.

        rc = storageSp->addVirtualStorage(
            errorDesc,
            bmqp::ProtocolUtil::k_DEFAULT_APP_ID,
            mqbi::QueueEngine::k_DEFAULT_APP_KEY);
    }

    BSLS_ASSERT_SAFE(rc == 0);
    static_cast<void>(rc);

    // Dispatch the registration of storage with the partition in appropriate
    // thread.

    mqbi::DispatcherEvent* queueEvent = dispatcher->getEvent(
        mqbi::DispatcherClientType::e_QUEUE);

    (*queueEvent)
        .makeDispatcherEvent()
        .setCallback(bdlf::BindUtil::bind(&registerQueueDispatched,
                                          bdlf::PlaceHolders::_1,  // processor
                                          fs,
                                          storageSp.get(),
                                          clusterDescription,
                                          partitionId,
                                          appIdKeyPairsToUse));

    fs->dispatchEvent(queueEvent);
}

void StorageUtil::unregisterQueueDispatched(
    BSLS_ANNOTATION_UNUSED const mqbi::Dispatcher::ProcessorHandle& processor,
    mqbs::FileStore*                                                fs,
    StorageSpMap*                                                   storageMap,
    bslmt::Mutex*        storagesLock,
    const ClusterData*   clusterData,
    int                  partitionId,
    const PartitionInfo& pinfo,
    const bmqt::Uri&     uri)
{
    // executed by *QUEUE_DISPATCHER* thread with the specified 'partitionId'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(fs);
    BSLS_ASSERT_SAFE(fs->inDispatcherThread());
    BSLS_ASSERT_SAFE(storageMap);
    BSLS_ASSERT_SAFE(
        0 <= partitionId &&
        partitionId <
            clusterData->clusterConfig().partitionConfig().numPartitions());

    // This method must be invoked only at partition's primary node.

    if (clusterData->membership().selfNode() != pinfo.primary()) {
        BMQTSK_ALARMLOG_ALARM("STORAGE")
            << clusterData->identity().description() << " Partition ["
            << partitionId << "]: queue [" << uri
            << "] unregistration requested but self is not primary. Current "
            << "primary: "
            << (pinfo.primary() ? pinfo.primary()->nodeDescription()
                                : "** none **")
            << ", current leaseId: " << pinfo.primaryLeaseId()
            << ", primary status: " << pinfo.primaryStatus() << "."
            << BMQTSK_ALARMLOG_END;
        return;  // RETURN
    }

    if (bmqp_ctrlmsg::PrimaryStatus::E_ACTIVE != pinfo.primaryStatus()) {
        BMQTSK_ALARMLOG_ALARM("STORAGE")
            << clusterData->identity().description() << " Partition ["
            << partitionId << "]: queue [" << uri
            << "] unregistration requested but self is not ACTIVE primary."
            << ". Current leaseId: " << pinfo.primaryLeaseId()
            << ", primary status: " << pinfo.primaryStatus() << "."
            << BMQTSK_ALARMLOG_END;
        return;  // RETURN
    }

    bslmt::LockGuard<bslmt::Mutex> guard(storagesLock);  // LOCK

    StorageSpMapIter it = storageMap->find(uri);
    if (it == storageMap->end()) {
        // This can occur in the following scenario: replica receives
        // open-queue request for an unknown queue, sends queue-assignment
        // request to the leader, leader successfully assigns the queue, but
        // replica does not complete open-queue request (it goes down).  In
        // such a scenario, queue will remain assigned, but will not be
        // registered with StorageManager at the primary node, and thus, this
        // 'if' snippet will be executed.
        BALL_LOG_WARN
            << clusterData->identity().description() << " Partition ["
            << partitionId << "]: queue [" << uri
            << "] requested for unregistration not found in storage manager.";
        return;  // RETURN
    }

    BSLS_ASSERT_SAFE(it->second->partitionId() == partitionId);

    const StorageSp&         storage = it->second;
    const bsls::Types::Int64 numMsgs = storage->numMessages(
        mqbu::StorageKey::k_NULL_KEY);

    BSLS_ASSERT_SAFE(0 <= numMsgs);

    if (0 != numMsgs) {
        // ClusterQueueHelper invokes 'StorageMgr::unregisterQueue' only if
        // number of outstanding messages for the queue is zero.  If that's not
        // the case here, its an error.

        BMQTSK_ALARMLOG_ALARM("STORAGE")
            << clusterData->identity().description() << ": Partition ["
            << partitionId << "]: cannot unregister queue"
            << " because it has [" << numMsgs
            << "] outstanding messages. Queue [" << uri << "], queueKey ["
            << storage->queueKey() << "]."
            << BMQTSK_ALARMLOG_END return;  // RETURN
    }

    // Storage has no outstanding messages.

    BALL_LOG_INFO << clusterData->identity().description() << ": Partition ["
                  << partitionId << "], Deleting storage for queue [" << uri
                  << "], queueKey [" << storage->queueKey()
                  << "] as primary, as it has no outstanding messages.";

    mqbs::DataStoreRecordHandle handle;

    // TODO_CSL Do not write this record when we logically delete the QLIST
    // file
    int rc = fs->writeQueueDeletionRecord(
        &handle,
        storage->queueKey(),
        mqbu::StorageKey(),  // AppKey
        bdlt::EpochUtil::convertToTimeT64(bdlt::CurrentTime::utc()));

    if (0 != rc) {
        BMQTSK_ALARMLOG_ALARM("FILE_IO")
            << clusterData->identity().description() << ": Partition ["
            << partitionId
            << "] failed to write QueueDeletionRecord for queue [" << uri
            << "], queueKey [" << storage->queueKey() << "], rc: " << rc
            << BMQTSK_ALARMLOG_END;
        return;  // RETURN
    }

    // Delete all QueueOp records associated with the queue.
    //
    // TODO_CSL When we logically delete the QLIST file, no need to delete the
    // QueueOpRecord.DELETE record written above.  Think about whether we need
    // to remove other QueueOp records.
    const mqbs::ReplicatedStorage::RecordHandles& recHandles =
        storage->queueOpRecordHandles();
    for (size_t idx = 0; idx < recHandles.size(); ++idx) {
        fs->removeRecordRaw(recHandles[idx]);
    }

    // Delete the QueueOpRecord.DELETE record written above.

    fs->removeRecordRaw(handle);

    // Unregister storage from the partition, and finally get rid of it.

    fs->unregisterStorage(storage.get());
    storageMap->erase(it);  // will invalidate 'storage'

    // Ok to release lock now.

    guard.release()->unlock();

    // Flush the partition.  This routine ('unregisterQueue[Dispatched]') is
    // invoked only at the primary when a LocalQueue is deleted.  In case it
    // was the last instance of LocalQueue at this node, we need to make sure
    // that the partition is flushed and the QueueDeletion record reaches
    // replicas.

    fs->dispatcherFlush(true, false);
}

int StorageUtil::updateQueuePrimary(StorageSpMap*           storageMap,
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
                                    bool                    isCSLMode)
{
    // executed by *QUEUE_DISPATCHER* thread with the specified 'partitionId'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(fs);
    BSLS_ASSERT_SAFE(fs->inDispatcherThread());
    BSLS_ASSERT_SAFE(storageMap);
    BSLS_ASSERT_SAFE(appKeys);
    BSLS_ASSERT_SAFE(0 <= partitionId);

    if (addedIdKeyPairs.empty() && removedIdKeyPairs.empty()) {
        return 0;  // RETURN
    }

    bslmt::LockGuard<bslmt::Mutex> guard(storagesLock);  // LOCK

    StorageSpMapIter it = storageMap->find(uri);
    if (storageMap->end() == it) {
        bmqu::Printer<AppIdKeyPairs> printer1(&addedIdKeyPairs);
        bmqu::Printer<AppIdKeyPairs> printer2(&removedIdKeyPairs);
        BALL_LOG_ERROR << clusterDescription << " Partition [" << partitionId
                       << "]: Error when updating queue '" << uri
                       << "' with addedAppIds: [" << printer1
                       << "], removedAppIds: [" << printer2
                       << "]: Failed to find associated queue storage.";

        return -1;  // RETURN
    }

    const StorageSp& storageSp = it->second;
    BSLS_ASSERT_SAFE(storageSp);
    BSLS_ASSERT_SAFE(storageSp->partitionId() == partitionId);
    BSLS_ASSERT_SAFE(storageSp->queueKey() == queueKey);

    return updateQueuePrimaryRaw(storageSp.get(),
                                 fs,
                                 appKeys,
                                 appKeysLock,
                                 clusterDescription,
                                 partitionId,
                                 addedIdKeyPairs,
                                 removedIdKeyPairs,
                                 true,  // isFanout
                                 isCSLMode);
}

void StorageUtil::registerQueueReplicaDispatched(
    int*                           status,
    StorageSpMap*                  storageMap,
    bslmt::Mutex*                  storagesLock,
    mqbs::FileStore*               fs,
    mqbi::DomainFactory*           domainFactory,
    bmqma::CountingAllocatorStore* allocators,
    const bsl::string&             clusterDescription,
    int                            partitionId,
    const bmqt::Uri&               uri,
    const mqbu::StorageKey&        queueKey,
    mqbi::Domain*                  domain,
    bool                           allowDuplicate)
{
    // executed by *QUEUE_DISPATCHER* thread with the specified 'partitionId'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(fs);
    BSLS_ASSERT_SAFE(fs->inDispatcherThread());
    BSLS_ASSERT_SAFE(storageMap);
    BSLS_ASSERT_SAFE(domainFactory);
    BSLS_ASSERT_SAFE(allocators);
    BSLS_ASSERT_SAFE(0 <= partitionId);
    BSLS_ASSERT_SAFE(uri.isValid());
    BSLS_ASSERT_SAFE(!queueKey.isNull());

    // In non-CSL mode, this routine is executed at replica nodes when they
    // received a queue creation record from the primary in the partition
    // stream.  In CSL mode, this is executed at follower nodes upon commit
    // callback of Queue Assignment Advisory or Queue Update Advisory from the
    // leader.

    enum {
        rc_SUCCESS                     = 0,
        rc_QUEUE_ALREADY_REGISTERED    = -1,
        rc_DOMAIN_FAILURE              = -2,
        rc_INVALID_DOMAIN_CONFIG       = -3,
        rc_INVALID_STORAGE_CONFIG      = -4,
        rc_INCOMPATIBLE_STORAGE_CONFIG = -5
    };

    bslmt::LockGuard<bslmt::Mutex> guard(storagesLock);  // LOCK

    StorageSpMapIter it = storageMap->find(uri);
    if (it != storageMap->end()) {
        if (!allowDuplicate) {
            // This is a new queue but StorageMgr is already aware of this uri,
            // which means QLIST file for this partition already contains an
            // entry for this queue (because StorageMgr's in-memory data
            // structure and QLIST entries are in sync).  This indicates a bug
            // BlazingMQ replication.

            BALL_LOG_ERROR << "Failed to create queue '" << uri
                           << "', queueKey: [" << queueKey
                           << "], partitionId: [" << partitionId
                           << "], rc: " << rc_QUEUE_ALREADY_REGISTERED;
        }

        if (status) {
            *status = rc_QUEUE_ALREADY_REGISTERED;
        }

        return;  // RETURN
    }

    if (0 == domain) {
        // Before we create an instance of ReplicatedStorage, we need to obtain
        // the corresponding domain.  Obtaining the domain is async, but we
        // need to make it sync by blocking.  We cannot delay the creation of
        // file-backed storage (by waiting for the async domain creation)
        // because queue's primary may immediately start sending messages for
        // this queue, and this (non-primary) node will raise alarm if it
        // doesn't find the file-backed storage corresponding to that queue.

        bslmt::Latch latch(1);

        domainFactory->createDomain(
            uri.qualifiedDomain(),
            bdlf::BindUtil::bind(&onDomain,
                                 bdlf::PlaceHolders::_1,  // status
                                 bdlf::PlaceHolders::_2,  // domain*
                                 &domain,
                                 &latch,
                                 clusterDescription,
                                 uri.qualifiedDomain(),
                                 partitionId));
        latch.wait();

        if (0 == domain) {
            // Failed to obtain a domain object.

            BALL_LOG_ERROR << "Failed to create queue '" << uri
                           << "', queueKey: [" << queueKey
                           << "], partitionId: [" << partitionId
                           << "], rc: " << rc_DOMAIN_FAILURE;
            if (status) {
                *status = rc_DOMAIN_FAILURE;
            }

            return;  // RETURN
        }
    }

    const mqbconfm::Domain&            domainCfg  = domain->config();
    const mqbconfm::StorageDefinition& storageDef = domainCfg.storage();

    if (domainCfg.mode().isUndefinedValue()) {
        BALL_LOG_ERROR << "Failed to create queue '" << uri << "', queueKey: ["
                       << queueKey << "], partitionId: [" << partitionId
                       << "], rc: " << rc_INVALID_DOMAIN_CONFIG;
        if (status) {
            *status = rc_INVALID_DOMAIN_CONFIG;
        }

        return;  // RETURN
    }

    if (storageDef.config().isUndefinedValue()) {
        BALL_LOG_ERROR << "Failed to create queue '" << uri << "', queueKey: ["
                       << queueKey << "], partitionId: [" << partitionId
                       << "], rc: " << rc_INVALID_STORAGE_CONFIG;
        if (status) {
            *status = rc_INVALID_STORAGE_CONFIG;
        }

        return;  // RETURN
    }

    if (storageDef.config().isInMemoryValue() !=
        domainCfg.mode().isBroadcastValue()) {
        // In-memory storage without broadcast mode, as well as broadcast mode
        // without in-memory storage are incompatible config in a clustered
        // setup.

        BALL_LOG_ERROR << "Failed to create queue '" << uri << "', queueKey: ["
                       << queueKey << "], partitionId: [" << partitionId
                       << "], rc: " << rc_INCOMPATIBLE_STORAGE_CONFIG;
        if (status) {
            *status = rc_INCOMPATIBLE_STORAGE_CONFIG;
        }

        return;  // RETURN
    }

    bsl::shared_ptr<mqbs::ReplicatedStorage> rs_sp;
    fs->createStorage(&rs_sp, uri, queueKey, domain);
    BSLS_ASSERT_SAFE(rs_sp);

    BSLS_ASSERT_SAFE(storageMap->end() == storageMap->find(uri));
    storageMap->insert(bsl::make_pair(uri, rs_sp));
    fs->registerStorage(rs_sp.get());

    BALL_LOG_INFO << clusterDescription << ": Partition [" << partitionId
                  << "] registered [" << uri << "], queueKey [" << queueKey
                  << "] with the storage as replica.";

    if (status) {
        *status = rc_SUCCESS;
    }
}

void StorageUtil::unregisterQueueReplicaDispatched(
    int*                    status,
    StorageSpMap*           storageMap,
    bslmt::Mutex*           storagesLock,
    mqbs::FileStore*        fs,
    AppKeys*                appKeys,
    bslmt::Mutex*           appKeysLock,
    const bsl::string&      clusterDescription,
    int                     partitionId,
    const bmqt::Uri&        uri,
    const mqbu::StorageKey& queueKey,
    const mqbu::StorageKey& appKey,
    bool                    isCSLMode)
{
    // executed by *QUEUE_DISPATCHER* thread with the specified 'processorId'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(fs);
    BSLS_ASSERT_SAFE(fs->inDispatcherThread());
    BSLS_ASSERT_SAFE(storageMap);
    BSLS_ASSERT_SAFE(appKeys);
    BSLS_ASSERT_SAFE(0 <= partitionId);
    BSLS_ASSERT_SAFE(uri.isValid());
    BSLS_ASSERT_SAFE(!queueKey.isNull());

    // In non-CSL mode, this routine is executed at replica nodes when they
    // received a queue deletion record from the primary in the partition
    // stream.  In CSL mode, this is executed at follower nodes upon commit
    // callback of Queue Unassigned Advisory or Queue Update Advisory from the
    // leader.

    enum {
        rc_SUCCESS            = 0,
        rc_UNKNOWN_QUEUE_URI  = -1,
        rc_QUEUE_KEY_MISMATCH = -2,
        rc_QUEUE_HAS_MESSAGES = -3,
        rc_MISC               = -4
    };

    bslmt::LockGuard<bslmt::Mutex> guard(storagesLock);  // LOCK

    StorageSpMapIter it = storageMap->find(uri);
    if (it == storageMap->end()) {
        // Storage manager is not aware of this uri.  This indicates a bug BMQ
        // replication.

        BMQTSK_ALARMLOG_ALARM("REPLICATION")
            << clusterDescription << " Partition [" << partitionId
            << "]: unaware of uri while deleting " << "storage for queue [ "
            << uri << "], queueKey [" << queueKey << "]. Ignoring this event."
            << BMQTSK_ALARMLOG_END;
        if (status) {
            *status = rc_UNKNOWN_QUEUE_URI;
        }

        return;  // RETURN
    }

    mqbs::ReplicatedStorage* rs = it->second.get();
    BSLS_ASSERT_SAFE(rs);

    if (queueKey != rs->queueKey()) {
        // This really means that cluster state is out of sync across nodes.

        BMQTSK_ALARMLOG_ALARM("REPLICATION")
            << clusterDescription << " Partition [" << partitionId
            << "]: queueKey mismatch while deleting " << "storage for queue [ "
            << uri << "]. Specified queueKey [" << queueKey
            << "], queueKey associated with storage [" << rs->queueKey()
            << "]. Ignoring this event." << BMQTSK_ALARMLOG_END;
        if (status) {
            *status = rc_QUEUE_KEY_MISMATCH;
        }

        return;  // RETURN
    }

    if (appKey.isNull()) {
        // Entire queue is being deleted.

        if (isCSLMode) {
            const bsls::Types::Int64 numMsgs = rs->numMessages(
                mqbu::StorageKey::k_NULL_KEY);
            if (0 != numMsgs) {
                BMQTSK_ALARMLOG_ALARM("REPLICATION")
                    << clusterDescription << " Partition [" << partitionId
                    << "]: Attempt to delete storage for queue [ " << uri
                    << "], queueKey [" << queueKey << "] which has ["
                    << numMsgs << "] outstanding messages."
                    << BMQTSK_ALARMLOG_END;
                if (status) {
                    *status = rc_QUEUE_HAS_MESSAGES;
                }

                return;  // RETURN
            }

            const mqbs::ReplicatedStorage::RecordHandles& recHandles =
                rs->queueOpRecordHandles();
            for (size_t idx = 0; idx < recHandles.size(); ++idx) {
                fs->removeRecordRaw(recHandles[idx]);
            }
        }

        BALL_LOG_INFO << clusterDescription << ": Partition [" << partitionId
                      << "], Deleting storage for queue [" << uri
                      << "], queueKey [" << queueKey << "] as replica.";

        fs->unregisterStorage(rs);
        storageMap->erase(it);
        if (status) {
            *status = rc_SUCCESS;
        }

        return;  // RETURN
    }

    // A specific appId is being deleted.
    if (isCSLMode) {
        rs->purge(appKey);
    }

    int rc = removeVirtualStorageInternal(rs,
                                          appKeys,
                                          appKeysLock,
                                          appKey,
                                          partitionId);
    if (0 != rc) {
        BMQTSK_ALARMLOG_ALARM("REPLICATION")
            << clusterDescription << " Partition [" << partitionId
            << "]: Failed to remove virtual storage " << "for appKey ["
            << appKey << "] for queue [" << uri << "] and queueKey ["
            << queueKey << ", rc: " << rc << ". Ignoring this event."
            << BMQTSK_ALARMLOG_END;
        if (status) {
            *status = rc_MISC;
        }

        return;  // RETURN
    }

    BALL_LOG_INFO << clusterDescription << ": Partition [" << partitionId
                  << "], Removed virtual storage for appKey [" << appKey
                  << "] for queue [" << uri << "], queueKey [" << queueKey
                  << "] as replica.";

    if (status) {
        *status = rc_SUCCESS;
    }
}

void StorageUtil::updateQueueReplicaDispatched(
    int*                    status,
    StorageSpMap*           storageMap,
    bslmt::Mutex*           storagesLock,
    AppKeys*                appKeys,
    bslmt::Mutex*           appKeysLock,
    mqbi::DomainFactory*    domainFactory,
    const bsl::string&      clusterDescription,
    int                     partitionId,
    const bmqt::Uri&        uri,
    const mqbu::StorageKey& queueKey,
    const AppIdKeyPairs&    appIdKeyPairs,
    bool                    isCSLMode,
    mqbi::Domain*           domain,
    bool                    allowDuplicate)
{
    // executed by *QUEUE_DISPATCHER* thread with the specified 'partitionId'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(0 <= partitionId);
    BSLS_ASSERT_SAFE(storageMap);
    BSLS_ASSERT_SAFE(appKeys);
    BSLS_ASSERT_SAFE(domainFactory);
    BSLS_ASSERT_SAFE(uri.isValid());
    BSLS_ASSERT_SAFE(!queueKey.isNull());

    // In non-CSL mode, this routine is executed at replica nodes when they
    // received a queue creation record from the primary in the partition
    // stream.  In CSL mode, this is executed at follower nodes upon commit
    // callback of Queue Update Advisory from the leader.

    enum {
        rc_SUCCESS                          = 0,
        rc_QUEUE_UNKNOWN                    = -1,
        rc_VIRTUAL_STORAGE_CREATION_FAILURE = -2
    };

    bslmt::LockGuard<bslmt::Mutex> guard(storagesLock);  // LOCK

    StorageSpMapIter it = storageMap->find(uri);
    if (it == storageMap->end()) {
        // Cluster state and/or partition are out of sync at this replica.

        bmqu::Printer<AppIdKeyPairs> printer(&appIdKeyPairs);
        BMQTSK_ALARMLOG_ALARM("REPLICATION")
            << "At partition [" << partitionId
            << "], failure while registering appIds [" << printer
            << "] for queue [" << uri << "], queueKey [" << queueKey
            << "], rc: " << rc_QUEUE_UNKNOWN << BMQTSK_ALARMLOG_END;
        if (status) {
            *status = rc_QUEUE_UNKNOWN;
        }

        return;  // RETURN
    }

    mqbs::ReplicatedStorage* storage = it->second.get();
    BSLS_ASSERT(storage);

    if (domain == 0) {
        domain = domainFactory->getDomain(uri.qualifiedDomain());
    }
    BSLS_ASSERT_SAFE(domain);

    int rc = addVirtualStoragesInternal(
        storage,
        appKeys,
        appKeysLock,
        appIdKeyPairs,
        clusterDescription,
        partitionId,
        domain->config().mode().isFanoutValue(),
        isCSLMode);
    if (rc != 0) {
        if (!allowDuplicate) {
            bmqu::Printer<AppIdKeyPairs> printer(&appIdKeyPairs);
            BMQTSK_ALARMLOG_ALARM("REPLICATION")
                << "At partition [" << partitionId
                << "], failure while registering appIds [" << printer
                << "] for queue [" << uri << "], queueKey [" << queueKey
                << "], rc: " << (rc * 10 + rc_VIRTUAL_STORAGE_CREATION_FAILURE)
                << BMQTSK_ALARMLOG_END;
            if (status) {
                *status = rc * 10 + rc_VIRTUAL_STORAGE_CREATION_FAILURE;
            }
        }

        return;  // RETURN
    }

    bmqu::Printer<AppIdKeyPairs> printer(&appIdKeyPairs);
    BALL_LOG_INFO << clusterDescription << ": Partition [" << partitionId
                  << "] updated [" << uri << "], queueKey [" << queueKey
                  << "] with the storage as replica: " << "addedIdKeyPairs:"
                  << printer;

    if (status) {
        *status = rc_SUCCESS;
    }
}

void StorageUtil::setQueueDispatched(
    StorageSpMap*                storageMap,
    bslmt::Mutex*                storagesLock,
    BSLS_ANNOTATION_UNUSED const mqbi::Dispatcher::ProcessorHandle& processor,
    const bsl::string& clusterDescription,
    int                partitionId,
    const bmqt::Uri&   uri,
    mqbi::Queue*       queue)
{
    // executed by *QUEUE_DISPATCHER* thread with the specified 'partitionId'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(storageMap);
    BSLS_ASSERT_SAFE(0 <= partitionId);

    bslmt::LockGuard<bslmt::Mutex> guard(storagesLock);  // LOCK

    StorageSpMapIter it = storageMap->find(uri);
    if (it == storageMap->end()) {
        BALL_LOG_ERROR << clusterDescription << " Partition [" << partitionId
                       << "]: queue [" << uri
                       << "] not found in storage manager.";
        return;  // RETURN
    }

    BSLS_ASSERT_SAFE(it->second->partitionId() == partitionId);

    it->second->setQueue(queue);
}

int StorageUtil::makeStorage(bsl::ostream& errorDescription,
                             bslma::ManagedPtr<mqbi::Storage>* out,
                             StorageSpMap*                     storageMap,
                             bslmt::Mutex*                     storagesLock,
                             const bmqt::Uri&                  uri,
                             const mqbu::StorageKey&           queueKey,
                             int                               partitionId,
                             const bsls::Types::Int64          messageTtl,
                             const int maxDeliveryAttempts,
                             const mqbconfm::StorageDefinition& storageDef)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(storageMap);
    BSLS_ASSERT_SAFE(0 <= partitionId);
    BSLS_ASSERT_SAFE(storageDef.config().isInMemoryValue() ||
                     storageDef.config().isFileBackedValue());

    enum {
        // Value for the various RC error categories
        rc_SUCCESS             = 0,
        rc_STORAGE_CFG_FAILURE = -1,
        rc_UNKNOWN_QUEUE       = -2
    };

    bslmt::LockGuard<bslmt::Mutex> guard(storagesLock);  // LOCK

    StorageSpMapConstIter cit = storageMap->find(uri);
    if (cit == storageMap->end()) {
        // This indicates a bug.  A queue must be registered with StorageMgr
        // before 'makeStorage' is invoked on the queue ('registerQueue'
        // populates 'storageMap' with an entry for the 'queue' uri).

        errorDescription << "Unknown queue: " << uri;
        return rc_UNKNOWN_QUEUE;  // RETURN
    }

    const StorageSp& storageSp = cit->second;

#ifdef BSLS_ASSERT_SAFE_IS_ACTIVE
    BSLS_ASSERT_SAFE(storageSp);
    BSLS_ASSERT_SAFE(queueKey == storageSp->queueKey());
    if (storageSp->isPersistent()) {
        BSLS_ASSERT_SAFE(storageDef.config().isFileBackedValue());
    }
    else {
        BSLS_ASSERT_SAFE(storageDef.config().isInMemoryValue());
    }
#endif

    // Configure the storage.  Note that if a queue calls 'makeStorage' twice,
    // its storage will be configured twice as things are currently.

    const int rc = storageSp->configure(errorDescription,
                                        storageDef.config(),
                                        storageDef.queueLimits(),
                                        messageTtl,
                                        maxDeliveryAttempts);

    if (0 != rc) {
        return 10 * rc + rc_STORAGE_CFG_FAILURE;  // RETURN
    }

    out->load(storageSp.get(),
              0,  // cookie
              bslma::ManagedPtrUtil::noOpDeleter);

    static_cast<void>(queueKey);

    return rc_SUCCESS;
}

void StorageUtil::processPrimaryStatusAdvisoryDispatched(
    mqbs::FileStore*                           fs,
    PartitionInfo*                             pinfo,
    const bmqp_ctrlmsg::PrimaryStatusAdvisory& advisory,
    const bsl::string&                         clusterDescription,
    mqbnet::ClusterNode*                       source,
    bool                                       isFSMWorkflow)
{
    // executed by *QUEUE_DISPATCHER* thread with the specified 'partitionId'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(fs && fs->inDispatcherThread());
    BSLS_ASSERT_SAFE(pinfo);
    BSLS_ASSERT_SAFE(source);
    BSLS_ASSERT_SAFE(bmqp_ctrlmsg::PrimaryStatus::E_UNDEFINED !=
                     advisory.status());

    if (source == pinfo->primary()) {
        if (advisory.primaryLeaseId() != pinfo->primaryLeaseId()) {
            BMQTSK_ALARMLOG_ALARM("CLUSTER_STATE")
                << clusterDescription << " Partition ["
                << advisory.partitionId()
                << "]: received primary advisory: " << advisory
                << ", from perceived primary: " << source->nodeDescription()
                << ", but with different leaseId. LeaseId "
                << "perceived by self: " << pinfo->primaryLeaseId()
                << BMQTSK_ALARMLOG_END;
            return;  // RETURN
        }

        if (!isFSMWorkflow &&
            bmqp_ctrlmsg::PrimaryStatus::E_ACTIVE == pinfo->primaryStatus()) {
            // Self node perceives 'source' as active primary.  Check some
            // invariant.

            BSLS_ASSERT_SAFE(pinfo->primary() == fs->primaryNode());
            BSLS_ASSERT_SAFE(pinfo->primaryLeaseId() == fs->primaryLeaseId());
        }
    }
    else {
        // source !=  pinfo->primary()

        if (0 != pinfo->primary()) {
            // Primary status advisory from a different node.

            BMQTSK_ALARMLOG_ALARM("CLUSTER_STATE")
                << clusterDescription << " Partition ["
                << advisory.partitionId()
                << "]: received primary advisory: " << advisory
                << ", from: " << source->nodeDescription()
                << ", but self perceives: "
                << pinfo->primary()->nodeDescription()
                << " as current primary." << BMQTSK_ALARMLOG_END;
            return;  // RETURN
        }

        // 'pinfo->primary()' is null.  This means we haven't heard from the
        // leader about the partition/primary mapping.  We apply this to the
        // partition anyways, so that we can continue to accept storage events
        // once self has recovered from the 'source' for this partition but
        // hasn't transitioned to AVAILABLE (because its awaiting to finish
        // recovery for other partitions).  This node will eventually hear from
        // the leader and will not process further storage events once it
        // becomes AVAILABLE.

        // TBD: This breaks our contracts that only leader informs a node of
        // partition/primary mapping, so this needs to be reviewed carefully.
        // See 'ClusterStateMgr::processPrimaryStatusAdvisory' as well.

        BALL_LOG_WARN << clusterDescription << " Partition ["
                      << advisory.partitionId()
                      << "]: received primary advisory: " << advisory
                      << ", from: " << source->nodeDescription()
                      << ". Self has not received partition/primary advisory "
                      << "from leader yet, but will go ahead and mark this "
                      << "node as primary.";
    }

    pinfo->setPrimary(source);
    pinfo->setPrimaryLeaseId(advisory.primaryLeaseId());
    pinfo->setPrimaryStatus(advisory.status());

    if (bmqp_ctrlmsg::PrimaryStatus::E_ACTIVE == advisory.status()) {
        fs->setActivePrimary(source, advisory.primaryLeaseId());
    }
}

void StorageUtil::processReplicaStatusAdvisoryDispatched(
    mqbc::ClusterData*              clusterData,
    mqbs::FileStore*                fs,
    int                             partitionId,
    const PartitionInfo&            pinfo,
    mqbnet::ClusterNode*            source,
    bmqp_ctrlmsg::NodeStatus::Value status)
{
    // executed by *QUEUE_DISPATCHER* thread with the specified 'partitionId'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(fs && fs->inDispatcherThread());
    BSLS_ASSERT_SAFE(0 <= partitionId);

    // If self is *active* primary, force-issue a syncPt.
    BSLS_ASSERT_SAFE(pinfo.primary() == clusterData->membership().selfNode());
    if (bmqp_ctrlmsg::PrimaryStatus::E_ACTIVE == pinfo.primaryStatus()) {
        BALL_LOG_INFO << clusterData->identity().description()
                      << " Partition [" << partitionId
                      << "]: Received node status: " << status
                      << ", from replica node: " << source->nodeDescription()
                      << ". Self is ACTIVE primary, force-issuing a primary "
                      << "status advisory and a SyncPt.";

        forceIssueAdvisoryAndSyncPt(clusterData, fs, source, pinfo);
    }
    else {
        BALL_LOG_INFO << clusterData->identity().description()
                      << " Partition [" << partitionId
                      << "]: not issuing a primary status advisory or SyncPt "
                      << "upon receiving status advisory: " << status
                      << ", from replica: " << source->nodeDescription()
                      << ", because self is not an ACTIVE primary.";
    }
}

void StorageUtil::processShutdownEventDispatched(ClusterData*     clusterData,
                                                 PartitionInfo*   pinfo,
                                                 mqbs::FileStore* fs,
                                                 int              partitionId)
{
    // executed by *QUEUE_DISPATCHER* thread with the specified 'partitionId'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(clusterData);
    BSLS_ASSERT_SAFE(pinfo);
    BSLS_ASSERT_SAFE(fs);
    BSLS_ASSERT_SAFE(fs->inDispatcherThread());
    BSLS_ASSERT_SAFE(
        partitionId >= 0 &&
        partitionId <
            clusterData->clusterConfig().partitionConfig().numPartitions());

    // Send any advisories if self is (active) primary for 'partitionId'.
    if (pinfo->primary() == clusterData->membership().selfNode()) {
        // Cancel and wait for all timers (like TTL, periodic SyncPt etc) which
        // are enabled at the primary node.

        fs->cancelTimersAndWait();

        if (bmqp_ctrlmsg::PrimaryStatus::E_ACTIVE == pinfo->primaryStatus()) {
            // Primary (self) is an active one.  Issue a forced syncPt --
            // the last 'write' event that this primary will issue for this
            // partition.

            BSLS_ASSERT_SAFE(fs->primaryNode() == pinfo->primary());
            BSLS_ASSERT_SAFE(fs->primaryLeaseId() == pinfo->primaryLeaseId());

            int rc = fs->issueSyncPoint();
            if (0 != rc) {
                BALL_LOG_ERROR
                    << clusterData->identity().description() << "Partition ["
                    << partitionId
                    << "]: failed to force-issue SyncPt, rc: " << rc;
            }
            else {
                BALL_LOG_INFO
                    << clusterData->identity().description() << "Partition ["
                    << partitionId
                    << "]: force-issued SyncPt: " << fs->syncPoints().back()
                    << ".";
            }
        }
        else {
            BALL_LOG_INFO << clusterData->identity().description()
                          << " Partition [" << partitionId
                          << "]: not issuing a sync point while shutting "
                          << "down because self is not an active primary.";
        }

        // Send a 'PASSIVE' primary status advisory.

        pinfo->setPrimaryStatus(bmqp_ctrlmsg::PrimaryStatus::E_PASSIVE);

        bmqp_ctrlmsg::ControlMessage         controlMsg;
        bmqp_ctrlmsg::PrimaryStatusAdvisory& primaryAdv =
            controlMsg.choice()
                .makeClusterMessage()
                .choice()
                .makePrimaryStatusAdvisory();
        primaryAdv.partitionId()    = partitionId;
        primaryAdv.primaryLeaseId() = pinfo->primaryLeaseId();
        primaryAdv.status()         = bmqp_ctrlmsg::PrimaryStatus::E_PASSIVE;

        clusterData->messageTransmitter().broadcastMessageSafe(controlMsg);
    }

    // Notify partition that self node is shutting down (this occurs
    // irrespective of node's primary/replica status).

    // If self is replica for the specified 'partitionId', notify the
    // partition, which will then stop processing any replication events once
    // it receives a SyncPt.  Note that this SyncPt does not necessarily need
    // to be the one which the primary published in response to self node
    // (replica) broadcasting the STOPPING event.  The idea is that last record
    // in the journal should be a SyncPt, and self node should not process any
    // event after that.

    fs->processShutdownEvent();
}

void StorageUtil::forceFlushFileStores(FileStores* fileStores)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(fileStores);

    // For each partition which is open, enqueue a TTL check event in that
    // partition's dispatcher thread.

    for (size_t i = 0; i < fileStores->size(); ++i) {
        mqbs::FileStore* fs = (*fileStores)[i].get();
        BSLS_ASSERT_SAFE(fs);

        if (!fs->isOpen()) {
            continue;  // CONTINUE
        }

        fs->execute(bdlf::BindUtil::bind(&mqbs::FileStore::flush, fs));
    }
}

void StorageUtil::purgeDomainDispatched(
    bsl::vector<bsl::vector<mqbcmd::PurgeQueueResult> >*
                       purgedQueuesResultsVec,
    bslmt::Latch*      latch,
    int                partitionId,
    StorageSpMapVec*   storageMapVec,
    bslmt::Mutex*      storagesLock,
    const FileStores*  fileStores,
    const bsl::string& domainName)
{
    // executed by *QUEUE_DISPATCHER* thread with the specified 'partitionId'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(fileStores);
    BSLS_ASSERT_SAFE((*fileStores)[partitionId]->inDispatcherThread());
    BSLS_ASSERT_SAFE(latch);
    BSLS_ASSERT_SAFE(purgedQueuesResultsVec);
    BSLS_ASSERT_SAFE(0 <= partitionId);
    BSLS_ASSERT_SAFE(static_cast<unsigned int>(partitionId) <
                     fileStores->size());
    BSLS_ASSERT_SAFE(purgedQueuesResultsVec->size() == fileStores->size());
    BSLS_ASSERT_SAFE(storageMapVec);
    BSLS_ASSERT_SAFE(storagesLock);

    const mqbs::FileStore* fileStore = (*fileStores)[partitionId].get();

    mqbs::StorageCollectionUtil::StorageFilter filter =
        mqbs::StorageCollectionUtilFilterFactory::byDomain(domainName);

    bsl::vector<mqbi::Storage*> domainStorages;
    {
        bslmt::LockGuard<bslmt::Mutex> guard(storagesLock);  // LOCK

        const StorageSpMap& partitionStorages = (*storageMapVec)[partitionId];
        for (StorageSpMap::const_iterator it = partitionStorages.cbegin();
             it != partitionStorages.cend();
             it++) {
            if (filter(it->second.get())) {
                domainStorages.push_back(it->second.get());
            }
        }
        // Prepare a vector of storages to purge and release `storagesLock`
        // as fast as possible.
    }

    bsl::vector<mqbcmd::PurgeQueueResult>& purgedQueuesResults =
        (*purgedQueuesResultsVec)[partitionId];
    purgedQueuesResults.reserve(domainStorages.size());

    for (size_t i = 0; i < domainStorages.size(); i++) {
        mqbcmd::PurgeQueueResult result;
        // No need to pass a Semaphore here because we call it in
        // a synchronous way
        purgeQueueDispatched(&result, NULL, domainStorages[i], fileStore, "");
        purgedQueuesResults.push_back(result);
    }

    latch->arrive();
}

void StorageUtil::purgeQueueDispatched(
    mqbcmd::PurgeQueueResult* purgedQueueResult,
    bslmt::Semaphore*         purgeFinishedSemaphore,
    mqbi::Storage*            storage,
    const mqbs::FileStore*    fileStore,
    const bsl::string&        appId)
{
    // executed by *QUEUE_DISPATCHER* thread with the specified 'fileStore'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(fileStore && fileStore->inDispatcherThread());
    BSLS_ASSERT_SAFE(purgedQueueResult);
    BSLS_ASSERT_SAFE(storage);
    BSLS_ASSERT_SAFE(fileStore->config().partitionId() ==
                     storage->partitionId());

    // RAII to ensure we will post on the semaphore no matter how we return
    bdlb::ScopeExitAny semaphorePost(
        bdlf::BindUtil::bind(&optionalSemaphorePost, purgeFinishedSemaphore));

    if (!fileStore->primaryNode()) {
        bmqu::MemOutStream errorMsg;
        errorMsg << "Not purging queue '" << storage->queueUri() << "' "
                 << " with file store: " << fileStore->description()
                 << "[reason: unset primary node]";
        mqbcmd::Error& error = purgedQueueResult->makeError();
        error.message()      = errorMsg.str();
        return;  // RETURN
    }

    const bool isPrimary = (fileStore->primaryNode()->nodeId() ==
                            fileStore->config().nodeId());
    if (!isPrimary) {
        bmqu::MemOutStream errorMsg;
        errorMsg << "Not purging queue '" << storage->queueUri() << "' "
                 << " with primary node: "
                 << fileStore->primaryNode()->nodeDescription()
                 << "[reason: queue is NOT local]";
        mqbcmd::Error& error = purgedQueueResult->makeError();
        error.message()      = errorMsg.str();
        return;  // RETURN
    }

    mqbu::StorageKey appKey;
    if (appId.empty()) {
        // Entire queue needs to be purged.  Note that
        // 'bmqp::ProtocolUtil::k_NULL_APP_ID' is the empty string.
        appKey = mqbu::StorageKey::k_NULL_KEY;
    }
    else {
        // A specific appId (i.e., virtual storage) needs to be purged.
        if (!storage->hasVirtualStorage(appId, &appKey)) {
            bmqu::MemOutStream errorMsg;
            errorMsg << "Specified appId '" << appId << "' not found in the "
                     << "storage of queue '" << storage->queueUri() << "'.";
            mqbcmd::Error& error = purgedQueueResult->makeError();
            error.message()      = errorMsg.str();
            return;  // RETURN
        }
    }

    const bsls::Types::Uint64 numMsgs  = storage->numMessages(appKey);
    const bsls::Types::Uint64 numBytes = storage->numBytes(appKey);

    const mqbi::StorageResult::Enum rc = storage->removeAll(appKey);
    if (rc != mqbi::StorageResult::e_SUCCESS) {
        bmqu::MemOutStream errorMsg;
        errorMsg << "Failed to purge appId '" << appId << "', appKey '"
                 << appKey << "' of queue '" << storage->queueUri()
                 << "' [reason: " << mqbi::StorageResult::toAscii(rc) << "]";
        BALL_LOG_WARN << "#QUEUE_PURGE_FAILURE " << errorMsg.str();
        mqbcmd::Error& error = purgedQueueResult->makeError();
        error.message()      = errorMsg.str();
        return;  // RETURN
    }

    if (storage->queue()) {
        BSLS_ASSERT_SAFE(storage->queue()->queueEngine());
        storage->queue()->queueEngine()->afterQueuePurged(appId, appKey);
    }

    mqbcmd::PurgedQueueDetails& queueDetails = purgedQueueResult->makeQueue();
    queueDetails.queueUri()                  = storage->queueUri().asString();
    queueDetails.appId()                     = appId;
    bmqu::MemOutStream appKeyStr;
    appKeyStr << appKey;
    queueDetails.appKey()            = appKeyStr.str();
    queueDetails.numMessagesPurged() = numMsgs;
    queueDetails.numBytesPurged()    = numBytes;
}

int StorageUtil::processCommand(mqbcmd::StorageResult*     result,
                                FileStores*                fileStores,
                                StorageSpMapVec*           storageMapVec,
                                bslmt::Mutex*              storagesLock,
                                const mqbi::DomainFactory* domainFactory,
                                int*                       replicationFactor,
                                const mqbcmd::StorageCommand& command,
                                const bslstl::StringRef& partitionLocation,
                                bslma::Allocator*        allocator)
{
    // executed by cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(result);
    BSLS_ASSERT_SAFE(fileStores);
    BSLS_ASSERT_SAFE(storageMapVec);
    BSLS_ASSERT_SAFE(storagesLock);
    BSLS_ASSERT_SAFE(domainFactory);
    BSLS_ASSERT_SAFE(replicationFactor);

    if (command.isSummaryValue()) {
        loadStorageSummary(result, *fileStores, partitionLocation);
        return 0;  // RETURN
    }
    else if (command.isPartitionValue()) {
        const int partitionId = command.partition().partitionId();

        if (partitionId < 0 ||
            partitionId >= static_cast<int>(fileStores->size())) {
            bdlma::LocalSequentialAllocator<256> localAllocator(allocator);
            bmqu::MemOutStream                   os(&localAllocator);
            os << "Invalid partitionId value: '" << partitionId << "'";
            result->makeError().message() = os.str();
            return -1;  // RETURN
        }

        if (command.partition().command().isSummaryValue()) {
            loadPartitionStorageSummary(result,
                                        fileStores,
                                        partitionId,
                                        partitionLocation);
            return 0;  // RETURN
        }

        const bool isAvailable = command.partition().command().isEnableValue();

        mqbs::FileStore* const fs = (*fileStores)[partitionId].get();
        BSLS_ASSERT_SAFE(fs);

        fs->execute(
            bdlf::BindUtil::bind(&mqbs::FileStore::setAvailabilityStatus,
                                 fs,
                                 isAvailable));
        // We don't need to wait for the completion of above command.
        result->makeSuccess();
        return 0;  // RETURN
    }
    else if (command.isDomainValue()) {
        if (!domainFactory->getDomain(command.domain().name())) {
            bdlma::LocalSequentialAllocator<256> localAllocator(allocator);
            bmqu::MemOutStream                   os(&localAllocator);
            os << "Unknown domain '" << command.domain().name() << "'";
            result->makeError().message() = os.str();
            return -1;  // RETURN
        }
        if (command.domain().command().isQueueStatusValue()) {
            mqbcmd::StorageContent& storageContent =
                result->makeStorageContent();
            loadStorages(&storageContent.storages(),
                         command.domain().name(),
                         *fileStores);
            return 0;  // RETURN
        }
        else if (command.domain().command().isPurgeValue()) {
            bsl::vector<bsl::vector<mqbcmd::PurgeQueueResult> >
                purgedQueuesVec;
            purgedQueuesVec.resize(fileStores->size());

            // To purge a domain, we have to purge queues in each partition
            // from the correct thread.  This is achieved by parallel launch
            // of `purgeDomainDispatched` across all FileStore's threads.
            // We need to wait here, using `latch`, until the command completes
            // in all threads.
            executeForEachPartitions(
                bdlf::BindUtil::bind(&purgeDomainDispatched,
                                     &purgedQueuesVec,
                                     bdlf::PlaceHolders::_2,  // latch
                                     bdlf::PlaceHolders::_1,  // partitionId
                                     storageMapVec,
                                     storagesLock,
                                     fileStores,
                                     command.domain().name()),
                *fileStores);

            mqbcmd::PurgedQueues& purgedQueues = result->makePurgedQueues();
            for (size_t i = 0; i < purgedQueuesVec.size(); ++i) {
                const bsl::vector<mqbcmd::PurgeQueueResult>& purgedQs =
                    purgedQueuesVec[i];

                purgedQueues.queues().insert(purgedQueues.queues().begin(),
                                             purgedQs.begin(),
                                             purgedQs.end());
            }

            return 0;  // RETURN
        }
    }
    else if (command.isQueueValue()) {
        BSLS_ASSERT_SAFE(command.queue().command().isPurgeAppIdValue());

        const bmqt::Uri uri(command.queue().canonicalUri(), allocator);

        BSLS_ASSERT_SAFE(uri.isCanonical());

        mqbi::Storage* queueStorage = NULL;
        {
            bslmt::LockGuard<bslmt::Mutex> guard(storagesLock);  // LOCK
            for (StorageSpMapVec::iterator it = storageMapVec->begin();
                 it != storageMapVec->end();
                 it++) {
                StorageSpMap::iterator storageIter = it->find(uri);
                if (storageIter != it->end()) {
                    queueStorage = storageIter->second.get();
                    break;  // BREAK
                }
            }
        }

        if (!queueStorage) {
            bdlma::LocalSequentialAllocator<256> localAllocator(allocator);
            bmqu::MemOutStream                   os(&localAllocator);
            os << "Queue was not found in a storage '" << uri << "'";
            result->makeError().message() = os.str();
            return -1;  // RETURN
        }

        // Empty string means all appIds, however, for the command, we require
        // the user to be explicit if the entire queue is to be deleted, and
        // therefore require '*' for the appid.
        const bsl::string& purgeAppId = command.queue().command().purgeAppId();
        bsl::string        appId      = (purgeAppId == "*") ? "" : purgeAppId;

        const int    partitionId = queueStorage->partitionId();
        FileStoreSp& fileStore   = (*fileStores)[partitionId];

        mqbcmd::PurgeQueueResult purgedQueueResult;
        bslmt::Semaphore         purgeFinishedSemaphore;
        fileStore->execute(bdlf::BindUtil::bind(&purgeQueueDispatched,
                                                &purgedQueueResult,
                                                &purgeFinishedSemaphore,
                                                queueStorage,
                                                fileStore.get(),
                                                appId));

        purgeFinishedSemaphore.wait();

        result->makePurgedQueues();
        result->purgedQueues().queues().push_back(purgedQueueResult);

        return 0;  // RETURN
    }
    else if (command.isReplicationValue()) {
        mqbcmd::ReplicationResult replicationResult;
        const int rc = processReplicationCommand(&replicationResult,
                                                 replicationFactor,
                                                 fileStores,
                                                 command.replication());
        if (replicationResult.isErrorValue()) {
            result->makeError(replicationResult.error());
        }
        else {
            result->makeReplicationResult(replicationResult);
        }
        return rc;  // RETURN
    }

    bdlma::LocalSequentialAllocator<256> localAllocator(allocator);
    bmqu::MemOutStream                   os(&localAllocator);
    os << "Unknown command '" << command << "'";
    result->makeError().message() = os.str();
    return -1;
}

void StorageUtil::onDomain(const bmqp_ctrlmsg::Status& status,
                           mqbi::Domain*               domain,
                           mqbi::Domain**              out,
                           bslmt::Latch*               latch,
                           const bsl::string&          clusterDescription,
                           const bsl::string&          domainName,
                           int                         partitionId)
{
    // executed by *ANY* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(latch);
    BSLS_ASSERT_SAFE(out);

    if (bmqp_ctrlmsg::StatusCategory::E_SUCCESS != status.category()) {
        BSLS_ASSERT_SAFE(0 == domain);
        *out = 0;

        BALL_LOG_ERROR << clusterDescription << " Partition [" << partitionId
                       << "]: Failed to create domain for [" << domainName
                       << "], reason: " << status;
    }
    else {
        BSLS_ASSERT_SAFE(domain);
        *out = domain;
    }

    latch->arrive();
}

void StorageUtil::forceIssueAdvisoryAndSyncPt(mqbc::ClusterData*   clusterData,
                                              mqbs::FileStore*     fs,
                                              mqbnet::ClusterNode* destination,
                                              const PartitionInfo& pinfo)
{
    // executed by *QUEUE_DISPATCHER* thread with the specified 'partitionId'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(fs && fs->inDispatcherThread());
    BSLS_ASSERT_SAFE(clusterData);
    BSLS_ASSERT_SAFE(pinfo.primary() == clusterData->membership().selfNode());
    BSLS_ASSERT_SAFE(pinfo.primary() == fs->primaryNode());
    BSLS_ASSERT_SAFE(fs->primaryLeaseId() == pinfo.primaryLeaseId());
    BSLS_ASSERT_SAFE(pinfo.primaryStatus() ==
                     bmqp_ctrlmsg::PrimaryStatus::E_ACTIVE);

    bmqp_ctrlmsg::ControlMessage         controlMsg;
    bmqp_ctrlmsg::PrimaryStatusAdvisory& primaryAdv =
        controlMsg.choice()
            .makeClusterMessage()
            .choice()
            .makePrimaryStatusAdvisory();
    primaryAdv.partitionId() = fs->config().partitionId();
    ;
    primaryAdv.primaryLeaseId() = pinfo.primaryLeaseId();
    primaryAdv.status()         = bmqp_ctrlmsg::PrimaryStatus::E_ACTIVE;

    if (destination) {
        clusterData->messageTransmitter().sendMessageSafe(controlMsg,
                                                          destination);
    }
    else {
        clusterData->messageTransmitter().broadcastMessageSafe(controlMsg);
    }
    int rc = fs->issueSyncPoint();
    if (0 == rc) {
        BALL_LOG_INFO << clusterData->identity().description() << "Partition ["
                      << fs->config().partitionId()
                      << "]: successfully issued a forced SyncPt.";
    }
    else {
        BALL_LOG_ERROR << clusterData->identity().description()
                       << "Partition [" << fs->config().partitionId()
                       << "]: failed to force-issue SyncPt, rc: " << rc;
    }
}

}  // close package namespace
}  // close enterprise namespace
