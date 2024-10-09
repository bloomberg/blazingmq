// Copyright 2021-2024 Bloomberg Finance L.P.
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

// mqbmock_storagemanager.h                                           -*-C++-*-
#ifndef INCLUDED_MQBMOCK_STORAGEMANAGER
#define INCLUDED_MQBMOCK_STORAGEMANAGER

//@PURPOSE: Provide a mock implementation of 'mqbi::StorageManager' interface.
//
//@CLASSES:
//  mqbmock::StorageManager: Mock impl of 'mqbi::StorageManager'
//
//@DESCRIPTION: This component provides a mock implementation,
// 'mqbmock::StorageManager', of the 'mqbi::StorageManager' protocol.
//
/// Thread Safety
///-------------
// The 'mqbmock::StorageManager' object is not thread safe.

// MQB
#include <mqbi_storagemanager.h>

// BDE
#include <bsls_keyword.h>

namespace BloombergLP {
namespace mqbmock {

// ====================
// class StorageManager
// ====================

/// Mock implementation of `mqbi::StorageManager` interface.
class StorageManager : public mqbi::StorageManager {
  public:
    // CREATORS

    /// Create a new `StorageManager` instance
    StorageManager();

    /// Destructor
    ~StorageManager() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    /// Start this storage manager.  Return 0 on success, or a non-zero rc
    /// otherwise, populating the specified `errorDescription` with a
    /// description of the error.
    ///
    /// THREAD: Executed by the cluster's dispatcher thread.
    virtual int start(bsl::ostream& errorDescription) BSLS_KEYWORD_OVERRIDE;

    /// Stop this storage manager.
    virtual void stop() BSLS_KEYWORD_OVERRIDE;

    /// Initialize the queue key info map based on information in the specified
    /// `clusterState`.  Note that this method should only be called once;
    /// subsequent calls will be ignored.
    virtual void initializeQueueKeyInfoMap(
        const mqbc::ClusterState& clusterState) BSLS_KEYWORD_OVERRIDE;

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
                               mqbi::Domain* domain) BSLS_KEYWORD_OVERRIDE;

    /// Synchronously unregister the queue with the specified `uri` from the
    /// specified `partitionId`.  Behavior is undefined unless this routine
    /// is invoked from the cluster dispatcher thread.
    ///
    /// THREAD: Executed by the Client's dispatcher thread.
    virtual void unregisterQueue(const bmqt::Uri& uri,
                                 int partitionId) BSLS_KEYWORD_OVERRIDE;

    /// Configure the fanout queue having specified `uri` and `queueKey`,
    /// assigned to the specified `partitionId` to have the specified
    /// `addedIdKeyPairs` appId/appKey pairs added and `removedIdKeyPairs`
    /// appId/appKey pairs removed.  Return zero on success, and non-zero
    /// value otherwise.  Behavior is undefined unless this function is
    /// invoked at the primary node.  Behavior is also undefined unless the
    /// queue is configured in fanout mode.
    ///
    /// THREAD: Executed by the Queue's dispatcher thread.
    virtual int
    updateQueue(const bmqt::Uri&        uri,
                const mqbu::StorageKey& queueKey,
                int                     partitionId,
                const AppIdKeyPairs&    addedIdKeyPairs,
                const AppIdKeyPairs& removedIdKeyPairs) BSLS_KEYWORD_OVERRIDE;

    virtual void
    registerQueueReplica(int                     partitionId,
                         const bmqt::Uri&        uri,
                         const mqbu::StorageKey& queueKey,
                         mqbi::Domain*           domain = 0,
                         bool allowDuplicate = false) BSLS_KEYWORD_OVERRIDE;

    virtual void unregisterQueueReplica(int                     partitionId,
                                        const bmqt::Uri&        uri,
                                        const mqbu::StorageKey& queueKey,
                                        const mqbu::StorageKey& appKey)
        BSLS_KEYWORD_OVERRIDE;

    virtual void
    updateQueueReplica(int                     partitionId,
                       const bmqt::Uri&        uri,
                       const mqbu::StorageKey& queueKey,
                       const AppIdKeyPairs&    appIdKeyPairs,
                       mqbi::Domain*           domain = 0,
                       bool allowDuplicate = false) BSLS_KEYWORD_OVERRIDE;

