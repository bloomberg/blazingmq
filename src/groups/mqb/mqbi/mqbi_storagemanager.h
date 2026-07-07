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

#ifndef INCLUDED_MQBI_STORAGEMANAGER
#define INCLUDED_MQBI_STORAGEMANAGER

//@PURPOSE: Provide an interface for storage manager, in charge of BlazingMQ
// storage.
//
//@CLASSES:
//  mqbi::StorageManager:
//
//@DESCRIPTION:
//
/// Thread Safety
///-------------
// Thread safe.

// MQB

#include <mqbi_dispatcher.h>
#include <mqbi_domain.h>
#include <mqbi_storage.h>
#include <mqbu_storagekey.h>

// BMQ
#include <bmqp_event.h>
#include <bmqt_uri.h>

// BDE
#include <ball_log.h>
#include <bmqp_ctrlmsg_messages.h>
#include <bsl_functional.h>
#include <bsl_memory.h>
#include <bsl_ostream.h>
#include <bsl_string.h>
#include <bsl_unordered_map.h>
#include <bsl_unordered_set.h>
#include <bsl_utility.h>
#include <bsl_vector.h>
#include <bslma_managedptr.h>
#include <bsls_atomic.h>
#include <bsls_cpp11.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace mqbc {
class ClusterState;
}
namespace mqbconfm {
class StorageDefinition;
}
namespace mqbcmd {
class StorageCommand;
}
namespace mqbcmd {
class StorageResult;
}
namespace mqbnet {
class ClusterNode;
}
namespace mqbs {
class FileStore;
}
namespace mqbs {
class ReplicatedStorage;
}
namespace mqbevt {
class RecoveryEvent;
class StorageEvent;
}

namespace mqbi {

// FORWARD DECLARATION
class Queue;

// ==================================
// class StorageManager_PartitionInfo
// ==================================

/// This class provides a mechanism to manage the partition details/info
/// stored inside storage.
class StorageManager_PartitionInfo {
  private:
    // DATA

    /// Node perceived as primary for this partition.
    mqbnet::ClusterNode* d_primary_p;

    /// Lease ID of the current primary.
    unsigned int d_primaryLeaseId;

    /// Primary status of this partition, stored as an int for atomic
    /// access.  Holds a `bmqp_ctrlmsg::PrimaryStatus::Value`.
    bsls::AtomicInt d_primaryStatus;

  public:
    // CREATORS
    StorageManager_PartitionInfo();

    StorageManager_PartitionInfo(const StorageManager_PartitionInfo& other);

    ~StorageManager_PartitionInfo();
    // Destructor

    // Copy assignment operator
    StorageManager_PartitionInfo&
    operator=(const StorageManager_PartitionInfo& rhs);

    // MANIPULATORS
    void setPrimary(mqbnet::ClusterNode* value);

    void setPrimaryLeaseId(unsigned int value);

    void setPrimaryStatus(bmqp_ctrlmsg::PrimaryStatus::Value value);

    // ACCESSORS
    mqbnet::ClusterNode* primary() const;

    unsigned int primaryLeaseId() const;

    /// Return the primary status of this partition.  Thread-safe.
    bmqp_ctrlmsg::PrimaryStatus::Value primaryStatus() const;
};

// =====================
// class StorageProvider
// =====================

/// Narrow interface for queue-infrastructure consumers (ClusterQueueHelper,
/// QueueState, LocalQueue, RemoteQueue, RootQueueEngine) that need to
/// register/unregister queues, configure storage, and query partition state.
/// Both the legacy 'StorageManager' and 'PartitionRaftManager' implement
/// this interface.
class StorageProvider {
  public:
    // TYPES
    typedef mqbi::Storage::AppInfos AppInfos;

  public:
    // CREATORS
    virtual ~StorageProvider();

    // MANIPULATORS

    /// Register a queue with the specified `uri`, `queueKey` and
    /// `partitionId`, having the specified `appIdKeyPairs`, and belonging
    /// to the specified `domain`.
    ///
    /// THREAD: Executed by the Client's dispatcher thread.
    virtual void registerQueue(const bmqt::Uri&        uri,
                               const mqbu::StorageKey& queueKey,
                               int                     partitionId,
                               const AppInfos&         appIdKeyPairs,
                               mqbi::Domain*           domain) = 0;

