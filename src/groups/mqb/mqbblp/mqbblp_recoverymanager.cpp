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

// mqbblp_recoverymanager.cpp                                         -*-C++-*-
#include <mqbblp_recoverymanager.h>

#include <mqbscm_version.h>
// MQB
#include <mqbc_clusterdata.h>
#include <mqbc_clusterutil.h>
#include <mqbc_recoveryutil.h>
#include <mqbs_datafileiterator.h>
#include <mqbs_filestoreprotocol.h>
#include <mqbs_filestoreprotocolutil.h>
#include <mqbs_filesystemutil.h>
#include <mqbs_journalfileiterator.h>
#include <mqbs_offsetptr.h>
#include <mqbs_qlistfileiterator.h>

// BMQ
#include <bmqp_event.h>
#include <bmqp_protocol.h>
#include <bmqp_recoveryeventbuilder.h>
#include <bmqp_recoverymessageiterator.h>
#include <bmqp_schemaeventbuilder.h>
#include <bmqp_storageeventbuilder.h>
#include <bmqp_storagemessageiterator.h>
#include <bmqt_resultcode.h>

#include <bmqio_status.h>
#include <bmqsys_time.h>
#include <bmqtsk_alarmlog.h>
#include <bmqu_blobobjectproxy.h>
#include <bmqu_memoutstream.h>
#include <bmqu_printutil.h>

// BDE
#include <bdlb_nullablevalue.h>
#include <bdlb_print.h>
#include <bdlde_md5.h>
#include <bdlf_bind.h>
#include <bdlf_placeholder.h>
#include <bdls_filesystemutil.h>
#include <bdlt_currenttime.h>
#include <bdlt_datetime.h>
#include <bdlt_epochutil.h>
#include <bsl_algorithm.h>
#include <bsl_cstddef.h>
#include <bsl_cstdlib.h>  // for bsl::rand()
#include <bsl_cstring.h>
#include <bsl_iostream.h>
#include <bslma_managedptr.h>
#include <bslmf_assert.h>
#include <bslmt_latch.h>
#include <bslmt_threadutil.h>
#include <bsls_annotation.h>
#include <bsls_timeinterval.h>

namespace BloombergLP {
namespace mqbblp {

namespace {

const char k_LOG_CATEGORY[] = "MQBBLP.RECOVERYMANAGER";

/// This class provides a custom comparator to compare two (sync-point,
/// offset) pairs.
class SyncPointOffsetPairComparator {
  public:
    // ACCESSORS
    bool operator()(const bmqp_ctrlmsg::SyncPointOffsetPair& lhs,
                    const bmqp_ctrlmsg::SyncPointOffsetPair& rhs) const
    {
        return lhs < rhs;
    }
};

/// Move all files associated with the specified `partitionId` located at
/// the specified `currentLocation` to the specified `archiveLocation`.
/// Behavior is undefined unless `partitionId` is non-zero, and both
/// `currentLocation` and `archiveLocation` are directories.
void movePartitionFiles(int                      partitionId,
                        const bslstl::StringRef& currentLocation,
                        const bslstl::StringRef& archiveLocation)
{
    BSLS_ASSERT_SAFE(0 <= partitionId);
    BSLS_ASSERT_SAFE(bdls::FilesystemUtil::isDirectory(currentLocation));
    BSLS_ASSERT_SAFE(bdls::FilesystemUtil::isDirectory(archiveLocation));

    BALL_LOG_SET_CATEGORY(k_LOG_CATEGORY);

    BALL_LOG_INFO << "For Partition [" << partitionId
                  << "], will archive all files.";

    bsl::vector<mqbs::FileStoreSet> fileSets;

    int rc = mqbs::FileStoreUtil::findFileSets(&fileSets,
                                               currentLocation,
                                               partitionId);
    if (0 != rc) {
        BALL_LOG_WARN << "For Partition [" << partitionId
                      << "], failed to find file sets at location ["
                      << currentLocation << "], rc: " << rc;
        return;  // RETURN
    }

    for (unsigned int i = 0; i < fileSets.size(); ++i) {
        BALL_LOG_INFO << "For Partition [" << partitionId
                      << "], archiving file set: " << fileSets[i];
        mqbs::FileSystemUtil::move(fileSets[i].dataFile(), archiveLocation);
        mqbs::FileSystemUtil::move(fileSets[i].journalFile(), archiveLocation);
        mqbs::FileSystemUtil::move(fileSets[i].qlistFile(), archiveLocation);
    }
}

}  // close unnamed namespace

// --------------------------------------
// class RecoveryManager_FileTransferInfo
// --------------------------------------

// MANIPULATORS
void RecoveryManager_FileTransferInfo::clear()
{
    d_journalFd.reset();
    d_dataFd.reset();
    d_qlistFd.reset();
    d_areFileMapped      = false;
    d_aliasedChunksCount = 0;
}

// -----------------------------------------
// struct RecoveryManager_RequestContextType
// -----------------------------------------

bsl::ostream& RecoveryManager_RequestContextType::print(
    bsl::ostream&                            stream,
    RecoveryManager_RequestContextType::Enum value,
    int                                      level,
    int                                      spacesPerLevel)
{
    bdlb::Print::indent(stream, level, spacesPerLevel);
    stream << RecoveryManager_RequestContextType::toAscii(value);

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

const char* RecoveryManager_RequestContextType::toAscii(
    RecoveryManager_RequestContextType::Enum value)
{
#define CASE(X)                                                               \
    case e_##X: return #X;

    switch (value) {
        CASE(UNDEFINED)
        CASE(RECOVERY)
        CASE(PARTITION_SYNC)
    default: return "(* UNKNOWN *)";
    }

#undef CASE
}

// ----------------------------------------------
// class RecoveryManager_PartitionRecoveryContext
// ----------------------------------------------

// MANIPULATORS
void RecoveryManager_RecoveryContext::clear()
{
    d_fileSet.reset();
    d_bufferedEvents.clear();
    d_recoveryCb         = PartitionRecoveryCb();
    d_oldSyncPoint       = bmqp_ctrlmsg::SyncPoint();
    d_oldSyncPointOffset = 0;
    d_newSyncPoint       = bmqp_ctrlmsg::SyncPoint();
    d_newSyncPointOffset = 0;
    d_journalFileOffset  = 0;
    d_dataFileOffset     = 0;
    d_qlistFileOffset    = 0;
    d_inRecovery         = false;
    d_recoveryPeer_p     = 0;
    d_responseType       = bmqp_ctrlmsg::StorageSyncResponseType::E_UNDEFINED;
    d_expectedChunkFileType     = bmqp::RecoveryFileChunkType::e_UNDEFINED;
    d_lastChunkSequenceNumber   = 0;
    d_recoveryStartupWaitHandle = EventHandle();
    d_recoveryStatusCheckHandle = EventHandle();
}

// ----------------------------------------
// class RecoveryManager_PrimarySyncContext
// ----------------------------------------

// MANIPULATORS
void RecoveryManager_PrimarySyncContext::clear()
{
    d_fs_p                     = 0;
    d_syncInProgress           = false;
    d_syncPeer_p               = 0;
    d_primarySyncCb            = PartitionPrimarySyncCb();
    d_selfPartitionSeqNum      = bmqp_ctrlmsg::PartitionSequenceNumber();
    d_selfLastSyncPtOffsetPair = bmqp_ctrlmsg::SyncPointOffsetPair();
    d_fileTransferInfo.clear();
    d_syncStatusEventHandle = EventHandle();
    d_peerPartitionStates.clear();
}

// ----------------------------------
// class RecoveryManager_ChunkDeleter
// ----------------------------------

// ACCESSORS
void RecoveryManager_ChunkDeleter::operator()(
    BSLS_ANNOTATION_UNUSED const void* ptr) const
{
    // executed by *ANY* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_requestContext_p || d_primarySyncContext_p);
    BSLS_ASSERT_SAFE(0 == d_requestContext_p || 0 == d_primarySyncContext_p);

    BALL_LOG_SET_CATEGORY(k_LOG_CATEGORY);

    typedef RecoveryManager_FileTransferInfo FileTransferInfo;

    FileTransferInfo* fti = d_requestContext_p
                                ? &d_requestContext_p->fileTransferInfo()
                                : &d_primarySyncContext_p->fileTransferInfo();

    BSLS_ASSERT_SAFE(fti);
    BSLS_ASSERT_SAFE(0 < fti->aliasedChunksCount());

    RecoveryManager* rm = d_requestContext_p
                              ? d_requestContext_p->recoveryManager()
                              : d_primarySyncContext_p->recoveryManager();
    BSLS_ASSERT_SAFE(rm);

    // If we maintain the type of chunk 'ptr' (DATA/JOURNAL/QLIST), then we can
    // also assert the range of 'ptr' (similar to mqbs::FileStore).

    if (0 != --fti->d_aliasedChunksCount) {
        return;  // RETURN
    }

    // Last chunk referring to the mapped region has gone.  Safe to unmap/close
    // all 3 files, and remove the request context from the list of pending
    // contexts, if its RequestContext.

    if (d_primarySyncContext_p) {
        // Enqueue a callback in the appropriate dispatcher thread, because we
        // will be updating one of the elements of 'd_primarySyncContexts'.

        int pid = d_primarySyncContext_p->fileStore()->config().partitionId();

        const mqbi::DispatcherClientData& dispData =
            d_primarySyncContext_p->fileStore()->dispatcherClientData();

        rm->d_dispatcher_p->execute(
            bdlf::BindUtil::bind(
                &RecoveryManager::partitionSyncCleanupDispatched,
                rm,
                pid),
            dispData);

        return;  // RETURN
    }

    // Its a RequestContext.

    int rc = mqbs::FileStoreUtil::closePartitionSet(&fti->dataFd(),
                                                    &fti->journalFd(),
                                                    &fti->qlistFd());
    if (rc != 0) {
        // Failed to close one or more partition files
        BALL_LOG_ERROR << "For Partition ["
                       << d_requestContext_p->partitionId() << "], failed"
                       << " to close one or more partition files "
                       << "[journalFd: " << fti->journalFd().fd()
                       << ", dataFd: " << fti->dataFd().fd()
                       << ", qlistFd: " << fti->qlistFd().fd()
                       << "], rc: " << rc;
    }

    typedef RecoveryManager::RequestContextIter RequestContextIter;
    typedef RecoveryManager::RequestContextType RequestContextType;
    typedef RecoveryManager::RequestContexts    RequestContexts;

    BSLS_ASSERT_SAFE(RequestContextType::e_UNDEFINED !=
                     d_requestContext_p->contextType());

    // Depending on the type of 'd_requestContext_p->contextType()', we need to
    // purge 'd_requestContext_p' from one of these:
    // - 'RecoveryMgr.d_recoveryRequestContexts'
    // - 'RecoveryMgr.d_primarySyncRequestContexts'
    // But before purging it, we need to acquire corresponding lock.  First, we
    // find the correct lock.

    RequestContexts* requestCtxs = 0;
    bsls::SpinLock*  lock        = 0;

    if (RequestContextType::e_RECOVERY == d_requestContext_p->contextType()) {
        lock        = &rm->d_recoveryRequestContextLock;
        requestCtxs = &rm->d_recoveryRequestContexts;
    }
    else {
        lock        = &rm->d_primarySyncRequestContextLock;
        requestCtxs = &rm->d_primarySyncRequestContexts;
    }

    BSLS_ASSERT_SAFE(lock);
    BSLS_ASSERT_SAFE(requestCtxs);

    // Then we acquire the lock before iterating over the correct container and
    // purging the appropriate element from it.

    bsls::SpinLockGuard guard(lock);  // LOCK
    RequestContextIter  beginIt;
    RequestContextIter  endIt;

    if (RequestContextType::e_RECOVERY == d_requestContext_p->contextType()) {
        beginIt = rm->d_recoveryRequestContexts.begin();
        endIt   = rm->d_recoveryRequestContexts.end();
    }
    else {
        beginIt = rm->d_primarySyncRequestContexts.begin();
        endIt   = rm->d_primarySyncRequestContexts.end();
    }

    for (; beginIt != endIt; ++beginIt) {
        if (d_requestContext_p == &(*beginIt)) {
            requestCtxs->erase(beginIt);
            return;  // RETURN
        }
    }

    // We must have found 'd_requestContext_p' in the container, otherwise its
    // a contract violation.
    BSLS_ASSERT_SAFE(false && "Unreachable by design.");
}

// ---------------------
// class RecoveryManager
// ---------------------

// PRIVATE MANIPULATORS
void RecoveryManager::recoveryStartupWaitCb(int partitionId)
{
    // executed by the *SCHEDULER* thread

    // Forward the callback to cluster's dispatcher thread.

    if (!d_isStarted) {
        return;  // RETURN
    }

    d_dispatcher_p->execute(
        bdlf::BindUtil::bind(&RecoveryManager::recoveryStartupWaitDispatched,
                             this,
                             partitionId),
        d_cluster_p);
}

void RecoveryManager::recoveryStartupWaitDispatched(int partitionId)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_dispatcher_p->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(0 <= partitionId);
    BSLS_ASSERT_SAFE(d_partitionsInfo.size() >
                     static_cast<size_t>(partitionId));

    // Check the status of each peer.  If none of the nodes are AVAILABLE, go
    // ahead with local recovery.  This assumes that if no peers notified this
    // node of their availability, none of the partitions have a primary.  This
    // is ok to assume because a node can be primary for a partition only if
    // its AVAILABLE.

    bsl::vector<mqbnet::ClusterNode*> availableNodes;
    ClusterNodeSessionMapConstIter    cit;
    for (cit = d_clusterData_p->membership().clusterNodeSessionMap().begin();
         cit != d_clusterData_p->membership().clusterNodeSessionMap().end();
         ++cit) {
        const mqbc::ClusterNodeSession* ns = cit->second.get();
        BSLS_ASSERT_SAFE(ns);
        if (bmqp_ctrlmsg::NodeStatus::E_AVAILABLE == ns->nodeStatus()) {
            availableNodes.push_back(cit->first);
        }
    }

    if (availableNodes.empty()) {
        // No peer is AVAILABLE.

        BALL_LOG_INFO << d_clusterData_p->identity().description()
                      << ": Partition [" << partitionId
                      << "], no peers are AVAILABLE after waiting for approx. "
                      << d_clusterConfig.partitionConfig()
                             .syncConfig()
                             .startupWaitDurationMs()
                      << " milliseconds after startup.";
    }

    // We may or may not have received sync point for the specified
    // 'partitionId'.  Enqueue an event in the partition dispatcher thread with
    // a list of all AVAILABLE nodes (even if the list is empty), so that
    // recovery manager can contact one of those AVAILABLE nodes (if any) for
    // the 'partitionId' for which active recovery hasn't started yet.  If none
    // of the nodes are AVAILABLE, local recovery will be performed for
    // 'partitionId'.  Note that we need to enqueue in partition's dispatcher
    // thread because all partition-related data structures are read/written
    // only from partition-dispatcher thread.

    const PartitionInfo&              pinfo    = d_partitionsInfo[partitionId];
    const mqbi::DispatcherClientData& dispData = pinfo.dispatcherClientData();
    d_dispatcher_p->execute(
        bdlf::BindUtil::bind(
            &RecoveryManager::recoveryStartupWaitPartitionDispatched,
            this,
            partitionId,
            availableNodes),
        dispData);
}

void RecoveryManager::recoveryStartupWaitPartitionDispatched(
    int                 partitionId,
    const ClusterNodes& availableNodes)
{
    // executed by each of the *STORAGE (QUEUE) DISPATCHER* threads

    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     static_cast<unsigned int>(partitionId) <
                         d_partitionsInfo.size());

    RecoveryContext& recoveryCtx = d_recoveryContexts[partitionId];

    if (!recoveryCtx.inRecovery()) {
        // Partition is not under recovery any longer.
        return;  // RETURN
    }

    if (recoveryCtx.recoveryPeer()) {
        // Partition is already under active recovery.  Nothing else to do.
        return;  // RETURN
    }

    // No sync point has been received for this partition in the startup wait
    // duration.

    if (availableNodes.empty()) {
        // No peers are AVAILABLE.  Node will recover from local files only.
        // In a multi-node cluster, when file store is recovering, it expects
        // last journal record to be a SyncPt, which may or may not be the case
        // here.  So its important to truncate partition files to the offsets
        // indicated in the last syncPt recovered in 'startRecovery'.  Files
        // are truncated in the 'onPartitionRecoveryStatus' routine.

        // Additionally, its possible that local journal may not have a syncPt.
        // In that case, we just archive all partition files and let file store
        // create new ones.

        if (0 != recoveryCtx.oldSyncPointOffset()) {
            BALL_LOG_WARN << d_clusterData_p->identity().description()
                          << ": Partition [" << partitionId
                          << "], no sync point has been received during "
                          << "recovery wait-time and no peers are available. "
                          << "Partition will be truncated to the last syncPt "
                          << "offsets & then local recovery will be performed."
                          << "Offset details: JOURNAL: "
                          << recoveryCtx.oldSyncPointOffset()
                          << ", DATA: " << recoveryCtx.dataFileOffset()
                          << ", QLIST: " << recoveryCtx.qlistFileOffset();

            // Files will be truncated by 'onPartitionRecoveryStatus'.
        }
        else {
            BALL_LOG_WARN << d_clusterData_p->identity().description()
                          << ": Partition [" << partitionId
                          << "], no sync point has been received during "
                          << "recovery wait-time and no peers are available. "
                          << "No syncPt is present in the local journal, and "
                          << "thus, local partition files will be archived. "
                          << "Recovery will proceed as if no local files "
                          << "exist.";
            movePartitionFiles(partitionId,
                               d_dataStoreConfig.location(),
                               d_dataStoreConfig.archiveLocation());
        }

        onPartitionRecoveryStatus(partitionId, 0 /* status */);
        return;  // RETURN
    }

    // There is at least one available peer.  Choose a peer randomly.

    ClusterNodesConstIter it = availableNodes.begin();
    bsl::advance(it, (bsl::rand() % availableNodes.size()));
    BSLS_ASSERT_SAFE(*it);
    BSLS_ASSERT_SAFE(*it != d_clusterData_p->membership().selfNode());
    mqbnet::ClusterNode* peer = *it;
    recoveryCtx.setRecoveryPeer(peer);

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << ": Partition [" << partitionId
                  << "], sending storage sync request to an AVAILABLE peer: "
                  << peer->nodeDescription()
                  << ", after no sync point was received during recovery "
                  << "wait-time.";

    sendStorageSyncRequesterHelper(&recoveryCtx, partitionId);
}

void RecoveryManager::recoveryStatusCb(int partitionId)
{
    // executed by the *SCHEDULER* thread
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     static_cast<unsigned int>(partitionId) <
                         d_recoveryContexts.size());

    if (!d_isStarted) {
        return;  // RETURN
    }

    const mqbi::DispatcherClientData& dispData =
        d_partitionsInfo[partitionId].dispatcherClientData();
    d_dispatcher_p->execute(
        bdlf::BindUtil::bind(&RecoveryManager::recoveryStatusDispatched,
                             this,
                             partitionId),
        dispData);
}

void RecoveryManager::recoveryStatusDispatched(int partitionId)
{
    // executed by each of the *STORAGE (QUEUE) DISPATCHER* threads

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     static_cast<unsigned int>(partitionId) <
                         d_recoveryContexts.size());

    RecoveryContext& recoveryCtx = d_recoveryContexts[partitionId];
    if (!recoveryCtx.inRecovery()) {
        return;  // RETURN
    }

    BMQTSK_ALARMLOG_ALARM("RECOVERY")
        << d_clusterData_p->identity().description() << ": For Partition ["
        << partitionId << "], recovery not completed "
        << "after maximum stipulated time of "
        << d_clusterConfig.partitionConfig()
               .syncConfig()
               .startupRecoveryMaxDurationMs()
        << " milliseconds. Cancelling on-going recovery. Recovery peer was: "
        << (recoveryCtx.recoveryPeer()
                ? recoveryCtx.recoveryPeer()->nodeDescription()
                : "** none **")
        << ". It was attempt #" << recoveryCtx.numAttempts() << "."
        << BMQTSK_ALARMLOG_END;

    // TBD: cancel the recovery.

    onPartitionRecoveryStatus(partitionId, -1 /* status */);
}

void RecoveryManager::primarySyncStatusCb(int partitionId)
{
    // executed by the *SCHEDULER* thread
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     static_cast<unsigned int>(partitionId) <
                         d_primarySyncContexts.size());

    if (!d_isStarted) {
        return;  // RETURN
    }

    const mqbi::DispatcherClientData& dispData =
        d_partitionsInfo[partitionId].dispatcherClientData();
    d_dispatcher_p->execute(
        bdlf::BindUtil::bind(&RecoveryManager::primarySyncStatusDispatched,
                             this,
                             partitionId),
        dispData);
}

void RecoveryManager::primarySyncStatusDispatched(int partitionId)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     static_cast<unsigned int>(partitionId) <
                         d_primarySyncContexts.size());

    PrimarySyncContext& primarySyncCtx = d_primarySyncContexts[partitionId];
    if (!primarySyncCtx.primarySyncInProgress()) {
        return;  // RETURN
    }

    BMQTSK_ALARMLOG_ALARM("RECOVERY")
        << d_clusterData_p->identity().description() << ": For Partition ["
        << partitionId << "], primary sync not "
        << "completed after maximum stipulated time of "
        << d_clusterConfig.partitionConfig()
               .syncConfig()
               .masterSyncMaxDurationMs()
        << " milliseconds. Cancelling on-going primary sync."
        << BMQTSK_ALARMLOG_END;

    // TBD: cancel primary sync

    onPartitionPrimarySyncStatus(partitionId, -1 /* status */);
}

void RecoveryManager::onNodeDownDispatched(int                  partitionId,
                                           mqbnet::ClusterNode* node)
{
    // executed by *STORAGE (QUEUE) DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(0 <= partitionId);
    BSLS_ASSERT_SAFE(static_cast<size_t>(partitionId) <
                     d_recoveryContexts.size());

    // We need to take action in any of these scenarios:
    // 1) Specified 'node' is serving the storage sync request for the
    //    'partitionId' from self node.
    // 2) Self node is a passive primary and is syncing the 'partitionId' with
    //    the 'node' (i.e., 'node' has more advanced view of the partition).

    // We need to abort the sync in either of these cases.  If we don't, the
    // default timeouts will eventually kick in and notify of the failure, but
    // those timeouts can be greater than 10 minutes.

    // Also note that a more robust approach (obviously!) is to reschedule the
    // storage sync with another peer as appropriate.

    RecoveryContext&    recoveryCtx    = d_recoveryContexts[partitionId];
    PrimarySyncContext& primarySyncCtx = d_primarySyncContexts[partitionId];

    if (recoveryCtx.inRecovery() && recoveryCtx.recoveryPeer() == node) {
        BALL_LOG_WARN << d_clusterData_p->identity().description()
                      << ": Partition [" << partitionId << "]: peer "
                      << node->nodeDescription()
                      << ", which was serving storage sync request to self "
                      << "node, has gone down. Notifying of partition recovery"
                      << " failure.";
        onPartitionRecoveryStatus(partitionId, -1 /* status */);
    }
    else if (primarySyncCtx.primarySyncInProgress() &&
             primarySyncCtx.syncPeer() == node) {
        BALL_LOG_WARN << d_clusterData_p->identity().description()
                      << ": Partition [" << partitionId << "]: peer "
                      << node->nodeDescription()
                      << ", which was serving primary-partition sync request "
                      << "to self node, has gone down. Notifying of partition "
                      << "primary sync failure.";
        onPartitionPrimarySyncStatus(partitionId, -1 /* status */);
    }
    // else: no action needed by recovery manager.
}