    /// Return a unique appKey for the specified `appId` for a queue
    /// assigned to the specified `partitionId`.  This routine can be
    /// invoked by any thread.
    mqbu::StorageKey generateAppKey(const bsl::string& appId,
                                    int partitionId) BSLS_KEYWORD_OVERRIDE;

    /// Set the queue instance associated with the file-backed storage for
    /// the specified `uri` mapped to the specified `partitionId` to the
    /// specified `queue` value.  Note that this method *does* *not*
    /// synchronize on the queue-dispatcher thread.
    virtual void setQueue(mqbi::Queue*     queue,
                          const bmqt::Uri& uri,
                          int              partitionId) BSLS_KEYWORD_OVERRIDE;

    /// Set the queue instance associated with the file-backed storage for
    /// the specified `uri` mapped to the specified `partitionId` to the
    /// specified `queue` value.  Behavior is undefined unless `queue` is
    /// non-null or unless this routine is invoked from the dispatcher
    /// thread associated with the `partitionId`.
    virtual void setQueueRaw(mqbi::Queue*     queue,
                             const bmqt::Uri& uri,
                             int partitionId) BSLS_KEYWORD_OVERRIDE;

    /// Behavior is undefined unless the specified 'partitionId' is in range
    /// and the specified 'primaryNode' is not null.
    ///
    /// THREAD: Executed in cluster dispatcher thread.
    virtual void
    setPrimaryForPartition(int                  partitionId,
                           mqbnet::ClusterNode* primaryNode,
                           unsigned int primaryLeaseId) BSLS_KEYWORD_OVERRIDE;

    /// Behavior is undefined unless the specified 'partitionId' is in range
    /// and the specified 'primaryNode' is not null.
    ///
    /// THREAD: Executed in cluster dispatcher thread.
    virtual void clearPrimaryForPartition(int                  partitionId,
                                          mqbnet::ClusterNode* primary)
        BSLS_KEYWORD_OVERRIDE;

    /// Set the primary status of the specified 'partitionId' to the specified
    /// 'value'.
    ///
    /// THREAD: Executed in cluster dispatcher thread.
    virtual void setPrimaryStatusForPartition(
        int                                partitionId,
        bmqp_ctrlmsg::PrimaryStatus::Value value) BSLS_KEYWORD_OVERRIDE;

    /// Process primary state request received from the specified `source`
    /// with the specified `message`.
    virtual void processPrimaryStateRequest(
        const bmqp_ctrlmsg::ControlMessage& message,
        mqbnet::ClusterNode*                source) BSLS_KEYWORD_OVERRIDE;

    /// Process replica state request received from the specified `source`
    /// with the specified `message`.
    virtual void processReplicaStateRequest(
        const bmqp_ctrlmsg::ControlMessage& message,
        mqbnet::ClusterNode*                source) BSLS_KEYWORD_OVERRIDE;

    /// Process replica data request received from the specified `source`
    /// with the specified `message`.
    virtual void processReplicaDataRequest(
        const bmqp_ctrlmsg::ControlMessage& message,
        mqbnet::ClusterNode*                source) BSLS_KEYWORD_OVERRIDE;

    virtual int makeStorage(bsl::ostream&                     errorDescription,
                            bslma::ManagedPtr<mqbi::Storage>* out,
                            const bmqt::Uri&                  uri,
                            const mqbu::StorageKey&           queueKey,
                            int                               partitionId,
                            const bsls::Types::Int64          messageTtl,
                            const int maxDeliveryAttempts,
                            const mqbconfm::StorageDefinition& storageDef)
        BSLS_KEYWORD_OVERRIDE;

    /// Executed in cluster dispatcher thread.
    virtual void processStorageEvent(const mqbi::DispatcherStorageEvent& event)
        BSLS_KEYWORD_OVERRIDE;

    /// Executed by any thread.
    virtual void processStorageSyncRequest(
        const bmqp_ctrlmsg::ControlMessage& message,
        mqbnet::ClusterNode*                source) BSLS_KEYWORD_OVERRIDE;

