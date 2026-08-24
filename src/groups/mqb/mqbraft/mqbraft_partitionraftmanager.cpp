// Copyright 2025-2026 Bloomberg Finance L.P.
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

// mqbraft_partitionraftmanager.cpp -*-C++-*-
#include <mqbraft_partitionraftmanager.h>

// MQB
#include <mqbc_storageutil.h>
#include <mqbcmd_messages.h>
#include <mqbs_filestore.h>
#include <mqbu_storagekey.h>

// BMQ
#include <bmqp_protocol.h>
#include <bmqu_blobobjectproxy.h>

// BDE
#include <ball_log.h>
#include <bdlf_bind.h>
#include <bdlf_placeholder.h>
#include <bslmt_latch.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace mqbraft {

namespace {
/// Interval at which TTL message GC is driven across partitions (mirrors the
/// legacy 'k_GC_MESSAGES_INTERVAL_SECONDS' in 'mqbc::StorageManager').
const int k_GC_MESSAGES_INTERVAL_SECONDS = 5;
}  // close unnamed namespace

// ===========================
// class PartitionRaftManager
// ===========================

// CREATORS
PartitionRaftManager::PartitionRaftManager(
    mqbc::ClusterData*               clusterData,
    mqbi::Cluster*                   cluster,
    mqbi::DomainFactory*             domainFactory,
    mqbc::ClusterState*              clusterState,
    const mqbcfg::ClusterDefinition& clusterConfig,
    const PartitionLeadershipCb&     leadershipCb,
    bslma::Allocator*                allocator)
: mqbc::StorageMonitor(cluster, allocator)
, d_allocators(allocator)
, d_clusterData_p(clusterData)
, d_cluster_p(cluster)
, d_domainFactory_p(domainFactory)
, d_clusterState_p(clusterState)
, d_clusterConfig(clusterConfig)
, d_replicationFactor(
      static_cast<int>(cluster->netCluster().nodes().size() / 2) + 1)
, d_leadershipCb(leadershipCb)
, d_allocator_p(bslma::Default::allocator(allocator))
, d_partitionRafts(allocator)
, d_fileStores(allocator)
, d_miscWorkThreadPool(1, 1, allocator)
, d_gcMessagesEventHandle()
{
    BSLS_ASSERT_SAFE(clusterData);
    BSLS_ASSERT_SAFE(cluster);
    BSLS_ASSERT_SAFE(domainFactory);
    BSLS_ASSERT_SAFE(clusterState);

    const int numPartitions = clusterConfig.partitionConfig().numPartitions();
    d_partitionRafts.resize(numPartitions);
    mqbc::StorageMonitor::resize(numPartitions);
}

PartitionRaftManager::~PartitionRaftManager()
{
    // Release the queue storages before the FileStores they reference (via
    // 'RecordStore* d_store_p') are destroyed.  The FileStores are held in the
    // derived member 'd_fileStores', which is destroyed after this body but
    // before the 'storageMonitor' base subobject that owns the storages.
    for (size_t i = 0; i < d_fileStores.size(); ++i) {
        mqbc::StorageMonitor::releaseStorages(static_cast<int>(i));
    }
}

// PRIVATE MANIPULATORS
void PartitionRaftManager::recoveredQueuesCb(
    int                    partitionId,
    const QueueKeyInfoMap* queueKeyInfoMap)
{
    // executed by *QUEUE_DISPATCHER* thread

    BSLS_ASSERT_SAFE(partitionId >= 0 &&
                     partitionId < static_cast<int>(d_partitionRafts.size()));
    BSLS_ASSERT_SAFE(d_fileStores[partitionId]->inDispatcherThread());

    if (d_cluster_p->isStopping()) {
        return;
    }

    mqbc::StorageUtil::recoveredQueuesCb(
        d_partitionRafts[partitionId].get(),
        d_domainFactory_p,
        0,  // unrecognizedDomainsLock
        0,  // unrecognizedDomains
        d_clusterState_p,
        d_clusterData_p->identity().description(),
        partitionId,
        queueKeyInfoMap,
        d_allocator_p);
}

