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
#include <bsl_memory.h>
#include <bsl_ostream.h>
#include <bsla_annotations.h>

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
int StorageManager::start(BSLA_UNUSED bsl::ostream& errorDescription)
{
    return 0;
}

void StorageManager::stop()
{
    // NOTHING
}

void StorageManager::initializeQueueKeyInfoMap(
    BSLA_UNUSED const mqbc::ClusterState& clusterState)
{
}

void StorageManager::registerQueue(
    BSLA_UNUSED const bmqt::Uri& uri,
    BSLA_UNUSED const mqbu::StorageKey& queueKey,
    BSLA_UNUSED int                     partitionId,
    BSLA_UNUSED const AppInfos&         appIdKeyPairs,
    BSLA_UNUSED mqbi::Domain* domain)
{
    // NOTHING
}

void StorageManager::unregisterQueue(BSLA_UNUSED const bmqt::Uri& uri,
                                     BSLA_UNUSED int              partitionId)
{
    // NOTHING
}

int StorageManager::updateQueuePrimary(
    BSLA_UNUSED const bmqt::Uri& uri,
    BSLA_UNUSED const mqbu::StorageKey& queueKey,
    BSLA_UNUSED int                     partitionId,
    BSLA_UNUSED const AppInfos&         addedIdKeyPairs,
    BSLA_UNUSED const AppInfos&         removedIdKeyPairs)
{
    return 0;
}

void StorageManager::registerQueueReplica(
    BSLA_UNUSED int   partitionId,
    BSLA_UNUSED const bmqt::Uri& uri,
    BSLA_UNUSED const mqbu::StorageKey& queueKey,
    BSLA_UNUSED mqbi::Domain* domain,
    BSLA_UNUSED bool          allowDuplicate)
{
    // NOTHING
}

void StorageManager::unregisterQueueReplica(
    BSLA_UNUSED int   partitionId,
    BSLA_UNUSED const bmqt::Uri& uri,
    BSLA_UNUSED const mqbu::StorageKey& queueKey,
    BSLA_UNUSED const mqbu::StorageKey& appKey)
{
    // NOTHING
}

void StorageManager::updateQueueReplica(
    BSLA_UNUSED int   partitionId,
    BSLA_UNUSED const bmqt::Uri& uri,
    BSLA_UNUSED const mqbu::StorageKey& queueKey,
    BSLA_UNUSED const AppInfos&         appIdKeyPairs,
    BSLA_UNUSED mqbi::Domain* domain,
    BSLA_UNUSED bool          allowDuplicate)
{
    // NOTHING
}

void StorageManager::setQueue(BSLA_UNUSED mqbi::Queue* queue,
                              BSLA_UNUSED const bmqt::Uri& uri,
                              BSLA_UNUSED int              partitionId)
{
    // NOTHING
}

void StorageManager::setQueueRaw(BSLA_UNUSED mqbi::Queue* queue,
                                 BSLA_UNUSED const bmqt::Uri& uri,
                                 BSLA_UNUSED int              partitionId)
{
    // NOTHING
}

void StorageManager::setPrimaryForPartition(
    BSLA_UNUSED int partitionId,
    BSLA_UNUSED mqbnet::ClusterNode* primaryNode,
    BSLA_UNUSED unsigned int         primaryLeaseId)
{
    // NOTHING
}

void StorageManager::clearPrimaryForPartition(
    BSLA_UNUSED int partitionId,
    BSLA_UNUSED mqbnet::ClusterNode* primary)
{
    // NOTHING
}

void StorageManager::setPrimaryStatusForPartition(
    BSLA_UNUSED int partitionId,
    BSLA_UNUSED bmqp_ctrlmsg::PrimaryStatus::Value value)
{
    // NOTHING
}

void StorageManager::processPrimaryStateRequest(
    BSLA_UNUSED const bmqp_ctrlmsg::ControlMessage& message,
    BSLA_UNUSED mqbnet::ClusterNode* source)
{
    // NOTHING
}

void StorageManager::processReplicaStateRequest(
    BSLA_UNUSED const bmqp_ctrlmsg::ControlMessage& message,
    BSLA_UNUSED mqbnet::ClusterNode* source)
{
    // NOTHING
}

void StorageManager::processReplicaDataRequest(
    BSLA_UNUSED const bmqp_ctrlmsg::ControlMessage& message,
    BSLA_UNUSED mqbnet::ClusterNode* source)
{
    // NOTHING
}

