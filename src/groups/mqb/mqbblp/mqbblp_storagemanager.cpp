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

// mqbblp_storagemanager.cpp                                          -*-C++-*-
#include <mqbblp_storagemanager.h>

#include <mqbscm_version.h>
// MQB
#include <mqbblp_recoverymanager.h>
#include <mqbc_clustermembership.h>
#include <mqbcmd_messages.h>
#include <mqbi_cluster.h>
#include <mqbi_queue.h>
#include <mqbnet_cluster.h>
#include <mqbnet_elector.h>
#include <mqbs_datastore.h>
#include <mqbs_filestoreprintutil.h>
#include <mqbs_filestoreprotocol.h>
#include <mqbs_storageprintutil.h>
#include <mqbs_virtualstorage.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_protocol.h>
#include <bmqp_recoverymessageiterator.h>
#include <bmqp_storagemessageiterator.h>
#include <bmqu_memoutstream.h>

#include <bmqsys_time.h>
#include <bmqtsk_alarmlog.h>
#include <bmqu_blob.h>
#include <bmqu_blobobjectproxy.h>
#include <bmqu_printutil.h>

// BDE
#include <bdlf_bind.h>
#include <bdlf_placeholder.h>
#include <bdlmt_fixedthreadpool.h>
#include <bdls_filesystemutil.h>
#include <bdls_pathutil.h>
#include <bdlt_currenttime.h>
#include <bdlt_epochutil.h>
#include <bdlt_timeunitratio.h>
#include <bsl_cstddef.h>
#include <bsl_cstdlib.h>
#include <bsl_cstring.h>
#include <bsl_iostream.h>
#include <bsl_numeric.h>
#include <bsl_utility.h>
#include <bslma_managedptr.h>
#include <bslmt_latch.h>
#include <bslmt_lockguard.h>
#include <bsls_annotation.h>
#include <bsls_timeinterval.h>

// SYS
#include <unistd.h>

namespace BloombergLP {
namespace mqbblp {

namespace {
const int k_GC_MESSAGES_INTERVAL_SECONDS = 30;

bsl::ostream& printRecoveryBanner(bsl::ostream&      out,
                                  const bsl::string& lastLineSuffix)
{
    out << "Starting"
        << "\n   _____"
        << "\n  |  __ \\"
        << "\n  | |__) |___  ___ _____   _____ _ __ _   _"
        << "\n  |  _  // _ \\/ __/ _ \\ \\ / / _ \\ '__| | | |"
        << "\n  | | \\ \\  __/ (_| (_) \\ V /  __/ |  | |_| |"
        << "\n  |_|  \\_\\___|\\___\\___/ \\_/ \\___|_|   \\__, |"
        << "\n                                       __/ |"
        << "\n                                      |___/"
        << " " << lastLineSuffix << "\n";

    return out;
}

/// Return true if the leader message sequence number in the specified `lms`
/// is zero, false otherwise.
bool isZero(const bmqp_ctrlmsg::LeaderMessageSequence& lms)
{
    return lms.electorTerm() == mqbnet::Elector::k_INVALID_TERM &&
           lms.sequenceNumber() == 0;
}

}  // close unnamed namespace

// ----------------------------
// class StorageManagerIterator
// ----------------------------

StorageManagerIterator::~StorageManagerIterator()
{
    d_manager_p->d_storagesLock.unlock();  // UNLOCK
}

// --------------------
// class StorageManager
// --------------------

// PRIVATE MANIPULATORS
void StorageManager::startRecoveryCb(int partitionId)
{
    // executed by each of the *STORAGE (QUEUE) DISPATCHER* threads

    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_fileStores.size()));
    BSLS_ASSERT_SAFE(d_recoveryManager_mp);
    BSLS_ASSERT_SAFE(d_recoveryManager_mp->isStarted());

    BALL_LOG_INFO_BLOCK
    {
        mqbc::StorageUtil::printRecoveryPhaseOneBanner(
            BALL_LOG_OUTPUT_STREAM,
            d_clusterData_p->identity().description(),
            partitionId);
    }

    mqbs::FileStore* fs = d_fileStores[partitionId].get();
    BSLS_ASSERT_SAFE(fs);
    BSLS_ASSERT_SAFE(fs->config().partitionId() == partitionId);

    const mqbi::DispatcherClientData& dispatcherClientData =
        fs->dispatcherClientData();

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << " ProcessorID [" << dispatcherClientData.processorHandle()
                  << "] | Partition [" << partitionId
                  << "]: Starting first phase of recovery.";

    // Start recovery for the partition through recovery manager.  Note that if
    // its a local cluster, recovery manager will invoke 'onPartitionRecovery'
    // callback right away in this thread *before* 'startRecovery' returns.  We
    // also bind current primary & leaseId to the specified callback which will
    // help us decide if we need to reschedule recovery in the callback.

    d_recoveryManager_mp->startRecovery(
        partitionId,
        dispatcherClientData,
        bdlf::BindUtil::bind(&StorageManager::onPartitionRecovery,
                             this,
                             bdlf::PlaceHolders::_1,  // partitionId
                             bdlf::PlaceHolders::_2,  // status
                             bdlf::PlaceHolders::_3,  // events
                             bdlf::PlaceHolders::_4,  // peer
                             bmqsys::Time::highResolutionTimer()));
}

void StorageManager::onPartitionRecovery(
    int                        partitionId,
    int                        status,
    const bsl::vector<BlobSp>& recoveryEvents,
    mqbnet::ClusterNode*       recoveryPeer,
    bsls::Types::Int64         recoveryStartTime)
{
    // executed by *QUEUE_DISPATCHER* thread associated with 'partitionId'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_fileStores.size()));

    if (d_cluster_p->isStopping()) {
        BALL_LOG_WARN << d_clusterData_p->identity().description()
                      << ": Partition [" << partitionId
                      << "] cluster is stopping; skipping partition recovery.";
        return;  // RETURN
    }

    mqbs::FileStore* fs = d_fileStores[partitionId].get();
    BSLS_ASSERT_SAFE(fs);
    BSLS_ASSERT_SAFE(!fs->isOpen());
    BSLS_ASSERT_SAFE(d_numPartitionsRecoveredFully <
                     static_cast<int>(d_fileStores.size()));

    // See if we need to schedule another recovery.  This needs to occur if
    // self became aware of a new primary while it was syncing storage with the
    // 'recoveryPeer' for this partition.  This needs to occur even if sync
    // with 'recoveryPeer' was successful (i.e., 'status' == 0).

    const PartitionInfo& pinfo = d_partitionInfoVec[partitionId];

    if (pinfo.primary() != 0 && recoveryPeer != 0 &&
        pinfo.primary() != recoveryPeer &&
        pinfo.primary() != d_clusterData_p->membership().selfNode()) {
        BALL_LOG_INFO << d_clusterData_p->identity().description()
                      << " Partition [" << partitionId
                      << "]: scheduling another recovery due to new "
                      << "primary being chosen while previous recovery was "
                      << "in progress with recovery peer: "
                      << recoveryPeer->nodeDescription() << ". New primary: "
                      << pinfo.primary()->nodeDescription() << ".";

        startRecoveryCb(partitionId);

        // The 'd_numPartitionsRecoveredFully' variable should not be
        // decremented in this case.
        return;  // RETURN
    }

    if (0 != status) {
        BMQTSK_ALARMLOG_ALARM("RECOVERY")
            << d_clusterData_p->identity().description() << ": Partition ["
            << partitionId << "] failed to recover with peer "
            << (recoveryPeer ? recoveryPeer->nodeDescription() : "**NA**")
            << " with status: " << status
            << ". This node will not be started now (even if other partitions "
            << "recover successfully)." << BMQTSK_ALARMLOG_END;
    }
    else {
        // Recovery was successful & there is no need to schedule another one.

        BALL_LOG_INFO << d_clusterData_p->identity().description()
                      << ": Partition [" << partitionId
                      << "] has successfully recovered with peer "
                      << (recoveryPeer ? recoveryPeer->nodeDescription()
                                       : "**NA**")
                      << ", and will now be opened.";

        int rc = fs->open();
        if (0 != rc) {
            BMQTSK_ALARMLOG_ALARM("FILE_IO")
                << d_clusterData_p->identity().description()
                << ": Failed to open Partition [" << partitionId
                << "] after recovery was finished, rc: " << rc
                << BMQTSK_ALARMLOG_END;
        }
        else {
            // Apply 'recoveryEvents' to the file store.

            BALL_LOG_INFO << d_clusterData_p->identity().description()
                          << ": Partition [" << partitionId
                          << "] opened successfully, applying "
                          << recoveryEvents.size()
                          << " buffered storage events to the partition.";

            for (size_t i = 0; i < recoveryEvents.size(); ++i) {
                rc = fs->processRecoveryEvent(recoveryEvents[i]);
                if (0 != rc) {
                    BMQTSK_ALARMLOG_ALARM("RECOVERY")
                        << d_clusterData_p->identity().description()
                        << ": Partition [" << partitionId
                        << "] failed to apply buffered storage event, rc: "
                        << rc << ". Closing the partition."
                        << BMQTSK_ALARMLOG_END;
                    fs->close();
                    break;  // BREAK
                }
            }

            // Get the latest leaseId and sequence number for this partition.
            if (fs->isOpen()) {
                d_recoveredPrimaryLeaseIds[partitionId] = fs->primaryLeaseId();

                BALL_LOG_INFO
                    << d_clusterData_p->identity().description()
                    << ": Partition [" << partitionId
                    << "] after applying buffered storage events, "
                    << "(recoveryPeerNode, primaryLeaseId, "
                    << "sequenceNumber): ("
                    << (recoveryPeer ? recoveryPeer->nodeDescription()
                                     : "**none**")
                    << ", " << fs->primaryLeaseId() << ", "
                    << fs->sequenceNumber() << ")";
            }
        }
    }

    // Print a summary of the recovered storages if the partition opened
    // successfully.
    bmqu::MemOutStream out;
    if (fs->isOpen()) {
        mqbs::StoragePrintUtil::printRecoveredStorages(
            out,
            &d_storagesLock,
            d_storages[partitionId],
            partitionId,
            d_clusterData_p->identity().description(),
            recoveryStartTime);
        BALL_LOG_INFO << out.str();
    }

    // Bump up the number of partitions for which recovery is complete
    // (irrespective of the recovery 'status' of the 'partitionId').  This must
    // be done at the end, because if all partitions' recovery is complete,
    // their status will be checked in this thread.  This means that we will be
    // invoking 'isOpen()' on all file stores, which is const-thread safe, and
    // we should be good.

    if (++d_numPartitionsRecoveredFully <
        static_cast<int>(d_fileStores.size())) {
        // One or more partitions have yet to recover.
        return;  // RETURN
    }

    out.reset();
    const bool success =
        mqbs::StoragePrintUtil::printStorageRecoveryCompletion(
            out,
            d_fileStores,
            d_clusterData_p->identity().description());
    BALL_LOG_INFO << out.str();

    if (success) {
        // All partitions have opened successfully.  Schedule a recurring event
        // in StorageMgr, which will in turn enqueue an event in each
        // partition's dispatcher thread for GC'ing expired messages as well as
        // cleaning history.

        d_clusterData_p->scheduler().scheduleRecurringEvent(
            &d_gcMessagesEventHandle,
            bsls::TimeInterval(k_GC_MESSAGES_INTERVAL_SECONDS),
            bdlf::BindUtil::bind(&StorageManager::forceFlushFileStores, this));

        d_recoveryStatusCb(0, d_recoveredPrimaryLeaseIds);
    }
    else {
        d_recoveredPrimaryLeaseIds.clear();
        d_recoveryStatusCb(-1, d_recoveredPrimaryLeaseIds);
    }
}