void PartitionRaftManager::queueCreationCb(
    int                            partitionId,
    const bmqt::Uri&               uri,
    const mqbu::StorageKey&        queueKey,
    const mqbi::Storage::AppInfos& appIdKeyPairs,
    bool                           isNewQueue)
{
    // executed by *QUEUE_DISPATCHER* thread

    BSLS_ASSERT_SAFE(partitionId >= 0 &&
                     partitionId < static_cast<int>(d_partitionRafts.size()));
    BSLS_ASSERT_SAFE(d_fileStores[partitionId]->inDispatcherThread());

    PartitionRaft* pr = d_partitionRafts[partitionId].get();

    if (isNewQueue) {
        mqbc::StorageUtil::createQueueStorageAsReplica(pr,
                                                       d_domainFactory_p,
                                                       uri,
                                                       queueKey,
                                                       appIdKeyPairs,
                                                       0);
    }
    else {
        mqbc::StorageUtil::updateQueueStorageDispatched(d_domainFactory_p,
                                                        pr,
                                                        uri,
                                                        queueKey,
                                                        appIdKeyPairs,
                                                        0);
    }
}

void PartitionRaftManager::queueDeletionCb(int                     partitionId,
                                           const bmqt::Uri&        uri,
                                           const mqbu::StorageKey& queueKey,
                                           const mqbu::StorageKey& appKey)
{
    // executed by *QUEUE_DISPATCHER* thread

    BSLS_ASSERT_SAFE(partitionId >= 0 &&
                     partitionId < static_cast<int>(d_partitionRafts.size()));
    BSLS_ASSERT_SAFE(d_fileStores[partitionId]->inDispatcherThread());

    PartitionRaft* pr = d_partitionRafts[partitionId].get();

    mqbc::StorageUtil::removeQueueStorageDispatched(pr, uri, queueKey, appKey);
}

// MANIPULATORS
int PartitionRaftManager::start(bsl::ostream& errorDescription)
{
    const mqbcfg::PartitionConfig& partitionCfg =
        d_clusterConfig.partitionConfig();
    const int numPartitions = partitionCfg.numPartitions();

    int rc = mqbc::StorageUtil::validatePartitionDirectory(partitionCfg,
                                                           errorDescription);
    if (rc != 0) {
        return rc;
    }

    rc = mqbc::StorageUtil::validateDiskSpace(
        partitionCfg,
        *d_clusterData_p,
        mqbc::StorageUtil::findMinReqDiskSpace(partitionCfg));
    if (rc != 0) {
        return rc;
    }

    d_fileStores.resize(numPartitions);

    rc = mqbc::StorageUtil::assignPartitionDispatcherThreads(
        &d_miscWorkThreadPool,
        d_clusterData_p,
        *d_cluster_p,
        d_clusterData_p->dispatcherClientData().dispatcher(),
        partitionCfg,
        &d_fileStores,
        this,
        &d_clusterData_p->blobSpPool(),
        &d_allocators,
        errorDescription,
        d_replicationFactor,
        bdlf::BindUtil::bind(&PartitionRaftManager::recoveredQueuesCb,
                             this,
                             bdlf::PlaceHolders::_1,
                             bdlf::PlaceHolders::_2),
        bdlf::BindUtil::bind(&PartitionRaftManager::queueCreationCb,
                             this,
                             bdlf::PlaceHolders::_1,
                             bdlf::PlaceHolders::_2,
                             bdlf::PlaceHolders::_3,
                             bdlf::PlaceHolders::_4,
                             bdlf::PlaceHolders::_5),
        bdlf::BindUtil::bind(&PartitionRaftManager::queueDeletionCb,
                             this,
                             bdlf::PlaceHolders::_1,
                             bdlf::PlaceHolders::_2,
                             bdlf::PlaceHolders::_3,
                             bdlf::PlaceHolders::_4));
    if (rc != 0) {
        return rc;
    }

    for (int i = 0; i < numPartitions; ++i) {
        d_partitionRafts[i].load(new (*d_allocator_p)
                                     PartitionRaft(i,
                                                   d_fileStores[i],
                                                   d_clusterData_p,
                                                   this,
                                                   d_leadershipCb,
                                                   d_allocator_p),
                                 d_allocator_p);
    }

    // 'PartitionRaft::start' opens (and recovers) the FileStore, which must
    // run on that partition's dispatcher thread.  'start()' here runs on the
    // cluster dispatcher thread, so dispatch each partition's start to its own
    // thread; open/recovery failures are handled in-thread by 'start()' (ALARM
    // + terminate).
    for (int i = 0; i < numPartitions; ++i) {
        d_fileStores[i]->execute(
            bdlf::BindUtil::bind(&PartitionRaft::start,
                                 d_partitionRafts[i].get()));
    }

    // Schedule a periodic event which enqueues a TTL message-GC pass in each
    // partition's dispatcher thread.  Legacy drives the equivalent from
    // 'mqbc::StorageManager'; that path does not cover Raft partitions, so it
    // is driven here instead.
    d_clusterData_p->scheduler().scheduleRecurringEvent(
        &d_gcMessagesEventHandle,
        bsls::TimeInterval(k_GC_MESSAGES_INTERVAL_SECONDS),
        bdlf::BindUtil::bind(&PartitionRaftManager::gcExpiredMessages, this));

    BALL_LOG_INFO << "PartitionRaftManager dispatched start for "
                  << numPartitions << " partitions";

    return 0;
}