    /// Synchronously unregister the queue with the specified `uri` from the
    /// specified `partitionId`.  Behavior is undefined unless this routine
    /// is invoked from the cluster dispatcher thread.
    ///
    /// THREAD: Executed by the Client's dispatcher thread.
    virtual void unregisterQueue(const bmqt::Uri& uri, int partitionId) = 0;

    /// Configure the fanout queue having specified `uri` and assigned to
    /// the specified `partitionId` to have the specified `addedIdKeyPairs`
    /// appId/appKey pairs added and `removedIdKeyPairs` appId/appKey pairs
    /// removed.  Return zero on success, and non-zero value otherwise.
    /// Behavior is undefined unless this function is invoked at the primary
    /// node.  Behavior is also undefined unless the queue is configured in
    /// fanout mode.
    ///
    /// THREAD: Executed by the Queue's dispatcher thread.
    virtual int updateQueuePrimary(const bmqt::Uri& uri,
                                   int              partitionId,
                                   const AppInfos&  addedIdKeyPairs,
                                   const AppInfos&  removedIdKeyPairs) = 0;

    virtual int
    configureStorage(bsl::ostream&                      errorDescription,
                     bsl::shared_ptr<mqbi::Storage>*    out,
                     const bmqt::Uri&                   uri,
                     const mqbu::StorageKey&            queueKey,
                     int                                partitionId,
                     const bsls::Types::Int64           messageTtl,
                     const int                          maxDeliveryAttempts,
                     const mqbconfm::StorageDefinition& storageDef) = 0;

    /// Start this storage provider.  Return 0 on success, or a non-zero rc
    /// otherwise, populating the specified `errorDescription` with a
    /// description of the error.
    ///
    /// THREAD: Executed by the cluster's dispatcher thread.
    virtual int start(bsl::ostream& errorDescription) = 0;

    /// Stop this storage provider.
    virtual void stop() = 0;

    /// Executed by any thread.
    virtual void processShutdownEvent() = 0;

    /// Process the specified storage `command`, and load the outcome in the
    /// specified `result`.  This function can be invoked from any thread,
    /// and will block until the potentially asynchronous operation is
    /// complete.
    virtual void processCommand(mqbcmd::StorageResult*        result,
                                const mqbcmd::StorageCommand& command) = 0;

    /// Purge the queues on a given domain.
    virtual int purgeQueueOnDomain(mqbcmd::StorageResult* result,
                                   const bsl::string&     domainName) = 0;

    // ACCESSORS

    /// Return the processor handle in charge of the specified
    /// `partitionId`.  The behavior is undefined if `partitionId` does not
    /// represent a valid partition id.
    virtual mqbi::Dispatcher::ProcessorHandle
    processorForPartition(int partitionId) const = 0;

    /// Return true if the queue having specified `uri` and assigned to the
    /// specified `partitionId` has no messages, false in any other case.
    /// Behavior is undefined unless this routine is invoked from cluster
    /// dispatcher thread.
    virtual bool isStorageEmpty(const bmqt::Uri& uri,
                                int              partitionId) const = 0;
};

// ====================
// class StorageManager
// ====================

/// Storage Manager, in charge of all the partitions.
class StorageManager : public StorageProvider {
  public:
    // TYPES
    typedef mqbi::Storage::AppInfos AppInfos;

    typedef bsl::unordered_set<bsl::string> AppIds;
    typedef AppIds::iterator                AppIdsIter;
    typedef AppIds::const_iterator          AppIdsConstIter;
    typedef bsl::pair<AppIdsIter, bool>     AppIdsInsertRc;

    typedef bsl::shared_ptr<mqbs::ReplicatedStorage> StorageSp;

    /// Map of QueueUri -> ReplicatedStorageSp
    typedef bsl::unordered_map<bmqt::Uri, StorageSp> StorageSpMap;
    typedef StorageSpMap::iterator                   StorageSpMapIter;
    typedef StorageSpMap::const_iterator             StorageSpMapConstIter;