void StorageManager::onPartitionPrimarySync(int partitionId, int status)
{
    // executed by *QUEUE_DISPATCHER* thread associated with 'partitionId'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(0 <= partitionId);
    BSLS_ASSERT_SAFE(partitionId < static_cast<int>(d_fileStores.size()));
    BSLS_ASSERT_SAFE(partitionId <
                     static_cast<int>(d_partitionInfoVec.size()));

    mqbc::StorageUtil::onPartitionPrimarySync(d_fileStores[partitionId].get(),
                                              &d_partitionInfoVec[partitionId],
                                              d_clusterData_p,
                                              d_partitionPrimaryStatusCb,
                                              partitionId,
                                              status);
}

void StorageManager::shutdownCb(int partitionId, bslmt::Latch* latch)
{
    // executed by *QUEUE_DISPATCHER* thread with the specified 'partitionId'
    mqbc::StorageUtil::shutdown(partitionId,
                                latch,
                                &d_fileStores,
                                d_clusterData_p->identity().description(),
                                d_clusterConfig);
}

void StorageManager::queueCreationCb(int*                    status,
                                     int                     partitionId,
                                     const bmqt::Uri&        uri,
                                     const mqbu::StorageKey& queueKey,
                                     const AppIdKeyPairs&    appIdKeyPairs,
                                     bool                    isNewQueue)
{
    // executed by *QUEUE_DISPATCHER* thread associated with 'partitionId'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_fileStores.size()));
    BSLS_ASSERT_SAFE(d_fileStores[partitionId]->inDispatcherThread());

    if (d_cluster_p->isCSLModeEnabled()) {
        // This callback is removed for CSL mode

        return;  // RETURN
    }

    // This routine is executed at replica nodes when they received a queue
    // creation record from the primary in the partition stream.

    if (isNewQueue) {
        mqbc::StorageUtil::registerQueueReplicaDispatched(
            status,
            &d_storages[partitionId],
            &d_storagesLock,
            d_fileStores[partitionId].get(),
            d_domainFactory_p,
            &d_allocators,
            d_clusterData_p->identity().description(),
            partitionId,
            uri,
            queueKey);

        if (*status != 0) {
            return;  // RETURN
        }
    }

    mqbc::StorageUtil::updateQueueReplicaDispatched(
        status,
        &d_storages[partitionId],
        &d_storagesLock,
        &d_appKeysVec[partitionId],
        &d_appKeysLock,
        d_domainFactory_p,
        d_clusterData_p->identity().description(),
        partitionId,
        uri,
        queueKey,
        appIdKeyPairs,
        d_cluster_p->isCSLModeEnabled());
}

void StorageManager::queueDeletionCb(int*                    status,
                                     int                     partitionId,
                                     const bmqt::Uri&        uri,
                                     const mqbu::StorageKey& queueKey,
                                     const mqbu::StorageKey& appKey)
{
    // executed by *QUEUE_DISPATCHER* thread associated with 'partitionId'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_fileStores.size()));
    BSLS_ASSERT_SAFE(d_fileStores[partitionId]->inDispatcherThread());

    if (d_cluster_p->isCSLModeEnabled()) {
        // This callback is removed for CSL mode

        return;  // RETURN
    }

    // This routine is executed at replica nodes when they received a queue
    // deletion record from the primary in the partition stream.

    mqbc::StorageUtil::unregisterQueueReplicaDispatched(
        status,
        &d_storages[partitionId],
        &d_storagesLock,
        d_fileStores[partitionId].get(),
        &d_appKeysVec[partitionId],
        &d_appKeysLock,
        d_clusterData_p->identity().description(),
        partitionId,
        uri,
        queueKey,
        appKey,
        d_cluster_p->isCSLModeEnabled());
}

void StorageManager::recoveredQueuesCb(int                    partitionId,
                                       const QueueKeyInfoMap& queueKeyInfoMap)
{
    // executed by *QUEUE_DISPATCHER* thread associated with 'partitionId'

    // PRECONDITIONS
    const int numPartitions = static_cast<int>(d_fileStores.size());
    BSLS_ASSERT_SAFE(0 <= partitionId && partitionId < numPartitions);
    BSLS_ASSERT_SAFE(d_fileStores[partitionId]->inDispatcherThread());
    BSLS_ASSERT_SAFE(d_numPartitionsRecoveredQueues < numPartitions);

    if (d_cluster_p->isStopping()) {
        BALL_LOG_WARN << d_clusterData_p->identity().description()
                      << " Partition [" << partitionId << "]: "
                      << ": Cluster is stopping; skipping partition recovery.";
        return;  // RETURN
    }

    // Main logic
    mqbc::StorageUtil::recoveredQueuesCb(
        &d_storages[partitionId],
        &d_storagesLock,
        d_fileStores[partitionId].get(),
        &d_appKeysVec[partitionId],
        &d_appKeysLock,
        d_domainFactory_p,
        &d_unrecognizedDomainsLock,
        &d_unrecognizedDomains[partitionId],
        d_clusterData_p->identity().description(),
        partitionId,
        queueKeyInfoMap,
        d_cluster_p->isCSLModeEnabled());

    if (++d_numPartitionsRecoveredQueues < numPartitions) {
        return;  // RETURN
    }

    mqbc::StorageUtil::dumpUnknownRecoveredDomains(
        d_clusterData_p->identity().description(),
        &d_unrecognizedDomainsLock,
        d_unrecognizedDomains);
}

void StorageManager::forceFlushFileStores()
{
    // executed by scheduler's dispatcher thread

    mqbc::StorageUtil::forceFlushFileStores(&d_fileStores);
}

void StorageManager::applyForEachQueue(int                 partitionId,
                                       const QueueFunctor& functor) const
{
    // executed by the *DISPATCHER* thread

    const mqbs::FileStore& fs = fileStore(partitionId);

    if (!fs.isOpen()) {
        return;  // RETURN
    }

    fs.applyForEachQueue(functor);
}