void PartitionRaftManager::gcExpiredMessages()
{
    // executed by the scheduler's *DISPATCHER* thread

    // Enqueue a cleanup pass (TTL message GC + history trim) on each
    // partition's own dispatcher thread.  The FileStore GCs expired messages
    // only if this node is that partition's Raft leader (its '!d_isPrimary'
    // guard), and those deletions replicate through the Raft log (the
    // storages' RecordStore is the 'PartitionRaft'); the history trim is an
    // in-memory operation, safe on any node.
    for (unsigned int i = 0; i < d_partitionRafts.size(); ++i) {
        PartitionRaft* pr = d_partitionRafts[i].get();
        if (!pr) {
            continue;  // CONTINUE
        }

        pr->execute(
            bdlf::BindUtil::bind(&mqbs::FileStore::scheduledCleanupStorages,
                                 pr->fileStore()));
    }
}

void PartitionRaftManager::stop()
{
    // Stop the periodic TTL message-GC event first, waiting for any in-flight
    // pass to finish, so no GC proposal/write occurs once the FileStores are
    // closed below.
    d_clusterData_p->scheduler().cancelEventAndWait(&d_gcMessagesEventHandle);

    // Cancel each partition's Raft tick timer first (waits for any in-flight
    // tick to finish), so no further proposals/writes occur once closing the
    // FileStores below.
    for (int i = 0; i < static_cast<int>(d_partitionRafts.size()); ++i) {
        if (d_partitionRafts[i]) {
            d_partitionRafts[i]->stop();
        }
    }

    // Close each partition's record store on its own dispatcher thread,
    // mirroring legacy 'StorageUtil::stop'/'shutdown' (a generic close, not
    // FSM/advisory specific -- safe to reuse).  This routes through
    // 'PartitionRaft::close()', which drops any outstanding pending write
    // before delegating to the underlying FileStore. 'FileStore::~FileStore()'
    // asserts '!d_isOpen', so this must complete before the FileStores are
    // destroyed.
    bslmt::Latch latch(d_fileStores.size());
    for (unsigned int i = 0; i < d_fileStores.size(); ++i) {
        const FileStoreSp& fs = d_fileStores[i];
        // 'fs' could be null if the partition was never created (start()
        // failed partway, or was never called).
        if (fs) {
            fs->execute(
                bdlf::BindUtil::bind(&mqbc::StorageUtil::shutdown,
                                     static_cast<int>(i),
                                     &latch,
                                     d_partitionRafts[i].get(),
                                     d_clusterData_p->identity().description(),
                                     d_clusterConfig));
        }
        else {
            latch.arrive();
        }
    }
    latch.wait();

    // Stop the misc work thread pool last, mirroring legacy
    // 'StorageManager::stop()'.  Closing FileStores above (or a
    // deferred-alias release, e.g. 'FileStore::gc') may have enqueued
    // 'gcWorkerDispatched' jobs onto this pool; 'stop()' blocks until the
    // queue drains and all worker threads join, so no job referencing a
    // FileStore is still in flight when the FileStores are later destroyed.
    d_miscWorkThreadPool.stop();

    BALL_LOG_INFO << "PartitionRaftManager stopped";
}