int StorageManager::makeStorage(
    BSLA_UNUSED bsl::ostream& errorDescription,
    BSLA_UNUSED bsl::shared_ptr<mqbi::Storage>* out,
    BSLA_UNUSED const bmqt::Uri& uri,
    BSLA_UNUSED const mqbu::StorageKey& queueKey,
    BSLA_UNUSED int                     partitionId,
    BSLA_UNUSED const bsls::Types::Int64 messageTtl,
    BSLA_UNUSED const int                maxDeliveryAttempts,
    BSLA_UNUSED const mqbconfm::StorageDefinition& storageDef)
{
    return 0;
}

void StorageManager::processStorageEvent(
    BSLA_UNUSED const mqbi::DispatcherStorageEvent& event)
{
    // NOTHING
}

void StorageManager::processStorageSyncRequest(
    BSLA_UNUSED const bmqp_ctrlmsg::ControlMessage& message,
    BSLA_UNUSED mqbnet::ClusterNode* source)
{
    // NOTHING
}

void StorageManager::processPartitionSyncStateRequest(
    BSLA_UNUSED const bmqp_ctrlmsg::ControlMessage& message,
    BSLA_UNUSED mqbnet::ClusterNode* source)
{
    // NOTHING
}

void StorageManager::processPartitionSyncDataRequest(
    BSLA_UNUSED const bmqp_ctrlmsg::ControlMessage& message,
    BSLA_UNUSED mqbnet::ClusterNode* source)
{
    // NOTHING
}

void StorageManager::processPartitionSyncDataRequestStatus(
    BSLA_UNUSED const bmqp_ctrlmsg::ControlMessage& message,
    BSLA_UNUSED mqbnet::ClusterNode* source)
{
    // NOTHING
}

void StorageManager::processRecoveryEvent(
    BSLA_UNUSED const mqbi::DispatcherRecoveryEvent& event)
{
    // NOTHING
}

void StorageManager::processReceiptEvent(
    BSLA_UNUSED const bmqp::Event& event,
    BSLA_UNUSED mqbnet::ClusterNode* source)
{
    // NOTHING
}

void StorageManager::bufferPrimaryStatusAdvisory(
    BSLA_UNUSED const bmqp_ctrlmsg::PrimaryStatusAdvisory& advisory,
    BSLA_UNUSED mqbnet::ClusterNode* source)
{
    // NOTHING
}

void StorageManager::processPrimaryStatusAdvisory(
    BSLA_UNUSED const bmqp_ctrlmsg::PrimaryStatusAdvisory& advisory,
    BSLA_UNUSED mqbnet::ClusterNode* source)
{
    // NOTHING
}

void StorageManager::processReplicaStatusAdvisory(
    BSLA_UNUSED int partitionId,
    BSLA_UNUSED mqbnet::ClusterNode* source,
    BSLA_UNUSED bmqp_ctrlmsg::NodeStatus::Value status)
{
    // NOTHING
}

void StorageManager::processShutdownEvent()
{
    // NOTHING
}

void StorageManager::applyForEachQueue(
    BSLA_UNUSED int                 partitionId,
    BSLA_UNUSED const QueueFunctor& functor) const
{
    // NOTHING
}

int StorageManager::processCommand(
    BSLA_UNUSED mqbcmd::StorageResult* result,
    BSLA_UNUSED const mqbcmd::StorageCommand& command)
{
    return 0;
}

void StorageManager::gcUnrecognizedDomainQueues()
{
    // NOTHING
}

int StorageManager::purgeQueueOnDomain(
    BSLA_UNUSED mqbcmd::StorageResult* result,
    BSLA_UNUSED const bsl::string& domainName)
{
    return 0;
}

// ACCESSORS
mqbi::Dispatcher::ProcessorHandle
StorageManager::processorForPartition(BSLA_UNUSED int partitionId) const
{
    return mqbi::Dispatcher::k_INVALID_PROCESSOR_HANDLE;
}

bool StorageManager::isStorageEmpty(BSLA_UNUSED const bmqt::Uri& uri,
                                    BSLA_UNUSED int partitionId) const
{
    return true;
}

const mqbs::FileStore&
StorageManager::fileStore(BSLA_UNUSED int partitionId) const
{
    BSLS_ASSERT_INVOKE_NORETURN("Unimplemented");
}

bslma::ManagedPtr<mqbi::StorageManagerIterator>
StorageManager::getIterator(BSLA_UNUSED int partitionId) const
{
    return bslma::ManagedPtr<mqbi::StorageManagerIterator>();
}

}  // close package namespace
}  // close enterprise namespace