void StorageManager::setPrimaryForPartitionDispatched(
    int                  partitionId,
    mqbnet::ClusterNode* primaryNode,
    unsigned int         primaryLeaseId,
    const ClusterNodes&  peers)
{
    // executed by *DISPATCHER* thread

    BSLS_ASSERT_SAFE(0 <= partitionId);
    BSLS_ASSERT_SAFE(d_fileStores.size() >
                     static_cast<unsigned int>(partitionId));
    BSLS_ASSERT_SAFE(d_partitionInfoVec.size() >
                     static_cast<unsigned int>(partitionId));

    mqbs::FileStore* fs    = d_fileStores[partitionId].get();
    PartitionInfo&   pinfo = d_partitionInfoVec[partitionId];

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << " Partition [" << partitionId
                  << "]: received partition/primary info. Primary: "
                  << (primaryNode ? primaryNode->nodeDescription()
                                  : "**null**")
                  << ", leaseId: " << primaryLeaseId << ". Current primary: "
                  << (pinfo.primary() ? pinfo.primary()->nodeDescription()
                                      : "** null **")
                  << ", leaseId: " << pinfo.primaryLeaseId();

    if (primaryLeaseId < pinfo.primaryLeaseId()) {
        BMQTSK_ALARMLOG_ALARM("REPLICATION")
            << d_clusterData_p->identity().description() << " Partition ["
            << partitionId
            << "]: Smaller new primaryLeaseId specified: " << primaryLeaseId
            << ", current primaryLeaseId: "
            << ". Ignoring this request. Specified primary node: "
            << primaryNode->nodeDescription() << ", current primary node: "
            << (pinfo.primary() ? pinfo.primary()->nodeDescription()
                                : "** null **")
            << BMQTSK_ALARMLOG_END;
        return;  // RETURN
    }

    if (pinfo.primaryLeaseId() == primaryLeaseId) {
        if (pinfo.primary() == primaryNode) {
            // A node can be informed again about same primary<->partition
            // mapping.

            if (primaryNode == d_clusterData_p->membership().selfNode() &&
                bmqp_ctrlmsg::PrimaryStatus::E_ACTIVE ==
                    pinfo.primaryStatus()) {
                // If self is an active primary, force-issue a primary status
                // advisory as well as a SyncPt..  in case of network issues,
                // one or more peers may not be aware that primary is already
                // active.

                mqbc::StorageUtil::forceIssueAdvisoryAndSyncPt(d_clusterData_p,
                                                               fs,
                                                               NULL,
                                                               pinfo);
            }
            // else: self is not primary, or it is, but not an active one.  In
            // either case, nothing to do.

            // Simply return here, because self node was told again about the
            // same partition<->primary mapping.

            return;  // RETURN
        }

        if (0 != pinfo.primary()) {
            // Same leaseId, different node.  This is an error.

            BMQTSK_ALARMLOG_ALARM("REPLICATION")
                << d_clusterData_p->identity().description() << " Partition ["
                << partitionId << "]: Same primaryLeaseId specified ["
                << primaryLeaseId
                << "] with a different primary node. Current primary: "
                << pinfo.primary()->nodeDescription()
                << ", specified primary: " << primaryNode->nodeDescription()
                << ". Ignoring this request." << BMQTSK_ALARMLOG_END;
            return;  // RETURN
        }

        // 'pinfo.primary()' is null.  This could occur if this node was
        // started so it had the latest primary leaseId, but no information for
        // the partition <-> primary mapping.
    }

    // Keep track of new (node, leaseId) for later use.

    pinfo.setPrimary(primaryNode);
    pinfo.setPrimaryLeaseId(primaryLeaseId);
    pinfo.setPrimaryStatus(bmqp_ctrlmsg::PrimaryStatus::E_PASSIVE);

    if (primaryNode != d_clusterData_p->membership().selfNode()) {
        return;  // RETURN
    }

    // Self is the specified primary.  Self must be AVAILABLE.  We can't read
    // cluster state from this thread, so the next best thing is to ensure that
    // partition is not in recovery.  This is not sufficient though as node
    // could be STOPPING, but we don't check for that now.

    BSLS_ASSERT_SAFE(!d_recoveryManager_mp->isRecoveryInProgress(partitionId));

    // Inform recovery manager to initiate partition sync.

    d_recoveryManager_mp->startPartitionPrimarySync(
        fs,
        peers,
        bdlf::BindUtil::bind(&StorageManager::onPartitionPrimarySync,
                             this,
                             bdlf::PlaceHolders::_1,    // partitionId
                             bdlf::PlaceHolders::_2));  // status
}

void StorageManager::clearPrimaryForPartitionDispatched(
    int                  partitionId,
    mqbnet::ClusterNode* primary)
{
    // executed by *DISPATCHER* thread

    // PRECONDITION
    BSLS_ASSERT_SAFE(0 <= partitionId);
    BSLS_ASSERT_SAFE(d_fileStores.size() >
                     static_cast<unsigned int>(partitionId));
    BSLS_ASSERT_SAFE(d_partitionInfoVec.size() >
                     static_cast<unsigned int>(partitionId));

    mqbs::FileStore* fs    = d_fileStores[partitionId].get();
    PartitionInfo&   pinfo = d_partitionInfoVec[partitionId];

    mqbc::StorageUtil::clearPrimaryForPartition(
        fs,
        &pinfo,
        d_clusterData_p->identity().description(),
        partitionId,
        primary);
}

void StorageManager::processStorageEventDispatched(
    int                                 partitionId,
    const bsl::shared_ptr<bdlbb::Blob>& blob,
    mqbnet::ClusterNode*                source)
{
    // executed by *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(0 <= partitionId);
    BSLS_ASSERT_SAFE(d_fileStores.size() >
                     static_cast<unsigned int>(partitionId));

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!d_isStarted)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        BALL_LOG_INFO << d_clusterData_p->identity().description()
                      << " Dropping storage event as storage has been closed.";
        return;  // RETURN
    }

    mqbs::FileStore* fs = d_fileStores[static_cast<size_t>(partitionId)].get();
    BSLS_ASSERT_SAFE(fs);

    const PartitionInfo& pinfo = d_partitionInfoVec[partitionId];
    BSLS_ASSERT_SAFE(pinfo.primary() == source);
    BSLS_ASSERT_SAFE(pinfo.primaryStatus() ==
                     bmqp_ctrlmsg::PrimaryStatus::E_ACTIVE);
    (void)pinfo;  // silence compiler warning

    if (d_recoveryManager_mp->isRecoveryInProgress(partitionId)) {
        d_recoveryManager_mp->processStorageEvent(partitionId, blob, source);
        return;  // RETURN
    }

    if (!fs->isOpen()) {
        // Partition has been closed (because cluster has been stopped).  Note
        // that if self node is recovering, partition will be closed in that
        // case as well.  So we check the recovery status above, before
        // checking whether partition is open or not.

        return;  // RETURN
    }

    fs->processStorageEvent(blob, false /* isPartitionSyncEvent */, source);
}

void StorageManager::processPartitionSyncEvent(
    const mqbi::DispatcherStorageEvent& event)
{
    // executed by the *CLUSTER DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_dispatcher_p->inDispatcherThread(d_cluster_p));

    mqbnet::ClusterNode* source = event.clusterNode();
    bmqp::Event          rawEvent(event.blob().get(), d_allocator_p);
    BSLS_ASSERT_SAFE(rawEvent.isPartitionSyncEvent());

    // A partition-sync event is received in one of the following scenarios:
    // 1) The chosen syncing peer ('source') sends missing storage events to
    //    the newly chosen primary (self).
    // 2) A newly chosen primary ('source') sends missing storage events to
    //    replica (self).
    if (d_clusterData_p->membership().selfNodeStatus() ==
        bmqp_ctrlmsg::NodeStatus::E_STARTING) {
        // If its (1), self node cannot be STARTING.  If its (2), there is no
        // way self node can process this event because its not AVAILABLE yet,
        // and is most likely recovering.
        return;  // RETURN
    }

    const unsigned int pid = mqbc::StorageUtil::extractPartitionId<false>(
        rawEvent);
    BSLS_ASSERT_SAFE(pid < d_fileStores.size());

    // Ensure that 'pid' is valid.
    if (pid >= d_clusterState.partitions().size()) {
        BMQTSK_ALARMLOG_ALARM("STORAGE")
            << d_cluster_p->description()
            << ": Received partition-sync event from node "
            << source->nodeDescription() << " with invalid"
            << " partitionId: " << pid << ". Ignoring entire event."
            << BMQTSK_ALARMLOG_END;
        return;  // RETURN
    }

    PartitionInfo                    pinfo;
    const ClusterStatePartitionInfo& cspinfo = d_clusterState.partition(pid);
    pinfo.setPrimary(cspinfo.primaryNode());
    pinfo.setPrimaryLeaseId(cspinfo.primaryLeaseId());
    pinfo.setPrimaryStatus(cspinfo.primaryStatus());
    if (!mqbc::StorageUtil::validatePartitionSyncEvent(rawEvent,
                                                       pid,
                                                       source,
                                                       pinfo,
                                                       *d_clusterData_p,
                                                       false)  // isFSMWorkflow
    ) {
        return;  // RETURN
    }

    mqbs::FileStore* fs = d_fileStores[pid].get();
    BSLS_ASSERT_SAFE(fs);

    fs->execute(bdlf::BindUtil::bind(
        &StorageManager::processPartitionSyncEventDispatched,
        this,
        pid,
        event.blob(),
        source));
}

void StorageManager::processPartitionSyncEventDispatched(
    int                                 partitionId,
    const bsl::shared_ptr<bdlbb::Blob>& blob,
    mqbnet::ClusterNode*                source)
{
    // executed by *DISPATCHER* thread

    BSLS_ASSERT_SAFE(0 <= partitionId);
    BSLS_ASSERT_SAFE(d_fileStores.size() >
                     static_cast<unsigned int>(partitionId));

    // Cluster forwards a partition-sync event to the storage only if node is
    // AVAILABLE, which means that this partition (or any partition for that
    // matter) cannot be in recovery.

    BSLS_ASSERT_SAFE(!d_recoveryManager_mp->isRecoveryInProgress(partitionId));

    const PartitionInfo& pinfo = d_partitionInfoVec[partitionId];

    if (pinfo.primary() == d_clusterData_p->membership().selfNode()) {
        // If self is primary for this partition, it must be in partition-sync
        // step, source must be the syncing peer, and self must be a passive
        // primary.

        if (!d_recoveryManager_mp->isPrimarySyncInProgress(partitionId)) {
            BALL_LOG_ERROR << d_clusterData_p->identity().description()
                           << " Partition [" << partitionId
                           << "]: received a partition sync event from: "
                           << source->nodeDescription()
                           << ", while self is not under partition-sync.";
            return;  // RETURN
        }

        if (source != d_recoveryManager_mp->primarySyncPeer(partitionId)) {
            BALL_LOG_ERROR << d_clusterData_p->identity().description()
                           << " Partition [" << partitionId
                           << "]: received a partition sync event from: "
                           << source->nodeDescription()
                           << ", while partition-sync peer is: "
                           << source->nodeDescription();
            return;  // RETURN
        }

        if (bmqp_ctrlmsg::PrimaryStatus::E_PASSIVE != pinfo.primaryStatus()) {
            BALL_LOG_ERROR << d_clusterData_p->identity().description()
                           << " Partition [" << partitionId
                           << "]: received a partition sync event from: "
                           << source->nodeDescription()
                           << ", while self is ACTIVE primary.";
            return;  // RETURN
        }
    }
    else if (pinfo.primary() == source) {
        // Self is replica for this partition, so it must perceive 'source' as
        // PASSIVE primary (recall that primary doesn't send partition-sync
        // events once it has transitioned to ACTIVE state).

        if (bmqp_ctrlmsg::PrimaryStatus::E_PASSIVE != pinfo.primaryStatus()) {
            BALL_LOG_ERROR << d_clusterData_p->identity().description()
                           << " Partition [" << partitionId
                           << "]: received a partition sync event from: "
                           << source->nodeDescription()
                           << ", but self perceives sender (primary) as "
                           << "ACTIVE.";
            return;  // RETURN
        }
    }
    else {
        BALL_LOG_ERROR << d_clusterData_p->identity().description()
                       << " Partition [" << partitionId
                       << "]: received a partition sync event from: "
                       << source->nodeDescription()
                       << ", but neither self is primary nor the sender is "
                       << "perceived as the primary.";
        return;  // RETURN
    }

    // A partition-sync event is processed by the file store just like a
    // storage event.

    mqbs::FileStore* fs =
        d_fileStores[static_cast<unsigned int>(partitionId)].get();
    BSLS_ASSERT_SAFE(fs);

    fs->processStorageEvent(blob, true /* isPartitionSyncEvent */, source);
}