void PartitionRaftManager::onRaftPartitionEvent(
    const bsl::shared_ptr<const bdlbb::Blob>& blob,
    mqbnet::ClusterNode*                      source)
{
    // executed by the *IO* thread
    BSLS_ASSERT_SAFE(blob);
    BSLS_ASSERT_SAFE(source);

    bmqu::BlobPosition position;

    if (0 != bmqu::BlobUtil::findOffsetSafe(&position,
                                            *blob,
                                            sizeof(bmqp::EventHeader))) {
        BALL_LOG_ERROR
            << "Failed to locate RaftHeader in e_RAFT_PARTITION event";
        return;
    }

    bmqu::BlobObjectProxy<bmqp::RaftHeader> rh(blob.get(),
                                               position,
                                               true,    // read
                                               false);  // write
    if (!rh.isSet()) {
        BALL_LOG_ERROR << "Failed to read RaftHeader from "
                       << "e_RAFT_PARTITION event";
        return;
    }

    unsigned int partitionId = rh->partitionId();
    if (!validate(partitionId)) {
        BALL_LOG_ERROR << "ignoring e_RAFT_PARTITION event for partitionId "
                       << partitionId;
        return;
    }

    // Both handlers run on the target partition's dispatcher thread.
    switch (rh->msgType()) {
    case bmqp::RaftHeader::k_MSG_TYPE_APPEND_ENTRIES: {
        d_fileStores[partitionId]->execute(
            bdlf::BindUtil::bind(&PartitionRaft::appendEntries,
                                 d_partitionRafts[partitionId].get(),
                                 blob,
                                 source));
    } break;
    case bmqp::RaftHeader::k_MSG_TYPE_APPEND_ENTRIES_RESP: {
        d_fileStores[partitionId]->execute(
            bdlf::BindUtil::bind(&PartitionRaft::onAppendEntriesResponse,
                                 d_partitionRafts[partitionId].get(),
                                 blob,
                                 source));
    } break;
    default: {
        BALL_LOG_ERROR << "ignoring e_RAFT_PARTITION event for partitionId "
                       << partitionId << " with unknown message type "
                       << rh->msgType();
    } break;
    }
}

void PartitionRaftManager::appendSnapshotChunk(
    const bsl::shared_ptr<const bdlbb::Blob>& blob,
    mqbnet::ClusterNode*                      source)
{
    // executed by the *IO* thread
    BSLS_ASSERT_SAFE(blob);
    BSLS_ASSERT_SAFE(source);

    bmqu::BlobPosition position;

    if (0 != bmqu::BlobUtil::findOffsetSafe(&position,
                                            *blob,
                                            sizeof(bmqp::EventHeader))) {
        BALL_LOG_ERROR
            << "Failed to locate RaftHeader in e_RAFT_PARTITION event";
        return;
    }

    bmqu::BlobObjectProxy<bmqp::SnapshotChunkHeader> hdr(blob.get(),
                                                         position,
                                                         true,    // read
                                                         false);  // write

    if (!hdr.isSet()) {
        BALL_LOG_ERROR << "Failed to read SnapshotChunkHeader from "
                       << "e_RAFT_SNAPSHOT event";
        return;
    }

    unsigned int partitionId = hdr->partitionId();
    if (!validate(partitionId)) {
        BALL_LOG_ERROR << "ignoring e_RAFT_SNAPSHOT event for partitionId "
                       << partitionId;
        return;
    }

    d_fileStores[partitionId]->execute(
        bdlf::BindUtil::bind(&PartitionRaft::appendSnapshotChunk,
                             d_partitionRafts[partitionId].get(),
                             blob,
                             source));
}