void RecoveryManager::partitionSyncCleanupDispatched(int partitionId)
{
    // executed by *STORAGE (QUEUE) DISPATCHER* thread

    BSLS_ASSERT_SAFE(0 <= partitionId);
    BSLS_ASSERT_SAFE(static_cast<size_t>(partitionId) <
                     d_primarySyncContexts.size());

    PrimarySyncContext& primarySyncCtx = d_primarySyncContexts[partitionId];

    BSLS_ASSERT_SAFE(partitionId ==
                     primarySyncCtx.fileStore()->config().partitionId());

    FileTransferInfo& fti = primarySyncCtx.fileTransferInfo();

    BSLS_ASSERT_SAFE(0 == fti.aliasedChunksCount());
    BSLS_ASSERT_SAFE(true == fti.areFilesMapped());

    int rc = mqbs::FileStoreUtil::closePartitionSet(&fti.dataFd(),
                                                    &fti.journalFd(),
                                                    &fti.qlistFd());
    if (rc != 0) {
        // Failed to close one or more partition files
        BALL_LOG_ERROR << "For Partition [" << partitionId << "], failed"
                       << " to close one or more partition files "
                       << "[journalFd: " << fti.journalFd().fd()
                       << ", dataFd: " << fti.dataFd().fd()
                       << ", qlistFd: " << fti.qlistFd().fd()
                       << "], rc: " << rc;
    }

    fti.setAreFilesMapped(false);

    // Cleanup partition's primary sync context.
    primarySyncCtx.clear();
}

void RecoveryManager::sendStorageSyncRequesterHelper(RecoveryContext* context,
                                                     int partitionId)
{
    // executed by *STORAGE (QUEUE) DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(context);
    BSLS_ASSERT_SAFE(context->recoveryPeer());

    RequestManagerType::RequestSp request =
        d_clusterData_p->requestManager().createRequest();

    bmqp_ctrlmsg::StorageSyncRequest& storageSyncReq =
        request->request()
            .choice()
            .makeClusterMessage()
            .choice()
            .makeStorageSyncRequest();

    storageSyncReq.partitionId() = partitionId;
    if (0 != context->oldSyncPointOffset()) {
        // Valid *A* sync-point.

        bmqp_ctrlmsg::SyncPointOffsetPair& spOffsetPair =
            storageSyncReq.beginSyncPointOffsetPair().makeValue();
        spOffsetPair.syncPoint() = context->oldSyncPoint();
        spOffsetPair.offset()    = context->oldSyncPointOffset();
    }

    if (0 != context->newSyncPointOffset()) {
        // Valid *B* sync-point.

        bmqp_ctrlmsg::SyncPointOffsetPair& spOffsetPair =
            storageSyncReq.endSyncPointOffsetPair().makeValue();
        spOffsetPair.syncPoint() = context->newSyncPoint();
        spOffsetPair.offset()    = context->newSyncPointOffset();
    }

    request->setResponseCb(
        bdlf::BindUtil::bind(&RecoveryManager::onStorageSyncResponse,
                             this,
                             bdlf::PlaceHolders::_1,
                             context->recoveryPeer()));

    bsls::TimeInterval timeoutMs;
    timeoutMs.setTotalMilliseconds(d_clusterConfig.partitionConfig()
                                       .syncConfig()
                                       .storageSyncReqTimeoutMs());
    bmqt::GenericResult::Enum status = d_clusterData_p->cluster().sendRequest(
        request,
        context->recoveryPeer(),
        timeoutMs);

    if (bmqt::GenericResult::e_SUCCESS != status) {
        // Request failed to encode/be sent; process error handling (note that
        // 'onStorageSyncResponse' won't be invoked in this case).

        BMQTSK_ALARMLOG_ALARM("RECOVERY")
            << d_clusterData_p->identity().description() << ": For Partition ["
            << partitionId << "], failed to send "
            << "storage sync request to node "
            << context->recoveryPeer()->nodeDescription() << ", rc: " << status
            << ". No retry attempt will be made." << BMQTSK_ALARMLOG_END;

        onPartitionRecoveryStatus(partitionId, -1 /* status */);

        // TBD: reschedule recovery.
    }

    // Bump up num attempts for storage-sync.

    context->setNumAttempts(context->numAttempts() + 1);

    // If this is the first time a storageSync request is being sent, schedule
    // an event to check recovery status.

    if (1 == context->numAttempts()) {
        bsls::TimeInterval after(bmqsys::Time::nowMonotonicClock());
        after.addMilliseconds(d_clusterConfig.partitionConfig()
                                  .syncConfig()
                                  .startupRecoveryMaxDurationMs());

        BSLS_ASSERT_SAFE(!context->recoveryStatusCheckHandle());
        d_clusterData_p->scheduler().scheduleEvent(
            &(context->recoveryStatusCheckHandle()),
            after,
            bdlf::BindUtil::bind(&RecoveryManager::recoveryStatusCb,
                                 this,
                                 partitionId));
    }
}

void RecoveryManager::onStorageSyncResponse(
    const RequestManagerType::RequestSp& context,
    const mqbnet::ClusterNode*           responder)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(responder);
    BSLS_ASSERT_SAFE(context->request().choice().isClusterMessageValue());
    BSLS_ASSERT_SAFE(context->request()
                         .choice()
                         .clusterMessage()
                         .choice()
                         .isStorageSyncRequestValue());

    int partitionId = context->request()
                          .choice()
                          .clusterMessage()
                          .choice()
                          .storageSyncRequest()
                          .partitionId();

    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     static_cast<unsigned int>(partitionId) <
                         d_recoveryContexts.size());

    const mqbi::DispatcherClientData& dispData =
        d_partitionsInfo[partitionId].dispatcherClientData();
    d_dispatcher_p->execute(
        bdlf::BindUtil::bind(&RecoveryManager::onStorageSyncResponseDispatched,
                             this,
                             partitionId,
                             context,
                             responder),
        dispData);
}

void RecoveryManager::onStorageSyncResponseDispatched(
    int                                  partitionId,
    const RequestManagerType::RequestSp& context,
    const mqbnet::ClusterNode*           responder)
{
    // executed by each of the *STORAGE (QUEUE) DISPATCHER* threads

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     static_cast<unsigned int>(partitionId) <
                         d_recoveryContexts.size());

    RecoveryContext& recoveryCtx = d_recoveryContexts[partitionId];
    BSLS_ASSERT_SAFE(bmqp_ctrlmsg::StorageSyncResponseType::E_UNDEFINED ==
                     recoveryCtx.responseType());
    BSLS_ASSERT_SAFE(bmqp::RecoveryFileChunkType::e_UNDEFINED ==
                     recoveryCtx.expectedChunkFileType());

    const bmqp_ctrlmsg::StorageSyncRequest& req = context->request()
                                                      .choice()
                                                      .clusterMessage()
                                                      .choice()
                                                      .storageSyncRequest();

    if (!recoveryCtx.inRecovery()) {
        BALL_LOG_WARN << d_clusterData_p->identity().description()
                      << " Partition [" << partitionId
                      << "]: " << "received storage sync response from: "
                      << responder->nodeDescription()
                      << " for request: " << req << ", but partition is no "
                      << "longer in recovery (probably due to recovery "
                      << "timeout). Ignoring this response. It was attempt #"
                      << recoveryCtx.numAttempts() << ".";
        return;  // RETURN
    }

    if (context->response().choice().isStatusValue()) {
        const bmqp_ctrlmsg::Status& status =
            context->response().choice().status();
        if (status.category() == bmqp_ctrlmsg::StatusCategory::E_CANCELED &&
            status.code() == mqbi::ClusterErrorCode::e_STOPPING) {
            // This storage-sync request was cancelled because self node is
            // stopping.  No need to retry.

            BALL_LOG_INFO
                << d_clusterData_p->identity().description() << " Partition ["
                << partitionId << "]: " << "storage sync request: " << req
                << " cancelled because self"
                << " node is stopping. No retry attempt will be made.";

            onPartitionRecoveryStatus(partitionId, -1 /* status */);
            return;  // RETURN
        }

        // Request failed due to some other reason.  Should be retried.

        const int maxAttempts = d_clusterConfig.partitionConfig()
                                    .syncConfig()
                                    .maxAttemptsStorageSync();
        if (maxAttempts == recoveryCtx.numAttempts()) {
            BMQTSK_ALARMLOG_ALARM("RECOVERY")
                << d_clusterData_p->identity().description()
                << ": For Partition [" << partitionId
                << "], received storage sync failure response: "
                << context->response().choice().status()
                << " from node: " << responder->nodeDescription()
                << " for request: " << req << ". Exhausted max attempts for "
                << "storage sync: " << maxAttempts
                << ". No retry attempt will be made." << BMQTSK_ALARMLOG_END;

            onPartitionRecoveryStatus(partitionId, -1 /* status */);

            return;  // RETURN
        }

        // Peer sent an error; it could be due to invalid request sent by self
        // or something else.  Retry with same peer again, but this time,
        // instead of requesting a patch (ie, sending a range of SyncPts), just
        // request the entire partition file by sending an empty begin SyncPt,
        // and same (valid) end SyncPt.  This is done with the hope that peer
        // will already be aware of the end SyncPt (since the peer would have
        // sent that SyncPt itself upon seeing self node), and hopefully will
        // be able to satisfy this new request.

        // Before we send the new request, we need to reset/remove any SyncPt
        // info that was retrieved in previous attempts (the "old" or "begin"
        // SyncPt).

        BALL_LOG_ERROR << d_clusterData_p->identity().description()
                       << ": For Partition [" << partitionId
                       << "], received storage sync failure response: "
                       << context->response().choice().status()
                       << " from node: " << responder->nodeDescription()
                       << " for request: " << req << ", for attempt #"
                       << recoveryCtx.numAttempts()
                       << ". Retrying, by requesting "
                       << "entire partition instead of a patch.";

        bmqp_ctrlmsg::SyncPoint syncPoint;
        recoveryCtx.setOldSyncPoint(syncPoint);
        recoveryCtx.setOldSyncPointOffset(0);
        recoveryCtx.setJournalFileOffset(0);
        recoveryCtx.setDataFileOffset(0);
        recoveryCtx.setQlistFileOffset(0);

        sendStorageSyncRequesterHelper(&recoveryCtx, partitionId);

        return;  // RETURN
    }

    BSLS_ASSERT(context->response().choice().isClusterMessageValue());
    BSLS_ASSERT(context->response()
                    .choice()
                    .clusterMessage()
                    .choice()
                    .isStorageSyncResponseValue());

    const bmqp_ctrlmsg::StorageSyncResponse& response =
        context->response()
            .choice()
            .clusterMessage()
            .choice()
            .storageSyncResponse();

    if (response.partitionId() != partitionId) {
        BMQTSK_ALARMLOG_ALARM("RECOVERY")
            << d_clusterData_p->identity().description() << ": For Partition ["
            << partitionId << "], invalid partitionId"
            << " specified in storage sync response: " << response
            << " from node: " << responder->nodeDescription()
            << " for storage sync request: " << req
            << ". No retry attempt will be made. It was attempt #"
            << recoveryCtx.numAttempts() << "." << BMQTSK_ALARMLOG_END;

        onPartitionRecoveryStatus(partitionId, -1 /* status */);

        // TBD: reschedule recovery.

        return;  // RETURN
    }

    if (response.storageSyncResponseType() ==
        bmqp_ctrlmsg::StorageSyncResponseType::E_UNDEFINED) {
        BMQTSK_ALARMLOG_ALARM("RECOVERY")
            << d_clusterData_p->identity().description() << ": For Partition ["
            << partitionId << "],  invalid "
            << "storage-sync response type specified: " << response
            << " from node: " << responder->nodeDescription()
            << " for storage sync request: " << req
            << ". No retry attempt will be made. It was attempt #"
            << recoveryCtx.numAttempts() << "." << BMQTSK_ALARMLOG_END;

        onPartitionRecoveryStatus(partitionId, -1 /* status */);

        // TBD: reschedule recovery.

        return;  // RETURN
    }

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << ": For Partition [" << partitionId
                  << "], received storage sync response: " << response
                  << " from node: " << responder->nodeDescription();

    bmqp_ctrlmsg::StorageSyncResponseType::Value rtype =
        response.storageSyncResponseType();

    recoveryCtx.setResponseType(rtype);

    if (bmqp_ctrlmsg::StorageSyncResponseType::E_IN_SYNC == rtype) {
        onPartitionRecoveryStatus(partitionId, 0 /* status */);
        return;  // RETURN
    }

    if (bmqp_ctrlmsg::StorageSyncResponseType::E_EMPTY == rtype) {
        // Archive all files for this partition at this node.  FileStore will
        // create a new (ie, empty) one.

        movePartitionFiles(partitionId,
                           d_dataStoreConfig.location(),
                           d_dataStoreConfig.archiveLocation());

        onPartitionRecoveryStatus(partitionId, 0 /* status */);
        return;  // RETURN
    }

    if (bmqp_ctrlmsg::StorageSyncResponseType::E_FILE == rtype) {
        // Validate sync points in response.

        const bmqp_ctrlmsg::SyncPoint& beginSp = response.beginSyncPoint();
        const bmqp_ctrlmsg::SyncPoint& endSp   = response.endSyncPoint();

        if (endSp <= beginSp) {
            BMQTSK_ALARMLOG_ALARM("RECOVERY")
                << d_clusterData_p->identity().description()
                << ": For Partition [" << partitionId
                << "], received incorrect sync points in storage sync response"
                << " type: " << rtype << ". Begin sync point: " << beginSp
                << ", end sync point: " << endSp << BMQTSK_ALARMLOG_END;

            onPartitionRecoveryStatus(partitionId, -1 /* status */);

            // TBD: reschedule recovery.

            return;  // RETURN
        }

        if ((0 != recoveryCtx.newSyncPointOffset() &&
             endSp < recoveryCtx.newSyncPoint())) {
            BMQTSK_ALARMLOG_ALARM("RECOVERY")
                << d_clusterData_p->identity().description()
                << ": For Partition [" << partitionId << "], received "
                << "incorrect end sync point in storage sync response type: "
                << rtype << ". End sync point: " << endSp
                << ", 'B' sync point: " << recoveryCtx.newSyncPoint()
                << BMQTSK_ALARMLOG_END;

            onPartitionRecoveryStatus(partitionId, -1 /* status */);

            // TBD: reschedule recovery.

            return;  // RETURN
        }

        if (endSp < recoveryCtx.oldSyncPoint()) {
            BMQTSK_ALARMLOG_ALARM("RECOVERY")
                << d_clusterData_p->identity().description()
                << ": For Partition [" << partitionId << "], received "
                << "incorrect end sync point in storage sync response type: "
                << rtype << ". End sync point: " << endSp
                << ", 'A' sync point: " << recoveryCtx.oldSyncPoint()
                << BMQTSK_ALARMLOG_END;

            onPartitionRecoveryStatus(partitionId, -1 /* status */);

            // TBD: reschedule recovery.

            return;  // RETURN
        }

        // Since the peer is sending entire file, close the ones that are open
        // for this partition, and archive *all* files for this partition then.
        // TBD: perhaps archiving should be done *after* file has been received
        // from the peer, so that if recovery doesn't complete, this node can
        // initiate recovery for this partition from scratch.

        if (recoveryCtx.journalFd().isValid()) {
            BSLS_ASSERT_SAFE(recoveryCtx.qlistFd().isValid());
            BSLS_ASSERT_SAFE(recoveryCtx.dataFd().isValid());

            int rc = mqbs::FileStoreUtil::closePartitionSet(
                &recoveryCtx.journalFd(),
                &recoveryCtx.dataFd(),
                &recoveryCtx.qlistFd());
            if (rc != 0) {
                // Failed to close one or more partition files
                BALL_LOG_ERROR
                    << "For Partition [" << partitionId << "], "
                    << "failed to close one or more partition "
                    << "files [journal: "
                    << recoveryCtx.fileSet().journalFile()
                    << ", data: " << recoveryCtx.fileSet().dataFile()
                    << ", qlist: " << recoveryCtx.fileSet().qlistFile() << "]"
                    << "rc: " << rc;
            }
        }

        BALL_LOG_INFO << d_clusterData_p->identity().description()
                      << ": For Partition [" << partitionId
                      << "], will archive all files.";

        bsl::vector<mqbs::FileStoreSet> fileSets;

        int rc = mqbs::FileStoreUtil::findFileSets(
            &fileSets,
            d_dataStoreConfig.location(),
            partitionId);
        if (rc != 0) {
            BALL_LOG_WARN << d_clusterData_p->identity().description()
                          << ": For Partition [" << partitionId
                          << "], failed to find file sets at location ["
                          << d_dataStoreConfig.location() << "], rc: " << rc;
        }
        else {
            for (unsigned int i = 0; i < fileSets.size(); ++i) {
                BALL_LOG_INFO << d_clusterData_p->identity().description()
                              << ": For Partition [" << partitionId
                              << "], archiving file set: " << fileSets[i];
                mqbs::FileSystemUtil::move(
                    fileSets[i].dataFile(),
                    d_dataStoreConfig.archiveLocation());
                mqbs::FileSystemUtil::move(
                    fileSets[i].journalFile(),
                    d_dataStoreConfig.archiveLocation());
                mqbs::FileSystemUtil::move(
                    fileSets[i].qlistFile(),
                    d_dataStoreConfig.archiveLocation());
            }
        }

        // Create new file set and map it, so that it can be written to when
        // peer starts sending files.  Nothing needs to be written to these
        // files though.
        // TBD: - should peer send expected journal, qlist and data file sizes?
        //     - should it also send file names, so that names are consistent
        //       across all nodes?
        //     - explicitly update the time and nodeId in the 'FileHeader'
        //       block of the received file.  Should the peer skip the
        //       FileHeader when sending the file?

        bdlt::Datetime now = bdlt::CurrentTime::utc();
        bsl::string    dataFileName;
        bsl::string    journalFileName;
        bsl::string    qlistFileName;
        mqbs::FileStoreUtil::createDataFileName(&dataFileName,
                                                d_dataStoreConfig.location(),
                                                partitionId,
                                                now);
        mqbs::FileStoreUtil::createJournalFileName(
            &journalFileName,
            d_dataStoreConfig.location(),
            partitionId,
            now);
        mqbs::FileStoreUtil::createQlistFileName(&qlistFileName,
                                                 d_dataStoreConfig.location(),
                                                 partitionId,
                                                 now);

        mqbs::FileStoreSet fileSet;
        fileSet.setDataFile(dataFileName)
            .setDataFileSize(d_dataStoreConfig.maxDataFileSize())
            .setJournalFile(journalFileName)
            .setJournalFileSize(d_dataStoreConfig.maxJournalFileSize())
            .setQlistFile(qlistFileName)
            .setQlistFileSize(d_dataStoreConfig.maxQlistFileSize());

        bmqu::MemOutStream errorDesc;
        rc = mqbs::FileStoreUtil::openFileSetWriteMode(
            errorDesc,
            fileSet,
            d_dataStoreConfig.hasPreallocate(),
            true,  // delete on failure
            &recoveryCtx.journalFd(),
            &recoveryCtx.dataFd(),
            &recoveryCtx.qlistFd(),
            d_dataStoreConfig.hasPrefaultPages());
        if (0 != rc) {
            BMQTSK_ALARMLOG_ALARM("RECOVERY")
                << d_clusterData_p->identity().description()
                << ": For Partition [" << partitionId << "], "
                << "failed to create file set: " << fileSet << ", rc: " << rc
                << ", reason: " << errorDesc.str() << ", during recovery."
                << BMQTSK_ALARMLOG_END;

            onPartitionRecoveryStatus(partitionId, -1 /* status */);
            return;  // RETURN
        }

        // All good.  Recovery will be complete when this node has received
        // entire storage from the peer.  Explicitly update the saved starting
        // data and journal offsets to zero.

        recoveryCtx.setDataFileOffset(0);
        recoveryCtx.setJournalFileOffset(0);
        recoveryCtx.setQlistFileOffset(0);
        recoveryCtx.setExpectedChunkFileType(
            bmqp::RecoveryFileChunkType::e_DATA);

        return;  // RETURN
    }

    if (bmqp_ctrlmsg::StorageSyncResponseType::E_PATCH == rtype) {
        // Validate sync points in response.

        const bmqp_ctrlmsg::SyncPoint& beginSp = response.beginSyncPoint();
        const bmqp_ctrlmsg::SyncPoint& endSp   = response.endSyncPoint();

        if (endSp <= beginSp) {
            BMQTSK_ALARMLOG_ALARM("RECOVERY")
                << d_clusterData_p->identity().description()
                << ": For Partition [" << partitionId << "], received "
                << "incorrect sync points in storage sync response type: "
                << rtype << ". Begin sync point: " << beginSp
                << ", end sync point: " << endSp << BMQTSK_ALARMLOG_END;

            onPartitionRecoveryStatus(partitionId, -1 /* status */);

            // TBD: reschedule recovery.

            return;  // RETURN
        }

        if (beginSp != recoveryCtx.oldSyncPoint()) {
            BMQTSK_ALARMLOG_ALARM("RECOVERY")
                << d_clusterData_p->identity().description()
                << ": For Partition [" << partitionId << "], received "
                << "incorrect sync points in storage sync response type: "
                << rtype << ". Begin sync point: " << beginSp
                << ", 'A' sync point: " << recoveryCtx.oldSyncPoint()
                << BMQTSK_ALARMLOG_END;

            onPartitionRecoveryStatus(partitionId, -1 /* status */);
            return;  // RETURN
        }

        if (0 != recoveryCtx.newSyncPointOffset() &&
            (recoveryCtx.newSyncPoint() != endSp)) {
            BMQTSK_ALARMLOG_ALARM("RECOVERY")
                << d_clusterData_p->identity().description()
                << ": For Partition [" << partitionId << "], received "
                << "incorrect sync points in storage sync response type: "
                << rtype << ". End sync point: " << endSp
                << ", 'B' sync point: " << recoveryCtx.newSyncPoint()
                << BMQTSK_ALARMLOG_END;

            onPartitionRecoveryStatus(partitionId, -1 /* status */);

            // TBD: reschedule recovery.

            return;  // RETURN
        }

        if (recoveryCtx.journalFd().isValid()) {
            // Files were opened in read-only mode.  Need to close them, and
            // re-open with read/write mode, with appropriate size.

            BSLS_ASSERT_SAFE(recoveryCtx.qlistFd().isValid());
            BSLS_ASSERT_SAFE(recoveryCtx.dataFd().isValid());

            int rc = mqbs::FileStoreUtil::closePartitionSet(
                &recoveryCtx.dataFd(),
                &recoveryCtx.journalFd(),
                &recoveryCtx.qlistFd());
            if (rc != 0) {
                // Failed to close one or more partition files
                BALL_LOG_ERROR
                    << "For Partition [" << partitionId << "], "
                    << "failed to close one or more partition "
                    << "files [journal: "
                    << recoveryCtx.fileSet().journalFile()
                    << ", data: " << recoveryCtx.fileSet().dataFile()
                    << ", qlist: " << recoveryCtx.fileSet().qlistFile() << "]"
                    << "rc: " << rc;
            }
        }

        bmqu::MemOutStream errorDesc;
        recoveryCtx.fileSet()
            .setJournalFileSize(d_dataStoreConfig.maxJournalFileSize())
            .setDataFileSize(d_dataStoreConfig.maxDataFileSize())
            .setQlistFileSize(d_dataStoreConfig.maxQlistFileSize());

        int rc = mqbs::FileStoreUtil::openFileSetWriteMode(
            errorDesc,
            recoveryCtx.fileSet(),
            d_dataStoreConfig.hasPreallocate(),
            false,  // don't delete on failure
            &recoveryCtx.journalFd(),
            &recoveryCtx.dataFd(),
            &recoveryCtx.qlistFd(),
            d_dataStoreConfig.hasPrefaultPages());
        if (0 != rc) {
            BMQTSK_ALARMLOG_ALARM("RECOVERY")
                << d_clusterData_p->identity().description()
                << ": For Partition [" << partitionId
                << "], failed to open file set: " << recoveryCtx.fileSet()
                << ", rc: " << rc << ", reason: " << errorDesc.str()
                << ", during recovery." << BMQTSK_ALARMLOG_END;

            onPartitionRecoveryStatus(partitionId, -1 /* status */);
            return;  // RETURN
        }

        // All good.  Recovery will be complete when this node has received
        // entire patch from the peer.

        recoveryCtx.setExpectedChunkFileType(
            bmqp::RecoveryFileChunkType::e_DATA);
        return;  // RETURN
    }

    BSLS_ASSERT_SAFE(false && "Unreachable by design.");
}