void StorageManager::processStorageSyncRequestDispatched(
    int                                 partitionId,
    const bmqp_ctrlmsg::ControlMessage& message,
    mqbnet::ClusterNode*                source)
{
    // executed by *DISPATCHER* thread

    BSLS_ASSERT_SAFE(0 <= partitionId);
    BSLS_ASSERT_SAFE(d_fileStores.size() >
                     static_cast<unsigned int>(partitionId));

    mqbs::FileStore* fs =
        d_fileStores[static_cast<unsigned int>(partitionId)].get();
    BSLS_ASSERT_SAFE(fs);

    d_recoveryManager_mp->processStorageSyncRequest(message, source, fs);
}

void StorageManager::processPartitionSyncStateRequestDispatched(
    int                                 partitionId,
    const bmqp_ctrlmsg::ControlMessage& message,
    mqbnet::ClusterNode*                source)
{
    // executed by *DISPATCHER* thread

    // PRECONDITION
    BSLS_ASSERT_SAFE(!d_recoveryManager_mp->isRecoveryInProgress(partitionId));

    const PartitionInfo& pinfo = d_partitionInfoVec[partitionId];

    if (pinfo.primary() != source && 0 != pinfo.primary()) {
        // This node does not perceive 'source' as the primary of partition,
        // but will reply anyways.

        BALL_LOG_WARN << d_clusterData_p->identity().description()
                      << " Partition [" << partitionId
                      << "]: received partition-sync state request: "
                      << message.choice()
                             .clusterMessage()
                             .choice()
                             .partitionSyncStateQuery()
                      << ", from node: " << source->nodeDescription()
                      << ", but self node perceives node "
                      << pinfo.primary()->nodeDescription() << " as primary. "
                      << "Self node will still reply.";
    }

    // TBD: when does a replica schedule partition sync with primary if it
    // misses
    //      the original syncstate request?  When it has received partition-
    //      primary advisory from the leader, AND self node is AVAILABLE, AND
    //      self (replica) receives a message from the new primary which it
    //      will reject in StorageMgr.processStorageEventDispatched() if
    //      PartitionInfo.d_isPrimaryActive flag is set to false.  How to
    //      recover though?  Close fs and initiate recovery?  Keep fs open,
    //      start buffering and then same as recovery?

    d_recoveryManager_mp->processPartitionSyncStateRequest(
        message,
        source,
        d_fileStores[partitionId].get());
}

void StorageManager::processPartitionSyncDataRequestDispatched(
    int                                 partitionId,
    const bmqp_ctrlmsg::ControlMessage& message,
    mqbnet::ClusterNode*                source)
{
    // executed by *DISPATCHER* thread

    d_recoveryManager_mp->processPartitionSyncDataRequest(
        message,
        source,
        d_fileStores[partitionId].get());
}

void StorageManager::processPartitionSyncDataRequestStatusDispatched(
    BSLS_ANNOTATION_UNUSED int          partitionId,
    const bmqp_ctrlmsg::ControlMessage& message,
    mqbnet::ClusterNode*                source)
{
    // executed by *DISPATCHER* thread

    d_recoveryManager_mp->processPartitionSyncDataRequestStatus(message,
                                                                source);
}

void StorageManager::processShutdownEventDispatched(int partitionId)
{
    // executed by *QUEUE_DISPATCHER* thread with the specified 'partitionId'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_fileStores.size()));
    BSLS_ASSERT_SAFE(d_fileStores[partitionId]->inDispatcherThread());

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << " Partition [" << partitionId
                  << "]: received shutdown event.";

    // Inform RecoveryMgr, which may cancel ongoing recovery or prevent a
    // recovery to be initiated in future.
    d_recoveryManager_mp->processShutdownEvent(partitionId);

    mqbc::StorageUtil::processShutdownEventDispatched(
        d_clusterData_p,
        &d_partitionInfoVec[partitionId],
        d_fileStores[partitionId].get(),
        partitionId);
}

// CREATORS
StorageManager::StorageManager(
    const mqbcfg::ClusterDefinition& clusterConfig,
    mqbi::Cluster*                   cluster,
    mqbc::ClusterData*               clusterData,
    const mqbc::ClusterState&        clusterState,
    const RecoveryStatusCb&          recoveryStatusCb,
    const PartitionPrimaryStatusCb&  partitionPrimaryStatusCb,
    mqbi::DomainFactory*             domainFactory,
    mqbi::Dispatcher*                dispatcher,
    bdlmt::FixedThreadPool*          threadPool,
    bslma::Allocator*                allocator)
: d_allocator_p(allocator)
, d_allocators(d_allocator_p)
, d_isStarted(false)
, d_lowDiskspaceWarning(false)
, d_unrecognizedDomainsLock()
, d_unrecognizedDomains(allocator)
, d_blobSpPool_p(&clusterData->blobSpPool())
, d_domainFactory_p(domainFactory)
, d_dispatcher_p(dispatcher)
, d_clusterConfig(clusterConfig)
, d_cluster_p(cluster)
, d_clusterData_p(clusterData)
, d_clusterState(clusterState)
, d_recoveryManager_mp()
, d_fileStores(allocator)
, d_miscWorkThreadPool_p(threadPool)
, d_numPartitionsRecoveredFully(0)
, d_numPartitionsRecoveredQueues(0)
, d_recoveryStatusCb(recoveryStatusCb)
, d_partitionPrimaryStatusCb(partitionPrimaryStatusCb)
, d_storagesLock()
, d_storages(allocator)
, d_appKeysLock()
, d_appKeysVec(allocator)
, d_storageMonitorEventHandle()
, d_gcMessagesEventHandle()
, d_recoveredPrimaryLeaseIds(allocator)
, d_partitionInfoVec(allocator)
, d_minimumRequiredDiskSpace(0)
, d_replicationFactor(0)
{
    BSLS_ASSERT_SAFE(allocator);
    BSLS_ASSERT_SAFE(d_clusterData_p);
    BSLS_ASSERT_SAFE(d_cluster_p);
    BSLS_ASSERT_SAFE(d_recoveryStatusCb);
    BSLS_ASSERT_SAFE(d_partitionPrimaryStatusCb);

    const mqbcfg::PartitionConfig& partitionCfg =
        d_clusterConfig.partitionConfig();

    d_unrecognizedDomains.resize(partitionCfg.numPartitions());
    d_fileStores.resize(partitionCfg.numPartitions());
    d_storages.resize(partitionCfg.numPartitions());
    d_recoveredPrimaryLeaseIds.resize(partitionCfg.numPartitions());
    d_partitionInfoVec.resize(partitionCfg.numPartitions());
    d_appKeysVec.resize(partitionCfg.numPartitions());

    d_minimumRequiredDiskSpace = mqbc::StorageUtil::findMinReqDiskSpace(
        partitionCfg);

    // Set the default replication-factor to one more than half of the cluster.
    // Do this here (rather than in the initializer-list) to avoid accessing
    // 'd_cluster_p' before the above non-nullness check.
    d_replicationFactor = (d_cluster_p->netCluster().nodes().size() / 2) + 1;
}

StorageManager::~StorageManager()
{
    BSLS_ASSERT_SAFE(!d_isStarted &&
                     "'stop()' must be called before the destructor");

    BALL_LOG_INFO << "StorageManager ["
                  << d_clusterData_p->identity().description()
                  << "]: destructor.";
}

void StorageManager::registerQueue(const bmqt::Uri&        uri,
                                   const mqbu::StorageKey& queueKey,
                                   int                     partitionId,
                                   const AppIdKeyPairs&    appIdKeyPairs,
                                   mqbi::Domain*           domain)
{
    // executed by the *CLUSTER DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_dispatcher_p->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(uri.isValid());
    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_fileStores.size()));
    BSLS_ASSERT_SAFE(domain);
    if (!d_cluster_p->isCSLModeEnabled()) {
        BSLS_ASSERT_SAFE(appIdKeyPairs.empty());
    }

    mqbc::StorageUtil::registerQueue(d_cluster_p,
                                     d_dispatcher_p,
                                     &d_storages[partitionId],
                                     &d_storagesLock,
                                     d_fileStores[partitionId].get(),
                                     &d_appKeysVec[partitionId],
                                     &d_appKeysLock,
                                     &d_allocators,
                                     processorForPartition(partitionId),
                                     uri,
                                     queueKey,
                                     d_clusterData_p->identity().description(),
                                     partitionId,
                                     appIdKeyPairs,
                                     domain);
}