void PartitionRaftManager::onRaftControlMessage(
    const bmqp_ctrlmsg::RaftMessage& message,
    int                              partitionId,
    mqbnet::ClusterNode*             source)
{
    // executed by the *IO* thread

    BSLS_ASSERT_SAFE(source);

    if (partitionId < 0 ||
        partitionId >= static_cast<int>(d_partitionRafts.size())) {
        BALL_LOG_ERROR << "Raft control message with invalid partitionId "
                       << partitionId;
        return;
    }

    if (!d_partitionRafts[partitionId]) {
        BALL_LOG_ERROR << "PartitionRaft not started for partition "
                       << partitionId;
        return;
    }

    d_fileStores[partitionId]->execute(
        bdlf::BindUtil::bind(&PartitionRaft::onRaftControlMessage,
                             d_partitionRafts[partitionId].get(),
                             message,
                             source));
}

// StorageProvider OVERRIDES
void PartitionRaftManager::registerQueue(const bmqt::Uri&        uri,
                                         const mqbu::StorageKey& queueKey,
                                         int                     partitionId,
                                         const AppInfos&         appIdKeyPairs,
                                         mqbi::Domain*           domain)
{
    // executed by the *CLUSTER DISPATCHER* thread

    BSLS_ASSERT_SAFE(d_cluster_p->inDispatcherThread());
    BSLS_ASSERT_SAFE(uri.isValid());
    BSLS_ASSERT_SAFE(partitionId >= 0 &&
                     partitionId < static_cast<int>(d_partitionRafts.size()));
    BSLS_ASSERT_SAFE(domain);

    d_fileStores[partitionId]->execute(
        bdlf::BindUtil::bind(&mqbc::StorageUtil::registerQueueAsPrimary,
                             d_partitionRafts[partitionId].get(),
                             uri,
                             queueKey,
                             appIdKeyPairs,
                             domain));
}

void PartitionRaftManager::unregisterQueueDispatched(int partitionId,
                                                     const bmqt::Uri& uri)
{
    // executed by *QUEUE_DISPATCHER* thread
    BSLS_ASSERT_SAFE(d_fileStores[partitionId]->inDispatcherThread());

    PartitionRaft* pr = d_partitionRafts[partitionId].get();
    BSLS_ASSERT_SAFE(pr);

    mqbi::StorageManager_PartitionInfo pinfo;
    if (pr->isLeader()) {
        pinfo.setPrimary(d_clusterData_p->membership().selfNode());
        pinfo.setPrimaryLeaseId(static_cast<unsigned int>(pr->currentTerm()));
        pinfo.setPrimaryStatus(bmqp_ctrlmsg::PrimaryStatus::E_ACTIVE);
    }

    mqbc::StorageUtil::unregisterQueueDispatched(pr,
                                                 d_clusterData_p,
                                                 partitionId,
                                                 pinfo,
                                                 uri);
}

void PartitionRaftManager::unregisterQueue(const bmqt::Uri& uri,
                                           int              partitionId)
{
    // executed by the *CLUSTER DISPATCHER* thread

    BSLS_ASSERT_SAFE(d_cluster_p->inDispatcherThread());
    BSLS_ASSERT_SAFE(partitionId >= 0 &&
                     partitionId < static_cast<int>(d_fileStores.size()));

    d_fileStores[partitionId]->execute(
        bdlf::BindUtil::bind(&PartitionRaftManager::unregisterQueueDispatched,
                             this,
                             partitionId,
                             uri));
}