    typedef bsl::vector<StorageSpMap>       StorageSpMapVec;
    typedef StorageSpMapVec::iterator       StorageSpMapVecIter;
    typedef StorageSpMapVec::const_iterator StorageSpMapVecConstIter;

    typedef bsl::function<
        void(int partitionId, int status, unsigned int primaryLeaseId)>
        PartitionPrimaryStatusCb;

  public:
    // CREATORS

    /// Destructor
    ~StorageManager() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    /// Initialize the queue key info map based on information in the specified
    /// `clusterState`.  Note that this method should only be called once;
    /// subsequent calls will be ignored.
    virtual void
    initializeQueueKeyInfoMap(const mqbc::ClusterState& clusterState) = 0;

    /// Behavior is undefined unless the specified 'partitionId' is in range
    /// and the specified 'primaryNode' is not null.
    ///
    /// THREAD: Executed in cluster dispatcher thread.
    virtual void setPrimaryForPartition(int                  partitionId,
                                        mqbnet::ClusterNode* primaryNode,
                                        unsigned int primaryLeaseId) = 0;

    /// Behavior is undefined unless the specified 'partitionId' is in range
    /// and the specified 'primaryNode' is not null.
    ///
    /// THREAD: Executed in cluster dispatcher thread.
    virtual void clearPrimaryForPartition(int                  partitionId,
                                          mqbnet::ClusterNode* primary) = 0;

    /// Set the primary status of the specified 'partitionId' to the specified
    /// 'value'.
    ///
    /// THREAD: Executed in cluster dispatcher thread.
    virtual void
    setPrimaryStatusForPartition(int partitionId,
                                 bmqp_ctrlmsg::PrimaryStatus::Value value) = 0;

    /// Stop all Partition FSMs.
    ///
    /// THREAD: Executed in cluster dispatcher thread.
    virtual void stopPFSMs() = 0;

    /// Apply `RST_UNKNOWN` event to the PartitionFSM for the specified
    /// `partitionId`.
    ///
    /// THREAD: Executed in cluster dispatcher thread.
    virtual void detectPrimaryLossInPFSM(int partitionId) = 0;

    /// Apply DETECT_SelfPrimary event to Partition FSM using the specified
    /// `partitionId`, `primaryNode`, `primaryLeaseId`.
    ///
    /// THREAD: Executed in cluster dispatcher thread.
    virtual void detectSelfPrimaryInPFSM(int                  partitionId,
                                         mqbnet::ClusterNode* primaryNode,
                                         unsigned int primaryLeaseId) = 0;

    /// Apply DETECT_SelfReplica event to Partition FSM using the specified
    /// `partitionId`, `primaryNode` and `primaryLeaseId`.
    ///
    /// THREAD: Executed in cluster dispatcher thread.
    virtual void detectSelfReplicaInPFSM(int                  partitionId,
                                         mqbnet::ClusterNode* primaryNode,
                                         unsigned int primaryLeaseId) = 0;

    /// Process primary state request received from the specified `source`
    /// with the specified `message`.
    virtual void
    processPrimaryStateRequest(const bmqp_ctrlmsg::ControlMessage& message,
                               mqbnet::ClusterNode*                source) = 0;

    /// Process replica state request received from the specified `source`
    /// with the specified `message`.
    virtual void
    processReplicaStateRequest(const bmqp_ctrlmsg::ControlMessage& message,
                               mqbnet::ClusterNode*                source) = 0;

    /// Process replica data request received from the specified `source`
    /// with the specified `message`.
    virtual void
    processReplicaDataRequest(const bmqp_ctrlmsg::ControlMessage& message,
                              mqbnet::ClusterNode*                source) = 0;

    /// Executed in cluster dispatcher thread.
    virtual void processStorageEvent(const mqbevt::StorageEvent& event) = 0;

    /// Executed by any thread.
    virtual void
    processStorageSyncRequest(const bmqp_ctrlmsg::ControlMessage& message,
                              mqbnet::ClusterNode*                source) = 0;

    /// Executed by any thread.
    virtual void processPartitionSyncStateRequest(
        const bmqp_ctrlmsg::ControlMessage& message,
        mqbnet::ClusterNode*                source) = 0;