void StorageManager::unregisterQueue(const bmqt::Uri& uri, int partitionId)
{
    // executed by the *CLUSTER DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_dispatcher_p->inDispatcherThread(d_cluster_p));

    // Dispatch the un-registration to appropriate thread.

    mqbi::DispatcherEvent* queueEvent = d_dispatcher_p->getEvent(
        mqbi::DispatcherClientType::e_QUEUE);

    (*queueEvent)
        .makeDispatcherEvent()
        .setCallback(
            bdlf::BindUtil::bind(&mqbc::StorageUtil::unregisterQueueDispatched,
                                 bdlf::PlaceHolders::_1,  // processor
                                 d_fileStores[partitionId].get(),
                                 &d_storages[partitionId],
                                 &d_storagesLock,
                                 d_clusterData_p,
                                 partitionId,
                                 bsl::cref(d_partitionInfoVec[partitionId]),
                                 uri));

    d_fileStores[partitionId]->dispatchEvent(queueEvent);
}

int StorageManager::updateQueuePrimary(const bmqt::Uri&        uri,
                                       const mqbu::StorageKey& queueKey,
                                       int                     partitionId,
                                       const AppIdKeyPairs&    addedIdKeyPairs,
                                       const AppIdKeyPairs& removedIdKeyPairs)
{
    // executed by *QUEUE_DISPATCHER* thread with the specified 'partitionId'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_fileStores.size()));
    BSLS_ASSERT_SAFE(d_fileStores[partitionId]->inDispatcherThread());

    return mqbc::StorageUtil::updateQueuePrimary(
        &d_storages[partitionId],
        &d_storagesLock,
        d_fileStores[partitionId].get(),
        &d_appKeysVec[partitionId],
        &d_appKeysLock,
        d_clusterData_p->identity().description(),
        uri,
        queueKey,
        partitionId,
        addedIdKeyPairs,
        removedIdKeyPairs,
        d_cluster_p->isCSLModeEnabled());
}

void StorageManager::registerQueueReplica(int                     partitionId,
                                          const bmqt::Uri&        uri,
                                          const mqbu::StorageKey& queueKey,
                                          mqbi::Domain*           domain,
                                          bool allowDuplicate)
{
    // executed by the *CLUSTER DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_dispatcher_p->inDispatcherThread(d_cluster_p));

    if (!d_cluster_p->isCSLModeEnabled()) {
        BALL_LOG_ERROR << "#CSL_MODE_MIX "
                       << "StorageManager::registerQueueReplica() should only "
                       << "be invoked in CSL mode.";

        return;  // RETURN
    }

    // This routine is executed at follower nodes upon commit callback of Queue
    // Assignment Advisory from the leader.

    mqbi::DispatcherEvent* queueEvent = d_dispatcher_p->getEvent(
        mqbi::DispatcherClientType::e_QUEUE);

    (*queueEvent)
        .makeDispatcherEvent()
        .setCallback(bdlf::BindUtil::bind(
            &mqbc::StorageUtil::registerQueueReplicaDispatched,
            static_cast<int*>(0),
            &d_storages[partitionId],
            &d_storagesLock,
            d_fileStores[partitionId].get(),
            d_domainFactory_p,
            &d_allocators,
            d_clusterData_p->identity().description(),
            partitionId,
            uri,
            queueKey,
            domain,
            allowDuplicate));

    d_fileStores[partitionId]->dispatchEvent(queueEvent);
}

void StorageManager::unregisterQueueReplica(int              partitionId,
                                            const bmqt::Uri& uri,
                                            const mqbu::StorageKey& queueKey,
                                            const mqbu::StorageKey& appKey)
{
    // executed by the *CLUSTER DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_dispatcher_p->inDispatcherThread(d_cluster_p));

    if (!d_cluster_p->isCSLModeEnabled()) {
        BALL_LOG_ERROR << "#CSL_MODE_MIX "
                       << "StorageManager::unregisterQueueReplica() should "
                       << "only be invoked in CSL mode.";

        return;  // RETURN
    }

    // This routine is executed at follower nodes upon commit callback of Queue
    // Unassigned Advisory or Queue Update Advisory from the leader.

    mqbi::DispatcherEvent* queueEvent = d_dispatcher_p->getEvent(
        mqbi::DispatcherClientType::e_QUEUE);

    (*queueEvent)
        .makeDispatcherEvent()
        .setCallback(bdlf::BindUtil::bind(
            &mqbc::StorageUtil::unregisterQueueReplicaDispatched,
            static_cast<int*>(0),
            &d_storages[partitionId],
            &d_storagesLock,
            d_fileStores[partitionId].get(),
            &d_appKeysVec[partitionId],
            &d_appKeysLock,
            d_clusterData_p->identity().description(),
            partitionId,
            uri,
            queueKey,
            appKey,
            d_cluster_p->isCSLModeEnabled()));

    d_fileStores[partitionId]->dispatchEvent(queueEvent);
}

void StorageManager::updateQueueReplica(int                     partitionId,
                                        const bmqt::Uri&        uri,
                                        const mqbu::StorageKey& queueKey,
                                        const AppIdKeyPairs&    appIdKeyPairs,
                                        mqbi::Domain*           domain,
                                        bool                    allowDuplicate)
{
    // executed by the *CLUSTER DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_dispatcher_p->inDispatcherThread(d_cluster_p));

    if (!d_cluster_p->isCSLModeEnabled()) {
        BALL_LOG_ERROR << "#CSL_MODE_MIX "
                       << "StorageManager::updateQueueReplica() should only "
                       << "be invoked in CSL mode.";

        return;  // RETURN
    }

    // This routine is executed at follower nodes upon commit callback of Queue
    // Queue Update Advisory from the leader.

    mqbi::DispatcherEvent* queueEvent = d_dispatcher_p->getEvent(
        mqbi::DispatcherClientType::e_QUEUE);

    (*queueEvent)
        .makeDispatcherEvent()
        .setCallback(bdlf::BindUtil::bind(
            &mqbc::StorageUtil::updateQueueReplicaDispatched,
            static_cast<int*>(0),
            &d_storages[partitionId],
            &d_storagesLock,
            &d_appKeysVec[partitionId],
            &d_appKeysLock,
            d_domainFactory_p,
            d_clusterData_p->identity().description(),
            partitionId,
            uri,
            queueKey,
            appIdKeyPairs,
            d_cluster_p->isCSLModeEnabled(),
            domain,
            allowDuplicate));

    d_fileStores[partitionId]->dispatchEvent(queueEvent);
}

mqbu::StorageKey StorageManager::generateAppKey(const bsl::string& appId,
                                                int                partitionId)
{
    // executed by *QUEUE_DISPATCHER* thread with the specified 'partitionId'
    // or by *CLUSTER DISPATCHER* thread.

    return mqbc::StorageUtil::generateAppKey(&d_appKeysVec[partitionId],
                                             &d_appKeysLock,
                                             appId);
}

void StorageManager::setQueue(mqbi::Queue*     queue,
                              const bmqt::Uri& uri,
                              int              partitionId)
{
    // executed by the *CLUSTER DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_dispatcher_p->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(uri.isValid());

    // Note that 'queue' can be null, which is a valid scenario.

    if (queue) {
        BSLS_ASSERT_SAFE(queue->uri() == uri);
    }

    mqbi::DispatcherEvent* queueEvent = d_dispatcher_p->getEvent(
        mqbi::DispatcherClientType::e_QUEUE);

    (*queueEvent)
        .makeDispatcherEvent()
        .setCallback(
            bdlf::BindUtil::bind(&mqbc::StorageUtil::setQueueDispatched,
                                 &d_storages[partitionId],
                                 &d_storagesLock,
                                 bdlf::PlaceHolders::_1,  // processor
                                 d_clusterData_p->identity().description(),
                                 partitionId,
                                 uri,
                                 queue));

    d_fileStores[partitionId]->dispatchEvent(queueEvent);
    ;
}

void StorageManager::setQueueRaw(mqbi::Queue*     queue,
                                 const bmqt::Uri& uri,
                                 int              partitionId)
{
    // executed by *QUEUE_DISPATCHER* thread with the specified 'partitionId'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(queue);
    BSLS_ASSERT_SAFE(d_cluster_p->dispatcher()->inDispatcherThread(queue));

    mqbc::StorageUtil::setQueueDispatched(
        &d_storages[partitionId],
        &d_storagesLock,
        processorForPartition(partitionId),
        d_clusterData_p->identity().description(),
        partitionId,
        uri,
        queue);
}

