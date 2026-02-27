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
int StorageManager::start(BSLA_MAYBE_UNUSED bsl::ostream& errorDescription)
{
    return 0;
}

void StorageManager::stop()
{
    // NOTHING
}

void StorageManager::initializeQueueKeyInfoMap(
    BSLA_MAYBE_UNUSED const mqbc::ClusterState& clusterState)
{
}

void StorageManager::registerQueue(
    BSLA_MAYBE_UNUSED const bmqt::Uri& uri,
    BSLA_MAYBE_UNUSED const mqbu::StorageKey& queueKey,
    BSLA_MAYBE_UNUSED int                     partitionId,
    BSLA_MAYBE_UNUSED const AppInfos&         appIdKeyPairs,
    BSLA_MAYBE_UNUSED mqbi::Domain* domain)
{
    // NOTHING
}

void StorageManager::unregisterQueue(BSLA_MAYBE_UNUSED const bmqt::Uri& uri,
                                     BSLA_MAYBE_UNUSED int partitionId)
{
    // NOTHING
}

int StorageManager::updateQueuePrimary(
    BSLA_MAYBE_UNUSED const bmqt::Uri& uri,
    BSLA_MAYBE_UNUSED int              partitionId,
    BSLA_MAYBE_UNUSED const AppInfos&  addedIdKeyPairs,
    BSLA_MAYBE_UNUSED const AppInfos&  removedIdKeyPairs)
{
    return 0;
}

void StorageManager::registerQueueReplica(
    BSLA_MAYBE_UNUSED int   partitionId,
    BSLA_MAYBE_UNUSED const bmqt::Uri& uri,
    BSLA_MAYBE_UNUSED const mqbu::StorageKey& queueKey,
    BSLA_MAYBE_UNUSED const AppInfos&         appIdKeyPairs,
    BSLA_MAYBE_UNUSED mqbi::Domain* domain)
{
    // NOTHING
}

void StorageManager::unregisterQueueReplica(
    BSLA_MAYBE_UNUSED int   partitionId,
    BSLA_MAYBE_UNUSED const bmqt::Uri& uri,
    BSLA_MAYBE_UNUSED const mqbu::StorageKey& queueKey,
    BSLA_MAYBE_UNUSED const mqbu::StorageKey& appKey)
{
    // NOTHING
}

void StorageManager::updateQueueReplica(
    BSLA_MAYBE_UNUSED int   partitionId,
    BSLA_MAYBE_UNUSED const bmqt::Uri& uri,
    BSLA_MAYBE_UNUSED const mqbu::StorageKey& queueKey,
    BSLA_MAYBE_UNUSED const AppInfos&         appIdKeyPairs,
    BSLA_MAYBE_UNUSED mqbi::Domain* domain)
{
    // NOTHING
}

void StorageManager::resetQueue(
    BSLA_MAYBE_UNUSED const bmqt::Uri& uri,
    BSLA_MAYBE_UNUSED int              partitionId,
    BSLA_MAYBE_UNUSED const bsl::shared_ptr<mqbi::Queue>& queue_sp)
{
    // NOTHING
}

void StorageManager::setPrimaryForPartition(
    BSLA_MAYBE_UNUSED int partitionId,
    BSLA_MAYBE_UNUSED mqbnet::ClusterNode* primaryNode,
    BSLA_MAYBE_UNUSED unsigned int         primaryLeaseId)
{
    // NOTHING
}

void StorageManager::clearPrimaryForPartition(
    BSLA_MAYBE_UNUSED int partitionId,
    BSLA_MAYBE_UNUSED mqbnet::ClusterNode* primary)
{
    // NOTHING
}

void StorageManager::setPrimaryStatusForPartition(
    BSLA_MAYBE_UNUSED int partitionId,
    BSLA_MAYBE_UNUSED bmqp_ctrlmsg::PrimaryStatus::Value value)
{
    // NOTHING
}

void StorageManager::stopPFSMs()
{
    // NOTHING
}

void StorageManager::detectPrimaryLossInPFSM(BSLA_MAYBE_UNUSED int partitionId)
{
    // NOTHING
}

void StorageManager::detectSelfPrimaryInPFSM(
    BSLA_MAYBE_UNUSED int partitionId,
    BSLA_MAYBE_UNUSED mqbnet::ClusterNode* primaryNode,
    BSLA_MAYBE_UNUSED unsigned int         primaryLeaseId)
{
    // NOTHING
}

void StorageManager::detectSelfReplicaInPFSM(
    BSLA_MAYBE_UNUSED int partitionId,
    BSLA_MAYBE_UNUSED mqbnet::ClusterNode* primaryNode,
    BSLA_MAYBE_UNUSED unsigned int         primaryLeaseId)
{
    // NOTHING
}

void StorageManager::processPrimaryStateRequest(
    BSLA_MAYBE_UNUSED const bmqp_ctrlmsg::ControlMessage& message,
    BSLA_MAYBE_UNUSED mqbnet::ClusterNode* source)
{
    // NOTHING
}

void StorageManager::processReplicaStateRequest(
    BSLA_MAYBE_UNUSED const bmqp_ctrlmsg::ControlMessage& message,
    BSLA_MAYBE_UNUSED mqbnet::ClusterNode* source)
{
    // NOTHING
}