void RecoveryManager::onPartitionRecoveryStatus(int partitionId, int status)
{
    // executed by each of the *STORAGE (QUEUE) DISPATCHER* threads

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     static_cast<unsigned int>(partitionId) <
                         d_recoveryContexts.size());

    RecoveryContext& recoveryCtx = d_recoveryContexts[partitionId];
    BSLS_ASSERT_SAFE(recoveryCtx.inRecovery());

    d_clusterData_p->scheduler().cancelEventAndWait(
        &recoveryCtx.recoveryStartupWaitHandle());
    d_clusterData_p->scheduler().cancelEventAndWait(
        &recoveryCtx.recoveryStatusCheckHandle());

    // Close all files if they are open and inform storage manager.

    if (recoveryCtx.journalFd().isValid()) {
        BSLS_ASSERT_SAFE(recoveryCtx.qlistFd().isValid());
        BSLS_ASSERT_SAFE(recoveryCtx.dataFd().isValid());

        BALL_LOG_INFO
            << d_clusterData_p->identity().description() << " Partition ["
            << partitionId
            << "]: truncating & closing partition with sizes: " << "journal: "
            << bmqu::PrintUtil::prettyNumber(static_cast<bsls::Types::Int64>(
                   recoveryCtx.journalFileOffset()))
            << ", data: "
            << bmqu::PrintUtil::prettyNumber(static_cast<bsls::Types::Int64>(
                   recoveryCtx.dataFileOffset()))
            << ", qlist: "
            << bmqu::PrintUtil::prettyNumber(static_cast<bsls::Types::Int64>(
                   recoveryCtx.qlistFileOffset()));

        // Truncate the files to correct size (recall that they were opened
        // with max configured size).
        bmqu::MemOutStream dummy;
        mqbs::FileSystemUtil::truncate(&recoveryCtx.journalFd(),
                                       recoveryCtx.journalFileOffset(),
                                       dummy);
        mqbs::FileSystemUtil::truncate(&recoveryCtx.qlistFd(),
                                       recoveryCtx.qlistFileOffset(),
                                       dummy);
        mqbs::FileSystemUtil::truncate(&recoveryCtx.dataFd(),
                                       recoveryCtx.dataFileOffset(),
                                       dummy);

        int rc = mqbs::FileStoreUtil::closePartitionSet(
            &recoveryCtx.dataFd(),
            &recoveryCtx.journalFd(),
            &recoveryCtx.qlistFd());
        if (rc != 0) {
            // Failed to close one or more partition files
            BALL_LOG_ERROR << "For Partition [" << partitionId << "], "
                           << "failed to close one or more partition "
                           << "files [journal: "
                           << recoveryCtx.fileSet().journalFile()
                           << ", data: " << recoveryCtx.fileSet().dataFile()
                           << ", qlist: " << recoveryCtx.fileSet().qlistFile()
                           << "]" << "rc: " << rc;
        }
    }

    // Cleanup partition's recovery context before notifying StorageManager via
    // the callback.  This is important because StorageManager can initiate a
    // new round of recovery in that callback, which will use the same context
    // object.
    mqbnet::ClusterNode* recoveryPeerNode = recoveryCtx.recoveryPeer();
    bsl::vector<bsl::shared_ptr<bdlbb::Blob> > bufferedStorageEvents(
        recoveryCtx.storageEvents());
    PartitionRecoveryCb partitionRecoveryCb = recoveryCtx.recoveryCb();
    recoveryCtx.clear();

    partitionRecoveryCb(partitionId,
                        status,
                        bufferedStorageEvents,
                        recoveryPeerNode);
}

void RecoveryManager::onPartitionPrimarySyncStatus(int partitionId, int status)
{
    // executed by each of the *STORAGE (QUEUE) DISPATCHER* threads

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(0 <= partitionId && static_cast<size_t>(partitionId) <
                                             d_primarySyncContexts.size());

    PrimarySyncContext& primarySyncCtx = d_primarySyncContexts[partitionId];
    BSLS_ASSERT_SAFE(primarySyncCtx.primarySyncInProgress());

    d_clusterData_p->scheduler().cancelEventAndWait(
        &primarySyncCtx.primarySyncStatusEventHandle());

    primarySyncCtx.partitionPrimarySyncCb()(partitionId, status);

    if (primarySyncCtx.fileTransferInfo().areFilesMapped()) {
        // Don't clear the 'primarySyncCtx' at this time because files are
        // still mapped.  It will be cleaned up when the chunk deleter
        // eventually invokes 'partitionSyncCleanupDispatched' routine.

        return;  // RETURN
    }

    // Ok to clear.
    primarySyncCtx.clear();
}

void RecoveryManager::stopDispatched(int partitionId, bslmt::Latch* latch)
{
    // executed by each of the *STORAGE (QUEUE) DISPATCHER* threads

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     static_cast<unsigned int>(partitionId) <
                         d_partitionsInfo.size());
    BSLS_ASSERT_SAFE(latch);

    RecoveryContext&    recoveryContext = d_recoveryContexts[partitionId];
    PrimarySyncContext& primarySyncCtx  = d_primarySyncContexts[partitionId];

    if (recoveryContext.inRecovery()) {
        BALL_LOG_WARN
            << d_clusterData_p->identity().description() << ": For Partition ["
            << partitionId
            << "], stopping recovery while it is in progress with "
            << (recoveryContext.recoveryPeer()
                    ? recoveryContext.recoveryPeer()->nodeDescription()
                    : "** null **")
            << " node serving the recovery request.";

        // TBD: cancel the recovery
    }
    recoveryContext.clear();

    if (primarySyncCtx.primarySyncInProgress()) {
        BALL_LOG_WARN << d_clusterData_p->identity().description()
                      << ": For Partition [" << partitionId << "], "
                      << "stopping primary sync while it's in progress with "
                      << (primarySyncCtx.syncPeer()
                              ? primarySyncCtx.syncPeer()->nodeDescription()
                              : "** null **")
                      << " node serving the primary-sync request.";

        // TBD: cancel the primary-sync request
    }
    primarySyncCtx.clear();

    latch->arrive();
}

int RecoveryManager::sendFile(RequestContext*                   context,
                              bsls::Types::Uint64               beginOffset,
                              bsls::Types::Uint64               endOffset,
                              unsigned int                      chunkSize,
                              bmqp::RecoveryFileChunkType::Enum chunkFileType)

{
    // executed by *ANY* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(context);
    BSLS_ASSERT_SAFE(beginOffset <= endOffset);
    BSLS_ASSERT_SAFE(0 < chunkSize);

    enum { rc_SUCCESS = 0, rc_BUILDER_FAILURE = -1, rc_WRITE_FAILURE = -2 };

    mqbs::MappedFileDescriptor* mfd = 0;
    FileTransferInfo&           fti = context->fileTransferInfo();

    switch (chunkFileType) {
    case bmqp::RecoveryFileChunkType::e_DATA:
        mfd = &fti.dataFd();
        break;  // BREAK

    case bmqp::RecoveryFileChunkType::e_QLIST:
        mfd = &fti.qlistFd();
        break;  // BREAK

    case bmqp::RecoveryFileChunkType::e_JOURNAL:
        mfd = &fti.journalFd();
        break;  // BREAK

    case bmqp::RecoveryFileChunkType::e_UNDEFINED:
        BSLS_ASSERT_SAFE(false);
        break;  // BREAK
    }

    BSLS_ASSERT_SAFE(0 != mfd);
    BSLS_ASSERT_SAFE(mfd->isValid());

    // Builder should be created on the stack because this routine can be
    // invoked from any thread.

    unsigned int               sequenceNumber = 0;
    bsls::Types::Uint64        currOffset     = beginOffset;
    bmqp::RecoveryEventBuilder builder(&d_clusterData_p->blobSpPool(),
                                       d_allocator_p);

    while ((currOffset + chunkSize) < endOffset) {
        BSLS_ASSERT_SAFE(currOffset < mfd->fileSize());

        bsl::shared_ptr<char> chunkBufferSp(mfd->mapping() + currOffset,
                                            ChunkDeleter(context));
        bdlbb::BlobBuffer     chunkBlobBuffer(chunkBufferSp, chunkSize);

        // Bump up aliased chunk counter now that 'chunkBufferSp' is referring
        // to the mapped region of type 'chunkFileType' (DATA/QLIST/JOURNAL).

        fti.incrementAliasedChunksCount();

        // Add chunk to recovery event builder.

        bmqt::EventBuilderResult::Enum buildRc = builder.packMessage(
            static_cast<unsigned int>(context->partitionId()),
            chunkFileType,
            ++sequenceNumber,
            chunkBlobBuffer,
            false);  // Not the last chunk

        if (bmqt::EventBuilderResult::e_SUCCESS != buildRc) {
            return static_cast<int>(buildRc) * 10 +
                   rc_BUILDER_FAILURE;  // RETURN
        }

        bmqt::GenericResult::Enum writeRc = context->requesterNode()->write(
            builder.blob_sp(),
            bmqp::EventType::e_RECOVERY);

        if (bmqt::GenericResult::e_SUCCESS != writeRc) {
            BALL_LOG_ERROR << "Failed to write " << chunkFileType
                           << " file chunk with sequence # " << sequenceNumber
                           << " for Partition [" << context->partitionId()
                           << "] to peer node: "
                           << context->requesterNode()->nodeDescription()
                           << ", rc: " << writeRc;
            return static_cast<int>(writeRc) * 10 +
                   rc_WRITE_FAILURE;  // RETURN
        }

        // Its important to reset the builder at this point so that it can
        // release its copy of 'chunkBufferSp', so that when other copies of
        // 'chunkBufferSp' are released, its custom deleter is invoked.

        builder.reset();

        currOffset += chunkSize;
    }

    // Send remaining part of the file.

    bsl::shared_ptr<char> chunkBufferSp(mfd->mapping() + currOffset,
                                        ChunkDeleter(context));
    bdlbb::BlobBuffer chunkBlobBuffer(chunkBufferSp, endOffset - currOffset);

    // Bump up aliased chunk counter now that 'chunkBufferSp' is referring to
    // the mapped region of type 'chunkFileType' (DATA/QLIST/JOURNAL).

    fti.incrementAliasedChunksCount();

    // Add chunk to recovery event builder.

    bmqt::EventBuilderResult::Enum buildRc = builder.packMessage(
        static_cast<unsigned int>(context->partitionId()),
        chunkFileType,
        ++sequenceNumber,
        chunkBlobBuffer,
        true);  // Its the last chunk

    if (bmqt::EventBuilderResult::e_SUCCESS != buildRc) {
        return static_cast<int>(buildRc) * 10 + rc_BUILDER_FAILURE;  // RETURN
    }

    bmqt::GenericResult::Enum writeRc = context->requesterNode()->write(
        builder.blob_sp(),
        bmqp::EventType::e_RECOVERY);

    if (bmqt::GenericResult::e_SUCCESS != writeRc) {
        BALL_LOG_ERROR << "Failed to write " << chunkFileType
                       << " file chunk with sequence # " << sequenceNumber
                       << " for Partition [" << context->partitionId() << "]"
                       << " to peer node: "
                       << context->requesterNode()->nodeDescription()
                       << ", rc: " << writeRc;
        return static_cast<int>(writeRc) * 10 + rc_WRITE_FAILURE;  // RETURN
    }

    // Its important to reset the builder at this point so that it can release
    // its copy of 'chunkBufferSp', so that when other copies of
    // 'chunkBufferSp' are released, its custom deleter is invoked.

    builder.reset();

    return rc_SUCCESS;
}

int RecoveryManager::replayPartition(
    RequestContext*                              requestContext,
    PrimarySyncContext*                          primarySyncContext,
    mqbnet::ClusterNode*                         destination,
    const bmqp_ctrlmsg::PartitionSequenceNumber& fromSequenceNum,
    const bmqp_ctrlmsg::PartitionSequenceNumber& toSequenceNum,
    bsls::Types::Uint64                          fromSyncPtOffset)
{
    BSLS_ASSERT_SAFE(requestContext || primarySyncContext);
    BSLS_ASSERT_SAFE(0 == requestContext || 0 == primarySyncContext);
    BSLS_ASSERT_SAFE(0 < fromSyncPtOffset);
    mqbc::RecoveryUtil::validateArgs(fromSequenceNum,
                                     toSequenceNum,
                                     destination);

    int pid = requestContext
                  ? requestContext->partitionId()
                  : primarySyncContext->fileStore()->config().partitionId();

    FileTransferInfo* fti = requestContext
                                ? &requestContext->fileTransferInfo()
                                : &primarySyncContext->fileTransferInfo();
    BSLS_ASSERT_SAFE(fti);

    BSLS_ASSERT_SAFE(fti->journalFd().isValid());
    BSLS_ASSERT_SAFE(fti->qlistFd().isValid());
    BSLS_ASSERT_SAFE(fti->dataFd().isValid());

    enum {
        rc_SUCCESS                  = 0,
        rc_JOURNAL_ITERATOR_FAILURE = -1,
        rc_INCOMPLETE_JOURNAL       = -2,
        rc_INVALID_SYNC_PT_OFFSET   = -3,
        rc_INVALID_JOURNALOP_RECORD = -4,
        rc_INVALID_SEQUENCE_NUMBER  = -5,
        rc_BUILDER_FAILURE          = -6,
        rc_WRITE_FAILURE            = -7,
        rc_INCOMPLETE_REPLAY        = -8
    };

    mqbs::JournalFileIterator journalIt;

    int rc = journalIt.reset(
        &(fti->journalFd()),
        mqbs::FileStoreProtocolUtil::bmqHeader(fti->journalFd()));

    if (0 != rc) {
        return rc_JOURNAL_ITERATOR_FAILURE;  // RETURN
    }

    // Skip JOURNAL records till 'fromSyncPtOffset' is reached.

    while (journalIt.recordOffset() < fromSyncPtOffset &&
           1 == journalIt.nextRecord())
        ;

    if (journalIt.recordOffset() != fromSyncPtOffset) {
        return rc_INCOMPLETE_JOURNAL;  // RETURN
    }

    // Ensure that record at the specified offset is indeed a SyncPt.

    const mqbs::RecordHeader& recordHeader = journalIt.recordHeader();
    if (mqbs::RecordType::e_JOURNAL_OP != recordHeader.type()) {
        return rc_INVALID_SYNC_PT_OFFSET;  // RETURN
    }

    if (mqbs::JournalOpType::e_SYNCPOINT !=
        journalIt.asJournalOpRecord().type()) {
        return rc_INVALID_JOURNALOP_RECORD;  // RETURN
    }

    // Retrieve sequence number from the RecordHeader in the SyncPt, not from
    // the SyncPt itself.

    bmqp_ctrlmsg::PartitionSequenceNumber currentSeqNum;
    rc = mqbc::RecoveryUtil::bootstrapCurrentSeqNum(&currentSeqNum,
                                                    journalIt,
                                                    fromSequenceNum);
    if (rc != 0) {
        return rc_INVALID_SEQUENCE_NUMBER;
    }

    bmqp::StorageEventBuilder builder(mqbs::FileStoreProtocol::k_VERSION,
                                      bmqp::EventType::e_PARTITION_SYNC,
                                      &d_clusterData_p->blobSpPool(),
                                      d_allocator_p);

    // Note that partition has to be replayed from the record *after*
    // 'fromSequenceNum'.  So move forward by one record in the JOURNAL.
    while (currentSeqNum < toSequenceNum) {
        char* journalRecordBase = 0;
        int journalRecordLen = mqbs::FileStoreProtocol::k_JOURNAL_RECORD_SIZE;
        char*                          payloadRecordBase = 0;
        int                            payloadRecordLen  = 0;
        bmqp::StorageMessageType::Enum storageMsgType =
            bmqp::StorageMessageType::e_UNDEFINED;

        rc = mqbc::RecoveryUtil::incrementCurrentSeqNum(
            &currentSeqNum,
            &journalRecordBase,
            fti->journalFd(),
            toSequenceNum,
            pid,
            *destination,
            d_clusterData_p->identity().description(),
            journalIt);
        if (rc != 0) {
            break;  // BREAK
        }

        mqbc::RecoveryUtil::processJournalRecord(&storageMsgType,
                                                 &payloadRecordBase,
                                                 &payloadRecordLen,
                                                 journalIt,
                                                 fti->dataFd(),
                                                 false,  // fsmWorkflow
                                                 fti->qlistFd());

        BSLS_ASSERT_SAFE(bmqp::StorageMessageType::e_UNDEFINED !=
                         storageMsgType);
        BSLS_ASSERT_SAFE(0 != journalRecordBase);
        BSLS_ASSERT_SAFE(0 != journalRecordLen);

        bsl::shared_ptr<char> journalRecordSp(
            journalRecordBase,
            requestContext ? ChunkDeleter(requestContext)
                           : ChunkDeleter(primarySyncContext));
        bdlbb::BlobBuffer journalRecordBlobBuffer(journalRecordSp,
                                                  journalRecordLen);

        // Bump up aliased chunk counter now that 'journalRecordSp' is
        // referring to the mapped region.

        fti->incrementAliasedChunksCount();

        bmqt::EventBuilderResult::Enum builderRc;
        if (0 != payloadRecordBase) {
            BSLS_ASSERT_SAFE(0 != payloadRecordLen);
            BSLS_ASSERT_SAFE(
                bmqp::StorageMessageType::e_DATA == storageMsgType ||
                bmqp::StorageMessageType::e_QLIST == storageMsgType);

            bsl::shared_ptr<char> payloadRecordSp(
                payloadRecordBase,
                requestContext ? ChunkDeleter(requestContext)
                               : ChunkDeleter(primarySyncContext));
            bdlbb::BlobBuffer payloadRecordBlobBuffer(payloadRecordSp,
                                                      payloadRecordLen);

            // Bump up aliased chunk counter now that 'payloadRecordSp' is
            // referring to the mapped region.

            fti->incrementAliasedChunksCount();

            builderRc = builder.packMessage(storageMsgType,
                                            pid,
                                            0,  // flags
                                            journalIt.recordOffset() /
                                                bmqp::Protocol::k_WORD_SIZE,
                                            journalRecordBlobBuffer,
                                            payloadRecordBlobBuffer);
        }
        else {
            builderRc = builder.packMessage(storageMsgType,
                                            pid,
                                            0,  // flags
                                            journalIt.recordOffset() /
                                                bmqp::Protocol::k_WORD_SIZE,
                                            journalRecordBlobBuffer);
        }

        if (bmqt::EventBuilderResult::e_SUCCESS != builderRc) {
            return rc_BUILDER_FAILURE + 10 * static_cast<int>(builderRc);
            // RETURN
        }

        if (d_clusterConfig.partitionConfig()
                .syncConfig()
                .partitionSyncEventSize() <= builder.eventSize()) {
            bmqt::GenericResult::Enum writeRc = destination->write(
                builder.blob_sp(),
                bmqp::EventType::e_PARTITION_SYNC);

            if (bmqt::GenericResult::e_SUCCESS != writeRc) {
                return static_cast<int>(writeRc) * 10 + rc_WRITE_FAILURE;
                // RETURN
            }

            builder.reset();
        }
    }

    if (currentSeqNum != toSequenceNum) {
        BALL_LOG_WARN << d_clusterData_p->identity().description()
                      << " Partition [" << pid
                      << "]: incomplete replay of partition. Sequence number "
                      << "of last record sent: " << currentSeqNum
                      << ", was supposed to send up to: " << toSequenceNum
                      << ". Peer: " << destination->nodeDescription() << ".";
        return rc_INCOMPLETE_REPLAY;  // RETURN
    }

    if (0 < builder.messageCount()) {
        bmqt::GenericResult::Enum writeRc = destination->write(
            builder.blob_sp(),
            bmqp::EventType::e_PARTITION_SYNC);

        if (bmqt::GenericResult::e_SUCCESS != writeRc) {
            return static_cast<int>(writeRc) * 10 +
                   rc_WRITE_FAILURE;  // RETURN
        }
    }

    return rc_SUCCESS;
}

void RecoveryManager::syncPeerPartitions(PrimarySyncContext* primarySyncCtx)
{
    BSLS_ASSERT_SAFE(primarySyncCtx);

    const mqbs::FileStore* fs       = primarySyncCtx->fileStore();
    PeerPartitionStates&   ppStates = primarySyncCtx->peerPartitionStates();
    int                    pid      = fs->config().partitionId();

    mqbs::FileStoreSet fileSet;
    fs->loadCurrentFiles(&fileSet);

    // Map JOURNAL file.
    FileTransferInfo& fti = primarySyncCtx->fileTransferInfo();
    BSLS_ASSERT_SAFE(false == fti.areFilesMapped());

    bmqu::MemOutStream errorDesc;
    int                rc = mqbs::FileStoreUtil::openFileSetReadMode(errorDesc,
                                                      fileSet,
                                                      &fti.journalFd(),
                                                      &fti.dataFd(),
                                                      &fti.qlistFd());

    if (0 != rc) {
        BMQTSK_ALARMLOG_ALARM("FILE_IO")
            << d_clusterData_p->identity().description() << " Partition ["
            << pid << "]: Failed to open JOURNAL/QLIST/DATA file, rc: " << rc
            << ", reason [" << errorDesc.str() << "] while new primary (self)"
            << " is initiating partition-sync with peers."
            << BMQTSK_ALARMLOG_END;
        return;  // RETURN
    }

    // Files are now mapped, so set appropriate flag.

    fti.setAreFilesMapped(true);

    // Create a proctor which will invoke the chunk-deleter (if required).

    bsl::shared_ptr<char> ftiProctor(reinterpret_cast<char*>(this),  // dummy
                                     ChunkDeleter(primarySyncCtx));
    fti.incrementAliasedChunksCount();

    for (size_t i = 0; i < ppStates.size(); ++i) {
        const PeerPartitionState& pps = ppStates[i];
        BSLS_ASSERT_SAFE(pps.peer());
        BSLS_ASSERT_SAFE(pps.peer() !=
                         d_clusterData_p->membership().selfNode());

        if (!pps.needsPartitionSync()) {
            BALL_LOG_INFO << d_clusterData_p->identity().description()
                          << " Partition [" << pid
                          << "]: skipping partition-sync with peer: "
                          << pps.peer()->nodeDescription()
                          << ". Peer's partition sequence num: "
                          << pps.partitionSequenceNum();
            continue;  // CONTINUE
        }

        rc = syncPeerPartition(primarySyncCtx, pps);
        if (0 == rc) {
            BALL_LOG_INFO << d_clusterData_p->identity().description()
                          << " Partition [" << pid
                          << "]: new primary (self) successfully synced "
                          << "partition with peer: "
                          << pps.peer()->nodeDescription();
        }
        else {
            BMQTSK_ALARMLOG_ALARM("CLUSTER")
                << d_clusterData_p->identity().description() << " Partition ["
                << pid
                << "]: new primary (self) failed to sync partition with peer: "
                << pps.peer()->nodeDescription() << ", rc: " << rc << "."
                << BMQTSK_ALARMLOG_END;
        }
    }
}