int StorageManager::start(bsl::ostream& errorDescription)
{
    // executed by the *CLUSTER DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_dispatcher_p->inDispatcherThread(d_cluster_p));

    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS                        = 0,
        rc_PARTITION_LOCATION_NONEXISTENT = -1,
        rc_RECOVERY_MANAGER_FAILURE       = -2,
        rc_FILE_STORE_OPEN_FAILURE        = -3,
        rc_FILE_STORE_RECOVERY_FAILURE    = -4,
        rc_NOT_ENOUGH_DISK_SPACE          = -5
    };

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << ": Starting StorageManager";

    // For convenience:
    const mqbcfg::PartitionConfig& partitionCfg =
        d_clusterConfig.partitionConfig();

    int rc = mqbc::StorageUtil::validatePartitionDirectory(partitionCfg,
                                                           errorDescription);
    if (rc != rc_SUCCESS) {
        return rc * 10 + rc_PARTITION_LOCATION_NONEXISTENT;  // RETURN
    }

    rc = mqbc::StorageUtil::validateDiskSpace(partitionCfg,
                                              *d_clusterData_p,
                                              d_minimumRequiredDiskSpace);
    if (rc != rc_SUCCESS) {
        return rc * 10 + rc_NOT_ENOUGH_DISK_SPACE;  // RETURN
    }

    // Schedule a periodic event (every minute) which monitors storage (disk
    // space, archive clean up, etc).
    d_clusterData_p->scheduler().scheduleRecurringEvent(
        &d_storageMonitorEventHandle,
        bsls::TimeInterval(bdlt::TimeUnitRatio::k_SECONDS_PER_MINUTE),
        bdlf::BindUtil::bind(&mqbc::StorageUtil::storageMonitorCb,
                             &d_lowDiskspaceWarning,
                             &d_isStarted,
                             d_minimumRequiredDiskSpace,
                             d_clusterData_p->identity().description(),
                             d_clusterConfig.partitionConfig()));

    rc = mqbc::StorageUtil::assignPartitionDispatcherThreads(
        d_miscWorkThreadPool_p,
        d_clusterData_p,
        *d_cluster_p,
        d_dispatcher_p,
        partitionCfg,
        &d_fileStores,
        d_blobSpPool_p,
        &d_allocators,
        errorDescription,
        d_replicationFactor,
        bdlf::BindUtil::bind(&StorageManager::recoveredQueuesCb,
                             this,
                             bdlf::PlaceHolders::_1,   // partitionId
                             bdlf::PlaceHolders::_2),  // queueKeyUriMap)
        bdlf::BindUtil::bind(&StorageManager::queueCreationCb,
                             this,
                             bdlf::PlaceHolders::_1,   // status
                             bdlf::PlaceHolders::_2,   // partitionId
                             bdlf::PlaceHolders::_3,   // QueueUri
                             bdlf::PlaceHolders::_4,   // QueueKey
                             bdlf::PlaceHolders::_5,   // AppIdKeyPairs
                             bdlf::PlaceHolders::_6),  // IsNewQueue)
        bdlf::BindUtil::bind(&StorageManager::queueDeletionCb,
                             this,
                             bdlf::PlaceHolders::_1,    // status
                             bdlf::PlaceHolders::_2,    // partitionId
                             bdlf::PlaceHolders::_3,    // QueueUri
                             bdlf::PlaceHolders::_4,    // QueueKey
                             bdlf::PlaceHolders::_5));  // AppKey

    BALL_LOG_INFO_BLOCK
    {
        BALL_LOG_OUTPUT_STREAM << d_clusterData_p->identity().description()
                               << ": Partition/processor mapping:\n";
        BALL_LOG_OUTPUT_STREAM << "    PartitionId    ProcessorID";
        for (int i = 0; i < partitionCfg.numPartitions(); ++i) {
            BALL_LOG_OUTPUT_STREAM << "\n         " << i << "             "
                                   << d_fileStores[i]->processorId();
        }
    }

    mqbs::DataStoreConfig dsCfg;
    dsCfg.setPreallocate(partitionCfg.preallocate())
        .setPrefaultPages(partitionCfg.prefaultPages())
        .setLocation(partitionCfg.location())
        .setArchiveLocation(partitionCfg.archiveLocation())
        .setNodeId(d_clusterData_p->membership().selfNode()->nodeId())
        .setMaxDataFileSize(partitionCfg.maxDataFileSize())
        .setMaxJournalFileSize(partitionCfg.maxJournalFileSize())
        .setMaxQlistFileSize(partitionCfg.maxQlistFileSize());
    // Only relevant fields of data store config are set.

    // Get named allocator from associated bmqma::CountingAllocatorStore
    bslma::Allocator* recoveryManagerAllocator = d_allocators.get(
        "RecoveryManager");

    d_recoveryManager_mp.load(new (*recoveryManagerAllocator)
                                  RecoveryManager(d_clusterConfig,
                                                  d_cluster_p,
                                                  d_clusterData_p,
                                                  dsCfg,
                                                  d_dispatcher_p,
                                                  recoveryManagerAllocator),
                              recoveryManagerAllocator);

    rc = d_recoveryManager_mp->start(errorDescription);
    if (rc != 0) {
        BALL_LOG_ERROR << d_clusterData_p->identity().description()
                       << ": Failed to start recovery manager, rc: " << rc;
        return 10 * rc + rc_RECOVERY_MANAGER_FAILURE;  // RETURN
    }

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << ": Enqueuing events in recovery manager for each "
                  << "partition to initiate first phase of storage recovery.";

    // In initial implementation, StorageMgr used to block until first phase of
    // recovery is complete (1st phase of recovery == retrieving last valid
    // sync pt from local storage.  But in case broker crashed in previous
    // instance, retrieving last syncPt took a few seconds because we had to
    // scan large files which were mostly empty (or had garbage).  This blocked
    // this thread (cluster dispatcher thread) for a few seconds, which in turn
    // delayed the processing of leader advisory, which is not desirable (but
    // not a deal-breaker though).  So we don't block for 1st phase of recovery
    // to complete anymore.

    BALL_LOG_INFO_BLOCK
    {
        printRecoveryBanner(BALL_LOG_OUTPUT_STREAM,
                            d_cluster_p->description());
    }

    for (unsigned int i = 0; i < d_fileStores.size(); ++i) {
        mqbs::FileStore* fs = d_fileStores[i].get();
        BSLS_ASSERT_SAFE(fs);

        fs->execute(bdlf::BindUtil::bind(&StorageManager::startRecoveryCb,
                                         this,
                                         static_cast<int>(i)));  // partitionId
    }

    d_isStarted = true;
    return rc_SUCCESS;
}

void StorageManager::stop()
{
    // executed by cluster *DISPATCHER* thread

    // PRECONDITION
    BSLS_ASSERT_SAFE(d_dispatcher_p->inDispatcherThread(d_cluster_p));

    if (!d_isStarted) {
        return;  // RETURN
    }

    d_isStarted = false;

    d_clusterData_p->scheduler().cancelEventAndWait(&d_gcMessagesEventHandle);

    d_clusterData_p->scheduler().cancelEventAndWait(
        &d_storageMonitorEventHandle);
    d_recoveryManager_mp->stop();

    mqbc::StorageUtil::stop(
        &d_fileStores,
        d_clusterData_p->identity().description(),
        bdlf::BindUtil::bind(&StorageManager::shutdownCb,
                             this,
                             bdlf::PlaceHolders::_1,    // partitionId
                             bdlf::PlaceHolders::_2));  // latch
}

void StorageManager::initializeQueueKeyInfoMap(
    BSLS_ANNOTATION_UNUSED const mqbc::ClusterState& clusterState)
{
    // executed by cluster *DISPATCHER* thread

    // PRECONDITION
    BSLS_ASSERT_SAFE(d_dispatcher_p->inDispatcherThread(d_cluster_p));

    BSLS_ASSERT_OPT(false && "Only the FSM version of this method from "
                             "mqbc::StorageManager should be invoked.");
}

void StorageManager::setPrimaryForPartition(int                  partitionId,
                                            mqbnet::ClusterNode* primaryNode,
                                            unsigned int primaryLeaseId)
{
    // executed by cluster *DISPATCHER* thread

    BSLS_ASSERT_SAFE(
        d_dispatcher_p->inDispatcherThread(&d_clusterData_p->cluster()));
    BSLS_ASSERT_SAFE(0 <= partitionId);
    BSLS_ASSERT_SAFE(primaryNode);

    if (d_clusterData_p->membership().selfNode() == primaryNode &&
        d_clusterData_p->membership().selfNodeStatus() !=
            bmqp_ctrlmsg::NodeStatus::E_AVAILABLE) {
        BALL_LOG_INFO << d_clusterData_p->identity().description()
                      << " Partition [" << partitionId
                      << "]: proposed primary is self but self is not "
                      << "AVAILABLE. Self status: "
                      << d_clusterData_p->membership().selfNodeStatus()
                      << ". Proposed leaseId: " << primaryLeaseId;
        return;  // RETURN
    }

    unsigned int pid = static_cast<unsigned int>(partitionId);
    BSLS_ASSERT_SAFE(d_fileStores.size() > pid);

    mqbs::FileStore* fs = d_fileStores[pid].get();
    BSLS_ASSERT_SAFE(fs);

    ClusterNodes peers;
    typedef mqbc::ClusterMembership::ClusterNodeSessionMapIter
        ClusterNodeSessionMapIter;
    for (ClusterNodeSessionMapIter nodeIt =
             d_clusterData_p->membership().clusterNodeSessionMap().begin();
         nodeIt != d_clusterData_p->membership().clusterNodeSessionMap().end();
         ++nodeIt) {
        if (nodeIt->first != d_clusterData_p->membership().selfNode()) {
            peers.push_back(nodeIt->first);
        }
    }

    fs->execute(
        bdlf::BindUtil::bind(&StorageManager::setPrimaryForPartitionDispatched,
                             this,
                             partitionId,
                             primaryNode,
                             primaryLeaseId,
                             peers));
}

void StorageManager::clearPrimaryForPartition(int                  partitionId,
                                              mqbnet::ClusterNode* primary)
{
    // executed by cluster *DISPATCHER* thread

    // PRECONDITION
    BSLS_ASSERT_SAFE(
        d_dispatcher_p->inDispatcherThread(&d_clusterData_p->cluster()));
    BSLS_ASSERT_SAFE(0 <= partitionId);

    unsigned int pid = static_cast<unsigned int>(partitionId);
    BSLS_ASSERT_SAFE(d_fileStores.size() > pid);

    mqbs::FileStore* fs = d_fileStores[pid].get();
    BSLS_ASSERT_SAFE(fs);

    fs->execute(bdlf::BindUtil::bind(
        &StorageManager::clearPrimaryForPartitionDispatched,
        this,
        partitionId,
        primary));
}

