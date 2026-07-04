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

// mqbraft_partitionraftmanager.h -*-C++-*-
#ifndef INCLUDED_MQBRAFT_PARTITIONRAFTMANAGER
#define INCLUDED_MQBRAFT_PARTITIONRAFTMANAGER

//@PURPOSE: Provide a container that owns N PartitionRaft instances, manages
// per-partition queue storages, and routes events by partition ID.
//
//@CLASSES:
//  mqbraft::PartitionRaftManager: Container for per-partition Raft groups.
//
//@DESCRIPTION: This component owns one 'PartitionRaft' per partition, creates
// and owns FileStores, manages per-partition queue storage maps (via the
// inherited 'mqbc::StoragesMonitor'), routes incoming 'e_RAFT_PARTITION'
// events and Raft control messages to the correct instance by partition ID,
// and manages the start/stop lifecycle.  Replaces 'StorageManager' in Raft
// mode. Owned by 'ClusterOrchestrator'.
//
/// Threading
///----------
// 'start()' and 'stop()' are called from the cluster dispatcher thread.
// Callbacks ('recoveredQueuesCb', 'queueCreationCb', 'queueDeletionCb') are
// executed on the partition dispatcher thread.
// Event routing methods dispatch to the appropriate partition thread via the
// target PartitionRaft.

// MQB
#include <mqbc_clusterdata.h>
#include <mqbc_storageutil.h>
#include <mqbcfg_messages.h>
#include <mqbi_storagemanager.h>
#include <mqbraft_partitionraft.h>
#include <mqbs_datastore.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>

// BDE
#include <bmqma_countingallocatorstore.h>

#include <ball_log.h>
#include <bdlbb_blob.h>
#include <bdlmt_fixedthreadpool.h>
#include <bsl_memory.h>
#include <bsl_vector.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bslmt_mutex.h>

