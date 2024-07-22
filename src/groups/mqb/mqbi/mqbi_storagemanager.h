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

// mqbi_storagemanager.h                                              -*-C++-*-
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

#include <mqbi_appkeygenerator.h>
#include <mqbi_dispatcher.h>
#include <mqbi_domain.h>
#include <mqbi_storage.h>
#include <mqbu_storagekey.h>

// BMQ
#include <bmqp_event.h>
#include <bmqt_uri.h>

// BDE
#include <ball_log.h>
#include <bsl_functional.h>
#include <bsl_memory.h>
#include <bsl_ostream.h>
#include <bsl_string.h>
#include <bsl_unordered_map.h>
#include <bsl_unordered_set.h>
#include <bsl_utility.h>
#include <bsl_vector.h>
#include <bslma_managedptr.h>
#include <bsls_cpp11.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace mqbc {
class ClusterState;
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
    mqbnet::ClusterNode* d_primary_p;

    unsigned int d_primaryLeaseId;

    bmqp_ctrlmsg::PrimaryStatus::Value d_primaryStatus;

  public:
    // CREATORS
    StorageManager_PartitionInfo();

    ~StorageManager_PartitionInfo();
    // Destructor

    // MANIPULATORS
    void setPrimary(mqbnet::ClusterNode* value);

    void setPrimaryLeaseId(unsigned int value);

    void setPrimaryStatus(bmqp_ctrlmsg::PrimaryStatus::Value value);

    // ACCESSORS
    mqbnet::ClusterNode* primary() const;

    unsigned int primaryLeaseId() const;

    bmqp_ctrlmsg::PrimaryStatus::Value primaryStatus() const;
};

// ============================
// class StorageManagerIterator
// ============================

/// Interface to provide a way for iteration through all the storages of a
/// partition in the storage manager.
class StorageManagerIterator {
  public:
    // CREATORS

    /// Destroy this iterator and unlock the storage manager associated with
    /// it.
    virtual ~StorageManagerIterator();

    // MANIPULATORS

    /// Advance this iterator to refer to the next storage of the associated
    /// partition; if there is no next storage in the associated partition,
    /// then this iterator becomes *invalid*.  The behavior is undefined
    /// unless this iterator is valid.  Note that the order of the iteration
    /// is not specified.
    virtual void operator++() = 0;

    // ACCESSORS

    /// Return non-zero if the iterator is *valid*, and 0 otherwise.
    virtual operator const void*() const = 0;

    /// Return a reference offering non-modifiable access to the queue URI
    /// being pointed by this iterator.  The behavior is undefined unless
    /// the iterator is *valid*.
    virtual const bmqt::Uri& uri() const = 0;

    /// Return a reference offering non-modifiable access to the storage
    /// being pointed by this iterator.  The behavior is undefined unless
    /// the iterator is *valid*. Note that since iterator is not a first
    /// class object, its okay to pass a raw pointer.
    virtual const mqbi::Storage* storage() const = 0;
};

// ====================
// class StorageManager
// ====================

/// Storage Manager, in charge of all the partitions.
class StorageManager : public mqbi::AppKeyGenerator {
  public:
    // TYPES
    typedef mqbi::Storage::AppIdKeyPair   AppIdKeyPair;
    typedef mqbi::Storage::AppIdKeyPairs  AppIdKeyPairs;
    typedef AppIdKeyPairs::const_iterator AppIdKeyPairsCIter;

    typedef bsl::unordered_set<bsl::string> AppIds;
    typedef AppIds::iterator                AppIdsIter;
    typedef AppIds::const_iterator          AppIdsConstIter;
    typedef bsl::pair<AppIdsIter, bool>     AppIdsInsertRc;

    typedef bsl::unordered_set<mqbu::StorageKey> AppKeys;
    typedef AppKeys::iterator                    AppKeysIter;
    typedef AppKeys::const_iterator              AppKeysConstIter;
    typedef bsl::pair<AppKeysIter, bool>         AppKeysInsertRc;