void StorageManager::setPrimaryStatusForPartition(
    BSLS_ANNOTATION_UNUSED int partitionId,
    BSLS_ANNOTATION_UNUSED bmqp_ctrlmsg::PrimaryStatus::Value value)
{
    // executed by cluster *DISPATCHER* thread

    // PRECONDITION
    BSLS_ASSERT_SAFE(
        d_dispatcher_p->inDispatcherThread(&d_clusterData_p->cluster()));

    BSLS_ASSERT_OPT(false && "This method should only be invoked in CSL mode");
}

void StorageManager::processPrimaryStateRequest(
    BSLS_ANNOTATION_UNUSED const bmqp_ctrlmsg::ControlMessage& message,
    BSLS_ANNOTATION_UNUSED mqbnet::ClusterNode* source)
{
    BSLS_ASSERT_OPT(false && "This method should only be invoked in CSL mode");
}

void StorageManager::processReplicaStateRequest(
    BSLS_ANNOTATION_UNUSED const bmqp_ctrlmsg::ControlMessage& message,
    BSLS_ANNOTATION_UNUSED mqbnet::ClusterNode* source)
{
    BSLS_ASSERT_OPT(false && "This method should only be invoked in CSL mode");
}

void StorageManager::processReplicaDataRequest(
    BSLS_ANNOTATION_UNUSED const bmqp_ctrlmsg::ControlMessage& message,
    BSLS_ANNOTATION_UNUSED mqbnet::ClusterNode* source)
{
    BSLS_ASSERT_OPT(false && "This method should only be invoked in CSL mode");
}

int StorageManager::makeStorage(bsl::ostream& errorDescription,
                                bslma::ManagedPtr<mqbi::Storage>* out,
                                const bmqt::Uri&                  uri,
                                const mqbu::StorageKey&           queueKey,
                                int                               partitionId,
                                const bsls::Types::Int64          messageTtl,
                                const int maxDeliveryAttempts,
                                const mqbconfm::StorageDefinition& storageDef)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_fileStores.size()));

    return mqbc::StorageUtil::makeStorage(errorDescription,
                                          out,
                                          &d_storages[partitionId],
                                          &d_storagesLock,
                                          uri,
                                          queueKey,
                                          partitionId,
                                          messageTtl,
                                          maxDeliveryAttempts,
                                          storageDef);
}

void StorageManager::processStorageEvent(
    const mqbi::DispatcherStorageEvent& event)
{
    // executed by *CLUSTER DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_dispatcher_p->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(event.isRelay() == false);

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!d_isStarted)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        BALL_LOG_INFO << d_clusterData_p->identity().description()
                      << " Dropping storage event as storage has been closed.";
        return;  // RETURN
    }

    mqbnet::ClusterNode* source = event.clusterNode();
    bmqp::Event          rawEvent(event.blob().get(), d_allocator_p);
    BSLS_ASSERT_SAFE(rawEvent.isStorageEvent() ||
                     rawEvent.isPartitionSyncEvent());
    // Note that DispatcherEventType::e_STORAGE may represent
    // bmqp::EventType::e_STORAGE or e_PARTITION_SYNC.

    if (rawEvent.isPartitionSyncEvent()) {
        processPartitionSyncEvent(event);
        return;  // RETURN
    }

    const unsigned int pid = mqbc::StorageUtil::extractPartitionId<false>(
        rawEvent);
    BSLS_ASSERT_SAFE(d_fileStores.size() > pid);

    // Ensure that 'pid' is valid.
    if (pid >= d_clusterState.partitions().size()) {
        BMQTSK_ALARMLOG_ALARM("STORAGE")
            << d_cluster_p->description() << ": Received storage event "
            << "from node " << source->nodeDescription() << " with "
            << "invalid Partition [" << pid << "]. Ignoring entire "
            << "storage event." << BMQTSK_ALARMLOG_END;
        return;  // RETURN
    }

    // Certain alarms are disabled if this node is starting, or if it
    // hasn't heard anything from the leader yet, as it may not be aware
    // of partition/primary mapping yet, among other things.
    bool skipAlarm =
        bmqp_ctrlmsg::NodeStatus::E_STARTING ==
            d_clusterData_p->membership().selfNodeStatus() ||
        isZero(d_clusterData_p->electorInfo().leaderMessageSequence());
    const ClusterStatePartitionInfo& pinfo = d_clusterState.partition(pid);
    if (!mqbc::StorageUtil::validateStorageEvent(
            rawEvent,
            pid,
            source,
            pinfo.primaryNode(),
            pinfo.primaryStatus(),
            d_clusterData_p->identity().description(),
            skipAlarm,
            false)) {  // isFSMWorkflow
        return;        // RETURN
    }

    mqbs::FileStore* fs = d_fileStores[pid].get();
    BSLS_ASSERT_SAFE(fs);

    fs->execute(
        bdlf::BindUtil::bind(&StorageManager::processStorageEventDispatched,
                             this,
                             pid,
                             event.blob(),
                             source));
}

void StorageManager::processStorageSyncRequest(
    const bmqp_ctrlmsg::ControlMessage& message,
    mqbnet::ClusterNode*                source)
{
    // executed by *ANY* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(source);

    const bmqp_ctrlmsg::StorageSyncRequest& req =
        message.choice().clusterMessage().choice().storageSyncRequest();

    int partitionId = req.partitionId();
    if (0 > partitionId ||
        partitionId >= static_cast<int>(d_fileStores.size())) {
        bmqp_ctrlmsg::ControlMessage controlMsg;
        controlMsg.rId() = message.rId();

        BALL_LOG_WARN << d_clusterData_p->identity().description()
                      << ": Unable to serve storage sync request " << req
                      << " from node " << source->nodeDescription()
                      << ". Invalid partitionId."
                      << d_clusterData_p->membership().selfNodeStatus();

        bmqp_ctrlmsg::Status& status = controlMsg.choice().makeStatus();
        status.category() = bmqp_ctrlmsg::StatusCategory::E_INVALID_ARGUMENT;
        status.code()     = -1;
        status.message()  = "Invalid partitionId.";
        d_clusterData_p->messageTransmitter().sendMessage(controlMsg, source);
        return;  // RETURN
    }

    BSLS_ASSERT_SAFE(d_recoveryManager_mp->isStarted());

    d_fileStores[partitionId]->execute(bdlf::BindUtil::bind(
        &StorageManager::processStorageSyncRequestDispatched,
        this,
        partitionId,
        message,
        source));
}

void StorageManager::processPartitionSyncStateRequest(
    const bmqp_ctrlmsg::ControlMessage& message,
    mqbnet::ClusterNode*                source)
{
    // executed by cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_dispatcher_p->inDispatcherThread(&d_clusterData_p->cluster()));
    BSLS_ASSERT_SAFE(source);
    BSLS_ASSERT_SAFE(bmqp_ctrlmsg::NodeStatus::E_AVAILABLE ==
                     d_clusterData_p->membership().selfNodeStatus());

    const bmqp_ctrlmsg::PartitionSyncStateQuery& req =
        message.choice().clusterMessage().choice().partitionSyncStateQuery();

    int partitionId = req.partitionId();
    if (0 > partitionId ||
        partitionId >= static_cast<int>(d_fileStores.size())) {
        bmqp_ctrlmsg::ControlMessage controlMsg;
        controlMsg.rId() = message.rId();

        BALL_LOG_WARN << d_clusterData_p->identity().description()
                      << ": Unable to serve partition sync state request "
                      << req << " from node " << source->nodeDescription()
                      << ". Invalid partitionId."
                      << d_clusterData_p->membership().selfNodeStatus();

        bmqp_ctrlmsg::Status& status = controlMsg.choice().makeStatus();
        status.category() = bmqp_ctrlmsg::StatusCategory::E_INVALID_ARGUMENT;
        status.code()     = -1;
        status.message()  = "Invalid partitionId.";
        d_clusterData_p->messageTransmitter().sendMessage(controlMsg, source);
        return;  // RETURN
    }

    d_fileStores[partitionId]->execute(bdlf::BindUtil::bind(
        &StorageManager::processPartitionSyncStateRequestDispatched,
        this,
        partitionId,
        message,
        source));
}

void StorageManager::processPartitionSyncDataRequest(
    const bmqp_ctrlmsg::ControlMessage& message,
    mqbnet::ClusterNode*                source)
{
    // executed by cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_dispatcher_p->inDispatcherThread(&d_clusterData_p->cluster()));
    BSLS_ASSERT_SAFE(source);
    BSLS_ASSERT_SAFE(bmqp_ctrlmsg::NodeStatus::E_AVAILABLE ==
                     d_clusterData_p->membership().selfNodeStatus());

    const bmqp_ctrlmsg::PartitionSyncDataQuery& req =
        message.choice().clusterMessage().choice().partitionSyncDataQuery();

    int partitionId = req.partitionId();
    if (0 > partitionId ||
        partitionId >= static_cast<int>(d_fileStores.size())) {
        bmqp_ctrlmsg::ControlMessage controlMsg;
        controlMsg.rId() = message.rId();

        BALL_LOG_WARN << d_clusterData_p->identity().description()
                      << ": Unable to serve partition sync data request "
                      << req << " from node " << source->nodeDescription()
                      << ". Invalid partitionId."
                      << d_clusterData_p->membership().selfNodeStatus();

        bmqp_ctrlmsg::Status& status = controlMsg.choice().makeStatus();
        status.category() = bmqp_ctrlmsg::StatusCategory::E_INVALID_ARGUMENT;
        status.code()     = -1;
        status.message()  = "Invalid partitionId.";
        d_clusterData_p->messageTransmitter().sendMessage(controlMsg, source);
        return;  // RETURN
    }

    d_fileStores[partitionId]->execute(bdlf::BindUtil::bind(
        &StorageManager::processPartitionSyncDataRequestDispatched,
        this,
        partitionId,
        message,
        source));
}