int PartitionRaftManager::updateQueuePrimary(const bmqt::Uri& uri,
                                             int              partitionId,
                                             const AppInfos&  addedIdKeyPairs,
                                             const AppInfos& removedIdKeyPairs)
{
    // executed by *QUEUE_DISPATCHER* thread

    BSLS_ASSERT_SAFE(partitionId >= 0 &&
                     partitionId < static_cast<int>(d_fileStores.size()));
    BSLS_ASSERT_SAFE(d_fileStores[partitionId]->inDispatcherThread());

    return mqbc::StorageUtil::updateQueuePrimary(
        d_partitionRafts[partitionId].get(),
        uri,
        addedIdKeyPairs,
        removedIdKeyPairs);
}

int PartitionRaftManager::configureStorage(
    bsl::ostream&                      errorDescription,
    bsl::shared_ptr<mqbi::Storage>*    out,
    const bmqt::Uri&                   uri,
    const mqbu::StorageKey&            queueKey,
    int                                partitionId,
    const bsls::Types::Int64           messageTtl,
    const int                          maxDeliveryAttempts,
    const mqbconfm::StorageDefinition& storageDef)
{
    // executed by *QUEUE_DISPATCHER* thread

    BSLS_ASSERT_SAFE(partitionId >= 0 &&
                     partitionId < static_cast<int>(d_fileStores.size()));
    BSLS_ASSERT_SAFE(d_fileStores[partitionId]->inDispatcherThread());

    return mqbc::StorageUtil::configureStorage(
        errorDescription,
        out,
        d_partitionRafts[partitionId].get(),
        uri,
        queueKey,
        partitionId,
        messageTtl,
        maxDeliveryAttempts,
        storageDef);
}

mqbi::Dispatcher::ProcessorHandle
PartitionRaftManager::processorForPartition(int partitionId) const
{
    BSLS_ASSERT_SAFE(partitionId >= 0 &&
                     partitionId < static_cast<int>(d_fileStores.size()));

    return d_fileStores[partitionId]->processorId();
}

bool PartitionRaftManager::isStorageEmpty(const bmqt::Uri& uri,
                                          int              partitionId) const
{
    // executed by the *CLUSTER DISPATCHER* thread

    BSLS_ASSERT_SAFE(d_cluster_p->inDispatcherThread());
    BSLS_ASSERT_SAFE(partitionId >= 0 &&
                     partitionId < static_cast<int>(d_fileStores.size()));

    return mqbc::StorageMonitor::isStorageEmpty(uri, partitionId);
}

bool PartitionRaftManager::hasStorage(const bmqt::Uri& uri,
                                      int              partitionId) const
{
    return mqbc::StorageMonitor::hasStorage(uri, partitionId);
}

bool PartitionRaftManager::loadAppIds(bsl::unordered_set<bsl::string>* out,
                                      const bmqt::Uri&                 uri,
                                      int partitionId) const
{
    return mqbc::StorageMonitor::loadAppIds(out, uri, partitionId);
}

bool PartitionRaftManager::isRaft() const
{
    return true;
}

void PartitionRaftManager::proposeDeferredSyncPoint(int partitionId)
{
    // executed by the *CLUSTER DISPATCHER* thread

    if (!validate(static_cast<unsigned int>(partitionId))) {
        return;  // RETURN
    }

    PartitionRaft* raft = d_partitionRafts[partitionId].get();
    if (0 == raft) {
        return;  // RETURN
    }

    // Hop to the partition's dispatcher thread; the sync-point write is a
    // no-op there unless this node is the leader and deferred one.
    raft->execute(
        bdlf::BindUtil::bind(&PartitionRaft::proposeDeferredSyncPoint, raft));
}