int RecoveryManager::syncPeerPartition(PrimarySyncContext* primarySyncCtx,
                                       const PeerPartitionState& ppState)
{
    BSLS_ASSERT_SAFE(primarySyncCtx);
    BSLS_ASSERT_SAFE(primarySyncCtx->primarySyncInProgress());
    BSLS_ASSERT_SAFE(ppState.needsPartitionSync());
    BSLS_ASSERT_SAFE(ppState.peer() !=
                     d_clusterData_p->membership().selfNode());

    enum {
        rc_SUCCESS                         = 0,
        rc_NEW_PRIMARY_BEHIND              = -1,
        rc_SEQ_NUM_MISMATCH_WITH_SYNC_PEER = -2,
        rc_ARCHIVED_FILE_SYNC_UNSUPPORTED  = -3,
        rc_PEER_SYNC_POINT_NOT_FOUND       = -4,
        rc_INVALID_JOURNAL_OFFSET          = -5,
        rc_INVALID_QLIST_OFFSET            = -6,
        rc_INVALID_DATA_OFFSET             = -7,
        rc_REPLAY_FAILURE                  = -8
    };

    const mqbs::FileStore* fs  = primarySyncCtx->fileStore();
    int                    pid = fs->config().partitionId();

    bmqp_ctrlmsg::PartitionSequenceNumber selfSequenceNum;
    selfSequenceNum.primaryLeaseId() = fs->primaryLeaseId();
    selfSequenceNum.sequenceNumber() = fs->sequenceNumber();

    const FileTransferInfo& fti = primarySyncCtx->fileTransferInfo();

    BSLS_ASSERT_SAFE(fti.areFilesMapped());

    if (selfSequenceNum < ppState.partitionSequenceNum()) {
        // TBD: assert?
        BALL_LOG_ERROR << d_clusterData_p->identity().description()
                       << " Partition [" << pid
                       << "]: new primary (self) has smaller sequence number: "
                       << selfSequenceNum << ", than "
                       << ppState.partitionSequenceNum()
                       << ", of peer: " << ppState.peer()->nodeDescription();
        return rc_NEW_PRIMARY_BEHIND;  // RETURN
    }

    if (primarySyncCtx->syncPeer() == ppState.peer()) {
        if (selfSequenceNum != ppState.partitionSequenceNum()) {
            // New primary (self) synced the partition with this peer, so their
            // sequence numbers *must* match.  TBD: assert?

            BALL_LOG_ERROR << d_clusterData_p->identity().description()
                           << " Partition [" << pid
                           << "]: new primary (self) has different sequence "
                           << "number: " << selfSequenceNum << ", than "
                           << ppState.partitionSequenceNum()
                           << ", of syncing peer: "
                           << ppState.peer()->nodeDescription();
            return rc_SEQ_NUM_MISMATCH_WITH_SYNC_PEER;  // RETURN
        }

        return rc_SUCCESS;  // RETURN
    }

    if (selfSequenceNum == ppState.partitionSequenceNum()) {
        BALL_LOG_INFO << d_clusterData_p->identity().description()
                      << " Partition [" << pid
                      << "]: new primary (self) has same sequence "
                      << "number: " << selfSequenceNum
                      << ", as: " << ppState.partitionSequenceNum()
                      << ", of peer: " << ppState.peer()->nodeDescription()
                      << ". No sync is needed.";
        return rc_SUCCESS;  // RETURN
    }

    const SyncPointOffsetPairs&              spOffsetPairs = fs->syncPoints();
    const bmqp_ctrlmsg::SyncPointOffsetPair& firstSpOffPair =
        spOffsetPairs.front();
    mqbs::JournalFileIterator jit;

    if (false == (firstSpOffPair.syncPoint() <=
                  ppState.lastSyncPointOffsetPair().syncPoint())) {
        // TBD: Need to peek into the archived files to retrieve the correct
        // one.  In practice, there should never be a need to peek beyond the
        // latest archived file, since a newly elected primary is supposed to
        // be almost up-to-date on the partition.  This scenario is currently
        // not handled.

        BALL_LOG_ERROR << d_clusterData_p->identity().description()
                       << " Partition [" << pid
                       << "]: partition sync from archived files not yet "
                       << "supported, for peer: "
                       << ppState.peer()->nodeDescription()
                       << ", peer syncPt: "
                       << ppState.lastSyncPointOffsetPair().syncPoint()
                       << ", self syncPt: " << firstSpOffPair.syncPoint();
        return rc_ARCHIVED_FILE_SYNC_UNSUPPORTED;  // RETURN
    }

    // Replay storage events from the current ("live") files of the partition.

    BSLS_ASSERT_SAFE(mqbc::ClusterUtil::isValid(
        ppState.lastSyncPointOffsetPair().syncPoint()));

    // Find ppState.lastSyncPoint() in the list of sync points maintained by
    // self for this partition.

    bsl::pair<SyncPointOffsetConstIter, SyncPointOffsetConstIter> rcPair =
        bsl::equal_range(spOffsetPairs.begin(),
                         spOffsetPairs.end(),
                         ppState.lastSyncPointOffsetPair(),
                         SyncPointOffsetPairComparator());

    if (rcPair.first == rcPair.second) {
        // This could be a bug in replication or 'ppState.lastSyncPoint'
        // represents a bogus sync point.

        BALL_LOG_WARN << d_clusterData_p->identity().description()
                      << " Partition [" << pid << "]: Last sync point: "
                      << ppState.lastSyncPointOffsetPair()
                      << ", of peer: " << ppState.peer()->nodeDescription()
                      << ", while syncing partition.";

        return rc_PEER_SYNC_POINT_NOT_FOUND;  // RETURN
    }

    const bmqp_ctrlmsg::SyncPointOffsetPair& spOffsetPair = *(rcPair.first);
    bsls::Types::Uint64 journalSpOffset = spOffsetPair.offset();

    bsls::Types::Uint64 minJournalSize =
        journalSpOffset + mqbs::FileStoreProtocol::k_JOURNAL_RECORD_SIZE;

    if (fti.journalFd().fileSize() < minJournalSize) {
        // Yikes, this is bad.

        BMQTSK_ALARMLOG_ALARM("CLUSTER")
            << d_clusterData_p->identity().description() << " Partition ["
            << pid << "] Encountered invalid sync point JOURNAL offset: "
            << journalSpOffset
            << ", JOURNAL size: " << fti.journalFd().fileSize()
            << ", sync point: " << spOffsetPair.syncPoint() << ", when "
            << "new primary (self) is initiating partition-sync with peer:"
            << ppState.peer()->nodeDescription() << BMQTSK_ALARMLOG_END;
        return rc_INVALID_JOURNAL_OFFSET;  // RETURN
    }

    // Retrieve the sync point from JOURNAL.

    mqbs::OffsetPtr<const mqbs::RecordHeader> journalRec(
        fti.journalFd().block(),
        journalSpOffset);
    BSLS_ASSERT_SAFE(mqbs::RecordType::e_JOURNAL_OP == journalRec->type());
    static_cast<void>(journalRec);

    mqbs::OffsetPtr<const mqbs::JournalOpRecord> syncPtRec(
        fti.journalFd().block(),
        journalSpOffset);

    BSLS_ASSERT_SAFE(mqbs::JournalOpType::e_SYNCPOINT == syncPtRec->type());
    BSLS_ASSERT_SAFE(mqbs::SyncPointType::e_UNDEFINED !=
                     syncPtRec->syncPointType());

    bsls::Types::Uint64 qlistMapOffset =
        static_cast<bsls::Types::Uint64>(syncPtRec->qlistFileOffsetWords()) *
        bmqp::Protocol::k_WORD_SIZE;

    if (fti.qlistFd().fileSize() < qlistMapOffset) {
        // Yikes, this is bad.

        BMQTSK_ALARMLOG_ALARM("CLUSTER")
            << d_clusterData_p->identity().description() << " Partition ["
            << pid << "]: Invalid QLIST offset in sync point: "
            << spOffsetPair.syncPoint()
            << ". Current QLIST file size: " << fti.qlistFd().fileSize()
            << ", while syncing peer: " << ppState.peer()->nodeDescription()
            << BMQTSK_ALARMLOG_END;

        return rc_INVALID_QLIST_OFFSET;  // RETURN
    }

    bsls::Types::Uint64 dataMapOffset =
        static_cast<bsls::Types::Uint64>(syncPtRec->dataFileOffsetDwords()) *
        bmqp::Protocol::k_DWORD_SIZE;

    if (fti.dataFd().fileSize() < dataMapOffset) {
        // Yikes, this is bad.

        BMQTSK_ALARMLOG_ALARM("CLUSTER")
            << d_clusterData_p->identity().description() << " Partition ["
            << pid << "]: Invalid DATA offset in sync point: "
            << spOffsetPair.syncPoint()
            << ". Current DATA file size: " << fti.dataFd().fileSize()
            << ", while syncing peer: " << ppState.peer()->nodeDescription()
            << BMQTSK_ALARMLOG_END;

        return rc_INVALID_DATA_OFFSET;  // RETURN
    }

    int rc = replayPartition(0,  // RequestContext
                             primarySyncCtx,
                             ppState.peer(),
                             ppState.partitionSequenceNum(),
                             selfSequenceNum,
                             journalSpOffset);
    if (0 != rc) {
        BMQTSK_ALARMLOG_ALARM("CLUSTER")
            << d_clusterData_p->identity().description() << " Partition ["
            << pid << "]: New primary (self) failed to "
            << "replay partition while syncing partition in peer: "
            << ppState.peer()->nodeDescription() << ", rc: " << rc
            << BMQTSK_ALARMLOG_END;
        return rc_REPLAY_FAILURE * 10 + rc;  // RETURN
    }

    return 0;
}

bool RecoveryManager::hasSyncPoint(bmqp_ctrlmsg::SyncPoint* syncPoint,
                                   mqbs::RecordHeader*      syncPointRecHeader,
                                   bmqu::BlobPosition* syncPointHeaderPosition,
                                   int*                messageNumber,
                                   bsls::Types::Uint64* journalOffset,
                                   int                  partitionId,
                                   const bsl::shared_ptr<bdlbb::Blob>& blob,
                                   const mqbnet::ClusterNode*          source)
{
    // executed by each of the *STORAGE (QUEUE) DISPATCHER* threads

    bmqp::Event                  event(blob.get(), d_allocator_p);
    bmqp::StorageMessageIterator iter;
    *messageNumber = 0;

    event.loadStorageMessageIterator(&iter);
    BSLS_ASSERT_SAFE(iter.isValid());

    while (1 == iter.next()) {
        ++(*messageNumber);

        const bmqp::StorageHeader& header = iter.header();

        BSLS_ASSERT_SAFE(header.partitionId() ==
                         static_cast<unsigned int>(partitionId));

        BSLS_ASSERT_SAFE(header.storageProtocolVersion() ==
                         mqbs::FileStoreProtocol::k_VERSION);

        if (bmqp::StorageMessageType::e_JOURNAL_OP != header.messageType()) {
            continue;  // CONTINUE
        }

        // So we have encountered a JournalOp record in the stream.  Currently,
        // there is 1 type of JournalOp record: SYNCPOINT (see
        // mqbs::JournalOpType).  We know that SYNCPOINT can be of 2 sub-types:
        // REGULAR and ROLLOVER (see mqbs::SyncPointType).  We don't care about
        // sub-type, it can be either.

        // Load the position of journal record.
        bmqu::BlobPosition syncPointPosition;
        int                rc = iter.loadDataPosition(&syncPointPosition);
        if (0 != rc) {
            BMQTSK_ALARMLOG_ALARM("RECOVERY")
                << d_clusterData_p->identity().description()
                << ": For Partition [" << partitionId << "]"
                << ", failed to load JOURNAL_OP record position in storage "
                << "message from " << source->nodeDescription()
                << ", rc: " << rc << ". Ignoring this message."
                << BMQTSK_ALARMLOG_END;
            continue;  // CONTINUE
        }

        // Load the JOURNAL_OP record.

        bmqu::BlobObjectProxy<mqbs::JournalOpRecord> journalOpRec(
            blob.get(),
            syncPointPosition,
            true,    // read
            false);  // write
        if (!journalOpRec.isSet()) {
            // Should never happen.
            BMQTSK_ALARMLOG_ALARM("RECOVERY")
                << d_clusterData_p->identity().description()
                << ": For Partition [" << partitionId << "]"
                << ", failed to load JournalOp record in storage message from "
                << source->nodeDescription() << ". Ignoring this message."
                << BMQTSK_ALARMLOG_END;
            continue;  // CONTINUE
        }

        *syncPointRecHeader = journalOpRec->header();

        // Validate the JournalOp record.

        if (0 == syncPointRecHeader->primaryLeaseId() ||
            0 == syncPointRecHeader->sequenceNumber()) {
            // Peer sent a SyncPt containing invalid seqnum.

            BALL_LOG_ERROR
                << d_clusterData_p->identity().description()
                << ": For Partition [" << partitionId << "]"
                << ", received a SyncPt with invalid sequence number in the "
                << "RecordHeader: (" << syncPointRecHeader->primaryLeaseId()
                << ", " << syncPointRecHeader->sequenceNumber() << "), from "
                << source->nodeDescription() << ". Ignoring this message.";
            continue;  // CONTINUE
        }

        if (mqbs::JournalOpType::e_SYNCPOINT != journalOpRec->type()) {
            // This should not occur.  Per BlazingMQ replication algo, only a
            // JournalOp record of type SYNCPOINT should be encountered.

            BMQTSK_ALARMLOG_ALARM("RECOVERY")
                << d_clusterData_p->identity().description()
                << ": For Partition [" << partitionId << "]"
                << ", received a JOURNAL_OP record of type "
                << journalOpRec->type()
                << " (expected type SYNCPOINT) in storage message with "
                << "sequence number (" << syncPointRecHeader->primaryLeaseId()
                << ", " << syncPointRecHeader->sequenceNumber() << "), from "
                << source->nodeDescription() << ". Ignoring this message."
                << BMQTSK_ALARMLOG_END;
            continue;  // CONTINUE
        }

        if (mqbs::SyncPointType::e_UNDEFINED ==
            journalOpRec->syncPointType()) {
            BMQTSK_ALARMLOG_ALARM("RECOVERY")
                << d_clusterData_p->identity().description()
                << ": For partition [" << partitionId << "]"
                << ", received a syncPoint record of UNDEFINED type (expected "
                << "type REGULAR/ROLLOVER), in storage message with sequence "
                << "number (" << syncPointRecHeader->primaryLeaseId() << ", "
                << syncPointRecHeader->sequenceNumber() << "), from "
                << source->nodeDescription() << ". Ignoring this message."
                << BMQTSK_ALARMLOG_END;
            continue;  // CONTINUE
        }

        // Note that 'journalOpRec.primaryLeaseId()' may be smaller than what
        // appears in its RecordHeader if a new primary was chosen for this
        // partition *while* self node was waiting for a SyncPt, *and* as its
        // first job, the new primary issues a sync point with old leaseId &
        // sequenceNum.  Also note that in this case, sequence number in the
        // RecordHeader will be different from 'journalOpRec->sequenceNumber()'
        // (see 'FileStore::writeJournalRecord' for same comment -- for the
        // same reason).

        if (syncPointRecHeader->primaryLeaseId() <
            journalOpRec->primaryLeaseId()) {
            // This indicates bug in BlazingMQ replication logic.

            BALL_LOG_ERROR << d_clusterData_p->identity().description()
                           << ": For Partition [" << partitionId << "]"
                           << ", received a SyncPt JOURNAL_OP record "
                           << "with invalid primaryLeaseId in RecordHeader."
                           << " Sequence number in RecordHeader ("
                           << syncPointRecHeader->primaryLeaseId() << ", "
                           << syncPointRecHeader->sequenceNumber()
                           << "). Sequence number in SyncPt ("
                           << journalOpRec->primaryLeaseId() << ", "
                           << journalOpRec->sequenceNum()
                           << "). Source: " << source->nodeDescription()
                           << ". Ignoring this message.";
            continue;  // CONTINUE
        }

        if (syncPointRecHeader->primaryLeaseId() ==
            journalOpRec->primaryLeaseId()) {
            if (syncPointRecHeader->sequenceNumber() !=
                journalOpRec->sequenceNum()) {
                // If leaseId's match, sequence numbers must match too.  Again
                // look at the above comment or see
                // 'FileStore::writeJournalRecord'.

                BMQTSK_ALARMLOG_ALARM("RECOVERY")
                    << d_clusterData_p->identity().description()
                    << " Partition [" << partitionId
                    << "]: " << "received a SyncPt record with mismatched "
                    << "sequence numbers. Sequence number in RecordHeader ("
                    << syncPointRecHeader->primaryLeaseId() << ", "
                    << syncPointRecHeader->sequenceNumber()
                    << "). Sequence number in SyncPt ("
                    << journalOpRec->primaryLeaseId() << ", "
                    << journalOpRec->sequenceNum()
                    << "). Source: " << source->nodeDescription()
                    << ". Ignoring this message." << BMQTSK_ALARMLOG_END;
                continue;  // CONTINUE
            }
        }

        // TBD: compare 'source->nodeId() == journalOpRec->primaryNodeId()' ?

        // Check if its a rolled over SyncPt; if so, skip it, and wait for the
        // next one.  Reason: primary immediately issues another SyncPt (which
        // is the 1st SyncPt in the new journal).  This rolled over SyncPt
        // cannot be used as primary will no longer have it (to be precise, it
        // will have the SyncPt but with a different (lower) offset in the new
        // journal).  In addition, we will explicitly clear out the retrieved
        // (aka "old") SyncPt, so that when a new SyncPt is received, self node
        // will send a storageSyncRequest with this range [null, NewSyncPt],
        // which will make peer to send entire file, which is the desired
        // behavior in this scenario.

        if (mqbs::SyncPointType::e_ROLLOVER == journalOpRec->syncPointType()) {
            BALL_LOG_INFO << d_clusterData_p->identity().description()
                          << " Partition [" << partitionId << "] :"
                          << "received a rolled-over SyncPt, but skipping it. "
                          << "Sequence number in RecordHeader ("
                          << syncPointRecHeader->primaryLeaseId() << ", "
                          << syncPointRecHeader->sequenceNumber()
                          << "). Sequence number in SyncPt ("
                          << journalOpRec->primaryLeaseId() << ", "
                          << journalOpRec->sequenceNum()
                          << "). Source: " << source->nodeDescription();

            bmqp_ctrlmsg::SyncPoint dummySyncPt;
            RecoveryContext& recoveryCtx = d_recoveryContexts[partitionId];
            recoveryCtx.setOldSyncPoint(dummySyncPt);
            recoveryCtx.setOldSyncPointOffset(0);
            recoveryCtx.setJournalFileOffset(0);
            recoveryCtx.setDataFileOffset(0);
            recoveryCtx.setQlistFileOffset(0);

            continue;  // CONTINUE
        }

        // This is a valid SyncPt.  Retrieve leaseId and seqNum from the SyncPt
        // payload, not the RecordHeader (same reasoning as above -- new
        // primary could be issuing a SyncPt on behalf of previous primary).

        syncPoint->primaryLeaseId() = journalOpRec->primaryLeaseId();
        syncPoint->sequenceNum()    = journalOpRec->sequenceNum();
        syncPoint->dataFileOffsetDwords() =
            journalOpRec->dataFileOffsetDwords();
        syncPoint->qlistFileOffsetWords() =
            journalOpRec->qlistFileOffsetWords();
        *syncPointHeaderPosition = iter.headerPosition();
        *journalOffset           = static_cast<bsls::Types::Uint64>(
                             header.journalOffsetWords()) *
                         bmqp::Protocol::k_WORD_SIZE;

        return true;  // RETURN
    }  // end 'while' loop

    return false;
}

void RecoveryManager::onPartitionSyncStateQueryResponse(
    const RequestContextSp& requestContext)
{
    // executed by the cluster *DISPATCHER* thread

    // Dispatch response to appropriate thread.

    const bmqp_ctrlmsg::PartitionSyncStateQuery& req =
        requestContext->request()
            .choice()
            .clusterMessage()
            .choice()
            .partitionSyncStateQuery();

    BSLS_ASSERT_SAFE(d_partitionsInfo.size() >
                     static_cast<unsigned int>(req.partitionId()));
    BSLS_ASSERT_SAFE(d_primarySyncContexts.size() >
                     static_cast<unsigned int>(req.partitionId()));

    const mqbi::DispatcherClientData& dispData =
        d_partitionsInfo[req.partitionId()].dispatcherClientData();

    d_dispatcher_p->execute(
        bdlf::BindUtil::bind(
            &RecoveryManager::onPartitionSyncStateQueryResponseDispatched,
            this,
            req.partitionId(),
            requestContext),
        dispData);
}

