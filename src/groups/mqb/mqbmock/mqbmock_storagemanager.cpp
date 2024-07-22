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

// mqbmock_storagemanager.cpp                                         -*-C++-*-
#include <mqbmock_storagemanager.h>

#include <mqbscm_version.h>

// BDE
#include <bsls_annotation.h>

namespace BloombergLP {
namespace mqbmock {

// --------------------
// class StorageManager
// --------------------

// CREATORS
StorageManager::StorageManager()
{
    // NOTHING
}

StorageManager::~StorageManager()
{
    // NOTHING
}

// MANIPULATORS
int StorageManager::start(
    BSLS_ANNOTATION_UNUSED bsl::ostream& errorDescription)
{
    return 0;
}

void StorageManager::stop()
{
    // NOTHING
}

void StorageManager::initializeQueueKeyInfoMap(
    BSLS_ANNOTATION_UNUSED const mqbc::ClusterState* clusterState)
{
}

void StorageManager::registerQueue(
    BSLS_ANNOTATION_UNUSED const bmqt::Uri& uri,
    BSLS_ANNOTATION_UNUSED const mqbu::StorageKey& queueKey,
    BSLS_ANNOTATION_UNUSED int                     partitionId,
    BSLS_ANNOTATION_UNUSED const AppIdKeyPairs&    appIdKeyPairs,
    BSLS_ANNOTATION_UNUSED mqbi::Domain* domain)
{
    // NOTHING
}

void StorageManager::unregisterQueue(
    BSLS_ANNOTATION_UNUSED const bmqt::Uri& uri,
    BSLS_ANNOTATION_UNUSED int              partitionId)
{
    // NOTHING
}

int StorageManager::updateQueue(
    BSLS_ANNOTATION_UNUSED const bmqt::Uri& uri,
    BSLS_ANNOTATION_UNUSED const mqbu::StorageKey& queueKey,
    BSLS_ANNOTATION_UNUSED int                     partitionId,
    BSLS_ANNOTATION_UNUSED const AppIdKeyPairs&    addedIdKeyPairs,
    BSLS_ANNOTATION_UNUSED const AppIdKeyPairs&    removedIdKeyPairs)
{
    return 0;
}

void StorageManager::registerQueueReplica(
    BSLS_ANNOTATION_UNUSED int   partitionId,
    BSLS_ANNOTATION_UNUSED const bmqt::Uri& uri,
    BSLS_ANNOTATION_UNUSED const mqbu::StorageKey& queueKey,
    BSLS_ANNOTATION_UNUSED mqbi::Domain* domain,
    BSLS_ANNOTATION_UNUSED bool          allowDuplicate)
{
    // NOTHING
}

void StorageManager::unregisterQueueReplica(
    BSLS_ANNOTATION_UNUSED int   partitionId,
    BSLS_ANNOTATION_UNUSED const bmqt::Uri& uri,
    BSLS_ANNOTATION_UNUSED const mqbu::StorageKey& queueKey,
    BSLS_ANNOTATION_UNUSED const mqbu::StorageKey& appKey)
{
    // NOTHING
}

void StorageManager::updateQueueReplica(
    BSLS_ANNOTATION_UNUSED int   partitionId,
    BSLS_ANNOTATION_UNUSED const bmqt::Uri& uri,
    BSLS_ANNOTATION_UNUSED const mqbu::StorageKey& queueKey,
    BSLS_ANNOTATION_UNUSED const AppIdKeyPairs&    appIdKeyPairs,
    BSLS_ANNOTATION_UNUSED mqbi::Domain* domain,
    BSLS_ANNOTATION_UNUSED bool          allowDuplicate)
{
    // NOTHING
}

mqbu::StorageKey
StorageManager::generateAppKey(BSLS_ANNOTATION_UNUSED const bsl::string& appId,
                               BSLS_ANNOTATION_UNUSED int partitionId)
{
    return mqbu::StorageKey();
}

void StorageManager::setQueue(BSLS_ANNOTATION_UNUSED mqbi::Queue* queue,
                              BSLS_ANNOTATION_UNUSED const bmqt::Uri& uri,
                              BSLS_ANNOTATION_UNUSED int partitionId)
{
    // NOTHING
}

void StorageManager::setQueueRaw(BSLS_ANNOTATION_UNUSED mqbi::Queue* queue,
                                 BSLS_ANNOTATION_UNUSED const bmqt::Uri& uri,
                                 BSLS_ANNOTATION_UNUSED int partitionId)
{
    // NOTHING
}

void StorageManager::setPrimaryForPartition(
    BSLS_ANNOTATION_UNUSED int partitionId,
    BSLS_ANNOTATION_UNUSED mqbnet::ClusterNode* primaryNode,
    BSLS_ANNOTATION_UNUSED unsigned int         primaryLeaseId)
{
    // NOTHING
}

void StorageManager::clearPrimaryForPartition(
    BSLS_ANNOTATION_UNUSED int partitionId,
    BSLS_ANNOTATION_UNUSED mqbnet::ClusterNode* primary)
{
    // NOTHING
}

void StorageManager::setPrimaryStatusForPartition(
    BSLS_ANNOTATION_UNUSED int partitionId,
    BSLS_ANNOTATION_UNUSED bmqp_ctrlmsg::PrimaryStatus::Value value)
{
    // NOTHING
}

void StorageManager::processPrimaryStateRequest(
    BSLS_ANNOTATION_UNUSED const bmqp_ctrlmsg::ControlMessage& message,
    BSLS_ANNOTATION_UNUSED mqbnet::ClusterNode* source)
{
    // NOTHING
}

void StorageManager::processReplicaStateRequest(
    BSLS_ANNOTATION_UNUSED const bmqp_ctrlmsg::ControlMessage& message,
    BSLS_ANNOTATION_UNUSED mqbnet::ClusterNode* source)
{
    // NOTHING
}

void StorageManager::processReplicaDataRequest(
    BSLS_ANNOTATION_UNUSED const bmqp_ctrlmsg::ControlMessage& message,
    BSLS_ANNOTATION_UNUSED mqbnet::ClusterNode* source)
{
    // NOTHING
}

int StorageManager::makeStorage(
    BSLS_ANNOTATION_UNUSED bsl::ostream& errorDescription,
    BSLS_ANNOTATION_UNUSED bslma::ManagedPtr<mqbi::Storage>* out,
    BSLS_ANNOTATION_UNUSED const bmqt::Uri& uri,
    BSLS_ANNOTATION_UNUSED const mqbu::StorageKey& queueKey,
    BSLS_ANNOTATION_UNUSED int                     partitionId,
    BSLS_ANNOTATION_UNUSED const bsls::Types::Int64 messageTtl,
    BSLS_ANNOTATION_UNUSED const int                maxDeliveryAttempts,
    BSLS_ANNOTATION_UNUSED const mqbconfm::StorageDefinition& storageDef)
{
    return 0;
}

void StorageManager::processStorageEvent(
    BSLS_ANNOTATION_UNUSED const mqbi::DispatcherStorageEvent& event)
{
    // NOTHING
}

void StorageManager::processStorageSyncRequest(
    BSLS_ANNOTATION_UNUSED const bmqp_ctrlmsg::ControlMessage& message,
    BSLS_ANNOTATION_UNUSED mqbnet::ClusterNode* source)
{
    // NOTHING
}

void StorageManager::processPartitionSyncStateRequest(
    BSLS_ANNOTATION_UNUSED const bmqp_ctrlmsg::ControlMessage& message,
    BSLS_ANNOTATION_UNUSED mqbnet::ClusterNode* source)
{
    // NOTHING
}

void StorageManager::processPartitionSyncDataRequest(
    BSLS_ANNOTATION_UNUSED const bmqp_ctrlmsg::ControlMessage& message,
    BSLS_ANNOTATION_UNUSED mqbnet::ClusterNode* source)
{
    // NOTHING
}

void StorageManager::processPartitionSyncDataRequestStatus(
    BSLS_ANNOTATION_UNUSED const bmqp_ctrlmsg::ControlMessage& message,
    BSLS_ANNOTATION_UNUSED mqbnet::ClusterNode* source)
{
    // NOTHING
}

void StorageManager::processRecoveryEvent(
    BSLS_ANNOTATION_UNUSED const mqbi::DispatcherRecoveryEvent& event)
{
    // NOTHING
}

void StorageManager::processReceiptEvent(
    BSLS_ANNOTATION_UNUSED const mqbi::DispatcherReceiptEvent& event)
{
    // NOTHING
}

void StorageManager::processPrimaryStatusAdvisory(
    BSLS_ANNOTATION_UNUSED const bmqp_ctrlmsg::PrimaryStatusAdvisory& advisory,
    BSLS_ANNOTATION_UNUSED mqbnet::ClusterNode* source)
{
    // NOTHING
}

void StorageManager::processReplicaStatusAdvisory(
    BSLS_ANNOTATION_UNUSED int partitionId,
    BSLS_ANNOTATION_UNUSED mqbnet::ClusterNode* source,
    BSLS_ANNOTATION_UNUSED bmqp_ctrlmsg::NodeStatus::Value status)
{
    // NOTHING
}

void StorageManager::processShutdownEvent()
{
    // NOTHING
}

void StorageManager::applyForEachQueue(
    BSLS_ANNOTATION_UNUSED int                 partitionId,
    BSLS_ANNOTATION_UNUSED const QueueFunctor& functor) const
{
    // NOTHING
}

int StorageManager::processCommand(
    BSLS_ANNOTATION_UNUSED mqbcmd::StorageResult* result,
    BSLS_ANNOTATION_UNUSED const mqbcmd::StorageCommand& command)
{
    return 0;
}

void StorageManager::gcUnrecognizedDomainQueues()
{
    // NOTHING
}

// ACCESSORS
mqbi::Dispatcher::ProcessorHandle StorageManager::processorForPartition(
    BSLS_ANNOTATION_UNUSED int partitionId) const
{
    return mqbi::Dispatcher::k_INVALID_PROCESSOR_HANDLE;
}

bool StorageManager::isStorageEmpty(
    BSLS_ANNOTATION_UNUSED const bmqt::Uri& uri,
    BSLS_ANNOTATION_UNUSED int              partitionId) const
{
    return true;
}

const mqbs::FileStore&
StorageManager::fileStore(BSLS_ANNOTATION_UNUSED int partitionId) const
{
}

bslma::ManagedPtr<mqbi::StorageManagerIterator>
StorageManager::getIterator(BSLS_ANNOTATION_UNUSED int partitionId) const
{
    return bslma::ManagedPtr<mqbi::StorageManagerIterator>();
}

}  // close package namespace
}  // close enterprise namespace