    /// Executed by any thread.
    virtual void processPartitionSyncDataRequest(
        const bmqp_ctrlmsg::ControlMessage& message,
        mqbnet::ClusterNode*                source) = 0;

    /// Executed by any thread.
    virtual void processPartitionSyncDataRequestStatus(
        const bmqp_ctrlmsg::ControlMessage& message,
        mqbnet::ClusterNode*                source) = 0;

    /// Executed in cluster dispatcher thread.
    virtual void processRecoveryEvent(const mqbevt::RecoveryEvent& event) = 0;

    /// Executed in IO thread.
    virtual void processReceiptEvent(const bmqp::Event&   event,
                                     mqbnet::ClusterNode* source) = 0;

    /// Executed by any thread.
    virtual void processPrimaryStatusAdvisory(
        const bmqp_ctrlmsg::PrimaryStatusAdvisory& advisory,
        mqbnet::ClusterNode*                       source) = 0;

    /// Executed by any thread.
    virtual void
    processReplicaStatusAdvisory(int                             partitionId,
                                 mqbnet::ClusterNode*            source,
                                 bmqp_ctrlmsg::NodeStatus::Value status) = 0;

    /// GC the queues from unrecognized domains, if any.
    virtual void gcUnrecognizedDomainQueues() = 0;

    // ACCESSORS

    /// Return partition corresponding to the specified `partitionId`.  The
    /// behavior is undefined if `partitionId` does not represent a valid
    /// partition id.
    virtual mqbs::FileStore& fileStore(int partitionId) const = 0;

    /// Load into the specified `result` all the storages of the specified
    /// `partitionId`.
    virtual void loadAllStorages(bsl::vector<StorageSp>* result,
                                 int                     partitionId) = 0;
};

// ============================================================================
//                            INLINE DEFINITIONS
// ============================================================================

// ----------------------------------
// class StorageManager_PartitionInfo
// ----------------------------------

// CREATORS
inline StorageManager_PartitionInfo::StorageManager_PartitionInfo()
: d_primary_p(0)
, d_primaryLeaseId(0)
, d_primaryStatus(static_cast<int>(bmqp_ctrlmsg::PrimaryStatus::E_PASSIVE))
{
}

inline StorageManager_PartitionInfo::StorageManager_PartitionInfo(
    const StorageManager_PartitionInfo& other)
: d_primary_p(other.d_primary_p)
, d_primaryLeaseId(other.d_primaryLeaseId)
, d_primaryStatus(other.d_primaryStatus.load())
{
}

inline StorageManager_PartitionInfo::~StorageManager_PartitionInfo()
{
}

inline StorageManager_PartitionInfo& StorageManager_PartitionInfo::operator=(
    const StorageManager_PartitionInfo& rhs)
{
    if (this != &rhs) {
        d_primary_p      = rhs.d_primary_p;
        d_primaryLeaseId = rhs.d_primaryLeaseId;
        d_primaryStatus  = rhs.d_primaryStatus.load();
    }
    return *this;
}

// MANIPULATORS
inline void
StorageManager_PartitionInfo::setPrimary(mqbnet::ClusterNode* value)
{
    d_primary_p = value;
}

inline void StorageManager_PartitionInfo::setPrimaryLeaseId(unsigned int value)
{
    d_primaryLeaseId = value;
}

inline void StorageManager_PartitionInfo::setPrimaryStatus(
    bmqp_ctrlmsg::PrimaryStatus::Value value)
{
    d_primaryStatus = static_cast<int>(value);
}

// ACCESSORS
inline mqbnet::ClusterNode* StorageManager_PartitionInfo::primary() const
{
    return d_primary_p;
}

inline unsigned int StorageManager_PartitionInfo::primaryLeaseId() const
{
    return d_primaryLeaseId;
}

inline bmqp_ctrlmsg::PrimaryStatus::Value
StorageManager_PartitionInfo::primaryStatus() const
{
    return static_cast<bmqp_ctrlmsg::PrimaryStatus::Value>(
        d_primaryStatus.load());
}

}  // close package namespace
}  // close enterprise namespace

#endif