void RecoveryManager::onPartitionSyncStateQueryResponseDispatched(
    int                     partitionId,
    const RequestContextSp& requestContext)
{
    // executed by *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     static_cast<unsigned int>(partitionId) <
                         d_primarySyncContexts.size());

    PrimarySyncContext& primarySyncCtx = d_primarySyncContexts[partitionId];
    const NodeResponsePairs& pairs     = requestContext->response();

    // A new primary never sends request to zero peers.

    BSLS_ASSERT_SAFE(!pairs.empty());

    mqbnet::ClusterNode* maxSeqNode = d_clusterData_p->membership().selfNode();
    bmqp_ctrlmsg::PartitionSequenceNumber maxSeq =
        primarySyncCtx.selfPartitionSequenceNum();
    bmqp_ctrlmsg::SyncPoint maxSeqNodeSyncPt =
        primarySyncCtx.selfLastSyncPtOffsetPair().syncPoint();

    BALL_LOG_DEBUG << d_clusterData_p->identity().description()
                   << " Partition [" << partitionId << "]: processing "
                   << pairs.size() << " partition sync state query responses";

    for (NodeResponsePairsConstIter it = pairs.begin(); it != pairs.end();
         ++it) {
        BSLS_ASSERT_SAFE(it->first);

        // Retrieve the 'PeerPartitionState' of 'it->first' (peer node).

        PeerPartitionState*  ppState  = 0;
        PeerPartitionStates& ppStates = primarySyncCtx.peerPartitionStates();
        for (size_t idx = 0; ppStates.size(); ++idx) {
            if (ppStates[idx].peer() == it->first) {
                ppState = &(ppStates[idx]);
                break;  // BREAK
            }
        }

        BSLS_ASSERT_SAFE(ppState);

        if (it->second.choice().isStatusValue()) {
            BALL_LOG_WARN << d_clusterData_p->identity().description()
                          << " Partition [" << partitionId
                          << "]: Received failed partition sync state query "
                          << "response " << it->second.choice().status()
                          << " from " << it->first->nodeDescription()
                          << ". Skipping this node's response.";

            ppState->setNeedsPartitionSync(false);
            continue;  // CONTINUE
        }

        BSLS_ASSERT_SAFE(it->second.choice().isClusterMessageValue());
        BSLS_ASSERT_SAFE(it->second.choice()
                             .clusterMessage()
                             .choice()
                             .isPartitionSyncStateQueryResponseValue());

        const bmqp_ctrlmsg::PartitionSyncStateQueryResponse& r =
            it->second.choice()
                .clusterMessage()
                .choice()
                .partitionSyncStateQueryResponse();

        BALL_LOG_INFO << "Received partition primary sync query response " << r
                      << " from " << it->first->nodeDescription();

        if (partitionId != r.partitionId()) {
            BALL_LOG_WARN << d_clusterData_p->identity().description()
                          << " Partition [" << partitionId
                          << "]: Invalid partitionId specified in partition "
                          << "primary sync query response: " << r.partitionId()
                          << ", from " << it->first->nodeDescription();

            ppState->setNeedsPartitionSync(false);
            continue;  // CONTINUE
        }

        if (0 == r.primaryLeaseId()) {
            // Either this peer never saw a primary for this partition, or it
            // has other issues.  TBD: self should attempt to bring this node
            // upto speed.
            BALL_LOG_WARN
                << d_clusterData_p->identity().description() << " Partition ["
                << partitionId
                << "]: Invalid primaryLeaseId specified in partition "
                << "primary sync query response" << ", from "
                << it->first->nodeDescription();

            ppState->setNeedsPartitionSync(false);
            continue;  // CONTINUE
        }

        // It is ok to have r.sequenceNum() to be zero.  This could occur if
        // old primary crashed as soon as becoming an active primary, but
        // before issuing any new SyncPt.

        // Note that both, leaseId and sequenceNum, fields in 'r.lastSyncPoint'
        // field could be zero if peer just came up, and there was no storage.
        // Also note that leaseId in that SyncPt can be smaller than the one in
        // 'r.primaryLeaseId' field.  This could occur if old primary crashed
        // as soon as it transitioned to active primary, but before it could
        // issue any new SyncPt.  Because of these reasons, we can't perform
        // any reasonable validations on 'r.lastSyncPoint' field.

        bmqp_ctrlmsg::PartitionSequenceNumber peerSeqNum;
        peerSeqNum.primaryLeaseId() = r.primaryLeaseId();
        peerSeqNum.sequenceNumber() = r.sequenceNum();

        if (maxSeq < peerSeqNum) {
            maxSeq           = peerSeqNum;
            maxSeqNode       = it->first;
            maxSeqNodeSyncPt = r.lastSyncPointOffsetPair().syncPoint();
        }

        ppState->setPartitionSequenceNum(peerSeqNum);
        ppState->setLastSyncPointOffsetPair(r.lastSyncPointOffsetPair());

        ppState->setNeedsPartitionSync(true);
    }

    BSLS_ASSERT_SAFE(maxSeqNode);

    if (maxSeqNode == d_clusterData_p->membership().selfNode()) {
        BALL_LOG_INFO << d_clusterData_p->identity().description()
                      << " Partition [" << partitionId
                      << "]: Self node has latest view of partition during "
                      << "primary sync, with sequence number " << maxSeq
                      << ". Self node will now attempt to sync replica peers.";

        syncPeerPartitions(&primarySyncCtx);

        // TBD: even though partition-sync may have failed w/ one or more
        // peers, we still notify partition-sync success to StorageMgr.  How
        // else could this be handled?

        // TBD: The below 'sleep' is an ugly *TEMPORARY* HACK around a network
        //      delay race condition: when the current primary (M1) of a
        //      partition goes down, it broadcasts a message to the peer nodes.
        //      Upon reception of that message, each node updates its internal
        //      state to reflect that the partitions this node was primary of
        //      is now orphan.  The leader (L) will select a new primary (M2)
        //      and broadcast that new mapping (msg1).  Upon reception of
        //      (msg1), the nodes update their internal state; and (M2)
        //      initiates a sync of it's own state as well as of the peers, to
        //      ensure all nodes in the cluster have the same view of the
        //      partition.  Once done, (M2) emits a 'primaryStatus = active'
        //      message (msg2).  If a replica node receives (msg2) before it
        //      has received (msg1) (potentially because there was a long IO
        //      queue between (L) and that replica, it will then ignore (msg2)
        //      and the partition will remain in 'inactive' state 'forever',
        //      making all operations such as 'openQueue' timeout.  The below
        //      sleep delays the notification of (msg2) in the hope that with
        //      that delay, (msg1) will have been received by all peers.  This
        //      hack is not needed if leader is configured to be primary for
        //      all nodes.

        if (mqbcfg::MasterAssignmentAlgorithm::E_LEADER_IS_MASTER_ALL !=
            d_clusterConfig.masterAssignment()) {
            BALL_LOG_INFO << "[HACK] Delaying transitioning to 'active' for "
                          << "partition [" << partitionId << "]";
            bslmt::ThreadUtil::microSleep(0, 3);  // micro seconds, seconds
        }

        onPartitionPrimarySyncStatus(partitionId, 0 /* status */);
        return;  // RETURN
    }

    // One of the peers is ahead.

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << " Partition [" << partitionId << "]: Peer "
                  << maxSeqNode->nodeDescription()
                  << " has most advanced view of the partition during primary "
                  << "sync " << maxSeq
                  << ", last sync-point: " << maxSeqNodeSyncPt
                  << ".  Will send primary sync data query to it.";

    primarySyncCtx.setPrimarySyncPeer(maxSeqNode);

    // Send PartitionSyncDataQuery to this peer.

    RequestManagerType::RequestSp request =
        d_clusterData_p->requestManager().createRequest();

    bmqp_ctrlmsg::PartitionSyncDataQuery& partitionSyncDataReq =
        request->request()
            .choice()
            .makeClusterMessage()
            .choice()
            .makePartitionSyncDataQuery();

    partitionSyncDataReq.partitionId() = partitionId;
    partitionSyncDataReq.lastPrimaryLeaseId() =
        primarySyncCtx.selfPartitionSequenceNum().primaryLeaseId();
    partitionSyncDataReq.lastSequenceNum() =
        primarySyncCtx.selfPartitionSequenceNum().sequenceNumber();
    partitionSyncDataReq.uptoPrimaryLeaseId() = maxSeq.primaryLeaseId();
    partitionSyncDataReq.uptoSequenceNum()    = maxSeq.sequenceNumber();
    partitionSyncDataReq.lastSyncPointOffsetPair() =
        primarySyncCtx.selfLastSyncPtOffsetPair();
    // Note that if self has an empty partition (ie, no records whatsoever,
    // only file headers), all of the above fields (lastPrimaryLeaseId,
    // lastSequenceNum and lastSyncPoint) will be null/zero.  While it
    // should not be possible for the partition to be empty (because the
    // recovery at startup should have brought partition upto speed), we
    // still send this request to the peer and let it deal with it.

    request->setResponseCb(bdlf::BindUtil::bind(
        &RecoveryManager::onPartitionSyncDataQueryResponse,
        this,
        bdlf::PlaceHolders::_1,
        maxSeqNode));

    bsls::TimeInterval timeoutMs;
    timeoutMs.setTotalMilliseconds(d_clusterConfig.partitionConfig()
                                       .syncConfig()
                                       .partitionSyncDataReqTimeoutMs());
    bmqt::GenericResult::Enum status =
        d_clusterData_p->cluster().sendRequest(request, maxSeqNode, timeoutMs);

    if (bmqt::GenericResult::e_SUCCESS != status) {
        // Request failed to encode/be sent; process error handling (note that
        // 'onPartitionSyncDataQueryResponse' won't be invoked in this case).

        BMQTSK_ALARMLOG_ALARM("RECOVERY")
            << d_clusterData_p->identity().description() << ": For Partition ["
            << partitionId << "], failed to send "
            << "partition sync data query to node "
            << maxSeqNode->nodeDescription() << ", rc: " << status
            << ". No retry attempt will be made." << BMQTSK_ALARMLOG_END;

        onPartitionPrimarySyncStatus(partitionId, -1 /* status */);

        // TBD: reschedule recovery.
    }
}

void RecoveryManager::onPartitionSyncDataQueryResponse(
    const RequestManagerType::RequestSp& context,
    const mqbnet::ClusterNode*           responder)
{
    // executed by *ANY* thread

    // Dispatch response to appropriate thread.

    const bmqp_ctrlmsg::PartitionSyncDataQuery& req =
        context->request()
            .choice()
            .clusterMessage()
            .choice()
            .partitionSyncDataQuery();

    BSLS_ASSERT_SAFE(d_partitionsInfo.size() >
                     static_cast<unsigned int>(req.partitionId()));
    BSLS_ASSERT_SAFE(d_primarySyncContexts.size() >
                     static_cast<unsigned int>(req.partitionId()));

    const mqbi::DispatcherClientData& dispData =
        d_partitionsInfo[req.partitionId()].dispatcherClientData();
    d_dispatcher_p->execute(
        bdlf::BindUtil::bind(
            &RecoveryManager::onPartitionSyncDataQueryResponseDispatched,
            this,
            req.partitionId(),
            context,
            responder),
        dispData);
}

void RecoveryManager::onPartitionSyncDataQueryResponseDispatched(
    int                                  partitionId,
    const RequestManagerType::RequestSp& context,
    const mqbnet::ClusterNode*           responder)
{
    // executed by *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     static_cast<unsigned int>(partitionId) <
                         d_primarySyncContexts.size());

    PrimarySyncContext& primarySyncCtx = d_primarySyncContexts[partitionId];

    const bmqp_ctrlmsg::PartitionSyncDataQuery& req =
        context->request()
            .choice()
            .clusterMessage()
            .choice()
            .partitionSyncDataQuery();

    if (context->response().choice().isStatusValue()) {
        BMQTSK_ALARMLOG_ALARM("RECOVERY")
            << d_clusterData_p->identity().description() << " Partition ["
            << partitionId
            << "]: " << "received partition sync data query failure response: "
            << context->response().choice().status()
            << " from node: " << responder->nodeDescription()
            << " for request: " << req << ". No retry attempt will be made."
            << BMQTSK_ALARMLOG_END;

        onPartitionPrimarySyncStatus(partitionId, -1 /* status */);

        // TBD: reschedule partition sync.

        return;  // RETURN
    }

    BSLS_ASSERT(context->response().choice().isClusterMessageValue());
    BSLS_ASSERT(context->response()
                    .choice()
                    .clusterMessage()
                    .choice()
                    .isPartitionSyncDataQueryResponseValue());

    const bmqp_ctrlmsg::PartitionSyncDataQueryResponse& response =
        context->response()
            .choice()
            .clusterMessage()
            .choice()
            .partitionSyncDataQueryResponse();

    if (response.partitionId() != partitionId) {
        BMQTSK_ALARMLOG_ALARM("RECOVERY")
            << d_clusterData_p->identity().description() << " Partition ["
            << partitionId << "]: "
            << "invalid partitionId specified in response: " << response
            << " from node: " << responder->nodeDescription()
            << " for partition sync data request: " << req
            << ". No retry attempt will be made." << BMQTSK_ALARMLOG_END;

        onPartitionPrimarySyncStatus(partitionId, -1 /* status */);

        // TBD: reschedule partition sync.

        return;  // RETURN
    }

    if (!primarySyncCtx.primarySyncInProgress()) {
        BALL_LOG_WARN << d_clusterData_p->identity().description()
                      << " Partition [" << partitionId << "]: "
                      << "received partition sync data response from: "
                      << responder->nodeDescription()
                      << " for request: " << req << ", but partition is no "
                      << "longer waiting to sync (probably due to timeout). "
                      << "Ignoring this response.";
        return;  // RETURN
    }

    if (primarySyncCtx.syncPeer() != responder) {
        BALL_LOG_WARN << d_clusterData_p->identity().description()
                      << " Partition [" << partitionId
                      << "]: " << "received partition sync data response from "
                      << "unexpected node: " << responder->nodeDescription()
                      << " for request: " << req << ", expected node: "
                      << primarySyncCtx.syncPeer()->nodeDescription()
                      << ". Ignoring this response.";
        return;  // RETURN
    }

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << " Partition [" << partitionId
                  << "]: received partition sync data response: " << response
                  << " from node: " << responder->nodeDescription();

    bmqp_ctrlmsg::PartitionSequenceNumber peerPartitionSeqNum;
    peerPartitionSeqNum.primaryLeaseId() = response.endPrimaryLeaseId();
    peerPartitionSeqNum.sequenceNumber() = response.endSequenceNum();

    if (peerPartitionSeqNum <= primarySyncCtx.selfPartitionSequenceNum()) {
        BMQTSK_ALARMLOG_ALARM("RECOVERY")
            << d_clusterData_p->identity().description() << " Partition ["
            << partitionId
            << "]: " << "invalid partition sequenceNum specified in response: "
            << response << " from node: " << responder->nodeDescription()
            << " for partition sync data request: " << req
            << ". Self partition seqNum: "
            << primarySyncCtx.selfPartitionSequenceNum()
            << "No retry attempt will be made." << BMQTSK_ALARMLOG_END;
        onPartitionPrimarySyncStatus(partitionId, -1 /* status */);

        // TBD: reschedule partition sync.

        return;  // RETURN
    }

    // All good wait for peer to sync the partition now.  Those events will be
    // routed directly to the FileStore.
}

// CREATORS
RecoveryManager::RecoveryManager(
    const mqbcfg::ClusterDefinition& clusterConfig,
    mqbi::DispatcherClient*          cluster,
    mqbc::ClusterData*               clusterData,
    const mqbs::DataStoreConfig&     dataStoreConfig,
    mqbi::Dispatcher*                dispatcher,
    bslma::Allocator*                allocator)
: d_allocator_p(allocator)
, d_isStarted(false)
, d_dispatcher_p(dispatcher)
, d_clusterConfig(clusterConfig)
, d_cluster_p(cluster)
, d_clusterData_p(clusterData)
, d_dataStoreConfig(dataStoreConfig)
, d_partitionsInfo(allocator)
, d_recoveryContexts(allocator)
, d_primarySyncContexts(allocator)
, d_recoveryRequestContexts(allocator)
, d_primarySyncRequestContexts(allocator)
, d_recoveryRequestContextLock(bsls::SpinLock::s_unlocked)
, d_primarySyncRequestContextLock(bsls::SpinLock::s_unlocked)
{
    BSLS_ASSERT_SAFE(d_allocator_p);
    BSLS_ASSERT_SAFE(d_dispatcher_p);
    BSLS_ASSERT_SAFE(d_cluster_p);

    BSLS_ASSERT(
        static_cast<unsigned int>(
            d_clusterConfig.partitionConfig().syncConfig().fileChunkSize()) <
        bmqp::RecoveryHeader::k_MAX_PAYLOAD_SIZE_SOFT);
    BSLS_ASSERT(static_cast<unsigned int>(d_clusterConfig.partitionConfig()
                                              .syncConfig()
                                              .partitionSyncEventSize()) <
                bmqp::RecoveryHeader::k_MAX_PAYLOAD_SIZE_SOFT);

    d_partitionsInfo.resize(d_clusterConfig.partitionConfig().numPartitions());
    d_recoveryContexts.resize(
        d_clusterConfig.partitionConfig().numPartitions());
    d_primarySyncContexts.resize(
        d_clusterConfig.partitionConfig().numPartitions());
}

RecoveryManager::~RecoveryManager()
{
    BSLS_ASSERT_SAFE(!isStarted() &&
                     "'stop()' must be called before the destructor");
}

// MANIPULATORS
int RecoveryManager::start(bsl::ostream& errorDescription)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_dispatcher_p->inDispatcherThread(d_cluster_p));

    if (isStarted()) {
        errorDescription << "Already started.";
        return -1;  // RETURN
    }

    BSLS_ASSERT_SAFE(static_cast<int>(d_partitionsInfo.size()) ==
                     d_clusterConfig.partitionConfig().numPartitions());
    BSLS_ASSERT_SAFE(static_cast<int>(d_recoveryContexts.size()) ==
                     d_clusterConfig.partitionConfig().numPartitions());
    BSLS_ASSERT_SAFE(static_cast<int>(d_primarySyncContexts.size()) ==
                     d_clusterConfig.partitionConfig().numPartitions());

    d_isStarted = true;

    d_clusterData_p->membership().netCluster()->registerObserver(this);

    return 0;
}

void RecoveryManager::stop()
{
    if (!isStarted()) {
        return;  // RETURN
    }

    d_isStarted = false;

    d_clusterData_p->membership().netCluster()->unregisterObserver(this);

    // Enqueue event in each partition's dispatcher thread to stop partition
    // recovery as well as primary sync.

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << " Enqueuing event to all partition dispatcher threads to "
                  << "stop any ongoing recovery or primary-sync, and waiting.";

    bslmt::Latch latch(d_partitionsInfo.size());
    for (unsigned int i = 0; i < d_partitionsInfo.size(); ++i) {
        const mqbi::DispatcherClientData& dispData =
            d_partitionsInfo[i].dispatcherClientData();
        d_dispatcher_p->execute(
            bdlf::BindUtil::bind(&RecoveryManager::stopDispatched,
                                 this,
                                 static_cast<int>(i),  // partitionId
                                 &latch),
            dispData);
    }

    latch.wait();
    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << " RecoveryManager stopped.";
}

void RecoveryManager::startRecovery(
    int                               partitionId,
    const mqbi::DispatcherClientData& dispatcherData,
    const PartitionRecoveryCb&        partitionRecoveryCb)
{
    // executed by each of the *STORAGE (QUEUE) DISPATCHER* threads

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(0 <= partitionId);
    BSLS_ASSERT_SAFE(static_cast<unsigned int>(partitionId) <
                     d_partitionsInfo.size());
    BSLS_ASSERT_SAFE(static_cast<unsigned int>(partitionId) <
                     d_recoveryContexts.size());
    BSLS_ASSERT_SAFE(partitionRecoveryCb);
    BSLS_ASSERT_SAFE(!isRecoveryInProgress(partitionId));
    d_partitionsInfo[partitionId].setDispatcherClientData(dispatcherData);

    RecoveryContext& recoveryCtx = d_recoveryContexts[partitionId];

    recoveryCtx.setRecoveryCb(partitionRecoveryCb);

    // Currently, this routine doesn't return error, so we update the recovery
    // status of this partition right away.

    recoveryCtx.setRecoveryStatus(true);

    if (d_clusterData_p->cluster().isLocal()) {
        onPartitionRecoveryStatus(partitionId, 0 /* status */);
        return;  // RETURN
    }

    bmqp_ctrlmsg::SyncPoint syncPoint;
    recoveryCtx.setOldSyncPoint(syncPoint);
    recoveryCtx.setOldSyncPointOffset(0);
    recoveryCtx.setJournalFileOffset(0);
    recoveryCtx.setDataFileOffset(0);
    recoveryCtx.setQlistFileOffset(0);

    // Schedule an event which will check if a sync-point has been received
    // for this partition.  If no, then self node will try to recovery w/ any
    // AVAILABLE peer.

    // Add a randomness between 0-5 seconds for the sync point wait, so that
    // in case all nodes in the cluster are started at the same time, they
    // don't wait the same amount of time.  Adding randomness here will ensure
    // at least one node proceeds with recovery and becomes available, thereby
    // syncing other nodes.
    const int startupWaitMs = d_clusterConfig.partitionConfig()
                                  .syncConfig()
                                  .startupWaitDurationMs() +
                              bsl::rand() % 5000;

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << ": For Partition [" << partitionId
                  << "], will check after " << startupWaitMs << " millisec "
                  << "if any sync point has been received by then.";

    bsls::TimeInterval after(bmqsys::Time::nowMonotonicClock());
    after.addMilliseconds(startupWaitMs);
    d_clusterData_p->scheduler().scheduleEvent(
        &recoveryCtx.recoveryStartupWaitHandle(),
        after,
        bdlf::BindUtil::bind(&RecoveryManager::recoveryStartupWaitCb,
                             this,
                             partitionId));

    // Max number of file sets that we want to be inspected.  TBD: add reason
    // for '2'.

    const int           k_MAX_NUM_FILE_SETS_TO_CHECK = 2;
    int                 rc                           = 0;
    bmqu::MemOutStream  errorDesc;
    bsls::Types::Uint64 journalFilePos;
    bsls::Types::Uint64 dataFilePos;

    rc = mqbs::FileStoreUtil::openRecoveryFileSet(errorDesc,
                                                  &recoveryCtx.journalFd(),
                                                  &recoveryCtx.dataFd(),
                                                  &recoveryCtx.fileSet(),
                                                  &journalFilePos,
                                                  &dataFilePos,
                                                  partitionId,
                                                  k_MAX_NUM_FILE_SETS_TO_CHECK,
                                                  d_dataStoreConfig,
                                                  true,   // readOnly
                                                  false,  // isFSMWorkflow
                                                  &recoveryCtx.qlistFd());

    if ((rc != 0) && (rc != 1)) {
        BMQTSK_ALARMLOG_ALARM("RECOVERY")
            << d_clusterData_p->identity().description() << ": For Partition ["
            << partitionId << "], failed to find or "
            << "open a recoverable file set, rc: " << rc
            << ", reason: " << errorDesc.str()
            << ". Recovery will proceed as if this node had no local "
            << "recoverable files for this partition." << BMQTSK_ALARMLOG_END;

        return;  // RETURN
    }

    if (rc == 1) {
        // Special 'rc' implying no file sets present => no sync point.
        BALL_LOG_INFO << d_clusterData_p->identity().description()
                      << ": For Partition [" << partitionId
                      << "], no recoverable file sets found, which implies no "
                      << "last sync point.";

        return;  // RETURN
    }

    BALL_LOG_INFO << d_clusterData_p->identity().description() << ": For"
                  << " Partition [" << partitionId << "], file set opened"
                  << " for recovery: " << recoveryCtx.fileSet();

    mqbs::JournalFileIterator jit;
    mqbs::QlistFileIterator   qit;
    mqbs::DataFileIterator    dit;
    rc = mqbs::FileStoreUtil::loadIterators(errorDesc,
                                            &jit,
                                            &dit,
                                            &qit,
                                            recoveryCtx.journalFd(),
                                            recoveryCtx.dataFd(),
                                            recoveryCtx.qlistFd(),
                                            recoveryCtx.fileSet());
    if (0 != rc) {
        rc = mqbs::FileStoreUtil::closePartitionSet(&recoveryCtx.dataFd(),
                                                    &recoveryCtx.journalFd(),
                                                    &recoveryCtx.qlistFd());
        if (rc != 0) {
            // Failed to close one or more partition files
            BALL_LOG_ERROR << "For Partition [" << partitionId << "], failed"
                           << " to close one or more partition files "
                           << "[journal: "
                           << recoveryCtx.fileSet().journalFile()
                           << ", data: " << recoveryCtx.fileSet().dataFile()
                           << ", qlist: " << recoveryCtx.fileSet().qlistFile()
                           << "], rc: " << rc;
        }

        BMQTSK_ALARMLOG_ALARM("RECOVERY")
            << d_clusterData_p->identity().description() << ": For Partition ["
            << partitionId << "], "
            << "failed to load iterator(s) for recoverable file set, "
            << "rc: " << rc << ", reason: " << errorDesc.str()
            << ". Recovery will proceed as if this node had no local "
            << "recoverable files for this partition." << BMQTSK_ALARMLOG_END;

        return;  // RETURN
    }

    // Retrieve old sync point from the journal, if there is one.

    bsls::Types::Uint64 lastSyncPointOffset = jit.lastSyncPointPosition();
    if (0 == lastSyncPointOffset) {
        // Nothing to retrieve from local storage.

        BALL_LOG_INFO << d_clusterData_p->identity().description()
                      << ": For Partition [" << partitionId
                      << "], no sync point found in local storage. Recovery "
                      << "will proceed as if this node had no local "
                      << "recoverable files for this partition.";
        return;  // RETURN
    }

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << " Partition [" << partitionId
                  << "]: Potential last sync point offset retrieved at: "
                  << bmqu::PrintUtil::prettyNumber(
                         static_cast<bsls::Types::Int64>(lastSyncPointOffset));

    // Check for the validity of the retrieved old sync point.

    const mqbs::JournalOpRecord& journalOpRec = jit.lastSyncPoint();
    BSLS_ASSERT_SAFE(mqbs::JournalOpType::e_SYNCPOINT == journalOpRec.type());

    if (mqbs::SyncPointType::e_UNDEFINED == journalOpRec.syncPointType()) {
        BMQTSK_ALARMLOG_ALARM("RECOVERY")
            << d_clusterData_p->identity().description() << ": For Partition ["
            << partitionId << "], last sync point has"
            << " invalid SyncPt sub-type: " << journalOpRec.syncPointType()
            << ". Ignoring this sync point. Recovery will proceed as if this "
            << "node had no local recoverable files for this partition."
            << BMQTSK_ALARMLOG_END;
        return;  // RETURN
    }

    if (mqbs::SyncPointType::e_ROLLOVER == journalOpRec.syncPointType()) {
        // This means that in its previous instance, this node crashed after
        // writing SyncPt with rollover sub-type to file, but before completing
        // the rollover step.  So it should just request the peer to send
        // latest file set for this partition, and ignore local files
        // altogether.

        BALL_LOG_INFO << d_clusterData_p->identity().description()
                      << " Partition [" << partitionId
                      << "]: Last sync point retrieved during recovery has "
                      << "sub-type [" << journalOpRec.syncPointType()
                      << "]. Recovery will proceed as if this node had no "
                      << "local recoverable files for this partition.";
        return;  // RETURN
    }

    if (0 == journalOpRec.primaryLeaseId() ||
        0 == journalOpRec.sequenceNum()) {
        BMQTSK_ALARMLOG_ALARM("RECOVERY")
            << d_clusterData_p->identity().description() << ": For Partition ["
            << partitionId << "], "
            << "last sync point has invalid primaryLeaseId: "
            << journalOpRec.primaryLeaseId()
            << " or sequenceNum: " << journalOpRec.sequenceNum()
            << ". Ignoring this sync point. Recovery will proceed as if this "
            << "node had no local recoverable files for this partition."
            << BMQTSK_ALARMLOG_END;
        return;  // RETURN
    }

    bsls::Types::Uint64 dataFileOffset =
        static_cast<bsls::Types::Uint64>(journalOpRec.dataFileOffsetDwords()) *
        bmqp::Protocol::k_DWORD_SIZE;

    if (bdls::FilesystemUtil::getFileSize(recoveryCtx.fileSet().dataFile()) <
        static_cast<bsls::Types::Int64>(dataFileOffset)) {
        BMQTSK_ALARMLOG_ALARM("RECOVERY")
            << d_clusterData_p->identity().description() << ": For Partition ["
            << partitionId << "], data file size is "
            << "smaller than data file offset present in last sync point ["
            << bdls::FilesystemUtil::getFileSize(
                   recoveryCtx.fileSet().dataFile())
            << " < " << dataFileOffset << "]. Ignoring this sync point. "
            << "Recovery will proceed as if this node had no local recoverable"
            << " files for this partition." << BMQTSK_ALARMLOG_END;
        return;  // RETURN
    }

    bsls::Types::Uint64 qlistFileOffset =
        static_cast<bsls::Types::Uint64>(journalOpRec.qlistFileOffsetWords()) *
        bmqp::Protocol::k_WORD_SIZE;

    if (bdls::FilesystemUtil::getFileSize(recoveryCtx.fileSet().qlistFile()) <
        static_cast<bsls::Types::Int64>(qlistFileOffset)) {
        BMQTSK_ALARMLOG_ALARM("RECOVERY")
            << d_clusterData_p->identity().description() << ": For Partition ["
            << partitionId << "], QLIST file size is "
            << "smaller than QLIST file offset present in last sync point ["
            << bdls::FilesystemUtil::getFileSize(
                   recoveryCtx.fileSet().qlistFile())
            << " < " << qlistFileOffset << "]. Ignoring this sync point. "
            << "Recovery will proceed as if this node had no local recoverable"
            << " files for this partition." << BMQTSK_ALARMLOG_END;
        return;  // RETURN
    }

    // Retrieved old sync point is valid.

    syncPoint.primaryLeaseId()       = journalOpRec.primaryLeaseId();
    syncPoint.sequenceNum()          = journalOpRec.sequenceNum();
    syncPoint.dataFileOffsetDwords() = journalOpRec.dataFileOffsetDwords();
    syncPoint.qlistFileOffsetWords() = journalOpRec.qlistFileOffsetWords();

    recoveryCtx.setOldSyncPoint(syncPoint);
    recoveryCtx.setOldSyncPointOffset(lastSyncPointOffset);
    recoveryCtx.setJournalFileOffset(
        lastSyncPointOffset + mqbs::FileStoreProtocol::k_JOURNAL_RECORD_SIZE);
    recoveryCtx.setDataFileOffset(dataFileOffset);
    recoveryCtx.setQlistFileOffset(qlistFileOffset);

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << ": For Partition [" << partitionId << "], retrieved "
                  << "old SyncPt at journal offset: "
                  << bmqu::PrintUtil::prettyNumber(
                         static_cast<bsls::Types::Int64>(lastSyncPointOffset))
                  << ". SyncPt details: " << syncPoint
                  << ". Sequence number is SyncPt's RecordHeader ("
                  << journalOpRec.header().primaryLeaseId() << ", "
                  << journalOpRec.header().sequenceNumber() << ").";
}