void StorageManager::processPartitionSyncDataRequestStatus(
    const bmqp_ctrlmsg::ControlMessage& message,
    mqbnet::ClusterNode*                source)
{
    // executed by *ANY* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(source);

    const bmqp_ctrlmsg::PartitionSyncDataQueryStatus& req =
        message.choice()
            .clusterMessage()
            .choice()
            .partitionSyncDataQueryStatus();

    int partitionId = req.partitionId();
    if (0 > partitionId ||
        partitionId >= static_cast<int>(d_fileStores.size())) {
        bmqp_ctrlmsg::ControlMessage controlMsg;
        controlMsg.rId() = message.rId();

        BALL_LOG_WARN << d_clusterData_p->identity().description()
                      << ": Unable to serve partition sync data request "
                      << "status: " << req << " from node "
                      << source->nodeDescription() << ". Invalid partitionId."
                      << d_clusterData_p->membership().selfNodeStatus();

        bmqp_ctrlmsg::Status& status = controlMsg.choice().makeStatus();
        status.category() = bmqp_ctrlmsg::StatusCategory::E_INVALID_ARGUMENT;
        status.code()     = -1;
        status.message()  = "Invalid partitionId.";
        d_clusterData_p->messageTransmitter().sendMessage(controlMsg, source);
        return;  // RETURN
    }

    d_fileStores[partitionId]->execute(bdlf::BindUtil::bind(
        &StorageManager::processPartitionSyncDataRequestStatusDispatched,
        this,
        partitionId,
        message,
        source));
}

void StorageManager::processRecoveryEvent(
    const mqbi::DispatcherRecoveryEvent& event)
{
    // executed by *CLUSTER DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(event.blob());
    BSLS_ASSERT_SAFE(d_dispatcher_p->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(event.isRelay() == false);

    mqbnet::ClusterNode* source = event.clusterNode();
    bmqp::Event          rawEvent(event.blob().get(), d_allocator_p);
    BSLS_ASSERT_SAFE(rawEvent.isRecoveryEvent());

    bmqp::RecoveryMessageIterator iter;
    rawEvent.loadRecoveryMessageIterator(&iter);
    BSLS_ASSERT_SAFE(iter.isValid());

    // Self node processes a recovery event only if its 'STARTING'.
    if (d_clusterData_p->membership().selfNodeStatus() !=
        bmqp_ctrlmsg::NodeStatus::E_STARTING) {
        BALL_LOG_WARN << d_cluster_p->description()
                      << ": Received recovery event from: "
                      << source->nodeDescription() << " but self node is not "
                      << "STARTING. Self status: "
                      << d_clusterData_p->membership().selfNodeStatus();
        return;  // RETURN
    }

    const unsigned int pid = mqbc::StorageUtil::extractPartitionId<true>(
        rawEvent);
    BSLS_ASSERT_SAFE(d_fileStores.size() > pid);

    // Ensure that 'pid' is valid.
    if (pid >= d_clusterState.partitions().size()) {
        BMQTSK_ALARMLOG_ALARM("CLUSTER")
            << d_cluster_p->description()
            << ": Received recovery event from node "
            << source->nodeDescription()
            << " with invalid partitionId: " << pid
            << ". Ignoring entire recovery event." << BMQTSK_ALARMLOG_END;
        return;  // RETURN
    }

    while (iter.next() == 1) {
        const bmqp::RecoveryHeader& header = iter.header();
        if (pid != header.partitionId()) {
            // A recovery event is sent by 'source' cluster node.  The node may
            // be primary for *zero* or more partitions, but as per the BMQ
            // replication design, *all* messages in this event will belong to
            // the *same* partition.  Any exception to this is a bug in the
            // implementation of recovery, and thus, if it occurs, we reject
            // the *entire* recovery event.

            BMQTSK_ALARMLOG_ALARM("CLUSTER")
                << d_cluster_p->description()
                << ": Received recovery event from node "
                << source->nodeDescription()
                << " with different partitionIds: " << pid << " vs "
                << header.partitionId() << ". Ignoring entire recovery event."
                << BMQTSK_ALARMLOG_END;
            return;  // RETURN
        }
    }

    mqbs::FileStore* fs = d_fileStores[pid].get();
    BSLS_ASSERT_SAFE(fs);

    // All good.  Forward the event to recovery manager.
    fs->execute(bdlf::BindUtil::bind(&RecoveryManager::processRecoveryEvent,
                                     d_recoveryManager_mp.get(),
                                     pid,
                                     event.blob(),
                                     source));
}

void StorageManager::processReceiptEvent(const bmqp::Event&   event,
                                         mqbnet::ClusterNode* source)
{
    // executed by *IO* thread

    bmqu::BlobPosition          position;
    BSLA_MAYBE_UNUSED const int rc = bmqu::BlobUtil::findOffsetSafe(
        &position,
        *event.blob(),
        sizeof(bmqp::EventHeader));
    BSLS_ASSERT_SAFE(rc == 0);

    bmqu::BlobObjectProxy<bmqp::ReplicationReceipt> receipt(
        event.blob(),
        position,
        true,    // read mode
        false);  // no write

    BSLS_ASSERT_SAFE(receipt.isSet());

    const unsigned int pid = receipt->partitionId();
    BSLS_ASSERT_SAFE(d_fileStores.size() > pid);

    mqbs::FileStore* fs = d_fileStores[pid].get();
    BSLS_ASSERT_SAFE(fs);

    // TODO: The same event can be dispatched to the 'fs' without this 'bind'
    //       (and potential heap allocation).
    fs->execute(bdlf::BindUtil::bind(&mqbs::FileStore::processReceiptEvent,
                                     fs,
                                     receipt->primaryLeaseId(),
                                     receipt->sequenceNum(),
                                     source));
}

void StorageManager::bufferPrimaryStatusAdvisory(
    BSLS_ANNOTATION_UNUSED const bmqp_ctrlmsg::PrimaryStatusAdvisory& advisory,
    BSLS_ANNOTATION_UNUSED mqbnet::ClusterNode* source)
{
    // executed by *ANY* thread

    BSLS_ASSERT_OPT(false && "This method should only be invoked in FSM mode");
}

void StorageManager::processPrimaryStatusAdvisory(
    const bmqp_ctrlmsg::PrimaryStatusAdvisory& advisory,
    mqbnet::ClusterNode*                       source)
{
    // executed by *ANY* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(source);
    BSLS_ASSERT_SAFE(d_fileStores.size() >
                     static_cast<size_t>(advisory.partitionId()));

    if (!d_isStarted) {
        return;  // RETURN
    }

    mqbs::FileStore* fs = d_fileStores[advisory.partitionId()].get();
    BSLS_ASSERT_SAFE(fs);

    fs->execute(bdlf::BindUtil::bind(
        &mqbc::StorageUtil::processPrimaryStatusAdvisoryDispatched,
        fs,
        &d_partitionInfoVec[advisory.partitionId()],
        advisory,
        d_clusterData_p->identity().description(),
        source,
        false));  // isFSMWorkflow
}

void StorageManager::processReplicaStatusAdvisory(
    int                             partitionId,
    mqbnet::ClusterNode*            source,
    bmqp_ctrlmsg::NodeStatus::Value status)
{
    // executed by *ANY* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(source);
    BSLS_ASSERT_SAFE(d_fileStores.size() > static_cast<size_t>(partitionId));

    mqbs::FileStore* fs = d_fileStores[partitionId].get();
    BSLS_ASSERT_SAFE(fs);

    fs->execute(bdlf::BindUtil::bind(
        &mqbc::StorageUtil::processReplicaStatusAdvisoryDispatched,
        d_clusterData_p,
        fs,
        partitionId,
        bsl::cref(d_partitionInfoVec[partitionId]),
        source,
        status));
}

void StorageManager::processShutdownEvent()
{
    // executed by *ANY* thread

    // Notify each partition that self is shutting down.  For all the
    // partitions for which self is primary, self will issue a syncPt and a
    // 'passive' primary status advisory.

    for (size_t i = 0; i < d_fileStores.size(); ++i) {
        mqbs::FileStore* fs = d_fileStores[i].get();

        fs->execute(bdlf::BindUtil::bind(
            &StorageManager::processShutdownEventDispatched,
            this,
            i));  // partitionId
    }
}

int StorageManager::processCommand(mqbcmd::StorageResult*        result,
                                   const mqbcmd::StorageCommand& command)
{
    // executed by cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_dispatcher_p->inDispatcherThread(&d_clusterData_p->cluster()));

    if (!d_isStarted) {
        result->makeError();
        result->error().message() = "StorageManager not yet started or is "
                                    "stopping.\n\n";
        return -1;  // RETURN
    }

    return mqbc::StorageUtil::processCommand(
        result,
        &d_fileStores,
        &d_storages,
        &d_storagesLock,
        d_domainFactory_p,
        &d_replicationFactor,
        command,
        d_clusterConfig.partitionConfig().location(),
        d_allocator_p);
}

void StorageManager::gcUnrecognizedDomainQueues()
{
    // executed by cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_dispatcher_p->inDispatcherThread(&d_clusterData_p->cluster()));

    mqbc::StorageUtil::gcUnrecognizedDomainQueues(&d_fileStores,
                                                  &d_unrecognizedDomainsLock,
                                                  d_unrecognizedDomains);
}

// ACCESSORS
bool StorageManager::isStorageEmpty(const bmqt::Uri& uri,
                                    int              partitionId) const
{
    // executed by the *CLUSTER DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_dispatcher_p->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_fileStores.size()));

    return mqbc::StorageUtil::isStorageEmpty(&d_storagesLock,
                                             d_storages[partitionId],
                                             uri,
                                             partitionId);
}

}  // close package namespace
}  // close enterprise namespace