void PartitionRaftManager::onPeerNodeStopping()
{
    // executed by the *CLUSTER DISPATCHER* thread

    for (unsigned int i = 0; i < d_partitionRafts.size(); ++i) {
        PartitionRaft* raft = d_partitionRafts[i].get();
        if (raft) {
            raft->execute(
                bdlf::BindUtil::bind(&PartitionRaft::proposeShutdownSyncPoint,
                                     raft));
        }
    }
}

void PartitionRaftManager::setElectionMode(ElectionMode::Enum mode)
{
    // executed by the *CLUSTER DISPATCHER* thread

    for (unsigned int i = 0; i < d_partitionRafts.size(); ++i) {
        PartitionRaft* raft = d_partitionRafts[i].get();
        if (raft) {
            raft->execute(bdlf::BindUtil::bind(&PartitionRaft::setElectionMode,
                                               raft,
                                               mode));
        }
    }
}

void PartitionRaftManager::processShutdownEvent()
{
    // executed by the *CLUSTER DISPATCHER* thread

    BSLS_ASSERT_SAFE(d_cluster_p->inDispatcherThread());

    // Propose a final sync point on each partition led by this node, for
    // legacy-recovery interop: legacy recovery truncates the journal after its
    // last sync point, and Raft otherwise writes one only on becoming leader.
    // Then set 'd_isStopping' on each FileStore, which gates the write path.
    for (unsigned int i = 0; i < d_fileStores.size(); ++i) {
        const FileStoreSp& fs = d_fileStores[i];
        if (!fs) {
            continue;  // CONTINUE
        }

        PartitionRaft* raft = d_partitionRafts[i].get();
        if (raft) {
            raft->execute(
                bdlf::BindUtil::bind(&PartitionRaft::proposeShutdownSyncPoint,
                                     raft));
        }

        fs->execute(
            bdlf::BindUtil::bind(&mqbs::FileStore::processShutdownEvent,
                                 fs.get()));
    }
}

bool PartitionRaftManager::canShutdown()
{
    // executed by the *CLUSTER DISPATCHER* thread

    bool result = true;

    for (unsigned int i = 0; i < d_partitionRafts.size(); ++i) {
        PartitionRaft* raft = d_partitionRafts[i].get();
        if (raft && !raft->canShutdown()) {
            result = false;
        }
    }

    return result;
}

void PartitionRaftManager::processCommand(
    mqbcmd::StorageResult*        result,
    const mqbcmd::StorageCommand& command)
{
    // executed by the *CLUSTER DISPATCHER* thread

    BSLS_ASSERT_SAFE(d_cluster_p->inDispatcherThread());

    mqbc::StorageUtil::RecordStores recordStores(d_allocator_p);
    recordStores.reserve(d_partitionRafts.size());
    for (PartitionRafts::iterator it = d_partitionRafts.begin();
         it != d_partitionRafts.end();
         ++it) {
        recordStores.push_back(it->get());
    }

    mqbc::StorageUtil::processCommand(
        result,
        recordStores,
        d_domainFactory_p,
        &d_replicationFactor,
        command,
        d_clusterConfig.partitionConfig().location(),
        d_allocator_p);
}

int PartitionRaftManager::purgeQueueOnDomain(mqbcmd::StorageResult* result,
                                             const bsl::string&     domainName)
{
    // executed by cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_clusterData_p->cluster().inDispatcherThread());

    mqbc::StorageUtil::RecordStores recordStores(d_allocator_p);
    recordStores.reserve(d_partitionRafts.size());
    for (PartitionRafts::iterator it = d_partitionRafts.begin();
         it != d_partitionRafts.end();
         ++it) {
        recordStores.push_back(it->get());
    }

    mqbc::StorageUtil::purgeQueueOnDomain(result, domainName, recordStores);

    return 0;
}

}  // close package namespace
}  // close enterprise namespace