namespace BloombergLP {

namespace mqbi {
class Cluster;
class Dispatcher;
class DomainFactory;
}

namespace mqbc {
class ClusterState;
}

namespace mqbnet {
class ClusterNode;
}

namespace mqbraft {

// ===========================
// class PartitionRaftManager
// ===========================

class PartitionRaftManager : public mqbi::StorageProvider,
                             public mqbc::StoragesMonitor {
  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBRAFT.PARTITIONRAFTMANAGER");

    // TYPES
    typedef bsl::vector<bslma::ManagedPtr<PartitionRaft> > PartitionRafts;
    typedef mqbs::DataStoreConfig::QueueKeyInfoMap         QueueKeyInfoMap;

    // TYPES (private)
    typedef bsl::shared_ptr<mqbs::FileStore> FileStoreSp;
    typedef bsl::vector<FileStoreSp>         FileStores;

    // DATA
    PartitionRafts d_partitionRafts;
    FileStores     d_fileStores;

    bdlmt::FixedThreadPool           d_miscWorkThreadPool;
    bmqma::CountingAllocatorStore    d_allocators;
    mqbc::ClusterData*               d_clusterData_p;
    mqbi::Cluster*                   d_cluster_p;
    mqbi::Dispatcher*                d_dispatcher_p;
    mqbi::DomainFactory*             d_domainFactory_p;
    mqbc::ClusterState*              d_clusterState_p;
    const mqbcfg::ClusterDefinition& d_clusterConfig;
    int                              d_replicationFactor;
    int                              d_numPartitionsRecoveredQueues;
    bslma::Allocator*                d_allocator_p;

    // NOT IMPLEMENTED
    PartitionRaftManager(const PartitionRaftManager&);
    PartitionRaftManager& operator=(const PartitionRaftManager&);

    // PRIVATE MANIPULATORS

    /// Callback invoked by FileStore after recovering queues from the
    /// journal for the specified 'partitionId'.
    void recoveredQueuesCb(int                    partitionId,
                           const QueueKeyInfoMap* queueKeyInfoMap);

    /// Callback invoked by FileStore when a queue creation record is
    /// replicated to this node for the specified 'partitionId'.
    void queueCreationCb(int                            partitionId,
                         const bmqt::Uri&               uri,
                         const mqbu::StorageKey&        queueKey,
                         const mqbi::Storage::AppInfos& appIdKeyPairs,
                         bool                           isNewQueue);

    /// Callback invoked by FileStore when a queue deletion record is
    /// replicated to this node for the specified 'partitionId'.
    void queueDeletionCb(int                     partitionId,
                         const bmqt::Uri&        uri,
                         const mqbu::StorageKey& queueKey,
                         const mqbu::StorageKey& appKey);

    /// Executed by the partition *DISPATCHER* thread for the specified
    /// 'partitionId'.
    void processShutdownEventDispatched(int partitionId);

    /// Executed by the partition *DISPATCHER* thread for the specified
    /// 'partitionId'.
    void unregisterQueueDispatched(int partitionId, const bmqt::Uri& uri);

    bool validate(unsigned int partitionId) const;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(PartitionRaftManager,
                                   bslma::UsesBslmaAllocator)

    // CREATORS
    PartitionRaftManager(mqbc::ClusterData*               clusterData,
                         mqbi::Cluster*                   cluster,
                         mqbi::Dispatcher*                dispatcher,
                         mqbi::DomainFactory*             domainFactory,
                         mqbc::ClusterState*              clusterState,
                         const mqbcfg::ClusterDefinition& clusterConfig,
                         bslma::Allocator*                allocator = 0);

    ~PartitionRaftManager() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    /// Create FileStores, assign dispatcher threads, create and start all
    /// partition Raft groups.  Return 0 on success.
    int start(bsl::ostream& errorDescription) BSLS_KEYWORD_OVERRIDE;

    /// Stop all partition Raft groups.
    void stop() BSLS_KEYWORD_OVERRIDE;

    /// Executed by any thread.
    void processShutdownEvent() BSLS_KEYWORD_OVERRIDE;

    /// Route an incoming binary AppendEntries event (e_RAFT_PARTITION)
    /// from the specified 'source' to the correct PartitionRaft by
    /// extracting the partition ID from the RaftHeader.
    void appendEntries(const bsl::shared_ptr<const bdlbb::Blob>& blob,
                       mqbnet::ClusterNode*                      source);

    /// Route an incoming snapshot chunk event (e_RAFT_SNAPSHOT) from the
    /// specified 'source' to the correct PartitionRaft by extracting the
    /// partition ID from the SnapshotChunkHeader.
    void appendSnapshotChunk(const bsl::shared_ptr<const bdlbb::Blob>& blob,
                             mqbnet::ClusterNode*                      source);

    /// Route an incoming Raft control message for the specified
    /// 'partitionId' from the specified 'source' to the correct
    /// PartitionRaft.
    void onRaftControlMessage(const bmqp_ctrlmsg::RaftMessage& message,
                              int                              partitionId,
                              mqbnet::ClusterNode*             source);

    // StorageProvider OVERRIDES

    /// Register a queue with the specified `uri`, `queueKey` and
    /// `partitionId`, having the specified `appIdKeyPairs`, and belonging
    /// to the specified `domain`.
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

    /// Configure the fanout queue having specified `uri` and assigned to
    /// the specified `partitionId` to have the specified `addedIdKeyPairs`
    /// appId/appKey pairs added and `removedIdKeyPairs` appId/appKey pairs
    /// removed.  Return zero on success, and non-zero value otherwise.
    /// Behavior is undefined unless this function is invoked at the primary
    /// node.  Behavior is also undefined unless the queue is configured in
    /// fanout mode.
    ///
    /// THREAD: Executed by the Queue's dispatcher thread.
    int updateQueuePrimary(const bmqt::Uri& uri,
                           int              partitionId,
                           const AppInfos&  addedIdKeyPairs,
                           const AppInfos&  removedIdKeyPairs)
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

    /// Process the specified storage `command`, and load the outcome in the
    /// specified `result`.  This function can be invoked from any thread,
    /// and will block until the potentially asynchronous operation is
    /// complete.
    void processCommand(mqbcmd::StorageResult*        result,
                        const mqbcmd::StorageCommand& command)
        BSLS_KEYWORD_OVERRIDE;

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

    /// Return the number of partitions.
    int numPartitions() const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

inline int PartitionRaftManager::numPartitions() const
{
    return static_cast<int>(d_partitionRafts.size());
}

inline bool PartitionRaftManager::validate(unsigned int partitionId) const
{
    if (partitionId >= d_partitionRafts.size()) {
        BALL_LOG_ERROR << "e_RAFT_PARTITION event with invalid partitionId "
                       << partitionId;
        return false;
    }

    if (!d_partitionRafts[partitionId]) {
        BALL_LOG_ERROR << "PartitionRaft not started for partition "
                       << partitionId;
        return false;
    }
    return true;
}

}  // close package namespace
}  // close enterprise namespace

#endif