    typedef bsl::vector<AppKeys>       AppKeysVec;
    typedef AppKeysVec::iterator       AppKeysVecIter;
    typedef AppKeysVec::const_iterator AppKeysVecConstIter;

    typedef bsl::shared_ptr<mqbs::ReplicatedStorage> StorageSp;

    /// Map of QueueUri -> ReplicatedStorageSp
    typedef bsl::unordered_map<bmqt::Uri, StorageSp> StorageSpMap;
    typedef StorageSpMap::iterator                   StorageSpMapIter;
    typedef StorageSpMap::const_iterator             StorageSpMapConstIter;

    typedef bsl::vector<StorageSpMap>       StorageSpMapVec;
    typedef StorageSpMapVec::iterator       StorageSpMapVecIter;
    typedef StorageSpMapVec::const_iterator StorageSpMapVecConstIter;

    /// Type of the functor required by `applyForEachQueue`.
    typedef bsl::function<void(mqbi::Queue*)> QueueFunctor;

    typedef bsl::function<
        void(int partitionId, int status, unsigned int primaryLeaseId)>
        PartitionPrimaryStatusCb;

  public:
    // CREATORS

    /// Destructor
    ~StorageManager() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    /// Start this storage manager.  Return 0 on success, or a non-zero rc
    /// otherwise, populating the specified `errorDescription` with a
    /// description of the error.
    ///
    /// THREAD: Executed by the cluster's dispatcher thread.
    virtual int start(bsl::ostream& errorDescription) = 0;

    /// Stop this storage manager.
    virtual void stop() = 0;

    /// Initialize the queue key info map based on information in the specified
    /// `clusterState`.
    virtual void
    initializeQueueKeyInfoMap(const mqbc::ClusterState* clusterState) = 0;

    /// Register a queue with the specified `uri`, `queueKey` and
    /// `partitionId`, having the specified `appIdKeyPairs`, and belonging
    /// to the specified `domain`.  Load into the specified `storage` the
    /// associated queue storage created.
    ///
    /// THREAD: Executed by the Client's dispatcher thread.
    virtual void registerQueue(const bmqt::Uri&        uri,
                               const mqbu::StorageKey& queueKey,
                               int                     partitionId,
                               const AppIdKeyPairs&    appIdKeyPairs,
                               mqbi::Domain*           domain) = 0;

    /// Synchronously unregister the queue with the specified `uri` from the
    /// specified `partitionId`.  Behavior is undefined unless this routine
    /// is invoked from the cluster dispatcher thread.
    ///
    /// THREAD: Executed by the Client's dispatcher thread.
    virtual void unregisterQueue(const bmqt::Uri& uri, int partitionId) = 0;

    /// Configure the fanout queue having specified `uri` and `queueKey`,
    /// assigned to the specified `partitionId` to have the specified
    /// `addedIdKeyPairs` appId/appKey pairs added and `removedIdKeyPairs`
    /// appId/appKey pairs removed.  Return zero on success, and non-zero
    /// value otherwise.  Behavior is undefined unless this function is
    /// invoked at the primary node.  Behavior is also undefined unless the
    /// queue is configured in fanout mode.
    ///
    /// THREAD: Executed by the Queue's dispatcher thread.
    virtual int updateQueue(const bmqt::Uri&        uri,
                            const mqbu::StorageKey& queueKey,
                            int                     partitionId,
                            const AppIdKeyPairs&    addedIdKeyPairs,
                            const AppIdKeyPairs&    removedIdKeyPairs) = 0;

    virtual void registerQueueReplica(int                     partitionId,
                                      const bmqt::Uri&        uri,
                                      const mqbu::StorageKey& queueKey,
                                      mqbi::Domain*           domain = 0,
                                      bool allowDuplicate = false) = 0;