void RecoveryManager::processStorageEvent(
    int                                 partitionId,
    const bsl::shared_ptr<bdlbb::Blob>& blob,
    mqbnet::ClusterNode*                source)
{
    // executed by the *STORAGE (QUEUE) DISPATCHER* thread

    if (!isRecoveryInProgress(partitionId)) {
        // Partition may not be in recovery.  This could be due to the fact
        // that recovery has not been started yet, or because it was
        // stopped/cancelled (because of timeout etc).
        return;  // RETURN
    }

    RecoveryContext& recoveryCtx = d_recoveryContexts[partitionId];
    if (recoveryCtx.recoveryPeer()) {
        // This partition is under active recovery.  If its recovering from the
        // specified 'source', add the storage event to the list of buffered
        // events, which will be applied later, else ignore the event.
        // 'source' can be different from the current recovery peer if there
        // was a primary switch for this partition while this active recovery
        // is in progress.  'source' represents the new primary in this
        // scenario. StorageMgr has taken note of this and will initiate a
        // recovery with new primary (ie, 'source') once this round of recovery
        // is complete.

        if (recoveryCtx.recoveryPeer() == source) {
            recoveryCtx.addStorageEvent(blob);

            BALL_LOG_INFO << d_clusterData_p->identity().description()
                          << ": For Partition [" << partitionId
                          << "], buffered a storage event from source node "
                          << source->nodeDescription()
                          << " as self is syncing storage with that node.";
        }
        else {
            BALL_LOG_WARN << d_clusterData_p->identity().description()
                          << ": For Partition [" << partitionId
                          << "], received storage event from source node "
                          << source->nodeDescription() << ", while self is "
                          << "syncing storage with "
                          << recoveryCtx.recoveryPeer()->nodeDescription()
                          << ". Dropping this event as recovery will be "
                          << "rescheduled with the new primary, if any.";
        }

        return;  // RETURN
    }

    // Partition is not under active recovery.  Iterate over the storage event
    // 'blob' to see if there is a sync point.

    bmqp_ctrlmsg::SyncPoint syncPoint;
    bmqu::BlobPosition      syncPointHeaderPosition;
    int                     msgNumber            = 0;
    bsls::Types::Uint64     primaryJournalOffset = 0;
    mqbs::RecordHeader      syncPointRecHeader;
    if (!hasSyncPoint(&syncPoint,
                      &syncPointRecHeader,
                      &syncPointHeaderPosition,
                      &msgNumber,
                      &primaryJournalOffset,
                      partitionId,
                      blob,
                      source)) {
        return;  // RETURN
    }

    // Encountered a sync point in the stream.  Buffer all storage messages in
    // the 'blob' (*after* the sync point).  The sync point storage message
    // can appear anywhere in the event, so we need to keep track of its
    // position.

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << ": For Partition [" << partitionId
                  << "], received sync point " << syncPoint << " from node "
                  << source->nodeDescription()
                  << ". Primary's journal offset: " << primaryJournalOffset
                  << ". Sequence number in SyncPt's RecordHeader: ("
                  << syncPointRecHeader.primaryLeaseId() << ", "
                  << syncPointRecHeader.sequenceNumber() << ").";

    bmqp::Event                  event(blob.get(), d_allocator_p);
    bmqp::StorageMessageIterator iter;

    event.loadStorageMessageIterator(&iter);
    BSLS_ASSERT_SAFE(iter.isValid());

    // Skip every message upto (*including*) the sync point.

    while (msgNumber--) {
        int rc = iter.next();
        BSLS_ASSERT_SAFE(1 == rc);
        static_cast<void>(rc);
    }

    bmqp::StorageEventBuilder seb(mqbs::FileStoreProtocol::k_VERSION,
                                  bmqp::EventType::e_STORAGE,
                                  &d_clusterData_p->blobSpPool(),
                                  d_allocator_p);

    while (1 == iter.next()) {
        bmqt::EventBuilderResult::Enum buildRc = seb.packMessageRaw(
            *blob,
            iter.headerPosition(),
            iter.header().messageWords() * bmqp::Protocol::k_WORD_SIZE);

        if (bmqt::EventBuilderResult::e_SUCCESS != buildRc) {
            BMQTSK_ALARMLOG_ALARM("RECOVERY")
                << d_clusterData_p->identity().description()
                << ": For Partition [" << partitionId << "], "
                << "failed to build first buffered storage event after "
                << "encountering sync point: " << syncPoint
                << " from node: " << source->nodeDescription()
                << ", rc: " << buildRc << ". Ignoring sync point."
                << BMQTSK_ALARMLOG_END;
            return;  // RETURN
        }
    }

    // If sync point was the last (or only) message in the event, there is
    // nothing to buffer in this event.

    if (0 < seb.messageCount()) {
        bsl::shared_ptr<bdlbb::Blob> blobSp;
        blobSp.createInplace(d_allocator_p,
                             &d_clusterData_p->bufferFactory(),
                             d_allocator_p);
        *blobSp = seb.blob();
        recoveryCtx.addStorageEvent(blobSp);
    }

    recoveryCtx.setNewSyncPoint(syncPoint);
    recoveryCtx.setNewSyncPointOffset(primaryJournalOffset);
    recoveryCtx.setRecoveryPeer(source);

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << ": For Partition [" << partitionId
                  << "], received sync point " << syncPoint << " from node "
                  << source->nodeDescription()
                  << ". Sending storage sync request to this node.";

    sendStorageSyncRequesterHelper(&recoveryCtx, partitionId);
}

void RecoveryManager::processRecoveryEvent(
    int                                 partitionId,
    const bsl::shared_ptr<bdlbb::Blob>& blob,
    mqbnet::ClusterNode*                source)
{
    // executed by the *STORAGE (QUEUE) DISPATCHER* thread

    if (!isRecoveryInProgress(partitionId)) {
        // Partition may not be in recovery.  This could be due to the fact
        // that recovery was stopped/cancelled (because of timeout etc).
        return;  // RETURN
    }

    RecoveryContext& recoveryCtx = d_recoveryContexts[partitionId];

    if (source != recoveryCtx.recoveryPeer()) {
        BMQTSK_ALARMLOG_ALARM("RECOVERY")
            << d_clusterData_p->identity().description() << ": For Partition ["
            << partitionId << "], received recovery event from node "
            << source->nodeDescription()
            << ", which is not identified as recovery peer node "
            << (recoveryCtx.recoveryPeer()
                    ? recoveryCtx.recoveryPeer()->nodeDescription()
                    : "** null **")
            << ". Ignoring this event." << BMQTSK_ALARMLOG_END;
        return;  // RETURN
    }

    bmqp::RecoveryMessageIterator iter;
    bmqp::Event                   rawEvent(blob.get(), d_allocator_p);

    rawEvent.loadRecoveryMessageIterator(&iter);
    BSLS_ASSERT_SAFE(iter.isValid());

    while (1 == iter.next()) {
        const bmqp::RecoveryHeader& header = iter.header();

        bool isData    = false;
        bool isJournal = false;
        bool isQlist   = false;

        switch (header.fileChunkType()) {
        case bmqp::RecoveryFileChunkType::e_DATA:
            isData = true;
            break;  // BREAK

        case bmqp::RecoveryFileChunkType::e_QLIST:
            isQlist = true;
            break;  // BREAK

        case bmqp::RecoveryFileChunkType::e_JOURNAL:
            isJournal = true;
            break;  // BREAK

        case bmqp::RecoveryFileChunkType::e_UNDEFINED:
            BSLS_ASSERT_SAFE(false && "Unreachable by design.");
        }

        BSLS_ASSERT_SAFE(isData || isJournal || isQlist);
        (void)isJournal;  // Compiler happiness

        if (header.fileChunkType() != recoveryCtx.expectedChunkFileType()) {
            BMQTSK_ALARMLOG_ALARM("RECOVERY")
                << d_clusterData_p->identity().description()
                << ": For Partition [" << partitionId
                << "], received incorrect file chunk type: "
                << header.fileChunkType()
                << ", expected: " << recoveryCtx.expectedChunkFileType()
                << ", from: " << source->nodeDescription()
                << ". Stopping recovery." << BMQTSK_ALARMLOG_END;

            onPartitionRecoveryStatus(partitionId, -1 /* status */);

            // TBD: reschedule recovery.  Note that peer will continue to send
            // recovery chunks because we don't notify the peer to cancel
            // recovery request.  The chunks sent by it will be rejected with
            // one of the two 'if' checks at the beginning of this routine.

            return;  // RETURN
        }

        unsigned int expectedSeqNum = recoveryCtx.lastChunkSequenceNumber() +
                                      1;

        if (header.chunkSequenceNumber() != expectedSeqNum) {
            BMQTSK_ALARMLOG_ALARM("RECOVERY")
                << d_clusterData_p->identity().description()
                << ": For Partition [" << partitionId
                << "], received incorrect chunk sequence "
                << "number: " << header.chunkSequenceNumber()
                << ", expected: " << expectedSeqNum
                << ", from: " << source->nodeDescription()
                << ". Stopping recovery." << BMQTSK_ALARMLOG_END;

            onPartitionRecoveryStatus(partitionId, -1 /* status */);

            // TBD: reschedule recovery.  Note that peer will continue to send
            // recovery chunks because we don't notify the peer to cancel
            // recovery request.  The chunks sent by it will be rejected with
            // one of the two 'if' checks at the beginning of this routine.

            return;  // RETURN
        }

        bmqu::BlobPosition chunkPosition;
        int                rc = iter.loadChunkPosition(&chunkPosition);

        if (0 != rc) {
            BMQTSK_ALARMLOG_ALARM("RECOVERY")
                << d_clusterData_p->identity().description()
                << ": For Partition [" << partitionId << "],"
                << " failed to load chunk position, rc: " << rc
                << ". Chunk type: " << header.fileChunkType()
                << ", chunk sequence number: " << header.chunkSequenceNumber()
                << ", from: " << source->nodeDescription()
                << ". Stopping recovery." << BMQTSK_ALARMLOG_END;

            onPartitionRecoveryStatus(partitionId, -1 /* status */);

            // TBD: reschedule recovery.  Note that peer will continue to send
            // recovery chunks because we don't notify the peer to cancel
            // recovery request.  The chunks sent by it will be rejected with
            // one of the two 'if' checks at the beginning of this routine.

            return;  // RETURN
        }

        unsigned int chunkSize = (header.messageWords() -
                                  header.headerWords()) *
                                 bmqp::Protocol::k_WORD_SIZE;

        // Perform md5 digest check only if chunk is of non-zero size.  Note
        // that if chunkSize is zero,
        // 'RecoveryMessageIterator::loadChunkPosition' will still return
        // success, but 'chunkPosition' will point to an invalid position (the
        // buffer index will be invalid).

        if (0 != chunkSize) {
            bdlde::Md5::Md5Digest md5Digest;
            rc = mqbs::FileStoreProtocolUtil::calculateMd5Digest(&md5Digest,
                                                                 *blob,
                                                                 chunkPosition,
                                                                 chunkSize);
            if (0 != rc) {
                BMQTSK_ALARMLOG_ALARM("RECOVERY")
                    << d_clusterData_p->identity().description()
                    << ": For Partition [" << partitionId << "], "
                    << "failed to calculate MD5 digest, " << "rc: " << rc
                    << ". Chunk type: " << header.fileChunkType()
                    << ", chunk sequence number: "
                    << header.chunkSequenceNumber()
                    << ", from: " << source->nodeDescription()
                    << ". Stopping recovery." << BMQTSK_ALARMLOG_END;

                onPartitionRecoveryStatus(partitionId, -1 /* status */);

                // TBD: reschedule recovery.  Note that peer will continue to
                // send recovery chunks because we don't notify the peer to
                // cancel recovery request.  The chunks sent by it will be
                // rejected with one of the two 'if' checks at the beginning of
                // this routine.

                return;  // RETURN
            }

            if (0 != bsl::memcmp(md5Digest.buffer(),
                                 header.md5Digest(),
                                 bmqp::RecoveryHeader::k_MD5_DIGEST_LEN)) {
                bmqu::MemOutStream out;
                out << d_clusterData_p->identity().description()
                    << ": For Partition [" << partitionId
                    << "], chunk MD5 digest mismatch. Calculated: ";

                bdlb::Print::singleLineHexDump(
                    out,
                    md5Digest.buffer(),
                    bmqp::RecoveryHeader::k_MD5_DIGEST_LEN);

                out << ", specified in header: ";

                bdlb::Print::singleLineHexDump(
                    out,
                    header.md5Digest(),
                    bmqp::RecoveryHeader::k_MD5_DIGEST_LEN);

                out << ". Chunk type: " << header.fileChunkType()
                    << ", chunk sequence number: "
                    << header.chunkSequenceNumber()
                    << ", from: " << source->nodeDescription()
                    << ". Stopping recovery.";

                BMQTSK_ALARMLOG_ALARM("RECOVERY")
                    << out.str() << BMQTSK_ALARMLOG_END;

                onPartitionRecoveryStatus(partitionId, -1 /* status */);

                // TBD: reschedule recovery.  Note that peer will continue to
                // send recovery chunks because we don't notify the peer to
                // cancel recovery request.  The chunks sent by it will be
                // rejected with one of the two 'if' checks at the beginning of
                // this routine.

                return;  // RETURN
            }
        }

        mqbs::MappedFileDescriptor* mfd    = 0;
        bsls::Types::Uint64         offset = 0;

        if (isData) {
            mfd    = &recoveryCtx.dataFd();
            offset = recoveryCtx.dataFileOffset();
        }
        else if (isQlist) {
            mfd    = &recoveryCtx.qlistFd();
            offset = recoveryCtx.qlistFileOffset();
        }
        else {
            BSLS_ASSERT_SAFE(isJournal);
            mfd    = &recoveryCtx.journalFd();
            offset = recoveryCtx.journalFileOffset();
        }

        // 'offset' can be zero if we are writing from the beginning of the
        // file.

        BSLS_ASSERT_SAFE(mfd);

        if (0 != chunkSize) {
            bmqu::BlobUtil::copyToRawBufferFromIndex(mfd->block().base() +
                                                         offset,
                                                     *blob,
                                                     chunkPosition.buffer(),
                                                     chunkPosition.byte(),
                                                     chunkSize);
            // Update offset.

            if (isData) {
                recoveryCtx.setDataFileOffset(offset + chunkSize);
            }
            else if (isQlist) {
                recoveryCtx.setQlistFileOffset(offset + chunkSize);
            }
            else {
                recoveryCtx.setJournalFileOffset(offset + chunkSize);
            }
        }

        if (!header.isFinalChunk()) {
            // There are more chunks to come for this file.

            recoveryCtx.setLastChunkSequenceNumber(
                header.chunkSequenceNumber());
            continue;  // CONTINUE
        }

        // Last chunk for the current file.  Update sequence number and the
        // expected file.

        recoveryCtx.setLastChunkSequenceNumber(0);
        if (isData) {
            recoveryCtx.setExpectedChunkFileType(
                bmqp::RecoveryFileChunkType::e_QLIST);
        }
        else if (isQlist) {
            recoveryCtx.setExpectedChunkFileType(
                bmqp::RecoveryFileChunkType::e_JOURNAL);
        }
        else {
            // Journal file is the last one, and this is its last chunk.  This
            // implies that recovery complete.

            onPartitionRecoveryStatus(partitionId, 0 /* status */);
        }
    }
}

void RecoveryManager::processShutdownEvent(int partitionId)
{
    // executed by the *STORAGE (QUEUE) DISPATCHER* thread

    // This method is invoked by StorageMgr for each partition when self node
    // is shutting down.  If recovery is in progress, it should be cancelled
    // and no attempt to initiate recovery should be made in future.  Any
    // timers should also be cancelled here.

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << " Partition [" << partitionId
                  << "]: received shutdown event.";

    RecoveryContext& recoveryCtx = d_recoveryContexts[partitionId];
    d_clusterData_p->scheduler().cancelEventAndWait(
        &recoveryCtx.recoveryStartupWaitHandle());
    if (isRecoveryInProgress(partitionId)) {
        // Recovery is in progress.  Cancel it.
        onPartitionRecoveryStatus(partitionId, -1 /* status */);
    }

    PrimarySyncContext& primarySyncCtx = d_primarySyncContexts[partitionId];
    d_clusterData_p->scheduler().cancelEventAndWait(
        &primarySyncCtx.primarySyncStatusEventHandle());
}