void StorageManager::processReplicaDataRequest(
    BSLA_MAYBE_UNUSED const bmqp_ctrlmsg::ControlMessage& message,
    BSLA_MAYBE_UNUSED mqbnet::ClusterNode* source)
{
    // NOTHING
}

int StorageManager::configureStorage(
    BSLA_MAYBE_UNUSED bsl::ostream& errorDescription,
    BSLA_MAYBE_UNUSED bsl::shared_ptr<mqbi::Storage>* out,
    BSLA_MAYBE_UNUSED const bmqt::Uri& uri,
    BSLA_MAYBE_UNUSED const mqbu::StorageKey& queueKey,
    BSLA_MAYBE_UNUSED int                     partitionId,
    BSLA_MAYBE_UNUSED const bsls::Types::Int64 messageTtl,
    BSLA_MAYBE_UNUSED const int                maxDeliveryAttempts,
    BSLA_MAYBE_UNUSED const mqbconfm::StorageDefinition& storageDef)
{
    return 0;
}

void StorageManager::processStorageEvent(
    BSLA_MAYBE_UNUSED const mqbevt::StorageEvent& event)
{
    // NOTHING
}

void StorageManager::processStorageSyncRequest(
    BSLA_MAYBE_UNUSED const bmqp_ctrlmsg::ControlMessage& message,
    BSLA_MAYBE_UNUSED mqbnet::ClusterNode* source)
{
    // NOTHING
}

void StorageManager::processPartitionSyncStateRequest(
    BSLA_MAYBE_UNUSED const bmqp_ctrlmsg::ControlMessage& message,
    BSLA_MAYBE_UNUSED mqbnet::ClusterNode* source)
{
    // NOTHING
}

void StorageManager::processPartitionSyncDataRequest(
    BSLA_MAYBE_UNUSED const bmqp_ctrlmsg::ControlMessage& message,
    BSLA_MAYBE_UNUSED mqbnet::ClusterNode* source)
{
    // NOTHING
}

void StorageManager::processPartitionSyncDataRequestStatus(
    BSLA_MAYBE_UNUSED const bmqp_ctrlmsg::ControlMessage& message,
    BSLA_MAYBE_UNUSED mqbnet::ClusterNode* source)
{
    // NOTHING
}

void StorageManager::processRecoveryEvent(
    BSLA_MAYBE_UNUSED const mqbevt::RecoveryEvent& event)
{
    // NOTHING
}

void StorageManager::processReceiptEvent(
    BSLA_MAYBE_UNUSED const bmqp::Event& event,
    BSLA_MAYBE_UNUSED mqbnet::ClusterNode* source)
{
    // NOTHING
}

void StorageManager::bufferPrimaryStatusAdvisory(
    BSLA_MAYBE_UNUSED const bmqp_ctrlmsg::PrimaryStatusAdvisory& advisory,
    BSLA_MAYBE_UNUSED mqbnet::ClusterNode* source)
{
    // NOTHING
}

void StorageManager::processPrimaryStatusAdvisory(
    BSLA_MAYBE_UNUSED const bmqp_ctrlmsg::PrimaryStatusAdvisory& advisory,
    BSLA_MAYBE_UNUSED mqbnet::ClusterNode* source)
{
    // NOTHING
}

void StorageManager::processReplicaStatusAdvisory(
    BSLA_MAYBE_UNUSED int partitionId,
    BSLA_MAYBE_UNUSED mqbnet::ClusterNode* source,
    BSLA_MAYBE_UNUSED bmqp_ctrlmsg::NodeStatus::Value status)
{
    // NOTHING
}

void StorageManager::processShutdownEvent()
{
    // NOTHING
}

void StorageManager::applyForEachQueue(
    BSLA_MAYBE_UNUSED int                 partitionId,
    BSLA_MAYBE_UNUSED const QueueFunctor& functor) const
{
    // NOTHING
}

int StorageManager::processCommand(
    BSLA_MAYBE_UNUSED mqbcmd::StorageResult* result,
    BSLA_MAYBE_UNUSED const mqbcmd::StorageCommand& command)
{
    return 0;
}

void StorageManager::gcUnrecognizedDomainQueues()
{
    // NOTHING
}

int StorageManager::purgeQueueOnDomain(
    BSLA_MAYBE_UNUSED mqbcmd::StorageResult* result,
    BSLA_MAYBE_UNUSED const bsl::string& domainName)
{
    return 0;
}

// ACCESSORS
mqbi::Dispatcher::ProcessorHandle
StorageManager::processorForPartition(BSLA_MAYBE_UNUSED int partitionId) const
{
    return mqbi::Dispatcher::k_INVALID_PROCESSOR_HANDLE;
}

bool StorageManager::isStorageEmpty(BSLA_MAYBE_UNUSED const bmqt::Uri& uri,
                                    BSLA_MAYBE_UNUSED int partitionId) const
{
    return true;
}

const mqbs::FileStore&
StorageManager::fileStore(BSLA_MAYBE_UNUSED int partitionId) const
{
    BSLS_ASSERT_INVOKE_NORETURN("Unimplemented");
}

bslma::ManagedPtr<mqbi::StorageManagerIterator>
StorageManager::getIterator(BSLA_MAYBE_UNUSED int partitionId) const
{
    return bslma::ManagedPtr<mqbi::StorageManagerIterator>();
}

}  // close package namespace
}  // close enterprise namespace