    virtual void unregisterQueueReplica(int                     partitionId,
                                        const bmqt::Uri&        uri,
                                        const mqbu::StorageKey& queueKey,
                                        const mqbu::StorageKey& appKey) = 0;

    virtual void updateQueueReplica(int                     partitionId,
                                    const bmqt::Uri&        uri,
                                    const mqbu::StorageKey& queueKey,
                                    const AppIdKeyPairs&    appIdKeyPairs,
                                    mqbi::Domain*           domain = 0,
                                    bool allowDuplicate = false) = 0;

    /// Return a unique appKey for the specified `appId` for a queue
    /// assigned to the specified `partitionId`.  This routine can be
    /// invoked by any thread.
    mqbu::StorageKey generateAppKey(const bsl::string& appId,
                                    int partitionId) BSLS_KEYWORD_OVERRIDE = 0;

    /// Set the queue instance associated with the file-backed storage for
    /// the specified `uri` mapped to the specified `partitionId` to the
    /// specified `queue` value.  Note that this method *does* *not*
    /// synchronize on the queue-dispatcher thread.
    virtual void
    setQueue(mqbi::Queue* queue, const bmqt::Uri& uri, int partitionId) = 0;

    /// Set the queue instance associated with the file-backed storage for
    /// the specified `uri` mapped to the specified `partitionId` to the
    /// specified `queue` value.  Behavior is undefined unless `queue` is
    /// non-null or unless this routine is invoked from the dispatcher
    /// thread associated with the `partitionId`.
    virtual void
    setQueueRaw(mqbi::Queue* queue, const bmqt::Uri& uri, int partitionId) = 0;

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

    virtual int makeStorage(bsl::ostream&                     errorDescription,
                            bslma::ManagedPtr<mqbi::Storage>* out,
                            const bmqt::Uri&                  uri,
                            const mqbu::StorageKey&           queueKey,
                            int                               partitionId,
                            const bsls::Types::Int64          messageTtl,
                            const int maxDeliveryAttempts,
                            const mqbconfm::StorageDefinition& storageDef) = 0;

    /// Executed in cluster dispatcher thread.
    virtual void
    processStorageEvent(const mqbi::DispatcherStorageEvent& event) = 0;

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
    virtual void
    processRecoveryEvent(const mqbi::DispatcherRecoveryEvent& event) = 0;

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

    /// Executed by any thread.
    virtual void processShutdownEvent() = 0;

    /// Invoke the specified `functor` with each queue associated to the
    /// partition identified by the specified `partitionId` if that
    /// partition has been successfully opened.  The behavior is undefined
    /// unless invoked from the queue thread corresponding to `partitionId`.
    virtual void applyForEachQueue(int                 partitionId,
                                   const QueueFunctor& functor) const = 0;

    /// Process the specified `command`, and load the result to the
    /// specified `result`.  Return 0 if the command was successfully
    /// processed, or a non-zero value otherwise.  This function can be
    /// invoked from any thread, and will block until the potentially
    /// asynchronous operation is complete.
    virtual int processCommand(mqbcmd::StorageResult*        result,
                               const mqbcmd::StorageCommand& command) = 0;

    /// GC the queues from unrecognized domains, if any.
    virtual void gcUnrecognizedDomainQueues() = 0;

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

    /// Return partition corresponding to the specified `partitionId`.  The
    /// behavior is undefined if `partitionId` does not represent a valid
    /// partition id.
    virtual const mqbs::FileStore& fileStore(int partitionId) const = 0;

    /// Return a StorageManagerIterator for the specified `partitionId`.
    virtual bslma::ManagedPtr<StorageManagerIterator>
    getIterator(int partitionId) const = 0;
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
, d_primaryStatus(bmqp_ctrlmsg::PrimaryStatus::E_PASSIVE)
{
}

inline StorageManager_PartitionInfo::~StorageManager_PartitionInfo()
{
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
    d_primaryStatus = value;
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
    return d_primaryStatus;
}

}  // close package namespace
}  // close enterprise namespace

#endif