void RecoveryManager::processStorageSyncRequest(
    const bmqp_ctrlmsg::ControlMessage& message,
    mqbnet::ClusterNode*                source,
    const mqbs::FileStore*              fs)
{
    // executed by the *STORAGE (QUEUE) DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(fs);
    BSLS_ASSERT_SAFE(fs->isOpen());

    const bmqp_ctrlmsg::StorageSyncRequest& req =
        message.choice().clusterMessage().choice().storageSyncRequest();

    bmqp_ctrlmsg::ControlMessage controlMsg;
    controlMsg.rId() = message.rId();

    RequestContext* requestContext = 0;

    {  // Lock scope
        bsls::SpinLockGuard guard(&d_recoveryRequestContextLock);  // LOCK

        for (RequestContextIter it = d_recoveryRequestContexts.begin();
             it != d_recoveryRequestContexts.end();
             ++it) {
            const RequestContext& ctx = *it;

            if (ctx.requesterNode() == source &&
                ctx.partitionId() == req.partitionId()) {
                BSLS_ASSERT_SAFE(RequestContextType::e_RECOVERY ==
                                 ctx.contextType());

                // Apparently 'source' has already requested a storage sync for
                // this partitionId, and this node is in the middle of sending
                // response to that request.  Reject this one.

                BALL_LOG_WARN << d_clusterData_p->identity().description()
                              << ": Received duplicate storage sync request "
                              << "from: " << source->nodeDescription()
                              << " for Partition [" << req.partitionId()
                              << "]. Ignoring this request.";

                bmqp_ctrlmsg::Status& status =
                    controlMsg.choice().makeStatus();
                status.category() =
                    bmqp_ctrlmsg::StatusCategory::E_INVALID_ARGUMENT;
                status.code()    = -1;
                status.message() = "Duplicate storage sync request.";

                d_clusterData_p->messageTransmitter().sendMessageSafe(
                    controlMsg,
                    source);
                return;  // RETURN
            }
        }

        // Not a duplicate request.  Add request context.  Keep track of the
        // iterator to the context which will be used to purge the entry from
        // the list in case of an intermediate error in this routine.

        d_recoveryRequestContexts.push_back(RequestContext());
        requestContext = &d_recoveryRequestContexts.back();

        requestContext->setContextType(RequestContextType::e_RECOVERY);
        requestContext->setRequesterNode(source);
        requestContext->setPartitionId(req.partitionId());
        requestContext->setRecoveryManager(this);
    }

    BSLS_ASSERT_SAFE(requestContext);

    RequestContext&   requestCtx = *requestContext;
    FileTransferInfo& fti        = requestCtx.fileTransferInfo();

    // Create a proctor to auto-remove 'requestCtx' from the list of request
    // contexts in case of failure.

    bsl::shared_ptr<char> contextProctor(
        reinterpret_cast<char*>(this),  // dummy
        ChunkDeleter(requestContext));
    fti.incrementAliasedChunksCount();

    // Note about variable names:
    //   asp/A: first sync point of the requester
    //   bsp/B: last sync point of the requester
    //   xsp/X: first sync point of this node
    //   ysp/Y: last sync point of this node

    // Validate A and B.

    bmqp_ctrlmsg::SyncPointOffsetPair asp, bsp;

    if (!req.beginSyncPointOffsetPair().isNull()) {
        const bmqp_ctrlmsg::SyncPointOffsetPair& beginSpOffsetPair =
            req.beginSyncPointOffsetPair().value();

        if (!mqbc::ClusterUtil::isValid(beginSpOffsetPair)) {
            BALL_LOG_WARN << d_clusterData_p->identity().description()
                          << ": Partition [" << req.partitionId()
                          << "] received invalid starting sync point (A) from "
                          << source->nodeDescription()
                          << " in its storage sync request: " << req
                          << ". Sending error.";
            bmqp_ctrlmsg::Status& status = controlMsg.choice().makeStatus();
            status.category() =
                bmqp_ctrlmsg::StatusCategory::E_INVALID_ARGUMENT;
            status.code()    = -1;
            status.message() = "Invalid starting sync point.";

            d_clusterData_p->messageTransmitter().sendMessageSafe(controlMsg,
                                                                  source);
            return;  // RETURN
        }

        asp = beginSpOffsetPair;
    }

    if (!req.endSyncPointOffsetPair().isNull()) {
        const bmqp_ctrlmsg::SyncPointOffsetPair& endSpOffsetPair =
            req.endSyncPointOffsetPair().value();

        if (!mqbc::ClusterUtil::isValid(endSpOffsetPair)) {
            BALL_LOG_WARN << d_clusterData_p->identity().description()
                          << ": Received invalid ending sync point (B) from "
                          << source->nodeDescription()
                          << " in its storage sync request: " << req
                          << ". Sending error.";
            bmqp_ctrlmsg::Status& status = controlMsg.choice().makeStatus();
            status.category() =
                bmqp_ctrlmsg::StatusCategory::E_INVALID_ARGUMENT;
            status.code()    = -1;
            status.message() = "Invalid end sync point.";

            d_clusterData_p->messageTransmitter().sendMessageSafe(controlMsg,
                                                                  source);
            return;  // RETURN
        }

        bsp = endSpOffsetPair;
    }

    bmqp_ctrlmsg::StorageSyncResponse response;

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << ": Received storage sync request: " << req
                  << " from node: " << source->nodeDescription();

    const SyncPointOffsetPairs& spOffsetPairs = fs->syncPoints();

    if (spOffsetPairs.empty()) {
        // Partition is empty.  Notify 'source' to create an empty file.

        response.partitionId() = req.partitionId();
        response.storageSyncResponseType() =
            bmqp_ctrlmsg::StorageSyncResponseType::E_EMPTY;
        response.beginSyncPoint() = bmqp_ctrlmsg::SyncPoint();
        response.endSyncPoint()   = bmqp_ctrlmsg::SyncPoint();

        controlMsg.choice()
            .makeClusterMessage()
            .choice()
            .makeStorageSyncResponse(response);

        d_clusterData_p->messageTransmitter().sendMessageSafe(controlMsg,
                                                              source);

        // This is success, but we don't release the 'contextProctor' because
        // its ok to remove the context now since request has been served.

        return;  // RETURN
    }

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << " Partition [" << req.partitionId()
                  << "]: Number of sync points in the "
                  << "current journal: " << spOffsetPairs.size();

    const bmqp_ctrlmsg::SyncPointOffsetPair& xsp = spOffsetPairs.front();
    const bmqp_ctrlmsg::SyncPointOffsetPair& ysp = spOffsetPairs.back();

    BSLS_ASSERT_SAFE(mqbc::ClusterUtil::isValid(xsp));
    BSLS_ASSERT_SAFE(mqbc::ClusterUtil::isValid(ysp));

    // Now that 'Y' has a valid value, check if 'B' is null, and if so, assign
    // it the value of 'Y'.  This can be thought of logically in this way: if
    // the 'source' didn't specify an end in its range, this node will send
    // everything upto to what it knows, which is 'Y'.  This check and
    // assignment must be done only after we have ensured above that 'Y' is not
    // null.

    if (req.endSyncPointOffsetPair().isNull()) {
        bsp = ysp;
    }

    BSLS_ASSERT_SAFE(mqbc::ClusterUtil::isValid(bsp));

    if (bsp < asp) {
        BALL_LOG_WARN << d_clusterData_p->identity().description()
                      << ": End sync point (B): " << bsp
                      << " is smaller than begin sync point (A): " << asp
                      << " in storage sync request: " << req
                      << ", from: " << source->nodeDescription()
                      << ". Sending error.";
        bmqp_ctrlmsg::Status& status = controlMsg.choice().makeStatus();
        status.category() = bmqp_ctrlmsg::StatusCategory::E_INVALID_ARGUMENT;
        status.code()     = -1;
        status.message()  = "Invalid sync point range.";

        d_clusterData_p->messageTransmitter().sendMessageSafe(controlMsg,
                                                              source);
        return;  // RETURN
    }

    if (bsp == asp) {
        // Requester provided an empty range, or it's in sync.

        BSLS_ASSERT_SAFE(mqbc::ClusterUtil::isValid(asp.syncPoint()));

        response.partitionId() = req.partitionId();
        response.storageSyncResponseType() =
            bmqp_ctrlmsg::StorageSyncResponseType::E_IN_SYNC;
        response.beginSyncPoint() = asp.syncPoint();
        response.endSyncPoint()   = bsp.syncPoint();

        controlMsg.choice()
            .makeClusterMessage()
            .choice()
            .makeStorageSyncResponse(response);

        d_clusterData_p->messageTransmitter().sendMessageSafe(controlMsg,
                                                              source);

        // This is success, but we don't release the 'contextProctor' because
        // its ok to remove the context now since request has been served.

        return;  // RETURN
    }

    if (ysp < bsp) {
        // Error, peer is behind.

        BMQTSK_ALARMLOG_ALARM("RECOVERY")
            << d_clusterData_p->identity().description()
            << ": Received newer sync point from " << source->nodeDescription()
            << " in its storage sync request. Self's newest sync point: "
            << ysp
            << ". This implies this node is behind despite being AVAILABLE."
            << BMQTSK_ALARMLOG_END;

        bmqp_ctrlmsg::Status& status = controlMsg.choice().makeStatus();
        status.category()            = bmqp_ctrlmsg::StatusCategory::E_UNKNOWN;
        status.code()                = -1;
        status.message()             = "Peer is behind.";

        d_clusterData_p->messageTransmitter().sendMessageSafe(controlMsg,
                                                              source);
        return;  // RETURN
    }

    // Either a patch or entire file needs to be sent.  Get the names of active
    // data, qlist and journal files from the file store and map them.

    mqbs::FileStoreSet fileSet;
    bmqu::MemOutStream errorDesc;
    fs->loadCurrentFiles(&fileSet);

    int rc = mqbs::FileStoreUtil::openFileSetReadMode(errorDesc,
                                                      fileSet,
                                                      &fti.journalFd(),
                                                      &fti.dataFd(),
                                                      &fti.qlistFd());

    if (0 != rc) {
        BMQTSK_ALARMLOG_ALARM("FILE_IO")
            << d_clusterData_p->identity().description()
            << ": Failed to open one of JOURNAL/QLIST/DATA file, rc: " << rc
            << ", reason [" << errorDesc.str()
            << "] while serving storage sync request from: "
            << source->nodeDescription() << BMQTSK_ALARMLOG_END;

        bmqp_ctrlmsg::Status& status = controlMsg.choice().makeStatus();
        status.category()            = bmqp_ctrlmsg::StatusCategory::E_UNKNOWN;
        status.code()                = -1;
        status.message()             = "Failed to open partition.";

        d_clusterData_p->messageTransmitter().sendMessageSafe(controlMsg,
                                                              source);
        return;  // RETURN
    }

    BSLS_ASSERT_SAFE(fti.journalFd().isValid());
    BSLS_ASSERT_SAFE(fti.qlistFd().isValid());
    BSLS_ASSERT_SAFE(fti.dataFd().isValid());

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << " Partition [" << req.partitionId()
                  << "]: successfully opened the partition.";

    bsls::Types::Uint64 journalFileBeginOffset = 0;
    bsls::Types::Uint64 journalFileEndOffset   = 0;
    bsls::Types::Uint64 dataFileBeginOffset    = 0;
    bsls::Types::Uint64 dataFileEndOffset      = 0;
    bsls::Types::Uint64 qlistFileBeginOffset   = 0;
    bsls::Types::Uint64 qlistFileEndOffset     = 0;

    bmqp_ctrlmsg::SyncPointOffsetPair            beginSpOffset;
    bmqp_ctrlmsg::SyncPointOffsetPair            endSpOffset;
    bmqp_ctrlmsg::StorageSyncResponseType::Value responseType;

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << " Partition [" << req.partitionId()
                  << "]: Sync point summary:\n"
                  << "\t'X': " << xsp << '\n'
                  << "\t'Y': " << ysp << '\n'
                  << "\t'A': " << asp << '\n'
                  << "\t'B': " << bsp;

    if (xsp.syncPoint() <= asp.syncPoint()) {
        // X <= A => journal hasn't rolled over.  Send patch in (A, B) range.

        BSLS_ASSERT_SAFE(mqbc::ClusterUtil::isValid(asp.syncPoint()));
        // coz X <= A
        BSLS_ASSERT_SAFE(bsp.syncPoint() > asp.syncPoint());

        // Find the 'A' and 'B' entries in the (sorted) list of sync points
        // maintained by this node, so that we can retrieve the journal offsets
        // corresponding to 'A' and 'B'.

        BALL_LOG_INFO << d_clusterData_p->identity().description()
                      << " Partition [" << req.partitionId()
                      << "]: starting search for sync point: " << asp
                      << " in a list of " << spOffsetPairs.size()
                      << " sync points.";

        bsl::pair<SyncPointOffsetConstIter, SyncPointOffsetConstIter> retA =
            bsl::equal_range(spOffsetPairs.begin(),
                             spOffsetPairs.end(),
                             asp,
                             SyncPointOffsetPairComparator());
        if (retA.first == retA.second) {
            BALL_LOG_ERROR << d_clusterData_p->identity().description()
                           << ": Begin sync point (A) not found: " << asp
                           << ", while serving storage sync request: " << req
                           << ", from: " << source->nodeDescription()
                           << ". Sending error.";

            bmqp_ctrlmsg::Status& status = controlMsg.choice().makeStatus();
            status.category() = bmqp_ctrlmsg::StatusCategory::E_UNKNOWN;
            status.code()     = -1;
            status.message()  = "Begin sync point (A) not found.";

            d_clusterData_p->messageTransmitter().sendMessageSafe(controlMsg,
                                                                  source);
            return;  // RETURN
        }

        BALL_LOG_INFO << d_clusterData_p->identity().description()
                      << " Partition [" << req.partitionId()
                      << "]: Begin SyncPt (A) found at journal offset: "
                      << retA.first->offset();
        // Note that above, the retrieved offset of SyncPt in the journal
        // will be same as 'asp.offset()'.

        // We skip the 'A' sync point in the response, because requester node
        // already has it.

        journalFileBeginOffset =
            retA.first->offset() +
            mqbs::FileStoreProtocol::k_JOURNAL_RECORD_SIZE;

        BALL_LOG_INFO << d_clusterData_p->identity().description()
                      << " Partition [" << req.partitionId()
                      << "]: starting search for sync point: " << bsp
                      << " in a list of " << spOffsetPairs.size()
                      << " sync points.";

        bsl::pair<SyncPointOffsetConstIter, SyncPointOffsetConstIter> retB =
            bsl::equal_range(spOffsetPairs.begin(),
                             spOffsetPairs.end(),
                             bsp,
                             SyncPointOffsetPairComparator());
        if (retB.first == retB.second) {
            BMQTSK_ALARMLOG_ALARM("RECOVERY")
                << d_clusterData_p->identity().description()
                << ": End sync point (B) not found: " << bsp
                << ", while serving storage sync request: " << req
                << ", from: " << source->nodeDescription()
                << ". Sending error." << BMQTSK_ALARMLOG_END;

            bmqp_ctrlmsg::Status& status = controlMsg.choice().makeStatus();
            status.category() = bmqp_ctrlmsg::StatusCategory::E_UNKNOWN;
            status.code()     = -1;
            status.message()  = "End sync point (B) not found.";

            d_clusterData_p->messageTransmitter().sendMessageSafe(controlMsg,
                                                                  source);
            return;  // RETURN
        }

        BALL_LOG_INFO << d_clusterData_p->identity().description()
                      << " Partition [" << req.partitionId()
                      << "]: End SyncPt (B) found at journal offset: "
                      << retB.first->offset();
        // Note that above, the retrieved offset of SyncPt in the journal
        // will be same as 'bsp.offset()'.

        // Include 'B' sync point as well because requester expects it (even
        // though it may have it).

        journalFileEndOffset = retB.first->offset() +
                               mqbs::FileStoreProtocol::k_JOURNAL_RECORD_SIZE;

        dataFileBeginOffset =
            static_cast<bsls::Types::Uint64>(
                retA.first->syncPoint().dataFileOffsetDwords()) *
            bmqp::Protocol::k_DWORD_SIZE;

        dataFileEndOffset =
            static_cast<bsls::Types::Uint64>(
                retB.first->syncPoint().dataFileOffsetDwords()) *
            bmqp::Protocol::k_DWORD_SIZE;

        qlistFileBeginOffset =
            static_cast<bsls::Types::Uint64>(
                retA.first->syncPoint().qlistFileOffsetWords()) *
            bmqp::Protocol::k_WORD_SIZE;

        qlistFileEndOffset =
            static_cast<bsls::Types::Uint64>(
                retB.first->syncPoint().qlistFileOffsetWords()) *
            bmqp::Protocol::k_WORD_SIZE;

        // The begin and end offsets for DATA/QLIST files patch could be equal.
        // This could occur if there are no message records in the journal b/w
        // 'asp' and 'bsp', and no queues were opened b/w 'asp' and 'bsp'.

        BSLS_ASSERT_SAFE(journalFileEndOffset > journalFileBeginOffset);
        BSLS_ASSERT_SAFE(dataFileEndOffset >= dataFileBeginOffset);
        BSLS_ASSERT_SAFE(qlistFileEndOffset >= qlistFileBeginOffset);

        responseType  = bmqp_ctrlmsg::StorageSyncResponseType::E_PATCH;
        beginSpOffset = asp;
        endSpOffset   = bsp;
    }
    else {
        // X > A => journal has rolled over, or requester has no storage.  Send
        // file from begining to max(X, B).

        journalFileBeginOffset = 0;
        dataFileBeginOffset    = 0;
        qlistFileBeginOffset   = 0;

        beginSpOffset = bmqp_ctrlmsg::SyncPointOffsetPair();  // beginning
        if (xsp.syncPoint() > bsp.syncPoint()) {
            endSpOffset = xsp;
        }
        else {
            endSpOffset = bsp;
        }

        BALL_LOG_INFO << d_clusterData_p->identity().description()
                      << " Partition [" << req.partitionId()
                      << "]: starting search for sync point: " << endSpOffset
                      << " in a list of " << spOffsetPairs.size()
                      << " sync points.";

        bsl::pair<SyncPointOffsetConstIter, SyncPointOffsetConstIter> rcPair =
            bsl::equal_range(spOffsetPairs.begin(),
                             spOffsetPairs.end(),
                             endSpOffset,
                             SyncPointOffsetPairComparator());

        BALL_LOG_INFO << d_clusterData_p->identity().description()
                      << " Partition [" << req.partitionId()
                      << "]: done with search.";

        if (rcPair.first == rcPair.second) {
            BMQTSK_ALARMLOG_ALARM("RECOVERY")
                << d_clusterData_p->identity().description()
                << ": End sync point (max(X, B)) not found: " << endSpOffset
                << ", while serving storage sync request: " << req
                << ", from: " << source->nodeDescription()
                << ". Sending error." << BMQTSK_ALARMLOG_END;

            bmqp_ctrlmsg::Status& status = controlMsg.choice().makeStatus();
            status.category() = bmqp_ctrlmsg::StatusCategory::E_UNKNOWN;
            status.code()     = -1;
            status.message()  = "End sync point (max(X, B)) not found.";

            d_clusterData_p->messageTransmitter().sendMessageSafe(controlMsg,
                                                                  source);
            return;  // RETURN
        }

        BALL_LOG_INFO << d_clusterData_p->identity().description()
                      << " Partition [" << req.partitionId()
                      << "]: End SyncPt (max(X, B)) found at journal offset: "
                      << rcPair.first->offset();
        // Note that the journal offset of the retrieved SyncPt printed
        // will be same as 'endSpOffset.offset()'.

        journalFileEndOffset = rcPair.first->offset() +
                               mqbs::FileStoreProtocol::k_JOURNAL_RECORD_SIZE;

        dataFileEndOffset =
            static_cast<bsls::Types::Uint64>(
                rcPair.first->syncPoint().dataFileOffsetDwords()) *
            bmqp::Protocol::k_DWORD_SIZE;

        qlistFileEndOffset =
            static_cast<bsls::Types::Uint64>(
                rcPair.first->syncPoint().qlistFileOffsetWords()) *
            bmqp::Protocol::k_WORD_SIZE;

        BSLS_ASSERT_SAFE(journalFileEndOffset > journalFileBeginOffset);
        BSLS_ASSERT_SAFE(dataFileEndOffset >= dataFileBeginOffset);
        BSLS_ASSERT_SAFE(qlistFileEndOffset >= qlistFileBeginOffset);

        responseType = bmqp_ctrlmsg::StorageSyncResponseType::E_FILE;
    }

    // Everything that needs to be sent has been validated by this point.
    // Reply to the 'source' node with what's coming its way.

    response.partitionId()             = req.partitionId();
    response.storageSyncResponseType() = responseType;
    response.beginSyncPoint()          = beginSpOffset.syncPoint();
    response.endSyncPoint()            = endSpOffset.syncPoint();
    controlMsg.choice().makeClusterMessage().choice().makeStorageSyncResponse(
        response);

    d_clusterData_p->messageTransmitter().sendMessageSafe(controlMsg, source);

    // Send data file patch first, in chunks.
    const bsls::Types::Int64 dataFileSize = dataFileEndOffset -
                                            dataFileBeginOffset;
    BSLS_ASSERT_SAFE(dataFileSize >= 0);
    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << " Partition [" << req.partitionId()
                  << "]: sending DATA patch/file of size: "
                  << bmqu::PrintUtil::prettyNumber(dataFileSize) << " bytes.";

    const int fileChunkSize =
        d_clusterConfig.partitionConfig().syncConfig().fileChunkSize();
    rc = sendFile(&requestCtx,
                  dataFileBeginOffset,
                  dataFileEndOffset,
                  fileChunkSize,
                  bmqp::RecoveryFileChunkType::e_DATA);
    if (0 != rc) {
        BMQTSK_ALARMLOG_ALARM("RECOVERY")
            << d_clusterData_p->identity().description()
            << ": Failed to send DATA file/patch to "
            << source->nodeDescription()
            << ", while serving storage sync request: " << req
            << ", from: " << source->nodeDescription() << ". [rc: " << rc
            << "]. " << "Sending error." << BMQTSK_ALARMLOG_END;
        return;  // RETURN
    }

    // Next, send qlist file, in chunks.
    const bsls::Types::Int64 qlistFileSize = qlistFileEndOffset -
                                             qlistFileBeginOffset;
    BSLS_ASSERT_SAFE(qlistFileSize >= 0);
    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << " Partition [" << req.partitionId()
                  << "]: sending QLIST file/patch of size: "
                  << bmqu::PrintUtil::prettyNumber(qlistFileSize) << " bytes.";

    rc = sendFile(&requestCtx,
                  qlistFileBeginOffset,
                  qlistFileEndOffset,
                  fileChunkSize,
                  bmqp::RecoveryFileChunkType::e_QLIST);
    if (0 != rc) {
        BMQTSK_ALARMLOG_ALARM("RECOVERY")
            << d_clusterData_p->identity().description()
            << ": Failed to send QLIST file/patch to "
            << source->nodeDescription()
            << ", while serving storage sync request: " << req
            << ", from: " << source->nodeDescription() << ". [rc: " << rc
            << "]. " << "Sending error." << BMQTSK_ALARMLOG_END;
        return;  // RETURN
    }

    // Send journal file once data file patch has been sent, in chunks.
    const bsls::Types::Int64 journalFileSize = journalFileEndOffset -
                                               journalFileBeginOffset;
    BSLS_ASSERT_SAFE(journalFileSize >= 0);
    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << " Partition [" << req.partitionId()
                  << "]: sending JOURNAL file/patch of size: "
                  << bmqu::PrintUtil::prettyNumber(journalFileSize)
                  << " bytes.";

    rc = sendFile(&requestCtx,
                  journalFileBeginOffset,
                  journalFileEndOffset,
                  fileChunkSize,
                  bmqp::RecoveryFileChunkType::e_JOURNAL);
    if (0 != rc) {
        BMQTSK_ALARMLOG_ALARM("RECOVERY")
            << d_clusterData_p->identity().description()
            << ": Failed to send JOURNAL file/patch to "
            << source->nodeDescription()
            << ", while serving storage sync request: " << req
            << ", from: " << source->nodeDescription() << ". [rc: " << rc
            << "]. " << "Sending error." << BMQTSK_ALARMLOG_END;
        return;  // RETURN
    }
}

void RecoveryManager::startPartitionPrimarySync(
    const mqbs::FileStore*                   fs,
    const bsl::vector<mqbnet::ClusterNode*>& peers,
    const PartitionPrimarySyncCb&            partitionPrimarySyncCb)
{
    // executed by each of the *STORAGE (QUEUE) DISPATCHER* threads

    BSLS_ASSERT_SAFE(fs);
    BSLS_ASSERT_SAFE(fs->isOpen());

    int pid = fs->config().partitionId();

    BSLS_ASSERT_SAFE(0 <= pid);
    BSLS_ASSERT_SAFE(static_cast<size_t>(pid) < d_partitionsInfo.size());
    BSLS_ASSERT_SAFE(static_cast<size_t>(pid) < d_primarySyncContexts.size());
    BSLS_ASSERT_SAFE(partitionPrimarySyncCb);

    PrimarySyncContext& primarySyncCtx = d_primarySyncContexts[pid];

    if (primarySyncCtx.primarySyncInProgress()) {
        // Attempting to start a 2nd primary sync while the previous one is in
        // progress.  Calling onPartitionPrimarySyncStatus() will cancel 1st
        // sync as well.  So we call 'partitionPrimarySyncCb' directly.  TBD:
        // this case needs to be handled in a higher component.

        BMQTSK_ALARMLOG_ALARM("RECOVERY")
            << d_clusterData_p->identity().description() << " Partition ["
            << pid << "]: " << "primary sync is already under progress with "
            << (primarySyncCtx.syncPeer()
                    ? primarySyncCtx.syncPeer()->nodeDescription()
                    : "** null **")
            << BMQTSK_ALARMLOG_END;
        partitionPrimarySyncCb(pid, -1 /* status */);
    }

    BSLS_ASSERT_SAFE(0 == primarySyncCtx.syncPeer());

    // Currently, this routine doesn't return error, so we update the primary
    // sync status of this partition right away.

    primarySyncCtx.setRecoveryManager(this);
    primarySyncCtx.setFileStore(fs);
    primarySyncCtx.setPrimarySyncInProgress(true);
    primarySyncCtx.setPartitionPrimarySyncCb(partitionPrimarySyncCb);

    bmqp_ctrlmsg::PartitionSequenceNumber tmp;
    tmp.primaryLeaseId() = fs->primaryLeaseId();
    tmp.sequenceNumber() = fs->sequenceNumber();
    primarySyncCtx.setSelfPartitionSequenceNum(tmp);

    if (!fs->syncPoints().empty()) {
        primarySyncCtx.setSelfLastSyncPtOffsetPair(fs->syncPoints().back());
    }

    if (d_clusterData_p->cluster().isLocal()) {
        BSLS_ASSERT_SAFE(peers.empty());
        onPartitionPrimarySyncStatus(pid, 0 /* status */);
        return;  // RETURN
    }

    BSLS_ASSERT_SAFE(!peers.empty());

    PeerPartitionStates& ppStates = primarySyncCtx.peerPartitionStates();
    for (size_t idx = 0; idx < peers.size(); ++idx) {
        BSLS_ASSERT_SAFE(peers[idx]);
        ppStates.emplace_back(peers[idx],
                              bmqp_ctrlmsg::PartitionSequenceNumber(),
                              bmqp_ctrlmsg::SyncPointOffsetPair(),
                              false);  // need partition sync
    }

    // Currently, this routine doesn't return error, so we schedule an event
    // right away to fire 2 minute from now to check recovery status.

    bsls::TimeInterval after(bmqsys::Time::nowMonotonicClock());
    after.addMilliseconds(d_clusterConfig.partitionConfig()
                              .syncConfig()
                              .masterSyncMaxDurationMs());
    d_clusterData_p->scheduler().scheduleEvent(
        &primarySyncCtx.primarySyncStatusEventHandle(),
        after,
        bdlf::BindUtil::bind(&RecoveryManager::primarySyncStatusCb,
                             this,
                             pid));

    // Send PartitionSyncStateRequest to all *AVAILABLE* peers.

    MultiRequestManagerType::RequestContextSp contextSp =
        d_clusterData_p->multiRequestManager().createRequestContext();

    bmqp_ctrlmsg::PartitionSyncStateQuery& req =
        contextSp->request()
            .choice()
            .makeClusterMessage()
            .choice()
            .makePartitionSyncStateQuery();
    req.partitionId() = pid;

    contextSp->setDestinationNodes(peers);
    contextSp->setResponseCb(bdlf::BindUtil::bind(
        &RecoveryManager::onPartitionSyncStateQueryResponse,
        this,
        bdlf::PlaceHolders::_1));

    bsls::TimeInterval timeoutMs;
    timeoutMs.setTotalMilliseconds(d_clusterConfig.partitionConfig()
                                       .syncConfig()
                                       .partitionSyncStateReqTimeoutMs());
    d_clusterData_p->multiRequestManager().sendRequest(contextSp, timeoutMs);
}