    /// Executed by any thread.
    virtual void processPartitionSyncStateRequest(
        const bmqp_ctrlmsg::ControlMessage& message,
        mqbnet::ClusterNode*                source) BSLS_KEYWORD_OVERRIDE;

    /// Executed by any thread.
    virtual void processPartitionSyncDataRequest(
        const bmqp_ctrlmsg::ControlMessage& message,
        mqbnet::ClusterNode*                source) BSLS_KEYWORD_OVERRIDE;

    /// Executed by any thread.
    virtual void processPartitionSyncDataRequestStatus(
        const bmqp_ctrlmsg::ControlMessage& message,
        mqbnet::ClusterNode*                source) BSLS_KEYWORD_OVERRIDE;

    /// Executed in cluster dispatcher thread.
    virtual void processRecoveryEvent(
        const mqbi::DispatcherRecoveryEvent& event) BSLS_KEYWORD_OVERRIDE;

    /// Executed in IO thread.
    virtual void
    processReceiptEvent(const bmqp::Event&   event,
                        mqbnet::ClusterNode* source) BSLS_KEYWORD_OVERRIDE;

    /// Executed by any thread.
    virtual void bufferPrimaryStatusAdvisory(
        const bmqp_ctrlmsg::PrimaryStatusAdvisory& advisory,
        mqbnet::ClusterNode* source) BSLS_KEYWORD_OVERRIDE;

    /// Executed by any thread.
    virtual void processPrimaryStatusAdvisory(
        const bmqp_ctrlmsg::PrimaryStatusAdvisory& advisory,
        mqbnet::ClusterNode* source) BSLS_KEYWORD_OVERRIDE;

    /// Executed by any thread.
    virtual void processReplicaStatusAdvisory(
        int                             partitionId,
        mqbnet::ClusterNode*            source,
        bmqp_ctrlmsg::NodeStatus::Value status) BSLS_KEYWORD_OVERRIDE;

    /// Executed by any thread.
    virtual void processShutdownEvent() BSLS_KEYWORD_OVERRIDE;

    /// Invoke the specified `functor` with each queue associated to the
    /// partition identified by the specified `partitionId` if that
    /// partition has been successfully opened.  The behavior is undefined
    /// unless invoked from the queue thread corresponding to `partitionId`.
    virtual void
    applyForEachQueue(int                 partitionId,
                      const QueueFunctor& functor) const BSLS_KEYWORD_OVERRIDE;

    /// Process the specified `command`, and load the result to the
    /// specified `result`.  Return 0 if the command was successfully
    /// processed, or a non-zero value otherwise.  This function can be
    /// invoked from any thread, and will block until the potentially
    /// asynchronous operation is complete.
    virtual int processCommand(mqbcmd::StorageResult*        result,
                               const mqbcmd::StorageCommand& command)
        BSLS_KEYWORD_OVERRIDE;

    /// GC the queues from unrecognized domains, if any.
    virtual void gcUnrecognizedDomainQueues() BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS

    /// Return the processor handle in charge of the specified
    /// `partitionId`.  The behavior is undefined if `partitionId` does not
    /// represent a valid partition id.
    virtual mqbi::Dispatcher::ProcessorHandle
    processorForPartition(int partitionId) const BSLS_KEYWORD_OVERRIDE;

    /// Return true if the queue having specified `uri` and assigned to the
    /// specified `partitionId` has no messages, false in any other case.
    /// Behavior is undefined unless this routine is invoked from cluster
    /// dispatcher thread.
    virtual bool isStorageEmpty(const bmqt::Uri& uri,
                                int partitionId) const BSLS_KEYWORD_OVERRIDE;

    /// Return partition corresponding to the specified `partitionId`.  The
    /// behavior is undefined if `partitionId` does not represent a valid
    /// partition id.
    virtual const mqbs::FileStore&
    fileStore(int partitionId) const BSLS_KEYWORD_OVERRIDE;

    /// Return a StorageManagerIterator for the specified `partitionId`.
    virtual bslma::ManagedPtr<mqbi::StorageManagerIterator>
    getIterator(int partitionId) const BSLS_KEYWORD_OVERRIDE;
};

}  // close package namespace
}  // close enterprise namespace

#endif