void RecoveryManager::processPartitionSyncStateRequest(
    const bmqp_ctrlmsg::ControlMessage& message,
    mqbnet::ClusterNode*                source,
    const mqbs::FileStore*              fs)
{
    // executed by the *STORAGE (QUEUE) DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(source);
    BSLS_ASSERT_SAFE(fs);

    const bmqp_ctrlmsg::PartitionSyncStateQuery& req =
        message.choice().clusterMessage().choice().partitionSyncStateQuery();

    BSLS_ASSERT_SAFE(req.partitionId() == fs->config().partitionId());
    BSLS_ASSERT_SAFE(!isRecoveryInProgress(req.partitionId()));

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << " Partition [" << req.partitionId()
                  << "]: received partition-sync state request: " << req
                  << ", from: " << source->nodeDescription();

    bmqp_ctrlmsg::ControlMessage controlMsg;
    controlMsg.rId() = message.rId();

    bmqp_ctrlmsg::ClusterMessage& clusterMsg =
        controlMsg.choice().makeClusterMessage();
    bmqp_ctrlmsg::PartitionSyncStateQueryResponse& response =
        clusterMsg.choice().makePartitionSyncStateQueryResponse();

    response.partitionId()    = req.partitionId();
    response.primaryLeaseId() = fs->primaryLeaseId();
    response.sequenceNum()    = fs->sequenceNumber();
    if (!fs->syncPoints().empty()) {
        response.lastSyncPointOffsetPair() = fs->syncPoints().back();
    }
    // else: unset value of lastSyncPoint() is ok.

    d_clusterData_p->messageTransmitter().sendMessageSafe(controlMsg, source);
}

void RecoveryManager::processPartitionSyncDataRequest(
    const bmqp_ctrlmsg::ControlMessage& message,
    mqbnet::ClusterNode*                source,
    const mqbs::FileStore*              fs)
{
    // executed by the *STORAGE (QUEUE) DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(source);
    BSLS_ASSERT_SAFE(fs);

    const bmqp_ctrlmsg::PartitionSyncDataQuery& req =
        message.choice().clusterMessage().choice().partitionSyncDataQuery();

    BSLS_ASSERT_SAFE(req.partitionId() == fs->config().partitionId());

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << " Partition [" << req.partitionId()
                  << "]: received partition-sync data request: " << req
                  << ", from: " << source->nodeDescription();

    bmqp_ctrlmsg::ControlMessage controlMsg;
    controlMsg.rId() = message.rId();

    RequestContext* primarySyncRequestContext = 0;

    {  // Lock scope
        bsls::SpinLockGuard guard(&d_primarySyncRequestContextLock);  // LOCK

        for (RequestContextIter it = d_primarySyncRequestContexts.begin();
             it != d_primarySyncRequestContexts.end();
             ++it) {
            const RequestContext& ctx = *it;

            if (ctx.requesterNode() == source &&
                ctx.partitionId() == req.partitionId()) {
                BSLS_ASSERT_SAFE(RequestContextType::e_PARTITION_SYNC ==
                                 ctx.contextType());

                // Apparently 'source' has already requested a partition sync
                // for this partitionId, and this node is in the middle of
                // sending response to that request.  Reject this one.

                BALL_LOG_WARN << d_clusterData_p->identity().description()
                              << ": Received duplicate partition sync request "
                              << "from: " << source->nodeDescription()
                              << " for Partition [" << req.partitionId()
                              << "]. Ignoring this request.";

                bmqp_ctrlmsg::Status& status =
                    controlMsg.choice().makeStatus();
                status.category() =
                    bmqp_ctrlmsg::StatusCategory::E_INVALID_ARGUMENT;
                status.code()    = -1;
                status.message() = "Duplicate storage sync request.";

                d_clusterData_p->messageTransmitter().sendMessageSafe(
                    controlMsg,
                    source);
                return;  // RETURN
            }
        }

        // Not a duplicate request.  Add request context.  Keep track of the
        // iterator to the context which will be used to purge the entry from
        // the list in case of an intermediate error in this routine.

        d_primarySyncRequestContexts.push_back(RequestContext());
        primarySyncRequestContext = &d_primarySyncRequestContexts.back();

        RequestContext& ctx = *primarySyncRequestContext;
        ctx.setContextType(RequestContextType::e_PARTITION_SYNC);
        ctx.setRequesterNode(source);
        ctx.setPartitionId(req.partitionId());
        ctx.setRecoveryManager(this);
    }

    BSLS_ASSERT_SAFE(primarySyncRequestContext);

    RequestContext&   requestCtx = *primarySyncRequestContext;
    FileTransferInfo& fti        = requestCtx.fileTransferInfo();

    // Create a proctor to auto-remove 'requestCtx' from the list of request
    // contexts in case of failure.

    bsl::shared_ptr<char> contextProctor(
        reinterpret_cast<char*>(this),  // dummy
        ChunkDeleter(&requestCtx));
    fti.incrementAliasedChunksCount();

    bmqp_ctrlmsg::PartitionSequenceNumber requesterLastSeqNum;
    requesterLastSeqNum.primaryLeaseId() = req.lastPrimaryLeaseId();
    requesterLastSeqNum.sequenceNumber() = req.lastSequenceNum();

    bmqp_ctrlmsg::PartitionSequenceNumber requesterUptoSeqNum;
    requesterUptoSeqNum.primaryLeaseId() = req.uptoPrimaryLeaseId();
    requesterUptoSeqNum.sequenceNumber() = req.uptoSequenceNum();

    bmqp_ctrlmsg::PartitionSequenceNumber selfSeqNum;
    selfSeqNum.primaryLeaseId() = fs->primaryLeaseId();
    selfSeqNum.sequenceNumber() = fs->sequenceNumber();

    if (requesterUptoSeqNum <= requesterLastSeqNum) {
        BALL_LOG_WARN << d_clusterData_p->identity().description()
                      << ": For Partition [" << req.partitionId()
                      << "], received partition sync data request from "
                      << source->nodeDescription() << ", with invalid "
                      << " 'last/upto' sequence numbers: "
                      << requesterLastSeqNum << ", " << requesterUptoSeqNum;

        bmqp_ctrlmsg::Status& status = controlMsg.choice().makeStatus();
        status.category() = bmqp_ctrlmsg::StatusCategory::E_INVALID_ARGUMENT;
        status.code()     = -1;
        status.message()  = "Invalid sequence number range..";

        d_clusterData_p->messageTransmitter().sendMessageSafe(controlMsg,
                                                              source);
        return;  // RETURN
    }

    if (selfSeqNum <= requesterLastSeqNum) {
        // Request should not have been sent to this node.

        BALL_LOG_WARN << d_clusterData_p->identity().description()
                      << ": For Partition [" << req.partitionId()
                      << "], received partition sync data request from "
                      << source->nodeDescription() << ", with equal or greater"
                      << " last sequence number " << requesterLastSeqNum
                      << ", self sequence number " << selfSeqNum;

        bmqp_ctrlmsg::Status& status = controlMsg.choice().makeStatus();
        status.category() = bmqp_ctrlmsg::StatusCategory::E_INVALID_ARGUMENT;
        status.code()     = -1;
        status.message()  = "Peer is behind new primary.";

        d_clusterData_p->messageTransmitter().sendMessageSafe(controlMsg,
                                                              source);
        return;  // RETURN
    }

    if (selfSeqNum < requesterUptoSeqNum) {
        // Request should not have been sent to this node.

        BALL_LOG_WARN << d_clusterData_p->identity().description()
                      << ": For Partition [" << req.partitionId()
                      << "], received partition sync data request from "
                      << source->nodeDescription() << ", with greater 'upto' "
                      << "sequence number " << requesterUptoSeqNum
                      << ", self sequence number " << selfSeqNum;

        bmqp_ctrlmsg::Status& status = controlMsg.choice().makeStatus();
        status.category() = bmqp_ctrlmsg::StatusCategory::E_INVALID_ARGUMENT;
        status.code()     = -1;
        status.message()  = "Peer is behind new primary.";

        d_clusterData_p->messageTransmitter().sendMessageSafe(controlMsg,
                                                              source);
        return;  // RETURN
    }

    // If self's partitionSequenceNumber is greater than 'requesterLastSeqNum',
    // it means self's leaseId must be greater than zero, because zero is the
    // smallest possible value for leaseId.  This also implies that self's
    // partition must have at least one sync point which captures that non-zero
    // leaseId value.

    BSLS_ASSERT_SAFE(0 != selfSeqNum.primaryLeaseId());
    BSLS_ASSERT_SAFE(!fs->syncPoints().empty());

    const bmqp_ctrlmsg::SyncPointOffsetPair& lastSpoPair =
        req.lastSyncPointOffsetPair();

    if (req.lastPrimaryLeaseId() != lastSpoPair.syncPoint().primaryLeaseId() &&
        req.lastSequenceNum() < lastSpoPair.syncPoint().sequenceNum()) {
        BALL_LOG_WARN << d_clusterData_p->identity().description()
                      << ": For Partition [" << req.partitionId()
                      << "], invalid sync-point and primaryLeaseId/seqNum "
                      << "specified in partition sync data request: " << req
                      << ", from " << source->nodeDescription();

        bmqp_ctrlmsg::Status& status = controlMsg.choice().makeStatus();
        status.category() = bmqp_ctrlmsg::StatusCategory::E_INVALID_ARGUMENT;
        status.code()     = -1;
        status.message()  = "Peer is behind new primary.";

        d_clusterData_p->messageTransmitter().sendMessageSafe(controlMsg,
                                                              source);
        return;  // RETURN
    }

    const SyncPointOffsetPairs&              spOffsetPairs = fs->syncPoints();
    const bmqp_ctrlmsg::SyncPointOffsetPair& firstSpOffPair =
        spOffsetPairs.front();
    mqbs::JournalFileIterator jit;

    if (false == (firstSpOffPair.syncPoint() <= lastSpoPair.syncPoint())) {
        // TBD: Need to peek into the archived files to retrieve the correct
        // one.  In practice, there should never be a need to peek beyond the
        // latest archived file, since a newly elected primary is supposed to
        // be almost up-to-date on the partition.  This scenario is currently
        // not handled.

        bmqp_ctrlmsg::Status& status = controlMsg.choice().makeStatus();
        status.category()            = bmqp_ctrlmsg::StatusCategory::E_REFUSED;
        status.code()                = -1;
        status.message() = "Sync from archived files not yet supported.";

        d_clusterData_p->messageTransmitter().sendMessageSafe(controlMsg,
                                                              source);
        return;  // RETURN
    }

    // Replay storage events from the current ("live") files of the partition.

    BSLS_ASSERT_SAFE(mqbc::ClusterUtil::isValid(lastSpoPair));

    // Find req.lastSyncPoint() in the list of sync points maintained by the
    // partition.

    bsl::pair<SyncPointOffsetConstIter, SyncPointOffsetConstIter> rcPair =
        bsl::equal_range(spOffsetPairs.begin(),
                         spOffsetPairs.end(),
                         lastSpoPair,
                         SyncPointOffsetPairComparator());

    if (rcPair.first == rcPair.second) {
        // This could be a bug in replication or 'source' sent a bogus sync
        // point.

        BALL_LOG_WARN << d_clusterData_p->identity().description()
                      << " Partition [" << req.partitionId()
                      << "]: Last sync point not found: " << lastSpoPair
                      << ", while serving partition-sync data request: " << req
                      << ", from: " << source->nodeDescription()
                      << ". Sending error.";

        bmqp_ctrlmsg::Status& status = controlMsg.choice().makeStatus();
        status.category()            = bmqp_ctrlmsg::StatusCategory::E_REFUSED;
        status.code()                = -1;
        status.message()             = "Last sync point not found.";

        d_clusterData_p->messageTransmitter().sendMessageSafe(controlMsg,
                                                              source);
        return;  // RETURN
    }

    bsls::Types::Uint64 journalSpOffset = lastSpoPair.offset();
    mqbs::FileStoreSet  fileSet;
    fs->loadCurrentFiles(&fileSet);

    bsls::Types::Uint64 minJournalSize =
        journalSpOffset + mqbs::FileStoreProtocol::k_JOURNAL_RECORD_SIZE;

    if (static_cast<bsls::Types::Uint64>(fileSet.journalFileSize()) <
        minJournalSize) {
        // Yikes, this is bad.

        BMQTSK_ALARMLOG_ALARM("CLUSTER")
            << d_clusterData_p->identity().description() << " Partition ["
            << req.partitionId()
            << "]: Encountered invalid sync point JOURNAL offset: "
            << journalSpOffset
            << ", JOURNAL size: " << fileSet.journalFileSize()
            << ", sync point: " << lastSpoPair.syncPoint()
            << ", while processing partition-sync" << " data request: " << req
            << ", from: " << source->nodeDescription() << BMQTSK_ALARMLOG_END;

        bmqp_ctrlmsg::Status& status = controlMsg.choice().makeStatus();
        status.category()            = bmqp_ctrlmsg::StatusCategory::E_REFUSED;
        status.code()                = -1;
        status.message()             = "Peer's JOURNAL is invalid.";

        d_clusterData_p->messageTransmitter().sendMessageSafe(controlMsg,
                                                              source);
        return;  // RETURN
    }

    // Map JOURNAL file at appropriate offset.

    bmqu::MemOutStream errorDesc;
    int                rc = mqbs::FileStoreUtil::openFileSetReadMode(errorDesc,
                                                      fileSet,
                                                      &fti.journalFd(),
                                                      &fti.dataFd(),
                                                      &fti.qlistFd());
    if (0 != rc) {
        BMQTSK_ALARMLOG_ALARM("FILE_IO")
            << d_clusterData_p->identity().description() << " Partition ["
            << req.partitionId()
            << "]: Failed to open one of JOURNAL/QLIST/DATA file, rc: " << rc
            << ", reason [" << errorDesc.str()
            << "] while serving partition-sync data request: " << req
            << ", from node: " << source->nodeDescription()
            << BMQTSK_ALARMLOG_END;

        bmqp_ctrlmsg::Status& status = controlMsg.choice().makeStatus();
        status.category()            = bmqp_ctrlmsg::StatusCategory::E_UNKNOWN;
        status.code()                = -1;
        status.message()             = "Failed to open partition.";

        d_clusterData_p->messageTransmitter().sendMessageSafe(controlMsg,
                                                              source);
        return;  // RETURN
    }

    // Retrieve the sync point from JOURNAL.

    mqbs::OffsetPtr<const mqbs::RecordHeader> journalRec(
        fti.journalFd().block(),
        journalSpOffset);
    BSLS_ASSERT_SAFE(mqbs::RecordType::e_JOURNAL_OP == journalRec->type());
    static_cast<void>(journalRec);

    mqbs::OffsetPtr<const mqbs::JournalOpRecord> syncPtRec(
        fti.journalFd().block(),
        journalSpOffset);

    BSLS_ASSERT_SAFE(mqbs::JournalOpType::e_SYNCPOINT == syncPtRec->type());
    BSLS_ASSERT_SAFE(mqbs::SyncPointType::e_UNDEFINED !=
                     syncPtRec->syncPointType());

    bsls::Types::Uint64 qlistMapOffset =
        static_cast<bsls::Types::Uint64>(syncPtRec->qlistFileOffsetWords()) *
        bmqp::Protocol::k_WORD_SIZE;
    if (static_cast<bsls::Types::Uint64>(fileSet.qlistFileSize()) <
        qlistMapOffset) {
        // Yikes, this is bad.

        BMQTSK_ALARMLOG_ALARM("CLUSTER")
            << d_clusterData_p->identity().description() << " Partition ["
            << req.partitionId() << "]: Invalid QLIST [" << fileSet.qlistFile()
            << "] offset in sync point: " << lastSpoPair.syncPoint()
            << ". Current QLIST file size: " << fileSet.qlistFileSize()
            << ". Error encountered while serving partition-sync data request "
            << "from: " << source->nodeDescription() << BMQTSK_ALARMLOG_END;

        bmqp_ctrlmsg::Status& status = controlMsg.choice().makeStatus();
        status.category()            = bmqp_ctrlmsg::StatusCategory::E_REFUSED;
        status.code()                = -1;
        status.message()             = "Peer's QLIST file is invalid.";

        d_clusterData_p->messageTransmitter().sendMessageSafe(controlMsg,
                                                              source);
        return;  // RETURN
    }

    bsls::Types::Uint64 dataMapOffset =
        static_cast<bsls::Types::Uint64>(syncPtRec->dataFileOffsetDwords()) *
        bmqp::Protocol::k_DWORD_SIZE;
    if (static_cast<bsls::Types::Uint64>(fileSet.dataFileSize()) <
        dataMapOffset) {
        // Yikes, this is bad.

        BMQTSK_ALARMLOG_ALARM("CLUSTER")
            << d_clusterData_p->identity().description() << " Partition ["
            << req.partitionId() << "]: Invalid DATA [" << fileSet.dataFile()
            << "] offset in sync point: " << lastSpoPair.syncPoint()
            << ". Current DATA file size: " << fileSet.dataFileSize()
            << ". Error encountered while serving partition-sync data request "
            << "from: " << source->nodeDescription() << BMQTSK_ALARMLOG_END;

        bmqp_ctrlmsg::Status& status = controlMsg.choice().makeStatus();
        status.category()            = bmqp_ctrlmsg::StatusCategory::E_REFUSED;
        status.code()                = -1;
        status.message()             = "Peer's DATA file is invalid.";

        d_clusterData_p->messageTransmitter().sendMessageSafe(controlMsg,
                                                              source);
        return;  // RETURN
    }

    // All initial things have been validated at this point.  Reply to the
    // 'source' node informing it with what's coming its way.

    bmqp_ctrlmsg::ClusterMessage& clusterMsg =
        controlMsg.choice().makeClusterMessage();
    bmqp_ctrlmsg::PartitionSyncDataQueryResponse& response =
        clusterMsg.choice().makePartitionSyncDataQueryResponse();

    response.partitionId()       = req.partitionId();
    response.endPrimaryLeaseId() = selfSeqNum.primaryLeaseId();
    response.endSequenceNum()    = selfSeqNum.sequenceNumber();

    d_clusterData_p->messageTransmitter().sendMessageSafe(controlMsg, source);

    // Send storage events, by leveraging 'jit', which is currently pointing to
    // 'req.lastSyncPoint()'.  Send records from after the last sync point upto
    // the 'beginSeqNum'.

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << " Partition [" << req.partitionId()
                  << "]: replaying partition from: " << requesterLastSeqNum
                  << " (exclusive) to: " << requesterUptoSeqNum
                  << " (inclusive) with preceding sync-point JOURNAL offset "
                  << "of " << journalSpOffset << ", while serving "
                  << "partition-sync data request: " << req
                  << ", from: " << source->nodeDescription();

    rc = replayPartition(&requestCtx,
                         0,  // PrimarySyncContext
                         requestCtx.requesterNode(),
                         requesterLastSeqNum,
                         requesterUptoSeqNum,
                         journalSpOffset);
    if (0 != rc) {
        BMQTSK_ALARMLOG_ALARM("CLUSTER")
            << d_clusterData_p->identity().description() << " Partition ["
            << req.partitionId()
            << "]: Failed to replay partition while serving partition sync "
            << "request: " << req
            << ", from node: " << source->nodeDescription() << ", rc: " << rc
            << BMQTSK_ALARMLOG_END;
    }

    // Send final notification indicating end of partition-sync-data query
    // response, based on the 'rc' value.

    bmqp_ctrlmsg::ControlMessage                statusMsg;
    bmqp_ctrlmsg::PartitionSyncDataQueryStatus& queryStatus =
        statusMsg.choice()
            .makeClusterMessage()
            .choice()
            .makePartitionSyncDataQueryStatus();

    queryStatus.partitionId()   = req.partitionId();
    queryStatus.status().code() = rc;

    if (0 != rc) {
        queryStatus.status().category() =
            bmqp_ctrlmsg::StatusCategory::E_UNKNOWN;
        queryStatus.status().message() = "Peer failed to replay partition.";
    }
    else {
        queryStatus.status().category() =
            bmqp_ctrlmsg::StatusCategory::E_SUCCESS;
    }

    d_clusterData_p->messageTransmitter().sendMessageSafe(statusMsg, source);
}

void RecoveryManager::processPartitionSyncDataRequestStatus(
    const bmqp_ctrlmsg::ControlMessage& message,
    mqbnet::ClusterNode*                source)
{
    // executed by the *STORAGE (QUEUE) DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(source);

    const bmqp_ctrlmsg::PartitionSyncDataQueryStatus& queryStatus =
        message.choice()
            .clusterMessage()
            .choice()
            .partitionSyncDataQueryStatus();

    if (queryStatus.partitionId() < 0 ||
        queryStatus.partitionId() >=
            static_cast<int>(d_primarySyncContexts.size())) {
        BALL_LOG_WARN << d_clusterData_p->identity().description()
                      << ": Invalid partitionId in partition-sync data query "
                      << "status: " << queryStatus;
        return;  // RETURN
    }

    PrimarySyncContext& primarySyncCtx =
        d_primarySyncContexts[queryStatus.partitionId()];
    if (!primarySyncCtx.primarySyncInProgress()) {
        BALL_LOG_WARN << d_clusterData_p->identity().description()
                      << " Partition [" << queryStatus.partitionId()
                      << "]: primary sync is not in progress, for the received"
                      << " status: " << queryStatus;
        return;  // RETURN
    }

    if (primarySyncCtx.syncPeer() != source) {
        BALL_LOG_WARN << d_clusterData_p->identity().description()
                      << " Partition [" << queryStatus.partitionId()
                      << "] received partition sync request status: "
                      << queryStatus
                      << ", from node: " << source->nodeDescription()
                      << ", but sync is already in progress with node: "
                      << (primarySyncCtx.syncPeer()
                              ? primarySyncCtx.syncPeer()->nodeDescription()
                              : "** null **");
        return;  // RETURN
    }

    const bmqp_ctrlmsg::Status& status = queryStatus.status();

    if (status.category() != bmqp_ctrlmsg::StatusCategory::E_SUCCESS) {
        BMQTSK_ALARMLOG_ALARM("CLUSTER")
            << d_clusterData_p->identity().description() << " Partition ["
            << queryStatus.partitionId()
            << "]: partition-sync with peer: " << source->nodeDescription()
            << " failed with status: " << status << BMQTSK_ALARMLOG_END;
        onPartitionPrimarySyncStatus(queryStatus.partitionId(),
                                     -1 /* status */);
        return;  // RETURN
    }

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << " Partition [" << queryStatus.partitionId()
                  << "]: partition-sync with peer: "
                  << source->nodeDescription() << " succeeded. Self node will "
                  << "now attempt to sync replica peers.";

    syncPeerPartitions(&primarySyncCtx);

    // TBD: even though partition-sync may have failed w/ one or more peers, we
    // still notify partition-sync success to StorageMgr.  How should this be
    // handled?

    onPartitionPrimarySyncStatus(queryStatus.partitionId(), 0);
}

void RecoveryManager::onNodeStateChange(mqbnet::ClusterNode* node,
                                        bool                 isAvailable)
{
    // executed by *ANY* thread
    // RecoveryManager cares only about node-down event.
    if (isAvailable) {
        return;  // RETURN
    }

    // We can't check if the specified 'node' is serving storage sync request
    // for any partitions in this thread.  So we enqueue the node-down event in
    // all partitions' dispatcher threads.

    for (size_t pid = 0; pid < d_partitionsInfo.size(); ++pid) {
        const PartitionInfo& pinfo = d_partitionsInfo[pid];
        if (pinfo.isInitialized()) {
            const mqbi::DispatcherClientData& disp =
                pinfo.dispatcherClientData();
            d_dispatcher_p->execute(
                bdlf::BindUtil::bind(&RecoveryManager::onNodeDownDispatched,
                                     this,
                                     pid,
                                     node),
                disp);
        }
        else {
            BALL_LOG_WARN << d_clusterData_p->identity().description()
                          << ": Partition [" << pid
                          << "]: ignoring node state change event from peer: "
                          << node->nodeDescription();
        }
    }
}

}  // close package namespace
}  // close enterprise namespace
